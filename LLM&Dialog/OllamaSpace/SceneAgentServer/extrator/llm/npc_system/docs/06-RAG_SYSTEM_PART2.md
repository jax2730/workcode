# 06 - RAG系统详解 (第2部分)

## 💡 实战示例

### 示例1: 为铁匠NPC创建知识库

#### 步骤1: 准备知识文档

创建 `knowledge_base/blacksmith/` 目录，添加以下文档：

**文件: `铁剑锻造教程.md`**
```markdown
---
title: 铁剑锻造教程
author: 老铁匠
difficulty: 基础
category: 武器制作
tags: [铁剑, 锻造, 基础]
---

# 铁剑锻造教程

## 所需材料
- 铁锭 × 3
- 木棍 × 1
- 木炭 × 5

## 所需工具
- 熔炉
- 铁砧
- 锻造锤
- 水桶（用于淬火）

## 制作步骤

### 1. 准备工作
将熔炉预热至红热状态，准备好所有材料和工具。

### 2. 熔炼铁锭
将3块铁锭放入熔炉，加热至完全熔化，呈现橙红色。

### 3. 锻打成型
将熔化的铁倒在铁砧上，用锻造锤反复锻打，塑造剑身形状。
注意保持温度，必要时重新加热。

### 4. 安装剑柄
在剑身底部开槽，插入木棍作为剑柄，用铁钉固定。

### 5. 淬火
将锻造好的剑身快速浸入水中淬火，使其变硬。

### 6. 打磨
用磨刀石打磨剑刃，使其锋利。

## 注意事项
- 温度控制很重要，过热会导致铁质变脆
- 淬火时要快速均匀，避免变形
- 新手建议在老师傅指导下进行

## 成品属性
- 攻击力: 30
- 耐久度: 100
- 重量: 2.5kg
- 价值: 50金币
```

**文件: `材料知识.md`**
```markdown
---
title: 锻造材料知识
category: 基础知识
---

# 锻造材料知识

## 铁锭
- 来源: 铁矿石冶炼
- 熔点: 1538°C
- 特性: 坚硬、易锻造
- 用途: 制作武器、工具、护甲

## 精钢锭
- 来源: 铁锭精炼
- 熔点: 1600°C
- 特性: 比铁更硬，更锋利
- 用途: 高级武器

## 木材
- 橡木: 坚硬，适合做剑柄
- 松木: 轻便，适合做弓
- 桦木: 柔韧，适合做盾牌框架
```

**文件: `价格表.json`**
```json
{
  "weapons": {
    "iron_sword": {
      "name": "铁剑",
      "buy_price": 50,
      "sell_price": 35,
      "materials": ["铁锭×3", "木棍×1"]
    },
    "steel_sword": {
      "name": "精钢剑",
      "buy_price": 120,
      "sell_price": 85,
      "materials": ["精钢锭×3", "优质木材×1"]
    }
  },
  "materials": {
    "iron_ingot": {
      "name": "铁锭",
      "price": 10
    },
    "steel_ingot": {
      "name": "精钢锭",
      "price": 25
    }
  }
}
```

#### 步骤2: 构建索引

```python
from npc_system import RAGTool, RAGConfig

# 创建RAG工具
rag = RAGTool(RAGConfig(
    knowledge_base_dir="./knowledge_base/blacksmith",
    index_dir="./rag_index/blacksmith",
    enable_embedding=True,
    chunk_size=500,
    chunk_overlap=100
))

# 加载并索引所有文档
rag.index_directory("./knowledge_base/blacksmith")

print("✅ 知识库索引构建完成")
```

#### 步骤3: 测试检索

