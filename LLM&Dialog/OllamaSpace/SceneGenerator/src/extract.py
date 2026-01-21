


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





resultParser = PydanticOutputParser(pydantic_object=SceneConfig)




# print(systemTemplate.format(format_instraction=resultParser.get_format_instructions()))

chat_history = InMemoryChatMessageHistory()
def get_session_history(session_id):
    return chat_history

def init():
    saveSceneTool = StructuredTool.from_function(
        func=saveScene,
        name="SaveScene",
        description="保存当前的环境信息",
        # response_format="content_and_artifact",
        args_schema=SceneConfig)

    tools = [saveSceneTool]

    model = ChatOllama(model="llama3.1", temperature=0)

    trimmer = trim_messages(
        max_tokens=1600,
        strategy="last",
        token_counter=model,
        include_system=True,
    )

    trimmer_model = trimmer | model

    
    # systemTemplate = """你是一个专业的信息提取助手。从用户输入中提取星球环境相关的信息：温度范围、湿度范围和星球外观颜色。
    # 如果用户提供的信息不全请简短提示他，确保所有信息完整，当所有信息提取完成后需要询问用户进行确认，用户确认后只输出json，此时除了json一个字也不要多说
    # {format_instraction}
    # """
    systemTemplate = """你是一个帮助用户创建星球的小助手，你需要收集一些关于星球环境描述的信息并汇总成json格式。这些信息包括：温度范围(-50到50之间为合法输入)、湿度范围(0到100之间为合法输入)、星球外观颜色、星球有哪些地形(沙地，森林，火山)。
    如果用户提供的信息不全请简短提示他,不需要告诉用户你的目的，确保所有信息完整，当所有信息提取完成后需要询问用户进行确认，用户确认后只输出json，此时除了json一个字也不要多说
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

    extractor = RunnableWithMessageHistory(
        prompt_model, 
        get_session_history,
        input_messages_key="input",
        history_messages_key="history")
    return extractor

def launch():
    extractor = init()
    while True:
        try:
            query = prompt("human: ")
        except EOFError:
            break
        if query == "quit":
            break
        if query.strip() == "":
            history = get_session_history("1")
            for messeage in history.messages:
                print(messeage)
            continue
    
        ai_msg = extractor.invoke(
            {
                "format_instraction": resultParser.get_format_instructions(), 
                "input": query
            },
            config={"configurable": {"session_id": "1"}}
        )
        try:
            configData = json.loads(ai_msg.content)
            return configData
            
        except Exception as e: 
            print("ai: " + ai_msg.content)
    
    return {}
