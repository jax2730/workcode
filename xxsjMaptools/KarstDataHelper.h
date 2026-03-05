#pragma once
#include <limits>
#include <memory>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <queue>
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <assert.h>
#include <string>
#include <type_traits>

struct Point {
	float x, z;

	Point(float x_ = 0, float z_ = 0) : x(x_), z(z_) {}
};

struct Point3 {
	float x, y, z;

	Point3(float x_ = 0, float y_ = 0, float z_ = 0) : x(x_), y(y_), z(z_) {}
};

struct RiverVertex {
	Point3 position;
	Point3 dir;
	Point uv;
};

using Vector3 = Point3;
using Vector2 = Point;

struct RoadVertex {
	Vector3 pos;
	Vector3 normal;
	Vector2 uv;
};

struct RoadMesh {
	std::vector<RoadVertex> vertices;
	std::vector<uint32_t> indices;
};

struct RoadSystemMeshes {
	std::vector<RoadMesh> roadMeshes;
	std::vector<RoadMesh> intersectionMeshes;
};

template<typename T>
struct LineSet {
	std::vector<T> line;
	T vMin;
	T vMax;
};

template<typename T>
struct TwoTuples {
	uint32_t idx;
	T val;
};

template<typename T>
inline T operator-(const T& a, const T& b) {
	if constexpr (std::is_same_v<T, Point>) {
		return T(a.x - b.x, a.z - b.z);
	}
	else if constexpr (std::is_same_v<T, Point3>) {
		return T(a.x - b.x, a.y - b.y, a.z - b.z);
	}
	else {
		T r{};
		return r;
	}
}

template<typename T, typename D>
inline T operator/(const T& a, const D& b) {
	if constexpr (std::is_same_v<T, Point>) {
		return T(a.x / b, a.z / b);
	}
	else if constexpr (std::is_same_v<T, Point3>) {
		return T(a.x / b, a.y / b, a.z / b);
	}
	else {
		T r{};
		return r;
	}
}

static float clamp(float x, float minVal, float maxVal) {
	return std::min(std::max(x, minVal), maxVal);
}

static float smoothStep(float x, float edge0, float edge1) {
	float t = clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
	return t * t * (3.0f - 2.0f * t);
}

template<typename T>
static float vlen(const T& v) {
	if constexpr (std::is_same_v<T, Point>) {
		return std::sqrt(v.x * v.x + v.z * v.z);
	}
	else if constexpr (std::is_same_v<T, Point3>) {
		return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
	}
	else {
		return 0.0f;
	}
}

template<typename T>
static T vadd(const T& a, const T& b) {
	if constexpr (std::is_same_v<T, Point>) {
		return T(a.x + b.x, a.z + b.z);
	}
	else if constexpr (std::is_same_v<T, Point3>) {
		return T(a.x + b.x, a.y + b.y, a.z + b.z);
	}
	else {
		return {};
	}
}

template<typename T>
static T vsub(const T& a, const T& b) {
	if constexpr (std::is_same_v<T, Point>) {
		return T(a.x - b.x, a.z - b.z);
	}
	else if constexpr (std::is_same_v<T, Point3>) {
		return T(a.x - b.x, a.y - b.y, a.z - b.z);
	}
	else {
		return {};
	}
}

template<typename T>
static T vmul(const T& a, float s) {
	if constexpr (std::is_same_v<T, Point>) {
		return { a.x * s, a.z * s };
	}
	else if constexpr (std::is_same_v<T, Point3>) {
		return return { a.x * s, a.y * s, a.z * s };
	}
	else {
		return {};
	}
}

template<typename T>
static T vnorm(const T& v) {
	float L = vlen(v);
	if constexpr (std::is_same_v<T, Point>) {
		return (L > 1e-8f) ? T{ v.x / L, v.z / L } : T{ 0.f, 0.f };
	}
	else if constexpr (std::is_same_v<T, Point3>) {
		return (L > 1e-8f) ? T{ v.x / L, v.y / L, v.z / L } : T{ 0.f, 0.f, 0.f };
	}
	else {
		return {};
	}
}

inline Point3 cross(const Point3& a, const Point3& b) {
	return Point3(
		a.y * b.z - a.z * b.y,
		a.z * b.x - a.x * b.z,
		a.x * b.y - a.y * b.x
	);
}

