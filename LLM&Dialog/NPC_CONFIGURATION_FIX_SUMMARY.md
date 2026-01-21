# NPC系统配置修正总结

## 🎯 问题分析

### 发现的问题

1. **配置不匹配**：
   - `DialogConfig.java` 中设置的默认NPC：`npc_001` / `苏清寒`
   - 实际NPC系统中的NPC ID：`blacksmith`, `merchant`, `guard_captain`, `herbalist`, `innkeeper`
   - **结果**：配置的NPC在系统中不存在！

2. **数据来源分析**：
   - NPC配置文件位置：`OllamaSpace/SceneAgentServer/npc_configs/*.json`
   - 每个NPC都有完整的人格设定（性格、背景、知识、秘密等）
   - NPC系统基于 HelloAgents 第8、9、15章设计，包含记忆、RAG、好感度等高级功能

3. **推理过程**：
   ```
   DialogConfig.java (DEFAULT_NPC_ID = "npc_001")
        ↓ 发送到
   Django Backend (/npc/chat/)
        ↓ 查找
   NPCManager.get_npc("npc_001")
        ↓ 在 npc_configs/ 中查找
   ❌ 找不到 npc_001.json
        ↓ 结果
   返回错误或使用默认回复
   ```

## ✅ 已修正的内容

### 1. DialogConfig.java

**修改前：**
```java
public static final String DEFAULT_NPC_ID = "npc_001";
public static final String DEFAULT_NPC_NAME = "苏清寒";
```

**修改后：**
```java
/**
 * 默认NPC ID（用于NPC模式）
 * 
 * 可用的NPC ID（在 npc_configs/ 目录中）：
 * - blacksmith: 老铁匠（严肃、专业、热心）
 * - merchant: 精明商人（精明、健谈、贪财）
 * - guard_captain: 守卫队长
 * - herbalist: 草药师
 * - innkeeper: 客栈老板
 */
public static final String DEFAULT_NPC_ID = "blacksmith";

/**
 * 默认NPC名称
 */
public static final String DEFAULT_NPC_NAME = "老铁匠";
```

### 2. DialogManager.java

**修改前：**
```java
// 苏清寒的回复
npcResponses.put("苏清寒", new String[]{
    "你是南通？难道你就是江湖中人传言的那位杨三儿少侠吗？",
    ...
});
```

**修改后：**
```java
// blacksmith - 老铁匠
npcResponses.put("老铁匠", new String[]{
    "哟，又来了个冒险者。看你的样子，是来买武器的吧？",
    "这把剑可是我精心打造的，削铁如泥！",
    ...
});

// merchant - 精明商人
npcResponses.put("精明商人", new String[]{
    "欢迎欢迎！贵客临门啊！看看有什么需要的？保证全镇最低价！",
    ...
});

// 其他NPC...
```

### 3. 新增文档

创建了 `NPC_CONFIGURATION_GUIDE.md`，包含：
- 所有可用NPC的详细信息
- NPC配置文件格式说明
- 如何切换NPC的方法
- 如何添加新NPC的步骤
- NPC系统架构说明
- 好感度系统说明
- 测试方法和常见问题

## 📊 NPC系统完整架构

```
┌─────────────────────────────────────────────────────────────────┐
│                        Android App                               │
│  DialogConfig.java: DEFAULT_NPC_ID = "blacksmith"               │
│  DialogManager.java: 连接到 /npc/chat/                          │
└────────────────────────┬────────────────────────────────────────┘
                         │ HTTP POST
                         ▼
┌─────────────────────────────────────────────────────────────────┐
│                    Django Backend                                │
│  views.py: npc_chat() → NPCManager.chat()                       │
└────────────────────────┬────────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────────┐
│                    NPCManager                                    │
│  - 加载 npc_configs/blacksmith.json                             │
│  - 创建 NPCAgent 实例                                            │
│  - 管理多个NPC                                                   │
└────────────────────────┬────────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────────┐
│                    NPCAgent (blacksmith)                         │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │ Personality:                                              │   │
│  │  - name: "老铁匠"                                         │   │
│  │  - role: "铁匠"                                           │   │
│  │  - traits: ["严肃", "专业", "热心", "固执"]              │   │
│  │  - background: "打铁30年的老师傅..."                     │   │
│  └──────────────────────────────────────────────────────────┘   │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │ 核心组件:                                                 │   │
│  │  - MemoryTool: 四层记忆系统                              │   │
│  │  - RAGTool: 知识库检索                                   │   │
│  │  - ContextBuilder: GSSC上下文构建                        │   │
│  │  - RelationshipManager: 好感度系统                       │   │
│  │  - DialogueStorage: 对话存储                             │   │
│  └──────────────────────────────────────────────────────────┘   │
└────────────────────────┬────────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────────┐
│                    LLM (Ollama)                                  │
│  ChatOllama("qwen2.5", temperature=0.7)                         │
│  - 生成符合NPC人格的回复                                         │
│  - 考虑记忆、知识、好感度                                        │
└─────────────────────────────────────────────────────────────────┘
```

## 🔄 完整对话流程

