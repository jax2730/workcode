# 07 - 上下文构建详解 (第2部分)

## ⚙️ 高级特性

### 1. 动态预算调整

根据实际情况动态调整各来源的token预算。

```python
class AdaptiveContextBuilder(ContextBuilder):
    """自适应上下文构建器"""
    
    def adjust_budget(
        self,
        packets: List[ContextPacket],
        query: str
    ) -> Dict[str, float]:
        """
        动态调整预算分配
        
        策略:
        - 如果某个来源的信息特别相关，增加其预算
        - 如果某个来源没有信息，将其预算分配给其他来源
        """
        # 统计各来源的信息量和平均相关性
        source_stats = {}
        for packet in packets:
            source = packet.source
            if source not in source_stats:
                source_stats[source] = {
                    "count": 0,
                    "total_relevance": 0.0,
                    "total_tokens": 0
                }
            source_stats[source]["count"] += 1
            source_stats[source]["total_relevance"] += packet.relevance_score
            source_stats[source]["total_tokens"] += packet.token_count
        
        # 计算平均相关性
        for source, stats in source_stats.items():
            if stats["count"] > 0:
                stats["avg_relevance"] = stats["total_relevance"] / stats["count"]
            else:
                stats["avg_relevance"] = 0.0
        
        # 调整预算
        adjusted_budget = {}
        base_budget = {
            "memory": self.config.memory_budget,
            "rag": self.config.rag_budget,
            "history": self.config.history_budget,
            "notes": self.config.notes_budget,
            "custom": self.config.custom_budget
        }
        
        # 根据相关性调整
        total_relevance = sum(s["avg_relevance"] for s in source_stats.values())
        if total_relevance > 0:
            for source, stats in source_stats.items():
                weight = stats["avg_relevance"] / total_relevance
                # 基础预算 + 相关性加成
                adjusted_budget[source] = base_budget.get(source, 0.1) * (1 + weight)
        
        # 归一化
        total = sum(adjusted_budget.values())
        if total > 0:
            adjusted_budget = {k: v/total for k, v in adjusted_budget.items()}
        
        return adjusted_budget
```

### 2. 上下文缓存

缓存最近构建的上下文，避免重复计算。

```python
from functools import lru_cache
import hashlib

class CachedContextBuilder(ContextBuilder):
    """带缓存的上下文构建器"""
    
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.cache = {}
        self.cache_ttl = 300  # 5分钟
        self.cache_hits = 0
        self.cache_misses = 0
    
    def build_context(
        self,
        message: str,
        player_id: str,
        npc_id: str,
        **kwargs
    ) -> str:
        """构建上下文 (带缓存)"""
        # 生成缓存键
        cache_key = self._generate_cache_key(message, player_id, npc_id)
        
        # 检查缓存
        if cache_key in self.cache:
            cached_data = self.cache[cache_key]
            age = (datetime.now() - cached_data["timestamp"]).seconds
            
            if age < self.cache_ttl:
                self.cache_hits += 1
                return cached_data["context"]
        
        # 缓存未命中，构建上下文
        self.cache_misses += 1
        context = super().build_context(message, player_id, npc_id, **kwargs)
        
        # 存入缓存
        self.cache[cache_key] = {
            "context": context,
            "timestamp": datetime.now()
        }
        
        # 清理过期缓存
        self._cleanup_cache()
        
        return context
    
    def _generate_cache_key(
        self,
        message: str,
        player_id: str,
        npc_id: str
    ) -> str:
        """生成缓存键"""
        # 使用消息的hash + player_id + npc_id
        msg_hash = hashlib.md5(message.encode()).hexdigest()[:8]
        return f"{npc_id}_{player_id}_{msg_hash}"
    
    def _cleanup_cache(self):
        """清理过期缓存"""
        now = datetime.now()
        expired_keys = []
        
        for key, data in self.cache.items():
            age = (now - data["timestamp"]).seconds
            if age >= self.cache_ttl:
                expired_keys.append(key)
        
        for key in expired_keys:
            del self.cache[key]
    
    def get_cache_stats(self) -> Dict[str, Any]:
        """获取缓存统计"""
        total = self.cache_hits + self.cache_misses
        hit_rate = self.cache_hits / total if total > 0 else 0
        
        return {
            "cache_size": len(self.cache),
            "cache_hits": self.cache_hits,
            "cache_misses": self.cache_misses,
            "hit_rate": f"{hit_rate:.2%}"
        }
```

### 3. 并行信息汇集

使用异步并行加速信息汇集。

