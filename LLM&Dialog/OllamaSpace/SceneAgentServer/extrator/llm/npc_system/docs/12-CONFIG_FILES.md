# 12 - 配置文件详解

> **面向对象**: 系统开发者、运维人员  
> **难度**: ⭐⭐⭐☆☆ (中高)  
> **前置知识**: Python、JSON/YAML、系统配置

## 📋 概述

本文档详细说明NPC系统的所有配置文件，包括：
- 配置文件结构
- 参数详解
- 配置示例
- 最佳实践

---

## 📁 配置文件结构

### 目录结构

```
npc_system/
├── config/
│   ├── system_config.yaml       # 系统全局配置
│   ├── llm_config.yaml          # LLM配置
│   ├── memory_config.yaml       # 记忆系统配置
│   ├── rag_config.yaml          # RAG系统配置
│   └── relationship_config.yaml # 关系系统配置
│
├── npc_configs/                 # NPC个体配置
│   ├── blacksmith.yaml
│   ├── merchant.yaml
│   └── guard.yaml
│
└── npc_data/                    # 运行时数据
    ├── .player_id               # 玩家ID持久化
    ├── databases/               # 数据库文件
    ├── memories/                # 记忆文件
    ├── knowledge_base/          # 知识库
    └── rag_index/               # RAG索引
```

---

## ⚙️ 系统全局配置

### system_config.yaml

```yaml
# 系统全局配置
system:
  # 系统名称
  name: "NPC System"
  version: "1.0.0"
  
  # 数据目录
  data_dir: "./npc_data"
  
  # 日志配置
  logging:
    level: "INFO"  # DEBUG, INFO, WARNING, ERROR
    file: "./logs/npc_system.log"
    max_size: "10MB"
    backup_count: 5
    format: "[%(asctime)s] [%(levelname)s] %(message)s"
  
  # 性能配置
  performance:
    # 并发配置
    max_concurrent_chats: 10
    chat_timeout: 30  # 秒
    
    # 缓存配置
    enable_cache: true
    cache_ttl: 300  # 秒
    cache_size: 1000  # 条目数
    
    # 批处理配置
    batch_size: 10
    batch_timeout: 5  # 秒
  
  # 数据库配置
  database:
    # SQLite配置
    sqlite:
      journal_mode: "WAL"  # WAL模式提升并发性能
      synchronous: "NORMAL"
      cache_size: 10000
      temp_store: "MEMORY"
    
    # 备份配置
    backup:
      enabled: true
      interval: 86400  # 24小时
      keep_days: 7
      backup_dir: "./backups"
  
  # 安全配置
  security:
    # 输入验证
    max_message_length: 1000
    max_session_messages: 100
    
    # 速率限制
    rate_limit:
      enabled: true
      max_requests_per_minute: 60
      max_requests_per_hour: 1000
```

### 参数说明

#### 日志配置
```yaml
logging:
  level: "INFO"
  # DEBUG: 详细调试信息 (开发环境)
  # INFO: 一般信息 (生产环境推荐)
  # WARNING: 警告信息
  # ERROR: 错误信息
```

#### 性能配置
```yaml
performance:
  max_concurrent_chats: 10
  # 同时处理的对话数量
  # 根据服务器性能调整
  # 推荐: 4核CPU = 10, 8核CPU = 20
  
  cache_ttl: 300
  # 缓存生存时间 (秒)
  # 较短: 更新及时但性能较低
  # 较长: 性能好但可能不够实时
```

---

## 🤖 LLM配置

### llm_config.yaml

```yaml
# LLM配置
llm:
  # 默认模型
  default_model: "qwen2.5:7b"
  
  # 模型列表
  models:
    # Qwen系列
    qwen_7b:
      name: "qwen2.5:7b"
      temperature: 0.7
      max_tokens: 2048
      top_p: 0.9
      top_k: 40
      repeat_penalty: 1.1
      context_length: 8192
      
    qwen_3b:
      name: "qwen2.5:3b"
      temperature: 0.7
      max_tokens: 2048
      context_length: 4096
    
    # Llama系列
    llama_7b:
      name: "llama2:7b"
      temperature: 0.8
      max_tokens: 2048
      context_length: 4096
  
  # Ollama配置
  ollama:
    base_url: "http://localhost:11434"
    timeout: 60  # 秒
    num_ctx: 8192  # 上下文窗口
    num_predict: 2048  # 最大生成长度
    
  # 提示词模板
  prompt_templates:
    # 系统提示词
    system_prompt: |
      你是{npc_name}，{npc_role}。
      
      {personality_description}
      
      请严格按照以下要求回复:
      1. 保持角色一致性
      2. 使用符合人设的语言风格
      3. 回复简洁自然 (50-150字)
      4. 不要重复用户的话
      5. 不要说"作为AI"之类的话
    
    # 用户提示词
    user_prompt: |
      {context}
      
      用户: {message}
      
      请以{npc_name}的身份回复:
  
  # 生成配置
  generation:
    # 停止词
    stop_sequences:
      - "\n用户:"
      - "\n玩家:"
      - "\nUser:"
      - "\nPlayer:"
    
    # 重试配置
    max_retries: 3
    retry_delay: 1  # 秒
    
    # 流式输出
    streaming:
      enabled: false
      chunk_size: 10
```

