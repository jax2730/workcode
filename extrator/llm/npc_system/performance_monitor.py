"""
性能监控器 - Performance Monitor
详细统计NPC对话系统各模块的耗时

监控的步骤:
1. 获取好感度 (get_affinity)
2. 构建上下文包 (build_context_packets)
3. 检索工作记忆 (search_working_memory)
4. 检索情景记忆 (search_episodic_memory)
5. 检索语义记忆 (search_semantic_memory)
6. 检索RAG知识 (search_rag_knowledge)
7. 检索笔记 (search_notes)
8. GSSC上下文构建 (gssc_build_context)
9. LLM生成回复 (llm_generate)
10. 更新好感度 (update_affinity)
11. 存储工作记忆 (store_working_memory)
12. 存储情景记忆 (store_episodic_memory)
13. 持久化到SQLite (persist_sqlite)
14. 保存到Markdown (save_markdown)
15. 更新会话历史 (update_session)
"""

import time
import json
from datetime import datetime
from dataclasses import dataclass, field
from typing import Dict, List, Optional, Any, Callable
from functools import wraps
from contextlib import contextmanager
import threading


@dataclass
class StepTiming:
    """单个步骤的计时信息"""
    step_name: str
    step_id: int
    start_time: float = 0.0
    end_time: float = 0.0
    duration_ms: float = 0.0
    success: bool = True
    error: str = ""
    metadata: Dict = field(default_factory=dict)
    
    def to_dict(self) -> dict:
        return {
            "step_id": self.step_id,
            "step_name": self.step_name,
            "duration_ms": round(self.duration_ms, 3),
            "success": self.success,
            "error": self.error,
            "metadata": self.metadata
        }


