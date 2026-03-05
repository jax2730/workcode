#define _CRT_SECURE_NO_WARNINGS
#include "KarstDataHelper.h"
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <iostream>

// ============================================================================
// PCG Line Data Generation
// 移植自 XXSJ_PCG_ROAD，将 road/river/railway 折线按 2048 块大小分块
// 输出引擎可直接加载的 pcg.page + section_X_Y.page 格式
// ============================================================================

static const float PCG_BLOCK_SIZE = 2048.0f;
static const float PCG_ROAD_HALF_WIDTH = 32.0f;

// 与引擎侧 PolylineSegment 结构完全对齐，保证二进制兼容
struct PCGPolylineSegment {
	enum TypeEnum : int {
		eUnknow = 0,
		eRoad = (1 << 0),
		eRiver = (1 << 1),
		eRailway = (1 << 2),
	};
	PCGPolylineSegment() : startD(0.0f), endD(0.0f), type(eUnknow) {}
	union {
		struct {
			float startD;   // 起点累计距离
			float endD;     // 终点累计距离
			TypeEnum type;  // 线段类型
		};
		float DATA[3];      // 用于二进制写入
	};
	std::vector<Point> points; // 2D点列 (x=世界X, z=世界Z)
};

using BlockIndex = std::pair<int, int>;
using BlockPolylineMap = std::map<BlockIndex, std::vector<PCGPolylineSegment>>;

// 与引擎 Vector2::length() 完全一致的距离计算（使用直接乘法而非 pow）
static inline float PCG_Distance(const Point& a, const Point& b)
{
	float dx = a.x - b.x;
	float dz = a.z - b.z;
	return std::sqrt(dx * dx + dz * dz);
}

// 移除连续重复点
static void PCG_RemoveNearDuplicatePoints(std::vector<Point>& points, float epsilon = 1e-3f)
{
	if (points.empty()) return;
	std::vector<Point> result;
	result.push_back(points[0]);
	for (size_t i = 1; i < points.size(); ++i) {
		if (PCG_Distance(points[i], result.back()) > epsilon) {
			result.push_back(points[i]);
		}
	}
	points = result;
}

// 将折线按 blockSize 切分到各个 block 中
static void PCG_SplitPolylineByBlock(
	std::vector<Point>& points,
	float blockSize,
	PCGPolylineSegment::TypeEnum type,
	float polylineWidth,
	BlockPolylineMap& blockPolylines)
{
	// 获取一条线段(p0-p1)AABB扩展后覆盖的所有block
	auto getBlocks = [&](const Point& p0, const Point& p1) {
		std::set<BlockIndex> blocks;
		float minX = std::min(p0.x, p1.x) - polylineWidth * 0.5f;
		float maxX = std::max(p0.x, p1.x) + polylineWidth * 0.5f;
		float minZ = std::min(p0.z, p1.z) - polylineWidth * 0.5f;
		float maxZ = std::max(p0.z, p1.z) + polylineWidth * 0.5f;
		int blockMinI = (int)std::floor(minX / blockSize);
		int blockMaxI = (int)std::floor(maxX / blockSize);
		int blockMinJ = (int)std::floor(minZ / blockSize);
		int blockMaxJ = (int)std::floor(maxZ / blockSize);
		for (int i = blockMinI; i <= blockMaxI; ++i) {
			for (int j = blockMinJ; j <= blockMaxJ; ++j) {
				float bMinX = i * blockSize;
				float bMaxX = (i + 1) * blockSize;
				float bMinZ = j * blockSize;
				float bMaxZ = (j + 1) * blockSize;
				if (!(maxX < bMinX || minX > bMaxX || maxZ < bMinZ || minZ > bMaxZ)) {
					blocks.emplace(i, j);
				}
			}
		}
		return blocks;
	};

	PCG_RemoveNearDuplicatePoints(points, 1.0f);
	if (points.empty()) return;

	// 单点情况
	if (points.size() == 1) {
		PCGPolylineSegment seg;
		seg.points = { points[0] };
		seg.startD = 0.0f;
		seg.endD = 0.0f;
		seg.type = type;
		auto blocks = getBlocks(points[0], points[0]);
		for (const auto& bi : blocks) {
			blockPolylines[bi].push_back(seg);
		}
		return;
	}

	// 计算累计距离
	std::vector<float> accDist(points.size(), 0.0f);
	for (size_t i = 1; i < points.size(); ++i) {
		accDist[i] = accDist[i - 1] + PCG_Distance(points[i], points[i - 1]);
	}

	// 逐段处理：连续覆盖相同block集合的子段合并为一个segment
	PCGPolylineSegment curSeg;
	std::set<BlockIndex> curBlocks;

	for (size_t i = 0; i + 1 < points.size(); ++i) {
		Point p0 = points[i];
		Point p1 = points[i + 1];
		float d0 = accDist[i];
		float d1 = accDist[i + 1];

		auto segBlocks = getBlocks(p0, p1);

		if (curSeg.points.empty()) {
			curSeg.points.push_back(p0);
			curSeg.startD = d0;
			curSeg.type = type;
			curBlocks = segBlocks;
		}

		if (segBlocks == curBlocks) {
			curSeg.points.push_back(p1);
			curSeg.endD = d1;
		}
		else {
			// block集合变化，提交当前segment
			if (curSeg.points.size() >= 2) {
				for (const auto& bi : curBlocks) {
					blockPolylines[bi].push_back(curSeg);
				}
			}
			// 开始新segment（包含过渡点）
			curSeg = PCGPolylineSegment();
			curSeg.points.push_back(p0);
			curSeg.points.push_back(p1);
			curSeg.startD = d0;
			curSeg.endD = d1;
			curSeg.type = type;
			curBlocks = segBlocks;
		}
	}

	// 提交最后一段
	if (curSeg.points.size() >= 2) {
		for (const auto& bi : curBlocks) {
			blockPolylines[bi].push_back(curSeg);
		}
	}
}

