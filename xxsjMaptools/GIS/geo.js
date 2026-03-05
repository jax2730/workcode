import { fromFile } from "geotiff";
import * as fspromise from 'node:fs/promises';
import fs from "fs"
import shp from 'shpjs';
import { normalize } from "node:path";

//const pixelSize = 32768 / 1023.0
const pixelSize = 32.0
//river:0,road:1
const dataType = 1

const xOffset = -262000
const zOffset = -163840
//const xOffset = 0
//const zOffset = 0

const lerp = (a, b, t) => (1 - t) * a + t * b;

function transform(a, b, M, roundToInt = false) {
  const round = (v) => (roundToInt ? v | 0 : v);
  return [
    round(M[0] + M[1] * a + M[2] * b),
    round(M[3] + M[4] * a + M[5] * b),
  ];
}

function isInChina(bbox, matrix) {
  let leftdown = transform(bbox[0], bbox[1], matrix, true);
  let righttop = transform(bbox[2], bbox[3], matrix, true);
  return !(leftdown[0] > 16384 || righttop[0] < 0 || leftdown[1] < 0 || righttop[1] > 13291)
}

function getHeight(raster, index) {
  if(dataType == 0){
    return raster[index] / 6.0 - 10.0
  }
  else if(dataType == 1){
    return raster[index] / 6.0 + 2.0
  }
}

function interValue(p, width, raster, curHeight) {
  let px0 = Math.floor(p[0])
  let px1 = px0 + 1;
  let py0 = Math.floor(p[1])
  let py1 = py0 + 1;

  let h0 = getHeight(raster, py0 * width + px0)
  let h1 = getHeight(raster, py1 * width + px0)
  let h2 = getHeight(raster, py0 * width + px1)
  let h3 = getHeight(raster, py1 * width + px1)

  let u = p[0] - px0;
  let v = p[1] - py0;

  let v1 = lerp(h0, h1, v)
  let v2 = lerp(h2, h3, v)
  let h = lerp(v1, v2, u)
  if(dataType == 0){
    return Math.min(h, curHeight)
  }
  else if(dataType == 1){
    return h;
  }
}

function calculatePosition(coordinate, matrix, width, raster, curHeight) {
  let p = transform(coordinate[0], coordinate[1], matrix, false);
  let height = interValue(p, width, raster, curHeight)
  let pos = []
  pos.length = 3
  pos[0] = p[0] * pixelSize + xOffset
  pos[1] = height
  pos[2] = p[1] * pixelSize + zOffset
  return { pos, height }
}

function calculatePlanePosition(coordinate, matrix) {
  let p = transform(coordinate[0], coordinate[1], matrix, false);
  let pos = []
  pos.length = 2
  pos[0] = p[0] * pixelSize + xOffset
  pos[1] = p[1] * pixelSize + zOffset
  return { pos }
}

function writeStringLineToStream(count, coordinates, stream, matrix, width, raster) {

  const buffer = Buffer.alloc(count * 12 + 4);
  let offset = 0;
  buffer.writeInt32LE(count, offset);
  offset += 4
  let curHeight = 100000;
  for (let i = 0; i < count; i++) {
    let coordinate = coordinates[i]
    let { pos, height } = calculatePosition(coordinate, matrix, width, raster, 100000)
    curHeight = height
    buffer.writeFloatLE(pos[0], offset);
    offset += 4
    buffer.writeFloatLE(pos[1], offset);
    offset += 4
    buffer.writeFloatLE(pos[2], offset);
    offset += 4
  }
  stream.write(buffer);
}

function normalizeVector(vector) {
  // 计算向量的长度（欧几里得范数）
  const length = Math.sqrt(vector.reduce((sum, component) => sum + component ** 2, 0));

  // 避免除以零的情况
  if (length === 0) {
    return [1, 0, 0]
  }

  // 将每个分量除以长度，返回新的单位向量
  return vector.map(component => component / length);
}

function caculatePlaneNormal(p0, p1) {
  let line = [p1[0] - p0[0], p1[2] - p0[2]]
  let nLine = normalizeVector(line)
  return [nLine[1], - nLine[0]]
}

function caculateAverageNormal(n0, n1) {
  let n = [n0[0] + n1[0], n0[1] + n1[1]]
  return normalizeVector(n)
}

