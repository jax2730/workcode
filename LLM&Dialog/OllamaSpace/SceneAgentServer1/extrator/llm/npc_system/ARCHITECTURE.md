# NPC 智能体系统架构设计

> 基于 HelloAgents 第8、9、15章设计  
> 创建日期: 2026-01-16  
> 更新日期: 2026-01-17

---

## 📚 新人友好文档

**如果你是第一次接触本系统，强烈建议先阅读以下文档：**

### 🎯 入门必读
- [📖 文档导航](./docs/README.md) - 完整文档索引
- [🚀 快速入门](./docs/01-QUICKSTART.md) - 5分钟上手指南
- [⚙️ 安装配置](./docs/02-INSTALLATION.md) - 详细安装步骤

### 🔧 核心模块详解
- [🧠 记忆系统详解](./docs/05-MEMORY_SYSTEM.md) - 四层记忆如何工作
- [📚 RAG系统详解](./docs/06-RAG_SYSTEM.md) - 知识库检索原理
- [🏗️ 上下文构建详解](./docs/07-CONTEXT_BUILDER.md) - GSSC流水线
- [❤️ 好感度系统详解](./docs/08-RELATIONSHIP_SYSTEM.md) - 关系管理
- [💾 对话存储详解](./docs/09-DIALOGUE_STORAGE.md) - 多格式存储

### 📖 使用指南
- [🎨 创建NPC指南](./docs/11-CREATE_NPC.md) - 如何创建自定义NPC
- [📝 配置文件说明](./docs/12-CONFIG_FILES.md) - 配置项详解
- [💻 API使用指南](./docs/13-API_USAGE.md) - 编程接口

**本文档是技术架构概览，适合已经熟悉系统的开发者。**

---

## 1. 系统概述

本系统为游戏 NPC 提供完整的智能对话能力，包括：
- **长期记忆**: 记住与玩家的所有互动历史
- **人设一致性**: 保持 NPC 性格和背景的一致性
- **好感度系统**: 根据互动动态调整关系
- **知识检索 (RAG)**: 从知识库中检索相关信息
- **上下文工程**: 智能管理有限的上下文窗口
- **完整存储**: SQLite + Markdown + Excel 多格式持久化

## 2. 核心架构

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                              NPC System                                      │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐    │
│  │                         NPCManager                                   │    │
│  │  - 多NPC统一管理                                                     │    │
│  │  - 批量对话生成 (轻负载模式)                                         │    │
│  │  - NPC状态同步                                                       │    │
│  └─────────────────────────────────────────────────────────────────────┘    │
│                                    │                                         │
│                                    ▼                                         │
│  ┌─────────────────────────────────────────────────────────────────────┐    │
│  │                          NPCAgent                                    │    │
│  │  ┌───────────┐ ┌───────────┐ ┌───────────┐ ┌───────────────────┐   │    │
│  │  │MemoryTool │ │  RAGTool  │ │ NoteTool  │ │RelationshipManager│   │    │
│  │  │ (四层记忆) │ │ (知识检索) │ │ (结构笔记) │ │   (好感度系统)    │   │    │
│  │  └─────┬─────┘ └─────┬─────┘ └─────┬─────┘ └─────────┬─────────┘   │    │
│  │        │             │             │                 │              │    │
│  │        └─────────────┴─────────────┴─────────────────┘              │    │
│  │                              │                                       │    │
│  │                              ▼                                       │    │
│  │                    ┌─────────────────┐                              │    │
│  │                    │ ContextBuilder  │                              │    │
│  │                    │ (GSSC上下文构建) │                              │    │
│  │                    └────────┬────────┘                              │    │
│  │                             │                                        │    │
│  │                             ▼                                        │    │
│  │                    ┌─────────────────┐                              │    │
│  │                    │      LLM        │                              │    │
│  │                    │ (ChatOllama等)  │                              │    │
│  │                    └─────────────────┘                              │    │
│  └─────────────────────────────────────────────────────────────────────┘    │
│                                    │                                         │
│                                    ▼                                         │
│  ┌─────────────────────────────────────────────────────────────────────┐    │
│  │                       StorageManager                                 │    │
│  │                      (统一存储管理器)                                 │    │
│  │  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────────┐  │    │
│  │  │ DialogueStorage │  │ FileMemoryStore │  │   StorageConfig     │  │    │
│  │  │ (SQLite+Excel)  │  │   (Markdown)    │  │   (路径配置)        │  │    │
│  │  └────────┬────────┘  └────────┬────────┘  └─────────────────────┘  │    │
│  │           │                    │                                     │    │
│  └───────────┼────────────────────┼─────────────────────────────────────┘    │
│              │                    │                                          │
└──────────────┼────────────────────┼──────────────────────────────────────────┘
               │                    │
               ▼                    ▼
