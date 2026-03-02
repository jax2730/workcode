# NPC 对话系统 API 指南

## 📋 概述

本系统提供 NPC 角色管理和对话功能，包含两个核心模块：

| 模块 | 功能 | 基础URL |
|------|------|---------|
| **Persona API** | 创建、管理 NPC 角色配置 | `/api/persona/` |
| **Chat API** | 与 NPC 进行对话 | `/connect/`, `/chat/`, `/clear/` |

---

## 🎯 标准使用流程

```
┌─────────────────────────────────────────────────────────────────┐
│                      完整对话流程                                │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  步骤1: 创建角色 (可选，如果角色已存在可跳过)                      │
│  POST /api/persona/                                             │
│  ↓ 返回 npc_id                                                  │
│                                                                 │
│  步骤2: 建立连接                                                 │
│  POST /connect/general/                                         │
│  ↓ 返回 session_id + 角色问候语                                  │
│                                                                 │
│  步骤3: 进行对话 (可多次)                                        │
│  POST /chat/general/  { session_id, npc_id, message }          │
│  ↓ 返回 AI 回复                                                 │
│                                                                 │
│  步骤4: 清除历史 (可选)                                          │
│  POST /clear/general/  { session_id }                          │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## 📖 快速开始（5分钟上手）

### 步骤 1：创建 NPC 角色

```powershell
$createResponse = Invoke-RestMethod -Uri "http://192.168.5.189:8000/api/persona/" -Method Post -Body @{
    planner_id = "test01"
    role_type = "tavern_owner"
    name = "李老板"
    role = "酒馆老板"
    greeting = "客官里面请！小店有上好的女儿红！"
    traits = "热情,健谈,八卦"
}

# 保存返回的 npc_id（重要！）
$npcId = $createResponse.data.npc_id
Write-Host "NPC ID: $npcId"
# 输出示例: a1b2c3d4_test01_tavern_owner
```

### 步骤 2：建立对话连接

```powershell
$connectBody = @{ persona = $npcId } | ConvertTo-Json
$connectResponse = Invoke-RestMethod -Uri "http://192.168.5.189:8000/connect/general/" `
    -Method Post -Body $connectBody -ContentType "application/json"

# 保存返回的 session_id（重要！）
$sessionId = $connectResponse.session_id
Write-Host "Session ID: $sessionId"
Write-Host "角色问候: $($connectResponse.reply)"
# 输出示例: 
# Session ID: general#192.168.5.189#uuid-xxx
# 角色问候: 客官里面请！小店有上好的女儿红！
```

### 步骤 3：进行对话

```powershell
$chatBody = @{
    message = "老板，有什么好酒推荐？"
    session_id = $sessionId
    persona = $npcId
} | ConvertTo-Json

$chatResponse = Invoke-RestMethod -Uri "http://192.168.5.189:8000/chat/general/" `
    -Method Post -Body $chatBody -ContentType "application/json"

Write-Host "AI: $($chatResponse.reply)"
# 输出示例: 哎呀客官好眼光！小店刚进了一批上好的女儿红...
```

### 步骤 4：继续对话（保持上下文）

```powershell
# 使用相同的 session_id 和 persona，对话会保持上下文
$chatBody = @{
    message = "那来一壶女儿红吧"
    session_id = $sessionId
    persona = $npcId
} | ConvertTo-Json

$chatResponse = Invoke-RestMethod -Uri "http://192.168.5.189:8000/chat/general/" `
    -Method Post -Body $chatBody -ContentType "application/json"

Write-Host "AI: $($chatResponse.reply)"
```

---

## 📚 API 详细说明

### 一、Persona API（角色管理）

#### 1.1 创建角色

**POST** `/api/persona/`

**请求参数：**

| 字段 | 类型 | 必填 | 说明 |
|------|------|:----:|------|
| planner_id | string | ✅ | 策划ID，2-20字符，字母数字下划线 |
| role_type | string | ✅ | 角色类型，2-50字符 |
| name | string | ✅ | 角色名称，≤50字符 |
| role | string | ✅ | 角色职业，≤100字符 |
| greeting | string | ❌ | 问候语 |
| traits | string | ❌ | 性格特点，逗号分隔 |
| background | string | ❌ | 背景故事，≤1000字符 |
| speech_style | string | ❌ | 说话风格，≤500字符 |
| knowledge | string | ❌ | 擅长领域，逗号分隔 |

