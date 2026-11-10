# 全北京建筑数据查询系统 - 文档导航

## 🎯 我想...（选择开始）

### 🚀 我想快速开始（5分钟）

→ 查看：**[QUICK_START.md](QUICK_START.md)**  

- 3步快速完成
- 命令一键运行
- 立即在QGIS中查看

### 📖 我想了解详细功能（15分钟）

→ 查看：**[README_ALL_BUILDING.md](README_ALL_BUILDING.md)**  

- 功能完整说明
- 数据字段解释
- QGIS着色教程
- 常见问题解答

### 🔧 我想了解技术实现细节（30分钟）

→ 查看：**[TECH_REFERENCE.md](TECH_REFERENCE.md)**  

- 系统架构
- 代码实现
- 算法说明
- 性能指标

### 🏗️ 我想深入研究代码（开发者）

→ 查看：**[IMPLEMENTATION_SUMMARY.md](IMPLEMENTATION_SUMMARY.md)**  

- 完整代码实现
- 新增/修改文件清单
- 集成方式
- 扩展指南

### 📚 我想查看API和完整文档（全面参考）

→ 查看：**[server/overpass_query/BUILDINGS_QUERY_GUIDE.md](server/overpass_query/BUILDINGS_QUERY_GUIDE.md)**  

- 完整API文档
- 参数说明
- 输出格式规范
- 集成建议

### 🔍 我想分析已生成的GPKG数据

→ 使用：**[demo_building_analysis.py](demo_building_analysis.py)**  

```bash
python demo_building_analysis.py "D:\Beijing\All_Building.gpkg" --show-stats
```

---

## 📊 文档速查表

| 文档 | 用途 | 时间 | 对象 |
|------|------|------|------|
| **QUICK_START.md** | 快速上手 | 5分钟 | 所有人 ⭐ |
| **README_ALL_BUILDING.md** | 功能详解 | 15分钟 | 用户/分析师 |
| **TECH_REFERENCE.md** | 技术参考 | 30分钟 | 开发者/技术人员 |
| **BUILDINGS_QUERY_GUIDE.md** | 全面手册 | 45分钟 | 开发者 |
| **IMPLEMENTATION_SUMMARY.md** | 实现细节 | 30分钟 | 开发者 |
| **demo_building_analysis.py** | 数据分析工具 | 运行时 | 所有人 |

---

## 💻 系统文件结构

```
cityarea/
├── 【新增/修改】
├── QUICK_START.md                      ⭐ 快速3步开始
├── README_ALL_BUILDING.md              ⭐ 中文功能说明
├── TECH_REFERENCE.md                   📚 技术参考
├── IMPLEMENTATION_SUMMARY.md           📖 实现完整说明
├── demo_building_analysis.py           🔍 数据分析工具
│
└── server/
    └── overpass_query/
        ├── query_buildings.py          ⭐ 主查询脚本（用户运行）
        ├── osm_util.py                 ✏️  修改：添加BuildingHandler等
        ├── BUILDINGS_QUERY_GUIDE.md    📚 详细英文文档
        └── query_landuse.py            （已有：补充数据）
```

---

## 🚦 典型工作流

### 场景1：我想在QGIS中查看全北京建筑

**所需文档**：QUICK_START.md  
**完成时间**：30分钟

```bash
# 步骤1：在服务器上运行
ssh zhoujiajun@server
cd /home/zhoujiajun/overpass_query
python query_buildings.py Beijing --log      # 15-30分钟

# 步骤2：下载文件
# 实际文件位置：/home/pixel/geo_tools/data/city/Beijing/All_Building.gpkg

# 步骤3：在QGIS中打开
# QGIS → 图层 → 添加图层 → 选择All_Building.gpkg → 添加
```

### 场景2：我想分析建筑分布和统计

**所需文档**：README_ALL_BUILDING.md + demo_building_analysis.py  
**完成时间**：60分钟

```bash
# 步骤1：下载GPKG文件（同上）

# 步骤2：运行分析工具
python demo_building_analysis.py "D:\Beijing\All_Building.gpkg" --show-stats --show-sample

# 步骤3：在QGIS中着色
# 右键图层 → 符号 → 分类 → category → 着色
```

### 场景3：我想定制分类规则

**所需文档**：TECH_REFERENCE.md + IMPLEMENTATION_SUMMARY.md  
**完成时间**：90分钟

```bash
# 步骤1：编辑分类定义
# 编辑：/home/pixel/geo_tools/data/type_list.py
# 修改：category_mapping 字典

# 步骤2：重新生成数据
python query_buildings.py Beijing --log

# 步骤3：在QGIS中验证
```

### 场景4：我想集成到其他分析中

**所需文档**：BUILDINGS_QUERY_GUIDE.md + IMPLEMENTATION_SUMMARY.md  
**完成时间**：120分钟+

```bash
# 步骤1：理解API
# 查看 osm_util.py 中的：
# - BuildingHandler 类
# - convert_building_osm_2_gpkg() 函数

# 步骤2：修改源代码（或调用API）
# 可以扩展BuildingHandler添加自定义属性
# 可以修改_map_building_to_category()自定义分类

# 步骤3：与其他功能集成
# classify_city_sector.py + All_Building.gpkg
# → 生成完整的城市分析模型
```

