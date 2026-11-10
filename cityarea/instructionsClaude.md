cityarea下的@image.png
  文件夹的图片是在QGIS读取OpenStreetMap的数据处理后得到的图。图显示了用代码处理后得到的多个区域层级划分等.
  @cityarea\README.md 展示了部分项目流程。主要关注 @city_process\filter_ways.py  （将有的建筑类型划分了出来），
  @city_process\street_block_process.py @overpass_query\query_city.py
  文件.主要是将从OpenStreetMap的到的gpkg数据处理成QGIS中一块一块的街区划分。请仔细阅读整个流程。

一块区域内，某种类型的建筑加起来的面积最大，就可以为该city_street_block的该区域Feature添上一个type类型，名称就为该建筑的名称。例如一块区域有School,residential,hospital,但是residential的建筑面积合起来都大于其余建筑的面积，那么该区域就为“居民区”，先从city_street_bolck的区域进行类型划分。注意使用相应的skill已经context-Engineer
