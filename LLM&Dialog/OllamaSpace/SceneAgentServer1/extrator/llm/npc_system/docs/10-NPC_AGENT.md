# 10 - NPC智能体详解 (NPCAgent)

> **面向对象**: 系统开发者、架构师  
> **前置知识**: Python、面向对象编程、异步编程  
> **相关模块**: 所有核心模块的集成

## 📋 模块概述

### 职责定义

NPCAgent 是 NPC 系统的**核心智能体**，是所有模块的集成中心，负责协调和管理整个对话流程。

**核心职责**:
1. **对话处理**: 接收用户输入，生成NPC回复
2. **模块协调**: 协调记忆、RAG、上下文、好感度等模块
3. **状态管理**: 管理NPC的内部状态和会话状态
4. **数据持久化**: 保存对话记录和更新记忆
5. **事件触发**: 触发好感度变化、秘密解锁等事件

### 设计理念

**智能体模式 (Agent Pattern)**:
```
用户输入 → NPCAgent → 协调各模块 → LLM推理 → 生成回复 → 持久化 → 返回结果
```

**为什么需要NPCAgent?**

```python
# 没有NPCAgent: 需要手动协调所有模块
memory = MemoryTool(...)
rag = RAGTool(...)
context_builder = ContextBuilder(...)
relationship = RelationshipManager(...)
dialogue_storage = DialogueStorage(...)

# 手动协调
memories = memory.search(message)
knowledge = rag.search(message)
affinity = relationship.get_affinity(npc_id, player_id)
context = context_builder.build(memories, knowledge, affinity)
reply = llm.invoke(context)
dialogue_storage.save(message, reply)
relationship.update(npc_id, player_id, delta)
# ... 太复杂！

# 有NPCAgent: 一行代码搞定
npc = NPCAgent(...)
result = npc.chat(player_id, message)
# 所有模块自动协调！
```

---

## 🏗️ 架构设计

### 类图

```python
┌─────────────────────────────────────────────────────────┐
│                    NPCAgent                              │
├─────────────────────────────────────────────────────────┤
│ # 核心属性                                               │
│ - npc_id: str                                           │
│ - personality: NPCPersonality                           │
│ - llm: ChatOllama                                       │
│                                                          │
│ # 工具模块                                               │
│ - memory: MemoryTool                                    │
│ - rag: RAGTool                                          │
│ - notes: NoteTool                                       │
│ - terminal: TerminalTool                                │
│                                                          │
│ # 管理模块                                               │
│ - context_builder: ContextBuilder                       │
│ - relationship: RelationshipManager                     │
│ - dialogue_storage: DialogueStorage                     │
│ - file_memory_store: FileMemoryStore                    │
│                                                          │
│ # 状态管理                                               │
│ - sessions: Dict[str, List]                             │
│ - active_dialogue_sessions: Set[str]                    │
├─────────────────────────────────────────────────────────┤
│ # 核心方法                                               │
│ + chat(player_id, message, session_id) -> Dict         │
│ + get_status(player_id) -> Dict                        │
│ + get_greeting(player_id) -> str                       │
│ + clear_session(player_id) -> None                     │
│                                                          │
│ # 记忆方法                                               │
│ + remember(content, memory_type, importance) -> str    │
│ + recall(query, limit) -> List                         │
│                                                          │
│ # 知识方法                                               │
│ + add_knowledge(content, doc_id) -> str                │
│ + search_knowledge(query) -> List                      │
│                                                          │
│ # 笔记方法                                               │
│ + take_note(title, content, note_type) -> str          │
│                                                          │
│ # 存储方法                                               │
│ + export_dialogue_history(player_id) -> str            │
│ + get_dialogue_history(player_id, limit) -> List       │
│ + get_dialogue_stats() -> Dict                         │
├─────────────────────────────────────────────────────────┤
│ # 内部方法                                               │
│ - _build_context(message, player_id) -> str            │
│ - _generate_reply(context, message) -> str             │
│ - _update_affinity(player_id, message, reply) -> ...   │
│ - _save_dialogue(player_id, message, reply) -> None    │
│ - _update_memory(player_id, message, reply) -> None    │
│ - _get_secrets_for_affinity(affinity) -> str           │
└─────────────────────────────────────────────────────────┘
```

