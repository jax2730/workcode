# Persona API 接口文档

> 📌 **完整对话流程指南**: 请参阅 **[API_GUIDE.md](API_GUIDE.md)**，了解 `创建角色 → 建立连接 → 对话` 的完整流程。

本文档仅描述 Persona API 的接口细节。

---

## 📋 快速开始

### 标准流程

```
1. POST /api/persona/      → 创建角色，获取 npc_id
2. POST /connect/general/  → 建立连接，获取 session_id（传入 npc_id）
3. POST /chat/general/     → 对话（传入 session_id + npc_id + message）
```

### 创建角色示例

**PowerShell (表单格式):**
```powershell
$response = Invoke-RestMethod -Uri "http://192.168.5.189:8000/api/persona/" -Method Post -Body @{
    planner_id = "1e23"
    role_type = "tavern_owner_cn"
    name = "李老板"
    role = "酒馆老板"
    greeting = "客官里面请！"
}
# 保存返回的 npc_id
$npcId = $response.data.npc_id
```

**响应:**
```json
{
    "success": true,
    "code": 201,
    "data": {
        "npc_id": "a1b2c3d4_1e23_tavern_owner_cn"
    }
}
```

> ⚠️ **注意**: `npc_id` 包含随机前缀，格式为 `{随机前缀}_{planner_id}_{role_type}`

### 建立连接并对话

```powershell
# 建立连接
$connectBody = @{ persona = $npcId } | ConvertTo-Json
$connectResponse = Invoke-RestMethod -Uri "http://192.168.5.189:8000/connect/general/" `
    -Method Post -Body $connectBody -ContentType "application/json"
$sessionId = $connectResponse.session_id

# 对话
$chatBody = @{
    message = "老板，有什么好酒？"
    session_id = $sessionId
    persona = $npcId
} | ConvertTo-Json
Invoke-RestMethod -Uri "http://192.168.5.189:8000/chat/general/" `
    -Method Post -Body $chatBody -ContentType "application/json"
```

---

## 概述

Persona API 提供 RESTful 接口，用于管理 NPC 人设配置。客户端可通过此 API 创建、查询、更新和删除人设。

**Base URL:** `http://<server>:8000/api/persona/`

**配置存储:** 所有人设配置保存在 `npc_configs/` 目录下，格式为 `<npc_id>.json`

### ⚠️ 重要更新：防止 ID 冲突的新规范

**问题：** 多个策划创建相同的 `npc_id`（如 `tavern_owner_cn`）会导致互相覆盖，造成数据丢失。

**解决方案：** 使用 `planner_id` + `role_type` 自动生成唯一的 `npc_id`（格式：`{server_prefix}_{planner_id}_{role_type}`）

**示例：**
- 策划 `1e23` 创建 `tavern_owner_cn` → 生成 `npc_id = a1b2c3d4_1e23_tavern_owner_cn`
- 策划 `2f45` 创建 `tavern_owner_cn` → 生成 `npc_id = e5f6g7h8_2f45_tavern_owner_cn`
- 策划 `1e23` 再次创建 `tavern_owner_cn` → 生成 `npc_id = i9j0k1l2_1e23_tavern_owner_cn`
- ✅ 不同策划可以创建相同角色类型，不会冲突
- ✅ 同一策划可以创建多个相同角色类型（每次生成不同的随机前缀）
- ✅ 服务器前缀确保全局唯一性

### 🎯 支持多种传输格式

API 支持多种传输格式，服务器端统一保存为 JSON 文件：

| 格式 | 适用场景 | 示例 |
|------|----------|------|
| **JSON** | 复杂数据、程序调用 | `{"planner_id":"1e23","personality":{...}}` |
| **表单** | 简单数据、命令行测试 | `planner_id=1e23&name=李老板` |
| **URL参数** | 快速测试、浏览器 | `?planner_id=1e23&name=李老板` |

详细说明请参考 [多种格式使用指南](PERSONA_API_FORMATS.md)

---

## 接口列表

| 方法 | 端点 | 描述 |
|------|------|------|
| GET | `/api/persona/` | 列出所有人设 |
| GET | `/api/persona/<npc_id>/` | 获取指定人设详情 |
| POST | `/api/persona/` | 创建新人设 |
| PUT | `/api/persona/<npc_id>/` | 更新指定人设 |
| DELETE | `/api/persona/<npc_id>/` | 删除指定人设 |

---

## 通用说明

### 请求头
```
Content-Type: application/json
```

### 响应格式
所有接口返回统一的 JSON 格式：

**成功响应:**
```json
{
    "success": true,
    "code": 200,
    "data": { ... }
}
```

