# 对话系统构建逻辑详解 (第2部分)

> **接续**: SYSTEM_CONSTRUCTION_ANALYSIS.md  
> **本部分内容**: 从零构建指南、共同点分析、最佳实践

---

## 🏗️ 从零构建指南

### 构建顺序建议

```
阶段1: 基础对话 (1-2天)
    └─ 实现 general_chat.py 级别的功能

阶段2: 工具调用 (3-5天)
    └─ 实现 llm.py 级别的功能

阶段3: 完整NPC系统 (1-2周)
    └─ 实现 npc_chat.py 级别的功能
```

---

## 📝 阶段1: 构建基础对话系统 (general_chat.py)

### 步骤1: 安装依赖

```bash
pip install langchain langchain-ollama langchain-community
pip install sqlalchemy
```

### 步骤2: 创建最小对话系统

```python
# minimal_chat.py
from langchain_ollama import ChatOllama
from langchain_core.messages import HumanMessage

# 1. 初始化LLM
llm = ChatOllama(model="qwen2.5", temperature=0.7)

# 2. 简单对话
def chat(message: str) -> str:
    response = llm.invoke([HumanMessage(content=message)])
    return response.content

# 3. 测试
if __name__ == "__main__":
    reply = chat("你好")
    print(reply)
```

**这是最简单的对话系统，只需3步！**

### 步骤3: 添加历史记录

```python
# chat_with_history.py
from langchain_community.chat_message_histories import SQLChatMessageHistory
from langchain_core.prompts import ChatPromptTemplate, MessagesPlaceholder
from langchain_core.runnables.history import RunnableWithMessageHistory

# 1. 历史记录管理
history_cache = {}

def get_session_history(session_id: str):
    if session_id not in history_cache:
        history_cache[session_id] = SQLChatMessageHistory(
            session_id, 
            connection="sqlite:///chat.db"
        )
    return history_cache[session_id]

# 2. 创建带历史的对话链
llm = ChatOllama(model="qwen2.5", temperature=0.7)

prompt = ChatPromptTemplate.from_messages([
    ("system", "你是一个友好的AI助手"),
    MessagesPlaceholder(variable_name="history"),
    MessagesPlaceholder(variable_name="input"),
])

chain = prompt | llm

chain_with_history = RunnableWithMessageHistory(
    chain,
    get_session_history,
    input_messages_key="input",
    history_messages_key="history"
)

# 3. 对话函数
def chat(message: str, session_id: str) -> str:
    response = chain_with_history.invoke(
        {"input": [HumanMessage(content=message)]},
        config={"configurable": {"session_id": session_id}}
    )
    return response.content
```

**现在有了历史记录功能！**

### 步骤4: 添加消息修剪

```python
from langchain_core.messages import trim_messages

# 防止历史消息过长
trimmer = trim_messages(
    max_tokens=2000,
    strategy="last",
    token_counter=len,
    include_system=True,
)

# 在链中添加trimmer
chain = prompt | trimmer | llm
```

**完成！这就是 general_chat.py 的核心逻辑。**

---

## 🔧 阶段2: 构建工具调用系统 (llm.py)

### 步骤1: 定义工具

```python
from pydantic import BaseModel, Field

# 1. 定义工具的数据结构
class WeatherQuery(BaseModel):
    """天气查询工具"""
    city: str = Field(..., description="城市名称")
    date: str = Field(default="今天", description="日期")

# 2. 绑定工具到LLM
llm = ChatOllama(model="qwen2.5")
llm_with_tools = llm.bind_tools([WeatherQuery])
```

### 步骤2: 创建LangGraph工作流

```python
from langgraph.graph import StateGraph, START, END
from typing_extensions import TypedDict
from typing import Annotated
from langgraph.graph.message import add_messages

# 1. 定义状态
class State(TypedDict):
    messages: Annotated[list, add_messages]

# 2. 创建图
workflow = StateGraph(State)

# 3. 定义节点函数
def call_model(state: State):
    response = llm_with_tools.invoke(state["messages"])
    return {"messages": [response]}

def call_tool(state: State):
    last_message = state["messages"][-1]
    tool_call = last_message.tool_calls[0]
    
    # 执行工具
    result = execute_weather_query(tool_call["args"])
    
    return {
        "messages": [ToolMessage(
            content=result,
            tool_call_id=tool_call["id"]
        )]
    }

# 4. 添加节点
workflow.add_node("agent", call_model)
workflow.add_node("tools", call_tool)

# 5. 添加边
def should_continue(state: State):
    last_message = state["messages"][-1]
    if last_message.tool_calls:
        return "tools"
    return END

workflow.add_conditional_edges("agent", should_continue, ["tools", END])
workflow.add_edge("tools", "agent")
workflow.add_edge(START, "agent")

# 6. 编译
graph = workflow.compile()
```