### 数据结构

#### NPCPersonality (NPC人设)

```python
from dataclasses import dataclass, field
from typing import List, Dict, Any

@dataclass
class NPCPersonality:
    """NPC人设"""
    name: str                           # NPC名称
    role: str                           # 角色 (铁匠/商人/守卫等)
    age: int = 30                       # 年龄
    gender: str = "男"                  # 性别
    traits: List[str] = field(default_factory=list)  # 性格特征
    background: str = ""                # 背景故事
    speech_style: str = ""              # 说话风格
    knowledge: List[str] = field(default_factory=list)  # 知识领域
    secrets: List[str] = field(default_factory=list)    # 秘密
    greeting: str = ""                  # 问候语
    
    def to_prompt(self) -> str:
        """转换为提示词"""
        prompt = f"你是{self.name}，{self.role}。"
        
        if self.age:
            prompt += f"年龄{self.age}岁。"
        
        if self.traits:
            traits_str = "、".join(self.traits)
            prompt += f"性格特征: {traits_str}。"
        
        if self.background:
            prompt += f"\n背景: {self.background}"
        
        if self.speech_style:
            prompt += f"\n说话风格: {self.speech_style}"
        
        if self.knowledge:
            knowledge_str = "、".join(self.knowledge)
            prompt += f"\n擅长领域: {knowledge_str}"
        
        return prompt
    
    def to_dict(self) -> dict:
        """转换为字典"""
        return {
            "name": self.name,
            "role": self.role,
            "age": self.age,
            "gender": self.gender,
            "traits": self.traits,
            "background": self.background,
            "speech_style": self.speech_style,
            "knowledge": self.knowledge,
            "secrets": self.secrets,
            "greeting": self.greeting
        }
```

#### NPCConfig (NPC配置)

```python
@dataclass
class NPCConfig:
    """NPC配置"""
    npc_id: str                         # NPC ID
    personality: NPCPersonality         # 人设
    
    # 数据目录
    data_dir: str = "./npc_data"
    
    # 记忆配置
    memory_config: Dict = field(default_factory=dict)
    
    # RAG配置
    rag_config: Dict = field(default_factory=dict)
    enable_rag: bool = True
    
    # 上下文配置
    context_config: Dict = field(default_factory=dict)
    max_context_tokens: int = 3000
    
    # 对话配置
    enable_dialogue_storage: bool = True
    enable_file_memory: bool = True
    
    # LLM配置
    llm_model: str = "qwen2.5:7b"
    llm_temperature: float = 0.7
    
    def to_dict(self) -> dict:
        return {
            "npc_id": self.npc_id,
            "personality": self.personality.to_dict(),
            "data_dir": self.data_dir,
            "memory_config": self.memory_config,
            "rag_config": self.rag_config,
            "enable_rag": self.enable_rag,
            "context_config": self.context_config,
            "max_context_tokens": self.max_context_tokens,
            "enable_dialogue_storage": self.enable_dialogue_storage,
            "enable_file_memory": self.enable_file_memory,
            "llm_model": self.llm_model,
            "llm_temperature": self.llm_temperature
        }
```

---

## 🔄 核心流程：10步对话处理

### 完整流程图

