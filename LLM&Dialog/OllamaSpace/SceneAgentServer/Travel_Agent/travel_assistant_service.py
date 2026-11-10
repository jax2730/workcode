import json
import logging
import math
import re
import time
import uuid
from pathlib import Path
from typing import Any, Dict, List, Optional, Tuple

import requests
from pydantic import BaseModel, Field, ValidationError
from langchain_community.chat_message_histories import SQLChatMessageHistory
from langchain_core.messages import AIMessage, HumanMessage, SystemMessage


TRAVEL_ASSISTANT_PROMPT = (
    "你是一个专业、自然、会连续对话的地点助手。"
    "你会基于得到的建筑信息，天气摘要和最近对话历史，给出贴近用户语气、可执行、简洁但不生硬的建议。"
    "不要重复输出调试字段，不要复述结构化输入。"
)

TRAVEL_DIALOG_STRATEGY_PROMPT = (
    "你是一个对话策略分析器。"
    "请根据用户当前消息和最近历史，判断这一轮是否需要："
    "1) 仅自然闲聊；2) 查找周边点位;"
    "不要机械关键词匹配，要综合语义和上下文。"
)

TRAVEL_RESPONSE_SYSTEM_PROMPT = (
    "先完成用户目的"
    "语气亲切、具体、简洁，不要模板化，不要复述结构化输入。"
    "当用户只是寒暄时，进行简单对话"
    "仅当策略建议需要时，才使用位置、天气、POI、路线数据。"
    "如用户询问附近建筑，直接输出一定范围内的建筑信息。"
    "在建筑查询场景中：不要寒暄，不要扩展无关建议；"
    "建筑名必须使用括号格式，例如（建筑名）；"
    "优先用短句列出结果，控制在1-3句。"
)

MAX_HISTORY_FOR_STRATEGY = 4
MAX_HISTORY_FOR_RESPONSE = 6
MAX_POI_FOR_MODEL_CONTEXT = 7
MAX_HISTORY_TEXT_LENGTH = 120
DEFAULT_ROUTE_TARGET_POI = 5
DEFAULT_INLINE_POI_REPLY_COUNT = 10
MAX_INLINE_POI_REPLY_COUNT = 80
PREVIEW_POI_REPLY_COUNT = 7
AUTO_CONTINUE_FULL_POI_BATCHES = True

POI_API_URL = "http://192.168.5.189:5050/get_poi"
AUTO_RADIUS_START = 10
AUTO_RADIUS_STEP = 50
AUTO_RADIUS_MAX = 10000
AUTO_RADIUS_LOCAL_MAX = 1200
AUTO_RADIUS_EXPANDED_STAGE_SPAN = 2000
AUTO_SEMANTIC_EARLY_PROBE_RADIUS = 450

_TRAVEL_LLM = None
_TRAVEL_HISTORY_CACHE: Dict[str, SQLChatMessageHistory] = {}
_TRAVEL_POI_MEMORY_CACHE: Dict[str, List[Dict[str, Any]]] = {}
TRAVEL_HISTORY_DB_URL = f"sqlite:///{(Path(__file__).resolve().parent / 'travel_chat.db').as_posix()}"
_TRAVEL_LLM_INIT_ERROR: Optional[str] = None

WEATHER_CODE_TO_TEXT: Dict[str, str] = {
    "01d": "晴",
    "01n": "晴夜",
    "02d": "少云",
    "02n": "少云",
    "03d": "多云",
    "03n": "多云",
    "04d": "阴",
    "04n": "阴",
    "09d": "阵雨",
    "09n": "阵雨",
    "10d": "小雨",
    "10n": "小雨",
    "11d": "雷雨",
    "11n": "雷雨",
    "13d": "雪",
    "13n": "雪",
    "50d": "雾",
    "50n": "雾",
}

POI_TYPE_TO_CN: Dict[str, str] = {
    "Airport": "机场",
    "AmusementPark": "主题公园",
    "Bank": "银行",
    "CarDealer": "汽车4S店",
    "CoachStation": "巴士站",
    "ConvienceStore": "便利店",
    "Factory": "工厂",
    "FarmerMarket": "农贸市场",
    "GasStation": "加油站",
    "HairSalon": "美发",
    "HomeBuildingMaterialMarket": "家居建材市场",
    "Hospital": "医院",
    "Hotel": "酒店",
    "Kindergarten": "幼儿园",
    "KTV": "KTV",
    "LanzhouRamen": "兰州拉面",
    "Library": "图书馆",
    "McDonalds": "麦当劳",
    "Museum": "博物馆",
    "OfficeBuilding": "办公楼",
    "Onsen": "温泉",
    "Park": "公园",
    "Port": "港口",
    "PostOffice": "邮局",
    "PublicBathroom": "公共洗手间",
    "ResidentalComplex": "住宅小区",
    "School": "学校",
    "ShoppingMall": "购物中心",
    "Stadium": "体育场",
    "Supermarket": "超市",
    "TrainStation": "火车站",
    "ViewSpot": "景点",
    "Zoo": "动物园",
}

logger = logging.getLogger("travel_assistant_service")
if not logger.handlers:
    logging.basicConfig(level=logging.INFO)


class TravelChatInput(BaseModel):
    """旅行对话输入。"""

    message: str = ""
    session_id: Optional[str] = None
    x: Optional[float] = None
    y: Optional[float] = None
    radius: Optional[int] = Field(default=None, ge=100, le=50000)
    state: Optional[str] = None
    city: Optional[str] = None
    county: Optional[str] = None
    area_name: Optional[str] = None
    temperature: Optional[float] = None
    weather_name: Optional[str] = None
    weather_description: Optional[str] = None
    weather_desc: Optional[str] = None
    weather_source: Optional[str] = None
    weather_pending: Optional[bool] = None
    wind_speed: Optional[float] = None
    top_k: int = Field(default=5, ge=3, le=20)
    auto_continue_full_poi_batches: bool = AUTO_CONTINUE_FULL_POI_BATCHES
    debug: bool = False
    debug_trace: bool = True


class WeatherSummary(BaseModel):
    """天气关键摘要。"""

    temperature: Optional[float] = None
    weather_name: Optional[str] = None
    weather_description: Optional[str] = None
    wind_speed: Optional[float] = None
    city: str
    state: str


class RoutePoint(BaseModel):
    """路线点位。"""

    name: str
    distance_hint: Optional[float] = None


class RoutePlan(BaseModel):
    """计算出的路线结果。"""

    summary: str
    route_points: List[RoutePoint]
    estimated_duration_hours: float
    strategy: str


class TravelChatOutput(BaseModel):
    """旅行对话输出。"""

    success: bool
    reply: str
    session_id: str
    weather_summary: WeatherSummary
    recommendation: Dict[str, Any]
    input: Optional[Dict[str, Any]] = None
    poi_sample: Optional[List[str]] = None
    search_meta: Optional[Dict[str, Any]] = None
    context_payload: Optional[Dict[str, Any]] = Field(
        default=None,
        validation_alias="model_context",
        serialization_alias="model_context",
    )


class DialogStrategy(BaseModel):
    """当前轮对话策略。"""

    intent: str = "casual_chat"
    user_goal: str = ""
    should_use_location_context: bool = False
    should_search_poi: bool = False
    should_plan_route: bool = False
    requested_poi_count: Optional[int] = Field(default=None, ge=1, le=5000)
    target_poi_count: Optional[int] = Field(default=None, ge=3, le=5000)
    min_required_poi: Optional[int] = Field(default=None, ge=1, le=5000)
    search_keywords: List[str] = Field(default_factory=list)
    search_scope: str = "local"
    history_relevance: str = "medium"
    response_tone: str = "friendly"
    confidence: float = 0.0
    ##followup_question: Optional[str] = None


class PoiSearchPreference(BaseModel):
    """POI语义检索偏好。"""

    has_semantic_target: bool = False
    target_labels: List[str] = Field(default_factory=list)
    must_find: bool = False
    allow_extended_search: bool = False
    goal_hint: str = ""


class PoiPresentationDecision(BaseModel):
    """POI结果展示决策。"""

    mode: str = "summary"
    requested_count: Optional[int] = None
    user_wants_all: bool = False
    need_confirmation_when_oversized: bool = True


def _normalize_text_for_match(value: str) -> str:
    """归一化文本用于实体名称匹配。"""
    text = (value or "").strip().lower()
    text = text.replace("（", "").replace("）", "")
    return re.sub(r"\s+", "", text)


def _remember_poi_candidates(
    session_id: str,
    poi_items: List[Dict[str, Any]],
    center_x: float,
    center_y: float,
) -> None:
    """将本轮检索到的POI写入会话缓存，供后续距离追问复用。"""
    memory = _TRAVEL_POI_MEMORY_CACHE.get(session_id, [])
    existing_keys = {
        _normalize_text_for_match(str(item.get("name") or ""))
        for item in memory
        if str(item.get("name") or "").strip()
    }

    for poi in poi_items[:80]:
        name = _poi_name(poi)
        key = _normalize_text_for_match(name)
        if not key or key in existing_keys:
            continue
        memory.append(
            {
                "name": name,
                "type": _poi_type(poi),
                "distance_hint": _poi_distance_hint(poi, center_x, center_y),
                "captured_at": time.time(),
            }
        )
        existing_keys.add(key)

    memory = memory[-120:]
    _TRAVEL_POI_MEMORY_CACHE[session_id] = memory


def _extract_distance_query_targets(user_message: str, strategy: DialogStrategy) -> List[str]:
    """提取距离追问中的目标实体候选。"""
    candidates: List[str] = []
    raw_message = str(user_message or "").strip()
    if raw_message:
        match = re.search(r"(.+?)(?:距离|离我|多远)", raw_message)
        if match:
            value = match.group(1).strip(" ，。！？?！")
            if value:
                candidates.append(value)

    for item in strategy.search_keywords:
        text = str(item).strip()
        if text:
            candidates.append(text)

    goal_hint = str(strategy.user_goal or "").strip()
    if goal_hint:
        goal_tokens = [token.strip() for token in re.split(r"[,，;/|\s]+", goal_hint) if token.strip()]
        candidates.extend(goal_tokens[:4])

    deduped: List[str] = []
    seen = set()
    for item in candidates:
        key = _normalize_text_for_match(item)
        if key and key not in seen:
            deduped.append(item)
            seen.add(key)
    return deduped[:8]


def _find_cached_poi_for_distance_query(
    session_id: str,
    user_message: str,
    strategy: DialogStrategy,
) -> Optional[Dict[str, Any]]:
    """在会话缓存中查找距离追问目标，命中则可跳过POI查询。"""
    memory = _TRAVEL_POI_MEMORY_CACHE.get(session_id, [])
    if not memory:
        return None

    targets = _extract_distance_query_targets(user_message, strategy)
    if not targets:
        return None

    normalized_targets = [_normalize_text_for_match(item) for item in targets]
    normalized_targets = [item for item in normalized_targets if item]
    if not normalized_targets:
        return None

    for target in normalized_targets:
        for item in reversed(memory):
            name_text = _normalize_text_for_match(str(item.get("name") or ""))
            type_text = _normalize_text_for_match(str(item.get("type") or ""))
            if not name_text and not type_text:
                continue
            if target == name_text or target == type_text or target in name_text:
                return item

    return None


