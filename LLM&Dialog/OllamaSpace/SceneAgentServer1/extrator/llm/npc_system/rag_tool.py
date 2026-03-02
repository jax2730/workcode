"""
RAGTool - 检索增强生成工具
基于 HelloAgents 第8章设计

功能:
- 文档加载和分块
- 向量化存储 (FAISS)
- 多策略检索 (基础/MQE/HyDE)
- 智能问答
"""

import os
import json
import hashlib
import pickle
from datetime import datetime
from pathlib import Path
from dataclasses import dataclass, field
from typing import List, Dict, Any, Optional, Tuple
import numpy as np

# 尝试导入可选依赖
try:
    import faiss
    FAISS_AVAILABLE = True
except ImportError:
    FAISS_AVAILABLE = False
    print("[RAGTool] FAISS未安装，将使用简单的余弦相似度检索")

try:
    from langchain_ollama import OllamaEmbeddings
    OLLAMA_EMBEDDINGS_AVAILABLE = True
except ImportError:
    OLLAMA_EMBEDDINGS_AVAILABLE = False
    print("[RAGTool] OllamaEmbeddings未安装，将使用TF-IDF")


# ==================== 数据结构 ====================

@dataclass
class RAGConfig:
    """RAG配置"""
    knowledge_base_path: str = "./knowledge_base"
    index_path: str = "./rag_index"
    embedding_model: str = "nomic-embed-text"  # Ollama嵌入模型
    embedding_dim: int = 768
    chunk_size: int = 500           # 分块大小(字符)
    chunk_overlap: int = 100        # 分块重叠
    top_k: int = 5                  # 检索数量
    min_score: float = 0.3          # 最低相关性分数
    enable_mqe: bool = True         # 多查询扩展
    enable_hyde: bool = False       # 假设文档嵌入
    mqe_expansions: int = 2         # MQE扩展数量


@dataclass
class Document:
    """文档对象"""
    doc_id: str
    content: str
    metadata: Dict[str, Any] = field(default_factory=dict)
    embedding: Optional[np.ndarray] = None
    
    def to_dict(self) -> dict:
        return {
            "doc_id": self.doc_id,
            "content": self.content,
            "metadata": self.metadata
        }


@dataclass
class SearchResult:
    """检索结果"""
    doc_id: str
    content: str
    score: float
    metadata: Dict[str, Any] = field(default_factory=dict)


# ==================== 嵌入服务 ====================

class EmbeddingService:
    """嵌入服务 - 支持多种后端"""
    
    def __init__(self, model_name: str = "nomic-embed-text", dim: int = 768):
        self.model_name = model_name
        self.dim = dim
        self.embedder = None
        self._init_embedder()
    
    def _init_embedder(self):
        """初始化嵌入器"""
        if OLLAMA_EMBEDDINGS_AVAILABLE:
            try:
                self.embedder = OllamaEmbeddings(model=self.model_name)
                # 测试连接
                test_embedding = self.embedder.embed_query("test")
                self.dim = len(test_embedding)
                print(f"[EmbeddingService] 使用Ollama嵌入模型: {self.model_name}, 维度: {self.dim}")
                return
            except Exception as e:
                print(f"[EmbeddingService] Ollama嵌入初始化失败: {e}")
        
        # 回退到TF-IDF
        print("[EmbeddingService] 使用TF-IDF嵌入")
        self.embedder = None
    
    def embed_text(self, text: str) -> np.ndarray:
        """嵌入单个文本"""
        if self.embedder:
            try:
                embedding = self.embedder.embed_query(text)
                return np.array(embedding, dtype=np.float32)
            except Exception as e:
                print(f"[EmbeddingService] 嵌入失败: {e}")
        
        # TF-IDF回退
        return self._tfidf_embed(text)
    
    def embed_texts(self, texts: List[str]) -> np.ndarray:
        """批量嵌入文本"""
        if self.embedder:
            try:
                embeddings = self.embedder.embed_documents(texts)
                return np.array(embeddings, dtype=np.float32)
            except Exception as e:
                print(f"[EmbeddingService] 批量嵌入失败: {e}")
        
        # TF-IDF回退
        return np.array([self._tfidf_embed(t) for t in texts], dtype=np.float32)
    
    def _tfidf_embed(self, text: str) -> np.ndarray:
        """简单的TF-IDF嵌入"""
        # 使用词频作为简单嵌入
        words = text.lower().split()
        word_freq = {}
        for word in words:
            word_freq[word] = word_freq.get(word, 0) + 1
        
        # 创建固定维度的向量
        embedding = np.zeros(self.dim, dtype=np.float32)
        for i, (word, freq) in enumerate(sorted(word_freq.items())[:self.dim]):
            embedding[hash(word) % self.dim] += freq
        
        # 归一化
        norm = np.linalg.norm(embedding)
        if norm > 0:
            embedding /= norm
        
        return embedding


