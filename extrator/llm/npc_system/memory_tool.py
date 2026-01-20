"""
MemoryTool - 四层记忆系统
基于 HelloAgents 第8章设计

记忆类型:
- WorkingMemory: 工作记忆 (短期, TTL自动清理)
- EpisodicMemory: 情景记忆 (时间序列事件)
- SemanticMemory: 语义记忆 (知识图谱, 实体关系)
- PerceptualMemory: 感知记忆 (多模态)
"""

import sqlite3
import json
import math
import hashlib
from datetime import datetime, timedelta
from dataclasses import dataclass, field, asdict
from typing import List, Dict, Any, Optional, Literal
from enum import Enum
from pathlib import Path


# ==================== 数据结构 ====================

class MemoryType(str, Enum):
    WORKING = "working"
    EPISODIC = "episodic"
    SEMANTIC = "semantic"
    PERCEPTUAL = "perceptual"


@dataclass
class MemoryConfig:
    """记忆系统配置"""
    db_path: str = "npc_memory.db"
    working_capacity: int = 50          # 工作记忆容量
    working_ttl_minutes: int = 120      # 工作记忆存活时间(分钟)
    episodic_decay_hours: float = 24.0  # 情景记忆衰减周期(小时)
    semantic_importance_threshold: float = 0.5  # 语义记忆重要性阈值
    embedding_dim: int = 384            # 嵌入向量维度(预留)


@dataclass
class MemoryItem:
    """记忆条目"""
    memory_id: str = ""
    content: str = ""
    memory_type: str = "working"
    importance: float = 0.5
    timestamp: str = ""
    metadata: Dict[str, Any] = field(default_factory=dict)
    user_id: str = ""
    session_id: str = ""
    # 检索时填充
    relevance_score: float = 0.0
    recency_score: float = 0.0
    combined_score: float = 0.0
    
    def to_dict(self) -> dict:
        return asdict(self)
    
    @classmethod
    def from_dict(cls, data: dict) -> 'MemoryItem':
        return cls(**{k: v for k, v in data.items() if k in cls.__dataclass_fields__})


# ==================== 工作记忆 ====================

