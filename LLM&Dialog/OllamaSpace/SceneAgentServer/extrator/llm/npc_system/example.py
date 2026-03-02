"""
NPC系统完整示例
演示端到端的对话流程和存储系统

功能演示:
1. 初始化数据目录结构
2. 创建NPC智能体
3. 进行对话并自动存储
4. 查看存储的数据 (SQLite + Markdown)
5. 导出对话历史到Excel
6. 使用NPCManager管理多个NPC
7. 批量生成NPC背景对话

运行方式:
    python -m extrator.llm.npc_system.example
    
或者:
    cd SceneAgentServer
    python -c "from extrator.llm.npc_system.example import main; main()"
"""

import os
import sys
from datetime import datetime

# 添加项目路径
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))))


def demo_basic_npc():
    """演示基本NPC创建和对话"""
    print("\n" + "=" * 60)
    print("📌 演示1: 基本NPC创建和对话")
    print("=" * 60)
    
    from extrator.llm.npc_system import (
        create_npc,
        init_npc_data_directories,
        get_data_paths
    )
    
    # 1. 初始化数据目录
    print("\n[步骤1] 初始化数据目录...")
    result = init_npc_data_directories("./npc_data")
    print(f"✅ 基础目录: {result['base_dir']}")
    print(f"   创建了 {len(result['created_directories'])} 个目录")
    
    # 显示路径信息
    paths = get_data_paths("./npc_data")
    print("\n📁 数据路径:")
    print(f"   对话数据库: {paths['dialogue_db']}")
    print(f"   记忆数据库: {paths['memory_db']}")
    print(f"   对话文件: {paths['dialogues_dir']}")
    print(f"   Excel导出: {paths['excel_exports_dir']}")
    
    # 2. 创建NPC (无LLM模式)
    print("\n[步骤2] 创建NPC智能体...")
    npc = create_npc(
        npc_id="blacksmith",
        name="老铁匠",
        role="铁匠",
        traits=["严肃", "专业", "热心"],
        background="在这个镇上打铁30年，见证了无数冒险者的成长。手艺精湛，对武器锻造有独到见解。",
        speech_style="说话干脆利落，常用专业术语，偶尔会讲述过去的故事",
        knowledge=["武器锻造", "金属材料", "镇上历史"],
        secrets=["知道镇长年轻时的秘密", "藏有传说中的锻造图纸"],
        data_dir="./npc_data",
        llm=None  # 无LLM模式，使用默认回复
    )
    print(f"✅ NPC创建成功: {npc.personality.name} ({npc.personality.role})")
    
    # 3. 进行对话
    print("\n[步骤3] 进行对话...")
    player_id = "player_001"
    
    # 第一轮对话
    print(f"\n玩家: 你好，老师傅！")
    result = npc.chat(player_id, "你好，老师傅！")
    print(f"{npc.personality.name}: {result['reply']}")
    print(f"   好感度: {result['affinity']['level']} ({result['affinity']['score']}/100)")
    
    # 第二轮对话
    print(f"\n玩家: 能帮我打一把剑吗？")
    result = npc.chat(player_id, "能帮我打一把剑吗？")
    print(f"{npc.personality.name}: {result['reply']}")
    print(f"   好感度: {result['affinity']['level']} ({result['affinity']['score']}/100)")
    
    # 第三轮对话
    print(f"\n玩家: 你打铁多少年了？")
    result = npc.chat(player_id, "你打铁多少年了？")
    print(f"{npc.personality.name}: {result['reply']}")
    print(f"   好感度: {result['affinity']['level']} ({result['affinity']['score']}/100)")
    
    # 4. 查看存储统计
    print("\n[步骤4] 查看存储统计...")
    stats = npc.get_dialogue_stats()
    print(f"✅ 对话统计:")
    print(f"   活跃会话: {stats['active_sessions']}")
    print(f"   记忆摘要: {stats['memory_summary']}")
    if 'dialogue_storage' in stats and stats['dialogue_storage']:
        print(f"   对话存储: {stats['dialogue_storage']}")
    if 'file_memory' in stats and stats['file_memory']:
        print(f"   文件存储: {stats['file_memory']}")
    
    # 5. 获取对话历史
    print("\n[步骤5] 获取对话历史...")
    history = npc.get_dialogue_history(player_id, limit=10)
    print(f"✅ 对话历史 ({len(history)} 条):")
    for msg in history[-4:]:  # 显示最近4条
        role = "玩家" if msg.get('role') == 'user' else npc.personality.name
        content = msg.get('content', '')[:50]
        print(f"   [{role}] {content}...")
    
    # 6. 导出到Excel
    print("\n[步骤6] 导出对话到Excel...")
    excel_path = npc.export_dialogue_history(player_id)
    if excel_path:
        print(f"✅ Excel导出成功: {excel_path}")
    else:
        print("⚠️ Excel导出未启用或失败")
    
    return npc


