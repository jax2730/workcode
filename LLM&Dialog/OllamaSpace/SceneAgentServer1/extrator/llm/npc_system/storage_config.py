"""
StorageConfig - 统一存储配置管理器
整合所有存储模块的配置和路径管理

功能:
- 统一管理所有数据存储路径
- 自动初始化目录结构
- 提供各存储模块的工厂方法
- 与原服务器存储方式兼容 (SQLite + Excel)
"""

import os
from pathlib import Path
from dataclasses import dataclass, field
from typing import Dict, Any, Optional


@dataclass
class StorageConfig:
    """
    统一存储配置
    
    目录结构:
    {base_dir}/
    ├── memories/                    # 记忆存储 (Markdown文件)
    │   ├── working/{npc_id}/{player_id}/
    │   ├── episodic/{npc_id}/{player_id}/
    │   ├── semantic/{npc_id}/
    │   └── dialogues/{npc_id}/{player_id}/
    ├── databases/                   # 数据库文件
    │   ├── dialogue_history.db      # 对话历史 (SQLite)
    │   ├── npc_memory.db            # NPC记忆 (SQLite)
    │   └── npc_relationship.db      # 好感度 (SQLite)
    ├── exports/                     # 导出文件
    │   ├── excel/                   # Excel导出
    │   └── json/                    # JSON导出
    ├── knowledge_base/{npc_id}/     # RAG知识库
    ├── rag_index/{npc_id}/          # RAG向量索引
    ├── notes/{npc_id}/              # 结构化笔记
    ├── configs/                     # NPC配置文件
    └── logs/                        # 日志文件
    """
    
    # 基础目录
    base_dir: str = "./npc_data"
    
    # 是否自动初始化目录
    auto_init: bool = True
    
    # 存储开关
    enable_sqlite: bool = True           # 启用SQLite存储
    enable_excel_export: bool = True     # 启用Excel导出
    enable_file_memory: bool = True      # 启用文件记忆存储
    enable_dialogue_logging: bool = True # 启用对话日志
    
    # 自动导出设置
    auto_export_excel: bool = False      # 会话结束时自动导出Excel
    auto_save_dialogue_md: bool = True   # 自动保存对话到Markdown
    
    # 记忆整合设置
    auto_consolidate: bool = True        # 自动整合工作记忆到情景记忆
    consolidate_threshold: float = 0.6   # 整合阈值
    
    # 文件清理设置
    cleanup_working_memory_days: int = 7  # 工作记忆文件保留天数
    
    def __post_init__(self):
        """初始化后处理"""
        self.base_path = Path(self.base_dir)
        
        if self.auto_init:
            self._init_directories()
    
    def _init_directories(self):
        """初始化目录结构"""
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
            
            # 其他
            "notes",
            "configs",
            "logs",
        ]
        
        for dir_path in directories:
            (self.base_path / dir_path).mkdir(parents=True, exist_ok=True)
    
    def init_npc_directories(self, npc_id: str):
        """为特定NPC初始化目录"""
        npc_dirs = [
            f"memories/working/{npc_id}",
            f"memories/episodic/{npc_id}",
            f"memories/semantic/{npc_id}",
            f"memories/dialogues/{npc_id}",
            f"knowledge_base/{npc_id}",
            f"rag_index/{npc_id}",
            f"notes/{npc_id}",
        ]
        
        for dir_path in npc_dirs:
            (self.base_path / dir_path).mkdir(parents=True, exist_ok=True)
    
    # ==================== 路径获取方法 ====================
    
    @property
    def memories_dir(self) -> str:
        return str(self.base_path / "memories")
    
    @property
    def databases_dir(self) -> str:
        return str(self.base_path / "databases")
    
    @property
    def exports_dir(self) -> str:
        return str(self.base_path / "exports")
    
    @property
    def configs_dir(self) -> str:
        return str(self.base_path / "configs")
    
    @property
    def logs_dir(self) -> str:
        return str(self.base_path / "logs")
    
    # 数据库路径
    @property
    def dialogue_db_path(self) -> str:
        return str(self.base_path / "databases" / "dialogue_history.db")
    
    @property
    def memory_db_path(self) -> str:
        return str(self.base_path / "databases" / "npc_memory.db")
    
    @property
    def relationship_db_path(self) -> str:
        return str(self.base_path / "databases" / "npc_relationship.db")
    
    # Excel导出路径
    @property
    def excel_exports_dir(self) -> str:
        return str(self.base_path / "exports" / "excel")
    
    @property
    def json_exports_dir(self) -> str:
        return str(self.base_path / "exports" / "json")
    
    # NPC特定路径
    def get_npc_memory_dir(self, npc_id: str, memory_type: str = "episodic") -> str:
        return str(self.base_path / "memories" / memory_type / npc_id)
    
    def get_npc_dialogue_dir(self, npc_id: str) -> str:
        return str(self.base_path / "memories" / "dialogues" / npc_id)
    
    def get_npc_knowledge_dir(self, npc_id: str) -> str:
        return str(self.base_path / "knowledge_base" / npc_id)
    
    def get_npc_rag_index_dir(self, npc_id: str) -> str:
        return str(self.base_path / "rag_index" / npc_id)
    
    def get_npc_notes_dir(self, npc_id: str) -> str:
        return str(self.base_path / "notes" / npc_id)
    
    def get_npc_config_path(self, npc_id: str) -> str:
        return str(self.base_path / "configs" / f"{npc_id}.json")
    
    def to_dict(self) -> Dict[str, Any]:
        """转换为字典"""
        return {
            "base_dir": self.base_dir,
            "dialogue_db": self.dialogue_db_path,
            "memory_db": self.memory_db_path,
            "relationship_db": self.relationship_db_path,
            "memories_dir": self.memories_dir,
            "exports_dir": self.exports_dir,
            "configs_dir": self.configs_dir,
            "logs_dir": self.logs_dir,
        }


