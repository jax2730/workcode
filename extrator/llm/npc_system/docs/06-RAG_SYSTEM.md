# 06 - RAG系统详解 (第1部分)

RAG (Retrieval-Augmented Generation) 是"检索增强生成"，让NPC能够从知识库中检索信息来回答问题。

## 🎯 什么是RAG？

### 简单比喻
想象你在考试：
- **没有RAG**: 只能靠记忆回答（可能记不全或记错）
- **有RAG**: 可以查书回答（准确、详细、可靠）

NPC的RAG系统就像给NPC配了一个"图书馆"！

### 核心概念
```
用户问题: "如何锻造铁剑？"
    ↓
RAG系统检索知识库
    ↓
找到相关文档: "铁剑锻造教程.md"
    ↓
提取相关段落
    ↓
LLM基于文档生成回答
    ↓
NPC回复: "需要3块铁锭和1根木棍..."
```

---

## 🏗️ RAG系统架构

```
┌─────────────────────────────────────────────────────────┐
│                    RAG系统                               │
├─────────────────────────────────────────────────────────┤
│                                                          │
│  ┌──────────────┐                                       │
│  │  文档加载器   │  ← 读取txt/md/json文件                │
│  │ Document     │                                       │
│  │ Loader       │                                       │
│  └──────┬───────┘                                       │
│         ↓                                                │
│  ┌──────────────┐                                       │
│  │  文档处理器   │  ← 分块、清洗、元数据提取             │
│  │ Document     │                                       │
│  │ Processor    │                                       │
│  └──────┬───────┘                                       │
│         ↓                                                │
│  ┌──────────────┐                                       │
│  │  向量化服务   │  ← 文本→向量 (可选)                   │
│  │ Embedding    │                                       │
│  │ Service      │                                       │
│  └──────┬───────┘                                       │
│         ↓                                                │
│  ┌──────────────┐                                       │
│  │  向量存储     │  ← FAISS索引 (可选)                   │
│  │ Vector       │    或余弦相似度 (备用)                 │
│  │ Store        │                                       │
│  └──────┬───────┘                                       │
│         ↓                                                │
│  ┌──────────────┐                                       │
│  │  检索器       │  ← 相似度搜索 + 重排序                │
│  │ Retriever    │                                       │
│  └──────────────┘                                       │
│                                                          │
└─────────────────────────────────────────────────────────┘
```

---

## 📚 核心组件详解

### 1. 文档加载器 (Document Loader)

#### 功能
从文件系统加载文档到内存。

#### 支持格式
```python
支持的文件格式:
├── .txt  - 纯文本
├── .md   - Markdown
├── .json - JSON数据
├── .pdf  - PDF文档 (需要额外库)
└── .docx - Word文档 (需要额外库)
```

#### 代码示例
```python
from npc_system import RAGTool, RAGConfig

# 创建RAG工具
rag = RAGTool(RAGConfig(
    knowledge_base_dir="./npc_data/knowledge_base/blacksmith",
    index_dir="./npc_data/rag_index/blacksmith"
))

# 加载单个文档
doc = rag.load_document("铁剑锻造教程.md")
print(doc.content)
print(doc.metadata)

# 批量加载目录下所有文档
docs = rag.load_documents_from_dir("./knowledge_base/")
print(f"加载了 {len(docs)} 个文档")
```

#### 文档结构
```python
class Document:
    """文档对象"""
    doc_id: str              # 文档ID
    content: str             # 文档内容
    metadata: dict           # 元数据
    chunks: List[str]        # 分块后的内容
    embedding: np.ndarray    # 向量嵌入(可选)
```

#### 元数据提取
```python
# 从文件名提取元数据
文件: "铁剑锻造教程_基础_2024.md"
元数据: {
    "title": "铁剑锻造教程",
    "category": "基础",
    "year": "2024",
    "file_type": "md",
    "file_size": 1024,
    "created_at": "2024-01-15"
}

# 从Markdown前置元数据提取
---
title: 铁剑锻造教程
author: 老铁匠
difficulty: 基础
tags: [锻造, 武器, 铁剑]
---

元数据: {
    "title": "铁剑锻造教程",
    "author": "老铁匠",
    "difficulty": "基础",
    "tags": ["锻造", "武器", "铁剑"]
}
```

