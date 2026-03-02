# 04 - 数据流程详解

> **面向对象**: 系统开发者、架构师  
> **前置知识**: 系统架构、数据库、消息流  
> **相关文档**: [03-架构总览](./03-ARCHITECTURE_OVERVIEW.md)

## 📋 概述

本文档详细说明NPC智能体系统中数据的完整流转过程，包括：
- 对话数据流
- 记忆数据流
- 知识数据流
- 关系数据流
- 存储数据流

---

## 🔄 完整对话流程

### 端到端数据流

```
用户输入
    ↓
[1] 请求接收层
    ↓
[2] 会话管理层
    ↓
[3] 数据汇集层 (Gather)
    ├─→ 记忆检索
    ├─→ RAG检索
    ├─→ 历史加载
    └─→ 好感度查询
    ↓
[4] 数据选择层 (Select)
    ↓
[5] 上下文构建层 (Structure)
    ↓
[6] LLM推理层
    ↓
[7] 响应处理层
    ↓
[8] 数据持久化层
    ├─→ 对话存储
    ├─→ 记忆更新
    └─→ 好感度更新
    ↓
[9] 响应返回层
    ↓
用户收到回复
```

---

## 1️⃣ 请求接收层

### 数据输入

```python
# HTTP请求 (Django API)
POST /api/npc/chat
{
    "npc_id": "blacksmith",
    "player_id": "player_001",
    "message": "你能帮我打造一把剑吗？",
    "session_id": "session_20260117_143022",
    "extra_context": {
        "location": "铁匠铺",
        "time_of_day": "下午"
    }
}

# 或命令行输入
$ python -m extrator.llm.npc_system.npc_chat
You: 你能帮我打造一把剑吗？
```

### 数据验证

```python
def validate_request(data: dict) -> dict:
    """验证请求数据"""
    required_fields = ["npc_id", "player_id", "message"]
    
    # 检查必需字段
    for field in required_fields:
        if field not in data:
            raise ValueError(f"缺少必需字段: {field}")
    
    # 验证数据类型
    if not isinstance(data["message"], str):
        raise TypeError("message必须是字符串")
    
    # 验证长度
    if len(data["message"]) > 1000:
        raise ValueError("消息长度不能超过1000字符")
    
    # 生成session_id (如果没有)
    if "session_id" not in data:
        data["session_id"] = f"session_{datetime.now().strftime('%Y%m%d_%H%M%S')}"
    
    return data
```

### 数据流转

```
HTTP请求 → Django View → NPCManager.chat() → NPCAgent.chat()
```

---

## 2️⃣ 会话管理层

### 会话加载

```python
def load_session(player_id: str, npc_id: str) -> dict:
    """加载会话数据"""
    session_key = f"{npc_id}_{player_id}"
    
    # 1. 从缓存加载
    if session_key in self.sessions:
        session = self.sessions[session_key]
        return session
    
    # 2. 从数据库加载
    session = self._load_session_from_db(session_key)
    
    # 3. 创建新会话
    if not session:
        session = {
            "session_id": session_key,
            "npc_id": npc_id,
            "player_id": player_id,
            "start_time": datetime.now(),
            "message_count": 0,
            "working_memory": []
        }
    
    # 4. 缓存会话
    self.sessions[session_key] = session
    
    return session
```

### 会话数据结构

```python
session = {
    "session_id": "blacksmith_player_001",
    "npc_id": "blacksmith",
    "player_id": "player_001",
    "start_time": "2026-01-17T14:30:00",
    "last_interaction": "2026-01-17T14:35:00",
    "message_count": 5,
    "working_memory": [
        {"role": "user", "content": "你好", "timestamp": "..."},
        {"role": "assistant", "content": "你好，需要什么？", "timestamp": "..."},
        # ... 最近10条
    ],
    "metadata": {
        "location": "铁匠铺",
        "mood": "正常"
    }
}
```

