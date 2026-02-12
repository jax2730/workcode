# 07 - 上下文构建详解 (ContextBuilder)

> **面向对象**: 系统开发者、维护人员  
> **前置知识**: Python、LangChain、Token计算  
> **相关模块**: MemoryTool, RAGTool, RelationshipManager

## 📋 模块概述

### 职责定义

ContextBuilder 是 NPC 系统的**上下文工程核心**，负责将多源异构信息整合为 LLM 可理解的结构化上下文。

**核心职责**:
1. **信息汇集** (Gather): 从多个数据源收集相关信息
2. **智能选择** (Select): 基于相关性和重要性筛选信息
3. **结构化组织** (Structure): 按照特定格式组织上下文
4. **兜底压缩** (Compress): 当超出限制时进行智能压缩

### 设计理念

**GSSC 流水线模式**:
```
Gather → Select → Structure → Compress
  ↓        ↓         ↓          ↓
汇集     选择      组织       压缩
```

**为什么需要 ContextBuilder?**

```python
# 问题场景
记忆检索结果: 2000 tokens
RAG检索结果: 1500 tokens
对话历史: 800 tokens
人设描述: 300 tokens
好感度信息: 200 tokens
----------------------------
总计: 4800 tokens

# LLM限制
qwen2.5 上下文窗口: 4096 tokens
可用空间: 4096 - 500(预留) = 3596 tokens

# 解决方案
ContextBuilder 智能裁剪到 3596 tokens
同时保持最重要的信息
```

---

## 🏗️ 架构设计

### 类图

```python
┌─────────────────────────────────────────────────────────┐
│                   ContextBuilder                         │
├─────────────────────────────────────────────────────────┤
│ - config: ContextConfig                                  │
│ - memory_tool: MemoryTool                               │
│ - rag_tool: RAGTool                                     │
│ - relationship_manager: RelationshipManager             │
├─────────────────────────────────────────────────────────┤
│ + build_context(message, player_id, npc_id) -> str     │
│ + gather_information(message, player_id) -> List       │
│ + select_relevant(packets, query) -> List              │
│ + structure_context(packets) -> str                    │
│ + compress_context(context, max_tokens) -> str         │
├─────────────────────────────────────────────────────────┤
│ - _calculate_relevance(packet, query) -> float         │
│ - _calculate_priority(packet) -> int                   │
│ - _estimate_tokens(text) -> int                        │
│ - _truncate_text(text, max_tokens) -> str              │
└─────────────────────────────────────────────────────────┘
```

### 数据结构

#### ContextPacket (信息包)

```python
@dataclass
class ContextPacket:
    """候选信息包"""
    content: str                        # 内容
    source: str                         # 来源: system/memory/rag/history/notes/custom
    timestamp: datetime                 # 时间戳
    token_count: int                    # Token数量
    relevance_score: float              # 相关性得分 (0-1)
    priority: int                       # 优先级 (0-10)
    metadata: Dict[str, Any]            # 元数据
    
    def __post_init__(self):
        if self.token_count == 0:
            self.token_count = self._estimate_tokens(self.content)
        self.relevance_score = max(0.0, min(1.0, self.relevance_score))
    
    @staticmethod
    def _estimate_tokens(text: str) -> int:
        """估算Token数量 (简化版: 1 token ≈ 1.5 字符)"""
        return int(len(text) / 1.5)
    
    def to_dict(self) -> dict:
        return {
            "content": self.content,
            "source": self.source,
            "timestamp": self.timestamp.isoformat(),
            "token_count": self.token_count,
            "relevance_score": self.relevance_score,
            "priority": self.priority,
            "metadata": self.metadata
        }
```

#### ContextConfig (配置)

```python
@dataclass
class ContextConfig:
    """上下文构建配置"""
    # Token预算
    max_tokens: int = 3000              # 最大token数
    reserve_ratio: float = 0.2          # 系统指令预留比例
    
    # 相关性阈值
    min_relevance: float = 0.1          # 最低相关性阈值
    
    # 权重配置
    recency_weight: float = 0.3         # 时间近因性权重
    relevance_weight: float = 0.7       # 相关性权重
    
    # 各来源的token预算分配
    memory_budget: float = 0.3          # 记忆预算 (30%)
    rag_budget: float = 0.25            # RAG预算 (25%)
    history_budget: float = 0.2         # 对话历史预算 (20%)
    notes_budget: float = 0.1           # 笔记预算 (10%)
    custom_budget: float = 0.15         # 自定义预算 (15%)
    
    # 压缩配置
    enable_compression: bool = True     # 是否启用压缩
    compression_ratio: float = 0.7      # 压缩比例
    
    # 角色描述
    role_description: str = ""          # NPC人设描述
    
    def __post_init__(self):
        # 验证权重和为1
        total = self.recency_weight + self.relevance_weight
        if abs(total - 1.0) > 0.01:
            self.recency_weight /= total
            self.relevance_weight /= total
        
        # 验证预算和为1
        budget_total = (self.memory_budget + self.rag_budget + 
                       self.history_budget + self.notes_budget + 
                       self.custom_budget)
        if abs(budget_total - 1.0) > 0.01:
            raise ValueError(f"预算总和必须为1.0，当前为{budget_total}")
```

