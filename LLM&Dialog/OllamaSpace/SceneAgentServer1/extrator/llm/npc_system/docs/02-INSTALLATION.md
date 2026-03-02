# 02 - 安装配置指南

> **面向对象**: 系统开发者、运维人员  
> **前置知识**: Linux基础、Python环境管理  
> **相关文档**: [01-快速入门](./01-QUICKSTART.md)

## 📋 系统要求

### 硬件要求

#### 最低配置
- **CPU**: 4核心
- **内存**: 8GB RAM
- **存储**: 20GB 可用空间
- **网络**: 稳定的网络连接

#### 推荐配置
- **CPU**: 8核心或更多
- **内存**: 16GB RAM 或更多
- **存储**: 50GB SSD
- **GPU**: NVIDIA GPU (可选，用于FAISS加速)

### 软件要求

#### 操作系统
- Ubuntu 20.04+ / Debian 11+
- CentOS 8+ / RHEL 8+
- macOS 11+
- Windows 10+ (WSL2推荐)

#### Python环境
- Python 3.8+
- pip 20.0+
- virtualenv 或 conda

#### 数据库
- SQLite 3.31+ (内置)

#### 其他依赖
- Ollama (LLM服务)
- Git (版本控制)

---

## 🚀 安装步骤

### 步骤1: 安装Python环境

#### Ubuntu/Debian
```bash
# 更新包列表
sudo apt update

# 安装Python 3.10
sudo apt install python3.10 python3.10-venv python3.10-dev

# 安装pip
sudo apt install python3-pip

# 验证安装
python3.10 --version
pip3 --version
```

#### CentOS/RHEL
```bash
# 安装EPEL仓库
sudo yum install epel-release

# 安装Python 3.10
sudo yum install python310 python310-devel

# 安装pip
sudo yum install python3-pip

# 验证安装
python3.10 --version
pip3 --version
```

#### macOS
```bash
# 使用Homebrew安装
brew install python@3.10

# 验证安装
python3.10 --version
pip3 --version
```

#### Windows (WSL2)
```bash
# 在WSL2中执行Ubuntu的安装步骤
# 或者直接下载Python安装包
# https://www.python.org/downloads/
```

---

### 步骤2: 安装Ollama

#### Linux
```bash
# 下载并安装Ollama
curl -fsSL https://ollama.com/install.sh | sh

# 验证安装
ollama --version

# 启动Ollama服务
ollama serve &

# 拉取qwen2.5模型
ollama pull qwen2.5:7b

# 验证模型
ollama list
```

#### macOS
```bash
# 下载Ollama安装包
# https://ollama.com/download/mac

# 或使用Homebrew
brew install ollama

# 启动服务
ollama serve &

# 拉取模型
ollama pull qwen2.5:7b
```

#### Windows
```bash
# 下载Ollama安装包
# https://ollama.com/download/windows

# 安装后，在PowerShell中运行
ollama pull qwen2.5:7b
```

---

### 步骤3: 克隆项目

```bash
# 克隆项目
git clone <your-repo-url> LLM-Dialog
cd LLM-Dialog/OllamaSpace/SceneAgentServer

# 查看项目结构
ls -la
```

---

### 步骤4: 创建虚拟环境

#### 使用venv
```bash
# 创建虚拟环境
python3.10 -m venv venv

# 激活虚拟环境
# Linux/macOS:
source venv/bin/activate

# Windows:
venv\Scripts\activate

# 验证
which python
python --version
```

#### 使用conda
```bash
# 创建conda环境
conda create -n npc-system python=3.10

# 激活环境
conda activate npc-system

# 验证
which python
python --version
```

---

### 步骤5: 安装Python依赖

#### 核心依赖
```bash
# 安装核心依赖
pip install langchain-ollama langchain-core langchain-community

# 安装数据处理
pip install pandas openpyxl

# 安装向量检索 (可选)
pip install faiss-cpu  # CPU版本
# 或
pip install faiss-gpu  # GPU版本 (需要CUDA)

# 安装其他依赖
pip install prompt_toolkit pyyaml python-dotenv

# 验证安装
pip list | grep langchain
pip list | grep faiss
```