---

## 3️⃣ 数据汇集层 (Gather)

### 并行数据检索

```python
async def gather_all_data(message: str, player_id: str, npc_id: str):
    """并行汇集所有数据"""
    
    # 创建并行任务
    tasks = [
        gather_memory_data(message, player_id, npc_id),
        gather_rag_data(message, npc_id),
        gather_history_data(player_id, npc_id),
        gather_affinity_data(player_id, npc_id),
        gather_perception_data(npc_id)
    ]
    
    # 并行执行
    results = await asyncio.gather(*tasks)
    
    # 合并结果
    all_packets = []
    for result in results:
        all_packets.extend(result)
    
    return all_packets
```

### 3.1 记忆数据流

```
用户消息: "你能帮我打造一把剑吗？"
    ↓
记忆检索请求
    ↓
┌─────────────────────────────────────────┐
│         MemoryTool.search()              │
├─────────────────────────────────────────┤
│                                          │
│  [工作记忆] SQLite查询                   │
│  SELECT * FROM memories                  │
│  WHERE memory_type='working'             │
│    AND user_id='player_001'              │
│    AND content LIKE '%剑%'               │
│  → 返回: 2条记录                         │
│                                          │
│  [情景记忆] SQLite查询 + 相似度计算      │
│  SELECT * FROM memories                  │
│  WHERE memory_type='episodic'            │
│    AND user_id='player_001'              │
│  → 计算相似度                            │
│  → 返回: 5条记录                         │
│                                          │
│  [语义记忆] SQLite查询 + 概念匹配        │
│  SELECT * FROM memories                  │
│  WHERE memory_type='semantic'            │
│    AND concepts LIKE '%武器%'            │
│  → 返回: 3条记录                         │
│                                          │
│  [感知记忆] 内存查询                     │
│  → 返回: 当前环境信息                    │
│                                          │
└─────────────────────────────────────────┘
    ↓
返回: List[MemoryItem]
    ↓
转换为: List[ContextPacket]
```

### 3.2 RAG数据流

```
用户消息: "你能帮我打造一把剑吗？"
    ↓
RAG检索请求
    ↓
┌─────────────────────────────────────────┐
│           RAGTool.search()               │
├─────────────────────────────────────────┤
│                                          │
│  [1] 查询向量化                          │
│  query_vector = embed("打造剑")          │
│  → [0.23, -0.45, 0.67, ..., 0.12]       │
│                                          │
│  [2] 向量检索 (FAISS)                    │
│  index.search(query_vector, k=10)        │
│  → 返回: 10个候选文档ID                  │
│                                          │
│  [3] 加载文档内容                        │
│  docs = load_documents(doc_ids)          │
│  → 返回: 10个文档                        │
│                                          │
│  [4] 重排序                              │
│  rerank(docs, query, metadata)           │
│  → 按综合得分排序                        │
│                                          │
│  [5] 返回Top-K                           │
│  → 返回: 前3个文档                       │
│                                          │
└─────────────────────────────────────────┘
    ↓
返回: List[Document]
    ↓
转换为: List[ContextPacket]
```

### 3.3 历史数据流

```
会话ID: "blacksmith_player_001"
    ↓
历史查询请求
    ↓
┌─────────────────────────────────────────┐
│    DialogueStorage.get_history()         │
├─────────────────────────────────────────┤
│                                          │
│  [1] SQLite查询                          │
│  SELECT * FROM dialogue_messages         │
│  WHERE session_id = ?                    │
│  ORDER BY timestamp DESC                 │
│  LIMIT 10                                │
│                                          │
│  [2] 解析消息                            │
│  messages = parse_messages(rows)         │
│                                          │
│  [3] 格式化                              │
│  formatted = format_for_context(messages)│
│                                          │
└─────────────────────────────────────────┘
    ↓
返回: List[DialogueMessage]
    ↓
转换为: List[ContextPacket]
```

