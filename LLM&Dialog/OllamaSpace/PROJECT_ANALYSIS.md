# OllamaSpace 项目详细分析文档

> 创建日期: 2026-01-15
> 版本: 1.0

## 目录
- [项目概述](#项目概述)
- [技术栈](#技术栈)
- [项目架构图](#项目架构图)
- [核心文件夹分析](#核心文件夹分析)
  - [explore_langchain](#1-explore_langchain---langchain探索与实验)
  - [ollama-tools](#2-ollama-tools---ollama工具调用库)
  - [SceneAgentServer](#3-sceneagentserver---场景代理服务器核心)
  - [SceneGenerator](#4-scenegenerator---场景生成器独立模块)
- [数据流图](#数据流图)
- [API接口说明](#api接口说明)
- [C++集成指南](#c集成指南)

---

## 项目概述

**OllamaSpace** 是一个基于 **LangChain + LangGraph + Ollama** 的 AI 代理系统，主要用于：
1. 通过自然语言对话收集星球环境参数（温度、湿度、颜色）
2. 根据用户输入生成星球的 Biome（生物群系）配置
3. 支持地形设置（经纬度+地形类型）
4. 使用向量数据库进行语义检索和推荐

### 核心功能
| 功能 | 描述 |
|------|------|
| 环境提取 | 从用户对话中提取温度范围、湿度范围、外观颜色 |
| Biome 生成 | 根据参数生成 BiomeMap 分布配置 |
| 地形配置 | 支持设置地形类型及其经纬度位置 |
| 智能推荐 | 当配置冲突时，使用 FAISS 向量库推荐合适的颜色/地形 |

---

## 技术栈

```
┌─────────────────────────────────────────────────────────────────┐
│                         技术架构                                 │
├─────────────────────────────────────────────────────────────────┤
│ LLM 推理层     │ Ollama (qwen2.5, llama3.1)                     │
├─────────────────────────────────────────────────────────────────┤
│ Agent 框架层   │ LangChain + LangGraph (状态图工作流)             │
├─────────────────────────────────────────────────────────────────┤
│ 向量数据库     │ Chroma + FAISS                                  │
├─────────────────────────────────────────────────────────────────┤
│ 嵌入模型       │ OllamaEmbeddings (llama3.1, nomic-embed-text)   │
├─────────────────────────────────────────────────────────────────┤
│ Web 框架       │ Django 5.1                                      │
├─────────────────────────────────────────────────────────────────┤
│ 消息持久化     │ SQLite (SQLChatMessageHistory)                  │
├─────────────────────────────────────────────────────────────────┤
│ 部署方式       │ uWSGI                                           │
└─────────────────────────────────────────────────────────────────┘
```

---

## 项目架构图

```
                                    ┌─────────────────┐
                                    │   C++ Client    │
                                    │  (HTTP请求)      │
                                    └────────┬────────┘
                                             │ HTTP POST
                                             ▼
┌────────────────────────────────────────────────────────────────────────────────┐
│                           SceneAgentServer (Django)                            │
│  ┌──────────────────────────────────────────────────────────────────────────┐  │
│  │                              views.py                                     │  │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────────────────┐   │  │
│  │  │  /connect/  │  │   /chat/    │  │           /qa/                  │   │  │
│  │  │ 创建会话ID  │  │ 处理对话    │  │    Web问答界面                   │   │  │
│  │  └──────┬──────┘  └──────┬──────┘  └─────────────────────────────────┘   │  │
│  └─────────┼────────────────┼───────────────────────────────────────────────┘  │
│            │                │                                                   │
│            ▼                ▼                                                   │
│  ┌──────────────────────────────────────────────────────────────────────────┐  │
│  │                         llm/ 模块                                         │  │
│  │  ┌───────────────┐  ┌───────────────┐  ┌───────────────┐                 │  │
│  │  │    llm.py     │  │ llm_3_args.py │  │ test_chat_z.py│◄─── 当前使用     │  │
│  │  │   (基础版)    │  │ (增强版+RAG)  │  │ (完整版+地形) │                  │  │
│  │  └───────┬───────┘  └───────┬───────┘  └───────┬───────┘                 │  │
│  │          │                  │                  │                          │  │
│  │          ▼                  ▼                  ▼                          │  │
│  │  ┌─────────────────────────────────────────────────────────────────────┐ │  │
│  │  │                    LangGraph StateGraph                              │ │  │
│  │  │  ┌─────────┐    ┌───────────────────┐    ┌────────────────────────┐ │ │  │
│  │  │  │  START  │───▶│    info_chain     │───▶│   add_tool_message     │ │ │  │
│  │  │  └─────────┘    │  (LLM对话处理)    │    │  (工具调用+数据生成)   │ │ │  │
│  │  │                 └─────────┬─────────┘    └───────────┬────────────┘ │ │  │
│  │  │                           │                          │              │ │  │
│  │  │                           ▼                          ▼              │ │  │
│  │  │                 ┌──────────────────────────────────────────────┐   │ │  │
│  │  │                 │         get_state (条件分支)                  │   │ │  │
│  │  │                 │   • tool_calls存在 → add_tool_message        │   │ │  │
│  │  │                 │   • HumanMessage → info_chain                │   │ │  │
│  │  │                 │   • 其他 → END                                │   │ │  │
│  │  │                 └──────────────────────────────────────────────┘   │ │  │
│  │  └─────────────────────────────────────────────────────────────────────┘ │  │
│  │                                                                           │  │
│  │  ┌───────────────────────────────────────────────────────────────────┐   │  │
│  │  │                     preload_1.py / preload_geo.py                  │   │  │
│  │  │  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────────┐│   │  │
│  │  │  │    BiomeMap     │  │ Chroma向量库    │  │  FAISS向量库        ││   │  │
│  │  │  │ (温湿度矩阵)    │  │ (颜色/材质检索) │  │ (地形类型检索)      ││   │  │
│  │  │  └─────────────────┘  └─────────────────┘  └─────────────────────┘│   │  │
│  │  └───────────────────────────────────────────────────────────────────┘   │  │
│  └──────────────────────────────────────────────────────────────────────────┘  │
│                                                                                 │
│  ┌───────────────────────────────────────────────────────────────────────────┐ │
│  │                              Ollama Server                                 │ │
│  │           ChatOllama("qwen2.5") ◄──► Tool Calling                         │ │
│  │           OllamaEmbeddings("llama3.1", "nomic-embed-text")                │ │
│  └───────────────────────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────────────────────┘
```

---

## 核心文件夹分析

### 1. explore_langchain - LangChain探索与实验

这是一个**实验性文件夹**，包含了LangChain/LangGraph的各种功能探索和原型代码。

```
explore_langchain/
├── note.ini                          # 笔记文件
├── extrator/                         # 信息提取器原型
│   ├── LangGraphExtrator.py          # 基础版提取器
│   └── LangGraphExtratorWithDBPersistence0_2.py  # 带数据库持久化版本
├── history_messages/                 # 消息历史管理
│   └── SaveMessage.py                # SQL持久化示例
├── persistence/                      # 持久化探索
│   ├── define_store.py               # InMemoryStore示例
│   └── Mongo.py                      # MongoDB检查点存储器实现
└── tool_call/                        # 工具调用探索
    ├── LangGraphMainInLoop.py        # 人机协作循环
    ├── LangGraphPrompt.py            # Prompt生成工作流
    ├── LangGraphTool.py              # 工具调用+中断
    ├── LangGraphToolAgent.py         # RAG工具代理
    └── todo.ini                      # 待办事项
```

#### 关键文件详解

| 文件 | 作用 | 技术要点 |
|------|------|---------|
| `LangGraphExtrator.py` | 最基础的信息提取器 | 使用 `bind_tools([PlanetInfo])` 绑定工具，内存中对话 |
| `LangGraphExtratorWithDBPersistence0_2.py` | 带SQL持久化的版本 | `SQLChatMessageHistory` + `RunnableWithMessageHistory` |
| `SaveMessage.py` | 消息历史存储示例 | 使用 `sqlite:///memory.db` 存储对话 |
| `define_store.py` | LangGraph Store示例 | `InMemoryStore` 跨线程共享记忆 |
| `Mongo.py` | MongoDB检查点存储 | 实现 `BaseCheckpointSaver` 接口，支持异步 |
| `LangGraphMainInLoop.py` | 人机协作模式 | `RequestAssistance` 模型 + `interrupt_before` 中断 |
| `LangGraphPrompt.py` | Prompt模板生成器 | 收集目标、变量、约束条件后生成Prompt |
| `LangGraphTool.py` | 工具调用与状态更新 | `update_state` 手动更新图状态 |
| `LangGraphToolAgent.py` | RAG代理示例 | `WebBaseLoader` + `create_retriever_tool` |

---

### 2. ollama-tools - Ollama工具调用库

这是一个**独立的工具库**，封装了Ollama的函数调用能力。

```
ollama-tools/
├── .gitignore
├── LICENSE
├── README.md
├── requirements.txt
├── example.py              # 示例：天气、时间、数学、搜索工具
├── ollama_tools.py         # 核心：函数描述生成器
├── sample_functions.py     # 示例工具函数
└── scene_generate.py       # 场景生成专用（温度/湿度提取）
```

#### 核心文件详解

##### `ollama_tools.py` - 工具描述生成器

```python
def generate_function_description(func):
    """
    将Python函数转换为Ollama Tool Calling格式
    
    输入: Python函数（带docstring和类型注解）
    输出: JSON格式的工具描述
    {
        'type': 'function',
        'function': {
            'name': 函数名,
            'description': docstring第一行,
            'parameters': {
                'type': 'object',
                'properties': {...},
                'required': [...]
            }
        }
    }
    """
```

##### `scene_generate.py` - 场景生成工具

| 工具名 | 功能 | 参数 |
|--------|------|------|
| `TemperatureRange` | 提取温度范围 | min: int, max: int |
| `HumidityRange` | 提取湿度范围 | min: int, max: int |

特色功能：
- 使用 `tool_example_to_messages()` 构建Few-shot示例
- 支持 `ToolMessage` 反馈机制

##### `example.py` - 完整示例

```python
# 支持的工具
tools = [
    get_current_weather,   # 天气查询
    get_current_time,      # 时间查询
    do_math,               # 数学计算
    query_duckduckgo       # 网络搜索
]

# 对话循环
while True:
    query = prompt()
    response = ollama.chat(model='llama3.1', messages=messages, tools=tools)
    if response['message']['tool_calls']:
        result = use_tools(tools_calls, functions)
```

---

### 3. SceneAgentServer - 场景代理服务器（核心）

这是**生产环境的Django服务器**，提供HTTP API接口。

```
SceneAgentServer/
├── manage.py                     # Django管理脚本
├── db.sqlite3                    # SQLite数据库
├── uwsgi.ini                     # uWSGI部署配置
├── README.md                     # GitLab默认README
├── .vscode/launch.json           # VSCode调试配置
│
├── agent/                        # Django项目配置
│   ├── __init__.py
│   ├── asgi.py                   # ASGI入口
│   ├── wsgi.py                   # WSGI入口
│   ├── settings.py               # Django设置
│   ├── urls.py                   # URL路由配置
│   └── templates/qa.html         # QA网页模板
│
├── extrator/                     # 核心应用
│   ├── __init__.py
│   ├── admin.py                  # Django Admin
│   ├── apps.py                   # 应用配置
│   ├── models.py                 # 数据模型
│   ├── tests.py                  # 测试
│   ├── views.py                  # ★ HTTP视图处理 ★
│   ├── migrations/               # 数据库迁移
│   └── llm/                      # ★ LLM核心模块 ★
│       ├── llm.py                # 基础版LLM链
│       ├── llm_3_args.py         # 三参数版（带RAG推荐）
│       ├── test_chat.py          # 测试版（严格确认模式）
│       ├── test_chat_z.py        # ★ 完整版（环境+地形双工具）★
│       ├── preload.py            # 基础预加载（Chroma）
│       ├── preload_1.py          # 增强预加载（BiomeMap矩阵）
│       ├── preload_geo.py        # 地形预加载（FAISS）
│       ├── utils.py              # 工具函数
│       ├── export_history.py     # 历史导出Excel
│       └── data/                 # 数据文件
│           ├── testData          # Biome库JSON
│           ├── stamp_terrain.json # 地形高度图配置
│           └── xingqiushili/     # 示例星球数据
│
├── biome_knowledge_index/        # FAISS索引（Biome知识库）
│   └── index.faiss
└── stamp_knowledge_index/        # FAISS索引（地形知识库）
    └── index.faiss
```

#### 核心模块详解

##### `views.py` - HTTP视图处理器

```python
# API端点
urlpatterns = [
    path('connect/', extratorView.connect),  # 创建会话
    path('chat/', extratorView.chat),        # 对话交互
    path('qa/', extratorView.qa_view),       # Web界面
]
```

| 端点 | 方法 | 功能 | 返回 |
|------|------|------|------|
| `/connect/` | POST | 创建会话，获取session_id | `{success, msg, session_id(cookie)}` |
| `/chat/` | POST | 发送消息，获取回复和数据 | `{success, msg, data, stamp}` |
| `/qa/` | GET/POST | Web问答界面 | HTML页面 |

##### `llm.py` - 基础版LLM链

```
流程: START → info_chain → get_state → add_tool_message → END
                  ↑                            │
                  └────────────────────────────┘
```

| 组件 | 功能 |
|------|------|
| `PlanetInfo` | Pydantic模型，包含temperature/humidity/colors |
| `info_chain` | 调用LLM处理对话 |
| `get_state` | 条件分支：有tool_calls→处理工具，否则→继续对话/结束 |
| `add_tool_message` | 处理工具调用，生成BiomeMap |

##### `test_chat_z.py` - 完整版（当前使用）

**增强功能：**
1. **双工具支持**：`PlanetInfo`（环境）+ `TerrainInfo`（地形）
2. **地形经纬度**：支持设置地形的具体位置
3. **FAISS检索**：智能推荐地形高度图

```python
# 数据模型
class PlanetInfo(BaseModel):
    temperature: TemperatureRange  # 温度范围
    humidity: HumidityRange        # 湿度范围
    colors: list[str]              # 外观颜色

class TerrainInfo(BaseModel):
    terrains: list[TerrainItem]    # 地形列表
    # TerrainItem: type(地形类型) + latitude(纬度) + longitude(经度)
```

##### `preload_1.py` - Biome预加载与矩阵

**核心数据结构：BiomeMap**

```
BiomeMap - 二维矩阵存储Biome索引
                     temperature →
          [[set(), set(), set(), ...],
           [set(), set(), set(), ...],
humidity   [set(), set(), set(), ...],
    ↓      [set(), set(), set(), ...],
           [set(), set(), set(), ...]]

每个set()存储该温湿度区间可用的Biome索引
```

**关键函数：**

| 函数 | 功能 |
|------|------|
| `parseBiomeLibrary(path)` | 加载testData，建立颜色/材质向量索引 |
| `parseHistoryFile(dir)` | 解析.biome文件，填充BiomeMap |
| `filterByColor(colors)` | 语义检索匹配颜色的Biome |
| `createSubBiomeMap(...)` | 创建子BiomeMap并细分区域 |
| `subdivision(...)` | 将BiomeMap细分为最大16个区域 |

##### `preload_geo.py` - 地形预加载

```python
# 地形类型配置
stamp_terrain.json = [
    {"name": "高原", "heightmap": "Height Map_gaoyuan.raw"},
    {"name": "海岛", "heightmap": "Height Map_haidao.raw"},
    {"name": "山脉", "heightmap": "Height Map_shanmai.raw"},
    ...
]

# 经纬度转换为球坐标
def get_normalized_coordinate(lat, lng):
    x = cos(lat) * cos(lng)
    y = cos(lat) * sin(lng)
    z = sin(lat)
    return x, y, z
```

##### `utils.py` - 工具函数

```python
load_json_from_file(path)      # 加载JSON
batch_load_json_from_files()   # 批量加载
getAbsPath(path)               # 获取绝对路径
recursiveLoadDir(dir, cb)      # 递归遍历目录
class cd:                       # 上下文管理器切换目录
```

---

### 4. SceneGenerator - 场景生成器（独立模块）

这是一个**命令行工具**，不依赖Django，可独立运行。

```
SceneGenerator/
├── main.py                   # 入口脚本
├── src/
│   ├── extract.py            # LLM信息提取
│   └── prepare/
│       └── preload.py        # Biome预加载（最完整版）
├── utils/
│   └── util.py               # 工具函数
└── data/                     # 数据文件
    ├── default.biomelibrary  # 默认Biome库
    ├── testData              # 测试数据
    └── xingqiushili/         # 示例星球
```

#### `main.py` - 主入口

```python
# 使用方式
python main.py <biome库路径> <历史数据目录>

# 流程
1. parseBiomeLibrary(file_path)  # 加载Biome库
2. parseHistoryFile(history_dir) # 解析历史数据
3. extract.launch()              # 启动对话提取
4. filterByColor()               # 颜色过滤
5. createSubBiomeMap()           # 生成BiomeMap
```

#### `src/extract.py` - 对话提取

```python
# 使用llama3.1进行对话
model = ChatOllama(model="llama3.1", temperature=0)

# Prompt模板
systemTemplate = """你是一个帮助用户创建星球的小助手...
信息包括：温度范围(-50到50)、湿度范围(0到100)、星球外观颜色、地形类型
{format_instraction}"""

# 工具
SaveScene(temperature, humidity, colors)  # 保存场景
```

#### `src/prepare/preload.py` - 完整预加载

这是最完整的预加载模块，包含：
1. **AbstractAgent**：LLM摘要提取（颜色+材质）
2. **Chroma向量库**：语义检索
3. **BiomeMap**：温湿度矩阵
4. **区域细分算法**：最大矩形合并

---

## 数据流图

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                              完整数据流                                       │
└─────────────────────────────────────────────────────────────────────────────┘

[用户输入] "我想要一个温度0-30度，湿度50-80%，绿色外观的星球"
    │
    ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                           LangGraph Workflow                                 │
│                                                                              │
│  ┌──────────────┐                                                           │
│  │ info_chain   │ ──► ChatOllama("qwen2.5").bind_tools([PlanetInfo])       │
│  └──────┬───────┘                                                           │
│         │                                                                    │
│         ▼                                                                    │
│  ┌──────────────────────────────────────────────────────────────────────┐   │
│  │ LLM Response:                                                         │   │
│  │ {                                                                     │   │
│  │   "tool_calls": [{                                                    │   │
│  │     "name": "PlanetInfo",                                             │   │
│  │     "args": {                                                         │   │
│  │       "temperature": {"min": 0, "max": 30},                          │   │
│  │       "humidity": {"min": 50, "max": 80},                            │   │
│  │       "colors": ["绿色"]                                              │   │
│  │     }                                                                 │   │
│  │   }]                                                                  │   │
│  │ }                                                                     │   │
│  └──────────────────────────────────────────────────────────────────────┘   │
│         │                                                                    │
│         ▼                                                                    │
│  ┌──────────────────┐                                                       │
│  │ add_tool_message │                                                       │
│  └────────┬─────────┘                                                       │
│           │                                                                  │
└───────────┼─────────────────────────────────────────────────────────────────┘
            │
            ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                           数据处理层                                         │
│                                                                              │
│  1. filterByColor(["绿色"])                                                  │
│     │                                                                        │
│     ▼                                                                        │
│  ┌──────────────────────────────────────────────────────────────────────┐   │
│  │ Chroma向量检索 → 返回匹配的Biome名称列表                               │   │
│  │ colorAssets = ["绿色草地0", "绿色石草1", "深绿色草土0", ...]           │   │
│  └──────────────────────────────────────────────────────────────────────┘   │
│     │                                                                        │
│     ▼                                                                        │
│  2. createSubBiomeMap(0, 30, 50, 80, colorAssets)                           │
│     │                                                                        │
│     ▼                                                                        │
│  ┌──────────────────────────────────────────────────────────────────────┐   │
│  │ BiomeMap.subMap() → 提取温度[0,30]×湿度[50,80]区域                    │   │
│  │ BiomeMap.intersectionUpdate() → 过滤出colorAssets中的Biome           │   │
│  │ BiomeMap.subdivision() → 细分为≤16个区域                              │   │
│  └──────────────────────────────────────────────────────────────────────┘   │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
            │
            ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                           返回结果                                           │
│  {                                                                           │
│    "success": 1,                                                             │
│    "msg": "环境设置完成",                                                     │
│    "data": {                                                                 │
│      "hRange": {"min": 50, "max": 80},                                      │
│      "tRange": {"min": 0, "max": 30},                                       │
│      "subRanges": [                                                         │
│        {"idx": 3, "temperature": {"min": 0, "max": 10}, "humidity": {...}}, │
│        {"idx": 7, "temperature": {"min": 10, "max": 20}, "humidity": {...}},│
│        ...                                                                   │
│      ]                                                                       │
│    },                                                                        │
│    "stamp": null  // 如果有地形设置，这里会有地形数据                          │
│  }                                                                           │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## API接口说明

### 1. 创建会话 - `/connect/`

**请求：**
```http
POST /connect/ HTTP/1.1
Host: 192.168.5.189:8000
Content-Type: application/json
```

**响应：**
```json
{
    "success": 1,
    "msg": "您好！我是帮助您设置星球环境的小助手...",
    "data": null
}
```
- Cookie中会设置 `agent_session_id`

### 2. 对话交互 - `/chat/`

**请求：**
```http
POST /chat/ HTTP/1.1
Host: 192.168.5.189:8000
Cookie: agent_session_id=xxx
Content-Type: text/plain

我想要温度0-30度，湿度50-80%，绿色外观
```

**响应（普通回复）：**
```json
{
    "success": 1,
    "msg": "好的，您设置的环境参数是：温度0-30°C，湿度50-80%，颜色绿色。请确认是否正确？",
    "data": null,
    "stamp": null
}
```

**响应（确认后生成数据）：**
```json
{
    "success": 1,
    "msg": "环境设置完成",
    "data": {
        "hRange": {"min": 50, "max": 80},
        "tRange": {"min": 0, "max": 30},
        "subRanges": [
            {
                "idx": 3,
                "temperature": {"min": 0, "max": 10},
                "humidity": {"min": 50, "max": 60}
            }
        ]
    },
    "stamp": null
}
```

**响应（地形设置）：**
```json
{
    "success": 1,
    "msg": "地形设置完成",
    "data": null,
    "stamp": [
        {
            "width": 1.0,
            "height": 1.0,
            "verticalScale": 1,
            "stamp_terrain_heightmap": "biome_terrain/StampTerrain/Height Map_shan.raw",
            "axis.x": 0.5,
            "axis.y": 0.7,
            "axis.z": 0.5
        }
    ]
}
```

---

## C++集成指南

### 方案一：HTTP客户端（推荐）

```cpp
// EchoLLMDialogManager.h
#pragma once
#include <string>
#include <functional>

namespace Echo {

struct LLMResponse {
    int success;
    std::string msg;
    std::string data;  // JSON字符串
    std::string stamp; // 地形数据JSON
};

class LLMDialogManager {
public:
    LLMDialogManager();
    ~LLMDialogManager();
    
    // 连接服务器，获取session_id
    bool Connect(const std::string& serverUrl);
    
    // 发送消息，获取回复
    LLMResponse Chat(const std::string& message);
    
    // 异步版本
    void ChatAsync(const std::string& message, 
                   std::function<void(const LLMResponse&)> callback);
    
    // 获取会话ID
    std::string GetSessionId() const { return m_sessionId; }
    
private:
    std::string m_serverUrl;
    std::string m_sessionId;
    
    // HTTP请求实现
    std::string HttpPost(const std::string& url, 
                         const std::string& body,
                         const std::string& contentType);
};

} // namespace Echo
```

### 方案二：Ollama C++ SDK

如果需要离线运行，可以直接调用Ollama的C++ API：
- [ollama-hpp](https://github.com/jmont-dev/ollama-hpp) - C++头文件库

### 建议的集成架构

```
┌─────────────────────────────────────────────────────────────────┐
│                      C++ Game Engine                             │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │                 EchoLLMDialogManager                       │  │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐│  │
│  │  │   Connect   │  │    Chat     │  │   ParseResponse     ││  │
│  │  │ (获取会话)  │  │ (发送消息)  │  │ (解析JSON返回)      ││  │
│  │  └──────┬──────┘  └──────┬──────┘  └──────────┬──────────┘│  │
│  └─────────┼────────────────┼────────────────────┼───────────┘  │
│            │                │                    │               │
│            ▼                ▼                    ▼               │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │              libcurl / WinHTTP / Boost.Beast              │  │
│  │                     (HTTP客户端)                           │  │
│  └───────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
                              │
                              │ HTTP POST
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                  SceneAgentServer (Python)                       │
│                   http://192.168.5.189:8000                      │
└─────────────────────────────────────────────────────────────────┘
```

---

## 附录

### A. 文件依赖关系

```
views.py
    ├── llm.py          (未使用，基础版)
    ├── llm_3_args.py   (三参数版)
    └── test_chat_z.py  ★当前使用★
           ├── preload_1.py
           │      ├── utils.py
           │      └── Chroma向量库
           └── preload_geo.py
                  └── FAISS向量库
```

### B. 模型配置

| 用途 | 模型 | 配置 |
|------|------|------|
| 对话生成 | qwen2.5 | temperature=0 |
| 文本嵌入 | llama3.1 | OllamaEmbeddings |
| 地形嵌入 | nomic-embed-text | OllamaEmbeddings |

### C. 数据库表

| 数据库 | 用途 |
|--------|------|
| memory.db | 对话历史存储 |
| test_memo.db | 测试对话历史 |
| db.sqlite3 | Django元数据 |

---

*文档结束*
