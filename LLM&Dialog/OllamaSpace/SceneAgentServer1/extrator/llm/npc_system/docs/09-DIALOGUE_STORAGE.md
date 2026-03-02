# 09 - 对话存储详解 (DialogueStorage)

> **面向对象**: 系统开发者、数据库管理员  
> **前置知识**: SQLite、文件系统、数据序列化  
> **相关模块**: NPCAgent, FileMemoryStore

## 📋 模块概述

### 职责定义

DialogueStorage 是 NPC 系统的**对话持久化核心**，负责将对话数据以多种格式存储和检索。

**核心职责**:
1. **SQLite存储**: 结构化存储，支持复杂查询
2. **Markdown存储**: 人类可读，便于审查和备份
3. **Excel导出**: 数据分析和报表生成
4. **格式兼容**: 兼容LangChain和原有系统

### 设计理念

**多格式存储策略**:
```
对话数据 → SQLite (快速查询)
         → Markdown (人类可读)
         → Excel (数据分析)
```

**为什么需要多格式存储?**

```python
# SQLite: 快速查询
SELECT * FROM dialogue_messages 
WHERE player_id = 'player_001' 
  AND timestamp > '2026-01-01'
ORDER BY timestamp DESC
LIMIT 10;
# 查询时间: 0.01秒

# Markdown: 人类可读
## 2026-01-17 14:30
**玩家**: 你好
**老铁匠**: 你好，需要什么？

## 2026-01-17 14:31
**玩家**: 能帮我打造一把剑吗？
**老铁匠**: 当然可以！你想要什么样的剑？

# Excel: 数据分析
| 时间 | NPC | 玩家 | 消息 | 好感度 |
|------|-----|------|------|--------|
| ... | ... | ... | ... | ... |
→ 可以用Excel进行数据透视、图表分析
```

---

## 🏗️ 架构设计

### 类图

```python
┌─────────────────────────────────────────────────────────┐
│              DialogueStorage                             │
├─────────────────────────────────────────────────────────┤
│ - sqlite_store: SQLiteDialogueStore                     │
│ - markdown_store: MarkdownDialogueStore                 │
│ - excel_exporter: ExcelDialogueExporter                 │
│ - config: StorageConfig                                 │
├─────────────────────────────────────────────────────────┤
│ + save_message(message) -> str                          │
│ + get_history(npc_id, player_id, limit) -> List        │
│ + get_session(session_id) -> DialogueSession           │
│ + export_to_excel(session_id, filepath) -> str         │
│ + get_stats() -> Dict                                   │
├─────────────────────────────────────────────────────────┤
│ - _save_to_sqlite(message) -> None                     │
│ - _save_to_markdown(message) -> None                   │
│ - _format_for_langchain(message) -> str                │
└─────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────┐
│          SQLiteDialogueStore                             │
├─────────────────────────────────────────────────────────┤
│ - db_path: str                                          │
│ - connection_pool: Dict                                 │
├─────────────────────────────────────────────────────────┤
│ + add_message(message) -> str                           │
│ + get_messages(session_id, limit) -> List              │
│ + get_npc_dialogues(npc_id, player_id) -> List         │
│ + create_session(session) -> str                       │
│ + update_session(session_id, data) -> bool             │
└─────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────┐
│        MarkdownDialogueStore                             │
├─────────────────────────────────────────────────────────┤
│ - base_dir: str                                         │
│ - file_cache: Dict                                      │
├─────────────────────────────────────────────────────────┤
│ + save_dialogue(message) -> str                         │
│ + load_dialogues(npc_id, player_id, date) -> List      │
│ + get_dialogue_file(npc_id, player_id, date) -> str    │
└─────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────┐
│         ExcelDialogueExporter                            │
├─────────────────────────────────────────────────────────┤
│ - export_dir: str                                       │
├─────────────────────────────────────────────────────────┤
│ + export_session(session_id, filepath) -> str          │
│ + export_npc_dialogues(npc_id, filepath) -> str        │
│ + export_player_dialogues(player_id, filepath) -> str  │
└─────────────────────────────────────────────────────────┘
```

### 数据结构

#### DialogueMessage (对话消息)