```
用户输入: "你能帮我打造一把剑吗？"
    ↓
┌─────────────────────────────────────────────────────────┐
│ 步骤1: 接收输入和验证                                     │
│  - 验证player_id和message                                │
│  - 生成或获取session_id                                   │
│  - 记录开始时间                                           │
└────────────────────┬────────────────────────────────────┘
                     ↓
┌─────────────────────────────────────────────────────────┐
│ 步骤2: 加载会话历史                                       │
│  - 从内存加载工作记忆 (最近10条)                          │
│  - 从数据库加载历史对话                                   │
│  - 初始化会话状态                                         │
└────────────────────┬────────────────────────────────────┘
                     ↓
┌─────────────────────────────────────────────────────────┐
│ 步骤3: 检索记忆 (MemoryTool)                             │
│  - 工作记忆: 当前对话内容                                 │
│  - 情景记忆: 与该玩家的互动历史                           │
│  - 语义记忆: NPC的知识和经验                              │
│  - 感知记忆: 当前环境信息                                 │
│  耗时: 0.2-0.5秒                                          │
└────────────────────┬────────────────────────────────────┘
                     ↓
┌─────────────────────────────────────────────────────────┐
│ 步骤4: 检索知识 (RAGTool)                                │
│  - 从知识库检索相关文档                                   │
│  - 向量相似度搜索                                         │
│  - 重排序和筛选                                           │
│  耗时: 0.2-1.0秒                                          │
└────────────────────┬────────────────────────────────────┘
                     ↓
┌─────────────────────────────────────────────────────────┐
│ 步骤5: 查询好感度 (RelationshipManager)                  │
│  - 获取当前好感度等级和分数                               │
│  - 获取可分享的秘密                                       │
│  - 应用时间衰减                                           │
│  耗时: 0.05秒                                             │
└────────────────────┬────────────────────────────────────┘
                     ↓
┌─────────────────────────────────────────────────────────┐
│ 步骤6: 构建上下文 (ContextBuilder)                       │
│  - Gather: 汇集所有信息                                  │
│  - Select: 智能选择相关信息                              │
│  - Structure: 结构化组织                                 │
│  - Compress: 兜底压缩                                    │
│  耗时: 0.1-0.3秒                                          │
└────────────────────┬────────────────────────────────────┘
                     ↓
┌─────────────────────────────────────────────────────────┐
│ 步骤7: LLM生成回复                                        │
│  - 将上下文和用户消息发送给LLM                            │
│  - LLM基于上下文生成回复                                  │
│  耗时: 1-3秒 (最大耗时)                                   │
└────────────────────┬────────────────────────────────────┘
                     ↓
┌─────────────────────────────────────────────────────────┐
│ 步骤8: 更新好感度                                         │
│  - 分析对话内容                                           │
│  - 计算好感度变化                                         │
│  - 更新数据库                                             │
│  - 检查等级变化                                           │
│  耗时: 0.05秒                                             │
└────────────────────┬────────────────────────────────────┘
                     ↓
┌─────────────────────────────────────────────────────────┐
│ 步骤9: 保存对话记录                                       │
│  - SQLite: 结构化存储                                     │
│  - Markdown: 人类可读格式                                 │
│  - 更新工作记忆                                           │
│  耗时: 0.1-0.2秒                                          │
└────────────────────┬────────────────────────────────────┘
                     ↓
┌─────────────────────────────────────────────────────────┐
│ 步骤10: 返回结果                                          │
│  {                                                       │
│    "reply": "当然可以！你想要什么样的剑？",               │
│    "affinity": {"level": "友好", "score": 57},           │
│    "npc_name": "老铁匠",                                  │
│    "session_id": "session_xxx"                           │
│  }                                                       │
└─────────────────────────────────────────────────────────┘
    ↓
用户收到回复
```

---

## 💻 核心实现

### NPCAgent类完整实现

