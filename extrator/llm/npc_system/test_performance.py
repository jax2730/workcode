"""
性能监控测试脚本
运行: python extrator/llm/npc_system/test_performance.py
"""

import sys
import os
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))))

from extrator.llm.npc_system.performance_monitor import PerformanceMonitor
import time


def test_performance_monitor():
    """测试性能监控器"""
    print("=" * 60)
    print("性能监控器测试")
    print("=" * 60)
    
    monitor = PerformanceMonitor()
    
    # 模拟几次对话
    for i in range(3):
        print(f"\n--- 模拟对话 {i+1} ---")
        
        # 开始追踪
        trace = monitor.start_dialogue("npc_001", "player_001", f"测试消息{i+1}")
        
        # 模拟各个步骤
        with monitor.track("get_affinity"):
            time.sleep(0.01)  # 10ms
        
        with monitor.track("build_context_packets"):
            time.sleep(0.005)  # 5ms
        
        with monitor.track("memory_search"):
            time.sleep(0.02)  # 20ms
        
        with monitor.track("rag_search"):
            time.sleep(0.015)  # 15ms
        
        with monitor.track("context_build"):
            time.sleep(0.03)  # 30ms
        
        with monitor.track("llm_generate"):
            time.sleep(0.5 + i * 0.1)  # 500-700ms (模拟LLM调用)
        
        with monitor.track("update_affinity"):
            time.sleep(0.01)  # 10ms
        
        with monitor.track("store_working_memory"):
            time.sleep(0.008)  # 8ms
        
        with monitor.track("save_dialogue_sqlite"):
            time.sleep(0.012)  # 12ms
        
        with monitor.track("save_dialogue_markdown"):
            time.sleep(0.006)  # 6ms
        
        # 结束追踪
        monitor.end_dialogue(trace, success=True, npc_reply=f"回复{i+1}")
        
        # 打印这次对话的分解
        trace.print_breakdown()
    
    # 打印整体统计
    print("\n\n")
    monitor.print_statistics()
    
    # 导出报告
    filepath = monitor.export_report("test_performance_report.json")
    print(f"\n报告已导出: {filepath}")
    
    print("\n✅ 测试完成!")


def test_bottleneck_detection():
    """测试瓶颈检测"""
    print("\n" + "=" * 60)
    print("瓶颈检测测试")
    print("=" * 60)
    
    monitor = PerformanceMonitor()
    
    # 模拟有明显瓶颈的对话
    for _ in range(5):
        trace = monitor.start_dialogue("npc_002", "player_002", "测试瓶颈")
        
        with monitor.track("get_affinity"):
            time.sleep(0.005)
        
        with monitor.track("llm_generate"):
            time.sleep(1.5)  # 明显的瓶颈
        
        with monitor.track("save_dialogue_sqlite"):
            time.sleep(0.3)  # 次要瓶颈
        
        monitor.end_dialogue(trace, success=True)
    
    # 获取瓶颈
    bottlenecks = monitor.get_bottlenecks(3)
    
    print("\n检测到的瓶颈:")
    for i, b in enumerate(bottlenecks, 1):
        print(f"  {i}. {b['step']}: 平均 {b['avg_ms']:.2f}ms")
        print(f"     建议: {b['recommendation']}")
    
    print("\n✅ 瓶颈检测完成!")


if __name__ == "__main__":
    test_performance_monitor()
    test_bottleneck_detection()
