# 聚类相关算法与参数说明

本文用于配合以下脚本阅读：

- `server/city_process/cluster_block_archetypes.py`
- `server/city_process/validate_cluster_reconstruction.py`
- `server/city_process/auto_optimize_cluster_count.py`
- `server/city_process/optimize_cluster_for_target.py`
- `server/city_process/export_block_cluster_gpkg.py`

重点说明：

- 聚类算法本身的结构与数学背景
- 每个参数的作用、用途、传递链路
- 各脚本之间如何串接
- 关键数学量为什么这样定义

---

## 1. 总体调用链路

完整流程可概括为：

1. `extract_block_features.py`
2. `cluster_block_archetypes.py`
3. `validate_cluster_reconstruction.py`
4. `auto_optimize_cluster_count.py`
5. `optimize_cluster_for_target.py`（可选，做二次修补）
6. `export_block_cluster_gpkg.py`

其中真正的调用关系是：

- `auto_optimize_cluster_count.py`
  - 反复调用 `cluster_block_archetypes.py` 中的 `cluster_blocks(...)`
  - 再调用 `validate_cluster_reconstruction.py` 中的 `validate_reconstruction(...)`
- `optimize_cluster_for_target.py`
  - 不再从头聚类
  - 而是在既有 `assignment + feature matrix` 上继续局部拆分
- `export_block_cluster_gpkg.py`
  - 不参与聚类计算
  - 只负责把最终结果回写到 block 几何

因此，这套系统的核心不是“跑一次 KMeans 就结束”，而是：

- 先构造 block 的组成向量
- 再生成 cluster archetype
- 再检查 archetype 是否真能代表其成员
- 再用搜索或贪心迭代改进聚类结构

---

## 2. 特征空间定义

对每个 block，聚类使用的是 `val_pct_*` 特征列构成的向量：

$$
\mathbf{x}_i = (x_{i1}, x_{i2}, \dots, x_{id})
$$

其中：

- $i$ 表示第 $i$ 个 block
- $d$ 表示保留下来的 value 维度数
- $x_{ij} \ge 0$

在排除 `construction` 或删除弱维度之后，会重新归一化，使得：

$$
\sum_{j=1}^{d} x_{ij} = 100
$$

严格来说，代码做的是：

$$
x'_{ij} = \frac{x_{ij}}{\sum_{k=1}^{d} x_{ik}} \times 100
$$

其中：

- $x_{ij}$ 是删维之前剩余维度的值
- $x'_{ij}$ 是删维后重新归一化后的值

### 这样做的原因

如果不重归一化，那么：

- 删除某些维度后，每一行剩余总和不再一致
- 欧式距离会同时受到“结构差异”和“剩余总量差异”的影响

而重归一化后：

- 距离更专注于“组成结构”
- centroid 更接近“典型组成原型”

---

## 3. `cluster_block_archetypes.py` 的算法原理

### 3.1 两阶段聚类框架

该脚本不是直接全局 KMeans，而是两阶段：

1. 先按 dominant value 家族分 macro bucket
2. 再在每个 macro bucket 内做局部 KMeans

---

### 3.2 第一阶段：macro bucket

对每个 block，先找最大组成维度：

$$
\mathrm{dominant}(i) = \arg\max_j x'_{ij}
$$

然后取该维度名称前缀作为 macro bucket key。

例如：

- `commercial_retail`
- `commercial_office`

都可能进入 `commercial` 这个大桶。

### 这样做的意义

如果直接全局 KMeans，可能出现：

- 商业主导 block
- 居住主导 block

被一起参与同一组中心竞争，导致边界先天混乱。

分桶后，相当于先做“粗分类”，再做“桶内细分”。

---

### 3.3 第二阶段：局部 KMeans

对于每个 macro bucket：

- 若样本数太少，则不再继续拆
- 若样本数足够大，则在该 bucket 内搜索合适的局部簇数 $k$

最终得到 bucket 内多个局部簇，并将它们映射成全局 `cluster_id`。

---

### 3.4 silhouette 选 k

