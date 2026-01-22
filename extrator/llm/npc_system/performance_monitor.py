"""
性能监控系统 - Performance Monitor
详细追踪NPC对话系统各模块的执行时间

功能:
- 追踪每个功能模块的调用耗时
- 统计各步骤的平均耗时
- 生成性能报告
- 识别性能瓶颈

使用方式:
    from .performance_monitor import PerformanceMonitor, timed
    
    monitor = PerformanceMonitor()
    
    with monitor.track("memory_search"):
        results = memory.search(query)
    
    # 或使用装饰器
    @timed(monitor, "llm_generate")
    def generate_reply():
        ...
"""

import time
import json
import statistics
from datetime import datetime
from dataclasses import dataclass, field
from typing import Dict, List, Optional, Any, Callable
from contextlib import contextmanager
from functools import wraps
from collections import defaultdict


@dataclass
class TimingRecord:
    """单次计时记录"""
    name: str                    # 步骤名称
    start_time: float           # 开始时间戳
    end_time: float = 0.0       # 结束时间戳
    duration_ms: float = 0.0    # 耗时(毫秒)
    success: bool = True        # 是否成功
    error: str = ""             # 错误信息
    metadata: Dict = field(default_factory=dict)  # 额外元数据
    
    def finish(self, success: bool = True, error: str = ""):
        """完成计时"""
        self.end_time = time.perf_counter()
        self.duration_ms = (self.end_time - self.start_time) * 1000
        self.success = success
        self.error = error


@dataclass
class StepStatistics:
    """步骤统计信息"""
    name: str
    count: int = 0
    total_ms: float = 0.0
    min_ms: float = float('inf')
    max_ms: float = 0.0
    success_count: int = 0
    failure_count: int = 0
    durations: List[float] = field(default_factory=list)
    
    @property
    def avg_ms(self) -> float:
        return self.total_ms / self.count if self.count > 0 else 0.0
    
    @property
    def success_rate(self) -> float:
        return self.success_count / self.count * 100 if self.count > 0 else 0.0
    
    @property
    def std_ms(self) -> float:
        """标准差"""
        if len(self.durations) < 2:
            return 0.0
        return statistics.stdev(self.durations)
    
    @property
    def median_ms(self) -> float:
        """中位数"""
        if not self.durations:
            return 0.0
        return statistics.median(self.durations)
    
    @property
    def p95_ms(self) -> float:
        """95百分位"""
        if not self.durations:
            return 0.0
        sorted_durations = sorted(self.durations)
        idx = int(len(sorted_durations) * 0.95)
        return sorted_durations[min(idx, len(sorted_durations) - 1)]
    
    def add_record(self, record: TimingRecord):
        """添加一条记录"""
        self.count += 1
        self.total_ms += record.duration_ms
        self.min_ms = min(self.min_ms, record.duration_ms)
        self.max_ms = max(self.max_ms, record.duration_ms)
        self.durations.append(record.duration_ms)
        
        if record.success:
            self.success_count += 1
        else:
            self.failure_count += 1
        
        # 保留最近1000条记录，防止内存溢出
        if len(self.durations) > 1000:
            self.durations = self.durations[-1000:]
    
    def to_dict(self) -> dict:
        return {
            "name": self.name,
            "count": self.count,
            "avg_ms": round(self.avg_ms, 2),
            "min_ms": round(self.min_ms, 2) if self.min_ms != float('inf') else 0,
            "max_ms": round(self.max_ms, 2),
            "median_ms": round(self.median_ms, 2),
            "std_ms": round(self.std_ms, 2),
            "p95_ms": round(self.p95_ms, 2),
            "success_rate": round(self.success_rate, 2),
            "total_ms": round(self.total_ms, 2)
        }


