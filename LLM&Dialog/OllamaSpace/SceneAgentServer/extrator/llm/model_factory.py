"""LLM模型工厂。

支持两种提供方:
- Ollama (默认)
- LM Studio(OpenAI兼容接口)

通过环境变量切换，避免在代码中硬编码模型提供方。
"""

import json
import os
from pathlib import Path
import importlib.util
from dataclasses import dataclass
from typing import Any, Dict, List, Union

import requests

from langchain_core.messages import AIMessage, BaseMessage, HumanMessage, SystemMessage


@dataclass
class SimpleAIResponse:
    """统一的最小响应对象，兼容 .content 访问。"""

    content: str


@dataclass
class ChatModelConfig:
    """对话模型配置。"""

    provider: str
    model: str
    temperature: float
    timeout: int
    ollama_base_url: str
    lm_studio_url: str
    lm_studio_api_key: str


def _parse_bool(value: str, default: bool = False) -> bool:
    """将字符串解析为布尔值。"""

    if value is None:
        return default
    return str(value).strip().lower() in {"1", "true", "yes", "y", "on"}


def _load_project_config() -> Dict[str, Any]:
    """从项目根目录的config.py读取配置（若存在）。"""

    project_root = Path(__file__).resolve().parent.parent.parent
    config_path = project_root / "config.py"
    if not config_path.exists():
        return {}

    module_name = "scene_agent_runtime_config"
    spec = importlib.util.spec_from_file_location(module_name, str(config_path))
    if not spec or not spec.loader:
        return {}

    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)

    return {
        "NPC_LLM_PROVIDER": getattr(module, "NPC_LLM_PROVIDER", None),
        "NPC_LLM_TEMPERATURE": getattr(module, "NPC_LLM_TEMPERATURE", None),
        "NPC_LLM_TIMEOUT": getattr(module, "NPC_LLM_TIMEOUT", None),
        "NPC_OLLAMA_MODEL": getattr(module, "NPC_OLLAMA_MODEL", None),
        "NPC_OLLAMA_BASE_URL": getattr(module, "NPC_OLLAMA_BASE_URL", None),
        "NPC_LM_STUDIO_MODEL": getattr(module, "NPC_LM_STUDIO_MODEL", None),
        "NPC_LM_STUDIO_URL": getattr(module, "NPC_LM_STUDIO_URL", None),
        "NPC_LM_STUDIO_API_KEY": getattr(module, "NPC_LM_STUDIO_API_KEY", None),
        "NPC_LLM_MODEL": getattr(module, "NPC_LLM_MODEL", None),
        "LM_STUDIO_URL": getattr(module, "LM_STUDIO_URL", None),
        "LM_STUDIO_API_KEY": getattr(module, "LM_STUDIO_API_KEY", None),
    }


def _pick_config_value(env_key: str, config_dict: Dict[str, Any], default: str) -> str:
    """按优先级取配置: 环境变量 > config.py > 默认值。"""

    env_value = os.getenv(env_key)
    if env_value is not None and str(env_value).strip() != "":
        return str(env_value).strip()

    config_value = config_dict.get(env_key)
    if config_value is not None and str(config_value).strip() != "":
        return str(config_value).strip()

    return default