class WorkingMemory:
    """
    工作记忆 - 短期记忆
    
    特点:
    - 纯内存存储
    - 容量限制 (默认50条)
    - TTL自动清理
    - TF-IDF + 关键词混合检索
    """
    
    def __init__(self, capacity: int = 50, ttl_minutes: int = 120):
        self.capacity = capacity
        self.ttl_minutes = ttl_minutes
        self.memories: Dict[str, MemoryItem] = {}
        self._counter = 0
    
    def add(self, content: str, importance: float = 0.5, 
            metadata: Dict = None, user_id: str = "", session_id: str = "") -> str:
        """添加工作记忆"""
        self._cleanup_expired()
        
        # 容量检查，移除最旧的
        while len(self.memories) >= self.capacity:
            oldest_key = min(self.memories.keys(), 
                           key=lambda k: self.memories[k].timestamp)
            del self.memories[oldest_key]
        
        self._counter += 1
        memory_id = f"wm_{datetime.now().strftime('%Y%m%d%H%M%S')}_{self._counter}"
        
        item = MemoryItem(
            memory_id=memory_id,
            content=content,
            memory_type=MemoryType.WORKING.value,
            importance=importance,
            timestamp=datetime.now().isoformat(),
            metadata=metadata or {},
            user_id=user_id,
            session_id=session_id
        )
        
        self.memories[memory_id] = item
        return memory_id
    
    def retrieve(self, query: str, limit: int = 5, user_id: str = "") -> List[MemoryItem]:
        """检索工作记忆 (TF-IDF + 关键词)"""
        self._cleanup_expired()
        
        results = []
        query_words = set(query.lower().split())
        
        for item in self.memories.values():
            if user_id and item.user_id != user_id:
                continue
            
            content_words = set(item.content.lower().split())
            
            # Jaccard相似度
            if query_words:
                intersection = len(query_words & content_words)
                union = len(query_words | content_words)
                relevance = intersection / union if union > 0 else 0
            else:
                relevance = 0
            
            item.relevance_score = relevance
            item.recency_score = self._calculate_recency(item.timestamp)
            # 工作记忆评分: 相关性为主
            item.combined_score = relevance * 0.8 + item.recency_score * 0.2
            
            results.append(item)
        
        results.sort(key=lambda x: x.combined_score, reverse=True)
        return results[:limit]
    
    def get_recent(self, limit: int = 10, user_id: str = "") -> List[MemoryItem]:
        """获取最近的记忆"""
        self._cleanup_expired()
        
        items = [m for m in self.memories.values() 
                 if not user_id or m.user_id == user_id]
        items.sort(key=lambda x: x.timestamp, reverse=True)
        return items[:limit]
    
    def clear(self, user_id: str = "", session_id: str = ""):
        """清除记忆"""
        if not user_id and not session_id:
            self.memories.clear()
        else:
            to_remove = []
            for key, item in self.memories.items():
                if user_id and item.user_id == user_id:
                    to_remove.append(key)
                elif session_id and item.session_id == session_id:
                    to_remove.append(key)
            for key in to_remove:
                del self.memories[key]
    
    def _cleanup_expired(self):
        """清理过期记忆"""
        now = datetime.now()
        cutoff = now - timedelta(minutes=self.ttl_minutes)
        
        to_remove = []
        for key, item in self.memories.items():
            try:
                item_time = datetime.fromisoformat(item.timestamp)
                if item_time < cutoff:
                    to_remove.append(key)
            except:
                pass
        
        for key in to_remove:
            del self.memories[key]
    
    def _calculate_recency(self, timestamp: str) -> float:
        """计算时间近因性"""
        try:
            item_time = datetime.fromisoformat(timestamp)
            age_minutes = (datetime.now() - item_time).total_seconds() / 60
            # 指数衰减
            decay = math.exp(-0.1 * age_minutes / self.ttl_minutes)
            return max(0.1, decay)
        except:
            return 0.5


# ==================== 情景记忆 ====================

