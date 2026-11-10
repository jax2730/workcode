/*
Overpass Turbo 模板：以中心城市为锚点，查询周边候选城市（节点 + 行政边界）

使用方法：
1) 把 CITY_NAME 改成目标城市（如 Beijing）
2) 把 MAX_RADIUS 改成最大搜索半径（米）
3) 运行后导出 JSON，在 Python 脚本里按距离排序取前 N（7~8 个）

注意：Overpass QL 本身不支持“按距离排序后直接取前 N”，
因此这里输出候选集合，前 N 由 query_nearby_cities.py 完成。
*/

[out:json][timeout:180][maxsize:1073741824];

/* ===== 参数区（手动替换） ===== */
/* CITY_NAME = Beijing */
/* MAX_RADIUS = 300000 */

(
  node[place~"city|town"]["name:en"="Beijing"];
  node[place~"city|town"]["name"="Beijing"];
  node[place~"city|town"]["name:zh"="Beijing"];
)->.center_city;

(
  node[place~"city|town|municipality"](around.center_city:300000);
  relation["boundary"="administrative"]["admin_level"~"4|5|6"](around.center_city:300000);
);

out tags center;