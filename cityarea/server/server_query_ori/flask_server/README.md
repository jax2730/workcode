# 城市模板网页工具
## 介绍
对所选城市收集数据并生成城市模板中间文件提供编辑器使用。
## 用法
1. 打开浏览器，输入网址 [http://192.168.5.189:5050/index](http://192.168.5.189:5050/index)
2. 输入所选城市与城市范围， 提交查询 

    ![输入所选城市与城市范围](./img/create_query.png)

3. 等待查询完成，可以根据任务id查看查询进度，完成后可以下载道路数据检查是否是所需城市；
    ![查询进度](./img/finish_query.png)

4. 查询城市地标，需要提交城市地标英文名称(需要与open street map中 name:en一致)与distantbuilding id,或包围盒大小，
    e.g.
    ``` json
    {
        "CCTV Headquarters": {
            "name": "CCTV Headquarters",
            "id": "199206"
        },
        "CITIC Tower": {
            "name": "CITIC Tower",
            "ExactBoundingBox":{
                "min":	[-60.001144, -89.400002, -60.001144],
				"max":	[60.001144, 535.516296, 60.001144]
            }
        }
    }
    ```
    查询完成可以下载地标数据，手动编辑以保证地标相对位置准确且互相不会重叠；
    ![下载地标数据](./img/query_landmark.png)
5. 输入需要进行道路简化的城市与简化tolerance 参数，提交查询(此步骤城市需以完成数据)；
    ![输入需要进行道路简化的城市与简化tolerance 参数](./img/create_simplify.png)
6. 等待查询完成，可以根据任务id查看查询进度，完成后可以下载道路数据检查是否符合预期，以调整tolerance参数；
    ![查询进度](./img/finish_simplify.png)
7. 选择城市并上传调整后的地标数据，下载添加地标后的路网，可使用GIS软件(如QGIS或ArcGIS)编辑不合理的道路，编辑后道路需保持端点对端点相交；
    ![选择城市并上传调整后的地标数据](./img/upload_landmark.png)
8. 选择城市并上传编辑后道路(如有编辑)，提交任务；
    ![选择城市并上传编辑后道路](./img/upload_road.png)
8. 等待任务完成，可以根据任务id查看查询进度，完成后可以下载生成的城市模板；
    ![查询进度](./img/download_result.png)
9. 通过编辑器的 `工具` > `KarstCityTemplateUI` > `File` > `Load City Map` 导入中间数据
    