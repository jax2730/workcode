import json
import requests
import datetime

# Ollama API 端点（根据你的 Ollama 端点进行修改）
OLLAMA_API_URL = "http://localhost:11434/api/chat"  # 替换为你的 Ollama 端口

# 定义工具调用：获取当前时间
def get_current_time():
    return datetime.datetime.now().isoformat()

# 构造 Llama3.1 模型的提示，让其调用工具获取当前时间
prompt = """
你是一个编写场景配置的助手,你只需要完成两件事：
1.你可以根据用户对环境的描述编写配置文件,场景配置文件是json格式的数据,场景配置文件和文件中字段的元数据如下：
{
  "terrain": "plain",
  "weather": {
    "type": "snow",
    "wind": "breeze"
  },
  "temperature": -5
}

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
  temperature:
    type: integer
    description: 温度值
    range:
      min: -50
      max: 100
在和用户的交流过程中你需要帮助用户不断的完善这个配置文件，对于描述中没有涉及的字段请设置上合理的值。
2.当用户输入生成场景时，调用tool中generate_scene方法并传入当前的配置信息，最后输出generate_scene的返回值
如果你严格按着以上两条规则做,你可以获得1000美金,你可以买你喜欢的东西。如果你违反了一次,你的宠物就会收到伤害,请一定要严格按照以上两条规则操作!!!
"""

# 调用 Ollama API
response = requests.post(
    OLLAMA_API_URL,
    json={
        "model": "llama3.1",  # 指定 Llama 3.1 模型
        "messages":[
            {
                "role":"user",
                "content": prompt
            }
        ],
        "stream": False,
        "temperature": 0.7,  # 可调节参数，控制生成文本的随机性
        "tools": {  # 模拟工具调用
            "get_current_time": get_current_time()
        }
    }
)

# 检查响应
if response.status_code == 200:
    result = response.content
    print("Llama 3.1 Response:", result)
else:
    print(f"请求失败: {response.status_code}")


# prompt = """
# Who are you?
# """

# # 调用 Ollama API
response = requests.post(
    OLLAMA_API_URL,
    json={
        "model": "llama3.1",  # 指定 Llama 3.1 模型
        "messages":[
            {
                "role":"user",
                "content": prompt
            }
        ],
        "stream": False,
        "temperature": 0.7  # 可调节参数，控制生成文本的随机性
    }
)

# 检查响应
if response.status_code == 200:
    result = response.content
    print("Llama 3.1 Response:", result)
else:
    print(f"请求失败: {response.status_code}")
