"""
NPC系统 Django Views
提供HTTP API接口

API端点:
- POST /npc/connect/          创建会话
- POST /npc/chat/             与NPC对话
- GET  /npc/status/           获取所有NPC状态
- GET  /npc/status/<npc_id>/  获取单个NPC状态
- GET  /npc/relationship/<npc_id>/<player_id>/  获取好感度
- POST /npc/batch_dialogue/   批量生成背景对话
- POST /npc/remember/         添加NPC记忆
- GET  /npc/recall/           检索NPC记忆

使用方式:
1. 在 urls.py 中添加:
   from extrator.llm.npc_system.views import npc_urlpatterns
   urlpatterns += npc_urlpatterns

2. 或者手动添加各个视图
"""

import json
import uuid
from datetime import datetime
from functools import wraps

from django.http import JsonResponse
from django.views.decorators.csrf import csrf_exempt
from django.views.decorators.http import require_http_methods
from django.urls import path

# 全局NPC管理器实例
_npc_manager = None
_llm = None


def get_npc_manager():
    """获取或创建NPC管理器"""
    global _npc_manager, _llm
    
    if _npc_manager is None:
        from .npc_manager import NPCManager, NPCManagerConfig
        
        # 尝试初始化LLM
        try:
            from langchain_ollama import ChatOllama
            _llm = ChatOllama(model="qwen2.5", temperature=0.7)
            print("[NPC Views] LLM初始化成功")
        except Exception as e:
            print(f"[NPC Views] LLM初始化失败: {e}")
            _llm = None
        
        # 创建管理器
        config = NPCManagerConfig(
            data_dir="./npc_data",
            config_dir="./npc_configs",
            enable_batch_generation=(_llm is not None)
        )
        _npc_manager = NPCManager(config, _llm)
        print(f"[NPC Views] NPCManager初始化完成")
    
    return _npc_manager


def json_response(func):
    """JSON响应装饰器"""
    @wraps(func)
    def wrapper(request, *args, **kwargs):
        try:
            result = func(request, *args, **kwargs)
            if isinstance(result, JsonResponse):
                return result
            return JsonResponse(result, json_dumps_params={'ensure_ascii': False})
        except json.JSONDecodeError as e:
            return JsonResponse({
                "success": False,
                "error": f"JSON解析错误: {str(e)}"
            }, status=400)
        except Exception as e:
            import traceback
            traceback.print_exc()
            return JsonResponse({
                "success": False,
                "error": str(e)
            }, status=500)
    return wrapper


# ==================== API Views ====================

@csrf_exempt
@require_http_methods(["POST"])
@json_response
def npc_connect(request):
    """
    创建会话
    
    POST /npc/connect/
    Body: {"player_id": "xxx"} (可选)
    
    Response: {
        "success": true,
        "session_id": "xxx",
        "player_id": "xxx",
        "message": "欢迎来到NPC世界！"
    }
    """
    # 解析请求
    try:
        data = json.loads(request.body) if request.body else {}
    except:
        data = {}
    
    player_id = data.get("player_id", f"player_{uuid.uuid4().hex[:8]}")
    session_id = f"session_{player_id}_{datetime.now().strftime('%Y%m%d%H%M%S')}"
    
    # 设置Cookie
    response = JsonResponse({
        "success": True,
        "session_id": session_id,
        "player_id": player_id,
        "message": "欢迎来到NPC世界！你可以与各种NPC交流。"
    })
    response.set_cookie("npc_session_id", session_id, max_age=86400)
    response.set_cookie("npc_player_id", player_id, max_age=86400*30)
    
    return response


@csrf_exempt
@require_http_methods(["POST"])
@json_response
def npc_chat(request):
    """
    与NPC对话
    
    POST /npc/chat/
    Body: {
        "npc_id": "blacksmith",
        "message": "你好！",
        "player_id": "xxx" (可选，从cookie获取),
        "session_id": "xxx" (可选，从cookie获取),
        "extra_context": {} (可选)
    }
    
    Response: {
        "success": true,
        "npc_id": "blacksmith",
        "npc_name": "老铁匠",
        "reply": "哟，来了个冒险者...",
        "affinity": {
            "score": 5,
            "level": "陌生",
            "interaction_count": 1
        }
    }
    """
    data = json.loads(request.body)
    
    npc_id = data.get("npc_id")
    message = data.get("message")
    
    if not npc_id:
        return {"success": False, "error": "缺少npc_id参数"}
    if not message:
        return {"success": False, "error": "缺少message参数"}
    
    # 获取player_id和session_id
    player_id = data.get("player_id") or request.COOKIES.get("npc_player_id", "anonymous")
    session_id = data.get("session_id") or request.COOKIES.get("npc_session_id")
    extra_context = data.get("extra_context")
    
    # 获取管理器并对话
    manager = get_npc_manager()
    result = manager.chat(npc_id, player_id, message, session_id, extra_context)
    
    if result.get("success"):
        return {
            "success": True,
            "npc_id": npc_id,
            "npc_name": result.get("npc_name", npc_id),
            "reply": result.get("reply", ""),
            "affinity": result.get("affinity", {})
        }
    else:
        return {
            "success": False,
            "error": result.get("error", "对话失败")
        }