def demo_storage_manager():
    """演示统一存储管理器"""
    print("\n" + "=" * 60)
    print("📌 演示2: 统一存储管理器")
    print("=" * 60)
    
    from extrator.llm.npc_system import (
        StorageConfig,
        StorageManager,
        create_storage_manager
    )
    
    # 1. 创建存储管理器
    print("\n[步骤1] 创建存储管理器...")
    storage = create_storage_manager(
        base_dir="./npc_data",
        enable_sqlite=True,
        enable_excel_export=True,
        enable_file_memory=True
    )
    print(f"✅ 存储管理器创建成功")
    print(f"   配置: {storage.config.to_dict()}")
    
    # 2. 开始对话会话
    print("\n[步骤2] 开始对话会话...")
    session = storage.start_dialogue_session(
        npc_id="merchant",
        player_id="player_002",
        session_id=f"demo_session_{datetime.now().strftime('%H%M%S')}"
    )
    if session:
        print(f"✅ 会话创建成功: {session.session_id}")
    
    # 3. 保存对话
    print("\n[步骤3] 保存对话记录...")
    storage.save_dialogue(
        session_id=session.session_id if session else "demo_session",
        player_message="有什么好东西卖吗？",
        npc_reply="欢迎欢迎！看看这些上等的药水，保证全镇最低价！",
        npc_id="merchant",
        player_id="player_002",
        metadata={"affinity": 10}
    )
    print("✅ 对话已保存到SQLite和Markdown")
    
    # 4. 保存情景记忆
    print("\n[步骤4] 保存情景记忆...")
    filepath = storage.save_episodic_memory(
        npc_id="merchant",
        player_id="player_002",
        event_type="trade",
        content="玩家购买了一瓶治疗药水，花费50金币",
        importance=0.7,
        metadata={"item": "治疗药水", "price": 50}
    )
    if filepath:
        print(f"✅ 情景记忆已保存: {filepath}")
    
    # 5. 保存语义记忆
    print("\n[步骤5] 保存语义记忆...")
    filepath = storage.save_semantic_memory(
        npc_id="merchant",
        topic="商品价格",
        content="治疗药水: 50金币\n魔力药水: 80金币\n解毒药水: 30金币",
        importance=0.8,
        concepts=["药水", "价格", "交易"]
    )
    if filepath:
        print(f"✅ 语义记忆已保存: {filepath}")
    
    # 6. 获取存储统计
    print("\n[步骤6] 获取存储统计...")
    stats = storage.get_storage_stats("merchant")
    print(f"✅ 存储统计:")
    print(f"   配置: {stats['config']['base_dir']}")
    if stats['dialogue_storage']:
        print(f"   对话存储: {stats['dialogue_storage']}")
    if stats['file_memory_store']:
        print(f"   文件存储: {stats['file_memory_store']}")
    
    return storage