**响应示例：**
```json
{
    "success": true,
    "code": 201,
    "data": {
        "npc_id": "a1b2c3d4_test01_tavern_owner",
        "planner_id": "test01",
        "role_type": "tavern_owner",
        "personality": {
            "name": "李老板",
            "role": "酒馆老板",
            "greeting": "客官里面请！",
            "traits": ["热情", "健谈", "八卦"]
        },
        "created_at": "2026-02-05T10:30:00.123456",
        "updated_at": "2026-02-05T10:30:00.123456"
    }
}
```

> ⚠️ **重要**：`npc_id` 由系统自动生成（格式：`{随机前缀}_{planner_id}_{role_type}`），请保存此 ID 用于后续操作！

#### 1.2 查询所有角色

**GET** `/api/persona/`

```powershell
Invoke-RestMethod -Uri "http://192.168.5.189:8000/api/persona/"
```

#### 1.3 查询指定角色

**GET** `/api/persona/<npc_id>/`

```powershell
Invoke-RestMethod -Uri "http://192.168.5.189:8000/api/persona/$npcId/"
```

#### 1.4 更新角色

**PUT** `/api/persona/<npc_id>/`

```powershell
Invoke-RestMethod -Uri "http://192.168.5.189:8000/api/persona/$npcId/" -Method Put -Body @{
    greeting = "欢迎光临！今天有新酿的桂花酒！"
}
```

#### 1.5 删除角色

**DELETE** `/api/persona/<npc_id>/`

```powershell
Invoke-RestMethod -Uri "http://192.168.5.189:8000/api/persona/$npcId/" -Method Delete
```

---

### 二、Chat API（对话接口）

#### 2.1 建立连接

**POST** `/connect/general/`

建立对话会话，获取 `session_id`。可选择指定角色 `persona`。

**请求：**
```json
{
    "persona": "a1b2c3d4_test01_tavern_owner"  // 可选，指定 npc_id
}
```

**响应：**
```json
{
    "success": true,
    "reply": "客官里面请！小店有上好的女儿红！",
    "session_id": "general#192.168.5.189#uuid-xxx",
    "backend": "general",
    "persona": "a1b2c3d4_test01_tavern_owner"
}
```

> 💡 **说明**：
> - 如果指定了 `persona`，会使用该角色的配置进行对话
> - 如果不指定，会使用默认助手角色
> - 返回的 `reply` 是角色的问候语或初始回复

#### 2.2 进行对话

**POST** `/chat/general/`

**请求：**
```json
{
    "message": "老板，有什么好酒？",
    "session_id": "general#192.168.5.189#uuid-xxx",
    "persona": "a1b2c3d4_test01_tavern_owner"
}
```

**参数说明：**

| 字段 | 类型 | 必填 | 说明 |
|------|------|:----:|------|
| message | string | ✅ | 用户消息 |
| session_id | string | ✅ | 从 connect 获取的会话ID |
| persona | string | ❌ | npc_id，指定对话角色 |

**响应：**
```json
{
    "success": true,
    "reply": "哎呀客官好眼光！小店刚进了一批上好的女儿红...",
    "session_id": "general#192.168.5.189#uuid-xxx",
    "response_time": 1.234,
    "backend": "general",
    "persona": "a1b2c3d4_test01_tavern_owner"
}
```

#### 2.3 清除对话历史

**POST** `/clear/general/`

**请求：**
```json
{
    "session_id": "general#192.168.5.189#uuid-xxx"
}
```

**响应：**
```json
{
    "success": true,
    "reply": "历史记录已清除",
    "session_id": "general#192.168.5.189#uuid-xxx"
}
```

---

## 🔑 关键概念

### npc_id vs session_id

| 概念 | 说明 | 生命周期 |
|------|------|----------|
| **npc_id** | 角色唯一标识，由 Persona API 创建时生成 | 持久化，直到手动删除 |
| **session_id** | 对话会话标识，由 connect 接口生成 | 临时，每次连接生成新的 |

### 数据流向