对样本 $i$：

- $a(i)$：样本 $i$ 到本簇其他点的平均距离
- $b(i)$：样本 $i$ 到最近其他簇的平均距离

silhouette 系数定义为：

$$
s(i) = \frac{b(i) - a(i)}{\max(a(i), b(i))}
$$

其均值越大，说明：

- 类内更紧
- 类间更分离

代码中对候选 $k \in [k_{\min}, k_{\max}]$，分别拟合一次 KMeans，并取平均 silhouette 最大的 $k$：

$$
k^\* = \arg\max_k \ \frac{1}{n}\sum_{i=1}^{n} s_k(i)
$$

其中：

- $s_k(i)$ 表示在候选簇数 $k$ 下样本 $i$ 的 silhouette

---

### 3.5 centroid 原型

对 cluster $c$，其 archetype 原型定义为成员均值向量：

$$
\mathbf{\mu}_c = \frac{1}{|C_c|}\sum_{i \in C_c} \mathbf{x}_i
$$

其中：

- $C_c$ 表示 cluster $c$ 中的样本集合
- $|C_c|$ 表示该集合大小

这个 centroid 有两个用途：

1. 在 `cluster_block_archetypes.py` 中，用作 cluster prototype 输出
2. 在 `validate_cluster_reconstruction.py` 中，用作重建模板

---

## 4. `validate_cluster_reconstruction.py` 的数学背景

这个脚本的核心问题是：

> 每个 cluster 的 centroid，能否代表其成员 block 的组成结构？

---

### 4.1 残差向量

对某个 block $i$，设其真实向量为 $\mathbf{x}_i$，所属 cluster 原型为 $\mathbf{\mu}_{c(i)}$。

定义残差：

$$
\mathbf{r}_i = \mathbf{x}_i - \mathbf{\mu}_{c(i)}
$$

---

### 4.2 MAE

平均绝对误差定义为：

$$
\mathrm{MAE}_i = \frac{1}{d}\sum_{j=1}^{d} |x_{ij} - \mu_{c(i),j}|
$$

作用：

- 衡量该 block 相对其类原型的平均逐维偏差

---

### 4.3 RMSE

均方根误差定义为：

$$
\mathrm{RMSE}_i = \sqrt{\frac{1}{d}\sum_{j=1}^{d} (x_{ij} - \mu_{c(i),j})^2}
$$

作用：

- 对大偏差更敏感
- 若某些 value 维度偏得非常离谱，RMSE 会比 MAE 更快拉高

---

### 4.4 Cosine similarity

余弦相似度定义为：

$$
\cos(\mathbf{x}_i, \mathbf{\mu}_{c(i)}) =
\frac{\mathbf{x}_i \cdot \mathbf{\mu}_{c(i)}}{\|\mathbf{x}_i\|\|\mathbf{\mu}_{c(i)}\|}
$$

作用：

- 看“结构方向是否相似”
- 不过分受整体量纲放缩影响

在本项目里，cosine 是最核心的质量指标之一。

---

### 4.5 Top-k overlap

设：

- $\mathrm{TopK}(\mathbf{x})$ 表示向量 $\mathbf{x}$ 中值最大的前 $k$ 个非零维度集合

则 overlap 定义为集合交并比：

$$
\mathrm{Overlap}_k(\mathbf{x}, \mathbf{\mu}) =
\frac{|\mathrm{TopK}(\mathbf{x}) \cap \mathrm{TopK}(\mathbf{\mu})|}
{|\mathrm{TopK}(\mathbf{x}) \cup \mathrm{TopK}(\mathbf{\mu})|}
$$

作用：

- 只关心主导 value 是否一致
- 不关心这些 value 的精确百分比差了多少

---

### 4.6 good block

代码中用阈值定义“好重建 block”：

$$
\mathrm{good}(i) =
\mathbb{1}\left[\cos(\mathbf{x}_i, \mathbf{\mu}_{c(i)}) \ge \tau\right]
$$

其中：

- $\tau$ 就是 `good_cosine_threshold`