@dataclass
class DialogueTrace:
    """单次对话的完整追踪"""
    trace_id: str
    npc_id: str = ""
    player_id: str = ""
    start_time: float = 0.0
    end_time: float = 0.0
    total_duration_ms: float = 0.0
    steps: List[TimingRecord] = field(default_factory=list)
    success: bool = True
    error: str = ""
    user_message: str = ""
    npc_reply: str = ""
    
    def add_step(self, record: TimingRecord):
        """添加步骤记录"""
        self.steps.append(record)
    
    def finish(self, success: bool = True, error: str = "", npc_reply: str = ""):
        """完成追踪"""
        self.end_time = time.perf_counter()
        self.total_duration_ms = (self.end_time - self.start_time) * 1000
        self.success = success
        self.error = error
        self.npc_reply = npc_reply
    
    def get_step_breakdown(self) -> List[Dict]:
        """获取步骤分解"""
        return [
            {
                "step": step.name,
                "duration_ms": round(step.duration_ms, 2),
                "percent": round(step.duration_ms / self.total_duration_ms * 100, 1) if self.total_duration_ms > 0 else 0,
                "success": step.success
            }
            for step in self.steps
        ]
    
    def to_dict(self) -> dict:
        return {
            "trace_id": self.trace_id,
            "npc_id": self.npc_id,
            "player_id": self.player_id,
            "total_duration_ms": round(self.total_duration_ms, 2),
            "success": self.success,
            "error": self.error,
            "steps": self.get_step_breakdown()
        }
    
    def print_breakdown(self):
        """打印步骤分解"""
        print(f"\n{'='*60}")
        print(f"对话性能追踪 [ID: {self.trace_id[:8]}]")
        print(f"{'='*60}")
        print(f"NPC: {self.npc_id} | 玩家: {self.player_id}")
        print(f"总耗时: {self.total_duration_ms:.2f}ms ({self.total_duration_ms/1000:.2f}秒)")
        print(f"状态: {'✅ 成功' if self.success else '❌ 失败: ' + self.error}")
        print(f"{'-'*60}")
        print(f"{'步骤':<25} {'耗时(ms)':<12} {'占比':<10} {'状态':<8}")
        print(f"{'-'*60}")
        
        for step in self.steps:
            percent = step.duration_ms / self.total_duration_ms * 100 if self.total_duration_ms > 0 else 0
            status = "✅" if step.success else "❌"
            # 高亮耗时最长的步骤
            highlight = "⚠️ " if percent > 30 else "   "
            print(f"{highlight}{step.name:<22} {step.duration_ms:>8.2f}ms   {percent:>5.1f}%     {status}")
        
        print(f"{'='*60}")