### 步骤3: 使用工作流

```python
def chat(message: str, session_id: str):
    result = graph.invoke(
        {"messages": [HumanMessage(content=message)]},
        config={"configurable": {"session_id": session_id}}
    )
    return result["messages"][-1].content
```

**完成！这就是 llm.py 的核心逻辑。**

---

## 🎮 阶段3: 构建NPC对话系统 (npc_chat.py)

### 步骤1: 设计NPC数据结构

```python
from dataclasses import dataclass
from typing import List

@dataclass
class NPCPersonality:
    """NPC人设"""
    name: str
    role: str
    age: int = 30
    traits: List[str] = None
    background: str = ""
    speech_style: str = ""
    knowledge: List[str] = None
    secrets: List[str] = None
    greeting: str = ""
```

### 步骤2: 创建记忆系统

```python
class MemorySystem:
    """简化的记忆系统"""
    
    def __init__(self):
        self.memories = []
    
    def add_memory(self, content: str, memory_type: str, importance: float):
        """添加记忆"""
        memory = {
            "content": content,
            "type": memory_type,
            "importance": importance,
            "timestamp": datetime.now()
        }
        self.memories.append(memory)
    
    def search(self, query: str, limit: int = 5):
        """检索记忆 (简化版，实际应使用向量检索)"""
        # 简单的关键词匹配
        results = []
        for memory in self.memories:
            if query.lower() in memory["content"].lower():
                results.append(memory)
        return results[:limit]
```

### 步骤3: 创建好感度系统

```python
class RelationshipManager:
    """好感度管理"""
    
    def __init__(self):
        self.relationships = {}  # {(npc_id, player_id): score}
    
    def get_affinity(self, npc_id: str, player_id: str) -> int:
        """获取好感度"""
        key = (npc_id, player_id)
        return self.relationships.get(key, 0)
    
    def update_affinity(self, npc_id: str, player_id: str, delta: int):
        """更新好感度"""
        key = (npc_id, player_id)
        current = self.relationships.get(key, 0)
        self.relationships[key] = max(0, min(100, current + delta))
    
    def get_level(self, score: int) -> str:
        """获取好感度等级"""
        if score < 20: return "陌生"
        elif score < 40: return "认识"
        elif score < 60: return "友好"
        elif score < 80: return "信任"
        else: return "挚友"
```

### 步骤4: 创建NPCAgent

```python
class NPCAgent:
    """NPC智能体"""
    
    def __init__(self, npc_id: str, personality: NPCPersonality, llm):
        self.npc_id = npc_id
        self.personality = personality
        self.llm = llm
        
        # 初始化子系统
        self.memory = MemorySystem()
        self.relationship = RelationshipManager()
        self.sessions = {}  # 会话历史
    
    def chat(self, player_id: str, message: str) -> dict:
        """对话处理"""
        # 1. 检索记忆
        memories = self.memory.search(message, limit=3)
        
        # 2. 获取好感度
        affinity_score = self.relationship.get_affinity(self.npc_id, player_id)
        affinity_level = self.relationship.get_level(affinity_score)
        
        # 3. 构建上下文
        context = self._build_context(memories, affinity_level)
        
        # 4. 生成回复
        reply = self._generate_reply(context, message)
        
        # 5. 更新好感度
        delta = self._calculate_affinity_change(message)
        self.relationship.update_affinity(self.npc_id, player_id, delta)
        
        # 6. 保存记忆
        self.memory.add_memory(f"玩家说: {message}", "episodic", 0.6)
        self.memory.add_memory(f"我回复: {reply}", "episodic", 0.5)
        
        return {
            "reply": reply,
            "affinity": {
                "score": affinity_score + delta,
                "level": self.relationship.get_level(affinity_score + delta)
            }
        }
    
    def _build_context(self, memories, affinity_level):
        """构建上下文"""
        context = f"你是{self.personality.name}，{self.personality.role}。\n"
        context += f"背景: {self.personality.background}\n"
        context += f"说话风格: {self.personality.speech_style}\n"
        context += f"当前好感度: {affinity_level}\n"
        
        if memories:
            context += "\n相关记忆:\n"
            for mem in memories:
                context += f"- {mem['content']}\n"
        
        return context
    
    def _generate_reply(self, context, message):
        """生成回复"""
        prompt = f"{context}\n\n用户: {message}\n{self.personality.name}:"
        response = self.llm.invoke([HumanMessage(content=prompt)])
        return response.content
    
    def _calculate_affinity_change(self, message):
        """计算好感度变化"""
        # 简单的情感分析
        positive_words = ["谢谢", "感谢", "帮助", "好"]
        negative_words = ["讨厌", "烦", "滚"]
        
        delta = 0
        for word in positive_words:
            if word in message:
                delta += 2
        for word in negative_words:
            if word in message:
                delta -= 3
        
        return delta
```