@csrf_exempt
@require_http_methods(["GET"])
@json_response
def npc_status_all(request):
    """
    获取所有NPC状态
    
    GET /npc/status/
    
    Response: {
        "success": true,
        "total_configs": 4,
        "loaded_npcs": 2,
        "npcs": {
            "blacksmith": {
                "name": "老铁匠",
                "role": "铁匠",
                "loaded": true,
                "background_dialogue": "..."
            }
        }
    }
    """
    manager = get_npc_manager()
    status = manager.get_all_status()
    status["success"] = True
    return status


@csrf_exempt
@require_http_methods(["GET"])
@json_response
def npc_status_single(request, npc_id):
    """
    获取单个NPC状态
    
    GET /npc/status/<npc_id>/
    Query: ?player_id=xxx (可选)
    
    Response: {
        "success": true,
        "npc_id": "blacksmith",
        "name": "老铁匠",
        "role": "铁匠",
        "memory_summary": {...},
        "relationship": {...}
    }
    """
    player_id = request.GET.get("player_id") or request.COOKIES.get("npc_player_id")
    
    manager = get_npc_manager()
    status = manager.get_npc_status(npc_id, player_id)
    
    if "error" in status:
        return {"success": False, "error": status["error"]}
    
    status["success"] = True
    return status


@csrf_exempt
@require_http_methods(["GET"])
@json_response
def npc_relationship(request, npc_id, player_id):
    """
    获取好感度信息
    
    GET /npc/relationship/<npc_id>/<player_id>/
    
    Response: {
        "success": true,
        "npc_id": "blacksmith",
        "player_id": "player_001",
        "score": 25,
        "level": "熟悉",
        "interaction_count": 5
    }
    """
    manager = get_npc_manager()
    npc = manager.get_npc(npc_id)
    
    if not npc:
        return {"success": False, "error": f"NPC不存在: {npc_id}"}
    
    affinity = npc.relationship.get_affinity(npc_id, player_id)
    
    return {
        "success": True,
        "npc_id": npc_id,
        "player_id": player_id,
        **affinity.to_dict()
    }


@csrf_exempt
@require_http_methods(["POST"])
@json_response
def npc_batch_dialogue(request):
    """
    批量生成NPC背景对话
    
    POST /npc/batch_dialogue/
    Body: {
        "context": "上午工作时间" (可选)
    }
    
    Response: {
        "success": true,
        "dialogues": {
            "老铁匠": "正在打磨一把新剑...",
            "商人": "整理着货架上的商品..."
        }
    }
    """
    try:
        data = json.loads(request.body) if request.body else {}
    except:
        data = {}
    
    context = data.get("context")
    
    manager = get_npc_manager()
    dialogues = manager.generate_batch_dialogues(context)
    
    return {
        "success": True,
        "dialogues": dialogues,
        "generated_at": datetime.now().isoformat()
    }


@csrf_exempt
@require_http_methods(["POST"])
@json_response
def npc_remember(request):
    """
    添加NPC记忆
    
    POST /npc/remember/
    Body: {
        "npc_id": "blacksmith",
        "content": "玩家完成了锻造任务",
        "memory_type": "episodic",
        "importance": 0.8,
        "user_id": "player_001" (可选)
    }
    
    Response: {
        "success": true,
        "memory_id": "em_xxx"
    }
    """
    data = json.loads(request.body)
    
    npc_id = data.get("npc_id")
    content = data.get("content")
    
    if not npc_id:
        return {"success": False, "error": "缺少npc_id参数"}
    if not content:
        return {"success": False, "error": "缺少content参数"}
    
    memory_type = data.get("memory_type", "episodic")
    importance = data.get("importance", 0.7)
    user_id = data.get("user_id", "")
    
    manager = get_npc_manager()
    npc = manager.get_npc(npc_id)
    
    if not npc:
        return {"success": False, "error": f"NPC不存在: {npc_id}"}
    
    memory_id = npc.remember(content, memory_type, importance, user_id)
    
    return {
        "success": True,
        "memory_id": memory_id
    }