```python
# 测试查询1: 制作方法
results = rag.search("如何制作铁剑", top_k=3)
print("\\n=== 查询: 如何制作铁剑 ===")
for i, result in enumerate(results, 1):
    print(f"\\n结果{i} (相似度: {result.score:.3f})")
    print(result.content[:200] + "...")

# 测试查询2: 价格信息
results = rag.search("铁剑多少钱", top_k=2)
print("\\n=== 查询: 铁剑多少钱 ===")
for i, result in enumerate(results, 1):
    print(f"\\n结果{i} (相似度: {result.score:.3f})")
    print(result.content[:200] + "...")

# 测试查询3: 材料知识
results = rag.search("精钢和铁有什么区别", top_k=2)
print("\\n=== 查询: 精钢和铁有什么区别 ===")
for i, result in enumerate(results, 1):
    print(f"\\n结果{i} (相似度: {result.score:.3f})")
    print(result.content[:200] + "...")
```

#### 步骤4: 集成到NPC

```python
from npc_system import create_npc
from langchain_ollama import ChatOllama

# 创建LLM
llm = ChatOllama(model="qwen2.5")

# 创建NPC (自动加载RAG)
npc = create_npc(
    npc_id="blacksmith",
    name="老铁匠",
    role="铁匠",
    traits=["专业", "严肃", "博学"],
    knowledge=["武器锻造", "材料知识", "价格"],
    llm=llm,
    data_dir="./npc_data"
)

# 对话测试
result = npc.chat("player_001", "你能教我怎么打造铁剑吗？")
print(result["reply"])
# NPC会从知识库中检索相关信息，然后回答
```

---

### 示例2: 动态更新知识库

```python
# 添加新知识
new_knowledge = """
# 剑的保养方法

## 日常保养
1. 使用后及时清洁，去除血迹和污垢
2. 涂抹防锈油，防止生锈
3. 存放在干燥通风处

## 定期维护
1. 每月检查剑刃是否有缺口
2. 每季度重新打磨剑刃
3. 每年更换剑柄缠绳

## 注意事项
- 不要用剑砍硬物（如石头、金属）
- 不要在潮湿环境中存放
- 发现损坏及时修复
"""

# 添加到RAG
rag.add_text(
    content=new_knowledge,
    doc_id="sword_maintenance",
    metadata={
        "title": "剑的保养方法",
        "category": "维护保养",
        "author": "老铁匠"
    }
)

print("✅ 新知识已添加")

# 测试新知识
results = rag.search("剑怎么保养", top_k=2)
print("\\n=== 查询: 剑怎么保养 ===")
for result in results:
    print(f"\\n{result.content[:200]}...")
```

---

## ⚙️ 配置详解

### RAGConfig 完整配置

```python
from npc_system import RAGConfig

config = RAGConfig(
    # ===== 基础配置 =====
    knowledge_base_dir="./knowledge_base",  # 知识库目录
    index_dir="./rag_index",                # 索引目录
    
    # ===== 文档处理 =====
    chunk_size=500,                         # 分块大小(字符)
    chunk_overlap=100,                      # 块重叠大小
    chunk_strategy="smart",                 # 分块策略: fixed/semantic/smart
    
    # ===== 向量化 =====
    enable_embedding=True,                  # 是否启用向量嵌入
    embedding_model="nomic-embed-text",     # 嵌入模型
    embedding_dimension=768,                # 向量维度
    
    # ===== 索引 =====
    index_type="faiss",                     # 索引类型: faiss/simple
    faiss_index_type="IndexFlatL2",         # FAISS索引类型
    
    # ===== 检索 =====
    default_top_k=5,                        # 默认返回数量
    min_similarity=0.1,                     # 最低相似度阈值
    retrieval_method="hybrid",              # 检索方法: vector/keyword/hybrid
    vector_weight=0.7,                      # 向量检索权重
    keyword_weight=0.3,                     # 关键词检索权重
    
    # ===== 重排序 =====
    enable_rerank=True,                     # 是否启用重排序
    rerank_weights={                        # 重排序权重
        "similarity": 0.6,
        "freshness": 0.2,
        "authority": 0.1,
        "completeness": 0.1
    },
    
    # ===== 缓存 =====
    enable_cache=True,                      # 是否启用缓存
    cache_size=100,                         # 缓存大小
    cache_ttl=300,                          # 缓存过期时间(秒)
    
    # ===== 性能 =====
    batch_size=32,                          # 批处理大小
    num_workers=4,                          # 并行工作线程数
    
    # ===== 调试 =====
    verbose=True,                           # 是否输出详细日志
    log_file="./logs/rag.log"              # 日志文件
)
```

