# 10 - NPC智能体详解 (第2部分)

> **接续**: 10-NPC_AGENT.md  
> **本部分内容**: 内部方法实现、便捷方法、高级特性

---

## 🔧 内部方法实现

### _build_context() - 构建上下文

```python
def _build_context(
    self,
    message: str,
    player_id: str,
    extra_context: Dict[str, Any] = None
) -> str:
    """
    构建对话上下文 (步骤3-5)
    
    流程:
    1. 检索记忆 (MemoryTool)
    2. 检索知识 (RAGTool)
    3. 查询好感度 (RelationshipManager)
    4. 构建上下文 (ContextBuilder)
    
    Args:
        message: 用户消息
        player_id: 玩家ID
        extra_context: 额外上下文
    
    Returns:
        str: 完整的上下文字符串
    """
    # 步骤3: 检索记忆
    memories = self.memory.search(
        query=message,
        memory_types=["episodic", "semantic"],
        limit=5
    )
    
    # 步骤4: 检索知识 (如果启用RAG)
    knowledge = []
    if self.rag:
        knowledge = self.rag.search(
            query=message,
            top_k=3
        )
    
    # 步骤5: 查询好感度
    affinity_info = self.relationship.get_affinity(
        npc_id=self.npc_id,
        player_id=player_id
    )
    
    # 获取可分享的秘密
    secrets = self._get_secrets_for_affinity(affinity_info.level)
    
    # 构建上下文
    context = self.context_builder.build(
        user_message=message,
        memories=memories,
        knowledge=knowledge,
        affinity=affinity_info,
        secrets=secrets,
        extra_context=extra_context
    )
    
    return context


def _get_secrets_for_affinity(self, affinity_level: str) -> str:
    """
    根据好感度等级获取可分享的秘密
    
    Args:
        affinity_level: 好感度等级
    
    Returns:
        str: 可分享的秘密文本
    """
    level_map = {
        "陌生": 0,
        "认识": 1,
        "友好": 2,
        "信任": 3,
        "挚友": 4
    }
    
    level_index = level_map.get(affinity_level, 0)
    
    # 获取该等级及以下的所有秘密
    available_secrets = []
    for i, secret in enumerate(self.personality.secrets):
        if i <= level_index:
            available_secrets.append(secret)
    
    if available_secrets:
        return "\n".join([f"- {s}" for s in available_secrets])
    else:
        return ""
```

### _generate_reply() - 生成回复

```python
def _generate_reply(self, context: str, message: str) -> str:
    """
    使用LLM生成回复 (步骤7)
    
    Args:
        context: 上下文
        message: 用户消息
    
    Returns:
        str: NPC回复
    """
    from langchain.schema import HumanMessage, SystemMessage
    
    # 构建消息
    messages = [
        SystemMessage(content=context),
        HumanMessage(content=message)
    ]
    
    # 调用LLM
    try:
        response = self.llm.invoke(messages)
        reply = response.content.strip()
        return reply
    except Exception as e:
        print(f"[NPCAgent] LLM调用失败: {e}")
        return f"抱歉，{self.personality.name}现在有些走神..."
```

### _update_affinity() - 更新好感度

```python
def _update_affinity(
    self,
    player_id: str,
    message: str,
    reply: str
) -> Any:
    """
    更新好感度 (步骤8)
    
    分析对话内容，计算好感度变化
    
    Args:
        player_id: 玩家ID
        message: 用户消息
        reply: NPC回复
    
    Returns:
        AffinityInfo: 更新后的好感度信息
    """
    # 简单的情感分析
    delta = 0
    
    # 正面词汇
    positive_words = ["谢谢", "感谢", "帮助", "好", "棒", "厉害"]
    # 负面词汇
    negative_words = ["讨厌", "烦", "滚", "笨", "蠢"]
    
    for word in positive_words:
        if word in message:
            delta += 2
    
    for word in negative_words:
        if word in message:
            delta -= 3
    
    # 对话长度奖励 (表示玩家投入)
    if len(message) > 20:
        delta += 1
    
    # 更新好感度
    new_affinity = self.relationship.update_affinity(
        npc_id=self.npc_id,
        player_id=player_id,
        delta=delta,
        reason=f"对话: {message[:20]}..."
    )
    
    return new_affinity
```