function writeTriangleToStream(p0, p1, p2, buffer, offset) {
  buffer.writeFloatLE(p0[0], offset);
  offset += 4
  buffer.writeFloatLE(p0[1], offset);
  offset += 4
  buffer.writeFloatLE(p0[2], offset);
  offset += 4

  buffer.writeFloatLE(p1[0], offset);
  offset += 4
  buffer.writeFloatLE(p1[1], offset);
  offset += 4
  buffer.writeFloatLE(p1[2], offset);
  offset += 4

  buffer.writeFloatLE(p2[0], offset);
  offset += 4
  buffer.writeFloatLE(p2[1], offset);
  offset += 4
  buffer.writeFloatLE(p2[2], offset);
  offset += 4

  return offset
}

function writePolygonTriangleToStream(p0, p1, p2, height, buffer, offset) {
  buffer.writeFloatLE(p0.x, offset);
  offset += 4
  buffer.writeFloatLE(height, offset);
  offset += 4
  buffer.writeFloatLE(p0.y, offset);
  offset += 4

  buffer.writeFloatLE(p1.x, offset);
  offset += 4
  buffer.writeFloatLE(height, offset);
  offset += 4
  buffer.writeFloatLE(p1.y, offset);
  offset += 4

  buffer.writeFloatLE(p2.x, offset);
  offset += 4
  buffer.writeFloatLE(height, offset);
  offset += 4
  buffer.writeFloatLE(p2.y, offset);
  offset += 4

  return offset
}

function writePointToStream(p0, buffer, offset) {
  buffer.writeFloatLE(p0.x, offset);
  offset += 4
  buffer.writeFloatLE(p0.y, offset);
  offset += 4
  buffer.writeFloatLE(p0.z, offset);
  offset += 4
  return offset
}

const riverHalfWidth = 10.0
const lakeExpandWidth = 0.0
function writeTriangleListToStream(count, coordinates, stream, matrix, width, raster) {
  let lineNum = count - 1
  let triangleNum = lineNum * 4

  const buffer = Buffer.alloc(triangleNum * 3 * 12 + 4);
  let offset = 0;
  buffer.writeInt32LE(triangleNum, offset);
  offset += 4
  let { pos, height } = calculatePosition(coordinates[0], matrix, width, raster, 10000)
  let p0 = pos
  let h0 = height
  let ln
  for (let i = 1; i < count; i++) {
    ({ pos, height } = calculatePosition(coordinates[i], matrix, width, raster, h0))
    let p1 = pos
    let h1 = height

    if (i + 1 != count) {
      ({ pos, height } = calculatePosition(coordinates[i + 1], matrix, width, raster, h1))
      let p2 = pos
      let h2 = height
      let n0 = caculatePlaneNormal(p0, p1)
      let n1 = caculatePlaneNormal(p1, p2)
      let normal = caculateAverageNormal(n0, n1)
      if (i == 1) {
        ln = n0
      }
      let p0l = [p0[0] + ln[0] * riverHalfWidth, h0, p0[2] + ln[1] * riverHalfWidth]
      let p1l = [p1[0] + normal[0] * riverHalfWidth, h1, p1[2] + normal[1] * riverHalfWidth]
      let p0r = [p0[0] - ln[0] * riverHalfWidth, h0, p0[2] - ln[1] * riverHalfWidth]
      let p1r = [p1[0] - normal[0] * riverHalfWidth, h1, p1[2] - normal[1] * riverHalfWidth]

      offset = writeTriangleToStream(p0, p1, p0l, buffer, offset)
      offset = writeTriangleToStream(p1, p1l, p0l, buffer, offset)
      offset = writeTriangleToStream(p0, p0r, p1r, buffer, offset)
      offset = writeTriangleToStream(p0, p1r, p1, buffer, offset)

      p0 = p1
      h0 = h1
      ln = normal
    }
    else {
      let normal = caculatePlaneNormal(p0, p1)
      if (i == 1) {
        ln = normal
      }
      let p2 = [p0[0] + ln[0] * riverHalfWidth, h0, p0[2] + ln[1] * riverHalfWidth]
      let p3 = [p1[0] + normal[0] * riverHalfWidth, h1, p1[2] + normal[1] * riverHalfWidth]
      let p4 = [p0[0] - ln[0] * riverHalfWidth, h0, p0[2] - ln[1] * riverHalfWidth]
      let p5 = [p1[0] - normal[0] * riverHalfWidth, h1, p1[2] - normal[1] * riverHalfWidth]

      offset = writeTriangleToStream(p0, p1, p2, buffer, offset)
      offset = writeTriangleToStream(p1, p3, p2, buffer, offset)
      offset = writeTriangleToStream(p0, p4, p5, buffer, offset)
      offset = writeTriangleToStream(p0, p5, p1, buffer, offset)
    }
  }
  stream.write(buffer);
}