def load_chat_model_config() -> ChatModelConfig:
    """加载模型配置（环境变量优先，支持项目根目录config.py）。"""

    project_config = _load_project_config()

    provider = _pick_config_value("NPC_LLM_PROVIDER", project_config, "ollama").lower()
    temperature = float(
        _pick_config_value("NPC_LLM_TEMPERATURE", project_config, "0.7")
    )
    timeout = int(_pick_config_value("NPC_LLM_TIMEOUT", project_config, "120"))

    ollama_model = _pick_config_value("NPC_OLLAMA_MODEL", project_config, "qwen2.5")
    lm_model = _pick_config_value("NPC_LM_STUDIO_MODEL", project_config, "qwen3.5")

    model = ollama_model if provider == "ollama" else lm_model
    custom_model = _pick_config_value("NPC_LLM_MODEL", project_config, "")
    if custom_model:
        model = custom_model

    return ChatModelConfig(
        provider=provider,
        model=model,
        temperature=temperature,
        timeout=timeout,
        ollama_base_url=_pick_config_value("NPC_OLLAMA_BASE_URL", project_config, ""),
        lm_studio_url=(
            _pick_config_value("NPC_LM_STUDIO_URL", project_config, "")
            or _pick_config_value("LM_STUDIO_URL", project_config, "")
            or "http://192.168.5.23:1234/v1"
        ),
        lm_studio_api_key=(
            _pick_config_value("NPC_LM_STUDIO_API_KEY", project_config, "")
            or _pick_config_value("LM_STUDIO_API_KEY", project_config, "")
        ),
    )


def get_chat_model_info() -> Dict[str, Any]:
    """返回当前模型配置信息，便于调试与接口查询。"""

    project_config = _load_project_config()
    config = load_chat_model_config()
    project_root = Path(__file__).resolve().parent.parent.parent
    config_path = project_root / "config.py"

    env_npc_key = os.getenv("NPC_LM_STUDIO_API_KEY")
    env_lm_key = os.getenv("LM_STUDIO_API_KEY")

    return {
        "provider": config.provider,
        "model": config.model,
        "temperature": config.temperature,
        "timeout": config.timeout,
        "lm_studio_url": config.lm_studio_url,
        "ollama_base_url": config.ollama_base_url,
        "lm_studio_api_key_present": bool(config.lm_studio_api_key),
        "lm_studio_api_key_length": len(config.lm_studio_api_key or ""),
        "config_debug": {
            "config_path": str(config_path),
            "config_exists": config_path.exists(),
            "project_config_key_present": bool(project_config.get("NPC_LM_STUDIO_API_KEY")),
            "project_config_key_length": len(str(project_config.get("NPC_LM_STUDIO_API_KEY") or "")),
            "env_npc_key_present": bool(env_npc_key),
            "env_npc_key_length": len(str(env_npc_key or "")),
            "env_lm_key_present": bool(env_lm_key),
            "env_lm_key_length": len(str(env_lm_key or "")),
        },
        "active_backend": (
            "lm_studio" if config.provider in {"lm_studio", "openai_compat", "openai"} else "ollama"
        ),
    }


