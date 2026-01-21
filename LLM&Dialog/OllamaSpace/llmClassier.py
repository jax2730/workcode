from langchain_ollama import ChatOllama, OllamaEmbeddings
from langchain_core.prompts import ChatPromptTemplate, FewShotChatMessagePromptTemplate
from prompt_toolkit import prompt
from langchain_chroma import Chroma
from langchain_core.example_selectors import SemanticSimilarityExampleSelector

system_prompt = """
你是一个文本筛选助手,给定筛选关键词数组和待筛选数据项数组，逐个使用筛选关键词从待筛选数据项数组中筛选数据项，只要被筛选的数据项符合筛选关键词描述，就将被筛选数据项放入输出数组，输出数据中不要有重复项，待筛选数据项数组中不存在的数据项不要放入输出数组
"""

# examples = [
#     # {"input": "筛选颜色: ['红色'], 输入数据: ['红色sand沙子001'], '苔原moss苔原001']", "output": "['红色sand沙子001']"},
#     # {"input": "筛选颜色: ['红色'], 输入数据: ['苔原moss苔原001']", "output": "[]"},
#     # {"input": "筛选颜色: ['绿色'], 输入数据: ['苔原moss苔原001']", "output": "['苔原moss苔原001']"},
#     # {"input": "筛选颜色: ['褐色'], 输入数据: ['红色sand沙子001', '沥青stone石头006']", "output": "['沥青stone石头006']"},
#     # {"input": "筛选颜色: ['红褐色'], 输入数据: ['红色sand沙子001', '沥青stone石头006']", "output": "['红色sand沙子001', '沥青stone石头006']"},
#     # {"input": "筛选颜色: ['黄色'], 输入数据: ['千岛soil沙子002', '红色sand沙子001', 'sand沙子001']", "output": "['千岛soil沙子002', 'sand沙子001']"},
#     {"input": "筛选自然材质: ['沙子'], 待筛选数据项: ['千岛soil沙子002', '红色sand沙子001', '红色sand沙子001', 'sand沙子001'，'苔原moss苔原001', '红色stone石头004', '砾石stone石头005', '沥青stone石头006', '红色soil土壤002', '凝胶ningjiaoo凝胶001', '硅基晶石stone石头011', '沥青soil土壤001']", "output": "['千岛soil沙子002', '红色sand沙子001', 'sand沙子001']"},
#     {"input": "筛选自然材质: ['石头'], 待筛选数据项: ['红色stone石头004', '红色sand沙子001', '苔原moss苔原001', '砾石stone石头005', '凝胶soil土壤001', '苔原stone石头002'', '千岛sand沙子001', '千岛soil土壤004', '硅基晶石stone石头011']", "output": "['红色stone石头004', '砾石stone石头005', '苔原stone石头002', '硅基晶石stone石头011']"},
#     {"input": "筛选自然材质: ['土壤'], 待筛选数据项: ['红色stone石头004', '红色sand沙子001', '苔原moss苔原001', '凝胶soil土壤001', '千岛soil土壤004']", "output": "['凝胶soil土壤001', '千岛soil土壤004']"},
#     {"input": "筛选自然材质: ['苔原'], 待筛选数据项: ['红色stone石头004', '红色sand沙子001', '苔原moss苔原001', '凝胶soil土壤001', '千岛soil土壤004']", "output": "['苔原moss苔原001']"},
#     {"input": "筛选自然材质: ['土'], 待筛选数据项: ['红色stone石头004', '红色sand沙子001', '苔原moss苔原001', '凝胶soil土壤001', '千岛soil土壤004']", "output": "['凝胶soil土壤001', '千岛soil土壤004']"},
#     {"input": "筛选自然材质: ['沙子'], 待筛选数据项: ['红色stone石头004', '苔原moss苔原001', '凝胶soil土壤001', '千岛soil土壤004']", "output": "[]"},
# ]


