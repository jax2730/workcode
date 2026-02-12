"""
通用对话模块 - General Chat
用于普通的自由对话，支持从 npc_configs/ 加载任意人设

使用方式:
    # 方式1: 使用默认人设 (general_assistant)
    result = chat("你好", session_id)
    
    # 方式2: 指定人设
    result = chat("你好", session_id, persona="blacksmith")
    
    # 方式3: 切换当前人设
    set_persona("merchant")
    result = chat("你好", session_id)
    
配置文件: npc_configs/*.json
"""

import os
import json
from pathlib import Path
from typing import Optional

from langchain_ollama import ChatOllama
from langchain_core.messages import AIMessage, HumanMessage, SystemMessage, trim_messages
from langchain_community.chat_message_histories import SQLChatMessageHistory
from langchain_core.prompts import ChatPromptTemplate, MessagesPlaceholder
from langchain_core.runnables.history import RunnableWithMessageHistory


# ==================== 人设配置加载 ====================

def get_config_dir() -> Path:
    """获取配置目录路径"""
    current_dir = Path(__file__).parent
    # SceneAgentServer/npc_configs/
    return current_dir.parent.parent / "npc_configs"


def list_personas() -> list:
    """列出所有可用的人设"""
    config_dir = get_config_dir()
    if not config_dir.exists():
        return []
    return [f.stem for f in config_dir.glob("*.json")]


def load_persona_config(persona: str) -> Optional[dict]:
    """加载指定人设的配置"""
    config_path = get_config_dir() / f"{persona}.json"
    if not config_path.exists():
        return None
    try:
        with open(config_path, 'r', encoding='utf-8') as f:
            return json.load(f)
    except Exception as e:
        print(f"[GeneralChat] 加载人设失败 {persona}: {e}")
        return None


def build_system_prompt(persona: str = "general_assistant") -> str:
    """根据人设构建系统提示词"""
    config = load_persona_config(persona)
    if not config:
        print(f"[GeneralChat] 人设 '{persona}' 不存在，使用默认")
        return _default_prompt()
    
    p = config.get("personality", config)
    name = p.get("name", "AI助手")
    role = p.get("role", "助手")
    traits = "、".join(p.get("traits", [])) or "友好"
    background = p.get("background", "")
    speech_style = p.get("speech_style", "")
    knowledge = "、".join(p.get("knowledge", [])) or "通用知识"
    
    return f"""你是{name}，一位{role}。

【性格特点】{traits}

【背景】{background}

【说话风格】{speech_style}

【擅长领域】{knowledge}

请保持角色一致性，用自然的方式回复用户。"""


def _default_prompt() -> str:
    """默认系统提示词"""
    return """你是一个友好、专业的AI助手。请用简洁、准确的语言回复用户。"""


# ==================== 对话链管理 ====================

