from . import preload_1
from . import preload_geo
from langchain_ollama import ChatOllama
from langchain_core.messages import AIMessage, HumanMessage, ToolMessage, SystemMessage, trim_messages
from pydantic import BaseModel, Field
from typing import List
from typing_extensions import TypedDict
from langchain_core.output_parsers import PydanticOutputParser
from langchain_community.chat_message_histories import SQLChatMessageHistory
from langchain_core.prompts import ChatPromptTemplate, MessagesPlaceholder
from langchain_core.runnables.history import RunnableWithMessageHistory
from langchain.docstore.document import Document



template_tool_call="""
**代理任务:**

请从用户输入中提取温度范围、湿度范围、外观颜色（中文颜色），协助用户设置星球的环境。如果用户选择设置地形类型，则调用专门的工具处理地形信息。请严格按照以下步骤执行，必须先确认再调用工具。

**用户交互步骤：**
1. 提示用户描述星球的环境：
   - **温度范围**（最小值-50°C，最大值50°C）。
   - **湿度范围**（（最小值0%，最大值100%）。
   - **外观颜色**（具体颜色，中文颜色）。
2. 提问用户是否需要设置地形类型（例如：山地、平原、沙漠等）。
3. 根据用户输入执行以下逻辑：
   - 若用户选择设置地形类型，提示输入具体类型，并调用设置地形工具。
   - 若用户不选择设置地形，则跳过地形相关流程。
4. 检查用户输入：
   - 温度或湿度范围超出规定范围时，明确提示用户如何重新输入。
   - 若用户输入了新参数（如非温度、湿度、颜色或地形类型的信息），立即提醒用户，并拒绝处理。
5. 用户确认无误后，依次调用工具并传入提取到的信息。
6. 输出工具反馈，等待用户新的输入。

**工具调用说明：**
1. **环境设置工具：** 用于处理温度、湿度、颜色。
2. **地形设置工具（可选）：** 用于处理用户提供的具体地形类型。

**注意:**
1. 专注于设定内的四个参数（温度、湿度、颜色、地形类型），及时提醒用户不可以添加其他参数。
2. 用户确认后，必须调用工具并逐一执行相关功能。
3. 不要输出JSON数据。
4. 若用户不选择地形类型，仅调用环境设置工具。
5. 不要因为用户输入而偏离主题。
"""
##########################
class TemperatureRange(BaseModel):
    """温度范围"""
    min: int = Field(..., description="最低温度")
    max: int = Field(..., description="最高温度")

class HumidityRange(BaseModel):
    """湿度范围"""
    min: int = Field(..., description="最低湿度")
    max: int = Field(..., description="最高湿度")

class Color(BaseModel):
    """颜色"""
    color: str = Field(..., description="颜色名称")

class TerrainType(BaseModel):
    '''地形类型'''
    terrain_name: str = Field(..., description="地形名称")

class PlanetInfo(BaseModel):
    """环境配置信息"""
    temperature: TemperatureRange = Field(default=None,description="环境整体温度范围")
    humidity: HumidityRange = Field(default=None,description="环境整体湿度范围")
    colors: list[str] = Field(default=[], description="星球外观颜色")
    terrain_name: list[str] = Field(default=[], description="星球具体地形名称")


argumentParser = PydanticOutputParser(pydantic_object=PlanetInfo)

##########################

##########################
history_cache = {}
def get_session_history(session_id):
    if session_id not in history_cache:
        history_cache[session_id] = SQLChatMessageHistory(session_id, connection="sqlite:///test_memo.db")
    return history_cache[session_id]

##########################
def createExtrator():
    llm = ChatOllama(
        temperature=0,
        model="qwen2.5"
    )
    llm_with_tool = llm.bind_tools([PlanetInfo])

    trimmer = trim_messages(
            max_tokens=20,
            strategy="last",
            token_counter=len,
            include_system=True,
        )

    prompt_model = ChatPromptTemplate.from_messages(
            [
                (
                    "system",
                    template_tool_call,
                ),
                MessagesPlaceholder(variable_name="history"),
                MessagesPlaceholder(variable_name="new_history"),
            ]
        ) | trimmer | llm_with_tool

    extractor = RunnableWithMessageHistory(
        prompt_model, 
        get_session_history,
        input_messages_key="new_history",
        history_messages_key="history")

    return extractor

