# Persona API 传输格式说明

> ⚠️ **重要提示**: 本文档仅说明传输格式。完整的 API 使用流程（创建角色 → 建立连接 → 对话）请参阅 **[API_GUIDE.md](API_GUIDE.md)**

---

## � 标准流程概述

```
1. POST /api/persona/      → 创建角色，获取 npc_id
2. POST /connect/general/  → 建立连接，获取 session_id  
3. POST /chat/general/     → 使用 npc_id + session_id 进行对话
```

详细说明请查看 [API_GUIDE.md](API_GUIDE.md)

---

## 🎯 传输格式快速参考

### 创建角色（表单格式 - 推荐）

```powershell
$response = Invoke-RestMethod -Uri "http://192.168.5.189:8000/api/persona/" -Method Post -Body @{
    planner_id = "test01"
    role_type = "my_first_npc"
    name = "测试角色"
    role = "新手NPC"
    greeting = "你好，我是测试角色！"
}
$npcId = $response.data.npc_id  # 保存此 ID
```

### 建立连接

```powershell
$connectBody = @{ persona = $npcId } | ConvertTo-Json
$connectResponse = Invoke-RestMethod -Uri "http://192.168.5.189:8000/connect/general/" `
    -Method Post -Body $connectBody -ContentType "application/json"
$sessionId = $connectResponse.session_id  # 保存此 ID
```

### 进行对话

```powershell
Invoke-RestMethod -Uri "http://192.168.5.189:8000/chat/general/" -Method Post -Body @{
    message = "你好"
    persona = "a1b2c3d4_test01_my_first_npc"
}
```

✅ 完成！现在你已经掌握了基本用法。

---

## 📋 支持的传输格式

API 支持多种传输格式，**服务器会自动将扁平字段组装成 `personality` 对象**：

| 格式 | 优点 | 缺点 | 适用场景 |
|------|------|------|----------|
| **JSON** | 结构清晰，支持复杂数据 | 命令行使用较复杂 | 复杂数据、程序调用 |
| **表单** | 简单易用，命令行友好 | 不适合嵌套数据 | 简单数据、快速测试 |
| **URL参数** | 最简单，可直接在浏览器测试 | 数据量有限制 | 简单查询、快速测试 |

### 扁平字段自动组装示例

**你发送的（扁平格式）：**
```
planner_id=test01
role_type=npc01
name=测试NPC        ← 这些字段会自动组装到 personality
role=测试角色       ← 
greeting=你好！     ← 
traits=热情,友好    ← 逗号分隔会自动转为数组
```

**服务器自动转换为：**
```json
{
    "planner_id": "test01",
    "role_type": "npc01",
    "personality": {
        "name": "测试NPC",
        "role": "测试角色",
        "greeting": "你好！",
        "traits": ["热情", "友好"]
    }
}
```

---

## ⚠️ Windows PowerShell 重要提示

**PowerShell 与 curl.exe 配合使用时，双引号处理有 bug！**

### ❌ 不要这样用（双引号会丢失）
```powershell
# 这些命令在 PowerShell 中都会失败！
curl.exe -d "{\"key\": \"value\"}"           # 反斜杠转义 - 失败
curl.exe -d '{"key": "value"}'               # 单引号 - 双引号丢失
$body = '{"key": "value"}'; curl.exe -d $body  # 变量 - 双引号丢失
```

### ✅ 正确的做法

**方法 1：使用 Invoke-RestMethod（推荐）**
```powershell
Invoke-RestMethod -Uri "http://..." -Method Post -Body @{key="value"} -ContentType "application/json"
```

**方法 2：使用 --% 停止解析**
```powershell
curl.exe --% -X POST "http://..." -H "Content-Type: application/json" -d "{\"key\": \"value\"}"
```

**方法 3：使用表单格式（避免 JSON）**
```powershell
curl.exe -X POST "http://..." -d "planner_id=test01&name=测试"
```

---

## 📝 完整测试流程

### 流程 1：创建角色 → 查询 → 对话 → 更新 → 删除

#### 步骤 1：创建角色（使用表单格式）

**PowerShell：**
```powershell
$response = Invoke-RestMethod -Uri "http://192.168.5.189:8000/api/persona/" -Method Post -Body @{
    planner_id = "test01"
    role_type = "tavern_owner"
    name = "李老板"
    role = "酒馆老板"
    age = 45
    traits = "热情,健谈,八卦"
    background = "经营酒馆二十年，见过各路江湖人士"
    speech_style = "说话热情洋溢，喜欢打听消息"
    knowledge = "本地消息,江湖传闻,酒水美食"
    greeting = "客官里面请！小店有上好的女儿红！"
}