# ==================== 向量存储 ====================

class VectorStore:
    """向量存储 - 支持FAISS和简单余弦相似度"""
    
    def __init__(self, dim: int = 768, index_path: str = "./rag_index"):
        self.dim = dim
        self.index_path = Path(index_path)
        self.index_path.mkdir(parents=True, exist_ok=True)
        
        self.documents: Dict[str, Document] = {}
        self.id_to_idx: Dict[str, int] = {}
        self.idx_to_id: Dict[int, str] = {}
        
        self.index = None
        self.embeddings: Optional[np.ndarray] = None
        
        self._init_index()
        self._load_index()
    
    def _init_index(self):
        """初始化索引"""
        if FAISS_AVAILABLE:
            self.index = faiss.IndexFlatIP(self.dim)  # 内积相似度
            print(f"[VectorStore] 使用FAISS索引, 维度: {self.dim}")
        else:
            self.embeddings = np.zeros((0, self.dim), dtype=np.float32)
            print(f"[VectorStore] 使用NumPy余弦相似度, 维度: {self.dim}")
    
    def add_documents(self, documents: List[Document]):
        """添加文档"""
        if not documents:
            return
        
        new_embeddings = []
        for doc in documents:
            if doc.doc_id in self.documents:
                continue
            
            idx = len(self.documents)
            self.documents[doc.doc_id] = doc
            self.id_to_idx[doc.doc_id] = idx
            self.idx_to_id[idx] = doc.doc_id
            
            if doc.embedding is not None:
                new_embeddings.append(doc.embedding)
        
        if new_embeddings:
            embeddings_array = np.array(new_embeddings, dtype=np.float32)
            
            if FAISS_AVAILABLE and self.index is not None:
                # 归一化用于内积相似度
                faiss.normalize_L2(embeddings_array)
                self.index.add(embeddings_array)
            else:
                if self.embeddings is None or len(self.embeddings) == 0:
                    self.embeddings = embeddings_array
                else:
                    self.embeddings = np.vstack([self.embeddings, embeddings_array])
        
        self._save_index()
    
    def search(self, query_embedding: np.ndarray, top_k: int = 5) -> List[Tuple[str, float]]:
        """搜索相似文档"""
        if len(self.documents) == 0:
            return []
        
        query_embedding = query_embedding.reshape(1, -1).astype(np.float32)
        
        if FAISS_AVAILABLE and self.index is not None:
            # FAISS搜索
            faiss.normalize_L2(query_embedding)
            scores, indices = self.index.search(query_embedding, min(top_k, len(self.documents)))
            
            results = []
            for score, idx in zip(scores[0], indices[0]):
                if idx >= 0 and idx in self.idx_to_id:
                    doc_id = self.idx_to_id[idx]
                    results.append((doc_id, float(score)))
            return results
        else:
            # NumPy余弦相似度
            if self.embeddings is None or len(self.embeddings) == 0:
                return []
            
            # 归一化
            query_norm = query_embedding / (np.linalg.norm(query_embedding) + 1e-8)
            embeddings_norm = self.embeddings / (np.linalg.norm(self.embeddings, axis=1, keepdims=True) + 1e-8)
            
            # 计算相似度
            similarities = np.dot(embeddings_norm, query_norm.T).flatten()
            
            # 排序
            top_indices = np.argsort(similarities)[::-1][:top_k]
            
            results = []
            for idx in top_indices:
                if idx in self.idx_to_id:
                    doc_id = self.idx_to_id[idx]
                    results.append((doc_id, float(similarities[idx])))
            return results
    
    def _save_index(self):
        """保存索引"""
        # 保存文档
        docs_path = self.index_path / "documents.pkl"
        with open(docs_path, 'wb') as f:
            pickle.dump({
                "documents": {k: v.to_dict() for k, v in self.documents.items()},
                "id_to_idx": self.id_to_idx,
                "idx_to_id": self.idx_to_id
            }, f)
        
        # 保存FAISS索引
        if FAISS_AVAILABLE and self.index is not None:
            faiss_path = self.index_path / "index.faiss"
            faiss.write_index(self.index, str(faiss_path))
        elif self.embeddings is not None:
            embeddings_path = self.index_path / "embeddings.npy"
            np.save(embeddings_path, self.embeddings)
    
    def _load_index(self):
        """加载索引"""
        docs_path = self.index_path / "documents.pkl"
        if docs_path.exists():
            try:
                with open(docs_path, 'rb') as f:
                    data = pickle.load(f)
                
                self.documents = {
                    k: Document(**v) for k, v in data["documents"].items()
                }
                self.id_to_idx = data["id_to_idx"]
                self.idx_to_id = {int(k): v for k, v in data["idx_to_id"].items()}
                
                print(f"[VectorStore] 加载了 {len(self.documents)} 个文档")
            except Exception as e:
                print(f"[VectorStore] 加载文档失败: {e}")
        
        # 加载FAISS索引
        if FAISS_AVAILABLE:
            faiss_path = self.index_path / "index.faiss"
            if faiss_path.exists():
                try:
                    self.index = faiss.read_index(str(faiss_path))
                    print(f"[VectorStore] 加载FAISS索引成功")
                except Exception as e:
                    print(f"[VectorStore] 加载FAISS索引失败: {e}")
                    self._init_index()
        else:
            embeddings_path = self.index_path / "embeddings.npy"
            if embeddings_path.exists():
                try:
                    self.embeddings = np.load(embeddings_path)
                    print(f"[VectorStore] 加载嵌入向量成功")
                except Exception as e:
                    print(f"[VectorStore] 加载嵌入向量失败: {e}")
    
    def clear(self):
        """清空索引"""
        self.documents.clear()
        self.id_to_idx.clear()
        self.idx_to_id.clear()
        self._init_index()
        
        # 删除文件
        for f in self.index_path.glob("*"):
            f.unlink()


