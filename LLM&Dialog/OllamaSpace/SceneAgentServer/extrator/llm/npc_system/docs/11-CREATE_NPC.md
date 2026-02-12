# 11 - 创建NPC实用指南

> **面向对象**: 游戏开发者、内容创作者  
> **难度**: ⭐⭐☆☆☆ (中等)  
> **前置知识**: Python基础、NPC系统架构

## 📋 概述

本文档提供**从零开始创建NPC**的完整指南，包括：
- NPC人设设计
- 配置文件编写
- 知识库准备
- 初始记忆设置
- 测试和调优

---

## 🎯 创建流程

### 完整流程图

```
1. 设计NPC人设
   ↓
2. 创建配置文件
   ↓
3. 准备知识库
   ↓
4. 设置初始记忆
   ↓
5. 初始化NPC
   ↓
6. 测试对话
   ↓
7. 调优优化
```

---

## 📝 步骤1: 设计NPC人设

### 人设要素

一个完整的NPC人设包含以下要素：

| 要素 | 说明 | 示例 |
|------|------|------|
| **基础信息** | 姓名、角色、年龄、性别 | 老铁匠、55岁、男 |
| **性格特征** | 3-5个关键词 | 严肃、专业、热心 |
| **背景故事** | 200-500字 | 在村里打铁30年... |
| **说话风格** | 语言特点 | 简洁直接、带方言 |
| **知识领域** | 擅长的领域 | 锻造、武器、盔甲 |
| **秘密** | 分层秘密 | 5个等级的秘密 |
| **问候语** | 初次见面 | "需要打造什么吗？" |

### 设计模板

```python
# npc_configs/blacksmith.py

from dataclasses import dataclass
from typing import List

@dataclass
class BlacksmithPersonality:
    """老铁匠人设"""
    
    # 基础信息
    name: str = "老铁匠"
    role: str = "铁匠"
    age: int = 55
    gender: str = "男"
    
    # 性格特征
    traits: List[str] = [
        "严肃认真",
        "技艺精湛", 
        "热心助人",
        "固执己见",
        "怀旧念旧"
    ]
    
    # 背景故事
    background: str = """
    我叫老铁匠，今年55岁了。从25岁开始在这个村子里打铁，
    一晃就是30年。我的手艺是从父亲那里学来的，他是这一带
    最好的铁匠。年轻时我也曾想过去大城市闯荡，但最终还是
    选择留在村里，继承父亲的铁匠铺。
    
    这些年来，我打造过无数的农具、武器和盔甲。村里的年轻人
    出门冒险，用的都是我打的剑。每次看到他们平安归来，我就
    觉得自己的工作很有意义。
    
    虽然现在年纪大了，但我的手艺还没有退步。只要炉火还在烧，
    我就会一直打下去。
    """
    
    # 说话风格
    speech_style: str = """
    - 说话简洁直接，不拐弯抹角
    - 偶尔带点方言口音
    - 谈到打铁时会变得滔滔不绝
    - 对年轻人有点严厉但关心
    - 喜欢用打铁的比喻
    """
    
    # 知识领域
    knowledge: List[str] = [
        "锻造技术",
        "武器制作",
        "盔甲修理",
        "金属材料",
        "火候控制",
        "村子历史"
    ]
    
    # 分层秘密 (5个等级)
    secrets: List[str] = [
        # 等级0 - 陌生: 无秘密
        "",
        
        # 等级1 - 认识: 小秘密
        "其实我年轻时也想过当冒险者，但父亲不同意。",
        
        # 等级2 - 友好: 个人秘密
        "我有一把自己打造的精钢剑，从来没卖过，一直藏在铁匠铺后面。",
        
        # 等级3 - 信任: 重要秘密
        "30年前，我曾经帮一个神秘旅人修理过一把魔法剑，他给了我一张藏宝图。",
        
        # 等级4 - 挚友: 核心秘密
        "那张藏宝图指向村子北边的古老矿洞，据说里面有传说中的秘银矿脉。我一直想去探索，但年纪大了，需要一个信得过的伙伴。"
    ]
    
    # 问候语
    greeting: str = "需要打造什么吗？"
    
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

### 设计技巧

**1. 性格特征要具体**
- ❌ 不好: "善良"
- ✅ 好: "对年轻冒险者特别关照，经常免费修理他们的装备"

**2. 背景故事要有细节**
- ❌ 不好: "他是个铁匠"
- ✅ 好: "从父亲那里继承了铁匠铺，30年来打造了无数武器"

**3. 秘密要分层递进**
```
陌生 → 认识 → 友好 → 信任 → 挚友
无    小秘密  个人秘密  重要秘密  核心秘密
```

**4. 说话风格要一致**
```python
# 老铁匠的说话风格
"嗯，这把剑确实不错。"  # ✅ 简洁
"哎呀！这把剑真是太棒了呢！" # ❌ 不符合人设
```

---

## ⚙️ 步骤2: 创建配置文件

### 配置文件结构

```python
# npc_configs/blacksmith_config.py

