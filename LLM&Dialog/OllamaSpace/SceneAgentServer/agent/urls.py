"""
URL configuration for agent project.

The `urlpatterns` list routes URLs to views. For more information please see:
    https://docs.djangoproject.com/en/5.1/topics/http/urls/
Examples:
Function views
    1. Add an import:  from my_app import views
    2. Add a URL to urlpatterns:  path('', views.home, name='home')
Class-based views
    1. Add an import:  from other_app.views import Home
    2. Add a URL to urlpatterns:  path('', Home.as_view(), name='home')
Including another URLconf
    1. Import the include() function: from django.urls import include, path
    2. Add a URL to urlpatterns:  path('blog/', include('blog.urls'))
"""
from django.contrib import admin
from django.urls import include, path
from extrator import views as extratorView
from extrator.llm.npc_system.views import npc_urlpatterns

urlpatterns = [
    # 旧接口 (保持兼容)
    path('chat/', extratorView.chat),
    path('connect/', extratorView.connect),
    path('admin/', admin.site.urls),
    path("qa/", extratorView.qa_view, name="qa_view"),

    # 通用对话 (旧接口)
    path('general/connect/', extratorView.general_connect),
    path('general/chat/', extratorView.general_chat_view),
    path('general/clear/', extratorView.general_clear),
    
    # Test 对话 (旧接口)
    path('test/connect/', extratorView.test_connect),
    path('test/chat/', extratorView.test_chat_view),
    path('test/clear/', extratorView.test_clear),

    # ==================== 统一接口 (推荐) ====================
    # /api/<backend>/chat/  - 支持: general, test, test_chat, npc
    # /api/<backend>/connect/
    # /api/<backend>/clear/
    path('api/<str:backend>/chat/', extratorView.unified_chat),
    path('api/<str:backend>/connect/', extratorView.unified_connect),
    path('api/<str:backend>/clear/', extratorView.unified_clear),
    
    # 旧路由 (保持兼容)
    path('chat/<str:backend>/', extratorView.unified_chat),
    path('connect/<str:backend>/', extratorView.unified_connect),
    path('clear/<str:backend>/', extratorView.unified_clear),
    
    # 列出所有可用后端
    path('api/backends/', extratorView.list_chat_backends),
    path('backends/', extratorView.list_chat_backends),
    
    # ==================== Persona API - 人设管理 ====================
    # GET/POST /api/persona/
    # GET/PUT/DELETE /api/persona/<id>/
    path('api/persona/', extratorView.persona_list_or_create),
    path('api/persona/<str:persona_id>/', extratorView.persona_detail),
]
urlpatterns += npc_urlpatterns