全局好块比例为：

$$
\mathrm{GoodRate} =
\frac{1}{N}\sum_{i=1}^{N} \mathrm{good}(i)
$$

若输出为百分比，则再乘以 $100$。

---

### 4.7 zero vector

若某个 block 在参与聚类/验证的维度上几乎没有有效值，即：

$$
\sum_{j=1}^{d} x_{ij} \le \varepsilon
$$

其中：

- $\varepsilon$ 就是 `zero_eps`

则该样本视作 zero vector。

原因：

- 它没有稳定的方向信息
- cosine 分母容易退化
- 留在全局评价里会稀释有效统计

---

## 5. `auto_optimize_cluster_count.py` 的搜索与评分

这个脚本本质是：

- 参数网格搜索
- 多随机种子重复试验
- 用重建质量 + 稳定性 + 碎片化惩罚来选最优组合

---

### 5.1 参数网格

搜索空间由以下参数列表做笛卡尔积生成：

- `max_global_list`
- `min_value_pct_list`
- `local_k_min_list`
- `local_k_divisor_list`
- `min_bucket_size_list`

如果这些列表长度分别为：

$$
m_1, m_2, m_3, m_4, m_5
$$

则总参数组合数为：

$$
M = m_1 m_2 m_3 m_4 m_5
$$

若每个组合再测试 $S$ 个 seed，则总试验数为：

$$
M \times S
$$

---

### 5.2 单次 trial 评分函数

对一次 trial，代码使用如下复合目标函数：

$$
\mathrm{Score} =
w_c \cdot \mathrm{CosMean}
 + w_g \cdot \frac{\mathrm{GoodPct}}{100}
 + w_o \cdot \mathrm{OverlapMean}
 - w_k \cdot \log(1 + K)
 - w_t \cdot \mathrm{TinyRatio}
 - w_s \cdot \mathrm{SingletonRatio}
$$

其中：

- $\mathrm{CosMean}$：全局平均 cosine
- $\mathrm{GoodPct}$：好 block 百分比
- $\mathrm{OverlapMean}$：全局平均 top-k overlap
- $K$：有效 cluster 数
- $\mathrm{TinyRatio}$：小簇比例
- $\mathrm{SingletonRatio}$：单点簇比例

解释：

- 前 3 项奖励“重建得好”
- 后 3 项惩罚“分得太碎”

---

### 5.3 tiny / singleton 惩罚

若 cluster 总数为 $K$，第 $c$ 个 cluster 的 block 数为 $n_c$。

则：

$$
\mathrm{TinyRatio} =
\frac{1}{K}\sum_{c=1}^{K}\mathbb{1}[n_c < T]
$$

其中：

- $T$ 为 `tiny_cluster_size`

而 singleton 比例：

$$
\mathrm{SingletonRatio} =
\frac{1}{K}\sum_{c=1}^{K}\mathbb{1}[n_c = 1]
$$

---

### 5.4 ARI 稳定性

同一参数组合下，不同随机 seed 会产生不同分区结果。

代码用 Adjusted Rand Index 比较两个分区：

$$
\mathrm{ARI}(\Pi_a, \Pi_b)
$$

性质：

- $1$：完全一致
- $0$：接近随机一致
- 允许标签编号不同，只看分区结构

若同一参数组合有 $S$ 个 seed，则共有：

$$
\binom{S}{2}
$$

对 trial 结果进行两两比较，得到 ARI 集合，再计算：

$$
\mu_{\mathrm{ARI}} = \mathrm{mean}(\mathrm{ARI})
$$

$$
\sigma_{\mathrm{ARI}} = \mathrm{std}(\mathrm{ARI})
$$

最终组合分数：

$$
\mathrm{ComboScore} =
\mathrm{BaseScore}
 + \alpha \mu_{\mathrm{ARI}}
 - \beta \sigma_{\mathrm{ARI}}
$$

代码中默认：

- $\alpha = 0.08$
- $\beta = 0.04$

解释：

- 平均 ARI 高：说明结果稳定，奖励
- ARI 波动大：说明结果靠运气，惩罚

