# 结合现代城市区域规则的 7 大宏观功能映射字典
category_mapping = {
    # ---------------- 1. 居住区 (Residential Area) ----------------
    'Residential': [
        # amenity
        'nursing_home', 'postpartum_care',
        # tourism
        'apartment', 'guest_house', 'chalet',
        # building
        'yes', 'apartments', 'residential', 'house', 'bungalow', 'detached', 'dormitory',
        'terrace', 'semidetached_house', 'hut', 'farm', 'garages', 'garage',
        'carport', 'shed', 'outbuilding', '49号楼', 'bungalow;yes', 'yes;house', 'ger',
        # landuse
        'residential', 'garages'
    ],

    # ---------------- 2. 商业区 (Commercial Area) ----------------
    'Commercial': [
        # amenity
        'restaurant', 'cafe', 'fast_food', 'food_court', 'bar', 'ice_cream',
        'nightclub', 'marketplace', 'bank', 'pharmacy', 'car_rental', 'car_wash',
        'vehicle_inspection', 'dentist', 'clinic', 'love_hotel', 'casino',
        'cinema', 'theatre', 'studio',
        # tourism
        'hotel', 'hostel', 'motel', 'resort', 'beach_resort',
        # building
        'commercial', 'retail', 'office', 'hotel', 'hotel;yes', 'restaurant', 'company',
        # landuse
        'commercial', 'retail'
    ],

    # ---------------- 3. 工业区 (Industrial Area) ----------------
    'Industrial': [
        # building
        'industrial', 'warehouse', 'greenhouse', 'hangar', 'silo', 'storage_tank',
        'kiln', 'cooling_tower', 'gasometer', 'farm_auxiliary', 'barn', 'sty',
        # landuse
        'industrial', 'brownfield', 'quarry',
        # industrial (专项细分)
        'depot', 'factory', 'brickyard', 'distributor', 'scrap_yard', 'storage',
        'communication', 'pharmaceuticals', 'heating_station', 'oil', 'concrete_plant',
        'mine', 'refinery', 'winery', 'food_processing', 'grain_storage_centre',
        'research', 'timber', 'water', 'railway', 'steelworks', 'research_facility',
        'chemical', 'metal_processing', 'steelmaking', 'manufacturing',
        #man_made
        'storage_tank', 'gasometer', 'works', 'wastewater_plant', 'cooling',
        'cooling_tower', 'chimney', 'water_works', 'pumping_station', 'silo',
        'crane', 'water_tower', 'reservoir_covered', 'clarifier', 'water_well', 'adit'
    ],

    # ---------------- 4. 公共服务区 (Public Service Area) ----------------
    'PublicServices': [
        # amenity
        'school', 'kindergarten', 'university', 'college', 'place_of_worship',
        'hospital', 'toilets', 'shelter', 'police', 'research_institute', 'library',
        'community_centre', 'social_facility', 'fire_station', 'fountain', 'prison',
        'courthouse', 'driver_training', 'townhall', 'arts_centre', 'post_office',
        'charging_station', 'conference_centre', 'parcel_locker', 'public_bath',
        'public_building', 'childcare', 'smoking_area', 'music_school', 'bench',
        'planetarium', 'shower', 'events_venue', 'crematorium', 'driving_school',
        'funeral_hall', 'language_school', 'public_bookcase', 'archive', 'prep_school',
        # leisure
        'hackerspace',
        # tourism
        'museum', 'artwork', 'information', 'gallery', 'aquarium',
        # building
        'school', 'university', 'hospital', 'college', 'kindergarten', 'temple',
        'public', 'mosque', 'church', 'government', 'civic', 'toilets', 'shrine',
        'police', 'chapel', 'religious', 'courthouse', 'fire_station', 'embassy',
        'pagoda', 'monastery', 'historic', 'washroom', 'museum', 'library',
        'gymnasium', 'univuniversity', 'hall', 'yes;school', '高校建筑',
        # landuse
        'education', 'government', 'governmental', 'govermental', 'civic_admin',
        'religional', 'religious', 'research', 'diplomatic', 'services', 'cemetery',
        # amenity (补充)
        'grave_yard'
    ],

    # ---------------- 5. 绿地与开敞空间 (Green Space and Open Space) ----------------
    'GreenSpace': [
        # leisure
        'pitch', 'park', 'garden', 'track', 'stadium', 'sports_centre', 'playground',
        'golf_course', 'swimming_pool', 'sports_hall', 'common', 'fitness_centre',
        'fitness_station', 'bleachers', 'nature_reserve', 'dance', 'outdoor_seating',
        'horse_riding', 'fishing', 'ice_rink', 'bandstand', 'water_park',
        'miniature_golf', 'marina',
        # tourism
        'attraction', 'theme_park', 'zoo', 'viewpoint', 'picnic_site', 'camp_site',
        # building
        'stadium', 'grandstand', 'pavilion', 'statue', 'sports_centre',
        # landuse
        'forest', 'farmland', 'grass', 'orchard', 'greenhouse_horticulture',
        'plant_nursery', 'farmyard', 'recreation_ground', 'meadow', 'flowerbed',
        'greenfield', 'village_green', 'vineyard', 'winter_sports', 'basin',
        'reservoir', 'greenery', 'allotments'
    ],

    # ---------------- 6. 交通设施区 (Transportation Facility Area) ----------------
    'Transportation': [
        # amenity
        'parking', 'bicycle_parking', 'fuel', 'parking_space', 'bus_station',
        'car_pooling', 'taxi', 'parking_entrance', 'bicycle_rental',
        'sanitary_dump_station', 'motorcycle_parking',
        # building
        'transportation', 'train_station', 'bridge', 'railway', 'depot', 'bus_station',
        'toll_booth', 'station',
        # landuse
        'railway', 'harbour', 'parking'

        #man_made
        'pier', 'tunnel', 'quay', 'embankment'

    ],

    # ---------------- 7. 特殊区域 (Special Area & Unknowns) ----------------
    'Special': [
        # amenity
        'waste_transfer_station', 'waste_disposal', 'recycling', 'animal_breeding',
        # tourism
        'yes',
        # building
        'roof', 'wall', 'construction', 'ruins', 'gate', 'bunker',
        'guardhouse', 'tower', 'no', 'stable', 'gatehouse', 'diaolou', 'boathouse',
        'temporary',
        # landuse
        'military', 'construction', 'landfill', 'yes'
    ]
}