class EpisodicMemory:
    """
    情景记忆 - 时间序列事件
    
    特点:
    - SQLite持久化存储
    - 时间序列检索
    - 会话级别组织
    - 评分: (相似度×0.6 + 时间近因性×0.2) × (0.8 + 重要性×0.4)
    """
    
    def __init__(self, db_path: str = "npc_memory.db"):
        self.db_path = db_path
        self._init_database()
    
    def _init_database(self):
        """初始化数据库"""
        conn = sqlite3.connect(self.db_path)
        cursor = conn.cursor()
        
        cursor.execute('''
            CREATE TABLE IF NOT EXISTS episodic_memory (
                memory_id TEXT PRIMARY KEY,
                content TEXT NOT NULL,
                importance REAL DEFAULT 0.5,
                timestamp TEXT NOT NULL,
                metadata TEXT,
                user_id TEXT,
                session_id TEXT,
                event_type TEXT,
                created_at TEXT DEFAULT CURRENT_TIMESTAMP
            )
        ''')
        
        cursor.execute('CREATE INDEX IF NOT EXISTS idx_episodic_user ON episodic_memory(user_id)')
        cursor.execute('CREATE INDEX IF NOT EXISTS idx_episodic_session ON episodic_memory(session_id)')
        cursor.execute('CREATE INDEX IF NOT EXISTS idx_episodic_timestamp ON episodic_memory(timestamp)')
        
        conn.commit()
        conn.close()
    
    def add(self, content: str, importance: float = 0.5,
            metadata: Dict = None, user_id: str = "", 
            session_id: str = "", event_type: str = "dialogue") -> str:
        """添加情景记忆"""
        memory_id = f"em_{datetime.now().strftime('%Y%m%d%H%M%S')}_{hashlib.md5(content.encode()).hexdigest()[:8]}"
        timestamp = datetime.now().isoformat()
        
        conn = sqlite3.connect(self.db_path)
        cursor = conn.cursor()
        
        cursor.execute('''
            INSERT INTO episodic_memory 
            (memory_id, content, importance, timestamp, metadata, user_id, session_id, event_type)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?)
        ''', (
            memory_id,
            content,
            importance,
            timestamp,
            json.dumps(metadata or {}),
            user_id,
            session_id,
            event_type
        ))
        
        conn.commit()
        conn.close()
        
        return memory_id
    
    def retrieve(self, query: str, limit: int = 5, 
                 user_id: str = "", session_id: str = "",
                 decay_hours: float = 24.0) -> List[MemoryItem]:
        """检索情景记忆"""
        conn = sqlite3.connect(self.db_path)
        cursor = conn.cursor()
        
        # 构建查询条件
        conditions = []
        params = []
        
        if user_id:
            conditions.append("user_id = ?")
            params.append(user_id)
        if session_id:
            conditions.append("session_id = ?")
            params.append(session_id)
        
        where_clause = " AND ".join(conditions) if conditions else "1=1"
        
        cursor.execute(f'''
            SELECT memory_id, content, importance, timestamp, metadata, 
                   user_id, session_id, event_type
            FROM episodic_memory
            WHERE {where_clause}
            ORDER BY timestamp DESC
            LIMIT 100
        ''', params)
        
        rows = cursor.fetchall()
        conn.close()
        
        # 计算评分
        query_words = set(query.lower().split())
        results = []
        
        for row in rows:
            memory_id, content, importance, timestamp, metadata_str, uid, sid, event_type = row
            
            content_words = set(content.lower().split())
            
            # 相似度
            if query_words:
                intersection = len(query_words & content_words)
                union = len(query_words | content_words)
                relevance = intersection / union if union > 0 else 0
            else:
                relevance = 0
            
            # 时间近因性
            recency = self._calculate_recency(timestamp, decay_hours)
            
            # 情景记忆评分公式
            # (相似度×0.6 + 时间近因性×0.2) × (0.8 + 重要性×0.4)
            base_relevance = relevance * 0.6 + recency * 0.2
            importance_weight = 0.8 + importance * 0.4
            combined_score = base_relevance * importance_weight
            
            item = MemoryItem(
                memory_id=memory_id,
                content=content,
                memory_type=MemoryType.EPISODIC.value,
                importance=importance,
                timestamp=timestamp,
                metadata=json.loads(metadata_str) if metadata_str else {},
                user_id=uid or "",
                session_id=sid or "",
                relevance_score=relevance,
                recency_score=recency,
                combined_score=combined_score
            )
            item.metadata["event_type"] = event_type
            results.append(item)
        
        results.sort(key=lambda x: x.combined_score, reverse=True)
        return results[:limit]
    
    def get_session_history(self, session_id: str, limit: int = 20) -> List[MemoryItem]:
        """获取会话历史"""
        conn = sqlite3.connect(self.db_path)
        cursor = conn.cursor()
        
        cursor.execute('''
            SELECT memory_id, content, importance, timestamp, metadata,
                   user_id, session_id, event_type
            FROM episodic_memory
            WHERE session_id = ?
            ORDER BY timestamp DESC
            LIMIT ?
        ''', (session_id, limit))
        
        rows = cursor.fetchall()
        conn.close()
        
        results = []
        for row in rows:
            memory_id, content, importance, timestamp, metadata_str, uid, sid, event_type = row
            item = MemoryItem(
                memory_id=memory_id,
                content=content,
                memory_type=MemoryType.EPISODIC.value,
                importance=importance,
                timestamp=timestamp,
                metadata=json.loads(metadata_str) if metadata_str else {},
                user_id=uid or "",
                session_id=sid or ""
            )
            results.append(item)
        
        return results
    
    def forget(self, strategy: str = "time", threshold: float = 0.3,
               user_id: str = "") -> int:
        """选择性遗忘"""
        conn = sqlite3.connect(self.db_path)
        cursor = conn.cursor()
        
        deleted = 0
        
        if strategy == "importance":
            # 按重要性遗忘
            conditions = ["importance < ?"]
            params = [threshold]
            if user_id:
                conditions.append("user_id = ?")
                params.append(user_id)
            
            cursor.execute(f'''
                DELETE FROM episodic_memory
                WHERE {" AND ".join(conditions)}
            ''', params)
            deleted = cursor.rowcount
            
        elif strategy == "time":
            # 按时间遗忘 (超过threshold天)
            cutoff = (datetime.now() - timedelta(days=threshold)).isoformat()
            conditions = ["timestamp < ?"]
            params = [cutoff]
            if user_id:
                conditions.append("user_id = ?")
                params.append(user_id)
            
            cursor.execute(f'''
                DELETE FROM episodic_memory
                WHERE {" AND ".join(conditions)}
            ''', params)
            deleted = cursor.rowcount
        
        conn.commit()
        conn.close()
        
        return deleted
    
    def _calculate_recency(self, timestamp: str, decay_hours: float) -> float:
        """计算时间近因性"""
        try:
            item_time = datetime.fromisoformat(timestamp)
            age_hours = (datetime.now() - item_time).total_seconds() / 3600
            decay = math.exp(-0.1 * age_hours / decay_hours)
            return max(0.1, decay)
        except:
            return 0.5


