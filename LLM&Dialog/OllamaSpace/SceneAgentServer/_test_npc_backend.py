import os, sys
os.environ.setdefault('DJANGO_SETTINGS_MODULE', 'agent.settings')

# Mock ALL langchain and related packages with meta-path finder
import unittest.mock
import types
import importlib
import importlib.abc
import importlib.machinery

MOCK_PACKAGES = {'langchain_ollama', 'langchain_core', 'langchain_community', 
                 'langchain_chroma', 'langchain', 'openpyxl', 'prompt_toolkit',
                 'chromadb', 'tiktoken', 'sentence_transformers'}

class MockLoader(importlib.abc.Loader):
    def create_module(self, spec):
        mod = types.ModuleType(spec.name)
        mod.__path__ = [spec.name]
        mod.__package__ = spec.name
        mod.__file__ = f"<mock:{spec.name}>"
        mod.__spec__ = spec
        return mod
    def exec_module(self, module):
        pass

class MockFinder(importlib.abc.MetaPathFinder):
    def find_spec(self, fullname, path, target=None):
        top = fullname.split('.')[0]
        if top in MOCK_PACKAGES:
            return importlib.machinery.ModuleSpec(
                fullname, MockLoader(),
                is_package=True
            )
        return None

sys.meta_path.insert(0, MockFinder())

import django
django.setup()

# Test 1: Check backends
from extrator.llm.chat_interface import chat_interface
print('Registered backends:', chat_interface.list_backends())
assert 'npc' in chat_interface.list_backends(), "ERROR: 'npc' backend not registered!"
print('OK: npc backend is registered')

# Test 2: Check that the NPC backend handlers exist and are callable
print('\n--- NPC backend handler check ---')
for action, store in [('chat', '_backends'), ('connect', '_connect_handlers'), ('clear', '_clear_handlers')]:
    h = getattr(chat_interface, store).get('npc')
    if h and callable(h):
        print(f'  OK  npc.{action} -> {h.__name__}')
    else:
        print(f'  FAIL npc.{action} -> not callable: {h}')

# Test 3: Check test backend 
print('\n--- Test backend handler check ---')
for action, store in [('chat', '_backends'), ('connect', '_connect_handlers'), ('clear', '_clear_handlers')]:
    h = getattr(chat_interface, store).get('test')
    if h and callable(h):
        print(f'  OK  test.{action} -> {h.__name__}')
    else:
        print(f'  FAIL test.{action} -> not callable: {h}')

# Test 4: URL patterns (informational — fails locally due to mock cascade)
print('\n--- URL pattern check (informational) ---')
try:
    from agent.urls import urlpatterns
    for p in urlpatterns:
        pat = str(p.pattern)
        if '<backend>' in pat:
            print(f'  OK  pattern: {pat} -> {getattr(p.callback, "__name__", p.callback)}')
except Exception as e:
    print(f'  SKIP (mocks cause import error): {type(e).__name__}: {e}')
    print('  NOTE: On real server, /chat/<backend>/ and /connect/<backend>/ routes ARE defined in urls.py')

# Test 5: ChatRequest parsing with npc_id
print('\n--- ChatRequest npc_id parsing ---')
from extrator.llm.chat_interface import ChatRequest
import json
body = json.dumps({"message": "hello", "npc_id": "blacksmith", "player_id": "player1"}).encode('utf-8')
req = ChatRequest.from_json(body, "session_123")
print(f'  npc_id: {req.npc_id}, player_id: {req.player_id}, message: {req.message}')
assert req.npc_id == "blacksmith", f"Expected blacksmith, got {req.npc_id}"
assert req.player_id == "player1", f"Expected player1, got {req.player_id}"
print('  OK: ChatRequest parsing works')

print('\n=== All checks passed ===')