def _build_search_preference_from_strategy(strategy: DialogStrategy) -> PoiSearchPreference:
    """基于策略模型输出构建检索偏好（不依赖别名表硬匹配）。"""
    normalized_scope = str(strategy.search_scope or "local").strip().lower()
    allow_extended = normalized_scope == "expanded"
    keywords = [str(item).strip() for item in strategy.search_keywords if str(item).strip()]
    goal_hint = str(strategy.user_goal or "").strip()

    if not keywords and goal_hint:
        goal_tokens = [token.strip() for token in re.split(r"[,，;/|\s]+", goal_hint) if token.strip()]
        keywords = goal_tokens[:8]

    return PoiSearchPreference(
        has_semantic_target=bool(keywords),
        target_labels=keywords[:12],
        must_find=allow_extended,
        allow_extended_search=allow_extended,
        goal_hint=goal_hint,
    )


def _to_bool(value: Any, default: bool = False) -> bool:
    if isinstance(value, bool):
        return value
    if value is None:
        return default
    return str(value).strip().lower() in {"1", "true", "yes", "y", "on"}


def _get_travel_llm():
    """延迟加载LLM实例。"""
    global _TRAVEL_LLM, _TRAVEL_LLM_INIT_ERROR
    if _TRAVEL_LLM is not None:
        return _TRAVEL_LLM
    try:
        from extrator.llm.model_factory import create_chat_model

        _TRAVEL_LLM = create_chat_model()
        _TRAVEL_LLM_INIT_ERROR = None
    except Exception as exception:
        _TRAVEL_LLM = None
        _TRAVEL_LLM_INIT_ERROR = str(exception)
    return _TRAVEL_LLM


def _get_session_history(session_id: str) -> List[Dict[str, str]]:
    """获取旅行助手会话历史（持久化到SQLite）。"""
    if session_id not in _TRAVEL_HISTORY_CACHE:
        _TRAVEL_HISTORY_CACHE[session_id] = SQLChatMessageHistory(
            session_id,
            connection=TRAVEL_HISTORY_DB_URL,
        )

    history = _TRAVEL_HISTORY_CACHE[session_id]
    messages: List[Dict[str, str]] = []
    for item in history.messages[-12:]:
        if isinstance(item, HumanMessage):
            messages.append({"role": "user", "content": str(item.content)})
        elif isinstance(item, AIMessage):
            messages.append({"role": "assistant", "content": str(item.content)})
        elif isinstance(item, SystemMessage):
            messages.append({"role": "system", "content": str(item.content)})
    return messages


def _append_session_history(session_id: str, role: str, content: str) -> None:
    """写入旅行助手会话历史。"""
    if session_id not in _TRAVEL_HISTORY_CACHE:
        _TRAVEL_HISTORY_CACHE[session_id] = SQLChatMessageHistory(
            session_id,
            connection=TRAVEL_HISTORY_DB_URL,
        )

    history = _TRAVEL_HISTORY_CACHE[session_id]
    if role == "user":
        history.add_user_message(content)
    elif role == "assistant":
        history.add_ai_message(content)


def _coerce_float(value: Any) -> Optional[float]:
    if isinstance(value, (int, float)):
        return float(value)
    if isinstance(value, str):
        try:
            return float(value)
        except Exception:
            return None
    return None


def _extract_float_from_text(text: str, key: str) -> Optional[float]:
    pattern = rf"{key}\s*[:=：]\s*(-?\d+(?:\.\d+)?)"
    match = re.search(pattern, text, flags=re.IGNORECASE)
    if match:
        try:
            return float(match.group(1))
        except Exception:
            return None

    pair_match = re.search(
        r"(-?\d+(?:\.\d+)?)\s*[,，]\s*(-?\d+(?:\.\d+)?)",
        text,
    )
    if pair_match:
        try:
            first_value = float(pair_match.group(1))
            second_value = float(pair_match.group(2))
            return first_value if key.lower() == "x" else second_value
        except Exception:
            return None
    return None


def _extract_radius_from_text(text: str) -> Optional[int]:
    """从用户文本中提取半径（米）。"""
    if not text:
        return None
    match = re.search(r"(\d{1,6})\s*(?:米|m|M)", text)
    if not match:
        return None
    try:
        value = int(match.group(1))
    except Exception:
        return None
    if value < 100:
        return 100
    if value > 50000:
        return 50000
    return value


def _extract_state_city_from_text(text: str) -> Tuple[Optional[str], Optional[str]]:
    state_match = re.search(
        r"(?:state|province|省)\s*[:=：]\s*([\w\-\u4e00-\u9fa5]+)",
        text,
        flags=re.IGNORECASE,
    )
    city_match = re.search(
        r"(?:city|市)\s*[:=：]\s*([\w\-\u4e00-\u9fa5]+)",
        text,
        flags=re.IGNORECASE,
    )

    state = state_match.group(1) if state_match else None
    city = city_match.group(1) if city_match else None
    if state and city:
        return state, city

    weather_path_match = re.search(
        r"weather/([\w\-\u4e00-\u9fa5]+)/([\w\-\u4e00-\u9fa5]+)",
        text,
        flags=re.IGNORECASE,
    )
    if weather_path_match:
        return weather_path_match.group(1), weather_path_match.group(2)

    slash_pair_match = re.search(
        r"\b([A-Za-z][\w\-]*)/([A-Za-z][\w\-]*)\b",
        text,
    )
    if slash_pair_match:
        return slash_pair_match.group(1), slash_pair_match.group(2)

    return state, city


def _normalize_poi_items(poi_response: Any) -> List[Dict[str, Any]]:
    if isinstance(poi_response, list):
        return poi_response
    if not isinstance(poi_response, dict):
        return []

    for key in ("pois", "data", "result", "results", "items"):
        value = poi_response.get(key)
        if isinstance(value, list):
            return value
        if isinstance(value, dict):
            for nested_key in ("pois", "items", "results"):
                nested_value = value.get(nested_key)
                if isinstance(nested_value, list):
                    return nested_value
    return []


def _poi_name(poi: Dict[str, Any]) -> str:
    raw_poi_type = str(poi.get("type") or "").strip()
    poi_type = POI_TYPE_TO_CN.get(raw_poi_type, raw_poi_type)
    for key in ("name", "poi_name", "building_name", "title"):
        value = poi.get(key)
        if value:
            name = str(value).strip()
            if not name:
                continue
            if name.lower() == "unnamed":
                return poi_type or "未知地点"
            if raw_poi_type and name.lower() == raw_poi_type.lower():
                return poi_type or name

            return name

    if poi_type:
        return poi_type

    return "未知地点"


def _poi_type(poi: Dict[str, Any]) -> str:
    value = poi.get("type")
    if value is None:
        return ""
    raw_type = str(value).strip()
    return POI_TYPE_TO_CN.get(raw_type, raw_type)


def _is_exact_type_or_name_match(poi: Dict[str, Any], preference: PoiSearchPreference) -> bool:
    """判断POI是否与语义目标形成精确匹配（优先 type，其次 name）。"""
    if not preference.has_semantic_target:
        return False

    poi_name = _poi_name(poi).lower()
    raw_type = str(poi.get("type") or "").strip().lower()
    mapped_type = _poi_type(poi).lower()

    for label in preference.target_labels:
        token = str(label).strip().lower()
        if not token:
            continue
        if token == raw_type or token == mapped_type or token == poi_name:
            return True
    return False


def _poi_semantic_score(poi: Dict[str, Any], preference: PoiSearchPreference) -> int:
    """计算POI与语义目标的匹配分数（精确type/name优先）。"""
    if not preference.has_semantic_target:
        return 0

    poi_name = _poi_name(poi).lower()
    raw_type = str(poi.get("type") or "").strip().lower()
    mapped_type = _poi_type(poi).lower()
    type_tokens = [token for token in (raw_type, mapped_type) if token]
    score = 0

    for label in preference.target_labels:
        token = str(label).strip().lower()
        if not token:
            continue

        if token in type_tokens:
            score += 12
        elif any(token in type_token for type_token in type_tokens):
            score += 5

        if token == poi_name:
            score += 8
        elif token in poi_name:
            score += 4

    return score


def _semantic_filter_poi_candidates_with_llm(
    poi_items: List[Dict[str, Any]],
    preference: PoiSearchPreference,
) -> List[Dict[str, Any]]:
    """使用LLM按语义从候选POI中筛选相关项，避免依赖关键词别名映射。"""
    if not preference.has_semantic_target or not poi_items:
        return []

    llm = _get_travel_llm()
    if llm is None:
        return []

    limited_items = poi_items[:30]
    goal_text = preference.goal_hint or "，".join(preference.target_labels[:6])

    candidates: List[Dict[str, Any]] = []
    for index, item in enumerate(limited_items):
        candidates.append(
            {
                "index": index,
                "name": _poi_name(item),
                "type": _poi_type(item),
            }
        )

    prompt = (
        "你是POI语义筛选器。"
        "根据用户目的，从候选列表中选出语义相关的index。"
        "优先依据type/name语义，不要靠字面同词。"
        "只返回JSON：{\"related_indexes\":[0,2]}。"
        f"用户目的：{goal_text}\n"
        f"候选：{_compact_json({'candidates': candidates})}"
    )

    try:
        response = llm.invoke(
            [
                {"role": "system", "content": "你是严谨的语义筛选器，只输出JSON。"},
                {"role": "user", "content": prompt},
            ]
        )
        raw_text = str(getattr(response, "content", "") or "").strip()
        parsed = _extract_first_json_block(raw_text) or {}
        indexes = parsed.get("related_indexes")
        if not isinstance(indexes, list):
            return []

        selected: List[Dict[str, Any]] = []
        for value in indexes:
            try:
                idx = int(value)
            except Exception:
                continue
            if 0 <= idx < len(limited_items):
                selected.append(limited_items[idx])
        return selected
    except Exception:
        return []


def _rank_poi_candidates(
    poi_items: List[Dict[str, Any]],
    center_x: float,
    center_y: float,
    preference: PoiSearchPreference,
) -> List[Dict[str, Any]]:
    """按语义匹配优先、距离次优进行排序。"""

    def _sort_key(item: Dict[str, Any]) -> Tuple[int, float, str]:
        score = _poi_semantic_score(item, preference)
        distance = _poi_distance_hint(item, center_x, center_y)
        normalized_distance = distance if distance is not None else 10**12
        name = _poi_name(item)
        return (-score, normalized_distance, name)

    return sorted(poi_items, key=_sort_key)

