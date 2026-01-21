# 对话系统集成完成

## 系统架构

```
Android App (Java)
    ↓ HTTP请求
Django Backend (Python)
    ↓ 调用
NPC System / General Chat
    ↓ 使用
LLM (Ollama/Qwen)
```

## 已完成的功能

### 1. Android端 (Java)

#### 核心文件
- `DialogManager.java` - 对话管理器，支持两种模式
- `DialogConfig.java` - 配置文件，方便切换模式和服务器
- `MainActivity.java` - 集成对话UI和管理器

#### 功能特性
- ✅ 支持两种对话模式：`GENERAL` 和 `NPC`
- ✅ 自动连接Django后端
- ✅ 发送消息后自动隐藏键盘
- ✅ 网络请求失败时使用本地模拟回复
- ✅ 可配置服务器地址和超时时间
- ✅ 支持禁用网络（纯本地模式）

### 2. Django后端 (Python)

#### 核心文件
- `extrator/llm/npc_system/views.py` - NPC系统API
- `agent/urls.py` - URL路由配置

#### API端点

**通用对话模式 (General)**
- `POST /general/connect/` - 创建会话
- `POST /general/chat/` - 发送消息
- `POST /general/clear/` - 清除历史

**NPC对话模式 (NPC)**
- `POST /npc/connect/` - 创建会话
- `POST /npc/chat/` - 与NPC对话
- `POST /npc/clear/` - 清除历史
- `GET /npc/status/` - 获取所有NPC状态
- `GET /npc/status/<npc_id>/` - 获取单个NPC状态
- `POST /npc/register/` - 注册新NPC
- `GET /npc/greeting/<npc_id>/` - 获取问候语

## 使用方法

### 切换对话模式

编辑 `DialogConfig.java`：

```java
// 使用通用对话模式
public static final DialogManager.DialogMode DEFAULT_MODE = DialogManager.DialogMode.GENERAL;

// 使用NPC对话模式
public static final DialogManager.DialogMode DEFAULT_MODE = DialogManager.DialogMode.NPC;
```

### 修改服务器地址

```java
// Android模拟器访问本机
public static final String SERVER_URL = "http://10.0.2.2:8000";

// 真机访问局域网
public static final String SERVER_URL = "http://192.168.1.100:8000";

// 远程服务器
public static final String SERVER_URL = "http://your-server.com:8000";
```

### 禁用网络（仅本地模拟）

```java
public static final boolean ENABLE_NETWORK = false;
```

## 测试流程

### 1. 启动Django服务器

```bash
cd OllamaSpace/SceneAgentServer
python manage.py runserver 0.0.0.0:8000
```

### 2. 测试API（可选）

```bash
# 测试通用对话
curl -X POST http://localhost:8000/general/connect/ \
  -H "Content-Type: application/json" \
  -d '{"player_id": "test_player"}'

curl -X POST http://localhost:8000/general/chat/ \
  -H "Content-Type: application/json" \
  -d '{"message": "你好", "session_id": "xxx"}'

# 测试NPC对话
curl -X POST http://localhost:8000/npc/connect/ \
  -H "Content-Type: application/json" \
  -d '{"player_id": "test_player"}'

curl -X POST http://localhost:8000/npc/chat/ \
  -H "Content-Type: application/json" \
  -d '{"npc_id": "npc_001", "message": "你好", "player_id": "test_player"}'
```

### 3. 运行Android应用

1. 在 `DialogConfig.java` 中配置模式和服务器地址
2. 编译并运行应用
3. 点击对话按钮打开对话界面
4. 输入消息并发送
5. 查看Logcat日志确认网络请求

## 关键配置说明

### DialogConfig.java 配置项

| 配置项 | 说明 | 默认值 |
|--------|------|--------|
| `DEFAULT_MODE` | 对话模式 | `DialogMode.NPC` |
| `SERVER_URL` | 服务器地址 | `http://10.0.2.2:8000` |
| `ENABLE_NETWORK` | 是否启用网络 | `true` |
| `CONNECT_TIMEOUT` | 连接超时(ms) | `10000` |
| `READ_TIMEOUT` | 读取超时(ms) | `10000` |
| `DEFAULT_NPC_ID` | 默认NPC ID | `npc_001` |
| `DEFAULT_NPC_NAME` | 默认NPC名称 | `苏清寒` |

## 网络权限

确保 `AndroidManifest.xml` 中已添加：

```xml
<uses-permission android:name="android.permission.INTERNET" />
<uses-permission android:name="android.permission.ACCESS_NETWORK_STATE" />
```

## 故障排查

### 问题1：无法连接到服务器

**解决方案：**
1. 检查Django服务器是否运行：`python manage.py runserver 0.0.0.0:8000`
2. 检查防火墙是否允许8000端口
3. 真机测试时确保手机和电脑在同一局域网
4. 检查 `SERVER_URL` 配置是否正确

### 问题2：请求超时

**解决方案：**
1. 增加超时时间：修改 `CONNECT_TIMEOUT` 和 `READ_TIMEOUT`
2. 检查网络连接质量
3. 查看Django日志是否有错误

### 问题3：收不到回复

**解决方案：**
1. 查看Logcat日志：`adb logcat | grep DialogManager`
2. 检查Django后端是否正常返回数据
3. 确认NPC ID是否存在（NPC模式）
4. 临时禁用网络测试本地模拟是否正常

## 下一步扩展

- [ ] 添加语音输入功能
- [ ] 支持多NPC切换
- [ ] 添加对话历史记录
- [ ] 实现离线缓存
- [ ] 添加表情功能
- [ ] 支持图片消息
