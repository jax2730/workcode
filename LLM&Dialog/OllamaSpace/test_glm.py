from langchain_openai import ChatOpenAI

from langchain_core.messages.system import SystemMessage
from langchain_core.messages import HumanMessage

from langgraph.checkpoint.memory import MemorySaver
from langgraph.graph import START, MessagesState, StateGraph
from langchain_core.tools import StructuredTool
from langchain_ollama import ChatOllama

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
    """场景配置信息"""
    temperature: TemperatureRange = Field(default=None,description="场景整体温度范围")
    humidity: HumidityRange = Field(default=None,description="场景整体湿度范围")


# def saveScene(sceneConfig: SceneConfig):
#     print("sceneConfig ->{0}".format(sceneConfig))
    
#     return "场景保存成功"

def saveScene(temperature: TemperatureRange, humidity: HumidityRange):
    # print("sceneConfig ->{0}".format(sceneConfig))
    
    return "场景保存成功"

saveSceneTool = StructuredTool.from_function(
    func=saveScene,
    name="SaveScene",
    description="保存当前的场景配置信息",
    # response_format="content_and_artifact",
    args_schema=SceneConfig)

tools = [saveSceneTool]


workflow = StateGraph(state_schema=MessagesState)

# model = ChatOpenAI(
#     temperature=0.1,
#     model="GLM-4-Flash",
#     openai_api_key="f7b56c21c0caea2d0805161c059b3cdc.QbNr5x3g87Z8CuPC",
#     openai_api_base="https://open.bigmodel.cn/api/paas/v4/"
# )

model = ChatOllama(model="llama3.1", temperature=0.9)
# model = model.bind_tools(tools)

def call_model(state: MessagesState):
    # system_prompt = (
    #     "You are a helpful assistant. "
    #     "Answer all questions to the best of your ability."
    # )
#     systemTemplate = """你是一个专业的信息提取助手。可以从用户提供的文本中提取场景相关的信息并调用合适的工具。最终的场景信息需要转化成json格式，例如:
#     {{
#         "temperature": {{
#             "min": -20,
#             "max": 50
#         }},
#         "humidity": {{
#             "min": 0,
#             "max": 100
#         }}
#     }}
# json各字段说明如下: {format_instraction}
# 如果信息中没有包含场景信息则不需要调用工具，请根据需要提取的信息向用户提问，帮助用户完善场景信息，积极询问未设置的值，如果用户输入与构建场景无关的问题，请提示他。
# """.format(format_instraction=SceneConfig.model_json_schema())
    systemTemplate = """你是一个专业的信息提取助手。可以从用户提供的文本中提取场景相关的信息并调用合适的工具。最终的场景信息需要转化成json格式，例如:
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
    请根据需要提取的信息向用户提问，帮助用户完善场景信息，积极询问未设置的值，如果用户输入与构建场景无关的问题，请提示他。当所有信息提取完成后需要询问用户进行确认，用户同意后请调用保存工具保存当前的场景信息
    """.format(format_instraction=SceneConfig.model_json_schema())
    print(systemTemplate)
    messages = [SystemMessage(content=systemTemplate)] + state["messages"]
    response = model.invoke(messages)
    return {"messages": response}


# Define the node and edge
workflow.add_node("model", call_model)
workflow.add_edge(START, "model")



memory = MemorySaver()
app = workflow.compile(checkpointer=memory)



while True:
    try:
        query = input()
    except EOFError:
        break
    if query == "quit":
        break
    if query.strip() == "":
        continue
   
    ai_msg = app.invoke(
        {"messages": [HumanMessage(content=query)]},
        config={"configurable": {"thread_id": "1"}},
    )

    if 'messages' in ai_msg:
        chat_history = ai_msg['messages']
        latest_response = chat_history[-1]
        print(latest_response)
        print("---------------------")
    else:
        print("======================")
        print(ai_msg)
        print("=======================")

