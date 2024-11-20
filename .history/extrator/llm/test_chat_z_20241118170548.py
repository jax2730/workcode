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

# 定义星球环境参数模型
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
    temperature: TemperatureRange = Field(default=None, description="环境整体温度范围")
    humidity: HumidityRange = Field(default=None, description="环境整体湿度范围")
    colors: list[str] = Field(default=[], description="星球外观颜色")

argumentParser = PydanticOutputParser(pydantic_object=PlanetInfo)

# 缓存历史对话
history_cache = {}
def get_session_history(session_id):
    if session_id not in history_cache:
        history_cache[session_id] = SQLChatMessageHistory(session_id, connection="sqlite:///test_memo.db")
    return history_cache[session_id]

# 创建提取器
def create_extractor():
    llm = ChatOllama(temperature=0, model="qwen2.5")
    llm_with_tool = llm.bind_tools([PlanetInfo])
    
    trimmer = trim_messages(
        max_tokens=20,
        strategy="last",
        token_counter=len,
        include_system=True,
    )
    
    prompt_model = ChatPromptTemplate.from_messages(
        [
            ("system", "{template_tool_call}"),
            MessagesPlaceholder(variable_name="history"),
            MessagesPlaceholder(variable_name="new_history"),
        ]
    ) | trimmer | llm_with_tool
    
    extractor = RunnableWithMessageHistory(
        prompt_model,
        get_session_history,
        input_messages_key="new_history",
        history_messages_key="history",
    )
    return extractor

extractor = create_extractor()

# 核心对话处理逻辑
def info_chain(state, config):
    response = extractor.invoke(
        {
            "format_instraction": argumentParser.get_format_instructions(),
            "new_history": state["messages"],
        },
        config=config,
    )
    return {"messages": [response]}

# 状态图定义
class State(TypedDict):
    messages: Annotated[list, add_messages]

workflow = StateGraph(State)
workflow.add_node("info", info_chain)

@workflow.add_node
def add_tool_message(state: State):
    content = "环境设置完成"
    return {
        "messages": [
            ToolMessage(
                content=content,
                tool_call_id=state["messages"][-1].tool_calls[0]["id"],
            )
        ]
    }

workflow.add_conditional_edges("info", lambda state: "add_tool_message" if state.get("messages", [])[-1].tool_calls else "info", ["add_tool_message", "info", END])
workflow.add_edge("add_tool_message", "info")
workflow.add_edge(START, "info")
graph = workflow.compile()

# 知识向量库初始化
def get_biome_docs():
    biome_data_docs = []
    for record in preload_1.biomeData.get_all_records():
        temp_min, temp_max = record["temperature_range"]
        hum_min, hum_max = record["humidity_range"]
        biome_indices = record["indices"]
        biome_name = [
            name for name, index in preload_1.originBiomeMap.items()
            if index in biome_indices
        ]
        content = f"Temperature range: {temp_min} to {temp_max}°C, Humidity range: {hum_min} to {hum_max}%"
        biome_data_docs.append(Document(
            page_content=content,
            metadata={
                "temperature_range": (temp_min, temp_max),
                "humidity_range": (hum_min, hum_max),
                "biome_indices": biome_indices,
                "biome_name": biome_name,
            },
        ))
    return biome_data_docs

global vectorstore
def init():
    print("----------初始化----------")
    preload_1.parseBiomeLibrary("extrator/llm/data/testData")
    preload_1.parseHistoryFile("extrator/llm/data/xingqiushili")
    embeddings = OllamaEmbeddings(model='nomic-embed-text')
    global vectorstore
    vectorstore = FAISS.from_documents(get_biome_docs(), embeddings)
    vectorstore.save_local("biome_knowledge_index")

# 主对话函数
def chat(humanMsg: str, session_id: str):
    print("-----------------chat-----------")
    result = {"success": 1, "msg": "", "data": None}
    args = None
    name = None

    for output in graph.invoke(
        {"messages": [HumanMessage(content=humanMsg)]},
        config={"configurable": {"session_id": session_id}},
        stream_mode="updates",
    ):
        last_message = next(iter(output.values()))["messages"][-1]
        if isinstance(last_message, AIMessage) and len(last_message.tool_calls) > 0:
            args = last_message.tool_calls[-1].get("args")
            name = last_message.tool_calls[-1].get("name")
            print("name", name, "args", args)
        last_message.pretty_print()

    if name and args:
        colorAssets = preload_1.filterByColor(args["colors"])
        subRange, have_idx0 = preload_1.createSubBiomeMap(
            args["temperature"]["min"],
            args["temperature"]["max"],
            args["humidity"]["min"],
            args["humidity"]["max"],
            colorAssets,
        )
        
        if have_idx0:
            docs = vectorstore.similarity_search(args, k=5)
            formatted_docs = [{"content": doc.page_content, "metadata": doc.metadata} for doc in docs]
            biome_names = [name for doc in formatted_docs for name in doc["metadata"]["biome_name"]]
            result["msg"] = "部分区域地形不可用，推荐调整参数。\n推荐地形类型：" + ", ".join(set(biome_names))
            result["data"] = subRange
            return result
        
        result["data"] = subRange
        result["msg"] = "环境设置完成"
        return result

    result["msg"] = "未检测到有效参数"
    return result