```python
import asyncio
from concurrent.futures import ThreadPoolExecutor

class AsyncContextBuilder(ContextBuilder):
    """异步上下文构建器"""
    
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.executor = ThreadPoolExecutor(max_workers=4)
    
    async def gather_information_async(
        self,
        message: str,
        player_id: str,
        npc_id: str
    ) -> List[ContextPacket]:
        """异步汇集信息"""
        tasks = []
        
        # 1. 记忆检索 (并行)
        if self.memory_tool:
            tasks.append(self._search_memory_async(message, player_id))
        
        # 2. RAG检索 (并行)
        if self.rag_tool:
            tasks.append(self._search_rag_async(message))
        
        # 3. 对话历史 (并行)
        if hasattr(self, 'dialogue_storage'):
            tasks.append(self._get_history_async(npc_id, player_id))
        
        # 4. 好感度信息 (并行)
        if self.relationship_manager:
            tasks.append(self._get_affinity_async(npc_id, player_id))
        
        # 等待所有任务完成
        results = await asyncio.gather(*tasks)
        
        # 合并结果
        all_packets = []
        for result in results:
            if result:
                all_packets.extend(result)
        
        return all_packets
    
    async def _search_memory_async(
        self,
        message: str,
        player_id: str
    ) -> List[ContextPacket]:
        """异步搜索记忆"""
        loop = asyncio.get_event_loop()
        
        # 在线程池中执行同步方法
        packets = await loop.run_in_executor(
            self.executor,
            self._search_memory_sync,
            message,
            player_id
        )
        
        return packets
    
    def _search_memory_sync(
        self,
        message: str,
        player_id: str
    ) -> List[ContextPacket]:
        """同步搜索记忆 (在线程池中执行)"""
        packets = []
        
        # 搜索各层记忆
        for memory_type in ["working", "episodic", "semantic"]:
            results = self.memory_tool.execute(
                "search",
                query=message,
                memory_type=memory_type,
                user_id=player_id,
                limit=5
            )
            
            for mem in results:
                packets.append(ContextPacket(
                    content=mem.content,
                    source=f"memory_{memory_type}",
                    timestamp=mem.timestamp,
                    relevance_score=mem.relevance_score,
                    priority=7,
                    metadata={"memory_type": memory_type}
                ))
        
        return packets
    
    # 类似地实现其他异步方法...
```

---

## 📊 性能优化

### 1. Token估算优化

使用更精确的token计数器。

```python
from transformers import AutoTokenizer

class PreciseContextBuilder(ContextBuilder):
    """精确token计数的上下文构建器"""
    
    def __init__(self, *args, tokenizer_name="Qwen/Qwen2.5-7B", **kwargs):
        super().__init__(*args, **kwargs)
        
        # 加载tokenizer
        try:
            self.tokenizer = AutoTokenizer.from_pretrained(tokenizer_name)
            self.use_precise_counting = True
        except Exception as e:
            print(f"[ContextBuilder] 无法加载tokenizer: {e}")
            print("[ContextBuilder] 使用简化的token估算")
            self.tokenizer = None
            self.use_precise_counting = False
    
    def _estimate_tokens(self, text: str) -> int:
        """精确估算token数量"""
        if self.use_precise_counting and self.tokenizer:
            # 使用真实的tokenizer
            tokens = self.tokenizer.encode(text)
            return len(tokens)
        else:
            # 简化估算: 1 token ≈ 1.5 字符
            return int(len(text) / 1.5)
```

### 2. 增量更新

只更新变化的部分，避免重新构建整个上下文。

```python
class IncrementalContextBuilder(ContextBuilder):
    """增量更新的上下文构建器"""
    
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.last_context = {}  # {session_id: context_data}
    
    def build_context_incremental(
        self,
        message: str,
        player_id: str,
        npc_id: str,
        session_id: str
    ) -> str:
        """增量构建上下文"""
        # 检查是否有上次的上下文
        if session_id not in self.last_context:
            # 首次构建，完整构建
            context = self.build_context(message, player_id, npc_id)
            self.last_context[session_id] = {
                "context": context,
                "timestamp": datetime.now(),
                "message_count": 1
            }
            return context
        
        # 增量更新
        last_data = self.last_context[session_id]
        
        # 只更新变化的部分
        # 1. 添加新的对话历史
        new_history = f"用户: {message}\n"
        
        # 2. 更新工作记忆 (如果有新信息)
        new_memories = self._get_new_memories(message, player_id, last_data["timestamp"])
        
        # 3. 重新构建上下文 (只更新变化的部分)
        context = self._update_context(
            last_data["context"],
            new_history=new_history,
            new_memories=new_memories
        )
        
        # 更新缓存
        self.last_context[session_id] = {
            "context": context,
            "timestamp": datetime.now(),
            "message_count": last_data["message_count"] + 1
        }
        
        return context
```

### 3. 批量处理

批量处理多个请求，提高吞吐量。

