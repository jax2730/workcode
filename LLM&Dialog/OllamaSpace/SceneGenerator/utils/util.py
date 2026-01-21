import json
import os
import argparse

# 读取 JSON 文件的函数
def load_json_from_file(file_path):
    
    try:
        with open(file_path, 'r', encoding='utf-8') as json_file:
            data = json.load(json_file)
            return data
    except FileNotFoundError:
        print(f"File {file_path} not found.")
    except json.JSONDecodeError:
        print("Error decoding JSON file.")
    return None

def batch_load_json_from_files(dir_path, files):
    return { f : load_json_from_file(os.path.join(dir_path, f)) for f in files}

# 转为绝对路径
def getAbsPath(file_path):
    if(not os.path.isabs(file_path)):
        file_path = os.path.abspath(file_path)
    return file_path


def getParentPath(file_path):
    return os.path.dirname(file_path)

def getBaseName(path):
    return os.path.basename(path)

def filePathReplace(file_path, newFile):
    return os.path.join(getParentPath(file_path), newFile)


def recursiveLoadDir(dir, callback):
    if not os.path.isdir(dir):
        return
    dirList = os.walk(dir)
    for root, dirs, files in dirList:
        if files:
            callback(root, files)
        

class cd:
    def __init__(self, newPath):
        self.newPath = os.path.expanduser(newPath)

    def __enter__(self):
        self.savedPath = os.getcwd()
        os.chdir(self.newPath)

    def __exit__(self, etype, value, traceback):
        os.chdir(self.savedPath)