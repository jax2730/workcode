# QA页面构建流程和修改指南

> **分析目标**: 详细解释QA页面的构建流程和数据流  
> **修改目标**: 如何切换到general_chat或npc_system

---

## 🔍 当前QA页面分析

### 为什么是星球对话？

**关键代码**:
```python
# extrator/views.py 第56行
def qa_view(request):
    if request.method == "POST":
        prompt = request.POST.get("prompt", "")
        
        # 这里调用的是 test_chat_z (星球环境对话)
        aimsg = test_chat_z.chat(prompt, session_id)
        response = aimsg.get("msg", "无法获取回复")
```

**原因**: `qa_view` 函数硬编码调用了 `test_chat_z.chat()`，这是星球环境对话系统。

---

## 📊 完整的数据流程

### 流程图

```
用户访问 http://localhost:8000/qa/
    ↓
agent/urls.py: path("qa/", extratorView.qa_view)
    ↓
extrator/views.py: qa_view(request)
    ↓
判断请求方法
    ├─ GET: 返回空白表单
    └─ POST: 处理用户输入
        ↓
    获取用户输入: prompt = request.POST.get("prompt")
        ↓
    调用对话系统: aimsg = test_chat_z.chat(prompt, session_id)
        ↓
    test_chat_z → llm.py → LangGraph → Ollama
        ↓
    返回结果: {"success": 1, "msg": "...", "data": {...}}
        ↓
    更新会话历史: session_history[session_id].append(...)
        ↓
    渲染模板: render(request, "qa.html", {...})
        ↓
    返回HTML页面给用户
```

### 详细步骤

#### 步骤1: 用户访问页面 (GET请求)
```python
# GET http://localhost:8000/qa/
def qa_view(request):
    session_id = request.COOKIES.get("agent_session_id")
    if not session_id:
        session_id = str(uuid.uuid4())
    
    # GET请求返回空模板
    return render(request, "qa.html", {
        "prompt": "",
        "response": "",
        "history": session_history.get(session_id, [])
    })
```

#### 步骤2: 用户提交表单 (POST请求)
```python
# POST http://localhost:8000/qa/
if request.method == "POST":
    # 1. 获取用户输入
    prompt = request.POST.get("prompt", "")
    
    # 2. 调用对话系统 (关键！)
    aimsg = test_chat_z.chat(prompt, session_id)
    response = aimsg.get("msg", "无法获取回复")
    
    # 3. 更新历史
    session_history[session_id].append({"role": "user", "content": prompt})
    session_history[session_id].append({"role": "ai", "content": response})
    
    # 4. 渲染模板
    return render(request, "qa.html", {
        "prompt": prompt,
        "response": response,
        "history": session_history[session_id]
    })
```

#### 步骤3: test_chat_z处理
```python
# extrator/llm/test_chat_z.py (推测)
def chat(prompt, session_id):
    # 调用LangGraph工作流
    result = graph.invoke(
        {"messages": [HumanMessage(content=prompt)]},
        config={"configurable": {"session_id": session_id}}
    )
    return {
        "success": 1,
        "msg": result["messages"][-1].content,
        "data": {...}
    }
```

#### 步骤4: 渲染HTML
```html
<!-- agent/templates/qa.html -->
<h1>QA System</h1>
<div class="chat-container">
    {% for message in history %}
        {% if message.role == "user" %}
            <strong>用户:</strong> {{ message.content }}
        {% else %}
            <strong>AI:</strong> {{ message.content }}
        {% endif %}
    {% endfor %}
</div>
<form method="post">
    <textarea name="prompt"></textarea>
    <button type="submit">提交</button>
</form>
```

---

## 🔧 修改方案

### 方案1: 修改为通用对话 (general_chat)

#### 修改 views.py

