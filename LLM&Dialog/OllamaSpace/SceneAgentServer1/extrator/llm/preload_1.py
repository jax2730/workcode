from langchain_ollama import OllamaEmbeddings
from langchain_chroma import Chroma
from langchain_core.example_selectors import SemanticSimilarityExampleSelector
import json
import math
import random
from . import utils as util

def embedding(dataMap, similarityCount = 1):
    toStrMap = []
    for key in dataMap.keys():
        toStrMap.append({ "input": key, "output":json.dumps(dataMap[key])})
    to_vectorize = dataMap.keys()
    embeddings = OllamaEmbeddings(model="llama3.1")
    vectorstore = Chroma.from_texts(to_vectorize, embeddings, metadatas=toStrMap)

    data_selecter = SemanticSimilarityExampleSelector(
        vectorstore=vectorstore,
        k = similarityCount
    )
    return data_selecter



colorSelecter = None
materialSelecter = None
originBiomeMap = {}

def parseBiomeLibrary(configPath):
    configPath = util.getAbsPath(configPath)
    # originBiomeData = util.load_json_from_file(configPath)
    # testDataPath = util.filePathReplace(configPath, "testData")
    testData = util.load_json_from_file(configPath)

    global originBiomeMap   
    for idx, ele in enumerate(testData):
        originBiomeMap[ele["name"]] = idx
    # print("originBiomeMap", originBiomeMap)
    
    colorMap = {}
    materialMap = {}
    for ele in testData:
        color = ele["abstract"]["color"]
        name = ele["name"]
        if color in colorMap:
            colorMap[color].append(name)
        else:
            colorMap[color] = [name]
        
        material = ele["abstract"]["material"]
        if material in materialMap:
            materialMap[material].append(name)
        else:
            materialMap[material] = [name]



    global colorSelecter
    colorSelecter = embedding(colorMap)


    global materialSelecter
    materialSelecter = embedding(materialMap)
 


def filterByColor(colors):
    result = []
    for color in colors:
        refsArr = colorSelecter.select_examples({"input":color})
        if len(refsArr) != 0:
            result.extend(json.loads(refsArr[0]["output"]))
    return result

def filterByMaterail(mats):
    result = []
    for mat in mats:
        refsArr = materialSelecter.select_examples({"input":mat})
        if len(refsArr) != 0:
            result.extend(json.loads(refsArr[0]["output"]))
    return result


def intersectionOfMat(inputs, filters):
    mats = filterByMaterail(filters)
    ret = []
    for ele in inputs:
        if mats.count(ele) > 0:
            ret.append(ele)
    return ret

def intersectionOfColor(inputs, filters):
    mats = filterByColor(filters)
    ret = []
    for ele in inputs:
        if mats.count(ele) > 0:
            ret.append(ele)
    return ret


class BiomeSubRange:
    def __init__(self, bId, row, col, width=1, height=1):
        self.biomeId = bId
        self.row = row
        self.col = col
        self.width = width
        self.height = height
    
    def __str__(self):
        return """biomeidx:{}, (x, y): {}, {}""".format(self.biomeId, self.col, self.row)

