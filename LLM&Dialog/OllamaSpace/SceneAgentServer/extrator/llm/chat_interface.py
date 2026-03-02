"""
统一对话接口处理模块 - Chat Interface

提供统一的请求解析和响应格式，支持多种对话后端切换。

支持的后端:
- general: 通用对话 (general_chat.py)
- test: 测试对话 (test.py)
- test_chat: 环境提取对话 (test_chat.py)

请求格式 (JSON):
{
    "message": "用户消息",
    "session_id": "会话ID (可选，会自动生成)",
    "persona": "人设名称 (可选，仅general支持)"
}

响应格式 (JSON):
{
    "success": true/false,
    "reply": "AI回复",
    "session_id": "会话ID",
    "response_time": 0.0,
    "backend": "当前后端"
}

使用方式:
    from extrator.llm.chat_interface import ChatInterface
    
    interface = ChatInterface()
    result = interface.handle_request("general", request_body, session_id_cookie)
"""

import json
import uuid
import time
from typing import Optional, Callable, Dict, Any
from dataclasses import dataclass, asdict


@dataclass
class ChatRequest:
    """对话请求"""
    message: str
    session_id: str
    persona: Optional[str] = None
    npc_id: Optional[str] = None      # NPC后端: NPC标识
    player_id: Optional[str] = None   # NPC后端: 玩家标识
    
    @classmethod
    def from_json(cls, body: bytes, cookie_session_id: str = None) -> "ChatRequest":
        """
        从请求体解析
        
        支持两种格式:
        1. JSON: {"message": "xxx", "session_id": "xxx", "persona": "xxx"}
        2. 纯文本: 直接作为message
        """
        text = body.decode('utf-8').strip()
        
        # 尝试JSON解析
        try:
            data = json.loads(text)
            if isinstance(data, dict):
                return cls(
                    message=data.get("message", text),
                    session_id=data.get("session_id") or cookie_session_id or str(uuid.uuid4()),
                    persona=data.get("persona"),
                    npc_id=data.get("npc_id"),
                    player_id=data.get("player_id")
                )
        except json.JSONDecodeError:
            pass
        
        # 纯文本格式
        return cls(
            message=text,
            session_id=cookie_session_id or str(uuid.uuid4()),
            persona=None
        )


@dataclass
class ChatResponse:
    """对话响应"""
    success: bool
    reply: str
    session_id: str
    response_time: float = 0.0
    backend: str = ""
    persona: str = ""
    data: Any = None  # 额外数据 (如工具调用结果)
    
    def to_dict(self) -> dict:
        """转换为字典 (用于JsonResponse)"""
        result = {
            "success": self.success,
            "reply": self.reply,
            "session_id": self.session_id,
            "response_time": round(self.response_time, 3),
            "backend": self.backend
        }
        if self.persona:
            result["persona"] = self.persona
        if self.data is not None:
            result["data"] = self.data
        return result
    
    def to_legacy_dict(self) -> dict:
        """兼容旧格式的字典 (success用1/0, msg代替reply)"""
        return {
            "success": 1 if self.success else 0,
            "msg": self.reply,
            "response_time": round(self.response_time, 3)
        }


