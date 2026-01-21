


from prompt_toolkit import prompt
## llm
from langchain_ollama import ChatOllama

from langchain_core.messages.system import SystemMessage
from langchain_core.messages import HumanMessage, trim_messages

from langchain_core.runnables.history import RunnableWithMessageHistory
from langchain_core.chat_history import InMemoryChatMessageHistory

from langchain_core.prompts import ChatPromptTemplate, MessagesPlaceholder

from langchain_core.output_parsers import PydanticOutputParser

from langchain_core.tools import StructuredTool

from pydantic import BaseModel, Field

import json

import testRAG as Util

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

class SceneConfig(BaseModel):
    """环境配置信息"""
    temperature: TemperatureRange = Field(default=None,description="环境整体温度范围")
    humidity: HumidityRange = Field(default=None,description="环境整体湿度范围")
    colors: list[str] = Field(default=[], description="星球外观颜色")


def saveScene(temperature: TemperatureRange, humidity: HumidityRange, colors: list[str]):
    # print("sceneConfig ->{0}".format(sceneConfig))
    print("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!")
    return "场景保存成功"

saveSceneTool = StructuredTool.from_function(
    func=saveScene,
    name="SaveScene",
    description="保存当前的环境信息",
    # response_format="content_and_artifact",
    args_schema=SceneConfig)

tools = [saveSceneTool]

model = ChatOllama(model="llama3.1", temperature=0)
# structured_llm = model.with_structured_output(SceneConfig)
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


# systemTemplate = """你是一个专业的信息提取助手。从用户输入中提取星球环境相关的信息：温度范围、湿度范围和星球外观颜色，如果用户提供的信息不包含需要的内容请简短提示他，确保所有信息完整，当需要的环境信息全部提取完成后,需要用户确认。当用户确认完成后，需要把提取到的环境信息转化成json格式后输出,除此之外一个字也不要多说。
#  {format_instraction}
# """
systemTemplate = """你是一个专业的信息提取助手。从用户输入中提取星球环境相关的信息：温度范围、湿度范围和星球外观颜色。
如果用户提供的信息不全请简短提示他，确保所有信息完整，当所有信息提取完成后需要询问用户进行确认，用户确认后只输出json，此时除了json一个字也不要多说
 {format_instraction}
"""


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


resultParser = PydanticOutputParser(pydantic_object=SceneConfig)


print(systemTemplate.format(format_instraction=resultParser.get_format_instructions()))

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
            "format_instraction": resultParser.get_format_instructions(), 
            "input": query
        },
        config={"configurable": {"session_id": "1"}}
    )
    try:
        configData = json.loads(ai_msg.content)
        print(configData)
        colorStr = ",".join(configData["colors"])
        
        resRetriver = Util.generateRetriver(model)
        print(resRetriver.invoke({"question": colorStr}))
        
    except Exception as e: 
        print("ai: " + ai_msg.content)