# 保存返回的 npc_id
$npcId = $response.data.npc_id
Write-Host "创建成功！NPC ID: $npcId" -ForegroundColor Green
```

**curl（Windows）：**
```bash
curl.exe -X POST "http://192.168.5.189:8000/api/persona/" ^
  -d "planner_id=test01" ^
  -d "role_type=tavern_owner" ^
  -d "name=李老板" ^
  -d "role=酒馆老板" ^
  -d "age=45" ^
  -d "traits=热情,健谈,八卦" ^
  -d "background=经营酒馆二十年" ^
  -d "greeting=客官里面请！"
```

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
            "age": 45,
            "traits": ["热情", "健谈", "八卦"],
            "background": "经营酒馆二十年，见过各路江湖人士",
            "speech_style": "说话热情洋溢，喜欢打听消息",
            "knowledge": ["本地消息", "江湖传闻", "酒水美食"],
            "greeting": "客官里面请！小店有上好的女儿红！"
        },
        "version": "1.0",
        "created_at": "2026-02-05T10:30:00.123456",
        "updated_at": "2026-02-05T10:30:00.123456"
    }
}
```

#### 步骤 2：查询所有角色

**PowerShell：**
```powershell
$allPersonas = Invoke-RestMethod -Uri "http://192.168.5.189:8000/api/persona/" -Method Get
Write-Host "共有 $($allPersonas.data.total) 个角色"
$allPersonas.data.personas | Format-Table npc_id, name, role
```

**curl：**
```bash
curl.exe http://192.168.5.189:8000/api/persona/
```

**响应示例：**
```json
{
    "success": true,
    "code": 200,
    "data": {
        "personas": [
            {
                "npc_id": "a1b2c3d4_test01_tavern_owner",
                "planner_id": "test01",
                "role_type": "tavern_owner",
                "name": "李老板",
                "role": "酒馆老板",
                "updated_at": "2026-02-05T10:30:00.123456"
            }
        ],
        "total": 1
    }
}
```

#### 步骤 3：查询特定角色详情

**PowerShell：**
```powershell
$persona = Invoke-RestMethod -Uri "http://192.168.5.189:8000/api/persona/$npcId/" -Method Get
$persona.data | ConvertTo-Json -Depth 10
```

**curl：**
```bash
curl.exe http://192.168.5.189:8000/api/persona/a1b2c3d4_test01_tavern_owner/
```

#### 步骤 4：使用角色进行对话

**PowerShell：**
```powershell
# 方式 1：建立连接时指定角色
$connectResponse = Invoke-RestMethod -Uri "http://192.168.5.189:8000/connect/general/" -Method Post -Body (@{persona=$npcId} | ConvertTo-Json) -ContentType "application/json"
$sessionId = $connectResponse.session_id
Write-Host "会话ID: $sessionId"
Write-Host "AI: $($connectResponse.reply)"

# 方式 2：对话时指定角色
$chatResponse = Invoke-RestMethod -Uri "http://192.168.5.189:8000/chat/general/" -Method Post -Body @{
    message = "老板，有什么好酒？"
    persona = $npcId
}
Write-Host "AI: $($chatResponse.reply)"
```

**curl：**
```bash
# 建立连接
curl.exe -X POST "http://192.168.5.189:8000/connect/general/" ^
  -H "Content-Type: application/json" ^
  -d "{\"persona\":\"a1b2c3d4_test01_tavern_owner\"}"

# 对话
curl.exe -X POST "http://192.168.5.189:8000/chat/general/" ^
  -H "Content-Type: application/json" ^
  -d "{\"message\":\"老板，有什么好酒？\",\"persona\":\"a1b2c3d4_test01_tavern_owner\"}"
```

**对话响应示例：**
```json
{
    "success": true,
    "reply": "哎呀客官好眼光！小店刚进了一批上好的女儿红，窖藏十年，香醇可口！还有杏花村的汾酒，也是极品！您要尝尝吗？",
    "session_id": "general#192.168.5.189#uuid-xxx",
    "response_time": 1.234,
    "backend": "general",
    "persona": "a1b2c3d4_test01_tavern_owner"
}
```

#### 步骤 5：更新角色信息

