from langchain_core.prompts import ChatPromptTemplate
from langchain_ollama import ChatOllama

system = """你是一个编写场景配置的助手,你只需要完成一件事：
根据用户描述场景转成json格式,严格遵守以下json字段说明, 每次只输出json内容,不做任何修饰
fields:
  terrain:
    type: string
    description: 表示环境地形类型（例如plain、hills等）
    enum:
      - Plain
      - Hills
      - Plateau
  weather:
    type: object
    properties:
      type:
        type: string
        description: 气候类型（例如snow、rain、sunny等）
        enum:
          - Snow
          - Rain
          - Sunny
      wind:
        type: string
        description: 风的状态（例如breeze、gust等）
        enum:
          - Breeze
          - Gust
          - Gentle Breeze
          - Calm
        default: Calm
  temperature:
    type: integer
    description: 温度值
    range:
      min: -90
      max: 90
"""

prompt = ChatPromptTemplate.from_messages([("system", system), ("human", "{input}")])



json_schema = {
	"type": "object",
	"title": "SceneConfig",
	"description": "场景配置信息",
	"properties":{
		"terrain" : {
			"type": "string",
			"description": "表示环境地形类型",
			"enum": ["Plain", "Hills", "Plateau"]
		},
		"weather": {
			"type": "object",
			"properties":{
				"type": {
					"type": "string",
					"description": "天气类型",
					"enum" : ["Snow", "Rain", "Sunny", "Overcast sky"]
				},
				"wind": {
					"type": "string",
					"description": "风的状态",
					"enum": ["Breeze", "Gust", "Gentle Breeze", "Calm"]
				}
			}
		},
		"temperature":{
			"type":"integer",
			"description": "温度值",
			"minimum": -90,
			"maximum": 90
		}
	}
}

llm = ChatOllama(model="qwen2.5", temperature=0)

structured_llm = llm.with_structured_output(json_schema)

# response = structured_llm.invoke("平坦的草原地形，晴空万里，天气温暖，温度为二十三度")

# print(response)


few_shot_structured_llm = prompt | structured_llm

while True:
    try:
        query = input()
    except EOFError:
        break
    if query == "quit":
        break
    if query.strip() == "":
        continue
   
    response = few_shot_structured_llm.invoke(query)

    print(response)