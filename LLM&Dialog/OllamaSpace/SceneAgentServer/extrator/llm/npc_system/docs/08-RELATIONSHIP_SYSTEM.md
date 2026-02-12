# 08 - 好感度系统详解 (RelationshipManager)

> **面向对象**: 系统开发者、维护人员  
> **前置知识**: Python、SQLite、枚举类型  
> **相关模块**: NPCAgent, DialogueStorage

## 📋 模块概述

### 职责定义

RelationshipManager 是 NPC 系统的**关系管理核心**，负责追踪和管理 NPC 与玩家之间的关系状态。

**核心职责**:
1. **好感度计算**: 根据互动内容动态计算好感度变化
2. **等级管理**: 管理5个好感度等级的转换
3. **互动统计**: 记录互动次数、时间、类型
4. **秘密解锁**: 根据好感度等级解锁NPC秘密
5. **关系持久化**: 将关系数据保存到数据库

### 设计理念

**动态关系系统**:
```
互动内容 → 情感分析 → 分数变化 → 等级更新 → 触发事件
```

**为什么需要好感度系统?**

```python
# 没有好感度系统
NPC: "你好。" (每次都一样)

# 有好感度系统
第1次: NPC: "你好，陌生人。" (陌生, 0分)
第5次: NPC: "哦，又是你。" (熟悉, 35分)
第20次: NPC: "嘿，朋友！" (友好, 55分)
第50次: NPC: "老朋友！我正想找你呢。" (亲密, 75分)
```

---

## 🏗️ 架构设计

### 类图

```python
┌─────────────────────────────────────────────────────────┐
│              RelationshipManager                         │
├─────────────────────────────────────────────────────────┤
│ - db_path: str                                          │
│ - affinity_cache: Dict[Tuple[str,str], AffinityInfo]   │
├─────────────────────────────────────────────────────────┤
│ + get_affinity(npc_id, player_id) -> AffinityInfo      │
│ + update_affinity(npc_id, player_id, delta) -> ...     │
│ + calculate_affinity_change(message, reply) -> int     │
│ + get_relationship_history(npc_id, player_id) -> List  │
│ + reset_affinity(npc_id, player_id) -> bool            │
├─────────────────────────────────────────────────────────┤
│ - _init_database() -> None                             │
│ - _get_level_from_score(score) -> AffinityLevel        │
│ - _analyze_sentiment(text) -> float                    │
│ - _save_to_db(affinity_info) -> None                   │
│ - _load_from_db(npc_id, player_id) -> AffinityInfo     │
└─────────────────────────────────────────────────────────┘
```

### 数据结构

#### AffinityLevel (好感度等级)

```python
from enum import Enum

class AffinityLevel(Enum):
    """好感度等级"""
    STRANGER = "陌生"      # 0-20分
    FAMILIAR = "熟悉"      # 21-40分
    FRIENDLY = "友好"      # 41-60分
    CLOSE = "亲密"         # 61-80分
    BEST_FRIEND = "挚友"   # 81-100分
    
    @classmethod
    def from_score(cls, score: int) -> 'AffinityLevel':
        """根据分数获取等级"""
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
    
    def get_score_range(self) -> Tuple[int, int]:
        """获取等级对应的分数范围"""
        ranges = {
            self.STRANGER: (0, 20),
            self.FAMILIAR: (21, 40),
            self.FRIENDLY: (41, 60),
            self.CLOSE: (61, 80),
            self.BEST_FRIEND: (81, 100)
        }
        return ranges[self]
```

#### AffinityInfo (好感度信息)