---

## 🔄 核心流程

### 1. Gather (信息汇集)

#### 流程图

```
用户消息: "你能帮我打造一把剑吗？"
    ↓
┌─────────────────────────────────────────────────────────┐
│              gather_information()                        │
├─────────────────────────────────────────────────────────┤
│                                                          │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐ │
│  │ 系统信息      │  │ 记忆检索      │  │ RAG检索      │ │
│  │ (人设描述)    │  │ (4层记忆)     │  │ (知识库)     │ │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘ │
│         │                  │                  │          │
│         ↓                  ↓                  ↓          │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐ │
│  │ 对话历史      │  │ 好感度信息    │  │ 笔记信息     │ │
│  │ (最近10条)    │  │ (关系等级)    │  │ (结构笔记)   │ │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘ │
│         │                  │                  │          │
│         └──────────────────┴──────────────────┘          │
│                            ↓                              │
│                  List[ContextPacket]                     │
│                  (候选信息包列表)                         │
└─────────────────────────────────────────────────────────┘
```

#### 实现代码

```python
def gather_information(
    self,
    message: str,
    player_id: str,
    npc_id: str,
    extra_context: Dict[str, Any] = None
) -> List[ContextPacket]:
    """
    汇集多源信息
    
    Args:
        message: 用户消息
        player_id: 玩家ID
        npc_id: NPC ID
        extra_context: 额外上下文
    
    Returns:
        List[ContextPacket]: 候选信息包列表
    """
    packets = []
    
    # 1. 系统信息 (人设描述)
    if self.config.role_description:
        packets.append(ContextPacket(
            content=self.config.role_description,
            source="system",
            timestamp=datetime.now(),
            relevance_score=1.0,  # 人设始终相关
            priority=10,          # 最高优先级
            metadata={"type": "role_description"}
        ))
    
    # 2. 记忆检索
    if self.memory_tool:
        # 2.1 工作记忆 (当前会话)
        working_memories = self.memory_tool.execute(
            "search",
            query=message,
            memory_type="working",
            user_id=player_id,
            limit=5
        )
        for mem in working_memories:
            packets.append(ContextPacket(
                content=mem.content,
                source="memory_working",
                timestamp=mem.timestamp,
                relevance_score=mem.relevance_score,
                priority=8,
                metadata={"memory_type": "working", "importance": mem.importance}
            ))
        
        # 2.2 情景记忆 (互动历史)
        episodic_memories = self.memory_tool.execute(
            "search",
            query=message,
            memory_type="episodic",
            user_id=player_id,
            limit=5
        )
        for mem in episodic_memories:
            packets.append(ContextPacket(
                content=mem.content,
                source="memory_episodic",
                timestamp=mem.timestamp,
                relevance_score=mem.relevance_score,
                priority=7,
                metadata={"memory_type": "episodic", "importance": mem.importance}
            ))
        
        # 2.3 语义记忆 (知识)
        semantic_memories = self.memory_tool.execute(
            "search",
            query=message,
            memory_type="semantic",
            limit=3
        )
        for mem in semantic_memories:
            packets.append(ContextPacket(
                content=mem.content,
                source="memory_semantic",
                timestamp=mem.timestamp,
                relevance_score=mem.relevance_score,
                priority=6,
                metadata={"memory_type": "semantic", "importance": mem.importance}
            ))
        
        # 2.4 感知记忆 (环境)
        perceptual_memories = self.memory_tool.execute(
            "get_current_perception"
        )
        if perceptual_memories:
            packets.append(ContextPacket(
                content=perceptual_memories,
                source="memory_perceptual",
                timestamp=datetime.now(),
                relevance_score=0.5,
                priority=5,
                metadata={"memory_type": "perceptual"}
            ))
    
    # 3. RAG检索 (知识库)
    if self.rag_tool:
        rag_results = self.rag_tool.search(message, top_k=3)
        for result in rag_results:
            packets.append(ContextPacket(
                content=result.content,
                source="rag",
                timestamp=datetime.now(),
                relevance_score=result.score,
                priority=6,
                metadata={
                    "doc_id": result.doc_id,
                    "doc_metadata": result.metadata
                }
            ))
    
    # 4. 对话历史
    if hasattr(self, 'dialogue_storage'):
        history = self.dialogue_storage.get_session_messages(
            session_id=f"{npc_id}_{player_id}",
            limit=10
        )
        for msg in history[-5:]:  # 最近5条
            packets.append(ContextPacket(
                content=f"{msg.role}: {msg.content}",
                source="history",
                timestamp=datetime.fromisoformat(msg.timestamp),
                relevance_score=0.7,
                priority=7,
                metadata={"role": msg.role}
            ))
    
    # 5. 好感度信息
    if self.relationship_manager:
        affinity = self.relationship_manager.get_affinity(npc_id, player_id)
        affinity_info = f"当前好感度: {affinity.level.value} ({affinity.score}/100)"
        
        # 添加可分享的秘密
        if affinity.score >= 60:
            secrets = self._get_unlocked_secrets(affinity)
            if secrets:
                affinity_info += f"\n可分享秘密: {secrets}"
        
        packets.append(ContextPacket(
            content=affinity_info,
            source="relationship",
            timestamp=datetime.now(),
            relevance_score=0.8,
            priority=7,
            metadata={"affinity_level": affinity.level.value, "score": affinity.score}
        ))
    
    # 6. 笔记信息
    if self.note_tool:
        notes = self.note_tool.execute(
            "search",
            query=message,
            limit=2
        )
        for note in notes:
            packets.append(ContextPacket(
                content=note.content,
                source="notes",
                timestamp=note.timestamp,
                relevance_score=note.relevance_score,
                priority=5,
                metadata={"note_type": note.note_type, "tags": note.tags}
            ))
    
    # 7. 额外上下文
    if extra_context:
        for key, value in extra_context.items():
            packets.append(ContextPacket(
                content=f"{key}: {value}",
                source="custom",
                timestamp=datetime.now(),
                relevance_score=0.5,
                priority=4,
                metadata={"custom_key": key}
            ))
    
    return packets
```