def _poi_distance_hint(poi: Dict[str, Any], center_x: float, center_y: float) -> Optional[float]:
    distance_value = _coerce_float(poi.get("distance"))
    if distance_value is not None:
        return distance_value

    # 常见坐标字段：coord / pos / axis.x-axis.y / x-y / lon-lat / longitude-latitude
    coord_value = poi.get("coord")
    if isinstance(coord_value, (list, tuple)) and len(coord_value) >= 2:
        coord_x = _coerce_float(coord_value[0])
        coord_y = _coerce_float(coord_value[1])
        if coord_x is not None and coord_y is not None:
            return float(math.hypot(coord_x - center_x, coord_y - center_y))

    pos_value = poi.get("pos")
    if isinstance(pos_value, (list, tuple)) and len(pos_value) >= 2:
        pos_x = _coerce_float(pos_value[0])
        pos_y = _coerce_float(pos_value[1])
        if pos_x is not None and pos_y is not None:
            return float(math.hypot(pos_x, pos_y))

    axis_x = _coerce_float(poi.get("axis.x") or poi.get("axis_x") or poi.get("x"))
    axis_y = _coerce_float(poi.get("axis.y") or poi.get("axis_y") or poi.get("y"))
    if axis_x is not None and axis_y is not None:
        return float(math.hypot(axis_x - center_x, axis_y - center_y))

    candidates = [
        (poi.get("x"), poi.get("y")),
        (poi.get("lon"), poi.get("lat")),
        (poi.get("longitude"), poi.get("latitude")),
        (poi.get("coord_x"), poi.get("coord_y")),
        (poi.get("pos_x"), poi.get("pos_y")),
    ]
    for point_x, point_y in candidates:
        x_value = _coerce_float(point_x)
        y_value = _coerce_float(point_y)
        if x_value is not None and y_value is not None:
            return float(math.hypot(x_value - center_x, y_value - center_y))
    return None


def _summarize_weather(payload: Optional[Dict[str, Any]], default_state: str, default_city: str) -> WeatherSummary:
    normalized_payload: Dict[str, Any] = payload if isinstance(payload, dict) else {}
    return WeatherSummary(
        temperature=normalized_payload.get("temperature"),
        weather_name=normalized_payload.get("weather_name"),
        weather_description=normalized_payload.get("weather_description"),
        wind_speed=normalized_payload.get("wind_speed"),
        city=str(normalized_payload.get("city") or default_city),
        state=str(normalized_payload.get("state") or default_state),
    )


def _compute_route(
    poi_items: List[Dict[str, Any]],
    center_x: float,
    center_y: float,
    weather_summary: WeatherSummary,
    top_k: int,
) -> RoutePlan:
    points: List[RoutePoint] = []
    for item in poi_items:
        name = _poi_name(item)
        distance_hint = _poi_distance_hint(item, center_x, center_y)
        points.append(RoutePoint(name=name, distance_hint=distance_hint))

    points.sort(
        key=lambda point: point.distance_hint if point.distance_hint is not None else 10**12
    )

    selected = points[:top_k]
    if not selected:
        return RoutePlan(
            summary="未找到可用建筑点位，建议扩大半径后重试。",
            route_points=[],
            estimated_duration_hours=0.0,
            strategy="先获取更多可用建筑点，再做路线。",
        )

    weather_brief = weather_summary.weather_description or weather_summary.weather_name or "天气信息有限"
    strategy = "优先近距离串联点位，减少来回折返。"
    summary = f"天气：{weather_brief}，按距离从近到远安排 {len(selected)} 个点位。"
    estimated_duration_hours = round(len(selected) * 0.8, 1)

    return RoutePlan(
        summary=summary,
        route_points=selected,
        estimated_duration_hours=estimated_duration_hours,
        strategy=strategy,
    )


def _extract_first_json_block(text: str) -> Optional[Dict[str, Any]]:
    """从模型文本中提取首个JSON对象。"""
    if not text:
        return None
    start = text.find("{")
    end = text.rfind("}")
    if start < 0 or end <= start:
        return None
    try:
        return json.loads(text[start : end + 1])
    except Exception:
        return None


def _normalize_weather_fields(request_data: TravelChatInput) -> None:
    """兼容天气字段并将图标码归一为可读文本。"""
    if not request_data.weather_description and request_data.weather_desc:
        request_data.weather_description = request_data.weather_desc

    if request_data.weather_name:
        weather_code = str(request_data.weather_name).strip()
        if not request_data.weather_description and weather_code in WEATHER_CODE_TO_TEXT:
            request_data.weather_description = WEATHER_CODE_TO_TEXT[weather_code]


def _build_default_strategy(user_message: str) -> DialogStrategy:
    """构建保守的默认策略（不强制规划）。"""
    return DialogStrategy(
        intent="casual_chat",
        user_goal=user_message.strip(),
        should_use_location_context=False,
        should_search_poi=False,
        should_plan_route=False,
        target_poi_count=None,
        min_required_poi=None,
        search_keywords=[],
        search_scope="local",
        history_relevance="medium",
        response_tone="friendly",
        confidence=0.3,
    )


def _resolve_retrieval_plan(
    strategy: DialogStrategy,
    request_data: TravelChatInput,
) -> Tuple[int, int]:
    """根据策略语义输出生成检索计划（目标数量+最小可用数量）。"""
    base_top_k = max(3, min(5000, int(request_data.top_k)))

    inferred_target = strategy.target_poi_count or strategy.requested_poi_count
    if inferred_target is None:
        if strategy.should_plan_route:
            inferred_target = DEFAULT_ROUTE_TARGET_POI
        elif strategy.should_search_poi:
            inferred_target = max(5, base_top_k)
        else:
            inferred_target = base_top_k

    effective_top_k = max(3, min(5000, int(inferred_target)))

    inferred_min_required = strategy.min_required_poi
    if inferred_min_required is None:
        if strategy.should_plan_route:
            inferred_min_required = max(4, int(math.ceil(effective_top_k * 0.6)))
        elif strategy.should_search_poi:
            inferred_min_required = max(3, int(math.ceil(effective_top_k * 0.5)))
        else:
            inferred_min_required = min(3, effective_top_k)

    min_required = max(1, min(effective_top_k, int(inferred_min_required)))
    strategy.target_poi_count = effective_top_k
    strategy.min_required_poi = min_required
    return effective_top_k, min_required


def _sanitize_strategy_json(payload: Dict[str, Any]) -> Dict[str, Any]:
    """清洗策略模型输出，避免0值触发校验错误。"""
    cleaned = dict(payload)
    count_fields = ["requested_poi_count", "target_poi_count", "min_required_poi"]

    for field_name in count_fields:
        raw_value = cleaned.get(field_name)
        if raw_value is None:
            continue
        try:
            normalized_value = int(raw_value)
        except Exception:
            cleaned[field_name] = None
            continue
        cleaned[field_name] = normalized_value if normalized_value > 0 else None

    confidence_value = cleaned.get("confidence")
    if confidence_value is not None:
        try:
            cleaned["confidence"] = max(0.0, min(1.0, float(confidence_value)))
        except Exception:
            cleaned["confidence"] = 0.0

    keywords = cleaned.get("search_keywords")
    keyword_list: List[str] = []
    if isinstance(keywords, list):
        keyword_list = [str(item).strip() for item in keywords if str(item).strip()]
    elif isinstance(keywords, str):
        parts = re.split(r"[,，;/|\s]+", keywords)
        keyword_list = [part.strip() for part in parts if part.strip()]

    cleaned["search_keywords"] = keyword_list[:12]

    scope = str(cleaned.get("search_scope") or "local").strip().lower()
    if scope not in {"local", "expanded"}:
        scope = "local"
    cleaned["search_scope"] = scope

    history_relevance = str(cleaned.get("history_relevance") or "medium").strip().lower()
    if history_relevance not in {"low", "medium", "high"}:
        history_relevance = "medium"
    cleaned["history_relevance"] = history_relevance

    return cleaned


def _infer_session_goal(
    session_history: List[Dict[str, str]],
    user_message: str,
) -> str:
    """从近期历史推断会话目标，供策略层最小上下文使用。"""
    recent_user_messages = [
        _short_text(str(item.get("content", "")), 60)
        for item in session_history[-6:]
        if str(item.get("role", "")) == "user"
    ]
    if recent_user_messages:
        return " / ".join(recent_user_messages[-2:])
    return _short_text(user_message, 60)


def _short_text(value: Optional[str], limit: int = MAX_HISTORY_TEXT_LENGTH) -> str:
    """截断长文本，控制上下文与日志体积。"""
    text = (value or "").strip()
    if len(text) <= limit:
        return text
    return f"{text[:limit]}..."


def _compact_history(
    session_history: List[Dict[str, str]],
    limit: int,
) -> List[Dict[str, str]]:
    """压缩会话历史，只保留近期若干条并截断文本。"""
    compacted: List[Dict[str, str]] = []
    for item in session_history[-limit:]:
        role = str(item.get("role", ""))
        if role not in {"user", "assistant"}:
            continue
        compacted.append(
            {
                "role": role,
                "content": _short_text(str(item.get("content", ""))),
            }
        )
    return compacted


def _log_stage_input(stage: str, turn_id: str, payload: Dict[str, Any]) -> None:
    """记录模型输入（单次，不重复）。"""
    logger.info(
        "[TravelAgent][MODEL_INPUT][%s][%s] %s",
        stage,
        turn_id,
        json.dumps(payload, ensure_ascii=False, default=str),
    )


def _log_stage_output(stage: str, turn_id: str, payload: Dict[str, Any]) -> None:
    """记录模型输出（包含原始文本与结构化附加信息）。"""
    logger.info(
        "[TravelAgent][MODEL_OUTPUT][%s][%s] %s",
        stage,
        turn_id,
        json.dumps(payload, ensure_ascii=False, default=str),
    )


def _log_stage_error(stage: str, turn_id: str, error_text: str) -> None:
    """记录模型调用错误。"""
    if not error_text:
        return
    logger.info("[TravelAgent][MODEL_ERROR][%s][%s] %s", stage, turn_id, error_text)


def _collect_response_meta(response: Any) -> Dict[str, Any]:
    """提取模型原始响应中的可用元信息（兼容不同后端对象结构）。"""
    if response is None:
        return {}

    meta: Dict[str, Any] = {
        "response_type": type(response).__name__,
    }

    for field_name in (
        "id",
        "name",
        "model",
        "usage",
        "usage_reason",
        "response_metadata",
        "usage_metadata",
        "additional_kwargs",
        "tool_calls",
        "invalid_tool_calls",
    ):
        if hasattr(response, field_name):
            try:
                meta[field_name] = getattr(response, field_name)
            except Exception:
                meta[field_name] = "<unavailable>"

    return meta