**错误响应:**
```json
{
    "success": false,
    "code": 400,
    "error": "错误描述"
}
```

### HTTP 状态码
| 状态码 | 含义 |
|--------|------|
| 200 | 成功 |
| 201 | 创建成功 |
| 400 | 请求参数错误 |
| 404 | 资源不存在 |
| 405 | 方法不允许 |
| 409 | 资源冲突（已存在） |

---

## 1. 列出所有人设

**GET** `/api/persona/`

### 请求示例
```bash
curl -X GET http://192.168.5.189:8000/api/persona/
```

### 响应示例
```json
{
    "success": true,
    "code": 200,
    "data": {
        "personas": [
            {
                "npc_id": "1e23_blacksmith",
                "planner_id": "1e23",
                "role_type": "blacksmith",
                "name": "老铁匠",
                "role": "铁匠铺老板",
                "updated_at": "2026-01-30T10:30:00"
            },
            {
                "npc_id": "2f45_merchant",
                "planner_id": "2f45",
                "role_type": "merchant",
                "name": "精明商人",
                "role": "杂货店老板",
                "updated_at": "2026-01-30T10:30:00"
            }
        ],
        "total": 2
    }
}
```

**说明：** 如果人设包含 `planner_id` 和 `role_type` 字段，会在列表中显示。旧版本创建的人设可能不包含这些字段。

---

## 2. 获取指定人设

**GET** `/api/persona/<npc_id>/`

### 请求示例
```bash
curl -X GET http://192.168.5.189:8000/api/persona/1e23_blacksmith/
```

**说明：** 使用完整的 `npc_id`（如 `1e23_blacksmith`）来获取人设详情。

### 响应示例
```json
{
    "success": true,
    "code": 200,
    "data": {
        "npc_id": "1e23_blacksmith",
        "planner_id": "1e23",
        "role_type": "blacksmith",
        "personality": {
            "name": "老铁匠",
            "role": "铁匠铺老板",
            "age": 55,
            "gender": "男",
            "traits": ["严肃", "专业", "热心", "固执"],
            "background": "在这个镇上打铁30年，见证了无数冒险者的成长...",
            "speech_style": "说话干脆利落，常用专业术语",
            "knowledge": ["武器锻造", "金属材料", "镇上历史"],
            "secrets": ["年轻时曾是著名的冒险者'铁臂'"],
            "greeting": "哟，又来了个冒险者。看你的样子，是来买武器的吧？"
        },
        "version": "1.0",
        "created_at": "2026-01-30T10:00:00",
        "updated_at": "2026-01-30T10:30:00"
    }
}
```

**说明：** 使用新规范创建的人设会包含 `planner_id` 和 `role_type` 字段。

---

## 3. 创建新人设

**POST** `/api/persona/`

### ⚠️ 重要更新：防止冲突的新规范

为了避免多个策划创建相同 `npc_id` 导致冲突，现在支持两种创建方式：

#### 方式一：推荐使用（自动生成唯一ID）

| 字段 | 类型 | 必填 | 描述 |
|------|------|------|------|
| planner_id | string | ✅ | 策划ID（工号/用户ID），2-20字符，仅含字母数字 |
| role_type | string | ✅ | 角色类型（如 `tavern_owner_cn`），2-50字符 |
| personality | object | ✅ | 性格配置对象 |
| version | string | ❌ | 版本号，默认 "1.0" |

**系统会自动生成唯一的 `npc_id`**：格式为 `{server_prefix}_{planner_id}_{role_type}`

其中 `server_prefix` 是服务器生成的8位随机十六进制字符，确保全局唯一性。

**优点：**
- ✅ 自动避免不同策划之间的ID冲突
- ✅ 同一策划可以创建多个相同角色类型
- ✅ 服务器前缀确保全局唯一性
- ✅ 返回生成的 `npc_id` 供后续使用
- ✅ 可追溯角色创建者

#### 方式二：兼容旧版本（手动指定ID）

| 字段 | 类型 | 必填 | 描述 |
|------|------|------|------|
| npc_id | string | ✅ | 人设ID，唯一标识（需自行保证唯一性） |
| personality | object | ✅ | 性格配置对象 |
| version | string | ❌ | 版本号，默认 "1.0" |

**personality 对象:**