---

### 5.5 partition cache

代码不是按“参数组合”缓存，而是按“最终 block 到 cluster 的映射结果”缓存。

若两次 trial 的 `_block_id -> cluster_id` 完全一致，则：

- 不再重复运行验证脚本
- 直接复用已有评估结果

数学上，这相当于：

若两个 trial 产生相同分区 $\Pi$，则其 centroid 与所有重建指标都相同，因此验证过程是确定可复用的。

---

## 6. `optimize_cluster_for_target.py` 的贪心迭代拆分

该脚本不是全局重做聚类，而是：

- 读取现有 cluster 结果
- 每轮找到“最差但还能拆”的 cluster
- 对该 cluster 做局部再分裂

目标是把全局好块比例提升到目标值。

---

### 6.1 目标条件

若目标好块百分比为 `target_good_pct`，则内部先换成比例：

$$
\mathrm{TargetGoodRate} = \frac{\mathrm{target\_good\_pct}}{100}
$$

停止条件之一为：

$$
\mathrm{GoodRate} \ge \mathrm{TargetGoodRate}
$$

---

### 6.2 选谁来拆

脚本先生成每个 cluster 的：

- `block_count`
- `cosine_mean`
- `cosine_p10`
- `good_rate`

再按以下顺序排序：

1. `good_rate` 低的优先
2. `cosine_mean` 低的优先
3. `block_count` 大的优先

这是一个贪心启发式：

- 先拆最弱 cluster
- 同样弱时，优先拆影响范围更大的 cluster

---

### 6.3 局部 split_k 策略

设被选中的 cluster 大小为 $n$，最小拆分规模阈值为 $m$。



代码规则近似为：

$$
k_{\mathrm{split}} =
\min\left(
\begin{cases}
2, & n < 4m \\
3, & n \ge 4m
\end{cases},
\max\left(2, \left\lfloor \frac{n}{m} \right\rfloor \right)
\right)
$$

作用：

- 对较小 cluster 保守二分
- 对较大 cluster 允许三分
- 但不希望把子簇平均切得比 `min_split_size` 更小



---

### 6.4 保留原 cluster_id

拆分后：

- 最大子簇保留原 `cluster_id`
- 其他子簇分配新 ID

这不是数学要求，而是工程策略：

- 维持标签连续性
- 减少编号抖动
- 降低人工对比成本

---

## 7. `export_block_cluster_gpkg.py` 的工程契约

该脚本不涉及新的聚类数学，但有一个非常关键的“索引契约”：

### `_block_id` 必须一致

导出时重建 `_block_id` 的方式必须与特征提取完全相同：

1. `reset_index(drop=True)`
2. `index.astype(str)`

否则：

- assignment 表中的 `_block_id`
- block 几何中的 `_block_id`

将不再一一对应，导致 merge 错位。

因此这个脚本本质上是：

- 用 `_block_id` 做精确回写
- 把 cluster 结果与可选的 centroid 重建字段挂回几何对象

---

## 8. 参数说明：作用、用途、传递链路

下面按脚本说明参数。

---

## 8.1 `cluster_block_archetypes.py`

### `city_name`

作用：

- 决定数据目录 `data/city/<city_name>/`

用途：

- 读取输入特征文件
- 写出 assignments / prototypes / feature matrix

传递：

- CLI 参数传入 `cluster_blocks(...)`
- `auto_optimize_cluster_count.py` 也会把它原样传给 `cluster_blocks(...)`

### `long_file`

作用：

- 指定长表输入文件

用途：

- 当 `prefer_wide=False` 或 wide 模式失败时，从长表重建聚类特征

传递：

- CLI -> `cluster_blocks(...)`
- 或由 `auto_optimize_cluster_count.py` 转发到 `cluster_blocks(...)`

### `wide_file`

作用：

- 指定宽表输入文件

用途：

- `prefer_wide=True` 时优先作为聚类输入

传递：

- CLI -> `cluster_blocks(...)`
- 或 `auto_optimize_cluster_count.py` -> `cluster_blocks(...)`

