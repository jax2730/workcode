from . import preload
from langchain_ollama import ChatOllama
from langchain_core.messages import AIMessage, HumanMessage, ToolMessage, SystemMessage, trim_messages
from pydantic import BaseModel, Field, ValidationError
from typing import List
from typing_extensions import TypedDict
from langchain_core.output_parsers import PydanticOutputParser
from langchain_community.chat_message_histories import SQLChatMessageHistory
from langchain_core.prompts import ChatPromptTemplate, MessagesPlaceholder
from langchain_core.runnables.history import RunnableWithMessageHistory


template_tool_call="""
**代理任务:**

请从用户输入中提取温度范围、湿度范围和外观颜色(中文颜色)，协助用户设置星球的环境。请严格按照以下步骤执行,必须先确认再调用工具

**用户交互步骤：**
1. 提示用户描述星球的环境。
2. 提取用户输入的温度范围（-50至50°C）。
3. 提取湿度范围（0 ~ 100%）的用户输入。
4. 提示用户输入外观颜色(具体的颜色)。
5. 若温度或湿度范围不符合要求，明确提示用户如何重新输入。
6. 用户确认无误后调用相关工具并传入提取到的信息。
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
            max_tokens=200,
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
    print("---------------当前节点-----------------: info")
    print("---------------state-----------------: ", state)
    parse_fresult = argumentParser.get_format_instructions()
    response = extractor.invoke(
            {
                "format_instraction": parse_fresult, 
                "new_history": state["messages"]
            },
            config=config
        )
    print("parse_fresult====================",parse_fresult)
    return {"messages": [response]}


# def info_chain(state, config):
#     print("---------------当前节点-----------------: info")
#     print("---------------state-----------------: ", state)

#     # 调用 extractor 获取输入数据
#     response = extractor.invoke(
#         {
#             "format_instraction": argumentParser.get_format_instructions(),
#             "new_history": state["messages"]
#         },
#         config=config
#     )

#     # 假设 response 中包含用户输入的数据
#     user_input = response.get("data", {})

#     # 使用 PydanticOutputParser 解析用户输入
#     try:
#         parsed_data = argumentParser.parse(user_input)
#     except ValidationError as e:
#         # 如果验证失败，返回错误信息
#         error_message = f"参数验证失败: {str(e)}"
#         return {"messages": [{"content": error_message}]}

#     # 检查是否所有必填字段都完整
#     missing_params = []
#     if not parsed_data.temperature:
#         missing_params.append("温度范围")
#     if not parsed_data.humidity:
#         missing_params.append("湿度范围")
#     if not parsed_data.colors:
#         missing_params.append("颜色信息")

#     if missing_params:
#         missing_message = "缺少以下参数: " + ", ".join(missing_params) + "，请补充完整。"
#         return {"messages": [{"content": missing_message}]}

#     # 如果所有字段完整，返回确认信息或者执行后续操作
#     return {"messages": [{"content": "所有参数已填写完整，准备调用工具进行处理。"}]}

###########################################################
     
from typing import Literal
from langgraph.graph import END


def get_state(state):
    messages = state["messages"]
    print("All messages: ", messages) 
    if isinstance(messages[-1], AIMessage) and messages[-1].tool_calls:
    # if messages[0].content == "确认" and isinstance(messages[-1], AIMessage) and messages[-1].tool_calls:
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
    print("------------------------当前节点-------------: add_tool_message")

    ## todo 生成场景数据
    return set_env(state)

workflow.add_conditional_edges("info", get_state, ["add_tool_message", "info", END])
workflow.add_edge("add_tool_message", "info")
workflow.add_edge(START, "info")
graph = workflow.compile()
########################################################################

def init():
    preload.parseBiomeLibrary("extrator/llm/data/testData")
    # 收集历史数据
    preload.parseHistoryFile("extrator/llm/data/xingqiushili")

def chat(humanMsg:str, session_id:str):
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
            args = last_message.tool_calls[-1].get('args')
            name = last_message.tool_calls[-1].get('name')

            print("name", name, "args", args)
        last_message.pretty_print()
    
    if name and args:
        result["msg"] = output["info"]['messages'][-1].content
        print("result------1------", result)
        return result
    else:
        result["msg"] = output["info"]['messages'][-1].content
        print("result------2------", result)
        return result

def set_env(state: State):
    # 提取消息中的工具调用参数
    args = state["messages"][-1].tool_calls[-1].get("args")  # 获取工具调用参数
    
    # 如果需要在这里处理工具调用，可以通过 args 调用工具
    if args:
        colorAssets = preload.filterByColor(args["colors"])
        subRange = preload.createSubBiomeMap(args["temperature"]["min"],
                                            args["temperature"]["max"],
                                            args["humidity"]["min"],
                                            args["humidity"]["max"],
                                            colorAssets)
        # print("=================colorAssets==================", colorAssets)
        
        # 设置返回消息
        content = "环境设置完成"
        return {
            "messages": [
                ToolMessage(
                    content=content,
                    tool_call_id=state["messages"][-1].tool_calls[0]["id"],  # 使用工具调用ID
                )
            ]
        }
    else:
        # 如果没有工具调用参数，则返回普通消息
        content = "没有工具调用参数"
        return {
            "messages": [
                ToolMessage(
                    content=content,
                )
            ]
        }