**PowerShell：**
```powershell
$updateResponse = Invoke-RestMethod -Uri "http://192.168.5.189:8000/api/persona/$npcId/" -Method Put -Body @{
    greeting = "欢迎光临！今天有新酿的桂花酒！"
    traits = "热情,健谈,八卦,精明"
}
Write-Host "更新成功！新问候语: $($updateResponse.data.personality.greeting)"
```

**curl：**
```bash
curl.exe -X PUT "http://192.168.5.189:8000/api/persona/a1b2c3d4_test01_tavern_owner/" ^
  -d "greeting=欢迎光临！今天有新酿的桂花酒！" ^
  -d "traits=热情,健谈,八卦,精明"
```

#### 步骤 6：删除角色（可选）

**PowerShell：**
```powershell
$deleteResponse = Invoke-RestMethod -Uri "http://192.168.5.189:8000/api/persona/$npcId/" -Method Delete
Write-Host $deleteResponse.data.message -ForegroundColor Yellow
```

**curl：**
```bash
curl.exe -X DELETE http://192.168.5.189:8000/api/persona/a1b2c3d4_test01_tavern_owner/
```

---

## 🎨 三种传输格式详解

### 格式 1：JSON 格式（适合复杂数据）

#### 特点：结构清晰，支持复杂嵌套数据

**PowerShell（推荐）：**
```powershell
$body = @{
    planner_id = "test01"
    role_type = "guard_npc"
    personality = @{
        name = "张守卫"
        role = "城门守卫"
        age = 35
        traits = @("警惕", "尽责", "严肃")
        background = "在城门站岗十年"
        speech_style = "说话简短有力"
        knowledge = @("城内治安", "进出规定")
        greeting = "站住！进城请出示通行证！"
    }
} | ConvertTo-Json -Depth 10

Invoke-RestMethod -Uri "http://192.168.5.189:8000/api/persona/" -Method Post -Body $body -ContentType "application/json; charset=utf-8"
```

**curl（Windows）：**
```bash
curl.exe --% -X POST "http://192.168.5.189:8000/api/persona/" -H "Content-Type: application/json" -d "{\"planner_id\":\"test01\",\"role_type\":\"guard_npc\",\"personality\":{\"name\":\"张守卫\",\"role\":\"城门守卫\",\"greeting\":\"站住！\"}}"
```

**Python：**
```python
import requests

data = {
    "planner_id": "test01",
    "role_type": "guard_npc",
    "personality": {
        "name": "张守卫",
        "role": "城门守卫",
        "age": 35,
        "traits": ["警惕", "尽责", "严肃"],
        "greeting": "站住！进城请出示通行证！"
    }
}

response = requests.post(
    "http://192.168.5.189:8000/api/persona/",
    json=data,
    headers={"Content-Type": "application/json; charset=utf-8"}
)
print(response.json())
```

---

### 格式 2：表单格式（适合简单数据，命令行友好）✨

#### 特点：扁平字段自动组装，无需手动构建 personality 对象

**PowerShell（最简单）：**
```powershell
Invoke-RestMethod -Uri "http://192.168.5.189:8000/api/persona/" -Method Post -Body @{
    planner_id = "test01"
    role_type = "merchant_npc"
    name = "王商人"
    role = "杂货店老板"
    age = 40
    traits = "精明,健谈,贪财"  # 逗号分隔自动转数组
    background = "经营杂货店多年"
    greeting = "客官要买点什么？"
}
```

**curl（Windows）：**
```bash
curl.exe -X POST "http://192.168.5.189:8000/api/persona/" ^
  -d "planner_id=test01" ^
  -d "role_type=merchant_npc" ^
  -d "name=王商人" ^
  -d "role=杂货店老板" ^
  -d "age=40" ^
  -d "traits=精明,健谈,贪财" ^
  -d "greeting=客官要买点什么？"
```

**curl（Linux/Mac）：**
```bash
curl -X POST "http://192.168.5.189:8000/api/persona/" \
  -d "planner_id=test01&role_type=merchant_npc&name=王商人&role=杂货店老板&age=40&traits=精明,健谈,贪财&greeting=客官要买点什么？"
```

---

### 格式 3：URL 参数格式（最快速测试）✨✨

#### 特点：可直接在浏览器地址栏测试（GET），或用于简单 POST

