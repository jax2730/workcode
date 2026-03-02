"""
Persona API - 人设管理接口

提供 RESTful API 供客户端管理 NPC 人设配置。

API 端点:
    GET    /api/persona/              - 列出所有人设
    GET    /api/persona/<id>/         - 获取指定人设
    POST   /api/persona/              - 创建新人设
    PUT    /api/persona/<id>/         - 更新人设
    DELETE /api/persona/<id>/         - 删除人设

请求/响应格式: JSON
"""

import os
import json
import re
import secrets
from pathlib import Path
from datetime import datetime
from typing import Optional, Dict, Any, List
from dataclasses import dataclass, asdict, field


# ==================== 配置 ====================

def get_config_dir() -> Path:
    """获取人设配置目录"""
    return Path(__file__).parent.parent.parent / "npc_configs"


# ==================== 数据模型 ====================

@dataclass
class PersonaPersonality:
    """人设性格配置"""
    name: str                           # 名称 (必填)
    role: str                           # 角色/职业 (必填)
    traits: List[str] = field(default_factory=list)        # 性格特点
    background: str = ""                # 背景故事
    speech_style: str = ""              # 说话风格
    knowledge: List[str] = field(default_factory=list)     # 擅长领域
    greeting: str = ""                  # 问候语


@dataclass
class PersonaConfig:
    """完整人设配置"""
    npc_id: str                         # 人设ID (必填, 唯一标识)
    personality: PersonaPersonality     # 性格配置
    planner_id: str = ""                # 策划ID (创建者标识)
    role_type: str = ""                 # 角色类型
    version: str = "1.0"                # 配置版本
    created_at: str = ""                # 创建时间
    updated_at: str = ""                # 更新时间
    
    def to_dict(self) -> Dict[str, Any]:
        """转换为字典"""
        result = {
            "npc_id": self.npc_id,
            "personality": asdict(self.personality),
            "version": self.version,
            "created_at": self.created_at,
            "updated_at": self.updated_at
        }
        if self.planner_id:
            result["planner_id"] = self.planner_id
        if self.role_type:
            result["role_type"] = self.role_type
        return result


# ==================== 验证器 ====================

class PersonaValidator:
    """人设配置验证器"""
    
    # ID 格式: 字母、数字、下划线、中划线，2-50字符（允许数字开头）
    ID_PATTERN = re.compile(r'^[a-zA-Z0-9][a-zA-Z0-9_-]{1,49}$')
    
    # Planner ID 格式: 字母、数字、下划线，2-20字符（允许数字开头）
    PLANNER_ID_PATTERN = re.compile(r'^[a-zA-Z0-9][a-zA-Z0-9_]{1,19}$')
    
    # Role Type 格式: 字母、数字、下划线、中划线，2-50字符（允许数字开头）
    ROLE_TYPE_PATTERN = re.compile(r'^[a-zA-Z0-9][a-zA-Z0-9_-]{1,49}$')
    
    # 保留ID (不可使用)
    RESERVED_IDS = {'admin', 'system', 'default', 'api', 'null', 'undefined'}
    
    @classmethod
    def validate_id(cls, persona_id: str) -> tuple[bool, str]:
        """验证人设ID"""
        if not persona_id:
            return False, "人设ID不能为空"
        
        if persona_id.lower() in cls.RESERVED_IDS:
            return False, f"'{persona_id}' 是保留ID，不可使用"
        
        if not cls.ID_PATTERN.match(persona_id):
            return False, "ID格式错误: 仅含字母/数字/下划线/中划线，2-50字符"
        
        return True, ""
    
    @classmethod
    def validate_planner_id(cls, planner_id: str) -> tuple[bool, str]:
        """验证策划ID"""
        if not planner_id:
            return False, "策划ID (planner_id) 不能为空"
        
        if not cls.PLANNER_ID_PATTERN.match(planner_id):
            return False, "策划ID格式错误: 仅含字母/数字/下划线，2-20字符"
        
        return True, ""
    
    @classmethod
    def validate_role_type(cls, role_type: str) -> tuple[bool, str]:
        """验证角色类型"""
        if not role_type:
            return False, "角色类型 (role_type) 不能为空"
        
        if not cls.ROLE_TYPE_PATTERN.match(role_type):
            return False, "角色类型格式错误: 仅含字母/数字/下划线/中划线，2-50字符"
        
        return True, ""
    
    @classmethod
    def generate_server_prefix(cls) -> str:
        """生成服务器唯一前缀（8位随机字符）"""
        return secrets.token_hex(4)  # 生成8位十六进制字符
    
    @classmethod
    def generate_npc_id(cls, planner_id: str, role_type: str) -> str:
        """生成唯一的 NPC ID，格式：{server_prefix}_{planner_id}_{role_type}"""
        server_prefix = cls.generate_server_prefix()
        return f"{server_prefix}_{planner_id}_{role_type}"
    
    @classmethod
    def validate_personality(cls, data: dict) -> tuple[bool, str]:
        """验证性格配置"""
        if not data:
            return False, "personality 配置不能为空"
        
        # 必填字段
        if not data.get("name"):
            return False, "personality.name 不能为空"
        if len(data.get("name", "")) > 50:
            return False, "personality.name 不能超过50字符"
        
        if not data.get("role"):
            return False, "personality.role 不能为空"
        if len(data.get("role", "")) > 100:
            return False, "personality.role 不能超过100字符"
        
        # 可选字段验证
        if len(data.get("background", "")) > 1000:
            return False, "personality.background 不能超过1000字符"
        
        if len(data.get("speech_style", "")) > 500:
            return False, "personality.speech_style 不能超过500字符"
        
        traits = data.get("traits", [])
        if not isinstance(traits, list):
            return False, "personality.traits 必须是数组"
        if len(traits) > 20:
            return False, "personality.traits 不能超过20项"
        
        knowledge = data.get("knowledge", [])
        if not isinstance(knowledge, list):
            return False, "personality.knowledge 必须是数组"
        if len(knowledge) > 20:
            return False, "personality.knowledge 不能超过20项"
        
        return True, ""
    
    @classmethod
    def validate_create(cls, data: dict) -> tuple[bool, str]:
        """验证创建请求（新版本：使用 planner_id + role_type）"""
        # 新版本：验证 planner_id 和 role_type
        if "planner_id" in data and "role_type" in data:
            valid, msg = cls.validate_planner_id(data.get("planner_id", ""))
            if not valid:
                return False, msg
            
            valid, msg = cls.validate_role_type(data.get("role_type", ""))
            if not valid:
                return False, msg
        # 兼容旧版本：直接使用 npc_id
        elif "npc_id" in data:
            valid, msg = cls.validate_id(data.get("npc_id", ""))
            if not valid:
                return False, msg
        else:
            return False, "必须提供 planner_id + role_type 或 npc_id"
        
        # 验证personality
        valid, msg = cls.validate_personality(data.get("personality", {}))
        if not valid:
            return False, msg
        
        return True, ""
    
    @classmethod
    def validate_update(cls, data: dict) -> tuple[bool, str]:
        """验证更新请求"""
        # 更新时personality是可选的，但如果提供了必须有效
        if "personality" in data:
            valid, msg = cls.validate_personality(data["personality"])
            if not valid:
                return False, msg
        
        return True, ""


