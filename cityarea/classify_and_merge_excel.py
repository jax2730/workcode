import pandas as pd
import re
from openpyxl import Workbook
from openpyxl.styles import Alignment, Font, PatternFill
from openpyxl.utils import get_column_letter

# Load the data
df = pd.read_csv('data/city/Beijing/best_csv/cluster_semantic_group_summary_v1 - 副本.csv', header=None)
signatures = df[3].dropna().tolist()

def parse_sig(sig):
    parts = sig.split('|')
    res = {}
    for p in parts:
        p = p.strip()
        if ':' in p:
            k, v = p.split(':')
            v = float(v.replace('%', ''))
            res[k] = v
    return res

def classify(sig_dict):
    # Calculate sum of specific categories
    res_tags = ['residential', 'apartments', 'common', 'cemetery'] # Wait, cemetery is not res
    res_val = sig_dict.get('residential', 0) + sig_dict.get('apartments', 0) + sig_dict.get('common', 0)
    
    green_val = sig_dict.get('park', 0) + sig_dict.get('forest', 0) + sig_dict.get('golf_course', 0) + sig_dict.get('farmland', 0) + sig_dict.get('garden', 0) + sig_dict.get('greenfield', 0) + sig_dict.get('grass', 0) + sig_dict.get('nature_reserve', 0) + sig_dict.get('meadow', 0) + sig_dict.get('orchard', 0) + sig_dict.get('cemetery', 0)

    ind_val = sig_dict.get('industrial', 0) + sig_dict.get('brownfield', 0) + sig_dict.get('wastewater_plant', 0) + sig_dict.get('water_works', 0) + sig_dict.get('landfill', 0) + sig_dict.get('farmyard', 0)
    
    com_val = sig_dict.get('commercial', 0) + sig_dict.get('retail', 0) + sig_dict.get('hotel', 0) + sig_dict.get('studio', 0) + sig_dict.get('mall', 0) + sig_dict.get('marketplace', 0) + sig_dict.get('restaurant', 0)
    
    pub_val = sig_dict.get('university', 0) + sig_dict.get('school', 0) + sig_dict.get('hospital', 0) + sig_dict.get('college', 0) + sig_dict.get('research_institute', 0) + sig_dict.get('place_of_worship', 0) + sig_dict.get('attraction', 0) + sig_dict.get('education', 0) + sig_dict.get('government', 0) + sig_dict.get('religious', 0) + sig_dict.get('governmental', 0) + sig_dict.get('civic_admin', 0) + sig_dict.get('monitoring_station', 0)
    
    tran_val = sig_dict.get('railway', 0) + sig_dict.get('parking', 0) + sig_dict.get('fuel', 0) + sig_dict.get('bus_station', 0) + sig_dict.get('track', 0) + sig_dict.get('train_station', 0)

    mil_val = sig_dict.get('military', 0)
    
    sport_val = sig_dict.get('sports_centre', 0) + sig_dict.get('stadium', 0) + sig_dict.get('horse_riding', 0) + sig_dict.get('pitch', 0) + sig_dict.get('theme_park',0) + sig_dict.get('zoo', 0) + sig_dict.get('fitness_centre', 0)
    
    # Priority classification based on dominance and mixture
    
    if res_val >= 20: 
        # User specified: "res > 20% might be ResidentialMix"
        macro = "居住区"
        if green_val > 15: sub = "综合配套绿色居住型"
        elif pub_val > 15: sub = "学区/医疗配套主导"
        elif com_val > 15: sub = "商业综合居住型"
        elif ind_val > 15: sub = "老旧/厂居混合社区"
        elif res_val >= 50: sub = "高纯度居住主导型"
        else: sub = "普通公房及平房区"
    elif green_val >= 35:
        macro = "绿地与生态区"
        if sig_dict.get('farmland', 0) > 30: sub = "高纯度农业保留地"
        elif sig_dict.get('golf_course', 0) > 30: sub = "高尔夫球生态区"
        elif sig_dict.get('park', 0) > 50: sub = "城市公园核心区"
        elif sig_dict.get('forest', 0) > 50: sub = "纯林地生态保护带"
        elif sig_dict.get('greenfield', 0) > 20 or sig_dict.get('brownfield',0) > 10: sub = "待开发绿地过渡带"
        else: sub = "复杂功能公园混合区"
    elif ind_val >= 30:
        macro = "工业与物流区"
        if sig_dict.get('brownfield', 0) > 40: sub = "高纯度储备待建地"
        elif sig_dict.get('wastewater_plant', 0) > 40 or sig_dict.get('water_works', 0) > 30 or sig_dict.get('landfill', 0) > 30: sub = "城市基础设施走廊"
        elif sig_dict.get('farmyard', 0) > 40: sub = "农业生产/养殖区"
        else: sub = "工业加工/物流园"
    elif com_val >= 25:
        macro = "商业服务区"
        if sig_dict.get('retail', 0) > 30: sub = "零售卖场主导"
        elif sig_dict.get('hotel', 0) > 50 or sig_dict.get('resort', 0) > 30: sub = "酒店与度假服务区"
        else: sub = "纯商业/商务主导带"
    elif pub_val >= 30:
        macro = "公共机构与服务区"
        if sig_dict.get('university', 0) > 25 or sig_dict.get('college', 0) > 25: sub = "高等教育及科研园区"
        elif sig_dict.get('school', 0) > 30: sub = "中小教育集聚区"
        elif sig_dict.get('hospital', 0) > 30: sub = "综合医疗服务中心"
        elif sig_dict.get('government', 0) > 30 or sig_dict.get('governmental', 0) > 30: sub = "政府及行政办公区"
        elif sig_dict.get('place_of_worship', 0) > 30 or sig_dict.get('religious', 0) > 30: sub = "宗教与文化场所"
        else: sub = "文教卫体综合配套"
    elif tran_val >= 30:
        macro = "交通枢纽设施"
        if sig_dict.get('parking', 0) > 40: sub = "大型地表停车场"
        elif sig_dict.get('railway', 0) > 30 or sig_dict.get('train_station', 0) > 20: sub = "铁路/火车站带"
        elif sig_dict.get('bus_station', 0) > 30: sub = "公交客运枢纽"
        else: sub = "道路及交通附属用地"
    elif mil_val >= 30:
        macro = "军管/保密区"
        sub = "军事防务保留地"
    elif sport_val >= 20:
        macro = "体育休闲区"
        if sig_dict.get('horse_riding', 0) > 40: sub = "马术及特色运动带"
        elif sig_dict.get('stadium', 0) > 25: sub = "大型体育场馆及周边"
        elif sig_dict.get('zoo', 0) > 40: sub = "动物园"
        elif sig_dict.get('theme_park', 0) > 40: sub = "主题乐园区"
        else: sub = "各类场馆/休闲设施"
    else:
        macro = "复合建成区"
        if res_val > 5: sub = "含少量居住的未明地块"
        else: sub = "功能极其混合/有待分类"
        
    return macro, sub