function getMinHeight(coordinates, matrix, width, raster) {
  let minHeight = 10000;
  for (let i = 0; i < coordinates.length; i++) {
    let p = transform(coordinates[i][0], coordinates[i][1], matrix, false);
    minHeight = Math.min(minHeight, interValue(p, width, raster, minHeight))
  }
  return minHeight
}

import { Point, triangulate } from "./triangulation.js"

function isClockwise(points) {
  /**
   * 判断多边形的点序是否为顺时针。
   * 
   * @param {Array} points - 多边形顶点的坐标数组，格式为 [[x1, y1], [x2, y2], ...]
   * @returns {Boolean} - true 表示点序为顺时针，false 表示点序为逆时针
   */
  let sum = 0;
  const n = points.length;

  for (let i = 0; i < n; i++) {
    // 当前点和下一个点
    const [x1, y1] = points[i];
    const [x2, y2] = points[(i + 1) % n]; // 循环到第一个点
    // 计算叉积
    sum += (x2 - x1) * (y2 + y1);
  }

  // sum > 0 表示顺时针，sum < 0 表示逆时针
  return sum > 0;
}

function writePointStream(count, coordinates, stream, matrix, width, raster) {

}

function writePolygonBoxToStream(count, coordinates, stream, matrix, width, raster) {
  if(count < 3) return
  const sizeExpand = 200.0
  let minX = Number.MAX_VALUE, minY = Number.MAX_VALUE, maxX = -Number.MAX_VALUE, maxY = -Number.MAX_VALUE
  let minHeight = getMinHeight(coordinates, matrix, width, raster)
  for (let i = 0; i < count; i++) {
    let { pos } = calculatePlanePosition(coordinates[i], matrix)
    minX = Math.min(minX, pos[0])
    minY = Math.min(minY, pos[1])
    maxX = Math.max(maxX, pos[0])
    maxY = Math.max(maxY, pos[1])
  }
  minX -= sizeExpand
  minY -= sizeExpand
  maxX += sizeExpand
  maxY += sizeExpand

  let triangleNum = 2
  const buffer = Buffer.alloc(triangleNum * 3 * 12 + 4);
  let offset = 0;
  buffer.writeInt32LE(triangleNum, offset);
  offset += 4

  let p0 = new Point(minX, minY, 0), p1 = new Point(minX, maxY, 0), p2 = new Point(maxX, maxY, 0), p3 = new Point(maxX,minY, 0)

  offset = writePolygonTriangleToStream(p0, p1, p2, minHeight, buffer, offset)
  offset = writePolygonTriangleToStream(p0, p2, p3, minHeight, buffer, offset)

  stream.write(buffer);
}

function writePolygonToStream(count, coordinates, stream, matrix, width, raster) {
  if(count < 3) return

  let positions = []
  let minHeight = getMinHeight(coordinates, matrix, width, raster)
  for (let i = 0; i < count; i++) {
    let { pos } = calculatePlanePosition(coordinates[i], matrix)
    positions.push(pos)
  }

  if (!isClockwise(positions)) {
    positions.reverse();
  }

  for (let i = 0; i < count; i++) {
    let p0,p1,p2
    if(i == 0)
    {
      p0 = [positions[count - 2][0], 0, positions[count - 2][1]]
      p1 = [positions[i][0], 0, positions[i][1]]
      p2 = [positions[i + 1][0], 0, positions[i + 1][1]]
    }
    else if(i == count - 1){
      p0 = [positions[i - 1][0], 0, positions[i - 1][1]]
      p1 = [positions[i][0], 0, positions[i][1]]
      p2 = [positions[1][0], 0, positions[1][1]]
    }
    else{
      p0 = [positions[i - 1][0], 0, positions[i - 1][1]]
      p1 = [positions[i][0], 0, positions[i][1]]
      p2 = [positions[i + 1][0], 0, positions[i + 1][1]]
    }
      let n0 = caculatePlaneNormal(p0, p1)
      let n1 = caculatePlaneNormal(p1, p2)
      let normal = caculateAverageNormal(n0, n1)
      positions[i][0] -= normal[0] * lakeExpandWidth;
      positions[i][1] -= normal[1] * lakeExpandWidth;
  }

  let triangles = triangulate(positions, false)
  let triangleNum = triangles.length
  const buffer = Buffer.alloc(triangleNum * 3 * 12 + 4);
  let offset = 0;
  buffer.writeInt32LE(triangleNum, offset);
  offset += 4
  for (let i = 0; i < triangleNum; i++) {
    let triangle = triangles[i]
    offset = writePolygonTriangleToStream(triangle.a, triangle.b, triangle.c, minHeight, buffer, offset)
  }

  stream.write(buffer);
}