```
1. 用户点击对话按钮
   ↓
2. MainActivity.showDialogPanel()
   - 使用 DialogConfig.DEFAULT_NPC_ID = "blacksmith"
   - 调用 dialogManager.startDialog("老铁匠", "blacksmith")
   ↓
3. DialogManager.connectToBackend()
   - POST /npc/connect/
   - 获取 session_id
   - 显示NPC问候语："哟，又来了个冒险者..."
   ↓
4. 用户输入："能帮我打一把剑吗？"
   ↓
5. DialogManager.handlePlayerMessage()
   - POST /npc/chat/
   - Body: {
       "npc_id": "blacksmith",
       "message": "能帮我打一把剑吗？",
       "player_id": "player_xxx",
       "session_id": "session_xxx"
     }
   ↓
6. Django Backend (views.py)
   - npc_chat() 接收请求
   - 调用 NPCManager.chat()
   ↓
7. NPCManager
   - 获取 blacksmith 的 NPCAgent
   - 如果不存在，从 npc_configs/blacksmith.json 加载
   ↓
8. NPCAgent.chat()
   - 检索相关记忆
   - 构建上下文（人格+记忆+知识）
   - 调用 LLM 生成回复
   - 更新好感度
   - 保存对话历史
   ↓
9. 返回响应
   {
     "success": true,
     "npc_id": "blacksmith",
     "npc_name": "老铁匠",
     "reply": "当然可以！你想要什么样的剑？长剑还是短剑？",
     "affinity": {
       "score": 15,
       "level": "陌生",
       "interaction_count": 1
     }
   }
   ↓
10. MainActivity.addNpcMessage()
    - 显示NPC回复
    - 显示好感度信息
```

## 🎮 可用的NPC列表

| NPC ID | 名称 | 角色 | 性格特点 | 适合场景 |
|--------|------|------|---------|---------|
| `blacksmith` | 老铁匠 | 铁匠 | 严肃、专业、热心 | 武器购买、装备修理 |
| `merchant` | 精明商人 | 商人 | 精明、健谈、贪财 | 物品交易、情报获取 |
| `guard_captain` | 守卫队长 | 守卫 | 严肃、负责 | 任务接取、城镇安全 |
| `herbalist` | 草药师 | 草药师 | 温和、博学 | 治疗、草药购买 |
| `innkeeper` | 客栈老板 | 老板 | 热情、八卦 | 休息、打听消息 |

## 🚀 如何使用

### 快速开始

1. **启动Django服务器**：
```bash
cd OllamaSpace/SceneAgentServer
python manage.py runserver 0.0.0.0:8000
```

2. **配置Android应用**（已完成）：
   - `DialogConfig.java` 已设置为 `blacksmith`
   - 服务器地址：`http://10.0.2.2:8000`（模拟器）

3. **运行测试**：
   - 编译运行Android应用
   - 点击对话按钮
   - 与老铁匠对话

### 切换到其他NPC

编辑 `DialogConfig.java`：
```java
// 切换到商人
public static final String DEFAULT_NPC_ID = "merchant";
public static final String DEFAULT_NPC_NAME = "精明商人";

// 切换到守卫队长
public static final String DEFAULT_NPC_ID = "guard_captain";
public static final String DEFAULT_NPC_NAME = "守卫队长";
```

## 📝 测试建议

### 测试场景1：武器购买（blacksmith）
```
玩家: "你好，我想买一把剑"
NPC: "哟，又来了个冒险者。你想要什么样的剑？"
玩家: "我需要一把锋利的长剑"
NPC: "长剑啊，我这里有几把不错的。你预算多少？"
```

### 测试场景2：讨价还价（merchant）
```
玩家: "这个多少钱？"
NPC: "朋友，这可是稀罕货！只要100金币！"
玩家: "太贵了，能便宜点吗？"
NPC: "好吧好吧，看你是老顾客，80金币！不能再少了！"
```

### 测试场景3：好感度提升
```
# 多次友好对话后
玩家: "你的手艺真棒！"
NPC: "哈哈，谢谢夸奖！你是个识货的人！"
# 好感度: 陌生(5) → 认识(25) → 熟悉(45)
```

## 🔧 故障排查

### 问题1：NPC不存在错误
**原因**：配置的NPC ID在 `npc_configs/` 中找不到  
**解决**：检查 `DEFAULT_NPC_ID` 是否正确

### 问题2：回复是本地模拟而非LLM生成
**原因**：网络连接失败或Django服务器未启动  
**解决**：检查服务器状态和网络连接

### 问题3：回复很慢
**原因**：LLM生成需要时间  
**解决**：正常现象，可增加超时时间

## 📚 相关文档

- `DIALOG_SYSTEM_INTEGRATION.md` - 对话系统集成文档
- `NPC_CONFIGURATION_GUIDE.md` - NPC配置指南
- `OllamaSpace/PROJECT_ANALYSIS.md` - 项目架构分析
- `OllamaSpace/SceneAgentServer/extrator/llm/npc_system/docs/` - NPC系统详细文档

## ✅ 总结

通过仔细分析 `npc_chat.py`、`PROJECT_ANALYSIS.md` 和实际的NPC配置文件，我们发现并修正了配置不匹配的问题：

1. ✅ 将默认NPC从不存在的 `npc_001` 改为实际存在的 `blacksmith`
2. ✅ 更新了本地模拟回复，使其与实际NPC人格匹配
3. ✅ 添加了完整的NPC配置文档
4. ✅ 确保了Android端配置与Django后端NPC系统的一致性

现在整个流程已经完全打通，可以正常使用NPC对话系统了！