---

### 2. 文档处理器 (Document Processor)

#### 功能
将长文档分块、清洗、标准化。

#### 为什么要分块？
```
问题: 文档太长 (5000字)
     ↓
LLM上下文限制 (只能处理2000 tokens)
     ↓
解决: 将文档分成小块 (每块500字)
     ↓
只检索相关的块，而不是整个文档
```

#### 分块策略

**策略1: 固定长度分块**
```python
# 每500字一块，重叠100字
chunks = processor.chunk_by_length(
    text=document.content,
    chunk_size=500,
    overlap=100
)

# 示例:
# 块1: [0:500]
# 块2: [400:900]   ← 与块1重叠100字
# 块3: [800:1300]  ← 与块2重叠100字
```

**策略2: 语义分块**
```python
# 按段落分块
chunks = processor.chunk_by_paragraph(text)

# 按章节分块
chunks = processor.chunk_by_section(text)

# 按句子分块
chunks = processor.chunk_by_sentence(text, sentences_per_chunk=5)
```

**策略3: 智能分块**
```python
# 根据内容自动选择分块点
chunks = processor.smart_chunk(
    text=document.content,
    max_chunk_size=500,
    prefer_boundaries=["\\n\\n", "。", "！", "？"]  # 优先在这些位置分块
)
```

#### 代码示例
```python
from npc_system.rag_tool import DocumentProcessor

processor = DocumentProcessor()

# 加载文档
with open("铁剑锻造教程.md", "r", encoding="utf-8") as f:
    text = f.read()

# 分块
chunks = processor.chunk_by_length(
    text=text,
    chunk_size=500,
    overlap=100
)

print(f"文档被分成 {len(chunks)} 块")

for i, chunk in enumerate(chunks):
    print(f"\\n=== 块 {i+1} ===")
    print(chunk[:100] + "...")
```

#### 文本清洗
```python
# 清洗文本
cleaned = processor.clean_text(text)

# 清洗操作:
# 1. 移除多余空白
# 2. 统一换行符
# 3. 移除特殊字符
# 4. 标准化标点
# 5. 移除HTML标签 (如果有)
```

---

### 3. 向量化服务 (Embedding Service)

#### 功能
将文本转换为数值向量，用于相似度计算。

#### 什么是向量嵌入？
```
文本: "铁剑需要3块铁锭"
  ↓ 向量化
向量: [0.23, -0.45, 0.67, ..., 0.12]  (768维)

文本: "锻造铁剑需要铁锭"
  ↓ 向量化
向量: [0.25, -0.43, 0.65, ..., 0.15]  (768维)

相似度计算:
cos_sim(向量1, 向量2) = 0.95  ← 非常相似！
```

#### 嵌入模型选择

**选项1: 本地模型 (推荐)**
```python
# 使用Ollama的嵌入模型
from langchain_ollama import OllamaEmbeddings

embeddings = OllamaEmbeddings(
    model="nomic-embed-text"  # 轻量级嵌入模型
)

# 嵌入单个文本
vector = embeddings.embed_query("铁剑锻造")
print(f"向量维度: {len(vector)}")  # 768

# 批量嵌入
vectors = embeddings.embed_documents([
    "铁剑锻造教程",
    "精钢剑制作方法",
    "武器维护指南"
])
```

**选项2: 简单嵌入 (备用)**
```python
# 不使用深度学习模型，使用TF-IDF
from sklearn.feature_extraction.text import TfidfVectorizer

vectorizer = TfidfVectorizer()
vectors = vectorizer.fit_transform([
    "铁剑锻造教程",
    "精钢剑制作方法"
])
```

**选项3: 无嵌入 (最简单)**
```python
# 直接使用余弦相似度 (基于词频)
# 不需要预训练模型，速度快但效果一般
```

