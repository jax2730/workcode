import argparse
import json
import pathlib

import numpy as np
import pandas as pd


REPO_ROOT = pathlib.Path(__file__).resolve().parents[2]
DEFAULT_DATA_DIR = REPO_ROOT / "data" / "csv_classify"


def _primary_name(col: str) -> str:
    return str(col).replace("primary_pct_", "", 1)


def normalize_primary_features(
    df: pd.DataFrame,
    include_unmapped: bool = False,
    zero_eps: float = 1e-9,
) -> tuple[pd.DataFrame, list[str]]:
    feature_cols = [
        c
        for c in df.columns
        if str(c).startswith("primary_pct_")
        and str(c)
        not in {
            "primary_pct_未映射",
        }
    ]
    if include_unmapped and "primary_pct_未映射" in df.columns:
        feature_cols.append("primary_pct_未映射")
    if not feature_cols:
        raise ValueError("No usable primary_pct_* columns found.")

    norm = df.copy()
    for col in feature_cols:
        norm[col] = pd.to_numeric(norm[col], errors="coerce").fillna(0.0)

    row_sum = norm[feature_cols].sum(axis=1)
    safe_sum = row_sum.where(row_sum > float(zero_eps), np.nan)
    norm[feature_cols] = norm[feature_cols].div(safe_sum, axis=0) * 100.0
    norm[feature_cols] = norm[feature_cols].fillna(0.0)
    norm["primary_feature_sum"] = row_sum.fillna(0.0)
    norm["is_zero_primary_vector"] = row_sum <= float(zero_eps)
    return norm, feature_cols


def _stable_int_key(text: str) -> int:
    out = 0
    for idx, ch in enumerate(str(text)):
        out += (idx + 1) * ord(ch)
    return int(out)


def _select_k_by_silhouette_seeded(
    x: np.ndarray,
    k_min: int,
    k_max: int,
    random_state: int,
) -> int:
    from sklearn.cluster import MiniBatchKMeans
    from sklearn.metrics import silhouette_score

    n = x.shape[0]
    if n < 20:
        return 2 if n >= 2 else 1

    k_min = max(2, min(int(k_min), n - 1))
    k_max = max(k_min, min(int(k_max), n - 1))

    best_k = k_min
    best_score = -1.0
    for k in range(k_min, k_max + 1):
        km = MiniBatchKMeans(n_clusters=k, random_state=random_state, n_init=10, batch_size=1024)
        labels = km.fit_predict(x)
        if len(set(labels)) <= 1:
            continue
        score = silhouette_score(x, labels, metric="euclidean")
        if score > best_score:
            best_score = score
            best_k = k
    return best_k


