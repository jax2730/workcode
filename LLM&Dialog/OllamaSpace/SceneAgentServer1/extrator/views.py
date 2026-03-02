from django.shortcuts import render
from django.views.decorators.csrf import csrf_exempt
from django.http import HttpResponse, JsonResponse
import uuid
import json
from .llm import llm, test_chat_z,test_chat,llm_3_args,general_chat
from .llm.chat_interface import chat_interface, ChatRequest

# llm.init()
# llm_3_args.init()
test_chat_z.init()
def getIpAddr(request):
    ip_address = request.META.get('REMOTE_ADDR')
    forwarded_for = request.META.get('HTTP_X_FORWARDED_FOR')
    if forwarded_for:
        ip_address = forwarded_for.split(',')[0]
    return ip_address


# Create your views here.
@csrf_exempt
def chat(request):
    # session_id = request.session.get("agent_session_id", "")
    session_id = request.COOKIES.get("agent_session_id")
    # session_id = request.get_signed_cookie('agent_session_id')
    if not session_id:
        return JsonResponse({"success":0, "msg":"Missing parameter :session_id"})
    
    prompt = request.body.decode('utf-8')
    print("human: " + prompt)
    aimsg = llm_3_args.chat(prompt, session_id)
    # print(aimsg["msg"])
    return JsonResponse(aimsg)

@csrf_exempt
def connect(request):
    ipAddr = getIpAddr(request)
    session_id = "#".join([ipAddr, str(uuid.uuid4())])
    print(session_id)
    aimsg = llm_3_args.chat("你好", session_id)
    response = JsonResponse(aimsg)
    response.set_cookie("agent_session_id", session_id)
    return response

session_history = {}
def qa_view(request):
    # 确保使用同一个 session_id
    session_id = request.COOKIES.get("agent_session_id")
    if not session_id:
        session_id = str(uuid.uuid4())

    # 初始化会话历史（如果不存在）
    if session_id not in session_history:
        session_history[session_id] = []

    if request.method == "POST":
        # 获取用户输入
        prompt = request.POST.get("prompt", "")

        # 调用 LLM 模型进行问答
        aimsg = test_chat_z.chat(prompt, session_id)
        response = aimsg.get("msg", "无法获取回复")

        # 更新会话历史
        session_history[session_id].append({"role": "user", "content": prompt})
        session_history[session_id].append({"role": "ai", "content": response})

        # 渲染模板，显示问答结果
        response_obj = render(request, "qa.html", {
            "prompt": prompt,
            "response": response,
            "history": session_history[session_id]
        })

        # 设置 cookie 保存 session_id
        response_obj.set_cookie("agent_session_id", session_id)
        return response_obj
    else:
        # GET 请求返回空的模板
        return render(request, "qa.html", {
            "prompt": "",
            "response": "",
            "history": session_history.get(session_id, [])
        })
    
    
@csrf_exempt
def general_connect(request):
    """创建通用对话会话"""
    ipAddr = getIpAddr(request)
    response_data = chat_interface.handle_connect("general", ipAddr)
    response = JsonResponse(response_data.to_dict())
    response.set_cookie("general_session_id", response_data.session_id)
    print(f"[GeneralChat] New session: {response_data.session_id}")
    return response


@csrf_exempt
def general_chat_view(request):
    """通用对话接口 - 支持JSON和纯文本格式"""
    cookie_session_id = request.COOKIES.get("general_session_id")
    
    response_data = chat_interface.handle_chat("general", request.body, cookie_session_id)
    print(f"[GeneralChat] human: {response_data.reply[:50]}...")
    
    result = JsonResponse(response_data.to_dict())
    # 如果session_id变化，更新cookie
    if response_data.session_id != cookie_session_id:
        result.set_cookie("general_session_id", response_data.session_id)
    return result


@csrf_exempt
def general_clear(request):
    """清除通用对话历史"""
    session_id = request.COOKIES.get("general_session_id")
    if not session_id:
        return JsonResponse({"success": False, "reply": "没有活动会话", "session_id": ""})
    
    response_data = chat_interface.handle_clear("general", session_id)
    return JsonResponse(response_data.to_dict())