| 字段 | 类型 | 必填 | 限制 | 描述 |
|------|------|------|------|------|
| name | string | ✅ | ≤50字符 | 人设名称 |
| role | string | ✅ | ≤100字符 | 角色/职业 |
| age | number | ❌ | - | 年龄 |
| gender | string | ❌ | - | 性别 |
| traits | array | ❌ | ≤20项 | 性格特点列表 |
| background | string | ❌ | ≤1000字符 | 背景故事 |
| speech_style | string | ❌ | ≤500字符 | 说话风格 |
| knowledge | array | ❌ | ≤20项 | 擅长领域列表 |
| secrets | array | ❌ | - | 秘密信息列表 |
| greeting | string | ❌ | - | 问候语 |

**ID 格式要求:**
- **planner_id**: 仅含字母、数字，2-20字符（如：`1e23`, `user001`, `planner42`）
- **role_type**: 以字母开头，仅含字母、数字、下划线，2-50字符（如：`tavern_owner_cn`, `city_guard`）
- **server_prefix**: 服务器自动生成的8位随机十六进制字符（如：`a1b2c3d4`）
- **生成的 npc_id**: 格式为 `{server_prefix}_{planner_id}_{role_type}`（如：`a1b2c3d4_1e23_tavern_owner_cn`）
- 不能使用保留词: `admin`, `system`, `default`, `api`, `test`, `null`, `undefined`

### 请求示例

#### 示例 1：使用新规范（推荐）

**使用 curl (Linux/Mac):**
```bash
curl -X POST http://192.168.5.189:8000/api/persona/ \
  -H "Content-Type: application/json; charset=utf-8" \
  -d '{
    "planner_id": "1e23",
    "role_type": "tavern_owner_cn",
    "personality": {
        "name": "李老板",
        "role": "酒馆老板",
        "age": 45,
        "gender": "男",
        "traits": ["热情", "健谈", "八卦"],
        "background": "经营这家酒馆已有二十年，见过各路江湖人士",
        "speech_style": "说话热情洋溢，喜欢打听消息",
        "knowledge": ["本地消息", "江湖传闻", "酒水美食"],
        "secrets": ["知道镇上很多秘密"],
        "greeting": "客官里面请！小店有上好的女儿红！"
    }
}'
```

**使用 PowerShell (Windows):**
```powershell
$body = @{
    planner_id = "1e23"
    role_type = "tavern_owner_cn"
    personality = @{
        name = "李老板"
        role = "酒馆老板"
        age = 45
        gender = "男"
        traits = @("热情", "健谈", "八卦")
        background = "经营这家酒馆已有二十年，见过各路江湖人士"
        speech_style = "说话热情洋溢，喜欢打听消息"
        knowledge = @("本地消息", "江湖传闻", "酒水美食")
        secrets = @("知道镇上很多秘密")
        greeting = "客官里面请！小店有上好的女儿红！"
    }
}

$json = $body | ConvertTo-Json -Depth 10
Invoke-RestMethod -Uri "http://192.168.5.189:8000/api/persona/" -Method Post -Body $json -ContentType "application/json; charset=utf-8"
```

**使用 Python:**
```python
import requests

data = {
    "planner_id": "1e23",
    "role_type": "tavern_owner_cn",
    "personality": {
        "name": "李老板",
        "role": "酒馆老板",
        "age": 45,
        "gender": "男",
        "traits": ["热情", "健谈", "八卦"],
        "background": "经营这家酒馆已有二十年，见过各路江湖人士",
        "speech_style": "说话热情洋溢，喜欢打听消息",
        "knowledge": ["本地消息", "江湖传闻", "酒水美食"],
        "secrets": ["知道镇上很多秘密"],
        "greeting": "客官里面请！小店有上好的女儿红！"
    }
}

response = requests.post(
    "http://192.168.5.189:8000/api/persona/",
    json=data,
    headers={"Content-Type": "application/json; charset=utf-8"}
)
print(response.json())
```

### 响应示例（新规范）
```json
{
    "success": true,
    "code": 201,
    "data": {
        "npc_id": "a1b2c3d4_1e23_tavern_owner_cn",
        "planner_id": "1e23",
        "role_type": "tavern_owner_cn",
        "personality": {
            "name": "李老板",
            "role": "酒馆老板",
            "age": 45,
            "gender": "男",
            "traits": ["热情", "健谈", "八卦"],
            "background": "经营这家酒馆已有二十年，见过各路江湖人士",
            "speech_style": "说话热情洋溢，喜欢打听消息",
            "knowledge": ["本地消息", "江湖传闻", "酒水美食"],
            "secrets": ["知道镇上很多秘密"],
            "greeting": "客官里面请！小店有上好的女儿红！"
        },
        "version": "1.0",
        "created_at": "2026-02-03T14:30:00.123456",
        "updated_at": "2026-02-03T14:30:00.123456"
    }
}
```

