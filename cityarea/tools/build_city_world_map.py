"""Build a static map and an offline interactive atlas from the city catalog."""

import html
import json
from pathlib import Path

import geonamescache
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib import font_manager
from matplotlib.patches import Polygon


ROOT = Path(__file__).resolve().parents[1]
REPORTS = ROOT / "reports"
COUNTRY_CODES = dict(zip(
    ["中国", "美国", "日本", "德国", "英国", "法国", "意大利", "西班牙", "波兰",
     "荷兰", "希腊", "比利时", "韩国", "丹麦", "瑞典", "芬兰", "葡萄牙", "挪威"],
    ["CN", "US", "JP", "DE", "GB", "FR", "IT", "ES", "PL", "NL", "GR", "BE",
     "KR", "DK", "SE", "FI", "PT", "NO"],
))
CHINA_REGIONS = dict(zip(
    ["安徽省", "浙江省", "江西省", "江苏省", "福建省", "河南省", "河北省", "湖南省",
     "湖北省", "甘肃省", "广西壮族自治区", "贵州省", "辽宁省", "内蒙古自治区",
     "北京市", "山西省", "山东省", "陕西省", "天津市", "云南省", "广东省", "海南省",
     "四川省", "重庆市"],
    ["01", "02", "03", "04", "07", "09", "10", "11", "12", "15", "16", "18",
     "19", "20", "22", "24", "25", "26", "28", "29", "30", "31", "32", "33"],
))
US_REGIONS = dict(zip(
    ["德克萨斯州", "马里兰州", "马萨诸塞州", "北卡罗来纳州", "伊利诺伊州",
     "俄亥俄州", "密歇根州", "印第安纳州", "佛罗里达州", "内华达州", "加利福尼亚州",
     "肯塔基州", "田纳西州", "亚利桑那州", "威斯康星州", "纽约州", "俄克拉何马州",
     "宾夕法尼亚州", "俄勒冈州", "华盛顿州", "哥伦比亚特区"],
    ["TX", "MD", "MA", "NC", "IL", "OH", "MI", "IN", "FL", "NV", "CA", "KY",
     "TN", "AZ", "WI", "NY", "OK", "PA", "OR", "WA", "DC"],
))
# Approximate geographic centers for districts or names absent from GeoNames.
OVERRIDES = {
    "Binhai": (39.003, 117.710),
    "Guangfeng": (28.437, 118.191),
    "Xiong'an": (38.990, 115.910),
}


def locate_cities():
    catalog = json.loads((REPORTS / "karstworldx4_city_countries.json").read_text(encoding="utf-8"))
    places = list(geonamescache.GeonamesCache().get_cities().values())
    result = []
    for city in catalog:
        name = city["name_en"]
        if name in OVERRIDES:
            lat, lon = OVERRIDES[name]
            origin = "approximate district center"
        else:
            code = COUNTRY_CODES[city["country"]]
            if city["state"] == "台湾省":
                code = "TW"
            elif city["state"] == "香港特别行政区":
                code = "HK"
            region = CHINA_REGIONS.get(city["state"]) if code == "CN" else (
                US_REGIONS.get(city["state"]) if code == "US" else None
            )
            candidates = [
                place for place in places
                if place["countrycode"] == code
                and (not region or place["admin1code"] == region)
                and (place["name"].casefold() == name.casefold()
                     or name.casefold() in [alias.casefold() for alias in place.get("alternatenames", [])])
            ]
            if not candidates:
                raise ValueError(f"No geographic location for {name}, {city['state']}")
            place = max(candidates, key=lambda p: (p["name"].casefold() == name.casefold(), p["population"]))
            lat, lon = place["latitude"], place["longitude"]
            origin = f"GeoNames {place['geonameid']}"
        result.append({key: city[key] for key in ("name", "name_en", "state", "country")}
                      | {"lat": lat, "lon": lon, "source": origin})
    return result


def polygons(world):
    for feature in world["features"]:
        geometry = feature["geometry"]
        if not geometry or feature["properties"]["name"] == "Antarctica":
            continue
        components = [geometry["coordinates"]] if geometry["type"] == "Polygon" else geometry["coordinates"]
        for component in components:
            yield component[0]


