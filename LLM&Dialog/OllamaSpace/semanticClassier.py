from transformers import BertTokenizer, BertModel
import torch
import numpy as np
from sklearn.cluster import KMeans
from collections import defaultdict
from sklearn.metrics import pairwise_distances_argmin_min

# 1. 初始化 BERT 模型和分词器
tokenizer = BertTokenizer.from_pretrained('bert-base-uncased')
model = BertModel.from_pretrained('bert-base-uncased')

# 2. 提取字符串的语义向量
def get_sentence_embedding(sentence):
    inputs = tokenizer(sentence, return_tensors='pt', truncation=True, padding=True)
    outputs = model(**inputs)
    # 提取每个句子的均值表示
    return outputs.last_hidden_state.mean(dim=1).detach().numpy()

# 3. 聚类函数，进行KMeans聚类
def cluster_sentences(sentences, num_clusters=5):
    embeddings = np.array([get_sentence_embedding(sentence)[0] for sentence in sentences])
    
    # 使用KMeans进行聚类
    kmeans = KMeans(n_clusters=num_clusters)
    kmeans.fit(embeddings)
    
    # 返回聚类标签和聚类中心
    return kmeans.labels_, kmeans.cluster_centers_

# 4. 将字符串按聚类结果分类
def classify_sentences(sentences, labels):
    clusters = defaultdict(list)
    for idx, label in enumerate(labels):
        clusters[label].append(sentences[idx])
    return clusters

# 5. 多类别归类，根据距离将句子分配到多个类别
def multi_classify_sentences(sentences, cluster_centers, threshold=0.1):
    embeddings = np.array([get_sentence_embedding(sentence)[0] for sentence in sentences])
    classification = defaultdict(list)

    for idx, embedding in enumerate(embeddings):
        # 计算每个字符串与所有聚类中心的距离
        distances = np.linalg.norm(cluster_centers - embedding, axis=1)
        
        # 如果与多个中心距离都接近阈值，则归类到多个类别
        for cluster_idx, distance in enumerate(distances):
            if distance < threshold:  # 可以根据实际情况调整阈值
                classification[cluster_idx].append(sentences[idx])
    
    return classification

# 6. 主程序，执行分类和多分类
def main():
    # 示例字符串数组
    sentences = ['红色stone石头002', '红色stone石头004', '红色sand沙子001', '红色sand沙子002', '红色soil土壤004', '凝胶ningjiaoo凝胶001', '硅基晶石stone石头011', '沼泽/沥青stone石头007', '沼泽/沥青stone石头008', '苔原moss苔原001', '苔原moss苔原002', '砾石stone石头005', '砾石stone石头006', '千岛sand沙子001', '红色石头001', '红色stone石头003', '红色stone石头005', '红色stone石头006', '红色stone石头007', '红色soil土壤002', '凝胶soil土壤001', '凝胶soil土壤002', '凝胶soil土壤003', '凝胶soil土壤004', '沥青stone石头006', '沥青soil土壤001', '苔原stone石头001', '苔原stone石头002', '苔原stone石头003', '砾石stone石头004', '砾石stone石头002', '千岛soil土壤004', '千岛soil土壤003', '千岛soil土壤005', '千岛soil土壤006', '千岛soil沙子002', '凝胶002']
    #  [
    #     "The cat sat on the mat.",
    #     "A dog was barking loudly.",
    #     "The sun is shining brightly today.",
    #     "She is reading a book.",
    #     "I love programming with Python.",
    #     "The weather is quite nice.",
    #     "He is playing the guitar.",
    #     "The sky is blue and clear."
    # ]
    
    # 设定聚类数目
    num_clusters = 8
    
    # 进行聚类
    labels, cluster_centers = cluster_sentences(sentences, num_clusters=num_clusters)
    
    # 分类到单一类别
    single_classification = classify_sentences(sentences, labels)
    
    print("单一类别分类结果：")
    for label, strings in single_classification.items():
        print(f"Cluster {label}: {strings}")
    
    # 多类别归类
    threshold = 0.5  # 距离阈值
    multi_classification = multi_classify_sentences(sentences, cluster_centers, threshold=threshold)
    
    print("\n多类别分类结果：")
    for label, strings in multi_classification.items():
        print(f"Cluster {label}: {strings}")

# 运行主程序
if __name__ == "__main__":
    main()