**⚠️ 注意：返回的 `npc_id` 包含服务器生成的随机前缀（如 `a1b2c3d4`），策划需要保存此完整ID用于后续操作！**

#### 示例 2：使用旧规范（兼容）

**使用 curl (Linux/Mac):**
```bash
curl -X POST http://192.168.5.189:8000/api/persona/ \
  -H "Content-Type: application/json; charset=utf-8" \
  -d '{
    "npc_id": "tavern_owner",
    "personality": {
        "name": "李老板",
        "role": "酒馆老板",
        "age": 45,
        "gender": "男",
        "traits": ["热情", "健谈", "八卦"],
        "background": "经营这家酒馆已有二十年，见过各路江湖人士",
        "speech_style": "说话热情洋溢，喜欢打听消息",
        "knowledge": ["本地消息", "江湖传闻", "酒水美食"],
        "secrets": ["知道镇上很多秘密"],
        "greeting": "客官里面请！小店有上好的女儿红！"
    }
}'
```
```

### 错误响应示例

**情况1：参数验证失败**
```json
{
    "success": false,
    "code": 400,
    "error": "策划ID格式错误: 仅含字母/数字，2-20字符"
}
```

**情况2：使用旧规范时ID冲突**
```json
{
    "success": false,
    "code": 409,
    "error": "人设 'tavern_owner' 已存在，请使用 PUT 更新"
}
```

**说明：** 使用新规范（planner_id + role_type）时，由于服务器会自动生成随机前缀，不会出现重复创建的409错误。

---

## 4. 更新人设

**PUT** `/api/persona/<npc_id>/`

### 说明
- 使用完整的 `npc_id` 进行更新（如 `1e23_tavern_owner_cn`）
- 只需传递要更新的字段
- `personality` 内的字段会合并更新，不会覆盖未传递的字段
- 不能修改 `npc_id`、`planner_id`、`role_type` 等标识字段

### 请求示例
```bash
curl -X PUT http://192.168.5.189:8000/api/persona/1e23_tavern_owner_cn/ \
  -H "Content-Type: application/json; charset=utf-8" \
  -d '{
    "personality": {
        "greeting": "欢迎光临！今天有新酿的米酒！",
        "traits": ["热情", "健谈", "八卦", "精明"]
    }
}'
```

### 响应示例
```json
{
    "success": true,
    "code": 200,
    "data": {
        "npc_id": "1e23_tavern_owner_cn",
        "planner_id": "1e23",
        "role_type": "tavern_owner_cn",
        "personality": {
            "name": "李老板",
            "role": "酒馆老板",
            "age": 45,
            "gender": "男",
            "traits": ["热情", "健谈", "八卦", "精明"],
            "background": "经营这家酒馆已有二十年，见过各路江湖人士",
            "speech_style": "说话热情洋溢，喜欢打听消息",
            "knowledge": ["本地消息", "江湖传闻", "酒水美食"],
            "secrets": ["知道镇上很多秘密"],
            "greeting": "欢迎光临！今天有新酿的米酒！"
        },
        "version": "1.0",
        "created_at": "2026-02-02T14:30:00.123456",
        "updated_at": "2026-02-03T15:00:00.654321"
    }
}
```

---

## 5. 删除人设

**DELETE** `/api/persona/<npc_id>/`

### 请求示例
```bash
curl -X DELETE http://192.168.5.189:8000/api/persona/1e23_tavern_owner_cn/
```

### 响应示例
```json
{
    "success": true,
    "code": 200,
    "data": {
        "message": "人设 '1e23_tavern_owner_cn' 已删除"
    }
}
```

### 注意
- 删除操作会直接删除 `npc_configs/<npc_id>.json` 文件
- 删除后无法恢复，请谨慎操作
- 建议在删除前先备份重要配置

---

## 完整使用流程示例

### 流程 1：使用新规范创建和管理 NPC（推荐）

#### 步骤 1：创建新 NPC 人设
```bash
# 策划 1e23 创建一个守卫NPC
curl -X POST http://192.168.5.189:8000/api/persona/ \
  -H "Content-Type: application/json; charset=utf-8" \
  -d '{
    "planner_id": "1e23",
    "role_type": "city_guard",
    "personality": {
        "name": "张守卫",
        "role": "城门守卫",
        "age": 35,
        "gender": "男",
        "traits": ["警惕", "尽责", "严肃"],
        "background": "在城门站岗十年，对进出人员了如指掌",
        "speech_style": "说话简短有力，带有官腔",
        "knowledge": ["城内治安", "进出规定", "可疑人员"],
        "secrets": ["知道城主的秘密通道"],
        "greeting": "站住！进城请出示通行证！"
    }
}'
```

**响应：**
```json
{
    "success": true,
    "code": 201,
    "data": {
        "npc_id": "1e23_city_guard",  // ⚠️ 保存此ID用于后续操作！
        "planner_id": "1e23",
        "role_type": "city_guard",
        ...
    }
}
```

#### 步骤 2：查询已创建的人设
```bash
# 获取所有人设列表
curl -X GET http://192.168.5.189:8000/api/persona/