┌──────────────────────────────────────────────────────────────────────────────┐
│                              数据存储层                                       │
├──────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  npc_data/                                                                   │
│  ├── databases/                    # SQLite数据库 (兼容原general_chat.py)    │
│  │   ├── dialogue_history.db       # 对话历史                                │
│  │   ├── npc_memory.db             # NPC记忆索引                             │
│  │   └── npc_relationship.db       # 好感度数据                              │
│  │                                                                           │
│  ├── memories/                     # Markdown记忆文件 (人类可读)             │
│  │   ├── working/{npc_id}/{player_id}/    # 工作记忆                         │
│  │   │   └── session_YYYYMMDD_xxx.md                                        │
│  │   ├── episodic/{npc_id}/{player_id}/   # 情景记忆                         │
│  │   │   └── event_YYYYMMDD_HHMMSS_type.md                                  │
│  │   ├── semantic/{npc_id}/               # 语义记忆                         │
│  │   │   └── knowledge_topic.md                                             │
│  │   └── dialogues/{npc_id}/{player_id}/  # 对话记录                         │
│  │       └── dialogue_YYYYMMDD.md                                           │
│  │                                                                           │
│  ├── exports/                      # 导出文件 (兼容原export_history.py)      │
│  │   ├── excel/                    # Excel导出                               │
│  │   │   └── dialogue_xxx.xlsx                                              │
│  │   └── json/                     # JSON导出                                │
│  │                                                                           │
│  ├── knowledge_base/{npc_id}/      # RAG知识库文档                           │
│  ├── rag_index/{npc_id}/           # FAISS向量索引                           │
│  ├── notes/{npc_id}/               # 结构化笔记                              │
│  ├── configs/                      # NPC配置文件                             │
│  │   └── {npc_id}.json                                                      │
│  └── logs/                         # 日志文件                                │
│                                                                              │
└──────────────────────────────────────────────────────────────────────────────┘
```

## 3. 数据流程

### 3.1 完整对话流程 (10步)

```
玩家消息
    │
    ▼
