"""
NoteTool - 结构化笔记工具
基于 HelloAgents 第9章设计

笔记类型:
- task_state: 任务状态和进度
- blocker: 阻塞问题 (最高优先级)
- action: 行动计划
- conclusion: 结论和发现
- reference: 参考资料
- general: 通用笔记
"""

import os
import json
import yaml
from datetime import datetime
from pathlib import Path
from typing import List, Dict, Any, Optional


class NoteTool:
    """
    结构化笔记工具
    
    使用 Markdown + YAML 混合格式存储
    支持:
    - 创建/读取/更新/删除笔记
    - 按类型/标签搜索
    - 笔记摘要统计
    """
    
    NOTE_TYPES = ["task_state", "blocker", "action", "conclusion", "reference", "general"]
    
    def __init__(self, workspace: str = "./npc_notes"):
        self.workspace = Path(workspace)
        self.workspace.mkdir(parents=True, exist_ok=True)
        
        self.index_file = self.workspace / "notes_index.json"
        self.index: Dict[str, dict] = {}
        
        self._load_index()
    
    def _load_index(self):
        """加载索引"""
        if self.index_file.exists():
            try:
                with open(self.index_file, 'r', encoding='utf-8') as f:
                    self.index = json.load(f)
            except:
                self.index = {}
        else:
            self.index = {}
    
    def _save_index(self):
        """保存索引"""
        with open(self.index_file, 'w', encoding='utf-8') as f:
            json.dump(self.index, f, ensure_ascii=False, indent=2)
    
    def execute(self, action: str, **kwargs) -> Any:
        """执行笔记操作"""
        actions = {
            "create": self._create,
            "read": self._read,
            "update": self._update,
            "delete": self._delete,
            "search": self._search,
            "list": self._list,
            "summary": self._summary,
        }
        
        if action not in actions:
            raise ValueError(f"未知操作: {action}, 可用操作: {list(actions.keys())}")
        
        return actions[action](**kwargs)
    
    def _create(self, title: str, content: str,
                note_type: str = "general",
                tags: List[str] = None,
                user_id: str = "",
                session_id: str = "") -> str:
        """创建笔记"""
        # 验证类型
        if note_type not in self.NOTE_TYPES:
            note_type = "general"
        
        # 生成ID
        timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
        note_id = f"note_{timestamp}_{len(self.index)}"
        
        # 构建元数据
        metadata = {
            "id": note_id,
            "title": title,
            "type": note_type,
            "tags": tags or [],
            "user_id": user_id,
            "session_id": session_id,
            "created_at": datetime.now().isoformat(),
            "updated_at": datetime.now().isoformat()
        }
        
        # 构建Markdown内容
        md_content = self._build_markdown(metadata, content)
        
        # 保存文件
        file_path = self.workspace / f"{note_id}.md"
        with open(file_path, 'w', encoding='utf-8') as f:
            f.write(md_content)
        
        # 更新索引
        metadata["file_path"] = str(file_path)
        self.index[note_id] = metadata
        self._save_index()
        
        return note_id
    
    def _read(self, note_id: str) -> Dict:
        """读取笔记"""
        if note_id not in self.index:
            raise ValueError(f"笔记不存在: {note_id}")
        
        file_path = self.index[note_id]["file_path"]
        
        with open(file_path, 'r', encoding='utf-8') as f:
            raw_content = f.read()
        
        metadata, content = self._parse_markdown(raw_content)
        
        return {
            "metadata": metadata,
            "content": content
        }
    
    def _update(self, note_id: str,
                title: str = None,
                content: str = None,
                note_type: str = None,
                tags: List[str] = None) -> str:
        """更新笔记"""
        if note_id not in self.index:
            raise ValueError(f"笔记不存在: {note_id}")
        
        # 读取现有笔记
        note = self._read(note_id)
        metadata = note["metadata"]
        old_content = note["content"]
        
        # 更新字段
        if title:
            metadata["title"] = title
        if note_type and note_type in self.NOTE_TYPES:
            metadata["type"] = note_type
        if tags is not None:
            metadata["tags"] = tags
        if content is not None:
            old_content = content
        
        metadata["updated_at"] = datetime.now().isoformat()
        
        # 重新构建并保存
        md_content = self._build_markdown(metadata, old_content)
        file_path = self.index[note_id]["file_path"]
        
        with open(file_path, 'w', encoding='utf-8') as f:
            f.write(md_content)
        
        # 更新索引
        self.index[note_id].update({
            "title": metadata["title"],
            "type": metadata["type"],
            "tags": metadata["tags"],
            "updated_at": metadata["updated_at"]
        })
        self._save_index()
        
        return f"笔记已更新: {metadata['title']}"
    
    def _delete(self, note_id: str) -> str:
        """删除笔记"""
        if note_id not in self.index:
            raise ValueError(f"笔记不存在: {note_id}")
        
        file_path = self.index[note_id]["file_path"]
        title = self.index[note_id].get("title", note_id)
        
        # 删除文件
        if os.path.exists(file_path):
            os.remove(file_path)
        
        # 更新索引
        del self.index[note_id]
        self._save_index()
        
        return f"笔记已删除: {title}"
    
    def _search(self, query: str = "",
                note_type: str = None,
                tags: List[str] = None,
                user_id: str = "",
                limit: int = 10) -> List[Dict]:
        """搜索笔记"""
        results = []
        query_lower = query.lower()
        
        for note_id, metadata in self.index.items():
            # 用户过滤
            if user_id and metadata.get("user_id") != user_id:
                continue
            
            # 类型过滤
            if note_type and metadata.get("type") != note_type:
                continue
            
            # 标签过滤
            if tags:
                note_tags = set(metadata.get("tags", []))
                if not note_tags.intersection(tags):
                    continue
            
            # 关键词搜索
            if query:
                try:
                    note = self._read(note_id)
                    content = note["content"]
                    title = metadata.get("title", "")
                    
                    if query_lower not in title.lower() and query_lower not in content.lower():
                        continue
                except:
                    continue
            
            # 读取内容
            try:
                note = self._read(note_id)
                results.append({
                    "note_id": note_id,
                    "title": metadata.get("title"),
                    "type": metadata.get("type"),
                    "tags": metadata.get("tags", []),
                    "content": note["content"],
                    "updated_at": metadata.get("updated_at")
                })
            except:
                continue
        
        # 按更新时间排序
        results.sort(key=lambda x: x.get("updated_at", ""), reverse=True)
        
        return results[:limit]
    
    def _list(self, note_type: str = None,
              tags: List[str] = None,
              user_id: str = "",
              limit: int = 20) -> List[Dict]:
        """列出笔记"""
        results = []
        
        for note_id, metadata in self.index.items():
            # 用户过滤
            if user_id and metadata.get("user_id") != user_id:
                continue
            
            # 类型过滤
            if note_type and metadata.get("type") != note_type:
                continue
            
            # 标签过滤
            if tags:
                note_tags = set(metadata.get("tags", []))
                if not note_tags.intersection(tags):
                    continue
            
            results.append({
                "note_id": note_id,
                "title": metadata.get("title"),
                "type": metadata.get("type"),
                "tags": metadata.get("tags", []),
                "updated_at": metadata.get("updated_at")
            })
        
        # 按更新时间排序
        results.sort(key=lambda x: x.get("updated_at", ""), reverse=True)
        
        return results[:limit]
    
    def _summary(self, user_id: str = "") -> Dict[str, Any]:
        """笔记摘要统计"""
        total_count = 0
        type_counts = {}
        
        for metadata in self.index.values():
            if user_id and metadata.get("user_id") != user_id:
                continue
            
            total_count += 1
            note_type = metadata.get("type", "general")
            type_counts[note_type] = type_counts.get(note_type, 0) + 1
        
        # 最近笔记
        all_notes = list(self.index.values())
        if user_id:
            all_notes = [n for n in all_notes if n.get("user_id") == user_id]
        
        all_notes.sort(key=lambda x: x.get("updated_at", ""), reverse=True)
        recent_notes = all_notes[:5]
        
        return {
            "total_notes": total_count,
            "type_distribution": type_counts,
            "recent_notes": [
                {
                    "id": note.get("id"),
                    "title": note.get("title", ""),
                    "type": note.get("type"),
                    "updated_at": note.get("updated_at")
                }
                for note in recent_notes
            ]
        }
    
    def _build_markdown(self, metadata: dict, content: str) -> str:
        """构建Markdown文件内容 (YAML + 正文)"""
        # 移除file_path避免循环
        meta_copy = {k: v for k, v in metadata.items() if k != "file_path"}
        yaml_header = yaml.dump(meta_copy, allow_unicode=True, sort_keys=False)
        
        return f"---\n{yaml_header}---\n\n{content}"
    
    def _parse_markdown(self, raw_content: str) -> tuple:
        """解析Markdown文件 (分离YAML和正文)"""
        parts = raw_content.split('---\n', 2)
        
        if len(parts) >= 3:
            yaml_str = parts[1]
            content = parts[2].strip()
            metadata = yaml.safe_load(yaml_str) or {}
        else:
            metadata = {}
            content = raw_content.strip()
        
        return metadata, content


# ==================== 便捷函数 ====================

def create_note_tool(workspace: str = "./npc_notes") -> NoteTool:
    """创建笔记工具"""
    return NoteTool(workspace)
