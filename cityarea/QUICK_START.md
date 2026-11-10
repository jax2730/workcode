# 🚀 快速开始 - 全北京建筑数据

## 【现在就做】3步生成All_Building.gpkg

### 步骤1️⃣：在服务器上查询（15-30分钟）

```bash
# 登录服务器
ssh zhoujiajun@<server_ip>

# 进入查询目录
cd /home/zhoujiajun/overpass_query

# 开始查询
python query_buildings.py Beijing --log
```

**搞定！** 输出文件会自动保存到：

```
/home/pixel/geo_tools/data/city/Beijing/All_Building.gpkg
```

---

### 步骤2️⃣：下载到本地Windows

使用FTP/SCP下载文件到本地：

```
D:\Beijing\All_Building.gpkg
```

---

### 步骤3️⃣：在QGIS中查看

1. **打开QGIS** → 图层 → 添加图层 → 矢量
2. **选择** `D:\Beijing\All_Building.gpkg`  
3. **选择** `building` 图层
4. **点击** 添加

**完成！🎉** 你现在看到全北京的建筑了。

---

## 💡 美化地图 - 按分类着色

在QGIS中：

1. **右键点击图层** → 属性 → 符号
2. **从"单一符号"改为"分类"**
3. **选择列：`category`**
4. **点击"分类"按钮**
5. **为每个分类选择颜色：**
   - Residential（蓝色） - 居住
   - Commercial（红色） - 商业
   - PublicServices（黄色） - 公共服务
   - Industrial（灰色） - 工业
   - GreenSpace（绿色） - 绿地
   - Others（浅灰） - 其他

---

## 📊 分析数据

在Windows上了解数据统计：

```bash
python demo_building_analysis.py "D:\Beijing\All_Building.gpkg" --show-stats
```

输出示例：

```
BUILDING DATA STATISTICS
=====================================
Total buildings:        298,456
Total area:            1,452.35 km²

By Category:
  Residential         156,234 (52.3%)
  Commercial           78,901 (26.4%)
  PublicServices       32,145 (10.8%)
  Industrial           18,923 ( 6.3%)
  GreenSpace           12,253 ( 4.1%)
```

---

## 🔍 数据说明

**输出文件包含：**

| 字段 | 说明 |
|------|------|
| `id` | OSM ID |
| `building` | 建筑类型（residential, commercial等） |
| `area_m2` | 建筑面积（平方米） |
| `category` | 分类（Residential/Commercial等） |
| `building:levels` | 楼层数 |
| `name:local` | 中文名称 |
| `geometry` | 建筑多边形 |

---

## ⚡ 更多选项

### 增加搜索范围

默认50km，想要更大范围：

```bash
python query_buildings.py Beijing --radius 60000 --log
```

### 查询其他城市

```bash
python query_buildings.py Shanghai --log
python query_buildings.py Chengdu --log
python query_buildings.py Xi'an --radius 40000 --log
```

### 自定义分类

编辑 `/home/pixel/geo_tools/data/type_list.py`，修改 `category_mapping` 字典，然后重新运行。

---

## 📖 详细文档

如需更多信息：

- **完整指南**：`BUILDINGS_QUERY_GUIDE.md`
- **中文简明版**：`README_ALL_BUILDING.md`  
- **实现细节**：`IMPLEMENTATION_SUMMARY.md`

---

## ❓ 常见问题

**Q: 查询需要多久？**  
A: 15-30分钟，取决于网络和本地OSM缓存

**Q: 文件有多大？**  
A: ~120 MB（包含约30万个建筑）

**Q: 数据准确吗？**  
A: 来自OpenStreetMap，北京市区数据较完整，郊区可能有缺陷

**Q: 能导出为其他格式吗？**  
A: 可以。QGIS中右键 → 导出为 → Shapefile/GeoJSON等

---

## 🎯 下一步

1. ✅ 生成 `All_Building.gpkg`
2. ✅ 在QGIS中着色查看
3. → 生成 `landuse.gpkg` 对比验证
4. → 运行 `classify_city_sector.py` 进行行政区分类
5. → 用于城市规划分析

---

**需要帮助？** 检查日志输出或查看详细文档 📚

**准备好了吗？** 开始第一步：`python query_buildings.py Beijing --log` 🚀