def _extract_token_usage_info(response: Any) -> Tuple[Optional[Dict[str, Any]], str, Optional[str]]:
    """提取真实 token 统计，不做估算；若缺失则返回明确原因。"""
    if response is None:
        return None, "none", "response_is_none"

    usage = getattr(response, "usage", None)
    if isinstance(usage, dict) and usage:
        return usage, "response.usage", None

    usage_metadata = getattr(response, "usage_metadata", None)
    if isinstance(usage_metadata, dict) and usage_metadata:
        return usage_metadata, "response.usage_metadata", None

    response_metadata = getattr(response, "response_metadata", None)
    if isinstance(response_metadata, dict):
        for key in ("usage", "token_usage"):
            nested = response_metadata.get(key)
            if isinstance(nested, dict) and nested:
                return nested, f"response.response_metadata.{key}", None

    usage_reason = getattr(response, "usage_reason", None)
    if usage_reason:
        return None, "none", str(usage_reason)

    return (
        None,
        "none",
        "No token usage found in response.usage / response.usage_metadata / response.response_metadata.usage",
    )


def _log_trace(turn_id: str, label: str, payload: Dict[str, Any], enabled: bool) -> None:
    """超详细追踪日志，仅在 debug_trace=true 时输出。"""
    if not enabled:
        return
    logger.info(
        "[TravelAgent][TRACE][%s][%s] %s",
        turn_id,
        label,
        json.dumps(payload, ensure_ascii=False, default=str),
    )


def _compact_json(value: Dict[str, Any]) -> str:
    """紧凑JSON序列化，降低提示词噪音。"""
    return json.dumps(value, ensure_ascii=False, separators=(",", ":"), default=str)


def _build_strategy_model_payload(strategy_context: Dict[str, Any]) -> Dict[str, Any]:
    """构建策略层最小输入上下文。"""
    recent_history = strategy_context.get("recent_history", [])
    compact_recent: List[Dict[str, str]] = []
    if isinstance(recent_history, list):
        for item in recent_history[-3:]:
            if not isinstance(item, dict):
                continue
            role = str(item.get("role", "")).strip()
            content = _short_text(str(item.get("content", "")), 60)
            if role and content:
                compact_recent.append({"role": role, "content": content})

    capability_status = strategy_context.get("capability_status", {})
    if not isinstance(capability_status, dict):
        capability_status = {}

    return {
        "message": _short_text(str(strategy_context.get("message", "")), 100),
        "session_goal": _short_text(str(strategy_context.get("session_goal", "")), 80),
        "recent_history": compact_recent,
        "capability_status": {
            "has_location": bool(capability_status.get("has_location")),
            "has_weather": bool(capability_status.get("has_weather")),
        },
    }


def _build_response_model_payload(model_context: Dict[str, Any]) -> Dict[str, Any]:
    """构建回复层最小输入上下文，减少结构化噪音。"""
    payload: Dict[str, Any] = {
        "message": _short_text(str(model_context.get("message", "")), 120),
    }

    recent_history = model_context.get("recent_history", [])
    if isinstance(recent_history, list):
        compact_recent: List[Dict[str, str]] = []
        for item in recent_history[-3:]:
            if not isinstance(item, dict):
                continue
            role = str(item.get("role", "")).strip()
            content = _short_text(str(item.get("content", "")), 70)
            if role and content:
                compact_recent.append({"role": role, "content": content})
        if compact_recent:
            payload["recent_history"] = compact_recent

    strategy = model_context.get("strategy", {})
    if isinstance(strategy, dict):
        payload["strategy"] = {
            "intent": str(strategy.get("intent", "")),
            "user_goal": _short_text(str(strategy.get("user_goal", "")), 80),
            "response_tone": str(strategy.get("response_tone", "friendly")),
            "history_relevance": str(strategy.get("history_relevance", "medium")),
        }

    execution_data = model_context.get("execution_data", {})
    if isinstance(execution_data, dict):
        compact_exec: Dict[str, Any] = {}

        location = execution_data.get("location")
        if isinstance(location, dict):
            compact_exec["location"] = {
                "state": location.get("state"),
                "city": location.get("city"),
                "county": location.get("county"),
                "area_name": location.get("area_name"),
            }

        weather = execution_data.get("weather")
        if isinstance(weather, dict):
            compact_exec["weather"] = {
                "temperature": weather.get("temperature"),
                "description": weather.get("description"),
                "weather_name": weather.get("weather_name"),
            }

        poi_candidates = execution_data.get("poi_candidates")
        if isinstance(poi_candidates, list) and poi_candidates:
            compact_pois: List[str] = []
            for item in poi_candidates[:MAX_INLINE_POI_REPLY_COUNT]:
                if not isinstance(item, dict):
                    continue
                name = str(item.get("name") or "").strip()
                poi_type = str(item.get("type") or "").strip()
                distance = item.get("distance_hint")
                if name:
                    if poi_type:
                        compact_pois.append(f"{name}({poi_type})@{distance}")
                    else:
                        compact_pois.append(f"{name}@{distance}")
            if compact_pois:
                compact_exec["poi_candidates"] = compact_pois

        for numeric_field in ("poi_total_count", "poi_display_count"):
            value = execution_data.get(numeric_field)
            if isinstance(value, int):
                compact_exec[numeric_field] = value

        oversized = execution_data.get("ask_confirmation_for_large_result")
        if isinstance(oversized, bool):
            compact_exec["ask_confirmation_for_large_result"] = oversized

        full_output_requested = execution_data.get("full_output_requested")
        if isinstance(full_output_requested, bool):
            compact_exec["full_output_requested"] = full_output_requested

        auto_continue_full_poi_batches = execution_data.get("auto_continue_full_poi_batches")
        if isinstance(auto_continue_full_poi_batches, bool):
            compact_exec["auto_continue_full_poi_batches"] = auto_continue_full_poi_batches

        full_output_batch_count = execution_data.get("full_output_batch_count")
        if isinstance(full_output_batch_count, int):
            compact_exec["full_output_batch_count"] = full_output_batch_count

        route_plan = execution_data.get("route_plan")
        if isinstance(route_plan, dict):
            compact_exec["route_plan"] = {
                "summary": _short_text(str(route_plan.get("summary", "")), 80),
                "route_points": list(route_plan.get("route_points", []))[:5],
            }

        if compact_exec:
            payload["execution_data"] = compact_exec

    return payload


def _select_response_history(
    session_history: List[Dict[str, str]],
    strategy: DialogStrategy,
) -> List[Dict[str, str]]:
    """根据策略层给出的历史相关性裁剪历史，降低跨话题污染。"""
    relevance = str(strategy.history_relevance or "medium").strip().lower()
    if relevance == "low":
        return _compact_history(session_history, 2)
    if relevance == "high":
        return _compact_history(session_history, MAX_HISTORY_FOR_RESPONSE)
    return _compact_history(session_history, 4)


def _build_response_model_context(
    user_message: str,
    strategy: DialogStrategy,
    search_preference: PoiSearchPreference,
    weather_summary: WeatherSummary,
    request_data: TravelChatInput,
    route_plan: Optional[RoutePlan],
    poi_items: List[Dict[str, Any]],
    session_history: List[Dict[str, str]],
    poi_total_count: Optional[int] = None,
    ask_confirmation_for_large_result: bool = False,
    full_output_requested: bool = False,
    auto_continue_full_poi_batches: bool = AUTO_CONTINUE_FULL_POI_BATCHES,
    full_output_batch_count: int = 0,
) -> Dict[str, Any]:
    """构建回复层最小上下文：当前句+近期历史+策略+执行结果。"""
    execution_data: Dict[str, Any] = {}

    if strategy.should_use_location_context or strategy.should_search_poi or strategy.should_plan_route:
        execution_data["location"] = {
            "state": request_data.state,
            "city": request_data.city,
            "county": request_data.county,
            "area_name": request_data.area_name,
            "x": request_data.x,
            "y": request_data.y,
        }

    has_weather = any(
        [
            weather_summary.temperature is not None,
            bool(weather_summary.weather_description),
            bool(weather_summary.weather_name),
        ]
    )
    if has_weather and (strategy.should_use_location_context or strategy.should_plan_route):
        execution_data["weather"] = {
            "temperature": weather_summary.temperature,
            "description": weather_summary.weather_description,
            "weather_name": weather_summary.weather_name,
        }

    if strategy.should_search_poi or strategy.should_plan_route:
        execution_data["poi_candidates"] = [
            {
                "name": _poi_name(item),
                "type": _poi_type(item),
                "distance_hint": _poi_distance_hint(
                    item,
                    request_data.x or 0.0,
                    request_data.y or 0.0,
                ),
            }
            for item in poi_items[:MAX_INLINE_POI_REPLY_COUNT]
        ]
        if poi_total_count is not None:
            execution_data["poi_total_count"] = int(max(0, poi_total_count))
            execution_data["poi_display_count"] = len(poi_items)
            execution_data["ask_confirmation_for_large_result"] = bool(
                ask_confirmation_for_large_result
            )
            execution_data["full_output_requested"] = bool(full_output_requested)
            execution_data["auto_continue_full_poi_batches"] = bool(
                auto_continue_full_poi_batches
            )
            if full_output_batch_count > 0:
                execution_data["full_output_batch_count"] = int(full_output_batch_count)

    if route_plan is not None:
        execution_data["route_plan"] = {
            "summary": route_plan.summary,
            "estimated_duration_hours": route_plan.estimated_duration_hours,
            "route_points": [point.name for point in route_plan.route_points],
        }

    recent_history = _select_response_history(session_history, strategy)

    context_payload: Dict[str, Any] = {
        "message": user_message,
        "recent_history": recent_history,
        "strategy": {
            "intent": strategy.intent,
            "user_goal": strategy.user_goal,
            "response_tone": strategy.response_tone,
            ##"followup_question": strategy.followup_question,
            "retrieval_plan": {
                "target_poi_count": strategy.target_poi_count,
                "min_required_poi": strategy.min_required_poi,
            },
            "semantic_preference": {
                "has_semantic_target": search_preference.has_semantic_target,
                "goal_hint": search_preference.goal_hint,
                "must_find": search_preference.must_find,
                "target_labels": search_preference.target_labels[:8],
            },
            "history_relevance": strategy.history_relevance,
        },
        "execution_data": execution_data,
    }

    return context_payload


def _build_strategy_prompt(strategy_context: Dict[str, Any]) -> str:
    """构建简洁策略提示词。"""
    model_payload = _build_strategy_model_payload(strategy_context)
    return (
        "你是对话策略分析器。只输出JSON，不要解释。\n"
        "字段：intent,user_goal,should_use_location_context,should_search_poi,should_plan_route,"
        "requested_poi_count,target_poi_count,min_required_poi,search_keywords,search_scope,"
        "history_relevance,response_tone,confidence。\n"
        "规则："
        "1) 非建筑查询时 requested_poi_count/target_poi_count/min_required_poi 用 null；"
        "2) search_scope 仅 local/expanded，默认 local，只有用户明确要求扩大范围时 expanded；"
        "3) history_relevance 仅 low/medium/high，新问候或新话题用 low；"
        "4) search_keywords 必须是数组（例如 [\"餐厅\",\"吃饭\",\"restaurant\"]），不能是字符串；"
        "5) search_keywords 用语义词，不抄整句，尽量同时给中文和英文同义词。\n"
        f"输入：{_compact_json(model_payload)}"
    )