function pushToStream(geo, stream, matrix, width, raster, writeBox = false) {
  if (geo.type == "LineString") {
    let count = geo.coordinates.length;
    //writeTriangleListToStream(count, geo.coordinates, stream, matrix, width, raster)
    writeStringLineToStream(count, geo.coordinates, stream, matrix, width, raster)
  }
  else if (geo.type == "MultiLineString") {
    for (let i = 0; i < geo.coordinates.length; i++) {
      let count = geo.coordinates[i].length;
      //writeTriangleListToStream(count, geo.coordinates[i], stream, matrix, width, raster)
      writeStringLineToStream(count, geo.coordinates[i], stream, matrix, width, raster)
    }
  }
  else if (geo.type == "Polygon") {
    for (let i = 0; i < geo.coordinates.length; i++) {
      let count = geo.coordinates[i].length;
      if(writeBox){
        writePolygonBoxToStream(count, geo.coordinates[i], stream, matrix, width, raster)
      }
      else  {
        writePolygonToStream(count, geo.coordinates[i], stream, matrix, width, raster)
      }
    }
  }
  else if (geo.type == "MultiPolygon") {
    for (let i = 0; i < geo.coordinates.length; i++) {
      for (let j = 0; j < geo.coordinates[i].length; j++) {
        let count = geo.coordinates[i][j].length;
        if(writeBox){
          writePolygonBoxToStream(count, geo.coordinates[i][j], stream, matrix, width, raster)
        }
        else  {
          writePolygonToStream(count, geo.coordinates[i][j], stream, matrix, width, raster)
        }
      }
    }
  }
  else {
    console.log(geo.type);
  }
}

function addPointToObjStream(p, height, content)
{
  content += `v ${p[0]} ${height} ${p[2]}\n`;
}

function addLineToObjStream(i0, i1, content)
{
  content += `l ${i0} ${i1}\n`;
}

function writeToObjContent(count, coordinates, matrix, width, raster, points, lines, index)
{
    //let h0 = 10000
    for (let i = 0; i < count; i++) {
      let { pos, height } = calculatePosition(coordinates[i], matrix, width, raster, 10000)
      //h0 = height;
      points.value += `v ${pos[0]} ${height} ${pos[2]}\n`;
      if(i > 0){
        lines.value += `l ${index} ${index + 1}\n`;
      }
      index++;
    }
    return index;
}

function pushToObjectStream(geojson, stream, matrix, width, raster) {
  let points = { value: "" };
  let lines = { value: "" };
  let index = 0;
  let geoCount = geojson.features.length;
  for (let i = 0; i < geoCount; i++) {
    let feature = geojson.features[i];
    if (isInChina(feature.geometry.bbox, matrix)) {
      let geo = feature.geometry;
      if (geo.type == "LineString") {
        let count = geo.coordinates.length;  
        index = writeToObjContent(count, geo.coordinates, matrix, width, raster, points, lines, index)
      }
      else if (geo.type == "MultiLineString") {
        for (let i = 0; i < geo.coordinates.length; i++) {
          let count = geo.coordinates[i].length;
          index = writeToObjContent(count, geo.coordinates[i], matrix, width, raster, points, lines, index)
        }
      }
    }
  }

  stream.write(points.value)
  stream.write(lines.value)
}

