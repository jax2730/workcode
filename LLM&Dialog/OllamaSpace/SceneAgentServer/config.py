"""SceneAgentServer 运行时配置。

说明:
- 该文件位于 SceneAgentServer 根目录。
- NPC LLM 读取优先级: 环境变量 > 本文件 > 代码默认值。
- 生产环境建议将敏感信息放在环境变量中。
"""

# 选择模型提供方: "ollama" 或 "lm_studio"
NPC_LLM_PROVIDER = "lm_studio"

# 公共参数
NPC_LLM_TEMPERATURE = 0.7
NPC_LLM_TIMEOUT = 120

# 默认 Ollama 配置（保留以便随时切回）
NPC_OLLAMA_MODEL = "qwen2.5"
NPC_OLLAMA_BASE_URL = ""

# LM Studio(OpenAI兼容) 配置
NPC_LM_STUDIO_URL = "http://192.168.5.23:1234/v1"
NPC_LM_STUDIO_MODEL = "xx"
NPC_LM_STUDIO_API_KEY = "sk-lm-kIGNIzk8:xBl4P6c2i9FuA1LE8IJW"

# 可选统一覆盖模型名（留空表示按 provider 选模型）
NPC_LLM_MODEL = ""

# 兼容 image_config 命名（可选）
LM_STUDIO_URL = NPC_LM_STUDIO_URL
LM_STUDIO_API_KEY = NPC_LM_STUDIO_API_KEY

# 调试接口开关（生产环境可设为 False）
ENABLE_MODEL_TEST_ENDPOINT = True