---

## ❓ 常见问题速查

**建筑数据**

- 建筑数据从哪里来？ → README_ALL_BUILDING.md - 数据说明
- 数据准确性如何？ → README_ALL_BUILDING.md - 常见问题
- 如何更新数据？ → QUICK_START.md - 步骤1

**技术相关**

- 如何修改分类？ → TECH_REFERENCE.md - 自定义分类
- 查询需要多久？ → QUICK_START.md 或 TECH_REFERENCE.md - 性能指标
- 如何让查询更快？ → BUILDINGS_QUERY_GUIDE.md - 故障排除

**QGIS相关**

- 如何导入GPKG？ → QUICK_START.md - 步骤3
- 如何按分类着色？ → QUICK_START.md - 美化地图 或 README_ALL_BUILDING.md
- 如何导出数据？ → README_ALL_BUILDING.md - 常见问题

**数据分析**

- 如何查看统计？ → QUICK_START.md 或 TECH_REFERENCE.md - 数据分析
- 如何理解数据？ → README_ALL_BUILDING.md - 数据说明 或 TECH_REFERENCE.md

---

## 🔗 快速链接

### 文档

- [快速开始（5分钟）](QUICK_START.md)
- [功能说明（15分钟）](README_ALL_BUILDING.md)
- [技术参考（30分钟）](TECH_REFERENCE.md)
- [完整手册（45分钟）](server/overpass_query/BUILDINGS_QUERY_GUIDE.md)

### 工具

- [数据分析脚本](demo_building_analysis.py)
- [主查询脚本](server/overpass_query/query_buildings.py)

### 关键文件

- [BuildingHandler代码](server/overpass_query/osm_util.py) - 搜索 "class BuildingHandler"
- [分类定义](../type_list.py) （在上级目录）
- [配置](server/constants.py)

---

## 🎓 学习路径

### 第一次使用者

1. 阅读：QUICK_START.md（5分钟）
2. 运行：`query_buildings.py Beijing --log`（20分钟）
3. 打开：All_Building.gpkg 在 QGIS（5分钟）

**总计**：30分钟，掌握基本用法 ✅

### 想深入了解的用户

1. 阅读：README_ALL_BUILDING.md（15分钟）
2. 运行：`demo_building_analysis.py`（5分钟）
3. 在QGIS中着色和分析（20分钟）

**总计**：40分钟，全面理解功能 ✅

### 开发者/技术人员

1. 阅读：TECH_REFERENCE.md（30分钟）
2. 查看：IMPLEMENTATION_SUMMARY.md（30分钟）
3. 研究代码：osm_util.py 和 query_buildings.py（30分钟）
4. 实验修改（根据需求）

**总计**：90分钟+，完全掌握技术细节 ✅

---

## 📋 检查清单

### 开始前

- [ ] 对应文档已读
- [ ] 服务器访问权限确认
- [ ] Python依赖已安装（geopandas, osmium）
- [ ] 磁盘空间充足（200+ MB）

### 执行期间

- [ ] 查询命令正确
- [ ] 查看日志确认进度
- [ ] 输出文件成功生成

### 完成后

- [ ] GPKG文件能在QGIS打开
- [ ] 数据着色成功
- [ ] 统计分析正确

---

## 🚀 现在就开始

**最快路径**（5分钟）：

```
1. 打开 QUICK_START.md
2. 按照3个步骤操作
3. 完成！
```

**推荐路径**（40分钟）：

```
1. 阅读 QUICK_START.md（5分钟）
2. 阅读 README_ALL_BUILDING.md（15分钟）
3. 运行 query_buildings.py（15分钟）
4. 在QGIS中查看和分析（5分钟）
```

**完整路径**（2小时+）：

```
1. QUICK_START.md
2. README_ALL_BUILDING.md
3. TECH_REFERENCE.md
4. BUILDINGS_QUERY_GUIDE.md（可选）
5. 查看源代码
6. 运行测试和实验
```

---

## 📞 需要帮助？

1. **查看错误日志**：加上 `--log` 参数重新运行
2. **查看对应文档**：找到问题所在，查看相关文档
3. **查看代码注释**：源代码中有详细注释
4. **运行分析工具**：`demo_building_analysis.py` 验证输出

---

**选择你的入口点，开始探索！** 🎯

- ⭐ 我很急，就想快速开始 → [QUICK_START.md](QUICK_START.md)
- 📖 我想全面了解功能 → [README_ALL_BUILDING.md](README_ALL_BUILDING.md)  
- 🔧 我想研究技术细节 → [TECH_REFERENCE.md](TECH_REFERENCE.md)
- 💻 我想看完整代码实现 → [IMPLEMENTATION_SUMMARY.md](IMPLEMENTATION_SUMMARY.md)

---

**版本**：1.0  
**更新**：2024年3月  
**状态**：✅ 完成并测试
