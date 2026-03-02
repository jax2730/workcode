#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Persona API 测试脚本
测试新规范：planner_id + role_type 自动生成 npc_id
"""

import requests
import json

BASE_URL = "http://192.168.5.189:8000/api/persona/"

def test_create_with_new_spec():
    """测试使用新规范创建角色"""
    print("\n=== 测试1: 使用新规范创建角色 ===")
    
    data = {
        "planner_id": "test001",
        "role_type": "tavern_owner_test",
        "personality": {
            "name": "测试老板",
            "role": "酒馆老板",
            "age": 45,
            "traits": ["热情", "健谈"],
            "greeting": "欢迎测试！"
        }
    }
    
    response = requests.post(BASE_URL, json=data)
    print(f"状态码: {response.status_code}")
    print(f"响应: {json.dumps(response.json(), ensure_ascii=False, indent=2)}")
    
    if response.status_code == 201:
        npc_id = response.json()["data"]["npc_id"]
        print(f"✅ 创建成功！生成的 npc_id: {npc_id}")
        return npc_id
    else:
        print("❌ 创建失败")
        return None


def test_duplicate_creation(planner_id, role_type):
    """测试重复创建（应该被拒绝）"""
    print("\n=== 测试2: 同一策划重复创建相同角色（应该失败） ===")
    
    data = {
        "planner_id": planner_id,
        "role_type": role_type,
        "personality": {
            "name": "重复测试",
            "role": "测试角色"
        }
    }
    
    response = requests.post(BASE_URL, json=data)
    print(f"状态码: {response.status_code}")
    print(f"响应: {json.dumps(response.json(), ensure_ascii=False, indent=2)}")
    
    if response.status_code == 409:
        print("✅ 正确拒绝了重复创建")
    else:
        print("❌ 应该返回 409 错误")


def test_different_planner_same_role():
    """测试不同策划创建相同角色类型（应该成功）"""
    print("\n=== 测试3: 不同策划创建相同角色类型（应该成功） ===")
    
    data = {
        "planner_id": "test002",
        "role_type": "tavern_owner_test",
        "personality": {
            "name": "另一个老板",
            "role": "酒馆老板"
        }
    }
    
    response = requests.post(BASE_URL, json=data)
    print(f"状态码: {response.status_code}")
    print(f"响应: {json.dumps(response.json(), ensure_ascii=False, indent=2)}")
    
    if response.status_code == 201:
        npc_id = response.json()["data"]["npc_id"]
        print(f"✅ 创建成功！生成的 npc_id: {npc_id}")
        return npc_id
    else:
        print("❌ 创建失败")
        return None


def test_get_persona(npc_id):
    """测试获取角色"""
    print(f"\n=== 测试4: 获取角色 {npc_id} ===")
    
    response = requests.get(f"{BASE_URL}{npc_id}/")
    print(f"状态码: {response.status_code}")
    
    if response.status_code == 200:
        data = response.json()["data"]
        print(f"✅ 获取成功")
        print(f"  - npc_id: {data['npc_id']}")
        print(f"  - planner_id: {data.get('planner_id', 'N/A')}")
        print(f"  - role_type: {data.get('role_type', 'N/A')}")
        print(f"  - name: {data['personality']['name']}")
    else:
        print("❌ 获取失败")


def test_list_personas():
    """测试列出所有角色"""
    print("\n=== 测试5: 列出所有角色 ===")
    
    response = requests.get(BASE_URL)
    print(f"状态码: {response.status_code}")
    
    if response.status_code == 200:
        personas = response.json()["data"]["personas"]
        print(f"✅ 共有 {len(personas)} 个角色")
        
        # 显示测试创建的角色
        test_personas = [p for p in personas if p.get("planner_id", "").startswith("test")]
        if test_personas:
            print("\n测试创建的角色:")
            for p in test_personas:
                print(f"  - {p['npc_id']}: {p['name']} (策划: {p.get('planner_id', 'N/A')})")
    else:
        print("❌ 列表获取失败")


def test_update_persona(npc_id):
    """测试更新角色"""
    print(f"\n=== 测试6: 更新角色 {npc_id} ===")
    
    data = {
        "personality": {
            "greeting": "更新后的问候语！"
        }
    }
    
    response = requests.put(f"{BASE_URL}{npc_id}/", json=data)
    print(f"状态码: {response.status_code}")
    
    if response.status_code == 200:
        print("✅ 更新成功")
        greeting = response.json()["data"]["personality"]["greeting"]
        print(f"  新问候语: {greeting}")
    else:
        print("❌ 更新失败")


def test_delete_persona(npc_id):
    """测试删除角色"""
    print(f"\n=== 测试7: 删除角色 {npc_id} ===")
    
    response = requests.delete(f"{BASE_URL}{npc_id}/")
    print(f"状态码: {response.status_code}")
    
    if response.status_code == 200:
        print("✅ 删除成功")
    else:
        print("❌ 删除失败")


def cleanup():
    """清理测试数据"""
    print("\n=== 清理测试数据 ===")
    
    response = requests.get(BASE_URL)
    if response.status_code == 200:
        personas = response.json()["data"]["personas"]
        test_personas = [p for p in personas if p.get("planner_id", "").startswith("test")]
        
        for p in test_personas:
            npc_id = p["npc_id"]
            print(f"删除: {npc_id}")
            requests.delete(f"{BASE_URL}{npc_id}/")
        
        print(f"✅ 清理完成，删除了 {len(test_personas)} 个测试角色")


if __name__ == "__main__":
    print("=" * 60)
    print("Persona API 新规范测试")
    print("=" * 60)
    
    try:
        # 测试1: 创建角色（新规范）
        npc_id_1 = test_create_with_new_spec()
        
        if npc_id_1:
            # 测试2: 重复创建（应该失败）
            test_duplicate_creation("test001", "tavern_owner_test")
            
            # 测试3: 不同策划创建相同角色类型
            npc_id_2 = test_different_planner_same_role()
            
            # 测试4: 获取角色
            test_get_persona(npc_id_1)
            
            # 测试5: 列出所有角色
            test_list_personas()
            
            # 测试6: 更新角色
            test_update_persona(npc_id_1)
            
            # 测试7: 删除角色
            if npc_id_2:
                test_delete_persona(npc_id_2)
        
        print("\n" + "=" * 60)
        print("测试完成！")
        print("=" * 60)
        
        # 询问是否清理
        cleanup_choice = input("\n是否清理所有测试数据？(y/n): ")
        if cleanup_choice.lower() == 'y':
            cleanup()
        
    except Exception as e:
        print(f"\n❌ 测试过程中出错: {e}")
        import traceback
        traceback.print_exc()