### _save_dialogue() - 保存对话

```python
def _save_dialogue(
    self,
    player_id: str,
    message: str,
    reply: str,
    session_id: str,
    timestamp: str
):
    """
    保存对话记录 (步骤9)
    
    Args:
        player_id: 玩家ID
        message: 用户消息
        reply: NPC回复
        session_id: 会话ID
        timestamp: 时间戳
    """
    if not self.dialogue_storage:
        return
    
    # 保存到SQLite和Markdown
    self.dialogue_storage.save_dialogue(
        npc_id=self.npc_id,
        player_id=player_id,
        user_message=message,
        npc_reply=reply,
        session_id=session_id,
        timestamp=timestamp
    )
```

### _update_memory() - 更新记忆

```python
def _update_memory(
    self,
    player_id: str,
    message: str,
    reply: str,
    timestamp: str
):
    """
    更新记忆系统
    
    Args:
        player_id: 玩家ID
        message: 用户消息
        reply: NPC回复
        timestamp: 时间戳
    """
    # 保存情景记忆
    self.memory.add_memory(
        content=f"玩家说: {message}",
        memory_type="episodic",
        importance=0.6,
        metadata={
            "player_id": player_id,
            "timestamp": timestamp,
            "type": "user_message"
        }
    )
    
    self.memory.add_memory(
        content=f"我回复: {reply}",
        memory_type="episodic",
        importance=0.5,
        metadata={
            "player_id": player_id,
            "timestamp": timestamp,
            "type": "npc_reply"
        }
    )
    
    # 如果对话包含重要信息，保存为语义记忆
    if self._is_important_dialogue(message, reply):
        self.memory.add_memory(
            content=f"与{player_id}的重要对话: {message} -> {reply}",
            memory_type="semantic",
            importance=0.8,
            metadata={
                "player_id": player_id,
                "timestamp": timestamp
            }
        )


def _is_important_dialogue(self, message: str, reply: str) -> bool:
    """判断对话是否重要"""
    important_keywords = ["任务", "秘密", "帮助", "请求", "承诺"]
    
    for keyword in important_keywords:
        if keyword in message or keyword in reply:
            return True
    
    return False
```

---

## 🎯 便捷方法

### 记忆相关方法

```python
def remember(
    self,
    content: str,
    memory_type: str = "semantic",
    importance: float = 0.7
) -> str:
    """
    手动添加记忆
    
    Args:
        content: 记忆内容
        memory_type: 记忆类型
        importance: 重要性
    
    Returns:
        str: 记忆ID
    """
    memory_id = self.memory.add_memory(
        content=content,
        memory_type=memory_type,
        importance=importance
    )
    
    print(f"[NPCAgent] 添加记忆: {content[:50]}...")
    return memory_id


def recall(
    self,
    query: str,
    memory_types: List[str] = None,
    limit: int = 5
) -> List[Dict]:
    """
    检索记忆
    
    Args:
        query: 查询文本
        memory_types: 记忆类型列表
        limit: 返回数量
    
    Returns:
        List[Dict]: 记忆列表
    """
    memories = self.memory.search(
        query=query,
        memory_types=memory_types,
        limit=limit
    )
    
    return memories


def forget(self, memory_id: str) -> bool:
    """
    删除记忆
    
    Args:
        memory_id: 记忆ID
    
    Returns:
        bool: 是否成功
    """
    return self.memory.delete_memory(memory_id)
```

### 知识相关方法