### 3.4 好感度数据流

```
NPC ID: "blacksmith"
Player ID: "player_001"
    ↓
好感度查询请求
    ↓
┌─────────────────────────────────────────┐
│  RelationshipManager.get_affinity()      │
├─────────────────────────────────────────┤
│                                          │
│  [1] 检查缓存                            │
│  cache_key = (npc_id, player_id)         │
│  if cache_key in cache:                  │
│      return cache[cache_key]             │
│                                          │
│  [2] SQLite查询                          │
│  SELECT * FROM relationships             │
│  WHERE npc_id = ? AND player_id = ?      │
│                                          │
│  [3] 应用时间衰减                        │
│  apply_time_decay(affinity)              │
│                                          │
│  [4] 缓存结果                            │
│  cache[cache_key] = affinity             │
│                                          │
└─────────────────────────────────────────┘
    ↓
返回: AffinityInfo
    ↓
转换为: ContextPacket
```

### 数据汇集结果

```python
all_packets = [
    # 系统信息
    ContextPacket(
        content="你是一位经验丰富的老铁匠...",
        source="system",
        priority=10,
        relevance_score=1.0
    ),
    
    # 工作记忆 (2条)
    ContextPacket(
        content="用户询问了铁剑的价格",
        source="memory_working",
        priority=8,
        relevance_score=0.85
    ),
    
    # 情景记忆 (5条)
    ContextPacket(
        content="玩家player_001上次购买了铁剑",
        source="memory_episodic",
        priority=7,
        relevance_score=0.75
    ),
    
    # 语义记忆 (3条)
    ContextPacket(
        content="铁剑需要3块铁锭和1根木棍制作",
        source="memory_semantic",
        priority=6,
        relevance_score=0.90
    ),
    
    # RAG知识 (3条)
    ContextPacket(
        content="【铁剑锻造教程】\n所需材料: 铁锭×3...",
        source="rag",
        priority=6,
        relevance_score=0.95
    ),
    
    # 对话历史 (5条)
    ContextPacket(
        content="user: 你好\nassistant: 你好，需要什么？",
        source="history",
        priority=7,
        relevance_score=0.70
    ),
    
    # 好感度信息
    ContextPacket(
        content="当前好感度: 友好 (55/100)",
        source="relationship",
        priority=7,
        relevance_score=0.80
    )
]

# 总计: ~25个ContextPacket
# 总token数: ~3500 tokens
```

---

## 4️⃣ 数据选择层 (Select)

### 选择算法

```python
def select_relevant_packets(
    packets: List[ContextPacket],
    max_tokens: int = 3000
) -> List[ContextPacket]:
    """智能选择相关信息"""
    
    # 1. 计算综合得分
    for packet in packets:
        packet.final_score = calculate_score(packet)
    
    # 2. 按得分排序
    sorted_packets = sorted(
        packets,
        key=lambda p: p.final_score,
        reverse=True
    )
    
    # 3. 贪心选择
    selected = []
    total_tokens = 0
    
    for packet in sorted_packets:
        if total_tokens + packet.token_count <= max_tokens:
            selected.append(packet)
            total_tokens += packet.token_count
        else:
            break
    
    return selected
```

### 选择结果

```python
selected_packets = [
    # 系统信息 (必选)
    ContextPacket(source="system", tokens=150),
    
    # 高相关性RAG知识
    ContextPacket(source="rag", tokens=400, score=0.95),
    
    # 高相关性语义记忆
    ContextPacket(source="memory_semantic", tokens=200, score=0.90),
    
    # 工作记忆
    ContextPacket(source="memory_working", tokens=150, score=0.85),
    
    # 好感度信息
    ContextPacket(source="relationship", tokens=100, score=0.80),
    
    # 情景记忆
    ContextPacket(source="memory_episodic", tokens=300, score=0.75),
    
    # 对话历史
    ContextPacket(source="history", tokens=500, score=0.70)
]

# 总计: 1800 tokens (在3000限制内)
# 选中: 7/25 个信息包
```

