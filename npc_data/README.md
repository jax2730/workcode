# NPC System Data Directory

Created: 2026-01-17 11:48:04

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
2. 知识库文档放在 `knowledge_base/{npc_id}/` 目录
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
{
  "npc_id": "merchant",
  "personality": {
    "name": "精明商人",
    "role": "商人",
    "traits": ["精明", "健谈"],
    ...
  }
}
```