template<typename T>
static float dot(const T& a, const T& b) {
	if constexpr (std::is_same_v<T, Point>) {
		return a.x * b.x + a.z * b.z;
	}
	else if constexpr (std::is_same_v<T, Point3>) {
		return a.x * b.x + a.y * b.y + a.z * b.z;
	}
	else {
		return 0.0f;
	}
}

template<typename T>
static float length(const T& a) {
	return vlen(a);
}

template<typename T>
static float calculateDist(const T& p0, const T& p1) {
	return sqrt(pow(p0.x - p1.x, 2.f) + pow(p0.z - p1.z, 2.f));
}

template<typename T>
static T calculateDirection(const T& p0, const T& p1) {
	return vnorm(p1 - p0);
}

template<typename T>
static void caculateLineBounding(std::vector<T>& line, T& vMin, T& vMax) {
	if constexpr (std::is_same_v<T, Point>) {
		vMin.x = FLT_MAX;
		vMin.z = FLT_MAX;
		vMax.x = FLT_MIN;
		vMax.z = FLT_MIN;

		for (auto& p : line) {
			vMin.x = std::min(p.x, vMin.x);
			vMin.z = std::min(p.z, vMin.z);
			vMax.x = std::max(p.x, vMax.x);
			vMax.z = std::max(p.z, vMax.z);
		}
	}
	else if constexpr (std::is_same_v<T, Point3>) {
		vMin.x = FLT_MAX;
		vMin.y = FLT_MAX;
		vMin.z = FLT_MAX;
		vMax.x = FLT_MIN;
		vMax.y = FLT_MIN;
		vMax.z = FLT_MIN;

		for (auto& p : line) {
			vMin.x = std::min(p.x, vMin.x);
			vMin.y = std::min(p.y, vMin.y);
			vMin.z = std::min(p.z, vMin.z);
			vMax.x = std::max(p.x, vMax.x);
			vMax.y = std::max(p.y, vMax.y);
			vMax.z = std::max(p.z, vMax.z);
		}
	}
}

template<typename T>
static void writePoint(const T& p, FILE* file) {
	if constexpr (std::is_same_v<T, Point>) {
		fwrite(&p.x, sizeof(float), 1, file);
		fwrite(&p.z, sizeof(float), 1, file);
	}
	else if constexpr (std::is_same_v<T, Point3>) {
		fwrite(&p.x, sizeof(float), 1, file);
		fwrite(&p.y, sizeof(float), 1, file);
		fwrite(&p.z, sizeof(float), 1, file);
	}
}

static void writeVertex(const RiverVertex& v, FILE* file) {
	fwrite(&v.position, sizeof(v.position), 1, file);
	fwrite(&v.uv, sizeof(v.uv), 1, file);
}

static void writeVertex(const RoadVertex& v, int index, FILE* file) {
	fwrite(&v.pos, sizeof(v.pos), 1, file);
	fwrite(&v.normal, sizeof(v.normal), 1, file);
	fwrite(&v.uv, sizeof(v.uv), 1, file);
	fwrite(&index, sizeof(int), 1, file);
}

template<typename T>
static bool BoundingIntersection(const LineSet<T>& lineset, const Point& pos) {
	return pos.x >= lineset.vMin.x && pos.z >= lineset.vMin.z &&
		pos.x <= lineset.vMax.x && pos.z <= lineset.vMax.z;
}

template<typename T>
static float CaculateDistancePointToLine(const T& p, const T& a, const T& b) {
	T ab = b - a;
	T ap = p - a;
	T bp = p - b;

	float ab_len_sq = dot(ab, ab);
	if (ab_len_sq == 0.0) {
		return length(ap);
	}

	float t = dot(ap, ab) / ab_len_sq;
	if (t < 0.0) {
		return length(ap);
	}
	else if (t > 1.0) {
		return length(bp);
	}
	else {
		if constexpr (std::is_same_v<T, Point>) {
			Point projection(a.x + t * ab.x, a.z + t * ab.z);
			return length(p - projection);
		}
		else if constexpr (std::is_same_v<T, Point3>) {
			Point3 projection(a.x + t * ab.x, a.y + t * ab.y, a.z + t * ab.z);
			return length(p - projection);
		}
	}
}

