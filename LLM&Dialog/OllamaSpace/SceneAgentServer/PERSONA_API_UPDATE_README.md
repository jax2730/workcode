# Persona API 接口规范化更新

## 📋 更新概述

本次更新解决了多个策划创建角色时的 **ID 冲突问题**，引入了新的创建规范。

---

## ⚠️ 核心问题

**问题确认：** 多个策划创建相同的 `npc_id` 会导致互相覆盖，造成数据丢失。

**示例：**
```
策划A 创建 tavern_owner_cn → 保存到 npc_configs/tavern_owner_cn.json
策划B 创建 tavern_owner_cn → 覆盖 npc_configs/tavern_owner_cn.json ❌
结果：策划A 的数据丢失
```

---

## ✨ 解决方案

### 新规范：planner_id + role_type

**创建请求：**
```json
{
    "planner_id": "1e23",           // 策划ID（工号/用户ID）
    "role_type": "tavern_owner_cn", // 角色类型
    "personality": { ... }
}
```

**系统自动生成唯一 npc_id：**
```
npc_id = planner_id + "_" + role_type
       = "1e23_tavern_owner_cn"
```

**返回响应：**
```json
{
    "success": true,
    "code": 201,
    "data": {
        "npc_id": "1e23_tavern_owner_cn",  // ⚠️ 保存此ID用于后续操作！
        "planner_id": "1e23",
        "role_type": "tavern_owner_cn",
        "personality": { ... }
    }
}
```

---

## 🔒 冲突防护

### ✅ 允许：不同策划创建相同角色类型
```
策划 1e23 创建 tavern_owner_cn → npc_id = 1e23_tavern_owner_cn ✅
策划 2f45 创建 tavern_owner_cn → npc_id = 2f45_tavern_owner_cn ✅
```

### ❌ 拒绝：同一策划重复创建
```
策划 1e23 创建 tavern_owner_cn → 成功 ✅
策划 1e23 再次创建 tavern_owner_cn → 返回 409 错误 ❌
```

---

## 📖 快速开始

### 1. 创建角色
```bash
curl -X POST http://192.168.5.189:8000/api/persona/ \
  -H "Content-Type: application/json; charset=utf-8" \
  -d '{
    "planner_id": "1e23",
    "role_type": "tavern_owner_cn",
    "personality": {
        "name": "李老板",
        "role": "酒馆老板",
        "greeting": "客官里面请！"
    }
}'
```

### 2. 保存返回的 npc_id
```json
{
    "npc_id": "1e23_tavern_owner_cn"  // ← 保存此ID！
}
```

### 3. 使用 npc_id 进行后续操作
```bash
# 查询
curl -X GET http://192.168.5.189:8000/api/persona/1e23_tavern_owner_cn/

# 更新
curl -X PUT http://192.168.5.189:8000/api/persona/1e23_tavern_owner_cn/ \
  -d '{"personality": {"greeting": "欢迎光临！"}}'

# 删除
curl -X DELETE http://192.168.5.189:8000/api/persona/1e23_tavern_owner_cn/

# 在对话中使用
curl -X POST http://192.168.5.189:8000/chat/general/ \
  -H "Content-Type: application/json" \
  -d '{"message": "你好", "session_id": "xxx", "persona": "1e23_tavern_owner_cn"}'
```

---

## 📚 文档

| 文档 | 说明 |
|------|------|
| [PERSONA_API.md](PERSONA_API.md) | 完整的 API 接口文档 |
| [CHANGELOG_PERSONA_API.md](CHANGELOG_PERSONA_API.md) | 详细的更新日志 |
| [test_persona_api.py](test_persona_api.py) | 测试脚本 |

---

## 🧪 测试

运行测试脚本验证功能：
```bash
python test_persona_api.py
```

---

## 🔄 兼容性

### ✅ 向后兼容
- 旧的 `npc_id` 格式仍然支持
- 现有数据无需修改
- 旧的 API 调用方式继续有效

### 📌 推荐做法
- **新角色**：使用新规范（planner_id + role_type）
- **旧角色**：可以保持不变，也可以逐步迁移

---

## 💡 最佳实践

### ✅ 推荐
```python
# 使用新规范
data = {
    "planner_id": "1e23",
    "role_type": "tavern_owner_cn",
    "personality": { ... }
}
response = requests.post(url, json=data)
npc_id = response.json()["data"]["npc_id"]  # 保存此ID
```

### ❌ 不推荐
```python
# 直接指定 npc_id（可能冲突）
data = {
    "npc_id": "tavern_owner_cn",  # 可能与其他策划冲突
    "personality": { ... }
}
```

---

## 📝 字段格式

| 字段 | 格式 | 示例 |
|------|------|------|
| planner_id | 字母数字，2-20字符 | `1e23`, `user001` |
| role_type | 字母开头，字母数字下划线，2-50字符 | `tavern_owner_cn` |
| npc_id（生成） | `{planner_id}_{role_type}` | `1e23_tavern_owner_cn` |

---

## ❓ 常见问题

### Q: 为什么要使用新规范？
**A:** 防止不同策划创建相同 `npc_id` 导致互相覆盖。

### Q: 必须使用新规范吗？
**A:** 不是必须，但强烈推荐。旧规范仍然支持，但需自行保证唯一性。

### Q: 如何查询我创建的所有角色？
**A:** 
```python
response = requests.get("http://192.168.5.189:8000/api/persona/")
my_personas = [p for p in response.json()["data"]["personas"] 
               if p.get("planner_id") == "1e23"]
```

### Q: 能修改已生成的 npc_id 吗？
**A:** 不能。`npc_id` 一旦生成就不可修改。

---

## 📞 技术支持

如有问题，请查看：
1. [完整 API 文档](PERSONA_API.md)
2. [更新日志](CHANGELOG_PERSONA_API.md)
3. 运行测试脚本：`python test_persona_api.py`

---

## 🎯 总结

- ✅ **问题确认**：ID 冲突问题确实存在
- ✅ **解决方案**：planner_id + role_type 自动生成唯一 npc_id
- ✅ **冲突防护**：不同策划可创建相同角色类型，同一策划不能重复创建
- ✅ **向后兼容**：旧规范仍然支持
- ✅ **返回 npc_id**：创建后返回生成的 npc_id 供后续使用