# 获取特定人设详情（使用返回的 npc_id）
curl -X GET http://192.168.5.189:8000/api/persona/1e23_city_guard/
```

#### 步骤 3：使用该人设进行对话
```bash
# 对话（指定 persona，使用返回的 npc_id）
curl -X POST http://192.168.5.189:8000/chat/general/ \
  -H "Content-Type: application/json; charset=utf-8" \
  -d '{
    "message": "大哥，我想进城",
    "persona": "1e23_city_guard"
}'
```

#### 步骤 4：更新人设
```bash
# 修改守卫的问候语（使用返回的 npc_id）
curl -X PUT http://192.168.5.189:8000/api/persona/1e23_city_guard/ \
  -H "Content-Type: application/json; charset=utf-8" \
  -d '{
    "personality": {
        "greeting": "来者何人？报上名来！"
    }
}'
```

#### 步骤 5：删除人设（可选）
```bash
# 删除测试用的人设（使用返回的 npc_id）
curl -X DELETE http://192.168.5.189:8000/api/persona/1e23_city_guard/
```

### 流程 2：多个策划创建相同角色类型（演示冲突防护）

```bash
# 策划 1e23 创建酒馆老板
curl -X POST http://192.168.5.189:8000/api/persona/ \
  -H "Content-Type: application/json; charset=utf-8" \
  -d '{
    "planner_id": "1e23",
    "role_type": "tavern_owner_cn",
    "personality": { "name": "李老板", "role": "酒馆老板", ... }
}'
# 返回: npc_id = "1e23_tavern_owner_cn" ✅

# 策划 2f45 也创建酒馆老板（不会冲突）
curl -X POST http://192.168.5.189:8000/api/persona/ \
  -H "Content-Type: application/json; charset=utf-8" \
  -d '{
    "planner_id": "2f45",
    "role_type": "tavern_owner_cn",
    "personality": { "name": "王老板", "role": "酒馆老板", ... }
}'
# 返回: npc_id = "2f45_tavern_owner_cn" ✅

# 策划 1e23 再次创建酒馆老板（会被拒绝）
curl -X POST http://192.168.5.189:8000/api/persona/ \
  -H "Content-Type: application/json; charset=utf-8" \
  -d '{
    "planner_id": "1e23",
    "role_type": "tavern_owner_cn",
    "personality": { "name": "赵老板", "role": "酒馆老板", ... }
}'
# 返回: 409 错误 - "策划 '1e23' 已创建角色类型 'tavern_owner_cn'" ❌
```

---

---

## Java/Android 客户端示例

### 创建人设（新规范 - 推荐）
```java
import org.json.JSONObject;
import org.json.JSONArray;
import java.net.HttpURLConnection;
import java.net.URL;
import java.io.OutputStream;
import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.nio.charset.StandardCharsets;

// 1. 构建请求数据（使用新规范）
JSONObject requestData = new JSONObject();
requestData.put("planner_id", "1e23");  // 策划ID
requestData.put("role_type", "my_npc");  // 角色类型

JSONObject personality = new JSONObject();
personality.put("name", "我的NPC");
personality.put("role", "测试角色");
personality.put("age", 30);
personality.put("gender", "男");
personality.put("traits", new JSONArray().put("友好").put("热情"));
personality.put("background", "这是一个测试NPC");
personality.put("speech_style", "说话友好亲切");
personality.put("knowledge", new JSONArray().put("测试知识"));
personality.put("secrets", new JSONArray().put("测试秘密"));
personality.put("greeting", "你好！");

requestData.put("personality", personality);

// 2. 发送 POST 请求
String serverUrl = "http://192.168.5.189:8000";
HttpURLConnection conn = (HttpURLConnection) new URL(serverUrl + "/api/persona/").openConnection();
conn.setRequestMethod("POST");
conn.setRequestProperty("Content-Type", "application/json; charset=utf-8");
conn.setDoOutput(true);

// 写入请求体
OutputStream os = conn.getOutputStream();
os.write(requestData.toString().getBytes(StandardCharsets.UTF_8));
os.close();