### 参数详解

#### Temperature (温度)
```yaml
temperature: 0.7
# 控制输出的随机性
# 0.0: 完全确定性 (总是选择最可能的词)
# 0.5: 较保守 (适合事实性回答)
# 0.7: 平衡 (推荐，自然对话)
# 1.0: 较随机 (创意性回答)
# 1.5+: 非常随机 (可能不连贯)
```

#### Top-p (核采样)
```yaml
top_p: 0.9
# 累积概率阈值
# 0.9: 考虑累积概率90%的词 (推荐)
# 0.95: 更多样化
# 0.8: 更保守
```

#### Repeat Penalty (重复惩罚)
```yaml
repeat_penalty: 1.1
# 惩罚重复词汇
# 1.0: 无惩罚
# 1.1: 轻微惩罚 (推荐)
# 1.3: 中等惩罚
# 1.5+: 强烈惩罚 (可能导致不自然)
```

---

## 🧠 记忆系统配置

### memory_config.yaml

```yaml
# 记忆系统配置
memory:
  # 工作记忆 (短期)
  working_memory:
    size: 10  # 保留最近N条对话
    ttl: 3600  # 1小时后清除
  
  # 情景记忆 (中期)
  episodic_memory:
    max_items: 100
    importance_threshold: 0.5
    decay_rate: 0.01  # 每天衰减1%
    
    # 检索配置
    retrieval:
      method: "similarity"  # similarity, recency, importance
      top_k: 5
      similarity_threshold: 0.7
      time_weight: 0.3  # 时间权重
      importance_weight: 0.7  # 重要性权重
  
  # 语义记忆 (长期)
  semantic_memory:
    max_items: 50
    importance_threshold: 0.7
    decay_rate: 0.001  # 每天衰减0.1%
    
    # 检索配置
    retrieval:
      method: "similarity"
      top_k: 3
      similarity_threshold: 0.8
  
  # 感知记忆 (即时)
  perceptual_memory:
    size: 5
    ttl: 300  # 5分钟
  
  # 向量化配置
  embedding:
    model: "sentence-transformers/all-MiniLM-L6-v2"
    dimension: 384
    batch_size: 32
    device: "cpu"  # cpu, cuda
  
  # 遗忘机制
  forgetting:
    enabled: true
    # 遗忘策略
    strategy: "importance_decay"  # importance_decay, lru, fifo
    # 清理间隔
    cleanup_interval: 3600  # 1小时
    # 保留最小数量
    min_keep: 10
```

### 记忆检索策略

#### 相似度检索
```yaml
retrieval:
  method: "similarity"
  # 基于向量相似度检索
  # 优点: 语义相关性强
  # 缺点: 计算开销大
```

#### 时间检索
```yaml
retrieval:
  method: "recency"
  # 基于时间新近度检索
  # 优点: 快速，适合短期记忆
  # 缺点: 可能忽略重要但较旧的记忆
```

#### 重要性检索
```yaml
retrieval:
  method: "importance"
  # 基于重要性分数检索
  # 优点: 保留关键信息
  # 缺点: 需要准确的重要性评估
```

#### 混合检索
```yaml
retrieval:
  method: "hybrid"
  time_weight: 0.3
  importance_weight: 0.4
  similarity_weight: 0.3
  # 综合考虑多个因素
  # 推荐用于生产环境
```

---

## 📚 RAG系统配置

### rag_config.yaml

