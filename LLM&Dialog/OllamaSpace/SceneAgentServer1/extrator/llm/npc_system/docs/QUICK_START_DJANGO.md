# 快速启动指南 - Django服务器

> **目标**: 快速启动Django服务器，实现远程数据传输  
> **适用**: 原项目已有Django环境

---

## ⚡ 快速启动 (3步)

### 步骤1: 激活虚拟环境

```bash
# Linux/Mac
cd /OllamaSpace/SceneAgentServer
source venv/bin/activate

# Windows
cd E:\C++pandan\workcode\LLM&Dialog\OllamaSpace\SceneAgentServer
venv\Scripts\activate
```

**验证**:
```bash
# 应该看到 (venv) 前缀
(venv) user@host:~/SceneAgentServer$

# 验证Django
python -c "import django; print(django.VERSION)"
# 输出: (5, 1, 2, 'final', 0)
```

### 步骤2: 启动Ollama (如果未运行)

```bash
# 新终端
ollama serve
```

### 步骤3: 启动Django服务器

```bash
# 在激活venv的终端
python manage.py runserver 0.0.0.0:8000
```

**成功输出**:
```
Watching for file changes with StatReloader
Performing system checks...

System check identified no issues (0 silenced).
January 19, 2026 - 10:00:00
Django version 5.1.2, using settings 'agent.settings'
Starting development server at http://0.0.0.0:8000/
Quit the server with CONTROL-C.
```

---

## 🧪 测试API

### 测试1: 通用对话

```bash
# 1. 创建会话
curl -X POST http://localhost:8000/general/connect/

# 输出:
# {"success": 1, "msg": "你好！有什么我可以帮助你的吗？", ...}
# Set-Cookie: general_session_id=xxx

# 2. 发送消息 (使用返回的session_id)
curl -X POST http://localhost:8000/general/chat/ \
  -H "Cookie: general_session_id=general#192.168.1.100#xxx" \
  -d "你好，请介绍一下自己"
```

### 测试2: 星球环境对话

```bash
# 1. 创建会话
curl -X POST http://localhost:8000/connect/

# 2. 发送消息
curl -X POST http://localhost:8000/chat/ \
  -H "Cookie: agent_session_id=xxx" \
  -d "我想创建一个温度在20-30度的星球"
```

---

## 🌐 远程访问

### 配置防火墙

```bash
# Linux
sudo ufw allow 8000

# 查看服务器IP
ip addr show
# 或
ifconfig
```

### 从其他设备访问

```bash
# 替换为你的服务器IP
curl -X POST http://192.168.1.100:8000/general/connect/
```

---

## ❌ 常见问题

### 问题1: ModuleNotFoundError: No module named 'django'

**原因**: 虚拟环境未激活

**解决**:
```bash
source venv/bin/activate  # Linux/Mac
venv\Scripts\activate     # Windows
```

### 问题2: Address already in use

**原因**: 端口8000被占用

**解决**:
```bash
# 查找占用进程
lsof -i :8000  # Linux/Mac
netstat -ano | findstr :8000  # Windows

# 杀死进程或使用其他端口
python manage.py runserver 0.0.0.0:8001
```

### 问题3: Ollama连接失败

**原因**: Ollama未运行

**解决**:
```bash
# 启动Ollama
ollama serve

# 验证
curl http://localhost:11434/api/tags
```

---

## 📊 完整启动流程

```bash
# 终端1: Ollama
ollama serve

# 终端2: Django
cd /OllamaSpace/SceneAgentServer
source venv/bin/activate
python manage.py runserver 0.0.0.0:8000

# 终端3: 测试
curl -X POST http://localhost:8000/general/connect/
```

---

## 🎯 API端点总览

| 端点 | 方法 | 功能 | Cookie |
|------|------|------|--------|
| `/connect/` | POST | 创建星球对话会话 | agent_session_id |
| `/chat/` | POST | 星球环境对话 | agent_session_id |
| `/general/connect/` | POST | 创建通用对话会话 | general_session_id |
| `/general/chat/` | POST | 通用对话 | general_session_id |
| `/general/clear/` | POST | 清除历史 | general_session_id |

---

## 🚀 生产环境部署

### 使用uWSGI

```bash
# 安装uWSGI
pip install uwsgi

# 启动
uwsgi --ini uwsgi.ini
```

### 使用Gunicorn

```bash
# 安装Gunicorn
pip install gunicorn

# 启动
gunicorn agent.wsgi:application --bind 0.0.0.0:8000
```

---

**现在你可以启动服务器了！** 🎉
