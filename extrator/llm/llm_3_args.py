import copy
from . import preload_1
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

请从用户输入中提取温度范围、湿度范围和外观颜色(中文颜色)，协助用户设置星球的环境。请严格按照以下步骤执行,必须先确认再调用工具

**用户交互步骤：**
1. 提示用户描述星球的环境。
2. 提取用户输入的温度范围（-50至50°C）。
3. 提取湿度范围（0 ~ 100%）的用户输入。
4. 提示用户输入外观颜色(具体的颜色，可以是多个，但不能为空)。
5. 温度和湿度范围必须严格按照规则！若温度或湿度范围不符合要求，明确提示用户重新输入，这很重要。
6. 如果用户输入了新的参数（如非温度、湿度、颜色的信息），立即提醒用户，并拒绝处理。
7. 三个参数都获取到了,并且用户确认无误后再调用相关工具并传入提取到的信息。
8. 输出工具反馈，等待新的输入。
调用工具参数说明：
{format_instraction}

**注意:**
1. 专注于设定内的三个参数，及时提醒用户不可以添加星球环境的新参数
2. 信息确认后不要忘记调用工具
3. 不要输出json数据
4. 你的作用是从用户获取星球环境信息(温度、湿度、颜色)，不要因为用户输入而偏离主题
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


class PlanetInfo(BaseModel):
    """环境配置信息"""
    temperature: TemperatureRange = Field(default=None,description="环境整体温度范围")
    humidity: HumidityRange = Field(default=None,description="环境整体湿度范围")
    colors: list[str] = Field(default=['灰色'], description="星球外观颜色")


argumentParser = PydanticOutputParser(pydantic_object=PlanetInfo)

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
        global vectorstore
        docs = vectorstore.similarity_search(tool_args, k=3)  # 查询最相关的 3 条记录
        
        formatted_docs = [{"content": doc.page_content, "metadata": doc.metadata} for doc in docs]
        print("---------------formatted_docs-----------", formatted_docs)          # 构造提醒信息
        user_message = (
            "环境设置完成,但是在生成数据时部分区域地形不可用，建议更改颜色参数更加适配温湿度。\n"
            "以下是根据您输入的上下文推荐的改进建议：\n"
        )
        biome_names = [name for doc in formatted_docs for name in doc['metadata']['biome_name']]
        user_message += "推荐地形类型：\n" + ", ".join(set(biome_names)) + "\n" + "请确认是否要增加颜色,如果需要,输入新的颜色（可以重复添加一种颜色）"
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
    global subRange
    subRange = []
    global biome_data_docs
    biome_data_docs = get_biome_docs()


    # 建立知识索引库 
    global vectorstore
    embeddings = OllamaEmbeddings(model='nomic-embed-text')
    # 如果实例数据有变化 要重新生成一下
    # vectorstore = FAISS.from_documents(biome_data_docs, embeddings)
    # vectorstore.save_local("biome_knowledge_index")

    vectorstore = FAISS.load_local(
        "biome_knowledge_index",
        embeddings,
        allow_dangerous_deserialization=True
    )

def chat(humanMsg:str, session_id:str):
    # print("-----------------chat-----------")
    # 临界资源
    result = {
        "success": 1,
        "msg": "",
        "data": None 
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
        # global subRange
        copied_subRange = copy.deepcopy(subRange)  
        copied_subRange.pop("_last_operation", None)
        copied_subRange.pop("_has_repeated_idx", None)

        if subRange.get("_has_repeated_idx", None) == False:
            subRange.clear()   
        # if subRange["_last_operation"] == "createSubBiomeMap":
        #     subRange.clear()   
        # elif subRange["_last_operation"] == "mod_subMap" and subRange["_has_repeated_idx"] == False :
        #     subRange.clear()    
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
