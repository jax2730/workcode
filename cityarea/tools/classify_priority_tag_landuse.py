from __future__ import annotations

import argparse
import datetime as dt
import functools
import json
import os
import pathlib
import re
from dataclasses import dataclass, field
from typing import Any

import pandas as pd
import requests
from sklearn.decomposition import TruncatedSVD
from sklearn.feature_extraction.text import TfidfVectorizer
from sklearn.preprocessing import normalize


REPO_ROOT = pathlib.Path(__file__).resolve().parents[1]
INPUT_CSV = REPO_ROOT / "data" / "tag_classify" / "multi_city_priority_tag_values_20260623_210813_unique_values.csv"
OUTPUT_DIR = REPO_ROOT / "data" / "tag_classify"
CATEGORY_TEXT_PATH = REPO_ROOT / "reports" / "P020231127391946850971.txt"
LLM_CACHE_PATH = OUTPUT_DIR / "priority_tag_landuse_llm_cache.json"
LEGACY_ALL_TAGS_XLSX_PATH = OUTPUT_DIR / "multi_city_priority_tag_values_20260623_210813_unique_values_landuse_mapping.xlsx"
BUILDING_REFERENCE_MAPPING_PATH = REPO_ROOT / "reports" / "landuse_tag_mapping" / "building_tag_landuse_mapping.csv"
CURATED_BASELINE_XLSX_PATH = OUTPUT_DIR / "multi_city_priority_tag_values_20260623_210813_unique_values_landuse_mapping.xlsx"


PLANNING_PRIMARY = {
    "00": "未映射/不建议直接判定",
    "01": "耕地",
    "02": "园地",
    "03": "林地",
    "04": "草地",
    "06": "农业设施建设用地",
    "07": "居住用地",
    "08": "公共管理与公共服务用地",
    "09": "商业服务业用地",
    "10": "工矿用地",
    "11": "仓储用地",
    "12": "交通运输用地",
    "13": "公用设施用地",
    "14": "绿地与开敞空间用地",
    "15": "特殊用地",
    "16": "留白/待开发用地",
}

LOW_CONFIDENCE_THRESHOLD = 0.75
LLM_TRIGGER_PRIMARY_MAX = 0.55
DEFAULT_LLM_TIMEOUT = 90.0
CURATED_BASELINE_CONFIDENCE_MIN = 0.9
CURATED_BASELINE_SEMANTIC_MIN = 0.55


PLANNING_SECONDARY = {
    "0000": "未映射/不建议直接判定",
    "0101": "水田",
    "0102": "水浇地",
    "0103": "旱地",
    "0201": "果园",
    "0202": "茶园",
    "0203": "橡胶园地",
    "0204": "油料园地",
    "0205": "其他园地",
    "0301": "乔木林地",
    "0302": "竹林地",
    "0303": "灌木林地",
    "0304": "其他林地",
    "0401": "天然牧草地",
    "0402": "人工牧草地",
    "0403": "其他草地",
    "0601": "农村道路",
    "0602": "设施农用地",
    "0701": "城镇住宅用地",
    "0702": "城镇社区服务设施用地",
    "0703": "农村宅基地",
    "0704": "农村社区服务设施用地",
    "0801": "机关团体用地",
    "0802": "科研用地",
    "0803": "文化用地",
    "0804": "教育用地",
    "0805": "体育用地",
    "0806": "医疗卫生用地",
    "0807": "社会福利用地",
    "0901": "商业用地",
    "0902": "商务金融用地",
    "0903": "娱乐用地",
    "0904": "其他商业服务业用地",
    "1001": "工业用地",
    "1002": "采矿用地",
    "1003": "盐田",
    "1101": "物流仓储用地",
    "1102": "储备库用地",
    "1201": "铁路用地",
    "1202": "公路用地",
    "1203": "机场用地",
    "1204": "港口码头用地",
    "1205": "管道运输用地",
    "1206": "城市轨道交通用地",
    "1207": "城镇村道路用地",
    "1208": "交通场站用地",
    "1209": "其他交通设施用地",
    "1301": "供水用地",
    "1302": "排水用地",
    "1303": "供电用地",
    "1304": "供燃气用地",
    "1305": "供热用地",
    "1306": "通信用地",
    "1307": "邮政用地",
    "1308": "广播电视设施用地",
    "1309": "环卫用地",
    "1310": "消防用地",
    "1311": "水工设施用地",
    "1312": "其他公用设施用地",
    "1401": "公园绿地",
    "1402": "防护绿地",
    "1403": "广场用地",
    "1501": "军事设施用地",
    "1502": "使领馆用地",
    "1503": "宗教用地",
    "1504": "文物古迹用地",
    "1505": "监教场所用地",
    "1506": "殡葬用地",
    "1507": "其他特殊用地",
    "1601": "留白/待开发用地",
}


PLANNING_TERTIARY = {
    "060101": "村道用地",
    "060102": "田间道",
    "060201": "种植设施建设用地",
    "060202": "畜禽养殖设施建设用地",
    "060203": "水产养殖设施建设用地",
    "070101": "一类城镇住宅用地",
    "070102": "二类城镇住宅用地",
    "070103": "三类城镇住宅用地",
    "070301": "一类农村宅基地",
    "070302": "二类农村宅基地",
    "080301": "图书与展览用地",
    "080302": "文化活动用地",
    "080401": "高等教育用地",
    "080402": "中等职业教育用地",
    "080403": "中小学用地",
    "080404": "幼儿园用地",
    "080405": "其他教育用地",
    "080501": "体育场馆用地",
    "080502": "体育训练用地",
    "080601": "医院用地",
    "080602": "基层医疗卫生设施用地",
    "080603": "公共卫生用地",
    "080701": "老年人社会福利用地",
    "080702": "儿童社会福利用地",
    "080703": "残疾人社会福利用地",
    "080704": "其他社会福利用地",
    "090101": "零售商业用地",
    "090102": "批发市场用地",
    "090103": "餐饮用地",
    "090104": "旅馆用地",
    "090105": "公用设施营业网点用地",
    "100101": "一类工业用地",
    "100102": "二类工业用地",
    "100103": "三类工业用地",
    "110101": "一类物流仓储用地",
    "110102": "二类物流仓储用地",
    "110103": "三类物流仓储用地",
    "120801": "对外交通场站用地",
    "120802": "公共交通场站用地",
    "120803": "社会停车场用地",
}


SOURCE_CATEGORY_GLOSSES = {
    "building": "building architecture built structure 建筑 建筑物",
    "landuse": "land use parcel function 用地 地类 地块",
    "amenity": "public amenity civic service facility 公共服务 配套 设施",
    "leisure": "leisure recreation sport park open space 休闲 游憩 运动 公园",
    "tourism": "tourism visitor attraction hospitality 旅游 游客 景点 游览",
    "man_made": "infrastructure utility engineered structure 人工构筑物 基础设施 工程设施",
}


CLASS_GLOSSES = {
    "01": "farmland cropland cultivated land agriculture",
    "02": "orchard garden perennial crops nursery plantation",
    "03": "forest woodland trees shrubland bamboo",
    "04": "grassland meadow pasture lawn open field",
    "06": "agricultural facility farm greenhouse breeding aquaculture rural road",
    "07": "residential housing dwelling apartment community service",
    "08": "public service civic government education health culture sport welfare research",
    "09": "commercial business finance retail restaurant hotel entertainment service",
    "10": "industrial mining quarry manufacturing factory heavy industry",
    "11": "warehouse logistics storage depot reserve",
    "12": "transportation traffic railway road airport harbor parking station terminal",
    "13": "utility municipal infrastructure water wastewater power gas telecom post sanitation fire",
    "14": "green space park plaza open space recreation landscape",
    "15": "special military embassy religion heritage prison cemetery",
    "16": "undeveloped construction brownfield greenfield reserve",
    "0701": "urban residential apartment housing dormitory neighborhood dwelling",
    "0702": "community service neighborhood support garage childcare clinic supermarket",
    "0801": "government civic administration public office court police embassy party union",
    "0802": "research institute laboratory observatory innovation science",
    "0803": "culture library museum theater cinema gallery archive arts exhibition concert",
    "0804": "education school university college kindergarten campus training",
    "0805": "sport stadium arena gym swimming field track fitness",
    "0806": "health hospital clinic doctor dentist medical public health care nursing",
    "0807": "welfare nursing home childcare social care charity elderly disability rescue",
    "0901": "retail market restaurant hotel shop mall supermarket hospitality fuel station",
    "0902": "office finance bank business coworking conference exhibition corporate",
    "0903": "entertainment cinema nightclub casino amusement theme park zoo aquarium attraction",
    "0904": "service repair beauty laundry pet wash logistics outlet",
    "1001": "factory industrial manufacturing workshop plant",
    "1002": "mining quarry extraction tailings",
    "1003": "salt pond saltern salt field",
    "1101": "warehouse logistics storage depot silo distribution",
    "1102": "strategic reserve storage reserve warehouse",
    "1201": "railway rail station track train",
    "1203": "airport aviation hangar aeroway",
    "1204": "harbor port quay pier marina dock ferry",
    "1208": "transport station terminal bus depot parking transit",
    "1209": "transport facility driving school fuel charging inspection",
    "1301": "water works water supply treatment pumping",
    "1302": "wastewater sewage drainage treatment",
    "1303": "power electricity substation transmission",
    "1304": "gas fuel gasometer gas supply",
    "1305": "heating cooling energy station",
    "1306": "telecom communication tower station",
    "1307": "postal post office mail sorting",
    "1309": "sanitation landfill toilet waste cleaning",
    "1310": "fire station firefighting emergency response",
    "1311": "dam dyke levee hydraulic reservoir water engineering",
    "1312": "utility maintenance municipal infrastructure",
    "1401": "park garden green public landscape recreation",
    "1402": "protective green buffer shelterbelt defense green",
    "1403": "plaza square playground recreation open ground activity",
    "1501": "military barracks defense bunker",
    "1502": "embassy diplomatic consulate foreign affairs",
    "1503": "religion worship church mosque temple chapel pagoda",
    "1504": "heritage historic ruin monument archaeology",
    "1505": "prison detention correctional custody",
    "1506": "cemetery grave funeral crematorium burial",
    "1507": "special protected use",
    "080301": "library museum gallery archive exhibition science museum planetarium",
    "080302": "theater cinema arts centre concert hall studio cultural activity",
    "080401": "university college higher education campus",
    "080402": "vocational technical secondary education",
    "080403": "school primary school middle school high school",
    "080404": "kindergarten preschool nursery childcare school",
    "080405": "education training language school music school special education",
    "080501": "stadium sports hall arena swimming pool sports centre field track gym",
    "080502": "sports training base practice ground",
    "080601": "hospital general hospital specialist hospital nursing hospital",
    "080602": "clinic doctor dentist community health primary care",
    "080603": "public health emergency blood center disease control",
    "080701": "nursing home elderly senior welfare aged care",
    "080702": "children welfare orphan childcare shelter",
    "080704": "charity rescue social welfare support",
    "090101": "retail mall supermarket convenience store bookstore furniture florist electronics clothes",
    "090102": "wholesale marketplace market trading",
    "090103": "restaurant cafe bar pub fast food food court dining",
    "090104": "hotel hostel guesthouse motel resort camp caravan lodging accommodation",
    "090105": "fuel station charging station utility outlet service point",
    "100101": "light industry low impact factory",
    "100102": "general industry medium impact manufacturing",
    "100103": "heavy industry hazardous industry kiln chimney petroleum",
    "110101": "general logistics warehouse distribution storage",
    "110103": "hazardous warehouse tank dangerous goods storage",
    "120801": "rail terminal ferry terminal external transport station train station",
    "120802": "bus station taxi station transit depot public transport terminal",
    "120803": "public parking social parking parking garage bicycle parking",
}


NON_CLASSIFIABLE_TAGS = {
    "yes",
    "no",
    "roof",
    "part:yes",
    "bench",
    "bbq",
    "bandstand",
    "bird_hide",
    "bleachers",
    "ceremonial_gate",
    "clothes_dryer",
    "crane",
    "embankment",
    "gate",
    "gatehouse",
    "grandstand",
    "historic",
    "outdoor_seating",
    "paved",
    "pavilion",
    "public",
    "ruins",
    "security",
    "shed",
    "shelter",
    "shower",
    "smoking_area",
    "tower",
    "wall",
}


NON_CLASSIFIABLE_HINTS = {
    "roof": "仅表示屋顶结构",
    "yes": "仅表示有建筑，不代表用地功能",
    "no": "否定性或无效语义",
    "part:yes": "建筑部件，不代表主体用地",
}


TAG_QUERY_HINTS = {
    "gynaeology": "gynecology obstetrics women health clinic hospital medical",
    "ger": "yurt dwelling residence housing home",
    "quango": "quasi autonomous non governmental organization public agency office",
    "bungalow_yes": "bungalow residential house low rise dwelling",
    "public_bookcase": "public bookcase reading cabinet library culture",
    "hackerspace": "makerspace innovation lab community workshop research studio",
    "carport": "parking shelter car garage parking structure",
    "outbuilding": "ancillary building utility shed accessory structure",
    "parcel_locker": "parcel locker package pickup postal logistics service point",
    "guide": "tour guide tourist information visitor service",
    "fitness_station": "outdoor fitness station exercise equipment recreation",
    "guardhouse": "guard house security booth gate security office",
}