class StorageManager:
    """
    存储管理器
    
    整合所有存储模块，提供统一的存储接口
    """
    
    def __init__(self, config: StorageConfig = None):
        self.config = config or StorageConfig()
        
        # 延迟初始化的存储模块
        self._dialogue_storage = None
        self._file_memory_store = None
        
        print(f"[StorageManager] 初始化完成，基础目录: {self.config.base_dir}")
    
    @property
    def dialogue_storage(self):
        """获取对话存储模块 (懒加载)"""
        if self._dialogue_storage is None and self.config.enable_sqlite:
            from .dialogue_storage import DialogueStorage
            self._dialogue_storage = DialogueStorage(
                db_path=self.config.dialogue_db_path,
                export_dir=self.config.excel_exports_dir,
                auto_export=self.config.auto_export_excel
            )
        return self._dialogue_storage
    
    @property
    def file_memory_store(self):
        """获取文件记忆存储模块 (懒加载)"""
        if self._file_memory_store is None and self.config.enable_file_memory:
            from .file_memory_store import FileMemoryStore, FileMemoryConfig
            file_config = FileMemoryConfig(base_dir=self.config.memories_dir)
            self._file_memory_store = FileMemoryStore(file_config)
        return self._file_memory_store
    
    def create_memory_tool(self, npc_id: str = "", user_id: str = ""):
        """创建记忆工具实例"""
        from .memory_tool import MemoryTool, MemoryConfig
        
        memory_config = MemoryConfig(db_path=self.config.memory_db_path)
        return MemoryTool(memory_config, user_id)
    
    def create_relationship_manager(self, llm=None):
        """创建好感度管理器实例"""
        from .relationship_manager import RelationshipManager
        return RelationshipManager(self.config.relationship_db_path, llm)
    
    def create_note_tool(self, npc_id: str):
        """创建笔记工具实例"""
        from .note_tool import NoteTool
        return NoteTool(workspace=self.config.get_npc_notes_dir(npc_id))
    
    def create_rag_tool(self, npc_id: str, llm=None):
        """创建RAG工具实例"""
        knowledge_dir = self.config.get_npc_knowledge_dir(npc_id)
        index_dir = self.config.get_npc_rag_index_dir(npc_id)
        
        # 检查知识库是否存在
        if not os.path.exists(knowledge_dir):
            return None
        
        try:
            from .rag_tool import RAGTool, RAGConfig
            rag_config = RAGConfig(
                knowledge_base_path=knowledge_dir,
                index_path=index_dir
            )
            return RAGTool(rag_config, llm)
        except Exception as e:
            print(f"[StorageManager] RAG初始化失败: {e}")
            return None
    
    def create_context_builder(self, npc_id: str, 
                               memory_tool=None,
                               rag_tool=None,
                               note_tool=None,
                               relationship_manager=None,
                               role_description: str = "",
                               llm=None):
        """创建上下文构建器实例"""
        from .context_builder import ContextBuilder, ContextConfig
        
        context_config = ContextConfig(
            max_tokens=3000,
            role_description=role_description
        )
        
        return ContextBuilder(
            config=context_config,
            memory_tool=memory_tool,
            rag_tool=rag_tool,
            note_tool=note_tool,
            relationship_manager=relationship_manager,
            llm=llm
        )
    
    # ==================== 对话存储操作 ====================
    
    def start_dialogue_session(self, npc_id: str, player_id: str,
                               session_id: str = None):
        """开始对话会话"""
        if self.dialogue_storage:
            return self.dialogue_storage.start_session(npc_id, player_id, session_id)
        return None
    
    def save_dialogue(self, session_id: str, 
                      player_message: str, npc_reply: str,
                      npc_id: str = "", player_id: str = "",
                      metadata: Dict = None):
        """保存对话记录"""
        # 保存到SQLite
        if self.dialogue_storage:
            self.dialogue_storage.add_dialogue_turn(
                session_id=session_id,
                player_message=player_message,
                npc_reply=npc_reply,
                npc_id=npc_id,
                player_id=player_id,
                metadata=metadata
            )
        
        # 保存到Markdown文件
        if self.file_memory_store and self.config.auto_save_dialogue_md:
            self.file_memory_store.save_dialogue(
                npc_id=npc_id,
                player_id=player_id,
                player_message=player_message,
                npc_reply=npc_reply,
                session_id=session_id,
                metadata=metadata
            )
    
    def end_dialogue_session(self, session_id: str, summary: str = ""):
        """结束对话会话"""
        if self.dialogue_storage:
            return self.dialogue_storage.end_session(session_id, summary)
        return None
    
    def get_dialogue_history(self, session_id: str = None,
                             npc_id: str = None,
                             player_id: str = None,
                             limit: int = 50):
        """获取对话历史"""
        if self.dialogue_storage:
            return self.dialogue_storage.get_history(
                session_id=session_id,
                npc_id=npc_id,
                player_id=player_id,
                limit=limit
            )
        return []
    
    def export_dialogues_to_excel(self, session_id: str = None,
                                  filename: str = None) -> Optional[str]:
        """导出对话到Excel"""
        if self.dialogue_storage:
            return self.dialogue_storage.export_to_excel(session_id, filename)
        return None
    
    # ==================== 记忆文件操作 ====================
    
    def save_episodic_memory(self, npc_id: str, player_id: str,
                             event_type: str, content: str,
                             importance: float = 0.5,
                             metadata: Dict = None) -> Optional[str]:
        """保存情景记忆到文件"""
        if self.file_memory_store:
            return self.file_memory_store.save_episodic_memory(
                npc_id=npc_id,
                player_id=player_id,
                event_type=event_type,
                content=content,
                importance=importance,
                metadata=metadata
            )
        return None
    
    def save_semantic_memory(self, npc_id: str, topic: str,
                             content: str, importance: float = 0.7,
                             concepts: list = None,
                             metadata: Dict = None) -> Optional[str]:
        """保存语义记忆到文件"""
        if self.file_memory_store:
            return self.file_memory_store.save_semantic_memory(
                npc_id=npc_id,
                topic=topic,
                content=content,
                importance=importance,
                concepts=concepts,
                metadata=metadata
            )
        return None
    
    def load_episodic_memories(self, npc_id: str, player_id: str = None,
                               event_type: str = None,
                               limit: int = 20):
        """加载情景记忆文件"""
        if self.file_memory_store:
            return self.file_memory_store.load_episodic_memories(
                npc_id=npc_id,
                player_id=player_id,
                event_type=event_type,
                limit=limit
            )
        return []
    
    def load_semantic_memories(self, npc_id: str, topic: str = None,
                               limit: int = 10):
        """加载语义记忆文件"""
        if self.file_memory_store:
            return self.file_memory_store.load_semantic_memories(
                npc_id=npc_id,
                topic=topic,
                limit=limit
            )
        return []
    
    def load_dialogues(self, npc_id: str, player_id: str = None,
                       date: str = None, limit: int = 10):
        """加载对话记录文件"""
        if self.file_memory_store:
            return self.file_memory_store.load_dialogues(
                npc_id=npc_id,
                player_id=player_id,
                date=date,
                limit=limit
            )
        return []
    
    # ==================== 统计和管理 ====================
    
    def get_storage_stats(self, npc_id: str = None) -> Dict[str, Any]:
        """获取存储统计信息"""
        stats = {
            "config": self.config.to_dict(),
            "dialogue_storage": None,
            "file_memory_store": None
        }
        
        if self.dialogue_storage:
            stats["dialogue_storage"] = self.dialogue_storage.get_stats()
        
        if self.file_memory_store:
            stats["file_memory_store"] = self.file_memory_store.get_stats(npc_id)
        
        return stats
    
    def cleanup_old_files(self, days: int = None) -> Dict[str, int]:
        """清理旧文件"""
        days = days or self.config.cleanup_working_memory_days
        result = {"working_memories_deleted": 0}
        
        if self.file_memory_store:
            result["working_memories_deleted"] = \
                self.file_memory_store.cleanup_old_working_memories(days)
        
        return result


# ==================== 便捷函数 ====================

# 全局存储管理器实例
_global_storage_manager: Optional[StorageManager] = None


def get_storage_manager(base_dir: str = None) -> StorageManager:
    """获取全局存储管理器实例"""
    global _global_storage_manager
    
    if _global_storage_manager is None:
        config = StorageConfig(base_dir=base_dir or "./npc_data")
        _global_storage_manager = StorageManager(config)
    
    return _global_storage_manager


def create_storage_manager(base_dir: str = "./npc_data",
                           enable_sqlite: bool = True,
                           enable_excel_export: bool = True,
                           enable_file_memory: bool = True) -> StorageManager:
    """创建新的存储管理器实例"""
    config = StorageConfig(
        base_dir=base_dir,
        enable_sqlite=enable_sqlite,
        enable_excel_export=enable_excel_export,
        enable_file_memory=enable_file_memory
    )
    return StorageManager(config)