# ==================== 存储管理 ====================

class PersonaStorage:
    """人设配置存储管理"""
    
    def __init__(self, config_dir: Path = None):
        self.config_dir = config_dir or get_config_dir()
        self.config_dir.mkdir(parents=True, exist_ok=True)
    
    def _get_path(self, persona_id: str) -> Path:
        """获取配置文件路径"""
        return self.config_dir / f"{persona_id}.json"
    
    def exists(self, persona_id: str) -> bool:
        """检查人设是否存在"""
        return self._get_path(persona_id).exists()
    
    def list_all(self) -> List[Dict[str, Any]]:
        """列出所有人设 (摘要信息)"""
        personas = []
        for f in self.config_dir.glob("*.json"):
            try:
                with open(f, 'r', encoding='utf-8') as fp:
                    data = json.load(fp)
                    # 返回摘要信息
                    summary = {
                        "npc_id": data.get("npc_id", f.stem),
                        "name": data.get("personality", {}).get("name", ""),
                        "role": data.get("personality", {}).get("role", ""),
                        "updated_at": data.get("updated_at", "")
                    }
                    # 添加策划ID和角色类型（如果存在）
                    if "planner_id" in data:
                        summary["planner_id"] = data["planner_id"]
                    if "role_type" in data:
                        summary["role_type"] = data["role_type"]
                    personas.append(summary)
            except Exception:
                continue
        return sorted(personas, key=lambda x: x["npc_id"])
    
    def get(self, persona_id: str) -> Optional[Dict[str, Any]]:
        """获取指定人设"""
        path = self._get_path(persona_id)
        if not path.exists():
            return None
        
        try:
            with open(path, 'r', encoding='utf-8') as f:
                data = json.load(f)
                data["npc_id"] = persona_id  # 确保ID一致
                return data
        except Exception:
            return None
    
    def create(self, persona_id: str, data: dict) -> Dict[str, Any]:
        """创建新人设"""
        now = datetime.now().isoformat()
        
        config = {
            "npc_id": persona_id,
            "personality": data["personality"],
            "version": data.get("version", "1.0"),
            "created_at": now,
            "updated_at": now
        }
        
        # 保存策划ID和角色类型（如果提供）
        if "planner_id" in data:
            config["planner_id"] = data["planner_id"]
        if "role_type" in data:
            config["role_type"] = data["role_type"]
        
        path = self._get_path(persona_id)
        with open(path, 'w', encoding='utf-8') as f:
            json.dump(config, f, ensure_ascii=False, indent=2)
        
        return config
    
    def update(self, persona_id: str, data: dict) -> Optional[Dict[str, Any]]:
        """更新人设"""
        existing = self.get(persona_id)
        if not existing:
            return None
        
        # 合并更新
        if "personality" in data:
            existing["personality"].update(data["personality"])
        if "version" in data:
            existing["version"] = data["version"]
        
        existing["updated_at"] = datetime.now().isoformat()
        
        path = self._get_path(persona_id)
        with open(path, 'w', encoding='utf-8') as f:
            json.dump(existing, f, ensure_ascii=False, indent=2)
        
        return existing
    
    def delete(self, persona_id: str) -> bool:
        """删除人设"""
        path = self._get_path(persona_id)
        if not path.exists():
            return False
        
        path.unlink()
        return True