#### 代码示例
```python
from npc_system import RAGTool, RAGConfig

# 配置RAG (启用嵌入)
config = RAGConfig(
    knowledge_base_dir="./knowledge_base",
    enable_embedding=True,
    embedding_model="nomic-embed-text"
)

rag = RAGTool(config)

# 添加文档 (自动嵌入)
rag.add_document(
    content="铁剑需要3块铁锭和1根木棍制作...",
    doc_id="ironsword_tutorial"
)

# 搜索 (使用向量相似度)
results = rag.search("如何制作铁剑", top_k=3)
```

---

### 4. 向量存储 (Vector Store)

#### 功能
存储和检索向量，支持高效的相似度搜索。

#### FAISS索引

**什么是FAISS？**
- Facebook开发的向量检索库
- 支持百万级向量的快速搜索
- 支持GPU加速

**安装FAISS**
```bash
# CPU版本
pip install faiss-cpu

# GPU版本 (如果有NVIDIA GPU)
pip install faiss-gpu
```

**使用FAISS**
```python
import faiss
import numpy as np

# 创建索引
dimension = 768  # 向量维度
index = faiss.IndexFlatL2(dimension)  # L2距离索引

# 添加向量
vectors = np.random.random((100, dimension)).astype('float32')
index.add(vectors)

# 搜索
query_vector = np.random.random((1, dimension)).astype('float32')
distances, indices = index.search(query_vector, k=5)

print(f"最相似的5个文档索引: {indices[0]}")
print(f"距离: {distances[0]}")
```

**索引类型**
```python
# 1. 精确搜索 (小数据集)
index = faiss.IndexFlatL2(dimension)

# 2. IVF索引 (中等数据集)
quantizer = faiss.IndexFlatL2(dimension)
index = faiss.IndexIVFFlat(quantizer, dimension, 100)  # 100个聚类

# 3. HNSW索引 (大数据集)
index = faiss.IndexHNSWFlat(dimension, 32)  # 32个邻居
```

#### 备用方案: 余弦相似度

**不使用FAISS时**
```python
from sklearn.metrics.pairwise import cosine_similarity
import numpy as np

class SimpleVectorStore:
    def __init__(self):
        self.vectors = []
        self.doc_ids = []
    
    def add(self, vector, doc_id):
        self.vectors.append(vector)
        self.doc_ids.append(doc_id)
    
    def search(self, query_vector, top_k=5):
        # 计算余弦相似度
        similarities = cosine_similarity(
            [query_vector],
            self.vectors
        )[0]
        
        # 排序
        top_indices = np.argsort(similarities)[::-1][:top_k]
        
        results = []
        for idx in top_indices:
            results.append({
                "doc_id": self.doc_ids[idx],
                "score": similarities[idx]
            })
        
        return results
```

---

### 5. 检索器 (Retriever)

#### 功能
根据查询检索最相关的文档块。

#### 检索流程
```
用户查询: "如何锻造铁剑？"
    ↓
1. 查询向量化
   query_vector = embed("如何锻造铁剑？")
    ↓
2. 向量搜索
   top_docs = vector_store.search(query_vector, k=10)
    ↓
3. 重排序 (可选)
   - 考虑文档元数据 (日期、作者、标签)
   - 考虑文档质量
   - 考虑多样性
    ↓
4. 返回Top-K结果
   results = top_docs[:5]
```

#### 检索策略

**策略1: 纯向量检索**
```python
results = rag.search(
    query="如何锻造铁剑",
    method="vector",
    top_k=5
)
```

**策略2: 关键词检索**
```python
results = rag.search(
    query="铁剑 锻造",
    method="keyword",
    top_k=5
)
```

**策略3: 混合检索**
```python
# 结合向量和关键词
results = rag.search(
    query="如何锻造铁剑",
    method="hybrid",
    vector_weight=0.7,    # 向量权重
    keyword_weight=0.3,   # 关键词权重
    top_k=5
)
```