```yaml
# RAG系统配置
rag:
  # 文档处理
  document_processing:
    # 分块配置
    chunking:
      strategy: "recursive"  # fixed, recursive, semantic
      chunk_size: 500
      chunk_overlap: 50
      separators: ["\n\n", "\n", "。", "！", "？", ". ", "! ", "? "]
    
    # 文本清理
    cleaning:
      remove_extra_whitespace: true
      remove_special_chars: false
      lowercase: false
  
  # 向量存储
  vector_store:
    type: "faiss"  # faiss, chroma, pinecone
    
    # FAISS配置
    faiss:
      index_type: "IndexFlatL2"  # IndexFlatL2, IndexIVFFlat
      metric: "l2"  # l2, cosine
      nlist: 100  # IVF聚类数
    
    # 持久化
    persistence:
      enabled: true
      save_interval: 300  # 5分钟
  
  # 检索配置
  retrieval:
    # 检索策略
    strategy: "hybrid"  # dense, sparse, hybrid
    
    # 密集检索 (向量)
    dense:
      top_k: 5
      similarity_threshold: 0.7
      rerank: true
    
    # 稀疏检索 (BM25)
    sparse:
      top_k: 10
      bm25_k1: 1.5
      bm25_b: 0.75
    
    # 混合检索
    hybrid:
      dense_weight: 0.7
      sparse_weight: 0.3
      top_k: 3
  
  # 重排序
  reranking:
    enabled: true
    model: "cross-encoder/ms-marco-MiniLM-L-6-v2"
    top_k: 3
  
  # 嵌入模型
  embedding:
    model: "sentence-transformers/all-MiniLM-L6-v2"
    dimension: 384
    batch_size: 32
    normalize: true
```

### 分块策略

#### 固定长度分块
```yaml
chunking:
  strategy: "fixed"
  chunk_size: 500
  chunk_overlap: 50
  # 优点: 简单快速
  # 缺点: 可能切断语义
```

#### 递归分块
```yaml
chunking:
  strategy: "recursive"
  chunk_size: 500
  chunk_overlap: 50
  separators: ["\n\n", "\n", "。"]
  # 优点: 保持语义完整性
  # 缺点: 分块大小不均匀
  # 推荐: 大多数场景
```

#### 语义分块
```yaml
chunking:
  strategy: "semantic"
  similarity_threshold: 0.8
  # 优点: 最佳语义完整性
  # 缺点: 计算开销大
  # 推荐: 高质量要求场景
```

---

## 💕 关系系统配置

### relationship_config.yaml

```yaml
# 关系系统配置
relationship:
  # 好感度等级
  affinity_levels:
    - name: "陌生"
      min_score: 0
      max_score: 20
      color: "#808080"
      
    - name: "认识"
      min_score: 21
      max_score: 40
      color: "#90EE90"
      
    - name: "友好"
      min_score: 41
      max_score: 60
      color: "#87CEEB"
      
    - name: "信任"
      min_score: 61
      max_score: 80
      color: "#FFD700"
      
    - name: "挚友"
      min_score: 81
      max_score: 100
      color: "#FF69B4"
  
  # 好感度变化
  affinity_changes:
    # 基础变化
    positive_chat: 1  # 正常对话
    negative_chat: -2  # 负面对话
    
    # 特殊事件
    gift: 5  # 送礼
    help: 10  # 帮助
    betray: -20  # 背叛
    
    # 对话质量
    long_message: 2  # 长消息 (>30字)
    question: 1  # 提问
    thanks: 3  # 感谢
    insult: -5  # 侮辱
  
  # 时间衰减
  decay:
    enabled: true
    rate: 0.1  # 每天衰减0.1分
    min_score: 0  # 最低不低于0
    interval: 86400  # 24小时
  
  # 秘密解锁
  secrets:
    # 解锁条件
    unlock_conditions:
      level_based: true  # 基于等级
      event_based: true  # 基于事件
      time_based: false  # 基于时间
    
    # 解锁提示
    notifications:
      enabled: true
      message: "你与{npc_name}的关系加深了，TA愿意分享更多秘密。"
```

---

## 🎮 NPC个体配置

### blacksmith.yaml

```yaml
# NPC: 老铁匠
npc:
  # 基础信息
  id: "blacksmith_001"
  name: "老铁匠"
  role: "铁匠"
  age: 55
  gender: "男"
  
  # 性格特征
  personality:
    traits:
      - "严肃认真"
      - "技艺精湛"
      - "热心助人"
      - "固执己见"
    
    background: |
      我叫老铁匠，今年55岁了。从25岁开始在这个村子里打铁，
      一晃就是30年。我的手艺是从父亲那里学来的...
    
    speech_style: |
      - 说话简洁直接
      - 偶尔带点方言
      - 谈到打铁时滔滔不绝
    
    knowledge:
      - "锻造技术"
      - "武器制作"
      - "盔甲修理"
    
    secrets:
      level_0: ""
      level_1: "其实我年轻时也想过当冒险者"
      level_2: "我有一把自己打造的精钢剑"
      level_3: "30年前我见过一把魔法剑"
      level_4: "我知道秘银矿脉的位置"
    
    greeting: "需要打造什么吗？"
  
  # 配置覆盖
  config_overrides:
    # LLM配置
    llm:
      model: "qwen2.5:7b"
      temperature: 0.7
    
    # 记忆配置
    memory:
      working_memory_size: 10
      episodic_memory_limit: 100
    
    # RAG配置
    rag:
      enabled: true
      top_k: 3
    
    # 上下文配置
    context:
      max_tokens: 3000
      memory_weight: 0.3
      knowledge_weight: 0.3
  
  # 知识库文件
  knowledge_files:
    - "knowledge_base/blacksmith_001/recipes.txt"
    - "knowledge_base/blacksmith_001/materials.txt"
    - "knowledge_base/blacksmith_001/techniques.txt"
  
  # 初始记忆
  initial_memories:
    semantic:
      - content: "我从父亲那里学会了锻造技术"
        importance: 0.8
      - content: "精钢剑是我最拿手的作品"
        importance: 0.7
    
    episodic:
      - content: "上周帮村长的儿子打造了一把铁剑"
        importance: 0.6
```

