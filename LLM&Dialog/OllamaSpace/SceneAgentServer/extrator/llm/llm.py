from . import preload
from langchain_ollama import ChatOllama
from langchain_core.messages import AIMessage, HumanMessage, ToolMessage, SystemMessage, trim_messages
from pydantic import BaseModel, Field
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
4. 提示用户输入外观颜色。
5. 提取完所有信息后，提示用户确认信息是否准确
6. 用户确认无误后调用相关工具并传入提取到的信息
7. 输出工具反馈,等待新的输入
调用工具参数说明：
{format_instraction}

**注意:**
1. 信息确认后不要忘记调用工具
2. 不要输出json数据
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
        history_cache[session_id] = SQLChatMessageHistory(session_id, connection="sqlite:///memory.db")
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

##########################
def info_chain(state, config):
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
    if isinstance(messages[-1], AIMessage) and messages[-1].tool_calls:
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
    last_message = state["messages"][-1]
    funcName = last_message.tool_calls[0]["name"]
    if funcName == "TemperatureRange":
        content = "温度设置完成"
    elif funcName == "HumidityRange":
        content = "湿度设置完成"
    else:
        content = "设置完成"
    ## todo 生成场景数据


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
        {"messages": [HumanMessage(content=humanMsg)]}, config={"configurable": {"session_id": session_id}}, stream_mode="updates"
    ):
        last_message = next(iter(output.values()))["messages"][-1]
        # last_message.pretty_print()
        if isinstance(last_message, AIMessage) and len(last_message.tool_calls) > 0:
            args = last_message.tool_calls[-1].get('args')
            name = last_message.tool_calls[-1].get('name')
        last_message.pretty_print()
    
    if name and args:
        colorAssets = preload.filterByColor(args["colors"])
        subRange = preload.createSubBiomeMap(args["temperature"]["min"],
                                args["temperature"]["max"],
                                args["humidity"]["min"],
                                args["humidity"]["max"],
                                colorAssets)
        print(colorAssets)
        result["data"] = subRange
        result["msg"] = output["info"]['messages'][-1].content
        return result
    else:
        result["msg"] = output["info"]['messages'][-1].content
        return result