# ==================== 语义记忆 ====================

class SemanticMemory:
    """
    语义记忆 - 知识和概念
    
    特点:
    - 概念和实体存储
    - 关系抽取 (简化版，不使用Neo4j)
    - 评分: (向量相似度×0.7 + 图相似度×0.3) × (0.8 + 重要性×0.4)
    """
    
    def __init__(self, db_path: str = "npc_memory.db"):
        self.db_path = db_path
        self._init_database()
    
    def _init_database(self):
        """初始化数据库"""
        conn = sqlite3.connect(self.db_path)
        cursor = conn.cursor()
        
        # 概念表
        cursor.execute('''
            CREATE TABLE IF NOT EXISTS semantic_concepts (
                concept_id TEXT PRIMARY KEY,
                concept_name TEXT NOT NULL,
                description TEXT,
                importance REAL DEFAULT 0.5,
                metadata TEXT,
                user_id TEXT,
                created_at TEXT DEFAULT CURRENT_TIMESTAMP
            )
        ''')
        
        # 关系表
        cursor.execute('''
            CREATE TABLE IF NOT EXISTS semantic_relations (
                relation_id TEXT PRIMARY KEY,
                source_concept TEXT,
                target_concept TEXT,
                relation_type TEXT,
                strength REAL DEFAULT 0.5,
                user_id TEXT,
                FOREIGN KEY (source_concept) REFERENCES semantic_concepts(concept_id),
                FOREIGN KEY (target_concept) REFERENCES semantic_concepts(concept_id)
            )
        ''')
        
        # 知识条目表
        cursor.execute('''
            CREATE TABLE IF NOT EXISTS semantic_knowledge (
                knowledge_id TEXT PRIMARY KEY,
                content TEXT NOT NULL,
                concepts TEXT,  -- JSON array of concept_ids
                importance REAL DEFAULT 0.5,
                timestamp TEXT,
                metadata TEXT,
                user_id TEXT
            )
        ''')
        
        cursor.execute('CREATE INDEX IF NOT EXISTS idx_semantic_user ON semantic_knowledge(user_id)')
        
        conn.commit()
        conn.close()
    
    def add(self, content: str, importance: float = 0.5,
            metadata: Dict = None, user_id: str = "",
            concepts: List[str] = None) -> str:
        """添加语义知识"""
        knowledge_id = f"sm_{datetime.now().strftime('%Y%m%d%H%M%S')}_{hashlib.md5(content.encode()).hexdigest()[:8]}"
        
        conn = sqlite3.connect(self.db_path)
        cursor = conn.cursor()
        
        cursor.execute('''
            INSERT INTO semantic_knowledge
            (knowledge_id, content, concepts, importance, timestamp, metadata, user_id)
            VALUES (?, ?, ?, ?, ?, ?, ?)
        ''', (
            knowledge_id,
            content,
            json.dumps(concepts or []),
            importance,
            datetime.now().isoformat(),
            json.dumps(metadata or {}),
            user_id
        ))
        
        conn.commit()
        conn.close()
        
        return knowledge_id
    
    def add_concept(self, name: str, description: str = "",
                    importance: float = 0.5, user_id: str = "") -> str:
        """添加概念"""
        concept_id = f"concept_{hashlib.md5(name.encode()).hexdigest()[:8]}"
        
        conn = sqlite3.connect(self.db_path)
        cursor = conn.cursor()
        
        cursor.execute('''
            INSERT OR REPLACE INTO semantic_concepts
            (concept_id, concept_name, description, importance, user_id)
            VALUES (?, ?, ?, ?, ?)
        ''', (concept_id, name, description, importance, user_id))
        
        conn.commit()
        conn.close()
        
        return concept_id
    
    def add_relation(self, source: str, target: str, 
                     relation_type: str, strength: float = 0.5,
                     user_id: str = "") -> str:
        """添加关系"""
        relation_id = f"rel_{hashlib.md5(f'{source}_{target}_{relation_type}'.encode()).hexdigest()[:8]}"
        
        conn = sqlite3.connect(self.db_path)
        cursor = conn.cursor()
        
        cursor.execute('''
            INSERT OR REPLACE INTO semantic_relations
            (relation_id, source_concept, target_concept, relation_type, strength, user_id)
            VALUES (?, ?, ?, ?, ?, ?)
        ''', (relation_id, source, target, relation_type, strength, user_id))
        
        conn.commit()
        conn.close()
        
        return relation_id
    
    def retrieve(self, query: str, limit: int = 5, user_id: str = "") -> List[MemoryItem]:
        """检索语义知识"""
        conn = sqlite3.connect(self.db_path)
        cursor = conn.cursor()
        
        conditions = []
        params = []
        
        if user_id:
            conditions.append("user_id = ?")
            params.append(user_id)
        
        where_clause = " AND ".join(conditions) if conditions else "1=1"
        
        cursor.execute(f'''
            SELECT knowledge_id, content, concepts, importance, timestamp, metadata, user_id
            FROM semantic_knowledge
            WHERE {where_clause}
        ''', params)
        
        rows = cursor.fetchall()
        conn.close()
        
        query_words = set(query.lower().split())
        results = []
        
        for row in rows:
            knowledge_id, content, concepts_str, importance, timestamp, metadata_str, uid = row
            
            content_words = set(content.lower().split())
            
            # 向量相似度 (简化为关键词)
            if query_words:
                intersection = len(query_words & content_words)
                union = len(query_words | content_words)
                vector_score = intersection / union if union > 0 else 0
            else:
                vector_score = 0
            
            # 图相似度 (简化: 概念匹配)
            concepts = json.loads(concepts_str) if concepts_str else []
            graph_score = 0.3 if any(c.lower() in query.lower() for c in concepts) else 0
            
            # 语义记忆评分公式
            # (向量相似度×0.7 + 图相似度×0.3) × (0.8 + 重要性×0.4)
            base_relevance = vector_score * 0.7 + graph_score * 0.3
            importance_weight = 0.8 + importance * 0.4
            combined_score = base_relevance * importance_weight
            
            item = MemoryItem(
                memory_id=knowledge_id,
                content=content,
                memory_type=MemoryType.SEMANTIC.value,
                importance=importance,
                timestamp=timestamp or "",
                metadata=json.loads(metadata_str) if metadata_str else {},
                user_id=uid or "",
                relevance_score=vector_score,
                combined_score=combined_score
            )
            item.metadata["concepts"] = concepts
            results.append(item)
        
        results.sort(key=lambda x: x.combined_score, reverse=True)
        return results[:limit]


