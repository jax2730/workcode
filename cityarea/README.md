# cityarea

![cityarea preview](picture/image.png)

面向城市 GIS 处理的 Python 工程：从 OpenStreetMap 抓取数据，构建行政区层级、城市道路与街区分块，并导出 QGIS 可视化图层及下游 JSON/网格资产。

## 项目目标

- 抓取 OSM 数据（道路、城市 POI、地标、河流、行政区）
- 生成多级行政区（state/district/county/town）及编码体系
- 生成城市多层级 sector（`lvl_0...lvl_n`）与街区填充结果
- 导出游戏/仿真可用的道路、街区、POI、网格、掩码等数据

## 核心目录

- `overpass_query/`：Overpass 查询与 OSM -> GPKG 转换
- `admin_process/`：行政区层级整理、父级归属、编码与栅格化
- `city_process/`：道路简化、sector 切分、街区生成与导出
- `citydata/`：当前仓库已有的北京样例产物

## 环境要求

- Python 3.10+
- GDAL / OGR（`osgeo`）
- GeoPandas、Shapely、NumPy、Pandas、Fiona、NetworkX
- pyosmium（读取 OSM XML）
- 可选：QGIS（人工检查/修正几何）

> [!IMPORTANT]
> 当前代码默认使用 `constants.py` 中的 `DATA_DIR = <repo>/data`。
> 但仓库样例数据位于 `citydata/`。若你直接跑脚本，请先统一目录（如复制或软链接到 `data/`，或按需修改 `constants.py`）。

## 快速开始（Windows / PowerShell）

```powershell
python -m venv .venv
.\.venv\Scripts\Activate.ps1
python -m pip install -U pip
python -m pip install geopandas shapely pandas numpy fiona networkx requests pyosmium rasterio scipy matplotlib triangle open3d xatlas momepy scikit-image
```

> [!NOTE]
> 项目中部分模块（如 `convert_coords`、`road_process`、`gen_voronoi`）来自你的本地工程生态；如果环境中缺失，请先补齐对应模块或调整调用链。

## 全流程（细分跑通）

下面给出与当前代码一致的阶段化流程。建议按阶段执行并在 QGIS 中检查结果。

### 1) 抓取基础数据（OSM）

- 城市道路与城市要素：`overpass_query/query_city.py`
- 国家道路关系与 way：`overpass_query/query_roads.py`
- 河流：`overpass_query/query_river.py`
- POI 与地标：`overpass_query/query_poi.py`、`overpass_query/query_landmark.py`

示例（按脚本内 `__main__` 执行）：

```powershell
python overpass_query\query_city.py
python overpass_query\query_roads.py
python overpass_query\query_river.py
```

### 2) 行政区层级整理与编码

1. 将各 `admin_level` 合并为四级层：`process_raw_admin_boundaries()`
2. 通过几何包含关系建立父子层级：`find_admin_parrent_country()`
3. 计算位编码 adcode 并校验：`encode_district_code_from_json()`

入口文件：`admin_process/administration_parrent_find.py`、`admin_process/administration_code_table.py`

### 3) 城市道路简化与 sector 切分

入口函数：`city_process/simplify_road.py` 中 `simplify_city_roads(city_name)`

该阶段会产出：

- `city_road_no_leaf_extended.gpkg`
- `city_sector.gpkg`（多层 `lvl_*`）
- `city_street_block.gpkg`
- `city_street_block_filled.gpkg`
- `city_road_filled.gpkg`

### 4) 街区/扇区关联导出

入口函数：`city_process/street_block_process.py` 中 `street_block_to_json_by_sector(city_name)`

输出示例：

- `city_street_block_new.json`
- `city_road.json`
- `city_building_base_line.gpkg`

### 5) 可选后处理

- 掩码：`city_process/create_city_mask.py`
- 3D 网格：`city_process/create_sector_mesh.py`
- 地形平滑：`city_process/smooth_terrain.py`
- POI 坐标转换：`city_process/pos_convert.py`

## QGIS 图层对应关系（与你截图一致）

- `city_sector.gpkg`
  - `lvl_0`, `lvl_1`, `lvl_2`, ...：递归道路等级切分得到的 sector 层级
- `city_street_block_filled.gpkg`
  - 街区填充后的最终面层（含 Voronoi 补全）
- `city_road_filled.gpkg`
  - 参与街区构建与导出的道路层

## 常见问题

- **`ogrinfo` / GDAL 命令不可用**：表示系统 PATH 未配置 GDAL CLI。可仅用 Python 侧 `osgeo`，或单独安装并加入 PATH。
- **Overpass 查询慢或失败**：脚本默认本地 `osm3s_query` 优先，需本地 OSM 数据库；否则切换到 `query_osm_data_remote()`。
- **几何拓扑异常**：复杂场景建议在 QGIS 手工检查吸附、断裂与自相交后再继续。
