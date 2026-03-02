# llm 模块说明

## 目录结构与文件分类

### 🟢 生产使用（当前 Java 客户端调用链路）

| 文件 | 作用 |
|---|---|
| `chat_interface.py` | **统一后端分发器** — 按 backend 名称 (general/test/test_chat/npc) 路由到对应的 chat/connect/clear 处理函数 |
| `general_chat.py` | **核心对话引擎** — ChatManager: 根据 persona 构建 langchain chain，SQLite 聊天历史，`npc_configs/` 加载人设 |
| `persona_api.py` | **人设 CRUD API** — 对 `npc_configs/*.json` 的增删改查 REST 接口 |

### 🟡 辅助/测试后端（可用但非 NPC 主链路）

| 文件 | 作用 |
|---|---|
| `test.py` | 简单测试对话后端 — 硬编码铁匠人设，不支持多 NPC 切换（backend=`"test"`） |
| `test_chat.py` | 星球环境抽取后端 — 用 PydanticOutputParser 提取温度/湿度/颜色（backend=`"test_chat"`） |
| `test_chat_z.py` | QA 视图后端 — 用于 `qa_view`，类似 test_chat |

### 🟠 旧版/遗留代码（可考虑归档或删除）

| 文件 | 作用 | 建议 |
|---|---|---|
| `llm.py` | 旧版星球环境抽取，与 `test_chat.py` 功能重复 | 可归档 |
| `llm_3_args.py` | 三参数版星球抽取，被旧 `/chat/` endpoint 调用 | 可归档 |
| `Practice.py` | **空文件** | **删除** |

### 🔵 数据预加载（RAG 向量化 - 星球数据，非 NPC）

| 文件 | 作用 |
|---|---|
| `preload.py` | Chroma 向量数据库加载 — 星球地理/生态数据嵌入 |
| `preload_1.py` | 数据预加载 — `llm_3_args` 的配套数据 |
| `preload_geo.py` | 地理数据预加载 |

### 🟣 工具/独立脚本

| 文件 | 作用 |
|---|---|
| `utils.py` | 通用文件工具 — JSON 加载、路径辅助 |
| `export_history.py` | 独立脚本 — SQLite 聊天记录导出为 Excel（依赖 openpyxl） |

### 🔴 高级 NPC 系统（完整但当前未接入）

| 目录 | 作用 |
|---|---|
| `npc_system/` | **完整 NPC 智能体系统** — 包含记忆(`memory_tool`)、RAG(`rag_tool`)、关系管理(`relationship_manager`)、上下文构建(`context_builder`)、持久化存储等。功能全面但依赖重，当前 Java 客户端走的是轻量级 `general_chat.py` 路径 |

---

## 为什么 general_chat 没有用到 RAG/记忆等功能？

`general_chat.py` 是轻量级实现：
- ✅ 多 persona 支持（通过 `npc_configs/*.json`）
- ✅ SQLite 聊天历史（按 session_id 存储）
- ✅ langchain ConversationChain
- ❌ **没有** RAG 向量检索
- ❌ **没有** NPC 记忆持久化（重启后记忆丢失）
- ❌ **没有** NPC 关系网络

这些高级功能都在 `npc_system/` 中实现了（`NPCManager` → `NPCAgent` → `MemoryTool` + `RAGTool`）。
未来如需升级，可将 `npc_system` 的功能逐步引入 `chat_interface.py` 的 npc backend。

---

## 相关外部目录

| 路径 | 作用 |
|---|---|
| `SceneAgentServer/npc_configs/` | NPC 人设配置 JSON 文件（blacksmith.json, guard_captain.json 等） |
| `extrator/llm/data/` | 星球数据文件（地形、生态等，供 preload 使用） |
| `extrator/llm/npc_system/docs/` | npc_system 架构文档（28 个 markdown 文件） |
