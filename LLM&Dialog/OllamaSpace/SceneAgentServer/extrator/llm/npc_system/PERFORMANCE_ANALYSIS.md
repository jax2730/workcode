# NPC系统性能分析与优化

## 问题1：重启后记忆丢失

### 根本原因
每次启动 `npc_chat.py` 都会生成新的 `player_id`，导致无法查询到之前的对话记录。

```python
# 旧代码 (有问题)
self.player_id = f"player_{uuid.uuid4().hex[:8]}"  # 每次都是新ID
```

### 解决方案
将 `player_id` 持久化到文件 `npc_data/.player_id`：

```python
# 新代码 (已修复)
def _load_or_create_player_id(self) -> str:
    """加载或创建持久化的player_id"""
    player_id_file = Path(self.data_dir) / ".player_id"
    
    if player_id_file.exists():
        player_id = player_id_file.read_text().strip()
        return player_id
    
    # 首次运行，创建新ID
    player_id = f"player_{uuid.uuid4().hex[:8]}"
    player_id_file.write_text(player_id)
    return player_id
```

### 存储流程说明

```
对话流程:
1. 用户输入消息
2. NPCAgent.chat() 处理
3. 保存到 SQLite (dialogue_history.db)
   - dialogue_messages 表: 详细消息记录
   - message_store 表: LangChain兼容格式
4. 保存到 Markdown (memories/dialogues/{npc_id}/{player_id}/)
5. 保存到内存 (self.sessions[player_id])

查询流程:
1. get_dialogue_history(player_id) 
2. 从 SQLite 查询: WHERE npc_id=? AND player_id=?
3. 返回消息列表

问题: player_id 不一致 → 查询不到历史记录
解决: 持久化 player_id → 每次启动使用相同ID
```

---

## 问题2：响应速度慢

### 影响因素分析

#### 1. **NPC系统 vs General Chat 对比**

| 组件 | General Chat | NPC System | 性能影响 |
|------|-------------|------------|---------|
| LLM调用 | ✅ 1次 | ✅ 1次 | 相同 |
| 记忆检索 | ❌ 无 | ✅ 4层记忆 | +0.1-0.5s |
| RAG检索 | ❌ 无 | ✅ 知识库检索 | +0.2-1.0s |
| 上下文构建 | ❌ 简单 | ✅ GSSC流水线 | +0.1-0.3s |
| 好感度计算 | ❌ 无 | ✅ 关系管理 | +0.05s |
| 对话存储 | ✅ SQLite | ✅ SQLite + Markdown | +0.1-0.2s |
| 笔记生成 | ❌ 无 | ✅ NoteTool | +0.05s |

**预估差异**: NPC系统比General Chat慢 **0.6-2.1秒**

#### 2. **核心耗时操作**

```python
# 1. 记忆检索 (MemoryTool)
- 工作记忆: 内存查询 (~0.01s)
- 情景记忆: SQLite查询 + 相似度计算 (~0.1-0.3s)
- 语义记忆: SQLite查询 + 向量检索 (~0.1-0.3s)
- 感知记忆: 内存查询 (~0.01s)

# 2. RAG检索 (RAGTool)
- 文档加载: 读取文件 (~0.05-0.2s)
- 向量检索: FAISS/余弦相似度 (~0.1-0.5s)
- 重排序: 相关性计算 (~0.05-0.3s)

# 3. 上下文构建 (ContextBuilder)
- 信息汇集: 多源数据整合 (~0.05s)
- 智能选择: 相关性排序 (~0.05s)
- 结构化: 格式化输出 (~0.01s)
- 压缩: Token计数和裁剪 (~0.05s)

# 4. LLM推理 (最大耗时)
- qwen2.5模型推理: 1-5秒 (取决于上下文长度)
```

#### 3. **上下文长度影响**

```
General Chat:
- 系统提示: ~100 tokens
- 历史消息: ~500 tokens (最近10条)
- 用户输入: ~50 tokens
总计: ~650 tokens → 推理快

NPC System:
- 系统提示: ~200 tokens (人设描述)
- 记忆上下文: ~500 tokens (检索结果)
- RAG知识: ~300 tokens (知识库)
- 历史消息: ~500 tokens
- 好感度信息: ~100 tokens
- 用户输入: ~50 tokens
总计: ~1650 tokens → 推理慢 (2.5倍)
```

---

## 优化方案

### 方案1: 缓存优化 (不影响功能)

```python
class NPCAgent:
    def __init__(self):
        # 缓存最近的记忆检索结果
        self.memory_cache = {}
        self.cache_ttl = 300  # 5分钟
    
    def _get_cached_memories(self, query: str):
        cache_key = f"{query}_{int(time.time() / self.cache_ttl)}"
        if cache_key in self.memory_cache:
            return self.memory_cache[cache_key]
        
        # 执行检索
        memories = self.memory.search(query)
        self.memory_cache[cache_key] = memories
        return memories
```

**预期提升**: 第2次及以后对话快 **0.2-0.5秒**

### 方案2: 异步并行检索 (不影响功能)

```python
async def _gather_context_async(self, message: str, player_id: str):
    """并行执行记忆检索和RAG检索"""
    tasks = [
        asyncio.create_task(self._search_memories_async(message)),
        asyncio.create_task(self._search_rag_async(message)),
        asyncio.create_task(self._get_history_async(player_id))
    ]
    
    results = await asyncio.gather(*tasks)
    return results
```

**预期提升**: 快 **30-50%** (0.3-0.8秒)

