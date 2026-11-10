# 定义宏观功能大类
category_mapping = {
    # ---------------- 1. 居住区 (Residential) ----------------
    'Residential': [
        # 来自 POI/Building
        'ResidentalComplex',
        # 来自 Landuse
        'residential', 'garages'
    ],

    # ---------------- 2. 商业区 (Commercial) ----------------
    'Commercial': [
        # 来自 POI/Building
        'Bank', 'CarDealer', 'ConvienceStore', 'HairSalon',
        'HomeBuildingMaterialMarket', 'Hotel', 'KTV', 'LanzhouRamen',
        'McDonalds', 'OfficeBuilding', 'ShoppingMall',
        # 来自 Landuse
        'commercial', 'retail'
    ],

    # ---------------- 3. 公共服务区 (Public Services) ----------------
    'PublicServices': [
        # 来自 POI/Building
        'Airport', 'CoachStation', 'Hospital', 'Kindergarten',
        'Library', 'Museum', 'PostOffice', 'PublicBathroom',
        'School', 'Stadium', 'TrainStation',
        # 来自 Landuse
        'education', 'government', 'governmental', 'govermental',
        'civic_admin', 'diplomatic', 'research', 'religious',
        'religional', 'parking'
    ],

    # ---------------- 4. 工业与交通设施 (Industrial) ----------------
    'Industrial': [
        # 来自 POI/Building
        'Factory', 'Port',
        # 来自 Landuse
        'industrial', 'construction', 'brownfield', 'military',
        'railway', 'quarry', 'landfill', 'harbour'
    ],

    # ---------------- 5. 绿地与农业区 (Green Space & Agriculture) ----------------
    'GreenSpace': [
        # 来自 POI/Building
        'AmusementPark', 'Park', 'ViewSpot', 'Zoo', 'Onsen',
        # 来自 Landuse
        'forest', 'farmland', 'grass', 'orchard', 'greenhouse_horticulture',
        'plant_nursery', 'farmyard', 'recreation_ground', 'cemetery',
        'meadow', 'flowerbed', 'greenfield', 'village_green',
        'winter_sports', 'vineyard', 'basin', 'reservoir',
        'greenery', 'allotments'
    ],

    # ---------------- 6. 其他/待定 (Others) ----------------
    'Others': [
        # 来自 Landuse
        'yes'
    ]
}