def _build_response_prompt(model_context: Dict[str, Any]) -> str:
    """构建回复层用户提示词（避免与system重复）。"""
    model_payload = _build_response_model_payload(model_context)
    strategy = model_context.get("strategy", {})
    intent = str(strategy.get("intent", "")).strip().lower()
    is_poi_query = intent in {
        "search_nearby_poi",
        "nearby_poi",
        "find_poi",
        "poi_search",
        "building_query",
        "building_comprehensive_search",
        "comprehensive_poi_search",
        "full_poi_list",
        "list_all_buildings",
        "list_all_poi",
    }

    extra_instruction = ""
    if is_poi_query:
        extra_instruction = (
            "\n这是建筑查询场景：请直接给建筑结果，不要寒暄；"
            "每个建筑名都要写成（建筑名）格式；"
            "先理解用户目的语义（如吃饭/理发/住宿等），优先给语义最相关的3-5个候选；"
            "若没有完全匹配，给出最接近选项并说明可继续扩大范围；"
            "不要使用固定模板句式，输出要自然。"
            "当用户询问建筑距离时，请直接输出该建筑距当前位置的具体距离"
            "当一定范围内没有找到最精确的建筑，但却返回了建筑时要说明原因"
        )

    execution_data = model_context.get("execution_data", {})
    if isinstance(execution_data, dict):
        total_count = execution_data.get("poi_total_count")
        display_count = execution_data.get("poi_display_count")
        ask_confirmation = bool(execution_data.get("ask_confirmation_for_large_result"))
        full_output_requested = bool(execution_data.get("full_output_requested"))
        auto_continue_batches = bool(execution_data.get("auto_continue_full_poi_batches"))
        full_output_batch_count = execution_data.get("full_output_batch_count")
        if isinstance(total_count, int) and isinstance(display_count, int):
            if full_output_requested and total_count <= display_count:
                extra_instruction += (
                    "\n用户已明确要求全部列出。"
                    "请把当前列表中的建筑名逐个列出，不要用“若干/多栋/多处”等概括词，"
                    "也不要再次反问用户。"
                )
            elif (
                full_output_requested
                and auto_continue_batches
                and isinstance(full_output_batch_count, int)
                and full_output_batch_count > 1
            ):
                extra_instruction += (
                    "\n用户已明确要求全部列出，系统已启用批次续传。"
                    f"本轮应连续输出{full_output_batch_count}批，不要再次反问用户。"
                )
            if ask_confirmation and total_count > display_count:
                extra_instruction += (
                    "\n本轮命中结果较多。"
                    f"当前命中总量约{total_count}个，仅展示了{display_count}个。"
                    "请先展示当前这批具体建筑名，"
                    "请先明确告知用户总量，并自然询问用户是要全量输出（分批）、"
                    "指定数量，还是按条件筛选。"
                )
            elif total_count > display_count:
                extra_instruction += (
                    "\n请明确说明：已命中总量与当前展示数量，"
                    "不要用“多处”“若干”等模糊词。"
                )

        semantic_no_match = execution_data.get("semantic_no_match")
        if isinstance(semantic_no_match, dict):
            covered_km = semantic_no_match.get("covered_km")
            next_km = semantic_no_match.get("next_km")
            allow_extended = bool(semantic_no_match.get("allow_extended_search"))
            if allow_extended and next_km is not None:
                extra_instruction += (
                    "\n当前语义检索未命中："
                    f"已覆盖约{covered_km}公里。"
                    f"请自然询问用户是否继续扩大到约{next_km}公里再检索。"
                    "不要使用固定模板。"
                )
            else:
                extra_instruction += (
                    "\n当前语义检索未命中："
                    f"已覆盖约{covered_km}公里。"
                    "请自然询问用户是否继续扩大范围。"
                    "不要使用固定模板。"
                )

    return (
        "先回答当前消息。仅在确有承接关系时引用历史；若冲突则忽略历史。"
        "回复自然、简洁、可执行。\n"
        f"输入：{_compact_json(model_payload)}"
        f"{extra_instruction}"
    )


def _collect_wrapped_poi_names(poi_items: List[Dict[str, Any]]) -> List[str]:
    """提取去重后的建筑名，并统一加中文括号。"""
    names: List[str] = []
    for item in poi_items:
        if not isinstance(item, dict):
            continue
        raw_name = str(item.get("name") or "").strip()
        if not raw_name:
            continue
        wrapped = f"（{raw_name}）"
        if wrapped not in names:
            names.append(wrapped)
    return names


def _build_full_poi_batch_reply(
    poi_items: List[Dict[str, Any]],
    total_count: Optional[int] = None,
    batch_size: int = MAX_INLINE_POI_REPLY_COUNT,
) -> Optional[str]:
    """用户已明确要全部结果时，按批次连续输出，避免再次询问。"""
    names = _collect_wrapped_poi_names(poi_items)
    if not names:
        return None

    safe_batch_size = max(1, int(batch_size or MAX_INLINE_POI_REPLY_COUNT))
    visible_count = len(names)
    reported_total = total_count if isinstance(total_count, int) and total_count > 0 else visible_count
    batch_count = int(math.ceil(visible_count / safe_batch_size))

    lines: List[str] = [
        f"你要全部列出，我按每批最多{safe_batch_size}个连续输出。",
        f"本次共命中约{reported_total}个建筑。",
    ]
    if reported_total != visible_count:
        lines.append(f"当前可输出{visible_count}个已取回建筑。")

    for batch_index in range(batch_count):
        start_index = batch_index * safe_batch_size
        end_index = min(start_index + safe_batch_size, visible_count)
        batch_names = names[start_index:end_index]
        lines.append(
            f"第{batch_index + 1}/{batch_count}批（{start_index + 1}-{end_index}）："
            + "、".join(batch_names)
            + "。"
        )

    lines.append("以上为当前命中的全部建筑。")
    return "".join(lines)


def _build_concise_poi_reply(model_context: Dict[str, Any]) -> Optional[str]:
    """建筑查询的简洁兜底回复：每个建筑名强制括号化，避免闲聊。"""
    execution_data = model_context.get("execution_data")
    if not isinstance(execution_data, dict):
        return None

    poi_candidates = execution_data.get("poi_candidates")
    if not isinstance(poi_candidates, list) or not poi_candidates:
        return None

    names = _collect_wrapped_poi_names(poi_candidates)

    if not names:
        return None

    execution_data = model_context.get("execution_data")
    poi_total_count = None
    ask_confirmation = False
    full_output_requested = False
    if isinstance(execution_data, dict):
        raw_total = execution_data.get("poi_total_count")
        if isinstance(raw_total, int):
            poi_total_count = raw_total
        ask_confirmation = bool(execution_data.get("ask_confirmation_for_large_result"))
        full_output_requested = bool(execution_data.get("full_output_requested"))

    top_items = names[:MAX_INLINE_POI_REPLY_COUNT]
    strategy = model_context.get("strategy", {})
    semantic_preference = strategy.get("semantic_preference", {}) if isinstance(strategy, dict) else {}
    goal_hint = str(semantic_preference.get("goal_hint") or "").strip()
    must_find = bool(semantic_preference.get("must_find"))

    lines: List[str] = []
    if goal_hint and len(goal_hint) <= 10:
        lines.append(f"我先按你这轮“{goal_hint}”的需求在附近筛了一轮。")
    else:
        lines.append("我先按你这轮需求在附近筛了一圈。")

    if poi_total_count is not None:
        lines.append(f"本次共命中约{poi_total_count}个建筑。")

    lines.append("本轮先展示：" + "、".join(top_items) + "。")

    if (
        ask_confirmation
        and not full_output_requested
        and poi_total_count is not None
        and poi_total_count > len(top_items)
    ):
        lines.append("结果较多，你要我全量分批输出，还是先按你指定数量继续？")

    if must_find:
        lines.append("你这轮是必须找到的话，我会继续按更大范围补查，直到给你可用结果。")
    else:
        lines.append("如果这批里没有你想去的，我再按你的要求扩大一点范围继续找。")

    return "".join(lines)


def _is_full_coverage_intent(strategy: DialogStrategy) -> bool:
    """判断是否为全量覆盖检索意图（基于策略层语义意图）。"""
    intent = str(strategy.intent or "").strip().lower()
    return intent in {
        "building_comprehensive_search",
        "comprehensive_poi_search",
        "full_poi_list",
        "list_all_buildings",
        "list_all_poi",
    }


def _fetch_poi_for_fixed_radius(
    x: float,
    y: float,
    radius: int,
    search_preference: PoiSearchPreference,
) -> Tuple[List[Dict[str, Any]], int, List[Dict[str, Any]], Optional[str]]:
    """按指定半径单次检索，返回该范围内完整候选结果。"""
    attempts: List[Dict[str, Any]] = []
    try:
        poi_response = requests.get(
            POI_API_URL,
            params={
                "x": x,
                "y": y,
                "radius": int(radius),
                "in_game": "true",
            },
            timeout=8,
        )
        poi_response.raise_for_status()
        poi_data = poi_response.json()
        poi_items = _normalize_poi_items(poi_data)

        if search_preference.has_semantic_target:
            exact_items = [
                item
                for item in poi_items
                if _is_exact_type_or_name_match(item, search_preference)
            ]
            semantic_items = [
                item
                for item in poi_items
                if _poi_semantic_score(item, search_preference) > 0
            ]
            selected_items = exact_items or semantic_items
            attempts.append(
                {
                    "radius": int(radius),
                    "poi_count": len(poi_items),
                    "exact_match_count": len(exact_items),
                    "semantic_count": len(semantic_items),
                }
            )
            return selected_items, int(radius), attempts, None

        attempts.append(
            {
                "radius": int(radius),
                "poi_count": len(poi_items),
            }
        )
        return poi_items, int(radius), attempts, None
    except Exception as exception:
        error_text = str(exception)
        attempts.append({"radius": int(radius), "poi_count": 0, "error": error_text})
        return [], int(radius), attempts, error_text


