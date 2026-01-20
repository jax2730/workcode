"""
NPCManager - 多NPC管理器
基于 HelloAgents 第15章设计

功能:
- 多NPC统一管理
- 批量对话生成 (轻负载模式)
- NPC状态同步
- 场景氛围生成
"""

import os
import json
import asyncio
from datetime import datetime
from typing import Dict, List, Any, Optional
from dataclasses import dataclass, field
from pathlib import Path


@dataclass
class NPCManagerConfig:
    """NPC管理器配置"""
    data_dir: str = "./npc_data"
    config_dir: str = "./npc_configs"
    batch_interval: int = 300           # 批量生成间隔(秒)
    max_npcs: int = 50                  # 最大NPC数量
    enable_batch_generation: bool = True


class NPCManager:
    """
    NPC管理器
    
    功能:
    - 统一管理多个NPC
    - 批量生成NPC对话 (轻负载模式)
    - NPC状态查询
    - 场景氛围生成
    
    批量生成模式:
    - 将多个NPC的对话请求合并为一次LLM调用
    - 适用于NPC背景对话、自言自语等
    - 大幅降低API调用成本
    """
    
    def __init__(self, config: NPCManagerConfig = None, llm=None):
        self.config = config or NPCManagerConfig()
        self.llm = llm
        
        # NPC实例缓存
        self.npcs: Dict[str, 'NPCAgent'] = {}
        
        # NPC配置缓存
        self.npc_configs: Dict[str, dict] = {}
        
        # 背景对话缓存
        self.background_dialogues: Dict[str, str] = {}
        self.last_batch_time: datetime = None
        
        # 创建目录
        os.makedirs(self.config.data_dir, exist_ok=True)
        os.makedirs(self.config.config_dir, exist_ok=True)
        
        # 加载NPC配置
        self._load_npc_configs()
        
        print(f"[NPCManager] 初始化完成，已加载 {len(self.npc_configs)} 个NPC配置")
    
    def _load_npc_configs(self):
        """加载所有NPC配置"""
        config_path = Path(self.config.config_dir)
        
        for json_file in config_path.glob("*.json"):
            try:
                with open(json_file, 'r', encoding='utf-8') as f:
                    config = json.load(f)
                
                npc_id = config.get("npc_id", json_file.stem)
                self.npc_configs[npc_id] = config
                print(f"[NPCManager] 加载NPC配置: {npc_id}")
            except Exception as e:
                print(f"[NPCManager] 加载配置失败 {json_file}: {e}")
    
    def get_npc(self, npc_id: str) -> Optional['NPCAgent']:
        """获取NPC实例 (懒加载)"""
        if npc_id in self.npcs:
            return self.npcs[npc_id]
        
        if npc_id not in self.npc_configs:
            print(f"[NPCManager] NPC不存在: {npc_id}")
            return None
        
        # 懒加载NPC
        try:
            from .npc_agent import load_npc_from_dict
            
            npc = load_npc_from_dict(
                self.npc_configs[npc_id],
                llm=self.llm,
                data_dir=self.config.data_dir
            )
            self.npcs[npc_id] = npc
            print(f"[NPCManager] 加载NPC实例: {npc_id}")
            return npc
        except Exception as e:
            print(f"[NPCManager] 加载NPC失败 {npc_id}: {e}")
            return None
    
    def register_npc(self, npc_id: str, config: dict, save: bool = True) -> bool:
        """注册新NPC"""
        if len(self.npc_configs) >= self.config.max_npcs:
            print(f"[NPCManager] NPC数量已达上限: {self.config.max_npcs}")
            return False
        
        config["npc_id"] = npc_id
        self.npc_configs[npc_id] = config
        
        if save:
            config_path = Path(self.config.config_dir) / f"{npc_id}.json"
            with open(config_path, 'w', encoding='utf-8') as f:
                json.dump(config, f, ensure_ascii=False, indent=2)
        
        print(f"[NPCManager] 注册NPC: {npc_id}")
        return True
    
    def unregister_npc(self, npc_id: str) -> bool:
        """注销NPC"""
        if npc_id in self.npcs:
            del self.npcs[npc_id]
        
        if npc_id in self.npc_configs:
            del self.npc_configs[npc_id]
            
            # 删除配置文件
            config_path = Path(self.config.config_dir) / f"{npc_id}.json"
            if config_path.exists():
                config_path.unlink()
            
            print(f"[NPCManager] 注销NPC: {npc_id}")
            return True
        
        return False
    
    def chat(self, npc_id: str, player_id: str, message: str,
             session_id: str = None, extra_context: Dict = None) -> Dict[str, Any]:
        """与指定NPC对话"""
        npc = self.get_npc(npc_id)
        if not npc:
            return {
                "success": False,
                "error": f"NPC不存在: {npc_id}"
            }
        
        try:
            result = npc.chat(player_id, message, session_id, extra_context)
            result["success"] = True
            return result
        except Exception as e:
            return {
                "success": False,
                "error": str(e)
            }
    
    def get_all_status(self) -> Dict[str, Any]:
        """获取所有NPC状态"""
        status = {
            "total_configs": len(self.npc_configs),
            "loaded_npcs": len(self.npcs),
            "npcs": {}
        }
        
        for npc_id, config in self.npc_configs.items():
            npc_status = {
                "name": config.get("personality", config).get("name", npc_id),
                "role": config.get("personality", config).get("role", ""),
                "loaded": npc_id in self.npcs,
                "background_dialogue": self.background_dialogues.get(npc_id, "")
            }
            
            if npc_id in self.npcs:
                npc = self.npcs[npc_id]
                npc_status["memory_summary"] = npc.memory.execute("summary")
            
            status["npcs"][npc_id] = npc_status
        
        return status
    
    def get_npc_status(self, npc_id: str, player_id: str = None) -> Dict[str, Any]:
        """获取单个NPC状态"""
        npc = self.get_npc(npc_id)
        if not npc:
            return {"error": f"NPC不存在: {npc_id}"}
        
        return npc.get_status(player_id)
    
    # ==================== 批量对话生成 ====================
    
    def generate_batch_dialogues(self, context: str = None) -> Dict[str, str]:
        """
        批量生成所有NPC的背景对话
        
        Args:
            context: 场景上下文 (如"上午工作时间"、"午餐时间"等)
            
        Returns:
            Dict[str, str]: NPC名称到对话内容的映射
        """
        if not self.llm:
            return {}
        
        if not self.npc_configs:
            return {}
        
        # 自动推断场景
        if context is None:
            context = self._get_current_context()
        
        # 构建批量生成提示词
        prompt = self._build_batch_prompt(context)
        
        try:
            response = self.llm.invoke([
                {"role": "system", "content": "你是一个游戏NPC对话生成器，擅长创作自然真实的对话。"},
                {"role": "user", "content": prompt}
            ])
            
            if hasattr(response, 'content'):
                response = response.content
            
            # 解析JSON响应
            dialogues = self._parse_batch_response(str(response))
            
            # 更新缓存
            self.background_dialogues.update(dialogues)
            self.last_batch_time = datetime.now()
            
            print(f"[NPCManager] 批量生成完成: {len(dialogues)} 个NPC")
            return dialogues
            
        except Exception as e:
            print(f"[NPCManager] 批量生成失败: {e}")
            return {}
    
    def _get_current_context(self) -> str:
        """根据时间自动推断场景"""
        hour = datetime.now().hour
        
        if 6 <= hour < 9:
            return "清晨，大家刚开始一天的工作"
        elif 9 <= hour < 12:
            return "上午工作时间，大家都在忙碌"
        elif 12 <= hour < 14:
            return "午餐时间，大家在休息或吃饭"
        elif 14 <= hour < 18:
            return "下午工作时间，大家继续工作"
        elif 18 <= hour < 21:
            return "傍晚，大家准备下班或已经下班"
        else:
            return "夜晚，大部分人已经休息"
    
    def _build_batch_prompt(self, context: str) -> str:
        """构建批量生成提示词"""
        # 构建NPC描述
        npc_descriptions = []
        npc_names = []
        
        for npc_id, config in self.npc_configs.items():
            personality = config.get("personality", config)
            name = personality.get("name", npc_id)
            role = personality.get("role", "角色")
            traits = personality.get("traits", [])
            
            npc_names.append(name)
            traits_str = "、".join(traits) if traits else "普通"
            desc = f"- {name}({role}): 性格{traits_str}"
            npc_descriptions.append(desc)
        
        npc_desc_text = "\n".join(npc_descriptions)
        names_json = json.dumps({name: "..." for name in npc_names}, ensure_ascii=False)
        
        prompt = f"""请为以下NPC生成当前的对话或行为描述。

【场景】{context}

【NPC信息】
{npc_desc_text}

【生成要求】
1. 每个NPC生成1句话(20-40字)
2. 内容要符合角色设定和场景氛围
3. 可以是自言自语、工作状态描述、或简单的思考
4. 要自然真实，像真实的人物
5. **必须严格按照JSON格式返回**

【输出格式】(严格遵守)
{names_json}

请生成(只返回JSON，不要其他内容):
"""
        return prompt
    
    def _parse_batch_response(self, response: str) -> Dict[str, str]:
        """解析批量生成响应"""
        # 尝试提取JSON
        import re
        
        # 查找JSON对象
        json_match = re.search(r'\{[^{}]*\}', response, re.DOTALL)
        if json_match:
            try:
                return json.loads(json_match.group())
            except json.JSONDecodeError:
                pass
        
        # 尝试直接解析
        try:
            return json.loads(response)
        except json.JSONDecodeError:
            pass
        
        # 解析失败，返回空
        print(f"[NPCManager] JSON解析失败: {response[:200]}")
        return {}
    
    def get_background_dialogue(self, npc_id: str) -> str:
        """获取NPC的背景对话"""
        # 检查是否需要更新
        if self.config.enable_batch_generation:
            if (self.last_batch_time is None or 
                (datetime.now() - self.last_batch_time).seconds > self.config.batch_interval):
                self.generate_batch_dialogues()
        
        # 查找NPC名称
        if npc_id in self.npc_configs:
            name = self.npc_configs[npc_id].get("personality", {}).get("name", npc_id)
            return self.background_dialogues.get(name, "")
        
        return self.background_dialogues.get(npc_id, "")
    
    # ==================== 场景氛围生成 ====================
    
    def generate_scene_atmosphere(self, scene_description: str,
                                   npc_ids: List[str] = None) -> Dict[str, Any]:
        """
        生成场景氛围
        
        Args:
            scene_description: 场景描述
            npc_ids: 参与的NPC列表 (None表示所有)
            
        Returns:
            Dict: {
                "atmosphere": str,      # 场景氛围描述
                "npc_states": dict,     # 各NPC状态
                "interactions": list    # NPC之间的互动
            }
        """
        if not self.llm:
            return {"error": "LLM未配置"}
        
        # 选择NPC
        if npc_ids is None:
            npc_ids = list(self.npc_configs.keys())
        
        # 构建NPC信息
        npc_info = []
        for npc_id in npc_ids:
            if npc_id in self.npc_configs:
                config = self.npc_configs[npc_id]
                personality = config.get("personality", config)
                npc_info.append({
                    "id": npc_id,
                    "name": personality.get("name", npc_id),
                    "role": personality.get("role", ""),
                    "traits": personality.get("traits", [])
                })
        
        prompt = f"""请为以下场景生成氛围描述和NPC状态。

【场景】
{scene_description}

【参与NPC】
{json.dumps(npc_info, ensure_ascii=False, indent=2)}

请生成JSON格式的响应:
{{
    "atmosphere": "场景整体氛围描述(50-100字)",
    "npc_states": {{
        "NPC名称": "该NPC当前的状态和行为(20-30字)"
    }},
    "interactions": [
        {{"from": "NPC1", "to": "NPC2", "action": "互动描述"}}
    ]
}}

只返回JSON:"""
        
        try:
            response = self.llm.invoke([{"role": "user", "content": prompt}])
            if hasattr(response, 'content'):
                response = response.content
            
            # 解析响应
            result = self._parse_batch_response(str(response))
            return result if result else {"error": "解析失败"}
            
        except Exception as e:
            return {"error": str(e)}
    
    # ==================== 异步支持 ====================
    
    async def async_chat(self, npc_id: str, player_id: str, message: str,
                         session_id: str = None, extra_context: Dict = None) -> Dict[str, Any]:
        """异步对话"""
        # 在线程池中执行同步方法
        loop = asyncio.get_event_loop()
        return await loop.run_in_executor(
            None,
            lambda: self.chat(npc_id, player_id, message, session_id, extra_context)
        )
    
    async def async_generate_batch_dialogues(self, context: str = None) -> Dict[str, str]:
        """异步批量生成"""
        loop = asyncio.get_event_loop()
        return await loop.run_in_executor(
            None,
            lambda: self.generate_batch_dialogues(context)
        )
    
    # ==================== 后台任务 ====================
    
    async def background_dialogue_updater(self, interval: int = None):
        """后台任务: 定期更新NPC背景对话"""
        interval = interval or self.config.batch_interval
        
        while True:
            try:
                if self.config.enable_batch_generation:
                    await self.async_generate_batch_dialogues()
                    print(f"[NPCManager] 背景对话更新完成")
            except Exception as e:
                print(f"[NPCManager] 背景对话更新失败: {e}")
            
            await asyncio.sleep(interval)


