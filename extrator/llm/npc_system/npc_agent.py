"""
NPCAgent - NPC智能体核心类
基于 HelloAgents 第8、9、15章设计

整合:
- MemoryTool: 四层记忆系统
- RAGTool: 知识库检索
- ContextBuilder: GSSC上下文构建
- RelationshipManager: 好感度系统
- NoteTool: 结构化笔记
- DialogueStorage: 对话记录存储 (SQLite + Excel)
- FileMemoryStore: 文件系统记忆存储 (Markdown)
- StorageManager: 统一存储管理

实现完整的NPC对话流程:
1. 获取好感度 → 2. 检索记忆 → 3. 检索知识 → 4. 构建上下文
→ 5. 生成回复 → 6. 更新好感度 → 7. 存储记忆 → 8. 持久化对话记录

数据存储:
- SQLite数据库: 对话历史、记忆索引、好感度数据
- Excel导出: 对话记录导出
- Markdown文件: 人类可读的记忆文档
"""

import os
import json
from datetime import datetime
from dataclasses import dataclass, field
from typing import Dict, Any, Optional, List


# ==================== NPC人设配置 ====================

@dataclass
class NPCPersonality:
    """NPC性格配置"""
    name: str                          # NPC名字
    role: str                          # 角色定位 (如: 铁匠、商人)
    age: int = 30                      # 年龄
    gender: str = "未知"               # 性别
    traits: List[str] = field(default_factory=list)  # 性格特点
    background: str = ""               # 背景故事
    speech_style: str = ""             # 说话风格
    knowledge: List[str] = field(default_factory=list)  # 专业知识领域
    secrets: List[str] = field(default_factory=list)    # 隐藏信息 (高好感解锁)
    greeting: str = ""                 # 初次见面问候语
    
    def to_prompt(self) -> str:
        """转换为系统提示词"""
        traits_str = "、".join(self.traits) if self.traits else "普通"
        knowledge_str = "、".join(self.knowledge) if self.knowledge else "无特殊专长"
        
        prompt = f"""你是{self.name}，一位{self.age}岁的{self.gender}{self.role}。

【性格特点】
{traits_str}

【背景故事】
{self.background if self.background else "一个普通的NPC。"}

【说话风格】
{self.speech_style if self.speech_style else "正常说话，语气自然。"}

【专业知识】
{knowledge_str}

【重要规则】
1. 始终保持角色一致性，不要打破第四面墙
2. 根据好感度调整语气和信息分享程度
3. 记住之前的对话内容，保持连贯性
4. 回复要自然，像真人对话一样
"""
        return prompt
    
    def to_dict(self) -> dict:
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
    
    @classmethod
    def from_dict(cls, data: dict) -> 'NPCPersonality':
        return cls(**{k: v for k, v in data.items() if k in cls.__dataclass_fields__})


@dataclass
class NPCConfig:
    """NPC配置"""
    npc_id: str
    personality: NPCPersonality
    
    # 数据存储路径
    data_dir: str = "./npc_data"
    memory_db: str = ""
    relationship_db: str = ""
    dialogue_db: str = ""
    notes_dir: str = ""
    knowledge_base: str = ""
    rag_index: str = ""
    memories_dir: str = ""
    exports_dir: str = ""
    
    # 存储选项
    enable_dialogue_storage: bool = True    # 启用对话存储
    enable_file_memory: bool = True         # 启用文件记忆存储
    auto_save_dialogue_md: bool = True      # 自动保存对话到Markdown
    auto_export_excel: bool = False         # 会话结束时自动导出Excel
    
    def __post_init__(self):
        # 创建数据目录
        os.makedirs(self.data_dir, exist_ok=True)
        
        # 设置默认路径 - 使用统一的目录结构
        databases_dir = os.path.join(self.data_dir, "databases")
        os.makedirs(databases_dir, exist_ok=True)
        
        if not self.memory_db:
            self.memory_db = os.path.join(databases_dir, "npc_memory.db")
        if not self.relationship_db:
            self.relationship_db = os.path.join(databases_dir, "npc_relationship.db")
        if not self.dialogue_db:
            self.dialogue_db = os.path.join(databases_dir, "dialogue_history.db")
        if not self.notes_dir:
            self.notes_dir = os.path.join(self.data_dir, "notes", self.npc_id)
        if not self.knowledge_base:
            self.knowledge_base = os.path.join(self.data_dir, "knowledge_base", self.npc_id)
        if not self.rag_index:
            self.rag_index = os.path.join(self.data_dir, "rag_index", self.npc_id)
        if not self.memories_dir:
            self.memories_dir = os.path.join(self.data_dir, "memories")
        if not self.exports_dir:
            self.exports_dir = os.path.join(self.data_dir, "exports", "excel")
        
        # 创建NPC特定目录
        for dir_path in [self.notes_dir, self.knowledge_base, self.rag_index, 
                         self.memories_dir, self.exports_dir]:
            os.makedirs(dir_path, exist_ok=True)