### `prefer_wide`

作用：

- 控制输入优先级

用途：

- `True`：先尝试宽表
- `False`：直接走长表

传递：

- CLI / `auto_optimize_cluster_count.py` -> `cluster_blocks(...)`

### `exclude_values`

作用：

- 指定不参与聚类距离计算的 value

用途：

- 常用于去掉 `construction` 这类暂态或干扰项

传递：

- CLI / `auto_optimize_cluster_count.py` -> `cluster_blocks(...)`
- 在 `build_feature_table(...)` 或 `build_feature_table_from_wide(...)` 中生效

### `min_value_pct`

作用：

- 特征筛选阈值

用途：

- 删除“在全局上几乎不起作用”的弱维度

生效位置：

- `build_feature_table(...)`
- `build_feature_table_from_wide(...)`

传递：

- CLI / `auto_optimize_cluster_count.py` -> `cluster_blocks(...)` -> 特征构建函数

### `max_global_clusters`

作用：

- 限制每个 macro bucket 可搜索的局部最大簇数

用途：

- 控制局部聚类复杂度上界

注意：

- 名字叫 global，但实际代码语义更接近“局部 k 上限 cap”

传递：

- CLI / `auto_optimize_cluster_count.py` -> `cluster_blocks(...)` -> `run_two_stage_clustering(...)`

### `random_state`

作用：

- 控制 KMeans 初始化和局部 k 搜索可复现性

用途：

- 让同样参数下结果可重复
- 在 `auto_optimize_cluster_count.py` 中，也用来生成跨 seed 稳定性比较

传递：

- CLI / `auto_optimize_cluster_count.py` -> `cluster_blocks(...)` -> `run_two_stage_clustering(...)`

### `local_k_min`

作用：

- 每个 macro bucket 局部 k 搜索的最小值

用途：

- 限制 silhouette 搜索下界

传递：

- CLI / `auto_optimize_cluster_count.py` -> `cluster_blocks(...)` -> `run_two_stage_clustering(...)`

### `local_k_divisor`

作用：

- 决定 `local_k_max ≈ n / divisor` 的增长速度

用途：

- 控制局部聚类保守还是激进

传递：

- CLI / `auto_optimize_cluster_count.py` -> `cluster_blocks(...)` -> `run_two_stage_clustering(...)`

### `min_bucket_size`

作用：

- 规定多小的 macro bucket 不再继续细分

用途：

- 避免把样本过少的桶硬拆成不稳定小簇

传递：

- CLI / `auto_optimize_cluster_count.py` -> `cluster_blocks(...)` -> `run_two_stage_clustering(...)`

### `verbose`

作用：

- 控制日志输出

用途：

- 调试时看详细过程

传递：

- CLI -> `cluster_blocks(...)`
- 在 `auto_optimize_cluster_count.py` 中通过 `loop_verbose` 间接控制

---

## 8.2 `validate_cluster_reconstruction.py`

### `city_name`

作用：

- 定位城市目录

传递：

- CLI -> `validate_reconstruction(...)`
- 或 `auto_optimize_cluster_count.py` -> `validate_reconstruction(...)`

### `feature_file`

作用：

- 指定聚类特征矩阵文件

用途：

- 用它恢复每个 block 的真实特征向量

传递：

- CLI / `auto_optimize_cluster_count.py` -> `validate_reconstruction(...)`

### `assignment_file`

作用：

- 指定 cluster assignment 文件

用途：

- 读取 `_block_id -> cluster_id`

传递：

- CLI / `auto_optimize_cluster_count.py` -> `validate_reconstruction(...)`

### `topk`

作用：

- 决定 top-k overlap 的 $k$

用途：

- 控制“主导 value 重合度”的关注范围

传递：

- CLI / `auto_optimize_cluster_count.py` -> `validate_reconstruction(...)` -> `_topk_overlap(...)`

### `good_cosine_threshold`

作用：

- 定义好 block 的 cosine 阈值

用途：