```
                    ┌──────────────────┐
                    │   npc_configs/   │
                    │   *.json 文件    │
                    └────────┬─────────┘
                             │ 读取角色配置
                             ▼
┌─────────┐  npc_id   ┌──────────────┐  session_id  ┌─────────────┐
│ Persona │ ────────► │   Connect    │ ───────────► │    Chat     │
│   API   │           │   接口       │              │    接口     │
└─────────┘           └──────────────┘              └─────────────┘
     │                       │                            │
     │ 创建/管理角色          │ 建立会话                    │ 进行对话
     ▼                       ▼                            ▼
┌─────────────────────────────────────────────────────────────────┐
│                     SQLite 数据库                               │
│               (general_chat.db - 存储对话历史)                   │
└─────────────────────────────────────────────────────────────────┘
```

---

## 💻 完整示例代码

### PowerShell 完整流程

```powershell
# ========================================
# NPC 对话完整流程示例
# ========================================

$baseUrl = "http://192.168.5.189:8000"

# ----- 步骤 1: 创建角色 -----
Write-Host "步骤 1: 创建角色..." -ForegroundColor Cyan

$createResponse = Invoke-RestMethod -Uri "$baseUrl/api/persona/" -Method Post -Body @{
    planner_id = "demo"
    role_type = "blacksmith"
    name = "铁匠张"
    role = "铁匠铺老板"
    greeting = "哟，客人来啦！需要打造什么兵器？"
    traits = "豪爽,实在,技艺精湛"
    background = "祖传三代铁匠，打造的刀剑远近闻名"
}

$npcId = $createResponse.data.npc_id
Write-Host "✓ 角色创建成功！NPC ID: $npcId" -ForegroundColor Green

# ----- 步骤 2: 建立连接 -----
Write-Host "`n步骤 2: 建立对话连接..." -ForegroundColor Cyan

$connectBody = @{ persona = $npcId } | ConvertTo-Json
$connectResponse = Invoke-RestMethod -Uri "$baseUrl/connect/general/" `
    -Method Post -Body $connectBody -ContentType "application/json"

$sessionId = $connectResponse.session_id
Write-Host "✓ 连接成功！Session ID: $sessionId" -ForegroundColor Green
Write-Host "  角色问候: $($connectResponse.reply)" -ForegroundColor Yellow

# ----- 步骤 3: 进行多轮对话 -----
Write-Host "`n步骤 3: 进行对话..." -ForegroundColor Cyan

$messages = @(
    "老板，我想打一把好剑",
    "需要多少钱？多长时间？",
    "那就麻烦老板了"
)