class PerformanceMonitor:
    """
    性能监控器
    
    用于追踪NPC对话系统各模块的执行时间
    
    使用示例:
        monitor = PerformanceMonitor()
        
        # 方式1: 上下文管理器
        with monitor.track("memory_search"):
            results = memory.search(query)
        
        # 方式2: 开始/结束追踪
        trace = monitor.start_dialogue("npc_001", "player_001", "你好")
        with monitor.track("step1"):
            ...
        monitor.end_dialogue(trace, success=True, reply="你好!")
        
        # 查看统计
        monitor.print_statistics()
    """
    
    # NPC对话的标准步骤定义
    DIALOGUE_STEPS = [
        "get_affinity",           # 1. 获取好感度
        "build_context_packets",  # 2. 构建上下文包
        "memory_search",          # 3. 检索记忆
        "rag_search",             # 4. 检索RAG知识
        "notes_search",           # 5. 检索笔记
        "context_build",          # 6. 构建完整上下文
        "llm_generate",           # 7. LLM生成回复
        "update_affinity",        # 8. 更新好感度
        "store_working_memory",   # 9. 存储工作记忆
        "store_episodic_memory",  # 10. 存储情景记忆
        "save_dialogue_sqlite",   # 11. 持久化对话(SQLite)
        "save_dialogue_markdown", # 12. 保存Markdown
        "save_episodic_file",     # 13. 保存情景记忆文件
        "other"                   # 其他
    ]
    
    def __init__(self, enabled: bool = True):
        self.enabled = enabled
        self.statistics: Dict[str, StepStatistics] = {}
        self.traces: List[DialogueTrace] = []
        self.current_trace: Optional[DialogueTrace] = None
        self._trace_counter = 0
        
        # 全局统计
        self.total_dialogues = 0
        self.successful_dialogues = 0
        self.failed_dialogues = 0
        
        # 初始化所有标准步骤的统计
        for step in self.DIALOGUE_STEPS:
            self.statistics[step] = StepStatistics(name=step)
    
    def start_dialogue(self, npc_id: str, player_id: str, 
                       user_message: str = "") -> DialogueTrace:
        """开始一次对话追踪"""
        self._trace_counter += 1
        trace_id = f"trace_{datetime.now().strftime('%Y%m%d_%H%M%S')}_{self._trace_counter}"
        
        trace = DialogueTrace(
            trace_id=trace_id,
            npc_id=npc_id,
            player_id=player_id,
            start_time=time.perf_counter(),
            user_message=user_message
        )
        
        self.current_trace = trace
        self.total_dialogues += 1
        
        return trace
    
    def end_dialogue(self, trace: DialogueTrace = None, 
                     success: bool = True, 
                     error: str = "",
                     npc_reply: str = ""):
        """结束对话追踪"""
        if trace is None:
            trace = self.current_trace
        
        if trace is None:
            return
        
        trace.finish(success, error, npc_reply)
        self.traces.append(trace)
        
        if success:
            self.successful_dialogues += 1
        else:
            self.failed_dialogues += 1
        
        # 保留最近100条追踪
        if len(self.traces) > 100:
            self.traces = self.traces[-100:]
        
        self.current_trace = None
    
    @contextmanager
    def track(self, step_name: str, **metadata):
        """
        追踪一个步骤的执行时间
        
        使用:
            with monitor.track("memory_search"):
                results = memory.search(query)
        """
        if not self.enabled:
            yield
            return
        
        record = TimingRecord(
            name=step_name,
            start_time=time.perf_counter(),
            metadata=metadata
        )
        
        error = ""
        success = True
        
        try:
            yield record
        except Exception as e:
            error = str(e)
            success = False
            raise
        finally:
            record.finish(success, error)
            
            # 添加到统计
            if step_name not in self.statistics:
                self.statistics[step_name] = StepStatistics(name=step_name)
            self.statistics[step_name].add_record(record)
            
            # 添加到当前追踪
            if self.current_trace:
                self.current_trace.add_step(record)
    
    def record_step(self, step_name: str, duration_ms: float, 
                    success: bool = True, error: str = ""):
        """手动记录一个步骤（用于无法使用上下文管理器的情况）"""
        if not self.enabled:
            return
        
        record = TimingRecord(
            name=step_name,
            start_time=time.perf_counter() - duration_ms / 1000,
            end_time=time.perf_counter(),
            duration_ms=duration_ms,
            success=success,
            error=error
        )
        
        if step_name not in self.statistics:
            self.statistics[step_name] = StepStatistics(name=step_name)
        self.statistics[step_name].add_record(record)
        
        if self.current_trace:
            self.current_trace.add_step(record)
    
    def get_statistics(self) -> Dict[str, dict]:
        """获取所有步骤的统计信息"""
        return {
            name: stats.to_dict() 
            for name, stats in self.statistics.items()
            if stats.count > 0
        }
    
    def get_dialogue_summary(self) -> dict:
        """获取对话总体统计"""
        if not self.traces:
            return {
                "total_dialogues": 0,
                "success_rate": 0,
                "avg_duration_ms": 0
            }
        
        durations = [t.total_duration_ms for t in self.traces]
        
        return {
            "total_dialogues": self.total_dialogues,
            "successful_dialogues": self.successful_dialogues,
            "failed_dialogues": self.failed_dialogues,
            "success_rate": round(self.successful_dialogues / self.total_dialogues * 100, 2) if self.total_dialogues > 0 else 0,
            "avg_duration_ms": round(sum(durations) / len(durations), 2),
            "min_duration_ms": round(min(durations), 2),
            "max_duration_ms": round(max(durations), 2),
            "median_duration_ms": round(statistics.median(durations), 2) if durations else 0
        }
    
    def get_bottlenecks(self, top_n: int = 5) -> List[dict]:
        """识别性能瓶颈（平均耗时最长的步骤）"""
        sorted_stats = sorted(
            [s for s in self.statistics.values() if s.count > 0],
            key=lambda s: s.avg_ms,
            reverse=True
        )
        
        return [
            {
                "step": s.name,
                "avg_ms": round(s.avg_ms, 2),
                "max_ms": round(s.max_ms, 2),
                "count": s.count,
                "recommendation": self._get_optimization_hint(s.name, s.avg_ms)
            }
            for s in sorted_stats[:top_n]
        ]
    
    def _get_optimization_hint(self, step_name: str, avg_ms: float) -> str:
        """获取优化建议"""
        hints = {
            "llm_generate": "考虑使用更快的模型或减少上下文长度",
            "rag_search": "优化向量索引或减少检索数量",
            "memory_search": "考虑添加索引或限制搜索范围",
            "context_build": "简化上下文构建逻辑或减少包含的信息",
            "save_dialogue_sqlite": "考虑异步写入或批量写入",
            "save_dialogue_markdown": "考虑异步写入",
            "update_affinity": "如果使用LLM分析，考虑切换到规则分析"
        }
        
        if avg_ms > 1000:
            return hints.get(step_name, "此步骤耗时较长，建议优化")
        elif avg_ms > 500:
            return hints.get(step_name, "可以考虑优化")
        else:
            return "性能良好"
    
    def print_statistics(self):
        """打印详细统计信息"""
        print(f"\n{'='*70}")
        print(f"{'NPC对话系统性能统计':^70}")
        print(f"{'='*70}")
        
        # 对话总览
        summary = self.get_dialogue_summary()
        print(f"\n📊 对话总览")
        print(f"   总对话数: {summary.get('total_dialogues', 0)}")
        print(f"   成功率: {summary.get('success_rate', 0)}%")
        print(f"   平均耗时: {summary.get('avg_duration_ms', 0):.2f}ms ({summary.get('avg_duration_ms', 0)/1000:.2f}秒)")
        print(f"   最快: {summary.get('min_duration_ms', 0):.2f}ms | 最慢: {summary.get('max_duration_ms', 0):.2f}ms")
        
        # 各步骤统计
        print(f"\n📈 各步骤耗时统计")
        print(f"{'-'*70}")
        print(f"{'步骤名称':<25} {'次数':<8} {'平均(ms)':<12} {'最小':<10} {'最大':<10} {'P95':<10}")
        print(f"{'-'*70}")
        
        # 按平均耗时排序
        sorted_stats = sorted(
            [s for s in self.statistics.values() if s.count > 0],
            key=lambda s: s.avg_ms,
            reverse=True
        )
        
        for stat in sorted_stats:
            min_val = stat.min_ms if stat.min_ms != float('inf') else 0
            print(f"{stat.name:<25} {stat.count:<8} {stat.avg_ms:<12.2f} {min_val:<10.2f} {stat.max_ms:<10.2f} {stat.p95_ms:<10.2f}")
        
        # 性能瓶颈
        print(f"\n⚠️  性能瓶颈 (Top 3)")
        print(f"{'-'*70}")
        bottlenecks = self.get_bottlenecks(3)
        for i, b in enumerate(bottlenecks, 1):
            print(f"   {i}. {b['step']}: 平均 {b['avg_ms']:.2f}ms")
            print(f"      💡 {b['recommendation']}")
        
        print(f"\n{'='*70}")
    
    def print_last_trace(self):
        """打印最近一次对话追踪"""
        if self.traces:
            self.traces[-1].print_breakdown()
        else:
            print("暂无对话追踪记录")
    
    def export_report(self, filepath: str = "performance_report.json"):
        """导出性能报告为JSON"""
        report = {
            "generated_at": datetime.now().isoformat(),
            "summary": self.get_dialogue_summary(),
            "step_statistics": self.get_statistics(),
            "bottlenecks": self.get_bottlenecks(5),
            "recent_traces": [t.to_dict() for t in self.traces[-10:]]
        }
        
        with open(filepath, 'w', encoding='utf-8') as f:
            json.dump(report, f, ensure_ascii=False, indent=2)
        
        print(f"✅ 性能报告已导出到: {filepath}")
        return filepath
    
    def reset(self):
        """重置所有统计"""
        self.statistics = {step: StepStatistics(name=step) for step in self.DIALOGUE_STEPS}
        self.traces = []
        self.total_dialogues = 0
        self.successful_dialogues = 0
        self.failed_dialogues = 0
        self._trace_counter = 0
        self.current_trace = None


def timed(monitor: PerformanceMonitor, step_name: str):
    """
    装饰器：追踪函数执行时间
    
    使用:
        @timed(monitor, "llm_generate")
        def generate_reply(context, message):
            ...
    """
    def decorator(func: Callable):
        @wraps(func)
        def wrapper(*args, **kwargs):
            with monitor.track(step_name):
                return func(*args, **kwargs)
        return wrapper
    return decorator


# 全局性能监控器实例
_global_monitor: Optional[PerformanceMonitor] = None


def get_global_monitor() -> PerformanceMonitor:
    """获取全局性能监控器"""
    global _global_monitor
    if _global_monitor is None:
        _global_monitor = PerformanceMonitor()
    return _global_monitor


def reset_global_monitor():
    """重置全局性能监控器"""
    global _global_monitor
    if _global_monitor:
        _global_monitor.reset()