---

## 🎯 高级技巧

### 1. 多语言支持

```python
# 中英文混合知识库
config = RAGConfig(
    knowledge_base_dir="./knowledge_base",
    embedding_model="multilingual-e5-base",  # 多语言模型
    text_splitter="multilingual"             # 多语言分词
)

# 添加英文文档
rag.add_text("""
# Iron Sword Crafting Guide

## Materials Required
- Iron Ingots × 3
- Wooden Stick × 1
""", doc_id="ironsword_en")

# 添加中文文档
rag.add_text("""
# 铁剑锻造指南

## 所需材料
- 铁锭 × 3
- 木棍 × 1
""", doc_id="ironsword_zh")

# 中文查询
results = rag.search("如何制作铁剑")

# 英文查询
results = rag.search("how to craft iron sword")
```

### 2. 元数据过滤

```python
# 按难度过滤
results = rag.search(
    query="锻造教程",
    filters={"difficulty": "基础"},
    top_k=5
)

# 按分类过滤
results = rag.search(
    query="制作方法",
    filters={"category": "武器制作"},
    top_k=5
)

# 多条件过滤
results = rag.search(
    query="教程",
    filters={
        "difficulty": "基础",
        "category": "武器制作",
        "author": "老铁匠"
    },
    top_k=5
)

# 日期范围过滤
from datetime import datetime, timedelta

results = rag.search(
    query="最新教程",
    filters={
        "created_after": datetime.now() - timedelta(days=30)
    },
    top_k=5
)
```

### 3. 自定义评分函数

```python
def custom_score_function(result, query, user_context):
    """
    自定义评分函数
    
    Args:
        result: 检索结果
        query: 用户查询
        user_context: 用户上下文 (等级、偏好等)
    
    Returns:
        float: 最终得分
    """
    # 基础相似度
    base_score = result.similarity_score
    
    # 用户等级加成
    if user_context.get("level", 1) >= 10:
        # 高等级用户看到高级内容
        if result.metadata.get("difficulty") == "高级":
            base_score *= 1.2
    else:
        # 低等级用户看到基础内容
        if result.metadata.get("difficulty") == "基础":
            base_score *= 1.2
    
    # 用户偏好加成
    user_interests = user_context.get("interests", [])
    doc_tags = result.metadata.get("tags", [])
    overlap = set(user_interests) & set(doc_tags)
    if overlap:
        base_score *= (1 + 0.1 * len(overlap))
    
    return base_score

# 使用自定义评分
rag.set_score_function(custom_score_function)

results = rag.search(
    query="锻造教程",
    user_context={
        "level": 15,
        "interests": ["武器", "锻造"]
    }
)
```

### 4. 增量更新

```python
# 监控知识库目录，自动更新
import time
from watchdog.observers import Observer
from watchdog.events import FileSystemEventHandler

class KnowledgeBaseHandler(FileSystemEventHandler):
    def __init__(self, rag):
        self.rag = rag
    
    def on_created(self, event):
        if not event.is_directory:
            print(f"新文档: {event.src_path}")
            self.rag.add_document_from_file(event.src_path)
    
    def on_modified(self, event):
        if not event.is_directory:
            print(f"文档更新: {event.src_path}")
            self.rag.update_document_from_file(event.src_path)
    
    def on_deleted(self, event):
        if not event.is_directory:
            print(f"文档删除: {event.src_path}")
            self.rag.remove_document(event.src_path)

# 启动监控
observer = Observer()
handler = KnowledgeBaseHandler(rag)
observer.schedule(handler, "./knowledge_base", recursive=True)
observer.start()

print("📁 知识库监控已启动")

try:
    while True:
        time.sleep(1)
except KeyboardInterrupt:
    observer.stop()
observer.join()
```