---

### 2. Select (智能选择)

#### 选择策略

**综合评分公式**:
```python
final_score = (
    relevance_score * relevance_weight +
    recency_score * recency_weight +
    priority_bonus
)

其中:
- relevance_score: 相关性得分 (0-1)
- recency_score: 时间近因性得分 (0-1)
- priority_bonus: 优先级加成 (0-1)
```

#### 实现代码

```python
def select_relevant(
    self,
    packets: List[ContextPacket],
    query: str,
    max_tokens: int = None
) -> List[ContextPacket]:
    """
    智能选择相关信息
    
    Args:
        packets: 候选信息包列表
        query: 查询文本
        max_tokens: 最大token数
    
    Returns:
        List[ContextPacket]: 选中的信息包列表
    """
    if max_tokens is None:
        max_tokens = int(self.config.max_tokens * (1 - self.config.reserve_ratio))
    
    # 1. 过滤低相关性信息
    filtered_packets = [
        p for p in packets 
        if p.relevance_score >= self.config.min_relevance
    ]
    
    # 2. 计算综合得分
    now = datetime.now()
    for packet in filtered_packets:
        # 2.1 相关性得分 (已有)
        relevance_score = packet.relevance_score
        
        # 2.2 时间近因性得分
        time_diff = (now - packet.timestamp).total_seconds()
        recency_score = math.exp(-time_diff / 3600)  # 1小时衰减
        
        # 2.3 优先级加成
        priority_bonus = packet.priority / 10.0
        
        # 2.4 综合得分
        packet.final_score = (
            relevance_score * self.config.relevance_weight +
            recency_score * self.config.recency_weight +
            priority_bonus * 0.1
        )
    
    # 3. 按综合得分排序
    sorted_packets = sorted(
        filtered_packets,
        key=lambda p: p.final_score,
        reverse=True
    )
    
    # 4. 贪心选择 (在token限制内选择最多信息)
    selected = []
    total_tokens = 0
    
    # 4.1 按来源分配预算
    budget_by_source = {
        "system": int(max_tokens * 0.15),
        "memory_working": int(max_tokens * self.config.memory_budget * 0.3),
        "memory_episodic": int(max_tokens * self.config.memory_budget * 0.4),
        "memory_semantic": int(max_tokens * self.config.memory_budget * 0.2),
        "memory_perceptual": int(max_tokens * self.config.memory_budget * 0.1),
        "rag": int(max_tokens * self.config.rag_budget),
        "history": int(max_tokens * self.config.history_budget),
        "relationship": int(max_tokens * 0.05),
        "notes": int(max_tokens * self.config.notes_budget),
        "custom": int(max_tokens * self.config.custom_budget)
    }
    
    used_by_source = {source: 0 for source in budget_by_source}
    
    # 4.2 按得分选择，同时考虑预算
    for packet in sorted_packets:
        source = packet.source
        budget = budget_by_source.get(source, 0)
        used = used_by_source.get(source, 0)
        
        # 检查是否超出该来源的预算
        if used + packet.token_count <= budget:
            selected.append(packet)
            total_tokens += packet.token_count
            used_by_source[source] = used + packet.token_count
        
        # 检查是否超出总预算
        if total_tokens >= max_tokens:
            break
    
    return selected
```

