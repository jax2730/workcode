import pandas as pd
import re
from openpyxl import Workbook
from openpyxl.styles import Alignment, Font, PatternFill
from openpyxl.utils import get_column_letter

COMPONENT_ZH_MAP = {
    'residential': '居住用地',
    'apartments': '公寓住宅',
    'common': '公共生活配套',
    'park': '公园绿地',
    'forest': '森林',
    'farmland': '农田',
    'garden': '园地',
    'greenfield': '未开发绿地',
    'grass': '草地',
    'nature_reserve': '自然保护区',
    'meadow': '草甸',
    'orchard': '果园',
    'cemetery': '墓地',
    'golf_course': '高尔夫场地',
    'theme_park': '主题公园',
    'zoo': '动物园',
    'resort': '度假区',
    'attraction': '景点设施',
    'industrial': '工业用地',
    'wastewater_plant': '污水处理设施',
    'water_works': '供水设施',
    'landfill': '垃圾填埋场',
    'farmyard': '农业院落',
    'brownfield': '棕地/待开发地',
    'commercial': '商业用地',
    'retail': '零售业态',
    'hotel': '酒店住宿',
    'studio': '工作室',
    'mall': '购物中心',
    'marketplace': '集市',
    'restaurant': '餐饮设施',
    'roof': '屋顶构筑',
    'hospital': '医疗设施',
    'place_of_worship': '宗教场所',
    'government': '政府机构',
    'religious': '宗教用地',
    'governmental': '行政管理设施',
    'civic_admin': '市政管理设施',
    'monitoring_station': '监测站',
    'university': '大学校园',
    'school': '学校',
    'college': '学院',
    'research_institute': '科研机构',
    'education': '教育设施',
    'railway': '铁路设施',
    'parking': '停车设施',
    'fuel': '加油设施',
    'bus_station': '公交场站',
    'track': '轨道/场道',
    'train_station': '火车站',
    'car': '汽车相关设施',
    'sports_centre': '体育中心',
    'stadium': '体育场',
    'horse_riding': '马术场地',
    'pitch': '球场',
    'fitness_centre': '健身中心',
    'military': '军事设施',
    'aquarium': '水族馆',
    'arts_centre': '艺术中心',
    'community_centre': '社区中心',
    'conference_centre': '会议中心',
    'driver_training': '驾驶培训设施',
    'fire_station': '消防站',
    'garages': '车库设施',
    'greenhouse': '温室',
    'greenhouse_horticulture': '园艺温室',
    'kindergarten': '幼儿园',
    'museum': '博物馆',
    'office': '办公用地',
    'plant_nursery': '苗圃',
    'prison': '监狱设施',
    'recreation_ground': '休闲活动场地',
    'sports_hall': '体育馆',
    'theatre': '剧院',
    'vineyard': '葡萄园',
    'warehouse': '仓储设施',
    'water_park': '水上乐园',
    'yes': '其他建设用地'
}

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

def translate_signature(sig):
    parts = [p.strip() for p in sig.split('|') if p.strip()]
    zh_parts = []
    for part in parts:
        if ':' in part:
            key, val = part.split(':', 1)
            key = key.strip()
            val = val.strip()
            zh_key = COMPONENT_ZH_MAP.get(key, key)
            zh_parts.append(f"{zh_key}:{val}")
        else:
            zh_parts.append(part)
    return ' | '.join(zh_parts)