#### 完整依赖列表

创建 `requirements.txt`:
```txt
# LLM和LangChain
langchain-ollama>=0.1.0
langchain-core>=0.1.0
langchain-community>=0.0.20

# 数据处理
pandas>=2.0.0
openpyxl>=3.1.0
pyyaml>=6.0

# 向量检索 (可选)
faiss-cpu>=1.7.4
# faiss-gpu>=1.7.4  # 如果有GPU

# 命令行工具
prompt_toolkit>=3.0.0

# 工具库
python-dotenv>=1.0.0
requests>=2.31.0

# 开发工具 (可选)
pytest>=7.4.0
black>=23.0.0
flake8>=6.0.0
```

安装：
```bash
pip install -r requirements.txt
```

---

### 步骤6: 初始化数据目录

```bash
# 进入项目目录
cd ~/OllamaSpace/SceneAgentServer

# 初始化数据目录
python -m extrator.llm.npc_system.init_data_dirs

# 验证目录结构
ls -la npc_data/
```

**预期输出**:
```
npc_data/
├── memories/
│   ├── working/
│   ├── episodic/
│   ├── semantic/
│   └── dialogues/
├── databases/
│   ├── dialogue_history.db
│   ├── npc_memory.db
│   └── npc_relationship.db
├── exports/
│   ├── excel/
│   └── json/
├── knowledge_base/
├── rag_index/
├── notes/
├── configs/
└── logs/
```

---

### 步骤7: 配置环境变量

创建 `.env` 文件：
```bash
# 在项目根目录创建.env
cat > .env << 'EOF'
# Ollama配置
OLLAMA_HOST=http://localhost:11434
OLLAMA_MODEL=qwen2.5:7b

# 数据目录
NPC_DATA_DIR=./npc_data
NPC_CONFIG_DIR=./npc_configs

# 日志配置
LOG_LEVEL=INFO
LOG_FILE=./npc_data/logs/npc_system.log

# 性能配置
ENABLE_CACHE=true
CACHE_TTL=300
MAX_WORKERS=4

# RAG配置
ENABLE_FAISS=true
EMBEDDING_MODEL=nomic-embed-text

# 数据库配置
DB_PATH=./npc_data/databases/
EOF
```

---

### 步骤8: 验证安装

#### 测试Ollama连接
```bash
# 测试Ollama API
curl http://localhost:11434/api/tags

# 测试模型
ollama run qwen2.5:7b "你好"
```

#### 测试Python环境
```bash
# 测试导入
python -c "from langchain_ollama import ChatOllama; print('✅ LangChain OK')"
python -c "import faiss; print('✅ FAISS OK')"
python -c "import pandas; print('✅ Pandas OK')"
```

#### 运行示例
```bash
# 运行NPC对话测试
python -m extrator.llm.npc_system.npc_chat

# 或运行示例脚本
python extrator/llm/npc_system/example.py
```

---

## ⚙️ 高级配置

### 1. GPU加速 (可选)

#### 安装CUDA (NVIDIA GPU)
```bash
# Ubuntu
sudo apt install nvidia-cuda-toolkit

# 验证CUDA
nvcc --version
nvidia-smi

# 安装FAISS GPU版本
pip uninstall faiss-cpu
pip install faiss-gpu
```

#### 配置GPU使用
```python
# 在代码中启用GPU
import faiss

# 检查GPU可用性
if faiss.get_num_gpus() > 0:
    print(f"✅ 检测到 {faiss.get_num_gpus()} 个GPU")
    # 使用GPU索引
    res = faiss.StandardGpuResources()
    index = faiss.index_cpu_to_gpu(res, 0, cpu_index)
else:
    print("⚠️ 未检测到GPU，使用CPU")
```

---

### 2. 多模型配置

```bash
# 拉取多个模型
ollama pull qwen2.5:7b
ollama pull qwen2.5:14b
ollama pull llama2:13b

# 在配置中指定模型
export OLLAMA_MODEL=qwen2.5:14b
```

---

### 3. 分布式部署

#### 主节点配置
```bash
# 主节点运行Ollama
ollama serve --host 0.0.0.0:11434

# 配置防火墙
sudo ufw allow 11434/tcp
```

