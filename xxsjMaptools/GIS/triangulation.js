
var points = [];
const interval = 750;


// point with lowest y coordinate
// given collision, pick the one with smallest x
// in svg, coordinates are reversed

export function Point(x, y, c) {
    this.x = x;
    this.y = y;
    this.c = c;
}


function distanceSq(a, b) {
    return (a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y);
}

function angle(a, b, c) {
    var temp = Math.atan2((c.y - b.y), (c.x - b.x)) - Math.atan2((a.y - b.y), (a.x - b.x));
    return temp < 0 ? Math.PI * 2 + temp : temp;
}

function Triangle(a, b, c) {
    this.a = a;
    this.b = b;
    this.c = c;
    this.area = function () {
        return (a.x * (b.y - c.y) + b.x * (c.y - a.y) + c.x * (a.y - b.y)) / 2;
    }
}




function measureAngles(points) {
    for (var i = 0; i < points.length + 1; i++) {
        points[(i + 1) % points.length].angle = angle(points[i % points.length],
            points[(i + 1) % points.length],
            points[(i + 2) % points.length]) * 180 / Math.PI;
    }

}

function isInsideTriangle(triangle, point) {
    var areaA, areaB, areaC;
    areaA = (new Triangle(triangle.a, triangle.b, point)).area();
    areaB = (new Triangle(triangle.b, triangle.c, point)).area();
    areaC = (new Triangle(triangle.c, triangle.a, point)).area();
    return (Math.sign(areaA) == Math.sign(areaB) &&
        Math.sign(areaA) == Math.sign(areaC));
}

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

export function triangulate(cPoints, force) {

    if (cPoints < 3) return;

    let points = []

    for (let i = 0; i < cPoints.length; i++) {
        points.push(new Point(cPoints[i][0], cPoints[i][1], 0))
    }

    var i = 1;
    var a, b, c;
    var triangles = [];

    measureAngles(points);
    let lastlength = -1;
    let inv = false;
    while (points.length > 3) {
        if (i % points.length == 1) {
            if (points.length == lastlength) {
                inv = true
                break
            }
            else
                lastlength = points.length;
        }
        a = points[i % points.length];
        b = points[(i + 1) % points.length];
        c = points[(i + 2) % points.length];

        if (angle(a, b, c) < Math.PI) {
            var tempTriangle = new Triangle(a, b, c);
            var isEar = true;
            for (var j = i + 3; j < points.length + i + 3; j++) {
                if (isInsideTriangle(tempTriangle, points[j % points.length]))
                    isEar = false;
            }
            if (isEar) {
                points.splice((i + 1) % points.length, 1); // erase middle point
                triangles.push(tempTriangle);
            }
        }
        i++;
    }
    triangles.push(new Triangle(points[0], points[1], points[2]));
    return triangles
    // if (!inv || force) {
    //     triangles.push(new Triangle(points[0], points[1], points[2]));
    //     return triangles
    // }
    // else {
    //     let invPoints = []
    //     for (let i = points.length - 1; i >= 0; i--) {
    //         invPoints.push(points[i])
    //     }
    //     return triangulate(invPoints, true)
    // }
}