def classify_simplified_with_reason(sig_dict):
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
    
    macro = ""
    sub = ""
    reason = ""

    # 1. 居住主导区
    if res_val >= 25:
        macro = "居住与生活区"
        if green_val > 15: 
            sub = "绿地生态居住型"
            reason = "居住属性(含residential/apartments)≥25%，且公园绿地等自然特征成分超过15%"
        elif com_val > 15 or pub_val > 10 or edu_val > 15: 
            sub = "综合配套居住型"
            reason = "居住属性≥25%，且商业、公共服务或教育科研设施占比明显(10~15%起)"
        elif ind_val > 10 or brown_val > 10: 
            sub = "职住混合/老旧社区"
            reason = "居住属性≥25%，但内部混有工业设施或未开发储备地(brownfield)成分(>10%)"
        else: 
            sub = "高纯度居住型"
            reason = "居住属性≥25%(很多达60%以上)，且其他设施成分极少，居住功能占据绝对主导"
        
    # 2. 绿地与生态保留区
    elif green_val >= 35:
        macro = "绿地与生态区"
        if eco_rec_val > 20: 
            sub = "休闲游憩绿地\n(高尔夫/主题乐园等)"
            reason = "绿地及生态总量≥35%，且其中高尔夫、度假村/乐园等人工游憩设施占比>20%"
        elif sig_dict.get('farmland', 0) + sig_dict.get('farmyard', 0) > 30: 
            sub = "农业与乡镇空间"
            reason = "绿地及生态总量≥35%，且核心主要由农田(farmland)或农业建筑构成(>30%)"
        elif sig_dict.get('forest', 0) > 40 or sig_dict.get('nature_reserve', 0) > 20: 
            sub = "森林与自然保护区"
            reason = "绿地及生态总量≥35%，由大面积森林(>40%)或自然生态保护区构成"
        else: 
            sub = "城市公园与混合绿地"
            reason = "绿地及生态特征≥35%，以常规城市公园体系、草地为主的自然空间"
        
    # 3. 商业与产业区
    elif industry_commerce_total >= 30:
        macro = "商业与产业区"
        if brown_val > 30: 
            sub = "储备与待建地\n(Brownfield)"
            reason = "非居住/绿地体系，呈建设态势。空置或废弃的储备用地占比>30%"
        elif com_val > 25: 
            sub = "商业与零售服务区"
            reason = "非居住体系，内部商业中心、零售卖场、酒店等纯商业类服务特征>25%"
        else: 
            sub = "工业/物流与研发园区"
            reason = "工商混合业态≥30%，且主要以工厂(industrial)、市政设施、仓储等功能为主"
        
    # 4. 公共服务与交通区
    elif public_transport_total >= 30:
        macro = "公共服务与交通设施"
        if tran_val > 30: 
            sub = "交通枢纽与场站"
            reason = "总公用配套≥30%，其中火车站、大公交枢纽或结构性停车场占据主导地位(>30%)"
        elif edu_val > 25: 
            sub = "教育科研集聚区"
            reason = "总公用配套≥30%，功能重度倾向于中小学或大学集聚区(>25%)"
        elif special_val > 25: 
            sub = "体育/休闲/特殊设施"
            reason = "总公用配套≥30%，主要由大型体育场馆集群或涉军(military)管理设施构成(>25%)"
        else: 
            sub = "综合医疗与行政服务"
            reason = "总公用配套≥30%，医院、政府行政、宗教文化等综合性市民社会设施显著"
        
    # 5. 混合建成区
    else:
        macro = "高度混合建成区"
        sub = "多功能混合业态地块"
        reason = "不满足居住、绿地或产业的单一阈值条件，内部建筑设施成分破碎且极度混合"
        
    return macro, sub, reason


def build_rows(source_csv='data/city/Beijing/best_csv/cluster_semantic_group_summary_v1 - 副本.csv'):
    df = pd.read_csv(source_csv, header=None)
    signatures = df[3].dropna().tolist()

    output_data = []
    for sig in signatures:
        sd = parse_sig(sig)
        macro, sub, reason = classify_simplified_with_reason(sd)
        sig_zh = translate_signature(sig)
        output_data.append((macro, sub, reason, sig_zh))

    macro_order = {"居住与生活区": 1, "商业与产业区": 2, "公共服务与交通设施": 3, "绿地与生态区": 4, "高度混合建成区": 5}
    output_data.sort(key=lambda x: (macro_order.get(x[0], 99), x[1]))
    return output_data


def write_workbook(output_path='data/city/Beijing/best_csv/reclassified_hierarchy_simplified_with_reason.xlsx', source_csv='data/city/Beijing/best_csv/cluster_semantic_group_summary_v1 - 副本.csv'):
    output_data = build_rows(source_csv=source_csv)

    wb = Workbook()
    ws = wb.active

    ws.cell(row=1, column=1, value="大类归属 (Macro Class)")
    ws.cell(row=1, column=2, value="微观类别 (Micro Class)")
    ws.cell(row=1, column=3, value="分类依据 (Classification Criteria)")
    ws.cell(row=1, column=4, value="组成成分（中文，含占比）")

    for i, row in enumerate(output_data):
        ws.cell(row=i+2, column=1, value=row[0])
        ws.cell(row=i+2, column=2, value=row[1])
        ws.cell(row=i+2, column=3, value=row[2])
        ws.cell(row=i+2, column=4, value=row[3])

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

    merge_consecutive(1)
    merge_consecutive(2)
    merge_consecutive(3)

    for col in [1, 2, 3]:
        for cell in ws[get_column_letter(col)]:
            cell.alignment = Alignment(horizontal='center', vertical='center', wrap_text=True)

    for cell in ws[get_column_letter(4)]:
        cell.alignment = Alignment(horizontal='left', vertical='center', wrap_text=True)

    for col in [1, 2, 3, 4]:
        ws.cell(row=1, column=col).font = Font(bold=True)
        ws.cell(row=1, column=col).alignment = Alignment(horizontal='center', vertical='center')

    ws.column_dimensions['A'].width = 18
    ws.column_dimensions['B'].width = 25
    ws.column_dimensions['C'].width = 40
    ws.column_dimensions['D'].width = 110

    wb.save(output_path)
    print(f"Successfully generated: e:/C++pandan/workcode/cityarea/{output_path}")


if __name__ == '__main__':
    write_workbook()
