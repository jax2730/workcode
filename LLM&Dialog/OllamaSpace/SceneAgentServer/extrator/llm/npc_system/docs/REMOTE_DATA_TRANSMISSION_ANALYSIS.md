# 原项目远程数据传输架构分析

> **分析目标**: 详细解释原项目如何通过Django进行远程数据传输  
> **更新时间**: 2026-01-19

---

## 🔍 关键发现

### 1. **Django已经安装在venv中！**

从目录结构可以看到：
```
venv/lib64/python3.10/site-packages/Django-5.1.2.dist-info/
venv/lib/python3.10/site-packages/Django-5.1.2.dist-info/
```

**说明**: 
- ✅ Django 5.1.2 已经安装
- ✅ 在虚拟环境 `venv` 中
- ❌ 你没有激活虚拟环境！

---

## 🚀 完整的远程数据传输架构

### 架构图

```
┌─────────────────────────────────────────────────────────────┐
│                  远程客户端 (浏览器/App)                      │
│                                                              │
│  JavaScript / Python / Unity / 任何HTTP客户端                │
└──────────────────────┬──────────────────────────────────────┘
                       │ HTTP请求
                       ↓
┌─────────────────────────────────────────────────────────────┐
│              Django Web服务器 (0.0.0.0:8000)                 │
│                                                              │
│  ┌────────────────────────────────────────────────────┐    │
│  │  1. manage.py runserver                            │    │
│  │     ↓                                               │    │
│  │  2. agent/settings.py (配置)                       │    │
│  │     ↓                                               │    │
│  │  3. agent/urls.py (路由分发)                       │    │
│  │     ↓                                               │    │
│  │  4. extrator/views.py (视图处理)                   │    │
│  └────────────────────────────────────────────────────┘    │
│                       ↓                                      │
│  ┌────────────────────────────────────────────────────┐    │
│  │  5. 调用对话系统                                    │    │
│  │     ├─ llm.py (星球环境对话)                       │    │
│  │     ├─ general_chat.py (通用对话)                  │    │
│  │     └─ npc_system (NPC对话)                        │    │
│  └────────────────────────────────────────────────────┘    │
│                       ↓                                      │
│  ┌────────────────────────────────────────────────────┐    │
│  │  6. 调用Ollama LLM                                  │    │
│  │     http://localhost:11434                          │    │
│  └────────────────────────────────────────────────────┘    │
│                       ↓                                      │
│  ┌────────────────────────────────────────────────────┐    │
│  │  7. 返回JSON响应                                    │    │
│  │     {"success": 1, "msg": "...", "data": {...}}    │    │
│  └────────────────────────────────────────────────────┘    │
└──────────────────────┬──────────────────────────────────────┘
                       │ HTTP响应
                       ↓
┌─────────────────────────────────────────────────────────────┐
│                  远程客户端接收响应                           │
└─────────────────────────────────────────────────────────────┘
```

---

## 📝 详细流程分析

### 步骤1: 启动Django服务器

```bash
# 1. 激活虚拟环境 (关键步骤！)
source venv/bin/activate  # Linux/Mac
# 或
venv\Scripts\activate  # Windows

# 2. 启动Django
python manage.py runserver 0.0.0.0:8000
```

**启动流程**:
```python
# manage.py
def main():
    os.environ.setdefault('DJANGO_SETTINGS_MODULE', 'agent.settings')
    from django.core.management import execute_from_command_line
    execute_from_command_line(sys.argv)
```

### 步骤2: Django加载配置

```python
# agent/settings.py
ALLOWED_HOSTS = ["*"]  # 允许所有主机访问

INSTALLED_APPS = [
    'django.contrib.admin',
    'django.contrib.auth',
    # ... 其他应用
]

# 数据库配置
DATABASES = {
    'default': {
        'ENGINE': 'django.db.backends.sqlite3',
        'NAME': BASE_DIR / 'db.sqlite3',
    }
}
```

### 步骤3: 路由配置

```python
# agent/urls.py
from extrator import views as extratorView

urlpatterns = [
    # 星球环境对话 (llm.py)
    path('chat/', extratorView.chat),
    path('connect/', extratorView.connect),
    
    # 通用对话 (general_chat.py)
    path('general/connect/', extratorView.general_connect),
    path('general/chat/', extratorView.general_chat_view),
    path('general/clear/', extratorView.general_clear),
    
    # NPC对话 (npc_system)
    # path('npc/...', ...) - 可以添加
]
```

### 步骤4: 视图处理