class BiomeMap:
    
    def __init__(self, minT=0, maxT=0, minH=0, maxH=0):
        self.minHumidity = minH
        self.maxHumidity = maxH
        self.minTemperature = minT
        self.maxTemperature = maxT
        self.step = 5
        #                                        temperature
        #               [[set(), set(), set(), set(), set(), set(), set(), set(), set(), set()],
        #                [set(), set(), set(), set(), set(), set(), set(), set(), set(), set()],
        #                [set(), set(), set(), set(), set(), set(), set(), set(), set(), set()],
        #                [set(), set(), set(), set(), set(), set(), set(), set(), set(), set()],
        # humidity       [set(), set(), set(), set(), set(), set(), set(), set(), set(), set()],
        #                [set(), set(), set(), set(), set(), set(), set(), set(), set(), set()],
        #                [set(), set(), set(), set(), set(), set(), set(), set(), set(), set()],
        #                [set(), set(), set(), set(), set(), set(), set(), set(), set(), set()],
        #                [set(), set(), set(), set(), set(), set(), set(), set(), set(), set()]]
        self.biomeIdxMap = [[ set() for _ in range(minT, maxT, self.step)] for _ in range(minH, maxH, self.step)]
        self.rangeList = []
                
    def insert(self, minT, maxT, minH, maxH, idx):
        if self.minTemperature <= minT <= maxT <= self.maxTemperature and self.minHumidity <= minH <= maxH <= self.maxHumidity:
            # humidity
            for i, h in enumerate(range(self.minHumidity, maxH, self.step)):
                if h < minH:
                    continue
                # temperature
                for j, t in enumerate(range(self.minTemperature, maxT, self.step)):
                    if t < minT:
                        continue
                    # self.biomeIdxMap[i][j].discard(-1)
                    self.biomeIdxMap[i][j].add(idx)

                                
        else:
            raise ValueError("range error")

    def retriveByRange(self, minT, maxT, minH, maxH):
        result = set()
        if self.minTemperature <= minT <= maxT <= self.maxTemperature and self.minHumidity <= minH <= maxH <= self.maxHumidity:
            col_begin = math.floor((minT - self.minTemperature) / self.step)
            col_end = math.floor((maxT - self.minTemperature) / self.step)
            row_begin = math.floor((minH - self.minHumidity) / self.step)
            row_end = math.floor((maxH - self.minHumidity) / self.step)
            for r in range(row_begin, row_end):
                for c in range(col_begin, col_end):
                    result.update(self.biomeIdxMap[r][c])
        return result
        
    def retrive(self, rowIdx, colIdx):
        if rowIdx > len(self.biomeIdxMap) or colIdx > len(self.biomeIdxMap[0]):
            return set()
        else:
            return self.biomeIdxMap[rowIdx][colIdx]

    def update(self, collection, minT, maxT, minH, maxH):
        if self.minTemperature <= minT <= maxT <= self.maxTemperature and self.minHumidity <= minH <= maxH <= self.maxHumidity:
            col_begin = math.floor((minT - self.minTemperature) / self.step)
            col_end = math.floor((maxT - self.minTemperature) / self.step)
            row_begin = math.floor((minH - self.minHumidity) / self.step)
            row_end = math.floor((maxH - self.minHumidity) / self.step)
            for r in range(row_begin, row_end):
                for c in range(col_begin, col_end):
                    self.biomeIdxMap[r][c].update(collection)


    def subMap(self, minT, maxT, minH, maxH):
        subCollection = BiomeMap(minT,maxT, minH, maxH)
        leftBound = max(minT, self.minTemperature)
        rightBound = min(maxT, self.maxTemperature)
        topBound = max(minH, self.minHumidity)
        bottomBound = min(maxH, self.maxHumidity)
        if leftBound >= rightBound or topBound >= bottomBound:
            return subCollection
        for cv in range(leftBound, rightBound, self.step):
            for rv in range(topBound, bottomBound, self.step):
                subCollection.update(self.retriveByRange(cv, cv + self.step, rv, rv + self.step), cv, cv + self.step, rv, rv + self.step)
        return subCollection

    def intersectionUpdate(self, abledResources):
        for row in self.biomeIdxMap:
            for ele in row:
                ele.intersection_update(abledResources)
                # if len(ele) == 0:
                #     ele.update(random.sample(collection, min(3, len(abledResources))))

    def subdivision(self, abledResources, limit = 16):
        # 可用资源集合
        abledResourceSet = set(abledResources)

        grid = self.biomeIdxMap
        # 检查两个格子是否共享至少一个相同的整数
        has_common_element = lambda x, y: bool(x & y)

        n = len(grid)
        m = len(grid[0])
        
        visited = [[False for _ in range(m)] for _ in range(n)]
        rangeMap = self.rangeMap = [[ set() for _ in range(m)] for _ in range(n)]
        rangeIdSet = self.rangeIdSet = set()

        # 尝试扩展当前的格子，找到最大矩形区域
        def find_max_rectangle(i, j):
            # 当前格子的共享数字集合
            current_set = set(grid[i][j])
            is_current_empty = len(current_set) == 0
            # 初始矩形的宽度和高度
            width = 1
            height = 1
            
            # 计算最大宽度
            for col in range(j + 1, m):
                if is_current_empty:
                    if not visited[i][col] and len(grid[i][col]) == 0:
                        width += 1
                    else:
                        break
                else:
                    if not visited[i][col] and has_common_element(current_set, grid[i][col]):
                        width += 1
                        current_set.intersection_update(grid[i][col])
                    else:
                        break
            
            # 计算最大高度
            tmp_shared_set = set()
            for row in range(i + 1, n):
                # 检查这一行是否与之前的宽度一致
                valid_row = True
                tmp_shared_set.update(current_set)
                for col in range(j, j + width):
                    if is_current_empty:
                        if visited[row][col] or len(grid[row][col]) > 0:
                            valid_row = False
                            break
                    else:
                        if visited[row][col] or not has_common_element(current_set, grid[row][col]):
                            valid_row = False
                            break
                        tmp_shared_set.intersection_update(grid[row][col])
                if valid_row:
                    height += 1
                    # 更新整行的共享集合
                    tmp_holder = current_set
                    current_set = tmp_shared_set
                    tmp_shared_set = tmp_holder
                    tmp_shared_set.clear()
                else:
                    break
            
            # 更新这个矩形区域内所有格子的值
            for row in range(i, i + height):
                for col in range(j, j + width):
                    visited[row][col] = True
                    rangeMap[row][col].update(current_set)


            
            biomeId = 0
            if len(rangeIdSet) < limit:
                if len(current_set) == 0:
                    intsec = rangeIdSet.intersection(abledResourceSet)
                    diff = abledResourceSet.difference(intsec)
                    biomeId = random.choice(list(diff if len(diff) > 0 else rangeIdSet))
                    rangeIdSet.add(biomeId)
                else:
                    intsec = rangeIdSet.intersection(current_set)
                    diff = current_set.difference(intsec)
                    biomeId = random.choice(list(diff if len(diff) > 0 else current_set))
                    rangeIdSet.add(biomeId)
            # else:
            #     biomeId = random.choice(list(rangeIdSet))
            
            self.rangeList.append(BiomeSubRange(biomeId, i, j, width, height))
            
        
        
        for i in range(n):
            for j in range(m):
                if not visited[i][j]:
                    # 从未访问的格子开始搜索，寻找最大矩形
                    find_max_rectangle(i, j)

        if len(rangeIdSet) > limit:
            self.printBiomeRange()
            raise ValueError("length of biomeid over range")



    def getSubRange(self):
        ret = []
        for subrange in self.rangeList:
            item = {
                "idx": subrange.biomeId,
                "temperature":{
                    "min": subrange.col * self.step + self.minTemperature,
                    "max": (subrange.col + subrange.width) * self.step + self.minTemperature,
                },
                "humidity":{
                    "min": subrange.row * self.step + self.minHumidity,
                    "max": (subrange.row + subrange.height) * self.step + self.minHumidity,
                },
            }
            ret.append(item)
        return ret
    
    
    def getSubRange_noRep(self):
        ret = []
        seen_ids = set()
        have_idx0 = False
        for subrange in self.rangeList:
            if subrange.biomeId in seen_ids:
                have_idx0 = True
            else:
                idx = subrange.biomeId
                seen_ids.add(subrange.biomeId)
            item = {
                "idx": idx,
                "temperature": {
                    "min": subrange.col * self.step + self.minTemperature,
                    "max": (subrange.col + subrange.width) * self.step + self.minTemperature,
                },
                "humidity": {
                    "min": subrange.row * self.step + self.minHumidity,
                    "max": (subrange.row + subrange.height) * self.step + self.minHumidity,
                },
            }
            ret.append(item)
        return ret, have_idx0

    def __str__(self):
        result = []
        max_lengths = []
        allSet = set()
        result.append("""temperature[{},{}], humidity[{},{}]""".format(self.minTemperature, self.maxTemperature, self.minHumidity, self.maxHumidity))
        for col in range(len(self.biomeIdxMap[0])):
           max_length = max(len(str(sorted(self.biomeIdxMap[row][col]))) for row in range(len(self.biomeIdxMap)))
           max_lengths.append(max_length)

        for row in self.biomeIdxMap:
            row_str = []
            for col_idx, item in enumerate(row):
                item_str = str(sorted(item))
                allSet.update(item)
                row_str.append(item_str.ljust(max_lengths[col_idx]))
            result.append("  ".join(row_str))
        result.append(str(sorted(allSet)))
        result.append("totol lens: {}".format(len(allSet)))
        return "\n".join(result)
    
    def printBiomeRange(self):
        result = []
        max_lengths = []
        allSet = set()
        grid = self.rangeMap
        for col in range(len(grid[0])):
           max_length = max(len(str(sorted(grid[row][col]))) for row in range(len(grid)))
           max_lengths.append(max_length)

        for row in grid:
            row_str = []
            for col_idx, item in enumerate(row):
                item_str = str(sorted(item))
                allSet.update(item)
                row_str.append(item_str.ljust(max_lengths[col_idx]))
            result.append("  ".join(row_str))
        result.append(str(sorted(allSet)))
        result.append("totol lens: {}".format(len(allSet)))
        result.append("used biome res: " + str(sorted(self.rangeIdSet)))
        print("\n".join(result))
        for subrange in self.rangeList:
            print(subrange)

    def get_all_records(self):
        records = []
        
        # 遍历 biomeIdxMap，计算每个单元格的温湿度范围及生物群系索引
        for row_idx, row in enumerate(self.biomeIdxMap):
            for col_idx, item in enumerate(row):
                # 计算温度范围
                temp_min = self.minTemperature + row_idx * 5
                temp_max = temp_min + 5
                
                # 计算湿度范围
                hum_min = self.minHumidity + col_idx * 5
                hum_max = hum_min + 5
                
                # 构造记录
                record = {
                    "temperature_range": (temp_min, temp_max),
                    "humidity_range": (hum_min, hum_max),
                    "indices": sorted(item),  # 将生物群系索引排序
                }
                
                # 将记录添加到结果中
                records.append(record)
        
        return records