TAG_ZH_OVERRIDES = {
    "pump station": "泵站",
    "animal_breeding": "畜禽养殖设施",
    "apartment": "公寓住宅",
    "apartments": "公寓住宅",
    "aquaculture": "水产养殖设施",
    "aquarium": "水族馆",
    "archive": "档案馆",
    "arts_centre": "艺术中心",
    "atm": "自动取款机",
    "bank": "银行",
    "bar": "酒吧",
    "barn": "农仓",
    "beauty": "美容店",
    "bench": "长椅",
    "bicycle_parking": "自行车停车设施",
    "bicycle_rental": "自行车租赁点",
    "boat_storage": "船只存放设施",
    "brownfield": "棕地/再开发用地",
    "bungalow": "平房住宅",
    "bus_station": "公交车站",
    "cafe": "咖啡馆",
    "camp_site": "露营地",
    "car_rental": "汽车租赁点",
    "car_repair": "汽车维修店",
    "car_wash": "洗车点",
    "caravan_site": "房车营地",
    "carport": "车棚",
    "cathedral": "主教座堂",
    "cemetery": "墓地",
    "charging_station": "充电站",
    "childcare": "托育设施",
    "chimney": "烟囱",
    "church": "教堂",
    "cinema": "电影院",
    "civic": "公共行政设施",
    "civic_admin": "公共行政办公",
    "clinic": "诊所",
    "college": "学院",
    "commercial": "商业建筑",
    "community_centre": "社区中心",
    "concert_hall": "音乐厅",
    "conference_centre": "会议中心",
    "convenience": "便利店",
    "cooling_tower": "冷却塔",
    "courthouse": "法院",
    "coworking": "共享办公空间",
    "cowshed": "牛舍",
    "crematorium": "火葬场",
    "dentist": "牙科诊所",
    "department_store": "百货商店",
    "depot": "仓储/场站",
    "detached": "独栋住宅",
    "doctors": "医生诊所",
    "dog_park": "犬类活动公园",
    "dormitory": "宿舍",
    "driver_training": "驾驶培训场",
    "driving_school": "驾校",
    "drying_yard": "晾晒场",
    "education": "教育设施",
    "educational_institution": "教育机构",
    "embassy": "使馆",
    "events_venue": "活动场馆",
    "exhibition_centre": "展览中心",
    "farm": "农场",
    "farm_auxiliary": "农用辅助建筑",
    "farmland": "农田",
    "farmyard": "农家庭院/农场院落",
    "fast_food": "快餐店",
    "ferry_terminal": "轮渡码头",
    "fire_station": "消防站",
    "fitness_centre": "健身中心",
    "fitness_station": "健身点",
    "food_court": "美食广场",
    "forest": "森林",
    "fuel": "加油站",
    "funeral_hall": "殡仪馆",
    "gallery": "画廊/展廊",
    "garage": "车库",
    "garages": "车库群",
    "garden": "花园",
    "garden_centre": "园艺中心",
    "gasometer": "储气设施",
    "gatehouse": "门房",
    "golf_course": "高尔夫球场",
    "government": "政府机构",
    "governmental": "政府机构",
    "govermental": "政府机构",
    "greenfield": "未开发绿地",
    "greenhouse": "温室",
    "greenhouse_horticulture": "园艺温室",
    "guardhouse": "警卫室/门卫室",
    "guest_house": "旅馆/民宿",
    "hackerspace": "创客空间",
    "hall": "大厅/会堂",
    "hangar": "机库",
    "hospital": "医院",
    "hostel": "青年旅舍",
    "hotel": "酒店",
    "house": "住宅",
    "industrial": "工业建筑",
    "kindergarten": "幼儿园",
    "kiosk": "售货亭",
    "landfill": "垃圾填埋场",
    "language_school": "语言学校",
    "laundry": "洗衣店",
    "library": "图书馆",
    "logistics": "物流设施",
    "mall": "购物中心",
    "manufacture": "制造业设施",
    "marketplace": "市场",
    "military": "军事设施",
    "monitoring_station": "监测站",
    "mosque": "清真寺",
    "motel": "汽车旅馆",
    "motorcycle_parking": "摩托车停车设施",
    "museum": "博物馆",
    "music_school": "音乐学校",
    "nature_reserve": "自然保护区",
    "nightclub": "夜总会",
    "nursing_home": "养老院",
    "observatory": "观测站",
    "office": "办公建筑",
    "orchard": "果园",
    "outbuilding": "附属建筑",
    "paddy": "水田",
    "pagoda": "塔/佛塔",
    "parcel_locker": "快递柜",
    "park": "公园",
    "parking": "停车场",
    "parking_entrance": "停车场入口",
    "parking_space": "停车位",
    "pavilion": "亭/展馆",
    "pharmacy": "药店",
    "picnic_site": "野餐点",
    "pier": "栈桥/码头",
    "pitch": "运动场",
    "place_of_worship": "宗教活动场所",
    "planetarium": "天文馆",
    "plant_nursery": "苗圃",
    "playground": "游乐场",
    "police": "警察机构",
    "post_depot": "邮政仓配点",
    "post_office": "邮局",
    "postpartum_care": "产后护理机构",
    "prison": "监狱",
    "property_management": "物业管理",
    "public_bath": "公共浴室",
    "public_building": "公共建筑",
    "pumping_station": "泵站",
    "quarry": "采石场",
    "railway": "铁路设施",
    "ranger_station": "护林/巡护站",
    "recreation_ground": "休闲活动场地",
    "recycling": "回收站",
    "religious": "宗教设施",
    "research": "科研设施",
    "research_institute": "科研机构",
    "reservoir": "水库",
    "residential": "居住建筑/居住用地",
    "resort": "度假村",
    "restaurant": "餐馆",
    "retail": "零售商业",
    "school": "学校",
    "semidetached_house": "半独立住宅",
    "social_centre": "社交活动中心",
    "social_facility": "社会福利设施",
    "sports_centre": "体育中心",
    "sports_hall": "体育馆",
    "stable": "马厩",
    "stadium": "体育场",
    "station": "场站",
    "storage_tank": "储罐",
    "studio": "工作室",
    "supermarket": "超市",
    "swimming_pool": "游泳池",
    "taxi": "出租车站",
    "telecommunication": "通信设施",
    "temple": "寺庙",
    "theatre": "剧院",
    "theme_park": "主题公园",
    "toilets": "厕所",
    "toll_booth": "收费亭",
    "tower": "塔",
    "townhall": "市政厅",
    "train_station": "火车站",
    "university": "大学",
    "vehicle_inspection": "机动车检测站",
    "veterinary": "兽医诊所",
    "viewpoint": "观景点",
    "warehouse": "仓库",
    "waste_disposal": "废弃物处置点",
    "waste_transfer_station": "垃圾转运站",
    "wastewater_plant": "污水处理厂",
    "water_park": "水上乐园",
    "water_tower": "水塔",
    "water_utility": "供水设施",
    "water_works": "自来水厂",
    "wholesale": "批发市场",
    "works": "工厂/作业场",
    "yes": "未细分建筑",
    "zoo": "动物园",
}


TAG_ZH_OVERRIDES.update(
    {
        "alcohol": "酒类商店",
        "aeroway": "航空相关设施",
        "allotments": "分租菜园/市民农园",
        "apartments_residential": "公寓住宅/居住建筑",
        "architect": "建筑师事务所",
        "artwork": "艺术品/公共艺术",
        "attraction": "旅游景点/吸引物",
        "bandstand": "露天演奏台",
        "barracks": "兵营/营房",
        "basin": "蓄水池/调蓄池",
        "bbq": "烧烤点",
        "beach_resort": "海滨度假设施",
        "beacon": "航标/信标",
        "biergarten": "啤酒花园",
        "bird_hide": "观鸟屋",
        "bleachers": "露天看台",
        "boathouse": "船屋/船库",
        "books": "书店/图书销售点",
        "breakwater": "防波堤",
        "bridge": "桥梁",
        "bureau_de_change": "货币兑换点",
        "bungalow_yes": "平房住宅",
        "bunker": "掩体/地下防护设施",
        "cabin": "木屋/小屋",
        "car": "汽车销售/汽车服务",
        "car_parts": "汽车配件店",
        "casino": "赌场/博彩场所",
        "ceremonial_gate": "牌楼/仪式门",
        "chalet": "山地木屋/度假小屋",
        "chapel": "小教堂",
        "charity": "慈善机构",
        "chiller": "制冷机房/冷水机组",
        "clothes": "服装店",
        "clothes_dryer": "晾晒设施",
        "coal_pile": "煤堆场",
        "commercial_yes": "商业建筑",
        "communications_tower": "通信塔",
        "common": "公共开敞地",
        "company": "公司办公",
        "computer": "电脑/数码产品店",
        "construction": "施工建设用地",
        "courtyard": "庭院",
        "crane": "起重机/吊装设施",
        "dam": "水坝",
        "dance": "舞蹈场所",
        "dancing_school": "舞蹈学校",
        "data_center": "数据中心",
        "diplomatic": "外交机构",
        "dressing_room": "更衣室",
        "dyke": "堤坝/堤防",
        "electricity": "电力设施",
        "electronics": "电子产品店",
        "embankment": "路堤/堤岸",
        "employment_agency": "职业中介",
        "energy_supplier": "能源供应营业点",
        "estate_agent": "房产中介",
        "exercise_point": "健身点",
        "fabric": "布料店",
        "financial": "金融机构",
        "fishing": "垂钓场所",
        "florist": "花店",
        "flowerbed": "花坛",
        "fountain": "喷泉",
        "funeral_directors": "殡葬服务点",
        "furniture": "家具店",
        "gate": "门/出入口",
        "geoglyph": "地景图案/地画",
        "ger": "蒙古包",
        "gift": "礼品店",
        "grass": "草地",
        "grave_yard": "墓园",
        "grandstand": "主看台",
        "greenery": "绿地/绿化带",
        "greengrocer": "蔬果店",
        "groyne": "丁坝/防沙堤",
        "guide": "导览服务点",
        "gynaeology": "妇科诊所",
        "hairdresser": "理发店",
        "harbour": "港湾/港区",
        "historic": "历史遗迹",
        "horse_riding": "骑马活动场",
        "houseware": "家居用品店",
        "hut": "小屋/简易房",
        "ice_cream": "冰淇淋店",
        "ice_rink": "溜冰场",
        "industrial_yes": "工业建筑",
        "information": "信息咨询点",
        "insurance": "保险机构",
        "it": "信息技术办公",
        "karaoke_box": "KTV/卡拉OK",
        "kiln": "窑炉",
        "lighthouse": "灯塔",
        "love_hotel": "情侣酒店",
        "marina": "游艇码头",
        "meadow": "草甸",
        "musical_instrument": "乐器店",
        "newspaper": "报刊亭/报刊店",
        "ngo": "非政府组织办公",
        "no": "非建筑/否定标签",
        "notary": "公证处",
        "optician": "眼镜店",
        "outdoor_seating": "室外座位区",
        "part:yes": "建筑局部",
        "paved": "硬化铺装地",
        "petroleum": "石油设施",
        "picnic_table": "野餐桌",
        "political_party": "政党机构",
        "pub": "酒馆",
        "quango": "半官方机构",
        "quay": "码头岸壁",
        "religional": "宗教设施",
        "repair": "维修店",
        "reservoir_covered": "覆土/封闭水库",
        "roof": "屋顶/雨棚",
        "ruins": "遗址/废墟",
        "satellite_dish": "卫星天线",
        "salt": "盐田/制盐设施",
        "salt_pond": "盐池",
        "security": "安保设施",
        "shed": "棚屋",
        "shelter": "庇护棚/候车亭",
        "shower": "淋浴设施",
        "silo": "筒仓",
        "skyscraper": "摩天楼/高层建筑",
        "smoking_area": "吸烟区",
        "sport": "体育设施",
        "stage": "舞台",
        "static_caravan": "固定房车/移动住宅",
        "stock_exchange": "证券交易所",
        "subway": "地铁设施",
        "tailings_pond": "尾矿池",
        "tea": "茶叶店/茶园",
        "terrace": "联排住宅",
        "therapist": "理疗诊所",
        "ticket": "售票点",
        "tobacco": "烟草店",
        "track": "跑道/赛道",
        "trade": "商贸/专业零售",
        "tunnel": "隧道",
        "union": "工会机构",
        "variety_store": "杂货店",
        "ventilation_shaft": "通风井",
        "village_green": "村庄绿地",
        "vineyard": "葡萄园",
        "wall": "墙",
        "wine": "酒类商店",
        "yes_roof": "未细分建筑/屋顶",
    }
)


TAG_ZH_TOKEN_FALLBACK = {
    "animal": "畜禽",
    "art": "艺术",
    "arts": "艺术",
    "building": "建筑",
    "centre": "中心",
    "charging": "充电",
    "community": "社区",
    "conference": "会议",
    "education": "教育",
    "facility": "设施",
    "farm": "农场",
    "food": "餐饮",
    "garden": "花园",
    "government": "政府",
    "greenhouse": "温室",
    "guest": "旅馆",
    "hall": "馆/厅",
    "house": "住宅",
    "industrial": "工业",
    "market": "市场",
    "office": "办公",
    "parking": "停车",
    "plant": "厂/设施",
    "public": "公共",
    "research": "科研",
    "school": "学校",
    "service": "服务",
    "social": "社会服务",
    "sports": "体育",
    "station": "站",
    "storage": "存储",
    "transportation": "交通",
    "waste": "废弃物",
    "water": "水务",
}


def tag_to_chinese_name(tag_value: str) -> str:
    tag = str(tag_value).strip()
    normalized = normalize_tag(tag)
    if normalized in TAG_ZH_OVERRIDES:
        return TAG_ZH_OVERRIDES[normalized]
    if tag in TAG_ZH_OVERRIDES:
        return TAG_ZH_OVERRIDES[tag]

    parts = [part for part in re.split(r"[_:\-\s]+", normalized) if part and part != "yes"]
    translated = [TAG_ZH_TOKEN_FALLBACK[part] for part in parts if part in TAG_ZH_TOKEN_FALLBACK]
    if translated:
        return "".join(translated)
    return f"未翻译标签（{tag}）"


SEMANTIC_PRIMARY_MIN_SCORE = 0.18
SEMANTIC_SECONDARY_MIN_SCORE = 0.2
SEMANTIC_TERTIARY_MIN_SCORE = 0.32
SEMANTIC_TERTIARY_MARGIN = 0.03


def make_result(
    primary_code: str,
    landuse_hint: str,
    confidence: float,
    rationale: str,
    matched_by: str,
    use_for_landuse_classification: bool = True,
) -> dict[str, Any]:
    return {
        "primary_code": primary_code,
        "primary_name": PLANNING_PRIMARY[primary_code],
        "landuse_hint": landuse_hint,
        "confidence": float(confidence),
        "use_for_landuse_classification": bool(use_for_landuse_classification),
        "matched_by": matched_by,
        "rationale": rationale,
    }


@dataclass
class LLMFallbackConfig:
    base_url: str
    api_key: str
    model: str | None = None
    timeout: float = DEFAULT_LLM_TIMEOUT
    trigger_primary_max: float = LLM_TRIGGER_PRIMARY_MAX
    cache_path: pathlib.Path = LLM_CACHE_PATH
    cache: dict[str, dict[str, Any]] = field(default_factory=dict)


@dataclass
class LegacyPrior:
    primary_code: str
    secondary_code: str = ""
    confidence: float = 0.0
    source: str = ""


def normalize_tag(value: str) -> str:
    return str(value).strip().lower()


def split_tag_text(tag: str) -> str:
    return re.sub(r"[_:\-]+", " ", normalize_tag(tag)).strip()


def read_text_with_fallback(path: pathlib.Path) -> str:
    raw = path.read_bytes()
    for encoding in ("utf-8", "utf-8-sig", "gb18030", "gbk", "cp936"):
        try:
            return raw.decode(encoding)
        except UnicodeDecodeError:
            continue

    try:
        return raw.decode()
    except UnicodeDecodeError:
        return raw.decode("latin1")


def parse_category_definitions() -> dict[str, str]:
    definitions: dict[str, str] = {}
    if not CATEGORY_TEXT_PATH.exists():
        return definitions

    pattern = re.compile(r"^(\d{2,6})\s+([^\t ]+)\s+(.+)$")
    for raw_line in read_text_with_fallback(CATEGORY_TEXT_PATH).splitlines():
        line = raw_line.strip()
        match = pattern.match(line)
        if not match:
            continue
        code, _name, description = match.groups()
        definitions[code] = description.strip()
    return definitions


def build_category_profiles() -> list[dict[str, str]]:
    definitions = parse_category_definitions()
    profiles: list[dict[str, str]] = []

    for code, name in PLANNING_PRIMARY.items():
        if code == "00":
            continue
        profiles.append(
            {
                "level": "primary",
                "code": code,
                "primary_code": code,
                "secondary_code": "",
                "tertiary_code": "",
                "name": name,
                "text": " | ".join(
                    filter(
                        None,
                        [
                            f"primary land use {code} {name}",
                            CLASS_GLOSSES.get(code, ""),
                            definitions.get(code, ""),
                        ],
                    )
                ),
            }
        )

    for code, name in PLANNING_SECONDARY.items():
        if code == "0000":
            continue
        primary_code = code[:2]
        profiles.append(
            {
                "level": "secondary",
                "code": code,
                "primary_code": primary_code,
                "secondary_code": code,
                "tertiary_code": "",
                "name": name,
                "text": " | ".join(
                    filter(
                        None,
                        [
                            f"secondary land use {code} {name}",
                            f"parent primary {PLANNING_PRIMARY.get(primary_code, '')}",
                            CLASS_GLOSSES.get(code, ""),
                            definitions.get(code, ""),
                        ],
                    )
                ),
            }
        )

    for code, name in PLANNING_TERTIARY.items():
        secondary_code = code[:4]
        primary_code = code[:2]
        profiles.append(
            {
                "level": "tertiary",
                "code": code,
                "primary_code": primary_code,
                "secondary_code": secondary_code,
                "tertiary_code": code,
                "name": name,
                "text": " | ".join(
                    filter(
                        None,
                        [
                            f"tertiary land use {code} {name}",
                            f"parent secondary {PLANNING_SECONDARY.get(secondary_code, '')}",
                            f"parent primary {PLANNING_PRIMARY.get(primary_code, '')}",
                            CLASS_GLOSSES.get(code, ""),
                            definitions.get(code, ""),
                        ],
                    )
                ),
            }
        )

    return profiles


def build_query_text(tag_value: str, source_categories: str) -> str:
    tag = normalize_tag(tag_value)
    pieces = [
        f"osm tag value: {tag}",
        f"normalized tag phrase: {split_tag_text(tag)}",
    ]
    if tag in TAG_QUERY_HINTS:
        pieces.append(f"semantic hint: {TAG_QUERY_HINTS[tag]}")

    categories = [normalize_tag(item) for item in str(source_categories).split("|") if str(item).strip()]
    if categories:
        pieces.append("source categories: " + ", ".join(categories))
        pieces.extend(SOURCE_CATEGORY_GLOSSES.get(category, "") for category in categories if category in SOURCE_CATEGORY_GLOSSES)

    return " | ".join(piece for piece in pieces if piece)


def normalize_base_url(base_url: str) -> str:
    return str(base_url).rstrip("/")


def load_llm_cache(path: pathlib.Path) -> dict[str, dict[str, Any]]:
    if not path.exists():
        return {}
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except Exception:
        return {}
    return data if isinstance(data, dict) else {}


