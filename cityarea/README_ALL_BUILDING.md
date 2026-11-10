# 全北京建筑数据查询 - All_Building.gpkg

快速生成全北京建筑数据，包含建筑面积和分类，可在QGIS中查看。

## 快速开始

### 第1步：在服务器上生成建筑数据

在 `zhoujiajun` 账户上运行：

```bash
cd /home/zhoujiajun/overpass_query
python query_buildings.py Beijing --log
```

**输出位置**：`/home/pixel/geo_tools/data/city/Beijing/All_Building.gpkg`

**耗时**：10-30分钟（取决于网络和本地缓存）

### 第2步：在QGIS中查看

1. 打开QGIS
2. 图层 → 添加图层 → 矢量
3. 选择 `All_Building.gpkg`
4. 选择 `building` 图层
5. 点击"添加"

### 第3步：按分类着色

在QGIS中按分类着色：

1. 右键点击图层 → 属性
2. 符号 → 分类
3. 选择列：`category`
4. 点击"分类"
5. 为每个分类选择颜色：
   - Residential（居住）→ 蓝色
   - Commercial（商业）→ 红色  
   - PublicServices（公共服务）→ 黄色
   - Industrial（工业）→ 灰色
   - GreenSpace（绿地）→ 绿色
   - Others（其他）→ 浅灰色

## 数据说明

### 输出文件

**路径**：`/home/pixel/geo_tools/data/city/Beijing/All_Building.gpkg`

**图层**：`building`

**大小**：约100-150 MB（包含约30万个建筑）

### 数据列

| 列名 | 说明 | 示例 |
|------|------|------|
| `id` | OSM ID | 12345678 |
| `building` | 建筑类型 | residential, commercial, yes |
| `building:levels` | 楼层数 | 6, 20 |
| `height` | 高度(米) | 18.5 |
| `area_m2` | 建筑面积(平方米) | 2500.5 |
| `category` | 宏观分类 | Residential, Commercial |
| `name:local` | 中文名称 | 北京大厦 |
| `name:en` | 英文名称 | Beijing Tower |
| `amenity` | 便民设施 | hospital, school |
| `shop` | 商铺类型 | supermarket, pharmacy |
| `office` | 办公室类型 | government, ngo |

### 分类说明

- **Residential（居住）**：住宅小区、房屋、公寓
- **Commercial（商业）**：商店、办公楼、酒店、餐厅、银行
- **PublicServices（公共服务）**：学校、医院、图书馆、政府、卫生间、停车场
- **Industrial（工业）**：工厂、仓库、港口、建筑工地
- **GreenSpace（绿地）**：温室、农业相关建筑
- **Others（其他）**：未分类建筑

## 命令行选项

```bash
python query_buildings.py <城市名> [--radius 半径] [--log]
```

### 参数

| 参数 | 说明 | 示例 |
|------|------|------|
| `<城市名>` | 城市英文名(必需) | Beijing, Shanghai |
| `--radius` | 搜索半径(米，默认50000) | 60000 |
| `--log` | 显示详细日志 | --log |

### 例子

```bash
# 基础查询
python query_buildings.py Beijing

# 显示日志
python query_buildings.py Beijing --log

# 增加搜索范围
python query_buildings.py Shanghai --radius 60000 --log

# 其他城市
python query_buildings.py Chengdu
python query_buildings.py Xi'an --radius 40000
```

## 在Windows上分析数据

### 分析生成的GPKG

下载完成后，可以用Python脚本分析数据：

```bash
python demo_building_analysis.py "C:\path\to\All_Building.gpkg" --show-stats --show-sample
```

### 输出示例