inline Point operator+(const Point& a, const Point& b) {
	return Point(a.x + b.x, a.z + b.z);
}
inline Point operator-(const Point& a, const Point& b) {
	return Point(a.x - b.x, a.z - b.z);
}
inline Point operator*(const Point& a, float s) {
	return Point(a.x * s, a.z * s);
}
inline Point operator*(float s, const Point& a) {
	return a * s;
}
inline Point operator-(const Point& a) {
	return Point(-a.x, -a.z);
}
inline float dot(const Point& a, const Point& b) {
	return a.x * b.x + a.z * b.z;
}
inline float lengthSquared(const Point& a) {
	return dot(a, a);
}

// ---------- Point3 (x,y,z) ----------
inline Point3 operator+(const Point3& a, const Point3& b) {
	return Point3(a.x + b.x, a.y + b.y, a.z + b.z);
}
inline Point3 operator-(const Point3& a, const Point3& b) {
	return Point3(a.x - b.x, a.y - b.y, a.z - b.z);
}
inline Point3 operator*(const Point3& a, float s) {
	return Point3(a.x * s, a.y * s, a.z * s);
}
inline Point3 operator*(float s, const Point3& a) {
	return a * s;
}
inline Point3 operator-(const Point3& a) {
	return Point3(-a.x, -a.y, -a.z);
}
inline float dot(const Point3& a, const Point3& b) {
	return a.x * b.x + a.y * b.y + a.z * b.z;
}
inline float lengthSquared(const Point3& a) {
	return dot(a, a);
}

// ---------- 组件级 min/max：更新包围盒 ----------
inline void expandBounds(Point& vMin, Point& vMax, const Point& p) {
	vMin.x = std::min(vMin.x, p.x);
	vMin.z = std::min(vMin.z, p.z);
	vMax.x = std::max(vMax.x, p.x);
	vMax.z = std::max(vMax.z, p.z);
}

inline void expandBounds(Point3& vMin, Point3& vMax, const Point3& p) {
	vMin.x = std::min(vMin.x, p.x);
	vMin.y = std::min(vMin.y, p.y);
	vMin.z = std::min(vMin.z, p.z);
	vMax.x = std::max(vMax.x, p.x);
	vMax.y = std::max(vMax.y, p.y);
	vMax.z = std::max(vMax.z, p.z);
}

// ----------------- Catmull–Rom 评估（均匀型） -----------------

// 标准 Uniform Catmull–Rom：给四个控制点 p0,p1,p2,p3 和参数 t∈[0,1]，求曲线点
template<typename T>
T catmullRom(const T& p0, const T& p1, const T& p2, const T& p3, float t) {
	// 使用常见的写法：
	// a = 2*p1
	// b = p2 - p0
	// c = 2*p0 - 5*p1 + 4*p2 - p3
	// d = -p0 + 3*p1 - 3*p2 + p3
	// P(t) = 0.5 * (a + b*t + c*t^2 + d*t^3)

	float t2 = t * t;
	float t3 = t2 * t;

	T a = p1 * 2.0f;
	T b = p2 - p0;
	T c = p0 * 2.0f - p1 * 5.0f + p2 * 4.0f - p3;
	T d = -p0 + p1 * 3.0f - p2 * 3.0f + p3;

	return (a + b * t + c * t2 + d * t3) * 0.5f;
}

template<typename T>
LineSet<T> sampleCatmullRom(const std::vector<T>& controlPoints,
	int samplesPerSegment) {
	LineSet<T> out;
	const size_t n = controlPoints.size();

	if (n == 0 || samplesPerSegment <= 0)
		return out;

	if (n == 1) {
		out.line.push_back(controlPoints[0]);
		out.vMin = out.vMax = controlPoints[0];
		return out;
	}

	std::vector<T> ext;
	ext.reserve(n + 2);

	if (n == 2) {
		ext.push_back(controlPoints[0]);
		ext.push_back(controlPoints[0]);
		ext.push_back(controlPoints[1]);
		ext.push_back(controlPoints[1]);
	}
	else {
		ext.push_back(controlPoints.front());
		ext.insert(ext.end(), controlPoints.begin(), controlPoints.end());
		ext.push_back(controlPoints.back());
	}

	const int extN = static_cast<int>(ext.size());

	bool firstSample = true;

	for (int i = 1; i + 2 < extN; ++i) {
		const T& p0 = ext[i - 1];
		const T& p1 = ext[i];
		const T& p2 = ext[i + 1];
		const T& p3 = ext[i + 2];

		for (int s = 0; s < samplesPerSegment; ++s) {
			float t = static_cast<float>(s) / static_cast<float>(samplesPerSegment);
			T p = catmullRom(p0, p1, p2, p3, t);

			if (firstSample) {
				out.vMin = out.vMax = p;
				firstSample = false;
			}
			else {
				expandBounds(out.vMin, out.vMax, p);
			}

			out.line.push_back(p);
		}
	}

	{
		const int i = extN - 3;
		const T& p0 = ext[i - 1];
		const T& p1 = ext[i];
		const T& p2 = ext[i + 1];
		const T& p3 = ext[i + 2];

		T pEnd = catmullRom(p0, p1, p2, p3, 1.0f);
		if (firstSample) {
			out.vMin = out.vMax = pEnd;
		}
		else {
			expandBounds(out.vMin, out.vMax, pEnd);
		}
		out.line.push_back(pEnd);
	}

	return out;
}