# ==================== NPCAgent ====================

class NPCAgent:
    """
    NPC智能体
    
    整合记忆、RAG、上下文、好感度、对话存储等系统，提供完整的NPC对话能力
    
    对话流程:
    1. 获取好感度信息
    2. 从记忆系统检索相关记忆
    3. 从RAG知识库检索相关知识
    4. 检索相关笔记
    5. 使用ContextBuilder构建优化的上下文
    6. 调用LLM生成回复
    7. 更新好感度
    8. 存储对话到记忆系统
    9. 持久化对话记录 (SQLite + Markdown)
    10. 可选: 导出到Excel
    
    存储系统:
    - SQLite: 对话历史、记忆索引、好感度 (兼容原general_chat.py)
    - Markdown: 人类可读的记忆文档和对话记录
    - Excel: 对话记录导出 (兼容原export_history.py)
    """
    
    def __init__(self, config: NPCConfig, llm=None):
        """
        初始化NPC智能体
        
        Args:
            config: NPC配置
            llm: LangChain的ChatOllama或其他LLM
        """
        self.config = config
        self.llm = llm
        self.npc_id = config.npc_id
        self.personality = config.personality
        
        # 初始化性能监控器
        from .performance_monitor import PerformanceMonitor
        self.perf_monitor = PerformanceMonitor(enabled=True)
        
        # 导入组件
        from .memory_tool import MemoryTool, MemoryConfig
        from .context_builder import ContextBuilder, ContextConfig
        from .relationship_manager import RelationshipManager
        from .note_tool import NoteTool
        
        # 初始化记忆系统
        memory_config = MemoryConfig(db_path=config.memory_db)
        self.memory = MemoryTool(memory_config)
        
        # 初始化好感度系统
        self.relationship = RelationshipManager(config.relationship_db, llm)
        
        # 初始化笔记工具
        self.notes = NoteTool(workspace=config.notes_dir)
        
        # 初始化对话存储 (SQLite + Excel)
        self.dialogue_storage = None
        if config.enable_dialogue_storage:
            try:
                from .dialogue_storage import DialogueStorage
                self.dialogue_storage = DialogueStorage(
                    db_path=config.dialogue_db,
                    export_dir=config.exports_dir,
                    auto_export=config.auto_export_excel
                )
                print(f"[NPCAgent:{self.npc_id}] 对话存储已初始化")
            except Exception as e:
                print(f"[NPCAgent:{self.npc_id}] 对话存储初始化失败: {e}")
        
        # 初始化文件记忆存储 (Markdown)
        self.file_memory_store = None
        if config.enable_file_memory:
            try:
                from .file_memory_store import FileMemoryStore, FileMemoryConfig
                file_config = FileMemoryConfig(base_dir=config.memories_dir)
                self.file_memory_store = FileMemoryStore(file_config)
                print(f"[NPCAgent:{self.npc_id}] 文件记忆存储已初始化")
            except Exception as e:
                print(f"[NPCAgent:{self.npc_id}] 文件记忆存储初始化失败: {e}")
        
        # 初始化RAG (可选)
        self.rag = None
        if os.path.exists(config.knowledge_base):
            try:
                from .rag_tool import RAGTool, RAGConfig
                rag_config = RAGConfig(
                    knowledge_base_path=config.knowledge_base,
                    index_path=config.rag_index
                )
                self.rag = RAGTool(rag_config, llm)
                print(f"[NPCAgent:{self.npc_id}] RAG知识库已加载")
            except Exception as e:
                print(f"[NPCAgent:{self.npc_id}] RAG初始化失败: {e}")
        
        # 初始化上下文构建器
        context_config = ContextConfig(
            max_tokens=3000,
            role_description=config.personality.to_prompt()
        )
        self.context_builder = ContextBuilder(
            config=context_config,
            memory_tool=self.memory,
            rag_tool=self.rag,
            note_tool=self.notes,
            relationship_manager=self.relationship,
            llm=llm
        )
        
        # 会话管理
        self.sessions: Dict[str, List] = {}  # player_id -> conversation_history
        self.active_dialogue_sessions: Dict[str, str] = {}  # player_id -> dialogue_session_id
        
        print(f"[NPCAgent:{self.npc_id}] 初始化完成: {self.personality.name}")
    
    def chat(self, player_id: str, message: str,
             session_id: str = None,
             extra_context: Dict = None) -> Dict[str, Any]:
        """
        与玩家对话
        
        Args:
            player_id: 玩家ID
            message: 玩家消息
            session_id: 会话ID (可选)
            extra_context: 额外的上下文信息 (如场景、时间等)
            
        Returns:
            Dict: {
                "reply": str,           # NPC回复
                "affinity": dict,       # 好感度信息
                "memories_used": int,   # 使用的记忆数量
                "rag_used": int,        # 使用的RAG结果数量
                "session_id": str,      # 会话ID
            }
        """
        if not session_id:
            session_id = f"session_{player_id}_{datetime.now().strftime('%Y%m%d')}"
        
        # 获取或创建会话历史
        if player_id not in self.sessions:
            self.sessions[player_id] = []
        conversation_history = self.sessions[player_id]
        
        # 确保对话存储会话已创建
        if self.dialogue_storage and player_id not in self.active_dialogue_sessions:
            dialogue_session = self.dialogue_storage.start_session(
                npc_id=self.npc_id,
                player_id=player_id,
                session_id=session_id
            )
            self.active_dialogue_sessions[player_id] = dialogue_session.session_id
        
        # 1. 获取好感度信息
        affinity = self.relationship.get_affinity(self.npc_id, player_id)
        
        # 2. 构建自定义上下文包
        from .context_builder import ContextPacket
        custom_packets = []
        
        # 添加秘密信息 (高好感度解锁)
        secrets_content = self._get_secrets_for_affinity(affinity)
        if secrets_content:
            custom_packets.append(ContextPacket(
                content=secrets_content,
                source="custom",
                relevance_score=0.8,
                priority=60,
                metadata={"type": "secrets"}
            ))
        
        # 添加额外上下文
        if extra_context:
            context_str = json.dumps(extra_context, ensure_ascii=False)
            custom_packets.append(ContextPacket(
                content=f"[场景信息] {context_str}",
                source="custom",
                relevance_score=0.7,
                priority=55,
                metadata={"type": "scene_context"}
            ))
        
        # 3. 使用ContextBuilder构建完整上下文
        from .context_builder import Message
        history_messages = [
            Message(content=h["content"], role=h["role"], 
                   timestamp=datetime.fromisoformat(h["timestamp"]))
            for h in conversation_history[-10:]  # 最近10条
        ]
        
        context = self.context_builder.build(
            user_query=message,
            conversation_history=history_messages,
            custom_packets=custom_packets,
            user_id=player_id,
            session_id=session_id,
            npc_id=self.npc_id,
            include_relationship=True
        )
        
        # 4. 生成回复
        reply = self._generate_reply(context, message)
        
        # 5. 更新好感度
        new_affinity = self.relationship.update_affinity(
            self.npc_id, player_id,
            message, reply,
            use_llm=(self.llm is not None)
        )
        
        # 6. 存储记忆
        now = datetime.now().isoformat()
        
        # 存储到工作记忆
        self.memory.execute(
            "add",
            content=f"玩家{player_id}说: {message}",
            memory_type="working",
            importance=0.6,
            user_id=player_id,
            session_id=session_id,
            metadata={"type": "player_message"}
        )
        
        self.memory.execute(
            "add",
            content=f"{self.personality.name}回复: {reply}",
            memory_type="working",
            importance=0.6,
            user_id=player_id,
            session_id=session_id,
            metadata={"type": "npc_reply"}
        )
        
        # 存储到情景记忆 (重要对话)
        if len(message) > 20 or new_affinity.score != affinity.score:
            self.memory.execute(
                "add",
                content=f"与玩家{player_id}的对话 - 玩家: {message[:100]} | {self.personality.name}: {reply[:100]}",
                memory_type="episodic",
                importance=0.7,
                user_id=player_id,
                session_id=session_id,
                event_type="dialogue",
                metadata={
                    "affinity_before": affinity.score,
                    "affinity_after": new_affinity.score
                }
            )
        
        # 7. 持久化对话记录到SQLite
        if self.dialogue_storage:
            try:
                dialogue_session_id = self.active_dialogue_sessions.get(player_id, session_id)
                self.dialogue_storage.add_dialogue_turn(
                    session_id=dialogue_session_id,
                    player_message=message,
                    npc_reply=reply,
                    npc_id=self.npc_id,
                    player_id=player_id,
                    metadata={
                        "affinity_before": affinity.score,
                        "affinity_after": new_affinity.score,
                        "affinity_level": new_affinity.level.value if hasattr(new_affinity.level, 'value') else str(new_affinity.level)
                    }
                )
            except Exception as e:
                print(f"[NPCAgent:{self.npc_id}] 对话存储失败: {e}")
        
        # 8. 保存对话到Markdown文件
        if self.file_memory_store and self.config.auto_save_dialogue_md:
            try:
                self.file_memory_store.save_dialogue(
                    npc_id=self.npc_id,
                    player_id=player_id,
                    player_message=message,
                    npc_reply=reply,
                    session_id=session_id,
                    metadata={
                        "affinity": new_affinity.score,
                        "affinity_level": new_affinity.level.value if hasattr(new_affinity.level, 'value') else str(new_affinity.level)
                    }
                )
            except Exception as e:
                print(f"[NPCAgent:{self.npc_id}] Markdown保存失败: {e}")
        
        # 9. 保存重要事件到情景记忆文件
        if self.file_memory_store and (len(message) > 50 or abs(new_affinity.score - affinity.score) >= 3):
            try:
                self.file_memory_store.save_episodic_memory(
                    npc_id=self.npc_id,
                    player_id=player_id,
                    event_type="important_dialogue",
                    content=f"玩家说: {message}\n\n{self.personality.name}回复: {reply}",
                    importance=0.8,
                    metadata={
                        "affinity_change": new_affinity.score - affinity.score,
                        "session_id": session_id
                    }
                )
            except Exception as e:
                print(f"[NPCAgent:{self.npc_id}] 情景记忆保存失败: {e}")
        
        # 10. 更新会话历史
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
        
        return {
            "reply": reply,
            "affinity": new_affinity.to_dict(),
            "npc_name": self.personality.name,
            "npc_role": self.personality.role,
            "session_id": session_id
        }
    
    def _generate_reply(self, context: str, message: str) -> str:
        """生成回复"""
        if not self.llm:
            # 无LLM时的默认回复
            affinity_replies = {
                "陌生": f"（{self.personality.name}礼貌地点点头）你好。",
                "熟悉": f"（{self.personality.name}微笑着）哦，是你啊。",
                "友好": f"（{self.personality.name}热情地）嘿，朋友！",
                "亲密": f"（{self.personality.name}亲切地）来了啊，正想找你呢。",
                "挚友": f"（{self.personality.name}开心地）太好了，你来了！"
            }
            return affinity_replies.get("熟悉", f"（{self.personality.name}看着你）...")
        
        messages = [
            {"role": "system", "content": context},
            {"role": "user", "content": message}
        ]
        
        try:
            response = self.llm.invoke(messages)
            if hasattr(response, 'content'):
                return response.content
            return str(response)
        except Exception as e:
            print(f"[NPCAgent:{self.npc_id}] LLM调用失败: {e}")
            return f"（{self.personality.name}似乎在思考什么...）"
    
    def _get_secrets_for_affinity(self, affinity) -> str:
        """根据好感度获取可分享的秘密"""
        from .relationship_manager import AffinityLevel
        
        secrets = self.personality.secrets
        if not secrets:
            return ""
        
        unlocked = []
        
        if affinity.level in [AffinityLevel.BEST_FRIEND]:
            unlocked = secrets
        elif affinity.level == AffinityLevel.CLOSE:
            unlocked = secrets[:len(secrets)*2//3]
        elif affinity.level == AffinityLevel.FRIENDLY:
            unlocked = secrets[:len(secrets)//3]
        
        if unlocked:
            secrets_str = "\n".join(f"- {s}" for s in unlocked)
            return f"[你可以分享的秘密]\n{secrets_str}"
        
        return ""
    
    # ==================== 便捷方法 ====================
    
    def chat_with_perf(self, player_id: str, message: str,
                       session_id: str = None,
                       extra_context: Dict = None,
                       print_trace: bool = False) -> Dict[str, Any]:
        """
        带性能监控的对话方法
        
        与 chat() 功能相同，但会详细追踪每个步骤的耗时
        
        Args:
            player_id: 玩家ID
            message: 玩家消息
            session_id: 会话ID
            extra_context: 额外上下文
            print_trace: 是否立即打印追踪结果
            
        Returns:
            Dict: 包含 reply, affinity, performance 等信息
        """
        # 开始追踪
        trace = self.perf_monitor.start_dialogue(self.npc_id, player_id, message)
        
        try:
            if not session_id:
                session_id = f"session_{player_id}_{datetime.now().strftime('%Y%m%d')}"
            
            # 获取或创建会话历史
            if player_id not in self.sessions:
                self.sessions[player_id] = []
            conversation_history = self.sessions[player_id]
            
            # 确保对话存储会话已创建
            if self.dialogue_storage and player_id not in self.active_dialogue_sessions:
                dialogue_session = self.dialogue_storage.start_session(
                    npc_id=self.npc_id,
                    player_id=player_id,
                    session_id=session_id
                )
                self.active_dialogue_sessions[player_id] = dialogue_session.session_id
            
            # 1. 获取好感度信息
            with self.perf_monitor.track("get_affinity"):
                affinity = self.relationship.get_affinity(self.npc_id, player_id)
            
            # 2. 构建自定义上下文包
            with self.perf_monitor.track("build_context_packets"):
                from .context_builder import ContextPacket
                custom_packets = []
                
                secrets_content = self._get_secrets_for_affinity(affinity)
                if secrets_content:
                    custom_packets.append(ContextPacket(
                        content=secrets_content,
                        source="custom",
                        relevance_score=0.8,
                        priority=60,
                        metadata={"type": "secrets"}
                    ))
                
                if extra_context:
                    context_str = json.dumps(extra_context, ensure_ascii=False)
                    custom_packets.append(ContextPacket(
                        content=f"[场景信息] {context_str}",
                        source="custom",
                        relevance_score=0.7,
                        priority=55,
                        metadata={"type": "scene_context"}
                    ))
            
            # 3. 使用ContextBuilder构建完整上下文 (内部包含memory_search, rag_search, notes_search)
            with self.perf_monitor.track("context_build"):
                from .context_builder import Message
                history_messages = [
                    Message(content=h["content"], role=h["role"], 
                           timestamp=datetime.fromisoformat(h["timestamp"]))
                    for h in conversation_history[-10:]
                ]
                
                context = self.context_builder.build(
                    user_query=message,
                    conversation_history=history_messages,
                    custom_packets=custom_packets,
                    user_id=player_id,
                    session_id=session_id,
                    npc_id=self.npc_id,
                    include_relationship=True
                )
            
            # 4. 生成回复 (LLM调用)
            with self.perf_monitor.track("llm_generate"):
                reply = self._generate_reply(context, message)
            
            # 5. 更新好感度
            with self.perf_monitor.track("update_affinity"):
                new_affinity = self.relationship.update_affinity(
                    self.npc_id, player_id,
                    message, reply,
                    use_llm=(self.llm is not None)
                )
            
            # 6. 存储到工作记忆
            now = datetime.now().isoformat()
            with self.perf_monitor.track("store_working_memory"):
                self.memory.execute(
                    "add",
                    content=f"玩家{player_id}说: {message}",
                    memory_type="working",
                    importance=0.6,
                    user_id=player_id,
                    session_id=session_id,
                    metadata={"type": "player_message"}
                )
                
                self.memory.execute(
                    "add",
                    content=f"{self.personality.name}回复: {reply}",
                    memory_type="working",
                    importance=0.6,
                    user_id=player_id,
                    session_id=session_id,
                    metadata={"type": "npc_reply"}
                )
            
            # 7. 存储到情景记忆 (重要对话)
            with self.perf_monitor.track("store_episodic_memory"):
                if len(message) > 20 or new_affinity.score != affinity.score:
                    self.memory.execute(
                        "add",
                        content=f"与玩家{player_id}的对话 - 玩家: {message[:100]} | {self.personality.name}: {reply[:100]}",
                        memory_type="episodic",
                        importance=0.7,
                        user_id=player_id,
                        session_id=session_id,
                        event_type="dialogue",
                        metadata={
                            "affinity_before": affinity.score,
                            "affinity_after": new_affinity.score
                        }
                    )
            
            # 8. 持久化对话记录到SQLite
            with self.perf_monitor.track("save_dialogue_sqlite"):
                if self.dialogue_storage:
                    try:
                        dialogue_session_id = self.active_dialogue_sessions.get(player_id, session_id)
                        self.dialogue_storage.add_dialogue_turn(
                            session_id=dialogue_session_id,
                            player_message=message,
                            npc_reply=reply,
                            npc_id=self.npc_id,
                            player_id=player_id,
                            metadata={
                                "affinity_before": affinity.score,
                                "affinity_after": new_affinity.score,
                                "affinity_level": new_affinity.level.value if hasattr(new_affinity.level, 'value') else str(new_affinity.level)
                            }
                        )
                    except Exception as e:
                        print(f"[NPCAgent:{self.npc_id}] 对话存储失败: {e}")
            
            # 9. 保存对话到Markdown文件
            with self.perf_monitor.track("save_dialogue_markdown"):
                if self.file_memory_store and self.config.auto_save_dialogue_md:
                    try:
                        self.file_memory_store.save_dialogue(
                            npc_id=self.npc_id,
                            player_id=player_id,
                            player_message=message,
                            npc_reply=reply,
                            session_id=session_id,
                            metadata={
                                "affinity": new_affinity.score,
                                "affinity_level": new_affinity.level.value if hasattr(new_affinity.level, 'value') else str(new_affinity.level)
                            }
                        )
                    except Exception as e:
                        print(f"[NPCAgent:{self.npc_id}] Markdown保存失败: {e}")
            
            # 10. 保存重要事件到情景记忆文件
            with self.perf_monitor.track("save_episodic_file"):
                if self.file_memory_store and (len(message) > 50 or abs(new_affinity.score - affinity.score) >= 3):
                    try:
                        self.file_memory_store.save_episodic_memory(
                            npc_id=self.npc_id,
                            player_id=player_id,
                            event_type="important_dialogue",
                            content=f"玩家说: {message}\n\n{self.personality.name}回复: {reply}",
                            importance=0.8,
                            metadata={
                                "affinity_change": new_affinity.score - affinity.score,
                                "session_id": session_id
                            }
                        )
                    except Exception as e:
                        print(f"[NPCAgent:{self.npc_id}] 情景记忆保存失败: {e}")
            
            # 更新会话历史
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
            
            if len(conversation_history) > 20:
                self.sessions[player_id] = conversation_history[-20:]
            
            # 完成追踪
            self.perf_monitor.end_dialogue(trace, success=True, npc_reply=reply)
            
            # 打印追踪结果
            if print_trace:
                trace.print_breakdown()
            
            return {
                "reply": reply,
                "affinity": new_affinity.to_dict(),
                "npc_name": self.personality.name,
                "npc_role": self.personality.role,
                "session_id": session_id,
                "performance": trace.to_dict()
            }
            
        except Exception as e:
            self.perf_monitor.end_dialogue(trace, success=False, error=str(e))
            raise
    
    def get_performance_stats(self) -> Dict:
        """获取性能统计信息"""
        return {
            "summary": self.perf_monitor.get_dialogue_summary(),
            "step_statistics": self.perf_monitor.get_statistics(),
            "bottlenecks": self.perf_monitor.get_bottlenecks(5)
        }
    
    def print_performance_stats(self):
        """打印性能统计"""
        self.perf_monitor.print_statistics()
    
    def export_performance_report(self, filepath: str = None) -> str:
        """导出性能报告"""
        if not filepath:
            filepath = f"performance_report_{self.npc_id}_{datetime.now().strftime('%Y%m%d_%H%M%S')}.json"
        return self.perf_monitor.export_report(filepath)
    
    def get_status(self, player_id: str = None) -> Dict[str, Any]:
        """获取NPC状态"""
        status = {
            "npc_id": self.npc_id,
            "name": self.personality.name,
            "role": self.personality.role,
            "memory_summary": self.memory.execute("summary"),
        }
        
        if player_id:
            affinity = self.relationship.get_affinity(self.npc_id, player_id)
            status["relationship"] = affinity.to_dict()
            status["conversation_count"] = len(self.sessions.get(player_id, []))
        
        return status
    
    def get_greeting(self, player_id: str) -> str:
        """获取问候语"""
        affinity = self.relationship.get_affinity(self.npc_id, player_id)
        
        if affinity.interaction_count == 0:
            # 初次见面
            if self.personality.greeting:
                return self.personality.greeting
            return f"你好，我是{self.personality.name}，{self.personality.role}。有什么可以帮你的吗？"
        
        # 根据好感度返回不同问候
        greetings = {
            "陌生": f"嗯？又是你。",
            "熟悉": f"哦，你来了。",
            "友好": f"嘿，{player_id}！好久不见。",
            "亲密": f"太好了，你来了！我正想找你呢。",
            "挚友": f"老朋友！快来快来，我有好东西给你看。"
        }
        
        return greetings.get(affinity.level.value, f"你好。")
    
    def remember(self, content: str, memory_type: str = "semantic",
                 importance: float = 0.7, user_id: str = "") -> str:
        """添加记忆"""
        return self.memory.execute(
            "add",
            content=content,
            memory_type=memory_type,
            importance=importance,
            user_id=user_id
        )
    
    def recall(self, query: str, limit: int = 5, user_id: str = "") -> List:
        """回忆相关记忆"""
        return self.memory.execute(
            "search",
            query=query,
            limit=limit,
            user_id=user_id
        )
    
    def take_note(self, title: str, content: str,
                  note_type: str = "general", tags: List[str] = None) -> str:
        """记笔记"""
        return self.notes.execute(
            "create",
            title=title,
            content=content,
            note_type=note_type,
            tags=tags or [self.npc_id]
        )
    
    def add_knowledge(self, content: str, doc_id: str = None) -> str:
        """添加知识到RAG"""
        if not self.rag:
            return "RAG未初始化"
        return self.rag.add_text(content, doc_id=doc_id)
    
    def consolidate_memories(self, user_id: str = "", threshold: float = 0.6) -> Dict:
        """整合记忆 (工作记忆 → 情景记忆)"""
        return self.memory.execute(
            "consolidate",
            threshold=threshold,
            user_id=user_id
        )
    
    def forget_old_memories(self, days: float = 30, user_id: str = "") -> Dict:
        """遗忘旧记忆"""
        return self.memory.execute(
            "forget",
            strategy="time",
            threshold=days,
            user_id=user_id
        )
    
    def clear_session(self, player_id: str):
        """清除会话历史"""
        if player_id in self.sessions:
            del self.sessions[player_id]
        
        # 结束对话存储会话
        if self.dialogue_storage and player_id in self.active_dialogue_sessions:
            session_id = self.active_dialogue_sessions[player_id]
            self.dialogue_storage.end_session(session_id)
            del self.active_dialogue_sessions[player_id]
    
    def export_dialogue_history(self, player_id: str = None,
                                filename: str = None) -> Optional[str]:
        """
        导出对话历史到Excel
        
        Args:
            player_id: 玩家ID (可选，不指定则导出所有)
            filename: 文件名 (可选)
            
        Returns:
            str: 导出文件路径
        """
        if not self.dialogue_storage:
            print(f"[NPCAgent:{self.npc_id}] 对话存储未启用")
            return None
        
        if player_id and player_id in self.active_dialogue_sessions:
            session_id = self.active_dialogue_sessions[player_id]
            return self.dialogue_storage.export_to_excel(session_id, filename)
        else:
            # 导出所有对话
            return self.dialogue_storage.export_to_excel(filename=filename)
    
    def get_dialogue_history(self, player_id: str = None,
                             limit: int = 50) -> List[Dict]:
        """
        获取对话历史
        
        Args:
            player_id: 玩家ID (可选)
            limit: 返回数量限制
            
        Returns:
            List[Dict]: 对话记录列表
        """
        if not self.dialogue_storage:
            return []
        
        messages = self.dialogue_storage.get_history(
            npc_id=self.npc_id,
            player_id=player_id,
            limit=limit
        )
        
        return [msg.to_dict() for msg in messages]
    
    def get_dialogue_stats(self) -> Dict[str, Any]:
        """获取对话统计信息"""
        stats = {
            "npc_id": self.npc_id,
            "npc_name": self.personality.name,
            "active_sessions": len(self.active_dialogue_sessions),
            "memory_summary": self.memory.execute("summary"),
        }
        
        if self.dialogue_storage:
            stats["dialogue_storage"] = self.dialogue_storage.get_stats()
        
        if self.file_memory_store:
            stats["file_memory"] = self.file_memory_store.get_stats(self.npc_id)
        
        return stats
    
    def load_dialogue_from_files(self, player_id: str = None,
                                 date: str = None,
                                 limit: int = 10) -> List[Dict]:
        """
        从Markdown文件加载对话记录
        
        Args:
            player_id: 玩家ID (可选)
            date: 日期 (格式: YYYYMMDD)
            limit: 返回数量限制
            
        Returns:
            List[Dict]: 对话记录列表
        """
        if not self.file_memory_store:
            return []
        
        return self.file_memory_store.load_dialogues(
            npc_id=self.npc_id,
            player_id=player_id,
            date=date,
            limit=limit
        )
    
    def save_knowledge(self, topic: str, content: str,
                       importance: float = 0.7,
                       concepts: List[str] = None) -> Optional[str]:
        """
        保存知识到语义记忆
        
        同时保存到SQLite和Markdown文件
        
        Args:
            topic: 知识主题
            content: 知识内容
            importance: 重要性
            concepts: 相关概念列表
            
        Returns:
            str: 记忆ID
        """
        # 保存到SQLite
        memory_id = self.memory.execute(
            "add",
            content=f"[{topic}] {content}",
            memory_type="semantic",
            importance=importance,
            concepts=concepts or []
        )
        
        # 保存到Markdown文件
        if self.file_memory_store:
            self.file_memory_store.save_semantic_memory(
                npc_id=self.npc_id,
                topic=topic,
                content=content,
                importance=importance,
                concepts=concepts
            )
        
        return memory_id
    
    def save_config(self, path: str = None):
        """保存NPC配置"""
        if not path:
            path = os.path.join(self.config.data_dir, f"{self.npc_id}_config.json")
        
        config_data = {
            "npc_id": self.npc_id,
            "personality": self.personality.to_dict(),
            "data_dir": self.config.data_dir
        }
        
        with open(path, 'w', encoding='utf-8') as f:
            json.dump(config_data, f, ensure_ascii=False, indent=2)
        
        return path


# ==================== 便捷函数 ====================

def create_npc(npc_id: str, name: str, role: str,
               traits: List[str] = None,
               background: str = "",
               speech_style: str = "",
               knowledge: List[str] = None,
               secrets: List[str] = None,
               data_dir: str = "./npc_data",
               llm=None) -> NPCAgent:
    """
    快速创建NPC
    
    Example:
        blacksmith = create_npc(
            npc_id="blacksmith_001",
            name="老铁匠",
            role="铁匠",
            traits=["严肃", "专业", "热心"],
            background="在这个镇上打铁30年...",
            speech_style="说话干脆利落，常用专业术语",
            knowledge=["武器锻造", "金属材料"],
            secrets=["知道镇长的秘密"],
            llm=chat_ollama
        )
    """
    personality = NPCPersonality(
        name=name,
        role=role,
        traits=traits or [],
        background=background,
        speech_style=speech_style,
        knowledge=knowledge or [],
        secrets=secrets or []
    )
    
    config = NPCConfig(
        npc_id=npc_id,
        personality=personality,
        data_dir=data_dir
    )
    
    return NPCAgent(config, llm)


def load_npc_from_json(json_path: str, llm=None, data_dir: str = None) -> NPCAgent:
    """从JSON文件加载NPC配置"""
    with open(json_path, 'r', encoding='utf-8') as f:
        data = json.load(f)
    
    personality = NPCPersonality.from_dict(data.get("personality", data))
    
    config = NPCConfig(
        npc_id=data.get("npc_id", personality.name),
        personality=personality,
        data_dir=data_dir or data.get("data_dir", "./npc_data")
    )
    
    return NPCAgent(config, llm)


def load_npc_from_dict(data: dict, llm=None, data_dir: str = None) -> NPCAgent:
    """从字典加载NPC配置"""
    personality = NPCPersonality.from_dict(data.get("personality", data))
    
    config = NPCConfig(
        npc_id=data.get("npc_id", personality.name),
        personality=personality,
        data_dir=data_dir or data.get("data_dir", "./npc_data")
    )
    
    return NPCAgent(config, llm)
