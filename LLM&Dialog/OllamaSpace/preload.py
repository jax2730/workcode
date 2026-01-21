import json
import os
import argparse

# 读取 JSON 文件的函数
def load_json_from_file(file_path):
    if(not os.path.isabs(file_path)):
        # tmp = os.path.join(os.getcwd(), file_path)
        file_path = os.path.abspath(file_path)
    try:
        with open(file_path, 'r', encoding='utf-8') as json_file:
            data = json.load(json_file)
            return data
    except FileNotFoundError:
        print(f"File {file_path} not found.")
    except json.JSONDecodeError:
        print("Error decoding JSON file.")
    return None

# 设置命令行参数的函数
def parse_arguments():
    parser = argparse.ArgumentParser(description="Load a JSON file from a specified path.")
    parser.add_argument('file_path', type=str, help='The path to the JSON file')
    return parser.parse_args()
    # return parser.parse_args()


# 主函数
if __name__ == "__main__":
  
    # 从命令行获取参数
    args = parse_arguments()
    
    # 加载 JSON 文件
    json_data = load_json_from_file(args.file_path)
    
    if json_data:
        print("JSON data loaded successfully.")
        print(json_data)