### 步骤5: 创建NPC管理器

```python
class NPCManager:
    """NPC管理器"""
    
    def __init__(self, llm):
        self.llm = llm
        self.npcs = {}  # {npc_id: NPCAgent}
    
    def register_npc(self, npc_id: str, personality: NPCPersonality):
        """注册NPC"""
        npc = NPCAgent(npc_id, personality, self.llm)
        self.npcs[npc_id] = npc
    
    def get_npc(self, npc_id: str) -> NPCAgent:
        """获取NPC"""
        return self.npcs.get(npc_id)
    
    def chat(self, npc_id: str, player_id: str, message: str) -> dict:
        """与NPC对话"""
        npc = self.get_npc(npc_id)
        if not npc:
            return {"success": False, "error": f"NPC不存在: {npc_id}"}
        
        result = npc.chat(player_id, message)
        result["success"] = True
        result["npc_name"] = npc.personality.name
        return result
```

**完成！这就是 npc_chat.py 的核心逻辑。**

---

## 🔍 共同点分析

### 1. 技术栈共同点

所有三个系统都使用：

```python
# 1. LangChain框架
from langchain_ollama import ChatOllama
from langchain_core.messages import HumanMessage, AIMessage
from langchain_core.prompts import ChatPromptTemplate

# 2. Ollama作为LLM后端
llm = ChatOllama(model="qwen2.5")

# 3. SQLite存储历史
from langchain_community.chat_message_histories import SQLChatMessageHistory

# 4. 会话管理
history_cache = {}
def get_session_history(session_id):
    if session_id not in history_cache:
        history_cache[session_id] = SQLChatMessageHistory(...)
    return history_cache[session_id]
```

### 2. 架构模式共同点

#### 模式1: 初始化 → 对话 → 返回

```python
# 所有系统都遵循这个基本模式

# 1. 初始化
llm = ChatOllama(...)
chain = create_chain(llm)

# 2. 对话函数
def chat(message: str, session_id: str) -> dict:
    result = chain.invoke(...)
    return format_result(result)

# 3. 返回标准格式
{
    "success": 1,
    "msg": "回复内容",
    "data": {...}  # 可选
}
```

#### 模式2: 会话管理

```python
# 所有系统都使用session_id管理会话

# llm.py
session_id = "#".join([ipAddr, str(uuid.uuid4())])

# general_chat.py
session_id = "general#" + "#".join([ipAddr, str(uuid.uuid4())])

# npc_chat.py
session_id = f"session_{player_id}_{datetime.now()}"
```

#### 模式3: Django集成

```python
# 所有系统都提供Django视图

@csrf_exempt
def chat_view(request):
    # 1. 获取session_id
    session_id = request.COOKIES.get("session_id")
    
    # 2. 获取用户输入
    message = request.body.decode('utf-8')
    
    # 3. 调用对话系统
    result = chat_system.chat(message, session_id)
    
    # 4. 返回JSON
    return JsonResponse(result)
```

### 3. 数据流共同点

```
用户输入 → 会话管理 → 历史加载 → 上下文构建 → LLM推理 → 历史保存 → 返回结果
```

所有系统都遵循这个基本数据流，只是在"上下文构建"阶段的复杂度不同：

- **general_chat.py**: 系统提示词 + 历史消息
- **llm.py**: 系统提示词 + 历史消息 + 工具定义
- **npc_chat.py**: 人设 + 记忆 + 知识 + 好感度 + 历史消息

---

## 🎯 差异点分析

### 1. 核心目标差异

```
llm.py
├─ 目标: 完成特定任务 (星球环境设置)
├─ 输出: 结构化数据
└─ 评价: 任务完成度

general_chat.py
├─ 目标: 自由对话
├─ 输出: 自然语言
└─ 评价: 对话质量

npc_chat.py
├─ 目标: 角色扮演
├─ 输出: 符合人设的回复
└─ 评价: 人设一致性 + 记忆连贯性
```

### 2. 状态管理差异

```python
# llm.py - 使用LangGraph管理复杂状态
workflow = StateGraph(State)
workflow.add_node("info", info_chain)
workflow.add_node("add_tool_message", add_tool_message)
workflow.add_conditional_edges(...)

# general_chat.py - 无状态管理，只有历史
# (历史由LangChain自动管理)

# npc_chat.py - 使用NPCAgent管理多种状态
class NPCAgent:
    def __init__(self):
        self.memory = MemorySystem()
        self.relationship = RelationshipManager()
        self.sessions = {}
        self.dialogue_storage = DialogueStorage()
```

