# 现代城市街区拓扑规则库 (Urban Block Typology Rules - 聚类驱动版)

urban_block_rules = {
    # ==========================================
    # 1. 核心居住区 (Residential Core)
    # ==========================================
    "ResidentialCore": {
        "macro_class": "核心居住区",
        "base_elements": ['residential', 'apartments', 'common', 'school', 'park', 'yes'],
        "profiles": [
            {
                "sub_type": "高纯度居住主导型",
                "dominant_tag": "residential",
                "signature": {"residential": 0.8831, "school": 0.0247, "park": 0.0193, "yes": 0.0130, "commercial": 0.0099, "retail": 0.0089, "parking": 0.0075}
            },
            {
                "sub_type": "综合配套居住型",
                "dominant_tag": "residential",
                "signature": {"residential": 0.5313, "school": 0.0651, "commercial": 0.0549, "yes": 0.0517, "forest": 0.0409, "retail": 0.0335, "industrial": 0.0289, "park": 0.0282, "brownfield": 0.0247, "parking": 0.0163}
            },
            {
                "sub_type": "学区及商业公寓混合型",
                "dominant_tag": "apartments",
                "signature": {"apartments": 0.4512, "school": 0.1967, "yes": 0.1164, "commercial": 0.1080, "residential": 0.0600, "retail": 0.0183, "hotel": 0.0141, "park": 0.0083, "garden": 0.0074, "hospital": 0.0073}
            },
            {
                "sub_type": "高密高绿化公寓主导型",
                "dominant_tag": "apartments",
                "signature": {"apartments": 0.6455, "yes": 0.1270, "commercial": 0.0705, "park": 0.0516, "brownfield": 0.0342, "university": 0.0280, "residential": 0.0070, "school": 0.0069, "retail": 0.0065, "hospital": 0.0054}
            },
            {
                "sub_type": "普通公房混合型",
                "dominant_tag": "common",
                "signature": {"common": 0.5778, "yes": 0.4222}
            }
        ]
    },

    # ==========================================
    # 2. 城市生态混合区 (Eco Urban Mix)
    # ==========================================
    "EcoUrbanMix": {
        "macro_class": "生态城市混合区",
        "base_elements": ['park', 'forest', 'farmland', 'golf_course', 'greenfield', 'garden'],
        "profiles": [
            {
                "sub_type": "城市公园核心区",
                "dominant_tag": "park",
                "signature": {"park": 0.9259, "yes": 0.0113, "residential": 0.0091, "parking": 0.0089, "forest": 0.0067}
            },
            {
                "sub_type": "公园社区生活圈",
                "dominant_tag": "park",
                "signature": {"park": 0.5154, "residential": 0.2995, "school": 0.0304, "forest": 0.0297, "commercial": 0.0179, "retail": 0.0178, "yes": 0.0166, "industrial": 0.0097, "parking": 0.0077, "golf_course": 0.0069}
            },
            {
                "sub_type": "公园商业过渡带",
                "dominant_tag": "park",
                "signature": {"park": 0.6228, "brownfield": 0.0418, "commercial": 0.0414, "industrial": 0.0320, "residential": 0.0226, "parking": 0.0222, "forest": 0.0217, "farmland": 0.0207, "school": 0.0180, "yes": 0.0153}
            },
            {
                "sub_type": "森林居住缓冲带",
                "dominant_tag": "forest",
                "signature": {"forest": 0.5361, "residential": 0.1131, "brownfield": 0.0667, "farmland": 0.0628, "industrial": 0.0387, "park": 0.0258, "military": 0.0216, "parking": 0.0166, "yes": 0.0162, "railway": 0.0116}
            },
            {
                "sub_type": "纯林地生态区",
                "dominant_tag": "forest",
                "signature": {"forest": 0.8861, "park": 0.0167, "brownfield": 0.0167, "parking": 0.0112, "school": 0.0103, "yes": 0.0096, "industrial": 0.0094, "residential": 0.0086, "commercial": 0.0077, "military": 0.0063}
            },
            {
                "sub_type": "复杂功能公园区",
                "dominant_tag": "park",
                "signature": {"park": 0.3528, "brownfield": 0.0964, "residential": 0.0937, "commercial": 0.0815, "industrial": 0.0765, "forest": 0.0261, "retail": 0.0209, "yes": 0.0207, "military": 0.0205, "school": 0.0199}
            },
            {
                "sub_type": "高尔夫高端社区",
                "dominant_tag": "golf_course",
                "signature": {"golf_course": 0.4954, "residential": 0.1341, "park": 0.1255, "forest": 0.0782, "theme_park": 0.0270, "brownfield": 0.0195, "yes": 0.0144, "school": 0.0142, "farmyard": 0.0133, "grass": 0.0097}
            },
            {
                "sub_type": "纯高尔夫球场及周边",
                "dominant_tag": "golf_course",
                "signature": {"golf_course": 0.8789, "forest": 0.0529, "commercial": 0.0096, "railway": 0.0094, "residential": 0.0093, "industrial": 0.0089, "park": 0.0081, "brownfield": 0.0076}
            },
            {
                "sub_type": "高纯度农业保留地",
                "dominant_tag": "farmland",
                "signature": {"farmland": 0.8970, "residential": 0.0221, "golf_course": 0.0205, "yes": 0.0180, "park": 0.0148, "sports_centre": 0.0127, "forest": 0.0058, "apartments": 0.0054}
            },
            {
                "sub_type": "待开发绿地居住区",
                "dominant_tag": "greenfield",
                "signature": {"greenfield": 0.3008, "park": 0.2747, "residential": 0.2580, "school": 0.0691, "yes": 0.0544, "parking": 0.0231, "kindergarten": 0.0099, "warehouse": 0.0070}
            }
        ]
    },

    # ==========================================
    # 3. 工业与物流区 (Industry & Logistics)
    # ==========================================
    "IndustryLogistics": {
        "macro_class": "工业与物流设施区",
        "base_elements": ['industrial', 'brownfield', 'wastewater_plant', 'water_works', 'farmyard', 'landfill'],
        "profiles": [
            {
                "sub_type": "高纯度工业核心区",
                "dominant_tag": "industrial",
                "signature": {"industrial": 0.8856, "park": 0.0273, "yes": 0.0262, "brownfield": 0.0164, "residential": 0.0100, "commercial": 0.0088, "forest": 0.0052}
            },
            {
                "sub_type": "工商业混合发展区",
                "dominant_tag": "industrial",
                "signature": {"industrial": 0.4075, "commercial": 0.3072, "park": 0.0533, "residential": 0.0523, "yes": 0.0489, "hospital": 0.0340, "brownfield": 0.0278, "retail": 0.0210, "forest": 0.0209, "garden": 0.0085}
            },
            {
                "sub_type": "纯废弃工业/棕地区",
                "dominant_tag": "brownfield",
                "signature": {"brownfield": 0.9105, "park": 0.0200, "forest": 0.0122, "residential": 0.0115, "retail": 0.0093, "parking": 0.0080, "yes": 0.0064}
            },
            {
                "sub_type": "棕地及居住过渡带",
                "dominant_tag": "brownfield",
                "signature": {"brownfield": 0.4924, "residential": 0.1657, "commercial": 0.0605, "park": 0.0581, "industrial": 0.0573, "forest": 0.0333, "school": 0.0209, "farmland": 0.0161, "yes": 0.0142, "retail": 0.0118}
            },
            {
                "sub_type": "市政水务/污水处理区",
                "dominant_tag": "wastewater_plant",
                "signature": {"wastewater_plant": 0.6597, "commercial": 0.0822, "prison": 0.0797, "brownfield": 0.0542, "park": 0.0264, "industrial": 0.0233, "farmland": 0.0179, "forest": 0.0147, "residential": 0.0146, "railway": 0.0105}
            },
            {
                "sub_type": "垃圾填埋及工业尾端区",
                "dominant_tag": "landfill",
                "signature": {"landfill": 0.4843, "brownfield": 0.1820, "industrial": 0.1142, "residential": 0.0743, "forest": 0.0594, "railway": 0.0369, "golf_course": 0.0247, "orchard": 0.0239}
            }
        ]
    },

    # ==========================================
    # 4. 商业及服务区 (Commerce & Service)
    # ==========================================
    "CommerceService": {
        "macro_class": "商业及服务业核心区",
        "base_elements": ['commercial', 'retail', 'hotel', 'studio', 'mall', 'marketplace'],
        "profiles": [
            {
                "sub_type": "高纯度商业核心",
                "dominant_tag": "commercial",
                "signature": {"commercial": 0.8704, "park": 0.0456, "yes": 0.0184, "retail": 0.0120, "industrial": 0.0089, "residential": 0.0080, "school": 0.0059}
            },
            {
                "sub_type": "商业及配套居住混合区",
                "dominant_tag": "commercial",
                "signature": {"commercial": 0.4725, "residential": 0.1587, "yes": 0.1014, "industrial": 0.0597, "park": 0.0337, "forest": 0.0266, "school": 0.0264, "parking": 0.0200, "brownfield": 0.0147, "retail": 0.0125}
            },
            {
                "sub_type": "高纯度零售区",
                "dominant_tag": "retail",
                "signature": {"retail": 0.9039, "yes": 0.0286, "commercial": 0.0232, "meadow": 0.0215, "forest": 0.0098}
            },
            {
                "sub_type": "零售及居住均衡社区",
                "dominant_tag": "retail",
                "signature": {"retail": 0.3788, "residential": 0.3278, "yes": 0.0436, "park": 0.0317, "school": 0.0284, "industrial": 0.0270, "parking": 0.0265, "military": 0.0254, "commercial": 0.0209, "bus_station": 0.0134}
            },
            {
                "sub_type": "大型商场及附属停车场",
                "dominant_tag": "mall",
                "signature": {"mall": 0.4152, "parking": 0.4142, "yes": 0.1448, "forest": 0.0253}
            },
            {
                "sub_type": "集贸市场与居住群",
                "dominant_tag": "marketplace",
                "signature": {"marketplace": 0.8215, "residential": 0.1779}
            }
        ]
    },

    # ==========================================
    # 5. 公共机构与科教区 (Public Institution)
    # ==========================================
    "PublicInstitution": {
        "macro_class": "科教文卫与行政区",
        "base_elements": ['university', 'school', 'hospital', 'college', 'research_institute', 'place_of_worship', 'government'],
        "profiles": [
            {
                "sub_type": "高纯度高校/大学城",
                "dominant_tag": "university",
                "signature": {"university": 0.8264, "yes": 0.0358, "school": 0.0249, "grass": 0.0241, "residential": 0.0171, "farmland": 0.0121, "recreation_ground": 0.0117, "parking": 0.0081, "garden": 0.0081, "restaurant": 0.0059}
            },
            {
                "sub_type": "高校与周边生活配套区",
                "dominant_tag": "university",
                "signature": {"university": 0.5280, "residential": 0.1970, "commercial": 0.0701, "yes": 0.0299, "school": 0.0267, "hospital": 0.0226, "park": 0.0179, "forest": 0.0166, "industrial": 0.0136, "grass": 0.0114}
            },
            {
                "sub_type": "学区房/中小学居住带",
                "dominant_tag": "school",
                "signature": {"school": 0.4707, "residential": 0.2990, "commercial": 0.0841, "park": 0.0544, "railway": 0.0186, "forest": 0.0181, "yes": 0.0143, "farmyard": 0.0100, "research_institute": 0.0055}
            },
            {
                "sub_type": "高纯度医疗核心区",
                "dominant_tag": "hospital",
                "signature": {"hospital": 0.7330, "park": 0.0784, "forest": 0.0410, "yes": 0.0360, "university": 0.0245, "industrial": 0.0208, "apartments": 0.0191, "parking": 0.0177, "retail": 0.0104}
            },
            {
                "sub_type": "医疗康养及居住区",
                "dominant_tag": "hospital",
                "signature": {"hospital": 0.4968, "residential": 0.3740, "museum": 0.0404, "commercial": 0.0291, "brownfield": 0.0268, "research_institute": 0.0086, "parking": 0.0064, "park": 0.0058, "retail": 0.0058}
            },
            {
                "sub_type": "行政办公及家属居住区",
                "dominant_tag": "government",
                "signature": {"government": 0.5476, "residential": 0.3066, "commercial": 0.1335, "parking": 0.0105}
            },
            {
                "sub_type": "高纯度政府/行政机关区",
                "dominant_tag": "governmental",
                "signature": {"governmental": 0.8483, "military": 0.0542, "residential": 0.0537, "yes": 0.0184, "grass": 0.0128}
            },
            {
                "sub_type": "宗教及附属社区配套",
                "dominant_tag": "place_of_worship",
                "signature": {"place_of_worship": 0.5128, "yes": 0.1846, "residential": 0.1154, "retail": 0.0407, "commercial": 0.0396, "marketplace": 0.0162, "parking": 0.0156, "school": 0.0155, "park": 0.0134, "museum": 0.0090}
            }
        ]
    },

    # ==========================================
    # 6. 交通枢纽区 (Transport Hub)
    # ==========================================
    "TransportHub": {
        "macro_class": "交通枢纽与配套服务区",
        "base_elements": ['railway', 'parking', 'fuel', 'bus_station', 'track', 'train_station'],
        "profiles": [
            {
                "sub_type": "高纯度铁路骨干网",
                "dominant_tag": "railway",
                "signature": {"railway": 0.8581, "park": 0.0295, "industrial": 0.0290, "forest": 0.0264, "residential": 0.0245, "yes": 0.0108, "parking": 0.0051}
            },
            {
                "sub_type": "火车站/轨道交通站点圈",
                "dominant_tag": "track",
                "signature": {"track": 0.5743, "train_station": 0.2185, "residential": 0.1393, "pitch": 0.0679}
            },
            {
                "sub_type": "大型集中停车场",
                "dominant_tag": "parking",
                "signature": {"parking": 0.7628, "yes": 0.0665, "brownfield": 0.0503, "retail": 0.0419, "pitch": 0.0388, "stadium": 0.0236, "commercial": 0.0161}
            },
            {
                "sub_type": "加油站及道路服务区",
                "dominant_tag": "fuel",
                "signature": {"fuel": 0.8273, "yes": 0.0805, "forest": 0.0766, "roof": 0.0156}
            },
            {
                "sub_type": "长途客运/公交总站带",
                "dominant_tag": "bus_station",
                "signature": {"bus_station": 0.5074, "park": 0.2054, "brownfield": 0.1395, "forest": 0.0604, "garden": 0.0294, "yes": 0.0260, "industrial": 0.0214}
            }
        ]
    },

    # ==========================================
    # 7. 国防及禁区 (Defense)
    # ==========================================
    "Defense": {
        "macro_class": "军事与国家安全设施",
        "base_elements": ['military'],
        "profiles": [
            {
                "sub_type": "高纯度军事禁区",
                "dominant_tag": "military",
                "signature": {"military": 0.9495, "yes": 0.0149, "residential": 0.0104, "school": 0.0074, "commercial": 0.0060}
            },
            {
                "sub_type": "军区家属院/后勤配套区",
                "dominant_tag": "military",
                "signature": {"military": 0.4008, "residential": 0.2028, "park": 0.0859, "forest": 0.0640, "yes": 0.0453, "brownfield": 0.0310, "commercial": 0.0296, "school": 0.0284, "hospital": 0.0284, "industrial": 0.0176}
            }
        ]
    },

    # ==========================================
    # 8. 纯生态及景观设施区 (Eco Green & Scenery)
    # ==========================================
    "EcoGreen": {
        "macro_class": "自然景观与特色地貌",
        "base_elements": ['grass', 'nature_reserve', 'theme_park', 'vineyard', 'cemetery', 'orchard'],
        "profiles": [
            {
                "sub_type": "自然保护区极核",
                "dominant_tag": "nature_reserve",
                "signature": {"nature_reserve": 0.9945}
            },
            {
                "sub_type": "主题乐园综合体",
                "dominant_tag": "theme_park",
                "signature": {"theme_park": 0.6772, "park": 0.1009, "forest": 0.0571, "parking": 0.0495, "fitness_centre": 0.0350, "industrial": 0.0263, "garden": 0.0236, "pitch": 0.0073, "residential": 0.0064, "theatre": 0.0055}
            },
            {
                "sub_type": "公墓及陵园林地",
                "dominant_tag": "cemetery",
                "signature": {"cemetery": 0.5929, "forest": 0.4071}
            },
            {
                "sub_type": "城市公共草地/文化广场",
                "dominant_tag": "grass",
                "signature": {"grass": 0.5292, "theatre": 0.0960, "commercial": 0.0728, "railway": 0.0646, "forest": 0.0644, "school": 0.0500, "yes": 0.0466, "residential": 0.0378, "golf_course": 0.0194, "parking": 0.0101}
            }
        ]
    },

    # ==========================================
    # 9. 体育运动与休闲区 (Sports & Leisure)
    # ==========================================
    "SportsLeisure": {
        "macro_class": "文体休闲与文娱区",
        "base_elements": ['sports_centre', 'stadium', 'horse_riding', 'pitch'],
        "profiles": [
            {
                "sub_type": "集中体育场馆区",
                "dominant_tag": "sports_centre",
                "signature": {"sports_centre": 0.5581, "brownfield": 0.0618, "pitch": 0.0593, "residential": 0.0570, "sports_hall": 0.0376, "grass": 0.0352, "stadium": 0.0351, "school": 0.0277, "yes": 0.0249, "commercial": 0.0219}
            },
            {
                "sub_type": "大型体育场商业综合体",
                "dominant_tag": "stadium",
                "signature": {"stadium": 0.4567, "yes": 0.0981, "pitch": 0.0733, "retail": 0.0528, "hotel": 0.0514, "commercial": 0.0469, "residential": 0.0436, "garages": 0.0360, "fitness_centre": 0.0339, "park": 0.0320}
            },
            {
                "sub_type": "开敞运动场与节点设施",
                "dominant_tag": "pitch",
                "signature": {"pitch": 0.3305, "stadium": 0.2583, "yes": 0.2104, "bus_station": 0.1466, "track": 0.0541}
            }
        ]
    },

    # ==========================================
    # 10. 居住绿化混合型 (Residential Green Mix)
    # ==========================================
    "ResidentialGreenMix": {
        "macro_class": "居住区及生态绿地",
        "base_elements": ['residential', 'park'],
        "profiles": [
            {
                "sub_type": "花园洋房/公园环绕住区",
                "dominant_tag": "residential",
                "signature": {"residential": 0.5396, "park": 0.2609, "forest": 0.0292, "commercial": 0.0271, "school": 0.0230, "brownfield": 0.0206, "yes": 0.0147, "parking": 0.0127, "industrial": 0.0113, "retail": 0.0074}
            }
        ]
    },

    # ==========================================
    # 11. 模糊建筑混合区 (Built Mixed Unknown)
    # ==========================================
    "BuiltMixedUnknown": {
        "macro_class": "待定/未分类混合建筑群",
        "base_elements": ['yes', 'park', 'school'],
        "profiles": [
            {
                "sub_type": "高纯度未定义建筑集群",
                "dominant_tag": "yes",
                "signature": {"yes": 0.9074, "industrial": 0.0118, "residential": 0.0112, "school": 0.0109, "park": 0.0080, "parking": 0.0064}
            },
            {
                "sub_type": "未定义功能与学区混合",
                "dominant_tag": "yes",
                "signature": {"yes": 0.5045, "school": 0.2445, "commercial": 0.0364, "residential": 0.0333, "hospital": 0.0307, "retail": 0.0213, "apartments": 0.0213, "parking": 0.0157, "bus_station": 0.0127, "park": 0.0114}
            }
        ]
    },

    # ==========================================
    # 12. 其他特殊设施区 (Other Special)
    # ==========================================
    "OtherSpecial": {
        "macro_class": "特殊或独立市政设施",
        "base_elements": ['monitoring_station'],
        "profiles": [
            {
                "sub_type": "独立监测站设施点",
                "dominant_tag": "monitoring_station",
                "signature": {"monitoring_station": 1.0000}
            }
        ]
    }
}