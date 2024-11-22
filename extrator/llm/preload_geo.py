'''
"stamp_terrain_instances":	[{
			"width":	0.542000,
			"height":	0.511000,
			"verticalScale":	3.209000,
			"transition":	0.476000,
			"axis.x":	-0.593488,
			"axis.y":	0.735420,
			"axis.z":	-0.327000,
			"angle":	103.957001,
			"verticalTrans":	-4.332000,
			"stamp_terrain_heightmap":	"biome_terrain/StampTerrain/Height Map_haidao.raw"
		}, {
			"width":	2,
			"height":	2,
			"verticalScale":	1,
			"transition":	0.200000,
			"axis.x":	0,
			"axis.y":	1,
			"axis.z":	0,
			"angle":	0,
			"verticalTrans":	0,
			"stamp_terrain_heightmap":	"biome_terrain/StampTerrain/Height Map_shan.raw"
		}],

'''

import os
from langchain_ollama import OllamaEmbeddings
from langchain_community.vectorstores import FAISS
from langchain.docstore.document import Document
import json
import math
import random
from . import utils as util

# 加载模板库
def load_stample_terrain_instance(dir):
    # stamp_terrain_instances:模板文件
    raw_files = [file for file in os.listdir(dir) if file.endswith('.raw')]
    print(raw_files)
    return raw_files

# 构建知识向量库
def get_stamp_docs():
    stamp_data_docs = []
    stampData = util.load_json_from_file("extrator/llm/data/stamp_terrain.json")
    for record in stampData: 
        heightmap = record["heightmap"]
        name = record["name"]
        # 构建文档内容
        content = (
            f"Heightmap filename: {heightmap}, "
            f"Terrain Type: {name}"
        )
        stamp_data_docs.append(Document(
            page_content=content,
            metadata={
                # todo 增加对于各个地形类型的描述信息作为检索的额外信息
                "heightmap": heightmap,
                "name": name
            }
        ))
    return stamp_data_docs

terrainstore = []
def init():
    print("----------初始化----------")

    # 建立知识索引库 
    global terrainstore
    embeddings = OllamaEmbeddings(model='nomic-embed-text')
    terrainstore = FAISS.from_documents(get_stamp_docs(), embeddings)
    print("terrainstore", terrainstore)
    # 保存索引
    terrainstore.save_local("stamp_knowledge_index")

def createTerrainStamp(TerrainAssets):
    terrain_stamps = []
    for terrain in TerrainAssets:
        docs = terrainstore.similarity_search(terrain, k=2) 
        for doc in docs:
            heightmap = doc.metadata.get('heightmap')
            if heightmap:  # 确保 heightmap 存在
                terrain_stamps.append(heightmap)
    print("terrain_stamps", terrain_stamps)
    return terrain_stamps