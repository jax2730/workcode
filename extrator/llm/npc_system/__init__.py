"""
NPC智能体系统
基于 HelloAgents 第8、9、15章设计

模块:
- MemoryTool: 四层记忆系统 (工作记忆、情景记忆、语义记忆、感知记忆)
- RAGTool: 检索增强生成 (知识库检索)
- ContextBuilder: GSSC上下文构建流水线
- RelationshipManager: 好感度系统
- NoteTool: 结构化笔记
- TerminalTool: 安全的文件系统访问
- DialogueStorage: 对话记录存储 (SQLite + Excel)
- FileMemoryStore: 文件系统记忆存储 (Markdown)
- StorageConfig/StorageManager: 统一存储配置管理
- NPCAgent: NPC智能体
- NPCManager: 多NPC管理器

数据存储结构:
```
npc_data/
├── memories/                    # 记忆存储 (Markdown文件)
│   ├── working/                 # 工作记忆
│   ├── episodic/                # 情景记忆
│   ├── semantic/                # 语义记忆
│   └── dialogues/               # 对话记录
├── databases/                   # 数据库文件
│   ├── dialogue_history.db      # 对话历史 (SQLite, 兼容原general_chat.py)
│   ├── npc_memory.db            # NPC记忆 (SQLite)
│   └── npc_relationship.db      # 好感度 (SQLite)
├── exports/                     # 导出文件
│   ├── excel/                   # Excel导出 (兼容原export_history.py)
│   └── json/                    # JSON导出
├── knowledge_base/              # RAG知识库
├── rag_index/                   # RAG向量索引
├── notes/                       # 结构化笔记
├── configs/                     # NPC配置
└── logs/                        # 日志文件
```

使用示例:
```python
from npc_system import create_npc, NPCManager, init_npc_data_directories
from npc_system import StorageManager, StorageConfig
from langchain_ollama import ChatOllama

# 初始化数据目录
init_npc_data_directories()

# 创建LLM
llm = ChatOllama(model="qwen2.5", temperature=0.7)

# 方式1: 快速创建单个NPC
npc = create_npc(
    npc_id="blacksmith",
    name="老铁匠",
    role="铁匠",
    traits=["严肃", "专业"],
    background="打铁30年的老师傅",
    llm=llm
)

# 对话
result = npc.chat("player_001", "能帮我打一把剑吗？")
print(result["reply"])

# 导出对话历史到Excel
excel_path = npc.export_dialogue_history("player_001")

# 方式2: 使用NPC管理器
manager = NPCManager(llm=llm)
manager.register_npc("blacksmith", {...})
result = manager.chat("blacksmith", "player_001", "你好")

# 方式3: 使用统一存储管理器
storage = StorageManager(StorageConfig(base_dir="./npc_data"))
storage.save_dialogue(session_id, player_msg, npc_reply, npc_id, player_id)
storage.export_dialogues_to_excel()
```
"""

# 记忆系统
from .memory_tool import (
    MemoryTool,
    MemoryConfig,
    MemoryItem,
    MemoryType,
    WorkingMemory,
    EpisodicMemory,
    SemanticMemory,
    create_memory_tool
)

# RAG系统
from .rag_tool import (
    RAGTool,
    RAGConfig,
    Document,
    SearchResult,
    EmbeddingService,
    VectorStore,
    DocumentProcessor,
    create_rag_tool
)

# 上下文构建
from .context_builder import (
    ContextBuilder,
    ContextConfig,
    ContextPacket,
    Message,
    create_context_builder
)

# 好感度系统
from .relationship_manager import (
    RelationshipManager,
    AffinityLevel,
    AffinityInfo,
    create_relationship_manager
)

# 笔记工具
from .note_tool import (
    NoteTool,
    create_note_tool
)

# 终端工具
from .terminal_tool import (
    TerminalTool,
    TerminalConfig,
    create_terminal_tool
)

# 对话存储
from .dialogue_storage import (
    DialogueStorage,
    DialogueMessage,
    DialogueSession,
    SQLiteDialogueStore,
    ExcelDialogueExporter,
    create_dialogue_storage
)

# 文件记忆存储
from .file_memory_store import (
    FileMemoryStore,
    FileMemoryConfig,
    create_file_memory_store
)

# 统一存储配置管理
from .storage_config import (
    StorageConfig,
    StorageManager,
    get_storage_manager,
    create_storage_manager
)

# 数据目录初始化
from .init_data_dirs import (
    init_npc_data_directories,
    init_npc_directories,
    get_data_paths
)

# NPC智能体
from .npc_agent import (
    NPCAgent,
    NPCConfig,
    NPCPersonality,
    create_npc,
    load_npc_from_json,
    load_npc_from_dict
)

# NPC管理器
from .npc_manager import (
    NPCManager,
    NPCManagerConfig,
    NPC_TEMPLATES,
    create_npc_manager,
    create_template_npc
)

__all__ = [
    # 记忆系统
    "MemoryTool",
    "MemoryConfig", 
    "MemoryItem",
    "MemoryType",
    "WorkingMemory",
    "EpisodicMemory",
    "SemanticMemory",
    "create_memory_tool",
    
    # RAG系统
    "RAGTool",
    "RAGConfig",
    "Document",
    "SearchResult",
    "EmbeddingService",
    "VectorStore",
    "DocumentProcessor",
    "create_rag_tool",
    
    # 上下文构建
    "ContextBuilder",
    "ContextConfig",
    "ContextPacket",
    "Message",
    "create_context_builder",
    
    # 好感度系统
    "RelationshipManager",
    "AffinityLevel",
    "AffinityInfo",
    "create_relationship_manager",
    
    # 笔记工具
    "NoteTool",
    "create_note_tool",
    
    # 终端工具
    "TerminalTool",
    "TerminalConfig",
    "create_terminal_tool",
    
    # 对话存储
    "DialogueStorage",
    "DialogueMessage",
    "DialogueSession",
    "SQLiteDialogueStore",
    "ExcelDialogueExporter",
    "create_dialogue_storage",
    
    # 文件记忆存储
    "FileMemoryStore",
    "FileMemoryConfig",
    "create_file_memory_store",
    
    # 统一存储配置管理
    "StorageConfig",
    "StorageManager",
    "get_storage_manager",
    "create_storage_manager",
    
    # 数据目录初始化
    "init_npc_data_directories",
    "init_npc_directories",
    "get_data_paths",
    
    # NPC智能体
    "NPCAgent",
    "NPCConfig",
    "NPCPersonality",
    "create_npc",
    "load_npc_from_json",
    "load_npc_from_dict",
    
    # NPC管理器
    "NPCManager",
    "NPCManagerConfig",
    "NPC_TEMPLATES",
    "create_npc_manager",
    "create_template_npc",
]

__version__ = "1.0.0"
