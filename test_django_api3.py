#!/usr/bin/env python3
"""
Django API 测试工具
用于测试所有对话系统的API端点

运行方式:
    python test_django_api.py
"""

import requests
import json
import sys

BASE_URL = "http://localhost:8000"

def print_section(title):
    """打印分节标题"""
    print("\n" + "=" * 60)
    print(f"  {title}")
    print("=" * 60)

def print_result(success, message):
    """打印测试结果"""
    icon = "✅" if success else "❌"
    print(f"{icon} {message}")

def test_qa_page():
    """测试QA页面"""
    print_section("测试1: QA页面 (Web界面)")
    
    try:
        response = requests.get(f"{BASE_URL}/qa/", timeout=5)
        
        print(f"URL: {BASE_URL}/qa/")
        print(f"状态码: {response.status_code}")
        print(f"内容类型: {response.headers.get('Content-Type')}")
        
        if response.status_code == 200 and 'text/html' in response.headers.get('Content-Type', ''):
            print_result(True, "QA页面正常访问")
            print(f"提示: 可以在浏览器中打开 {BASE_URL}/qa/ 进行对话")
            return True
        else:
            print_result(False, "QA页面访问失败")
            return False
            
    except requests.exceptions.ConnectionError:
        print_result(False, "无法连接到Django服务器")
        print("请确保Django正在运行: python manage.py runserver 0.0.0.0:8000")
        return False
    except Exception as e:
        print_result(False, f"测试出错: {e}")
        return False

def test_general_chat():
    """测试通用对话"""
    print_section("测试2: 通用对话 (general_chat)")
    
    try:
        session = requests.Session()
        
        # 1. 创建会话
        print("\n步骤1: 创建会话...")
        print(f"URL: {BASE_URL}/general/connect/")
        
        response = session.post(f"{BASE_URL}/general/connect/", timeout=10)
        
        print(f"状态码: {response.status_code}")
        
        if response.status_code != 200:
            print_result(False, "创建会话失败")
            print(f"响应内容: {response.text[:200]}")
            return False
        
        # 检查是否是JSON
        try:
            data = response.json()
            print_result(True, "会话创建成功")
            print(f"   AI消息: {data.get('msg', '')[:80]}...")
            print(f"   响应时间: {data.get('response_time', 0):.2f}秒")
        except json.JSONDecodeError:
            print_result(False, "响应不是JSON格式 (可能是HTML错误页面)")
            print(f"响应内容: {response.text[:200]}")
            return False
        
        # 2. 发送消息
        print("\n步骤2: 发送消息...")
        print(f"URL: {BASE_URL}/general/chat/")
        print(f"消息: 你好，请介绍一下自己")
        
        response = session.post(
            f"{BASE_URL}/general/chat/",
            data="你好，请介绍一下自己".encode('utf-8'),
            timeout=10
        )
        
        print(f"状态码: {response.status_code}")
        
        if response.status_code == 200:
            try:
                data = response.json()
                print_result(True, "对话成功")
                print(f"   AI回复: {data.get('msg', '')[:100]}...")
                print(f"   响应时间: {data.get('response_time', 0):.2f}秒")
            except json.JSONDecodeError:
                print_result(False, "响应不是JSON格式")
                return False
        else:
            print_result(False, "对话失败")
            return False
        
        # 3. 继续对话
        print("\n步骤3: 继续对话...")
        print(f"消息: 今天天气怎么样？")
        
        response = session.post(
            f"{BASE_URL}/general/chat/",
            data="今天天气怎么样？".encode('utf-8'),
            timeout=10
        )
        
        if response.status_code == 200:
            try:
                data = response.json()
                print_result(True, "对话成功 (历史记忆保持)")
                print(f"   AI回复: {data.get('msg', '')[:100]}...")
            except json.JSONDecodeError:
                print_result(False, "响应不是JSON格式")
                return False
        
        return True
        
    except requests.exceptions.Timeout:
        print_result(False, "请求超时 (Ollama可能未运行)")
        print("请确保Ollama正在运行: ollama serve")
        return False
    except Exception as e:
        print_result(False, f"测试出错: {e}")
        return False