---

## 5️⃣ 上下文构建层 (Structure)

### 结构化组织

```python
context = """
【角色设定】
你是一位经验丰富的老铁匠，性格严肃但专业。

【当前状态】
好感度: 友好 (55/100)
互动次数: 12次

【相关记忆】
- 玩家player_001上次购买了铁剑
- 用户询问了铁剑的价格

【相关知识】
【铁剑锻造教程】
所需材料: 铁锭×3, 木棍×1
制作步骤:
1. 将铁锭放入熔炉加热
2. 锻打成剑身形状
3. 安装木质剑柄
4. 淬火并打磨

【对话历史】
user: 你好
assistant: 你好，需要什么？
user: 铁剑多少钱？
assistant: 铁剑50金币

【当前情境】
时间: 2026-01-17 14:30
地点: 铁匠铺
氛围: 正常

【指令】
请基于以上信息，以老铁匠的身份回复玩家。
保持角色一致性，考虑好感度和历史互动。
"""

# Token数: ~1800
```

---

## 6️⃣ LLM推理层

### 数据发送

```python
# 构建消息
messages = [
    {"role": "system", "content": context},
    {"role": "user", "content": "你能帮我打造一把剑吗？"}
]

# 发送给LLM
response = llm.invoke(messages)

# LLM处理
# Input tokens: ~1850
# Output tokens: ~120
# 推理时间: 2.3秒
```

### LLM响应

```python
response = {
    "content": "当然可以！你想要什么样的剑？普通的铁剑50金币，如果想要更好的精钢剑，需要120金币。制作时间大约需要2天。",
    "metadata": {
        "model": "qwen2.5:7b",
        "tokens": {
            "input": 1850,
            "output": 120
        },
        "time": 2.3
    }
}
```

---

## 7️⃣ 响应处理层

### 好感度计算

```python
# 分析对话内容
delta = calculate_affinity_change(
    message="你能帮我打造一把剑吗？",
    reply="当然可以！你想要什么样的剑？...",
    interaction_type="dialogue"
)

# delta = +2 (正面对话 +1, 积极情感 +1)
```

### 好感度更新

```python
# 更新好感度
old_affinity = get_affinity("blacksmith", "player_001")
# old: 友好 (55/100)

new_affinity = update_affinity(
    npc_id="blacksmith",
    player_id="player_001",
    delta=+2
)
# new: 友好 (57/100)
```

---

## 8️⃣ 数据持久化层

### 8.1 对话存储

```
对话数据
    ↓
┌─────────────────────────────────────────┐
│      DialogueStorage.save()              │
├─────────────────────────────────────────┤
│                                          │
│  [1] SQLite存储                          │
│  INSERT INTO dialogue_messages           │
│  (message_id, session_id, npc_id,        │
│   player_id, role, content, timestamp)   │
│  VALUES (?, ?, ?, ?, ?, ?, ?)            │
│                                          │
│  [2] Markdown存储                        │
│  File: memories/dialogues/blacksmith/    │
│        player_001/20260117.md            │
│  Append: "## 14:30\n用户: ...\nNPC: ..." │
│                                          │
│  [3] 更新会话统计                        │
│  UPDATE dialogue_sessions                │
│  SET message_count = message_count + 2   │
│                                          │
└─────────────────────────────────────────┘
```

### 8.2 记忆更新

