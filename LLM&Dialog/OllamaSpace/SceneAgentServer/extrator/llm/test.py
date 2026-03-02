"""
Docstring for OllamaSpace.SceneAgentServer.extrator.llm.NpcDialog
"""

import json
from pathlib import Path

from langchain_ollama import ChatOllama
from langchain_core.messages import AIMessage, HumanMessage, SystemMessage, trim_messages
from langchain_community.chat_message_histories import SQLChatMessageHistory
from langchain_core.prompts import ChatPromptTemplate, MessagesPlaceholder
from langchain_core.runnables.history import RunnableWithMessageHistory


def load_system_prompt(persona: str = "blacksmith") -> str:
    """从 npc_configs/ 加载人设提示词"""
    config_path = Path(__file__).parent.parent.parent / "npc_configs" / f"{persona}.json"
    try:
        with open(config_path, 'r', encoding='utf-8') as f:
            p = json.load(f).get("personality", {})
        prompt = f"""你是{p.get('name', 'AI助手')}，一位{p.get('role', '助手')}。
【性格】{"、".join(p.get('traits', []))}
【背景】{p.get('background', '')}
【说话风格】{p.get('speech_style', '')}
请保持角色一致性，用自然的方式回复用户。"""
        print(f"[Test] 已加载人设: {persona} ({p.get('name', '未知')})")
        return prompt
    except Exception as e:
        print(f"[Test] 加载人设 '{persona}' 失败: {e}，使用默认")
        return "你是一个友好、专业的AI助手。请用简洁、准确的语言回复用户。"


SYSTEM_PROMPT = load_system_prompt()

# history cache
history_cache = {}

def get_session_history(session_id: str):
    if session_id not in history_cache:
        history_cache[session_id] = SQLChatMessageHistory(
            session_id,
            connection="sqlite:///general_chat.db"
        )
    return history_cache[session_id]


def create_chat_chain():
    """创建对话链"""
    llm = ChatOllama(
        temperature=0.7,  # 稍微提高一点温度，让回复更自然
        model="qwen2.5"   # 使用 qwen2.5 模型
    )
    
    # 消息修剪器 - 防止历史消息过长
    trimmer = trim_messages(
        max_tokens=2000,
        strategy="last",
        token_counter=len,
        include_system=True,
    )
    
    # 构建提示模板
    prompt = ChatPromptTemplate.from_messages([
        ("system", SYSTEM_PROMPT),
        MessagesPlaceholder(variable_name="history"),
        MessagesPlaceholder(variable_name="input"),
    ])
    
    # 组合链
    chain = prompt | trimmer | llm
    
    # 添加历史记录支持
    chain_with_history = RunnableWithMessageHistory(
        chain,
        get_session_history,
        input_messages_key="input",
        history_messages_key="history"
    )
    
    return chain_with_history


# 创建全局对话链
chat_chain = create_chat_chain()



def chat(user_message: str, session_id: str) -> dict:
    """
    处理用户消息并返回AI回复
    
    Args:
        user_message: 用户输入的消息
        session_id: 会话ID
        
    Returns:
        dict: {"success": 1, "msg": "AI回复内容", "response_time": float}
    """
    import time
    
    result = {
        "success": 1,
        "msg": "",
        "response_time": 0.0
    }
    
    try:
        start_time = time.time()
        response = chat_chain.invoke(
            {"input": [HumanMessage(content=user_message)]},
            config={"configurable": {"session_id": session_id}}
        )
        
        result["response_time"] = time.time() - start_time
        result["msg"] = response.content
        print(f"[GeneralChat] User: {user_message}")
        print(f"[GeneralChat] AI: {response.content}")
        
    except Exception as e:
        result["success"] = 0
        result["msg"] = f"对话出错: {str(e)}"
        print(f"[GeneralChat] Error: {e}")
    
    return result


def clear_history(session_id: str) -> dict:
    """清除指定会话的历史记录"""
    try:
        history = get_session_history(session_id)
        history.clear()
        return {"success": 1, "msg": "历史记录已清除"}
    except Exception as e:
        return {"success": 0, "msg": f"清除失败: {str(e)}"}


# ==================== 命令行测试 ====================
if __name__ == "__main__":
    from prompt_toolkit import prompt
    import uuid
    
    session_id = str(uuid.uuid4())
    print("=" * 50)
    print("通用对话测试 (输入 'quit' 退出, 'clear' 清除历史)")
    print(f"Session ID: {session_id}")
    print("=" * 50)
    
    while True:
        try:
            user_input = prompt("You: ")
        except (EOFError, KeyboardInterrupt):
            print("\n再见!")
            break
            
        if user_input.lower() == 'quit':
            print("再见!")
            break
        elif user_input.lower() == 'clear':
            result = clear_history(session_id)
            print(f"System: {result['msg']}")
            continue
        elif user_input.strip() == '':
            continue
            
        result = chat(user_input, session_id)
        print(f"AI: {result['msg']}")
        print(f"[响应时间: {result['response_time']:.2f}s]")
        print("-" * 50)