class LMStudioChatModel:
    """LM Studio(OpenAI兼容)的最小聊天模型适配器。"""

    is_openai_compatible = True

    def __init__(self, config: ChatModelConfig):
        """初始化适配器。"""

        self.config = config
        self.api_url = f"{config.lm_studio_url.rstrip('/')}/chat/completions"
        self.models_url = f"{config.lm_studio_url.rstrip('/')}/models"

    def _normalize_messages(
        self, messages: Union[str, List[Union[Dict[str, str], BaseMessage]]]
    ) -> List[Dict[str, str]]:
        """将输入消息统一转换为OpenAI消息格式。"""

        if isinstance(messages, str):
            return [{"role": "user", "content": messages}]

        normalized: List[Dict[str, str]] = []
        for item in messages:
            if isinstance(item, dict):
                role = str(item.get("role", "user"))
                content = str(item.get("content", ""))
                normalized.append({"role": role, "content": content})
                continue

            if isinstance(item, SystemMessage):
                normalized.append({"role": "system", "content": str(item.content)})
            elif isinstance(item, HumanMessage):
                normalized.append({"role": "user", "content": str(item.content)})
            elif isinstance(item, AIMessage):
                normalized.append(
                    {"role": "assistant", "content": str(item.content)}
                )
            else:
                normalized.append({"role": "user", "content": str(item)})

        return normalized

    def invoke(
        self, messages: Union[str, List[Union[Dict[str, str], BaseMessage]]]
    ) -> SimpleAIResponse:
        """调用LM Studio并返回兼容响应对象。"""

        print(
            f"[LLM Factory] invoke provider=lm_studio model={self.config.model} "
            f"url={self.config.lm_studio_url}"
        )

        payload = {
            "model": self.config.model,
            "messages": self._normalize_messages(messages),
            "temperature": self.config.temperature,
            "max_tokens": 1024,
        }

        def _get_default_model_id(with_auth: bool) -> str:
            headers = {"Content-Type": "application/json"}
            if with_auth and self.config.lm_studio_api_key:
                headers["Authorization"] = f"Bearer {self.config.lm_studio_api_key}"

            response = requests.get(
                self.models_url,
                headers=headers,
                timeout=self.config.timeout,
            )
            response.raise_for_status()
            data = response.json() if response.text else {}
            models = data.get("data") if isinstance(data, dict) else None
            if not isinstance(models, list) or not models:
                return ""
            first = models[0] if isinstance(models[0], dict) else {}
            model_id = str(first.get("id") or "").strip()
            return model_id

        def _send(with_auth: bool) -> Dict[str, Any]:
            headers = {"Content-Type": "application/json"}
            if with_auth and self.config.lm_studio_api_key:
                headers["Authorization"] = (
                    f"Bearer {self.config.lm_studio_api_key}"
                )

            response = requests.post(
                self.api_url,
                headers=headers,
                json=payload,
                timeout=self.config.timeout,
            )
            response.raise_for_status()
            return response.json()

        try:
            data = _send(with_auth=True)
        except requests.HTTPError as http_error:
            response = http_error.response
            status_code = response.status_code if response is not None else None
            error_code = ""

            if response is not None:
                try:
                    error_payload = response.json()
                    if isinstance(error_payload, dict):
                        err = error_payload.get("error")
                        if isinstance(err, dict):
                            error_code = str(err.get("code") or "").strip().lower()
                except Exception:
                    error_code = ""

            # 模型名不存在时，自动探测已加载模型并重试一次。
            if status_code == 400 and error_code == "model_not_found":
                default_model_id = ""
                try:
                    default_model_id = _get_default_model_id(with_auth=True)
                except Exception:
                    default_model_id = ""

                if not default_model_id:
                    try:
                        default_model_id = _get_default_model_id(with_auth=False)
                    except Exception:
                        default_model_id = ""

                if default_model_id:
                    payload["model"] = default_model_id
                    print(
                        "[LLM Factory] model_not_found, fallback to default loaded model="
                        f"{default_model_id}"
                    )
                    try:
                        data = _send(with_auth=True)
                    except requests.HTTPError as retry_error:
                        if retry_error.response is not None and retry_error.response.status_code in {401, 403}:
                            data = _send(with_auth=False)
                        else:
                            raise
                else:
                    raise
            # LM Studio 通常不需要 API Key；如果错误的是 401/403，尝试去掉认证头重试一次。
            elif status_code in {401, 403}:
                data = _send(with_auth=False)
            else:
                raise

        choices = data.get("choices") or []
        if not choices:
            return SimpleAIResponse(content="")

        message = choices[0].get("message") or {}
        return SimpleAIResponse(content=str(message.get("content") or ""))


def create_chat_model() -> Any:
    """根据配置创建聊天模型实例。"""

    config = load_chat_model_config()

    if config.provider in {"lm_studio", "openai_compat", "openai"}:
        print(
            f"[LLM Factory] provider=lm_studio model={config.model} "
            f"url={config.lm_studio_url}"
        )
        return LMStudioChatModel(config)

    kwargs: Dict[str, Any] = {
        "model": config.model,
        "temperature": config.temperature,
    }
    if config.ollama_base_url:
        kwargs["base_url"] = config.ollama_base_url

    print(f"[LLM Factory] provider=ollama model={config.model}")
    from langchain_ollama import ChatOllama

    return ChatOllama(**kwargs)
