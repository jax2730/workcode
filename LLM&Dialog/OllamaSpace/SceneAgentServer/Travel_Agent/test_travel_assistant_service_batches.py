import importlib.util
import sys
import types
from pathlib import Path


def _install_langchain_stubs() -> None:
    histories = types.ModuleType("langchain_community.chat_message_histories")
    histories.SQLChatMessageHistory = object
    community = types.ModuleType("langchain_community")
    community.chat_message_histories = histories

    messages = types.ModuleType("langchain_core.messages")
    messages.AIMessage = object
    messages.HumanMessage = object
    messages.SystemMessage = object
    core = types.ModuleType("langchain_core")
    core.messages = messages

    sys.modules.setdefault("langchain_community", community)
    sys.modules.setdefault("langchain_community.chat_message_histories", histories)
    sys.modules.setdefault("langchain_core", core)
    sys.modules.setdefault("langchain_core.messages", messages)


def _load_service_module():
    _install_langchain_stubs()
    module_path = Path(__file__).with_name("travel_assistant_service.py")
    spec = importlib.util.spec_from_file_location("travel_assistant_service_under_test", module_path)
    module = importlib.util.module_from_spec(spec)
    assert spec.loader is not None
    spec.loader.exec_module(module)
    return module


def test_full_poi_batch_reply_lists_every_batch_without_asking():
    service = _load_service_module()
    items = [{"name": f"Building{i:03d}"} for i in range(1, 86)]

    reply = service._build_full_poi_batch_reply(items, total_count=85, batch_size=40)

    assert "第1/3批（1-40）" in reply
    assert "第2/3批（41-80）" in reply
    assert "第3/3批（81-85）" in reply
    assert "（Building001）" in reply
    assert "（Building085）" in reply
    assert "是否" not in reply
    assert "要我" not in reply


if __name__ == "__main__":
    test_full_poi_batch_reply_lists_every_batch_without_asking()
    print("batch reply tests passed")
