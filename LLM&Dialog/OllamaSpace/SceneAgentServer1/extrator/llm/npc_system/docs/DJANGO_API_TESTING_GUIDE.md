# Django API测试完整指南

> **解决问题**: curl测试404错误、session_id设置  
> **适用场景**: 测试Django API端点

---

## ❌ 你遇到的问题

### 错误信息
```
Page not found at /general/connect/
```

### 可能原因

1. ✅ **URL配置正确** (已确认)
2. ❌ **Django没有重新加载** (最可能)
3. ❌ **端口不对**
4. ❌ **路径错误**

---

## 🔧 解决方案

### 方案1: 重启Django (推荐)

```bash
# 1. 停止Django
# 在Django终端按 Ctrl+C

# 2. 重新启动
python manage.py runserver 0.0.0.0:8000

# 3. 等待启动完成
# 看到 "Starting development server at http://0.0.0.0:8000/"
```

### 方案2: 检查Django日志

Django终端应该显示：
```
System check identified no issues (0 silenced).
Django version 5.1.2, using settings 'agent.settings'
Starting development server at http://0.0.0.0:8000/
Quit the server with CONTROL-C.
```

如果看到错误，说明配置有问题。

---

## 🧪 正确的测试步骤

### 步骤1: 确认Django正在运行

```bash
# 终端1: Django服务器
(venv) $ python manage.py runserver 0.0.0.0:8000
Starting development server at http://0.0.0.0:8000/
```

### 步骤2: 测试基础连接

```bash
# 终端2: 测试
# 先测试最简单的端点
curl http://localhost:8000/qa/
```

**预期结果**: 返回HTML页面（QA System）

### 步骤3: 测试通用对话连接

```bash
# 测试 general/connect/
curl -X POST http://localhost:8000/general/connect/
```

**预期结果**:
```json
{
    "success": 1,
    "msg": "你好！有什么我可以帮助你的吗？",
    "response_time": 1.23
}
```

**如果返回HTML错误页面**:
- Django没有找到这个路由
- 需要重启Django

### 步骤4: 保存session_id并测试对话

```bash
# 1. 创建会话并保存响应
curl -v -X POST http://localhost:8000/general/connect/ > response.txt

# 2. 查看响应头，找到Set-Cookie
# Set-Cookie: general_session_id=general#127.0.0.1#xxx; Path=/

# 3. 使用session_id进行对话
curl -X POST http://localhost:8000/general/chat/ \
  -H "Cookie: general_session_id=general#127.0.0.1#你的UUID" \
  -d "你好，请介绍一下自己"
```

---

## 📝 Session ID 详解

### Session ID 的作用

Session ID用于**识别用户会话**，保持对话历史的连续性。

### Session ID 的生成

```python
# extrator/views.py
def general_connect(request):
    ipAddr = getIpAddr(request)  # 获取IP地址
    session_id = "general#" + "#".join([ipAddr, str(uuid.uuid4())])
    # 例如: general#127.0.0.1#a1b2c3d4-e5f6-7890-abcd-ef1234567890
```

### Session ID 的传递

通过HTTP Cookie自动传递：

```
请求1: POST /general/connect/
响应1: Set-Cookie: general_session_id=xxx

请求2: POST /general/chat/
请求头: Cookie: general_session_id=xxx
```

---

## 🎯 完整测试脚本

### 方法1: 使用curl (手动)

```bash
#!/bin/bash

echo "=== 测试Django API ==="

# 1. 测试基础连接
echo "1. 测试基础连接..."
curl http://localhost:8000/qa/ | head -n 5

# 2. 创建会话
echo -e "\n\n2. 创建通用对话会话..."
RESPONSE=$(curl -s -c cookies.txt -X POST http://localhost:8000/general/connect/)
echo $RESPONSE

# 3. 发送消息
echo -e "\n\n3. 发送消息..."
curl -s -b cookies.txt -X POST http://localhost:8000/general/chat/ \
  -d "你好，请介绍一下自己"

echo -e "\n\n=== 测试完成 ==="
```

**运行**:
```bash
chmod +x test_api.sh
./test_api.sh
```

### 方法2: 使用Python (推荐)

创建 `test_django_api.py`:

```python
#!/usr/bin/env python3
import requests
import json

BASE_URL = "http://localhost:8000"

def test_qa_page():
    """测试QA页面"""
    print("=== 测试QA页面 ===")
    response = requests.get(f"{BASE_URL}/qa/")
    print(f"状态码: {response.status_code}")
    print(f"内容类型: {response.headers.get('Content-Type')}")
    if response.status_code == 200:
        print("✅ QA页面正常")
    else:
        print("❌ QA页面错误")
    print()

def test_general_chat():
    """测试通用对话"""
    print("=== 测试通用对话 ===")
    
    # 1. 创建会话
    print("1. 创建会话...")
    session = requests.Session()
    response = session.post(f"{BASE_URL}/general/connect/")
    
    print(f"状态码: {response.status_code}")
    print(f"响应: {response.text[:200]}")
    
    if response.status_code != 200:
        print("❌ 创建会话失败")
        return
    
    try:
        data = response.json()
        print(f"✅ 会话创建成功")
        print(f"   消息: {data.get('msg', '')[:50]}...")
    except:
        print("❌ 响应不是JSON格式")
        return
    
    # 2. 发送消息
    print("\n2. 发送消息...")
    response = session.post(
        f"{BASE_URL}/general/chat/",
        data="你好，请介绍一下自己".encode('utf-8')
    )
    
    print(f"状态码: {response.status_code}")
    
    if response.status_code == 200:
        try:
            data = response.json()
            print(f"✅ 对话成功")
            print(f"   AI回复: {data.get('msg', '')[:100]}...")
            print(f"   响应时间: {data.get('response_time', 0):.2f}秒")
        except:
            print("❌ 响应不是JSON格式")
    else:
        print("❌ 对话失败")
    
    # 3. 继续对话
    print("\n3. 继续对话...")
    response = session.post(
        f"{BASE_URL}/general/chat/",
        data="今天天气怎么样？".encode('utf-8')
    )
    
    if response.status_code == 200:
        try:
            data = response.json()
            print(f"✅ 对话成功")
            print(f"   AI回复: {data.get('msg', '')[:100]}...")
        except:
            print("❌ 响应不是JSON格式")
    
    print()

def test_planet_chat():
    """测试星球对话"""
    print("=== 测试星球对话 ===")
    
    session = requests.Session()
    
    # 1. 创建会话
    print("1. 创建会话...")
    response = session.post(f"{BASE_URL}/connect/")
    print(f"状态码: {response.status_code}")
    
    if response.status_code == 200:
        print("✅ 会话创建成功")
    else:
        print("❌ 创建会话失败")
        return
    
    # 2. 发送消息
    print("\n2. 发送消息...")
    response = session.post(
        f"{BASE_URL}/chat/",
        data="我想创建一个温度20-30度的星球".encode('utf-8')
    )
    
    print(f"状态码: {response.status_code}")
    
    if response.status_code == 200:
        try:
            data = response.json()
            print(f"✅ 对话成功")
            print(f"   AI回复: {data.get('msg', '')[:100]}...")
        except:
            print("❌ 响应不是JSON格式")
    
    print()

if __name__ == "__main__":
    print("=" * 60)
    print("Django API 测试工具")
    print("=" * 60)
    print()
    
    # 测试所有端点
    test_qa_page()
    test_general_chat()
    test_planet_chat()
    
    print("=" * 60)
    print("测试完成")
    print("=" * 60)
```

**运行**:
```bash
python test_django_api.py
```

---

## 🔍 调试技巧

### 1. 查看Django日志

Django终端会显示所有请求：

```
[19/Jan/2026 07:11:03] "POST /general/connect/ HTTP/1.1" 200 123
[GeneralChat] New session: general#127.0.0.1#xxx
[GeneralChat] User: 你好
[GeneralChat] AI: 你好！有什么我可以帮助你的吗？
[19/Jan/2026 07:11:05] "POST /general/chat/ HTTP/1.1" 200 456
```

**如果看到404**:
```
[19/Jan/2026 07:11:03] "POST /general/connect/ HTTP/1.1" 404 2345
```
说明URL路由没有配置或Django没有重新加载。

### 2. 使用 -v 参数查看详细信息

```bash
curl -v -X POST http://localhost:8000/general/connect/
```

**输出**:
```
> POST /general/connect/ HTTP/1.1
> Host: localhost:8000
> User-Agent: curl/7.68.0
> Accept: */*
> 
< HTTP/1.1 200 OK
< Content-Type: application/json
< Set-Cookie: general_session_id=xxx; Path=/
< 
{"success": 1, "msg": "..."}
```

### 3. 检查可用的URL

```bash
# 查看Django注册的所有URL
python manage.py show_urls  # 需要安装 django-extensions

# 或者手动查看
cat agent/urls.py
```

---

## 📊 可用的API端点

### 1. QA页面 (Web界面)
```
GET  http://localhost:8000/qa/
POST http://localhost:8000/qa/
```

### 2. 星球对话 (API)
```
POST http://localhost:8000/connect/
POST http://localhost:8000/chat/
Cookie: agent_session_id=xxx
```

### 3. 通用对话 (API)
```
POST http://localhost:8000/general/connect/
POST http://localhost:8000/general/chat/
POST http://localhost:8000/general/clear/
Cookie: general_session_id=xxx
```

### 4. NPC对话 (API)
```
POST http://localhost:8000/npc/connect/
POST http://localhost:8000/npc/chat/
POST http://localhost:8000/npc/status/
...
Cookie: npc_session_id=xxx, npc_player_id=xxx
```

---

## 💡 常见问题

### Q1: 为什么返回HTML而不是JSON？

**A**: Django返回了错误页面（404/500）

**解决**:
1. 检查URL是否正确
2. 重启Django
3. 查看Django日志

### Q2: Session ID如何设置？

**A**: 不需要手动设置！

```bash
# 方法1: 使用 -c 保存Cookie
curl -c cookies.txt -X POST http://localhost:8000/general/connect/
curl -b cookies.txt -X POST http://localhost:8000/general/chat/ -d "你好"

# 方法2: 使用Python requests (自动管理)
session = requests.Session()
session.post("http://localhost:8000/general/connect/")
session.post("http://localhost:8000/general/chat/", data="你好")
```

### Q3: 可以只用QA页面吗？

**A**: 可以！

QA页面是Web界面，不需要curl：
```
http://localhost:8000/qa/
```

直接在浏览器中输入消息即可。

---

## 🚀 推荐测试方式

### 最简单: 浏览器
```
http://localhost:8000/qa/
```

### 最灵活: Python脚本
```bash
python test_django_api.py
```

### 最专业: Postman
1. 导入API集合
2. 自动管理Cookie
3. 保存测试用例

---

## 📝 总结

### 你的问题

1. ❌ **404错误**: Django没有找到路由
2. ❓ **Session ID**: 不需要手动设置

### 解决方案

1. ✅ **重启Django**
2. ✅ **使用Python脚本测试** (推荐)
3. ✅ **或直接使用QA页面**

### 立即测试

```bash
# 方法1: 重启Django
python manage.py runserver 0.0.0.0:8000

# 方法2: 使用Python测试
python test_django_api.py

# 方法3: 浏览器访问
http://localhost:8000/qa/
```

---

**现在试试Python测试脚本吧！** 🚀