---

## 📊 性能优化

### 1. 批量索引

```python
# 不推荐: 逐个添加文档
for doc in documents:
    rag.add_document(doc)  # 每次都重建索引，很慢

# 推荐: 批量添加
rag.add_documents_batch(documents)  # 一次性重建索引
```

### 2. 索引压缩

```python
# 使用量化索引 (减少内存占用)
config = RAGConfig(
    faiss_index_type="IndexIVFPQ",  # 乘积量化
    nlist=100,                       # 聚类数
    m=8,                             # 子量化器数量
    nbits=8                          # 每个子量化器的位数
)

# 内存占用: 原始 100MB → 压缩后 10MB
```

### 3. GPU加速

```python
# 使用GPU加速向量检索
import faiss

# 将索引移到GPU
gpu_index = faiss.index_cpu_to_gpu(
    faiss.StandardGpuResources(),
    0,  # GPU ID
    cpu_index
)

# 检索速度提升 10-100倍
```

### 4. 缓存策略

```python
from functools import lru_cache

class RAGTool:
    @lru_cache(maxsize=100)
    def search_cached(self, query, top_k=5):
        """缓存搜索结果"""
        return self.search(query, top_k)

# 相同查询直接返回缓存结果
results1 = rag.search_cached("如何锻造铁剑")  # 2秒
results2 = rag.search_cached("如何锻造铁剑")  # 0.001秒 (缓存)
```

---

## 🐛 调试技巧

### 1. 查看检索过程

```python
# 启用详细日志
config = RAGConfig(verbose=True)
rag = RAGTool(config)

results = rag.search("如何锻造铁剑", top_k=3)

# 输出:
# [RAG] 查询向量化: 0.05s
# [RAG] 向量检索: 0.12s (候选: 50)
# [RAG] 重排序: 0.03s
# [RAG] 返回结果: 3
```

### 2. 分析检索质量

```python
# 查看每个结果的详细信息
for i, result in enumerate(results, 1):
    print(f"\\n=== 结果 {i} ===")
    print(f"文档ID: {result.doc_id}")
    print(f"相似度: {result.similarity_score:.3f}")
    print(f"重排序得分: {result.final_score:.3f}")
    print(f"元数据: {result.metadata}")
    print(f"内容: {result.content[:100]}...")
    
    # 分析为什么这个结果被选中
    print(f"\\n得分分解:")
    print(f"  - 向量相似度: {result.vector_score:.3f}")
    print(f"  - 关键词匹配: {result.keyword_score:.3f}")
    print(f"  - 新鲜度: {result.freshness_score:.3f}")
    print(f"  - 权威性: {result.authority_score:.3f}")
```

### 3. 评估检索效果

```python
# 准备测试集
test_queries = [
    ("如何制作铁剑", ["ironsword_tutorial"]),
    ("铁剑多少钱", ["price_list"]),
    ("精钢和铁的区别", ["material_knowledge"])
]

# 评估
correct = 0
total = len(test_queries)

for query, expected_docs in test_queries:
    results = rag.search(query, top_k=3)
    retrieved_docs = [r.doc_id for r in results]
    
    # 检查是否检索到期望的文档
    if any(doc in retrieved_docs for doc in expected_docs):
        correct += 1
        print(f"✅ {query}")
    else:
        print(f"❌ {query}")
        print(f"   期望: {expected_docs}")
        print(f"   实际: {retrieved_docs}")

accuracy = correct / total
print(f"\\n准确率: {accuracy:.2%}")
```

---

## 🔗 相关文档

- [05-记忆系统详解](./05-MEMORY_SYSTEM.md) - 记忆与RAG的区别
- [07-上下文构建详解](./07-CONTEXT_BUILDER.md) - 如何整合RAG结果
- [10-NPC智能体详解](./10-NPC_AGENT.md) - RAG在Agent中的应用

---

恭喜！你现在已经完全掌握了RAG系统！🎉
