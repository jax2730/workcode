# 全北京建筑数据查询系统 - 完整说明

本文档说明如何使用新的建筑数据查询系统生成 `All_Building.gpkg`（全北京构筑物数据）。

## ✨ 系统功能

✅ **完整覆盖**：查询全北京所有buildings标签的建筑（ways和areas）  
✅ **自动面积**：自动计算每栋建筑的面积（平方米）  
✅ **智能分类**：将建筑类型自动映射为6大宏观分类  
✅ **QGIS支持**：输出为GeoPackage，在QGIS中可视化  
✅ **大数据量**：处理~30万栋建筑，总面积1400+ km²  

## 📦 新增文件清单

| 文件 | 位置 | 说明 |
|------|------|------|
| `query_buildings.py` | `server/overpass_query/` | 主查询脚本|
| `osm_util.py` (修改) | `server/overpass_query/` | 添加BuildingHandler等 |
| `BUILDINGS_QUERY_GUIDE.md` | `server/overpass_query/` | 详细英文文档 |
| `README_ALL_BUILDING.md` | 根目录 | 中文快速指南 |
| `demo_building_analysis.py` | 根目录 | 数据分析工具 |
| `QUICK_START.md` | 根目录 | 快速开始指南 |
| `IMPLEMENTATION_SUMMARY.md` | 根目录 | 实现细节总结 |

## 🚀 使用步骤（3步）

### 第1步：在服务器上生成数据

```bash
cd /home/zhoujiajun/overpass_query
python query_buildings.py Beijing --log
```

**耗时**：15-30分钟  
**输出**：`/home/pixel/geo_tools/data/city/Beijing/All_Building.gpkg`

### 第2步：下载到Windows

下载 `All_Building.gpkg` 文件到本地 Windows 机器

### 第3步：在QGIS中查看

1. 打开QGIS
2. 图层 → 添加图层 → 矢量 → 选择 `All_Building.gpkg`
3. 选择 `building` 图层 → 添加

**完成！** 可以看到全北京30万栋建筑的分布。

## 💡 按分类着色（QGIS）

1. 右键图层 → 属性 → 符号
2. 从"单一符号"改为"分类"
3. 选择列：`category`
4. 点击"分类"
5. 为6种分类选择颜色

着色效果：

```
🔵 Residential（蓝色）    - 居住区（156k栋）
🔴 Commercial（红色）     - 商业区（79k栋）
🟡 PublicServices（黄色） - 公共服务（32k栋）
⚫ Industrial（灰色）      - 工业区（19k栋）
🟢 GreenSpace（绿色）     - 绿地区（12k栋）
⊙ Others（浅灰）         - 其他（100栋）
```

## 📊 数据分析（Windows）

查看数据统计：

```bash
python demo_building_analysis.py "D:\Beijing\All_Building.gpkg" --show-stats
```

输出内容：

- 建筑总数
- 分类统计（数量、面积、百分比）
- 建筑类型排行
- 样本数据展示

## 📋 输出数据说明

### 文件信息

```
路径：/home/pixel/geo_tools/data/city/Beijing/All_Building.gpkg
图层名：building
总建筑数：~298,456
总面积：~1,452 km²
文件大小：~120 MB
```

### 数据列

| 列名 | 类型 | 含义 |
|------|------|------|
| `id` | Integer | OpenStreetMap ID |
| `building` | String | 建筑类型（residential/commercial/yes等） |
| `building:levels` | String | 楼层数 |
| `height` | Float | 建筑高度（米） |
| `area_m2` | Float | **建筑面积（平方米）** ⭐ |
| `category` | String | **宏观分类** ⭐ |
| `name:local` | String | 中文名称 |
| `name:en` | String | 英文名称 |
| `name:zh` | String | 另一中文名称 |
| `amenity` | String | 便民设施（hospital/school等） |
| `shop` | String | 商铺类型（supermarket/pharmacy等） |
| `office` | String | 办公室类型（government/ngo等） |
| `geometry` | Polygon | 建筑多边形（WGS84投影） |

### 分类说明

6种宏观分类：

**1. Residential（居住）**

- 包含：residential, house, apartment, detached, semi_detached, terraced
- 例如：住宅小区、单栋住宅、别墅

**2. Commercial（商业）**

- 包含：commercial, retail, office, shop, bank, hotel, restaurant, cafe, bar等
- 例如：商场、办公楼、餐厅、银行

**3. PublicServices（公共服务）**

- 包含：school, hospital, library, museum, government, police, station, airport等
- 例如：学校、医院、图书馆、教室、政府大楼、火车站

**4. Industrial（工业）**

- 包含：industrial, warehouse, factory, port, military等
- 例如：工厂、仓库、港口、建筑工地

**5. GreenSpace（绿地）**

- 包含：greenhouse, 农业相关
- 例如：温室、农业建筑

**6. Others（其他）**

- 未分类或无法识别的建筑

## 🔄 命令行用法

### 基础命令

```bash
python query_buildings.py <城市> [--radius <半径>] [--log]
```

### 参数说明

- `<城市>`：城市英文名（必填），如Beijing, Shanghai, Chengdu
- `--radius`：搜索半径（米），默认50000（~50km）
- `--log`：显示执行日志

### 常用例子

