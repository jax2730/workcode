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

def classify_simplified(sig_dict):
    res_val = sig_dict.get('residential', 0) + sig_dict.get('apartments', 0) + sig_dict.get('common', 0)
    green_val = sig_dict.get('park', 0) + sig_dict.get('forest', 0) + sig_dict.get('farmland', 0) + sig_dict.get('garden', 0) + sig_dict.get('greenfield', 0) + sig_dict.get('grass', 0) + sig_dict.get('nature_reserve', 0) + sig_dict.get('meadow', 0) + sig_dict.get('orchard', 0) + sig_dict.get('cemetery', 0)
    
    # Eco-recreation (Golf, Theme Park, Zoo)
    eco_rec_val = sig_dict.get('golf_course', 0) + sig_dict.get('theme_park',0) + sig_dict.get('zoo', 0) + sig_dict.get('resort', 0) + sig_dict.get('attraction', 0)
    green_val += eco_rec_val

    ind_val = sig_dict.get('industrial', 0) + sig_dict.get('wastewater_plant', 0) + sig_dict.get('water_works', 0) + sig_dict.get('landfill', 0) + sig_dict.get('farmyard', 0)
    brown_val = sig_dict.get('brownfield', 0)
    
    com_val = sig_dict.get('commercial', 0) + sig_dict.get('retail', 0) + sig_dict.get('hotel', 0) + sig_dict.get('studio', 0) + sig_dict.get('mall', 0) + sig_dict.get('marketplace', 0) + sig_dict.get('restaurant', 0) + sig_dict.get('roof', 0)
    
    pub_val = sig_dict.get('hospital', 0) + sig_dict.get('place_of_worship', 0)  + sig_dict.get('government', 0) + sig_dict.get('religious', 0) + sig_dict.get('governmental', 0) + sig_dict.get('civic_admin', 0) + sig_dict.get('monitoring_station', 0)
    edu_val = sig_dict.get('university', 0) + sig_dict.get('school', 0) + sig_dict.get('college', 0) + sig_dict.get('research_institute', 0) + sig_dict.get('education', 0)

    tran_val = sig_dict.get('railway', 0) + sig_dict.get('parking', 0) + sig_dict.get('fuel', 0) + sig_dict.get('bus_station', 0) + sig_dict.get('track', 0) + sig_dict.get('train_station', 0) + sig_dict.get('car', 0)
    
    special_val = sig_dict.get('sports_centre', 0) + sig_dict.get('stadium', 0) + sig_dict.get('horse_riding', 0) + sig_dict.get('pitch', 0) + sig_dict.get('fitness_centre', 0) + sig_dict.get('military', 0)
    
    # Total combined categories
    industry_commerce_total = ind_val + brown_val + com_val
    public_transport_total = pub_val + edu_val + tran_val + special_val
    
    # 1. 居住主导区
    if res_val >= 25:
        macro = "居住与生活区"
        if green_val > 15: sub = "绿地生态居住型"
        elif com_val > 15 or pub_val > 10 or edu_val > 15: sub = "综合配套居住型"
        elif ind_val > 10 or brown_val > 10: sub = "职住混合/老旧社区"
        else: sub = "高纯度居住型"
        
    # 2. 绿地与生态保留区
    elif green_val >= 35:
        macro = "绿地与生态区"
        if eco_rec_val > 20: sub = "休闲游憩绿地(高尔夫/主题乐园等)"
        elif sig_dict.get('farmland', 0) + sig_dict.get('farmyard', 0) > 30: sub = "农业与乡镇空间"
        elif sig_dict.get('forest', 0) > 40 or sig_dict.get('nature_reserve', 0) > 20: sub = "森林与自然保护区"
        else: sub = "城市公园与混合绿地"
        
    # 3. 商业与产业区
    elif industry_commerce_total >= 30:
        macro = "商业与产业区"
        if brown_val > 30: sub = "储备与待建地(Brownfield)"
        elif com_val > 25: sub = "商业与零售服务区"
        else: sub = "工业/物流与研发园区"
        
    # 4. 公共服务与交通区
    elif public_transport_total >= 30:
        macro = "公共服务与交通设施"
        if tran_val > 30: sub = "交通枢纽与场站"
        elif edu_val > 25: sub = "教育科研集聚区"
        elif special_val > 25: sub = "体育/休闲/特殊设施"
        else: sub = "综合医疗与行政服务"
        
    # 5. 混合建成区
    else:
        macro = "高度混合建成区"
        sub = "多功能混合业态地块"
        
    return macro, sub

output_data = []
for sig in signatures:
    sd = parse_sig(sig)
    macro, sub = classify_simplified(sd)
    output_data.append((macro, sub, sig))

# Define sorting order for Macros and Subs to ensure they group together logically
macro_order = {"居住与生活区": 1, "商业与产业区": 2, "公共服务与交通设施": 3, "绿地与生态区": 4, "高度混合建成区": 5}
output_data.sort(key=lambda x: (macro_order.get(x[0], 99), x[1]))

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

# merge macro and micro categories
merge_consecutive(1)
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
ws.column_dimensions['B'].width = 35
ws.column_dimensions['C'].width = 150

output_path = 'data/city/Beijing/best_csv/reclassified_hierarchy_simplified.xlsx'
wb.save(output_path)
print(f"Successfully generated: e:/C++pandan/workcode/cityarea/{output_path}")