@dataclass
class DialogueMetrics:
    """单次对话的完整性能指标"""
    dialogue_id: str
    timestamp: str
    player_id: str
    npc_id: str
    message_length: int = 0
    reply_length: int = 0
    steps: List[StepTiming] = field(default_factory=list)
    total_time_ms: float = 0.0
    
    # 阶段汇总
    retrieval_time_ms: float = 0.0    # 检索阶段 (记忆+RAG+笔记)
    context_build_time_ms: float = 0.0  # 上下文构建
    llm_time_ms: float = 0.0          # LLM生成
    storage_time_ms: float = 0.0      # 存储阶段
    
    def calculate_totals(self):
        """计算各阶段总耗时"""
        # 步骤分类 (基于 npc_agent.py 中实际使用的步骤名称)
        retrieval_steps = ['search_working_memory', 'search_episodic_memory', 
                          'search_semantic_memory', 'search_rag_knowledge', 'search_notes']
        context_steps = ['context_build', 'build_context_packets', 'gssc_build_context']
        llm_steps = ['llm_generate']
        storage_steps = ['store_working_memory', 'store_episodic_memory', 
                        'save_dialogue_sqlite', 'save_dialogue_markdown', 
                        'save_episodic_file', 'persist_sqlite', 'save_markdown', 
                        'update_session', 'update_affinity']
        affinity_steps = ['get_affinity', 'update_affinity']
        
        # 重置统计
        self.retrieval_time_ms = 0.0
        self.context_build_time_ms = 0.0
        self.llm_time_ms = 0.0
        self.storage_time_ms = 0.0
        
        for step in self.steps:
            if step.step_name in retrieval_steps:
                self.retrieval_time_ms += step.duration_ms
            elif step.step_name in context_steps:
                self.context_build_time_ms += step.duration_ms
            elif step.step_name in llm_steps:
                self.llm_time_ms += step.duration_ms
            elif step.step_name in storage_steps:
                self.storage_time_ms += step.duration_ms
            # 好感度操作归入context或storage（根据实际耗时影响较小）
            elif step.step_name == 'get_affinity':
                self.context_build_time_ms += step.duration_ms
            elif step.step_name == 'update_affinity':
                self.storage_time_ms += step.duration_ms
        
        self.total_time_ms = sum(s.duration_ms for s in self.steps)
    
    def to_dict(self) -> dict:
        self.calculate_totals()
        return {
            "dialogue_id": self.dialogue_id,
            "timestamp": self.timestamp,
            "player_id": self.player_id,
            "npc_id": self.npc_id,
            "message_length": self.message_length,
            "reply_length": self.reply_length,
            "total_time_ms": round(self.total_time_ms, 3),
            "phase_summary": {
                "retrieval_ms": round(self.retrieval_time_ms, 3),
                "context_build_ms": round(self.context_build_time_ms, 3),
                "llm_generate_ms": round(self.llm_time_ms, 3),
                "storage_ms": round(self.storage_time_ms, 3)
            },
            "steps": [s.to_dict() for s in self.steps]
        }
    
    def get_summary_table(self) -> str:
        """生成可视化的汇总表格"""
        self.calculate_totals()
        
        lines = []
        lines.append("╔════════════════════════════════════════════════════════════════════════════╗")
        lines.append("║                         对话性能详细报告                                    ║")
        lines.append("╠════════════════════════════════════════════════════════════════════════════╣")
        lines.append(f"║  对话ID: {self.dialogue_id[:60]:<64} ║")
        lines.append(f"║  时间:   {self.timestamp:<64} ║")
        lines.append(f"║  NPC:    {self.npc_id:<64} ║")
        lines.append(f"║  玩家:   {self.player_id:<64} ║")
        lines.append("╠════════════════════════════════════════════════════════════════════════════╣")
        lines.append("║  步骤                                         耗时(ms)     状态           ║")
        lines.append("╠════════════════════════════════════════════════════════════════════════════╣")
        
        for step in self.steps:
            status = "✓" if step.success else "✗"
            step_display = step.step_name[:40]
            lines.append(f"║  {step.step_id:2d}. {step_display:<40} {step.duration_ms:>10.2f}     {status:<10} ║")
        
        lines.append("╠════════════════════════════════════════════════════════════════════════════╣")
        lines.append("║  阶段汇总                                                                  ║")
        lines.append("╠════════════════════════════════════════════════════════════════════════════╣")
        
        total = self.total_time_ms if self.total_time_ms > 0 else 1
        
        retrieval_pct = (self.retrieval_time_ms / total) * 100
        context_pct = (self.context_build_time_ms / total) * 100
        llm_pct = (self.llm_time_ms / total) * 100
        storage_pct = (self.storage_time_ms / total) * 100
        
        lines.append(f"║  📥 检索阶段 (记忆+RAG+笔记):  {self.retrieval_time_ms:>10.2f} ms  ({retrieval_pct:>5.1f}%)           ║")
        lines.append(f"║  🔧 上下文构建 (GSSC):         {self.context_build_time_ms:>10.2f} ms  ({context_pct:>5.1f}%)           ║")
        lines.append(f"║  🤖 LLM生成回复:               {self.llm_time_ms:>10.2f} ms  ({llm_pct:>5.1f}%)           ║")
        lines.append(f"║  💾 存储阶段:                  {self.storage_time_ms:>10.2f} ms  ({storage_pct:>5.1f}%)           ║")
        lines.append("╠════════════════════════════════════════════════════════════════════════════╣")
        lines.append(f"║  ⏱️  总耗时:                    {self.total_time_ms:>10.2f} ms  ({self.total_time_ms/1000:.2f} 秒)         ║")
        lines.append("╚════════════════════════════════════════════════════════════════════════════╝")
        
        return "\n".join(lines)
    
    def get_compact_summary(self) -> str:
        """获取紧凑的单行摘要"""
        self.calculate_totals()
        return (f"[耗时 {self.total_time_ms:.0f}ms] "
                f"检索:{self.retrieval_time_ms:.0f}ms | "
                f"构建:{self.context_build_time_ms:.0f}ms | "
                f"LLM:{self.llm_time_ms:.0f}ms | "
                f"存储:{self.storage_time_ms:.0f}ms")
    
    def print_breakdown(self):
        """打印详细的性能分解报告（在控制台输出）"""
        print(self.get_summary_table())
    
    def print_compact(self):
        """打印紧凑的性能摘要"""
        print(self.get_compact_summary())