// 3. 读取响应
int responseCode = conn.getResponseCode();
if (responseCode == 201) {
    BufferedReader in = new BufferedReader(new InputStreamReader(conn.getInputStream(), StandardCharsets.UTF_8));
    StringBuilder response = new StringBuilder();
    String inputLine;
    while ((inputLine = in.readLine()) != null) {
        response.append(inputLine);
    }
    in.close();
    
    // 4. 解析响应，获取生成的 npc_id
    JSONObject jsonResponse = new JSONObject(response.toString());
    JSONObject data = jsonResponse.getJSONObject("data");
    String npcId = data.getString("npc_id");  // ← 保存此ID！例如: "1e23_my_npc"
    
    System.out.println("创建成功！NPC ID: " + npcId);
    // 将 npcId 保存到本地数据库或配置文件，用于后续操作
    
} else if (responseCode == 409) {
    System.out.println("该角色已存在，请使用不同的 role_type 或更新现有角色");
} else {
    System.out.println("创建失败: " + responseCode);
}
```

### 创建人设（旧规范 - 兼容）
```java
// 使用旧规范（直接指定 npc_id）
JSONObject persona = new JSONObject();
persona.put("npc_id", "my_custom_npc");  // 需自行保证唯一性

JSONObject personality = new JSONObject();
personality.put("name", "我的NPC");
personality.put("role", "测试角色");
// ... 其他字段

persona.put("personality", personality);

// 发送请求（同上）
```

### 获取人设列表
```java
HttpURLConnection conn = (HttpURLConnection) new URL(serverUrl + "/api/persona/").openConnection();
conn.setRequestMethod("GET");

int responseCode = conn.getResponseCode();
if (responseCode == 200) {
    BufferedReader in = new BufferedReader(new InputStreamReader(conn.getInputStream(), StandardCharsets.UTF_8));
    String inputLine;
    StringBuilder response = new StringBuilder();
    while ((inputLine = in.readLine()) != null) {
        response.append(inputLine);
    }
    in.close();
    
    JSONObject jsonResponse = new JSONObject(response.toString());
    System.out.println(jsonResponse.toString(2));
}
```

### 更新人设
```java
JSONObject updateData = new JSONObject();
JSONObject personality = new JSONObject();
personality.put("greeting", "欢迎光临！");
updateData.put("personality", personality);

// 使用完整的 npc_id（如 "1e23_my_npc"）
HttpURLConnection conn = (HttpURLConnection) new URL(serverUrl + "/api/persona/1e23_my_npc/").openConnection();
conn.setRequestMethod("PUT");
conn.setRequestProperty("Content-Type", "application/json; charset=utf-8");
conn.setDoOutput(true);

OutputStream os = conn.getOutputStream();
os.write(updateData.toString().getBytes(StandardCharsets.UTF_8));
os.close();

int responseCode = conn.getResponseCode();  // 200 = 更新成功
```

### 删除人设
```java
// 使用完整的 npc_id（如 "1e23_my_npc"）
HttpURLConnection conn = (HttpURLConnection) new URL(serverUrl + "/api/persona/1e23_my_npc/").openConnection();
conn.setRequestMethod("DELETE");

int responseCode = conn.getResponseCode();  // 200 = 删除成功
```

### Python 客户端示例（新规范）
```python
import requests

# 1. 创建角色（使用新规范）
response = requests.post(
    "http://192.168.5.189:8000/api/persona/",
    json={
        "planner_id": "1e23",
        "role_type": "tavern_owner_cn",
        "personality": {
            "name": "李老板",
            "role": "酒馆老板",
            "age": 45,
            "gender": "男",
            "traits": ["热情", "健谈"],
            "background": "经营酒馆二十年",
            "speech_style": "热情洋溢",
            "knowledge": ["本地消息"],
            "greeting": "客官里面请！"
        }
    },
    headers={"Content-Type": "application/json; charset=utf-8"}
)

# 2. 检查响应并保存 npc_id
if response.status_code == 201:
    data = response.json()["data"]
    npc_id = data["npc_id"]  # ← 保存此ID！例如: "1e23_tavern_owner_cn"
    print(f"创建成功！NPC ID: {npc_id}")
    # 保存 npc_id 到配置文件或数据库
    
elif response.status_code == 409:
    print("该角色已存在")
else:
    print(f"创建失败: {response.json()}")

# 3. 使用保存的 npc_id 进行后续操作
# 查询
response = requests.get(f"http://192.168.5.189:8000/api/persona/{npc_id}/")