# ==================== 便捷函数 ====================

def create_npc_manager(data_dir: str = "./npc_data",
                       config_dir: str = "./npc_configs",
                       llm=None) -> NPCManager:
    """创建NPC管理器"""
    config = NPCManagerConfig(
        data_dir=data_dir,
        config_dir=config_dir
    )
    return NPCManager(config, llm)


# ==================== 预定义NPC模板 ====================

NPC_TEMPLATES = {
    "blacksmith": {
        "npc_id": "blacksmith",
        "personality": {
            "name": "老铁匠",
            "role": "铁匠",
            "age": 55,
            "gender": "男",
            "traits": ["严肃", "专业", "热心"],
            "background": "在这个镇上打铁30年，见证了无数冒险者的成长。",
            "speech_style": "说话干脆利落，常用专业术语",
            "knowledge": ["武器锻造", "金属材料", "镇上历史"],
            "secrets": ["知道镇长年轻时的秘密", "藏有传说中的锻造图纸"],
            "greeting": "哟，又来了个冒险者。需要打造什么武器？"
        }
    },
    "merchant": {
        "npc_id": "merchant",
        "personality": {
            "name": "精明商人",
            "role": "商人",
            "age": 40,
            "gender": "男",
            "traits": ["精明", "健谈", "贪财"],
            "background": "走南闯北的商人，什么都卖，什么都买。",
            "speech_style": "说话圆滑，喜欢讨价还价",
            "knowledge": ["商品价格", "各地风土", "交易技巧"],
            "secrets": ["知道黑市的位置", "有走私渠道"],
            "greeting": "欢迎欢迎！看看有什么需要的？保证全镇最低价！"
        }
    },
    "innkeeper": {
        "npc_id": "innkeeper",
        "personality": {
            "name": "旅店老板娘",
            "role": "旅店老板",
            "age": 35,
            "gender": "女",
            "traits": ["热情", "八卦", "善良"],
            "background": "经营这家旅店十年，是镇上的消息中心。",
            "speech_style": "说话热情，喜欢打听八卦",
            "knowledge": ["镇上八卦", "旅客信息", "烹饪"],
            "secrets": ["知道很多旅客的秘密", "暗中帮助过逃犯"],
            "greeting": "哎呀，来客人了！要住店还是吃饭？"
        }
    },
    "guard": {
        "npc_id": "guard",
        "personality": {
            "name": "守卫队长",
            "role": "守卫",
            "age": 45,
            "gender": "男",
            "traits": ["正直", "严肃", "忠诚"],
            "background": "镇上守卫队的队长，维护治安二十年。",
            "speech_style": "说话严肃，公事公办",
            "knowledge": ["镇上治安", "周边地形", "战斗技巧"],
            "secrets": ["知道镇外有强盗窝点", "曾经是佣兵"],
            "greeting": "站住！有什么事？"
        }
    }
}


def create_template_npc(template_name: str, llm=None, data_dir: str = "./npc_data") -> 'NPCAgent':
    """从模板创建NPC"""
    if template_name not in NPC_TEMPLATES:
        raise ValueError(f"未知模板: {template_name}, 可用模板: {list(NPC_TEMPLATES.keys())}")
    
    from .npc_agent import load_npc_from_dict
    return load_npc_from_dict(NPC_TEMPLATES[template_name], llm, data_dir)