```python
class BatchContextBuilder(ContextBuilder):
    """批量处理的上下文构建器"""
    
    def build_contexts_batch(
        self,
        requests: List[Dict[str, str]]
    ) -> List[str]:
        """
        批量构建上下文
        
        Args:
            requests: [
                {"message": "...", "player_id": "...", "npc_id": "..."},
                ...
            ]
        
        Returns:
            List[str]: 上下文列表
        """
        # 1. 批量汇集信息
        all_packets = []
        for req in requests:
            packets = self.gather_information(
                req["message"],
                req["player_id"],
                req["npc_id"]
            )
            all_packets.append(packets)
        
        # 2. 批量选择
        all_selected = []
        for packets, req in zip(all_packets, requests):
            selected = self.select_relevant(packets, req["message"])
            all_selected.append(selected)
        
        # 3. 批量结构化
        contexts = []
        for selected, req in zip(all_selected, requests):
            context = self.structure_context(
                selected,
                req.get("npc_name", "NPC"),
                req["player_id"]
            )
            contexts.append(context)
        
        return contexts
```

---

## 🐛 调试和监控

### 1. 详细日志

```python
import logging

class LoggingContextBuilder(ContextBuilder):
    """带详细日志的上下文构建器"""
    
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.logger = logging.getLogger("ContextBuilder")
        self.logger.setLevel(logging.DEBUG)
    
    def build_context(self, message, player_id, npc_id, **kwargs):
        """构建上下文 (带日志)"""
        self.logger.info(f"开始构建上下文: npc={npc_id}, player={player_id}")
        
        # 1. Gather
        start_time = time.time()
        packets = self.gather_information(message, player_id, npc_id)
        gather_time = time.time() - start_time
        self.logger.debug(f"Gather完成: {len(packets)}个信息包, 耗时{gather_time:.3f}s")
        
        # 记录各来源的信息量
        source_counts = {}
        for p in packets:
            source_counts[p.source] = source_counts.get(p.source, 0) + 1
        self.logger.debug(f"信息来源分布: {source_counts}")
        
        # 2. Select
        start_time = time.time()
        selected = self.select_relevant(packets, message)
        select_time = time.time() - start_time
        self.logger.debug(f"Select完成: 选中{len(selected)}/{len(packets)}个, 耗时{select_time:.3f}s")
        
        # 记录选中的信息
        total_tokens = sum(p.token_count for p in selected)
        self.logger.debug(f"选中信息总token数: {total_tokens}")
        
        # 3. Structure
        start_time = time.time()
        context = self.structure_context(selected, npc_id, player_id)
        structure_time = time.time() - start_time
        self.logger.debug(f"Structure完成: 耗时{structure_time:.3f}s")
        
        # 4. Compress (如果需要)
        context_tokens = self._estimate_tokens(context)
        max_tokens = int(self.config.max_tokens * (1 - self.config.reserve_ratio))
        
        if context_tokens > max_tokens:
            self.logger.warning(f"上下文超长: {context_tokens} > {max_tokens}, 开始压缩")
            start_time = time.time()
            context = self.compress_context(context, max_tokens)
            compress_time = time.time() - start_time
            compressed_tokens = self._estimate_tokens(context)
            self.logger.info(f"压缩完成: {context_tokens} → {compressed_tokens}, 耗时{compress_time:.3f}s")
        
        total_time = gather_time + select_time + structure_time
        self.logger.info(f"上下文构建完成: 总耗时{total_time:.3f}s")
        
        return context
```

### 2. 性能监控

