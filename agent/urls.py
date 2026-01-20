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

# 调试：检查 general_connect 是否存在
print(f"[DEBUG] extrator.views 中的函数: {dir(extratorView)}")
print(f"[DEBUG] 是否有 general_connect: {hasattr(extratorView, 'general_connect')}")

from extrator.llm.npc_system.views import npc_urlpatterns

urlpatterns = [
    # 星球环境对话
    path('chat/', extratorView.chat),
    path('connect/', extratorView.connect),
    path('admin/', admin.site.urls),
    path("qa/", extratorView.qa_view, name="qa_view"),
    
    # 通用对话 (新增)
    path('general/connect/', extratorView.general_connect),   # 创建会话
    path('general/chat/', extratorView.general_chat_view),    # 对话
    path('general/clear/', extratorView.general_clear),       # 清除历史
]
urlpatterns += npc_urlpatterns  # 添加NPC系统路由