examples = [
    # {"input": "筛选颜色: ['红色'], 输入数据: ['红色sand沙子001'], '苔原moss苔原001']", "output": "['红色sand沙子001']"},
    # {"input": "筛选颜色: ['红色'], 输入数据: ['苔原moss苔原001']", "output": "[]"},
    # {"input": "筛选颜色: ['绿色'], 输入数据: ['苔原moss苔原001']", "output": "['苔原moss苔原001']"},
    # {"input": "筛选颜色: ['褐色'], 输入数据: ['红色sand沙子001', '沥青stone石头006']", "output": "['沥青stone石头006']"},
    # {"input": "筛选颜色: ['红褐色'], 输入数据: ['红色sand沙子001', '沥青stone石头006']", "output": "['红色sand沙子001', '沥青stone石头006']"},
    # {"input": "筛选颜色: ['黄色'], 输入数据: ['千岛soil沙子002', '红色sand沙子001', 'sand沙子001']", "output": "['千岛soil沙子002', 'sand沙子001']"},
    {"input": "石头/石子/石块", "output": """["红色石头001","红色stone石头002","红色stone石头003","红色stone石头004","红色stone石头005","红色stone石头006","红色stone石头007","苔原stone石头001","苔原stone石头002","苔原stone石头003","砾石stone石头004","砾石stone石头002","沥青stone石头006","砾石stone石头005","砾石stone石头006","硅基晶石stone石头011","沼泽/沥青stone石头007"]"""},
    {"input": "沙子/沙/沙场", "output": """["红色sand沙子001","红色sand沙子002","千岛sand沙子001","千岛soil沙子002"]"""},
    {"input": "苔原/苔藓", "output": """["苔原moss苔原001","苔原moss苔原002","苔原stone石头001","苔原stone石头002","苔原stone石头003"]"""},
    {"input": "土地/土/田地/土壤/田", "output": """["红色soil土壤002","红色soil土壤004","凝胶soil土壤001","凝胶soil土壤002","凝胶soil土壤003","凝胶soil土壤004","千岛soil土壤004","千岛soil土壤003","千岛soil土壤005","千岛soil土壤006","沥青soil土壤001"]"""},
    {"input": "凝胶/冰块/冻土", "output": """["凝胶ningjiaoo凝胶001","凝胶002"]"""},
    
]

example_prompt = ChatPromptTemplate.from_messages(
    [
        ("human", "{input}"),
        ("ai","{output}")
    ]
)

#####
# to_vectorize = [" ".join(example.values()) for example in examples]
# embeddings = OllamaEmbeddings(model="llama3.1")
# vectorstore = Chroma.from_texts(to_vectorize, embeddings, metadatas=examples)

# example_selecter = SemanticSimilarityExampleSelector(
#     vectorstore=vectorstore,
#     k = 6
# )

to_vectorize = [example["input"] for example in examples]
embeddings = OllamaEmbeddings(model="llama3.1")
vectorstore = Chroma.from_texts(to_vectorize, embeddings, metadatas=examples)

example_selecter = SemanticSimilarityExampleSelector(
    vectorstore=vectorstore,
    k = 1
)

while True:
    try:
        query = prompt()
    except EOFError:
        break
    if query == "quit":
        break
    if query.strip() == "":

        continue
   
    ai_msg = example_selecter.select_examples({"input":query})
    print(ai_msg)

# result = example_selecter.select_examples({"input":"过滤颜色: ['红色'], 输入数据: ['红色sand沙子002']"})
# print(result)
#####

few_shot_prompt = FewShotChatMessagePromptTemplate(
    # input_variables=["input"],
    # example_selector=example_selecter,
    example_prompt=example_prompt,
    examples=examples
)

qa_prompt = ChatPromptTemplate.from_messages(
    [
        ("system", system_prompt),
        few_shot_prompt,
        ("human", "{input}"),
    ]
)

llm = ChatOllama(model="llama3.1", temperature=0.95)

prompt_llm = qa_prompt | llm

while True:
    try:
        query = prompt()
    except EOFError:
        break
    if query == "quit":
        break
    if query.strip() == "":

        continue
   
    ai_msg = prompt_llm.invoke(
        {
            "input": query
        }
    )
    print("\nai: " + ai_msg.content)