```python
from dataclasses import dataclass, field
from datetime import datetime
from typing import Dict, Any

@dataclass
class DialogueMessage:
    """对话消息"""
    message_id: str = ""                    # 消息ID
    session_id: str = ""                    # 会话ID
    npc_id: str = ""                        # NPC ID
    player_id: str = ""                     # 玩家ID
    role: str = "user"                      # 角色: user/assistant/system
    content: str = ""                       # 消息内容
    timestamp: str = ""                     # 时间戳
    metadata: Dict[str, Any] = field(default_factory=dict)  # 元数据
    
    def __post_init__(self):
        """初始化后处理"""
        if not self.message_id:
            self.message_id = f"msg_{datetime.now().strftime('%Y%m%d%H%M%S%f')}"
        if not self.timestamp:
            self.timestamp = datetime.now().isoformat()
    
    def to_dict(self) -> dict:
        """转换为字典"""
        return {
            "message_id": self.message_id,
            "session_id": self.session_id,
            "npc_id": self.npc_id,
            "player_id": self.player_id,
            "role": self.role,
            "content": self.content,
            "timestamp": self.timestamp,
            "metadata": self.metadata
        }
    
    @classmethod
    def from_dict(cls, data: dict) -> 'DialogueMessage':
        """从字典创建"""
        return cls(
            message_id=data.get("message_id", ""),
            session_id=data.get("session_id", ""),
            npc_id=data.get("npc_id", ""),
            player_id=data.get("player_id", ""),
            role=data.get("role", "user"),
            content=data.get("content", ""),
            timestamp=data.get("timestamp", ""),
            metadata=data.get("metadata", {})
        )
```

#### DialogueSession (对话会话)

```python
@dataclass
class DialogueSession:
    """对话会话"""
    session_id: str = ""                    # 会话ID
    npc_id: str = ""                        # NPC ID
    player_id: str = ""                     # 玩家ID
    start_time: str = ""                    # 开始时间
    end_time: str = ""                      # 结束时间
    message_count: int = 0                  # 消息数量
    summary: str = ""                       # 会话摘要
    metadata: Dict[str, Any] = field(default_factory=dict)  # 元数据
    
    def __post_init__(self):
        if not self.session_id:
            self.session_id = f"session_{datetime.now().strftime('%Y%m%d_%H%M%S')}"
        if not self.start_time:
            self.start_time = datetime.now().isoformat()
    
    def to_dict(self) -> dict:
        return {
            "session_id": self.session_id,
            "npc_id": self.npc_id,
            "player_id": self.player_id,
            "start_time": self.start_time,
            "end_time": self.end_time,
            "message_count": self.message_count,
            "summary": self.summary,
            "metadata": self.metadata
        }
```

---

## 💾 SQLite存储

### 数据库设计

#### 表结构

```sql
-- 会话表
CREATE TABLE dialogue_sessions (
    session_id TEXT PRIMARY KEY,
    npc_id TEXT NOT NULL,
    player_id TEXT NOT NULL,
    start_time TEXT NOT NULL,
    end_time TEXT,
    message_count INTEGER DEFAULT 0,
    summary TEXT,
    metadata TEXT,  -- JSON
    created_at TEXT DEFAULT CURRENT_TIMESTAMP,
    updated_at TEXT DEFAULT CURRENT_TIMESTAMP
);

-- 消息表
CREATE TABLE dialogue_messages (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    message_id TEXT UNIQUE NOT NULL,
    session_id TEXT NOT NULL,
    npc_id TEXT NOT NULL,
    player_id TEXT NOT NULL,
    role TEXT NOT NULL,  -- user/assistant/system
    content TEXT NOT NULL,
    timestamp TEXT NOT NULL,
    metadata TEXT,  -- JSON
    created_at TEXT DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (session_id) REFERENCES dialogue_sessions(session_id)
);

-- LangChain兼容表
CREATE TABLE message_store (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    session_id TEXT NOT NULL,
    message TEXT NOT NULL,  -- JSON格式
    created_at TEXT DEFAULT CURRENT_TIMESTAMP
);

-- 索引
CREATE INDEX idx_messages_session ON dialogue_messages(session_id);
CREATE INDEX idx_messages_npc ON dialogue_messages(npc_id);
CREATE INDEX idx_messages_player ON dialogue_messages(player_id);
CREATE INDEX idx_messages_timestamp ON dialogue_messages(timestamp);
CREATE INDEX idx_messages_npc_player ON dialogue_messages(npc_id, player_id);
CREATE INDEX idx_langchain_session ON message_store(session_id);
CREATE INDEX idx_sessions_npc_player ON dialogue_sessions(npc_id, player_id);
```

### 核心实现

#### 保存消息