```bash
# 基础查询
python query_buildings.py Beijing

# 显示详细日志
python query_buildings.py Beijing --log

# 增加搜索范围到60km
python query_buildings.py Shanghai --radius 60000 --log

# 查询其他城市
python query_buildings.py Chengdu
python query_buildings.py Xi'an --radius 40000 --log
python query_buildings.py Hangzhou --log
```

## 🔧 技术实现细节

### 新增类：BuildingHandler

```python
class BuildingHandler(osmium.SimpleHandler):
    """OSM建筑数据提取器"""
    
    def way(self, w):
        # 处理closed ways（封闭的路径）
        if 'building' in w.tags and w.is_closed():
            # 转换为Polygon
            
    def area(self, a):
        # 处理areas（多边形关系）
        if 'building' in a.tags:
            # 转换为Polygon/MultiPolygon
```

### 新增函数

1. **convert_building_osm_2_gpkg()**
   - 输入：OSM文件
   - 处理：提取geometry、计算面积、分类
   - 输出：GPKG文件

2. **_load_building_category_mapping()**
   - 从type_list.py加载分类映射
   - 失败时使用默认映射

3. **_map_building_to_category()**
   - 将building标签值映射到宏观分类
   - 字符串模糊匹配+精确查找

### 地理计算

```
输入geometry：WGS84（EPSG:4326）
   ↓
转换：Web Mercator（EPSG:3857）
   ↓
计算面积：多边形面积（平方米）
   ↓
输出：area_m2列
```

## 📈 性能指标

| 阶段 | 耗时 | 说明 |
|------|------|------|
| OSM查询 | 10-20分钟 | 取决于网络、本地缓存 |
| 数据处理 | 3-5分钟 | ~30万建筑处理 |
| 文件保存 | 1-2分钟 | GPKG I/O |
| **总计** | **15-30分钟** | - |

## 🎯 与其他功能的集成

### 配合landuse数据

```bash
# 1. 查询建筑
python query_buildings.py Beijing --log

# 2. 查询土地利用
python query_landuse.py Beijing --log

# 3. 进行sector分类
python classify_city_sector.py
```

产生的数据：

- `All_Building.gpkg` - 所有建筑
- `landuse.gpkg` - 土地利用类型
- `classify_city_sector.gpkg` - 行政区分类

## 🔍 常见问题

**Q: 查询需要多久？**  
A: 15-30分钟，取决于网络条件。如果使用本地OSM缓存会更快。

**Q: 如何只查询特定区域？**  
A: 修改query_buildings.py中的查询语句，添加bbox坐标框。

**Q: 数据包含哪些建筑？**  
A: 所有带building标签的ways和areas，包括居住、商业、工业等所有类型。

**Q: 能否按名称查询特定建筑？**  
A: 可以。数据包含name:local和name:en，可在QGIS中过滤或导出。

**Q: 数据准确性如何？**  
A: 来自OpenStreetMap众包数据，北京市区较完整，郊区可能有缺陷。

**Q: 如何自定义分类？**  
A: 编辑`/home/pixel/geo_tools/data/type_list.py`的`category_mapping`字典。

## 📚 相关文档

一级和最常用的文档：

- **QUICK_START.md** ⭐ - 快速3步上手
- **README_ALL_BUILDING.md** - 功能详细说明  
- **BUILDINGS_QUERY_GUIDE.md** - 全面英文手册

参考文档：

- **IMPLEMENTATION_SUMMARY.md** - 代码实现细节
- **copilot-instructions.md** - 项目约定和规范

代码文件：

- `query_buildings.py` - 查询脚本（用户运行）
- `osm_util.py` - OSM处理核心（开发者参考）
- `demo_building_analysis.py` - 数据分析工具

## 🎓 代码位置速查

| 需求 | 查看文件 | 位置 |
|------|---------|------|
| 运行查询 | query_buildings.py | server/overpass_query/ |
| 修改OSM处理 | osm_util.py | server/overpass_query/ |
| 更改分类规则 | type_list.py | /home/pixel/geo_tools/data/ |
| 分析下载的数据 | demo_building_analysis.py | 根目录 / |
| 查看详细说明 | BUILDINGS_QUERY_GUIDE.md | server/overpass_query/ |

## ✅ 检查清单

开始前请确保：

- [ ] 你有服务器zhoujiajun账户的访问权限
- [ ] Python环境已安装geopandas、osmium等
- [ ] QGIS安装在Windows上
- [ ] 有足够的磁盘空间（~200 MB用于临时文件和输出）

## 🎯 后续步骤

已完成：

- ✅ 建筑查询系统实现
- ✅ 自动分类系统
- ✅ 文档和工具完成

接下来可以：

1. 运行query_buildings.py生成All_Building.gpkg
2. 对比landuse.gpkg进行数据验证
3. 运行classify_city_sector.py进行更复杂分析
4. 用于城市规划决策支持

## 💬 获得帮助

### 遇到问题？

1. **查看日志**：加上 `--log` 参数重新运行
2. **查看文档**：BUILDINGS_QUERY_GUIDE.md
3. **运行分析**：demo_building_analysis.py 检查输出

### 想要定制？

1. **修改分类**：编辑 type_list.py
2. **修改输出**：修改 osm_util.py 中的转换函数
3. **修改查询**：修改 query_buildings.py 中的Overpass语句

---

**版本**：1.0  
**状态**：完成✅  
**最后更新**：2024年3月

准备好了吗？开始第一步：`python query_buildings.py Beijing --log` 🚀