```python
class MonitoredContextBuilder(ContextBuilder):
    """带性能监控的上下文构建器"""
    
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.metrics = {
            "build_count": 0,
            "total_time": 0.0,
            "gather_time": 0.0,
            "select_time": 0.0,
            "structure_time": 0.0,
            "compress_time": 0.0,
            "avg_packets": 0,
            "avg_selected": 0,
            "avg_tokens": 0,
            "compression_count": 0
        }
    
    def build_context(self, message, player_id, npc_id, **kwargs):
        """构建上下文 (带监控)"""
        start_time = time.time()
        
        # Gather
        gather_start = time.time()
        packets = self.gather_information(message, player_id, npc_id)
        self.metrics["gather_time"] += time.time() - gather_start
        
        # Select
        select_start = time.time()
        selected = self.select_relevant(packets, message)
        self.metrics["select_time"] += time.time() - select_start
        
        # Structure
        structure_start = time.time()
        context = self.structure_context(selected, npc_id, player_id)
        self.metrics["structure_time"] += time.time() - structure_start
        
        # Compress (如果需要)
        context_tokens = self._estimate_tokens(context)
        max_tokens = int(self.config.max_tokens * (1 - self.config.reserve_ratio))
        
        if context_tokens > max_tokens:
            compress_start = time.time()
            context = self.compress_context(context, max_tokens)
            self.metrics["compress_time"] += time.time() - compress_start
            self.metrics["compression_count"] += 1
        
        # 更新统计
        self.metrics["build_count"] += 1
        self.metrics["total_time"] += time.time() - start_time
        self.metrics["avg_packets"] = (
            (self.metrics["avg_packets"] * (self.metrics["build_count"] - 1) + len(packets)) /
            self.metrics["build_count"]
        )
        self.metrics["avg_selected"] = (
            (self.metrics["avg_selected"] * (self.metrics["build_count"] - 1) + len(selected)) /
            self.metrics["build_count"]
        )
        self.metrics["avg_tokens"] = (
            (self.metrics["avg_tokens"] * (self.metrics["build_count"] - 1) + context_tokens) /
            self.metrics["build_count"]
        )
        
        return context
    
    def get_metrics(self) -> Dict[str, Any]:
        """获取性能指标"""
        count = self.metrics["build_count"]
        if count == 0:
            return self.metrics
        
        return {
            "build_count": count,
            "avg_total_time": self.metrics["total_time"] / count,
            "avg_gather_time": self.metrics["gather_time"] / count,
            "avg_select_time": self.metrics["select_time"] / count,
            "avg_structure_time": self.metrics["structure_time"] / count,
            "avg_compress_time": self.metrics["compress_time"] / count if self.metrics["compression_count"] > 0 else 0,
            "avg_packets": self.metrics["avg_packets"],
            "avg_selected": self.metrics["avg_selected"],
            "avg_tokens": self.metrics["avg_tokens"],
            "compression_rate": self.metrics["compression_count"] / count
        }
```

---

## 🔗 与其他模块的集成

### 在 NPCAgent 中使用

```python
class NPCAgent:
    def __init__(self, ...):
        # 创建上下文构建器
        self.context_builder = ContextBuilder(
            config=ContextConfig(
                max_tokens=3000,
                role_description=self.personality.to_prompt()
            ),
            memory_tool=self.memory,
            rag_tool=self.rag,
            relationship_manager=self.relationship
        )
    
    def chat(self, player_id: str, message: str, ...):
        # 1. 构建上下文
        context = self.context_builder.build_context(
            message=message,
            player_id=player_id,
            npc_id=self.npc_id
        )
        
        # 2. 发送给LLM
        response = self.llm.invoke([
            {"role": "system", "content": context},
            {"role": "user", "content": message}
        ])
        
        return response.content
```

---

## 📚 最佳实践

### 1. 合理设置token预算

```python
# 推荐配置
config = ContextConfig(
    max_tokens=3000,
    reserve_ratio=0.2,  # 预留20%给系统指令和回复
    
    # 预算分配 (总和=1.0)
    memory_budget=0.30,    # 记忆30% (最重要)
    rag_budget=0.25,       # RAG 25%
    history_budget=0.20,   # 历史20%
    notes_budget=0.10,     # 笔记10%
    custom_budget=0.15     # 自定义15%
)
```

### 2. 优先级设置

```python
优先级建议:
10: 角色设定 (system)
9:  关键指令
8:  工作记忆 (当前对话)
7:  情景记忆、对话历史、好感度
6:  语义记忆、RAG知识
5:  感知记忆、笔记
4:  自定义信息
```

### 3. 压缩策略

```python
# 启用压缩
config.enable_compression = True
config.compression_ratio = 0.7  # 保留70%

# 压缩顺序 (从低到高优先级)
compression_order = [
    "notes",           # 最先压缩
    "custom",
    "perceptual",
    "semantic",
    "rag",
    "episodic",
    "history",
    "working",
    "relationship",
    "system"           # 最后压缩 (通常不压缩)
]
```

---

## 🎓 总结

### 核心要点

1. **GSSC流水线**: Gather → Select → Structure → Compress
2. **智能选择**: 基于相关性、时间、优先级的综合评分
3. **预算管理**: 为不同来源分配token预算
4. **兜底压缩**: 超出限制时智能压缩

### 性能优化

1. **缓存**: 缓存最近的上下文
2. **并行**: 并行汇集信息
3. **增量**: 增量更新上下文
4. **批量**: 批量处理请求

### 监控调试

1. **详细日志**: 记录每个阶段的耗时
2. **性能指标**: 统计平均耗时和token数
3. **可视化**: 可视化上下文构建过程

---

## 🔗 相关文档

- [05-记忆系统详解](./05-MEMORY_SYSTEM.md) - 记忆检索
- [06-RAG系统详解](./06-RAG_SYSTEM.md) - 知识检索
- [08-好感度系统详解](./08-RELATIONSHIP_SYSTEM.md) - 关系信息
- [10-NPC智能体详解](./10-NPC_AGENT.md) - 集成使用

---

恭喜！你现在已经完全掌握了上下文构建系统！🎉