@csrf_exempt
@require_http_methods(["GET"])
@json_response
def npc_recall(request):
    """
    检索NPC记忆
    
    GET /npc/recall/?npc_id=xxx&query=xxx&limit=5
    
    Response: {
        "success": true,
        "memories": [
            {
                "memory_id": "xxx",
                "content": "...",
                "memory_type": "episodic",
                "score": 0.85
            }
        ]
    }
    """
    npc_id = request.GET.get("npc_id")
    query = request.GET.get("query", "")
    limit = int(request.GET.get("limit", 5))
    user_id = request.GET.get("user_id", "")
    
    if not npc_id:
        return {"success": False, "error": "缺少npc_id参数"}
    
    manager = get_npc_manager()
    npc = manager.get_npc(npc_id)
    
    if not npc:
        return {"success": False, "error": f"NPC不存在: {npc_id}"}
    
    memories = npc.recall(query, limit, user_id)
    
    return {
        "success": True,
        "memories": [
            {
                "memory_id": m.memory_id,
                "content": m.content,
                "memory_type": m.memory_type,
                "score": m.combined_score,
                "timestamp": m.timestamp
            }
            for m in memories
        ]
    }


@csrf_exempt
@require_http_methods(["GET"])
@json_response
def npc_greeting(request, npc_id):
    """
    获取NPC问候语
    
    GET /npc/greeting/<npc_id>/?player_id=xxx
    
    Response: {
        "success": true,
        "npc_id": "blacksmith",
        "greeting": "哟，又来了个冒险者..."
    }
    """
    player_id = request.GET.get("player_id") or request.COOKIES.get("npc_player_id", "anonymous")
    
    manager = get_npc_manager()
    npc = manager.get_npc(npc_id)
    
    if not npc:
        return {"success": False, "error": f"NPC不存在: {npc_id}"}
    
    greeting = npc.get_greeting(player_id)
    
    return {
        "success": True,
        "npc_id": npc_id,
        "npc_name": npc.personality.name,
        "greeting": greeting
    }


@csrf_exempt
@require_http_methods(["POST"])
@json_response
def npc_register(request):
    """
    注册新NPC
    
    POST /npc/register/
    Body: {
        "npc_id": "new_npc",
        "personality": {
            "name": "新NPC",
            "role": "角色",
            "traits": ["特点1", "特点2"],
            "background": "背景故事",
            "speech_style": "说话风格"
        }
    }
    
    Response: {
        "success": true,
        "npc_id": "new_npc"
    }
    """
    data = json.loads(request.body)
    
    npc_id = data.get("npc_id")
    if not npc_id:
        return {"success": False, "error": "缺少npc_id参数"}
    
    manager = get_npc_manager()
    success = manager.register_npc(npc_id, data)
    
    if success:
        return {"success": True, "npc_id": npc_id}
    else:
        return {"success": False, "error": "注册失败，可能已达到NPC数量上限"}


@csrf_exempt
@require_http_methods(["POST"])
@json_response
def npc_clear(request):
    """
    清除对话历史
    
    POST /npc/clear/
    Body: {
        "session_id": "xxx" (可选)
    }
    
    Response: {
        "success": true,
        "message": "历史已清除"
    }
    """
    # 这里可以根据session_id清除特定会话的历史
    # 目前简单返回成功
    return {
        "success": True,
        "message": "对话历史已清除"
    }


# ==================== URL Patterns ====================

npc_urlpatterns = [
    path('npc/connect/', npc_connect, name='npc_connect'),
    path('npc/chat/', npc_chat, name='npc_chat'),
    path('npc/clear/', npc_clear, name='npc_clear'),
    path('npc/status/', npc_status_all, name='npc_status_all'),
    path('npc/status/<str:npc_id>/', npc_status_single, name='npc_status_single'),
    path('npc/relationship/<str:npc_id>/<str:player_id>/', npc_relationship, name='npc_relationship'),
    path('npc/batch_dialogue/', npc_batch_dialogue, name='npc_batch_dialogue'),
    path('npc/remember/', npc_remember, name='npc_remember'),
    path('npc/recall/', npc_recall, name='npc_recall'),
    path('npc/greeting/<str:npc_id>/', npc_greeting, name='npc_greeting'),
    path('npc/register/', npc_register, name='npc_register'),
]