```python
from dataclasses import dataclass, field
from datetime import datetime
from typing import List, Dict, Any

@dataclass
class AffinityInfo:
    """好感度信息"""
    npc_id: str                         # NPC ID
    player_id: str                      # 玩家ID
    score: int = 0                      # 好感度分数 (0-100)
    level: AffinityLevel = AffinityLevel.STRANGER  # 好感度等级
    interaction_count: int = 0          # 互动次数
    last_interaction: datetime = field(default_factory=datetime.now)  # 最后互动时间
    first_met: datetime = field(default_factory=datetime.now)         # 初次见面时间
    positive_count: int = 0             # 正面互动次数
    negative_count: int = 0             # 负面互动次数
    gifts_given: int = 0                # 赠送礼物次数
    quests_completed: int = 0           # 完成任务次数
    metadata: Dict[str, Any] = field(default_factory=dict)  # 额外元数据
    
    def __post_init__(self):
        """初始化后处理"""
        # 确保分数在0-100范围内
        self.score = max(0, min(100, self.score))
        
        # 根据分数更新等级
        self.level = AffinityLevel.from_score(self.score)
    
    def to_dict(self) -> dict:
        """转换为字典"""
        return {
            "npc_id": self.npc_id,
            "player_id": self.player_id,
            "score": self.score,
            "level": self.level.value,
            "interaction_count": self.interaction_count,
            "last_interaction": self.last_interaction.isoformat(),
            "first_met": self.first_met.isoformat(),
            "positive_count": self.positive_count,
            "negative_count": self.negative_count,
            "gifts_given": self.gifts_given,
            "quests_completed": self.quests_completed,
            "metadata": self.metadata
        }
    
    @classmethod
    def from_dict(cls, data: dict) -> 'AffinityInfo':
        """从字典创建"""
        return cls(
            npc_id=data["npc_id"],
            player_id=data["player_id"],
            score=data["score"],
            level=AffinityLevel(data["level"]),
            interaction_count=data["interaction_count"],
            last_interaction=datetime.fromisoformat(data["last_interaction"]),
            first_met=datetime.fromisoformat(data["first_met"]),
            positive_count=data.get("positive_count", 0),
            negative_count=data.get("negative_count", 0),
            gifts_given=data.get("gifts_given", 0),
            quests_completed=data.get("quests_completed", 0),
            metadata=data.get("metadata", {})
        )
```

---

## 🔄 核心流程

### 1. 好感度计算

#### 计算公式

```python
好感度变化 = 基础分 + 情感加成 + 互动类型加成 + 时间衰减

基础分:
- 正面对话: +1~3分
- 中性对话: +0~1分
- 负面对话: -1~5分

情感加成:
- 非常积极: +2分
- 积极: +1分
- 中性: 0分
- 消极: -1分
- 非常消极: -2分

互动类型加成:
- 赠送礼物: +5~15分
- 完成任务: +10~20分
- 帮助NPC: +5~10分
- 攻击NPC: -20~-50分

时间衰减:
- 长时间未互动: -0.1分/天 (最多-10分)
```

#### 实现代码

```python
class RelationshipManager:
    """关系管理器"""
    
    def calculate_affinity_change(
        self,
        message: str,
        reply: str,
        interaction_type: str = "dialogue",
        extra_context: Dict[str, Any] = None
    ) -> int:
        """
        计算好感度变化
        
        Args:
            message: 用户消息
            reply: NPC回复
            interaction_type: 互动类型 (dialogue/gift/quest/help/attack)
            extra_context: 额外上下文
        
        Returns:
            int: 好感度变化值 (-50 ~ +20)
        """
        delta = 0
        
        # 1. 基础分 (根据互动类型)
        base_scores = {
            "dialogue": 1,      # 普通对话
            "gift": 10,         # 赠送礼物
            "quest": 15,        # 完成任务
            "help": 8,          # 帮助NPC
            "attack": -30,      # 攻击NPC
            "trade": 2,         # 交易
            "praise": 3,        # 称赞
            "insult": -5        # 侮辱
        }
        delta += base_scores.get(interaction_type, 1)
        
        # 2. 情感分析加成
        sentiment_score = self._analyze_sentiment(message)
        
        if sentiment_score > 0.5:
            delta += 2  # 非常积极
        elif sentiment_score > 0.2:
            delta += 1  # 积极
        elif sentiment_score < -0.5:
            delta -= 2  # 非常消极
        elif sentiment_score < -0.2:
            delta -= 1  # 消极
        
        # 3. 关键词加成
        positive_keywords = ["谢谢", "感谢", "帮助", "朋友", "喜欢", "好"]
        negative_keywords = ["讨厌", "滚", "笨", "蠢", "骗子", "坏"]
        
        for kw in positive_keywords:
            if kw in message:
                delta += 1
                break
        
        for kw in negative_keywords:
            if kw in message:
                delta -= 2
                break
        
        # 4. 额外上下文加成
        if extra_context:
            # 礼物价值
            if "gift_value" in extra_context:
                gift_value = extra_context["gift_value"]
                delta += min(gift_value // 10, 10)  # 最多+10分
            
            # 任务难度
            if "quest_difficulty" in extra_context:
                difficulty = extra_context["quest_difficulty"]
                difficulty_bonus = {
                    "easy": 5,
                    "medium": 10,
                    "hard": 15,
                    "legendary": 20
                }
                delta += difficulty_bonus.get(difficulty, 10)
        
        # 5. 限制范围
        delta = max(-50, min(20, delta))
        
        return delta
    
    def _analyze_sentiment(self, text: str) -> float:
        """
        情感分析
        
        Returns:
            float: 情感得分 (-1.0 ~ 1.0)
                  -1.0: 非常消极
                   0.0: 中性
                   1.0: 非常积极
        """
        # 简化版情感分析 (实际可以使用NLP模型)
        positive_words = [
            "好", "棒", "优秀", "喜欢", "爱", "感谢", "谢谢",
            "帮助", "朋友", "开心", "高兴", "快乐", "美好"
        ]
        negative_words = [
            "坏", "差", "讨厌", "恨", "笨", "蠢", "傻",
            "骗", "滚", "死", "烂", "垃圾", "糟糕"
        ]
        
        positive_count = sum(1 for word in positive_words if word in text)
        negative_count = sum(1 for word in negative_words if word in text)
        
        total = positive_count + negative_count
        if total == 0:
            return 0.0
        
        score = (positive_count - negative_count) / total
        return max(-1.0, min(1.0, score))
```