**PowerShell：**
```powershell
$url = "http://192.168.5.189:8000/api/persona/?planner_id=test01&role_type=simple_npc&name=简单NPC&role=测试角色&greeting=你好"
Invoke-RestMethod -Uri $url -Method Post
```

**curl：**
```bash
curl.exe -X POST "http://192.168.5.189:8000/api/persona/?planner_id=test01&role_type=simple_npc&name=简单NPC&role=测试角色&greeting=你好"
```

**浏览器（仅查询）：**
```
http://192.168.5.189:8000/api/persona/
```

---

## 🧪 完整测试脚本

### PowerShell 自动化测试脚本

创建 `test_persona_api.ps1` 文件：

```powershell
# Persona API 完整测试脚本
# 测试创建、查询、更新、对话、删除功能

$baseUrl = "http://192.168.5.189:8000"
$plannerId = "test_script"
$roleType = "test_npc_$(Get-Date -Format 'HHmmss')"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Persona API 完整测试" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

# 步骤 1: 创建角色
Write-Host "`n[步骤 1] 创建测试角色..." -ForegroundColor Green
try {
    $createResponse = Invoke-RestMethod -Uri "$baseUrl/api/persona/" -Method Post -Body @{
        planner_id = $plannerId
        role_type = $roleType
        name = "测试NPC"
        role = "测试角色"
        age = 25
        traits = "友好,热情,乐于助人"
        background = "这是一个用于测试的NPC角色"
        speech_style = "说话温和友善"
        knowledge = "测试知识,API使用"
        greeting = "你好！我是测试NPC！"
    }
    
    $npcId = $createResponse.data.npc_id
    Write-Host "✓ 创建成功！" -ForegroundColor Green
    Write-Host "  NPC ID: $npcId" -ForegroundColor Yellow
    Write-Host "  名称: $($createResponse.data.personality.name)" -ForegroundColor Yellow
} catch {
    Write-Host "✗ 创建失败: $_" -ForegroundColor Red
    exit 1
}

# 步骤 2: 查询所有角色
Write-Host "`n[步骤 2] 查询所有角色..." -ForegroundColor Green
try {
    $listResponse = Invoke-RestMethod -Uri "$baseUrl/api/persona/" -Method Get
    Write-Host "✓ 查询成功！共有 $($listResponse.data.total) 个角色" -ForegroundColor Green
    
    # 显示前3个角色
    $listResponse.data.personas | Select-Object -First 3 | ForEach-Object {
        Write-Host "  - $($_.npc_id): $($_.name) ($($_.role))" -ForegroundColor Yellow
    }
} catch {
    Write-Host "✗ 查询失败: $_" -ForegroundColor Red
}

# 步骤 3: 查询特定角色详情
Write-Host "`n[步骤 3] 查询角色详情..." -ForegroundColor Green
try {
    $getResponse = Invoke-RestMethod -Uri "$baseUrl/api/persona/$npcId/" -Method Get
    Write-Host "✓ 查询成功！" -ForegroundColor Green
    Write-Host "  名称: $($getResponse.data.personality.name)" -ForegroundColor Yellow
    Write-Host "  角色: $($getResponse.data.personality.role)" -ForegroundColor Yellow
    Write-Host "  问候语: $($getResponse.data.personality.greeting)" -ForegroundColor Yellow
} catch {
    Write-Host "✗ 查询失败: $_" -ForegroundColor Red
}

# 步骤 4: 建立对话连接
Write-Host "`n[步骤 4] 建立对话连接..." -ForegroundColor Green
try {
    $connectBody = @{persona = $npcId} | ConvertTo-Json
    $connectResponse = Invoke-RestMethod -Uri "$baseUrl/connect/general/" -Method Post -Body $connectBody -ContentType "application/json"
    $sessionId = $connectResponse.session_id
    Write-Host "✓ 连接成功！" -ForegroundColor Green
    Write-Host "  会话ID: $sessionId" -ForegroundColor Yellow
    Write-Host "  AI回复: $($connectResponse.reply)" -ForegroundColor Cyan
} catch {
    Write-Host "✗ 连接失败: $_" -ForegroundColor Red
}