try {
  //const data = await fspromise.readFile("./国道.zip");
  //const data = await fspromise.readFile("./普通城市驻地.zip");
  //const geojson = await shp(data);
  const tiff = await fromFile('F:\\HeightField\\global\\tiles\\worldmap_fill_022_102.tif');
  //const tiff = await fromFile('E:\\show\\地貌\\新建文件夹\\dimao.tif');
  const image = await tiff.getImage();
  //const width = image.getWidth();
  const height = image.getHeight();
  const tileWidth = image.getTileWidth();
  const tileHeight = image.getTileHeight();
  const samplesPerPixel = image.getSamplesPerPixel();

  const { ModelPixelScale: s, ModelTiepoint: t } = image.fileDirectory;
  let [sx, sy, sz] = s;
  let [px, py, k, gx, gy, gz] = t;
  sy = -sy; // WGS-84 tiles have a "flipped" y component

  const pixelToGPS = [gx, sx, 0, gy, 0, sy];
  console.log(`pixel to GPS transform matrix:`, pixelToGPS);

  const gpsToPixel = [-gx / sx, 1 / sx, 0, -gy / sy, 0, 1 / sy];
  console.log(`GPS to pixel transform matrix:`, gpsToPixel);

  // // Convert a GPS coordinate to a pixel coordinate in our tile:
  // const [gx1, gy1, gx2, gy2] = image.getBoundingBox();
  // const lat = lerp(gy1, gy2, Math.random());
  // const long = lerp(gx1, gx2, Math.random());
  // console.log(`Looking up GPS coordinate (${lat.toFixed(6)},${long.toFixed(6)})`);

  // const [x, y] = transform(gx1, gy1, gpsToPixel, true);
  // console.log(`Corresponding tile pixel coordinate: [${x}][${y}]`);

  // const [x1, y1] = transform(gx2, gy2, gpsToPixel, true);
  // console.log(`Corresponding tile pixel coordinate: [${x1}][${y1}]`);

  const rasters = await image.readRasters();
  const { width, [0]: raster } = rasters;

  // let count = geojson[1].features.length;
  // const writableStream = fs.createWriteStream("city_output1.txt");

  let count = geojson.features.length;
  //const objStream = fs.createWriteStream("road_output.obj");
  const writableStream = fs.createWriteStream("road_output.bin");
  //let gpsToPixel = [-53519.28335928889, 728.1777777777778, 0, 39007.724794311114, 0, -728.1777777777778];
  // let width = 0
  // let raster

  for (let i = 0; i < count; i++) {
    let obj = geojson.features[i];
    if (isInChina(obj.geometry.bbox, gpsToPixel)) {
      pushToStream(obj.geometry, writableStream, gpsToPixel, width, raster, true)
    }
  }

  // let content = ""
  // for (let i = 0; i < count; i++) {
   
  //   let city = geojson[1].features[i];
  //   content += city.properties.Name;
  //   let p = calculatePosition(city.geometry.coordinates, gpsToPixel, width, raster, 10000);
  //   content += `(${p.pos[0]}, ${p.pos[1]}, ${p.pos[2]})\n`;
  // }
  // writableStream.write(content)
  // for (let i = 0; i < count; i++) {
  //   let river = geojson.features[i];
  //   let num = river.properties.HYDC[7]
  //   if (num == '1' || num == '2') {
  //     pushToStream(river.geometry, writableStream, gpsToPixel, width, raster)
  //   }
  // }

  //pushToObjectStream(geojson, objStream, gpsToPixel, width, raster)

  //objStream.end();
  writableStream.end();

  // And as each pixel in the tile covers a geographic area, not a single
  // GPS coordinate, get the area that this pixel covers:
  // const gpsBBox = [transform(x, y, pixelToGPS), transform(x + 1, y + 1, pixelToGPS)];
  // console.log(`Pixel covers the following GPS area:`, gpsBBox);

  // // Finally, retrieve the elevation associated with this pixel's geographic area:
  // const rasters = await image.readRasters();
  // const { width, [0]: raster } = rasters;
  // const elevation = raster[x + y * width];
  // console.log(`The elevation at (${lat.toFixed(6)},${long.toFixed(6)}) is ${elevation}m`);

} catch (err) {
  console.error(err);
}