---

### 3. Structure (结构化组织)

#### 上下文模板

```python
CONTEXT_TEMPLATE = """
【角色设定】
{role_description}

【当前状态】
{current_state}

【相关记忆】
{memories}

【相关知识】
{knowledge}

【对话历史】
{history}

【当前情境】
时间: {current_time}
地点: {location}
氛围: {atmosphere}

【指令】
请基于以上信息，以{npc_name}的身份回复玩家。
保持角色一致性，考虑好感度和历史互动。
"""
```

#### 实现代码

```python
def structure_context(
    self,
    packets: List[ContextPacket],
    npc_name: str,
    player_id: str
) -> str:
    """
    结构化组织上下文
    
    Args:
        packets: 选中的信息包列表
        npc_name: NPC名称
        player_id: 玩家ID
    
    Returns:
        str: 结构化的上下文文本
    """
    # 按来源分组
    grouped = {}
    for packet in packets:
        source = packet.source
        if source not in grouped:
            grouped[source] = []
        grouped[source].append(packet)
    
    # 构建各部分
    sections = {}
    
    # 1. 角色设定
    if "system" in grouped:
        sections["role_description"] = grouped["system"][0].content
    else:
        sections["role_description"] = f"你是{npc_name}"
    
    # 2. 当前状态
    current_state_parts = []
    if "relationship" in grouped:
        current_state_parts.append(grouped["relationship"][0].content)
    if "memory_perceptual" in grouped:
        current_state_parts.append(grouped["memory_perceptual"][0].content)
    sections["current_state"] = "\n".join(current_state_parts) if current_state_parts else "正常"
    
    # 3. 相关记忆
    memory_parts = []
    for mem_type in ["memory_working", "memory_episodic", "memory_semantic"]:
        if mem_type in grouped:
            for packet in grouped[mem_type]:
                memory_parts.append(f"- {packet.content}")
    sections["memories"] = "\n".join(memory_parts) if memory_parts else "无相关记忆"
    
    # 4. 相关知识
    if "rag" in grouped:
        knowledge_parts = [f"- {p.content}" for p in grouped["rag"]]
        sections["knowledge"] = "\n".join(knowledge_parts)
    else:
        sections["knowledge"] = "无相关知识"
    
    # 5. 对话历史
    if "history" in grouped:
        history_parts = [p.content for p in grouped["history"]]
        sections["history"] = "\n".join(history_parts)
    else:
        sections["history"] = "首次对话"
    
    # 6. 当前情境
    sections["current_time"] = datetime.now().strftime("%Y-%m-%d %H:%M")
    sections["location"] = "未知"  # 可从感知记忆中提取
    sections["atmosphere"] = "正常"  # 可从感知记忆中提取
    sections["npc_name"] = npc_name
    
    # 7. 填充模板
    context = CONTEXT_TEMPLATE.format(**sections)
    
    return context
```

---

### 4. Compress (兜底压缩)

#### 压缩策略

```python
压缩优先级 (从低到高):
1. 笔记信息 (notes)
2. 自定义信息 (custom)
3. 感知记忆 (perceptual)
4. 语义记忆 (semantic)
5. RAG知识 (rag)
6. 情景记忆 (episodic)
7. 对话历史 (history)
8. 工作记忆 (working)
9. 好感度信息 (relationship)
10. 角色设定 (system) - 不压缩
```