def run_two_stage_primary_clustering(
    norm_df: pd.DataFrame,
    feature_cols: list[str],
    max_clusters_per_primary: int,
    random_state: int,
    local_k_min: int,
    local_k_divisor: int,
    min_bucket_size: int,
) -> tuple[pd.DataFrame, pd.DataFrame]:
    from sklearn.cluster import MiniBatchKMeans

    work = norm_df[~norm_df["is_zero_primary_vector"]].copy()
    id_cols = [c for c in ["_block_id", "sector_id"] if c in work.columns]
    assignments = work[id_cols].copy()

    dominant_cols = work[feature_cols].idxmax(axis=1)
    assignments["macro_primary"] = dominant_cols.map(_primary_name)

    labels_out = pd.Series(index=work.index, dtype="int64")
    prototype_rows = []
    next_cluster_id = 0

    for macro_primary, group_index in assignments.groupby("macro_primary", sort=False).groups.items():
        idx_list = list(group_index)
        part = work.loc[idx_list, feature_cols].copy()
        n = len(part)

        if n < max(2, int(min_bucket_size)):
            labels = np.zeros(n, dtype=int)
            local_k = 1
        else:
            divisor = max(2, int(local_k_divisor))
            local_k_max = min(int(max_clusters_per_primary), max(2, n // divisor))
            local_k_max = max(2, min(local_k_max, n - 1))
            macro_seed = int(random_state + _stable_int_key(macro_primary) % 100000)
            local_k = _select_k_by_silhouette_seeded(
                part.to_numpy(dtype=float),
                k_min=max(2, int(local_k_min)),
                k_max=local_k_max,
                random_state=macro_seed,
            )
            km = MiniBatchKMeans(n_clusters=local_k, random_state=macro_seed, n_init=10, batch_size=1024)
            labels = km.fit_predict(part.to_numpy(dtype=float))

        for local_label in sorted(set(labels)):
            local_idx = [idx_list[i] for i, label in enumerate(labels) if label == local_label]
            labels_out.loc[local_idx] = next_cluster_id
            centroid = work.loc[local_idx, feature_cols].mean()
            text = build_classification_text(centroid, feature_cols)
            prototype_rows.append(
                {
                    "primary_cluster_id": next_cluster_id,
                    "macro_primary": macro_primary,
                    "local_k_in_macro": int(local_k),
                    "block_count": int(len(local_idx)),
                    **text,
                    **{col: float(centroid[col]) for col in feature_cols},
                }
            )
            next_cluster_id += 1

    assignments["primary_cluster_id"] = labels_out.loc[assignments.index].astype(int).to_numpy()
    return assignments.reset_index(drop=True), pd.DataFrame(prototype_rows)


def _primary_signature(row: pd.Series, feature_cols: list[str], top_n: int = 5) -> str:
    pairs = []
    for col in feature_cols:
        value = float(row.get(col, 0.0))
        if value > 0:
            pairs.append((_primary_name(col), value))
    pairs.sort(key=lambda x: x[1], reverse=True)
    return " | ".join([f"{name}:{pct:.2f}%" for name, pct in pairs[:top_n]])


def build_classification_text(row: pd.Series, feature_cols: list[str]) -> dict[str, str]:
    pairs = [(_primary_name(col), float(row.get(col, 0.0))) for col in feature_cols]
    pairs = [(name, pct) for name, pct in pairs if pct > 0]
    pairs.sort(key=lambda x: x[1], reverse=True)
    if not pairs:
        return {
            "macro_class": "空向量",
            "micro_class": "无有效primary特征",
            "classification_criteria": "有效primary特征总和为0，无法根据用地组成判定分类。",
            "primary_signature": "",
        }

    first_name, first_pct = pairs[0]
    second_name, second_pct = pairs[1] if len(pairs) > 1 else ("", 0.0)
    third_name, third_pct = pairs[2] if len(pairs) > 2 else ("", 0.0)

    if first_pct >= 80:
        micro = f"高纯度{first_name}"
        criteria = f"{first_name}占比{first_pct:.2f}%，单一用地特征显著。"
    elif first_pct >= 60:
        micro = f"{first_name}主导型"
        criteria = f"{first_name}占比{first_pct:.2f}%，构成主要用地；{second_name}占比{second_pct:.2f}%，作为次要混合成分。"
    elif first_pct >= 40 and second_pct >= 20:
        micro = f"{first_name}-{second_name}混合型"
        criteria = f"{first_name}占比{first_pct:.2f}%为第一主导，{second_name}占比{second_pct:.2f}%为第二主导，呈双核心混合结构。"
    elif first_pct >= 40:
        micro = f"{first_name}相对主导型"
        criteria = f"{first_name}占比{first_pct:.2f}%为相对主导，第二成分{second_name}占比{second_pct:.2f}%，其余用地共同构成复合背景。"
    else:
        micro = "多元复合型"
        criteria = f"最高占比为{first_name}{first_pct:.2f}%，未达到单一主导阈值；{second_name}占比{second_pct:.2f}%，{third_name}占比{third_pct:.2f}%，整体呈多元混合结构。"

    return {
        "macro_class": first_name,
        "micro_class": micro,
        "classification_criteria": criteria,
        "primary_signature": " | ".join([f"{name}:{pct:.2f}%" for name, pct in pairs[:5]]),
    }


def compute_clustering_metrics(
    x: pd.DataFrame,
    labels: pd.Series | np.ndarray,
) -> tuple[dict, pd.DataFrame]:
    from sklearn.metrics import calinski_harabasz_score, davies_bouldin_score, silhouette_samples, silhouette_score

    x_arr = np.asarray(x, dtype=float)
    label_arr = np.asarray(labels, dtype=int)
    valid_mask = label_arr >= 0
    x_arr = x_arr[valid_mask]
    label_arr = label_arr[valid_mask]

    sample_count = int(x_arr.shape[0])
    feature_count = int(x_arr.shape[1]) if x_arr.ndim == 2 else 0
    unique_labels = sorted(int(v) for v in set(label_arr.tolist()))
    cluster_count = int(len(unique_labels))

    # WCSS（within-cluster sum of squares）衡量簇内紧凑度：
    # 对每个样本 x_i，找到其簇中心 μ_c，累加 ||x_i - μ_c||^2。
    # 数值越小，说明同簇地块在 10 维用地组成比例上越接近。
    per_cluster_rows = []
    total_wcss = 0.0
    for cluster_id in unique_labels:
        cluster_mask = label_arr == cluster_id
        cluster_x = x_arr[cluster_mask]
        centroid = cluster_x.mean(axis=0)
        squared_distances = ((cluster_x - centroid) ** 2).sum(axis=1)
        cluster_wcss = float(squared_distances.sum())
        total_wcss += cluster_wcss
        per_cluster_rows.append(
            {
                "cluster_id": int(cluster_id),
                "block_count": int(cluster_x.shape[0]),
                "within_cluster_ss": cluster_wcss,
                "mean_squared_distance_to_centroid": float(squared_distances.mean()) if len(squared_distances) else None,
            }
        )

    metrics = {
        "sample_count": sample_count,
        "feature_count": feature_count,
        "cluster_count": cluster_count,
        "within_cluster_ss": total_wcss,
        "mean_within_cluster_ss_per_sample": float(total_wcss / sample_count) if sample_count else None,
        "silhouette_score": None,
        "calinski_harabasz_score": None,
        "davies_bouldin_score": None,
    }

    if 1 < cluster_count < sample_count:
        # Silhouette 轮廓系数：
        # a(i)=样本 i 到同簇样本的平均距离，b(i)=样本 i 到最近其他簇的平均距离。
        # s(i)=(b(i)-a(i))/max(a(i),b(i))，整体取均值；越接近 1，簇越分离且越紧凑。
        sample_silhouette = silhouette_samples(x_arr, label_arr, metric="euclidean")
        metrics["silhouette_score"] = float(silhouette_score(x_arr, label_arr, metric="euclidean"))

        # Calinski-Harabasz：簇间离散度 / 簇内离散度；越大越好。
        metrics["calinski_harabasz_score"] = float(calinski_harabasz_score(x_arr, label_arr))

        # Davies-Bouldin：每个簇与最相似簇的“簇内散度 / 簇间距离”平均值；越小越好。
        metrics["davies_bouldin_score"] = float(davies_bouldin_score(x_arr, label_arr))

        for row in per_cluster_rows:
            cluster_mask = label_arr == row["cluster_id"]
            row["mean_silhouette"] = float(sample_silhouette[cluster_mask].mean())
    else:
        for row in per_cluster_rows:
            row["mean_silhouette"] = None

    return metrics, pd.DataFrame(per_cluster_rows)


def build_pca_projection(
    x: pd.DataFrame,
    labels: pd.Series | np.ndarray,
) -> tuple[pd.DataFrame, dict]:
    from sklearn.decomposition import PCA

    x_arr = np.asarray(x, dtype=float)
    label_arr = np.asarray(labels, dtype=int)

    # PCA 把 10 个 primary_pct_* 维度线性压缩到二维：
    # PC1 = w11*x1 + w12*x2 + ... + w1d*xd，捕捉最大方差方向；
    # PC2 = w21*x1 + w22*x2 + ... + w2d*xd，且与 PC1 正交，捕捉第二大方差方向。
    # 图的横坐标 pca_x 即 PC1，纵坐标 pca_y 即 PC2；点越近，说明 10 维组成比例越相似。
    pca = PCA(n_components=2, random_state=42)
    coords = pca.fit_transform(x_arr)
    coord_df = pd.DataFrame(
        {
            "pca_x": coords[:, 0],
            "pca_y": coords[:, 1],
            "cluster_id": label_arr,
        }
    )
    summary = {
        "pca_x_axis": "PC1: 第一主成分，横坐标，表示10维primary比例中方差最大的线性组合",
        "pca_y_axis": "PC2: 第二主成分，纵坐标，表示与PC1正交的第二大方差线性组合",
        "pca_x_explained_variance_ratio": float(pca.explained_variance_ratio_[0]),
        "pca_y_explained_variance_ratio": float(pca.explained_variance_ratio_[1]),
        "pca_total_explained_variance_ratio": float(pca.explained_variance_ratio_[:2].sum()),
    }
    return coord_df, summary


def write_clustering_plots(
    pca_coords: pd.DataFrame,
    catalog: pd.DataFrame,
    feature_cols: list[str],
    output_prefix: pathlib.Path,
    pca_summary: dict,
) -> dict[str, str]:
    from html import escape

    cluster_plot_path = output_prefix.with_name(output_prefix.name + "_pca_cluster_plot.svg")
    macro_plot_path = output_prefix.with_name(output_prefix.name + "_pca_macro_plot.svg")
    heatmap_path = output_prefix.with_name(output_prefix.name + "_centroid_heatmap.svg")

    x_label = f"PC1 / pca_x（解释方差 {pca_summary['pca_x_explained_variance_ratio']:.1%}）"
    y_label = f"PC2 / pca_y（解释方差 {pca_summary['pca_y_explained_variance_ratio']:.1%}）"

    palette = [
        "#1f77b4",
        "#ff7f0e",
        "#2ca02c",
        "#d62728",
        "#9467bd",
        "#8c564b",
        "#e377c2",
        "#7f7f7f",
        "#bcbd22",
        "#17becf",
        "#4e79a7",
        "#f28e2b",
        "#e15759",
        "#76b7b2",
        "#59a14f",
        "#edc948",
        "#b07aa1",
        "#ff9da7",
        "#9c755f",
        "#bab0ac",
    ]

    def scale_points(df: pd.DataFrame, width: int, height: int, margin: int):
        x_min = float(df["pca_x"].min())
        x_max = float(df["pca_x"].max())
        y_min = float(df["pca_y"].min())
        y_max = float(df["pca_y"].max())
        x_span = x_max - x_min if x_max > x_min else 1.0
        y_span = y_max - y_min if y_max > y_min else 1.0

        def project(x_value: float, y_value: float) -> tuple[float, float]:
            x_svg = margin + (float(x_value) - x_min) / x_span * (width - margin * 2)
            y_svg = height - margin - (float(y_value) - y_min) / y_span * (height - margin * 2)
            return x_svg, y_svg

        return project, (x_min, x_max, y_min, y_max)

    def write_scatter_svg(path: pathlib.Path, color_col: str, title: str, legend_title: str) -> None:
        width = 1180
        height = 780
        margin = 90
        legend_x = 930
        project, bounds = scale_points(pca_coords, width=legend_x - 30, height=height, margin=margin)
        values = list(pd.unique(pca_coords[color_col]))
        color_map = {value: palette[idx % len(palette)] for idx, value in enumerate(values)}
        axis_x1, axis_y1 = margin, height - margin
        axis_x2, axis_y2 = legend_x - 30, margin
        lines = [
            f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" viewBox="0 0 {width} {height}">',
            "<style>text{font-family:'Microsoft YaHei','SimHei',Arial,sans-serif;} .axis{stroke:#333;stroke-width:1.2;} .grid{stroke:#ddd;stroke-width:0.8;} .pt{fill-opacity:0.68;} .small{font-size:12px;} .label{font-size:14px;} .title{font-size:22px;font-weight:700;}</style>",
            f'<rect x="0" y="0" width="{width}" height="{height}" fill="#fbfaf6"/>',
            f'<text x="{width / 2}" y="34" text-anchor="middle" class="title">{escape(title)}</text>',
        ]
        for i in range(6):
            gx = axis_x1 + i * (axis_x2 - axis_x1) / 5
            gy = axis_y1 - i * (axis_y1 - axis_y2) / 5
            lines.append(f'<line x1="{gx:.2f}" y1="{axis_y1}" x2="{gx:.2f}" y2="{axis_y2}" class="grid"/>')
            lines.append(f'<line x1="{axis_x1}" y1="{gy:.2f}" x2="{axis_x2}" y2="{gy:.2f}" class="grid"/>')
        lines.append(f'<line x1="{axis_x1}" y1="{axis_y1}" x2="{axis_x2}" y2="{axis_y1}" class="axis"/>')
        lines.append(f'<line x1="{axis_x1}" y1="{axis_y1}" x2="{axis_x1}" y2="{axis_y2}" class="axis"/>')
        lines.append(f'<text x="{(axis_x1 + axis_x2) / 2}" y="{height - 30}" text-anchor="middle" class="label">{escape(x_label)}</text>')
        lines.append(f'<text x="24" y="{(axis_y1 + axis_y2) / 2}" text-anchor="middle" transform="rotate(-90 24 {(axis_y1 + axis_y2) / 2})" class="label">{escape(y_label)}</text>')
        lines.append(f'<text x="{axis_x1}" y="{axis_y1 + 20}" text-anchor="middle" class="small">{bounds[0]:.1f}</text>')
        lines.append(f'<text x="{axis_x2}" y="{axis_y1 + 20}" text-anchor="middle" class="small">{bounds[1]:.1f}</text>')
        lines.append(f'<text x="{axis_x1 - 10}" y="{axis_y1 + 4}" text-anchor="end" class="small">{bounds[2]:.1f}</text>')
        lines.append(f'<text x="{axis_x1 - 10}" y="{axis_y2 + 4}" text-anchor="end" class="small">{bounds[3]:.1f}</text>')
        for _, row in pca_coords.iterrows():
            x_svg, y_svg = project(row["pca_x"], row["pca_y"])
            color = color_map[row[color_col]]
            lines.append(f'<circle cx="{x_svg:.2f}" cy="{y_svg:.2f}" r="3.2" fill="{color}" class="pt"/>')
        if color_col == "cluster_id":
            centers = pca_coords.groupby("cluster_id")[["pca_x", "pca_y"]].mean()
            for cluster_id, row in centers.iterrows():
                x_svg, y_svg = project(row["pca_x"], row["pca_y"])
                lines.append(f'<text x="{x_svg:.2f}" y="{y_svg:.2f}" text-anchor="middle" dominant-baseline="central" font-size="10" font-weight="700" fill="#111">{escape(str(cluster_id))}</text>')
        lines.append(f'<text x="{legend_x}" y="80" class="label" font-weight="700">{escape(legend_title)}</text>')
        for idx, value in enumerate(values[:34]):
            y = 105 + idx * 18
            lines.append(f'<rect x="{legend_x}" y="{y - 10}" width="10" height="10" fill="{color_map[value]}"/>')
            lines.append(f'<text x="{legend_x + 16}" y="{y}" class="small">{escape(str(value))}</text>')
        if len(values) > 34:
            lines.append(f'<text x="{legend_x}" y="{105 + 34 * 18}" class="small">... 共{len(values)}类</text>')
        lines.append("</svg>")
        path.write_text("\n".join(lines), encoding="utf-8")

    def heat_color(value: float) -> str:
        ratio = max(0.0, min(float(value) / 100.0, 1.0))
        low = np.array([247, 252, 185], dtype=float)
        high = np.array([8, 104, 172], dtype=float)
        rgb = (low + (high - low) * ratio).astype(int)
        return f"#{rgb[0]:02x}{rgb[1]:02x}{rgb[2]:02x}"

    def write_heatmap_svg(path: pathlib.Path) -> None:
        heatmap_df = catalog.sort_values("primary_cluster_id").set_index("primary_cluster_id")[feature_cols]
        cell_w = 88
        cell_h = 22
        left = 86
        top = 86
        width = left + cell_w * len(feature_cols) + 160
        height = top + cell_h * len(heatmap_df) + 130
        lines = [
            f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" viewBox="0 0 {width} {height}">',
            "<style>text{font-family:'Microsoft YaHei','SimHei',Arial,sans-serif;} .small{font-size:11px;} .label{font-size:14px;} .title{font-size:22px;font-weight:700;}</style>",
            f'<rect x="0" y="0" width="{width}" height="{height}" fill="#fbfaf6"/>',
            f'<text x="{width / 2}" y="34" text-anchor="middle" class="title">各最终聚类原型中心的10维primary组成比例</text>',
            f'<text x="{width / 2}" y="{height - 28}" text-anchor="middle" class="label">横轴：10个primary_pct_*维度；颜色：原型中心占比（%）</text>',
            f'<text x="24" y="{top + cell_h * len(heatmap_df) / 2}" text-anchor="middle" transform="rotate(-90 24 {top + cell_h * len(heatmap_df) / 2})" class="label">纵轴：最终 cluster_id</text>',
        ]
        for col_idx, col in enumerate(feature_cols):
            x = left + col_idx * cell_w + cell_w / 2
            lines.append(f'<text x="{x:.2f}" y="{top - 12}" text-anchor="start" transform="rotate(-38 {x:.2f} {top - 12})" class="small">{escape(_primary_name(col))}</text>')
        for row_idx, (cluster_id, row) in enumerate(heatmap_df.iterrows()):
            y = top + row_idx * cell_h
            lines.append(f'<text x="{left - 10}" y="{y + 15}" text-anchor="end" class="small">{escape(str(cluster_id))}</text>')
            for col_idx, col in enumerate(feature_cols):
                x = left + col_idx * cell_w
                value = float(row[col])
                lines.append(f'<rect x="{x}" y="{y}" width="{cell_w}" height="{cell_h}" fill="{heat_color(value)}" stroke="#f7f7f7" stroke-width="0.5"/>')
                if value >= 20:
                    lines.append(f'<text x="{x + cell_w / 2:.2f}" y="{y + 15}" text-anchor="middle" class="small" fill="#fff">{value:.0f}</text>')
        legend_x = left + cell_w * len(feature_cols) + 26
        legend_y = top
        for i in range(11):
            value = i * 10
            lines.append(f'<rect x="{legend_x}" y="{legend_y + i * 18}" width="20" height="18" fill="{heat_color(value)}"/>')
            lines.append(f'<text x="{legend_x + 28}" y="{legend_y + i * 18 + 13}" class="small">{value}%</text>')
        lines.append("</svg>")
        path.write_text("\n".join(lines), encoding="utf-8")

    write_scatter_svg(
        cluster_plot_path,
        color_col="cluster_id",
        title="10维primary组成比例的最终聚类结果（PCA二维投影）",
        legend_title="最终 cluster_id",
    )
    write_scatter_svg(
        macro_plot_path,
        color_col="macro_primary",
        title="第一阶段macro_primary分桶结果（PCA二维投影）",
        legend_title="第一阶段主导用地",
    )
    write_heatmap_svg(heatmap_path)

    return {
        "pca_cluster_plot": str(cluster_plot_path),
        "pca_macro_plot": str(macro_plot_path),
        "centroid_heatmap": str(heatmap_path),
    }


def to_bestauto_prototypes(catalog: pd.DataFrame) -> pd.DataFrame:
    out = pd.DataFrame(
        {
            "cluster_id": catalog["primary_cluster_id"].astype(int),
            "macro_bucket": catalog["macro_primary"],
            "block_count_x": catalog["block_count"].astype(int),
            "top_signature": catalog["primary_signature"].fillna(""),
            "block_count_y": catalog["block_count"].astype(int),
        }
    )

    for col in ["macro_class", "micro_class", "classification_criteria"]:
        if col in catalog.columns:
            out[col] = catalog[col]

    primary_cols = [c for c in catalog.columns if str(c).startswith("primary_pct_")]
    for col in primary_cols:
        out[col] = catalog[col]
    return out


def to_bestauto_assignments(assignments: pd.DataFrame) -> pd.DataFrame:
    out = pd.DataFrame()
    if "_block_id" in assignments.columns:
        out["_block_id"] = assignments["_block_id"]
    if "sector_id" in assignments.columns:
        out["sector_id"] = assignments["sector_id"]
    out["dominant_value"] = assignments["macro_primary"].fillna("zero_primary_vector")
    out["cluster_id"] = assignments["primary_cluster_id"].astype(int)
    if "primary_signature" in assignments.columns:
        out["cluster_signature"] = assignments["primary_signature"].fillna("")
    out["cluster_status"] = assignments["cluster_status"]
    return out


def cluster_primary_merged(
    input_file: pathlib.Path,
    output_prefix: pathlib.Path,
    sheet_name: str = "primary_merged_matrix",
    include_unmapped: bool = False,
    max_clusters_per_primary: int = 40,
    local_k_min: int = 2,
    local_k_divisor: int = 10,
    min_bucket_size: int = 8,
    random_state: int = 42,
    zero_eps: float = 1e-9,
) -> dict:
    if input_file.suffix.lower() in {".xlsx", ".xlsm", ".xls"}:
        df = pd.read_excel(input_file, sheet_name=sheet_name)
    else:
        df = pd.read_csv(input_file)
    norm, feature_cols = normalize_primary_features(df, include_unmapped=include_unmapped, zero_eps=zero_eps)

    assignments, catalog = run_two_stage_primary_clustering(
        norm_df=norm,
        feature_cols=feature_cols,
        max_clusters_per_primary=max_clusters_per_primary,
        random_state=random_state,
        local_k_min=local_k_min,
        local_k_divisor=local_k_divisor,
        min_bucket_size=min_bucket_size,
    )

    clustered_features = norm.loc[~norm["is_zero_primary_vector"], feature_cols].reset_index(drop=True)
    clustered_labels = assignments["primary_cluster_id"].reset_index(drop=True)
    metrics, per_cluster_metrics = compute_clustering_metrics(clustered_features, clustered_labels)
    pca_coords, pca_summary = build_pca_projection(clustered_features, clustered_labels)
    pca_coords = pd.concat(
        [
            assignments[[c for c in ["_block_id", "sector_id", "macro_primary"] if c in assignments.columns]].reset_index(drop=True),
            pca_coords,
        ],
        axis=1,
    )

    all_assignments = df[[c for c in ["_block_id", "sector_id", "primary_dominant", "primary_signature"] if c in df.columns]].copy()
    all_assignments = all_assignments.merge(assignments, on=[c for c in ["_block_id", "sector_id"] if c in all_assignments.columns and c in assignments.columns], how="left")
    all_assignments["primary_cluster_id"] = all_assignments["primary_cluster_id"].fillna(-1).astype(int)
    all_assignments["cluster_status"] = np.where(all_assignments["primary_cluster_id"] >= 0, "clustered", "zero_primary_vector")

    catalog = catalog.sort_values(["macro_class", "block_count"], ascending=[True, False]).reset_index(drop=True)

    assignment_path = output_prefix.with_name(output_prefix.name + "_assignments.csv")
    catalog_path = output_prefix.with_name(output_prefix.name + "_catalog.csv")
    prototypes_bestauto_path = output_prefix.with_name(output_prefix.name + "_prototypes_bestauto.csv")
    assignments_bestauto_path = output_prefix.with_name(output_prefix.name + "_assignments_bestauto.csv")
    xlsx_path = output_prefix.with_name(output_prefix.name + "_classification.xlsx")
    report_path = output_prefix.with_name(output_prefix.name + "_report.json")
    metrics_path = output_prefix.with_name(output_prefix.name + "_metrics.json")
    per_cluster_metrics_path = output_prefix.with_name(output_prefix.name + "_per_cluster_metrics.csv")
    pca_coordinates_path = output_prefix.with_name(output_prefix.name + "_pca_coordinates.csv")
    visualization_notes_path = output_prefix.with_name(output_prefix.name + "_visualization_notes.md")

    prototypes_bestauto = to_bestauto_prototypes(catalog)
    assignments_bestauto = to_bestauto_assignments(all_assignments)
    plot_paths = write_clustering_plots(
        pca_coords=pca_coords,
        catalog=catalog,
        feature_cols=feature_cols,
        output_prefix=output_prefix,
        pca_summary=pca_summary,
    )

    all_assignments.to_csv(assignment_path, index=False, encoding="utf-8-sig")
    catalog.to_csv(catalog_path, index=False, encoding="utf-8-sig")
    prototypes_bestauto.to_csv(prototypes_bestauto_path, index=False, encoding="utf-8-sig")
    assignments_bestauto.to_csv(assignments_bestauto_path, index=False, encoding="utf-8-sig")
    per_cluster_metrics.to_csv(per_cluster_metrics_path, index=False, encoding="utf-8-sig")
    pca_coords.to_csv(pca_coordinates_path, index=False, encoding="utf-8-sig")
    metrics_payload = {
        **metrics,
        **pca_summary,
        "metric_direction": {
            "silhouette_score": "越接近1越好；接近0表示簇边界重叠；小于0表示可能分错簇",
            "calinski_harabasz_score": "越大越好；表示簇间离散度相对簇内离散度更高",
            "davies_bouldin_score": "越小越好；表示簇内更紧凑且簇间更分离",
            "within_cluster_ss": "越小越紧凑；会随样本量和簇数变化，适合比较同一数据集不同参数",
        },
    }
    metrics_path.write_text(json.dumps(metrics_payload, ensure_ascii=False, indent=2), encoding="utf-8")
    visualization_notes_path.write_text(
        "\n".join(
            [
                "# primary组成聚类评分与图示说明",
                "",
                "## 10维如何进入最终聚类",
                "",
                "1. 输入每个地块的10个 `primary_pct_*` 占比列，构成向量 `x_i = [p_1, p_2, ..., p_10]`。",
                "2. 排除 `未映射` 后，对有效primary比例重新归一化，使每个地块的10维有效占比和为100%。",
                "3. 第一阶段按最大占比维度分桶，得到 `macro_primary = argmax_j p_j`。",
                "4. 第二阶段在每个 `macro_primary` 桶内用 MiniBatchKMeans 聚类，并用 silhouette 在候选 `k` 中选局部最优。",
                "5. 所有桶内局部簇重新编号，得到最终 `cluster_id`。",
                "",
                "## 图的横坐标和纵坐标",
                "",
                f"- 横坐标 `pca_x`：{pca_summary['pca_x_axis']}，解释方差 {pca_summary['pca_x_explained_variance_ratio']:.2%}。",
                f"- 纵坐标 `pca_y`：{pca_summary['pca_y_axis']}，解释方差 {pca_summary['pca_y_explained_variance_ratio']:.2%}。",
                f"- 二维图合计保留10维变化量的 {pca_summary['pca_total_explained_variance_ratio']:.2%}。",
                "- 图中每个点是一个地块；点越近，表示原始10维primary组成比例越相似。",
                "- `pca_cluster_plot` 用颜色表示最终 `cluster_id`；数字标注为每个簇在PCA平面中的中心位置。",
                "- `pca_macro_plot` 用颜色表示第一阶段 `macro_primary` 分桶，用来观察第一阶段如何进入第二阶段。",
                "- `centroid_heatmap` 的横轴是10个primary维度，纵轴是最终 `cluster_id`，颜色是该聚类原型中心的占比。",
                "",
                "## 评分指标",
                "",
                "- `silhouette_score`：轮廓系数，越接近1越好。",
                "- `calinski_harabasz_score`：簇间/簇内离散度比，越大越好。",
                "- `davies_bouldin_score`：簇内散度与簇间距离比，越小越好。",
                "- `within_cluster_ss`：簇内平方和，越小表示簇内越紧凑；比较不同参数时应使用同一输入数据。",
            ]
        ),
        encoding="utf-8",
    )
    with pd.ExcelWriter(xlsx_path, engine="openpyxl") as writer:
        prototypes_bestauto.to_excel(writer, sheet_name="prototypes_bestauto_style", index=False)
        catalog.to_excel(writer, sheet_name="classification_catalog", index=False)
        assignments_bestauto.to_excel(writer, sheet_name="assignments_bestauto_style", index=False)
        all_assignments.to_excel(writer, sheet_name="block_assignments", index=False)
        norm.to_excel(writer, sheet_name="normalized_primary_matrix", index=False)
        per_cluster_metrics.to_excel(writer, sheet_name="per_cluster_metrics", index=False)
        pca_coords.to_excel(writer, sheet_name="pca_coordinates", index=False)

    report = {
        "input_file": str(input_file),
        "sheet_name": sheet_name,
        "blocks_total": int(len(df)),
        "blocks_clustered": int((all_assignments["primary_cluster_id"] >= 0).sum()),
        "blocks_zero_primary_vector": int((all_assignments["primary_cluster_id"] < 0).sum()),
        "feature_cols": feature_cols,
        "clusters": int(len(catalog)),
        "parameters": {
            "include_unmapped": include_unmapped,
            "max_clusters_per_primary": max_clusters_per_primary,
            "local_k_min": local_k_min,
            "local_k_divisor": local_k_divisor,
            "min_bucket_size": min_bucket_size,
            "random_state": random_state,
            "zero_eps": zero_eps,
        },
        "outputs": {
            "assignments": str(assignment_path),
            "catalog": str(catalog_path),
            "prototypes_bestauto": str(prototypes_bestauto_path),
            "assignments_bestauto": str(assignments_bestauto_path),
            "classification_xlsx": str(xlsx_path),
            "metrics": str(metrics_path),
            "per_cluster_metrics": str(per_cluster_metrics_path),
            "pca_coordinates": str(pca_coordinates_path),
            "visualization_notes": str(visualization_notes_path),
            **plot_paths,
        },
        "metrics": metrics_payload,
    }
    report_path.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
    return report


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Cluster primary-merged block feature matrix and generate readable classification criteria."
    )
    parser.add_argument(
        "--input-file",
        type=pathlib.Path,
        default=DEFAULT_DATA_DIR / "block_cluster_feature_matrix_primary_merged_with_elements.xlsx",
    )
    parser.add_argument("--sheet-name", default="primary_merged_matrix")
    parser.add_argument(
        "--output-prefix",
        type=pathlib.Path,
        default=DEFAULT_DATA_DIR / "block_cluster_primary_archetypes",
    )
    parser.add_argument("--include-unmapped", action="store_true")
    parser.add_argument("--max-clusters-per-primary", type=int, default=40)
    parser.add_argument("--local-k-min", type=int, default=2)
    parser.add_argument("--local-k-divisor", type=int, default=10)
    parser.add_argument("--min-bucket-size", type=int, default=8)
    parser.add_argument("--random-state", type=int, default=42)
    parser.add_argument("--zero-eps", type=float, default=1e-9)
    args = parser.parse_args()

    result = cluster_primary_merged(
        input_file=args.input_file,
        output_prefix=args.output_prefix,
        sheet_name=args.sheet_name,
        include_unmapped=args.include_unmapped,
        max_clusters_per_primary=args.max_clusters_per_primary,
        local_k_min=args.local_k_min,
        local_k_divisor=args.local_k_divisor,
        min_bucket_size=args.min_bucket_size,
        random_state=args.random_state,
        zero_eps=args.zero_eps,
    )
    print(json.dumps(result, ensure_ascii=False, indent=2))