from npc_system.npc_agent import NPCConfig, NPCPersonality
from .blacksmith import BlacksmithPersonality

def create_blacksmith_config() -> NPCConfig:
    """创建老铁匠配置"""
    
    # 人设
    personality = NPCPersonality(
        name=BlacksmithPersonality.name,
        role=BlacksmithPersonality.role,
        age=BlacksmithPersonality.age,
        gender=BlacksmithPersonality.gender,
        traits=BlacksmithPersonality.traits,
        background=BlacksmithPersonality.background,
        speech_style=BlacksmithPersonality.speech_style,
        knowledge=BlacksmithPersonality.knowledge,
        secrets=BlacksmithPersonality.secrets,
        greeting=BlacksmithPersonality.greeting
    )
    
    # 配置
    config = NPCConfig(
        npc_id="blacksmith_001",
        personality=personality,
        
        # 数据目录
        data_dir="./npc_data",
        
        # 记忆配置
        memory_config={
            "working_memory_size": 10,
            "episodic_memory_limit": 100,
            "semantic_memory_limit": 50,
            "importance_threshold": 0.5
        },
        
        # RAG配置
        enable_rag=True,
        rag_config={
            "chunk_size": 500,
            "chunk_overlap": 50,
            "top_k": 3,
            "similarity_threshold": 0.7
        },
        
        # 上下文配置
        context_config={
            "max_tokens": 3000,
            "memory_weight": 0.3,
            "knowledge_weight": 0.3,
            "affinity_weight": 0.2,
            "history_weight": 0.2
        },
        max_context_tokens=3000,
        
        # 对话配置
        enable_dialogue_storage=True,
        enable_file_memory=True,
        
        # LLM配置
        llm_model="qwen2.5:7b",
        llm_temperature=0.7
    )
    
    return config
```

### 配置参数说明

#### 记忆配置
```python
memory_config={
    "working_memory_size": 10,      # 工作记忆容量 (最近N条对话)
    "episodic_memory_limit": 100,   # 情景记忆上限
    "semantic_memory_limit": 50,    # 语义记忆上限
    "importance_threshold": 0.5     # 重要性阈值 (低于此值会被遗忘)
}
```

#### RAG配置
```python
rag_config={
    "chunk_size": 500,              # 文档分块大小
    "chunk_overlap": 50,            # 分块重叠大小
    "top_k": 3,                     # 检索返回数量
    "similarity_threshold": 0.7     # 相似度阈值
}
```

#### 上下文配置
```python
context_config={
    "max_tokens": 3000,             # 最大token数
    "memory_weight": 0.3,           # 记忆权重
    "knowledge_weight": 0.3,        # 知识权重
    "affinity_weight": 0.2,         # 好感度权重
    "history_weight": 0.2           # 历史对话权重
}
```

---

## 📚 步骤3: 准备知识库

### 知识库目录结构

```
npc_data/
└── knowledge_base/
    └── blacksmith_001/
        ├── recipes.txt          # 配方知识
        ├── materials.txt        # 材料知识
        ├── techniques.txt       # 技术知识
        └── village_history.txt  # 村子历史
```

### 知识文档示例

#### recipes.txt - 配方知识
```
# 武器配方

## 铁剑
材料: 铁锭x3, 木棍x1
难度: 简单
制作时间: 2小时
特点: 基础武器，适合新手冒险者

