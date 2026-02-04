"""
ContextBuilder - GSSC上下文构建流水线
基于 HelloAgents 第9章设计

GSSC流程:
- Gather: 汇集多源信息 (记忆、RAG、笔记、终端)
- Select: 智能信息选择
- Structure: 结构化输出
- Compress: 兜底压缩

整合:
- MemoryTool: 四层记忆系统
- RAGTool: 知识库检索
- NoteTool: 结构化笔记
- RelationshipManager: 好感度系统
"""

import math
from datetime import datetime
from dataclasses import dataclass, field
from typing import List, Dict, Any, Optional, TYPE_CHECKING

# 类型检查时导入
if TYPE_CHECKING:
    from .memory_tool import MemoryTool, MemoryItem
    from .rag_tool import RAGTool
    from .note_tool import NoteTool
    from .relationship_manager import RelationshipManager


# ==================== 数据结构 ====================

@dataclass
class ContextConfig:
    """上下文构建配置"""
    max_tokens: int = 3000              # 最大token数
    reserve_ratio: float = 0.2          # 系统指令预留比例
    min_relevance: float = 0.1          # 最低相关性阈值
    enable_compression: bool = True     # 是否启用压缩
    recency_weight: float = 0.3         # 时间近因性权重
    relevance_weight: float = 0.7       # 相关性权重
    
    # 各来源的token预算分配
    memory_budget: float = 0.3          # 记忆预算 (30%)
    rag_budget: float = 0.25            # RAG预算 (25%)
    history_budget: float = 0.2         # 对话历史预算 (20%)
    notes_budget: float = 0.1           # 笔记预算 (10%)
    custom_budget: float = 0.15         # 自定义预算 (15%)
    
    # 角色描述 (用于NPC)
    role_description: str = ""
    
    # [优化] 快速模式 - 禁用耗时的检索操作
    # 注意：这些字段必须是 dataclass 字段，否则外部传 fast_mode=True 会报
    # "ContextConfig has no attribute 'fast_mode'"。
    fast_mode: bool = False             # 快速模式开关
    enable_memory_search: bool = True   # 是否启用记忆检索
    enable_rag_search: bool = True      # 是否启用RAG检索
    enable_notes_search: bool = True    # 是否启用笔记检索

    
    def __post_init__(self):
        assert 0.0 <= self.reserve_ratio <= 1.0
        assert 0.0 <= self.min_relevance <= 1.0
        # 快速模式下禁用所有检索
        # 用 getattr 做边界收束，防止老版本对象缺字段导致 AttributeError
        if getattr(self, "fast_mode", False):
            self.enable_memory_search = False
            self.enable_rag_search = False
            self.enable_notes_search = False
        # 权重之和应为1
        total = self.recency_weight + self.relevance_weight
        if abs(total - 1.0) > 0.01:
            self.recency_weight /= total
            self.relevance_weight /= total


@dataclass
class ContextPacket:
    """候选信息包"""
    content: str
    source: str = "general"             # 来源: system/memory/rag/history/notes/custom
    timestamp: datetime = field(default_factory=datetime.now)
    token_count: int = 0
    relevance_score: float = 0.5
    priority: int = 0                   # 优先级 (越高越优先)
    metadata: Dict[str, Any] = field(default_factory=dict)
    
    def __post_init__(self):
        if self.token_count == 0:
            self.token_count = self._estimate_tokens(self.content)
        self.relevance_score = max(0.0, min(1.0, self.relevance_score))
    
    @staticmethod
    def _estimate_tokens(text: str) -> int:
        """估算token数"""
        if not text:
            return 0
        chinese_chars = sum(1 for ch in text if '\u4e00' <= ch <= '\u9fff')
        other_chars = len(text) - chinese_chars
        return chinese_chars + other_chars // 4


@dataclass
class Message:
    """对话消息"""
    content: str
    role: str = "user"
    timestamp: datetime = field(default_factory=datetime.now)


# ==================== ContextBuilder ====================