### 方案3: 智能上下文裁剪 (不影响功能)

```python
class ContextBuilder:
    def smart_select(self, packets: List[ContextPacket], 
                     max_tokens: int) -> List[ContextPacket]:
        """智能选择最相关的信息"""
        # 1. 按相关性排序
        sorted_packets = sorted(packets, 
                               key=lambda p: p.relevance_score, 
                               reverse=True)
        
        # 2. 贪心选择
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

**预期提升**: 减少上下文长度 **20-40%**，推理快 **0.3-1.0秒**

### 方案4: 延迟写入 (不影响功能)

```python
class NPCAgent:
    def chat(self, player_id: str, message: str):
        # ... 生成回复 ...
        
        # 异步保存 (不阻塞返回)
        asyncio.create_task(self._save_dialogue_async(
            player_id, message, reply
        ))
        
        return {"reply": reply, ...}
```

**预期提升**: 快 **0.1-0.2秒**

### 方案5: 模型量化 (需要重新部署)

```bash
# 使用量化模型
ollama pull qwen2.5:7b-instruct-q4_K_M  # 4-bit量化

# 或使用更小的模型
ollama pull qwen2.5:3b-instruct  # 3B参数
```

**预期提升**: 推理快 **40-60%** (1-3秒)，但可能影响质量

### 方案6: 分层响应 (不影响功能)

```python
def chat_with_streaming(self, player_id: str, message: str):
    """流式响应: 先返回快速回复，再补充详细信息"""
    
    # 第1阶段: 快速回复 (不检索记忆)
    quick_reply = self._generate_quick_reply(message)
    yield {"type": "quick", "content": quick_reply}
    
    # 第2阶段: 检索记忆和RAG (后台)
    context = self._build_full_context(message, player_id)
    
    # 第3阶段: 完整回复
    full_reply = self._generate_full_reply(context, message)
    yield {"type": "full", "content": full_reply}
```

**用户体验**: 立即看到回复 (0.5秒)，然后看到完整回复 (2秒)

---

## 推荐优化组合

### 🎯 方案A: 保守优化 (不改变架构)

1. ✅ 缓存优化 (方案1)
2. ✅ 智能上下文裁剪 (方案3)
3. ✅ 延迟写入 (方案4)

**预期效果**: 快 **0.6-1.7秒**，接近General Chat

### 🚀 方案B: 激进优化 (需要改代码)

1. ✅ 方案A的所有优化
2. ✅ 异步并行检索 (方案2)
3. ✅ 模型量化 (方案5)

**预期效果**: 快 **2-4秒**，比General Chat还快

### 💡 方案C: 用户体验优化 (推荐)

1. ✅ 方案A的所有优化
2. ✅ 分层响应 (方案6)

**预期效果**: 用户感知延迟 **<1秒**，实际功能完整

---

## 实施步骤

### 第1步: 测量基线性能

```bash
# 运行对比测试
python -m extrator.llm.general_chat  # 测试General Chat
python -m extrator.llm.npc_system.npc_chat  # 测试NPC System

# 使用 'stats' 命令查看统计
```

### 第2步: 实施方案A (保守优化)

```bash
# 1. 添加缓存 (修改 npc_agent.py)
# 2. 优化上下文构建 (修改 context_builder.py)
# 3. 异步存储 (修改 dialogue_storage.py)
```

### 第3步: 验证效果

```bash
# 再次测试，对比优化前后
python -m extrator.llm.npc_system.npc_chat
# 输入 'stats' 查看平均响应时间
```

### 第4步: 根据结果决定是否实施方案B或C

---

## 性能监控

### 添加详细计时

```python
import time
from functools import wraps

def timing_decorator(func):
    @wraps(func)
    def wrapper(*args, **kwargs):
        start = time.time()
        result = func(*args, **kwargs)
        elapsed = time.time() - start
        print(f"[Timing] {func.__name__}: {elapsed:.3f}s")
        return result
    return wrapper

class NPCAgent:
    @timing_decorator
    def chat(self, player_id: str, message: str):
        # ...
    
    @timing_decorator
    def _build_context(self, message: str, player_id: str):
        # ...
    
    @timing_decorator
    def _generate_reply(self, context: str, message: str):
        # ...
```

### 性能日志

```python
# 记录每次对话的性能数据
performance_log = {
    "timestamp": datetime.now().isoformat(),
    "npc_id": self.npc_id,
    "player_id": player_id,
    "timings": {
        "memory_search": 0.15,
        "rag_search": 0.23,
        "context_build": 0.08,
        "llm_inference": 2.34,
        "storage": 0.12,
        "total": 2.92
    },
    "context_tokens": 1650,
    "response_tokens": 120
}
```

---

## 总结

### 核心问题
1. ✅ **记忆丢失**: player_id不持久化 → 已修复
2. ⚠️ **响应慢**: 功能丰富导致额外耗时 → 可优化

### 性能差异
- General Chat: ~1-2秒 (简单对话)
- NPC System: ~2-4秒 (完整功能)
- 差异原因: 记忆检索 + RAG + 上下文构建

### 优化建议
1. **短期**: 实施方案A (缓存 + 裁剪 + 异步)
2. **中期**: 实施方案C (分层响应)
3. **长期**: 考虑方案B (模型量化)

### 权衡
- 功能完整性 vs 响应速度
- 记忆准确性 vs 检索耗时
- 上下文丰富度 vs 推理速度

**推荐**: 保持长记忆功能，通过缓存和异步优化来提升速度。