#### 工作节点配置
```bash
# 工作节点连接到主节点
export OLLAMA_HOST=http://master-node:11434

# 测试连接
curl $OLLAMA_HOST/api/tags
```

---

### 4. 性能优化

#### 调整Ollama参数
```bash
# 设置并发数
export OLLAMA_NUM_PARALLEL=4

# 设置上下文长度
export OLLAMA_CONTEXT_LENGTH=4096

# 设置GPU层数
export OLLAMA_GPU_LAYERS=35
```

#### 调整Python参数
```python
# 在代码中配置
from langchain_ollama import ChatOllama

llm = ChatOllama(
    model="qwen2.5:7b",
    temperature=0.7,
    num_ctx=4096,        # 上下文长度
    num_gpu=35,          # GPU层数
    num_thread=8,        # 线程数
    repeat_penalty=1.1   # 重复惩罚
)
```

---

## 🐛 故障排查

### 问题1: Ollama连接失败

**症状**:
```
ConnectionError: Failed to connect to Ollama
```

**解决方案**:
```bash
# 检查Ollama是否运行
ps aux | grep ollama

# 如果没有运行，启动它
ollama serve &

# 检查端口
netstat -tlnp | grep 11434

# 测试连接
curl http://localhost:11434/api/tags
```

---

### 问题2: 模型未找到

**症状**:
```
Error: model 'qwen2.5' not found
```

**解决方案**:
```bash
# 查看已安装的模型
ollama list

# 拉取模型
ollama pull qwen2.5:7b

# 验证
ollama run qwen2.5:7b "测试"
```

---

### 问题3: 内存不足

**症状**:
```
MemoryError: Unable to allocate array
```

**解决方案**:
```bash
# 使用更小的模型
ollama pull qwen2.5:3b

# 或减少上下文长度
export OLLAMA_CONTEXT_LENGTH=2048

# 或增加swap
sudo fallocate -l 8G /swapfile
sudo chmod 600 /swapfile
sudo mkswap /swapfile
sudo swapon /swapfile
```

---

### 问题4: FAISS安装失败

**症状**:
```
ERROR: Could not build wheels for faiss-cpu
```

**解决方案**:
```bash
# 安装编译依赖
sudo apt install build-essential cmake

# 或使用conda安装
conda install -c conda-forge faiss-cpu

# 或使用预编译包
pip install faiss-cpu --no-cache-dir
```

---

## 📊 性能基准测试

### 运行基准测试
```bash
# 测试LLM响应时间
python -c "
from langchain_ollama import ChatOllama
import time

llm = ChatOllama(model='qwen2.5:7b')
start = time.time()
response = llm.invoke('你好')
elapsed = time.time() - start
print(f'响应时间: {elapsed:.2f}秒')
"

# 测试NPC系统响应时间
python extrator/llm/npc_system/npc_chat.py
# 输入几次对话后，使用 'stats' 命令查看统计
```

### 预期性能

| 配置 | LLM响应 | NPC系统响应 |
|------|---------|-------------|
| 最低配置 (4核/8GB) | 3-5秒 | 5-8秒 |
| 推荐配置 (8核/16GB) | 1-2秒 | 2-4秒 |
| 高性能 (16核/32GB+GPU) | 0.5-1秒 | 1-2秒 |

---

## 🔗 相关文档

- [01-快速入门](./01-QUICKSTART.md) - 快速上手指南
- [03-架构总览](./03-ARCHITECTURE_OVERVIEW.md) - 系统架构
- [17-性能优化](../PERFORMANCE_ANALYSIS.md) - 性能优化指南
- [19-FAQ](./19-FAQ.md) - 常见问题

---

## 📞 获取帮助

### 官方资源
- Ollama文档: https://ollama.com/docs
- LangChain文档: https://python.langchain.com/docs
- FAISS文档: https://github.com/facebookresearch/faiss

### 社区支持
- 提交Issue
- 查看Wiki
- 加入讨论组

---

恭喜！你已经完成了NPC智能体系统的安装配置！🎉

下一步：阅读 [01-快速入门](./01-QUICKSTART.md) 开始使用系统。