# 步骤 5: 进行对话
Write-Host "`n[步骤 5] 进行对话测试..." -ForegroundColor Green
try {
    $chatBody = @{
        message = "你能帮我做什么？"
        persona = $npcId
        session_id = $sessionId
    } | ConvertTo-Json
    
    $chatResponse = Invoke-RestMethod -Uri "$baseUrl/chat/general/" -Method Post -Body $chatBody -ContentType "application/json"
    Write-Host "✓ 对话成功！" -ForegroundColor Green
    Write-Host "  用户: 你能帮我做什么？" -ForegroundColor White
    Write-Host "  AI: $($chatResponse.reply)" -ForegroundColor Cyan
    Write-Host "  响应时间: $($chatResponse.response_time)秒" -ForegroundColor Yellow
} catch {
    Write-Host "✗ 对话失败: $_" -ForegroundColor Red
}

# 步骤 6: 更新角色
Write-Host "`n[步骤 6] 更新角色信息..." -ForegroundColor Green
try {
    $updateResponse = Invoke-RestMethod -Uri "$baseUrl/api/persona/$npcId/" -Method Put -Body @{
        greeting = "你好！我是更新后的测试NPC！"
        traits = "友好,热情,乐于助人,专业"
    }
    Write-Host "✓ 更新成功！" -ForegroundColor Green
    Write-Host "  新问候语: $($updateResponse.data.personality.greeting)" -ForegroundColor Yellow
    Write-Host "  新特征: $($updateResponse.data.personality.traits -join ', ')" -ForegroundColor Yellow
} catch {
    Write-Host "✗ 更新失败: $_" -ForegroundColor Red
}

# 步骤 7: 删除角色
Write-Host "`n[步骤 7] 删除测试角色..." -ForegroundColor Green
try {
    $deleteResponse = Invoke-RestMethod -Uri "$baseUrl/api/persona/$npcId/" -Method Delete
    Write-Host "✓ 删除成功！" -ForegroundColor Green
    Write-Host "  $($deleteResponse.data.message)" -ForegroundColor Yellow
} catch {
    Write-Host "✗ 删除失败: $_" -ForegroundColor Red
}

Write-Host "`n========================================" -ForegroundColor Cyan
Write-Host "测试完成！" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
```

**运行测试：**
```powershell
.\test_persona_api.ps1
```

**预期输出：**
```
========================================
Persona API 完整测试
========================================

[步骤 1] 创建测试角色...
✓ 创建成功！
  NPC ID: a1b2c3d4_test_script_test_npc_143022
  名称: 测试NPC

[步骤 2] 查询所有角色...
✓ 查询成功！共有 5 个角色
  - a1b2c3d4_test_script_test_npc_143022: 测试NPC (测试角色)
  - ...

[步骤 3] 查询角色详情...
✓ 查询成功！
  名称: 测试NPC
  角色: 测试角色
  问候语: 你好！我是测试NPC！

[步骤 4] 建立对话连接...
✓ 连接成功！
  会话ID: general#192.168.5.189#uuid-xxx
  AI回复: 你好！我是测试NPC！

[步骤 5] 进行对话测试...
✓ 对话成功！
  用户: 你能帮我做什么？
  AI: 我可以帮助你测试API功能，回答你的问题...
  响应时间: 1.234秒

[步骤 6] 更新角色信息...
✓ 更新成功！
  新问候语: 你好！我是更新后的测试NPC！
  新特征: 友好, 热情, 乐于助人, 专业

[步骤 7] 删除测试角色...
✓ 删除成功！
  人设 'a1b2c3d4_test_script_test_npc_143022' 已删除