def _decide_poi_presentation_with_llm(
    user_message: str,
    strategy: DialogStrategy,
    total_count: int,
    session_history: List[Dict[str, str]],
    turn_id: str,
    trace_enabled: bool,
) -> Tuple[PoiPresentationDecision, bool, Optional[str]]:
    """使用LLM理解用户期望展示规模，不依赖关键词规则。"""
    llm = _get_travel_llm()
    if llm is None:
        default_mode = "all" if total_count <= MAX_INLINE_POI_REPLY_COUNT else "summary"
        return (
            PoiPresentationDecision(
                mode=default_mode,
                requested_count=min(DEFAULT_INLINE_POI_REPLY_COUNT, max(1, total_count)),
                user_wants_all=default_mode == "all",
                need_confirmation_when_oversized=True,
            ),
            False,
            _TRAVEL_LLM_INIT_ERROR,
        )

    recent_history = _compact_history(session_history, 4)
    prompt_payload = {
        "message": _short_text(user_message, 120),
        "recent_history": recent_history,
        "intent": strategy.intent,
        "user_goal": _short_text(strategy.user_goal, 120),
        "total_count": total_count,
        "max_inline": MAX_INLINE_POI_REPLY_COUNT,
        "preview_count": PREVIEW_POI_REPLY_COUNT,
    }
    prompt = (
        "你是结果展示决策器。只输出JSON，不要解释。"
        "根据用户当前表达和最近历史，判断用户这轮是要全量、指定数量、还是仅摘要。"
        "不要做关键词匹配，必须结合语义与上下文。"
        "当命中结果较多时，不要直接全量铺开；先走预览（约6-7条）并询问用户要输出多少。"
        "输出字段：mode,requested_count,user_wants_all,need_confirmation_when_oversized。"
        "其中 mode 只能是 all/count/summary。"
        f"输入：{_compact_json(prompt_payload)}"
    )

    try:
        messages = [
            {"role": "system", "content": "你是严谨的展示决策器。"},
            {"role": "user", "content": prompt},
        ]
        _log_trace(
            turn_id=turn_id,
            label="presentation_before_invoke",
            payload={"messages": messages},
            enabled=trace_enabled,
        )
        response = llm.invoke(messages)
        raw_text = str(getattr(response, "content", "") or "").strip()
        _log_trace(
            turn_id=turn_id,
            label="presentation_after_invoke",
            payload={"raw_content": raw_text, "response_repr": repr(response)},
            enabled=trace_enabled,
        )
        json_block = _extract_first_json_block(raw_text)
        if not isinstance(json_block, dict):
            return PoiPresentationDecision(), True, "presentation_parse_failed"

        mode = str(json_block.get("mode") or "summary").strip().lower()
        if mode not in {"all", "count", "summary"}:
            mode = "summary"

        requested_count = json_block.get("requested_count")
        normalized_count: Optional[int] = None
        if requested_count is not None:
            try:
                normalized_count = max(1, int(requested_count))
            except Exception:
                normalized_count = None

        decision = PoiPresentationDecision(
            mode=mode,
            requested_count=normalized_count,
            user_wants_all=bool(json_block.get("user_wants_all")),
            need_confirmation_when_oversized=bool(
                json_block.get("need_confirmation_when_oversized", True)
            ),
        )
        return decision, True, None
    except Exception as exception:
        return PoiPresentationDecision(), True, str(exception)


def _decide_dialog_strategy(
    user_message: str,
    session_history: List[Dict[str, str]],
    request_data: TravelChatInput,
    turn_id: str,
    trace_enabled: bool,
) -> Tuple[DialogStrategy, bool, Optional[str]]:
    """用LLM判定当前轮对话策略，避免固定模板回复。"""
    llm = _get_travel_llm()
    if llm is None:
        return _build_default_strategy(user_message), False, _TRAVEL_LLM_INIT_ERROR

    strategy_context = {
        "message": user_message,
        "recent_history": _compact_history(session_history, MAX_HISTORY_FOR_STRATEGY),
        "session_goal": _infer_session_goal(session_history, user_message),
        
        "capability_status": {
            "has_location": request_data.x is not None and request_data.y is not None,
            "has_weather": any(
                [
                    request_data.temperature is not None,
                    bool(request_data.weather_name),
                    bool(request_data.weather_description),
                ]
            ),
        },
    }
    prompt = _build_strategy_prompt(strategy_context)

    try:
        messages = [
            {"role": "system", "content": "你是严谨的对话策略分析器。"},
            {"role": "user", "content": prompt},
        ]
        _log_stage_input(
            stage="strategy",
            turn_id=turn_id,
            payload={
                "user_message": user_message,
                "context_overview": {
                    "history_count": len(strategy_context.get("recent_history", [])),
                    "has_location": strategy_context.get("capability_status", {}).get("has_location"),
                    "has_weather": strategy_context.get("capability_status", {}).get("has_weather"),
                },
                "prompt_length": len(prompt),
            },
        )
        _log_trace(
            turn_id=turn_id,
            label="strategy_before_invoke",
            payload={
                "messages": messages,
                "message_count": len(messages),
            },
            enabled=trace_enabled,
        )
        start_time = time.perf_counter()
        response = llm.invoke(messages)
        elapsed_ms = round((time.perf_counter() - start_time) * 1000.0, 3)
        raw_text = str(getattr(response, "content", "") or "").strip()
        token_usage, token_usage_source, token_absent_reason = _extract_token_usage_info(response)
        _log_stage_output(
            stage="strategy",
            turn_id=turn_id,
            payload={
                "content": raw_text,
                "response_repr": repr(response),
                "elapsed_ms": elapsed_ms,
                "token_usage": token_usage,
                "token_usage_source": token_usage_source,
                "token_absent_reason": token_absent_reason,
            },
        )
        _log_trace(
            turn_id=turn_id,
            label="strategy_after_invoke",
            payload={
                "elapsed_ms": elapsed_ms,
                "token_usage": token_usage,
                "token_usage_source": token_usage_source,
                "token_absent_reason": token_absent_reason,
                "response_meta": _collect_response_meta(response),
                "response_repr": repr(response),
                "raw_content": raw_text,
            },
            enabled=trace_enabled,
        )
        strategy_json = _extract_first_json_block(raw_text)
        if not strategy_json:
            return _build_default_strategy(user_message), True, "strategy_parse_failed"

        _log_stage_output(
            stage="strategy_parsed",
            turn_id=turn_id,
            payload={
                "json": strategy_json,
            },
        )

        strategy = DialogStrategy.model_validate(_sanitize_strategy_json(strategy_json))
        _resolve_retrieval_plan(strategy, request_data)
        return strategy, True, None
    except Exception as exception:
        _log_stage_error(stage="strategy", turn_id=turn_id, error_text=str(exception))
        fallback = _build_default_strategy(user_message)
        _resolve_retrieval_plan(fallback, request_data)
        return fallback, True, str(exception)


def _build_dialog_recommendation(
    route_plan: Optional[RoutePlan],
    strategy: DialogStrategy,
) -> Dict[str, Any]:
    """统一 recommendation 输出，兼容非规划型对话。"""
    if route_plan is not None:
        return {
            "summary": route_plan.summary,
            "strategy": route_plan.strategy,
            "estimated_duration_hours": route_plan.estimated_duration_hours,
            "route_points": [point.model_dump() for point in route_plan.route_points],
        }

    return {
        "summary": "当前以对话沟通为主，尚未进入正式路线规划。",
        "strategy": strategy.user_goal or "先确认你的偏好，再决定是否规划路线。",
        "estimated_duration_hours": 0.0,
        "route_points": [],
    }


def _fallback_reply(
    user_message: str,
    strategy: DialogStrategy,
    route_plan: Optional[RoutePlan],
    has_location: bool,
) -> str:
    """LLM不可用时的降级回复。"""
    _ = strategy, route_plan, has_location

    message = (user_message or "").strip()
    if not message:
        return "你好，我在。"
    return "收到，我会按你这轮需求继续处理。"


def _generate_agent_reply(
    user_message: str,
    model_context: Dict[str, Any],
    strategy: DialogStrategy,
    route_plan: Optional[RoutePlan],
    has_location: bool,
    turn_id: str,
    trace_enabled: bool,
) -> Tuple[str, bool, Optional[str]]:
    """生成本轮回复（优先回应用户当前意图）。"""
    concise_poi_reply: Optional[str] = None
    if strategy.should_search_poi and not strategy.should_plan_route:
        concise_poi_reply = _build_concise_poi_reply(model_context)

    llm = _get_travel_llm()
    if llm is None:
        if concise_poi_reply:
            return concise_poi_reply, False, "poi_concise_reply_fallback"
        return _fallback_reply(user_message, strategy, route_plan, has_location), False, _TRAVEL_LLM_INIT_ERROR

    try:
        prompt = _build_response_prompt(model_context)
        messages = [
            {"role": "system", "content": TRAVEL_RESPONSE_SYSTEM_PROMPT},
            {"role": "user", "content": prompt},
        ]
        _log_stage_input(
            stage="response",
            turn_id=turn_id,
            payload={
                "user_message": user_message,
                "context_overview": {
                    "intent": strategy.intent,
                    "history_relevance": strategy.history_relevance,
                    "has_execution_data": bool(model_context.get("execution_data")),
                },
                "prompt_length": len(prompt),
            },
        )
        _log_trace(
            turn_id=turn_id,
            label="response_before_invoke",
            payload={
                "messages": messages,
                "message_count": len(messages),
            },
            enabled=trace_enabled,
        )
        start_time = time.perf_counter()
        response = llm.invoke(messages)
        elapsed_ms = round((time.perf_counter() - start_time) * 1000.0, 3)
        content = str(getattr(response, "content", "") or "").strip()
        token_usage, token_usage_source, token_absent_reason = _extract_token_usage_info(response)
        _log_stage_output(
            stage="response",
            turn_id=turn_id,
            payload={
                "content": content,
                "response_repr": repr(response),
                "elapsed_ms": elapsed_ms,
                "token_usage": token_usage,
                "token_usage_source": token_usage_source,
                "token_absent_reason": token_absent_reason,
            },
        )
        _log_trace(
            turn_id=turn_id,
            label="response_after_invoke",
            payload={
                "elapsed_ms": elapsed_ms,
                "token_usage": token_usage,
                "token_usage_source": token_usage_source,
                "token_absent_reason": token_absent_reason,
                "response_meta": _collect_response_meta(response),
                "response_repr": repr(response),
                "raw_content": content,
            },
            enabled=trace_enabled,
        )
        if content:
            return content, True, None
        if concise_poi_reply:
            return concise_poi_reply, False, "poi_concise_reply_after_empty"
        return _fallback_reply(user_message, strategy, route_plan, has_location), False, "llm_empty_response"
    except Exception as exception:
        _log_stage_error(stage="response", turn_id=turn_id, error_text=str(exception))
        if concise_poi_reply:
            return concise_poi_reply, False, "poi_concise_reply_after_error"
        return _fallback_reply(user_message, strategy, route_plan, has_location), False, str(exception)


