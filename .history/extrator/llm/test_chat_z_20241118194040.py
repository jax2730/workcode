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
4. 提示用户输入外观颜色(具体的颜色)。
5. 若温度或湿度范围不符合要求，明确提示用户如何重新输入。
6. 如果用户输入了新的参数（如非温度、湿度、颜色的信息），立即提醒用户，并拒绝处理。
7. 用户确认无误后调用相关工具并传入提取到的信息。
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
    colors: list[str] = Field(default=[], description="星球外观颜色")


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
    if isinstance(messages[-1], AIMessage) and messages[-1].tool_calls:
        # if messages[-1].tool_calls:

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
    print("当前节点-----------add_tool_message----------------")
    print("state", state)
    print(state['messages'][-1])
    print(".tool_calls[-1].get('args')", state['messages'][-1]['tool_calls'])
    content = "环境设置完成"
    ## todo 生成场景数据
    return {
        "messages": [
            ToolMessage(
                content=content,
                tool_call_id=state["messages"][-1].tool_calls[0]["id"],
                submap_info = ""
            )
        ]
    }

# @workflow.add_node
# def recheck(state: State):

#     content = "修改部分内容"

#     return {
#         "messages": [
#             AIMessage(
#                 content=content,
#             )
#         ]
#     }

workflow.add_conditional_edges("info", get_state, ["add_tool_message", "info", END])
workflow.add_edge("add_tool_message", "info")
workflow.add_edge(START, "info")
graph = workflow.compile()
########################################################################

# from langchain_community.embeddings import HuggingFaceEmbeddings
from langchain_community.vectorstores import FAISS
from langchain_community.embeddings import OllamaEmbeddings
# from langchain.ollama import OllamaEmbeddings


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

def init():

    print("----------初始化----------")
    preload_1.parseBiomeLibrary("extrator/llm/data/testData")
    # 收集历史数据
    preload_1.parseHistoryFile("extrator/llm/data/xingqiushili")
    # 建立知识索引库 
    # 使用自定义嵌入
    # global vectorstore
    # embeddings = OllamaEmbeddings(model='nomic-embed-text')
    # vectorstore = FAISS.from_documents(get_biome_docs(), embeddings)
    # # 保存索引
    # vectorstore.save_local("biome_knowledge_index")


def chat(humanMsg:str, session_id:str):
    print("-----------------chat-----------")
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
        last_message = next(iter(output.values()))["messages"][-1]
        # last_message.pretty_print()
        if isinstance(last_message, AIMessage) and len(last_message.tool_calls) > 0:
            print("last_message.usage_metadata----------", last_message.tool_calls[-1])
            # if last_message.usage_metadata["submap_info"]:
            #     print("有不合法的地貌贴图！！！！！！！！！")
            args = last_message.tool_calls[-1].get('args')
            name = last_message.tool_calls[-1].get('name')

            print("name", name, "args", args)
        last_message.pretty_print()
    
    if name and args:
        # global vectorstore



        # colorAssets = preload_1.filterByColor(args["colors"])
        # subRange, have_idx0 = preload_1.createSubBiomeMap(args["temperature"]["min"],
        #                         args["temperature"]["max"],
        #                         args["humidity"]["min"],
        #                         args["humidity"]["max"],
        #                         colorAssets)
        
        if have_idx0:
        #    # 查询 FAISS 索引
        #     docs = vectorstore.similarity_search(args, k=5)  # 查询最相关的 5 条记录
            
        #     # 示例：格式化返回结果
        #     formatted_docs = [{"content": doc.page_content, "metadata": doc.metadata} for doc in docs]
        #     print("---------------formatted_docs-----------", formatted_docs)          # 构造提醒信息
        #     user_message = (
        #         "在生成数据时检测到部分区域地形不可用，建议更改颜色参数更加适配温湿度。\n"
        #         "以下是根据您输入的上下文推荐的改进建议：\n"
        #     )
        #     biome_names = [name for doc in formatted_docs for name in doc['metadata']['biome_name']]
        #     user_message += "推荐地形类型：\n" + ", ".join(set(biome_names)) + "\n"
        #     result["msg"] = output["info"]['messages'][-1].content + user_message

            result["data"] = {"have_idx0------", True}
            # result["submap_info"] = have_idx0
            print("result------2------have_idx0", result)
            return result

        print(colorAssets)
        result["data"] = subRange
        result["msg"] = output["info"]['messages'][-1].content
        print("result------1------", result)
        return result
    else:
        result["msg"] = output["info"]['messages'][-1].content
        print("result------2------", result)
        return result

def set_env(args):
        colorAssets = preload_1.filterByColor(args["colors"])
        subRange, have_idx0 = preload_1.createSubBiomeMap(args["temperature"]["min"],
                                args["temperature"]["max"],
                                args["humidity"]["min"],
                                args["humidity"]["max"],
                                colorAssets)
        

