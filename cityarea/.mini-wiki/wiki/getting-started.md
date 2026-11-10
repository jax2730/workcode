# 快速开始

## 运行前提

| 项 | 说明 |
|---|---|
| Python | 建议 `3.10+` |
| 关键依赖 | `numpy` `pandas` `geopandas` `shapely` `gdal` `networkx` |
| 数据目录 | 默认使用 [constants.py](/E:/C++pandan/workcode/cityarea/constants.py:4) 中的 `data/` |
| 主要工作区 | `server/city_process/` |

## 环境准备

参考仓库自带说明，可先建立虚拟环境并安装 GIS 相关依赖。若只阅读聚类脚本，最关键的是确保：

- `numpy`
- `pandas`
- `scikit-learn`
- `geopandas`
- `fiona` / `pyogrio`

可用的基础安装命令：

```powershell
python -m venv .venv
.\.venv\Scripts\Activate.ps1
python -m pip install -U pip
python -m pip install -r requirements.txt
python -m pip install scikit-learn
```

## 聚类运行顺序

```mermaid
flowchart LR
    A["extract_block_features.py"] --> B["cluster_block_archetypes.py"]
    B --> C["validate_cluster_reconstruction.py"]
    C --> D["auto_optimize_cluster_count.py"]
    D --> E["export_block_cluster_gpkg.py"]
```

## 典型命令

### 1. 生成 block 特征

```powershell
python server/city_process/extract_block_features.py <city>
```

### 2. 跑一次基础聚类

```powershell
python server/city_process/cluster_block_archetypes.py <city>
```

### 3. 做重建验证

```powershell
python server/city_process/validate_cluster_reconstruction.py <city>
```

### 4. 做自动调参

```powershell
python server/city_process/auto_optimize_cluster_count.py <city>
```

### 5. 导出聚类结果图层

```powershell
python server/city_process/export_block_cluster_gpkg.py <city> --include-reconstruction
```

## 常见注意点

- `_block_id` 是整条链路的关键主键。导出阶段必须和特征提取阶段保持一致。
- `cluster_block_archetypes.py` 会排除 `construction` 等值后再重归一化，因此 `block_cluster_feature_matrix.csv` 与原始特征表不完全等价。
- `auto_optimize_cluster_count.py` 不是简单遍历簇数，而是在多组参数和多 seed 上比较综合评分。

## 下一步阅读

- [architecture.md](/E:/C++pandan/workcode/cityarea/.mini-wiki/wiki/architecture.md:1)
- [城市聚类系统/聚类流水线.md](/E:/C++pandan/workcode/cityarea/.mini-wiki/wiki/城市聚类系统/聚类流水线.md:1)
- [docs/clustering_algorithm_parameters.md](/E:/C++pandan/workcode/cityarea/docs/clustering_algorithm_parameters.md:1)

**Section sources**
- [README.md](/E:/C++pandan/workcode/cityarea/README.md:18)
- [requirements.txt](/E:/C++pandan/workcode/cityarea/requirements.txt:1)
- [constants.py](/E:/C++pandan/workcode/cityarea/constants.py:4)
