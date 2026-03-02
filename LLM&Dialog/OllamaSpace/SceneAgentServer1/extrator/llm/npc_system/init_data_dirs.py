"""
数据目录初始化脚本
创建NPC系统所需的完整目录结构
"""

import os
import json
from pathlib import Path
from datetime import datetime


def init_npc_data_directories(base_dir: str = None) -> dict:
    """
    初始化NPC数据目录结构
    
    目录结构:
    npc_data/
    ├── memories/                    # 记忆存储 (Markdown文件)
    │   ├── working/                 # 工作记忆 (临时，按会话)
    │   │   └── {npc_id}/
    │   │       └── {player_id}/
    │   ├── episodic/                # 情景记忆 (事件记录)
    │   │   └── {npc_id}/
    │   │       └── {player_id}/
    │   ├── semantic/                # 语义记忆 (知识库)
    │   │   └── {npc_id}/
    │   └── dialogues/               # 对话记录 (Markdown)
    │       └── {npc_id}/
    │           └── {player_id}/
    ├── databases/                   # 数据库文件
    │   ├── dialogue_history.db      # 对话历史 (SQLite)
    │   ├── npc_memory.db            # NPC记忆 (SQLite)
    │   └── npc_relationship.db      # 好感度 (SQLite)
    ├── exports/                     # 导出文件
    │   ├── excel/                   # Excel导出
    │   └── json/                    # JSON导出
    ├── knowledge_base/              # RAG知识库
    │   └── {npc_id}/
    ├── rag_index/                   # RAG向量索引
    │   └── {npc_id}/
    ├── notes/                       # 结构化笔记
    │   └── {npc_id}/
    ├── configs/                     # NPC配置文件
    │   └── {npc_id}.json
    └── logs/                        # 日志文件
        └── npc_system.log
    
    Returns:
        dict: 创建的目录信息
    """
    if base_dir is None:
        # 默认在SceneAgentServer目录下
        base_dir = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
        base_dir = os.path.join(base_dir, "npc_data")
    
    base_path = Path(base_dir)
    
    # 定义目录结构
    directories = [
        # 记忆存储
        "memories/working",
        "memories/episodic", 
        "memories/semantic",
        "memories/dialogues",
        
        # 数据库
        "databases",
        
        # 导出
        "exports/excel",
        "exports/json",
        
        # RAG
        "knowledge_base",
        "rag_index",
        
        # 笔记
        "notes",
        
        # 配置
        "configs",
        
        # 日志
        "logs",
    ]
    
    created_dirs = []
    
    for dir_path in directories:
        full_path = base_path / dir_path
        full_path.mkdir(parents=True, exist_ok=True)
        created_dirs.append(str(full_path))
    
    # 创建README文件
    readme_content = f"""# NPC System Data Directory

Created: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}

## 目录结构说明

### memories/ - 记忆存储
- `working/` - 工作记忆 (临时，按会话存储)
- `episodic/` - 情景记忆 (事件记录)
- `semantic/` - 语义记忆 (知识库)
- `dialogues/` - 对话记录 (Markdown格式)

### databases/ - 数据库文件
- `dialogue_history.db` - 对话历史 (SQLite)
- `npc_memory.db` - NPC记忆 (SQLite)
- `npc_relationship.db` - 好感度系统 (SQLite)

### exports/ - 导出文件
- `excel/` - Excel格式导出
- `json/` - JSON格式导出

### knowledge_base/ - RAG知识库
存放NPC的专业知识文档 (txt, md, json)

### rag_index/ - RAG向量索引
FAISS向量索引文件

### notes/ - 结构化笔记
NoteTool生成的Markdown笔记

### configs/ - NPC配置
NPC人设配置文件 (JSON格式)

### logs/ - 日志文件
系统运行日志

## 使用说明

1. NPC配置文件放在 `configs/` 目录
2. 知识库文档放在 `knowledge_base/{{npc_id}}/` 目录
3. 对话记录会自动保存到 `memories/dialogues/` 和 `databases/`
4. 可以通过 `exports/` 目录导出数据

## 文件格式

### 记忆文件 (Markdown + YAML)
```markdown
---
type: episodic_memory
npc_id: merchant
player_id: player_001
timestamp: 2024-01-15T10:30:00
importance: 0.8
---

# 事件标题

事件内容...
```

### NPC配置 (JSON)
```json
{{
  "npc_id": "merchant",
  "personality": {{
    "name": "精明商人",
    "role": "商人",
    "traits": ["精明", "健谈"],
    ...
  }}
}}
```
"""
    
    readme_path = base_path / "README.md"
    with open(readme_path, 'w', encoding='utf-8') as f:
        f.write(readme_content)
    
    # 创建.gitkeep文件保持空目录
    for dir_path in directories:
        gitkeep_path = base_path / dir_path / ".gitkeep"
        if not any((base_path / dir_path).iterdir()):
            gitkeep_path.touch()
    
    return {
        "base_dir": str(base_path),
        "created_directories": created_dirs,
        "readme": str(readme_path)
    }


