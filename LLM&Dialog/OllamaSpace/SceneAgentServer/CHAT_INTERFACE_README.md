# 对话系统接口重构说明

## 概述

本次重构创建了一个统一的对话接口系统，支持在Java Android客户端中轻松切换不同的Python对话后端。

## 架构图

```
┌─────────────────────────────────────────────────────────────────┐
│                        Java Android客户端                        │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                    DialogConfig.java                     │   │
│  │  CURRENT_ENDPOINT = DialogEndpoint.API_GENERAL          │   │
│  │                       ↓                                  │   │
│  │  DialogEndpoint 枚举定义所有可用端点:                     │   │
│  │  - NPC, GENERAL, TEST (原始路径)                         │   │
│  │  - API_GENERAL, API_TEST, API_TEST_CHAT (统一API)       │   │
│  └─────────────────────────────────────────────────────────┘   │
│                              ↓                                   │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                   DialogManager.java                     │   │
│  │  - connectToBackend(): 使用 endpoint.getConnectPath()    │   │
│  │  - handlePlayerMessage(): 使用 endpoint.getChatPath()    │   │
│  │  - 自动兼容新旧响应格式                                   │   │
│  └─────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
                               │
                               ↓ HTTP POST (JSON)
┌─────────────────────────────────────────────────────────────────┐
│                        Django 后端                               │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                      urls.py                             │   │
│  │  原始路径:                                                │   │
│  │  - /general/connect/, /general/chat/, /general/clear/   │   │
│  │  - /test/connect/, /test/chat/, /test/clear/            │   │
│  │                                                          │   │
│  │  统一API路径 (推荐):                                      │   │
│  │  - /connect/<backend>/                                   │   │
│  │  - /chat/<backend>/                                      │   │
│  │  - /clear/<backend>/                                     │   │
│  │  - /backends/  (列出可用后端)                             │   │
│  └─────────────────────────────────────────────────────────┘   │
│                              ↓                                   │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                    views.py                              │   │
│  │  unified_chat(request, backend) → chat_interface        │   │
│  │  unified_connect(request, backend) → chat_interface     │   │
│  │  unified_clear(request, backend) → chat_interface       │   │
│  └─────────────────────────────────────────────────────────┘   │
│                              ↓                                   │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                chat_interface.py                         │   │
│  │  ChatInterface 类管理所有后端:                            │   │
│  │  - "general" → general_chat.py                          │   │
│  │  - "test"    → test.py                                  │   │
│  │  - "test_chat" → test_chat.py                           │   │
│  │                                                          │   │
│  │  统一请求/响应格式                                        │   │
│  │  支持动态注册新后端                                       │   │
│  └─────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
```

## 文件修改清单

### 新增文件
- `SceneAgentServer/extrator/llm/chat_interface.py` - 统一对话接口处理器

### 修改文件

#### Python 后端
1. **`SceneAgentServer/extrator/views.py`**
   - 添加 `test_connect`, `test_chat_view`, `test_clear` 视图
   - 添加 `unified_chat`, `unified_connect`, `unified_clear` 统一API视图
   - 添加 `list_chat_backends` 视图

2. **`SceneAgentServer/agent/urls.py`**
   - 添加 `/test/` 路由
   - 添加 `/api/<backend>/` 统一路由

#### Java 客户端
1. **`DialogConfig.java`**
   - 添加 `API_GENERAL`, `API_TEST`, `API_TEST_CHAT` 端点
   - 更新文档说明统一API格式

2. **`DialogManager.java`**
   - `connectToBackend()` 改为使用 `DialogConfig.CURRENT_ENDPOINT`
   - `handlePlayerMessage()` 改为使用配置的端点路径
   - 响应解析兼容新旧格式

## 使用方法

### 切换对话后端 (Java端)

只需修改 `DialogConfig.java` 中的一行代码：

```java
// 使用NPC对话系统
public static final DialogEndpoint CURRENT_ENDPOINT = DialogEndpoint.NPC;

// 使用通用对话系统 (统一API)
public static final DialogEndpoint CURRENT_ENDPOINT = DialogEndpoint.API_GENERAL;

// 使用测试对话系统 (统一API)
public static final DialogEndpoint CURRENT_ENDPOINT = DialogEndpoint.API_TEST;

// 使用环境提取对话 (统一API)
public static final DialogEndpoint CURRENT_ENDPOINT = DialogEndpoint.API_TEST_CHAT;
```

### 添加新的对话后端

#### 1. Python 端：注册新后端

在 `chat_interface.py` 中：

```python
# 方式1: 在 _register_default_backends() 中添加
self.register_backend(
    "my_custom",
    chat_handler=self._my_custom_chat,
    connect_handler=self._my_custom_connect,
    clear_handler=self._my_custom_clear
)

# 方式2: 动态注册
from extrator.llm.chat_interface import chat_interface

def my_chat(request):
    # 处理逻辑
    return ChatResponse(success=True, reply="回复", session_id=request.session_id)

chat_interface.register_backend("my_custom", chat_handler=my_chat)
```

#### 2. Java 端：添加端点

在 `DialogConfig.java` 中：

```java
public enum DialogEndpoint {
    // ... 现有端点 ...
    
    // 新增自定义端点
    API_MY_CUSTOM("my_custom", "/connect/my_custom/", "/chat/my_custom/", "/clear/my_custom/", false);
}

// 使用新端点
public static final DialogEndpoint CURRENT_ENDPOINT = DialogEndpoint.API_MY_CUSTOM;
```

## 请求/响应格式

### 统一API请求格式 (POST JSON)

```json
{
    "message": "用户消息",
    "session_id": "会话ID（可选，会自动生成）",
    "persona": "人设名称（可选，仅general支持）"
}
```

### 统一API响应格式

```json
{
    "success": true,
    "reply": "AI回复内容",
    "session_id": "会话ID",
    "response_time": 0.123,
    "backend": "general",
    "persona": "blacksmith"
}
```

### 兼容性

Java端现在可以正确解析两种格式：
- 新格式: `{"success": true, "reply": "xxx"}`
- 旧格式: `{"success": 1, "msg": "xxx"}`

## API 端点列表

| 端点 | 路径 | 说明 |
|------|------|------|
| 原始-General | `/general/chat/` | 通用对话（原始路径） |
| 原始-Test | `/test/chat/` | 测试对话（原始路径） |
| 原始-NPC | `/npc/chat/` | NPC对话（原始路径） |
| 统一-General | `/chat/general/` | 通用对话（统一API） |
| 统一-Test | `/chat/test/` | 测试对话（统一API） |
| 统一-TestChat | `/chat/test_chat/` | 环境提取（统一API） |
| 后端列表 | `/backends/` | 列出所有可用后端 |

## 获取可用后端

```bash
curl http://localhost:8000/backends/
# 响应: {"success": true, "backends": ["general", "test", "test_chat"]}
```

## 测试示例

```bash
# 连接
curl -X POST http://localhost:8000/connect/general/

# 对话
curl -X POST http://localhost:8000/chat/general/ \
  -H "Content-Type: application/json" \
  -d '{"message": "你好", "session_id": "test123"}'

# 清除历史
curl -X POST http://localhost:8000/clear/general/ \
  -H "Content-Type: application/json" \
  -d '{"session_id": "test123"}'
```
