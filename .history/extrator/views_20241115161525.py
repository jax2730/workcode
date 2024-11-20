from django.shortcuts import render
from django.views.decorators.csrf import csrf_exempt
from django.http import HttpResponse, JsonResponse
import uuid
from .llm import llm, test_chat_z,test_chat

llm.init()
# test_chat_z.init()
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
    aimsg = llm.chat(prompt, session_id)
    # print(aimsg["msg"])
    return JsonResponse(aimsg)

@csrf_exempt
def connect(request):
    ipAddr = getIpAddr(request)
    session_id = "#".join([ipAddr, str(uuid.uuid4())])
    print(session_id)
    aimsg = llm.chat("你好", session_id)
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
        aimsg = llm.chat(prompt, session_id)
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