class ContextBuilder:
    """
    上下文构建器
    
    实现GSSC流水线:
    1. Gather: 从多个来源汇集信息
    2. Select: 根据相关性和新近性选择
    3. Structure: 组织成结构化模板
    4. Compress: 压缩超限内容
    
    支持的信息来源:
    - 系统指令 (角色设定、好感度)
    - 记忆系统 (工作记忆、情景记忆、语义记忆)
    - RAG知识库
    - 对话历史
    - 结构化笔记
    - 自定义信息包
    """
    
    def __init__(self, 
                 config: ContextConfig = None,
                 memory_tool: 'MemoryTool' = None,
                 rag_tool: 'RAGTool' = None,
                 note_tool: 'NoteTool' = None,
                 relationship_manager: 'RelationshipManager' = None,
                 llm=None):
        self.config = config or ContextConfig()
        self.memory_tool = memory_tool
        self.rag_tool = rag_tool
        self.note_tool = note_tool
        self.relationship_manager = relationship_manager
        self.llm = llm
    
    def build(self,
              user_query: str,
              conversation_history: List[Message] = None,
              system_instructions: str = None,
              custom_packets: List[ContextPacket] = None,
              user_id: str = "",
              session_id: str = "",
              npc_id: str = "",
              include_relationship: bool = True) -> str:
        """
        构建优化的上下文
        
        Args:
            user_query: 用户查询
            conversation_history: 对话历史
            system_instructions: 系统指令 (会与角色描述合并)
            custom_packets: 自定义信息包
            user_id: 用户ID (用于记忆和好感度)
            session_id: 会话ID
            npc_id: NPC ID (用于好感度)
            include_relationship: 是否包含好感度信息
            
        Returns:
            str: 结构化的上下文字符串
        """
        # 1. Gather - 汇集信息
        packets = self._gather(
            user_query=user_query,
            conversation_history=conversation_history,
            system_instructions=system_instructions,
            custom_packets=custom_packets,
            user_id=user_id,
            session_id=session_id,
            npc_id=npc_id,
            include_relationship=include_relationship
        )
        
        # 2. Select - 选择信息
        available_tokens = int(self.config.max_tokens * (1 - self.config.reserve_ratio))
        selected = self._select(packets, user_query, available_tokens)
        
        # 3. Structure - 结构化输出
        context = self._structure(selected, user_query)
        
        # 4. Compress - 压缩
        if self.config.enable_compression:
            context = self._compress(context, self.config.max_tokens)
        
        return context
    
    def _gather(self,
                user_query: str,
                conversation_history: List[Message] = None,
                system_instructions: str = None,
                custom_packets: List[ContextPacket] = None,
                user_id: str = "",
                session_id: str = "",
                npc_id: str = "",
                include_relationship: bool = True) -> List[ContextPacket]:
        """Gather阶段 - 汇集所有候选信息"""
        packets = []
        
        # ========== 1. 系统指令 (最高优先级) ==========
        system_content = []
        
        # 角色描述
        if self.config.role_description:
            system_content.append(self.config.role_description)
        
        # 自定义系统指令
        if system_instructions:
            system_content.append(system_instructions)
        
        # 好感度信息
        if include_relationship and self.relationship_manager and npc_id and user_id:
            try:
                relationship_prompt = self.relationship_manager.get_prompt_modifier(
                    npc_id, user_id
                )
                system_content.append(relationship_prompt)
            except Exception as e:
                print(f"[ContextBuilder] 获取好感度信息失败: {e}")
        
        if system_content:
            packets.append(ContextPacket(
                content="\n\n".join(system_content),
                source="system",
                relevance_score=1.0,
                priority=100,  # 最高优先级
                metadata={"type": "system_instruction"}
            ))
        
        # ========== 2. 记忆系统 ==========
        if self.memory_tool and self.config.enable_memory_search:
            try:
                # 工作记忆
                working_memories = self.memory_tool.execute(
                    "search",
                    query=user_query,
                    limit=5,
                    memory_type="working",
                    user_id=user_id
                )
                for mem in working_memories:
                    packets.append(ContextPacket(
                        content=f"[最近对话] {mem.content}",
                        source="memory",
                        timestamp=datetime.fromisoformat(mem.timestamp) if mem.timestamp else datetime.now(),
                        relevance_score=mem.combined_score,
                        priority=80,
                        metadata={"type": "working_memory", "memory_id": mem.memory_id}
                    ))
                
                # 情景记忆
                episodic_memories = self.memory_tool.execute(
                    "search",
                    query=user_query,
                    limit=5,
                    memory_type="episodic",
                    user_id=user_id,
                    session_id=session_id
                )
                for mem in episodic_memories:
                    packets.append(ContextPacket(
                        content=f"[历史记忆] {mem.content}",
                        source="memory",
                        timestamp=datetime.fromisoformat(mem.timestamp) if mem.timestamp else datetime.now(),
                        relevance_score=mem.combined_score,
                        priority=60,
                        metadata={"type": "episodic_memory", "memory_id": mem.memory_id}
                    ))
                
                # 语义记忆
                semantic_memories = self.memory_tool.execute(
                    "search",
                    query=user_query,
                    limit=3,
                    memory_type="semantic",
                    user_id=user_id
                )
                for mem in semantic_memories:
                    packets.append(ContextPacket(
                        content=f"[知识] {mem.content}",
                        source="memory",
                        timestamp=datetime.fromisoformat(mem.timestamp) if mem.timestamp else datetime.now(),
                        relevance_score=mem.combined_score,
                        priority=70,
                        metadata={"type": "semantic_memory", "memory_id": mem.memory_id}
                    ))
                    
            except Exception as e:
                print(f"[ContextBuilder] 记忆检索失败: {e}")
        
        # ========== 3. RAG知识库 ==========
        if self.rag_tool and self.config.enable_rag_search:
            try:
                rag_results = self.rag_tool.search(
                    query=user_query,
                    limit=5,
                    min_score=self.config.min_relevance
                )
                for result in rag_results:
                    packets.append(ContextPacket(
                        content=f"[参考资料] {result.content}",
                        source="rag",
                        relevance_score=result.score,
                        priority=50,
                        metadata={
                            "type": "rag_result",
                            "doc_id": result.doc_id,
                            **result.metadata
                        }
                    ))
            except Exception as e:
                print(f"[ContextBuilder] RAG检索失败: {e}")
        
        # ========== 4. 对话历史 ==========
        if conversation_history:
            recent_history = conversation_history[-5:]
            for i, msg in enumerate(recent_history):
                role_label = "玩家" if msg.role == "user" else "NPC"
                packets.append(ContextPacket(
                    content=f"{role_label}: {msg.content}",
                    source="history",
                    timestamp=msg.timestamp,
                    relevance_score=0.6 + 0.08 * i,  # 越近的越相关
                    priority=40,
                    metadata={"type": "conversation_history", "role": msg.role}
                ))
        
        # ========== 5. 结构化笔记 ==========
        if self.note_tool:
            try:
                # 优先检索blocker类型
                blockers = self.note_tool.execute(
                    "list",
                    note_type="blocker",
                    user_id=user_id,
                    limit=2
                )
                for note in blockers:
                    note_content = self.note_tool.execute("read", note_id=note["note_id"])
                    packets.append(ContextPacket(
                        content=f"[重要提醒] {note['title']}: {note_content.get('content', '')[:200]}",
                        source="notes",
                        relevance_score=0.9,
                        priority=75,
                        metadata={"type": "note_blocker", "note_id": note["note_id"]}
                    ))
                
                # 搜索相关笔记
                related_notes = self.note_tool.execute(
                    "search",
                    query=user_query,
                    user_id=user_id,
                    limit=3
                )
                for note in related_notes:
                    if note["note_id"] not in [p.metadata.get("note_id") for p in packets]:
                        packets.append(ContextPacket(
                            content=f"[笔记:{note['type']}] {note['title']}: {note.get('content', '')[:150]}",
                            source="notes",
                            relevance_score=0.7,
                            priority=45,
                            metadata={"type": f"note_{note['type']}", "note_id": note["note_id"]}
                        ))
            except Exception as e:
                print(f"[ContextBuilder] 笔记检索失败: {e}")
        
        # ========== 6. 自定义信息包 ==========
        if custom_packets:
            for packet in custom_packets:
                packet.source = "custom"
                if packet.priority == 0:
                    packet.priority = 30
                packets.append(packet)
        
        return packets
    
    def _select(self,
                packets: List[ContextPacket],
                user_query: str,
                available_tokens: int) -> List[ContextPacket]:
        """Select阶段 - 智能选择信息"""
        if not packets:
            return []
        
        # 1. 分离系统指令 (必须保留)
        system_packets = [p for p in packets if p.source == "system"]
        other_packets = [p for p in packets if p.source != "system"]
        
        # 2. 计算系统指令占用
        system_tokens = sum(p.token_count for p in system_packets)
        remaining_tokens = available_tokens - system_tokens
        
        if remaining_tokens <= 0:
            return system_packets
        
        # 3. 为其他信息计算综合分数
        scored_packets = []
        for packet in other_packets:
            # 计算相关性 (如果未设置)
            if packet.relevance_score == 0.5 and packet.source not in ["history"]:
                relevance = self._calculate_relevance(packet.content, user_query)
                packet.relevance_score = relevance
            
            # 计算新近性
            recency = self._calculate_recency(packet.timestamp)
            
            # 综合分数 = 优先级加成 + 相关性 + 新近性
            priority_bonus = packet.priority / 100.0 * 0.3
            combined_score = (
                priority_bonus +
                self.config.relevance_weight * packet.relevance_score +
                self.config.recency_weight * recency
            )
            
            # 过滤低相关性
            if packet.relevance_score >= self.config.min_relevance:
                scored_packets.append((combined_score, packet))
        
        # 4. 排序
        scored_packets.sort(key=lambda x: x[0], reverse=True)
        
        # 5. 按来源分配预算的贪心选择
        selected = list(system_packets)
        current_tokens = system_tokens
        
        # 计算各来源预算
        source_budgets = {
            "memory": int(remaining_tokens * self.config.memory_budget),
            "rag": int(remaining_tokens * self.config.rag_budget),
            "history": int(remaining_tokens * self.config.history_budget),
            "notes": int(remaining_tokens * self.config.notes_budget),
            "custom": int(remaining_tokens * self.config.custom_budget),
        }
        source_used = {k: 0 for k in source_budgets}
        
        for score, packet in scored_packets:
            source = packet.source
            budget = source_budgets.get(source, remaining_tokens)
            used = source_used.get(source, 0)
            
            # 检查来源预算和总预算
            if used + packet.token_count <= budget and current_tokens + packet.token_count <= available_tokens:
                selected.append(packet)
                current_tokens += packet.token_count
                source_used[source] = used + packet.token_count
        
        return selected
    
    def _structure(self, 
                   selected_packets: List[ContextPacket],
                   user_query: str) -> str:
        """Structure阶段 - 结构化输出"""
        # 按来源分组
        system_instructions = []
        evidence = []
        memories = []
        history = []
        notes = []
        custom = []
        
        for packet in selected_packets:
            if packet.source == "system":
                system_instructions.append(packet.content)
            elif packet.source == "rag":
                evidence.append(packet.content)
            elif packet.source == "memory":
                memories.append(packet.content)
            elif packet.source == "history":
                history.append(packet.content)
            elif packet.source == "notes":
                notes.append(packet.content)
            else:
                custom.append(packet.content)
        
        # 构建结构化模板
        sections = []
        
        # [Role & Policies]
        if system_instructions:
            sections.append("[角色设定]\n" + "\n\n".join(system_instructions))
        
        # [Task]
        sections.append(f"[当前任务]\n玩家说: {user_query}")
        
        # [Evidence] - RAG知识
        if evidence:
            sections.append("[参考知识]\n" + "\n---\n".join(evidence))
        
        # [Memory] - 记忆
        if memories:
            sections.append("[相关记忆]\n" + "\n".join(memories))
        
        # [History] - 对话历史
        if history:
            sections.append("[对话历史]\n" + "\n".join(history))
        
        # [Notes] - 笔记
        if notes:
            sections.append("[笔记提醒]\n" + "\n".join(notes))
        
        # [Custom] - 自定义
        if custom:
            sections.append("[其他信息]\n" + "\n".join(custom))
        
        # [Output]
        sections.append("[输出要求]\n请以角色身份回复玩家，保持人设一致性，根据好感度调整语气。")
        
        return "\n\n".join(sections)
    
    def _compress(self, context: str, max_tokens: int) -> str:
        """Compress阶段 - 压缩超限内容"""
        current_tokens = ContextPacket._estimate_tokens(context)
        
        if current_tokens <= max_tokens:
            return context
        
        print(f"[ContextBuilder] 上下文超限 ({current_tokens} > {max_tokens})，执行压缩")
        
        # 分区压缩
        sections = context.split("\n\n")
        compressed_sections = []
        current_total = 0
        
        # 优先保留的section关键词
        priority_keywords = ["角色设定", "当前任务", "输出要求"]
        
        # 先添加优先section
        for section in sections:
            if any(kw in section for kw in priority_keywords):
                section_tokens = ContextPacket._estimate_tokens(section)
                compressed_sections.append(section)
                current_total += section_tokens
        
        # 再添加其他section
        for section in sections:
            if any(kw in section for kw in priority_keywords):
                continue
            
            section_tokens = ContextPacket._estimate_tokens(section)
            
            if current_total + section_tokens <= max_tokens:
                compressed_sections.append(section)
                current_total += section_tokens
            else:
                remaining_tokens = max_tokens - current_total
                if remaining_tokens > 50:
                    truncated = self._truncate_text(section, remaining_tokens)
                    compressed_sections.append(truncated + "\n[... 内容已压缩 ...]")
                break
        
        return "\n\n".join(compressed_sections)
    
    def _calculate_relevance(self, content: str, query: str) -> float:
        """计算相关性 (Jaccard相似度)"""
        if not content or not query:
            return 0.0
        
        content_words = set(content.lower().split())
        query_words = set(query.lower().split())
        
        if not query_words:
            return 0.0
        
        intersection = content_words & query_words
        union = content_words | query_words
        
        return len(intersection) / len(union) if union else 0.0
    
    def _calculate_recency(self, timestamp: datetime) -> float:
        """计算时间近因性"""
        if not timestamp:
            return 0.5
        
        age_hours = (datetime.now() - timestamp).total_seconds() / 3600
        
        # 指数衰减: 24小时内保持高分
        decay_factor = 0.1
        recency_score = math.exp(-decay_factor * age_hours / 24)
        
        return max(0.1, min(1.0, recency_score))
    
    def _truncate_text(self, text: str, max_tokens: int) -> str:
        """截断文本"""
        current_tokens = ContextPacket._estimate_tokens(text)
        if current_tokens <= max_tokens:
            return text
        
        ratio = max_tokens / current_tokens
        max_chars = int(len(text) * ratio)
        
        return text[:max_chars]


# ==================== 便捷函数 ====================

def create_context_builder(memory_tool=None,
                           rag_tool=None,
                           note_tool=None,
                           relationship_manager=None,
                           max_tokens: int = 3000,
                           role_description: str = "") -> ContextBuilder:
    """创建上下文构建器"""
    config = ContextConfig(
        max_tokens=max_tokens,
        role_description=role_description
    )
    return ContextBuilder(
        config=config,
        memory_tool=memory_tool,
        rag_tool=rag_tool,
        note_tool=note_tool,
        relationship_manager=relationship_manager
    )