def demo_npc_manager():
    """演示NPC管理器"""
    print("\n" + "=" * 60)
    print("📌 演示3: NPC管理器")
    print("=" * 60)
    
    from extrator.llm.npc_system import (
        NPCManager,
        NPCManagerConfig,
        NPC_TEMPLATES,
        create_template_npc
    )
    
    # 1. 查看预定义模板
    print("\n[步骤1] 预定义NPC模板:")
    for name, template in NPC_TEMPLATES.items():
        personality = template.get("personality", {})
        print(f"   - {name}: {personality.get('name', name)} ({personality.get('role', '')})")
    
    # 2. 创建NPC管理器
    print("\n[步骤2] 创建NPC管理器...")
    config = NPCManagerConfig(
        data_dir="./npc_data",
        config_dir="./npc_configs",
        enable_batch_generation=False  # 无LLM时禁用批量生成
    )
    manager = NPCManager(config, llm=None)
    print(f"✅ 管理器创建成功")
    print(f"   已加载配置: {len(manager.npc_configs)} 个")
    
    # 3. 注册新NPC
    print("\n[步骤3] 注册新NPC...")
    manager.register_npc("herbalist", {
        "personality": {
            "name": "草药师",
            "role": "药剂师",
            "age": 45,
            "gender": "女",
            "traits": ["温和", "博学", "神秘"],
            "background": "精通各种草药和药剂配方，据说能治愈任何疾病。",
            "speech_style": "说话轻柔，常引用古老的谚语",
            "knowledge": ["草药学", "药剂配方", "疾病治疗"],
            "greeting": "欢迎来到我的小店，需要什么药材吗？"
        }
    })
    print("✅ 草药师已注册")
    
    # 4. 获取所有NPC状态
    print("\n[步骤4] 获取所有NPC状态...")
    status = manager.get_all_status()
    print(f"✅ NPC状态:")
    print(f"   总配置数: {status['total_configs']}")
    print(f"   已加载数: {status['loaded_npcs']}")
    for npc_id, npc_status in status['npcs'].items():
        print(f"   - {npc_id}: {npc_status['name']} (已加载: {npc_status['loaded']})")
    
    # 5. 与NPC对话
    print("\n[步骤5] 与NPC对话...")
    result = manager.chat("herbalist", "player_003", "你好，我需要一些治疗药水")
    if result.get("success"):
        print(f"✅ 对话成功:")
        print(f"   NPC: {result.get('npc_name', 'herbalist')}")
        print(f"   回复: {result.get('reply', '')}")
    else:
        print(f"⚠️ 对话失败: {result.get('error', '未知错误')}")
    
    return manager


def demo_file_structure():
    """演示生成的文件结构"""
    print("\n" + "=" * 60)
    print("📌 演示4: 查看生成的文件结构")
    print("=" * 60)
    
    import os
    from pathlib import Path
    
    base_dir = Path("./npc_data")
    
    if not base_dir.exists():
        print("⚠️ npc_data目录不存在，请先运行其他演示")
        return
    
    print(f"\n📁 {base_dir}/")
    
    def print_tree(path, prefix=""):
        items = sorted(path.iterdir(), key=lambda x: (x.is_file(), x.name))
        for i, item in enumerate(items):
            is_last = i == len(items) - 1
            current_prefix = "└── " if is_last else "├── "
            next_prefix = "    " if is_last else "│   "
            
            if item.is_dir():
                # 检查目录是否为空
                has_content = any(item.iterdir()) if item.exists() else False
                marker = "/" if has_content else "/ (空)"
                print(f"{prefix}{current_prefix}{item.name}{marker}")
                if has_content and len(list(item.iterdir())) <= 10:
                    print_tree(item, prefix + next_prefix)
            else:
                size = item.stat().st_size
                size_str = f"{size}B" if size < 1024 else f"{size/1024:.1f}KB"
                print(f"{prefix}{current_prefix}{item.name} ({size_str})")
    
    print_tree(base_dir)


def main():
    """主函数"""
    print("\n" + "=" * 60)
    print("🎮 NPC系统完整示例")
    print("=" * 60)
    print("""
本示例演示NPC系统的完整功能:
1. 基本NPC创建和对话
2. 统一存储管理器
3. NPC管理器
4. 查看生成的文件结构

注意: 本示例使用无LLM模式，NPC回复为默认文本。
要获得智能回复，请配置LLM (如 ChatOllama)。
""")
    
    try:
        # 演示1: 基本NPC
        npc = demo_basic_npc()
        
        # 演示2: 存储管理器
        storage = demo_storage_manager()
        
        # 演示3: NPC管理器
        manager = demo_npc_manager()
        
        # 演示4: 文件结构
        demo_file_structure()
        
        print("\n" + "=" * 60)
        print("✅ 所有演示完成!")
        print("=" * 60)
        print("""
生成的数据文件:
- ./npc_data/databases/*.db     - SQLite数据库
- ./npc_data/memories/**/*.md   - Markdown记忆文件
- ./npc_data/exports/excel/*.xlsx - Excel导出文件
- ./npc_configs/*.json          - NPC配置文件

下一步:
1. 配置LLM以获得智能回复
2. 在Django views中使用NPC系统
3. 与游戏引擎集成
""")
        
    except Exception as e:
        import traceback
        print(f"\n❌ 演示出错: {e}")
        traceback.print_exc()


if __name__ == "__main__":
    main()