# ==================== 文档处理 ====================

class DocumentProcessor:
    """文档处理器"""
    
    def __init__(self, chunk_size: int = 500, chunk_overlap: int = 100):
        self.chunk_size = chunk_size
        self.chunk_overlap = chunk_overlap
    
    def load_file(self, file_path: str) -> str:
        """加载文件"""
        path = Path(file_path)
        
        if not path.exists():
            raise FileNotFoundError(f"文件不存在: {file_path}")
        
        suffix = path.suffix.lower()
        
        if suffix in ['.txt', '.md', '.json']:
            with open(path, 'r', encoding='utf-8') as f:
                return f.read()
        else:
            raise ValueError(f"不支持的文件格式: {suffix}")
    
    def chunk_text(self, text: str, metadata: Dict = None) -> List[Document]:
        """分块文本"""
        chunks = []
        
        # 按段落分割
        paragraphs = text.split('\n\n')
        
        current_chunk = ""
        chunk_idx = 0
        
        for para in paragraphs:
            para = para.strip()
            if not para:
                continue
            
            if len(current_chunk) + len(para) <= self.chunk_size:
                current_chunk += para + "\n\n"
            else:
                if current_chunk:
                    doc_id = f"chunk_{hashlib.md5(current_chunk.encode()).hexdigest()[:8]}_{chunk_idx}"
                    chunks.append(Document(
                        doc_id=doc_id,
                        content=current_chunk.strip(),
                        metadata={
                            **(metadata or {}),
                            "chunk_idx": chunk_idx
                        }
                    ))
                    chunk_idx += 1
                
                # 保留重叠部分
                if self.chunk_overlap > 0 and len(current_chunk) > self.chunk_overlap:
                    current_chunk = current_chunk[-self.chunk_overlap:] + para + "\n\n"
                else:
                    current_chunk = para + "\n\n"
        
        # 最后一个块
        if current_chunk.strip():
            doc_id = f"chunk_{hashlib.md5(current_chunk.encode()).hexdigest()[:8]}_{chunk_idx}"
            chunks.append(Document(
                doc_id=doc_id,
                content=current_chunk.strip(),
                metadata={
                    **(metadata or {}),
                    "chunk_idx": chunk_idx
                }
            ))
        
        return chunks
    
    def load_directory(self, dir_path: str) -> List[Document]:
        """加载目录下所有文档"""
        path = Path(dir_path)
        if not path.exists():
            return []
        
        all_docs = []
        
        for file_path in path.rglob("*"):
            if file_path.suffix.lower() in ['.txt', '.md', '.json']:
                try:
                    content = self.load_file(str(file_path))
                    docs = self.chunk_text(content, metadata={
                        "source": str(file_path),
                        "filename": file_path.name
                    })
                    all_docs.extend(docs)
                    print(f"[DocumentProcessor] 加载文件: {file_path.name}, {len(docs)}个分块")
                except Exception as e:
                    print(f"[DocumentProcessor] 加载文件失败 {file_path}: {e}")
        
        return all_docs