**策略4: 元数据过滤**
```python
# 先过滤，再检索
results = rag.search(
    query="锻造方法",
    filters={
        "category": "武器制作",
        "difficulty": "基础",
        "author": "老铁匠"
    },
    top_k=5
)
```

#### 重排序

**为什么需要重排序？**
```
初始检索结果 (仅基于相似度):
1. 文档A: 相似度0.95, 但是过时的信息
2. 文档B: 相似度0.92, 最新的教程
3. 文档C: 相似度0.90, 权威作者

重排序后 (考虑多个因素):
1. 文档B: 综合得分0.96 (最新 + 高相似度)
2. 文档C: 综合得分0.94 (权威 + 高相似度)
3. 文档A: 综合得分0.85 (过时 - 扣分)
```

**重排序算法**
```python
def rerank(results, query, metadata_weights):
    """
    重排序算法
    
    综合得分 = 相似度 × 0.6 
              + 新鲜度 × 0.2 
              + 权威性 × 0.1 
              + 完整性 × 0.1
    """
    for result in results:
        # 基础相似度得分
        similarity_score = result.score
        
        # 新鲜度得分 (越新越好)
        days_old = (now - result.created_at).days
        freshness_score = max(0, 1 - days_old / 365)
        
        # 权威性得分 (基于作者)
        authority_score = metadata_weights.get(
            result.author, 0.5
        )
        
        # 完整性得分 (文档长度)
        completeness_score = min(1, len(result.content) / 1000)
        
        # 综合得分
        result.final_score = (
            similarity_score * 0.6 +
            freshness_score * 0.2 +
            authority_score * 0.1 +
            completeness_score * 0.1
        )
    
    # 按综合得分排序
    return sorted(results, key=lambda x: x.final_score, reverse=True)
```

---

## 🔄 完整工作流程

### 1. 索引构建阶段

```python
from npc_system import RAGTool, RAGConfig

# 1. 创建RAG工具
rag = RAGTool(RAGConfig(
    knowledge_base_dir="./knowledge_base/blacksmith",
    index_dir="./rag_index/blacksmith",
    enable_embedding=True
))

# 2. 加载文档
docs = rag.load_documents_from_dir("./knowledge_base/blacksmith/")
print(f"加载了 {len(docs)} 个文档")

# 3. 处理文档 (分块)
for doc in docs:
    chunks = rag.chunk_document(doc, chunk_size=500)
    print(f"{doc.doc_id}: {len(chunks)} 块")

# 4. 向量化
for doc in docs:
    rag.embed_document(doc)
    print(f"{doc.doc_id}: 向量化完成")

# 5. 构建索引
rag.build_index()
print("索引构建完成")

# 6. 保存索引
rag.save_index("./rag_index/blacksmith/index.faiss")
print("索引已保存")
```

### 2. 检索阶段

```python
# 1. 加载索引
rag.load_index("./rag_index/blacksmith/index.faiss")

# 2. 用户查询
query = "如何锻造铁剑？"

# 3. 检索
results = rag.search(query, top_k=3)

# 4. 查看结果
for i, result in enumerate(results, 1):
    print(f"\\n=== 结果 {i} ===")
    print(f"文档: {result.doc_id}")
    print(f"相似度: {result.score:.3f}")
    print(f"内容: {result.content[:200]}...")
    print(f"元数据: {result.metadata}")
```

### 3. 生成阶段

```python
# 1. 检索相关文档
context_docs = rag.search("如何锻造铁剑", top_k=3)

# 2. 构建上下文
context = "\\n\\n".join([
    f"【文档{i+1}】\\n{doc.content}"
    for i, doc in enumerate(context_docs)
])

# 3. 构建提示词
prompt = f"""基于以下知识库内容回答问题。

知识库:
{context}

问题: 如何锻造铁剑？

请基于知识库内容回答:"""

# 4. LLM生成
response = llm.invoke(prompt)
print(response.content)
```

---

继续阅读 [06-RAG系统详解(第2部分)](./06-RAG_SYSTEM_PART2.md)