```
BUILDING DATA STATISTICS
============================================================

Overall Statistics:
  Total buildings:         298,456
  Total area:              1,452.35 km²
  Average area:            4,863.2 m²
  Median area:             892.5 m²

Buildings by Category:
  Residential           156,234 buildings (52.3%)  583.42 km²
  Commercial             78,901 buildings (26.4%)  521.83 km²
  PublicServices         32,145 buildings (10.8%)  187.25 km²
  Industrial             18,923 buildings (6.3%)   98.45 km²
  GreenSpace             12,253 buildings (4.1%)   61.40 km²
  Others                    100 buildings (0.1%)    0.00 km²

Top Building Types:
  residential              156,234 (52.3%)
  commercial                45,123 (15.1%)
  yes                       38,901 (13.0%)
  office                    18,234 (6.1%)
  ...
```

## 常见问题

### Q: 为什么没有找到建筑？

A: 确保城市名称正确（英文），尝试增加搜索范围 `--radius 60000`

### Q: 数据文件很大，怎么处理？

A:

- 在QGIS中设置空间索引加速
- 使用属性过滤器只显示特定分类
- 导出为更小的格式（如shapefile）

### Q: 如何更新分类系统？

A: 编辑 `/home/pixel/geo_tools/data/type_list.py` 的 `category_mapping` 字典，然后重新运行查询。

### Q: 数据准确性如何？

A: 数据来自OpenStreetMap，质量取决于本地编辑情况。北京市区数据较完整，郊区可能有缺陷。

## 与其他数据的集成

### 土地利用数据（Landuse）

结合建筑和土地利用数据：

```bash
python query_landuse.py Beijing --log
```

生成 `landuse.gpkg`，用于验证和补充建筑分类

### 行政区分类

自动将建筑分类到各行政区：

```bash
python classify_city_sector.py
```

输出：`classify_city_sector.gpkg`

## 高级用法

### 自定义分类规则

编辑 `server/overpass_query/osm_util.py` 的 `_map_building_to_category()` 函数：

```python
def _map_building_to_category(building_type: str, category_mapping: dict) -> str:
    building_type = str(building_type).lower().strip()
    
    # 自定义映射（优先级最高）
    if 'dormitory' in building_type:
        return 'Residential'
    if 'mosque' in building_type:
        return 'PublicServices'
    
    # 使用type_list.py中的映射
    for category, building_types in category_mapping.items():
        if any(building_type.find(bt.lower()) >= 0 for bt in building_types):
            return category
    
    return 'Others'
```

### 导出特定分类

在QGIS中：

1. 右键图层 → 导出为...
2. 在SQL查询中添加 WHERE 条件：`category = 'Commercial'`
3. 保存为新文件

## 性能说明

| 操作 | 耗时 | 输出大小 |
|------|------|---------|
| 查询OSM数据 | 5-20分钟 | 100-200 MB (OSM) |
| 处理和分类 | 3-5分钟 | 100-150 MB (GPKG) |
| QGIS打开 | <30秒 | - |
| QGIS渲染全图 | 1-5秒 | - |

## 文件位置速查

| 文件 | 位置 |
|------|------|
| 查询脚本 | `/home/zhoujiajun/overpass_query/query_buildings.py` |
| OSM原始数据 | `/home/pixel/geo_tools/data/city/Beijing/buildings.osm` |
| 最终输出 | `/home/pixel/geo_tools/data/city/Beijing/All_Building.gpkg` |
| 分类定义 | `/home/pixel/geo_tools/data/type_list.py` |
| 文档 | `/home/zhoujiajun/overpass_query/BUILDINGS_QUERY_GUIDE.md` |

## 下一步

1. ✓ 生成 `All_Building.gpkg`
2. ✓ 在QGIS中查看并着色
3. → 生成 `landuse.gpkg` 补充验证
4. → 运行 `classify_city_sector.py` 进行行政区分类
5. → 进行城市规划分析

## 支持和反馈

- 查看详细文档：`BUILDINGS_QUERY_GUIDE.md`
- 查看完整代码：`query_buildings.py`, `osm_util.py`
- 检查日志输出：运行 `--log` 参数
- 分析数据：运行 `demo_building_analysis.py`
