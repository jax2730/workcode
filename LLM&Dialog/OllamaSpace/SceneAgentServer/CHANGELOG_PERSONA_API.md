# Persona API 更新日志

## 版本 2.0 - 2026-02-03

### 🎯 核心更新：防止 ID 冲突的新规范

#### 问题描述
多个策划创建相同的 `npc_id`（如 `tavern_owner_cn`）会导致互相覆盖，造成数据丢失。

**确认：此问题确实存在！**

#### 解决方案
引入 `planner_id` + `role_type` 机制，系统自动生成唯一的 `npc_id`。

---

## 📝 主要变更

### 1. 新增字段

#### `planner_id` (策划ID)
- **类型**: string
- **格式**: 仅含字母数字，2-20字符
- **示例**: `1e23`, `user001`, `planner42`
- **用途**: 标识角色创建者，作为命名空间隔离

#### `role_type` (角色类型)
- **类型**: string
- **格式**: 字母开头，含字母数字下划线，2-50字符
- **示例**: `tavern_owner_cn`, `city_guard`, `blacksmith`
- **用途**: 描述角色类型

### 2. 自动生成 `npc_id`

**格式**: `{planner_id}_{role_type}`

**示例**:
- 输入: `planner_id="1e23"`, `role_type="tavern_owner_cn"`
- 输出: `npc_id="1e23_tavern_owner_cn"`

### 3. 冲突防护机制

#### ✅ 允许：不同策划创建相同角色类型
```
策划 1e23 创建 tavern_owner_cn → npc_id = 1e23_tavern_owner_cn
策划 2f45 创建 tavern_owner_cn → npc_id = 2f45_tavern_owner_cn
```

#### ❌ 拒绝：同一策划重复创建
```
策划 1e23 创建 tavern_owner_cn → 成功
策划 1e23 再次创建 tavern_owner_cn → 返回 409 错误
```

---

## 🔄 API 变更

### POST `/api/persona/` - 创建人设

#### 新规范（推荐）
```json
{
    "planner_id": "1e23",
    "role_type": "tavern_owner_cn",
    "personality": { ... }
}
```

**响应**:
```json
{
    "success": true,
    "code": 201,
    "data": {
        "npc_id": "1e23_tavern_owner_cn",  // 自动生成
        "planner_id": "1e23",
        "role_type": "tavern_owner_cn",
        "personality": { ... }
    }
}
```

#### 旧规范（兼容）
```json
{
    "npc_id": "my_npc",
    "personality": { ... }
}
```

仍然支持，但需自行保证唯一性。

### GET `/api/persona/` - 列出所有人设

**响应变更**: 新增 `planner_id` 和 `role_type` 字段

```json
{
    "personas": [
        {
            "npc_id": "1e23_tavern_owner_cn",
            "planner_id": "1e23",        // 新增
            "role_type": "tavern_owner_cn",  // 新增
            "name": "李老板",
            "role": "酒馆老板",
            "updated_at": "2026-02-03T10:30:00"
        }
    ]
}
```

### GET `/api/persona/<npc_id>/` - 获取指定人设

**响应变更**: 新增 `planner_id` 和 `role_type` 字段

```json
{
    "npc_id": "1e23_tavern_owner_cn",
    "planner_id": "1e23",        // 新增
    "role_type": "tavern_owner_cn",  // 新增
    "personality": { ... }
}
```

---

## 📂 文件变更

### 修改的文件

1. **`extrator/llm/persona_api.py`**
   - 新增 `validate_planner_id()` 方法
   - 新增 `validate_role_type()` 方法
   - 新增 `generate_npc_id()` 方法
   - 修改 `validate_create()` 支持新旧两种格式
   - 修改 `create_persona()` 实现自动生成 npc_id
   - 修改 `PersonaConfig` 数据模型，新增字段
   - 修改 `PersonaStorage.create()` 保存新字段
   - 修改 `PersonaStorage.list_all()` 返回新字段

2. **`PERSONA_API.md`**
   - 全面更新文档，说明新规范
   - 新增快速开始部分
   - 更新所有示例代码
   - 新增常见问题解答
   - 新增最佳实践部分

3. **`test_persona_api.py`** (新增)
   - 完整的测试脚本
   - 测试新规范的所有功能
   - 测试冲突防护机制

---

## 🔧 代码示例

### Python 客户端
```python
import requests

# 创建角色（新规范）
response = requests.post(
    "http://192.168.5.189:8000/api/persona/",
    json={
        "planner_id": "1e23",
        "role_type": "tavern_owner_cn",
        "personality": {
            "name": "李老板",
            "role": "酒馆老板",
            "greeting": "客官里面请！"
        }
    }
)

# 保存返回的 npc_id
if response.status_code == 201:
    npc_id = response.json()["data"]["npc_id"]
    print(f"创建成功: {npc_id}")  # 输出: 1e23_tavern_owner_cn
```

### Java 客户端
```java
JSONObject data = new JSONObject();
data.put("planner_id", "1e23");
data.put("role_type", "tavern_owner_cn");
data.put("personality", personality);

// 发送请求...
// 获取返回的 npc_id
String npcId = response.getJSONObject("data").getString("npc_id");
```

---

## ⚠️ 重要提示

### 对现有系统的影响

1. **向后兼容**: 旧的 `npc_id` 格式仍然支持
2. **现有数据**: 不受影响，可以继续使用
3. **新功能**: 推荐使用新规范创建角色

### 迁移建议

#### 不需要立即迁移
- 现有的人设配置文件无需修改
- 旧的 API 调用方式继续有效

#### 推荐逐步迁移
1. 新创建的角色使用新规范
2. 旧角色可以保持不变
3. 如需统一管理，可以逐步迁移

#### 迁移步骤（可选）
```python
# 1. 获取旧角色
old_persona = requests.get(f"{url}/tavern_owner/").json()["data"]

# 2. 使用新规范重新创建
new_data = {
    "planner_id": "1e23",
    "role_type": "tavern_owner",
    "personality": old_persona["personality"]
}
response = requests.post(url, json=new_data)
new_npc_id = response.json()["data"]["npc_id"]  # 1e23_tavern_owner

# 3. 更新引用此角色的地方

# 4. 删除旧角色（可选）
requests.delete(f"{url}/tavern_owner/")
```

---

## 🧪 测试

### 运行测试脚本
```bash
cd OllamaSpace/SceneAgentServer
python test_persona_api.py
```

### 测试覆盖
- ✅ 使用新规范创建角色
- ✅ 重复创建检测（同一策划）
- ✅ 不同策划创建相同角色类型
- ✅ 获取角色详情
- ✅ 列出所有角色
- ✅ 更新角色
- ✅ 删除角色

---

## 📊 性能影响

- **存储**: 每个人设增加约 50 字节（planner_id + role_type）
- **查询**: 无影响
- **创建**: 增加 ID 格式验证，性能影响可忽略

---

## 🔮 未来计划

### 可能的增强功能
1. 按 `planner_id` 批量查询接口
2. 角色权限管理（策划只能修改自己的角色）
3. 角色版本历史记录
4. 角色模板功能

---

## 📞 支持

如有问题，请参考：
- [完整 API 文档](PERSONA_API.md)
- [测试脚本](test_persona_api.py)
- 或联系开发团队