class ChatInterface:
    """
    统一对话接口
    
    支持多种后端，统一请求/响应格式
    """
    
    def __init__(self):
        self._backends: Dict[str, Callable] = {}
        self._connect_handlers: Dict[str, Callable] = {}
        self._clear_handlers: Dict[str, Callable] = {}
        self._register_default_backends()
    
    def _register_default_backends(self):
        """注册默认后端"""
        # General Chat
        self.register_backend(
            "general",
            chat_handler=self._general_chat,
            connect_handler=self._general_connect,
            clear_handler=self._general_clear
        )
        
        # Test (简单对话)
        self.register_backend(
            "test",
            chat_handler=self._test_chat,
            connect_handler=self._test_connect,
            clear_handler=self._test_clear
        )
        
        # Test Chat (环境提取)
        self.register_backend(
            "test_chat",
            chat_handler=self._test_chat_env,
            connect_handler=self._test_chat_connect,
            clear_handler=None
        )
        
        # NPC 对话 (多NPC系统，复用general_chat的persona机制)
        self.register_backend(
            "npc",
            chat_handler=self._npc_chat,
            connect_handler=self._npc_connect,
            clear_handler=self._npc_clear
        )
    
    def register_backend(
        self, 
        name: str, 
        chat_handler: Callable,
        connect_handler: Callable = None,
        clear_handler: Callable = None
    ):
        """
        注册新的对话后端
        
        Args:
            name: 后端名称
            chat_handler: 对话处理函数 (request: ChatRequest) -> ChatResponse
            connect_handler: 连接处理函数 (ip: str) -> ChatResponse
            clear_handler: 清除历史函数 (session_id: str) -> ChatResponse
        """
        self._backends[name] = chat_handler
        if connect_handler:
            self._connect_handlers[name] = connect_handler
        if clear_handler:
            self._clear_handlers[name] = clear_handler
    
    def list_backends(self) -> list:
        """列出所有可用后端"""
        return list(self._backends.keys())
    
    def handle_chat(
        self, 
        backend: str, 
        body: bytes, 
        cookie_session_id: str = None
    ) -> ChatResponse:
        """
        处理对话请求
        
        Args:
            backend: 后端名称 (general, test, test_chat)
            body: 请求体 (JSON或纯文本)
            cookie_session_id: Cookie中的session_id (可选)
        """
        if backend not in self._backends:
            return ChatResponse(
                success=False,
                reply=f"未知后端: {backend}，可用: {self.list_backends()}",
                session_id=cookie_session_id or "",
                backend=backend
            )
        
        # 解析请求
        request = ChatRequest.from_json(body, cookie_session_id)
        
        # 调用后端
        try:
            response = self._backends[backend](request)
            response.backend = backend
            return response
        except Exception as e:
            return ChatResponse(
                success=False,
                reply=f"处理出错: {str(e)}",
                session_id=request.session_id,
                backend=backend
            )
    
    def handle_connect(self, backend: str, ip: str, persona: str = None) -> ChatResponse:
        """
        处理连接请求
        
        Args:
            backend: 后端名称
            ip: 客户端IP
            persona: 指定的角色 npc_id（可选）
        """
        print(f"[handle_connect] backend={backend}, ip={ip}, persona={persona}")
        
        if backend not in self._connect_handlers:
            # 默认生成session_id
            session_id = f"{backend}#{ip}#{uuid.uuid4()}"
            return ChatResponse(
                success=True,
                reply="欢迎！",
                session_id=session_id,
                backend=backend,
                persona=persona or ""
            )
        
        # 调用后端的连接处理器
        handler = self._connect_handlers[backend]
        
        # 对于 general 后端，始终传递 persona
        if backend == "general":
            return handler(ip, persona)
        
        # 其他后端检查是否支持 persona 参数
        import inspect
        sig = inspect.signature(handler)
        # 绑定方法的 self 不计入，所以检查是否有 2 个以上参数（包括 self）
        params = [p for p in sig.parameters.keys() if p != 'self']
        if len(params) >= 2:
            return handler(ip, persona)
        else:
            return handler(ip)
    
    def handle_clear(self, backend: str, session_id: str) -> ChatResponse:
        """处理清除历史请求"""
        if backend not in self._clear_handlers or not self._clear_handlers[backend]:
            return ChatResponse(
                success=False,
                reply="该后端不支持清除历史",
                session_id=session_id,
                backend=backend
            )
        
        return self._clear_handlers[backend](session_id)
    
    # ==================== General Chat 后端 ====================
    
    def _general_chat(self, request: ChatRequest) -> ChatResponse:
        """通用对话"""
        from . import general_chat
        
        start_time = time.time()
        result = general_chat.chat(
            request.message, 
            request.session_id,
            persona=request.persona
        )
        
        return ChatResponse(
            success=result.get("success", 0) == 1,
            reply=result.get("msg", ""),
            session_id=request.session_id,
            response_time=time.time() - start_time,
            persona=result.get("persona", "")
        )
    
    def _general_connect(self, ip: str, persona: str = None) -> ChatResponse:
        """
        通用对话连接
        
        Args:
            ip: 客户端IP
            persona: 指定的角色 npc_id（可选）
        """
        from . import general_chat
        
        print(f"[_general_connect] ip={ip}, persona={persona}")
        
        session_id = f"general#{ip}#{uuid.uuid4()}"
        
        # 如果指定了 persona，使用该角色的问候语进行初始化
        if persona:
            print(f"[_general_connect] 使用指定角色: {persona}")
            result = general_chat.chat("你好", session_id, persona=persona)
        else:
            print(f"[_general_connect] 未指定角色，使用默认")
            result = general_chat.chat("你好", session_id)
        
        print(f"[_general_connect] result persona: {result.get('persona', 'N/A')}")
        
        return ChatResponse(
            success=result.get("success", 0) == 1,
            reply=result.get("msg", ""),
            session_id=session_id,
            persona=result.get("persona", persona or "")
        )
    
    def _general_clear(self, session_id: str) -> ChatResponse:
        """通用对话清除历史"""
        from . import general_chat
        
        result = general_chat.clear_history(session_id)
        return ChatResponse(
            success=result.get("success", 0) == 1,
            reply=result.get("msg", ""),
            session_id=session_id
        )
    
    # ==================== Test 后端 ====================
    
    def _test_chat(self, request: ChatRequest) -> ChatResponse:
        """测试对话"""
        from . import test
        
        start_time = time.time()
        result = test.chat(request.message, request.session_id)
        
        return ChatResponse(
            success=result.get("success", 0) == 1,
            reply=result.get("msg", ""),
            session_id=request.session_id,
            response_time=time.time() - start_time
        )
    
    def _test_connect(self, ip: str) -> ChatResponse:
        """测试对话连接"""
        session_id = f"test#{ip}#{uuid.uuid4()}"
        return ChatResponse(
            success=True,
            reply="欢迎使用测试对话！",
            session_id=session_id
        )
    
    def _test_clear(self, session_id: str) -> ChatResponse:
        """测试对话清除历史"""
        from . import test
        
        result = test.clear_history(session_id)
        return ChatResponse(
            success=result.get("success", 0) == 1,
            reply=result.get("msg", ""),
            session_id=session_id
        )
    
    # ==================== Test Chat (环境提取) 后端 ====================
    
    def _test_chat_env(self, request: ChatRequest) -> ChatResponse:
        """环境提取对话"""
        from . import test_chat
        
        start_time = time.time()
        result = test_chat.chat(request.message, request.session_id)
        
        return ChatResponse(
            success=result.get("success", 0) == 1,
            reply=result.get("msg", ""),
            session_id=request.session_id,
            response_time=time.time() - start_time,
            data=result.get("data")
        )
    
    def _test_chat_connect(self, ip: str) -> ChatResponse:
        """环境提取对话连接"""
        from . import test_chat
        
        test_chat.init()  # 初始化数据
        session_id = f"test_chat#{ip}#{uuid.uuid4()}"
        
        return ChatResponse(
            success=True,
            reply="欢迎！请描述你想要创建的星球环境。",
            session_id=session_id
        )
    
    # ==================== NPC 对话后端 ====================
    
    def _npc_chat(self, request: ChatRequest) -> ChatResponse:
        """
        NPC对话 - 利用 general_chat 的 persona 机制
        npc_id 即 persona (对应 npc_configs/<npc_id>.json)
        """
        from . import general_chat
        
        start_time = time.time()
        npc_id = request.npc_id or request.persona or "blacksmith"
        
        result = general_chat.chat(
            request.message,
            request.session_id,
            persona=npc_id
        )
        
        # 从配置获取NPC名称
        npc_name = npc_id
        config = general_chat.load_persona_config(npc_id)
        if config:
            p = config.get("personality", config)
            npc_name = p.get("name", npc_id)
        
        return ChatResponse(
            success=result.get("success", 0) == 1,
            reply=result.get("msg", ""),
            session_id=request.session_id,
            response_time=time.time() - start_time,
            persona=npc_id,
            data={"npc_id": npc_id, "npc_name": npc_name}
        )
    
    def _npc_connect(self, ip: str, persona: str = None) -> ChatResponse:
        """
        NPC对话连接
        persona 参数即 npc_id
        """
        from . import general_chat
        
        npc_id = persona or "blacksmith"
        session_id = f"npc#{ip}#{uuid.uuid4()}"
        
        # 获取NPC问候语
        npc_name = npc_id
        greeting = "你好！"
        config = general_chat.load_persona_config(npc_id)
        if config:
            p = config.get("personality", config)
            npc_name = p.get("name", npc_id)
            greeting = p.get("greeting", "")
            if not greeting:
                # 如果没有预设问候语，用LLM生成
                result = general_chat.chat("你好", session_id, persona=npc_id)
                greeting = result.get("msg", "你好！")
        
        return ChatResponse(
            success=True,
            reply=greeting,
            session_id=session_id,
            persona=npc_id,
            data={"npc_id": npc_id, "npc_name": npc_name}
        )
    
    def _npc_clear(self, session_id: str) -> ChatResponse:
        """NPC对话清除历史"""
        from . import general_chat
        
        result = general_chat.clear_history(session_id)
        return ChatResponse(
            success=result.get("success", 0) == 1,
            reply=result.get("msg", ""),
            session_id=session_id
        )


# ==================== 全局实例 ====================
chat_interface = ChatInterface()


# ==================== 便捷函数 ====================
def handle_chat(backend: str, body: bytes, session_id: str = None) -> dict:
    """处理对话请求 (返回字典)"""
    response = chat_interface.handle_chat(backend, body, session_id)
    return response.to_dict()


def handle_connect(backend: str, ip: str) -> dict:
    """处理连接请求 (返回字典)"""
    response = chat_interface.handle_connect(backend, ip)
    return response.to_dict()


def handle_clear(backend: str, session_id: str) -> dict:
    """处理清除历史请求 (返回字典)"""
    response = chat_interface.handle_clear(backend, session_id)
    return response.to_dict()


def list_backends() -> list:
    """列出可用后端"""
    return chat_interface.list_backends()
