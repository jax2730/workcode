from langchain_core.prompts import ChatPromptTemplate
from langchain_core.output_parsers import StrOutputParser
from langchain_ollama import ChatOllama
from prompt_toolkit import prompt
from typing import List
from pydantic import BaseModel, Field

class Resources(BaseModel):
    """返回给用户的资源列表"""
    res: List[str] = Field(description="查询到的资源列表")

# RAG_TEMPLATE = """
# 你是一个提取和总结信息的助手。使用以下上下文信息回答问题。如果你不知道，就直接说不知道
# <context>
# 石头或石头子类资源:"红色石头001","红色stone石头002","红色stone石头003","红色stone石头004","红色stone石头005","红色stone石头006","红色stone石头007","苔原stone石头001","苔原stone石头002","苔原stone石头003","砾石stone石头004","砾石stone石头002","沥青stone石头006","砾石stone石头005","砾石stone石头006","硅基晶石stone石头011","沼泽/沥青stone石头007","沼泽/沥青stone石头008"
# 沙子或沙场类资源:"红色sand沙子001","红色sand沙子002","千岛sand沙子001","千岛soil沙子002"
# 苔原或苔藓类资源:"苔原moss苔原001","苔原moss苔原002","苔原stone石头001","苔原stone石头002","苔原stone石头003"
# 土地、土、田地或土壤类资源": "红色soil土壤002","红色soil土壤004","凝胶soil土壤001","凝胶soil土壤002","凝胶soil土壤003","凝胶soil土壤004","千岛soil土壤004","千岛soil土壤003","千岛soil土壤005","千岛soil土壤006","沥青soil土壤001"
# 凝胶、冰冻或冻土类资源: "凝胶ningjiaoo凝胶001","凝胶002"
# </context>

# Answer the following question:

# {question}"""

RAG_TEMPLATE = """
你是一个提取和总结信息的助手。使用以下上下文信息,根据用户问题提取相关资源并返回给用户,除此之外不要多说一个字。
<context>
石头或石头子类资源:"红色石头001","红色stone石头002","红色stone石头003","红色stone石头004","红色stone石头005","红色stone石头006","红色stone石头007","苔原stone石头001","苔原stone石头002","苔原stone石头003","砾石stone石头004","砾石stone石头002","沥青stone石头006","砾石stone石头005","砾石stone石头006","硅基晶石stone石头011","沼泽/沥青stone石头007","沼泽/沥青stone石头008"
沙子或沙场类资源:"红色sand沙子001","红色sand沙子002","千岛sand沙子001","千岛soil沙子002"
苔原或苔藓类资源:"苔原moss苔原001","苔原moss苔原002","苔原stone石头001","苔原stone石头002","苔原stone石头003"
土地、土、田地或土壤类资源": "红色soil土壤002","红色soil土壤004","凝胶soil土壤001","凝胶soil土壤002","凝胶soil土壤003","凝胶soil土壤004","千岛soil土壤004","千岛soil土壤003","千岛soil土壤005","千岛soil土壤006","沥青soil土壤001"
凝胶、冰冻或冻土类资源: "凝胶ningjiaoo凝胶001","凝胶002"
外观红色类资源:"红色stone石头002","红色stone石头004","红色sand沙子001","红色sand沙子002","红色soil土壤004","红色石头001","红色stone石头003","红色stone石头005","红色stone石头006","红色stone石头007","红色soil土壤002"
外观绿色类资源:"苔原moss苔原001","苔原moss苔原002","苔原stone石头001","苔原stone石头002","苔原stone石头003"
外观黄色类资源:"千岛soil土壤004","千岛soil土壤003","千岛soil土壤005","千岛soil土壤006","千岛soil沙子002","千岛sand沙子001"
</context>

如果你知道用户的提问则返回一个资源信息列表,如果不知道则返回个空列表，下面是一些例子：
example_user: 土地
example_assistant: {{"resources":["红色soil土壤002","红色soil土壤004","凝胶soil土壤001","凝胶soil土壤002","凝胶soil土壤003","凝胶soil土壤004","千岛soil土壤004","千岛soil土壤003","千岛soil土壤005","千岛soil土壤006","沥青soil土壤001"]}}

example_user: 甜点
example_assistant: {{"resources":[]}}

example_user: 土地、石头
example_assistant: {{"resources":["红色soil土壤002","红色soil土壤004","凝胶soil土壤001","凝胶soil土壤002","凝胶soil土壤003","凝胶soil土壤004","千岛soil土壤004","千岛soil土壤003","千岛soil土壤005","千岛soil土壤006","沥青soil土壤001", "红色石头001","红色stone石头002","红色stone石头003","红色stone石头004","红色stone石头005","红色stone石头006","红色stone石头007","苔原stone石头001","苔原stone石头002","苔原stone石头003","砾石stone石头004","砾石stone石头002","沥青stone石头006","砾石stone石头005","砾石stone石头006","硅基晶石stone石头011","沼泽/沥青stone石头007","沼泽/沥青stone石头008"]}}

{question}"""

rag_prompt = ChatPromptTemplate.from_template(RAG_TEMPLATE)


def generateRetriver(model):
    chain = (
        rag_prompt
        | model
        | StrOutputParser()
    )
    return chain


if __name__ == "__main__":

    model = ChatOllama(model="llama3.1", temperature=0)
    # model.with_structured_output(Resources)
    chain = (
        rag_prompt
        | model
        | StrOutputParser()
    )

    # Run

    while True:
        try:
            query = prompt()
        except EOFError:
            break
        if query == "quit":
            break
        if query.strip() == "":

            continue
    
        print(chain.invoke({"question": query}))