// ----------------- 点到线段距离 -----------------

// 适用于 Point / Point3（依赖 "+" "-" "*" dot lengthSquared）
template<typename T>
float pointToSegmentDistance(const T& p, const T& a, const T& b) {
	T v = b - a; // 线段方向
	T w = p - a;

	float c2 = lengthSquared(v);
	if (c2 <= 0.0f) {
		// a 与 b 重合，退化为点
		return std::sqrt(lengthSquared(w));
	}

	float c1 = dot(w, v);
	float t = c1 / c2;          // 投影参数
	if (t < 0.0f) t = 0.0f;
	else if (t > 1.0f) t = 1.0f;

	T proj = a + v * t;         // 最近点
	T diff = p - proj;
	return std::sqrt(lengthSquared(diff));
}

static float pointToSegmentDistance(const Point& p, const Point3& a, const Point3& b, Point3& closestPoint) {
	// 把 a,b 投到 XZ 平面上做计算
	Point a2{ a.x, a.z };
	Point b2{ b.x, b.z };

	Point v = { b2.x - a2.x, b2.z - a2.z }; // 线段方向（XZ）
	Point w = { p.x - a2.x, p.z - a2.z }; // a -> p（XZ）

	float c2 = lengthSquared(v);
	if (c2 <= 0.0f) {
		// a 与 b 在 XZ 上重合，退化为点
		closestPoint = a;  // y 直接取 a 的
		return std::sqrt(lengthSquared(w));
	}

	float c1 = dot(w, v);
	float t = c1 / c2;    // 投影参数（在 XZ 平面）

	// clamp 到 [0,1]，保证最近点在线段内部
	if (t < 0.0f)      t = 0.0f;
	else if (t > 1.0f) t = 1.0f;

	// 在线段 3D 上插值，得到最近点的完整 3D 位置（包含 y）
	closestPoint = a + (b - a) * t;

	// 用 XZ 平面的距离
	Point proj2{ closestPoint.x, closestPoint.z };
	Point diff = { p.x - proj2.x, p.z - proj2.z };

	return std::sqrt(lengthSquared(diff));
}

// ----------------- 点到整条折线（曲线采样后）的最小距离 -----------------

template<typename T>
float distancePointToLineSet(const T& p, const LineSet<T>& ls) {
	if (ls.line.size() < 2)
		return std::numeric_limits<float>::infinity();

	float minDist = std::numeric_limits<float>::infinity();
	const auto& pts = ls.line;

	for (size_t i = 1; i < pts.size(); ++i) {
		float d = pointToSegmentDistance(p, pts[i - 1], pts[i]);
		if (d < minDist)
			minDist = d;
	}
	return minDist;
}

static float distancePointToLineSetWithHeight(const Point& p, const LineSet<Point3>& ls, float& height) {
	if (ls.line.size() < 2)
		return std::numeric_limits<float>::infinity();

	float minDist = std::numeric_limits<float>::infinity();
	const auto& pts = ls.line;

	for (size_t i = 1; i < pts.size(); ++i) {
		Point3 closestPoint;
		float d = pointToSegmentDistance(p, pts[i - 1], pts[i], closestPoint);
		if (d < minDist) {
			minDist = d;
			height = closestPoint.y;
		}
	}
	return minDist;
}

struct MeshStrip {
	std::vector<Point3> vertices;       // 顶点位置
	std::vector<unsigned int> indices;  // 三角形索引（3 个一组）
};