## 精钢剑
材料: 精钢锭x3, 皮革x1, 宝石x1
难度: 中等
制作时间: 5小时
特点: 锋利耐用，是冒险者的首选

## 秘银剑
材料: 秘银锭x5, 龙鳞x2, 魔法水晶x1
难度: 困难
制作时间: 3天
特点: 传说级武器，拥有魔法属性

---

## 盔甲配方

## 铁甲
材料: 铁锭x8, 皮革x4
难度: 简单
制作时间: 4小时
特点: 基础防护

## 精钢甲
材料: 精钢锭x12, 皮革x6, 布料x4
难度: 中等
制作时间: 8小时
特点: 优秀的防护性能
```

#### materials.txt - 材料知识
```
# 金属材料

## 铁
来源: 铁矿石冶炼
特点: 最常见的金属，容易加工
用途: 制作基础武器和工具

## 精钢
来源: 铁+碳，高温锻造
特点: 比铁更硬更锋利
用途: 制作高级武器和盔甲

## 秘银
来源: 古老矿洞深处
特点: 轻盈、坚固、可附魔
用途: 制作传说级装备

---

# 辅助材料

## 皮革
来源: 动物皮毛鞣制
用途: 剑柄、盔甲内衬

## 宝石
来源: 矿洞、商人
用途: 武器镶嵌，增加属性

## 魔法水晶
来源: 魔法生物、遗迹
用途: 附魔，赋予魔法属性
```

#### techniques.txt - 技术知识
```
# 锻造技术

## 基础锻造
1. 加热金属至红热状态
2. 用锤子反复敲打塑形
3. 淬火冷却定型
4. 打磨抛光

## 折叠锻造
1. 将金属加热至白热
2. 对折锻打
3. 重复100次以上
4. 可大幅提升强度和韧性

## 附魔锻造
1. 在锻造过程中加入魔法水晶
2. 念诵附魔咒语
3. 用魔法火焰加热
4. 需要魔法师协助

---

# 火候控制

## 红热 (700-800°C)
适合: 基础塑形

## 橙热 (900-1000°C)
适合: 精细锻造

## 白热 (1200°C+)
适合: 折叠锻造、附魔
```

### 添加知识到NPC

```python
from npc_system.npc_agent import NPCAgent
from langchain_community.chat_models import ChatOllama

# 创建NPC
config = create_blacksmith_config()
llm = ChatOllama(model=config.llm_model)
npc = NPCAgent(
    npc_id=config.npc_id,
    personality=config.personality,
    llm=llm,
    data_dir=config.data_dir,
    config=config
)

# 添加知识
knowledge_files = [
    "npc_data/knowledge_base/blacksmith_001/recipes.txt",
    "npc_data/knowledge_base/blacksmith_001/materials.txt",
    "npc_data/knowledge_base/blacksmith_001/techniques.txt",
    "npc_data/knowledge_base/blacksmith_001/village_history.txt"
]

for file_path in knowledge_files:
    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()
        npc.add_knowledge(
            content=content,
            doc_id=file_path,
            metadata={"source": file_path}
        )
        print(f"已添加知识: {file_path}")
```

---

## 🧠 步骤4: 设置初始记忆

### 初始记忆类型

```python
def setup_initial_memories(npc: NPCAgent):
    """设置初始记忆"""
    
    # 1. 语义记忆 (长期知识)
    semantic_memories = [
        "我从父亲那里学会了锻造技术",
        "村子里的年轻冒险者都用我打的武器",
        "精钢剑是我最拿手的作品",
        "我知道村子北边有个古老的矿洞",
        "30年前我见过一把魔法剑"
    ]
    
    for memory in semantic_memories:
        npc.remember(
            content=memory,
            memory_type="semantic",
            importance=0.8
        )
    
    # 2. 情景记忆 (具体事件)
    episodic_memories = [
        "上周帮村长的儿子打造了一把铁剑",
        "昨天有个商人来买了三套盔甲",
        "前天炉子坏了，修了一整天"
    ]
    
    for memory in episodic_memories:
        npc.remember(
            content=memory,
            memory_type="episodic",
            importance=0.6
        )
    
    # 3. 感知记忆 (当前状态)
    perceptual_memories = [
        "现在是下午，铁匠铺里很热",
        "炉火正旺，可以开始工作",
        "工作台上放着几块铁锭"
    ]
    
    for memory in perceptual_memories:
        npc.remember(
            content=memory,
            memory_type="perceptual",
            importance=0.4
        )
    
    print("初始记忆设置完成！")