- 决定 `good_blocks_pct`

传递：

- CLI / `auto_optimize_cluster_count.py` -> `validate_reconstruction(...)`

### `skip_zero_vectors`

作用：

- 控制 zero vector 是否排除在全局统计分母外

用途：

- 防止无信息样本稀释评价

传递：

- CLI / `auto_optimize_cluster_count.py` -> `validate_reconstruction(...)`

### `zero_eps`

作用：

- zero vector 判定阈值

用途：

- 判定 $\sum_j x_{ij}$ 是否足够小到应被视作零向量

传递：

- CLI / `auto_optimize_cluster_count.py` -> `validate_reconstruction(...)`

### `verbose`

作用：

- 控制详细日志输出

传递：

- CLI -> `validate_reconstruction(...)`
- `auto_optimize_cluster_count.py` 会将其设为 `False` 并用外部静默包装

---

## 8.3 `auto_optimize_cluster_count.py`

### `city_name`

作用：

- 指定城市目录

用途：

- 统一传给聚类与验证函数

### `long_file` / `wide_file` / `prefer_wide` / `exclude_values`

作用：

- 控制聚类输入与预处理

传递链路：

- `auto_optimize(...)`
- `-> _call_cluster_blocks_compat(...)`
- `-> cluster_blocks(...)`

### `topk`

作用：

- 控制验证时的 top-k overlap

传递链路：

- `auto_optimize(...)`
- `-> _call_validate_compat(...)`
- `-> validate_reconstruction(...)`

### `good_threshold`

作用：

- 控制“好 block”的 cosine 阈值

传递链路：

- `auto_optimize(...)`
- `-> validate_reconstruction(good_cosine_threshold=...)`

### `skip_zero_vectors`

作用：

- 控制 zero vector 是否进入全局统计分母

传递链路：

- `auto_optimize(...)`
- `-> validate_reconstruction(skip_zero_vectors=...)`

### `zero_eps`

作用：

- zero vector 阈值

传递链路：

- `auto_optimize(...)`
- `-> validate_reconstruction(zero_eps=...)`

### `max_global_list`

作用：

- 候选 `max_global_clusters` 列表

用途：

- 决定局部复杂度搜索上界的候选集合

传递：

- 外层网格搜索枚举
- 每次 trial 传给 `cluster_blocks(...)`

### `min_value_pct_list`

作用：

- 候选 `min_value_pct` 列表

用途：

- 搜索不同特征稀疏化强度

传递：

- 网格搜索 -> `cluster_blocks(...)`

### `local_k_min_list`

作用：

- 候选 `local_k_min` 列表

用途：

- 搜索局部 k 下界的合理范围

### `local_k_divisor_list`

作用：

- 候选 `local_k_divisor` 列表

用途：

- 搜索“局部簇数增长速度”的合理强度

### `min_bucket_size_list`

作用：

- 候选 `min_bucket_size` 列表

用途：

- 搜索多小的 macro bucket 不应细分

### `seed_list`

作用：

- 为同一参数组合提供多个随机初始化

用途：

- 既看平均表现，也看稳定性

传递：

- 网格搜索内层循环
- 每次 trial 作为 `random_state` 传给 `cluster_blocks(...)`

### `tiny_cluster_size`

作用：

- 定义 tiny cluster 阈值

用途：

- 参与 `tiny_cluster_ratio` 惩罚

生效位置：

- `auto_optimize(...)` 内对 `cluster_eval_df['block_count']` 计算比例

### `weights`

作用：

- 定义各评价项在复合评分中的权重

字段：

- `cosine`
- `good`
- `overlap`
- `cluster_penalty`
- `tiny_penalty`
- `singleton_penalty`

用途：

- 平衡“重建好不好”和“是否过碎”

### `loop_verbose`

作用：

- 是否打印每轮 trial 内部细节

用途：

- 调试搜索过程

---

## 8.4 `optimize_cluster_for_target.py`

### `city_name`

作用：

- 指定城市目录

### `feature_file`

作用：

- 指定特征矩阵输入