# ==================== Test 对话接口 ====================

@csrf_exempt
def test_connect(request):
    """创建Test对话会话"""
    ipAddr = getIpAddr(request)
    response_data = chat_interface.handle_connect("test", ipAddr)
    response = JsonResponse(response_data.to_dict())
    response.set_cookie("test_session_id", response_data.session_id)
    print(f"[TestChat] New session: {response_data.session_id}")
    return response


@csrf_exempt
def test_chat_view(request):
    """Test对话接口"""
    cookie_session_id = request.COOKIES.get("test_session_id")
    
    response_data = chat_interface.handle_chat("test", request.body, cookie_session_id)
    
    result = JsonResponse(response_data.to_dict())
    if response_data.session_id != cookie_session_id:
        result.set_cookie("test_session_id", response_data.session_id)
    return result


@csrf_exempt
def test_clear(request):
    """清除Test对话历史"""
    session_id = request.COOKIES.get("test_session_id")
    if not session_id:
        return JsonResponse({"success": False, "reply": "没有活动会话", "session_id": ""})
    
    response_data = chat_interface.handle_clear("test", session_id)
    return JsonResponse(response_data.to_dict())


# ==================== 统一对话接口 (推荐) ====================

@csrf_exempt
def unified_chat(request, backend: str):
    """
    统一对话接口 - 支持所有后端
    
    URL: /chat/<backend>/
    例如: /chat/general/, /chat/test/, /chat/test_chat/, /chat/npc/
    
    请求格式 (POST JSON):
    {
        "message": "用户消息",
        "session_id": "会话ID (可选)",
        "persona": "人设名称 (可选，仅general支持)",
        "npc_id": "NPC标识 (仅npc后端)",
        "player_id": "玩家ID (仅npc后端)"
    }
    
    响应格式:
    {
        "success": true/false,
        "reply": "AI回复",
        "session_id": "会话ID",
        "response_time": 0.123,
        "backend": "general"
    }
    """
    # NPC后端: 委托给npc_system专用views
    if backend == "npc":
        try:
            from .llm.npc_system.views import npc_chat
            return npc_chat(request)
        except Exception as e:
            import traceback
            traceback.print_exc()
            return JsonResponse({"success": False, "error": f"NPC后端错误: {str(e)}"}, status=500)

    # 尝试从cookie获取session_id
    cookie_session_id = request.COOKIES.get(f"{backend}_session_id")
    
    response_data = chat_interface.handle_chat(backend, request.body, cookie_session_id)
    result = JsonResponse(response_data.to_dict())
    
    # 设置session_id到cookie
    if response_data.session_id and response_data.session_id != cookie_session_id:
        result.set_cookie(f"{backend}_session_id", response_data.session_id)
    
    return result


@csrf_exempt
def unified_connect(request, backend: str):
    """
    统一连接接口
    
    URL: /connect/<backend>/

    persona : 指定npc
    """
    # NPC后端: 委托给npc_system专用views
    if backend == "npc":
        try:
            from .llm.npc_system.views import npc_connect
            return npc_connect(request)
        except Exception as e:
            import traceback
            traceback.print_exc()
            return JsonResponse({"success": False, "error": f"NPC连接错误: {str(e)}"}, status=500)

    ipAddr = getIpAddr(request)
    persona = None
    if request.body:
        try:
            body_str = request.body.decode('utf-8')
            print(f"[unified_connect] 收到请求体: {body_str}")
            data = json.loads(body_str)
            persona = data.get("persona")
            print(f"[unified_connect] 解析到 persona: {persona}")
        except Exception as e:
            print(f"[unified_connect] 解析请求体失败: {e}")
    
    print(f"[unified_connect] 调用 handle_connect: backend={backend}, ip={ipAddr}, persona={persona}")
    response_data = chat_interface.handle_connect(backend, ipAddr, persona)
    
    result = JsonResponse(response_data.to_dict())
    result.set_cookie(f"{backend}_session_id", response_data.session_id)
    
    print(f"[{backend}] New session: {response_data.session_id}, persona:{persona}")
    return result