class ChatManager:
    """对话管理器 - 支持多人设切换"""
    
    def __init__(self, default_persona: str = "general_assistant"):
        self.current_persona = default_persona
        self.llm = ChatOllama(temperature=0.7, model="qwen2.5")
        self.history_cache = {}
        self._chain_cache = {}  # 缓存不同人设的对话链
    
    def set_persona(self, persona: str) -> bool:
        """切换人设"""
        if persona not in list_personas():
            print(f"[GeneralChat] 人设 '{persona}' 不存在")
            print(f"[GeneralChat] 可用人设: {list_personas()}")
            return False
        self.current_persona = persona
        print(f"[GeneralChat] 已切换人设: {persona}")
        return True
    
    def get_session_history(self, session_id: str):
        """获取会话历史"""
        if session_id not in self.history_cache:
            self.history_cache[session_id] = SQLChatMessageHistory(
                session_id, 
                connection="sqlite:///general_chat.db"
            )
        return self.history_cache[session_id]
    
    def _get_chain(self, persona: str):
        """获取指定人设的对话链（带缓存）"""
        if persona not in self._chain_cache:
            system_prompt = build_system_prompt(persona)
            
            trimmer = trim_messages(
                max_tokens=2000,
                strategy="last",
                token_counter=len,
                include_system=True,
            )
            
            prompt = ChatPromptTemplate.from_messages([
                ("system", system_prompt),
                MessagesPlaceholder(variable_name="history"),
                MessagesPlaceholder(variable_name="input"),
            ])
            
            chain = prompt | trimmer | self.llm
            
            self._chain_cache[persona] = RunnableWithMessageHistory(
                chain,
                self.get_session_history,
                input_messages_key="input",
                history_messages_key="history"
            )
        
        return self._chain_cache[persona]
    
    def chat(self, message: str, session_id: str, persona: str = None) -> dict:
        """
        对话接口
        
        Args:
            message: 用户消息
            session_id: 会话ID
            persona: 人设名称（None则使用当前人设）
        """
        import time
        
        persona = persona or self.current_persona
        result = {"success": 1, "msg": "", "response_time": 0.0, "persona": persona}
        
        try:
            start_time = time.time()
            chain = self._get_chain(persona)
            
            response = chain.invoke(
                {"input": [HumanMessage(content=message)]},
                config={"configurable": {"session_id": session_id}}
            )
            
            result["msg"] = response.content
            result["response_time"] = time.time() - start_time
            
        except Exception as e:
            result["success"] = 0
            result["msg"] = f"对话出错: {str(e)}"
            print(f"[GeneralChat] Error: {e}")
        
        return result
    
    def clear_history(self, session_id: str) -> dict:
        """清除会话历史"""
        try:
            self.get_session_history(session_id).clear()
            return {"success": 1, "msg": "历史记录已清除"}
        except Exception as e:
            return {"success": 0, "msg": f"清除失败: {str(e)}"}
    
    def clear_chain_cache(self):
        """清除对话链缓存（人设配置更新后调用）"""
        self._chain_cache.clear()
        print("[GeneralChat] 对话链缓存已清除")


# ==================== 全局实例和便捷函数 ====================

_manager = ChatManager()

def chat(message: str, session_id: str, persona: str = None) -> dict:
    """对话（使用默认管理器）"""
    return _manager.chat(message, session_id, persona)

def set_persona(persona: str) -> bool:
    """切换当前人设"""
    return _manager.set_persona(persona)

def get_persona() -> str:
    """获取当前人设"""
    return _manager.current_persona

def clear_history(session_id: str) -> dict:
    """清除会话历史"""
    return _manager.clear_history(session_id)

def reload_personas():
    """重新加载人设配置（配置文件更新后调用）"""
    _manager.clear_chain_cache()


# ==================== 命令行测试 ====================
if __name__ == "__main__":
    try:
        from prompt_toolkit import prompt as cli_prompt
    except ImportError:
        cli_prompt = input
    import uuid
    
    session_id = str(uuid.uuid4())
    response_times = []
    
    print("=" * 60)
    print("通用对话测试")
    print("=" * 60)
    print(f"可用人设: {list_personas()}")
    print(f"当前人设: {get_persona()}")
    print("-" * 60)
    print("命令: quit | clear | stats | persona <名称> | list")
    print("=" * 60)
    
    while True:
        try:
            user_input = cli_prompt("You: ").strip()
        except (EOFError, KeyboardInterrupt):
            print("\n再见!")
            break
        
        if not user_input:
            continue
        
        # 命令处理
        if user_input.lower() == 'quit':
            break
        elif user_input.lower() == 'clear':
            print(f"System: {clear_history(session_id)['msg']}")
            continue
        elif user_input.lower() == 'list':
            print(f"可用人设: {list_personas()}")
            continue
        elif user_input.lower().startswith('persona '):
            new_persona = user_input.split(' ', 1)[1].strip()
            set_persona(new_persona)
            continue
        elif user_input.lower() == 'stats':
            if response_times:
                print(f"对话次数: {len(response_times)}, 平均: {sum(response_times)/len(response_times):.2f}s")
            continue
        
        # 对话
        result = chat(user_input, session_id)
        response_times.append(result.get('response_time', 0))
        print(f"[{result['persona']}]: {result['msg']}")
        print(f"[响应: {result['response_time']:.2f}s]")
        print("-" * 60)