foreach ($msg in $messages) {
    Write-Host "`n用户: $msg" -ForegroundColor White
    
    $chatBody = @{
        message = $msg
        session_id = $sessionId
        persona = $npcId
    } | ConvertTo-Json
    
    $chatResponse = Invoke-RestMethod -Uri "$baseUrl/chat/general/" `
        -Method Post -Body $chatBody -ContentType "application/json"
    
    Write-Host "AI: $($chatResponse.reply)" -ForegroundColor Green
    Write-Host "[响应时间: $($chatResponse.response_time)s]" -ForegroundColor DarkGray
}

# ----- 步骤 4: 清除历史（可选）-----
Write-Host "`n步骤 4: 清除对话历史..." -ForegroundColor Cyan

$clearBody = @{ session_id = $sessionId } | ConvertTo-Json
$clearResponse = Invoke-RestMethod -Uri "$baseUrl/clear/general/" `
    -Method Post -Body $clearBody -ContentType "application/json"

Write-Host "✓ $($clearResponse.reply)" -ForegroundColor Green

# ----- 步骤 5: 删除测试角色（可选）-----
Write-Host "`n步骤 5: 删除测试角色..." -ForegroundColor Cyan

$deleteResponse = Invoke-RestMethod -Uri "$baseUrl/api/persona/$npcId/" -Method Delete
Write-Host "✓ $($deleteResponse.data.message)" -ForegroundColor Green

Write-Host "`n========================================" -ForegroundColor Cyan
Write-Host "完整流程演示结束！" -ForegroundColor Cyan
```

### Python 完整流程

```python
import requests
import json

BASE_URL = "http://192.168.5.189:8000"

# 步骤 1: 创建角色
print("步骤 1: 创建角色...")
create_response = requests.post(
    f"{BASE_URL}/api/persona/",
    json={
        "planner_id": "demo",
        "role_type": "blacksmith",
        "personality": {
            "name": "铁匠张",
            "role": "铁匠铺老板",
            "greeting": "哟，客人来啦！需要打造什么兵器？",
            "traits": ["豪爽", "实在", "技艺精湛"],
            "background": "祖传三代铁匠，打造的刀剑远近闻名"
        }
    },
    headers={"Content-Type": "application/json; charset=utf-8"}
)
npc_id = create_response.json()["data"]["npc_id"]
print(f"✓ NPC ID: {npc_id}")

# 步骤 2: 建立连接
print("\n步骤 2: 建立连接...")
connect_response = requests.post(
    f"{BASE_URL}/connect/general/",
    json={"persona": npc_id},
    headers={"Content-Type": "application/json"}
)
data = connect_response.json()
session_id = data["session_id"]
print(f"✓ Session ID: {session_id}")
print(f"  角色问候: {data['reply']}")

# 步骤 3: 进行对话
print("\n步骤 3: 进行对话...")
messages = ["老板，我想打一把好剑", "需要多少钱？", "那就麻烦老板了"]

for msg in messages:
    print(f"\n用户: {msg}")
    chat_response = requests.post(
        f"{BASE_URL}/chat/general/",
        json={
            "message": msg,
            "session_id": session_id,
            "persona": npc_id
        },
        headers={"Content-Type": "application/json"}
    )
    reply = chat_response.json()
    print(f"AI: {reply['reply']}")
    print(f"[响应时间: {reply['response_time']}s]")

# 步骤 4: 清除历史
print("\n步骤 4: 清除历史...")
clear_response = requests.post(
    f"{BASE_URL}/clear/general/",
    json={"session_id": session_id},
    headers={"Content-Type": "application/json"}
)
print(f"✓ {clear_response.json()['reply']}")

# 步骤 5: 删除角色
print("\n步骤 5: 删除角色...")
delete_response = requests.delete(f"{BASE_URL}/api/persona/{npc_id}/")
print(f"✓ {delete_response.json()['data']['message']}")
```

---

## ❓ 常见问题

### Q1: 必须先创建角色才能对话吗？

**A:** 不是。如果不需要自定义角色，可以跳过创建步骤：
- 直接调用 `/connect/general/`（不传 persona），会使用默认助手角色
- 也可以使用已存在的 npc_id

### Q2: session_id 和 npc_id 的关系是什么？

**A:** 
- `npc_id` 标识**角色是谁**（持久化存储在 JSON 文件中）
- `session_id` 标识**对话会话**（用于保持对话上下文）
- 同一个 `npc_id` 可以有多个 `session_id`（多个用户同时与同一角色对话）

### Q3: 对话时必须同时传 session_id 和 persona 吗？

**A:** 
- `session_id` 是**必须的**，用于维护对话上下文
- `persona` 是**可选的**，如果不传会使用默认角色或 connect 时指定的角色

### Q4: 为什么 npc_id 前面有一串随机字符？

**A:** 这是**服务器前缀**（8位随机十六进制），用于确保全局唯一性：
- 不同策划可以创建相同 `role_type` 的角色而不冲突
- 同一策划也可以创建多个相同类型的角色

### Q5: 如何在多轮对话中保持上下文？

**A:** 使用**相同的 session_id**：
```powershell
# 第一轮
$response1 = Invoke-RestMethod ... -Body @{ message="我叫张三"; session_id=$sessionId; persona=$npcId }
# 第二轮 - AI 会记住你叫张三
$response2 = Invoke-RestMethod ... -Body @{ message="你还记得我叫什么吗？"; session_id=$sessionId; persona=$npcId }
```

---

## 📊 HTTP 状态码

| 状态码 | 含义 |
|--------|------|
| 200 | 成功 |
| 201 | 创建成功 |
| 400 | 请求参数错误 |
| 404 | 资源不存在 |
| 405 | 方法不允许 |
| 409 | 资源冲突（旧规范 npc_id 重复） |

---

## 🔗 相关文件

| 文件 | 说明 |
|------|------|
| `extrator/views.py` | Django 视图层，路由处理 |
| `extrator/llm/persona_api.py` | Persona API 核心实现 |
| `extrator/llm/chat_interface.py` | Chat 接口统一处理 |
| `extrator/llm/general_chat.py` | 通用对话模块 |
| `npc_configs/*.json` | 角色配置文件存储 |
| `general_chat.db` | 对话历史数据库 |
