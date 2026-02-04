"""
NPC对话命令行工具 - NPC Chat CLI
类似 general_chat.py，用于在命令行中直接与NPC对话

功能:
- 选择NPC进行对话
- 自动保存对话历史到SQLite和Markdown
- 显示好感度变化
- 支持多个NPC切换

运行方式:
    python -m extrator.llm.npc_system.npc_chat
    
或者:
    cd SceneAgentServer
    python extrator/llm/npc_system/npc_chat.py
"""

import os
import sys
import uuid
import time
from datetime import datetime
from pathlib import Path

# 确保能找到模块
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))))

from langchain_ollama import ChatOllama

# 导入NPC系统
from extrator.llm.npc_system import (
    create_npc,
    NPCManager,
    NPCManagerConfig,
    init_npc_data_directories,
    NPC_TEMPLATES
)
from extrator.llm.npc_system.performance_monitor import (
    PerformanceMonitor,
    get_performance_monitor,
    DialogueMetrics
)


class NPCChatCLI:
    """NPC命令行对话工具"""
    
    def __init__(self, data_dir: str = "./npc_data", config_dir: str = "./npc_configs"):
        self.data_dir = data_dir
        self.config_dir = config_dir
        self.llm = None
        self.manager = None
        self.current_npc_id = None
        
        # 持久化player_id (保存到本地文件)
        self.player_id = self._load_or_create_player_id()
        self.session_id = f"session_{datetime.now().strftime('%Y%m%d_%H%M%S')}"
        
        # 响应时间统计 (简单版)
        self.response_times = []
        
        # 保存最后一次性能结果
        self.last_perf_result = None
        
        # 显示模式: 'compact' 紧凑, 'detailed' 详细, 'none' 不显示
        self.perf_display_mode = 'compact'
    
    def _load_or_create_player_id(self) -> str:
        """加载或创建持久化的player_id"""
        player_id_file = Path(self.data_dir) / ".player_id"
        
        if player_id_file.exists():
            try:
                player_id = player_id_file.read_text().strip()
                if player_id:
                    print(f"[系统] 加载玩家ID: {player_id}")
                    return player_id
            except Exception as e:
                print(f"[系统] 读取玩家ID失败: {e}")
        
        # 创建新的player_id
        player_id = f"player_{uuid.uuid4().hex[:8]}"
        
        try:
            os.makedirs(self.data_dir, exist_ok=True)
            player_id_file.write_text(player_id)
            print(f"[系统] 创建新玩家ID: {player_id}")
        except Exception as e:
            print(f"[系统] 保存玩家ID失败: {e}")
        
        return player_id
        
    def initialize(self):
        """初始化系统"""
        print("=" * 60)
        print("🎮 NPC对话系统 - 命令行版")
        print("=" * 60)
        
        # 1. 初始化数据目录
        print("\n[1/3] 初始化数据目录...")
        init_npc_data_directories(self.data_dir)
        print(f"✅ 数据目录: {self.data_dir}")
        
        # 2. 初始化LLM
        print("\n[2/3] 初始化LLM (qwen2.5)...")
        try:
            # [优化] LLM参数优化以加速响应
            self.llm = ChatOllama(
                model="qwen2.5",
                temperature=0.7,
                # 性能优化参数
                num_predict=150,    # 限制最大生成token数 (默认128，适当增加)
                num_ctx=2048,       # 减少上下文窗口 (默认4096)
                repeat_penalty=1.1, # 适度重复惩罚
                # top_k=40,         # 可选：限制采样范围
                # top_p=0.9,        # 可选：nucleus采样
            )
            # 测试LLM连接
            test_response = self.llm.invoke("你好")
            print(f"✅ LLM连接成功")
        except Exception as e:
            print(f"❌ LLM初始化失败: {e}")
            print("   请确保Ollama服务正在运行: ollama serve")
            return False
        
        # 3. 初始化NPC管理器
        print("\n[3/3] 初始化NPC管理器...")
        config = NPCManagerConfig(
            data_dir=self.data_dir,
            config_dir=self.config_dir,
            enable_batch_generation=True
        )
        self.manager = NPCManager(config, self.llm)
        print(f"✅ 已加载 {len(self.manager.npc_configs)} 个NPC配置")
        
        return True
    
    def show_npc_list(self):
        """显示可用的NPC列表"""
        print("\n" + "=" * 60)
        print("📋 可用的NPC列表:")
        print("=" * 60)
        
        # 从配置文件加载的NPC
        if self.manager.npc_configs:
            print("\n【已配置的NPC】")
            for i, (npc_id, config) in enumerate(self.manager.npc_configs.items(), 1):
                personality = config.get("personality", config)
                name = personality.get("name", npc_id)
                role = personality.get("role", "未知")
                print(f"  {i}. {npc_id}: {name} ({role})")
        
        # 预定义模板
        print("\n【预定义模板】")
        for i, (template_id, template) in enumerate(NPC_TEMPLATES.items(), 1):
            personality = template.get("personality", {})
            name = personality.get("name", template_id)
            role = personality.get("role", "未知")
            print(f"  T{i}. {template_id}: {name} ({role})")
        
        print("\n" + "-" * 60)
    
    def select_npc(self) -> bool:
        """选择要对话的NPC"""
        self.show_npc_list()
        
        print("\n请输入NPC ID (或输入 'quit' 退出):")
        
        try:
            npc_input = input("NPC ID: ").strip()
        except (EOFError, KeyboardInterrupt):
            return False
        
        if npc_input.lower() == 'quit':
            return False
        
        # 检查是否是已配置的NPC
        if npc_input in self.manager.npc_configs:
            self.current_npc_id = npc_input
            npc = self.manager.get_npc(npc_input)
            if npc:
                print(f"\n✅ 已选择: {npc.personality.name} ({npc.personality.role})")
                return True
        
        # 检查是否是预定义模板
        if npc_input in NPC_TEMPLATES:
            self.current_npc_id = npc_input
            # 注册模板NPC
            self.manager.register_npc(npc_input, NPC_TEMPLATES[npc_input])
            npc = self.manager.get_npc(npc_input)
            if npc:
                print(f"\n✅ 已选择模板NPC: {npc.personality.name} ({npc.personality.role})")
                return True
        
        print(f"\n❌ 未找到NPC: {npc_input}")
        return self.select_npc()
    
    def show_help(self):
        """显示帮助信息"""
        print("""
╔════════════════════════════════════════════════════════════╗
║                      命令帮助                               ║
╠════════════════════════════════════════════════════════════╣
║  quit / exit    - 退出对话                                 ║
║  switch         - 切换NPC                                  ║
║  status         - 查看当前NPC状态和好感度                   ║
║  history        - 查看对话历史                              ║
║  clear          - 清除当前会话                              ║
║  export         - 导出对话到Excel                          ║
║  stats          - 查看简要响应时间统计                      ║
║  perf           - 查看详细性能分析 (各模块耗时)             ║
║  perf last      - 查看最近一次对话的性能分解                ║
║  perf export    - 导出性能报告到JSON                       ║
║  perf reset     - 重置性能统计                              ║
║  perf on/off    - 开启/关闭详细性能追踪                     ║
║  reset_player   - 重置玩家ID (开始新档案)                   ║
║  help           - 显示此帮助                                ║
╚════════════════════════════════════════════════════════════╝
""")
    
    def show_status(self):
        """显示当前NPC状态"""
        if not self.current_npc_id:
            print("❌ 未选择NPC")
            return
        
        npc = self.manager.get_npc(self.current_npc_id)
        if not npc:
            print("❌ NPC未加载")
            return
        
        # 获取好感度
        affinity = npc.relationship.get_affinity(self.current_npc_id, self.player_id)
        
        print(f"""
╔════════════════════════════════════════════════════════════╗
║                    NPC状态                                  ║
╠════════════════════════════════════════════════════════════╣
║  NPC ID:    {self.current_npc_id:<46} ║
║  名称:      {npc.personality.name:<46} ║
║  角色:      {npc.personality.role:<46} ║
║  好感度:    {affinity.level.value if hasattr(affinity.level, 'value') else str(affinity.level):<46} ║
║  分数:      {affinity.score}/100{' ' * 40} ║
║  互动次数:  {affinity.interaction_count:<46} ║
╠════════════════════════════════════════════════════════════╣
║  玩家ID:    {self.player_id:<46} ║
║  会话ID:    {self.session_id[:40]:<46} ║
╚════════════════════════════════════════════════════════════╝
""")
    
    def chat_loop(self):
        """对话循环"""
        npc = self.manager.get_npc(self.current_npc_id)
        if not npc:
            print("❌ NPC未加载")
            return
        
        # 显示NPC问候
        greeting = npc.get_greeting(self.player_id)
        print(f"\n{npc.personality.name}: {greeting}")
        print("-" * 60)
        
        self.show_help()
        
        while True:
            try:
                user_input = input(f"\n你: ").strip()
            except (EOFError, KeyboardInterrupt):
                print("\n\n再见!")
                break
            
            if not user_input:
                continue
            
            # 处理命令
            cmd = user_input.lower()
            cmd_parts = cmd.split()
            
            if cmd in ['quit', 'exit']:
                print("\n再见!")
                break
            elif cmd == 'switch':
                if self.select_npc():
                    npc = self.manager.get_npc(self.current_npc_id)
                    greeting = npc.get_greeting(self.player_id)
                    print(f"\n{npc.personality.name}: {greeting}")
                continue
            elif cmd == 'status':
                self.show_status()
                continue
            elif cmd == 'history':
                self.show_history()
                continue
            elif cmd == 'clear':
                npc.clear_session(self.player_id)
                print("✅ 会话已清除")
                continue
            elif cmd == 'export':
                self.export_dialogue()
                continue
            elif cmd == 'stats':
                self.show_stats()
                continue
            elif cmd == 'reset_player':
                self.reset_player_id()
                continue
            elif cmd == 'help':
                self.show_help()
                continue
            # ========== 性能监控命令 ==========
            elif cmd == 'perf' or cmd == 'perf last':
                self.show_last_perf()
                continue
            elif cmd == 'perf all':
                self.show_aggregate_perf()
                continue
            elif cmd == 'perf export':
                self.export_perf_log()
                continue
            elif cmd == 'perf reset':
                self.reset_perf_stats()
                continue
            elif cmd == 'perf on':
                self.perf_display_mode = 'compact'
                print("✅ 性能监控已开启 (紧凑模式)")
                continue
            elif cmd == 'perf off':
                self.perf_display_mode = 'none'
                print("✅ 性能监控已关闭")
                continue
            elif cmd == 'perf detailed':
                self.perf_display_mode = 'detailed'
                print("✅ 性能监控切换为详细模式")
                continue
            
            # 正常对话 - 使用带性能监控的版本
            print(f"\n{npc.personality.name}: ", end="", flush=True)
            
            try:
                # 记录开始时间
                start_time = time.time()
                
                # 使用带性能监控的chat方法 (使用NPC内部的perf_monitor)
                result = npc.chat_with_perf(
                    player_id=self.player_id,
                    message=user_input,
                    session_id=self.session_id,
                    print_trace=(self.perf_display_mode == 'detailed')
                )
                
                # 计算总响应时间
                response_time = time.time() - start_time
                self.response_times.append(response_time)
                
                # 保存性能结果
                if result.get("performance"):
                    self.last_perf_result = result["performance"]
                
                # 显示回复
                print(result["reply"])
                
                # 显示好感度
                affinity = result.get("affinity", {})
                level = affinity.get("level", "")
                score = affinity.get("score", 0)
                
                # 根据显示模式显示性能信息
                if self.perf_display_mode == 'detailed' and self.last_perf_result:
                    print(f"\n  [好感度: {level} ({score}/100)]")
                    self._print_detailed_perf(self.last_perf_result)
                elif self.perf_display_mode == 'compact':
                    perf_summary = result.get("performance_summary", "")
                    if perf_summary:
                        print(f"\n  [好感度: {level} ({score}/100)]")
                        print(f"  {perf_summary}")
                    else:
                        print(f"\n  [好感度: {level} ({score}/100)] [响应: {response_time:.2f}秒]")
                else:
                    print(f"\n  [好感度: {level} ({score}/100)] [响应: {response_time:.2f}秒]")
                    
            except Exception as e:
                import traceback
                print(f"(出错: {e})")
                traceback.print_exc()
            
            print("-" * 60)
    
    def show_history(self):
        """显示对话历史"""
        npc = self.manager.get_npc(self.current_npc_id)
        if not npc:
            return
        
        history = npc.get_dialogue_history(self.player_id, limit=10)
        
        print(f"\n📜 最近对话历史 ({len(history)} 条):")
        print("-" * 60)
        
        for msg in history:
            role = "你" if msg.get("role") == "user" else npc.personality.name
            content = msg.get("content", "")[:80]
            timestamp = msg.get("timestamp", "")[:19]
            print(f"[{timestamp}] {role}: {content}...")
        
        print("-" * 60)
    
    def export_dialogue(self):
        """导出对话到Excel"""
        npc = self.manager.get_npc(self.current_npc_id)
        if not npc:
            return
        
        try:
            filepath = npc.export_dialogue_history(self.player_id)
            if filepath:
                print(f"✅ 对话已导出: {filepath}")
            else:
                print("⚠️ 导出失败或Excel功能未启用")
        except Exception as e:
            print(f"❌ 导出失败: {e}")
    
    def show_stats(self):
        """显示响应时间统计"""
        if not self.response_times:
            print("\n⚠️ 暂无响应时间数据")
            return
        
        avg_time = sum(self.response_times) / len(self.response_times)
        min_time = min(self.response_times)
        max_time = max(self.response_times)
        
        print(f"""
╔════════════════════════════════════════════════════════════╗
║                  响应时间统计                               ║
╠════════════════════════════════════════════════════════════╣
║  总对话次数:  {len(self.response_times):<46} ║
║  平均响应:    {avg_time:.2f}秒{' ' * 42} ║
║  最快响应:    {min_time:.2f}秒{' ' * 42} ║
║  最慢响应:    {max_time:.2f}秒{' ' * 42} ║
╚════════════════════════════════════════════════════════════╝
""")
        
        # 显示最近5次响应时间
        if len(self.response_times) > 0:
            recent = self.response_times[-5:]
            print("最近5次响应时间:")
            for i, t in enumerate(recent, 1):
                print(f"  {i}. {t:.2f}秒")
    
    # ==================== 详细性能监控方法 ====================
    
    def _get_npc_perf_monitor(self):
        """获取当前NPC的性能监控器"""
        if not self.current_npc_id:
            return None
        npc = self.manager.get_npc(self.current_npc_id)
        if npc and hasattr(npc, 'perf_monitor'):
            return npc.perf_monitor
        return None
    
    def show_last_perf(self):
        """显示最近一次对话的详细性能"""
        monitor = self._get_npc_perf_monitor()
        if not monitor:
            print("\n⚠️ 未选择NPC或NPC不支持性能监控")
            return
        
        history = monitor.get_history(limit=1)
        if not history:
            print("\n⚠️ 暂无性能数据，请先进行一次对话")
            return
        
        metrics = history[-1]
        print(metrics.get_summary_table())
    
    def show_aggregate_perf(self):
        """显示聚合性能统计"""
        monitor = self._get_npc_perf_monitor()
        if not monitor:
            print("\n⚠️ 未选择NPC或NPC不支持性能监控")
            return
        print(monitor.get_aggregate_summary_table())
    
    def export_perf_log(self):
        """导出性能日志到JSON"""
        monitor = self._get_npc_perf_monitor()
        if not monitor:
            print("\n⚠️ 未选择NPC或NPC不支持性能监控")
            return
        try:
            filepath = monitor.export_to_json()
            print(f"\n✅ 性能日志已导出: {filepath}")
        except Exception as e:
            print(f"\n❌ 导出失败: {e}")
    
    def reset_perf_stats(self):
        """重置性能统计"""
        monitor = self._get_npc_perf_monitor()
        if not monitor:
            print("\n⚠️ 未选择NPC或NPC不支持性能监控")
            return
        
        confirm = input("\n⚠️ 确认重置所有性能统计？(y/n): ")
        if confirm.lower() != 'y':
            print("已取消")
            return
        
        monitor.clear_history()
        self.last_perf_result = None
        print("✅ 性能统计已重置")
    
    def _print_detailed_perf(self, perf_dict: dict):
        """打印详细性能信息"""
        if not perf_dict:
            return
        
        print("\n┌─────────────────────────────────────────────────────────────┐")
        print("│                    详细步骤耗时                              │")
        print("├─────────────────────────────────────────────────────────────┤")
        
        steps = perf_dict.get("steps", [])
        for step in steps:
            name = step.get("step_name", "")[:35]
            duration = step.get("duration_ms", 0)
            status = "✓" if step.get("success", True) else "✗"
            print(f"│  {step.get('step_id', 0):2d}. {name:<35} {duration:>8.2f}ms {status} │")
        
        print("├─────────────────────────────────────────────────────────────┤")
        
        phase = perf_dict.get("phase_summary", {})
        total = perf_dict.get("total_time_ms", 0)
        
        print(f"│  📥 检索:   {phase.get('retrieval_ms', 0):>8.2f}ms                            │")
        print(f"│  🔧 构建:   {phase.get('context_build_ms', 0):>8.2f}ms                            │")
        print(f"│  🤖 LLM:    {phase.get('llm_generate_ms', 0):>8.2f}ms                            │")
        print(f"│  💾 存储:   {phase.get('storage_ms', 0):>8.2f}ms                            │")
        print("├─────────────────────────────────────────────────────────────┤")
        print(f"│  ⏱️  总计:   {total:>8.2f}ms ({total/1000:.2f}秒)                     │")
        print("└─────────────────────────────────────────────────────────────┘")
    
    def reset_player_id(self):
        """重置玩家ID"""
        confirm = input("\n⚠️ 确认重置玩家ID？这将开始新的游戏档案 (y/n): ")
        if confirm.lower() != 'y':
            print("已取消")
            return
        
        # 删除旧的player_id文件
        player_id_file = Path(self.data_dir) / ".player_id"
        if player_id_file.exists():
            player_id_file.unlink()
        
        # 生成新的player_id
        old_player_id = self.player_id
        self.player_id = f"player_{uuid.uuid4().hex[:8]}"
        player_id_file.write_text(self.player_id)
        
        print(f"✅ 玩家ID已重置")
        print(f"   旧ID: {old_player_id}")
        print(f"   新ID: {self.player_id}")
        print(f"   (旧档案数据仍保留在数据库中)")
    
    def run(self):
        """运行主程序"""
        # 初始化
        if not self.initialize():
            print("\n初始化失败，请检查配置后重试。")
            return
        
        # 选择NPC
        if not self.select_npc():
            print("\n再见!")
            return
        
        # 开始对话
        self.chat_loop()
        
        # 结束
        print("\n" + "=" * 60)
        print("感谢使用NPC对话系统!")
        print("=" * 60)


def main():
    """主函数"""
    cli = NPCChatCLI(
        data_dir="./npc_data",
        config_dir="./npc_configs"
    )
    cli.run()


if __name__ == "__main__":
    main()