```python
from typing import Dict, List, Any, Optional
from datetime import datetime
import json

class NPCAgent:
    """
    NPC智能体
    
    核心功能:
    - 对话处理
    - 模块协调
    - 状态管理
    - 数据持久化
    """
    
    def __init__(
        self,
        npc_id: str,
        personality: NPCPersonality,
        llm: Any,
        data_dir: str = "./npc_data",
        config: NPCConfig = None
    ):
        """
        初始化NPC智能体
        
        Args:
            npc_id: NPC ID
            personality: NPC人设
            llm: LLM实例
            data_dir: 数据目录
            config: NPC配置
        """
        self.npc_id = npc_id
        self.personality = personality
        self.llm = llm
        self.data_dir = data_dir
        self.config = config or NPCConfig(npc_id=npc_id, personality=personality)
        
        # 初始化工具模块
        self._init_tools()
        
        # 初始化管理模块
        self._init_managers()
        
        # 初始化状态
        self.sessions = {}  # {player_id: [messages]}
        self.active_dialogue_sessions = set()
        
        print(f"[NPCAgent] {self.npc_id} ({self.personality.name}) 初始化完成")
    
    def _init_tools(self):
        """初始化工具模块"""
        from .memory_tool import MemoryTool, MemoryConfig
        from .rag_tool import RAGTool, RAGConfig
        from .note_tool import NoteTool
        from .terminal_tool import TerminalTool, TerminalConfig
        
        # 记忆工具
        memory_config = MemoryConfig(
            data_dir=self.data_dir,
            **self.config.memory_config
        )
        self.memory = MemoryTool(memory_config)
        
        # RAG工具
        if self.config.enable_rag:
            rag_config = RAGConfig(
                knowledge_base_dir=f"{self.data_dir}/knowledge_base/{self.npc_id}",
                index_dir=f"{self.data_dir}/rag_index/{self.npc_id}",
                **self.config.rag_config
            )
            self.rag = RAGTool(rag_config)
        else:
            self.rag = None
        
        # 笔记工具
        self.notes = NoteTool(notes_dir=f"{self.data_dir}/notes/{self.npc_id}")
        
        # 终端工具
        terminal_config = TerminalConfig(
            allowed_dirs=[self.data_dir],
            read_only=True
        )
        self.terminal = TerminalTool(terminal_config)
    
    def _init_managers(self):
        """初始化管理模块"""
        from .context_builder import ContextBuilder, ContextConfig
        from .relationship_manager import RelationshipManager
        from .dialogue_storage import DialogueStorage
        from .file_memory_store import FileMemoryStore, FileMemoryConfig
        
        # 上下文构建器
        context_config = ContextConfig(
            max_tokens=self.config.max_context_tokens,
            role_description=self.personality.to_prompt(),
            **self.config.context_config
        )
        self.context_builder = ContextBuilder(
            config=context_config,
            memory_tool=self.memory,
            rag_tool=self.rag
        )
        
        # 关系管理器
        self.relationship = RelationshipManager(
            db_path=f"{self.data_dir}/databases/npc_relationship.db"
        )
        
        # 对话存储
        if self.config.enable_dialogue_storage:
            self.dialogue_storage = DialogueStorage(
                db_path=f"{self.data_dir}/databases/dialogue_history.db",
                markdown_dir=f"{self.data_dir}/memories/dialogues"
            )
        else:
            self.dialogue_storage = None
        
        # 文件记忆存储
        if self.config.enable_file_memory:
            file_memory_config = FileMemoryConfig(
                base_dir=self.data_dir
            )
            self.file_memory_store = FileMemoryStore(file_memory_config)
        else:
            self.file_memory_store = None
    
    def chat(
        self,
        player_id: str,
        message: str,
        session_id: str = None,
        extra_context: Dict[str, Any] = None
    ) -> Dict[str, Any]:
        """
        处理对话 (10步流程)
        
        Args:
            player_id: 玩家ID
            message: 用户消息
            session_id: 会话ID (可选)
            extra_context: 额外上下文 (可选)
        
        Returns:
            Dict: {
                "reply": str,
                "affinity": dict,
                "npc_name": str,
                "npc_role": str,
                "session_id": str
            }
        """
        # 步骤1: 接收输入和验证
        if not session_id:
            session_id = f"{self.npc_id}_{player_id}"
        
        now = datetime.now().isoformat()
        
        # 步骤2: 加载会话历史
        if player_id not in self.sessions:
            self.sessions[player_id] = []
        
        conversation_history = self.sessions[player_id]
        
        # 步骤3-5: 构建上下文 (内部调用记忆、RAG、好感度)
        context = self._build_context(message, player_id, extra_context)
        
        # 步骤6: LLM生成回复
        reply = self._generate_reply(context, message)
        
        # 步骤7: 更新好感度
        new_affinity = self._update_affinity(player_id, message, reply)
        
        # 步骤8: 保存对话记录
        self._save_dialogue(player_id, message, reply, session_id, now)
        
        # 步骤9: 更新记忆
        self._update_memory(player_id, message, reply, now)
        
        # 步骤10: 更新会话历史
        conversation_history.append({
            "role": "user",
            "content": message,
            "timestamp": now
        })
        conversation_history.append({
            "role": "assistant",
            "content": reply,
            "timestamp": now
        })
        
        # 限制会话历史长度
        if len(conversation_history) > 20:
            self.sessions[player_id] = conversation_history[-20:]
        
        # 返回结果
        return {
            "reply": reply,
            "affinity": new_affinity.to_dict(),
            "npc_name": self.personality.name,
            "npc_role": self.personality.role,
            "session_id": session_id
        }
```

---

继续阅读第2部分...