```python
# extrator/views.py
from .llm import llm_3_args, general_chat

# 星球环境对话
@csrf_exempt
def connect(request):
    ipAddr = getIpAddr(request)
    session_id = "#".join([ipAddr, str(uuid.uuid4())])
    
    # 调用llm.py
    aimsg = llm_3_args.chat("你好", session_id)
    
    response = JsonResponse(aimsg)
    response.set_cookie("agent_session_id", session_id)
    return response

@csrf_exempt
def chat(request):
    session_id = request.COOKIES.get("agent_session_id")
    prompt = request.body.decode('utf-8')
    
    # 调用llm.py
    aimsg = llm_3_args.chat(prompt, session_id)
    return JsonResponse(aimsg)

# 通用对话
@csrf_exempt
def general_connect(request):
    ipAddr = getIpAddr(request)
    session_id = "general#" + "#".join([ipAddr, str(uuid.uuid4())])
    
    # 调用general_chat.py
    result = general_chat.chat("你好", session_id)
    
    response = JsonResponse(result)
    response.set_cookie("general_session_id", session_id)
    return response

@csrf_exempt
def general_chat_view(request):
    session_id = request.COOKIES.get("general_session_id")
    prompt = request.body.decode('utf-8')
    
    # 调用general_chat.py
    result = general_chat.chat(prompt, session_id)
    return JsonResponse(result)
```

### 步骤5: 对话系统处理

```python
# extrator/llm/general_chat.py
def chat(user_message: str, session_id: str) -> dict:
    result = {
        "success": 1,
        "msg": "",
        "response_time": 0.0
    }
    
    try:
        # 调用LangChain对话链
        response = chat_chain.invoke(
            {"input": [HumanMessage(content=user_message)]},
            config={"configurable": {"session_id": session_id}}
        )
        
        result["msg"] = response.content
        
    except Exception as e:
        result["success"] = 0
        result["msg"] = f"对话出错: {str(e)}"
    
    return result
```

### 步骤6: Ollama LLM处理

```python
# LangChain内部调用Ollama
llm = ChatOllama(model="qwen2.5", temperature=0.7)

# 实际HTTP请求
POST http://localhost:11434/api/chat
{
    "model": "qwen2.5",
    "messages": [...],
    "stream": false
}
```

---

## 🌐 远程访问示例

### 客户端1: JavaScript (浏览器)

```javascript
// 1. 创建会话
fetch('http://your-server-ip:8000/general/connect/', {
    method: 'POST',
    credentials: 'include'  // 包含Cookie
})
.then(response => response.json())
.then(data => {
    console.log('Session created:', data);
    
    // 2. 发送消息
    return fetch('http://your-server-ip:8000/general/chat/', {
        method: 'POST',
        credentials: 'include',
        body: '你好，请介绍一下自己'
    });
})
.then(response => response.json())
.then(data => {
    console.log('AI回复:', data.msg);
});
```

### 客户端2: Python

```python
import requests

# 服务器地址
BASE_URL = "http://your-server-ip:8000"

# 1. 创建会话
session = requests.Session()
response = session.post(f"{BASE_URL}/general/connect/")
print("会话创建:", response.json())

# 2. 发送消息
response = session.post(
    f"{BASE_URL}/general/chat/",
    data="你好，请介绍一下自己".encode('utf-8')
)
result = response.json()
print("AI回复:", result["msg"])

# 3. 继续对话
response = session.post(
    f"{BASE_URL}/general/chat/",
    data="今天天气怎么样？".encode('utf-8')
)
result = response.json()
print("AI回复:", result["msg"])
```

### 客户端3: Unity (C#)

```csharp
using UnityEngine;
using UnityEngine.Networking;
using System.Collections;

public class ChatClient : MonoBehaviour
{
    private string baseUrl = "http://your-server-ip:8000";
    private string sessionCookie = "";
    
    IEnumerator Start()
    {
        // 1. 创建会话
        yield return StartCoroutine(Connect());
        
        // 2. 发送消息
        yield return StartCoroutine(Chat("你好"));
    }
    
    IEnumerator Connect()
    {
        UnityWebRequest request = UnityWebRequest.Post(
            baseUrl + "/general/connect/", 
            ""
        );
        
        yield return request.SendWebRequest();
        
        if (request.result == UnityWebRequest.Result.Success)
        {
            // 保存Cookie
            sessionCookie = request.GetResponseHeader("Set-Cookie");
            Debug.Log("会话创建成功");
        }
    }
    
    IEnumerator Chat(string message)
    {
        UnityWebRequest request = UnityWebRequest.Post(
            baseUrl + "/general/chat/",
            message
        );
        
        // 添加Cookie
        request.SetRequestHeader("Cookie", sessionCookie);
        
        yield return request.SendWebRequest();
        
        if (request.result == UnityWebRequest.Result.Success)
        {
            string response = request.downloadHandler.text;
            Debug.Log("AI回复: " + response);
        }
    }
}
```

