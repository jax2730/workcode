# NPC智能体系统 - 完整文档

欢迎来到NPC智能体系统！这是一个功能完整的游戏NPC对话系统，具有长期记忆、知识库检索、好感度系统等高级功能。

## 📚 文档导航

### 🎯 快速开始
- [01-快速入门指南](./01-QUICKSTART.md) - 5分钟上手
- [02-安装配置指南](./02-INSTALLATION.md) - 详细安装步骤

### 🏗️ 系统架构
- [03-系统架构总览](./03-ARCHITECTURE_OVERVIEW.md) - 整体设计思路
- [04-数据流程详解](./04-DATA_FLOW.md) - 数据如何流转

### 🔧 核心模块详解
- [05-记忆系统详解](./05-MEMORY_SYSTEM.md) - 四层记忆如何工作
- [06-RAG系统详解](./06-RAG_SYSTEM.md) - 知识库检索原理
- [07-上下文构建详解](./07-CONTEXT_BUILDER.md) - GSSC流水线
- [08-好感度系统详解](./08-RELATIONSHIP_SYSTEM.md) - 关系管理
- [09-对话存储详解](./09-DIALOGUE_STORAGE.md) - 多格式存储
- [10-NPC智能体详解](./10-NPC_AGENT.md) - 核心Agent类

### 📖 使用指南
- [11-创建NPC指南](./11-CREATE_NPC.md) - 如何创建自定义NPC
- [12-配置文件说明](./12-CONFIG_FILES.md) - 配置项详解
- [13-API使用指南](./13-API_USAGE.md) - 编程接口
- [14-命令行工具](./14-CLI_TOOLS.md) - 命令行使用

### 🎨 高级功能
- [15-批量对话生成](./15-BATCH_GENERATION.md) - 多NPC管理
- [16-场景氛围生成](./16-SCENE_ATMOSPHERE.md) - 场景系统
- [17-性能优化指南](./17-PERFORMANCE.md) - 优化技巧
- [18-扩展开发指南](./18-EXTENSION.md) - 如何扩展

### 🔍 故障排查
- [19-常见问题FAQ](./19-FAQ.md) - 常见问题解答
- [20-调试指南](./20-DEBUGGING.md) - 如何调试

---

## 🎓 学习路径

### 路径1: 快速使用者（1小时）
1. 阅读 [01-快速入门指南](./01-QUICKSTART.md)
2. 阅读 [02-安装配置指南](./02-INSTALLATION.md)
3. 阅读 [11-创建NPC指南](./11-CREATE_NPC.md)
4. 开始使用！

### 路径2: 系统理解者（3小时）
1. 完成路径1
2. 阅读 [03-系统架构总览](./03-ARCHITECTURE_OVERVIEW.md)
3. 阅读 [04-数据流程详解](./04-DATA_FLOW.md)
4. 阅读核心模块文档（05-10）
5. 深入理解系统

### 路径3: 开发扩展者（1天）
1. 完成路径2
2. 阅读 [13-API使用指南](./13-API_USAGE.md)
3. 阅读 [18-扩展开发指南](./18-EXTENSION.md)
4. 阅读源码并开始开发

---

## 🌟 核心特性

### 1. 四层记忆系统
- **工作记忆**: 当前对话的临时信息
- **情景记忆**: 与玩家的互动历史
- **语义记忆**: NPC的知识和经验
- **感知记忆**: 环境和场景信息

### 2. RAG知识库
- 支持文本、Markdown、JSON文档
- FAISS向量检索（可选）
- 余弦相似度检索（备用）
- 智能文档分块

### 3. 好感度系统
- 5个等级：陌生 → 熟悉 → 友好 → 亲密 → 挚友
- 动态分数计算（0-100）
- 互动次数统计
- 解锁秘密机制

### 4. 多格式存储
- SQLite数据库（结构化查询）
- Excel导出（数据分析）
- Markdown文件（人类可读）
- 兼容LangChain格式

### 5. GSSC上下文构建
- **Gather**: 汇集多源信息
- **Select**: 智能信息选择
- **Structure**: 结构化输出
- **Compress**: 兜底压缩

---

## 📦 项目结构

```
npc_system/
├── docs/                          # 📚 文档目录（你在这里）
│   ├── README.md                  # 文档导航
│   ├── 01-QUICKSTART.md          # 快速入门
│   ├── 02-INSTALLATION.md        # 安装配置
│   └── ...                        # 其他文档
│
├── __init__.py                    # 模块导出
├── npc_agent.py                   # 🤖 NPC智能体核心
├── npc_manager.py                 # 👥 多NPC管理器
├── npc_chat.py                    # 💬 命令行对话工具
│
├── memory_tool.py                 # 🧠 四层记忆系统
├── rag_tool.py                    # 📚 RAG知识库
├── context_builder.py             # 🏗️ 上下文构建
├── relationship_manager.py        # ❤️ 好感度系统
├── dialogue_storage.py            # 💾 对话存储
├── file_memory_store.py           # 📁 文件记忆存储
├── storage_config.py              # ⚙️ 统一存储配置
├── note_tool.py                   # 📝 结构化笔记
├── terminal_tool.py               # 🔧 文件系统工具
│
├── init_data_dirs.py              # 📂 数据目录初始化
├── example.py                     # 📖 使用示例
├── ARCHITECTURE.md                # 🏛️ 架构文档（旧版）
├── PERFORMANCE_ANALYSIS.md        # 📊 性能分析
└── TODO.md                        # ✅ 开发计划
```

---

## 🎯 设计理念

### 1. 模块化设计
每个功能都是独立的模块，可以单独使用或组合使用。

```python
# 可以单独使用记忆系统
memory = MemoryTool(config)
memory.add("重要信息")

# 可以单独使用RAG
rag = RAGTool(config)
rag.search("查询内容")

# 也可以组合使用
npc = NPCAgent(memory=memory, rag=rag)
```

### 2. 配置驱动
所有行为都可以通过配置文件控制，无需修改代码。

```json
{
  "npc_id": "blacksmith",
  "personality": {
    "name": "老铁匠",
    "traits": ["严肃", "专业"]
  }
}
```

### 3. 多格式存储
同时支持SQLite、Excel、Markdown，满足不同需求。

```
对话记录 → SQLite (快速查询)
         → Excel (数据分析)
         → Markdown (人类阅读)
```

### 4. 兼容性优先
兼容LangChain、原有的general_chat.py等系统。

---

## 🚀 快速预览

### 创建一个NPC只需3步

```python
from npc_system import create_npc
from langchain_ollama import ChatOllama

# 1. 创建LLM
llm = ChatOllama(model="qwen2.5")

# 2. 创建NPC
npc = create_npc(
    npc_id="blacksmith",
    name="老铁匠",
    role="铁匠",
    traits=["严肃", "专业"],
    llm=llm
)

# 3. 开始对话
result = npc.chat("player_001", "你好！")
print(result["reply"])
```

### 命令行对话

```bash
python -m extrator.llm.npc_system.npc_chat
```

---

## 📞 获取帮助

### 文档问题
- 如果文档有不清楚的地方，请查看 [19-常见问题FAQ](./19-FAQ.md)
- 如果遇到错误，请查看 [20-调试指南](./20-DEBUGGING.md)

### 技术支持
- 查看源码注释（代码中有详细说明）
- 运行 `example.py` 查看示例
- 阅读 `ARCHITECTURE.md` 了解设计思路

---

## 🎉 开始学习

现在，请从 [01-快速入门指南](./01-QUICKSTART.md) 开始你的学习之旅！

祝你使用愉快！🚀