---

## 🔧 配置加载

### Python代码

```python
import yaml
from typing import Dict, Any

class ConfigLoader:
    """配置加载器"""
    
    def __init__(self, config_dir: str = "./config"):
        self.config_dir = config_dir
        self.configs = {}
    
    def load_all(self):
        """加载所有配置"""
        self.configs['system'] = self.load_yaml('system_config.yaml')
        self.configs['llm'] = self.load_yaml('llm_config.yaml')
        self.configs['memory'] = self.load_yaml('memory_config.yaml')
        self.configs['rag'] = self.load_yaml('rag_config.yaml')
        self.configs['relationship'] = self.load_yaml('relationship_config.yaml')
    
    def load_yaml(self, filename: str) -> Dict[str, Any]:
        """加载YAML文件"""
        filepath = f"{self.config_dir}/{filename}"
        with open(filepath, 'r', encoding='utf-8') as f:
            return yaml.safe_load(f)
    
    def get(self, key: str, default: Any = None) -> Any:
        """获取配置值"""
        keys = key.split('.')
        value = self.configs
        
        for k in keys:
            if isinstance(value, dict) and k in value:
                value = value[k]
            else:
                return default
        
        return value
    
    def load_npc_config(self, npc_id: str) -> Dict[str, Any]:
        """加载NPC配置"""
        filepath = f"./npc_configs/{npc_id}.yaml"
        with open(filepath, 'r', encoding='utf-8') as f:
            return yaml.safe_load(f)


# 使用示例
config_loader = ConfigLoader()
config_loader.load_all()

# 获取配置
llm_model = config_loader.get('llm.default_model')
memory_size = config_loader.get('memory.working_memory.size')

# 加载NPC配置
blacksmith_config = config_loader.load_npc_config('blacksmith_001')
```

---

## 📊 配置最佳实践

### 1. 开发环境配置

```yaml
# config/dev.yaml
system:
  logging:
    level: "DEBUG"  # 详细日志
  
  performance:
    enable_cache: false  # 禁用缓存便于调试
    max_concurrent_chats: 1  # 单线程便于调试

llm:
  default_model: "qwen2.5:3b"  # 使用小模型加快速度
  
memory:
  working_memory:
    size: 5  # 减少记忆数量
```

### 2. 生产环境配置

```yaml
# config/prod.yaml
system:
  logging:
    level: "INFO"  # 适度日志
  
  performance:
    enable_cache: true  # 启用缓存
    max_concurrent_chats: 20  # 支持并发

llm:
  default_model: "qwen2.5:7b"  # 使用大模型提升质量
  
memory:
  working_memory:
    size: 10  # 完整记忆
```

### 3. 配置验证

```python
def validate_config(config: Dict[str, Any]) -> bool:
    """验证配置"""
    
    # 检查必需字段
    required_fields = [
        'system.data_dir',
        'llm.default_model',
        'memory.working_memory.size'
    ]
    
    for field in required_fields:
        if not get_nested_value(config, field):
            print(f"缺少必需配置: {field}")
            return False
    
    # 检查数值范围
    if config['memory']['working_memory']['size'] < 1:
        print("working_memory.size 必须 >= 1")
        return False
    
    if config['llm']['temperature'] < 0 or config['llm']['temperature'] > 2:
        print("temperature 必须在 0-2 之间")
        return False
    
    return True
```

---

## 📝 总结

配置文件是NPC系统的核心：

1. **system_config.yaml** - 系统全局设置
2. **llm_config.yaml** - LLM模型配置
3. **memory_config.yaml** - 记忆系统配置
4. **rag_config.yaml** - RAG系统配置
5. **relationship_config.yaml** - 关系系统配置
6. **{npc_id}.yaml** - NPC个体配置

**关键要点**:
- 开发/生产环境分离
- 参数验证
- 合理的默认值
- 详细的注释

**下一步**: 系统已完整配置，可以开始创建和测试NPC！