用途：

- 每轮计算 block 与 centroid 的 cosine

### `assignment_file`

作用：

- 指定当前 cluster assignment 输入

用途：

- 作为贪心局部拆分的起点

### `target_good_pct`

作用：

- 设定目标好块比例

用途：

- 作为贪心拆分的停止条件之一

### `good_threshold`

作用：

- 定义什么叫“好 block”

用途：

- 直接影响每轮 `good_rate`

### `min_split_size`

作用：

- 规定 cluster 至少多大才允许拆分

用途：

- 防止拆分样本过小的 cluster

### `max_extra_clusters`

作用：

- 规定最多可以新增多少 cluster

用途：

- 限制贪心拆分的复杂度膨胀

### `max_iter`

作用：

- 最大迭代轮数

用途：

- 防止无限循环或过长调优

### `random_state`

作用：

- 控制局部 KMeans 的随机初始化

用途：

- 每轮实际使用 `random_state + iter_index`

### `zero_eps`

作用：

- zero vector 阈值

用途：

- 与验证逻辑一致，避免无信息样本干扰优化指标

---

## 8.5 `export_block_cluster_gpkg.py`

### `city_name`

作用：

- 定位城市目录

### `block_file`

作用：

- 指定 block 几何输入文件

### `block_layer`

作用：

- 指定几何图层

用途：

- 不指定时自动选 `cat_pct_*` 列最多的图层

### `assignment_file`

作用：

- 指定要回写的 cluster assignment 文件

用途：

- 默认优先找 `block_cluster_assignments_bestauto.csv`

### `reconstruction_file`

作用：

- 指定重建结果文件

用途：

- 用于给 GIS 附加 `val_pct_*` archetype 列

### `output_file`

作用：

- 输出 gpkg 文件名

### `output_layer`

作用：

- 输出图层名

### `include_reconstruction`

作用：

- 是否把 centroid 重建字段也一并挂回 geometry

用途：

- 若为 `True`，导出结果不仅有 cluster_id，还有 archetype 组成字段

---

## 9. 参数调优建议

### 想提高重建质量

可优先尝试：

- 提高 `max_global_clusters`
- 降低 `local_k_divisor`
- 运行 `optimize_cluster_for_target.py`

代价：

- cluster 数量增多
- tiny / singleton 风险上升

---

### 想减少碎片化

可优先尝试：

- 提高 `local_k_divisor`
- 提高 `min_bucket_size`
- 提高 `cluster_penalty`
- 提高 `tiny_penalty`
- 提高 `singleton_penalty`

---

### 想提高稳定性

可优先尝试：

- 降低 `max_global_clusters`
- 提高 `min_bucket_size`
- 提高 `min_value_pct`

原因：

- 自由度越高，seed 敏感性通常越强

---

## 10. 输出文件如何理解

### `block_cluster_assignments.csv`

含义：

- 每个 block 被分到哪个 cluster

### `block_cluster_prototypes.csv`

含义：

- 每个 cluster 的原型摘要、规模、construction 统计

### `block_cluster_feature_matrix.csv`

含义：

- 实际参与聚类的归一化特征矩阵

### `cluster_reconstruction_global_eval.csv`

含义：

- 一次聚类结果的全局重建质量摘要

### `cluster_auto_optimize_trials.csv`

含义：

- 每个参数组合、每个 seed 的 trial 级结果

### `cluster_auto_optimize_runs.csv`

含义：

- 每个参数组合聚合后的平均质量与稳定性结果

### `cluster_tuning_log.csv`

含义：

- 贪心拆分过程中每轮的质量变化轨迹

---

## 11. 一句话总结各脚本职责

- `cluster_block_archetypes.py`：生成初始 archetype 聚类
- `validate_cluster_reconstruction.py`：检验 archetype 是否真能代表成员
- `auto_optimize_cluster_count.py`：自动搜索更优超参数
- `optimize_cluster_for_target.py`：对弱 cluster 做局部迭代修补
- `export_block_cluster_gpkg.py`：把结果挂回空间数据
