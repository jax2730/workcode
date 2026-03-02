"""
FileMemoryStore - 文件系统记忆存储
基于 HelloAgents 第9章 filesystem-context 设计

功能:
- 将记忆存储为Markdown文件
- 支持记忆的持久化和版本控制
- 人类可读的记忆格式
- 与MemoryTool集成
"""

import os
import json
import yaml
from datetime import datetime
from pathlib import Path
from typing import List, Dict, Any, Optional
from dataclasses import dataclass, asdict


# ==================== 数据结构 ====================

@dataclass
class FileMemoryConfig:
    """文件记忆配置"""
    base_dir: str = "./npc_data/memories"
    
    # 子目录结构
    working_dir: str = "working"      # 工作记忆
    episodic_dir: str = "episodic"    # 情景记忆
    semantic_dir: str = "semantic"    # 语义记忆
    dialogue_dir: str = "dialogues"   # 对话记录
    
    # 文件格式
    use_yaml_header: bool = True      # 使用YAML头部
    max_file_size: int = 100 * 1024   # 最大文件大小 (100KB)


# ==================== 文件记忆存储 ====================

class FileMemoryStore:
    """
    文件系统记忆存储
    
    目录结构:
    npc_data/
    ├── memories/
    │   ├── working/           # 工作记忆 (临时)
    │   │   └── {npc_id}/
    │   │       └── {player_id}/
    │   │           └── session_{date}.md
    │   ├── episodic/          # 情景记忆 (事件)
    │   │   └── {npc_id}/
    │   │       └── {player_id}/
    │   │           └── event_{timestamp}.md
    │   ├── semantic/          # 语义记忆 (知识)
    │   │   └── {npc_id}/
    │   │       └── knowledge_{topic}.md
    │   └── dialogues/         # 对话记录
    │       └── {npc_id}/
    │           └── {player_id}/
    │               └── dialogue_{date}.md
    ├── npc_configs/           # NPC配置
    ├── dialogue_history.db    # SQLite数据库
    └── dialogue_exports/      # Excel导出
    """
    
    def __init__(self, config: FileMemoryConfig = None):
        self.config = config or FileMemoryConfig()
        self.base_dir = Path(self.config.base_dir)
        self._init_directories()
    
    def _init_directories(self):
        """初始化目录结构"""
        dirs = [
            self.base_dir,
            self.base_dir / self.config.working_dir,
            self.base_dir / self.config.episodic_dir,
            self.base_dir / self.config.semantic_dir,
            self.base_dir / self.config.dialogue_dir,
        ]
        
        for d in dirs:
            d.mkdir(parents=True, exist_ok=True)
        
        print(f"[FileMemoryStore] 初始化目录: {self.base_dir}")
    
    def _get_memory_path(self, memory_type: str, npc_id: str,
                         player_id: str = None, filename: str = None) -> Path:
        """获取记忆文件路径"""
        type_dirs = {
            "working": self.config.working_dir,
            "episodic": self.config.episodic_dir,
            "semantic": self.config.semantic_dir,
            "dialogue": self.config.dialogue_dir,
        }
        
        type_dir = type_dirs.get(memory_type, self.config.working_dir)
        path = self.base_dir / type_dir / npc_id
        
        if player_id:
            path = path / player_id
        
        path.mkdir(parents=True, exist_ok=True)
        
        if filename:
            return path / filename
        return path
    
    def _build_markdown(self, metadata: Dict, content: str) -> str:
        """构建Markdown文件内容"""
        if self.config.use_yaml_header:
            yaml_header = yaml.dump(metadata, allow_unicode=True, sort_keys=False)
            return f"---\n{yaml_header}---\n\n{content}"
        else:
            return content
    
    def _parse_markdown(self, raw_content: str) -> tuple:
        """解析Markdown文件"""
        if raw_content.startswith('---'):
            parts = raw_content.split('---\n', 2)
            if len(parts) >= 3:
                yaml_str = parts[1]
                content = parts[2].strip()
                metadata = yaml.safe_load(yaml_str) or {}
                return metadata, content
        
        return {}, raw_content.strip()
    
    # ==================== 工作记忆 ====================
    
    def save_working_memory(self, npc_id: str, player_id: str,
                            session_id: str, memories: List[Dict]) -> str:
        """保存工作记忆到文件"""
        date_str = datetime.now().strftime('%Y%m%d')
        filename = f"session_{date_str}_{session_id[:8]}.md"
        filepath = self._get_memory_path("working", npc_id, player_id, filename)
        
        metadata = {
            "type": "working_memory",
            "npc_id": npc_id,
            "player_id": player_id,
            "session_id": session_id,
            "created_at": datetime.now().isoformat(),
            "memory_count": len(memories)
        }
        
        content_parts = [f"# 工作记忆 - {npc_id} 与 {player_id}\n"]
        content_parts.append(f"会话ID: {session_id}\n")
        content_parts.append(f"记录时间: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n\n")
        
        for i, mem in enumerate(memories, 1):
            content_parts.append(f"## 记忆 {i}\n")
            content_parts.append(f"- 内容: {mem.get('content', '')}\n")
            content_parts.append(f"- 重要性: {mem.get('importance', 0.5)}\n")
            content_parts.append(f"- 时间: {mem.get('timestamp', '')}\n\n")
        
        content = "".join(content_parts)
        md_content = self._build_markdown(metadata, content)
        
        with open(filepath, 'w', encoding='utf-8') as f:
            f.write(md_content)
        
        return str(filepath)
    
    def load_working_memory(self, npc_id: str, player_id: str,
                            session_id: str = None) -> List[Dict]:
        """加载工作记忆"""
        path = self._get_memory_path("working", npc_id, player_id)
        memories = []
        
        # 获取最新的会话文件
        files = sorted(path.glob("session_*.md"), reverse=True)
        
        if session_id:
            files = [f for f in files if session_id[:8] in f.name]
        
        if not files:
            return []
        
        # 读取最新文件
        latest_file = files[0]
        with open(latest_file, 'r', encoding='utf-8') as f:
            raw_content = f.read()
        
        metadata, content = self._parse_markdown(raw_content)
        
        # 简单解析内容 (实际使用时可以更精细)
        return [{
            "content": content,
            "metadata": metadata,
            "source_file": str(latest_file)
        }]
    
    # ==================== 情景记忆 ====================
    
    def save_episodic_memory(self, npc_id: str, player_id: str,
                             event_type: str, content: str,
                             importance: float = 0.5,
                             metadata: Dict = None) -> str:
        """保存情景记忆"""
        timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
        filename = f"event_{timestamp}_{event_type}.md"
        filepath = self._get_memory_path("episodic", npc_id, player_id, filename)
        
        meta = {
            "type": "episodic_memory",
            "event_type": event_type,
            "npc_id": npc_id,
            "player_id": player_id,
            "importance": importance,
            "timestamp": datetime.now().isoformat(),
            **(metadata or {})
        }
        
        md_content = self._build_markdown(meta, f"# {event_type}\n\n{content}")
        
        with open(filepath, 'w', encoding='utf-8') as f:
            f.write(md_content)
        
        return str(filepath)
    
    def load_episodic_memories(self, npc_id: str, player_id: str = None,
                               event_type: str = None,
                               limit: int = 20) -> List[Dict]:
        """加载情景记忆"""
        if player_id:
            path = self._get_memory_path("episodic", npc_id, player_id)
        else:
            path = self._get_memory_path("episodic", npc_id)
        
        memories = []
        
        pattern = f"event_*_{event_type}.md" if event_type else "event_*.md"
        files = sorted(path.rglob(pattern), reverse=True)[:limit]
        
        for filepath in files:
            try:
                with open(filepath, 'r', encoding='utf-8') as f:
                    raw_content = f.read()
                
                metadata, content = self._parse_markdown(raw_content)
                memories.append({
                    "content": content,
                    "metadata": metadata,
                    "source_file": str(filepath)
                })
            except Exception as e:
                print(f"[FileMemoryStore] 读取文件失败 {filepath}: {e}")
        
        return memories
    
    # ==================== 语义记忆 ====================
    
    def save_semantic_memory(self, npc_id: str, topic: str,
                             content: str, importance: float = 0.7,
                             concepts: List[str] = None,
                             metadata: Dict = None) -> str:
        """保存语义记忆 (知识)"""
        # 清理topic作为文件名
        safe_topic = "".join(c if c.isalnum() or c in '-_' else '_' for c in topic)
        filename = f"knowledge_{safe_topic}.md"
        filepath = self._get_memory_path("semantic", npc_id, filename=filename)
        
        meta = {
            "type": "semantic_memory",
            "topic": topic,
            "npc_id": npc_id,
            "importance": importance,
            "concepts": concepts or [],
            "created_at": datetime.now().isoformat(),
            "updated_at": datetime.now().isoformat(),
            **(metadata or {})
        }
        
        # 如果文件已存在，追加内容
        if filepath.exists():
            with open(filepath, 'r', encoding='utf-8') as f:
                existing = f.read()
            old_meta, old_content = self._parse_markdown(existing)
            
            # 更新元数据
            meta["created_at"] = old_meta.get("created_at", meta["created_at"])
            old_concepts = old_meta.get("concepts", [])
            meta["concepts"] = list(set(old_concepts + (concepts or [])))
            
            # 追加内容
            content = f"{old_content}\n\n---\n\n## 更新 ({datetime.now().strftime('%Y-%m-%d %H:%M')})\n\n{content}"
        
        md_content = self._build_markdown(meta, f"# {topic}\n\n{content}")
        
        with open(filepath, 'w', encoding='utf-8') as f:
            f.write(md_content)
        
        return str(filepath)
    
    def load_semantic_memories(self, npc_id: str, topic: str = None,
                               limit: int = 10) -> List[Dict]:
        """加载语义记忆"""
        path = self._get_memory_path("semantic", npc_id)
        memories = []
        
        if topic:
            safe_topic = "".join(c if c.isalnum() or c in '-_' else '_' for c in topic)
            pattern = f"knowledge_{safe_topic}*.md"
        else:
            pattern = "knowledge_*.md"
        
        files = sorted(path.glob(pattern), reverse=True)[:limit]
        
        for filepath in files:
            try:
                with open(filepath, 'r', encoding='utf-8') as f:
                    raw_content = f.read()
                
                metadata, content = self._parse_markdown(raw_content)
                memories.append({
                    "content": content,
                    "metadata": metadata,
                    "source_file": str(filepath)
                })
            except Exception as e:
                print(f"[FileMemoryStore] 读取文件失败 {filepath}: {e}")
        
        return memories
    
    # ==================== 对话记录 ====================
    
    def save_dialogue(self, npc_id: str, player_id: str,
                      player_message: str, npc_reply: str,
                      session_id: str = None,
                      metadata: Dict = None) -> str:
        """保存对话记录到Markdown文件"""
        date_str = datetime.now().strftime('%Y%m%d')
        filename = f"dialogue_{date_str}.md"
        filepath = self._get_memory_path("dialogue", npc_id, player_id, filename)
        
        timestamp = datetime.now().strftime('%H:%M:%S')
        
        # 对话条目
        dialogue_entry = f"""
### {timestamp}

**玩家**: {player_message}

**{npc_id}**: {npc_reply}

"""
        
        # 如果文件存在，追加内容
        if filepath.exists():
            with open(filepath, 'a', encoding='utf-8') as f:
                f.write(dialogue_entry)
        else:
            # 创建新文件
            meta = {
                "type": "dialogue_record",
                "npc_id": npc_id,
                "player_id": player_id,
                "date": date_str,
                "session_id": session_id or "",
                **(metadata or {})
            }
            
            header = f"# 对话记录 - {npc_id} 与 {player_id}\n\n"
            header += f"日期: {datetime.now().strftime('%Y-%m-%d')}\n\n"
            header += "---\n"
            
            md_content = self._build_markdown(meta, header + dialogue_entry)
            
            with open(filepath, 'w', encoding='utf-8') as f:
                f.write(md_content)
        
        return str(filepath)
    
    def load_dialogues(self, npc_id: str, player_id: str = None,
                       date: str = None, limit: int = 10) -> List[Dict]:
        """加载对话记录"""
        if player_id:
            path = self._get_memory_path("dialogue", npc_id, player_id)
        else:
            path = self._get_memory_path("dialogue", npc_id)
        
        dialogues = []
        
        if date:
            pattern = f"dialogue_{date}.md"
        else:
            pattern = "dialogue_*.md"
        
        files = sorted(path.rglob(pattern), reverse=True)[:limit]
        
        for filepath in files:
            try:
                with open(filepath, 'r', encoding='utf-8') as f:
                    raw_content = f.read()
                
                metadata, content = self._parse_markdown(raw_content)
                dialogues.append({
                    "content": content,
                    "metadata": metadata,
                    "source_file": str(filepath)
                })
            except Exception as e:
                print(f"[FileMemoryStore] 读取文件失败 {filepath}: {e}")
        
        return dialogues
    
    # ==================== 统计和管理 ====================
    
    def get_stats(self, npc_id: str = None) -> Dict[str, Any]:
        """获取存储统计"""
        stats = {
            "base_dir": str(self.base_dir),
            "working_memories": 0,
            "episodic_memories": 0,
            "semantic_memories": 0,
            "dialogue_files": 0,
            "total_size_kb": 0
        }
        
        for memory_type in ["working", "episodic", "semantic", "dialogue"]:
            type_dir = getattr(self.config, f"{memory_type}_dir", memory_type)
            path = self.base_dir / type_dir
            
            if npc_id:
                path = path / npc_id
            
            if path.exists():
                files = list(path.rglob("*.md"))
                stats[f"{memory_type}_memories"] = len(files)
                
                for f in files:
                    stats["total_size_kb"] += f.stat().st_size / 1024
        
        stats["total_size_kb"] = round(stats["total_size_kb"], 2)
        
        return stats
    
    def cleanup_old_working_memories(self, days: int = 7) -> int:
        """清理旧的工作记忆"""
        from datetime import timedelta
        
        cutoff = datetime.now() - timedelta(days=days)
        deleted = 0
        
        working_path = self.base_dir / self.config.working_dir
        
        for filepath in working_path.rglob("session_*.md"):
            try:
                # 从文件名解析日期
                date_str = filepath.stem.split('_')[1]
                file_date = datetime.strptime(date_str, '%Y%m%d')
                
                if file_date < cutoff:
                    filepath.unlink()
                    deleted += 1
            except Exception as e:
                print(f"[FileMemoryStore] 清理文件失败 {filepath}: {e}")
        
        return deleted


# ==================== 便捷函数 ====================

def create_file_memory_store(base_dir: str = "./npc_data/memories") -> FileMemoryStore:
    """创建文件记忆存储"""
    config = FileMemoryConfig(base_dir=base_dir)
    return FileMemoryStore(config)