biomeData = None
def parseHistoryFile(fileDir):
    biomeDataCollection = []
    # temperatureMin, temperatureMax, humidityMin, humidityMax
    range = [math.inf, -math.inf, math.inf, -math.inf]

    def filter(basePath, files):
        basename = util.getBaseName(basePath)
        biomeName = basename + ".biome"
        biomecompoName = basename + ".biomecompo"
        biometerrainName = basename + ".terrain"
        targetFilesName = [biomeName, biomecompoName, biometerrainName]
        newset = set(targetFilesName) & set(files)
        if len(newset) < 3:
            return
        
        targetFile = util.batch_load_json_from_files(basePath, targetFilesName)

        biome = targetFile[biomeName]
        for idx, distrib in enumerate(biome["biome distributions"]):
            biomeDataCollection.append({"distrib":distrib, "idx":targetFile[biomecompoName]["compositions"][idx]["biome id"]})
            range[0] = min(distrib["temperature"]["min"], range[0])
            range[1] = max(distrib["temperature"]["max"], range[1])
            range[2] = min(distrib["humidity"]["min"], range[2])
            range[3] = max(distrib["humidity"]["max"], range[3])

    util.recursiveLoadDir(fileDir,filter)

    global biomeData

    biomeData = BiomeMap(*range)
    for ele in biomeDataCollection:
        biomeData.insert(ele["distrib"]["temperature"]["min"],
                         ele["distrib"]["temperature"]["max"],
                         ele["distrib"]["humidity"]["min"],
                         ele["distrib"]["humidity"]["max"],
                         ele["idx"])
    # print("biomeData-------------------", biomeData)



def createSubBiomeMap(minT, maxT, minH, maxH, intersectionBy=[]):
    def clampValue(a):
        tmp = math.floor((abs(a)+4)/5)*5
        return tmp if a > 0 else -tmp
    minT = clampValue(minT)
    maxT = clampValue(maxT)
    minH = clampValue(minH)
    maxH = clampValue(maxH)
    subMap = biomeData.subMap(minT, maxT, minH, maxH)
    print("-------------subMap1--------------", subMap)
    resIndies = set()
    for res in intersectionBy:
        if res in originBiomeMap:
            resIndies.add(originBiomeMap[res])
        else:
            raise ValueError("error biome res name: " + res)
    print('resIndies',resIndies)
    subMap.intersectionUpdate(resIndies)
    # subdivision(subMap.biomeIdxMap)
    # print(subMap)
    subMap.subdivision(resIndies)

    subRange, repeated_idx = subMap.getSubRange_noRep()
    subMap.printBiomeRange()

    ret = [{
        "hRange": {
            "min": subMap.minHumidity,
            "max": subMap.maxHumidity,
        },
        "tRange": {
            "min": subMap.minTemperature,
            "max": subMap.maxTemperature,
        },
        "subRanges": subRange,
    }, repeated_idx]

    return ret