# 更新
response = requests.put(
    f"http://192.168.5.189:8000/api/persona/{npc_id}/",
    json={"personality": {"greeting": "欢迎光临！"}}
)

# 删除
response = requests.delete(f"http://192.168.5.189:8000/api/persona/{npc_id}/")
```

---

---

## 注意事项

### 1. 字段命名
- ✅ 使用 `npc_id` 而不是 `id`
- ✅ 与 `npc_configs/` 目录下的 JSON 文件格式保持一致

### 2. ID 唯一性和冲突防护
- **推荐使用新规范**：提供 `planner_id` + `role_type`，系统自动生成唯一的 `npc_id`
- **格式**：`{server_prefix}_{planner_id}_{role_type}`（如：`a1b2c3d4_1e23_tavern_owner_cn`）
- **服务器前缀**：8位随机十六进制字符，确保全局唯一性
- **冲突防护**：
  - ✅ 不同策划可以创建相同的 `role_type`，不会冲突
  - ✅ 同一策划可以创建多个相同的 `role_type`（每次生成不同的随机前缀）
  - ✅ 服务器前缀确保每个 `npc_id` 都是唯一的
- **旧规范兼容**：仍可直接指定 `npc_id`，但需自行保证唯一性
- **保存返回的 npc_id**：创建成功后，务必保存返回的完整 `npc_id` 用于后续操作

### 3. 编码问题
- **必须使用 UTF-8 编码**
- 请求头建议添加 `charset=utf-8`
- Windows PowerShell 用户建议使用 `Invoke-RestMethod` 而不是 `curl.exe`
- 推荐使用 Python 或 Java 客户端，避免命令行编码问题

### 4. 热更新
- 更新人设后，正在进行的对话会在下次请求时使用新配置
- 无需重启服务器

### 5. 数据持久化
- 所有配置保存在 `npc_configs/<npc_id>.json` 文件中
- 删除操作会直接删除文件，无法恢复
- 建议定期备份 `npc_configs/` 目录

### 6. 字段限制
| 字段 | 限制 |
|------|------|
| planner_id | 2-20字符，仅含字母数字 |
| role_type | 2-50字符，字母开头，含字母数字下划线 |
| server_prefix | 8字符，服务器自动生成 |
| npc_id（生成） | 格式：{server_prefix}_{planner_id}_{role_type} |
| name | ≤50字符 |
| role | ≤100字符 |
| background | ≤1000字符 |
| speech_style | ≤500字符 |
| traits | ≤20项 |
| knowledge | ≤20项 |

### 7. 与对话系统集成
- 创建人设后，可在对话 API 中通过 `persona` 参数指定使用
- 使用完整的 `npc_id`（如 `1e23_tavern_owner_cn`）
- 参考 `CHAT_INTERFACE_README.md` 了解对话接口使用方法

---

## 常见问题

### Q1: 中文字符显示为问号或乱码？
**A:** 确保请求头包含 `Content-Type: application/json; charset=utf-8`，并使用 UTF-8 编码发送数据。

### Q2: 为什么要使用 planner_id + role_type？
**A:** 防止不同策划创建相同 `npc_id` 导致互相覆盖。每个策划有独立的命名空间，系统自动生成唯一的 `npc_id`。

### Q3: 多个策划能创建相同的角色类型吗？
**A:** 可以。例如：
- 策划 `1e23` 创建 `tavern_owner_cn` → `npc_id = a1b2c3d4_1e23_tavern_owner_cn`
- 策划 `2f45` 创建 `tavern_owner_cn` → `npc_id = e5f6g7h8_2f45_tavern_owner_cn`
- 两者不会冲突，各自独立

### Q4: 同一策划能创建多个相同类型的角色吗？
**A:** 可以！由于服务器会自动生成随机前缀，每次创建都会生成不同的 `npc_id`：
- 第一次创建：`a1b2c3d4_1e23_tavern_owner_cn`
- 第二次创建：`e5f6g7h8_1e23_tavern_owner_cn`
- 第三次创建：`i9j0k1l2_1e23_tavern_owner_cn`

### Q5: 创建后返回的 npc_id 有什么用？
**A:** 返回的 `npc_id`（如 `a1b2c3d4_1e23_tavern_owner_cn`）是系统自动生成的唯一标识，包含：
- **服务器前缀**（`a1b2c3d4`）：确保全局唯一性
- **策划ID**（`1e23`）：标识创建者
- **角色类型**（`tavern_owner_cn`）：描述角色

必须保存并用于所有后续操作（查询、更新、删除、对话）。

### Q6: 能修改已生成的 npc_id 吗？
**A:** 不能。`npc_id`、`planner_id`、`role_type` 一旦生成就不可修改。如需更改，请删除后重新创建。

### Q7: 如何查询我创建的所有角色？
**A:** 调用列表接口，筛选 `planner_id` 字段：
```python
response = requests.get("http://192.168.5.189:8000/api/persona/")
my_personas = [p for p in response.json()["data"]["personas"] 
               if p.get("planner_id") == "1e23"]