---

### 2. 好感度更新

#### 流程图

```
用户互动
    ↓
计算好感度变化 (calculate_affinity_change)
    ↓
获取当前好感度 (get_affinity)
    ↓
更新分数 (score += delta)
    ↓
检查等级变化
    ↓
┌─────────┴─────────┐
↓                   ↓
等级提升            等级下降
触发升级事件        触发降级事件
    ↓                   ↓
解锁新秘密          锁定秘密
    ↓                   ↓
保存到数据库
    ↓
返回新的好感度信息
```

#### 实现代码

```python
def update_affinity(
    self,
    npc_id: str,
    player_id: str,
    delta: int,
    interaction_type: str = "dialogue",
    metadata: Dict[str, Any] = None
) -> AffinityInfo:
    """
    更新好感度
    
    Args:
        npc_id: NPC ID
        player_id: 玩家ID
        delta: 好感度变化值
        interaction_type: 互动类型
        metadata: 额外元数据
    
    Returns:
        AffinityInfo: 更新后的好感度信息
    """
    # 1. 获取当前好感度
    affinity = self.get_affinity(npc_id, player_id)
    old_level = affinity.level
    old_score = affinity.score
    
    # 2. 更新分数
    affinity.score += delta
    affinity.score = max(0, min(100, affinity.score))  # 限制在0-100
    
    # 3. 更新等级
    new_level = AffinityLevel.from_score(affinity.score)
    affinity.level = new_level
    
    # 4. 更新统计
    affinity.interaction_count += 1
    affinity.last_interaction = datetime.now()
    
    if delta > 0:
        affinity.positive_count += 1
    elif delta < 0:
        affinity.negative_count += 1
    
    # 根据互动类型更新
    if interaction_type == "gift":
        affinity.gifts_given += 1
    elif interaction_type == "quest":
        affinity.quests_completed += 1
    
    # 5. 更新元数据
    if metadata:
        affinity.metadata.update(metadata)
    
    # 6. 检查等级变化
    if new_level != old_level:
        self._on_level_change(affinity, old_level, new_level)
    
    # 7. 保存到数据库
    self._save_to_db(affinity)
    
    # 8. 更新缓存
    cache_key = (npc_id, player_id)
    self.affinity_cache[cache_key] = affinity
    
    # 9. 记录历史
    self._record_history(
        npc_id=npc_id,
        player_id=player_id,
        old_score=old_score,
        new_score=affinity.score,
        delta=delta,
        interaction_type=interaction_type
    )
    
    return affinity

def _on_level_change(
    self,
    affinity: AffinityInfo,
    old_level: AffinityLevel,
    new_level: AffinityLevel
):
    """
    等级变化时的回调
    
    Args:
        affinity: 好感度信息
        old_level: 旧等级
        new_level: 新等级
    """
    # 判断是升级还是降级
    level_order = [
        AffinityLevel.STRANGER,
        AffinityLevel.FAMILIAR,
        AffinityLevel.FRIENDLY,
        AffinityLevel.CLOSE,
        AffinityLevel.BEST_FRIEND
    ]
    
    old_index = level_order.index(old_level)
    new_index = level_order.index(new_level)
    
    if new_index > old_index:
        # 升级
        print(f"[RelationshipManager] {affinity.npc_id}与{affinity.player_id}的关系升级: {old_level.value} → {new_level.value}")
        
        # 触发升级事件
        self._trigger_event("level_up", affinity, old_level, new_level)
        
        # 解锁新秘密
        self._unlock_secrets(affinity)
        
    else:
        # 降级
        print(f"[RelationshipManager] {affinity.npc_id}与{affinity.player_id}的关系降级: {old_level.value} → {new_level.value}")
        
        # 触发降级事件
        self._trigger_event("level_down", affinity, old_level, new_level)
        
        # 锁定秘密
        self._lock_secrets(affinity)

def _trigger_event(
    self,
    event_type: str,
    affinity: AffinityInfo,
    old_level: AffinityLevel,
    new_level: AffinityLevel
):
    """触发事件"""
    # 可以在这里添加事件监听器
    # 例如: 发送通知、更新UI、触发剧情等
    
    event_data = {
        "type": event_type,
        "npc_id": affinity.npc_id,
        "player_id": affinity.player_id,
        "old_level": old_level.value,
        "new_level": new_level.value,
        "score": affinity.score,
        "timestamp": datetime.now().isoformat()
    }
    
    # 调用注册的事件处理器
    if hasattr(self, 'event_handlers'):
        for handler in self.event_handlers.get(event_type, []):
            handler(event_data)
```