// ============================================================================
// 对外接口: 生成 PCG 线段数据
// inputDir: 包含 china_road.bin / china_river.bin / railway.bin 的目录
// outputDir: 输出 pcg.page + section_X_Y.page 的目录
// ============================================================================
void GeneratePCGLineData(const std::string& inputDir, const std::string& outputDir)
{
	namespace fs = std::filesystem;

	std::cout << "[PCG] inputDir = [" << inputDir << "] (len=" << inputDir.size() << ")" << std::endl;
	std::cout << "[PCG] outputDir = [" << outputDir << "] (len=" << outputDir.size() << ")" << std::endl;

	// 确保 inputDir 以 / 结尾
	std::string inDir = inputDir;
	if (!inDir.empty() && inDir.back() != '/' && inDir.back() != '\\') {
		inDir += '/';
	}

	// 确保 outputDir 以 / 结尾, 先清空再创建目录（与引擎一致：remove_all + create_directory）
	std::string outDir = outputDir;
	if (!outDir.empty() && outDir.back() != '/' && outDir.back() != '\\') {
		outDir += '/';
	}
	std::error_code ec;
	fs::remove_all(outDir, ec);
	fs::create_directories(outDir);

	BlockPolylineMap splinePageMaps;

	// 参照项目其他模块: 直接拼字符串 + ifstream 打开
	struct FileEntry {
		const char* filename;
		PCGPolylineSegment::TypeEnum type;
		const char* label;
	};
	FileEntry entries[] = {
		{ "china_road.bin",  PCGPolylineSegment::eRoad,    "Road" },
		{ "china_river.bin", PCGPolylineSegment::eRiver,   "River" },
		{ "railway.bin",     PCGPolylineSegment::eRailway, "Railway" },
	};

	for (auto& e : entries) {
		std::string path = inDir + e.filename;
		std::cout << "[PCG] Trying " << e.label << ": [" << path << "]" << std::endl;

		std::ifstream stream(path, std::ios::binary);
		if (!stream) {
			std::cout << "[PCG] " << e.label << " file not found, skip." << std::endl;
			continue;
		}
		std::cout << "[PCG] Processing " << e.label << " data..." << std::endl;

		std::vector<std::vector<Point>> lines;
		// 读取bin: int withHeight, int lineNum, [int count, point*count]*lineNum
		int withHeight = 0;
		stream.read(reinterpret_cast<char*>(&withHeight), sizeof(int));
		int lineNum = 0;
		stream.read(reinterpret_cast<char*>(&lineNum), sizeof(int));
		lines.resize(lineNum);

		for (int i = 0; i < lineNum; ++i) {
			int pointCount = 0;
			stream.read(reinterpret_cast<char*>(&pointCount), sizeof(int));
			lines[i].resize(pointCount);
			if (withHeight == 0) {
				stream.read(reinterpret_cast<char*>(lines[i].data()), pointCount * sizeof(Point));
			}
			else {
				for (int j = 0; j < pointCount; ++j) {
					float x, y, z;
					stream.read(reinterpret_cast<char*>(&x), sizeof(float));
					stream.read(reinterpret_cast<char*>(&y), sizeof(float));
					stream.read(reinterpret_cast<char*>(&z), sizeof(float));
					lines[i][j] = Point(x, z);
				}
			}
		}
		stream.close();

		std::cout << "[PCG] " << e.label << ": read " << lineNum << " lines" << std::endl;
		for (auto& line : lines) {
			PCG_SplitPolylineByBlock(line, PCG_BLOCK_SIZE, e.type, PCG_ROAD_HALF_WIDTH, splinePageMaps);
		}
	}

	if (splinePageMaps.empty()) {
		std::cout << "[PCG] No data generated." << std::endl;
		return;
	}

	// 写入 pcg.page 索引文件
	{
		std::string pagePath = outDir + "pcg.page";
		FILE* f = nullptr;
		fopen_s(&f, pagePath.c_str(), "wb");
		if (!f) {
			std::cout << "[PCG] Failed to create: " << pagePath << std::endl;
			return;
		}

		uint32_t pageCount = (uint32_t)splinePageMaps.size();
		fwrite(&pageCount, sizeof(uint32_t), 1, f);

		for (auto& [idx, segs] : splinePageMaps) {
			int x = idx.first;
			int z = idx.second;
			fwrite(&x, sizeof(int), 1, f);
			fwrite(&z, sizeof(int), 1, f);
		}
		fclose(f);
		std::cout << "[PCG] Written pcg.page (" << pageCount << " blocks)" << std::endl;
	}

	// 写入每个 block 的 section 文件
	for (auto& [idx, segments] : splinePageMaps) {
		std::string secPath = outDir + "section_" + std::to_string(idx.first) + "_" + std::to_string(idx.second) + ".page";

		FILE* f = nullptr;
		fopen_s(&f, secPath.c_str(), "wb");
		if (!f) continue;

		int splineCount = (int)segments.size();
		fwrite(&splineCount, sizeof(int), 1, f);

		for (auto& seg : segments) {
			fwrite(seg.DATA, sizeof(float), 3, f);
			uint32_t ptCount = (uint32_t)seg.points.size();
			fwrite(&ptCount, sizeof(uint32_t), 1, f);
			if (ptCount > 0) {
				fwrite(seg.points.data(), sizeof(Point), ptCount, f);
			}
		}
		fclose(f);
	}

	std::cout << "[PCG] Complete: " << splinePageMaps.size() << " sections written to " << outDir << std::endl;
}