extractor = createExtrator()




def info_chain(state, config):
    print("当前节点-----------info.config----------------")
    response = extractor.invoke(
            {
                "format_instraction": argumentParser.get_format_instructions(), 
                "new_history": state["messages"]
            },
            config=config
        )

    
    return {"messages": [response]}

###########################################################
     
from typing import Literal
from langgraph.graph import END


def get_state(state):
    messages = state["messages"]
    print("All messages: ", messages)  
    # for message in messages:
    #     if isinstance(message, ToolMessage):
    #         print("--------------------------------ToolMessage---------------------", message)
    #         if hasattr(message, 'submap_info'):
    #             print("submap_info:", message.submap_info)
    #             return "recheck"
    if isinstance(messages[-1], AIMessage) and messages[-1].tool_calls:
        print("-------------------确认---------------")
        return "add_tool_message"
    elif not isinstance(messages[-1], HumanMessage):
        return END
    return "info"


########################################################################




from langgraph.checkpoint.memory import MemorySaver
from langgraph.graph import StateGraph, START
from langgraph.graph.message import add_messages
from typing import Annotated

class State(TypedDict):
    messages: Annotated[list, add_messages]
    
workflow = StateGraph(State)
workflow.add_node("info", info_chain)

@workflow.add_node
def add_tool_message(state: State):
    tool_calls = state['messages'][-1].response_metadata.get('message', {}).get('tool_calls', [])
    print("当前节点-----------add_tool_message----------------")
    # 生成biomeMap
    tool_args = tool_calls[0].get('function', {}).get('arguments', {})
    subRange, have_idx0 = set_env(tool_args)
    print("have_idx0------------------", have_idx0)
    if have_idx0:
        # global vectorstore
        # # todo:搜索时只根据不合法区域的温湿度来检索
        # docs = vectorstore.similarity_search(tool_args, k=5)  # 查询最相关的 5 条记录
        
        # formatted_docs = [{"content": doc.page_content, "metadata": doc.metadata} for doc in docs]
        user_message = (
            "环境设置完成,但是在生成数据时部分区域地形不可用，建议更改颜色参数更加适配温湿度。\n"
            "以下是根据您输入的上下文推荐的改进建议：\n"
        )
        # biome_names = [name for doc in formatted_docs for name in doc['metadata']['biome_name']]
        # user_message += "推荐地形类型：\n" + ", ".join(set(biome_names)) + "\n" + "请确认是否要增加颜色,如果需要,输入新的颜色"
        return {
        "messages": [
            ToolMessage(
                content=user_message,
                tool_call_id=state["messages"][-1].tool_calls[0]["id"],
            )
            ]
        }

    content = "环境设置完成"

    return {
        "messages": [
            ToolMessage(
                content=content,
                tool_call_id=state["messages"][-1].tool_calls[0]["id"],
            )
        ]
    }



workflow.add_conditional_edges("info", get_state, ["add_tool_message", "info", END])
workflow.add_edge("add_tool_message", "info")
workflow.add_edge(START, "info")
graph = workflow.compile()
########################################################################

from langchain_community.vectorstores import FAISS
from langchain_community.embeddings import OllamaEmbeddings

# 构建知识向量库
def get_biome_docs():
    biome_data_docs = []
    for record in preload_1.biomeData.get_all_records():  # 假设 get_all_records 返回温湿度范围和索引
        temp_min, temp_max = record["temperature_range"]  # 获取温度范围
        hum_min, hum_max = record["humidity_range"]  # 获取湿度范围
        biome_indices = record["indices"]  # 生物群系索引列表
        biome_name = [
            name for name, index in preload_1.originBiomeMap.items() 
            if index in biome_indices  # 假设 biome_indices 是一个包含多个索引的列表
        ]

        # 构建文档内容
        content = (
            f"Temperature range: {temp_min} to {temp_max}°C, "
            f"Humidity range: {hum_min} to {hum_max}%"
        )
        
        # 创建 Document 对象并添加到列表
        biome_data_docs.append(Document(
            page_content=content,
            metadata={
                "temperature_range": (temp_min, temp_max),
                "humidity_range": (hum_min, hum_max),
                "biome_indices": biome_indices,
                "biome_name": biome_name
            }
        ))
    return biome_data_docs