```python
class SQLiteDialogueStore:
    """SQLite对话存储"""
    
    def __init__(self, db_path: str):
        self.db_path = db_path
        self._init_database()
    
    def add_message(self, message: DialogueMessage) -> str:
        """
        添加消息
        
        Args:
            message: 对话消息
        
        Returns:
            str: 消息ID
        """
        conn = sqlite3.connect(self.db_path)
        cursor = conn.cursor()
        
        try:
            # 1. 添加到dialogue_messages表
            cursor.execute("""
                INSERT INTO dialogue_messages
                (message_id, session_id, npc_id, player_id, role, content, timestamp, metadata)
                VALUES (?, ?, ?, ?, ?, ?, ?, ?)
            """, (
                message.message_id,
                message.session_id,
                message.npc_id,
                message.player_id,
                message.role,
                message.content,
                message.timestamp,
                json.dumps(message.metadata, ensure_ascii=False)
            ))
            
            # 2. 添加到message_store表 (LangChain兼容)
            langchain_message = json.dumps({
                "type": message.role,
                "data": {
                    "content": message.content,
                    "additional_kwargs": message.metadata
                }
            }, ensure_ascii=False)
            
            cursor.execute("""
                INSERT INTO message_store (session_id, message)
                VALUES (?, ?)
            """, (message.session_id, langchain_message))
            
            # 3. 更新会话统计
            cursor.execute("""
                UPDATE dialogue_sessions
                SET message_count = message_count + 1,
                    end_time = ?,
                    updated_at = ?
                WHERE session_id = ?
            """, (message.timestamp, datetime.now().isoformat(), message.session_id))
            
            # 4. 如果会话不存在，创建它
            if cursor.rowcount == 0:
                self._create_session_if_not_exists(cursor, message)
            
            conn.commit()
            return message.message_id
            
        except Exception as e:
            conn.rollback()
            raise e
        finally:
            conn.close()
    
    def _create_session_if_not_exists(self, cursor, message: DialogueMessage):
        """创建会话（如果不存在）"""
        cursor.execute("""
            INSERT OR IGNORE INTO dialogue_sessions
            (session_id, npc_id, player_id, start_time, message_count)
            VALUES (?, ?, ?, ?, 1)
        """, (
            message.session_id,
            message.npc_id,
            message.player_id,
            message.timestamp
        ))
```

#### 查询消息

```python
def get_session_messages(
    self,
    session_id: str,
    limit: int = 100,
    offset: int = 0
) -> List[DialogueMessage]:
    """
    获取会话消息
    
    Args:
        session_id: 会话ID
        limit: 返回数量限制
        offset: 偏移量
    
    Returns:
        List[DialogueMessage]: 消息列表
    """
    conn = sqlite3.connect(self.db_path)
    cursor = conn.cursor()
    
    cursor.execute("""
        SELECT message_id, session_id, npc_id, player_id, 
               role, content, timestamp, metadata
        FROM dialogue_messages
        WHERE session_id = ?
        ORDER BY timestamp ASC
        LIMIT ? OFFSET ?
    """, (session_id, limit, offset))
    
    rows = cursor.fetchall()
    conn.close()
    
    messages = []
    for row in rows:
        messages.append(DialogueMessage(
            message_id=row[0],
            session_id=row[1],
            npc_id=row[2],
            player_id=row[3],
            role=row[4],
            content=row[5],
            timestamp=row[6],
            metadata=json.loads(row[7]) if row[7] else {}
        ))
    
    return messages

def get_npc_dialogues(
    self,
    npc_id: str,
    player_id: str = None,
    start_date: str = None,
    end_date: str = None,
    limit: int = 50
) -> List[DialogueMessage]:
    """
    获取NPC的对话记录
    
    Args:
        npc_id: NPC ID
        player_id: 玩家ID (可选)
        start_date: 开始日期 (可选)
        end_date: 结束日期 (可选)
        limit: 返回数量限制
    
    Returns:
        List[DialogueMessage]: 消息列表
    """
    conn = sqlite3.connect(self.db_path)
    cursor = conn.cursor()
    
    # 构建查询
    query = """
        SELECT message_id, session_id, npc_id, player_id,
               role, content, timestamp, metadata
        FROM dialogue_messages
        WHERE npc_id = ?
    """
    params = [npc_id]
    
    if player_id:
        query += " AND player_id = ?"
        params.append(player_id)
    
    if start_date:
        query += " AND timestamp >= ?"
        params.append(start_date)
    
    if end_date:
        query += " AND timestamp <= ?"
        params.append(end_date)
    
    query += " ORDER BY timestamp DESC LIMIT ?"
    params.append(limit)
    
    cursor.execute(query, params)
    rows = cursor.fetchall()
    conn.close()
    
    messages = []
    for row in rows:
        messages.append(DialogueMessage(
            message_id=row[0],
            session_id=row[1],
            npc_id=row[2],
            player_id=row[3],
            role=row[4],
            content=row[5],
            timestamp=row[6],
            metadata=json.loads(row[7]) if row[7] else {}
        ))
    
    return messages
```

#### 统计查询