---

## 🔧 你的问题解决方案

### 问题: Django未找到

**原因**: 虚拟环境未激活

**解决方案**:

```bash
# 方法1: 激活虚拟环境 (推荐)
cd /OllamaSpace/SceneAgentServer
source venv/bin/activate  # Linux
# 或
venv\Scripts\activate  # Windows

# 验证Django已安装
python -c "import django; print(django.VERSION)"
# 输出: (5, 1, 2, 'final', 0)

# 启动服务器
python manage.py runserver 0.0.0.0:8000

# 方法2: 使用venv中的Python直接运行
venv/bin/python manage.py runserver 0.0.0.0:8000
```

---

## 📊 完整的数据流

### 1. 客户端 → Django

```
HTTP POST http://server:8000/general/chat/
Headers:
  Cookie: general_session_id=xxx
Body:
  "你好，请介绍一下自己"
```

### 2. Django → views.py

```python
def general_chat_view(request):
    session_id = request.COOKIES.get("general_session_id")
    prompt = request.body.decode('utf-8')
    result = general_chat.chat(prompt, session_id)
    return JsonResponse(result)
```

### 3. views.py → general_chat.py

```python
def chat(user_message: str, session_id: str):
    response = chat_chain.invoke(
        {"input": [HumanMessage(content=user_message)]},
        config={"configurable": {"session_id": session_id}}
    )
    return {"success": 1, "msg": response.content}
```

### 4. general_chat.py → LangChain

```python
chain_with_history = RunnableWithMessageHistory(
    chain, get_session_history
)
```

### 5. LangChain → Ollama

```
HTTP POST http://localhost:11434/api/chat
{
    "model": "qwen2.5",
    "messages": [
        {"role": "system", "content": "你是一个友好的AI助手"},
        {"role": "user", "content": "你好，请介绍一下自己"}
    ]
}
```

### 6. Ollama → LangChain → general_chat.py → views.py → Django → 客户端

```json
{
    "success": 1,
    "msg": "你好！我是一个AI助手，可以帮助你回答问题...",
    "response_time": 1.23
}
```

---

## 🎯 关键要点

### 1. **虚拟环境是关键**

```bash
# 必须激活虚拟环境
source venv/bin/activate

# 或者使用venv中的Python
venv/bin/python manage.py runserver
```

### 2. **Django作为HTTP服务器**

- 监听 `0.0.0.0:8000`
- 接收HTTP请求
- 调用对话系统
- 返回JSON响应

### 3. **Cookie管理会话**

- `agent_session_id` - 星球环境对话
- `general_session_id` - 通用对话
- `npc_session_id` + `npc_player_id` - NPC对话

### 4. **三层架构**

```
客户端 (浏览器/App)
    ↓ HTTP
Django (Web服务器)
    ↓ 函数调用
对话系统 (llm.py/general_chat.py/npc_chat.py)
    ↓ HTTP
Ollama (LLM服务)
```

---

## 🚀 启动完整服务

```bash
# 终端1: 启动Ollama
ollama serve

# 终端2: 启动Django
cd /OllamaSpace/SceneAgentServer
source venv/bin/activate
python manage.py runserver 0.0.0.0:8000

# 终端3: 测试
curl -X POST http://localhost:8000/general/connect/
curl -X POST http://localhost:8000/general/chat/ \
  -H "Cookie: general_session_id=xxx" \
  -d "你好"
```

---

## 📝 总结

**原项目的远程数据传输方式**:

1. ✅ **使用Django作为Web服务器**
2. ✅ **通过HTTP API接收远程请求**
3. ✅ **Cookie管理会话状态**
4. ✅ **调用本地对话系统处理**
5. ✅ **返回JSON格式响应**

**你的问题**:
- ❌ 虚拟环境未激活
- ✅ Django已安装在venv中
- ✅ 只需激活venv即可运行

**解决方案**:
```bash
source venv/bin/activate
python manage.py runserver 0.0.0.0:8000
```

现在明白了吗？🚀
