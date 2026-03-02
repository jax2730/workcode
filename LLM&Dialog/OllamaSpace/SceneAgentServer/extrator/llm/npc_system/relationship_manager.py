"""
RelationshipManager - 好感度系统
基于 HelloAgents 第15章设计

好感度等级:
- 陌生 (0-20): 礼貌但疏远
- 熟悉 (21-40): 正常交流
- 友好 (41-60): 当作朋友
- 亲密 (61-80): 非常信任
- 挚友 (81-100): 无话不谈
"""

import sqlite3
import json
import re
from datetime import datetime
from dataclasses import dataclass
from typing import Dict, Any, Optional, List, Tuple
from enum import Enum


class AffinityLevel(str, Enum):
    """好感度等级"""
    STRANGER = "陌生"      # 0-20
    FAMILIAR = "熟悉"      # 21-40
    FRIENDLY = "友好"      # 41-60
    CLOSE = "亲密"         # 61-80
    BEST_FRIEND = "挚友"   # 81-100
    
    @classmethod
    def from_score(cls, score: int) -> 'AffinityLevel':
        if score <= 20:
            return cls.STRANGER
        elif score <= 40:
            return cls.FAMILIAR
        elif score <= 60:
            return cls.FRIENDLY
        elif score <= 80:
            return cls.CLOSE
        else:
            return cls.BEST_FRIEND
    
    def get_prompt_hint(self) -> str:
        """获取对应等级的Prompt提示"""
        hints = {
            AffinityLevel.STRANGER: "你刚认识这位玩家,保持礼貌但不要过于热情。回复简短专业。",
            AffinityLevel.FAMILIAR: "你已经认识这位玩家,可以进行正常的交流。回复自然友好。",
            AffinityLevel.FRIENDLY: "你把这位玩家当作朋友,愿意分享更多信息。回复详细热情。",
            AffinityLevel.CLOSE: "你非常信任这位玩家,可以分享私人话题。回复充满关心。",
            AffinityLevel.BEST_FRIEND: "你把这位玩家当作最好的朋友,无话不谈。回复亲切真诚。"
        }
        return hints.get(self, hints[AffinityLevel.STRANGER])


@dataclass
class AffinityInfo:
    """好感度信息"""
    npc_id: str
    player_id: str
    score: int = 0
    level: AffinityLevel = AffinityLevel.STRANGER
    interaction_count: int = 0
    last_interaction: str = ""
    history: List[Dict] = None
    
    def __post_init__(self):
        if self.history is None:
            self.history = []
        self.level = AffinityLevel.from_score(self.score)
    
    def to_dict(self) -> dict:
        return {
            "npc_id": self.npc_id,
            "player_id": self.player_id,
            "score": self.score,
            "level": self.level.value,
            "interaction_count": self.interaction_count,
            "last_interaction": self.last_interaction,
        }