========================================
测试完成！
========================================
```

---

## 🔧 常用操作速查

### 创建角色（表单格式 - 最简单）

```powershell
Invoke-RestMethod -Uri "http://192.168.5.189:8000/api/persona/" -Method Post -Body @{
    planner_id = "你的工号"
    role_type = "角色类型"
    name = "角色名称"
    role = "角色职业"
    greeting = "问候语"
}
```

### 查询所有角色

```powershell
Invoke-RestMethod -Uri "http://192.168.5.189:8000/api/persona/"
```

### 查询特定角色

```powershell
Invoke-RestMethod -Uri "http://192.168.5.189:8000/api/persona/你的npc_id/"
```

### 更新角色

```powershell
Invoke-RestMethod -Uri "http://192.168.5.189:8000/api/persona/你的npc_id/" -Method Put -Body @{
    greeting = "新的问候语"
}
```

### 删除角色

```powershell
Invoke-RestMethod -Uri "http://192.168.5.189:8000/api/persona/你的npc_id/" -Method Delete
```

### 使用角色对话

```powershell
Invoke-RestMethod -Uri "http://192.168.5.189:8000/chat/general/" -Method Post -Body @{
    message = "你的消息"
    persona = "你的npc_id"
}
```

---

## 📊 字段说明

### 必填字段

| 字段 | 类型 | 说明 | 示例 |
|------|------|------|------|
| planner_id | string | 策划ID/工号（2-20字符） | `"test01"`, `"user001"` |
| role_type | string | 角色类型（2-50字符） | `"tavern_owner"`, `"city_guard"` |
| name | string | 角色名称（≤50字符） | `"李老板"`, `"张守卫"` |
| role | string | 角色职业（≤100字符） | `"酒馆老板"`, `"城门守卫"` |

### 可选字段

| 字段 | 类型 | 说明 | 示例 |
|------|------|------|------|
| age | number | 年龄 | `45` |
| gender | string | 性别 | `"男"`, `"女"` |
| traits | string/array | 性格特点（逗号分隔或数组，≤20项） | `"热情,健谈"` 或 `["热情", "健谈"]` |
| background | string | 背景故事（≤1000字符） | `"经营酒馆二十年..."` |
| speech_style | string | 说话风格（≤500字符） | `"说话热情洋溢"` |
| knowledge | string/array | 擅长领域（逗号分隔或数组，≤20项） | `"本地消息,江湖传闻"` |
| greeting | string | 问候语 | `"客官里面请！"` |

### 自动生成字段

| 字段 | 说明 | 示例 |
|------|------|------|
| npc_id | 系统自动生成的唯一ID | `"a1b2c3d4_test01_tavern_owner"` |
| created_at | 创建时间 | `"2026-02-05T10:30:00.123456"` |
| updated_at | 更新时间 | `"2026-02-05T10:30:00.123456"` |
| version | 版本号（默认"1.0"） | `"1.0"` |

---

## ❓ 常见问题

### Q1: 为什么返回的 npc_id 和我指定的不一样？

**A:** 使用新规范（`planner_id` + `role_type`）时，系统会自动生成带随机前缀的 `npc_id`，格式为：`{随机前缀}_{planner_id}_{role_type}`

**示例：**
```
你提供: planner_id=test01, role_type=tavern_owner
系统生成: npc_id=a1b2c3d4_test01_tavern_owner
```

**原因：** 防止不同策划创建相同角色类型时发生冲突，确保每个角色都有唯一ID。

### Q2: 如何保存和使用 npc_id？

**A:** 创建角色后，从响应中获取 `npc_id` 并保存：

```powershell
$response = Invoke-RestMethod -Uri "..." -Method Post -Body @{...}
$npcId = $response.data.npc_id  # 保存此ID
Write-Host "NPC ID: $npcId"

# 后续使用
Invoke-RestMethod -Uri "http://192.168.5.189:8000/api/persona/$npcId/"
```

### Q3: 数组字段如何传递？

**A:** 支持两种方式：

**方式 1：逗号分隔字符串（表单格式）**
```powershell
-Body @{ traits = "热情,健谈,八卦" }
```

**方式 2：数组（JSON格式）**
```powershell
-Body (@{ traits = @("热情", "健谈", "八卦") } | ConvertTo-Json)
```

### Q4: 中文字符显示乱码怎么办？

**A:** 确保使用 UTF-8 编码：

**PowerShell（推荐）：**
```powershell
# PowerShell 自动处理编码，无需额外设置
Invoke-RestMethod -Uri "..." -Method Post -Body @{name="中文名称"}
```

**curl：**
```bash
# 使用 --% 停止解析，或使用表单格式
curl.exe -X POST "..." -d "name=中文名称"
```

### Q5: 如何查询我创建的所有角色？

**A:** 查询所有角色后，根据 `planner_id` 筛选：

```powershell
$all = Invoke-RestMethod -Uri "http://192.168.5.189:8000/api/persona/"
$myPersonas = $all.data.personas | Where-Object { $_.planner_id -eq "test01" }
$myPersonas | Format-Table npc_id, name, role
```

### Q6: 更新角色时会覆盖其他字段吗？

**A:** 不会。更新是**合并操作**，只更新你提供的字段：

```powershell
# 只更新 greeting，其他字段保持不变
Invoke-RestMethod -Uri "http://192.168.5.189:8000/api/persona/$npcId/" -Method Put -Body @{
    greeting = "新的问候语"
}
```

### Q7: 删除角色后能恢复吗？

**A:** 不能。删除操作会直接删除配置文件，无法恢复。建议删除前先备份重要角色。

### Q8: 同一个角色类型能创建多次吗？

**A:** 可以！由于系统会自动生成随机前缀，每次创建都会生成不同的 `npc_id`：

```
第一次: a1b2c3d4_test01_tavern_owner
第二次: e5f6g7h8_test01_tavern_owner
第三次: i9j0k1l2_test01_tavern_owner
```

### Q9: 如何在对话中切换角色？

**A:** 在对话请求中指定不同的 `persona` 参数：

```powershell
# 使用角色A对话
Invoke-RestMethod -Uri "http://192.168.5.189:8000/chat/general/" -Method Post -Body @{
    message = "你好"
    persona = "npc_id_A"
}