```

### Q8: 旧的 npc_id 格式还能用吗？
**A:** 可以。系统同时支持新旧两种方式：
- **新规范**（推荐）：提供 `planner_id` + `role_type`，自动生成 `npc_id`
- **旧规范**（兼容）：直接提供 `npc_id`，需自行保证唯一性

### Q9: 如何批量导入人设？
**A:** 可以直接将符合格式的 JSON 文件放入 `npc_configs/` 目录，服务器会自动识别。

### Q10: 删除人设后能恢复吗？
**A:** 不能。删除操作会直接删除文件，建议删除前先备份。

### Q11: 人设配置有大小限制吗？
**A:** 建议单个人设配置不超过 10KB，过大的配置可能影响性能。

---

## 📌 最佳实践

### 1. 使用新规范创建角色
✅ **推荐做法：**
```json
{
    "planner_id": "1e23",
    "role_type": "tavern_owner_cn",
    "personality": { ... }
}
```

❌ **不推荐：**
```json
{
    "npc_id": "tavern_owner_cn",  // 可能与其他策划冲突
    "personality": { ... }
}
```

### 2. 保存并使用返回的 npc_id
```python
# ✅ 正确做法
response = requests.post(url, json=data)
npc_id = response.json()["data"]["npc_id"]
save_to_config(npc_id)  # 保存到配置文件或数据库

# 后续使用
requests.get(f"{url}/{npc_id}/")

# ❌ 错误做法
# 不要硬编码或猜测 npc_id
requests.get(f"{url}/tavern_owner_cn/")  # 可能不存在
```

### 3. 使用有意义的 role_type
```python
# ✅ 好的命名
"tavern_owner_cn"      # 中文酒馆老板
"city_guard_north"     # 北门守卫
"blacksmith_master"    # 铁匠大师

# ❌ 不好的命名
"npc1"                 # 无意义
"test"                 # 不明确
"aaa"                  # 难以理解
```

### 4. 统一管理 planner_id
```python
# ✅ 使用工号或用户ID
planner_id = "1e23"      # 工号
planner_id = "user001"   # 用户ID
planner_id = "team5"     # 团队标识

# ❌ 不要使用随机值
planner_id = "abc123xyz"  # 难以追溯
```

### 5. 错误处理
```python
response = requests.post(url, json=data)

if response.status_code == 201:
    # 创建成功
    npc_id = response.json()["data"]["npc_id"]
    print(f"创建成功: {npc_id}")
    
elif response.status_code == 409:
    # 已存在，可以选择更新
    error = response.json()["error"]
    print(f"角色已存在: {error}")
    # 可以改用 PUT 更新
    
elif response.status_code == 400:
    # 参数错误
    error = response.json()["error"]
    print(f"参数错误: {error}")
    
else:
    print(f"未知错误: {response.status_code}")
```

### 6. 批量管理角色
```python
# 查询某个策划的所有角色
def get_planner_personas(planner_id):
    response = requests.get("http://192.168.5.189:8000/api/persona/")
    all_personas = response.json()["data"]["personas"]
    return [p for p in all_personas if p.get("planner_id") == planner_id]

# 使用
my_personas = get_planner_personas("1e23")
for persona in my_personas:
    print(f"{persona['npc_id']}: {persona['name']}")
```

### 7. 版本控制和备份
```bash
# 定期备份配置目录
tar -czf npc_configs_backup_$(date +%Y%m%d).tar.gz npc_configs/

# 或使用 git 管理
cd npc_configs/
git add .
git commit -m "Update NPC configs"
```

### 8. 测试流程
```python
# 1. 创建测试角色
test_data = {
    "planner_id": "test",
    "role_type": "test_npc",
    "personality": {"name": "测试NPC", "role": "测试"}
}
response = requests.post(url, json=test_data)
npc_id = response.json()["data"]["npc_id"]

# 2. 测试功能
# ...

# 3. 清理测试数据
requests.delete(f"{url}/{npc_id}/")
```

---

## 相关文档

- [对话接口文档](CHAT_INTERFACE_README.md) - 了解如何使用人设进行对话
- [项目 README](README.md) - 项目整体说明
- [npc_configs/](npc_configs/) - 查看现有人设配置示例