### 3. 上下文构建差异

```python
# llm.py - 任务导向的上下文
context = f"""
**代理任务:**
请从用户输入中提取温度、湿度、颜色...

**步骤:**
1. 提取温度
2. 提取湿度
3. 确认信息
4. 调用工具
"""

# general_chat.py - 简单的系统提示词
context = """
你是一个友好、专业的AI助手。
请用简洁、准确的语言回复用户。
"""

# npc_chat.py - 多维度的上下文
context = f"""
你是{npc.name}，{npc.role}。
背景: {npc.background}
性格: {npc.traits}
说话风格: {npc.speech_style}

相关记忆:
{memories}

相关知识:
{knowledge}

当前好感度: {affinity_level}
可分享的秘密: {secrets}
"""
```

### 4. 输出处理差异

```python
# llm.py - 解析结构化输出
if last_message.tool_calls:
    args = last_message.tool_calls[-1].get('args')
    result["data"] = process_planet_data(args)

# general_chat.py - 直接返回文本
result["msg"] = response.content

# npc_chat.py - 返回多维度信息
return {
    "reply": reply,
    "affinity": {
        "score": score,
        "level": level
    },
    "npc_name": npc.name,
    "session_id": session_id
}
```

---

## 💡 最佳实践

### 1. 选择合适的系统

**使用 general_chat.py 当你需要**:
- ✅ 快速原型
- ✅ 简单问答
- ✅ 性能基准
- ❌ 不需要复杂功能

**使用 llm.py 当你需要**:
- ✅ 结构化信息提取
- ✅ 工具/API调用
- ✅ 多步骤任务
- ❌ 不需要角色扮演

**使用 npc_chat.py 当你需要**:
- ✅ 游戏NPC
- ✅ 角色扮演
- ✅ 长期记忆
- ✅ 关系管理
- ❌ 可以接受较慢的响应

### 2. 性能优化建议

```python
# 1. 使用缓存
from functools import lru_cache

@lru_cache(maxsize=100)
def get_npc_context(npc_id: str):
    # 缓存NPC上下文
    pass

# 2. 批量处理
def batch_chat(messages: List[str]):
    # 批量调用LLM
    pass

# 3. 异步处理
async def chat_async(message: str):
    # 异步调用
    pass

# 4. 减少上下文长度
trimmer = trim_messages(max_tokens=2000)
```

### 3. 错误处理

```python
def chat(message: str, session_id: str) -> dict:
    result = {"success": 1, "msg": ""}
    
    try:
        # 1. 验证输入
        if not message or not session_id:
            raise ValueError("缺少必要参数")
        
        # 2. 调用LLM
        response = llm.invoke(...)
        result["msg"] = response.content
        
    except ValueError as e:
        result["success"] = 0
        result["msg"] = f"参数错误: {e}"
    except Exception as e:
        result["success"] = 0
        result["msg"] = f"系统错误: {e}"
        # 记录日志
        logger.error(f"Chat error: {e}")
    
    return result
```

### 4. 测试建议

```python
# 1. 单元测试
def test_chat():
    result = chat("你好", "test_session")
    assert result["success"] == 1
    assert len(result["msg"]) > 0

# 2. 性能测试
def test_performance():
    start = time.time()
    chat("你好", "test_session")
    duration = time.time() - start
    assert duration < 5.0  # 5秒内响应

# 3. 集成测试
def test_django_integration():
    response = client.post('/chat/', data={"message": "你好"})
    assert response.status_code == 200
```

---

## 📚 总结

### 构建对话系统的关键步骤

1. **选择合适的复杂度**: 从简单开始，逐步增加功能
2. **使用标准框架**: LangChain提供了完整的工具链
3. **管理好会话**: session_id是核心
4. **优化性能**: 缓存、批量、异步
5. **完善错误处理**: 确保系统稳定性
6. **集成Django**: 提供Web API接口

### 三个系统的演进路径

```
general_chat.py (基础)
    ↓ 添加工具调用
llm.py (中级)
    ↓ 添加记忆、知识、好感度
npc_chat.py (高级)
```

### 推荐学习路径

1. **第1周**: 实现 general_chat.py，理解基础对话
2. **第2周**: 实现 llm.py，理解工具调用和状态管理
3. **第3-4周**: 实现 npc_chat.py，理解完整的NPC系统

---

## 🎓 延伸阅读

- LangChain官方文档: https://python.langchain.com/
- LangGraph教程: https://langchain-ai.github.io/langgraph/
- Ollama文档: https://ollama.ai/
- Django REST Framework: https://www.django-rest-framework.org/

---

**祝你构建成功！** 🚀