def save_llm_cache(path: pathlib.Path, cache: dict[str, dict[str, Any]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(cache, ensure_ascii=False, indent=2), encoding="utf-8")


def build_llm_cache_key(tag_value: str, source_categories: str, model: str) -> str:
    return f"{normalize_tag(tag_value)}|{normalize_tag(source_categories)}|{model}"


@functools.lru_cache(maxsize=1)
def load_legacy_priors() -> dict[str, LegacyPrior]:
    priors: dict[str, LegacyPrior] = {}

    if LEGACY_ALL_TAGS_XLSX_PATH.exists():
        try:
            with pd.ExcelFile(LEGACY_ALL_TAGS_XLSX_PATH) as xlsx:
                if "all_tags" in xlsx.sheet_names:
                    df = pd.read_excel(xlsx, sheet_name="all_tags")
                    for _, row in df.iterrows():
                        tag = normalize_tag(row.get("value", ""))
                        if not tag:
                            continue
                        if str(row.get("matched_by", "")).strip().lower() != "exact":
                            continue
                        if not bool(row.get("use_for_landuse_classification", False)):
                            continue
                        confidence = float(row.get("confidence", 0.0) or 0.0)
                        if confidence < 0.85:
                            continue
                        primary_code = str(row.get("primary_code", "")).strip().zfill(2)
                        if primary_code not in PLANNING_PRIMARY or primary_code == "00":
                            continue
                        priors[tag] = LegacyPrior(
                            primary_code=primary_code,
                            confidence=confidence,
                            source="legacy_xlsx_exact",
                        )
                elif "tag_mapping" in xlsx.sheet_names:
                    df = pd.read_excel(xlsx, sheet_name="tag_mapping")
                    for _, row in df.iterrows():
                        tag = normalize_tag(row.get("tag", ""))
                        if not tag:
                            continue
                        if str(row.get("matched_by", "")).strip().lower() != "legacy_prior":
                            continue
                        if not bool(row.get("use_for_landuse_classification", False)):
                            continue
                        confidence = float(row.get("weight_l1", 0.0) or 0.0)
                        if confidence < 0.85:
                            continue
                        primary_code = str(row.get("primary_code", "")).strip().zfill(2)
                        secondary_code = str(row.get("secondary_code", "")).strip().zfill(4) if str(row.get("secondary_code", "")).strip() else ""
                        if primary_code not in PLANNING_PRIMARY or primary_code == "00":
                            continue
                        priors[tag] = LegacyPrior(
                            primary_code=primary_code,
                            secondary_code=secondary_code if secondary_code in PLANNING_SECONDARY else "",
                            confidence=confidence,
                            source="legacy_xlsx_tag_mapping",
                        )
        except Exception:
            pass

    if BUILDING_REFERENCE_MAPPING_PATH.exists():
        try:
            df = pd.read_csv(BUILDING_REFERENCE_MAPPING_PATH)
            for _, row in df.iterrows():
                tag = normalize_tag(row.get("tag", ""))
                if not tag:
                    continue
                if not bool(row.get("use_for_landuse_classification", False)):
                    continue
                confidence = float(row.get("classification_weight", 0.0) or 0.0)
                if confidence < 0.85:
                    continue
                primary_code = str(row.get("primary_code", "")).strip().zfill(2)
                secondary_code = str(row.get("secondary_code", "")).strip().zfill(4) if str(row.get("secondary_code", "")).strip() else ""
                if primary_code not in PLANNING_PRIMARY or primary_code == "00":
                    continue
                prior = priors.get(tag)
                if prior is None or confidence >= prior.confidence:
                    priors[tag] = LegacyPrior(
                        primary_code=primary_code,
                        secondary_code=secondary_code if secondary_code in PLANNING_SECONDARY else "",
                        confidence=confidence,
                        source="building_reference_mapping",
                    )
                elif prior.secondary_code == "" and secondary_code in PLANNING_SECONDARY:
                    prior.secondary_code = secondary_code
        except Exception:
            pass

    return priors


class SemanticSimilarityEngine:
    def __init__(self, profiles: list[dict[str, str]]):
        self.profiles = profiles
        self.backend = "semantic_tfidf_lsa"
        self._st_model = None
        self._matrix = None
        self._vectorizer = None
        self._svd = None
        self._fit()

    def _fit(self) -> None:
        texts = [item["text"] for item in self.profiles]
        try:
            from sentence_transformers import SentenceTransformer

            self._st_model = SentenceTransformer("sentence-transformers/paraphrase-multilingual-MiniLM-L12-v2")
            self._matrix = self._st_model.encode(texts, normalize_embeddings=True)
            self.backend = "semantic_embedding"
            return
        except Exception:
            self._st_model = None

        vectorizer = TfidfVectorizer(analyzer="char_wb", ngram_range=(2, 5), lowercase=True)
        tfidf = vectorizer.fit_transform(texts)
        max_components = max(1, min(64, tfidf.shape[0] - 1, tfidf.shape[1] - 1))
        svd = TruncatedSVD(n_components=max_components, random_state=42)
        matrix = normalize(svd.fit_transform(tfidf))
        self._vectorizer = vectorizer
        self._svd = svd
        self._matrix = matrix

    def _encode_query(self, query_text: str):
        if self.backend == "semantic_embedding":
            return self._st_model.encode([query_text], normalize_embeddings=True)[0]
        vector = self._vectorizer.transform([query_text])
        return normalize(self._svd.transform(vector))[0]

    def rank(self, query_text: str, level: str, prefix: str | None = None) -> list[dict[str, Any]]:
        query_vector = self._encode_query(query_text)
        scored: list[dict[str, Any]] = []
        for index, item in enumerate(self.profiles):
            if item["level"] != level:
                continue
            if prefix and not item["code"].startswith(prefix):
                continue
            score = float(query_vector @ self._matrix[index])
            scored.append({**item, "score": score})
        return sorted(scored, key=lambda row: row["score"], reverse=True)


@functools.lru_cache(maxsize=1)
def get_semantic_engine() -> SemanticSimilarityEngine:
    return SemanticSimilarityEngine(build_category_profiles())


# Legacy reference only. Semantic pipeline below does not read this table.
LEGACY_EXACT_MAPPING_UNUSED = {
    "residential": make_result("07", "城镇住宅用地", 1.0, "直接表达住宅功能", "exact"),
    "apartment": make_result("07", "城镇住宅用地", 0.98, "公寓型住宅", "exact"),
    "apartments": make_result("07", "城镇住宅用地", 1.0, "公寓型住宅", "exact"),
    "house": make_result("07", "城镇住宅用地", 0.95, "独立住宅", "exact"),
    "detached": make_result("07", "城镇住宅用地", 0.95, "独立住宅", "exact"),
    "semidetached_house": make_result("07", "城镇住宅用地", 0.95, "半独立住宅", "exact"),
    "bungalow": make_result("07", "城镇住宅用地", 0.9, "低层住宅", "exact"),
    "bungalow_yes": make_result("07", "城镇住宅用地", 0.75, "住宅语义较强，但标签混杂", "exact"),
    "chalet": make_result("07", "居住/度假住宅用地", 0.78, "多为度假住宅或低密住宅", "exact"),
    "cabin": make_result("07", "居住/度假住宅用地", 0.7, "小型居住建筑", "exact"),
    "dormitory": make_result("07", "居住用地", 0.88, "宿舍类居住设施", "exact"),
    "ger": make_result("07", "居住用地", 0.8, "居住帐篷/毡房", "exact"),
    "hut": make_result("07", "居住/附属建筑用地", 0.55, "小型居住或附属建筑", "exact"),
    "stilt_house": make_result("07", "居住用地", 0.85, "住宅建筑类型", "exact"),
    "garages": make_result("07", "居住配套设施用地", 0.65, "常作为住宅配套", "exact"),
    "carport": make_result("07", "居住配套设施用地", 0.55, "停车棚多为附属配套", "exact"),
    "outbuilding": make_result("07", "居住/院落附属设施用地", 0.45, "附属建筑语义较弱", "exact"),
    "public_building": make_result("08", "公共建筑用地", 0.5, "表明公共建筑，但功能不够细", "exact"),
    "school": make_result("08", "教育用地", 1.0, "学校教育设施", "exact"),
    "college": make_result("08", "教育用地", 1.0, "高等教育设施", "exact"),
    "university": make_result("08", "教育用地", 1.0, "高等教育设施", "exact"),
    "kindergarten": make_result("08", "教育用地", 1.0, "学前教育设施", "exact"),
    "education": make_result("08", "教育用地", 0.95, "教育设施", "exact"),
    "educational_institution": make_result("08", "教育用地", 0.95, "教育机构", "exact"),
    "prep_school": make_result("08", "教育用地", 0.9, "预科学校", "exact"),
    "language_school": make_result("08", "教育用地", 0.9, "语言培训学校", "exact"),
    "music_school": make_result("08", "教育用地", 0.88, "专业教育设施", "exact"),
    "research_institute": make_result("08", "科研教育用地", 1.0, "科研机构", "exact"),
    "research": make_result("08", "科研教育用地", 0.85, "科研功能", "exact"),
    "archive": make_result("08", "文化设施用地", 0.88, "档案馆属于公共文化设施", "exact"),
    "arts_centre": make_result("08", "文化设施用地", 1.0, "艺术中心", "exact"),
    "art": make_result("08", "文化设施用地", 0.6, "艺术功能，语义偏文化", "exact"),
    "artwork": make_result("14", "绿地与开敞空间附属景观设施", 0.35, "更像景观小品，不直接判定主体用地", "exact", False),
    "museum": make_result("08", "文化设施用地", 1.0, "博物馆", "exact"),
    "theatre": make_result("08", "文化设施用地", 1.0, "剧院", "exact"),
    "cinema": make_result("08", "文化设施用地", 0.85, "文化娱乐设施", "exact"),
    "gallery": make_result("08", "文化设施用地", 0.85, "展览/画廊设施", "exact"),
    "library": make_result("08", "文化设施用地", 1.0, "图书馆", "exact"),
    "planetarium": make_result("08", "文化科普设施用地", 0.95, "天文馆", "exact"),
    "observatory": make_result("08", "科研教育用地", 0.85, "天文/观测设施", "exact"),
    "community_centre": make_result("08", "社区公共服务用地", 0.95, "社区服务中心", "exact"),
    "social_centre": make_result("08", "社会福利用地", 0.88, "社会服务设施", "exact"),
    "social_facility": make_result("08", "社会福利用地", 0.88, "社会福利设施", "exact"),
    "charity": make_result("08", "社会福利用地", 0.75, "慈善服务设施", "exact"),
    "childcare": make_result("08", "社会福利/托育用地", 0.85, "托育照护设施", "exact"),
    "nursing_home": make_result("08", "社会福利用地", 0.95, "养老护理设施", "exact"),
    "hospital": make_result("08", "医疗卫生用地", 1.0, "医院", "exact"),
    "clinic": make_result("08", "医疗卫生用地", 0.95, "诊所", "exact"),
    "doctors": make_result("08", "医疗卫生用地", 0.9, "医疗服务机构", "exact"),
    "dentist": make_result("08", "医疗卫生用地", 0.9, "口腔医疗机构", "exact"),
    "gynaeology": make_result("08", "医疗卫生用地", 0.88, "专科医疗机构", "exact"),
    "veterinary": make_result("08", "公共服务/动物医疗用地", 0.7, "动物医疗服务", "exact"),
    "postpartum_care": make_result("08", "医疗卫生/护理用地", 0.82, "产后护理服务", "exact"),
    "post_office": make_result("08", "公共服务用地", 0.9, "邮政服务设施", "exact"),
    "public_bookcase": make_result("08", "文化设施用地", 0.7, "公共书柜属于公共文化服务", "exact"),
    "concert_hall": make_result("08", "文化设施用地", 0.95, "音乐厅", "exact"),
    "studio": make_result("08", "文化设施用地", 0.72, "工作室/创作空间，偏文化功能", "exact"),
    "hackerspace": make_result("08", "科研教育/创新服务用地", 0.68, "创客空间", "exact"),
    "political_party": make_result("08", "机关团体用地", 0.82, "政党组织设施", "exact"),
    "quango": make_result("08", "机关团体用地", 0.72, "准公共机构", "exact"),
    "union": make_result("08", "机关团体/公共服务用地", 0.7, "工会等组织设施", "exact"),
    "ngo": make_result("08", "社会福利/公共服务用地", 0.72, "社会组织机构", "exact"),
    "toilets": make_result("08", "公共服务附属设施用地", 0.45, "公共厕所，通常为附属服务设施", "exact", False),
    "public_bath": make_result("08", "公共服务用地", 0.62, "公共浴室/洗浴设施", "exact"),
    "swimming_pool": make_result("08", "体育用地", 0.82, "游泳设施", "exact"),
    "dance": make_result("08", "文化体育用地", 0.62, "舞蹈活动设施", "exact"),
    "dressing_room": make_result("08", "体育/公共服务附属设施用地", 0.42, "更像附属更衣设施", "exact", False),
    "hall": make_result("08", "公共管理与公共服务用地", 0.5, "hall 语义较宽，偏公共场馆", "exact"),
    "government": make_result("08", "机关团体用地", 1.0, "政府机关", "exact"),
    "governmental": make_result("08", "机关团体用地", 1.0, "政府机关", "exact"),
    "govermental": make_result("08", "机关团体用地", 0.95, "governmental 拼写变体", "exact"),
    "civic": make_result("08", "机关团体/公共管理用地", 0.85, "公共事务设施", "exact"),
    "civic_admin": make_result("08", "机关团体用地", 1.0, "行政管理设施", "exact"),
    "townhall": make_result("08", "机关团体用地", 1.0, "市政厅", "exact"),
    "courthouse": make_result("08", "机关团体/司法用地", 0.95, "法院", "exact"),
    "embassy": make_result("08", "机关团体/外交用地", 0.95, "使馆", "exact"),
    "diplomatic": make_result("08", "机关团体/外交用地", 0.88, "外交用途", "exact"),
    "police": make_result("08", "公共安全设施用地", 0.95, "警务设施", "exact"),
    "fire_station": make_result("13", "消防用地", 1.0, "消防设施", "exact"),
    "sports_centre": make_result("08", "体育用地", 1.0, "体育中心", "exact"),
    "sports_hall": make_result("08", "体育用地", 1.0, "体育馆", "exact"),
    "stadium": make_result("08", "体育用地", 1.0, "体育场", "exact"),
    "pitch": make_result("08", "体育用地", 0.9, "运动场地", "exact"),
    "track": make_result("08", "体育用地", 0.9, "跑道/运动场", "exact"),
    "fitness_centre": make_result("08", "体育用地", 0.78, "健身设施", "exact"),
    "fitness_station": make_result("14", "开敞空间附属体育设施", 0.55, "户外健身点，偏公园附属设施", "exact"),
    "ice_rink": make_result("08", "体育用地", 0.9, "冰上运动设施", "exact"),
    "winter_sports": make_result("08", "体育用地", 0.75, "冰雪运动设施", "exact"),
    "sport": make_result("08", "体育用地", 0.5, "体育语义明确但类型较粗", "exact"),
    "commercial": make_result("09", "商业用地", 1.0, "商业功能", "exact"),
    "retail": make_result("09", "商业用地", 1.0, "零售商业", "exact"),
    "mall": make_result("09", "商业用地", 1.0, "购物中心", "exact"),
    "department_store": make_result("09", "商业用地", 1.0, "百货商场", "exact"),
    "supermarket": make_result("09", "商业用地", 0.98, "超市零售", "exact"),
    "convenience": make_result("09", "商业用地", 0.92, "便利店", "exact"),
    "marketplace": make_result("09", "商业用地", 1.0, "市场", "exact"),
    "wholesale": make_result("09", "商业用地", 0.9, "批发商业", "exact"),
    "variety_store": make_result("09", "商业用地", 0.85, "日杂零售", "exact"),
    "books": make_result("09", "商业用地", 0.82, "书店零售", "exact"),
    "electronics": make_result("09", "商业用地", 0.82, "电子产品零售", "exact"),
    "furniture": make_result("09", "商业用地", 0.82, "家具零售", "exact"),
    "gift": make_result("09", "商业用地", 0.82, "礼品零售", "exact"),
    "greengrocer": make_result("09", "商业用地", 0.82, "生鲜零售", "exact"),
    "houseware": make_result("09", "商业用地", 0.82, "家居用品零售", "exact"),
    "kiosk": make_result("09", "商业用地", 0.8, "亭式零售", "exact"),
    "musical_instrument": make_result("09", "商业用地", 0.8, "乐器零售", "exact"),
    "newspaper": make_result("09", "商业用地", 0.7, "报刊零售点", "exact"),
    "optician": make_result("09", "商业用地", 0.82, "眼镜零售服务", "exact"),
    "pharmacy": make_result("09", "商业用地", 0.78, "药店零售", "exact"),
    "tea": make_result("09", "商业用地", 0.78, "茶叶商业", "exact"),
    "tobacco": make_result("09", "商业用地", 0.75, "烟草零售", "exact"),
    "wine": make_result("09", "商业用地", 0.78, "酒类零售", "exact"),
    "bar": make_result("09", "商业用地", 0.95, "酒吧餐饮娱乐", "exact"),
    "cafe": make_result("09", "商业用地", 0.95, "咖啡馆", "exact"),
    "restaurant": make_result("09", "商业用地", 1.0, "餐饮设施", "exact"),
    "fast_food": make_result("09", "商业用地", 0.95, "快餐", "exact"),
    "food_court": make_result("09", "商业用地", 0.9, "餐饮聚合空间", "exact"),
    "ice_cream": make_result("09", "商业用地", 0.82, "餐饮零售", "exact"),
    "pub": make_result("09", "商业用地", 0.9, "酒馆餐饮", "exact"),
    "beauty": make_result("09", "商业用地", 0.85, "美容服务", "exact"),
    "estate_agent": make_result("09", "商业用地", 0.82, "地产中介服务", "exact"),
    "hairdresser": make_result("09", "商业用地", 0.85, "理发服务", "exact"),
    "laundry": make_result("09", "商业用地", 0.82, "洗衣服务", "exact"),
    "repair": make_result("09", "商业服务业用地", 0.7, "维修服务", "exact"),
    "company": make_result("09", "商务办公用地", 0.82, "企业办公", "exact"),
    "conference_centre": make_result("09", "商务会展用地", 0.88, "会议中心", "exact"),
    "coworking": make_result("09", "商务办公用地", 0.82, "共享办公", "exact"),
    "events_venue": make_result("09", "商务会展/活动用地", 0.78, "活动场馆", "exact"),
    "exhibition_centre": make_result("09", "商务会展用地", 0.9, "展览中心", "exact"),
    "office": make_result("09", "商务办公用地", 1.0, "办公功能", "exact"),
    "it": make_result("09", "商务办公用地", 0.75, "信息技术办公", "exact"),
    "property_management": make_result("09", "商务服务用地", 0.75, "物业管理办公", "exact"),
    "atm": make_result("09", "商业服务业用地", 0.65, "银行附属服务点", "exact"),
    "bank": make_result("09", "商务金融用地", 0.95, "银行金融服务", "exact"),
    "casino": make_result("09", "商业服务业用地", 0.85, "娱乐服务设施", "exact"),
    "hotel": make_result("09", "旅馆用地", 1.0, "酒店", "exact"),
    "hostel": make_result("09", "旅馆用地", 0.9, "旅舍", "exact"),
    "guest_house": make_result("09", "旅馆用地", 0.9, "宾馆/民宿", "exact"),
    "motel": make_result("09", "旅馆用地", 0.9, "汽车旅馆", "exact"),
    "love_hotel": make_result("09", "旅馆用地", 0.88, "主题旅馆", "exact"),
    "resort": make_result("09", "旅游服务用地", 0.92, "度假设施", "exact"),
    "beach_resort": make_result("09", "旅游服务用地", 0.9, "度假设施", "exact"),
    "camp_site": make_result("09", "旅游服务用地", 0.82, "营地", "exact"),
    "caravan_site": make_result("09", "旅游服务用地", 0.82, "房车营地", "exact"),
    "aquarium": make_result("09", "旅游服务/娱乐用地", 0.78, "水族馆常兼具旅游吸引功能", "exact"),
    "attraction": make_result("09", "旅游服务用地", 0.7, "景点设施", "exact"),
    "theme_park": make_result("09", "旅游娱乐用地", 0.88, "主题公园", "exact"),
    "water_park": make_result("09", "旅游娱乐用地", 0.88, "水上乐园", "exact"),
    "zoo": make_result("09", "旅游娱乐用地", 0.75, "动物园兼具公共与旅游功能，这里按旅游娱乐", "exact"),
    "guide": make_result("09", "旅游服务用地", 0.6, "导览/旅游服务", "exact"),
    "ticket": make_result("09", "旅游/商业附属服务用地", 0.45, "售票点，多为附属设施", "exact", False),
    "industrial": make_result("10", "工业用地", 1.0, "工业功能", "exact"),
    "manufacture": make_result("10", "工业用地", 0.92, "制造业功能", "exact"),
    "works": make_result("10", "工业用地", 0.85, "工厂/厂场", "exact"),
    "quarry": make_result("10", "工矿用地", 0.95, "采石场", "exact"),
    "coal_pile": make_result("10", "工矿用地", 0.8, "工业散料堆场", "exact"),
    "kiln": make_result("10", "工业用地", 0.85, "窑炉生产设施", "exact"),
    "warehouse": make_result("11", "物流仓储用地", 1.0, "仓储设施", "exact"),
    "logistics": make_result("11", "物流仓储用地", 0.92, "物流功能", "exact"),
    "boat_storage": make_result("11", "仓储用地", 0.75, "船舶存放设施", "exact"),
    "depot": make_result("11", "仓储/堆场用地", 0.8, "堆场仓储功能", "exact"),
    "post_depot": make_result("11", "物流仓储用地", 0.85, "邮政分拨/仓储", "exact"),
    "storage_tank": make_result("11", "仓储用地", 0.7, "储罐设施", "exact"),
    "silo": make_result("11", "仓储用地", 0.8, "筒仓", "exact"),
    "parking": make_result("12", "交通场站用地", 0.95, "停车设施", "exact"),
    "parking_entrance": make_result("12", "交通场站附属设施用地", 0.72, "停车出入口设施", "exact"),
    "parking_space": make_result("12", "交通场站用地", 0.9, "停车位", "exact"),
    "bicycle_parking": make_result("12", "交通场站用地", 0.88, "自行车停车设施", "exact"),
    "motorcycle_parking": make_result("12", "交通场站用地", 0.88, "摩托停车设施", "exact"),
    "bicycle_rental": make_result("12", "交通服务设施用地", 0.7, "共享单车/租赁点", "exact"),
    "taxi": make_result("12", "交通服务设施用地", 0.72, "出租车服务点", "exact"),
    "bus_station": make_result("12", "公共交通场站用地", 1.0, "公交场站", "exact"),
    "train_station": make_result("12", "铁路场站用地", 1.0, "火车站", "exact"),
    "station": make_result("12", "交通场站用地", 0.7, "站点设施，语义较粗", "exact"),
    "railway": make_result("12", "铁路用地", 1.0, "铁路设施", "exact"),
    "ferry_terminal": make_result("12", "水运交通场站用地", 0.95, "轮渡码头", "exact"),
    "marina": make_result("12", "水运交通服务用地", 0.82, "游艇码头", "exact"),
    "harbour": make_result("12", "港口用地", 0.9, "港口设施", "exact"),
    "pier": make_result("12", "水运交通设施用地", 0.88, "码头栈桥", "exact"),
    "quay": make_result("12", "港口用地", 0.85, "码头岸线设施", "exact"),
    "breakwater": make_result("12", "港航工程设施用地", 0.7, "防波堤工程", "exact"),
    "groyne": make_result("12", "港航工程设施用地", 0.6, "丁坝/导流工程", "exact"),
    "toll_booth": make_result("12", "交通服务设施用地", 0.88, "收费站", "exact"),
    "charging_station": make_result("12", "交通服务设施用地", 0.82, "充电站常服务交通", "exact"),
    "fuel": make_result("12", "交通服务设施用地", 0.9, "加油站", "exact"),
    "tunnel": make_result("12", "交通运输工程用地", 0.82, "隧道工程设施", "exact"),
    "lighthouse": make_result("12", "航运交通设施用地", 0.78, "航标/灯塔", "exact"),
    "car": make_result("09", "商业服务业用地", 0.7, "汽车销售/服务", "exact"),
    "car_parts": make_result("09", "商业服务业用地", 0.75, "汽配商业", "exact"),
    "car_rental": make_result("12", "交通服务设施用地", 0.82, "汽车租赁服务", "exact"),
    "car_repair": make_result("09", "商业服务业用地", 0.78, "汽车维修服务", "exact"),
    "car_wash": make_result("09", "商业服务业用地", 0.75, "洗车服务", "exact"),
    "driver_training": make_result("12", "交通服务设施用地", 0.82, "驾驶训练场", "exact"),
    "driving_school": make_result("12", "交通服务设施用地", 0.82, "驾校", "exact"),
    "vehicle_inspection": make_result("12", "交通服务设施用地", 0.85, "车辆检测站", "exact"),
    "transportation": make_result("12", "交通运输用地", 0.8, "交通运输功能", "exact"),
    "aeroway": make_result("12", "航空交通设施用地", 0.85, "航空设施", "exact"),
    "hangar": make_result("12", "航空交通设施用地", 0.82, "机库", "exact"),
    "water_works": make_result("13", "供水设施用地", 1.0, "供水厂站", "exact"),
    "wastewater_plant": make_result("13", "排水设施用地", 1.0, "污水处理厂", "exact"),
    "waste_disposal": make_result("13", "环卫设施用地", 0.92, "垃圾处理设施", "exact"),
    "waste_transfer_station": make_result("13", "环卫设施用地", 0.95, "垃圾转运站", "exact"),
    "landfill": make_result("13", "环卫设施用地", 1.0, "垃圾填埋场", "exact"),
    "recycling": make_result("13", "环卫设施用地", 0.82, "再生资源回收站", "exact"),
    "sanitary_dump_station": make_result("13", "环卫设施用地", 0.85, "环卫清运设施", "exact"),
    "pumping_station": make_result("13", "市政公用设施用地", 0.9, "泵站设施", "exact"),
    "pump station": make_result("13", "市政公用设施用地", 0.9, "泵站设施", "exact"),
    "pump_station": make_result("13", "市政公用设施用地", 0.9, "泵站设施", "exact"),
    "monitoring_station": make_result("13", "市政公用设施用地", 0.78, "监测站设施", "exact"),
    "telecommunication": make_result("13", "市政公用设施用地", 0.82, "通信设施", "exact"),
    "energy_supplier": make_result("13", "市政公用设施用地", 0.78, "能源供应设施", "exact"),
    "water_utility": make_result("13", "供水设施用地", 0.85, "供水公用设施", "exact"),
    "water_tower": make_result("13", "供水设施用地", 0.88, "水塔", "exact"),
    "water_point": make_result("13", "供水设施附属点位", 0.45, "取水点，附属设施", "exact", False),
    "water_wellfield": make_result("13", "供水设施用地", 0.9, "水源井场", "exact"),
    "basin": make_result("13", "市政水工设施用地", 0.72, "水池/调蓄设施", "exact"),
    "reservoir": make_result("13", "市政水工设施用地", 0.8, "水库/调蓄设施", "exact"),
    "reservoir_covered": make_result("13", "市政水工设施用地", 0.82, "覆盖式蓄水设施", "exact"),
    "dam": make_result("13", "市政水工设施用地", 0.82, "水坝工程", "exact"),
    "tailings_pond": make_result("13", "公用设施/工业尾矿设施用地", 0.7, "尾矿库", "exact"),
    "cooling_tower": make_result("13", "市政公用设施用地", 0.72, "冷却设施", "exact"),
    "chiller": make_result("13", "市政公用设施用地", 0.65, "制冷公用设施", "exact"),
    "gasometer": make_result("13", "市政公用设施用地", 0.82, "储气设施", "exact"),
    "petroleum": make_result("13", "市政公用设施/能源设施用地", 0.68, "能源设施语义", "exact"),
    "farmland": make_result("01", "耕地", 1.0, "农田", "exact"),
    "paddy": make_result("01", "耕地", 1.0, "水田", "exact"),
    "orchard": make_result("02", "园地", 1.0, "果园", "exact"),
    "vineyard": make_result("02", "园地", 1.0, "葡萄园", "exact"),
    "plant_nursery": make_result("02", "园地/苗圃用地", 0.9, "苗圃", "exact"),
    "forest": make_result("03", "林地", 1.0, "森林", "exact"),
    "nature_reserve": make_result("03", "自然保护/生态林地", 0.82, "生态保育空间", "exact"),
    "grass": make_result("04", "草地", 0.95, "草地", "exact"),
    "meadow": make_result("04", "草地", 0.95, "草甸", "exact"),
    "allotments": make_result("06", "农业设施/社区种植用地", 0.72, "小块耕作地", "exact"),
    "animal_breeding": make_result("06", "农业设施建设用地", 0.9, "养殖设施", "exact"),
    "aquaculture": make_result("06", "农业设施建设用地", 0.9, "水产养殖设施", "exact"),
    "farm": make_result("06", "农业设施建设用地", 0.82, "农场设施", "exact"),
    "farm_auxiliary": make_result("06", "农业设施建设用地", 0.82, "农场附属设施", "exact"),
    "farmyard": make_result("06", "农业设施建设用地", 0.9, "农院/农场场院", "exact"),
    "barn": make_result("06", "农业设施建设用地", 0.9, "谷仓/农用仓棚", "exact"),
    "cowshed": make_result("06", "农业设施建设用地", 0.9, "牛棚", "exact"),
    "stable": make_result("06", "农业设施建设用地", 0.88, "畜棚", "exact"),
    "greenhouse": make_result("06", "农业设施建设用地", 0.95, "温室", "exact"),
    "greenhouse_horticulture": make_result("06", "农业设施建设用地", 0.95, "园艺温室", "exact"),
    "drying_yard": make_result("06", "农业设施建设用地", 0.72, "晒场", "exact"),
    "park": make_result("14", "公园绿地", 1.0, "公园", "exact"),
    "garden": make_result("14", "绿地与开敞空间用地", 0.88, "花园绿地", "exact"),
    "greenery": make_result("14", "绿地与开敞空间用地", 0.8, "绿化空间", "exact"),
    "common": make_result("14", "开敞空间用地", 0.72, "公共开敞地", "exact"),
    "recreation_ground": make_result("14", "开敞活动场地", 0.85, "休闲活动场地", "exact"),
    "dog_park": make_result("14", "公园绿地", 0.82, "宠物活动公园", "exact"),
    "golf_course": make_result("14", "开敞活动场地", 0.82, "高尔夫场地", "exact"),
    "playground": make_result("14", "开敞活动场地", 0.88, "儿童活动场地", "exact"),
    "village_green": make_result("14", "开敞空间用地", 0.88, "村镇公共绿地", "exact"),
    "fishing": make_result("14", "开敞活动场地", 0.6, "休闲垂钓空间", "exact"),
    "flowerbed": make_result("14", "绿地附属景观设施", 0.45, "花坛，多为附属景观", "exact", False),
    "fountain": make_result("14", "开敞空间附属景观设施", 0.4, "喷泉，多为附属景观", "exact", False),
    "picnic_site": make_result("14", "开敞活动场地", 0.7, "野餐场地", "exact"),
    "picnic_table": make_result("14", "开敞空间附属设施", 0.35, "野餐桌，多为附属设施", "exact", False),
    "viewpoint": make_result("14", "开敞空间用地", 0.6, "观景点", "exact"),
    "beach_resort": make_result("09", "旅游服务用地", 0.9, "度假设施", "exact"),
    "horse_riding": make_result("14", "开敞活动场地", 0.7, "户外骑乘活动场地", "exact"),
    "greenfield": make_result("16", "留白/待开发用地", 0.95, "未开发地", "exact"),
    "brownfield": make_result("16", "留白/待开发用地", 0.95, "待再开发地块", "exact"),
    "construction": make_result("16", "施工/待开发用地", 0.85, "施工中地块，尚未稳定形成最终用地", "exact"),
    "salt": make_result("10", "工矿用地", 0.72, "盐业生产或堆场", "exact"),
    "salt_pond": make_result("10", "工矿用地", 0.78, "盐池/盐田生产空间", "exact"),
    "military": make_result("15", "军事设施用地", 1.0, "军事设施", "exact"),
    "prison": make_result("15", "特殊用地", 1.0, "监狱", "exact"),
    "cemetery": make_result("15", "殡葬用地", 1.0, "墓地", "exact"),
    "grave_yard": make_result("15", "殡葬用地", 0.98, "墓地", "exact"),
    "crematorium": make_result("15", "殡葬用地", 1.0, "火葬场", "exact"),
    "funeral_directors": make_result("15", "殡葬用地", 0.78, "殡仪服务设施", "exact"),
    "funeral_hall": make_result("15", "殡葬用地", 0.82, "殡仪馆", "exact"),
    "place_of_worship": make_result("15", "宗教用地", 1.0, "宗教场所", "exact"),
    "religious": make_result("15", "宗教用地", 1.0, "宗教设施", "exact"),
    "religional": make_result("15", "宗教用地", 0.9, "religious 拼写变体", "exact"),
    "church": make_result("15", "宗教用地", 1.0, "教堂", "exact"),
    "chapel": make_result("15", "宗教用地", 1.0, "礼拜堂", "exact"),
    "cathedral": make_result("15", "宗教用地", 1.0, "大教堂", "exact"),
    "temple": make_result("15", "宗教用地", 1.0, "寺庙", "exact"),
    "mosque": make_result("15", "宗教用地", 1.0, "清真寺", "exact"),
    "pagoda": make_result("15", "宗教/历史建筑用地", 0.82, "塔类宗教建筑", "exact"),
    "bunker": make_result("15", "特殊用地", 0.7, "地堡/防御设施", "exact"),
    "roof": make_result("00", "不建议直接判定", 0.0, "仅表示屋顶结构", "exact", False),
    "yes": make_result("00", "不建议直接判定", 0.0, "仅表示有建筑，不代表用地功能", "exact", False),
    "no": make_result("00", "不建议直接判定", 0.0, "否定性或无效语义", "exact", False),
    "part:yes": make_result("00", "不建议直接判定", 0.0, "建筑部件，不代表主体用地", "exact", False),
    "bench": make_result("00", "不建议直接判定", 0.0, "设施家具，不代表主体用地", "exact", False),
    "bbq": make_result("00", "不建议直接判定", 0.2, "附属休闲设施", "exact", False),
    "bandstand": make_result("00", "不建议直接判定", 0.2, "附属表演设施", "exact", False),
    "bird_hide": make_result("00", "不建议直接判定", 0.2, "观察设施，不代表主体用地", "exact", False),
    "bleachers": make_result("00", "不建议直接判定", 0.2, "看台，多为附属设施", "exact", False),
    "ceremonial_gate": make_result("00", "不建议直接判定", 0.2, "门类景观/附属设施", "exact", False),
    "clothes_dryer": make_result("00", "不建议直接判定", 0.0, "生活附属设施", "exact", False),
    "crane": make_result("00", "不建议直接判定", 0.2, "设备设施，不代表主体用地", "exact", False),
    "chimney": make_result("10", "工矿用地", 0.68, "烟囱多与工业设施相关", "exact"),
    "embankment": make_result("00", "不建议直接判定", 0.1, "工程边坡，不宜直接判地类", "exact", False),
    "gate": make_result("00", "不建议直接判定", 0.0, "入口设施", "exact", False),
    "gatehouse": make_result("00", "不建议直接判定", 0.2, "门卫房，多为附属设施", "exact", False),
    "grandstand": make_result("00", "不建议直接判定", 0.2, "看台，多为附属设施", "exact", False),
    "guide": make_result("09", "旅游服务用地", 0.5, "导览服务", "exact", False),
    "historic": make_result("00", "不建议直接判定", 0.2, "仅表示历史属性", "exact", False),
    "information": make_result("09", "旅游服务附属设施用地", 0.42, "旅游信息点，多为附属服务", "exact", False),
    "outdoor_seating": make_result("00", "不建议直接判定", 0.15, "室外座位，多为附属设施", "exact", False),
    "paved": make_result("00", "不建议直接判定", 0.0, "铺装属性，不代表用地功能", "exact", False),
    "pavilion": make_result("00", "不建议直接判定", 0.25, "亭类附属设施", "exact", False),
    "public": make_result("00", "不建议直接判定", 0.2, "public 过于宽泛", "exact", False),
    "ruins": make_result("00", "不建议直接判定", 0.1, "遗址属性，不宜直接判地类", "exact", False),
    "security": make_result("00", "不建议直接判定", 0.15, "安保点，多为附属设施", "exact", False),
    "shed": make_result("00", "不建议直接判定", 0.2, "棚舍，多为附属设施", "exact", False),
    "shelter": make_result("00", "不建议直接判定", 0.2, "遮蔽物，多为附属设施", "exact", False),
    "shower": make_result("00", "不建议直接判定", 0.15, "淋浴设施，多为附属设施", "exact", False),
    "smoking_area": make_result("00", "不建议直接判定", 0.1, "吸烟区，多为附属空间", "exact", False),
    "tower": make_result("00", "不建议直接判定", 0.2, "塔状构筑物，主体功能不清", "exact", False),
    "wall": make_result("00", "不建议直接判定", 0.0, "围墙构筑物", "exact", False),
    "florist": make_result("09", "商业用地", 0.8, "花店零售", "exact"),
    "clothes": make_result("09", "商业用地", 0.82, "服装零售", "exact"),
    "nightclub": make_result("09", "商业服务业用地", 0.82, "夜间娱乐服务", "exact"),
    "garage": make_result("07", "居住配套设施用地", 0.55, "garage 常为附属停车建筑", "exact", False),
    "service": make_result("09", "商业服务业用地", 0.38, "service 语义过宽，仅弱映射", "exact", False),
    "guardhouse": make_result("08", "公共安全/附属管理设施用地", 0.35, "门卫房，多为附属设施", "exact", False),
    "dyke": make_result("13", "公用设施用地", 0.68, "堤防工程", "exact"),
    "geoglyph": make_result("14", "开敞空间附属景观设施", 0.35, "地画景观，多为附属景观", "exact", False),
}


LEGACY_KEYWORD_RULES_UNUSED = []


def score_to_weight(score: float) -> float:
    return round(max(0.0, min(1.0, float(score))), 4)


def build_alt_candidate_from_profile(profile: dict[str, Any], weight: float, basis: str) -> dict[str, Any]:
    primary_code = profile["primary_code"]
    secondary_code = profile["secondary_code"]
    tertiary_code = profile["tertiary_code"]
    secondary_weight = round(weight * 0.94, 4) if secondary_code else ""
    tertiary_weight = round(weight * 0.9, 4) if tertiary_code else ""
    return {
        "primary_code": primary_code,
        "primary": PLANNING_PRIMARY.get(primary_code, ""),
        "secondary_code": secondary_code,
        "secondary": PLANNING_SECONDARY.get(secondary_code, "") if secondary_code else "",
        "tertiary_code": tertiary_code,
        "tertiary": PLANNING_TERTIARY.get(tertiary_code, "") if tertiary_code else "",
        "weight_l1": round(weight, 4),
        "weight_l2": secondary_weight,
        "weight_l3": tertiary_weight,
        "basis": basis,
    }


def choose_tertiary_candidate(tertiary_ranked: list[dict[str, Any]], secondary_weight: float) -> dict[str, Any] | None:
    if not tertiary_ranked:
        return None

    top = tertiary_ranked[0]
    top_weight = score_to_weight(top["score"])
    second_weight = score_to_weight(tertiary_ranked[1]["score"]) if len(tertiary_ranked) > 1 else 0.0
    margin = top_weight - second_weight
    if top_weight < SEMANTIC_TERTIARY_MIN_SCORE:
        return None
    if top_weight + 1e-6 < secondary_weight:
        return None
    if margin < SEMANTIC_TERTIARY_MARGIN and top_weight < max(secondary_weight + 0.05, 0.45):
        return None
    return top


def choose_global_tertiary_candidate(
    tertiary_ranked: list[dict[str, Any]],
    secondary_weight_map: dict[str, float],
) -> dict[str, Any] | None:
    if not tertiary_ranked:
        return None

    for index, candidate in enumerate(tertiary_ranked):
        candidate_weight = score_to_weight(candidate["score"])
        parent_secondary_weight = secondary_weight_map.get(candidate["secondary_code"], 0.0)
        next_weight = score_to_weight(tertiary_ranked[index + 1]["score"]) if index + 1 < len(tertiary_ranked) else 0.0
        margin = candidate_weight - next_weight

        if candidate_weight < SEMANTIC_TERTIARY_MIN_SCORE:
            continue
        if parent_secondary_weight > 0 and candidate_weight + 1e-6 < parent_secondary_weight:
            continue
        if margin < SEMANTIC_TERTIARY_MARGIN and candidate_weight < max(parent_secondary_weight + 0.05, 0.45):
            continue
        return candidate

    return None


def build_semantic_alternatives(
    query_text: str,
    chosen_primary: str,
    chosen_secondary: str,
    chosen_tertiary: str,
) -> str:
    engine = get_semantic_engine()
    alternatives: list[dict[str, Any]] = []
    seen: set[tuple[str, str, str]] = set()

    for candidate in engine.rank(query_text, "tertiary")[:8]:
        key = (candidate["primary_code"], candidate["secondary_code"], candidate["tertiary_code"])
        if key == (chosen_primary, chosen_secondary, chosen_tertiary) or key in seen:
            continue
        seen.add(key)
        alternatives.append(
            build_alt_candidate_from_profile(
                candidate,
                score_to_weight(candidate["score"]),
                "语义检索到相近的三级用地定义",
            )
        )
        if len(alternatives) >= 3:
            return json.dumps(alternatives, ensure_ascii=False)

    for candidate in engine.rank(query_text, "secondary")[:8]:
        key = (candidate["primary_code"], candidate["secondary_code"], "")
        if key == (chosen_primary, chosen_secondary, "") or key in seen:
            continue
        seen.add(key)
        alternatives.append(
            build_alt_candidate_from_profile(
                candidate,
                score_to_weight(candidate["score"]),
                "语义检索到相近的二级用地定义",
            )
        )
        if len(alternatives) >= 3:
            break

    return json.dumps(alternatives[:3], ensure_ascii=False) if alternatives else ""


def should_use_llm_fallback(semantic_result: dict[str, Any], config: LLMFallbackConfig | None) -> bool:
    if config is None:
        return False
    if str(semantic_result.get("matched_by", "")) == "legacy_prior":
        return False
    if semantic_result.get("matched_by") == "semantic_exclusion":
        return False
    if str(semantic_result.get("primary_code", "")) == "00":
        return True
    primary_score = float(semantic_result.get("primary_score", semantic_result.get("confidence", 0.0)) or 0.0)
    return primary_score <= float(config.trigger_primary_max)


def choose_best_secondary_within_primary(
    engine: SemanticSimilarityEngine,
    query_text: str,
    primary_code: str,
) -> tuple[str, float | str]:
    ranked = engine.rank(query_text, "secondary", prefix=primary_code)
    if not ranked:
        return "", ""
    best = ranked[0]
    weight = score_to_weight(best["score"])
    if weight < SEMANTIC_SECONDARY_MIN_SCORE:
        return "", ""
    return best["code"], weight


def choose_best_tertiary_within_secondary(
    engine: SemanticSimilarityEngine,
    query_text: str,
    secondary_code: str,
    secondary_weight: float | str,
) -> tuple[str, float | str]:
    if not secondary_code or secondary_weight == "":
        return "", ""
    ranked = engine.rank(query_text, "tertiary", prefix=secondary_code)
    best = choose_tertiary_candidate(ranked, float(secondary_weight))
    if not best:
        return "", ""
    return best["code"], score_to_weight(best["score"])


def is_valid_secondary_code(primary_code: str, secondary_code: str) -> bool:
    return bool(primary_code and secondary_code and secondary_code in PLANNING_SECONDARY and secondary_code[:2] == primary_code)


def is_valid_tertiary_code(secondary_code: str, tertiary_code: str) -> bool:
    return bool(secondary_code and tertiary_code and tertiary_code in PLANNING_TERTIARY and tertiary_code[:4] == secondary_code)


def clamp_score(value: float | str) -> float | str:
    if value == "" or value is None:
        return ""
    try:
        score = float(value)
    except (TypeError, ValueError):
        return value
    return round(max(0.0, min(1.0, score)), 4)


def apply_planning_detail_refinement(tag_value: str, result: dict[str, Any], force_inferred: bool = False) -> dict[str, Any]:
    primary_code = str(result.get("primary_code", "")).zfill(2)
    if primary_code not in PLANNING_PRIMARY:
        return result
    if primary_code == "00":
        result["secondary_code"] = "0000"
        result["secondary_name"] = PLANNING_SECONDARY["0000"]
        result["tertiary_code"] = ""
        result["tertiary_name"] = ""
        result["secondary_score"] = result.get("secondary_score", "")
        result["tertiary_score"] = ""
        result["landuse_hint"] = PLANNING_SECONDARY["0000"]
        return result

    detail = infer_landuse_detail(
        tag_value,
        {
            "primary_code": primary_code,
            "landuse_hint": PLANNING_PRIMARY.get(primary_code, ""),
        },
    )

    current_secondary = str(result.get("secondary_code", "")).strip()
    current_tertiary = str(result.get("tertiary_code", "")).strip()
    inferred_secondary = str(detail.get("secondary_code", "")).strip()
    inferred_tertiary = str(detail.get("tertiary_code", "")).strip()

    secondary_code = current_secondary if is_valid_secondary_code(primary_code, current_secondary) else ""
    tertiary_code = current_tertiary if is_valid_tertiary_code(secondary_code, current_tertiary) else ""

    if force_inferred and is_valid_secondary_code(primary_code, inferred_secondary):
        secondary_code = inferred_secondary
        if is_valid_tertiary_code(secondary_code, inferred_tertiary):
            tertiary_code = inferred_tertiary
        elif not is_valid_tertiary_code(secondary_code, tertiary_code):
            tertiary_code = ""
    elif is_valid_secondary_code(primary_code, inferred_secondary):
        if not secondary_code:
            secondary_code = inferred_secondary
        if not tertiary_code and secondary_code == inferred_secondary and is_valid_tertiary_code(secondary_code, inferred_tertiary):
            tertiary_code = inferred_tertiary

    primary_score = clamp_score(result.get("primary_score", result.get("confidence", "")))
    if primary_score == "":
        primary_score = clamp_score(result.get("confidence", 0.0))

    current_secondary_score = result.get("secondary_score", "")
    current_tertiary_score = result.get("tertiary_score", "")

    if secondary_code:
        if secondary_code != current_secondary or current_secondary_score == "":
            if isinstance(primary_score, float):
                current_secondary_score = clamp_score(primary_score * float(detail.get("secondary_factor", 0.0)))
            else:
                current_secondary_score = clamp_score(detail.get("secondary_factor", ""))
    else:
        current_secondary_score = ""

    if tertiary_code:
        if tertiary_code != current_tertiary or current_tertiary_score == "":
            if isinstance(primary_score, float):
                current_tertiary_score = clamp_score(primary_score * float(detail.get("tertiary_factor", 0.0)))
            else:
                current_tertiary_score = clamp_score(detail.get("tertiary_factor", ""))
    else:
        current_tertiary_score = ""

    result["secondary_code"] = secondary_code
    result["secondary_name"] = PLANNING_SECONDARY.get(secondary_code, "")
    result["tertiary_code"] = tertiary_code
    result["tertiary_name"] = PLANNING_TERTIARY.get(tertiary_code, "")
    result["secondary_score"] = current_secondary_score
    result["tertiary_score"] = current_tertiary_score

    if tertiary_code:
        result["landuse_hint"] = " / ".join(
            [
                PLANNING_SECONDARY[secondary_code],
                PLANNING_TERTIARY[tertiary_code],
            ]
        )
    elif secondary_code:
        result["landuse_hint"] = PLANNING_SECONDARY[secondary_code]
    else:
        result["landuse_hint"] = PLANNING_PRIMARY[primary_code]

    return result


def apply_legacy_prior_override(
    tag_value: str,
    source_categories: str,
    query_text: str,
    semantic_result: dict[str, Any],
) -> dict[str, Any]:
    prior = load_legacy_priors().get(normalize_tag(tag_value))
    if prior is None:
        return semantic_result

    semantic_primary = str(semantic_result.get("primary_code", "")).zfill(2)
    if semantic_primary == prior.primary_code:
        return semantic_result

    engine = get_semantic_engine()
    primary_weight = prior.confidence
    secondary_code = prior.secondary_code
    secondary_weight: float | str = ""
    tertiary_code = ""
    tertiary_weight: float | str = ""

    if secondary_code:
        secondary_ranked = engine.rank(query_text, "secondary", prefix=prior.primary_code)
        secondary_map = {item["code"]: score_to_weight(item["score"]) for item in secondary_ranked}
        secondary_weight = secondary_map.get(secondary_code, round(max(0.75, prior.confidence - 0.05), 4))
    else:
        secondary_code, secondary_weight = choose_best_secondary_within_primary(engine, query_text, prior.primary_code)

    tertiary_code, tertiary_weight = choose_best_tertiary_within_secondary(engine, query_text, secondary_code, secondary_weight)

    landuse_hint = (
        " / ".join(part for part in [PLANNING_SECONDARY.get(secondary_code, ""), PLANNING_TERTIARY.get(tertiary_code, "")] if part)
        if tertiary_code
        else PLANNING_SECONDARY.get(secondary_code) or PLANNING_PRIMARY[prior.primary_code]
    )
    rationale = (
        f"legacy prior override from {prior.source}: tag `{normalize_tag(tag_value)}` "
        f"strongly maps to {prior.primary_code} {PLANNING_PRIMARY[prior.primary_code]}"
    )
    result = make_result(
        prior.primary_code,
        landuse_hint,
        float(primary_weight),
        rationale,
        "legacy_prior",
        True,
    )
    result.update(
        {
            "secondary_code": secondary_code,
            "secondary_name": PLANNING_SECONDARY.get(secondary_code, ""),
            "tertiary_code": tertiary_code,
            "tertiary_name": PLANNING_TERTIARY.get(tertiary_code, ""),
            "primary_score": round(float(primary_weight), 4),
            "secondary_score": secondary_weight,
            "tertiary_score": tertiary_weight,
            "alt_candidates_json": build_semantic_alternatives(query_text, prior.primary_code, secondary_code, tertiary_code),
            "semantic_backend": semantic_result.get("semantic_backend", ""),
        }
    )
    return apply_planning_detail_refinement(tag_value, result, force_inferred=True)


def resolve_llm_model(config: LLMFallbackConfig) -> str:
    if config.model:
        return config.model

    response = requests.get(
        f"{normalize_base_url(config.base_url)}/models",
        headers={"Authorization": f"Bearer {config.api_key}"},
        timeout=config.timeout,
    )
    response.raise_for_status()
    data = response.json().get("data", [])
    for item in data:
        model_id = str(item.get("id", "")).strip()
        if model_id.startswith("gpt-5"):
            config.model = model_id
            return model_id
    if data:
        config.model = str(data[0].get("id", "")).strip()
        return config.model
    raise RuntimeError("No model available from LLM endpoint")


def build_llm_candidate_context(tag_value: str, source_categories: str, semantic_result: dict[str, Any]) -> str:
    query_text = build_query_text(tag_value, source_categories)
    engine = get_semantic_engine()
    definitions = parse_category_definitions()

    lines = [
        f"tag_value: {tag_value}",
        f"source_categories: {source_categories}",
        "semantic_current:",
        json.dumps(
            {
                "primary_code": semantic_result.get("primary_code", ""),
                "secondary_code": semantic_result.get("secondary_code", ""),
                "tertiary_code": semantic_result.get("tertiary_code", ""),
                "primary_score": semantic_result.get("primary_score", ""),
                "secondary_score": semantic_result.get("secondary_score", ""),
                "tertiary_score": semantic_result.get("tertiary_score", ""),
            },
            ensure_ascii=False,
        ),
        "candidate_categories:",
    ]

    seen: set[str] = set()
    for level, limit in (("primary", 4), ("secondary", 6), ("tertiary", 8)):
        for item in engine.rank(query_text, level)[:limit]:
            code = item["code"]
            if code in seen:
                continue
            seen.add(code)
            definition = definitions.get(code, "")
            lines.append(f"- {level} {code} {item['name']} | score={score_to_weight(item['score'])} | {definition}")

    return "\n".join(lines)


def extract_json_object(text: str) -> dict[str, Any]:
    stripped = text.strip()
    if stripped.startswith("```"):
        stripped = re.sub(r"^```(?:json)?\s*", "", stripped)
        stripped = re.sub(r"\s*```$", "", stripped)
    match = re.search(r"\{.*\}", stripped, flags=re.DOTALL)
    candidate = match.group(0) if match else stripped
    data = json.loads(candidate)
    if not isinstance(data, dict):
        raise ValueError("LLM response JSON is not an object")
    return data


def request_llm_landuse_classification(
    config: LLMFallbackConfig,
    value: str,
    source_categories: str,
    semantic_result: dict[str, Any],
) -> dict[str, Any]:
    model = resolve_llm_model(config)
    cache_key = build_llm_cache_key(value, source_categories, model)
    cached = config.cache.get(cache_key)
    if isinstance(cached, dict):
        return cached

    taxonomy = {
        "primary": PLANNING_PRIMARY,
        "secondary": PLANNING_SECONDARY,
        "tertiary": PLANNING_TERTIARY,
    }
    candidate_context = build_llm_candidate_context(value, source_categories, semantic_result)
    payload = {
        "model": model,
        "temperature": 0,
        "messages": [
            {
                "role": "system",
                "content": (
                    "你是城市规划与地块用地分类专家。"
                    "请依据 tag_value 语义、source_categories、国土空间分类候选，"
                    "输出严格 JSON，不要输出 markdown。"
                ),
            },
            {
                "role": "user",
                "content": (
                    "任务：对一个 tag_value 进行用地分类。\n"
                    "要求：\n"
                    "1. 只能输出一个 JSON 对象。\n"
                    "2. 字段必须包含 primary_code, secondary_code, tertiary_code, weight_l1, weight_l2, weight_l3, alt_candidates_json, comment。\n"
                    "3. code 必须来自给定 taxonomy；如果无法可靠判断，可输出 primary_code='00'。\n"
                    "4. weight 取 0 到 1 之间；没有对应层级可用空字符串。\n"
                    "5. alt_candidates_json 输出数组，每项含 primary_code, secondary_code, tertiary_code, weight_l1, basis。\n\n"
                    f"taxonomy={json.dumps(taxonomy, ensure_ascii=False)}\n\n"
                    f"{candidate_context}"
                ),
            },
        ],
    }

    response = requests.post(
        f"{normalize_base_url(config.base_url)}/chat/completions",
        headers={
            "Authorization": f"Bearer {config.api_key}",
            "Content-Type": "application/json",
        },
        json=payload,
        timeout=config.timeout,
    )
    response.raise_for_status()
    data = response.json()
    content = data["choices"][0]["message"]["content"]
    parsed = extract_json_object(content)
    config.cache[cache_key] = parsed
    save_llm_cache(config.cache_path, config.cache)
    return parsed


def apply_llm_result(tag_value: str, semantic_result: dict[str, Any], llm_result: dict[str, Any]) -> dict[str, Any]:
    primary_code = str(llm_result.get("primary_code", "")).strip() or semantic_result["primary_code"]
    secondary_code = str(llm_result.get("secondary_code", "")).strip()
    tertiary_code = str(llm_result.get("tertiary_code", "")).strip()

    if primary_code not in PLANNING_PRIMARY:
        return semantic_result
    if secondary_code and secondary_code not in PLANNING_SECONDARY:
        secondary_code = ""
    if tertiary_code and tertiary_code not in PLANNING_TERTIARY:
        tertiary_code = ""
    if tertiary_code and tertiary_code[:4] != secondary_code:
        tertiary_code = ""
    if secondary_code and secondary_code[:2] != primary_code:
        secondary_code = ""
        tertiary_code = ""

    weight_l1 = llm_result.get("weight_l1", semantic_result.get("primary_score", semantic_result.get("confidence", 0.0)))
    weight_l2 = llm_result.get("weight_l2", "")
    weight_l3 = llm_result.get("weight_l3", "")
    alt_candidates = llm_result.get("alt_candidates_json", [])
    if isinstance(alt_candidates, str):
        alt_candidates_json = alt_candidates
    else:
        alt_candidates_json = json.dumps(alt_candidates, ensure_ascii=False)

    landuse_hint = (
        " / ".join(part for part in [PLANNING_SECONDARY.get(secondary_code, ""), PLANNING_TERTIARY.get(tertiary_code, "")] if part)
        if tertiary_code
        else PLANNING_SECONDARY.get(secondary_code) or PLANNING_PRIMARY[primary_code]
    )
    result = make_result(
        primary_code,
        landuse_hint,
        float(weight_l1 or 0.0),
        str(llm_result.get("comment", "LLM fallback classification")),
        "llm_openai_fallback",
        primary_code != "00",
    )
    result.update(
        {
            "secondary_code": secondary_code,
            "secondary_name": PLANNING_SECONDARY.get(secondary_code, ""),
            "tertiary_code": tertiary_code,
            "tertiary_name": PLANNING_TERTIARY.get(tertiary_code, ""),
            "primary_score": weight_l1,
            "secondary_score": weight_l2,
            "tertiary_score": weight_l3,
            "alt_candidates_json": alt_candidates_json,
            "semantic_backend": semantic_result.get("semantic_backend", ""),
        }
    )
    return apply_planning_detail_refinement(tag_value, result, force_inferred=False)


def classify_tag_legacy_unused(value: str, source_categories: str) -> dict[str, Any]:
    tag = normalize_tag(value)
    if tag in NON_CLASSIFIABLE_TAGS:
        rationale = NON_CLASSIFIABLE_HINTS.get(tag, "该标签更像构筑物、属性值或附属设施，不适合直接判定主体用地")
        result = make_result("00", "未映射/不建议直接判定", 0.0, rationale, "semantic_exclusion", False)
        result.update(
            {
                "secondary_code": "0000",
                "secondary_name": PLANNING_SECONDARY["0000"],
                "tertiary_code": "",
                "tertiary_name": "",
                "primary_score": 0.0,
                "secondary_score": "",
                "tertiary_score": "",
                "alt_candidates_json": "",
                "semantic_backend": "exclusion",
            }
        )
        return result

    query_text = build_query_text(value, source_categories)
    engine = get_semantic_engine()
    primary_ranked = engine.rank(query_text, "primary")
    if not primary_ranked:
        result = make_result("00", "未映射/需人工复核", 0.0, "语义检索未返回可用候选", f"{engine.backend}:empty", False)
        result.update(
            {
                "secondary_code": "0000",
                "secondary_name": PLANNING_SECONDARY["0000"],
                "tertiary_code": "",
                "tertiary_name": "",
                "primary_score": 0.0,
                "secondary_score": "",
                "tertiary_score": "",
                "alt_candidates_json": "",
                "semantic_backend": engine.backend,
            }
        )
        return result

    best_primary = primary_ranked[0]
    primary_weight = score_to_weight(best_primary["score"])
    if primary_weight < SEMANTIC_PRIMARY_MIN_SCORE:
        result = make_result("00", "未映射/需人工复核", primary_weight, "语义匹配置信度过低，建议人工复核", engine.backend, False)
        result.update(
            {
                "secondary_code": "0000",
                "secondary_name": PLANNING_SECONDARY["0000"],
                "tertiary_code": "",
                "tertiary_name": "",
                "primary_score": primary_weight,
                "secondary_score": "",
                "tertiary_score": "",
                "alt_candidates_json": build_semantic_alternatives(query_text, "", "", ""),
                "semantic_backend": engine.backend,
            }
        )
        return result

    secondary_ranked = engine.rank(query_text, "secondary", prefix=best_primary["code"])
    best_secondary = secondary_ranked[0] if secondary_ranked else None
    secondary_weight = score_to_weight(best_secondary["score"]) if best_secondary else ""
    if best_secondary and secondary_weight < SEMANTIC_SECONDARY_MIN_SCORE:
        best_secondary = None
        secondary_weight = ""

    best_tertiary = None
    tertiary_weight: float | str = ""
    if best_secondary:
        tertiary_ranked = engine.rank(query_text, "tertiary", prefix=best_secondary["code"])
        best_tertiary = choose_tertiary_candidate(tertiary_ranked, float(secondary_weight))
        if best_tertiary:
            tertiary_weight = score_to_weight(best_tertiary["score"])

    primary_code = best_primary["code"]
    secondary_code = best_secondary["code"] if best_secondary else ""
    tertiary_code = best_tertiary["code"] if best_tertiary else ""
    landuse_hint = (
        PLANNING_TERTIARY.get(tertiary_code)
        or PLANNING_SECONDARY.get(secondary_code)
        or PLANNING_PRIMARY[primary_code]
    )
    lowest_weight = min(
        weight
        for weight in [primary_weight, secondary_weight if secondary_weight != "" else primary_weight, tertiary_weight if tertiary_weight != "" else primary_weight]
        if weight != ""
    )
    alt_candidates_json = ""
    if lowest_weight < LOW_CONFIDENCE_THRESHOLD:
        alt_candidates_json = build_semantic_alternatives(query_text, primary_code, secondary_code, tertiary_code)

    rationale_parts = [
        f"语义匹配一级类 {primary_code} {PLANNING_PRIMARY[primary_code]} (score={primary_weight})",
    ]
    if secondary_code:
        rationale_parts.append(f"二级类 {secondary_code} {PLANNING_SECONDARY[secondary_code]} (score={secondary_weight})")
    if tertiary_code:
        rationale_parts.append(f"三级类 {tertiary_code} {PLANNING_TERTIARY[tertiary_code]} (score={tertiary_weight})")

    result = make_result(primary_code, landuse_hint, primary_weight, "；".join(rationale_parts), engine.backend, True)
    result.update(
        {
            "secondary_code": secondary_code,
            "secondary_name": PLANNING_SECONDARY.get(secondary_code, ""),
            "tertiary_code": tertiary_code,
            "tertiary_name": PLANNING_TERTIARY.get(tertiary_code, ""),
            "primary_score": primary_weight,
            "secondary_score": secondary_weight,
            "tertiary_score": tertiary_weight,
            "alt_candidates_json": alt_candidates_json,
            "semantic_backend": engine.backend,
        }
    )
    return result


def classify_tag(value: str, source_categories: str, llm_config: LLMFallbackConfig | None = None) -> dict[str, Any]:
    tag = normalize_tag(value)
    if tag in NON_CLASSIFIABLE_TAGS:
        rationale = NON_CLASSIFIABLE_HINTS.get(tag, "该标签更像构筑物、属性值或附属设施，不适合直接判定主体用地")
        result = make_result("00", "未映射，不建议直接判定", 0.0, rationale, "semantic_exclusion", False)
        result.update(
            {
                "secondary_code": "0000",
                "secondary_name": PLANNING_SECONDARY["0000"],
                "tertiary_code": "",
                "tertiary_name": "",
                "primary_score": 0.0,
                "secondary_score": "",
                "tertiary_score": "",
                "alt_candidates_json": "",
                "semantic_backend": "exclusion",
            }
        )
        return result

    query_text = build_query_text(value, source_categories)
    engine = get_semantic_engine()
    primary_ranked = engine.rank(query_text, "primary")
    if not primary_ranked:
        result = make_result("00", "未映射，需人工复核", 0.0, "语义检索未返回可用候选", f"{engine.backend}:empty", False)
        result.update(
            {
                "secondary_code": "0000",
                "secondary_name": PLANNING_SECONDARY["0000"],
                "tertiary_code": "",
                "tertiary_name": "",
                "primary_score": 0.0,
                "secondary_score": "",
                "tertiary_score": "",
                "alt_candidates_json": "",
                "semantic_backend": engine.backend,
            }
        )
        return result

    primary_weight_map = {candidate["code"]: score_to_weight(candidate["score"]) for candidate in primary_ranked}
    secondary_ranked = engine.rank(query_text, "secondary")
    secondary_weight_map = {candidate["code"]: score_to_weight(candidate["score"]) for candidate in secondary_ranked}
    tertiary_ranked = engine.rank(query_text, "tertiary")

    best_primary = primary_ranked[0]
    best_primary_weight = primary_weight_map[best_primary["code"]]
    if best_primary_weight < SEMANTIC_PRIMARY_MIN_SCORE:
        result = make_result("00", "未映射，需人工复核", best_primary_weight, "语义匹配置低于主分类阈值", engine.backend, False)
        result.update(
            {
                "secondary_code": "0000",
                "secondary_name": PLANNING_SECONDARY["0000"],
                "tertiary_code": "",
                "tertiary_name": "",
                "primary_score": best_primary_weight,
                "secondary_score": "",
                "tertiary_score": "",
                "alt_candidates_json": build_semantic_alternatives(query_text, "", "", ""),
                "semantic_backend": engine.backend,
            }
        )
        return result

    best_secondary = secondary_ranked[0] if secondary_ranked else None
    best_tertiary = choose_global_tertiary_candidate(tertiary_ranked, secondary_weight_map)

    primary_code = best_primary["code"]
    secondary_code = ""
    tertiary_code = ""
    primary_weight = best_primary_weight
    secondary_weight: float | str = ""
    tertiary_weight: float | str = ""

    if best_tertiary:
        primary_code = best_tertiary["primary_code"]
        secondary_code = best_tertiary["secondary_code"]
        tertiary_code = best_tertiary["code"]
        primary_weight = primary_weight_map.get(primary_code, best_primary_weight)
        secondary_weight = secondary_weight_map.get(secondary_code, "")
        tertiary_weight = score_to_weight(best_tertiary["score"])
    elif best_secondary:
        candidate_secondary_weight = secondary_weight_map[best_secondary["code"]]
        if candidate_secondary_weight >= SEMANTIC_SECONDARY_MIN_SCORE:
            primary_code = best_secondary["primary_code"]
            secondary_code = best_secondary["code"]
            primary_weight = primary_weight_map.get(primary_code, best_primary_weight)
            secondary_weight = candidate_secondary_weight

    if tertiary_code:
        landuse_hint = " / ".join(
            part
            for part in [
                PLANNING_SECONDARY.get(secondary_code, ""),
                PLANNING_TERTIARY.get(tertiary_code, ""),
            ]
            if part
        )
    else:
        landuse_hint = PLANNING_SECONDARY.get(secondary_code) or PLANNING_PRIMARY[primary_code]

    lowest_weight = min(
        weight
        for weight in [
            primary_weight,
            secondary_weight if secondary_weight != "" else primary_weight,
            tertiary_weight if tertiary_weight != "" else primary_weight,
        ]
        if weight != ""
    )
    alt_candidates_json = ""
    if lowest_weight < LOW_CONFIDENCE_THRESHOLD:
        alt_candidates_json = build_semantic_alternatives(query_text, primary_code, secondary_code, tertiary_code)

    rationale_parts = [
        f"语义匹配一级类 {primary_code} {PLANNING_PRIMARY[primary_code]} (score={primary_weight})",
    ]
    if secondary_code:
        rationale_parts.append(f"二级类 {secondary_code} {PLANNING_SECONDARY[secondary_code]} (score={secondary_weight})")
    if tertiary_code:
        rationale_parts.append(f"三级类 {tertiary_code} {PLANNING_TERTIARY[tertiary_code]} (score={tertiary_weight})")

    result = make_result(primary_code, landuse_hint, primary_weight, "；".join(rationale_parts), engine.backend, True)
    result.update(
        {
            "secondary_code": secondary_code,
            "secondary_name": PLANNING_SECONDARY.get(secondary_code, ""),
            "tertiary_code": tertiary_code,
            "tertiary_name": PLANNING_TERTIARY.get(tertiary_code, ""),
            "primary_score": primary_weight,
            "secondary_score": secondary_weight,
            "tertiary_score": tertiary_weight,
            "alt_candidates_json": alt_candidates_json,
            "semantic_backend": engine.backend,
        }
    )

    result = apply_legacy_prior_override(value, source_categories, query_text, result)

    if should_use_llm_fallback(result, llm_config):
        try:
            llm_result = request_llm_landuse_classification(llm_config, value, source_categories, result)
            return apply_llm_result(value, result, llm_result)
        except Exception as exc:
            result["rationale"] = f"{result['rationale']} | llm_fallback_failed: {type(exc).__name__}: {exc}"

    return result


def fallback_output_path(path: pathlib.Path) -> pathlib.Path:
    timestamp = dt.datetime.now().strftime("%Y%m%d_%H%M%S")
    return path.with_name(f"{path.stem}_{timestamp}{path.suffix}")


def write_csv_with_fallback(df: pd.DataFrame, path: pathlib.Path) -> pathlib.Path:
    try:
        df.to_csv(path, index=False, encoding="utf-8-sig")
        return path
    except PermissionError:
        alt_path = fallback_output_path(path)
        df.to_csv(alt_path, index=False, encoding="utf-8-sig")
        return alt_path


def write_text_with_fallback(path: pathlib.Path, content: str, encoding: str = "utf-8") -> pathlib.Path:
    try:
        path.write_text(content, encoding=encoding)
        return path
    except PermissionError:
        alt_path = fallback_output_path(path)
        alt_path.write_text(content, encoding=encoding)
        return alt_path


def write_xlsx_with_fallback(export_df: pd.DataFrame, weak_only: pd.DataFrame, path: pathlib.Path) -> pathlib.Path:
    candidates = [path, fallback_output_path(path)]
    for candidate in candidates:
        try:
            with pd.ExcelWriter(candidate, engine="openpyxl") as writer:
                export_df.to_excel(writer, sheet_name="tag_mapping", index=False)
                weak_only.to_excel(writer, sheet_name="weak_review", index=False)
            return candidate
        except PermissionError:
            continue
    raise PermissionError(f"Unable to write xlsx output: {path}")


def classify_dataframe(df: pd.DataFrame, llm_config: LLMFallbackConfig | None = None) -> pd.DataFrame:
    rows: list[dict[str, Any]] = []
    for _, row in df.iterrows():
        result = classify_tag(row["value"], row.get("source_categories", ""), llm_config=llm_config)
        out = row.to_dict()
        out.update(result)
        rows.append(out)
    return pd.DataFrame(rows)


def has_any(text: str, keywords: tuple[str, ...]) -> bool:
    return any(keyword in text for keyword in keywords)


def make_detail(
    secondary_code: str = "",
    tertiary_code: str = "",
    secondary_factor: float = 0.95,
    tertiary_factor: float = 0.9,
) -> dict[str, Any]:
    return {
        "secondary_code": secondary_code,
        "secondary": PLANNING_SECONDARY.get(secondary_code, "") if secondary_code else "",
        "tertiary_code": tertiary_code,
        "tertiary": PLANNING_TERTIARY.get(tertiary_code, "") if tertiary_code else "",
        "secondary_factor": secondary_factor if secondary_code else 0.0,
        "tertiary_factor": tertiary_factor if tertiary_code else 0.0,
    }


def infer_landuse_detail(tag_value: str, row: pd.Series | dict[str, Any]) -> dict[str, Any]:
    tag = normalize_tag(tag_value)
    primary_code = str(row.get("primary_code", "00"))
    hint = str(row.get("landuse_hint", ""))
    text = f"{tag} {hint}".lower()

    if primary_code == "00":
        return make_detail("0000", "", 1.0, 0.0)

    if primary_code == "01":
        if tag == "paddy" or "水田" in hint:
            return make_detail("0101", "", 1.0, 0.0)
        return make_detail("0103", "", 0.92, 0.0)

    if primary_code == "02":
        if tag == "orchard" or "果园" in hint:
            return make_detail("0201", "", 1.0, 0.0)
        if "茶" in hint or tag == "tea_garden":
            return make_detail("0202", "", 1.0, 0.0)
        return make_detail("0205", "", 0.9, 0.0)

    if primary_code == "03":
        if "竹" in hint or "bamboo" in tag:
            return make_detail("0302", "", 0.9, 0.0)
        if "灌木" in hint or "shrub" in tag:
            return make_detail("0303", "", 0.9, 0.0)
        if tag in {"forest", "nature_reserve"} or "森林" in hint or "林地" in hint:
            return make_detail("0301", "", 0.95, 0.0)
        return make_detail("0304", "", 0.85, 0.0)

    if primary_code == "04":
        if tag == "meadow" or "天然" in hint:
            return make_detail("0401", "", 0.95, 0.0)
        if tag == "grass" or "人工" in hint:
            return make_detail("0402", "", 0.9, 0.0)
        return make_detail("0403", "", 0.85, 0.0)

    if primary_code == "06":
        if has_any(text, ("aquaculture", "水产")):
            return make_detail("0602", "060203", 0.95, 0.95)
        if has_any(text, ("animal_breeding", "cowshed", "stable", "畜", "养殖", "牛棚")):
            return make_detail("0602", "060202", 0.95, 0.92)
        if has_any(text, ("greenhouse", "plant", "farm", "barn", "drying", "温室", "种植", "农场", "谷仓", "晒场")):
            return make_detail("0602", "060201", 0.95, 0.88)
        return make_detail("0602", "", 0.85, 0.0)

    if primary_code == "07":
        if has_any(text, ("garages", "garage", "carport", "outbuilding", "配套", "社区服务")):
            return make_detail("0702", "", 0.82, 0.0)
        if has_any(text, ("detached", "semidetached", "house", "bungalow", "chalet", "独立", "低层")):
            return make_detail("0701", "070101", 0.95, 0.7)
        if has_any(text, ("apartment", "apartments", "dormitory", "公寓", "宿舍")):
            return make_detail("0701", "070102", 0.95, 0.72)
        return make_detail("0701", "", 0.9, 0.0)

    if primary_code == "08":
        if has_any(text, ("research", "科研", "创新")):
            return make_detail("0802", "", 0.92, 0.0)
        if has_any(text, ("library", "museum", "gallery", "archive", "planetarium", "图书", "博物", "展览", "档案")):
            return make_detail("0803", "080301", 0.95, 0.9)
        if has_any(text, ("theatre", "cinema", "arts", "art", "concert", "studio", "文化", "剧院", "艺术", "音乐")):
            return make_detail("0803", "080302", 0.95, 0.86)
        if has_any(text, ("university", "college", "高等")):
            return make_detail("0804", "080401", 0.98, 0.98)
        if has_any(text, ("kindergarten", "幼儿园")):
            return make_detail("0804", "080404", 0.98, 0.98)
        if has_any(text, ("school", "中小学", "学校")):
            return make_detail("0804", "080403", 0.96, 0.88)
        if has_any(text, ("education", "prep_school", "language_school", "music_school", "教育")):
            return make_detail("0804", "080405", 0.92, 0.82)
        if has_any(text, ("sports_centre", "sports_hall", "stadium", "swimming_pool", "fitness", "pitch", "track", "ice_rink", "体育", "运动", "游泳")):
            return make_detail("0805", "080501", 0.95, 0.88)
        if tag == "hospital" or "医院" in hint:
            return make_detail("0806", "080601", 0.98, 0.98)
        if has_any(text, ("clinic", "doctors", "dentist", "gynaeology", "postpartum", "诊所", "医疗", "护理")):
            return make_detail("0806", "080602", 0.95, 0.86)
        if has_any(text, ("nursing_home", "养老")):
            return make_detail("0807", "080701", 0.95, 0.92)
        if has_any(text, ("childcare", "托育", "儿童")):
            return make_detail("0807", "080702", 0.9, 0.82)
        if has_any(text, ("social", "charity", "welfare", "福利", "慈善")):
            return make_detail("0807", "080704", 0.9, 0.78)
        if has_any(text, ("government", "governmental", "govermental", "civic", "townhall", "courthouse", "embassy", "diplomatic", "police", "party", "union", "ngo", "机关", "政府", "行政", "法院", "使馆", "警务")):
            return make_detail("0801", "", 0.95, 0.0)
        return make_detail("0801", "", 0.75, 0.0)

    if primary_code == "09":
        if has_any(text, ("marketplace", "wholesale", "批发", "市场")):
            return make_detail("0901", "090102", 0.95, 0.9)
        if has_any(text, ("restaurant", "fast_food", "food_court", "cafe", "bar", "pub", "ice_cream", "餐饮", "咖啡", "酒吧", "饭店")):
            return make_detail("0901", "090103", 0.98, 0.95)
        if has_any(text, ("hotel", "hostel", "guest_house", "motel", "resort", "camp_site", "caravan_site", "旅馆", "酒店", "住宿", "度假")):
            return make_detail("0901", "090104", 0.98, 0.95)
        if has_any(text, ("retail", "mall", "department_store", "supermarket", "convenience", "store", "books", "electronics", "furniture", "gift", "greengrocer", "houseware", "kiosk", "musical_instrument", "newspaper", "optician", "pharmacy", "tea", "tobacco", "wine", "florist", "clothes", "商业", "零售", "商场", "超市", "店")):
            return make_detail("0901", "090101", 0.96, 0.9)
        if has_any(text, ("office", "bank", "company", "conference", "coworking", "exhibition", "events_venue", "it", "property_management", "商务", "办公", "金融", "会议", "展览")):
            return make_detail("0902", "", 0.94, 0.0)
        if has_any(text, ("casino", "nightclub", "theme_park", "water_park", "aquarium", "zoo", "attraction", "娱乐", "游乐", "景点")):
            return make_detail("0903", "", 0.86, 0.0)
        if has_any(text, ("car", "repair", "beauty", "laundry", "service", "wash", "pet", "维修", "美容", "洗车", "洗浴")):
            return make_detail("0904", "", 0.82, 0.0)
        return make_detail("0901", "", 0.8, 0.0)

    if primary_code == "10":
        if has_any(text, ("quarry", "采石", "采矿")):
            return make_detail("1002", "", 0.96, 0.0)
        if has_any(text, ("salt", "盐")):
            return make_detail("1003", "", 0.96, 0.0)
        if has_any(text, ("kiln", "chimney", "coal", "petroleum", "窑", "烟囱", "煤")):
            return make_detail("1001", "100103", 0.92, 0.72)
        if has_any(text, ("manufacture", "works", "制造", "工厂")):
            return make_detail("1001", "100102", 0.95, 0.78)
        return make_detail("1001", "", 0.9, 0.0)

    if primary_code == "11":
        if has_any(text, ("storage_tank", "petroleum", "hazard", "储罐", "危险")):
            return make_detail("1101", "110103", 0.92, 0.72)
        if has_any(text, ("warehouse", "logistics", "depot", "silo", "仓储", "物流", "筒仓")):
            return make_detail("1101", "110101", 0.95, 0.75)
        return make_detail("1101", "", 0.9, 0.0)

    if primary_code == "12":
        if has_any(text, ("railway", "铁路")):
            return make_detail("1201", "", 0.95, 0.0)
        if has_any(text, ("aeroway", "hangar", "航空", "机场", "机库")):
            return make_detail("1203", "", 0.9, 0.0)
        if has_any(text, ("harbour", "marina", "pier", "quay", "ferry", "港口", "码头", "轮渡")):
            if "terminal" in tag or "ferry" in tag:
                return make_detail("1208", "120801", 0.94, 0.9)
            return make_detail("1204", "", 0.9, 0.0)
        if has_any(text, ("bus_station", "taxi", "公共交通", "公交", "出租车")):
            return make_detail("1208", "120802", 0.95, 0.9)
        if has_any(text, ("parking", "停车")):
            return make_detail("1208", "120803", 0.96, 0.95)
        if has_any(text, ("train_station", "station", "terminal", "场站")):
            return make_detail("1208", "120801", 0.9, 0.82)
        if has_any(text, ("fuel", "charging_station", "car_rental", "driver_training", "driving_school", "vehicle_inspection", "交通服务", "驾校", "检测")):
            return make_detail("1209", "", 0.82, 0.0)
        return make_detail("1209", "", 0.75, 0.0)

    if primary_code == "13":
        if has_any(text, ("water_works", "供水")):
            return make_detail("1301", "", 0.96, 0.0)
        if has_any(text, ("wastewater", "drain", "排水", "污水")):
            return make_detail("1302", "", 0.96, 0.0)
        if has_any(text, ("power", "electric", "供电", "电力")):
            return make_detail("1303", "", 0.9, 0.0)
        if has_any(text, ("gasometer", "gas", "燃气", "储气")):
            return make_detail("1304", "", 0.9, 0.0)
        if has_any(text, ("cooling", "chiller", "heat", "供热", "制冷")):
            return make_detail("1305", "", 0.82, 0.0)
        if has_any(text, ("telecom", "communication", "通信")):
            return make_detail("1306", "", 0.9, 0.0)
        if has_any(text, ("post", "邮政")):
            return make_detail("1307", "", 0.9, 0.0)
        if has_any(text, ("landfill", "waste", "toilets", "环卫", "垃圾", "厕所")):
            return make_detail("1309", "", 0.9, 0.0)
        if has_any(text, ("fire_station", "消防")):
            return make_detail("1310", "", 0.98, 0.0)
        if has_any(text, ("dyke", "dam", "reservoir", "watermill", "水工", "堤", "坝", "水利")):
            return make_detail("1311", "", 0.9, 0.0)
        return make_detail("1312", "", 0.75, 0.0)

    if primary_code == "14":
        if has_any(text, ("park", "garden", "greenery", "dog_park", "公园", "绿地", "花园")):
            return make_detail("1401", "", 0.9, 0.0)
        if has_any(text, ("protect", "防护")):
            return make_detail("1402", "", 0.85, 0.0)
        if has_any(text, ("common", "recreation", "playground", "golf", "fishing", "picnic", "viewpoint", "horse_riding", "开敞", "广场", "活动", "游憩")):
            return make_detail("1403", "", 0.86, 0.0)
        return make_detail("1401", "", 0.75, 0.0)

    if primary_code == "15":
        if has_any(text, ("military", "军事")):
            return make_detail("1501", "", 0.98, 0.0)
        if has_any(text, ("embassy", "diplomatic", "使馆", "外交")):
            return make_detail("1502", "", 0.78, 0.0)
        if has_any(text, ("worship", "relig", "church", "chapel", "cathedral", "temple", "mosque", "pagoda", "宗教", "教堂", "寺庙")):
            return make_detail("1503", "", 0.96, 0.0)
        if has_any(text, ("historic", "文物", "古迹")):
            return make_detail("1504", "", 0.75, 0.0)
        if has_any(text, ("prison", "监狱", "监教")):
            return make_detail("1505", "", 0.98, 0.0)
        if has_any(text, ("cemetery", "grave", "funeral", "crematorium", "殡葬", "墓地", "火葬")):
            return make_detail("1506", "", 0.98, 0.0)
        return make_detail("1507", "", 0.75, 0.0)

    if primary_code == "16":
        return make_detail("1601", "", 0.95, 0.0)

    return make_detail()


def weighted(confidence: float, factor: float) -> float | str:
    if factor <= 0:
        return ""
    return round(min(confidence, confidence * factor), 4)


def candidate_payload(
    primary_code: str,
    secondary_code: str,
    tertiary_code: str,
    weight_l1: float,
    basis: str,
) -> dict[str, Any]:
    weight_l2 = weighted(weight_l1, 0.9 if secondary_code else 0.0)
    weight_l3 = weighted(float(weight_l2), 0.85 if tertiary_code and weight_l2 != "" else 0.0)
    return {
        "primary_code": primary_code,
        "primary": PLANNING_PRIMARY.get(primary_code, ""),
        "secondary_code": secondary_code,
        "secondary": PLANNING_SECONDARY.get(secondary_code, "") if secondary_code else "",
        "tertiary_code": tertiary_code,
        "tertiary": PLANNING_TERTIARY.get(tertiary_code, "") if tertiary_code else "",
        "weight_l1": round(weight_l1, 4),
        "weight_l2": weight_l2,
        "weight_l3": weight_l3,
        "basis": basis,
    }


def build_alt_candidates_json(row: pd.Series | dict[str, Any], detail: dict[str, Any]) -> str:
    confidence = float(row.get("confidence", 0.0))
    use_direct = bool(row.get("use_for_landuse_classification", False))
    layer_weights: list[float] = [confidence]
    weight_l2 = weighted(confidence, float(detail["secondary_factor"]))
    weight_l3 = weighted(confidence, float(detail["tertiary_factor"]))
    if weight_l2 != "":
        layer_weights.append(float(weight_l2))
    if weight_l3 != "":
        layer_weights.append(float(weight_l3))
    if use_direct and min(layer_weights) >= LOW_CONFIDENCE_THRESHOLD:
        return ""

    tag = normalize_tag(str(row.get("value", "")))
    hint = str(row.get("landuse_hint", ""))
    source = normalize_tag(str(row.get("source_categories", "")))
    text = f"{tag} {hint} {source}".lower()
    main_key = (str(row.get("primary_code", "")), detail["secondary_code"], detail["tertiary_code"])
    candidates: list[dict[str, Any]] = []
    seen: set[tuple[str, str, str]] = set()

    def add(primary: str, secondary: str, tertiary: str, weight: float, basis: str) -> None:
        key = (primary, secondary, tertiary)
        if key == main_key or key in seen:
            return
        seen.add(key)
        candidates.append(candidate_payload(primary, secondary, tertiary, weight, basis))

    if has_any(text, ("garage", "carport", "parking", "停车")):
        add("12", "1208", "120803", 0.62, "停车/车库语义也可能表示交通场站或社会停车场")
        add("07", "0702", "", 0.5, "住宅或社区附属停车设施")
    if has_any(text, ("public", "hall", "public_bath", "toilets", "公共", "浴室", "厕所")):
        add("13", "1309", "", 0.45, "公共厕所、洗浴等也可能是环卫或公共附属设施")
        add("08", "0801", "", 0.4, "公共服务语义宽泛，可作为公共管理服务弱证据")
    if has_any(text, ("art", "studio", "hackerspace", "gallery", "文化", "艺术", "创客")):
        add("09", "0902", "", 0.45, "工作室/创作空间也可能是商务办公或商业服务")
        add("08", "0803", "080302", 0.42, "文化活动设施备选")
    if has_any(text, ("aquarium", "zoo", "attraction", "theme_park", "water_park", "景点", "游乐")):
        add("08", "0803", "080301", 0.42, "大型展览科普设施可归公共文化")
        add("14", "1401", "", 0.35, "开放游憩空间可作为绿地开敞空间弱证据")
    if has_any(text, ("fuel", "charging_station", "加油", "充电")):
        add("09", "0901", "090105", 0.6, "加油、充换电也符合公用设施营业网点用地")
        add("12", "1209", "", 0.5, "服务交通出行的附属设施")
    if has_any(text, ("fitness", "swimming_pool", "playground", "pitch", "track", "运动", "健身")):
        add("08", "0805", "080501", 0.5, "体育设施备选")
        add("14", "1403", "", 0.45, "户外活动场地备选")
    if has_any(text, ("service", "repair", "car_wash", "laundry", "beauty", "维修", "洗车", "洗衣", "美容")):
        add("09", "0904", "", 0.5, "服务业网点可归其他商业服务业用地")
    if "building" in source:
        add("07", "0701", "", 0.35, "building 来源仅说明建筑实体，住宅是弱备选")
    if "amenity" in source:
        add("08", "0801", "", 0.35, "amenity 来源可作为公共服务弱备选")
    if "leisure" in source:
        add("14", "1403", "", 0.38, "leisure 来源可作为开敞活动场地弱备选")
    if "tourism" in source:
        add("09", "0903", "", 0.4, "tourism 来源可作为娱乐/旅游商业弱备选")
    if "man_made" in source:
        add("13", "1312", "", 0.32, "man_made 来源可作为公用设施或构筑物弱备选")

    candidates = sorted(candidates, key=lambda item: item["weight_l1"], reverse=True)[:3]
    return json.dumps(candidates, ensure_ascii=False) if candidates else ""


def build_export_mapping_dataframe(mapped: pd.DataFrame) -> pd.DataFrame:
    rows: list[dict[str, Any]] = []
    for _, row in mapped.iterrows():
        rows.append(
            {
                "tag": str(row["value"]),
                "tag_zh": tag_to_chinese_name(str(row["value"])),
                "primary_code": str(row["primary_code"]),
                "primary": str(row["primary_name"]),
                "secondary_code": str(row.get("secondary_code", "")),
                "secondary": str(row.get("secondary_name", "")),
                "tertiary_code": str(row.get("tertiary_code", "")),
                "tertiary": str(row.get("tertiary_name", "")),
                "weight_l1": row.get("primary_score", round(float(row["confidence"]), 4)),
                "weight_l2": row.get("secondary_score", ""),
                "weight_l3": row.get("tertiary_score", ""),
                "alt_candidates_json": str(row.get("alt_candidates_json", "")),
                "use_for_landuse_classification": bool(row["use_for_landuse_classification"]),
                "matched_by": str(row["matched_by"]),
                "comment": str(row["rationale"]),
            }
        )
    export_df = pd.DataFrame(rows)
    return export_df.sort_values(
        ["use_for_landuse_classification", "primary_code", "tag"],
        ascending=[False, True, True],
        kind="stable",
    ).reset_index(drop=True)


def build_mapping_json(export_df: pd.DataFrame) -> dict[str, Any]:
    return {
        "primary_classes": PLANNING_PRIMARY,
        "secondary_classes": PLANNING_SECONDARY,
        "tertiary_classes": PLANNING_TERTIARY,
        "low_confidence_threshold": LOW_CONFIDENCE_THRESHOLD,
        "semantic_backend_hint": "sentence-transformers multilingual embeddings when available, otherwise tfidf-lsa semantic retrieval",
        "tag_value_mapping": {
            row["tag"]: {key: value for key, value in row.items() if key != "tag"}
            for row in export_df.to_dict(orient="records")
        },
    }


def build_export_summary(export_df: pd.DataFrame) -> list[str]:
    usable = export_df[export_df["use_for_landuse_classification"]]
    unusable = export_df[~export_df["use_for_landuse_classification"]]
    primary_counts = usable["primary"].value_counts()
    with_tertiary = export_df[export_df["tertiary_code"].astype(str) != ""]
    with_alternatives = export_df[export_df["alt_candidates_json"].astype(str) != ""]

    lines = [
        "# priority tag value 用地类型映射说明",
        "",
        "## 方法",
        "",
        "- 仅以 `tag_value` 作为分类对象，不输出 `city`、`area`、`total_count`、`area_m2_sum` 等统计字段。",
        "- 主分类路径改为语义匹配：将 `tag_value` 与用地分类定义文本进行相似度检索，不再依赖大批量手写关键词分支。",
        "- 如果环境已安装 `sentence-transformers`，优先使用多语言 embedding；否则回退到 `tfidf + LSA` 的语义检索后备实现。",
        "- 输出固定列 `primary/secondary/tertiary`，能从标签语义直接判断三级类时填充三级类；证据不足时保留三级为空。",
        "- `weight_l1`、`weight_l2`、`weight_l3` 分别表示一级、二级、三级分类置信度；层级越细，权重不会高于上一级。",
        f"- 当主分类权重低于 {LOW_CONFIDENCE_THRESHOLD} 或不建议直接分类时，`alt_candidates_json` 给出其他可能分类及分层权重。",
        "- 对仅表示构筑物、附属设施、景观小品或语义过弱的标签，标记为 `00`，且不建议直接用于用地分类。",
        "",
        "## 统计",
        "",
        f"- tag_value 总数：{len(export_df)}",
        f"- 可直接用于用地判定：{len(usable)}",
        f"- 需人工复核或不建议直接判定：{len(unusable)}",
        f"- 已细化到三级类：{len(with_tertiary)}",
        f"- 带备选分类依据：{len(with_alternatives)}",
        "",
        "## 一级类覆盖",
        "",
    ]
    lines.extend(f"- {name}: {count} 个 tag_value" for name, count in primary_counts.items())
    if len(unusable) > 0:
        lines.extend(
            [
                "",
                "## 未直接用于用地判定的标签示例",
                "",
                "- " + ", ".join(unusable["tag"].head(30).astype(str).tolist()),
            ]
        )
    return lines


def build_summary(mapped: pd.DataFrame) -> list[str]:
    usable = mapped[mapped["use_for_landuse_classification"]]
    unusable = mapped[~mapped["use_for_landuse_classification"]]
    primary_counts = usable["primary_name"].value_counts()

    lines = [
        "# 多城市 priority tag 用地划分说明",
        "",
        "## 方法",
        "",
        "- 以 `multi_city_priority_tag_values_*_unique_values.csv` 中的 `value` 为分类对象。",
        "- 优先使用人工精确映射，其次使用关键词和来源类型回退规则。",
        "- 依据国土空间规划一级用地框架输出 `primary_code` 和 `primary_name`。",
        "- 对仅表示构筑物、附属设施、景观小品或语义过弱的标签，标记为 `00`，不建议直接用于主体用地判定。",
        "",
        "## 统计",
        "",
        f"- 标签总数：{len(mapped)}",
        f"- 可直接用于用地判定：{len(usable)}",
        f"- 需人工复核或不建议直接判定：{len(unusable)}",
        "",
        "## 一级类覆盖",
        "",
    ]
    lines.extend(f"- {name}: {count} 个 tag" for name, count in primary_counts.items())
    if len(unusable) > 0:
        lines.extend(
            [
                "",
                "## 未直接用于用地判定的标签示例",
                "",
                "- " + ", ".join(unusable["value"].head(30).astype(str).tolist()),
            ]
        )
    return lines


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Classify priority tag values into planning land-use groups.")
    parser.add_argument("--xlsx", action="store_true", help="Also export an Excel workbook with review sheets")
    parser.add_argument("--llm-fallback", action="store_true", help="Use OpenAI-compatible LLM only for low-confidence tags")
    parser.add_argument("--llm-base-url", default=os.getenv("OPENAI_BASE_URL", ""), help="OpenAI-compatible base url, e.g. http://host:4000/v1")
    parser.add_argument("--llm-api-key", default=os.getenv("OPENAI_API_KEY", ""), help="OpenAI-compatible api key")
    parser.add_argument("--llm-model", default=os.getenv("OPENAI_MODEL", ""), help="Model id; if empty, resolve from /models")
    parser.add_argument("--llm-timeout", type=float, default=DEFAULT_LLM_TIMEOUT, help="LLM request timeout seconds")
    parser.add_argument("--llm-trigger-primary-max", type=float, default=LLM_TRIGGER_PRIMARY_MAX, help="Trigger LLM when no tertiary result and primary score <= this threshold")
    parser.add_argument("--llm-cache-path", default=str(LLM_CACHE_PATH), help="Cache file for LLM fallback results")
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    df = pd.read_csv(INPUT_CSV)
    llm_config = None
    if args.llm_fallback:
        if not args.llm_base_url or not args.llm_api_key:
            raise ValueError("LLM fallback requires --llm-base-url and --llm-api-key")
        llm_config = LLMFallbackConfig(
            base_url=args.llm_base_url,
            api_key=args.llm_api_key,
            model=args.llm_model or None,
            timeout=args.llm_timeout,
            trigger_primary_max=args.llm_trigger_primary_max,
            cache_path=pathlib.Path(args.llm_cache_path),
            cache=load_llm_cache(pathlib.Path(args.llm_cache_path)),
        )

    mapped = classify_dataframe(df, llm_config=llm_config)
    export_df = build_export_mapping_dataframe(mapped)

    stem = INPUT_CSV.stem
    csv_path = OUTPUT_DIR / f"{stem}_landuse_mapping.csv"
    json_path = OUTPUT_DIR / f"{stem}_landuse_mapping.json"
    md_path = OUTPUT_DIR / f"{stem}_landuse_mapping.md"
    xlsx_path = OUTPUT_DIR / f"{stem}_landuse_mapping.xlsx"

    csv_path = write_csv_with_fallback(export_df, csv_path)
    json_path = write_text_with_fallback(
        json_path,
        json.dumps(build_mapping_json(export_df), ensure_ascii=False, indent=2),
        encoding="utf-8",
    )
    md_path = write_text_with_fallback(md_path, "\n".join(build_export_summary(export_df)), encoding="utf-8")
    if args.xlsx:
        try:
            weak_only = export_df[~export_df["use_for_landuse_classification"]].copy()
            xlsx_path = write_xlsx_with_fallback(export_df, weak_only, xlsx_path)
        except Exception:
            xlsx_path = pathlib.Path("")
    else:
        xlsx_path = pathlib.Path("")

    xlsx_value = str(xlsx_path) if args.xlsx and str(xlsx_path) not in {"", "."} else ""

    print(
        json.dumps(
            {
                "csv": str(csv_path),
                "json": str(json_path),
                "md": str(md_path),
                "xlsx": xlsx_value,
                "total": int(len(export_df)),
                "usable": int(export_df["use_for_landuse_classification"].sum()),
                "unmapped": int((export_df["primary_code"] == "00").sum()),
                "with_tertiary": int((export_df["tertiary_code"].astype(str) != "").sum()),
                "with_alternatives": int((export_df["alt_candidates_json"].astype(str) != "").sum()),
                "llm_fallback_enabled": bool(llm_config is not None),
                "llm_cache_path": str(llm_config.cache_path) if llm_config is not None else "",
                "llm_model": llm_config.model if llm_config is not None else "",
            },
            ensure_ascii=False,
            indent=2,
        )
    )


if __name__ == "__main__":
    main()