def create_image(cities, world):
    chinese_font = Path("C:/Windows/Fonts/msyh.ttc")
    if chinese_font.exists():
        font_manager.fontManager.addfont(str(chinese_font))
        plt.rcParams["font.family"] = font_manager.FontProperties(fname=str(chinese_font)).get_name()
    plt.rcParams["axes.unicode_minus"] = False
    fig, ax = plt.subplots(figsize=(18, 9), dpi=180)
    fig.patch.set_facecolor("#ffffff")
    ax.set_facecolor("#eaf4f7")
    for polygon in polygons(world):
        ax.add_patch(Polygon(polygon, closed=True, facecolor="#f4f6f2", edgecolor="#b9c6c8", linewidth=.45))
    ax.scatter([c["lon"] for c in cities], [c["lat"] for c in cities],
               s=20, c="#d45241", edgecolors="white", linewidths=.45, alpha=.92, zorder=4)
    ax.set_xlim(-180, 180)
    ax.set_ylim(-60, 85)
    ax.set_aspect("equal")
    ax.set_xticks(range(-180, 181, 60))
    ax.set_yticks(range(-60, 86, 30))
    ax.grid(color="#cddcde", linewidth=.5, alpha=.6)
    ax.tick_params(colors="#657878", labelsize=9, length=0, pad=7)
    for spine in ax.spines.values():
        spine.set_visible(False)
    fig.suptitle("世界城市分布", x=.07, y=.94, ha="left", fontsize=23, fontweight="bold", color="#183e40")
    fig.text(.07, .885, f"{len(cities)} 个城市  /  {len(set(c['country'] for c in cities))} 个国家",
             fontsize=12, color="#657878")
    fig.text(.07, .07, "点位为城市大致地理中心 · GeoNames / Natural Earth",
             fontsize=10, color="#657878")
    fig.subplots_adjust(left=.06, right=.97, bottom=.14, top=.84)
    fig.savefig(REPORTS / "city_world_map.png", dpi=180, facecolor=fig.get_facecolor())
    plt.close(fig)


def create_webpage(cities, world):
    paths = []
    for polygon in polygons(world):
        coordinates = [(round((lon + 180) * 1200 / 360, 2),
                        round((85 - lat) * 1200 / 360, 2)) for lon, lat in polygon]
        paths.append("M" + "L".join(f"{x},{y}" for x, y in coordinates) + "Z")
    template = (ROOT / "tools" / "city_world_map_template.html").read_text(encoding="utf-8")
    countries = sorted({city["country"] for city in cities})
    page = template.replace("__WORLD_PATHS__", "".join(f'<path d="{path}"/>' for path in paths))
    page = page.replace("__CITY_DATA__", json.dumps(cities, ensure_ascii=False).replace("</", "<\\/"))
    page = page.replace("__COUNTRIES__", "".join(
        f'<option value="{html.escape(country)}">{html.escape(country)}</option>' for country in countries))
    page = page.replace("__SUMMARY__", f"{len(cities)} 个城市 · {len(countries)} 个国家")
    page = page.replace("__STATIC_POINTS__", "".join(
        f'<circle cx="{(city["lon"] + 180) * 1200 / 360:.3f}" '
        f'cy="{(85 - city["lat"]) * 1200 / 360:.3f}" r="3.1" '
        f'fill="#d45241" stroke="white" stroke-width="0.7"><title>'
        f'{html.escape(city["name"])} · {html.escape(city["state"])} · '
        f'{html.escape(city["country"])}</title></circle>' for city in cities))
    if any(token in page for token in (
            "__WORLD_PATHS__", "__CITY_DATA__", "__COUNTRIES__", "__STATIC_POINTS__", "__SUMMARY__")):
        raise ValueError("Unresolved template placeholders")
    (REPORTS / "city_world_map.html").write_text(page, encoding="utf-8")
    (REPORTS / "city_world_map_offline.html").write_text(page, encoding="utf-8")


def main():
    cities = locate_cities()
    world = json.loads((REPORTS / "city_map_world.geojson").read_text(encoding="utf-8"))
    (REPORTS / "city_geographic_locations.json").write_text(
        json.dumps(cities, ensure_ascii=False, indent=2), encoding="utf-8")
    create_image(cities, world)
    create_webpage(cities, world)
    print(f"Created PNG and offline HTML for {len(cities)} cities.")


if __name__ == "__main__":
    main()