# ==================== API 响应 ====================

@dataclass
class ApiResponse:
    """API 统一响应格式"""
    success: bool
    data: Any = None
    error: str = ""
    code: int = 200
    
    def to_dict(self) -> Dict[str, Any]:
        result = {
            "success": self.success,
            "code": self.code
        }
        if self.success:
            result["data"] = self.data
        else:
            result["error"] = self.error
        return result


# ==================== API 处理器 ====================

class PersonaAPI:
    """人设管理 API"""
    
    def __init__(self):
        self.storage = PersonaStorage()
        self.validator = PersonaValidator()
    
    def list_personas(self) -> ApiResponse:
        """GET /api/persona/ - 列出所有人设"""
        personas = self.storage.list_all()
        return ApiResponse(
            success=True,
            data={"personas": personas, "total": len(personas)}
        )
    
    def get_persona(self, persona_id: str) -> ApiResponse:
        """GET /api/persona/<id>/ - 获取指定人设"""
        # 验证ID格式
        valid, msg = self.validator.validate_id(persona_id)
        if not valid:
            return ApiResponse(success=False, error=msg, code=400)
        
        persona = self.storage.get(persona_id)
        if not persona:
            return ApiResponse(
                success=False, 
                error=f"人设 '{persona_id}' 不存在",
                code=404
            )
        
        return ApiResponse(success=True, data=persona)
    
    def create_persona(self, data: dict) -> ApiResponse:
        """POST /api/persona/ - 创建新人设
        
        支持两种方式：
        1. 新版本（推荐）：提供 planner_id + role_type，系统自动生成 npc_id
        2. 旧版本（兼容）：直接提供 npc_id
        """
        # 验证请求数据
        valid, msg = self.validator.validate_create(data)
        if not valid:
            return ApiResponse(success=False, error=msg, code=400)
        
        # 确定 persona_id
        if "planner_id" in data and "role_type" in data:
            # 新版本：自动生成 npc_id（带服务器随机前缀）
            planner_id = data["planner_id"]
            role_type = data["role_type"]
            persona_id = self.validator.generate_npc_id(planner_id, role_type)
            
            # 注意：由于添加了随机前缀，每次创建都会生成唯一的 ID
            # 因此同一策划可以创建多个相同 role_type 的角色
            
            # 将生成的 npc_id 添加到数据中
            data["npc_id"] = persona_id
        else:
            # 旧版本：使用提供的 npc_id
            persona_id = data["npc_id"]
            
            # 检查是否已存在
            if self.storage.exists(persona_id):
                return ApiResponse(
                    success=False,
                    error=f"人设 '{persona_id}' 已存在，请使用 PUT 更新",
                    code=409
                )
        
        # 创建
        config = self.storage.create(persona_id, data)
        return ApiResponse(success=True, data=config, code=201)
    
    def update_persona(self, persona_id: str, data: dict) -> ApiResponse:
        """PUT /api/persona/<id>/ - 更新人设"""
        # 验证ID
        valid, msg = self.validator.validate_id(persona_id)
        if not valid:
            return ApiResponse(success=False, error=msg, code=400)
        
        # 检查是否存在
        if not self.storage.exists(persona_id):
            return ApiResponse(
                success=False,
                error=f"人设 '{persona_id}' 不存在",
                code=404
            )
        
        # 验证更新数据
        valid, msg = self.validator.validate_update(data)
        if not valid:
            return ApiResponse(success=False, error=msg, code=400)
        
        # 更新
        config = self.storage.update(persona_id, data)
        return ApiResponse(success=True, data=config)
    
    def delete_persona(self, persona_id: str) -> ApiResponse:
        """DELETE /api/persona/<id>/ - 删除人设"""
        # 验证ID
        valid, msg = self.validator.validate_id(persona_id)
        if not valid:
            return ApiResponse(success=False, error=msg, code=400)
        
        # 删除
        if not self.storage.delete(persona_id):
            return ApiResponse(
                success=False,
                error=f"人设 '{persona_id}' 不存在",
                code=404
            )
        
        return ApiResponse(
            success=True,
            data={"message": f"人设 '{persona_id}' 已删除"}
        )


# ==================== 全局实例 ====================

persona_api = PersonaAPI()