def init_npc_directories(base_dir: str, npc_id: str) -> dict:
    """
    为特定NPC初始化目录
    
    Args:
        base_dir: 基础数据目录
        npc_id: NPC ID
        
    Returns:
        dict: 创建的目录信息
    """
    base_path = Path(base_dir)
    
    npc_dirs = [
        f"memories/working/{npc_id}",
        f"memories/episodic/{npc_id}",
        f"memories/semantic/{npc_id}",
        f"memories/dialogues/{npc_id}",
        f"knowledge_base/{npc_id}",
        f"rag_index/{npc_id}",
        f"notes/{npc_id}",
    ]
    
    created_dirs = []
    
    for dir_path in npc_dirs:
        full_path = base_path / dir_path
        full_path.mkdir(parents=True, exist_ok=True)
        created_dirs.append(str(full_path))
    
    return {
        "npc_id": npc_id,
        "created_directories": created_dirs
    }


def get_data_paths(base_dir: str = None) -> dict:
    """
    获取所有数据路径
    
    Returns:
        dict: 各类数据的路径
    """
    if base_dir is None:
        base_dir = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
        base_dir = os.path.join(base_dir, "npc_data")
    
    base_path = Path(base_dir)
    
    return {
        "base_dir": str(base_path),
        
        # 记忆
        "memories_dir": str(base_path / "memories"),
        "working_memory_dir": str(base_path / "memories/working"),
        "episodic_memory_dir": str(base_path / "memories/episodic"),
        "semantic_memory_dir": str(base_path / "memories/semantic"),
        "dialogues_dir": str(base_path / "memories/dialogues"),
        
        # 数据库
        "databases_dir": str(base_path / "databases"),
        "dialogue_db": str(base_path / "databases/dialogue_history.db"),
        "memory_db": str(base_path / "databases/npc_memory.db"),
        "relationship_db": str(base_path / "databases/npc_relationship.db"),
        
        # 导出
        "exports_dir": str(base_path / "exports"),
        "excel_exports_dir": str(base_path / "exports/excel"),
        "json_exports_dir": str(base_path / "exports/json"),
        
        # RAG
        "knowledge_base_dir": str(base_path / "knowledge_base"),
        "rag_index_dir": str(base_path / "rag_index"),
        
        # 其他
        "notes_dir": str(base_path / "notes"),
        "configs_dir": str(base_path / "configs"),
        "logs_dir": str(base_path / "logs"),
    }


# ==================== 命令行入口 ====================

if __name__ == "__main__":
    import argparse
    
    parser = argparse.ArgumentParser(description="初始化NPC系统数据目录")
    parser.add_argument("--base-dir", type=str, default=None, help="基础数据目录")
    parser.add_argument("--npc-id", type=str, default=None, help="初始化特定NPC的目录")
    
    args = parser.parse_args()
    
    # 初始化基础目录
    result = init_npc_data_directories(args.base_dir)
    print(f"✅ 基础目录初始化完成: {result['base_dir']}")
    print(f"   创建了 {len(result['created_directories'])} 个目录")
    
    # 如果指定了NPC，初始化NPC目录
    if args.npc_id:
        npc_result = init_npc_directories(result['base_dir'], args.npc_id)
        print(f"✅ NPC目录初始化完成: {args.npc_id}")
        print(f"   创建了 {len(npc_result['created_directories'])} 个目录")
    
    # 显示路径信息
    paths = get_data_paths(result['base_dir'])
    print("\n📁 数据路径:")
    for key, value in paths.items():
        if key != "base_dir":
            print(f"   {key}: {value}")