def _resolve_input(raw: Dict[str, Any]) -> Tuple[Optional[TravelChatInput], Optional[Dict[str, Any]]]:
    try:
        request_data = TravelChatInput.model_validate(raw)
    except ValidationError as validation_error:
        return None, {
            "success": False,
            "error": "输入参数校验失败",
            "detail": json.loads(validation_error.json()),
        }

    if request_data.x is None and request_data.message:
        request_data.x = _extract_float_from_text(request_data.message, "x")
    if request_data.y is None and request_data.message:
        request_data.y = _extract_float_from_text(request_data.message, "y")
    if (not request_data.state or not request_data.city) and request_data.message:
        parsed_state, parsed_city = _extract_state_city_from_text(request_data.message)
        request_data.state = request_data.state or parsed_state
        request_data.city = request_data.city or parsed_city
    if request_data.radius is None and request_data.message:
        request_data.radius = _extract_radius_from_text(request_data.message)

    _normalize_weather_fields(request_data)
    if not request_data.county and request_data.area_name:
        request_data.county = request_data.area_name

    if not request_data.city and request_data.county:
        request_data.city = request_data.county
    request_data.state = request_data.state or "UnknownState"
    request_data.city = request_data.city or "UnknownCity"

    return request_data, None


def _build_session_id(request_data: TravelChatInput) -> str:
    """生成或复用会话ID。"""
    if request_data.session_id:
        return str(request_data.session_id)
    return f"travel#{uuid.uuid4()}"


def _build_radius_sequence(initial_radius: Optional[int], max_radius: int) -> List[int]:
    """根据初始半径构建递增搜索序列。"""
    if initial_radius is None:
        start = AUTO_RADIUS_START
    else:
        start = max(100, int(initial_radius))

    max_radius = max(start, int(max_radius))

    radii: List[int] = []
    current = start
    while current <= max_radius:
        radii.append(current)
        current += AUTO_RADIUS_STEP

    if max_radius not in radii:
        radii.append(max_radius)

    deduped = []
    seen = set()
    for radius in radii:
        if radius not in seen:
            deduped.append(radius)
            seen.add(radius)
    return deduped


def _fetch_poi_with_adaptive_radius(
    x: float,
    y: float,
    initial_radius: Optional[int],
    target_count: int,
    min_required_count: int,
    search_preference: PoiSearchPreference,
) -> Tuple[List[Dict[str, Any]], int, List[Dict[str, Any]], Optional[str]]:
    """按半径递增调用POI接口，优先满足目标数量，不足时回退最优可用结果。"""
    attempts: List[Dict[str, Any]] = []
    best_items: List[Dict[str, Any]] = []
    best_exact_items: List[Dict[str, Any]] = []
    best_semantic_items: List[Dict[str, Any]] = []
    best_items_radius = initial_radius or AUTO_RADIUS_START
    best_exact_radius = initial_radius or AUTO_RADIUS_START
    best_semantic_radius = initial_radius or AUTO_RADIUS_START
    llm_semantic_evaluated = False
    last_error: Optional[str] = None
    max_radius = AUTO_RADIUS_MAX if search_preference.allow_extended_search else AUTO_RADIUS_LOCAL_MAX
    if search_preference.allow_extended_search:
        stage_base = max(100, int(initial_radius or AUTO_RADIUS_START))
        max_radius = min(max_radius, stage_base + AUTO_RADIUS_EXPANDED_STAGE_SPAN)

    for radius in _build_radius_sequence(initial_radius, max_radius=max_radius):
        try:
            poi_response = requests.get(
                POI_API_URL,
                params={
                    "x": x,
                    "y": y,
                    "radius": radius,
                    "in_game": "true",
                },
                timeout=8,
            )
            poi_response.raise_for_status()
            poi_data = poi_response.json()
            poi_items = _normalize_poi_items(poi_data)
            exact_items = [
                item for item in poi_items if _is_exact_type_or_name_match(item, search_preference)
            ]
            semantic_items = [
                item for item in poi_items if _poi_semantic_score(item, search_preference) > 0
            ]
            attempts.append(
                {
                    "radius": radius,
                    "poi_count": len(poi_items),
                    "exact_match_count": len(exact_items),
                    "semantic_count": len(semantic_items),
                }
            )
            if len(poi_items) > len(best_items):
                best_items = poi_items
                best_items_radius = radius

            if len(exact_items) > len(best_exact_items):
                best_exact_items = exact_items
                best_exact_radius = radius

            if len(semantic_items) > len(best_semantic_items):
                best_semantic_items = semantic_items
                best_semantic_radius = radius

            if search_preference.has_semantic_target and len(exact_items) >= min_required_count:
                return exact_items, radius, attempts, None

            if search_preference.has_semantic_target and len(semantic_items) >= min_required_count:
                return semantic_items, radius, attempts, None

            if (not search_preference.has_semantic_target) and len(poi_items) >= target_count:
                return poi_items, radius, attempts, None

            if (
                search_preference.has_semantic_target
                and (not search_preference.allow_extended_search)
                and radius >= AUTO_SEMANTIC_EARLY_PROBE_RADIUS
                and (not best_exact_items)
                and (not best_semantic_items)
            ):
                break
        except Exception as exception:
            last_error = str(exception)
            attempts.append({"radius": radius, "poi_count": 0, "error": last_error})

    final_radius = attempts[-1]["radius"] if attempts else (initial_radius or AUTO_RADIUS_START)

    if search_preference.has_semantic_target and best_exact_items:
        return best_exact_items, int(best_exact_radius), attempts, None

    if search_preference.has_semantic_target and (not best_semantic_items) and best_items and (not llm_semantic_evaluated):
        llm_semantic_evaluated = True
        llm_semantic_items = _semantic_filter_poi_candidates_with_llm(
            poi_items=best_items,
            preference=search_preference,
        )
        if llm_semantic_items:
            return llm_semantic_items, int(best_items_radius), attempts, None

    if search_preference.has_semantic_target and best_semantic_items:
        return best_semantic_items, int(best_semantic_radius), attempts, None

    if search_preference.has_semantic_target:
        return [], int(final_radius), attempts, None

    if len(best_items) >= min_required_count:
        if search_preference.allow_extended_search:
            return best_items, int(final_radius), attempts, None
        return best_items, int(best_items_radius), attempts, None

    return best_items, int(final_radius), attempts, last_error