---

### 3. 时间衰减

长时间未互动会导致好感度下降。

```python
def apply_time_decay(
    self,
    npc_id: str,
    player_id: str
) -> AffinityInfo:
    """
    应用时间衰减
    
    策略:
    - 每天未互动: -0.1分
    - 最多衰减: -10分
    - 挚友等级: 衰减减半
    """
    affinity = self.get_affinity(npc_id, player_id)
    
    # 计算距离上次互动的天数
    now = datetime.now()
    days_since_last = (now - affinity.last_interaction).days
    
    if days_since_last > 0:
        # 计算衰减
        decay_rate = 0.1  # 每天0.1分
        
        # 挚友等级衰减减半
        if affinity.level == AffinityLevel.BEST_FRIEND:
            decay_rate *= 0.5
        
        decay = min(days_since_last * decay_rate, 10)  # 最多-10分
        
        # 应用衰减
        if decay > 0:
            print(f"[RelationshipManager] 应用时间衰减: {npc_id}-{player_id}, -{decay:.1f}分 ({days_since_last}天未互动)")
            self.update_affinity(
                npc_id=npc_id,
                player_id=player_id,
                delta=-int(decay),
                interaction_type="time_decay"
            )
    
    return affinity

def apply_time_decay_batch(self):
    """批量应用时间衰减 (定时任务)"""
    # 获取所有关系
    conn = sqlite3.connect(self.db_path)
    cursor = conn.cursor()
    
    cursor.execute("""
        SELECT npc_id, player_id, last_interaction
        FROM relationships
        WHERE julianday('now') - julianday(last_interaction) > 1
    """)
    
    rows = cursor.fetchall()
    conn.close()
    
    # 应用衰减
    for npc_id, player_id, last_interaction in rows:
        self.apply_time_decay(npc_id, player_id)
    
    print(f"[RelationshipManager] 批量时间衰减完成: {len(rows)}个关系")
```

---

## 💾 数据库设计

### 表结构