```python
def add_knowledge(
    self,
    content: str,
    doc_id: str = None,
    metadata: Dict = None
) -> str:
    """
    添加知识
    
    Args:
        content: 知识内容
        doc_id: 文档ID
        metadata: 元数据
    
    Returns:
        str: 文档ID
    """
    if not self.rag:
        print("[NPCAgent] RAG未启用")
        return None
    
    doc_id = self.rag.add_document(
        content=content,
        doc_id=doc_id,
        metadata=metadata
    )
    
    print(f"[NPCAgent] 添加知识: {content[:50]}...")
    return doc_id


def search_knowledge(
    self,
    query: str,
    top_k: int = 3
) -> List[Dict]:
    """
    搜索知识
    
    Args:
        query: 查询文本
        top_k: 返回数量
    
    Returns:
        List[Dict]: 知识列表
    """
    if not self.rag:
        return []
    
    return self.rag.search(query=query, top_k=top_k)
```

### 笔记相关方法

```python
def take_note(
    self,
    title: str,
    content: str,
    note_type: str = "general"
) -> str:
    """
    记笔记
    
    Args:
        title: 标题
        content: 内容
        note_type: 笔记类型
    
    Returns:
        str: 笔记ID
    """
    note_id = self.notes.create_note(
        title=title,
        content=content,
        note_type=note_type
    )
    
    print(f"[NPCAgent] 创建笔记: {title}")
    return note_id


def read_note(self, note_id: str) -> Dict:
    """读取笔记"""
    return self.notes.get_note(note_id)


def list_notes(self, note_type: str = None) -> List[Dict]:
    """列出笔记"""
    return self.notes.list_notes(note_type=note_type)
```

### 状态查询方法

```python
def get_status(self, player_id: str) -> Dict[str, Any]:
    """
    获取NPC状态
    
    Args:
        player_id: 玩家ID
    
    Returns:
        Dict: 状态信息
    """
    # 获取好感度
    affinity = self.relationship.get_affinity(
        npc_id=self.npc_id,
        player_id=player_id
    )
    
    # 获取对话统计
    dialogue_count = 0
    if self.dialogue_storage:
        history = self.dialogue_storage.get_dialogue_history(
            npc_id=self.npc_id,
            player_id=player_id,
            limit=1000
        )
        dialogue_count = len(history)
    
    # 获取记忆统计
    memory_stats = self.memory.get_stats()
    
    return {
        "npc_id": self.npc_id,
        "npc_name": self.personality.name,
        "npc_role": self.personality.role,
        "affinity": affinity.to_dict(),
        "dialogue_count": dialogue_count,
        "memory_stats": memory_stats,
        "has_active_session": player_id in self.sessions
    }


def get_greeting(self, player_id: str) -> str:
    """
    获取问候语
    
    根据好感度返回不同的问候语
    
    Args:
        player_id: 玩家ID
    
    Returns:
        str: 问候语
    """
    affinity = self.relationship.get_affinity(
        npc_id=self.npc_id,
        player_id=player_id
    )
    
    # 根据好感度等级返回不同问候语
    greetings = {
        "陌生": f"你好，我是{self.personality.name}。",
        "认识": f"哦，是你啊。有什么事吗？",
        "友好": f"嘿！很高兴见到你！",
        "信任": f"我的朋友！又见面了！",
        "挚友": f"老朋友！我正想着你呢！"
    }
    
    greeting = greetings.get(affinity.level, self.personality.greeting)
    
    return greeting
```

### 会话管理方法

```python
def clear_session(self, player_id: str):
    """
    清除会话历史
    
    Args:
        player_id: 玩家ID
    """
    if player_id in self.sessions:
        del self.sessions[player_id]
        print(f"[NPCAgent] 清除会话: {player_id}")


def get_session_history(self, player_id: str) -> List[Dict]:
    """
    获取会话历史
    
    Args:
        player_id: 玩家ID
    
    Returns:
        List[Dict]: 会话历史
    """
    return self.sessions.get(player_id, [])
```

---

## 🚀 高级特性

### 1. 多玩家并发支持