def handle_travel_chat(data: Dict[str, Any]) -> Tuple[Dict[str, Any], int]:
    request_data, input_error = _resolve_input(data)
    if input_error:
        return input_error, 400

    session_id = _build_session_id(request_data)
    turn_id = uuid.uuid4().hex
    trace_enabled = _to_bool(data.get("debug_trace"), default=request_data.debug_trace)
    auto_continue_full_poi_batches = _to_bool(
        data.get("auto_continue_full_poi_batches"),
        default=request_data.auto_continue_full_poi_batches,
    )
    logger.info("[TravelAgent][USER_MESSAGE][%s] %s", turn_id, request_data.message)
    session_history = _get_session_history(session_id)
    strategy, strategy_llm_used, strategy_error = _decide_dialog_strategy(
        user_message=request_data.message or "",
        session_history=session_history,
        request_data=request_data,
        turn_id=turn_id,
        trace_enabled=trace_enabled,
    )

    _append_session_history(session_id, "user", request_data.message or "")

    has_location = request_data.x is not None and request_data.y is not None
    normalized_intent = str(strategy.intent or "").strip().lower()
    is_distance_query = normalized_intent in {"distance_query", "query_distance", "distance"}

    effective_top_k = strategy.target_poi_count or max(3, min(20, int(request_data.top_k)))
    min_required = strategy.min_required_poi or min(3, effective_top_k)
    if is_distance_query:
        effective_top_k = 1
        min_required = 1

    should_fetch_poi = (strategy.should_search_poi or (strategy.should_plan_route and not is_distance_query)) and has_location
    search_preference = _build_search_preference_from_strategy(strategy)

    poi_items: List[Dict[str, Any]] = []
    used_radius = request_data.radius or AUTO_RADIUS_START
    poi_attempts: List[Dict[str, Any]] = []
    poi_error: Optional[str] = None
    cached_distance_hit: Optional[Dict[str, Any]] = None
    semantic_no_match = False
    total_poi_count = 0
    ask_confirmation_for_large_result = False
    full_output_requested = False
    full_output_batch_count = 0
    full_poi_batch_reply: Optional[str] = None

    if is_distance_query:
        cached_distance_hit = _find_cached_poi_for_distance_query(
            session_id=session_id,
            user_message=request_data.message or "",
            strategy=strategy,
        )
        if cached_distance_hit is not None:
            should_fetch_poi = False
            poi_items = [cached_distance_hit]
            poi_attempts = [{"source": "session_cache", "hit": True, "name": cached_distance_hit.get("name")}]
            logger.info(
                "[TravelAgent][CACHE_HIT][%s] %s",
                turn_id,
                json.dumps({"distance_query": True, "poi": cached_distance_hit}, ensure_ascii=False),
            )

    if should_fetch_poi and request_data.x is not None and request_data.y is not None:
        force_full_coverage = _is_full_coverage_intent(strategy) and request_data.radius is not None
        if force_full_coverage:
            poi_items, used_radius, poi_attempts, poi_error = _fetch_poi_for_fixed_radius(
                x=request_data.x,
                y=request_data.y,
                radius=int(request_data.radius),
                search_preference=search_preference,
            )
        else:
            poi_items, used_radius, poi_attempts, poi_error = _fetch_poi_with_adaptive_radius(
                x=request_data.x,
                y=request_data.y,
                initial_radius=request_data.radius,
                target_count=effective_top_k,
                min_required_count=min_required,
                search_preference=search_preference,
            )

        poi_items = _rank_poi_candidates(
            poi_items=poi_items,
            center_x=request_data.x,
            center_y=request_data.y,
            preference=search_preference,
        )
        total_poi_count = len(poi_items)

        if strategy.should_search_poi and (not strategy.should_plan_route) and total_poi_count > 0:
            presentation_decision, presentation_llm_used, presentation_error = _decide_poi_presentation_with_llm(
                user_message=request_data.message or "",
                strategy=strategy,
                total_count=total_poi_count,
                session_history=session_history,
                turn_id=turn_id,
                trace_enabled=trace_enabled,
            )
            logger.info(
                "[TravelAgent][PRESENTATION_PLAN][%s] %s",
                turn_id,
                json.dumps(
                    {
                        "total_poi_count": total_poi_count,
                        "decision": presentation_decision.model_dump(),
                        "llm_used": presentation_llm_used,
                        "error": presentation_error,
                    },
                    ensure_ascii=False,
                ),
            )

            display_count = min(DEFAULT_INLINE_POI_REPLY_COUNT, total_poi_count)
            if presentation_decision.user_wants_all:
                full_output_requested = True
                if auto_continue_full_poi_batches and total_poi_count > MAX_INLINE_POI_REPLY_COUNT:
                    display_count = total_poi_count
                    full_output_batch_count = int(
                        math.ceil(total_poi_count / MAX_INLINE_POI_REPLY_COUNT)
                    )
                    full_poi_batch_reply = _build_full_poi_batch_reply(
                        poi_items=poi_items,
                        total_count=total_poi_count,
                        batch_size=MAX_INLINE_POI_REPLY_COUNT,
                    )
                else:
                    display_count = min(total_poi_count, MAX_INLINE_POI_REPLY_COUNT)
                    if total_poi_count > display_count:
                        ask_confirmation_for_large_result = True
            elif presentation_decision.mode == "all":
                display_count = min(PREVIEW_POI_REPLY_COUNT, total_poi_count)
                ask_confirmation_for_large_result = total_poi_count > display_count
            elif presentation_decision.mode == "count" and presentation_decision.requested_count is not None:
                requested = min(
                    total_poi_count,
                    MAX_INLINE_POI_REPLY_COUNT,
                    max(1, int(presentation_decision.requested_count)),
                )
                if requested > PREVIEW_POI_REPLY_COUNT:
                    display_count = min(PREVIEW_POI_REPLY_COUNT, total_poi_count)
                    ask_confirmation_for_large_result = True
                else:
                    display_count = requested
            else:
                display_count = min(PREVIEW_POI_REPLY_COUNT, total_poi_count)
                ask_confirmation_for_large_result = (
                    total_poi_count > display_count
                    and presentation_decision.need_confirmation_when_oversized
                )

            poi_items = poi_items[:display_count]

        if search_preference.has_semantic_target and not poi_items and not poi_error:
            semantic_no_match = True

    if has_location and request_data.x is not None and request_data.y is not None and poi_items:
        _remember_poi_candidates(
            session_id=session_id,
            poi_items=poi_items,
            center_x=request_data.x,
            center_y=request_data.y,
        )

    if should_fetch_poi or strategy_error:
        logger.info(
            "[TravelAgent][RETRIEVAL_PLAN][%s] %s",
            turn_id,
            json.dumps(
                {
                    "intent": strategy.intent,
                    "should_fetch_poi": should_fetch_poi,
                    "target_poi_count": effective_top_k,
                    "min_required_poi": min_required,
                    "fetched_poi_count": len(poi_items),
                    "total_poi_count": total_poi_count,
                    "target_reached": len(poi_items) >= effective_top_k,
                    "min_required_reached": len(poi_items) >= min_required,
                    "used_radius": used_radius,
                    "attempts": poi_attempts,
                    "search_preference": search_preference.model_dump(),
                    "strategy_error": strategy_error,
                },
                ensure_ascii=False,
            ),
        )

    if should_fetch_poi and not poi_items and poi_error:
        return {
            "success": False,
            "error_code": "POI_SERVICE_ERROR",
            "error": f"POI接口调用失败: {poi_error}",
            "poi_url": POI_API_URL,
            "search_meta": {
                "attempts": poi_attempts,
                "used_radius": used_radius,
            },
        }, 502

    if should_fetch_poi and semantic_no_match:
        covered_km = round(float(used_radius) / 1000.0, 2)
        next_radius = min(AUTO_RADIUS_MAX, int(used_radius) + AUTO_RADIUS_EXPANDED_STAGE_SPAN)
        next_km = round(float(next_radius) / 1000.0, 2)

        weather_summary = _summarize_weather(
            {
                "temperature": request_data.temperature,
                "weather_name": request_data.weather_name,
                "weather_description": request_data.weather_description,
                "wind_speed": request_data.wind_speed,
                "city": request_data.city,
                "state": request_data.state,
            },
            default_state=request_data.state or "UnknownState",
            default_city=request_data.city or "UnknownCity",
        )
        model_context = _build_response_model_context(
            user_message=request_data.message,
            session_history=session_history,
            strategy=strategy,
            search_preference=search_preference,
            weather_summary=weather_summary,
            request_data=request_data,
            route_plan=None,
            poi_items=[],
            poi_total_count=0,
            ask_confirmation_for_large_result=False,
            full_output_requested=False,
        )
        execution_data = model_context.get("execution_data")
        if isinstance(execution_data, dict):
            execution_data["semantic_no_match"] = {
                "covered_km": covered_km,
                "next_km": next_km,
                "allow_extended_search": search_preference.allow_extended_search,
            }

        reply, llm_used, llm_error = _generate_agent_reply(
            user_message=request_data.message,
            model_context=model_context,
            strategy=strategy,
            route_plan=None,
            has_location=has_location,
            turn_id=turn_id,
            trace_enabled=trace_enabled,
        )
        if llm_error and not llm_used:
            return {
                "success": False,
                "error_code": "LLM_SERVICE_ERROR",
                "error": llm_error,
            }, 503

        _append_session_history(session_id, "assistant", reply)
        recommendation = _build_dialog_recommendation(route_plan=None, strategy=strategy)
        output_data: Dict[str, Any] = {
            "success": True,
            "reply": reply,
            "session_id": session_id,
            "weather_summary": weather_summary,
            "recommendation": recommendation,
        }

        include_debug = _to_bool(data.get("debug"), default=request_data.debug)
        if include_debug:
            output_data.update({
                "input": {
                    **request_data.model_dump(),
                    "requested_radius": request_data.radius,
                    "effective_radius": used_radius,
                    "effective_top_k": effective_top_k,
                },
                "poi_sample": [],
                "search_meta": {
                    "attempts": poi_attempts,
                    "used_radius": used_radius,
                    "min_required_poi": min_required,
                    "found_poi": 0,
                    "auto_radius": request_data.radius is None,
                    "dialog_strategy": strategy.model_dump(),
                    "search_preference": search_preference.model_dump(),
                    "semantic_no_match": True,
                    "llm_used": llm_used,
                    "llm_error": llm_error,
                },
                "model_context": model_context,
            })

        output = TravelChatOutput(
            success=True,
            reply=reply,
            session_id=session_id,
            weather_summary=output_data["weather_summary"],
            recommendation=output_data["recommendation"],
            input=output_data.get("input"),
            poi_sample=output_data.get("poi_sample"),
            search_meta=output_data.get("search_meta"),
            context_payload=output_data.get("model_context"),
        )
        return output.model_dump(exclude_none=True, by_alias=True), 200

    has_client_weather = any([
        request_data.temperature is not None,
        bool(request_data.weather_name),
        bool(request_data.weather_description),
        request_data.wind_speed is not None,
    ])

    weather_summary = _summarize_weather(
        {
            "temperature": request_data.temperature,
            "weather_name": request_data.weather_name,
            "weather_description": request_data.weather_description,
            "wind_speed": request_data.wind_speed,
            "city": request_data.city,
            "state": request_data.state,
        },
        default_state=request_data.state or "UnknownState",
        default_city=request_data.city or "UnknownCity",
    )

    route_plan: Optional[RoutePlan] = None
    if (not is_distance_query) and strategy.should_plan_route and has_location and request_data.x is not None and request_data.y is not None:
        route_plan = _compute_route(
            poi_items,
            center_x=request_data.x,
            center_y=request_data.y,
            weather_summary=weather_summary,
            top_k=effective_top_k,
        )

    model_context = _build_response_model_context(
        user_message=request_data.message,
        session_history=session_history,
        strategy=strategy,
        search_preference=search_preference,
        weather_summary=weather_summary,
        request_data=request_data,
        route_plan=route_plan,
        poi_items=poi_items,
        poi_total_count=total_poi_count,
        ask_confirmation_for_large_result=ask_confirmation_for_large_result,
        full_output_requested=full_output_requested,
        auto_continue_full_poi_batches=auto_continue_full_poi_batches,
        full_output_batch_count=full_output_batch_count,
    )
    logger.info("[TravelAgent][MODEL_CONTEXT][%s] %s", turn_id, json.dumps(model_context, ensure_ascii=False))

    if full_poi_batch_reply:
        reply, llm_used, llm_error = full_poi_batch_reply, False, None
        logger.info(
            "[TravelAgent][FULL_BATCH_REPLY][%s] %s",
            turn_id,
            json.dumps(
                {
                    "total_poi_count": total_poi_count,
                    "batch_count": full_output_batch_count,
                    "batch_size": MAX_INLINE_POI_REPLY_COUNT,
                    "auto_continue_full_poi_batches": auto_continue_full_poi_batches,
                },
                ensure_ascii=False,
            ),
        )
    else:
        reply, llm_used, llm_error = _generate_agent_reply(
            user_message=request_data.message,
            model_context=model_context,
            strategy=strategy,
            route_plan=route_plan,
            has_location=has_location,
            turn_id=turn_id,
            trace_enabled=trace_enabled,
        )
    if llm_error and not llm_used:
        return {
            "success": False,
            "error_code": "LLM_SERVICE_ERROR",
            "error": llm_error,
        }, 503

    _append_session_history(session_id, "assistant", reply)
    logger.info("[TravelAgent][FINAL_REPLY][%s] %s", turn_id, reply)

    include_debug = _to_bool(data.get("debug"), default=request_data.debug)

    recommendation = _build_dialog_recommendation(route_plan, strategy)

    output_data: Dict[str, Any] = {
        "success": True,
        "reply": reply,
        "session_id": session_id,
        "weather_summary": weather_summary,
        "recommendation": recommendation,
    }

    if include_debug:
        output_data.update({
            "input": {
                **request_data.model_dump(),
                "requested_radius": request_data.radius,
                "effective_radius": used_radius,
                "effective_top_k": effective_top_k,
            },
            "poi_sample": [_poi_name(item) for item in poi_items[:8]],
            "search_meta": {
                "attempts": poi_attempts,
                "used_radius": used_radius,
                "min_required_poi": min_required,
                "found_poi": len(poi_items),
                "total_poi_count": total_poi_count,
                "full_output_requested": full_output_requested,
                "auto_continue_full_poi_batches": auto_continue_full_poi_batches,
                "full_output_batch_count": full_output_batch_count,
                "full_batch_reply_used": bool(full_poi_batch_reply),
                "auto_radius": request_data.radius is None,
                "weather_source": request_data.weather_source or "client_context",
                "has_client_weather": has_client_weather,
                "dialog_strategy": strategy.model_dump(),
                "search_preference": search_preference.model_dump(),
                "strategy_llm_used": strategy_llm_used,
                "strategy_error": strategy_error,
                "llm_used": llm_used,
                "llm_error": llm_error,
                "debug_trace": trace_enabled,
                "cache_hit": cached_distance_hit is not None,
            },
            "model_context": model_context,
        })

    output = TravelChatOutput(
        success=True,
        reply=reply,
        session_id=session_id,
        weather_summary=weather_summary,
        recommendation=output_data["recommendation"],
        input=output_data.get("input"),
        poi_sample=output_data.get("poi_sample"),
        search_meta=output_data.get("search_meta"),
        context_payload=output_data.get("model_context"),
    )
    return output.model_dump(exclude_none=True, by_alias=True), 200