```python
# extrator/views.py

# 原代码 (第56-75行)
def qa_view(request):
    session_id = request.COOKIES.get("agent_session_id")
    if not session_id:
        session_id = str(uuid.uuid4())

    if session_id not in session_history:
        session_history[session_id] = []

    if request.method == "POST":
        prompt = request.POST.get("prompt", "")
        
        # 修改这里！从 test_chat_z 改为 general_chat
        # aimsg = test_chat_z.chat(prompt, session_id)
        # response = aimsg.get("msg", "无法获取回复")
        
        # 新代码：使用 general_chat
        result = general_chat.chat(prompt, session_id)
        response = result.get("msg", "无法获取回复")

        session_history[session_id].append({"role": "user", "content": prompt})
        session_history[session_id].append({"role": "ai", "content": response})

        response_obj = render(request, "qa.html", {
            "prompt": prompt,
            "response": response,
            "history": session_history[session_id]
        })

        response_obj.set_cookie("agent_session_id", session_id)
        return response_obj
    else:
        return render(request, "qa.html", {
            "prompt": "",
            "response": "",
            "history": session_history.get(session_id, [])
        })
```

#### 修改 qa.html (可选，更新标题)

```html
<!-- agent/templates/qa.html -->
<h1>通用对话系统</h1>  <!-- 从 QA System 改为 通用对话系统 -->
```

---

### 方案2: 修改为NPC对话

#### 步骤1: 修改 views.py

```python
# extrator/views.py

# 在文件顶部添加导入
from .llm.npc_system.npc_manager import NPCManager, NPCManagerConfig
from langchain_ollama import ChatOllama

# 初始化NPC管理器 (全局变量)
_npc_manager = None

def get_npc_manager():
    global _npc_manager
    if _npc_manager is None:
        llm = ChatOllama(model="qwen2.5", temperature=0.7)
        config = NPCManagerConfig(
            data_dir="./npc_data",
            config_dir="./npc_configs"
        )
        _npc_manager = NPCManager(config, llm)
    return _npc_manager

# 修改 qa_view 函数
def qa_view(request):
    session_id = request.COOKIES.get("agent_session_id")
    player_id = request.COOKIES.get("player_id", "player_web_001")
    
    if not session_id:
        session_id = str(uuid.uuid4())

    if session_id not in session_history:
        session_history[session_id] = []

    if request.method == "POST":
        prompt = request.POST.get("prompt", "")
        npc_id = request.POST.get("npc_id", "blacksmith")  # 默认铁匠
        
        # 使用NPC系统
        manager = get_npc_manager()
        result = manager.chat(npc_id, player_id, prompt, session_id)
        
        if result.get("success"):
            response = result.get("reply", "无法获取回复")
            affinity = result.get("affinity", {})
            
            # 添加好感度信息到回复
            response_with_affinity = f"{response}\n\n[好感度: {affinity.get('level', '未知')} ({affinity.get('score', 0)}/100)]"
        else:
            response_with_affinity = result.get("error", "对话失败")

        session_history[session_id].append({"role": "user", "content": prompt})
        session_history[session_id].append({"role": "ai", "content": response_with_affinity})

        response_obj = render(request, "qa.html", {
            "prompt": prompt,
            "response": response_with_affinity,
            "history": session_history[session_id]
        })

        response_obj.set_cookie("agent_session_id", session_id)
        response_obj.set_cookie("player_id", player_id)
        return response_obj
    else:
        return render(request, "qa.html", {
            "prompt": "",
            "response": "",
            "history": session_history.get(session_id, [])
        })
```

#### 步骤2: 修改 qa.html (添加NPC选择)

```html
<!-- agent/templates/qa.html -->
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>NPC对话系统</title>
    <style>
        .chat-container {
            max-height: 500px;
            overflow-y: auto;
            border: 1px solid #ddd;
            padding: 10px;
            margin-bottom: 10px;
        }
        .chat-message {
            margin: 5px 0;
        }
        .user-message {
            color: blue;
        }
        .ai-message {
            color: green;
        }
    </style>
</head>
<body>
    <h1>NPC对话系统</h1>
    
    <div class="chat-container">
        {% for message in history %}
            <div class="chat-message">
                {% if message.role == "user" %}
                    <strong class="user-message">用户:</strong> {{ message.content }}
                {% else %}
                    <strong class="ai-message">NPC:</strong> {{ message.content }}
                {% endif %}
            </div>
        {% endfor %}
    </div>
    
    <form method="post">
        {% csrf_token %}
        
        <!-- 添加NPC选择 -->
        <label for="npc_id">选择NPC:</label>
        <select name="npc_id" id="npc_id">
            <option value="blacksmith">老铁匠</option>
            <option value="merchant">商人</option>
            <option value="guard_captain">守卫队长</option>
            <option value="herbalist">草药师</option>
            <option value="innkeeper">旅店老板</option>
        </select>
        <br><br>
        
        <textarea name="prompt" rows="4" style="width: 100%;" placeholder="请输入您的问题..."></textarea>
        <br>
        <button type="submit">提交</button>
    </form>
</body>
</html>
```