┌─────────────────────────────────────────────────────────────┐
│ 1. 获取好感度信息 (RelationshipManager)                      │
│    └─ 从 npc_relationship.db 读取                           │
├─────────────────────────────────────────────────────────────┤
│ 2. 检索相关记忆 (MemoryTool)                                 │
│    ├─ 工作记忆: 内存 + memories/working/*.md                 │
│    ├─ 情景记忆: npc_memory.db + memories/episodic/*.md       │
│    └─ 语义记忆: npc_memory.db + memories/semantic/*.md       │
├─────────────────────────────────────────────────────────────┤
│ 3. 检索知识库 (RAGTool)                                      │
│    └─ 从 knowledge_base/ 和 rag_index/ 检索                  │
├─────────────────────────────────────────────────────────────┤
│ 4. 构建上下文 (ContextBuilder - GSSC流水线)                  │
│    ├─ Gather: 汇集多源信息                                   │
│    ├─ Select: 智能选择                                       │
│    ├─ Structure: 结构化输出                                  │
│    └─ Compress: 兜底压缩                                     │
├─────────────────────────────────────────────────────────────┤
│ 5. 生成回复 (LLM)                                            │
├─────────────────────────────────────────────────────────────┤
│ 6. 更新好感度 (RelationshipManager)                          │
│    └─ 写入 npc_relationship.db                               │
├─────────────────────────────────────────────────────────────┤
│ 7. 存储记忆 (MemoryTool)                                     │
│    ├─ 工作记忆: 内存                                         │
│    └─ 情景记忆: npc_memory.db                                │
├─────────────────────────────────────────────────────────────┤
│ 8. 持久化对话 (DialogueStorage)                              │
│    ├─ SQLite: dialogue_history.db                            │
│    └─ 兼容LangChain的message_store表                         │
├─────────────────────────────────────────────────────────────┤
│ 9. 保存Markdown (FileMemoryStore)                            │
│    ├─ 对话记录: memories/dialogues/{npc}/{player}/           │
│    └─ 重要事件: memories/episodic/{npc}/{player}/            │
├─────────────────────────────────────────────────────────────┤
│ 10. 可选: 导出Excel                                          │
│     └─ exports/excel/dialogue_xxx.xlsx                       │
└─────────────────────────────────────────────────────────────┘
    │
    ▼
NPC回复
```

### 3.2 记忆系统层次

```
┌─────────────────────────────────────────────────────────────────┐
│                        记忆系统                                  │
├─────────────────────────────────────────────────────────────────┤
│ 工作记忆 (WorkingMemory)                                        │
│ - 容量: 50条                                                    │
│ - TTL: 120分钟                                                  │
│ - 存储: 纯内存 + Markdown备份                                   │
│ - 用途: 当前对话上下文                                          │
├─────────────────────────────────────────────────────────────────┤
│ 情景记忆 (EpisodicMemory)                                       │
│ - 容量: 无限                                                    │
│ - 存储: SQLite + Markdown                                       │
│ - 用途: 对话历史、事件记录                                      │
│ - 评分: (相似度×0.6 + 时间近因性×0.2) × (0.8 + 重要性×0.4)      │
├─────────────────────────────────────────────────────────────────┤
│ 语义记忆 (SemanticMemory)                                       │
│ - 容量: 无限                                                    │
│ - 存储: SQLite + Markdown + 向量索引                            │
│ - 用途: 知识、概念、实体关系                                    │
│ - 评分: (向量相似度×0.7 + 图相似度×0.3) × (0.8 + 重要性×0.4)    │
├─────────────────────────────────────────────────────────────────┤
│ 感知记忆 (PerceptualMemory) [预留]                              │
│ - 用途: 多模态数据 (图像、音频)                                 │
└─────────────────────────────────────────────────────────────────┘
```

## 4. 好感度系统

### 4.1 等级划分

| 等级 | 分数范围 | 行为表现 |
|------|----------|----------|
| 陌生 | 0-20 | 礼貌但疏远，回复简短 |
| 熟悉 | 21-40 | 正常交流，偶尔分享信息 |
| 友好 | 41-60 | 当作朋友，详细热情 |
| 亲密 | 61-80 | 非常信任，分享私人话题 |
| 挚友 | 81-100 | 无话不谈，解锁秘密信息 |

### 4.2 分数变化规则

- 友好词汇 (谢谢、厉害等): +5
- 问候词汇 (你好、早上好): +3
- 中立交流: +2
- 不友好词汇 (讨厌、烦等): -3

## 5. 上下文工程 (GSSC)

### 5.1 结构化模板

```
[Role & Policies]
你是{NPC名字}，一位{角色}...
当前与玩家的关系: {好感度等级}
{好感度行为提示}

[Task]
{玩家当前问题}

[Evidence]
{从RAG检索的相关知识}
{从语义记忆检索的概念}

[Context]
{工作记忆 - 最近对话}
{情景记忆 - 相关历史}
{笔记 - 任务状态}

[Output]
请基于以上信息，以{NPC名字}的身份回复。
```

### 5.2 Token预算分配

- 总预算: 3000 tokens
- 系统指令预留: 20% (600 tokens)
- 可用预算: 80% (2400 tokens)
  - 角色设定: ~300 tokens
  - 好感度信息: ~100 tokens
  - 记忆检索: ~800 tokens
  - RAG知识: ~600 tokens
  - 对话历史: ~400 tokens
  - 笔记: ~200 tokens

## 6. 模块说明

### 6.1 核心模块

| 模块 | 文件 | 功能 |
|------|------|------|
| MemoryTool | memory_tool.py | 四层记忆系统 |
| RAGTool | rag_tool.py | 知识库检索 |
| ContextBuilder | context_builder.py | GSSC上下文构建 |
| RelationshipManager | relationship_manager.py | 好感度系统 |
| NoteTool | note_tool.py | 结构化笔记 |
| TerminalTool | terminal_tool.py | 文件系统访问 |

### 6.2 存储模块

| 模块 | 文件 | 功能 |
|------|------|------|
| DialogueStorage | dialogue_storage.py | SQLite + Excel存储 |
| FileMemoryStore | file_memory_store.py | Markdown文件存储 |
| StorageConfig | storage_config.py | 统一配置管理 |
| StorageManager | storage_config.py | 统一存储接口 |

### 6.3 智能体模块

| 模块 | 文件 | 功能 |
|------|------|------|
| NPCAgent | npc_agent.py | 单个NPC智能体 |
| NPCManager | npc_manager.py | 多NPC管理器 |

## 7. 与原服务器的兼容性

### 7.1 兼容 general_chat.py
- 使用相同的SQLite存储格式
- 兼容 `SQLChatMessageHistory` 的 `message_store` 表
- 支持 `session_id` 会话管理

### 7.2 兼容 export_history.py
- Excel导出格式一致
- 支持按会话导出
- 支持批量导出

## 8. 文件结构

```
npc_system/
├── __init__.py              # 模块导出
├── ARCHITECTURE.md          # 本文档
├── example.py               # 完整示例
│
├── memory_tool.py           # 四层记忆系统 ✅
├── context_builder.py       # GSSC上下文构建 ✅
├── relationship_manager.py  # 好感度系统 ✅
├── note_tool.py             # 结构化笔记 ✅
├── rag_tool.py              # RAG知识检索 ✅
├── terminal_tool.py         # 文件系统访问 ✅
│
├── dialogue_storage.py      # 对话存储 (SQLite+Excel) ✅
├── file_memory_store.py     # 文件记忆存储 (Markdown) ✅
├── storage_config.py        # 统一存储配置 ✅
├── init_data_dirs.py        # 目录初始化 ✅
│
├── npc_agent.py             # NPC智能体 ✅
├── npc_manager.py           # 多NPC管理 ✅
└── views.py                 # Django视图 ✅
```

## 9. API 接口设计

### 9.1 Django Views

```python
# POST /npc/connect/
# 创建会话，返回session_id

# POST /npc/chat/
# 请求: {"npc_id": "xxx", "player_id": "xxx", "message": "xxx"}
# 响应: {"reply": "xxx", "affinity": {...}, "session_id": "xxx"}

# GET /npc/status/
# 获取所有NPC状态

# GET /npc/status/{npc_id}/
# 获取单个NPC状态

# GET /npc/relationship/{npc_id}/{player_id}/
# 获取好感度信息

# POST /npc/batch_dialogue/
# 批量生成NPC对话 (轻负载模式)

# POST /npc/remember/
# 添加NPC记忆

# GET /npc/recall/
# 检索NPC记忆

# GET /npc/greeting/{npc_id}/
# 获取NPC问候语

# POST /npc/register/
# 注册新NPC
```

## 10. 使用示例

### 10.1 基本使用

```python
from npc_system import create_npc, init_npc_data_directories
from langchain_ollama import ChatOllama

# 1. 初始化数据目录
init_npc_data_directories("./npc_data")

# 2. 创建LLM
llm = ChatOllama(model="qwen2.5", temperature=0.7)

# 3. 创建NPC
npc = create_npc(
    npc_id="blacksmith",
    name="老铁匠",
    role="铁匠",
    traits=["严肃", "专业"],
    background="打铁30年的老师傅",
    llm=llm
)

# 4. 对话 (自动存储)
result = npc.chat("player_001", "能帮我打一把剑吗？")
print(result["reply"])

# 5. 导出对话历史
excel_path = npc.export_dialogue_history("player_001")
```

### 10.2 使用存储管理器

```python
from npc_system import StorageManager, StorageConfig

# 创建存储管理器
storage = StorageManager(StorageConfig(base_dir="./npc_data"))

# 保存对话
storage.save_dialogue(
    session_id="session_001",
    player_message="你好",
    npc_reply="你好，冒险者",
    npc_id="blacksmith",
    player_id="player_001"
)

# 导出到Excel
storage.export_dialogues_to_excel()
```

### 10.3 使用NPC管理器

```python
from npc_system import NPCManager, NPCManagerConfig

# 创建管理器
manager = NPCManager(NPCManagerConfig(), llm=llm)

# 注册NPC
manager.register_npc("merchant", {...})

# 对话
result = manager.chat("merchant", "player_001", "有什么好东西？")
```

## 11. 文件格式

### 11.1 Markdown记忆文件

```markdown
---
type: episodic_memory
npc_id: blacksmith
player_id: player_001
timestamp: 2024-01-15T10:30:00
importance: 0.8
event_type: dialogue
---

# 事件标题

事件内容...
```

### 11.2 NPC配置文件 (JSON)

```json
{
  "npc_id": "blacksmith",
  "personality": {
    "name": "老铁匠",
    "role": "铁匠",
    "age": 55,
    "gender": "男",
    "traits": ["严肃", "专业", "热心"],
    "background": "在这个镇上打铁30年...",
    "speech_style": "说话干脆利落",
    "knowledge": ["武器锻造", "金属材料"],
    "secrets": ["知道镇长的秘密"],
    "greeting": "哟，来了个冒险者。"
  }
}
```

## 12. 参考资料

- HelloAgents 第8章: 记忆与检索
- HelloAgents 第9章: 上下文工程
- HelloAgents 第15章: 构建赛博小镇
- SKILL.md: Agent Skills for Context Engineering