```python
def get_stats(self) -> Dict[str, Any]:
    """获取统计信息"""
    conn = sqlite3.connect(self.db_path)
    cursor = conn.cursor()
    
    # 总消息数
    cursor.execute("SELECT COUNT(*) FROM dialogue_messages")
    total_messages = cursor.fetchone()[0]
    
    # 总会话数
    cursor.execute("SELECT COUNT(*) FROM dialogue_sessions")
    total_sessions = cursor.fetchone()[0]
    
    # 活跃NPC数
    cursor.execute("SELECT COUNT(DISTINCT npc_id) FROM dialogue_messages")
    active_npcs = cursor.fetchone()[0]
    
    # 活跃玩家数
    cursor.execute("SELECT COUNT(DISTINCT player_id) FROM dialogue_messages")
    active_players = cursor.fetchone()[0]
    
    # 今日消息数
    today = datetime.now().date().isoformat()
    cursor.execute("""
        SELECT COUNT(*) FROM dialogue_messages
        WHERE DATE(timestamp) = ?
    """, (today,))
    today_messages = cursor.fetchone()[0]
    
    # 数据库大小
    cursor.execute("SELECT page_count * page_size as size FROM pragma_page_count(), pragma_page_size()")
    db_size = cursor.fetchone()[0]
    
    conn.close()
    
    return {
        "total_messages": total_messages,
        "total_sessions": total_sessions,
        "active_npcs": active_npcs,
        "active_players": active_players,
        "today_messages": today_messages,
        "db_size_mb": db_size / (1024 * 1024)
    }
```

---

## 📝 Markdown存储

### 文件组织

```
npc_data/memories/dialogues/
├── blacksmith/              # NPC目录
│   ├── player_001/          # 玩家目录
│   │   ├── 20260117.md      # 按日期存储
│   │   ├── 20260118.md
│   │   └── 20260119.md
│   └── player_002/
│       └── 20260117.md
├── merchant/
│   └── player_001/
│       └── 20260117.md
└── innkeeper/
    └── player_001/
        └── 20260117.md
```

### Markdown格式

```markdown
---
npc_id: blacksmith
player_id: player_001
date: 2026-01-17
session_id: session_20260117_143022
message_count: 10
---

# 对话记录 - 2026-01-17

## 14:30:15
**玩家**: 你好

**老铁匠**: 你好，需要什么？

---

## 14:31:22
**玩家**: 能帮我打造一把剑吗？

**老铁匠**: 当然可以！你想要什么样的剑？普通的铁剑50金币，如果想要更好的精钢剑，需要120金币。制作时间大约需要2天。

*[好感度: 友好 (57/100)]*

---

## 14:32:45
**玩家**: 我要铁剑

**老铁匠**: 好的，给我50金币和2天时间。我会给你打造一把结实的铁剑。

*[好感度: 友好 (58/100)]*

---

## 统计信息
- 总消息数: 6
- 开始时间: 14:30:15
- 结束时间: 14:35:30
- 持续时间: 5分15秒
```

### 核心实现

```python
class MarkdownDialogueStore:
    """Markdown对话存储"""
    
    def __init__(self, base_dir: str):
        self.base_dir = Path(base_dir)
        self.file_cache = {}
    
    def save_dialogue(
        self,
        message: DialogueMessage,
        affinity_info: str = None
    ) -> str:
        """
        保存对话到Markdown
        
        Args:
            message: 对话消息
            affinity_info: 好感度信息 (可选)
        
        Returns:
            str: 文件路径
        """
        # 1. 确定文件路径
        date = datetime.fromisoformat(message.timestamp).date()
        file_path = self._get_dialogue_file(
            message.npc_id,
            message.player_id,
            date.isoformat()
        )
        
        # 2. 确保目录存在
        file_path.parent.mkdir(parents=True, exist_ok=True)
        
        # 3. 读取或创建文件
        if file_path.exists():
            content = file_path.read_text(encoding='utf-8')
        else:
            # 创建新文件，添加YAML头
            content = self._create_markdown_header(message, date)
        
        # 4. 添加新消息
        time_str = datetime.fromisoformat(message.timestamp).strftime('%H:%M:%S')
        role_name = "玩家" if message.role == "user" else message.metadata.get("npc_name", "NPC")
        
        new_content = f"\n## {time_str}\n"
        new_content += f"**{role_name}**: {message.content}\n"
        
        # 添加好感度信息
        if affinity_info and message.role == "assistant":
            new_content += f"\n*[{affinity_info}]*\n"
        
        new_content += "\n---\n"
        
        # 5. 写入文件
        with open(file_path, 'a', encoding='utf-8') as f:
            f.write(new_content)
        
        return str(file_path)
    
    def _create_markdown_header(
        self,
        message: DialogueMessage,
        date: datetime.date
    ) -> str:
        """创建Markdown文件头"""
        header = f"""---
npc_id: {message.npc_id}
player_id: {message.player_id}
date: {date.isoformat()}
session_id: {message.session_id}
---

# 对话记录 - {date.isoformat()}

"""
        return header
    
    def _get_dialogue_file(
        self,
        npc_id: str,
        player_id: str,
        date: str
    ) -> Path:
        """获取对话文件路径"""
        return self.base_dir / "dialogues" / npc_id / player_id / f"{date}.md"
```

---

继续阅读第2部分...
