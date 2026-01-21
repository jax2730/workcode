import argparse

from src.prepare import preload
from src import extract
from utils.util import cd

def parse_arguments():
    parser = argparse.ArgumentParser(description="Load a JSON file from a specified path.")
    parser.add_argument('file_path', type=str, help='The path to the JSON file')
    parser.add_argument('history_file_dir', type=str, help='The dir of history file')
    return parser.parse_args()


def launch():
    inputData = extract.launch()
    colorAssets = preload.filterByColor(inputData["colors"])
    ret = preload.createSubBiomeMap(inputData["temperature"]["min"],
                              inputData["temperature"]["max"],
                              inputData["humidity"]["min"],
                              inputData["humidity"]["max"],
                              colorAssets)
    print(ret)
    print(colorAssets)


# 主函数
if __name__ == "__main__":
    with cd("./SceneGenerator"):
        # 从命令行获取参数
        args = parse_arguments()
        
        # 加载 biome 文件
        preload.parseBiomeLibrary(args.file_path)
        # 收集历史数据
        preload.parseHistoryFile(args.history_file_dir)
        
        launch()