@csrf_exempt
def unified_clear(request, backend: str):
    """
    统一清除历史接口
    
    URL: /clear/<backend>/
    """
    # NPC后端: 委托给npc_system专用views
    if backend == "npc":
        try:
            from .llm.npc_system.views import npc_clear
            return npc_clear(request)
        except Exception as e:
            return JsonResponse({"success": False, "error": f"NPC清除错误: {str(e)}"}, status=500)

    session_id = request.COOKIES.get(f"{backend}_session_id")
    
    # 也支持从请求体获取session_id
    if not session_id:
        try:
            data = json.loads(request.body.decode('utf-8'))
            session_id = data.get("session_id")
        except:
            pass
    
    if not session_id:
        return JsonResponse({"success": False, "reply": "没有活动会话", "session_id": ""})
    
    response_data = chat_interface.handle_clear(backend, session_id)
    return JsonResponse(response_data.to_dict())


@csrf_exempt
def list_chat_backends(request):
    """列出所有可用的对话后端"""
    from .llm.chat_interface import list_backends
    return JsonResponse({
        "success": True,
        "backends": list_backends()
    })


# ==================== Persona API - 人设管理接口 ====================

from .llm.persona_api import persona_api

def parse_request_data(request):
    """
    解析请求数据，支持多种格式：
    1. JSON (Content-Type: application/json)
    2. 表单数据 (Content-Type: application/x-www-form-urlencoded)
    3. URL 参数 (GET 或 POST 请求的查询字符串)
    4. 原始 body (key=value&key2=value2 格式)
    """
    data = {}
    
    # 1. 尝试解析 JSON (仅当明确指定 Content-Type 为 JSON 时)
    if request.content_type and 'application/json' in request.content_type:
        try:
            data = json.loads(request.body.decode('utf-8'))
            return data, None
        except json.JSONDecodeError as e:
            return None, f"无效的 JSON 格式: {str(e)}"
    
    # 2. 尝试解析表单数据 (POST/PUT) - Django 自动解析的表单
    if request.method in ['POST', 'PUT'] and request.POST:
        # 获取所有 POST 参数
        for key in request.POST.keys():
            value = request.POST.get(key)
            # 尝试解析 JSON 字符串
            if value and (value.startswith('{') or value.startswith('[')):
                try:
                    data[key] = json.loads(value)
                except:
                    data[key] = value
            else:
                data[key] = value
        
        if data:
            return data, None
    
    # 3. 尝试解析 URL 参数 (GET 请求的查询字符串，或 POST 请求的 URL 参数)
    if request.GET:
        for key in request.GET.keys():
            value = request.GET.get(key)
            # 尝试解析 JSON 字符串
            if value and (value.startswith('{') or value.startswith('[')):
                try:
                    data[key] = json.loads(value)
                except:
                    data[key] = value
            else:
                data[key] = value
        
        if data:
            return data, None
    
    # 4. 尝试解析原始 body（key=value&key2=value2 格式，或纯 JSON）
    if request.body:
        try:
            body_text = request.body.decode('utf-8').strip()
            
            # 4.1 如果看起来像 JSON，尝试解析
            if body_text.startswith('{') or body_text.startswith('['):
                try:
                    data = json.loads(body_text)
                    return data, None
                except json.JSONDecodeError:
                    pass
            
            # 4.2 如果是 key=value 格式
            if '=' in body_text:
                from urllib.parse import parse_qs, unquote
                # 使用 parse_qs 解析 URL 编码的表单数据
                parsed = parse_qs(body_text, keep_blank_values=True)
                for key, values in parsed.items():
                    # parse_qs 返回列表，取第一个值
                    value = unquote(values[0]) if values else ""
                    # 尝试解析 JSON 字符串
                    if value and (value.startswith('{') or value.startswith('[')):
                        try:
                            data[key] = json.loads(value)
                        except:
                            data[key] = value
                    else:
                        data[key] = value
                if data:
                    return data, None
        except:
            pass
    
    return data, None