########################################################################


########################################################################
global vectorstore
subRange = []
global biome_data_docs

def init():
    print("----------初始化----------")
    preload_1.parseBiomeLibrary("extrator/llm/data/testData")
    # 收集历史数据
    preload_1.parseHistoryFile("extrator/llm/data/xingqiushili")    
    preload_geo.init() 
    global subRange
    subRange = []
    global biome_data_docs
    biome_data_docs = get_biome_docs()


    # # 建立知识索引库 
    # global vectorstore
    # embeddings = OllamaEmbeddings(model='llama3.1')
    # vectorstore = FAISS.from_documents(biome_data_docs, embeddings)
    # # 保存索引
    # vectorstore.save_local("biome_knowledge_index")


def chat(humanMsg:str, session_id:str):
    # print("-----------------chat-----------")
    # 临界资源
    result = {
        "success": 1,
        "msg": "",
        "data": None,
        "stamp": None
    }
    output = None
    args = None
    name = None
    for output in graph.invoke(
        {"messages": [HumanMessage(content=humanMsg)]}, 
        config={"configurable": {"session_id": session_id}}, 
        stream_mode="updates"
    ):
        Messages = next(iter(output.values()))["messages"]
        last_message = Messages[-1]
        # last_message.pretty_print()
        if isinstance(last_message, AIMessage) and len(last_message.tool_calls) > 0:
            args = last_message.tool_calls[-1].get('args')
            name = last_message.tool_calls[-1].get('name')

        last_message.pretty_print()
    
    if name and args:
        # print("subRange-------",subRange)
        # 设置前三个参数
        copied_subRange = subRange.copy()  
        copied_subRange.pop("_last_operation", None)
        copied_subRange.pop("_has_repeated_idx", None)
        if subRange["_has_repeated_idx"] == False:
             subRange.clear()   
        result["data"] = copied_subRange
        result["msg"] = output["info"]['messages'][-1].content

        # 设置地形参数

        result["stamp"] = preload_geo.createTerrainStamp(args["terrain_name"])
        print("result------1------", result)
        return result
    else:
        result["msg"] = output["info"]['messages'][-1].content
        print("result------2------", result)
        return result

def set_env(args):
    global subRange
    colorAssets = preload_1.filterByColor(args["colors"])
    if len(subRange)>0:
        subRange, have_idx0 = mod_subMap(colorAssets,subRange)
        subRange["_last_operation"] = "mod_subMap" 
    else:
        subRange, have_idx0 = preload_1.createSubBiomeMap(args["temperature"]["min"],
                                args["temperature"]["max"],
                                args["humidity"]["min"],
                                args["humidity"]["max"],
                                colorAssets)
        subRange["_last_operation"] = "createSubBiomeMap" 
        
    subRange["_has_repeated_idx"] = have_idx0
    return subRange, have_idx0  

def mod_subMap(colorAssets, subRange):
    seen_ids = set()
    unused_colors = set(colorAssets)
    have_idx0 = False
    for subrange in subRange['subRanges']:
        if subrange["idx"] in seen_ids:
            subrange["idx"] = -1
        else:
            seen_ids.add(subrange["idx"])
    for name, index in preload_1.originBiomeMap.items():
        if index in seen_ids:
            # 如果索引在 seen_ids 中，从 unused_colors 中移除该名称
            unused_colors.discard(name)

    for subrange in subRange['subRanges']:
        if subrange["idx"] == -1:
            if len(unused_colors)>0:
                selected_color = unused_colors.pop()
                # print("还有可用颜色", selected_color)
                if selected_color in preload_1.originBiomeMap:
                    selected_idx = preload_1.originBiomeMap[selected_color]
                    subrange["idx"] = selected_idx
                    # print(f"Color '{selected_color}'对应的索引号是: {selected_idx}")
                    break
            else:
                # print("没有可用的颜色了，colorAssets：",colorAssets)
                have_idx0 = True
                break

    return subRange, have_idx0  