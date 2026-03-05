import { fromFile } from "geotiff";
import { parse } from 'csv-parse';
import { createReadStream, createWriteStream } from 'fs';
import { Transform } from 'stream';

const height_map_path = "F:\\HeightField\\global\\tiles\\"
const MAP_OFFSET = [-2097152.0, -1048576.0]; // 地图0点偏移
const PIXEL_SCALE = 32.0; // 每像素代表的实际距离（米）

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

        // 规范化经度差，确保在 [-π, π] 范围内
        let deltaLon = lam - lam0;
        deltaLon = ((deltaLon + Math.PI) % (2 * Math.PI) + 2 * Math.PI) % (2 * Math.PI) - Math.PI;

        const sinp = Math.sin(phi);
        const term = Math.pow((1 - e * sinp) / (1 + e * sinp), e / 2);
        const x = a * k0 * deltaLon;
        const y = a * k0 * Math.log(Math.tan(PI / 4 + phi / 2) * term);

        return { x, y };
    }

    // 添加地理坐标转像素坐标的方法
    function geoToPixel(lon, lat) {
        // 先将 WGS84 转换为墨卡托坐标
        const mercator = lonLatToMercator2SP(lon, lat);

        // 然后将墨卡托坐标转换为像素坐标
        const pixelX = Math.round((mercator.x - origin.x) / pixelScale[0]);
        const pixelY = Math.round((origin.y - mercator.y) / pixelScale[1]); // 注意Y方向是减法
        return { pixelX, pixelY };
    }

    return { origin, pixelScale, geoToPixel };
}

async function ConvertPosition(lon, lat, geoToPixel) {

    const { pixelX, pixelY } = geoToPixel(lon, lat);
    return {
        x: (pixelX - 1) * PIXEL_SCALE + MAP_OFFSET[0],
        y: (pixelY - 1) * PIXEL_SCALE + MAP_OFFSET[1]
    }
}

async function processCSVFile(inputPath, outputPath, geoToPixel) {
    return new Promise((resolve, reject) => {
        const output = createWriteStream(outputPath);
        let isHeader = true;

        // 创建转换流
        const transformer = new Transform({
            objectMode: true,
            async transform(row, encoding, callback) {
                try {
                    if (isHeader) {
                        // 处理表头，添加新列名
                        row.push('转换后X', '转换后Y');
                        isHeader = false;
                    } else {
                        // 获取第6、7列的经纬度（索引为5、6）
                        const lon = parseFloat(row[5]);
                        const lat = parseFloat(row[6]);

                        // 调用 ConvertPosition 进行坐标转换
                        const pos = await ConvertPosition(lon, lat, geoToPixel);

                        // 添加转换后的坐标到新列
                        row.push(pos.x.toString(), pos.y.toString());
                    }

                    // 将行数据转换为 CSV 格式
                    const csvLine = row.join(',') + '\n';
                    callback(null, csvLine);
                } catch (error) {
                    callback(error);
                }
            }
        });

        // 创建解析器
        const parser = parse({
            delimiter: ',',
            from_line: 1
        });

        // 设置管道和错误处理
        createReadStream(inputPath)
            .pipe(parser)
            .pipe(transformer)
            .pipe(output)
            .on('finish', resolve)
            .on('error', reject);
    });
}

// 修改 main 函数以处理 CSV 文件
async function main() {
    try {
        const baseHeightField = await loadHeightField(1, 1);
        const { geoToPixel } = await extractGeoTransform(baseHeightField);

        // 处理 CSV 文件

        const inputPath = 'GIS/coord/';  // 输入文件路径
        const fileName = 'DNK_adm2_centroids';

        await processCSVFile(inputPath + fileName + '.csv', inputPath + fileName + '_converted' + '.csv', geoToPixel);
        console.log('CSV 文件处理完成');
        
    } catch (err) {
        console.error('处理过程中出错:', err);
    }
}

main();