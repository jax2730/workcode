#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
测试创建 NPC 人设的脚本
使用 npc_id 字段，符合 npc_configs 中的 JSON 格式
"""

import requests
import json
import sys

# 服务器地址
BASE_URL = "http://192.168.5.189:8000"


def print_section(title):
    """打印分隔线"""
    print("\n" + "=" * 60)
    print(f"  {title}")
    print("=" * 60)


def test_get_personas():
    """测试获取所有人设"""
    print_section("测试 GET /api/persona/")
    
    try:
        response = requests.get(f"{BASE_URL}/api/persona/", timeout=5)
        print(f"状态码: {response.status_code}")
        print(f"响应:\n{json.dumps(response.json(), indent=2, ensure_ascii=False)}")
        return response.status_code == 200
    except requests.exceptions.ConnectionError:
        print(f"❌ 无法连接到服务器 {BASE_URL}")
        print("   请确认服务器是否运行")
        return False
    except Exception as e:
        print(f"❌ 错误: {e}")
        return False


def test_create_npc_english():
    """测试创建英文 NPC"""
    print_section("测试创建英文 NPC")
    
    data = {
        "npc_id": "tavern_owner",
        "personality": {
            "name": "Li Boss",
            "role": "Tavern Owner",
            "age": 45,
            "gender": "male",
            "traits": ["warm", "talkative", "gossip"],
            "background": "Running this tavern for 20 years, seen many adventurers",
            "speech_style": "Enthusiastic and friendly",
            "knowledge": ["local news", "rumors", "food and wine"],
            "secrets": ["knows many town secrets"],
            "greeting": "Welcome! We have the best wine!"
        }
    }
    
    print(f"发送数据:\n{json.dumps(data, indent=2, ensure_ascii=False)}")
    
    try:
        response = requests.post(
            f"{BASE_URL}/api/persona/",
            json=data,
            headers={"Content-Type": "application/json; charset=utf-8"},
            timeout=10
        )
        print(f"\n状态码: {response.status_code}")
        print(f"响应:\n{json.dumps(response.json(), indent=2, ensure_ascii=False)}")
        
        if response.status_code in [200, 201]:
            print("✅ 创建成功")
            return True
        elif response.status_code == 409:
            print("⚠️  NPC 已存在")
            return True
        else:
            print("❌ 创建失败")
            return False
    except Exception as e:
        print(f"❌ 错误: {e}")
        return False


def test_create_npc_chinese():
    """测试创建中文 NPC"""
    print_section("测试创建中文 NPC")
    
    data = {
        "npc_id": "tavern_owner_cn",
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
    
    print(f"发送数据:\n{json.dumps(data, indent=2, ensure_ascii=False)}")
    
    try:
        response = requests.post(
            f"{BASE_URL}/api/persona/",
            json=data,
            headers={"Content-Type": "application/json; charset=utf-8"},
            timeout=10
        )
        print(f"\n状态码: {response.status_code}")
        print(f"响应:\n{json.dumps(response.json(), indent=2, ensure_ascii=False)}")
        
        if response.status_code in [200, 201]:
            print("✅ 创建成功")
            return True
        elif response.status_code == 409:
            print("⚠️  NPC 已存在")
            return True
        else:
            print("❌ 创建失败")
            return False
    except Exception as e:
        print(f"❌ 错误: {e}")
        return False


def test_get_specific_npc(npc_id):
    """测试获取特定 NPC"""
    print_section(f"测试 GET /api/persona/{npc_id}/")
    
    try:
        response = requests.get(f"{BASE_URL}/api/persona/{npc_id}/", timeout=5)
        print(f"状态码: {response.status_code}")
        
        if response.status_code == 200:
            data = response.json()
            print(f"响应:\n{json.dumps(data, indent=2, ensure_ascii=False)}")
            
            # 验证保存的文件是否正确
            if data.get("success") and "data" in data:
                npc_data = data["data"]
                personality = npc_data.get("personality", {})
                print(f"\n✅ NPC 信息:")
                print(f"   ID: {npc_data.get('npc_id')}")
                print(f"   名称: {personality.get('name')}")
                print(f"   角色: {personality.get('role')}")
                print(f"   问候语: {personality.get('greeting')}")
            return True
        else:
            print(f"❌ 获取失败: {response.json()}")
            return False
    except Exception as e:
        print(f"❌ 错误: {e}")
        return False


def test_delete_npc(npc_id):
    """测试删除 NPC（清理测试数据）"""
    print_section(f"清理测试数据: DELETE /api/persona/{npc_id}/")
    
    try:
        response = requests.delete(f"{BASE_URL}/api/persona/{npc_id}/", timeout=5)
        print(f"状态码: {response.status_code}")
        if response.status_code == 200:
            print(f"✅ 已删除 {npc_id}")
        return True
    except Exception as e:
        print(f"⚠️  删除失败: {e}")
        return False


if __name__ == "__main__":
    print("=" * 60)
    print("  NPC Persona API 测试工具")
    print("=" * 60)
    print(f"服务器地址: {BASE_URL}")
    print(f"Python 版本: {sys.version}")
    
    # 1. 测试服务器连接
    if not test_get_personas():
        print("\n" + "=" * 60)
        print("❌ 服务器连接失败，测试终止")
        print("=" * 60)
        sys.exit(1)
    
    # 2. 测试创建英文 NPC
    english_success = test_create_npc_english()
    if english_success:
        test_get_specific_npc("tavern_owner")
    
    # 3. 测试创建中文 NPC
    chinese_success = test_create_npc_chinese()
    if chinese_success:
        test_get_specific_npc("tavern_owner_cn")
    
    # 4. 总结
    print("\n" + "=" * 60)
    print("  测试总结")
    print("=" * 60)
    print(f"英文 NPC: {'✅ 成功' if english_success else '❌ 失败'}")
    print(f"中文 NPC: {'✅ 成功' if chinese_success else '❌ 失败'}")
    
    # 5. 询问是否清理测试数据
    print("\n提示: 测试 NPC 已创建，可以在 npc_configs/ 目录查看")
    print("      如需清理测试数据，请手动删除或运行删除命令")
    
    print("=" * 60)
    print("测试完成")
    print("=" * 60)