```python
import threading

class NPCAgent:
    def __init__(self, ...):
        # ... 其他初始化 ...
        
        # 线程锁
        self._session_lock = threading.Lock()
        self._affinity_lock = threading.Lock()
    
    def chat(self, player_id: str, message: str, ...) -> Dict:
        """线程安全的对话处理"""
        
        # 会话锁
        with self._session_lock:
            # 加载会话历史
            conversation_history = self.sessions.get(player_id, [])
        
        # 构建上下文和生成回复 (不需要锁)
        context = self._build_context(message, player_id)
        reply = self._generate_reply(context, message)
        
        # 好感度锁
        with self._affinity_lock:
            new_affinity = self._update_affinity(player_id, message, reply)
        
        # 保存对话 (数据库自带锁)
        self._save_dialogue(player_id, message, reply, session_id, now)
        
        # 更新会话历史
        with self._session_lock:
            self.sessions[player_id] = conversation_history + [...]
        
        return {...}
```

### 2. 异步对话处理

```python
import asyncio
from typing import Coroutine

class AsyncNPCAgent(NPCAgent):
    """异步版本的NPCAgent"""
    
    async def chat_async(
        self,
        player_id: str,
        message: str,
        session_id: str = None
    ) -> Dict[str, Any]:
        """
        异步对话处理
        
        适用于Web服务器等异步环境
        """
        # 并行执行检索任务
        memory_task = asyncio.create_task(
            self._search_memory_async(message)
        )
        knowledge_task = asyncio.create_task(
            self._search_knowledge_async(message)
        )
        affinity_task = asyncio.create_task(
            self._get_affinity_async(player_id)
        )
        
        # 等待所有任务完成
        memories, knowledge, affinity = await asyncio.gather(
            memory_task,
            knowledge_task,
            affinity_task
        )
        
        # 构建上下文
        context = self.context_builder.build(
            user_message=message,
            memories=memories,
            knowledge=knowledge,
            affinity=affinity
        )
        
        # 异步调用LLM
        reply = await self._generate_reply_async(context, message)
        
        # 异步保存
        await self._save_dialogue_async(player_id, message, reply)
        
        return {"reply": reply, ...}
```

### 3. 流式回复

```python
def chat_stream(
    self,
    player_id: str,
    message: str
) -> Generator[str, None, None]:
    """
    流式对话 (逐字返回)
    
    适用于实时显示NPC打字效果
    
    Yields:
        str: 回复的每个token
    """
    # 构建上下文
    context = self._build_context(message, player_id)
    
    # 流式调用LLM
    full_reply = ""
    for token in self.llm.stream(context + "\n" + message):
        full_reply += token
        yield token
    
    # 保存完整回复
    self._save_dialogue(player_id, message, full_reply, ...)
    self._update_affinity(player_id, message, full_reply)
```

### 4. 工具调用 (Function Calling)

```python
def chat_with_tools(
    self,
    player_id: str,
    message: str
) -> Dict[str, Any]:
    """
    支持工具调用的对话
    
    NPC可以主动调用工具 (如查询天气、计算等)
    """
    # 定义可用工具
    tools = [
        {
            "name": "search_knowledge",
            "description": "搜索知识库",
            "parameters": {"query": "str"}
        },
        {
            "name": "take_note",
            "description": "记笔记",
            "parameters": {"title": "str", "content": "str"}
        }
    ]
    
    # 构建上下文 (包含工具描述)
    context = self._build_context(message, player_id)
    context += "\n\n可用工具:\n" + json.dumps(tools, ensure_ascii=False)
    
    # LLM生成回复 (可能包含工具调用)
    reply = self._generate_reply(context, message)
    
    # 解析工具调用
    if "<tool_call>" in reply:
        tool_result = self._execute_tool_call(reply)
        # 将工具结果反馈给LLM
        final_reply = self._generate_reply(
            context + f"\n工具结果: {tool_result}",
            message
        )
        return {"reply": final_reply, "tool_used": True}
    
    return {"reply": reply, "tool_used": False}
```