# 切换到角色B
Invoke-RestMethod -Uri "http://192.168.5.189:8000/chat/general/" -Method Post -Body @{
    message = "你好"
    persona = "npc_id_B"
}
```

### Q10: 如何测试 API 是否正常工作？

**A:** 运行快速测试：

```powershell
# 1. 查询所有角色（应该返回列表）
Invoke-RestMethod -Uri "http://192.168.5.189:8000/api/persona/"

# 2. 创建测试角色
$response = Invoke-RestMethod -Uri "http://192.168.5.189:8000/api/persona/" -Method Post -Body @{
    planner_id = "test"
    role_type = "test_npc"
    name = "测试"
    role = "测试"
}

# 3. 删除测试角色
$npcId = $response.data.npc_id
Invoke-RestMethod -Uri "http://192.168.5.189:8000/api/persona/$npcId/" -Method Delete
```

---

## 🚨 错误处理

### 常见错误码

| 状态码 | 含义 | 原因 | 解决方法 |
|--------|------|------|----------|
| 200 | 成功 | 操作成功 | - |
| 201 | 创建成功 | 角色创建成功 | - |
| 400 | 请求错误 | 参数格式错误、缺少必填字段 | 检查请求参数 |
| 404 | 未找到 | 角色不存在 | 检查 npc_id 是否正确 |
| 409 | 冲突 | 角色已存在（旧规范） | 使用 PUT 更新或换个 ID |

### 错误响应示例

**400 - 参数错误：**
```json
{
    "success": false,
    "code": 400,
    "error": "策划ID格式错误: 仅含字母/数字/下划线，2-20字符"
}
```

**404 - 角色不存在：**
```json
{
    "success": false,
    "code": 404,
    "error": "人设 'xxx' 不存在"
}
```

### 调试技巧

**1. 查看详细错误信息：**
```powershell
try {
    $response = Invoke-RestMethod -Uri "..." -Method Post -Body @{...}
} catch {
    Write-Host "错误: $($_.Exception.Message)" -ForegroundColor Red
    Write-Host "响应: $($_.ErrorDetails.Message)" -ForegroundColor Yellow
}
```

**2. 测试连接：**
```powershell
# 测试服务器是否可访问
Test-NetConnection -ComputerName 192.168.5.189 -Port 8000
```

**3. 验证 JSON 格式：**
```powershell
$json = @{...} | ConvertTo-Json -Depth 10
Write-Host $json  # 检查生成的 JSON 是否正确
```

---

## 💡 最佳实践

### 1. 使用有意义的命名

**✅ 推荐：**
```powershell
planner_id = "user001"           # 工号或用户ID
role_type = "tavern_owner_cn"    # 描述性的角色类型
name = "李老板"                   # 有意义的名称
```

**❌ 不推荐：**
```powershell
planner_id = "abc123"            # 无意义的随机字符
role_type = "npc1"               # 不明确
name = "测试"                     # 太泛化
```

### 2. 保存 npc_id 到配置文件

```powershell
# 创建角色并保存ID
$response = Invoke-RestMethod -Uri "..." -Method Post -Body @{...}
$npcId = $response.data.npc_id

# 保存到配置文件
@{
    npc_id = $npcId
    planner_id = "test01"
    role_type = "tavern_owner"
    created_at = Get-Date
} | ConvertTo-Json | Out-File "my_npcs.json"
```

### 3. 批量管理角色

```powershell
# 创建多个角色
$roles = @(
    @{role_type="guard"; name="张守卫"; role="城门守卫"},
    @{role_type="merchant"; name="王商人"; role="商人"},
    @{role_type="blacksmith"; name="铁匠"; role="铁匠"}
)