```

---

## 🚀 步骤5: 初始化NPC

### 完整初始化脚本

```python
# create_blacksmith.py

from npc_system.npc_agent import NPCAgent
from langchain_community.chat_models import ChatOllama
from npc_configs.blacksmith_config import create_blacksmith_config
import os

def create_blacksmith():
    """创建老铁匠NPC"""
    
    print("=" * 50)
    print("开始创建老铁匠NPC")
    print("=" * 50)
    
    # 1. 创建配置
    print("\n[1/5] 创建配置...")
    config = create_blacksmith_config()
    print(f"✓ NPC ID: {config.npc_id}")
    print(f"✓ NPC名称: {config.personality.name}")
    
    # 2. 初始化LLM
    print("\n[2/5] 初始化LLM...")
    llm = ChatOllama(
        model=config.llm_model,
        temperature=config.llm_temperature
    )
    print(f"✓ 模型: {config.llm_model}")
    
    # 3. 创建NPC
    print("\n[3/5] 创建NPC...")
    npc = NPCAgent(
        npc_id=config.npc_id,
        personality=config.personality,
        llm=llm,
        data_dir=config.data_dir,
        config=config
    )
    print("✓ NPC创建成功")
    
    # 4. 添加知识
    print("\n[4/5] 添加知识库...")
    knowledge_dir = f"{config.data_dir}/knowledge_base/{config.npc_id}"
    if os.path.exists(knowledge_dir):
        for filename in os.listdir(knowledge_dir):
            if filename.endswith('.txt'):
                file_path = os.path.join(knowledge_dir, filename)
                with open(file_path, 'r', encoding='utf-8') as f:
                    content = f.read()
                    npc.add_knowledge(content=content, doc_id=filename)
                    print(f"  ✓ {filename}")
    else:
        print("  ! 知识库目录不存在，跳过")
    
    # 5. 设置初始记忆
    print("\n[5/5] 设置初始记忆...")
    setup_initial_memories(npc)
    print("  ✓ 初始记忆设置完成")
    
    print("\n" + "=" * 50)
    print("老铁匠NPC创建完成！")
    print("=" * 50)
    
    return npc


if __name__ == "__main__":
    npc = create_blacksmith()
    
    # 测试对话
    print("\n测试对话:")
    result = npc.chat(
        player_id="test_player",
        message="你好，我想打造一把剑"
    )
    print(f"\n{npc.personality.name}: {result['reply']}")
```

---

## 🧪 步骤6: 测试对话

### 测试脚本

```python
# test_blacksmith.py

def test_npc_dialogue(npc: NPCAgent):
    """测试NPC对话"""
    
    test_cases = [
        # 基础对话
        {
            "message": "你好",
            "expected_keywords": ["你好", "需要", "打造"]
        },
        
        # 知识查询
        {
            "message": "精钢剑需要什么材料？",
            "expected_keywords": ["精钢锭", "皮革", "宝石"]
        },
        
        # 背景故事
        {
            "message": "你做铁匠多久了？",
            "expected_keywords": ["30年", "父亲", "村子"]
        },
        
        # 情感对话
        {
            "message": "你的手艺真棒！",
            "expected_keywords": ["谢谢", "经验"]
        }
    ]
    
    player_id = "test_player"
    
    print("\n" + "=" * 50)
    print("开始测试对话")
    print("=" * 50)
    
    for i, test in enumerate(test_cases, 1):
        print(f"\n测试 {i}/{len(test_cases)}")
        print(f"玩家: {test['message']}")
        
        result = npc.chat(player_id=player_id, message=test['message'])
        reply = result['reply']
        
        print(f"{npc.personality.name}: {reply}")
        
        # 检查关键词
        found_keywords = []
        for keyword in test['expected_keywords']:
            if keyword in reply:
                found_keywords.append(keyword)
        
        if found_keywords:
            print(f"✓ 包含关键词: {', '.join(found_keywords)}")
        else:
            print(f"! 未找到预期关键词: {', '.join(test['expected_keywords'])}")
        
        print(f"好感度: {result['affinity']['level']} ({result['affinity']['score']})")
    
    print("\n" + "=" * 50)
    print("测试完成")
    print("=" * 50)