---

### 方案3: 创建多个页面 (推荐)

#### 步骤1: 创建新的视图函数

```python
# extrator/views.py

# 通用对话页面
def qa_general_view(request):
    """通用对话Web界面"""
    session_id = request.COOKIES.get("general_session_id")
    if not session_id:
        session_id = str(uuid.uuid4())

    if session_id not in session_history:
        session_history[session_id] = []

    if request.method == "POST":
        prompt = request.POST.get("prompt", "")
        
        # 使用 general_chat
        result = general_chat.chat(prompt, session_id)
        response = result.get("msg", "无法获取回复")

        session_history[session_id].append({"role": "user", "content": prompt})
        session_history[session_id].append({"role": "ai", "content": response})

        response_obj = render(request, "qa_general.html", {
            "prompt": prompt,
            "response": response,
            "history": session_history[session_id]
        })

        response_obj.set_cookie("general_session_id", session_id)
        return response_obj
    else:
        return render(request, "qa_general.html", {
            "prompt": "",
            "response": "",
            "history": session_history.get(session_id, [])
        })


# NPC对话页面
def qa_npc_view(request):
    """NPC对话Web界面"""
    # ... (使用方案2的代码)
    pass
```

#### 步骤2: 添加URL路由

```python
# agent/urls.py

urlpatterns = [
    # 原有的星球对话
    path("qa/", extratorView.qa_view, name="qa_view"),
    
    # 新增：通用对话
    path("qa/general/", extratorView.qa_general_view, name="qa_general"),
    
    # 新增：NPC对话
    path("qa/npc/", extratorView.qa_npc_view, name="qa_npc"),
    
    # ... 其他路由
]
```

#### 步骤3: 创建对应的HTML模板

```bash
# 复制模板
cp agent/templates/qa.html agent/templates/qa_general.html
cp agent/templates/qa.html agent/templates/qa_npc.html

# 修改标题
# qa_general.html: <h1>通用对话系统</h1>
# qa_npc.html: <h1>NPC对话系统</h1> (并添加NPC选择下拉框)
```

---

## 📊 三种方案对比

| 方案 | 优点 | 缺点 | 推荐度 |
|------|------|------|--------|
| **方案1: 修改现有页面** | 简单快速 | 失去原有功能 | ⭐⭐⭐ |
| **方案2: 改为NPC** | 功能强大 | 配置复杂 | ⭐⭐⭐⭐ |
| **方案3: 创建多个页面** | 保留所有功能 | 需要更多代码 | ⭐⭐⭐⭐⭐ |

---

## 🚀 快速实施

### 最简单的修改 (方案1)

```python
# 只需修改 extrator/views.py 第62行

# 原代码
aimsg = test_chat_z.chat(prompt, session_id)
response = aimsg.get("msg", "无法获取回复")

# 改为
result = general_chat.chat(prompt, session_id)
response = result.get("msg", "无法获取回复")
```

**重启Django**:
```bash
# Ctrl+C 停止服务器
# 重新启动
python manage.py runserver 0.0.0.0:8000
```

**访问测试**:
```
http://localhost:8000/qa/
```

---

## 📝 总结

### 当前QA页面的数据流

```
浏览器 → Django (qa_view) → test_chat_z → LangGraph → Ollama → 返回
```

### 修改为general_chat的数据流

```
浏览器 → Django (qa_view) → general_chat → LangChain → Ollama → 返回
```

### 修改为NPC的数据流

```
浏览器 → Django (qa_view) → NPCManager → NPCAgent → 
    ├─ 记忆检索
    ├─ RAG检索
    ├─ 好感度查询
    ├─ 上下文构建
    └─ LLM生成 → 返回
```

---

**现在你可以根据需要选择方案进行修改了！** 🎉

推荐：先用方案1快速测试，然后再实施方案3创建完整的多页面系统。