def normalize_persona_data(data: dict) -> dict:
    """
    规范化人设数据，支持扁平格式自动组装为嵌套格式
    
    扁平格式（表单/URL参数）:
        planner_id=xxx, role_type=xxx, name=xxx, role=xxx, greeting=xxx, traits=xxx
    
    会自动转换为嵌套格式:
        {
            "planner_id": "xxx",
            "role_type": "xxx", 
            "personality": {
                "name": "xxx",
                "role": "xxx",
                "greeting": "xxx",
                "traits": ["xxx", "xxx"]
            }
        }
    """
    if not data:
        return data
    
    # 如果已经有 personality 对象，直接返回
    if "personality" in data and isinstance(data["personality"], dict):
        return data
    
    # personality 相关的字段
    personality_fields = ["name", "role", "traits", "background", "speech_style", "knowledge", "greeting"]
    
    # 检查是否有扁平的 personality 字段
    has_flat_fields = any(field in data for field in personality_fields)
    
    if has_flat_fields:
        # 构建 personality 对象
        personality = {}
        for field in personality_fields:
            if field in data:
                value = data.pop(field)
                # 处理数组字段（traits, knowledge）
                if field in ["traits", "knowledge"]:
                    if isinstance(value, str):
                        # 支持逗号分隔的字符串转数组
                        value = [v.strip() for v in value.split(",") if v.strip()]
                    elif not isinstance(value, list):
                        value = [value]
                personality[field] = value
        
        data["personality"] = personality
    
    return data


@csrf_exempt
def persona_list_or_create(request):
    """
    GET  /api/persona/  - 列出所有人设
    POST /api/persona/  - 创建新人设
    
    支持多种传输格式：
    1. JSON: Content-Type: application/json
    2. 表单: Content-Type: application/x-www-form-urlencoded
    3. URL参数: ?planner_id=xxx&role_type=xxx&name=xxx...
    """
    if request.method == 'GET':
        response = persona_api.list_personas()
    elif request.method == 'POST':
        # 解析请求数据（支持多种格式）
        data, error = parse_request_data(request)
        if error:
            return JsonResponse({
                "success": False, "error": error, "code": 400
            }, status=400)
        
        if not data:
            return JsonResponse({
                "success": False, "error": "请求数据为空", "code": 400
            }, status=400)
        
        # 规范化数据：扁平格式自动组装为嵌套格式
        data = normalize_persona_data(data)
        
        response = persona_api.create_persona(data)
    else:
        return JsonResponse({
            "success": False, "error": "不支持的请求方法", "code": 405
        }, status=405)
    
    return JsonResponse(response.to_dict(), status=response.code)


@csrf_exempt
def persona_detail(request, persona_id: str):
    """
    GET    /api/persona/<id>/  - 获取指定人设
    PUT    /api/persona/<id>/  - 更新人设
    DELETE /api/persona/<id>/  - 删除人设
    
    支持多种传输格式（PUT）：
    1. JSON: Content-Type: application/json
    2. 表单: Content-Type: application/x-www-form-urlencoded
    """
    if request.method == 'GET':
        response = persona_api.get_persona(persona_id)
    elif request.method == 'PUT':
        # 解析请求数据（支持多种格式）
        data, error = parse_request_data(request)
        if error:
            return JsonResponse({
                "success": False, "error": error, "code": 400
            }, status=400)
        # 规范化数据：扁平格式自动组装为嵌套格式
        data = normalize_persona_data(data)
        response = persona_api.update_persona(persona_id, data)
    elif request.method == 'DELETE':
        response = persona_api.delete_persona(persona_id)
    else:
        return JsonResponse({
            "success": False, "error": "不支持的请求方法", "code": 405
        }, status=405)
    
    return JsonResponse(response.to_dict(), status=response.code)
