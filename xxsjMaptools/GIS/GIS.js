import { fromFile } from "geotiff";
import * as fspromise from 'node:fs/promises';
import shp from 'shpjs';

const height_map_path = "F:\\HeightField\\global\\tiles\\"
const EFFECTIVE_SIZE = 1024;// 每张图片的有效像素数（不包括重叠部分）
const OVERLAP = 3; // 重叠区域的像素数
const MAP_OFFSET = [-2097152.0, -1048576.0]; // 地图0点偏移
const PIXEL_SCALE = 32.0; // 每像素代表的实际距离（米）
const HEIGHT_SCALE = 1.0 / 8.0; // 高程缩放比例

function padZero(num) {
    return num.toString().padStart(3, '0');
}

function getHeightFileName(x, y) {
    return height_map_path + "worldmap_fill_" + padZero(y) + "_" + padZero(x) + ".tif";
}

function loadHeightField(x, y) {
    return fromFile(getHeightFileName(x, y));
}

async function extractGeoTransform(tiff) {
    const image = await tiff.getImage();
    // 获取仿射变换参数
    const tiePoints = image.getTiePoints();
    const pixelScale = image.getFileDirectory().ModelPixelScale;
    const origin = tiePoints[0];

    const geoKeys = image.getGeoKeys();
    const projParams = image.getFileDirectory().GeoDoubleParams || [];

    // 预先将整张影像的栅格数据加载到内存，节省后续逐像素 IO
    const width = image.getWidth();
    const height = image.getHeight();
    const fullRasters = await image.readRasters(); // 可能是 TypedArray 或 [band0, band1,...]
    let band0Array;
    if (Array.isArray(fullRasters)) {
        band0Array = fullRasters[0];
    } else {
        band0Array = fullRasters;
    }
    // 确保 band0Array 是可索引的一维数组，长度应该为 width*height
    // 若不是则在读取时退回到按需读取（兼容性保障）
    const useInMemory = band0Array && band0Array.length >= width * height;

    // WGS84椭球体参数
    const WGS84 = {
        a: 6378137.0,               // semi-major axis (meters)
        rf: 298.257223563,          // inverse flattening
    };

    const PI = Math.PI;
    const deg2rad = d => d * PI / 180;
    // 提取投影参数
    let centralMeridian = 150.0; // 默认值
    let standardParallel = 60.0; // 默认值
    let semiMajorAxis = WGS84.a; // 默认值
    let inverseFlattening = WGS84.rf; // 默认值

    // 遍历 GeoKeys 寻找投影参数
    if (geoKeys) {
        // 中央经度线通常存储在 ProjCenterLong 或 ProjNatOriginLong 中
        centralMeridian = projParams[geoKeys.ProjCenterLongGeoKey] ||
            projParams[geoKeys.ProjNatOriginLongGeoKey] ||
            centralMeridian;

        // 标准纬度线通常存储在 ProjStdParallel1 中
        standardParallel = projParams[geoKeys.ProjStdParallel1GeoKey] ||
            standardParallel;

        // 椭球体参数
        semiMajorAxis = projParams[geoKeys.GeogSemiMajorAxisGeoKey] ||
            semiMajorAxis;
        inverseFlattening = projParams[geoKeys.GeogInvFlatteningGeoKey] ||
            inverseFlattening;
    }

    // 第一偏心率 e
    function eccentricity() {
        const f = 1.0 / inverseFlattening;
        return Math.sqrt(2 * f - f * f);
    }

    function k0FromLatTS(lat_ts_deg) {
        const e = eccentricity();
        const phi1 = deg2rad(lat_ts_deg);
        return Math.cos(phi1) / Math.sqrt(1 - e * e * Math.sin(phi1) ** 2);
    }

    function lonLatToMercator2SP(lon_deg, lat_deg) {
        const lat_ts = standardParallel;
        const lon0 = centralMeridian;

        // 避免数值发散
        if (lat_deg > 89.5) lat_deg = 89.5;
        if (lat_deg < -89.5) lat_deg = -89.5;

        const e = eccentricity();
        const k0 = k0FromLatTS(lat_ts);
        const a = semiMajorAxis;

        const lam = deg2rad(lon_deg);
        const phi = deg2rad(lat_deg);
        const lam0 = deg2rad(lon0);

        const sinp = Math.sin(phi);
        const term = Math.pow((1 - e * sinp) / (1 + e * sinp), e / 2);
        // 处理经度差值，确保在 [-π, π] 范围内
        let deltaLon = lam - lam0;
        while (deltaLon > Math.PI) deltaLon -= 2 * Math.PI;
        while (deltaLon < -Math.PI) deltaLon += 2 * Math.PI;
        const x = a * k0 * deltaLon;
        const y = a * k0 * Math.log(Math.tan(PI / 4 + phi / 2) * term);

        return { x, y };
    }

    // 添加地理坐标转像素坐标的方法
    function geoToPixel(lon, lat) {
        // 先将 WGS84 转换为墨卡托坐标
        const mercator = lonLatToMercator2SP(lon, lat);

        // 然后将墨卡托坐标转换为像素坐标
        const pixelX = (mercator.x - origin.x) / pixelScale[0];
        const pixelY = (origin.y - mercator.y) / pixelScale[1]; // 注意Y方向是减法
        return { pixelX, pixelY };
    }

    // 直接从当前打开的 tiff 中读取高度（使用内存中的 rasters，若不可用则回退到按需读取）
    async function getHeightAtLonLat(lon, lat) {
        const { pixelX, pixelY } = geoToPixel(lon, lat);

        // 转为浮点像素坐标并约束到图像范围（允许在边界处插值）
        let fx = pixelX;
        let fy = pixelY;
        if (fx < 0) fx = 0;
        if (fy < 0) fy = 0;
        if (fx > width - 1) fx = width - 1;
        if (fy > height - 1) fy = height - 1;

        // 邻近像素索引
        const x0 = Math.floor(fx);
        const y0 = Math.floor(fy);
        const x1 = x0 + 1 <= width - 1 ? x0 + 1 : x0;
        const y1 = y0 + 1 <= height - 1 ? y0 + 1 : y0;

        // 分数部分
        const dx = (x1 === x0) ? 0 : (fx - x0);
        const dy = (y1 === y0) ? 0 : (fy - y0);

        try {
            let v00 = 0, v10 = 0, v01 = 0, v11 = 0;
            if (useInMemory) {
                // 直接从内存数组索引
                const idx00 = y0 * width + x0;
                const idx10 = y0 * width + x1;
                const idx01 = y1 * width + x0;
                const idx11 = y1 * width + x1;
                v00 = band0Array[idx00] !== undefined ? band0Array[idx00] : 0;
                v10 = band0Array[idx10] !== undefined ? band0Array[idx10] : 0;
                v01 = band0Array[idx01] !== undefined ? band0Array[idx01] : 0;
                v11 = band0Array[idx11] !== undefined ? band0Array[idx11] : 0;
            } else {
                // 回退：逐像素从 image 读取（兼容性保障）
                async function readPixel(px, py) {
                    let rx = px;
                    let ry = py;
                    if (rx < 0) rx = 0;
                    if (ry < 0) ry = 0;
                    if (rx >= width) rx = width - 1;
                    if (ry >= height) ry = height - 1;
                    const ras = await image.readRasters({ window: [rx, ry, rx + 1, ry + 1] });
                    if (Array.isArray(ras)) {
                        const b0 = ras[0];
                        return b0 && b0.length > 0 ? b0[0] : 0;
                    } else if (ras && ras.length > 0) {
                        return ras[0];
                    } else if (typeof ras === 'number') {
                        return ras;
                    }
                    return 0;
                }
                v00 = await readPixel(x0, y0);
                v10 = await readPixel(x1, y0);
                v01 = await readPixel(x0, y1);
                v11 = await readPixel(x1, y1);
            }

            // 双线性插值
            const i0 = v00 * (1 - dx) + v10 * dx;
            const i1 = v01 * (1 - dx) + v11 * dx;
            const value = i0 * (1 - dy) + i1 * dy;

            return value;
        } catch (err) {
            return 0;
        }
    }

    return { origin, pixelScale, geoToPixel, getHeightAtLonLat };
}

