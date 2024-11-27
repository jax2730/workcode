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

协助用户设置星球环境和地形，严格按照以下步骤提取参数并调用相应工具。

**用户交互步骤：**
1. 询问环境参数：
   - 温度范围（-50°C 至 50°C）。
   - 湿度范围（0% 至 100%）。
   - 外观颜色（中文颜色）。
2. 询问地形设置需求：
   - 是否需要设置地形类型（如山地、平原、沙漠等）。
3. 输入处理逻辑：
   - 若用户输入完整环境参数，提示确认后**调用环境设置工具**。
   - 若用户选择设置地形，提示输入以下信息并**调用地形设置工具**：
     - 地形类型（中文名称）。
     - 纬度（一个浮点数）。
     - 经度（一个浮点数）。
   - 同时包含环境和地形信息时，分别调用两个工具。
   - 若用户输入超出规定范围的值，明确提示并要求重新输入。
   - 输入无关信息时，提醒用户并拒绝处理。
4. 用户确认后执行：
   - 调用工具并传入提取到的信息。
   - 输出简洁的工具反馈，等待用户新的输入。

**工具调用规则：**
1. 必须确保参数齐全再调用相关工具
2. 环境设置工具：处理温度、湿度、颜色。
3. 地形设置工具：处理地形类型及其经纬度，**整合所有的地形数据**再进行调用。
4. 两工具可独立调用，但需根据用户输入分别执行。
5. 若用户未提及地形，仅调用环境设置工具。

**注意事项:**
1. 专注设定的六个参数：温度、湿度、颜色、地形类型、纬度、经度。
2. 用户确认后，必须要自动调用工具。
3. 不输出 JSON 数据或代码。
4. 确保响应主题明确，不偏离任务目标。
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


class LatitudeValue(BaseModel):
    """经度"""
    latitude: float = Field(..., alias="lat", description="经度")

class LongitudeValue(BaseModel):
    """纬度"""
    longitude: float = Field(..., alias="lon", description="纬度")

class TerrainType(BaseModel):
    '''地形类型'''
    terrain: str = Field(..., alias="terrain", description="地形名称")

class PlanetInfo(BaseModel):
    """环境配置信息"""
    temperature: TemperatureRange = Field(default=None,description="环境整体温度范围")
    humidity: HumidityRange = Field(default=None,description="环境整体湿度范围")
    colors: list[str] = Field(default=[], description="星球外观颜色")

class TerrainItem(BaseModel):
    """地形配置信息"""
    type: TerrainType = Field(default=[], alias="type", description="星球具体地形名称")
    latitude: LatitudeValue= Field(default=None, alias="latitude", description="地形所在经度")
    longitude: LongitudeValue= Field(default=None, alias="longitude", description="地形所在纬度")

class TerrainInfo(BaseModel):
    """多种地形信息"""
    terrains: list[TerrainItem] = Field(default=[], alias="terrains",  description="地形配置列表，每个元素包含地形名称和经纬度的值")

##########################

argumentParser = PydanticOutputParser(pydantic_object=PlanetInfo)
terrainParser = PydanticOutputParser(pydantic_object=TerrainInfo)


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
    llm_with_tool = llm.bind_tools([PlanetInfo, TerrainInfo])

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
        history_messages_key="history",
        post_processors=[
            lambda response: argumentParser.parse(response.content),  # 自动解析 PlanetInfo
            lambda response: terrainParser.parse(response.content)   # 自动解析 TerrainInfo
        ])

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
    
    print("response", response)

    
    return {"messages": [response]}

###########################################################
     
from typing import Literal
from langgraph.graph import END


def get_state(state):
    messages = state["messages"]
    print("All messages: ", messages)  
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
    tool_name = tool_calls[0].get('function', {}).get('name', {})
    tool_args = tool_calls[0].get('function', {}).get('arguments', {})


    if tool_name == "TerrainInfo":
        content = "星球子地形环境设置完成"

        return {
            "messages": [
                ToolMessage(
                    content=content,
                    tool_call_id=state["messages"][-1].tool_calls[0]["id"],
                )
            ]
        }
    elif tool_name == "PlanetInfo":
    
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

        content = "星球外观环境设置完成"

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


##############################全局参数###################################

global vectorstore
subRange = []
global biome_data_docs
stamp_data_sheet = []
########################################################################


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
    print("name", name, "args", args)
    if name and args:
        if name == "TerrainInfo":
            for entry in args["terrains"]:
                # 统一字段名为 'type'
                if 'terrain' in entry:
                    entry['type'] = entry.pop('terrain')
            result["stamp"] = preload_geo.createTerrainStamp(args["terrains"])
            # print("output", output)

        if name == "PlanetInfo":
            copied_subRange = subRange.copy()  
            copied_subRange.pop("_last_operation", None)
            copied_subRange.pop("_has_repeated_idx", None)
            if subRange["_has_repeated_idx"] == False:
                subRange.clear()   
            result["data"] = copied_subRange
        result["msg"] = output["info"]['messages'][-1].content
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
            # subrange["idx"] = -1
            None
        else:
            seen_ids.add(subrange["idx"])
    for name, index in preload_1.originBiomeMap.items():
        if index in seen_ids:
            unused_colors.discard(name)
    limit = 16-len(seen_ids)
    temp = 0
    for subrange in subRange['subRanges']:
        if subrange["idx"] in seen_ids and temp<limit:
            temp += 1
            if len(unused_colors)>0:
                selected_color = unused_colors.pop()
                # print("还有可用颜色", selected_color)
                if selected_color in preload_1.originBiomeMap:
                    selected_idx = preload_1.originBiomeMap[selected_color]
                    subrange["idx"] = selected_idx
                    print(f"Color '{selected_color}'对应的索引号是: {selected_idx}")
            else:
                have_idx0 = True
                break

    return subRange, have_idx0  