class PerformanceMonitor:
    """
    性能监控器
    
    使用方式:
    ```python
    monitor = PerformanceMonitor()
    
    # 开始新对话监控
    monitor.start_dialogue("player_1", "npc_blacksmith", "你好")
    
    # 记录各步骤
    with monitor.step("get_affinity", 1):
        affinity = relationship.get_affinity(...)
    
    with monitor.step("search_memory", 2):
        memories = memory.search(...)
    
    # ... 其他步骤
    
    # 结束监控并获取报告
    metrics = monitor.end_dialogue(reply="你好，欢迎来到我的铁匠铺！")
    print(metrics.get_summary_table())
    ```
    """
    
    # 预定义的步骤列表 (按执行顺序)
    STEPS = {
        1: "get_affinity",
        2: "build_context_packets",
        3: "search_working_memory",
        4: "search_episodic_memory",
        5: "search_semantic_memory",
        6: "search_rag_knowledge",
        7: "search_notes",
        8: "gssc_build_context",
        9: "llm_generate",
        10: "update_affinity",
        11: "store_working_memory",
        12: "store_episodic_memory",
        13: "persist_sqlite",
        14: "save_markdown",
        15: "update_session"
    }
    
    def __init__(self, enabled: bool = True):
        self.enabled = enabled
        self.current_dialogue: Optional[DialogueMetrics] = None
        self.history: List[DialogueMetrics] = []
        self._lock = threading.Lock()
        self._step_counter = 0
    
    def start_dialogue(self, npc_id: str, player_id: str, 
                       message: str, dialogue_id: str = None) -> Optional[DialogueMetrics]:
        """
        开始监控一次对话
        
        Args:
            npc_id: NPC ID
            player_id: 玩家ID
            message: 玩家消息
            dialogue_id: 对话ID (可选)
            
        Returns:
            DialogueMetrics: 对话追踪对象 (trace)，用于后续传给 end_dialogue
        """
        if not self.enabled:
            return None
        
        with self._lock:
            if dialogue_id is None:
                dialogue_id = f"dlg_{datetime.now().strftime('%Y%m%d_%H%M%S_%f')}"
            
            self.current_dialogue = DialogueMetrics(
                dialogue_id=dialogue_id,
                timestamp=datetime.now().isoformat(),
                player_id=player_id,
                npc_id=npc_id,
                message_length=len(message)
            )
            self._step_counter = 0
            
            # 返回当前对话指标对象作为 trace
            return self.current_dialogue
    
    @contextmanager
    def step(self, step_name: str, step_id: int = None, **metadata):
        """
        使用上下文管理器记录步骤耗时
        
        Args:
            step_name: 步骤名称
            step_id: 步骤ID (可选，自动递增)
            **metadata: 额外的元数据
        """
        if not self.enabled or self.current_dialogue is None:
            yield
            return
        
        with self._lock:
            self._step_counter += 1
            if step_id is None:
                step_id = self._step_counter
        
        timing = StepTiming(
            step_name=step_name,
            step_id=step_id,
            metadata=metadata
        )
        timing.start_time = time.perf_counter() * 1000  # 转换为毫秒
        
        try:
            yield timing
            timing.success = True
        except Exception as e:
            timing.success = False
            timing.error = str(e)
            raise
        finally:
            timing.end_time = time.perf_counter() * 1000
            timing.duration_ms = timing.end_time - timing.start_time
            
            with self._lock:
                if self.current_dialogue:
                    self.current_dialogue.steps.append(timing)
    
    # track 是 step 的别名，保持兼容性
    track = step
    
    def record_step(self, step_name: str, duration_ms: float, 
                    step_id: int = None, success: bool = True,
                    error: str = "", **metadata) -> StepTiming:
        """手动记录一个步骤 (不使用上下文管理器)"""
        if not self.enabled or self.current_dialogue is None:
            return None
        
        with self._lock:
            self._step_counter += 1
            if step_id is None:
                step_id = self._step_counter
            
            timing = StepTiming(
                step_name=step_name,
                step_id=step_id,
                duration_ms=duration_ms,
                success=success,
                error=error,
                metadata=metadata
            )
            self.current_dialogue.steps.append(timing)
            return timing
    
    def end_dialogue(self, trace: DialogueMetrics = None, 
                     success: bool = True, 
                     npc_reply: str = "", 
                     error: str = "",
                     reply: str = "") -> Optional[DialogueMetrics]:
        """
        结束对话监控并返回完整指标
        
        Args:
            trace: start_dialogue 返回的追踪对象 (可选，向后兼容)
            success: 对话是否成功完成
            npc_reply: NPC回复内容
            error: 错误信息 (如果 success=False)
            reply: npc_reply的别名，保持向后兼容
            
        Returns:
            DialogueMetrics: 完整的对话性能指标
        """
        if not self.enabled:
            return trace
        
        # 兼容旧的调用方式: end_dialogue(reply="...")
        actual_reply = npc_reply or reply
        
        # 确定要操作的 metrics 对象
        metrics = trace if trace is not None else self.current_dialogue
        
        if metrics is None:
            return None
        
        with self._lock:
            metrics.reply_length = len(actual_reply)
            metrics.calculate_totals()
            
            # 记录成功/失败状态
            if hasattr(metrics, 'success'):
                metrics.success = success
            if hasattr(metrics, 'error') and error:
                metrics.error = error
            
            # 如果是当前对话，加入历史并清空
            if metrics is self.current_dialogue:
                self.history.append(metrics)
                self.current_dialogue = None
            elif trace is not None and trace not in self.history:
                # 如果是通过 trace 传入的，也加入历史
                self.history.append(trace)
            
            return metrics
    
    def get_history(self, limit: int = 10) -> List[DialogueMetrics]:
        """获取历史记录"""
        return self.history[-limit:]
    
    def get_aggregate_stats(self) -> Dict[str, Any]:
        """获取聚合统计数据"""
        if not self.history:
            return {}
        
        # 按步骤聚合
        step_stats: Dict[str, List[float]] = {}
        total_times = []
        
        for metrics in self.history:
            total_times.append(metrics.total_time_ms)
            for step in metrics.steps:
                if step.step_name not in step_stats:
                    step_stats[step.step_name] = []
                step_stats[step.step_name].append(step.duration_ms)
        
        # 计算每个步骤的统计
        step_summary = {}
        for name, times in step_stats.items():
            step_summary[name] = {
                "count": len(times),
                "avg_ms": sum(times) / len(times) if times else 0,
                "min_ms": min(times) if times else 0,
                "max_ms": max(times) if times else 0,
                "total_ms": sum(times)
            }
        
        return {
            "dialogue_count": len(self.history),
            "total_time": {
                "avg_ms": sum(total_times) / len(total_times) if total_times else 0,
                "min_ms": min(total_times) if total_times else 0,
                "max_ms": max(total_times) if total_times else 0
            },
            "steps": step_summary
        }
    
    def get_aggregate_summary_table(self) -> str:
        """生成聚合统计表格"""
        stats = self.get_aggregate_stats()
        if not stats:
            return "暂无统计数据"
        
        lines = []
        lines.append("╔════════════════════════════════════════════════════════════════════════════╗")
        lines.append("║                         性能聚合统计报告                                    ║")
        lines.append("╠════════════════════════════════════════════════════════════════════════════╣")
        lines.append(f"║  总对话次数: {stats['dialogue_count']:<62} ║")
        lines.append(f"║  平均耗时:   {stats['total_time']['avg_ms']:.2f} ms{' ' * 54} ║")
        lines.append(f"║  最快耗时:   {stats['total_time']['min_ms']:.2f} ms{' ' * 54} ║")
        lines.append(f"║  最慢耗时:   {stats['total_time']['max_ms']:.2f} ms{' ' * 54} ║")
        lines.append("╠════════════════════════════════════════════════════════════════════════════╣")
        lines.append("║  步骤                              调用次数    平均(ms)    最大(ms)        ║")
        lines.append("╠════════════════════════════════════════════════════════════════════════════╣")
        
        # 按平均耗时排序
        sorted_steps = sorted(
            stats['steps'].items(), 
            key=lambda x: x[1]['avg_ms'], 
            reverse=True
        )
        
        for name, info in sorted_steps:
            name_display = name[:32]
            lines.append(
                f"║  {name_display:<32} {info['count']:>8}    "
                f"{info['avg_ms']:>8.2f}    {info['max_ms']:>8.2f}        ║"
            )
        
        lines.append("╚════════════════════════════════════════════════════════════════════════════╝")
        
        return "\n".join(lines)
    
    def export_to_json(self, filepath: str = None) -> str:
        """导出历史记录到JSON"""
        if filepath is None:
            filepath = f"performance_log_{datetime.now().strftime('%Y%m%d_%H%M%S')}.json"
        
        data = {
            "export_time": datetime.now().isoformat(),
            "dialogue_count": len(self.history),
            "aggregate_stats": self.get_aggregate_stats(),
            "dialogues": [m.to_dict() for m in self.history]
        }
        
        with open(filepath, 'w', encoding='utf-8') as f:
            json.dump(data, f, ensure_ascii=False, indent=2)
        
        return filepath
    
    def clear_history(self):
        """清除历史记录"""
        with self._lock:
            self.history.clear()
    
    def get_dialogue_summary(self) -> Dict[str, Any]:
        """
        获取对话性能摘要 (用于 NPCAgent.get_performance_stats)
        
        Returns:
            Dict: 包含对话次数、平均耗时等汇总信息
        """
        if not self.history:
            return {
                "dialogue_count": 0,
                "avg_time_ms": 0,
                "total_time_ms": 0
            }
        
        total_times = [m.total_time_ms for m in self.history]
        return {
            "dialogue_count": len(self.history),
            "avg_time_ms": sum(total_times) / len(total_times),
            "min_time_ms": min(total_times),
            "max_time_ms": max(total_times),
            "total_time_ms": sum(total_times)
        }
    
    def get_statistics(self) -> Dict[str, Dict[str, float]]:
        """
        获取各步骤的统计信息 (用于 NPCAgent.get_performance_stats)
        
        Returns:
            Dict: 每个步骤的调用次数、平均/最小/最大耗时
        """
        stats = self.get_aggregate_stats()
        return stats.get('steps', {})
    
    def get_bottlenecks(self, top_n: int = 5) -> List[Dict[str, Any]]:
        """
        获取最耗时的步骤 (性能瓶颈) (用于 NPCAgent.get_performance_stats)
        
        Args:
            top_n: 返回前N个最慢的步骤
            
        Returns:
            List: 按平均耗时降序排列的步骤列表
        """
        stats = self.get_aggregate_stats()
        steps = stats.get('steps', {})
        
        if not steps:
            return []
        
        # 按平均耗时降序排列
        sorted_steps = sorted(
            steps.items(),
            key=lambda x: x[1].get('avg_ms', 0),
            reverse=True
        )
        
        bottlenecks = []
        for name, info in sorted_steps[:top_n]:
            bottlenecks.append({
                "step_name": name,
                "avg_ms": info.get('avg_ms', 0),
                "max_ms": info.get('max_ms', 0),
                "count": info.get('count', 0),
                "total_ms": info.get('total_ms', 0)
            })
        
        return bottlenecks


# 全局性能监控器实例
_global_monitor: Optional[PerformanceMonitor] = None


def get_performance_monitor() -> PerformanceMonitor:
    """获取全局性能监控器"""
    global _global_monitor
    if _global_monitor is None:
        _global_monitor = PerformanceMonitor(enabled=True)
    return _global_monitor


def set_performance_monitor(monitor: PerformanceMonitor):
    """设置全局性能监控器"""
    global _global_monitor
    _global_monitor = monitor


# ==================== 装饰器 ====================

def timed_step(step_name: str, step_id: int = None):
    """
    装饰器: 自动记录函数执行时间
    
    使用:
    ```python
    @timed_step("search_memory", 2)
    def search_memory(self, query):
        ...
    ```
    """
    def decorator(func: Callable):
        @wraps(func)
        def wrapper(*args, **kwargs):
            monitor = get_performance_monitor()
            if monitor and monitor.current_dialogue:
                with monitor.step(step_name, step_id):
                    return func(*args, **kwargs)
            else:
                return func(*args, **kwargs)
        return wrapper
    return decorator