output_data = []
for sig in signatures:
    sd = parse_sig(sig)
    macro, sub = classify(sd)
    output_data.append((macro, sub, sig))

# Sort to cluster similar macros and subs
output_data.sort(key=lambda x: (x[0], x[1]))

wb = Workbook()
ws = wb.active

# Write Headers
ws.cell(row=1, column=1, value="大类归属 (Macro Class)")
ws.cell(row=1, column=2, value="微观类别 (Micro Class)")
ws.cell(row=1, column=3, value="组成成分 (Components)")

for i, row in enumerate(output_data):
    ws.cell(row=i+2, column=1, value=row[0])
    ws.cell(row=i+2, column=2, value=row[1])
    ws.cell(row=i+2, column=3, value=row[2])

def merge_consecutive(col_idx):
    prev_val = ws.cell(row=2, column=col_idx).value
    start_row = 2
    for row in range(3, len(output_data) + 3):
        if row <= len(output_data) + 1:
            current_val = ws.cell(row=row, column=col_idx).value
        else:
            current_val = None
            
        if current_val != prev_val:
            if prev_val is not None and (row - start_row) >= 1:
                ws.merge_cells(start_row=start_row, start_column=col_idx, end_row=row-1, end_column=col_idx)
            prev_val = current_val
            start_row = row

# merge macro category
merge_consecutive(1)
# merge micro category
merge_consecutive(2)

# Styling
for col in [1, 2]:
    for cell in ws[get_column_letter(col)]:
        cell.alignment = Alignment(horizontal='center', vertical='center')

# Header bold
for col in [1, 2, 3]:
    ws.cell(row=1, column=col).font = Font(bold=True)
    ws.cell(row=1, column=col).alignment = Alignment(horizontal='center', vertical='center')

# Adjust width
ws.column_dimensions['A'].width = 25
ws.column_dimensions['B'].width = 30
ws.column_dimensions['C'].width = 150

wb.save('data/city/Beijing/best_csv/reclassified_hierarchy.xlsx')
print("Successfully generated: e:/C++pandan/workcode/cityarea/data/city/Beijing/best_csv/reclassified_hierarchy.xlsx")