---

## 📊 性能优化

### 1. 缓存机制

```python
from functools import lru_cache
import hashlib

class NPCAgent:
    def __init__(self, ...):
        # ... 其他初始化 ...
        
        # 上下文缓存
        self._context_cache = {}
        self._cache_ttl = 300  # 5分钟
    
    def _build_context_cached(
        self,
        message: str,
        player_id: str
    ) -> str:
        """带缓存的上下文构建"""
        
        # 生成缓存键
        cache_key = hashlib.md5(
            f"{player_id}:{message}".encode()
        ).hexdigest()
        
        # 检查缓存
        if cache_key in self._context_cache:
            cached_data, timestamp = self._context_cache[cache_key]
            if time.time() - timestamp < self._cache_ttl:
                return cached_data
        
        # 构建上下文
        context = self._build_context(message, player_id)
        
        # 保存到缓存
        self._context_cache[cache_key] = (context, time.time())
        
        return context
```

### 2. 批量处理

```python
def chat_batch(
    self,
    requests: List[Dict[str, str]]
) -> List[Dict[str, Any]]:
    """
    批量处理对话
    
    Args:
        requests: [{"player_id": "p1", "message": "hi"}, ...]
    
    Returns:
        List[Dict]: 回复列表
    """
    results = []
    
    # 批量检索记忆
    all_memories = self.memory.search_batch(
        [req["message"] for req in requests]
    )
    
    # 批量检索知识
    all_knowledge = []
    if self.rag:
        all_knowledge = self.rag.search_batch(
            [req["message"] for req in requests]
        )
    
    # 逐个生成回复
    for i, req in enumerate(requests):
        context = self.context_builder.build(
            user_message=req["message"],
            memories=all_memories[i],
            knowledge=all_knowledge[i] if all_knowledge else []
        )
        
        reply = self._generate_reply(context, req["message"])
        
        results.append({
            "player_id": req["player_id"],
            "reply": reply
        })
    
    return results
```

---

## 🎓 使用示例

### 基础使用

```python
# 创建NPC
personality = NPCPersonality(
    name="老铁匠",
    role="铁匠",
    age=55,
    traits=["严肃", "专业", "热心"],
    background="在村里打铁30年",
    speech_style="简洁直接",
    knowledge=["锻造", "武器", "盔甲"],
    secrets=["年轻时的冒险故事", "藏宝图的秘密"],
    greeting="需要打造什么吗？"
)

npc = NPCAgent(
    npc_id="blacksmith_001",
    personality=personality,
    llm=ChatOllama(model="qwen2.5:7b")
)

# 对话
result = npc.chat(
    player_id="player_123",
    message="你能帮我打造一把剑吗？"
)

print(result["reply"])
# 输出: "当然可以！你想要什么样的剑？长剑还是短剑？"
```

### 高级使用

```python
# 添加知识
npc.add_knowledge(
    content="精钢剑需要精钢锭x3、皮革x1、宝石x1",
    doc_id="recipe_steel_sword"
)

# 手动添加记忆
npc.remember(
    content="玩家player_123曾经救过我的命",
    memory_type="semantic",
    importance=0.9
)

# 查询状态
status = npc.get_status("player_123")
print(f"好感度: {status['affinity']['level']}")
print(f"对话次数: {status['dialogue_count']}")

# 流式对话
for token in npc.chat_stream("player_123", "告诉我你的故事"):
    print(token, end="", flush=True)
```

---

## 📝 总结

NPCAgent是整个系统的核心，它：

1. **集成所有模块**: 记忆、RAG、上下文、好感度、存储
2. **提供简洁API**: 一行代码完成复杂对话
3. **支持高级特性**: 并发、异步、流式、工具调用
4. **性能优化**: 缓存、批量处理

**下一步**: 阅读 `11-CREATE_NPC.md` 学习如何创建自己的NPC