// 修改 processShapefile 函数
async function saveLineDataFromShapeFile(shpPath, geoToPixel, outputFile) {
    // 读取 shapefile
    const geojson = await shp(await fspromise.readFile(shpPath));

    const lines = [];// 用于存储所有折线
    // 处理每个要素
    let index = 0;
    for (const feature of geojson.features) {
        if(feature.geometry == null) {
            continue;
        }
        const geometryType = feature.geometry.type;
        let lineStrings = [];
        console.log(`Processing feature ${index++} of type ${geometryType}`);
        // 根据几何类型获取坐标数组
        if (geometryType === 'LineString') {
            lineStrings = [feature.geometry.coordinates];
        } else if (geometryType === 'MultiLineString') {
            lineStrings = feature.geometry.coordinates;
        } else {
            continue;
        }

        // 处理每条折线
        for (const coordinates of lineStrings) {
            const line = [];
            for (const [lon, lat] of coordinates) {
                const { pixelX, pixelY } = geoToPixel(lon, lat);
                line.push({ x: pixelX, y: pixelY });
            }
            lines.push(line);
        }
    }

    var totalSize = lines.reduce((sum, line) => {
        const pointCount = line.length;
        return sum + 4 + (pointCount * 8); // 4 bytes for count + 8 bytes per point
    }, 0);
    totalSize += 4; // 额外的4字节用于标识文件头（是否含高度），之后写入总折线数
    totalSize += 4; // 写入总折线数

    // 创建缓冲区
    const buffer = Buffer.alloc(totalSize);
    let offset = 0;

    // 文件头：写入是否含高度的标识（0 = 无高度）
    buffer.writeInt32LE(0, offset);
    offset += 4;

    // 写入总折线数（4字节整数）
    buffer.writeInt32LE(lines.length, offset);
    offset += 4;
    
    // 写入每条折线的数据
    for (const line of lines) {
        const pointCount = line.length;

        // 写入点的数量（4字节整数）
        buffer.writeInt32LE(pointCount, offset);
        offset += 4;

        // 写入每个点的坐标（每个坐标2个4字节浮点：x,y）
        for (const { x, y } of line) {
            buffer.writeFloatLE((x - 1) * PIXEL_SCALE + MAP_OFFSET[0], offset);
            buffer.writeFloatLE((y - 1) * PIXEL_SCALE + MAP_OFFSET[1], offset + 4);
            offset += 8;
        }
    }

    // 保存二进制文件
    await fspromise.writeFile(
        outputFile,
        buffer
    );
}