```
对话内容
    ↓
重要性评估
    ↓
┌─────────────────────────────────────────┐
│       MemoryTool.add()                   │
├─────────────────────────────────────────┤
│                                          │
│  [1] 添加到工作记忆 (内存)              │
│  working_memory.append({                 │
│      "content": "用户询问打造剑",        │
│      "timestamp": now                    │
│  })                                      │
│                                          │
│  [2] 如果重要 (importance > 0.7)         │
│  添加到情景记忆 (SQLite)                 │
│  INSERT INTO memories                    │
│  (memory_type, content, importance, ...) │
│  VALUES ('episodic', '...', 0.8, ...)    │
│                                          │
│  [3] 提取知识 → 语义记忆                 │
│  如果包含新知识:                         │
│  INSERT INTO memories                    │
│  (memory_type, content, concepts, ...)   │
│  VALUES ('semantic', '...', [...], ...)  │
│                                          │
└─────────────────────────────────────────┘
```

### 8.3 好感度存储

```
好感度变化
    ↓
┌─────────────────────────────────────────┐
│  RelationshipManager.save()              │
├─────────────────────────────────────────┤
│                                          │
│  [1] 更新关系表                          │
│  UPDATE relationships                    │
│  SET score = 57,                         │
│      level = '友好',                     │
│      interaction_count = 13,             │
│      last_interaction = now              │
│  WHERE npc_id = ? AND player_id = ?      │
│                                          │
│  [2] 记录历史                            │
│  INSERT INTO relationship_history        │
│  (npc_id, player_id, old_score,          │
│   new_score, delta, interaction_type)    │
│  VALUES (?, ?, 55, 57, 2, 'dialogue')    │
│                                          │
│  [3] 更新缓存                            │
│  cache[(npc_id, player_id)] = affinity   │
│                                          │
└─────────────────────────────────────────┘
```

---

## 9️⃣ 响应返回层

### 响应数据结构

```python
response = {
    "success": True,
    "reply": "当然可以！你想要什么样的剑？普通的铁剑50金币...",
    "affinity": {
        "level": "友好",
        "score": 57,
        "delta": +2
    },
    "npc_name": "老铁匠",
    "npc_role": "铁匠",
    "session_id": "blacksmith_player_001",
    "metadata": {
        "response_time": 2.8,
        "tokens": {
            "input": 1850,
            "output": 120
        }
    }
}
```

### 数据返回

```
响应数据
    ↓
JSON序列化
    ↓
HTTP响应 / 命令行输出
    ↓
用户收到回复
```

---

## 📊 完整数据流时序图

```
时间轴 (总计: 2.8秒)
│
├─ 0.00s: 接收请求
├─ 0.01s: 验证数据
├─ 0.02s: 加载会话
│
├─ 0.05s: 开始数据汇集 (并行)
│   ├─ 0.05-0.25s: 记忆检索 (0.20s)
│   ├─ 0.05-0.35s: RAG检索 (0.30s)
│   ├─ 0.05-0.15s: 历史加载 (0.10s)
│   └─ 0.05-0.10s: 好感度查询 (0.05s)
├─ 0.35s: 数据汇集完成
│
├─ 0.35-0.45s: 数据选择 (0.10s)
├─ 0.45-0.50s: 上下文构建 (0.05s)
│
├─ 0.50-2.80s: LLM推理 (2.30s) ← 最大耗时
│
├─ 2.80-2.85s: 好感度计算 (0.05s)
├─ 2.85-3.00s: 数据持久化 (0.15s)
│   ├─ SQLite写入 (0.08s)
│   ├─ Markdown写入 (0.05s)
│   └─ 缓存更新 (0.02s)
│
└─ 3.00s: 返回响应
```

---

## 🔗 相关文档

- [03-架构总览](./03-ARCHITECTURE_OVERVIEW.md) - 系统架构
- [05-记忆系统](./05-MEMORY_SYSTEM.md) - 记忆数据流
- [06-RAG系统](./06-RAG_SYSTEM.md) - 知识数据流
- [07-上下文构建](./07-CONTEXT_BUILDER.md) - 上下文数据流
- [08-好感度系统](./08-RELATIONSHIP_SYSTEM.md) - 关系数据流

---

恭喜！你现在完全理解了NPC系统的数据流程！🎉
