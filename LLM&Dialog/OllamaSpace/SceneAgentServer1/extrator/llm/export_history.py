import sqlite3
import json
from openpyxl import Workbook

# 数据库文件路径
db_file = "memory.db"

# 1. 连接到数据库并读取内容
conn = sqlite3.connect(db_file)
cursor = conn.cursor()

# 读取表名
cursor.execute("SELECT name FROM sqlite_master WHERE type='table';")
tables = cursor.fetchall()

if not tables:
    print("数据库中没有表！")
    conn.close()
    exit()

print("找到以下表：")
for i, table in enumerate(tables):
    print(f"{i + 1}. {table[0]}")

table_index = int(input("请选择一个表导出数据（输入序号）：")) - 1
table_name = tables[table_index][0]

# 读取指定表的数据
cursor.execute(f"SELECT * FROM {table_name}")
rows = cursor.fetchall()
columns = [description[0] for description in cursor.description]

# 关闭数据库连接
conn.close()

# 2. 处理数据（将 JSON 字符串中的 Unicode 编码解析为中文，并处理嵌套字典）
def decode_and_flatten(value):
    if isinstance(value, str):
        try:
            # 尝试将字符串解析为 JSON
            value = json.loads(value)
        except (json.JSONDecodeError, TypeError):
            pass
    if isinstance(value, dict):
        # 展平嵌套字典为字符串
        return json.dumps(value, ensure_ascii=False)
    return value

decoded_rows = [[decode_and_flatten(cell) for cell in row] for row in rows]

# 3. 将数据写入 Excel 文件
excel_file = "history.xlsx"
wb = Workbook()
ws = wb.active
ws.title = table_name

# 写入表头
ws.append(columns)

# 写入数据
for row in decoded_rows:
    ws.append(row)

# 保存 Excel 文件
wb.save(excel_file)

print(f"表 '{table_name}' 的数据已成功导出到 {excel_file}")
