# 对话系统构建逻辑 - 快速参考

> **快速查阅版本**  
> **完整版**: SYSTEM_CONSTRUCTION_ANALYSIS.md + PART2.md

---

## 📊 三个系统对比表

| 特性 | llm.py | general_chat.py | npc_chat.py |
|------|--------|-----------------|-------------|
| **目标** | 星球环境设置 | 通用对话 | NPC角色扮演 |
| **复杂度** | ⭐⭐⭐⭐ | ⭐⭐ | ⭐⭐⭐⭐⭐ |
| **响应时间** | 2-3秒 | 1-2秒 | 3-5秒 |
| **核心技术** | LangGraph | LangChain | 完整NPC系统 |
| **工具调用** | ✅ | ❌ | ❌ |
| **记忆系统** | ❌ | ❌ | ✅ 四层 |
| **知识库** | ❌ | ❌ | ✅ RAG |
| **好感度** | ❌ | ❌ | ✅ 5级 |
| **Django端点** | 2个 | 3个 | 10个 |

---

## 🏗️ 核心构建逻辑

### llm.py - 工具调用系统

```python
# 核心流程
定义数据结构 (Pydantic)
    ↓
创建提示词模板 (引导LLM提取信息)
    ↓
构建LangGraph工作流 (状态机)
    ↓
实现工具调用节点
    ↓
条件分支判断 (是否调用工具)
    ↓
返回结构化数据
```

**关键代码**:
```python
# 1. 定义工具
class PlanetInfo(BaseModel):
    temperature: TemperatureRange
    humidity: HumidityRange
    colors: list[str]

# 2. 绑定工具
llm_with_tool = llm.bind_tools([PlanetInfo])

# 3. 创建工作流
workflow = StateGraph(State)
workflow.add_node("info", info_chain)
workflow.add_node("add_tool_message", add_tool_message)
workflow.add_conditional_edges("info", get_state)
graph = workflow.compile()
```

---

### general_chat.py - 通用对话系统

```python
# 核心流程
初始化LLM
    ↓
创建提示模板 (系统提示词 + 历史 + 输入)
    ↓
添加消息修剪器 (防止历史过长)
    ↓
组合对话链
    ↓
添加历史记录支持
    ↓
返回文本回复
```

**关键代码**:
```python
# 1. 创建链
llm = ChatOllama(model="qwen2.5", temperature=0.7)
trimmer = trim_messages(max_tokens=2000)
prompt = ChatPromptTemplate.from_messages([
    ("system", SYSTEM_PROMPT),
    MessagesPlaceholder(variable_name="history"),
    MessagesPlaceholder(variable_name="input"),
])
chain = prompt | trimmer | llm

# 2. 添加历史
chain_with_history = RunnableWithMessageHistory(
    chain, get_session_history,
    input_messages_key="input",
    history_messages_key="history"
)
```

---

### npc_chat.py - NPC对话系统

```python
# 核心流程
初始化NPC管理器
    ↓
加载NPC配置 (人设、知识、记忆)
    ↓
持久化玩家ID
    ↓
选择NPC
    ↓
10步对话处理:
  1. 接收输入
  2. 加载会话历史
  3. 检索记忆 (MemoryTool)
  4. 检索知识 (RAGTool)
  5. 查询好感度 (RelationshipManager)
  6. 构建上下文 (ContextBuilder)
  7. LLM生成回复
  8. 更新好感度
  9. 保存对话记录
  10. 返回结果
```

**关键代码**:
```python
# 1. 初始化管理器
manager = NPCManager(config, llm)

# 2. 对话处理
result = manager.chat(
    npc_id=npc_id,
    player_id=player_id,
    message=message
)

# 3. NPCAgent内部 (10步流程)
class NPCAgent:
    def chat(self, player_id, message):
        # 步骤3-5: 检索
        memories = self.memory.search(message)
        knowledge = self.rag.search(message)
        affinity = self.relationship.get_affinity(npc_id, player_id)
        
        # 步骤6: 构建上下文
        context = self.context_builder.build(
            memories, knowledge, affinity
        )
        
        # 步骤7: 生成
        reply = self._generate_reply(context, message)
        
        # 步骤8-9: 更新和保存
        self._update_affinity(player_id, message, reply)
        self._save_dialogue(player_id, message, reply)
        
        return {"reply": reply, "affinity": affinity}
```