# ==================== RAGTool ====================

class RAGTool:
    """
    RAG工具 - 检索增强生成
    
    功能:
    - 添加文档/文本到知识库
    - 多策略检索 (基础/MQE/HyDE)
    - 智能问答
    """
    
    def __init__(self, config: RAGConfig = None, llm=None):
        self.config = config or RAGConfig()
        self.llm = llm
        
        # 初始化组件
        self.embedding_service = EmbeddingService(
            model_name=self.config.embedding_model,
            dim=self.config.embedding_dim
        )
        
        # 更新维度
        self.config.embedding_dim = self.embedding_service.dim
        
        self.vector_store = VectorStore(
            dim=self.config.embedding_dim,
            index_path=self.config.index_path
        )
        
        self.doc_processor = DocumentProcessor(
            chunk_size=self.config.chunk_size,
            chunk_overlap=self.config.chunk_overlap
        )
        
        # 自动加载知识库
        self._auto_load_knowledge_base()
    
    def _auto_load_knowledge_base(self):
        """自动加载知识库目录"""
        kb_path = Path(self.config.knowledge_base_path)
        if kb_path.exists() and len(self.vector_store.documents) == 0:
            print(f"[RAGTool] 自动加载知识库: {kb_path}")
            self.add_directory(str(kb_path))
    
    def execute(self, action: str, **kwargs) -> Any:
        """执行RAG操作"""
        actions = {
            "add_text": self._add_text,
            "add_document": self._add_document,
            "add_directory": self._add_directory,
            "search": self._search,
            "ask": self._ask,
            "stats": self._stats,
            "clear": self._clear,
        }
        
        if action not in actions:
            raise ValueError(f"未知操作: {action}, 可用操作: {list(actions.keys())}")
        
        return actions[action](**kwargs)
    
    def _add_text(self, text: str, doc_id: str = None, 
                  metadata: Dict = None, **kwargs) -> str:
        """添加文本"""
        if not doc_id:
            doc_id = f"text_{hashlib.md5(text.encode()).hexdigest()[:8]}"
        
        # 分块
        docs = self.doc_processor.chunk_text(text, metadata)
        
        # 嵌入
        for doc in docs:
            doc.embedding = self.embedding_service.embed_text(doc.content)
        
        # 存储
        self.vector_store.add_documents(docs)
        
        return f"添加了 {len(docs)} 个文档块"
    
    def _add_document(self, file_path: str, **kwargs) -> str:
        """添加文档文件"""
        content = self.doc_processor.load_file(file_path)
        docs = self.doc_processor.chunk_text(content, metadata={
            "source": file_path,
            "filename": Path(file_path).name
        })
        
        # 嵌入
        for doc in docs:
            doc.embedding = self.embedding_service.embed_text(doc.content)
        
        # 存储
        self.vector_store.add_documents(docs)
        
        return f"添加了 {len(docs)} 个文档块 (来自 {Path(file_path).name})"
    
    def _add_directory(self, dir_path: str, **kwargs) -> str:
        """添加目录"""
        docs = self.doc_processor.load_directory(dir_path)
        
        if not docs:
            return "目录为空或无有效文档"
        
        # 批量嵌入
        contents = [doc.content for doc in docs]
        embeddings = self.embedding_service.embed_texts(contents)
        
        for doc, emb in zip(docs, embeddings):
            doc.embedding = emb
        
        # 存储
        self.vector_store.add_documents(docs)
        
        return f"添加了 {len(docs)} 个文档块"
    
    # 公开方法别名
    def add_text(self, text: str, **kwargs) -> str:
        return self._add_text(text, **kwargs)
    
    def add_document(self, file_path: str, **kwargs) -> str:
        return self._add_document(file_path, **kwargs)
    
    def add_directory(self, dir_path: str, **kwargs) -> str:
        return self._add_directory(dir_path, **kwargs)
    
    def _search(self, query: str, limit: int = None, 
                min_score: float = None,
                enable_mqe: bool = None,
                enable_hyde: bool = None,
                **kwargs) -> List[SearchResult]:
        """检索文档"""
        limit = limit or self.config.top_k
        min_score = min_score if min_score is not None else self.config.min_score
        enable_mqe = enable_mqe if enable_mqe is not None else self.config.enable_mqe
        enable_hyde = enable_hyde if enable_hyde is not None else self.config.enable_hyde
        
        # 收集所有查询
        queries = [query]
        
        # MQE: 多查询扩展
        if enable_mqe and self.llm:
            expanded = self._expand_query(query)
            queries.extend(expanded)
        
        # HyDE: 假设文档嵌入
        if enable_hyde and self.llm:
            hyde_doc = self._generate_hyde(query)
            if hyde_doc:
                queries.append(hyde_doc)
        
        # 执行检索
        all_results: Dict[str, SearchResult] = {}
        
        for q in queries:
            query_embedding = self.embedding_service.embed_text(q)
            results = self.vector_store.search(query_embedding, limit * 2)
            
            for doc_id, score in results:
                if score < min_score:
                    continue
                
                if doc_id not in all_results or score > all_results[doc_id].score:
                    doc = self.vector_store.documents.get(doc_id)
                    if doc:
                        all_results[doc_id] = SearchResult(
                            doc_id=doc_id,
                            content=doc.content,
                            score=score,
                            metadata=doc.metadata
                        )
        
        # 排序并返回
        sorted_results = sorted(all_results.values(), key=lambda x: x.score, reverse=True)
        return sorted_results[:limit]
    
    def search(self, query: str, **kwargs) -> List[SearchResult]:
        """检索文档 (公开方法)"""
        return self._search(query, **kwargs)
    
    def _ask(self, question: str, limit: int = None, **kwargs) -> str:
        """智能问答"""
        if not self.llm:
            return "LLM未配置，无法进行问答"
        
        # 检索相关文档
        results = self._search(question, limit=limit or 5)
        
        if not results:
            return "未找到相关信息"
        
        # 构建上下文
        context_parts = []
        for i, result in enumerate(results, 1):
            context_parts.append(f"[{i}] {result.content}")
        
        context = "\n\n".join(context_parts)
        
        # 构建提示词
        prompt = f"""基于以下参考信息回答问题。如果信息不足，请说明。

【参考信息】
{context}

【问题】
{question}

【回答】"""
        
        try:
            response = self.llm.invoke([{"role": "user", "content": prompt}])
            if hasattr(response, 'content'):
                return response.content
            return str(response)
        except Exception as e:
            return f"问答失败: {e}"
    
    def ask(self, question: str, **kwargs) -> str:
        """智能问答 (公开方法)"""
        return self._ask(question, **kwargs)
    
    def _expand_query(self, query: str) -> List[str]:
        """MQE: 多查询扩展"""
        if not self.llm:
            return []
        
        prompt = f"""请为以下查询生成{self.config.mqe_expansions}个语义相似但表述不同的查询。
每行一个，不要编号。

原始查询: {query}

扩展查询:"""
        
        try:
            response = self.llm.invoke([{"role": "user", "content": prompt}])
            if hasattr(response, 'content'):
                response = response.content
            
            lines = [l.strip() for l in str(response).split('\n') if l.strip()]
            return lines[:self.config.mqe_expansions]
        except Exception as e:
            print(f"[RAGTool] MQE扩展失败: {e}")
            return []
    
    def _generate_hyde(self, query: str) -> Optional[str]:
        """HyDE: 生成假设文档"""
        if not self.llm:
            return None
        
        prompt = f"""请根据以下问题，写一段可能的答案性段落（约100字）。
这段文字将用于检索相关文档。

问题: {query}

答案段落:"""
        
        try:
            response = self.llm.invoke([{"role": "user", "content": prompt}])
            if hasattr(response, 'content'):
                return response.content
            return str(response)
        except Exception as e:
            print(f"[RAGTool] HyDE生成失败: {e}")
            return None
    
    def _stats(self, **kwargs) -> Dict[str, Any]:
        """统计信息"""
        return {
            "total_documents": len(self.vector_store.documents),
            "embedding_dim": self.config.embedding_dim,
            "embedding_model": self.config.embedding_model,
            "knowledge_base_path": self.config.knowledge_base_path,
            "index_path": self.config.index_path
        }
    
    def _clear(self, **kwargs) -> str:
        """清空知识库"""
        self.vector_store.clear()
        return "知识库已清空"


# ==================== 便捷函数 ====================

def create_rag_tool(knowledge_base_path: str = "./knowledge_base",
                    llm=None) -> RAGTool:
    """创建RAG工具"""
    config = RAGConfig(knowledge_base_path=knowledge_base_path)
    return RAGTool(config, llm)