$npcIds = @()
foreach ($r in $roles) {
    $response = Invoke-RestMethod -Uri "http://192.168.5.189:8000/api/persona/" -Method Post -Body @{
        planner_id = "test01"
        role_type = $r.role_type
        name = $r.name
        role = $r.role
        greeting = "你好！"
    }
    $npcIds += $response.data.npc_id
}

Write-Host "创建了 $($npcIds.Count) 个角色"
```

### 4. 定期备份配置

```powershell
# 导出所有角色配置
$all = Invoke-RestMethod -Uri "http://192.168.5.189:8000/api/persona/"
$all.data.personas | ConvertTo-Json -Depth 10 | Out-File "backup_$(Get-Date -Format 'yyyyMMdd').json"
```

### 5. 使用环境变量

```powershell
# 设置环境变量
$env:PERSONA_API_URL = "http://192.168.5.189:8000"
$env:PLANNER_ID = "test01"

# 使用环境变量
Invoke-RestMethod -Uri "$env:PERSONA_API_URL/api/persona/" -Method Post -Body @{
    planner_id = $env:PLANNER_ID
    role_type = "test_npc"
    name = "测试"
    role = "测试"
}
```

### 6. 错误处理和重试

```powershell
function Create-Persona {
    param($Data, $MaxRetries = 3)
    
    for ($i = 0; $i -lt $MaxRetries; $i++) {
        try {
            $response = Invoke-RestMethod -Uri "http://192.168.5.189:8000/api/persona/" -Method Post -Body $Data
            return $response
        } catch {
            Write-Host "尝试 $($i+1) 失败: $_" -ForegroundColor Yellow
            if ($i -eq $MaxRetries - 1) {
                throw "创建失败，已重试 $MaxRetries 次"
            }
            Start-Sleep -Seconds 2
        }
    }
}

# 使用
$response = Create-Persona -Data @{
    planner_id = "test01"
    role_type = "test_npc"
    name = "测试"
    role = "测试"
}
```

---

## 🔗 相关文档

- **[PERSONA_API.md](PERSONA_API.md)** - API 接口详细文档
  - 数据结构说明
  - 字段验证规则
  - HTTP 状态码
  - Java/Python 客户端示例
  
- **[README.md](README.md)** - 项目整体说明
  - 项目架构
  - 安装部署
  - 配置说明

- **[npc_configs/](npc_configs/)** - 角色配置文件目录
  - 查看现有角色配置
  - 配置文件格式示例

---

## 📞 技术支持

### 遇到问题？

1. **查看文档：** 先查阅本文档和 `PERSONA_API.md`
2. **运行测试脚本：** 使用上面提供的测试脚本验证功能
3. **检查日志：** 查看服务器日志获取详细错误信息
4. **联系开发团队：** 提供详细的错误信息和复现步骤

### 反馈建议

如果您有任何建议或发现问题，欢迎反馈：
- 文档不清楚的地方
- 缺少的功能
- 使用中遇到的困难

---

## 📝 更新日志

### 2026-02-05
- ✅ 添加完整测试流程说明
- ✅ 添加 PowerShell 自动化测试脚本
- ✅ 添加常用操作速查表
- ✅ 添加详细的字段说明
- ✅ 添加常见问题解答（10个问题）
- ✅ 添加错误处理和调试技巧
- ✅ 添加最佳实践指南
- ✅ 完善三种传输格式的使用示例
- ✅ 添加对话集成测试示例

### 原有内容
- 支持多种传输格式（JSON、表单、URL参数）
- 扁平字段自动组装功能
- Windows PowerShell 使用注意事项

---

## 🎉 总结

本文档提供了 Persona API 的完整测试指南，包括：

✅ **快速开始** - 3分钟上手创建第一个角色  
✅ **完整流程** - 从创建到删除的完整测试流程  
✅ **三种格式** - JSON、表单、URL参数的详细使用方法  
✅ **自动化测试** - PowerShell 测试脚本，一键运行  
✅ **速查表** - 常用操作命令，复制即用  
✅ **字段说明** - 所有字段的详细说明和示例  
✅ **常见问题** - 10个常见问题的解答  
✅ **错误处理** - 错误码说明和调试技巧  
✅ **最佳实践** - 6个实用的最佳实践建议  

**选择最适合您的方式开始使用吧！** 🚀

---

**提示：** 如需了解更多技术细节，请参阅 [PERSONA_API.md](PERSONA_API.md) 文档。