# ==================== MemoryTool 统一接口 ====================

class MemoryTool:
    """
    记忆工具 - 统一接口
    
    操作类型:
    - add: 添加记忆
    - search: 检索记忆
    - forget: 选择性遗忘
    - consolidate: 记忆整合 (短期→长期)
    - summary: 记忆摘要统计
    - clear: 清除记忆
    """
    
    def __init__(self, config: MemoryConfig = None, user_id: str = ""):
        self.config = config or MemoryConfig()
        self.default_user_id = user_id
        
        # 初始化各层记忆
        self.working_memory = WorkingMemory(
            capacity=self.config.working_capacity,
            ttl_minutes=self.config.working_ttl_minutes
        )
        self.episodic_memory = EpisodicMemory(db_path=self.config.db_path)
        self.semantic_memory = SemanticMemory(db_path=self.config.db_path)
    
    def execute(self, action: str, **kwargs) -> Any:
        """执行记忆操作"""
        actions = {
            "add": self._add,
            "search": self._search,
            "forget": self._forget,
            "consolidate": self._consolidate,
            "summary": self._summary,
            "clear": self._clear,
            "get_recent": self._get_recent,
        }
        
        if action not in actions:
            raise ValueError(f"未知操作: {action}, 可用操作: {list(actions.keys())}")
        
        return actions[action](**kwargs)
    
    def _add(self, content: str, memory_type: str = "working",
             importance: float = 0.5, metadata: Dict = None,
             user_id: str = "", session_id: str = "", **kwargs) -> str:
        """添加记忆"""
        uid = user_id or self.default_user_id
        
        if memory_type == "working":
            return self.working_memory.add(
                content, importance, metadata, uid, session_id
            )
        elif memory_type == "episodic":
            return self.episodic_memory.add(
                content, importance, metadata, uid, session_id,
                event_type=kwargs.get("event_type", "dialogue")
            )
        elif memory_type == "semantic":
            return self.semantic_memory.add(
                content, importance, metadata, uid,
                concepts=kwargs.get("concepts", [])
            )
        else:
            raise ValueError(f"未知记忆类型: {memory_type}")
    
    def _search(self, query: str, limit: int = 5,
                memory_type: str = None, user_id: str = "",
                session_id: str = "", **kwargs) -> List[MemoryItem]:
        """检索记忆"""
        uid = user_id or self.default_user_id
        results = []
        
        if memory_type is None or memory_type == "all":
            # 从所有记忆层检索
            results.extend(self.working_memory.retrieve(query, limit, uid))
            results.extend(self.episodic_memory.retrieve(query, limit, uid, session_id))
            results.extend(self.semantic_memory.retrieve(query, limit, uid))
            
            # 合并排序
            results.sort(key=lambda x: x.combined_score, reverse=True)
            return results[:limit]
        
        elif memory_type == "working":
            return self.working_memory.retrieve(query, limit, uid)
        elif memory_type == "episodic":
            return self.episodic_memory.retrieve(query, limit, uid, session_id)
        elif memory_type == "semantic":
            return self.semantic_memory.retrieve(query, limit, uid)
        else:
            raise ValueError(f"未知记忆类型: {memory_type}")
    
    def _forget(self, strategy: str = "importance", threshold: float = 0.3,
                memory_type: str = "episodic", user_id: str = "", **kwargs) -> Dict:
        """选择性遗忘"""
        uid = user_id or self.default_user_id
        deleted = 0
        
        if memory_type == "episodic":
            deleted = self.episodic_memory.forget(strategy, threshold, uid)
        elif memory_type == "working":
            self.working_memory.clear(uid)
            deleted = -1  # 工作记忆全清
        
        return {"deleted": deleted, "strategy": strategy, "threshold": threshold}
    
    def _consolidate(self, threshold: float = 0.5, user_id: str = "",
                     session_id: str = "", **kwargs) -> Dict:
        """记忆整合: 工作记忆 → 情景记忆"""
        uid = user_id or self.default_user_id
        consolidated = 0
        
        # 获取工作记忆中重要的条目
        working_items = self.working_memory.get_recent(limit=50, user_id=uid)
        
        for item in working_items:
            if item.importance >= threshold:
                # 转移到情景记忆
                self.episodic_memory.add(
                    content=item.content,
                    importance=item.importance,
                    metadata=item.metadata,
                    user_id=uid,
                    session_id=session_id,
                    event_type="consolidated"
                )
                consolidated += 1
        
        # 清除已整合的工作记忆
        if consolidated > 0:
            self.working_memory.clear(user_id=uid)
        
        return {"consolidated": consolidated, "threshold": threshold}
    
    def _summary(self, user_id: str = "", **kwargs) -> Dict:
        """记忆摘要统计"""
        uid = user_id or self.default_user_id
        
        working_count = len([m for m in self.working_memory.memories.values()
                            if not uid or m.user_id == uid])
        
        conn = sqlite3.connect(self.config.db_path)
        cursor = conn.cursor()
        
        if uid:
            cursor.execute('SELECT COUNT(*) FROM episodic_memory WHERE user_id = ?', (uid,))
        else:
            cursor.execute('SELECT COUNT(*) FROM episodic_memory')
        episodic_count = cursor.fetchone()[0]
        
        if uid:
            cursor.execute('SELECT COUNT(*) FROM semantic_knowledge WHERE user_id = ?', (uid,))
        else:
            cursor.execute('SELECT COUNT(*) FROM semantic_knowledge')
        semantic_count = cursor.fetchone()[0]
        
        conn.close()
        
        return {
            "working_memory": working_count,
            "episodic_memory": episodic_count,
            "semantic_memory": semantic_count,
            "total": working_count + episodic_count + semantic_count
        }
    
    def _clear(self, memory_type: str = "all", user_id: str = "",
               session_id: str = "", **kwargs) -> Dict:
        """清除记忆"""
        uid = user_id or self.default_user_id
        cleared = {}
        
        if memory_type in ["all", "working"]:
            self.working_memory.clear(uid, session_id)
            cleared["working"] = True
        
        if memory_type in ["all", "episodic"]:
            conn = sqlite3.connect(self.config.db_path)
            cursor = conn.cursor()
            
            if uid:
                cursor.execute('DELETE FROM episodic_memory WHERE user_id = ?', (uid,))
            else:
                cursor.execute('DELETE FROM episodic_memory')
            
            cleared["episodic"] = cursor.rowcount
            conn.commit()
            conn.close()
        
        return cleared
    
    def _get_recent(self, limit: int = 10, memory_type: str = "working",
                    user_id: str = "", session_id: str = "", **kwargs) -> List[MemoryItem]:
        """获取最近的记忆"""
        uid = user_id or self.default_user_id
        
        if memory_type == "working":
            return self.working_memory.get_recent(limit, uid)
        elif memory_type == "episodic":
            return self.episodic_memory.get_session_history(session_id, limit)
        else:
            return []


# ==================== 便捷函数 ====================

def create_memory_tool(db_path: str = "npc_memory.db", user_id: str = "") -> MemoryTool:
    """创建记忆工具实例"""
    config = MemoryConfig(db_path=db_path)
    return MemoryTool(config, user_id)
