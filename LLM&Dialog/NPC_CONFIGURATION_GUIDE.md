# NPC系统配置说明

## 可用的NPC列表

### 1. blacksmith (老铁匠)
- **ID**: `blacksmith`
- **名称**: 老铁匠
- **角色**: 铁匠
- **年龄**: 55岁
- **性格**: 严肃、专业、热心、固执
- **背景**: 在镇上打铁30年，曾是冒险者"铁臂"
- **问候语**: "哟，又来了个冒险者。看你的样子，是来买武器的吧？"
- **知识领域**: 武器锻造、金属材料、镇上历史、冒险者装备、战斗技巧

### 2. merchant (精明商人)
- **ID**: `merchant`
- **名称**: 精明商人
- **角色**: 商人
- **年龄**: 42岁
- **性格**: 精明、健谈、贪财、消息灵通
- **背景**: 走南闯北的商人，商队遍布整个大陆
- **问候语**: "欢迎欢迎！贵客临门啊！看看有什么需要的？保证全镇最低价！"
- **知识领域**: 商品价格、各地风土人情、交易技巧、黑市信息、贵族八卦

### 3. guard_captain (守卫队长)
- **ID**: `guard_captain`
- **名称**: 守卫队长
- **角色**: 守卫
- **配置文件**: `npc_configs/guard_captain.json`

### 4. herbalist (草药师)
- **ID**: `herbalist`
- **名称**: 草药师
- **角色**: 草药师
- **配置文件**: `npc_configs/herbalist.json`

### 5. innkeeper (客栈老板)
- **ID**: `innkeeper`
- **名称**: 客栈老板
- **角色**: 客栈老板
- **配置文件**: `npc_configs/innkeeper.json`

## 如何切换NPC

### 方法1: 修改 DialogConfig.java

```java
// 在 DialogConfig.java 中修改默认NPC
public static final String DEFAULT_NPC_ID = "blacksmith";  // 改为其他NPC ID
public static final String DEFAULT_NPC_NAME = "老铁匠";     // 改为对应的名称
```

### 方法2: 在代码中动态切换

```java
// 在 MainActivity.java 中
dialogManager.startDialog("merchant", "精明商人");  // 切换到商人
dialogManager.startDialog("guard_captain", "守卫队长");  // 切换到守卫队长
```

### 方法3: 通过UI选择（需要实现）

可以添加一个NPC选择界面，让玩家选择要对话的NPC。

## NPC配置文件格式

位置: `OllamaSpace/SceneAgentServer/npc_configs/`

```json
{
  "npc_id": "npc的唯一标识",
  "personality": {
    "name": "NPC名称",
    "role": "角色职业",
    "age": 年龄,
    "gender": "性别",
    "traits": ["性格特点1", "性格特点2"],
    "background": "背景故事",
    "speech_style": "说话风格描述",
    "knowledge": ["知识领域1", "知识领域2"],
    "secrets": ["秘密1", "秘密2"],
    "greeting": "问候语"
  }
}
```

## 添加新NPC

### 步骤1: 创建配置文件

在 `npc_configs/` 目录下创建新的JSON文件，例如 `my_npc.json`：

```json
{
  "npc_id": "my_npc",
  "personality": {
    "name": "我的NPC",
    "role": "角色",
    "age": 30,
    "gender": "男",
    "traits": ["友好", "幽默"],
    "background": "这是一个新NPC的背景故事",
    "speech_style": "说话风格",
    "knowledge": ["知识1", "知识2"],
    "secrets": ["秘密1"],
    "greeting": "你好，欢迎！"
  }
}
```

### 步骤2: 更新 DialogManager.java

在 `initNpcResponses()` 方法中添加本地模拟回复：

```java
npcResponses.put("我的NPC", new String[]{
    "你好，欢迎！",
    "有什么可以帮助你的吗？",
    "很高兴见到你！"
});
```

### 步骤3: 使用新NPC

```java
// 在 DialogConfig.java 中
public static final String DEFAULT_NPC_ID = "my_npc";
public static final String DEFAULT_NPC_NAME = "我的NPC";
```

## NPC系统架构

```
Android App
    ↓ HTTP请求
Django Backend (/npc/chat/)
    ↓
NPCManager (npc_manager.py)
    ↓
NPCAgent (npc_agent.py)
    ├── MemoryTool (记忆系统)
    ├── RAGTool (知识检索)
    ├── ContextBuilder (上下文构建)
    ├── RelationshipManager (好感度系统)
    └── DialogueStorage (对话存储)
```

## 数据存储位置

```
npc_data/
├── memories/              # 记忆存储
│   ├── working/          # 工作记忆
│   ├── episodic/         # 情景记忆
│   └── semantic/         # 语义记忆
├── databases/            # 数据库
│   ├── dialogue_history.db    # 对话历史
│   ├── npc_memory.db          # NPC记忆
│   └── npc_relationship.db    # 好感度
└── exports/              # 导出文件
    └── excel/            # Excel导出
```

## 好感度系统

NPC会根据对话内容自动调整对玩家的好感度：

| 好感度等级 | 分数范围 | 描述 |
|-----------|---------|------|
| 陌生 | 0-20 | 初次见面 |
| 认识 | 21-40 | 有过几次交流 |
| 熟悉 | 41-60 | 经常交流 |
| 友好 | 61-80 | 关系不错 |
| 亲密 | 81-100 | 非常信任 |

## 测试NPC系统

### 命令行测试

```bash
cd OllamaSpace/SceneAgentServer
python -m extrator.llm.npc_system.npc_chat
```

### API测试

```bash
# 连接
curl -X POST http://localhost:8000/npc/connect/ \
  -H "Content-Type: application/json" \
  -d '{"player_id": "test_player"}'

# 对话
curl -X POST http://localhost:8000/npc/chat/ \
  -H "Content-Type: application/json" \
  -d '{
    "npc_id": "blacksmith",
    "message": "你好",
    "player_id": "test_player"
  }'
```

## 常见问题

### Q: 为什么NPC回复很慢？
A: 需要等待LLM生成回复，通常需要2-5秒。可以在 `DialogConfig.java` 中增加超时时间。

### Q: 如何查看NPC的记忆？
A: 访问 `GET /npc/status/<npc_id>/?player_id=xxx` 查看NPC状态和记忆摘要。

### Q: 如何重置好感度？
A: 删除 `npc_data/databases/npc_relationship.db` 文件，或使用新的 player_id。

### Q: 如何导出对话历史？
A: 调用 `POST /npc/export/` 或在命令行工具中输入 `export` 命令。
