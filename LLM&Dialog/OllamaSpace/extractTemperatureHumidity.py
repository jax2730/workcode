


from prompt_toolkit import prompt
## llm
from langchain_openai import ChatOpenAI
from langchain_ollama import ChatOllama

from langchain_core.messages.system import SystemMessage
from langchain_core.messages import HumanMessage, trim_messages

from langchain_core.runnables.history import RunnableWithMessageHistory
from langchain_core.chat_history import InMemoryChatMessageHistory

from langchain_core.prompts import ChatPromptTemplate, MessagesPlaceholder

from langchain_core.output_parsers.string import StrOutputParser
from langchain_core.output_parsers import PydanticOutputParser

from langgraph.checkpoint.memory import MemorySaver
from langgraph.graph import START, MessagesState, StateGraph
from langchain_core.tools import StructuredTool


from pydantic import BaseModel, Field
class TemperatureRange(BaseModel):
    """温度范围"""
    min: int = Field(..., description="最低温度")
    max: int = Field(..., description="最高温度")

class HumidityRange(BaseModel):
    """湿度范围"""
    min: int = Field(..., description="最低湿度")
    max: int = Field(..., description="最高湿度")

class SceneConfig(BaseModel):
    """环境配置信息"""
    temperature: TemperatureRange = Field(default=None,description="环境整体温度范围")
    humidity: HumidityRange = Field(default=None,description="环境整体湿度范围")




# model = ChatOpenAI(
#     temperature=0.1,
#     model="GLM-4-Flash",
#     openai_api_key="f7b56c21c0caea2d0805161c059b3cdc.QbNr5x3g87Z8CuPC",
#     openai_api_base="https://open.bigmodel.cn/api/paas/v4/"
# )

model = ChatOllama(model="llama3.1", temperature=0)
# model = model.bind_tools(tools)

trimmer = trim_messages(
    max_tokens=1600,
    strategy="last",
    token_counter=model,
    include_system=True,
)

trimmer_model = trimmer | model

chat_history = InMemoryChatMessageHistory()
def get_session_history(session_id):
    return chat_history


# systemTemplate = """你是一个专业的信息提取助手。可以从用户提供的文本中提取环境相关的信息并调用合适的工具。最终的环境信息需要转化成json格式，例如:
# {{
#     "temperature": {{
#         "min": -20,
#         "max": 50
#     }},
#     "humidity": {{
#         "min": 0,
#         "max": 100
#     }}
# }}
# json各字段说明如下: {format_instraction}
# 请根据需要提取的信息向用户提问，帮助用户完善环境信息，积极询问未设置的值，如果用户输入与构建环境无关的问题，请提示他。
# 当所有信息提取完成后需要询问用户进行确认，用户确认后输只输出json，除此之外一个字也不要多说
#     """ #.format(format_instraction=SceneConfig.model_json_schema())

# systemTemplate = """你是一个专业的信息提取助手。可以从用户提供的文本中提取环境相关的信息并调用合适的工具。最终的环境信息需要转化成json格式，例如:
# {{
#     "temperature": {{
#         "min": -20,
#         "max": 50
#     }},
#     "humidity": {{
#         "min": 0,
#         "max": 100
#     }}
# }}
# json各字段说明如下: {format_instraction}
# 请根据需要提取的信息向用户提问，帮助用户完善环境信息，积极询问未设置的值，如果用户输入与构建环境无关的问题，请简短提示他。
# 当所有信息提取完成后需要询问用户进行确认，用户确认后输只输出json，除此之外一个字也不要多说
#     """ #.format(format_instraction=SceneConfig.model_json_schema())

systemTemplate = """你是一个专业的信息提取助手。可以从用户提供的文本中提取环境相关的信息并调用合适的工具。最终的环境信息需要转化成json格式，例如:
{{
    "temperature": {{
        "min": -20,
        "max": 50
    }},
    "humidity": {{
        "min": 0,
        "max": 100
    }}
}}
json各字段说明如下: {format_instraction}
请根据json字段中存在的未提取信息向用户提问，帮助用户完善环境信息，积极询问未设置的值，如果用户输入与构建环境无关或json中不存在信息，请简短提示他。
当所有信息提取完成后需要询问用户进行确认，用户确认后只输出json，除此之外一个字也不要多说
    """ #.format(format_instraction=SceneConfig.model_json_schema())


prompt_model = ChatPromptTemplate.from_messages(
    [
        (
            "system",
            systemTemplate,
        ),
        MessagesPlaceholder(variable_name="history"),
        ("human", "{input}"),
    ]
) | trimmer_model

runable_with_history = RunnableWithMessageHistory(
    prompt_model, 
    get_session_history,
    input_messages_key="input",
    history_messages_key="history")

# print(systemTemplate.format(format_instraction=SceneConfig.model_json_schema()))

resultParser = PydanticOutputParser(pydantic_object=SceneConfig)


print("你好，我可以帮助您创建一个星球，请描述以下星球的环境信息，比如温度和湿度范围")
while True:
    try:
        query = prompt()
    except EOFError:
        break
    if query == "quit":
        break
    if query.strip() == "":
        history = get_session_history("1")
        for messeage in history.messages:
            print(messeage)
        continue
   
    ai_msg = runable_with_history.invoke(
        {
            "format_instraction": SceneConfig.model_json_schema(), 
            "input": query
        },
        config={"configurable": {"session_id": "1"}}
    )


    try:
        print(resultParser.invoke(ai_msg))
        
        print("!!!Success!!!")
    except Exception as e:
        if "tool_calls" in ai_msg:
            print(ai_msg)
        else:
            print("AI: " + ai_msg.content)

    