```sql
-- 关系表
CREATE TABLE relationships (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    npc_id TEXT NOT NULL,
    player_id TEXT NOT NULL,
    score INTEGER DEFAULT 0,
    level TEXT DEFAULT '陌生',
    interaction_count INTEGER DEFAULT 0,
    last_interaction TEXT,
    first_met TEXT,
    positive_count INTEGER DEFAULT 0,
    negative_count INTEGER DEFAULT 0,
    gifts_given INTEGER DEFAULT 0,
    quests_completed INTEGER DEFAULT 0,
    metadata TEXT,  -- JSON
    created_at TEXT DEFAULT CURRENT_TIMESTAMP,
    updated_at TEXT DEFAULT CURRENT_TIMESTAMP,
    UNIQUE(npc_id, player_id)
);

-- 关系历史表
CREATE TABLE relationship_history (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    npc_id TEXT NOT NULL,
    player_id TEXT NOT NULL,
    old_score INTEGER,
    new_score INTEGER,
    delta INTEGER,
    interaction_type TEXT,
    timestamp TEXT DEFAULT CURRENT_TIMESTAMP,
    metadata TEXT  -- JSON
);

-- 索引
CREATE INDEX idx_relationships_npc ON relationships(npc_id);
CREATE INDEX idx_relationships_player ON relationships(player_id);
CREATE INDEX idx_relationships_level ON relationships(level);
CREATE INDEX idx_history_npc_player ON relationship_history(npc_id, player_id);
CREATE INDEX idx_history_timestamp ON relationship_history(timestamp);
```

### 数据库操作

```python
def _init_database(self):
    """初始化数据库"""
    conn = sqlite3.connect(self.db_path)
    cursor = conn.cursor()
    
    # 创建表 (见上面的SQL)
    cursor.execute("""...""")
    
    conn.commit()
    conn.close()
    
    print(f"[RelationshipManager] 数据库初始化完成: {self.db_path}")

def _save_to_db(self, affinity: AffinityInfo):
    """保存到数据库"""
    conn = sqlite3.connect(self.db_path)
    cursor = conn.cursor()
    
    cursor.execute("""
        INSERT OR REPLACE INTO relationships
        (npc_id, player_id, score, level, interaction_count,
         last_interaction, first_met, positive_count, negative_count,
         gifts_given, quests_completed, metadata, updated_at)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    """, (
        affinity.npc_id,
        affinity.player_id,
        affinity.score,
        affinity.level.value,
        affinity.interaction_count,
        affinity.last_interaction.isoformat(),
        affinity.first_met.isoformat(),
        affinity.positive_count,
        affinity.negative_count,
        affinity.gifts_given,
        affinity.quests_completed,
        json.dumps(affinity.metadata, ensure_ascii=False),
        datetime.now().isoformat()
    ))
    
    conn.commit()
    conn.close()

def _load_from_db(
    self,
    npc_id: str,
    player_id: str
) -> Optional[AffinityInfo]:
    """从数据库加载"""
    conn = sqlite3.connect(self.db_path)
    cursor = conn.cursor()
    
    cursor.execute("""
        SELECT npc_id, player_id, score, level, interaction_count,
               last_interaction, first_met, positive_count, negative_count,
               gifts_given, quests_completed, metadata
        FROM relationships
        WHERE npc_id = ? AND player_id = ?
    """, (npc_id, player_id))
    
    row = cursor.fetchone()
    conn.close()
    
    if row:
        return AffinityInfo(
            npc_id=row[0],
            player_id=row[1],
            score=row[2],
            level=AffinityLevel(row[3]),
            interaction_count=row[4],
            last_interaction=datetime.fromisoformat(row[5]),
            first_met=datetime.fromisoformat(row[6]),
            positive_count=row[7],
            negative_count=row[8],
            gifts_given=row[9],
            quests_completed=row[10],
            metadata=json.loads(row[11]) if row[11] else {}
        )
    
    return None

def _record_history(
    self,
    npc_id: str,
    player_id: str,
    old_score: int,
    new_score: int,
    delta: int,
    interaction_type: str,
    metadata: Dict = None
):
    """记录历史"""
    conn = sqlite3.connect(self.db_path)
    cursor = conn.cursor()
    
    cursor.execute("""
        INSERT INTO relationship_history
        (npc_id, player_id, old_score, new_score, delta, interaction_type, metadata)
        VALUES (?, ?, ?, ?, ?, ?, ?)
    """, (
        npc_id,
        player_id,
        old_score,
        new_score,
        delta,
        interaction_type,
        json.dumps(metadata, ensure_ascii=False) if metadata else None
    ))
    
    conn.commit()
    conn.close()
```

---

继续阅读第2部分...