async function saveLineDataWithHeightFromShapeFile(shpPath, geoToPixel, getHeightAtLonLat, outputFile) {
    // 读取 shapefile
    const geojson = await shp(await fspromise.readFile(shpPath));

    const lines = [];// 用于存储所有折线
    // 处理每个要素
    for (const feature of geojson.features) {
        if(feature.geometry == null) {
            continue;
        }
        const geometryType = feature.geometry.type;
        let lineStrings = [];

        // 根据几何类型获取坐标数组
        if (geometryType === 'LineString') {
            lineStrings = [feature.geometry.coordinates];
        } else if (geometryType === 'MultiLineString') {
            lineStrings = feature.geometry.coordinates;
        } else {
            continue;
        }

        // 处理每条折线
        for (const coordinates of lineStrings) {
            const line = [];
            for (const [lon, lat] of coordinates) {
                const { pixelX, pixelY } = geoToPixel(lon, lat);
                let z = 0;
                try {
                    if (typeof getHeightAtLonLat === 'function') {
                        z = await getHeightAtLonLat(lon, lat);
                    }
                } catch (err) {
                    z = 0;
                }
                line.push({ x: pixelX, y: pixelY, z });
            }
            lines.push(line);
        }
    }

    // 计算总大小：4字节标志 + 4字节总折线数 + 每条：4字节点数 + 每点 12 字节（x,y,z）
    var totalSize = lines.reduce((sum, line) => {
        const pointCount = line.length;
        return sum + 4 + (pointCount * 12); // 4 bytes for count + 12 bytes per point (3 floats)
    }, 0);
    totalSize += 4; // 额外的4字节用于标识文件头（是否含高度）
    totalSize += 4; // 写入总折线数

    // 创建缓冲区
    const buffer = Buffer.alloc(totalSize);
    let offset = 0;

    // 文件头：写入是否含高度的标识（1 = 含高度）
    buffer.writeInt32LE(1, offset);
    offset += 4;

    // 写入总折线数（4字节整数）
    buffer.writeInt32LE(lines.length, offset);
    offset += 4;

    // 写入每条折线的数据
    for (const line of lines) {
        const pointCount = line.length;

        // 写入点的数量（4字节整数）
        buffer.writeInt32LE(pointCount, offset);
        offset += 4;

        // 写入每个点的坐标（每个坐标3个4字节浮点：x,y,z）
        for (const { x, y, z } of line) {
            buffer.writeFloatLE((x - 1) * PIXEL_SCALE + MAP_OFFSET[0], offset);
            buffer.writeFloatLE(z * HEIGHT_SCALE, offset + 4); // 高度值按比例缩放后写入
            buffer.writeFloatLE((y - 1) * PIXEL_SCALE + MAP_OFFSET[1], offset + 8);
            offset += 12;
        }
    }

    // 保存二进制文件
    await fspromise.writeFile(
        outputFile,
        buffer
    );
}

// 使用示例：
async function main() {
    try {
        const baseTile = await loadHeightField(1, 1);
        const baseHeightField = await fromFile("E:/show/WorldMap/worldhm.tif");
        const { geoToPixel } = await extractGeoTransform(baseTile);
        const { getHeightAtLonLat } = await extractGeoTransform(baseHeightField);

        //const shpPath = 'F:\\HeightField\\global\\road.zip';
        //await saveLineDataFromShapeFile(shpPath, geoToPixel, "china_shp/road_lines.bin");
        //await saveLineDataWithHeightFromShapeFile(shpPath, geoToPixel, getHeightAtLonLat, "china_shp/road_lines_with_height.bin");

        const shpPath = 'F:\\HeightField\\global\\river.zip';
        await saveLineDataWithHeightFromShapeFile(shpPath, geoToPixel, getHeightAtLonLat, "china_shp/water_lines_with_height.bin");
        console.log('转换完成，文件已保存');
    } catch (err) {
        console.error('处理过程中出错:', err);
    }
}

main();