---

## 🔗 Django集成方式

### 共同模式

```python
# 1. views.py - 导入对话系统
from .llm import llm_system

# 2. 创建连接端点
@csrf_exempt
def connect(request):
    session_id = generate_session_id()
    response = JsonResponse({"session_id": session_id})
    response.set_cookie("session_id", session_id)
    return response

# 3. 创建对话端点
@csrf_exempt
def chat(request):
    session_id = request.COOKIES.get("session_id")
    message = request.body.decode('utf-8')
    result = llm_system.chat(message, session_id)
    return JsonResponse(result)

# 4. urls.py - 配置路由
urlpatterns = [
    path('connect/', views.connect),
    path('chat/', views.chat),
]
```

### 差异点

| 系统 | Cookie | 全局状态 | 初始化时机 |
|------|--------|---------|-----------|
| **llm.py** | session_id | 无 | 模块导入时 |
| **general_chat.py** | session_id | 无 | 模块导入时 |
| **npc_chat.py** | session_id + player_id | NPCManager单例 | 首次请求时 |

---

## 🎯 共同点总结

### 1. 技术栈
- ✅ LangChain框架
- ✅ Ollama (qwen2.5)
- ✅ SQLite历史存储
- ✅ 会话管理 (session_id)

### 2. 架构模式
```python
# 所有系统都遵循
初始化 → 对话 → 返回

# 标准返回格式
{
    "success": 1,
    "msg": "回复内容",
    "data": {...}  # 可选
}
```

### 3. 数据流
```
用户输入 → 会话管理 → 历史加载 → 上下文构建 → LLM → 保存 → 返回
```

---

## 🔍 差异点总结

### 1. 核心目标
- **llm.py**: 完成任务 (结构化输出)
- **general_chat.py**: 自由对话 (自然语言)
- **npc_chat.py**: 角色扮演 (人设一致性)

### 2. 状态管理
- **llm.py**: LangGraph状态机
- **general_chat.py**: 无状态 (仅历史)
- **npc_chat.py**: NPCAgent多状态

### 3. 上下文复杂度
- **llm.py**: 任务提示词 + 工具定义
- **general_chat.py**: 系统提示词
- **npc_chat.py**: 人设 + 记忆 + 知识 + 好感度

---

## 🚀 从零构建建议

### 阶段1: 基础对话 (1-2天)
```python
# 实现最简单的对话
llm = ChatOllama(model="qwen2.5")
response = llm.invoke([HumanMessage(content="你好")])
```

### 阶段2: 添加历史 (1天)
```python
# 添加SQLite历史记录
chain_with_history = RunnableWithMessageHistory(
    chain, get_session_history
)
```

### 阶段3: 工具调用 (3-5天)
```python
# 实现LangGraph工作流
workflow = StateGraph(State)
workflow.add_node("agent", call_model)
workflow.add_node("tools", call_tool)
graph = workflow.compile()
```

### 阶段4: 完整NPC系统 (1-2周)
```python
# 实现记忆、知识、好感度
class NPCAgent:
    def __init__(self):
        self.memory = MemorySystem()
        self.rag = RAGTool()
        self.relationship = RelationshipManager()
```

---

## 💡 最佳实践

### 1. 选择合适的系统
- 简单问答 → general_chat.py
- 任务导向 → llm.py
- 角色扮演 → npc_chat.py

### 2. 性能优化
```python
# 缓存
@lru_cache(maxsize=100)
def get_context(npc_id): pass

# 批量处理
def batch_chat(messages): pass

# 异步
async def chat_async(message): pass
```

### 3. 错误处理
```python
try:
    result = chat(message, session_id)
except ValueError as e:
    return {"success": 0, "msg": f"参数错误: {e}"}
except Exception as e:
    return {"success": 0, "msg": f"系统错误: {e}"}
```

---

## 📚 文档索引

- **完整分析**: SYSTEM_CONSTRUCTION_ANALYSIS.md
- **构建指南**: SYSTEM_CONSTRUCTION_ANALYSIS_PART2.md
- **NPC系统文档**: 01-QUICKSTART.md ~ 12-CONFIG_FILES.md

---

**快速参考完毕！** 🎉