class RelationshipManager:
    """
    好感度管理器
    
    功能:
    - 跟踪NPC与玩家的好感度
    - 基于对话内容分析情感
    - 动态调整好感度
    - 提供好感度相关的Prompt提示
    """
    
    def __init__(self, db_path: str = "npc_relationship.db", llm=None):
        self.db_path = db_path
        self.llm = llm  # 可选的LLM，用于高级情感分析
        self._init_database()
    
    def _init_database(self):
        """初始化数据库"""
        conn = sqlite3.connect(self.db_path)
        cursor = conn.cursor()
        
        cursor.execute('''
            CREATE TABLE IF NOT EXISTS affinity (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                npc_id TEXT NOT NULL,
                player_id TEXT NOT NULL,
                score INTEGER DEFAULT 0,
                interaction_count INTEGER DEFAULT 0,
                last_interaction TEXT,
                created_at TEXT DEFAULT CURRENT_TIMESTAMP,
                updated_at TEXT DEFAULT CURRENT_TIMESTAMP,
                UNIQUE(npc_id, player_id)
            )
        ''')
        
        cursor.execute('''
            CREATE TABLE IF NOT EXISTS interaction_history (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                npc_id TEXT NOT NULL,
                player_id TEXT NOT NULL,
                player_message TEXT,
                npc_reply TEXT,
                sentiment TEXT,
                score_change INTEGER,
                timestamp TEXT DEFAULT CURRENT_TIMESTAMP
            )
        ''')
        
        cursor.execute('CREATE INDEX IF NOT EXISTS idx_affinity_npc_player ON affinity(npc_id, player_id)')
        
        conn.commit()
        conn.close()
    
    def get_affinity(self, npc_id: str, player_id: str) -> AffinityInfo:
        """获取好感度信息"""
        conn = sqlite3.connect(self.db_path)
        cursor = conn.cursor()
        
        cursor.execute('''
            SELECT score, interaction_count, last_interaction
            FROM affinity
            WHERE npc_id = ? AND player_id = ?
        ''', (npc_id, player_id))
        
        row = cursor.fetchone()
        conn.close()
        
        if row:
            score, interaction_count, last_interaction = row
            return AffinityInfo(
                npc_id=npc_id,
                player_id=player_id,
                score=score,
                interaction_count=interaction_count,
                last_interaction=last_interaction or ""
            )
        else:
            return AffinityInfo(npc_id=npc_id, player_id=player_id)
    
    def update_affinity(self, npc_id: str, player_id: str,
                        player_message: str, npc_reply: str,
                        use_llm: bool = False) -> AffinityInfo:
        """
        更新好感度
        
        Args:
            npc_id: NPC ID
            player_id: 玩家ID
            player_message: 玩家消息
            npc_reply: NPC回复
            use_llm: 是否使用LLM进行高级情感分析
            
        Returns:
            AffinityInfo: 更新后的好感度信息
        """
        # 1. 分析情感
        if use_llm and self.llm:
            sentiment, score_change = self._analyze_sentiment_llm(player_message, npc_reply)
        else:
            sentiment, score_change = self._analyze_sentiment_rule(player_message)
        
        # 2. 获取当前好感度
        current = self.get_affinity(npc_id, player_id)
        
        # 3. 计算新分数 (限制在0-100)
        new_score = max(0, min(100, current.score + score_change))
        
        # 4. 更新数据库
        conn = sqlite3.connect(self.db_path)
        cursor = conn.cursor()
        
        now = datetime.now().isoformat()
        
        cursor.execute('''
            INSERT INTO affinity (npc_id, player_id, score, interaction_count, last_interaction, updated_at)
            VALUES (?, ?, ?, 1, ?, ?)
            ON CONFLICT(npc_id, player_id) DO UPDATE SET
                score = ?,
                interaction_count = interaction_count + 1,
                last_interaction = ?,
                updated_at = ?
        ''', (npc_id, player_id, new_score, now, now,
              new_score, now, now))
        
        # 5. 记录交互历史
        cursor.execute('''
            INSERT INTO interaction_history
            (npc_id, player_id, player_message, npc_reply, sentiment, score_change)
            VALUES (?, ?, ?, ?, ?, ?)
        ''', (npc_id, player_id, player_message, npc_reply, sentiment, score_change))
        
        conn.commit()
        conn.close()
        
        # 6. 返回更新后的信息
        return AffinityInfo(
            npc_id=npc_id,
            player_id=player_id,
            score=new_score,
            interaction_count=current.interaction_count + 1,
            last_interaction=now
        )
    
    def _analyze_sentiment_rule(self, message: str) -> Tuple[str, int]:
        """
        基于规则的情感分析
        
        Returns:
            (sentiment, score_change)
        """
        message_lower = message.lower()
        
        # 友好词汇
        friendly_words = [
            "谢谢", "感谢", "太好了", "厉害", "棒", "赞", "喜欢",
            "帮助", "有趣", "开心", "高兴", "期待", "欣赏", "佩服",
            "thank", "thanks", "great", "awesome", "nice", "good", "love"
        ]
        
        # 不友好词汇
        unfriendly_words = [
            "讨厌", "烦", "滚", "闭嘴", "无聊", "废话", "蠢",
            "hate", "annoying", "shut up", "stupid", "boring", "useless"
        ]
        
        # 问候词汇 (轻微正面)
        greeting_words = [
            "你好", "嗨", "早上好", "晚上好", "再见", "拜拜",
            "hello", "hi", "hey", "morning", "goodbye", "bye"
        ]
        
        # 检测情感
        for word in friendly_words:
            if word in message_lower:
                return "positive", 5
        
        for word in unfriendly_words:
            if word in message_lower:
                return "negative", -3
        
        for word in greeting_words:
            if word in message_lower:
                return "greeting", 3
        
        # 默认中立
        return "neutral", 2
    
    def _analyze_sentiment_llm(self, player_message: str, npc_reply: str) -> Tuple[str, int]:
        """
        基于LLM的情感分析
        """
        if not self.llm:
            return self._analyze_sentiment_rule(player_message)
        
        prompt = f"""分析以下对话中玩家的态度:
玩家: {player_message}
NPC: {npc_reply}

请判断玩家的态度并返回一个数字:
- 5: 非常友好 (礼貌、热情、表示感谢或赞同)
- 3: 友好问候 (简单的打招呼)
- 2: 中立 (普通的询问或陈述)
- -3: 不友好 (粗鲁、冷漠、批评或否定)

只返回数字,不要其他内容。"""
        
        try:
            response = self.llm.invoke([{"role": "user", "content": prompt}])
            if hasattr(response, 'content'):
                response = response.content
            
            # 提取数字
            match = re.search(r'-?\d+', str(response))
            if match:
                score = int(match.group())
                score = max(-3, min(5, score))
                
                if score >= 4:
                    return "positive", score
                elif score == 3:
                    return "greeting", score
                elif score <= -1:
                    return "negative", score
                else:
                    return "neutral", score
        except Exception as e:
            print(f"[RelationshipManager] LLM分析失败: {e}")
        
        return self._analyze_sentiment_rule(player_message)
    
    def get_prompt_modifier(self, npc_id: str, player_id: str) -> str:
        """
        获取好感度相关的Prompt修饰语
        
        可直接添加到NPC的系统提示词中
        """
        affinity = self.get_affinity(npc_id, player_id)
        
        return f"""
当前与玩家的关系: {affinity.level.value} (好感度: {affinity.score}/100)
互动次数: {affinity.interaction_count}
{affinity.level.get_prompt_hint()}
"""
    
    def get_interaction_history(self, npc_id: str, player_id: str,
                                 limit: int = 10) -> List[Dict]:
        """获取交互历史"""
        conn = sqlite3.connect(self.db_path)
        cursor = conn.cursor()
        
        cursor.execute('''
            SELECT player_message, npc_reply, sentiment, score_change, timestamp
            FROM interaction_history
            WHERE npc_id = ? AND player_id = ?
            ORDER BY timestamp DESC
            LIMIT ?
        ''', (npc_id, player_id, limit))
        
        rows = cursor.fetchall()
        conn.close()
        
        return [
            {
                "player_message": row[0],
                "npc_reply": row[1],
                "sentiment": row[2],
                "score_change": row[3],
                "timestamp": row[4]
            }
            for row in rows
        ]
    
    def get_all_relationships(self, npc_id: str = None,
                               player_id: str = None) -> List[AffinityInfo]:
        """获取所有关系"""
        conn = sqlite3.connect(self.db_path)
        cursor = conn.cursor()
        
        conditions = []
        params = []
        
        if npc_id:
            conditions.append("npc_id = ?")
            params.append(npc_id)
        if player_id:
            conditions.append("player_id = ?")
            params.append(player_id)
        
        where_clause = " AND ".join(conditions) if conditions else "1=1"
        
        cursor.execute(f'''
            SELECT npc_id, player_id, score, interaction_count, last_interaction
            FROM affinity
            WHERE {where_clause}
            ORDER BY score DESC
        ''', params)
        
        rows = cursor.fetchall()
        conn.close()
        
        return [
            AffinityInfo(
                npc_id=row[0],
                player_id=row[1],
                score=row[2],
                interaction_count=row[3],
                last_interaction=row[4] or ""
            )
            for row in rows
        ]
    
    def reset_affinity(self, npc_id: str, player_id: str) -> bool:
        """重置好感度"""
        conn = sqlite3.connect(self.db_path)
        cursor = conn.cursor()
        
        cursor.execute('''
            DELETE FROM affinity
            WHERE npc_id = ? AND player_id = ?
        ''', (npc_id, player_id))
        
        cursor.execute('''
            DELETE FROM interaction_history
            WHERE npc_id = ? AND player_id = ?
        ''', (npc_id, player_id))
        
        conn.commit()
        conn.close()
        
        return True


# ==================== 便捷函数 ====================

def create_relationship_manager(db_path: str = "npc_relationship.db",
                                 llm=None) -> RelationshipManager:
    """创建关系管理器"""
    return RelationshipManager(db_path, llm)
