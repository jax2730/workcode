# CLAUDE.md

## 项目上下文

- 这是一个城市地理数据处理项目（Python），以 OpenStreetMap 数据为输入，生成可用于游戏引擎或城市仿真的分层地理数据集。
- 主要目录：`admin_process/`、`city_process/`、`overpass_query/`、`test/`。
- 样本数据：`citydata/Beijing/`（1.5GB+），运行时通过 `data/` 软链接访问。
- 优先保持脚本式、可复用、可验证的实现方式。

## 项目整体流程

```
1. OSM 数据获取（overpass_query/query_city.py）
   └─ 查询城市边界 + 道路 + POI → city.osm / city.gpkg / osm/*.osm / poi_alt.gpkg

2. 行政边界处理（admin_process/）
   └─ 多级行政边界 → 父子关系 / 区划代码

3. 道路简化 & 街区生成（city_process/simplify_road.py）
   └─ city_road_no_leaf.gpkg → city_sector.gpkg → city_street_block_filled.gpkg

4. 街区分类（city_process/classify_blocks_by_point.py / classify_blocks_by_area.py）
   └─ 对每个街区打上 POI 类型标签 → city_street_block_filled_classify.gpkg

5. 街区 & 道路导出为 JSON（city_process/street_block_process.py）
   └─ city_street_block_new.json / city_road.json

6. 可选后处理
   └─ 3D mesh（FBX/OBJ）/ 地形 mask / 坐标转换
```

## 关键文件

| 文件 | 作用 |
|---|---|
| `constants.py` | 全局路径（WORKING_DIR / DATA_DIR）、投影 WKT、API URL |
| `overpass_query/query_city.py` | Overpass QL 查询城市数据和 27 类 POI |
| `overpass_query/osm_util.py` | OSM 解析器（osmium handlers）+ GPKG 转换工具 |
| `city_process/simplify_road.py` | 道路简化、sector 生成、街区多边形提取 |
| `city_process/filter_ways.py` | 过滤与城市 sector 相交的道路 |
| `city_process/street_block_process.py` | 街区匹配道路、生成建筑基线、导出 JSON |
| `city_process/classify_blocks_by_point.py` | 按 POI 点数对街区分类（来源：poi_alt.gpkg） |
| `city_process/classify_blocks_by_area.py` | 按 POI polygon 交集面积对街区分类（来源：osm/*.osm） |

## 街区分类模块说明

### 核心数据文件

- `city_street_block_filled.gpkg` — 街区多边形（输入）
- `city_sector.gpkg` — 多层级区域划分（lvl_0, lvl_1, ...）
- `poi_alt.gpkg` — 各类 POI 的 centroid 点（每个 layer 对应一种类型）
- `osm/*.osm` — 原始 OSM 文件，保留 polygon 几何（面积计算用）

### 两种分类方式

**按点数（classify_blocks_by_point.py）**
- 数据源：`poi_alt.gpkg`（已是 centroid 点）
- 逻辑：`gpd.sjoin` 统计各类型点落在街区内的数量，取最多者
- 输出字段：`type`、`type_count`
- 适用：POI 本身为点状要素（便利店、餐厅、银行等）

**按面积（classify_blocks_by_area.py）**
- 数据源：`osm/*.osm` 原始文件，用 `POIPolygonHandler` 解析 polygon
- 逻辑：`gpd.overlay` 求交集，汇总各类型面积，取最大者
- 输出字段：`type`、`type_area`
- 输出文件：`city_street_block_filled_classify.gpkg`（不覆盖原始文件）
- 适用：POI 本身为面状要素（居民区、公园、学校、工厂等）

### POI 类型参考（来自 query_city.py）

Airport / AmusementPark / Bank / CarDealer / CoachStation / ConvienceStore /
Factory / FarmerMarket / GasStation / HairSalon / Hospital / Hotel /
Kindergarten / KTV / Library / McDonalds / Museum / OfficeBuilding /
Park / PostOffice / PublicBathroom / ResidentalComplex / School /
ShoppingMall / Stadium / TrainStation / ViewSpot / Zoo

## 坐标系约定

- 输入：WGS84 (EPSG:4326)
- 处理（面积/空间运算）：Web Mercator (EPSG:3857)
- 导出 JSON：自定义偏移坐标系，原点 `[-2097152.0, -1048576.0]`，像素缩放 32

## 道路等级与缓冲距离

| 类型 | highway_code | buffer(m) |
|---|---|---|
| motorway | 1 | 80 |
| trunk | 2 | 50 |
| primary | 3 | 30 |
| secondary | 4 | 25 |
| tertiary | 5 | 20 |
| unclassified/track/path | 6-8 | 20 |

## Agent Skills 使用规则

- 当任务涉及 Agent 架构、上下文优化、评测体系时，优先参考：
  - `Agent-Skills-for-Context-Engineering-main/skills/`
- 当任务涉及 Copilot 资源（agents/instructions/skills）组织与规范时，参考：
  - `awesome-copilot-main/AGENTS.md`
  - `awesome-copilot-main/.github/copilot-instructions.md`

## 工作偏好

- 先最小可行改动，再增量优化。
- 修改代码后优先执行与改动最相关的测试/脚本验证。
- 不引入无关重构，不修改与任务无关文件。

## 配置说明

- 本项目使用 `.vscode/settings.json` 统一 VS Code / Copilot 行为。
- Claude 项目级设置见 `.claude/settings.json`。
- 不在仓库中保存任何 API Token 或密钥。