#### 实现代码

```python
def compress_context(
    self,
    context: str,
    max_tokens: int
) -> str:
    """
    兜底压缩上下文
    
    Args:
        context: 原始上下文
        max_tokens: 最大token数
    
    Returns:
        str: 压缩后的上下文
    """
    current_tokens = self._estimate_tokens(context)
    
    if current_tokens <= max_tokens:
        return context
    
    if not self.config.enable_compression:
        # 不启用压缩，直接截断
        return self._truncate_text(context, max_tokens)
    
    # 解析上下文各部分
    sections = self._parse_context_sections(context)
    
    # 压缩策略
    compression_order = [
        "notes",
        "custom",
        "memory_perceptual",
        "memory_semantic",
        "knowledge",
        "memory_episodic",
        "history",
        "memory_working",
        "current_state"
    ]
    
    # 逐步压缩
    for section_name in compression_order:
        if section_name not in sections:
            continue
        
        # 压缩该部分
        original = sections[section_name]
        compressed = self._compress_section(
            original,
            ratio=self.config.compression_ratio
        )
        sections[section_name] = compressed
        
        # 重新构建上下文
        context = self._rebuild_context(sections)
        current_tokens = self._estimate_tokens(context)
        
        if current_tokens <= max_tokens:
            break
    
    # 如果还是超出，最后截断
    if current_tokens > max_tokens:
        context = self._truncate_text(context, max_tokens)
    
    return context

def _compress_section(
    self,
    text: str,
    ratio: float = 0.7
) -> str:
    """
    压缩文本段落
    
    策略:
    1. 提取关键句子
    2. 移除冗余信息
    3. 简化表达
    """
    sentences = text.split('\n')
    
    # 计算每个句子的重要性
    sentence_scores = []
    for sent in sentences:
        score = self._calculate_sentence_importance(sent)
        sentence_scores.append((sent, score))
    
    # 按重要性排序
    sentence_scores.sort(key=lambda x: x[1], reverse=True)
    
    # 选择top-k句子
    target_count = int(len(sentences) * ratio)
    selected = sentence_scores[:target_count]
    
    # 恢复原始顺序
    selected_sents = [s[0] for s in selected]
    original_order = [s for s in sentences if s in selected_sents]
    
    return '\n'.join(original_order)

def _calculate_sentence_importance(self, sentence: str) -> float:
    """计算句子重要性"""
    score = 0.0
    
    # 1. 长度加分 (但不要太长)
    length = len(sentence)
    if 10 < length < 100:
        score += 0.3
    
    # 2. 关键词加分
    keywords = ["重要", "关键", "必须", "秘密", "特殊"]
    for kw in keywords:
        if kw in sentence:
            score += 0.2
    
    # 3. 数字和具体信息加分
    if any(c.isdigit() for c in sentence):
        score += 0.1
    
    # 4. 问号和感叹号加分 (情感信息)
    if '?' in sentence or '！' in sentence:
        score += 0.1
    
    return score
```

---

## 🎯 完整使用示例

```python
from npc_system import (
    ContextBuilder,
    ContextConfig,
    MemoryTool,
    RAGTool,
    RelationshipManager
)

# 1. 创建配置
config = ContextConfig(
    max_tokens=3000,
    reserve_ratio=0.2,
    memory_budget=0.3,
    rag_budget=0.25,
    history_budget=0.2,
    role_description="你是一位经验丰富的老铁匠，性格严肃但专业。"
)

# 2. 创建工具
memory_tool = MemoryTool(...)
rag_tool = RAGTool(...)
relationship_manager = RelationshipManager(...)

# 3. 创建ContextBuilder
builder = ContextBuilder(
    config=config,
    memory_tool=memory_tool,
    rag_tool=rag_tool,
    relationship_manager=relationship_manager
)

# 4. 构建上下文
context = builder.build_context(
    message="你能帮我打造一把剑吗？",
    player_id="player_001",
    npc_id="blacksmith"
)

print(f"上下文长度: {builder._estimate_tokens(context)} tokens")
print(f"上下文内容:\n{context}")

# 5. 发送给LLM
from langchain_ollama import ChatOllama

llm = ChatOllama(model="qwen2.5")
response = llm.invoke([
    {"role": "system", "content": context},
    {"role": "user", "content": "你能帮我打造一把剑吗？"}
])

print(f"NPC回复: {response.content}")
```

---

继续阅读下一部分...