if __name__ == "__main__":
    npc = create_blacksmith()
    test_npc_dialogue(npc)
```

---

## 🔧 步骤7: 调优优化

### 常见问题和解决方案

#### 问题1: NPC回复不符合人设

**症状**: NPC说话风格不一致

**解决方案**:
```python
# 强化人设描述
personality.speech_style = """
你必须严格遵守以下说话风格:
1. 使用简短的句子 (10-20字)
2. 不使用"呢"、"哦"等语气词
3. 多用"嗯"、"好"等简洁回应
4. 谈到打铁时可以多说几句

示例:
- 好的，需要什么？
- 嗯，这把剑不错。
- 打铁讲究火候，火候不对，剑就废了。
"""
```

#### 问题2: NPC不使用知识库

**症状**: 回答问题时不引用知识

**解决方案**:
```python
# 调整RAG配置
rag_config={
    "top_k": 5,  # 增加检索数量
    "similarity_threshold": 0.6  # 降低阈值
}

# 在上下文中强调知识
context_config={
    "knowledge_weight": 0.4,  # 提高知识权重
    "memory_weight": 0.2
}
```

#### 问题3: 好感度变化太快/太慢

**症状**: 好感度不合理

**解决方案**:
```python
# 自定义好感度计算
def _update_affinity(self, player_id, message, reply):
    delta = 0
    
    # 更细致的情感分析
    if "谢谢" in message or "感谢" in message:
        delta += 3
    if "帮助" in message or "请教" in message:
        delta += 2
    if "讨厌" in message or "烦" in message:
        delta -= 5
    
    # 对话质量奖励
    if len(message) > 30:  # 长对话
        delta += 2
    if "?" in message or "？" in message:  # 提问
        delta += 1
    
    return self.relationship.update_affinity(
        npc_id=self.npc_id,
        player_id=player_id,
        delta=delta
    )
```

#### 问题4: 回复速度太慢

**症状**: 响应时间超过5秒

**解决方案**:
```python
# 1. 减少上下文长度
context_config={
    "max_tokens": 2000  # 从3000降到2000
}

# 2. 减少检索数量
rag_config={
    "top_k": 2  # 从3降到2
}

memory_config={
    "working_memory_size": 5  # 从10降到5
}

# 3. 使用更小的模型
llm_model="qwen2.5:3b"  # 从7b降到3b
```

---

## 📊 完整示例

### 创建一个商人NPC

```python
# merchant_personality.py

merchant_personality = NPCPersonality(
    name="精明商人",
    role="商人",
    age=40,
    gender="男",
    traits=["精明", "健谈", "贪财", "消息灵通"],
    background="""
    我是个走南闯北的商人，在各个城镇之间贩卖货物。
    我知道很多消息，也认识很多人。只要价钱合适，
    什么都能帮你搞到。
    """,
    speech_style="""
    - 说话圆滑，喜欢讨价还价
    - 经常说"这个价格很公道"
    - 喜欢打听消息
    - 对钱很敏感
    """,
    knowledge=["商品价格", "贸易路线", "各地消息", "讨价还价"],
    secrets=[
        "",
        "我知道哪里能买到便宜货",
        "我认识城里的贵族",
        "我知道一条走私路线",
        "我手里有一张藏宝图，但需要资金去探索"
    ],
    greeting="来看看吧，都是好货！"
)
```

---

## 📝 总结

创建NPC的7个步骤：

1. ✅ **设计人设** - 详细、具体、有层次
2. ✅ **创建配置** - 合理的参数设置
3. ✅ **准备知识** - 丰富的知识库
4. ✅ **设置记忆** - 初始记忆铺垫
5. ✅ **初始化** - 完整的创建流程
6. ✅ **测试** - 全面的对话测试
7. ✅ **调优** - 根据测试结果优化

**下一步**: 阅读 `12-CONFIG_FILES.md` 了解配置文件详情