def test_planet_chat():
    """测试星球对话"""
    print_section("测试3: 星球环境对话 (llm.py)")
    
    try:
        session = requests.Session()
        
        # 1. 创建会话
        print("\n步骤1: 创建会话...")
        print(f"URL: {BASE_URL}/connect/")
        
        response = session.post(f"{BASE_URL}/connect/", timeout=10000)
        
        print(f"状态码: {response.status_code}")
        
        if response.status_code != 200:
            print_result(False, "创建会话失败")
            return False
        
        try:
            data = response.json()
            print_result(True, "会话创建成功")
            print(f"   AI消息: {data.get('msg', '')[:80]}...")
        except json.JSONDecodeError:
            print_result(False, "响应不是JSON格式")
            return False
        
        # 2. 发送消息
        print("\n步骤2: 发送消息...")
        print(f"URL: {BASE_URL}/chat/")
        print(f"消息: 我想创建一个温度20-30度的星球")
        
        response = session.post(
            f"{BASE_URL}/chat/",
            data="我想创建一个温度20-30度的星球".encode('utf-8'),
            timeout=15000
        )
        
        print(f"状态码: {response.status_code}")
        
        if response.status_code == 200:
            try:
                data = response.json()
                print_result(True, "对话成功")
                print(f"   AI回复: {data.get('msg', '')[:100]}...")
                if data.get('data'):
                    print(f"   提取的数据: {data.get('data')}")
            except json.JSONDecodeError:
                print_result(False, "响应不是JSON格式")
                return False
        else:
            print_result(False, "对话失败")
            return False
        
        return True
        
    except requests.exceptions.Timeout:
        print_result(False, "请求超时")
        return False
    except Exception as e:
        print_result(False, f"测试出错: {e}")
        return False

def test_npc_system():
    """测试NPC系统"""
    print_section("测试4: NPC对话系统")
    
    try:
        session = requests.Session()
        
        # 1. 创建会话
        print("\n步骤1: 创建会话...")
        print(f"URL: {BASE_URL}/npc/connect/")
        
        response = session.post(
            f"{BASE_URL}/npc/connect/",
            json={"player_id": "test_player_001"},
            timeout=10000
        )
        
        print(f"状态码: {response.status_code}")
        
        if response.status_code != 200:
            print_result(False, "创建会话失败 (NPC系统可能未配置)")
            return False
        
        try:
            data = response.json()
            print_result(True, "会话创建成功")
            print(f"   玩家ID: {data.get('player_id')}")
            print(f"   会话ID: {data.get('session_id')}")
        except json.JSONDecodeError:
            print_result(False, "响应不是JSON格式")
            return False
        
        # 2. 与NPC对话
        print("\n步骤2: 与NPC对话...")
        print(f"URL: {BASE_URL}/npc/chat/")
        print(f"NPC: blacksmith (老铁匠)")
        print(f"消息: 你好，你能帮我打造一把剑吗？")
        
        response = session.post(
            f"{BASE_URL}/npc/chat/",
            json={
                "npc_id": "blacksmith",
                "message": "你好，你能帮我打造一把剑吗？"
            },
            timeout=15000
        )
        
        print(f"状态码: {response.status_code}")
        
        if response.status_code == 200:
            try:
                data = response.json()
                if data.get('success'):
                    print_result(True, "对话成功")
                    print(f"   NPC名称: {data.get('npc_name')}")
                    print(f"   NPC回复: {data.get('reply', '')[:100]}...")
                    affinity = data.get('affinity', {})
                    print(f"   好感度: {affinity.get('level')} ({affinity.get('score')}/100)")
                else:
                    print_result(False, f"对话失败: {data.get('error')}")
                    return False
            except json.JSONDecodeError:
                print_result(False, "响应不是JSON格式")
                return False
        else:
            print_result(False, "对话失败")
            return False
        
        return True
        
    except requests.exceptions.Timeout:
        print_result(False, "请求超时")
        return False
    except Exception as e:
        print_result(False, f"测试出错: {e}")
        return False

def main():
    """主函数"""
    print("\n" + "=" * 60)
    print("  Django API 测试工具")
    print("  测试服务器: " + BASE_URL)
    print("=" * 60)
    
    results = []
    
    # 运行所有测试
    results.append(("QA页面", test_qa_page()))
    results.append(("通用对话", test_general_chat()))
    results.append(("星球对话", test_planet_chat()))
    results.append(("NPC系统", test_npc_system()))
    
    # 显示总结
    print_section("测试总结")
    
    for name, success in results:
        icon = "✅" if success else "❌"
        print(f"{icon} {name}: {'通过' if success else '失败'}")
    
    passed = sum(1 for _, success in results if success)
    total = len(results)
    
    print(f"\n通过: {passed}/{total}")
    
    if passed == total:
        print("\n🎉 所有测试通过！")
        return 0
    else:
        print("\n⚠️  部分测试失败，请检查Django和Ollama是否正常运行")
        return 1

if __name__ == "__main__":
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        print("\n\n测试被中断")
        sys.exit(1)
