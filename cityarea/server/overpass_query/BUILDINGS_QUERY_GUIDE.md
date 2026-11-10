# Building Data Query Guide

This guide explains how to query and process all building data from OSM for Beijing with classifications.

## Overview

The building query system provides:

- **Complete building coverage**: All ways and relations with `building` tag
- **Area calculation**: Automatic area computation in square meters
- **Building classification**: Maps building types to macro categories (Residential, Commercial, PublicServices, Industrial, GreenSpace, Others)
- **GeoPackage output**: `All_Building.gpkg` viewable in QGIS

## Files

### Core Components

1. **`osm_util.py`** - Main OSM processing module
   - `BuildingHandler`: Osmium handler for extracting building data
   - `convert_building_osm_2_gpkg()`: Converts OSM buildings to GPKG with area and classifications
   - `_load_building_category_mapping()`: Loads category mappings from `type_list.py`
   - `_map_building_to_category()`: Maps individual building types to categories

2. **`query_buildings.py`** - Main query script
   - `query_city_buildings()`: Orchestrates building query and conversion
   - Supports command-line arguments for city name, search radius, logging

### Data Files

- **Input**: OSM data from local cache or remote Overpass API
- **Output**:
  - `{DATA_DIR}/city/Beijing/buildings.osm` - Raw OSM data
  - `{DATA_DIR}/city/Beijing/All_Building.gpkg` - Processed building data with classifications

## Usage

### Basic Query (Server)

On the server (zhoujiajun account):

```bash
cd /home/zhoujiajun/overpass_query
python query_buildings.py Beijing --log
```

### Parameters

- `city` (required): City name, e.g., 'Beijing'
- `--radius`: Search radius in meters (default: 50000, up to ~80km for Beijing)
- `--log`: Enable detailed logging output

### Examples

```bash
# Query Beijing with default 50km radius
python query_buildings.py Beijing

# Query with larger radius and logging
python query_buildings.py Beijing --radius 60000 --log

# Query other cities
python query_buildings.py Shanghai --log
python query_buildings.py Chengdu --radius 30000 --log
```

## Output Format (GPKG)

### Layer: `building`

**Columns:**

- `id`: OSM ID
- `building`: Building type (e.g., 'residential', 'commercial', 'yes')
- `building:levels`: Number of levels (if tagged)
- `height`: Building height (if tagged)
- `name:local`: Chinese name
- `name:en`: English name
- `name:zh`: Alternative Chinese name
- `amenity`: Amenity type (if applicable)
- `shop`: Shop type (if applicable)
- `office`: Office type (if applicable)
- `area_m2`: Building area in square meters
- `category`: Macro category
  - `Residential`: Residential complexes, houses
  - `Commercial`: Commercial buildings, shops, offices, hotels, restaurants
  - `PublicServices`: Schools, hospitals, libraries, government, stations, parking
  - `Industrial`: Factories, warehouses, ports, industrial areas
  - `GreenSpace`: Agricultural structures, greenhouses
  - `Others`: Unclassified buildings

### Geometry

- Type: Polygon or MultiPolygon
- CRS: EPSG:4326 (WGS84)

## Category Mapping

Building types are automatically mapped to macro categories:

```
Residential:
  - residential, house, apartment, detached, semi_detached, terraced

Commercial:
  - commercial, retail, office, shopping_center, mall, supermarket, 
    shop, bank, hotel, restaurant, cafe, bar

PublicServices:
  - school, university, hospital, library, museum, government, 
    police, fire_station, parking, station, train_station, 
    bus_station, airport

Industrial:
  - industrial, warehouse, factory, manufacture, construction, 
    military, port

GreenSpace:
  - greenhouse, parking

Others:
  - Unmatched types, 'yes', etc.
```

The mapping can be customized by editing `type_list.py` at the workspace root.

## Example QGIS Visualization

In QGIS:

1. Open file → `All_Building.gpkg`
2. Select/load the `building` layer
3. Right-click → Properties → Symbology
4. Style by category:
   - Residential → Blue
   - Commercial → Red
   - PublicServices → Yellow
   - Industrial → Gray
   - GreenSpace → Green
   - Others → Light Gray

## Performance Notes

- Query process takes 10-30 minutes depending on:
  - Data size (Beijing ~300k buildings)
  - Local cache availability vs. remote API
  - Network speed
  
- Output file size:
  - Beijing: ~100-150 MB GPKG file
  - Includes geometry, attributes, spatial index

## Troubleshooting

### "No buildings found"

- Verify city name matches OSM data (English name for place=city)
- Increase search radius: `--radius 80000`
- Check local OSM database is up-to-date

### Category mapping not loading

- Ensure `type_list.py` exists at workspace root
- Falls back to hardcoded mapping if file missing
- Check `--log` output for loading errors

### Out of memory

- Process uses geodataframe in-memory
- For very large cities, may need to process in chunks
- Can reduce radius or filter building types before processing

### Slow performance

- Check local OSM cache availability
- Remote API may be rate-limited
- Retry with `--log` to see query timing

## Integration with Sector Classification

The building data can be used with sector classification:

1. Generate buildings: `python query_buildings.py Beijing --log`
2. Generate landuse: `python query_landuse.py Beijing --log`  
3. Classify sectors: `python classify_city_sector.py`

This produces a complete city model with:

- All buildings by type and area
- Sector classification by landuse/POI evidence
- Building coverage by sector

## Development Notes

### Adding Custom Building Types

Edit `_map_building_to_category()` in `osm_util.py`:

```python
def _map_building_to_category(building_type: str, category_mapping: dict) -> str:
    building_type = str(building_type).lower().strip()
    
    # Custom mappings (before type_list lookup)
    if 'dormitory' in building_type:
        return 'Residential'
    if 'mosque' in building_type:
        return 'PublicServices'
    
    # Fall back to type_list
    for category, building_types in category_mapping.items():
        if any(building_type.find(bt.lower()) >= 0 for bt in building_types):
            return category
    
    return 'Others'
```

### Extending Handler

The `BuildingHandler` can be extended to extract additional attributes:

```python
class BuildingHandler(osmium.SimpleHandler):
    def way(self, w: osm_types.Way):
        if 'building' in w.tags:
            # Add custom tag extraction
            brand = w.tags.get('brand')
            operator = w.tags.get('operator')
            # ... add to features
```

## Related Files

- `constants.py` - Data directory configuration
- `type_list.py` - Category mapping definitions
- `classify_city_sector.py` - Sector classification using buildings + landuse
- `query_landuse.py` - Landuse data query (complements building data)

## Future Enhancements

- [ ] Building height estimation from area and levels
- [ ] Per-building population estimates (research)
- [ ] Temporal analysis (OSM edit history)
- [ ] Street-level imagery integration
- [ ] 3D building model export
