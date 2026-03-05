#include <thread>
#include <filesystem>
#include "KarstDataHelper.h"
#include "Task.h"

const int BlockOffset[2] = { -512, -256 };
const float TerrainOffset[2] = { -2097152.0f, -1048576.0f };

const static int hAll = 1690;
const static int hMin = -625;
const static float BLOCK_SIZE = 32768.0f;

void BuildRoadSystemFromLineSet(std::vector<std::vector<Point3>>& lineSets, RoadSystemMeshes& outMeshes);
void SampleCatmullRomPolyline(const std::vector<Vector3>& ctrl, std::vector<Vector3>& out, float step = 0.2f, float smooth = 0.7f);

inline uint32_t ConvertHeightFloatToU16(float height)
{
	return uint32_t(std::max((height - hMin), 0.0f) / hAll * 65535);
}

template<typename T>
static void caculatePointIndex(T p, int& x, int& y, float sizeScale)
{
	p.x = p.x - TerrainOffset[0];
	p.z = p.z - TerrainOffset[1];
	float scale = 1.0f / sizeScale;
	float blockSize = BLOCK_SIZE * scale;
	x = (int)floor(p.x / blockSize);
	y = (int)floor(p.z / blockSize);
}

template<typename T>
void addToMap(std::vector<T>& points, std::vector<T>& line, std::unordered_map<int, std::vector<std::vector<T>>>& lineMap, float sizeScale)
{
	std::unordered_map<int, std::vector<T>> railwaySubMap;
	int blockIdx = -1;
	T lastPos;
	for (int i = 0; i < points.size(); i++)
	{
		auto& p = points[i];

		/*if (calculateDist(lastPos, p) < 100.0 && i != 0 && (i != points.size() - 1) )
			continue;*/

		lastPos = p;

		int idx, idy;
		caculatePointIndex(p, idx, idy, sizeScale);

		int curIdx = idx << 16 | idy;

		if (blockIdx == -1) {
			blockIdx = curIdx;
		}

		line.push_back(points[i]);

		if (blockIdx == curIdx) {
			railwaySubMap[blockIdx].push_back(points[i]);
		}
		else {
			railwaySubMap[blockIdx].push_back(points[i]);
			railwaySubMap[curIdx].push_back(points[i]);
			blockIdx = curIdx;
		}
	}

	for (auto& it : railwaySubMap)
	{
		lineMap[it.first].push_back(it.second);
	}
}

template<typename T>
void savePointToFiles(std::string savePath, bool* selectedGrid, int rowSize, int colSize, std::unordered_map<int, std::vector<std::vector<T>>>& lineMap)
{
	std::string indexPath = savePath + "roadline.idx";

	FILE* indexFile;
	fopen_s(&indexFile, indexPath.c_str(), "wb");
	int indexNum = (int)lineMap.size();
	fwrite(&indexNum, sizeof(int), 1, indexFile);
	for (auto it : lineMap)
	{
		int blockIdx = it.first;
		int x = blockIdx >> 16, y = blockIdx & 0xffff;

		if (!selectedGrid[(y / 8) * rowSize + x / 8]) {
			continue;
		}

		fwrite(&x, sizeof(int), 1, indexFile);
		fwrite(&y, sizeof(int), 1, indexFile);

		std::string path = savePath + "road" + std::to_string(x) + "_" + std::to_string(y) + ".line";
		FILE* file = nullptr;
		fopen_s(&file, path.c_str(), "wb");

		int roadNum = (int)it.second.size();
		fwrite(&roadNum, sizeof(int), 1, file);
		for (auto& roadPoints : it.second)
		{
			int num = (int)roadPoints.size();
			if (num < 2)
				continue;

			fwrite(&num, sizeof(int), 1, file);

			for (int i = 0; i < num; i++)
			{
				T direction;
				if (i == 0) {

					direction = calculateDirection(roadPoints[1], roadPoints[0]);
				}
				else if (i == num - 1) {
					direction = calculateDirection(roadPoints[num - 1], roadPoints[num - 2]);
				}
				else {
					direction = calculateDirection(roadPoints[i + 1], roadPoints[i - 1]);
				}
				direction = direction * 50.0;
				writePoint(roadPoints[i], file);
				writePoint(direction, file);
			}
		}
		fclose(file);
	}

	fclose(indexFile);
}

void readPointDataFile(std::ifstream& dataStream, std::unordered_map<int, std::vector< std::vector<Point>>>& lineMap, float sizeScale)
{
	int withHeight;
	dataStream.read(reinterpret_cast<char*>(&withHeight), sizeof(int));

	int roadNum;
	dataStream.read(reinterpret_cast<char*>(&roadNum), sizeof(int));

	for (int i = 0; i < roadNum; i++)
	{
		std::vector<Point> points;
		int count;
		dataStream.read(reinterpret_cast<char*>(&count), sizeof(int));

		for (int i = 0; i < count; i++)
		{
			Point point;
			dataStream.read(reinterpret_cast<char*>(&point.x), sizeof(float));
			if(withHeight)
			{
				float y;
				dataStream.read(reinterpret_cast<char*>(&y), sizeof(float));
			}
			dataStream.read(reinterpret_cast<char*>(&point.z), sizeof(float));
			points.push_back(point);
		}
		std::vector<Point> line;
		addToMap(points, line, lineMap, sizeScale);
	}
}

void readLinePointDataFromFile(std::ifstream& dataStream, std::vector<std::vector<Point3>>& lines)
{
	int withHeight;
	dataStream.read(reinterpret_cast<char*>(&withHeight), sizeof(int));

	int roadNum;
	dataStream.read(reinterpret_cast<char*>(&roadNum), sizeof(int));

	for (int i = 0; i < roadNum; i++)
	{
		std::vector<Point3> points;
		int count;
		dataStream.read(reinterpret_cast<char*>(&count), sizeof(int));

		for (int i = 0; i < count; i++)
		{
			Point3 point;
			dataStream.read(reinterpret_cast<char*>(&point.x), sizeof(float));
			if (withHeight) {
				dataStream.read(reinterpret_cast<char*>(&point.y), sizeof(float));
			}
			else {
				point.y = 0.0f;
			}
			dataStream.read(reinterpret_cast<char*>(&point.z), sizeof(float));
			points.push_back(point);
		}
		lines.push_back(points);
	}
}

void readPointDataFile(std::ifstream& dataStream, std::unordered_map<int, std::vector< std::vector<Point3>>>& lineMap, float sizeScale)
{
	std::vector<std::vector<Point3>> lines;
	readLinePointDataFromFile(dataStream, lines);

	for (size_t i = 0; i<lines.size();i++)
	{
		std::vector<Point3> line;
		addToMap(lines[i], line, lineMap, sizeScale);
	}
}

void ProcRoadsLineData(std::string dataPath, std::string savePath, bool* selectedGrid, int rowSize, int colSize)
{
	std::ifstream dataStream(dataPath, std::ios::binary);
	if (dataStream)
	{
		std::unordered_map<int, std::vector< std::vector<Point3>>> lineMap;
		readPointDataFile(dataStream, lineMap, 8.0f);
		savePointToFiles(savePath, selectedGrid, rowSize, colSize, lineMap);
	}
}

template<typename T>
void filterLinePoints(std::vector<T>& points, std::vector<T>& line)
{
	T lastPos;
	for (int i = 0; i < points.size(); i++)
	{
		auto& p = points[i];
		/*if (calculateDist(lastPos, p) < 100.0 && i != 0 && (i != points.size() - 1))
			continue;*/
		lastPos = p;
		line.push_back(points[i]);
	}
}

const float gridSize = 4.0f;

template<typename T>
void CalculateWaterHighPrecisionMesh(std::vector<TwoTuples<uint8_t>>& tuples, const std::vector<LineSet<T>>& waterLineSetsTable, int pagex, int pagey, float blockSize)
{	
	for (uint32_t y = 0; y < 8193; y++)
	{
		for (uint32_t x = 0; x < 8193; x++)
		{
			Point pos = { pagex * blockSize + x * gridSize + TerrainOffset[0],  pagey * blockSize + y * gridSize + TerrainOffset[1] };

			float distMin = FLT_MAX;
			for (auto& lineset : waterLineSetsTable)
			{
				if (BoundingIntersection(lineset, pos))
				{
					float dist = distancePointToLineSet(pos, lineset);
					distMin = std::min(distMin, dist);
				}
			}

			float val = 1.0 - smoothStep(distMin, 0.0, 15.0);

			if (val > 0.001f) {
				uint8_t _val = uint8_t(val * 255);
				tuples.push_back(TwoTuples<uint8_t>{ x << 16 | y, _val });
			}
		}
	}
}

void CalculateRoadHighPrecisionMesh(std::vector<TwoTuples<uint32_t>>& tuples, const std::vector<LineSet<Point3>>& roadLineSetsTable, int pagex, int pagey, float blockSize)
{
	for (uint32_t y = 0; y < 8193; y++)
	{
		for (uint32_t x = 0; x < 8193; x++)
		{
			Point pos = { pagex * blockSize + x * gridSize + TerrainOffset[0],  pagey * blockSize + y * gridSize + TerrainOffset[1] };

			float distMin = FLT_MAX;
			float distMinHeight = 0.f;
			for (auto& lineset : roadLineSetsTable)
			{
				if (BoundingIntersection(lineset, pos))
				{
					float height;
					float dist = distancePointToLineSetWithHeight(pos, lineset, height);
					if (dist < distMin) {
						distMin = dist;
						distMinHeight = height;
					}
				}
			}

			float val = 1.0 - smoothStep(distMin, 10.0, 40.0);

			if (val > 0.001f) {
				uint32_t heightU16 = ConvertHeightFloatToU16(distMinHeight);
				uint8_t _val = uint8_t(val * 255);
				tuples.push_back(TwoTuples<uint32_t>{ x << 16 | y, heightU16 << 16 | _val});
			}
		}
	}
}

template<typename T>
void pushToLineSetsTable(bool* selectedGrid, const LineSet<T>& lineSet, std::unordered_map<uint32_t, std::vector<LineSet<T>>>& waterLineSetsTable, int rowSize, int colSize, int sizeScale, float blockSize)
{
	for (int i = 0; i < rowSize * sizeScale; i++)
	{
		for (int j = 0; j < colSize * sizeScale; j++)
		{
			if (!selectedGrid[(j / sizeScale) * rowSize + i / sizeScale]) {
				continue;
			}
			float x_start = i * blockSize + TerrainOffset[0];
			float x_end = (i + 1) * blockSize + TerrainOffset[0];
			float z_start = j * blockSize + TerrainOffset[1];
			float z_end = (j + 1) * blockSize + TerrainOffset[1];

			if (lineSet.vMax.x < x_start || lineSet.vMax.z < z_start || lineSet.vMin.x > x_end || lineSet.vMin.z > z_end) {
				continue;
			}

			bool inGrid = false;
			for (const auto& p : lineSet.line)
			{
				if (p.x >= x_start && p.z >= z_start && p.x <= x_end && p.z <= z_end) {
					inGrid = true;
					break;
				}
			}
			uint32_t index = i << 16 | j;
			if (inGrid) {
				waterLineSetsTable[index].push_back(lineSet);
			}
		}
	}
}

template<typename T>
void writeMask(std::string filePath, std::vector<TwoTuples<T>>& tuples, int pagex, int pagey)
{
	
	FILE* file = nullptr;
	fopen_s(&file, filePath.c_str(), "wb");
	int size = tuples.size();
	fwrite(&size, sizeof(uint32_t), 1, file);

	for (auto& tuple : tuples)
	{
		fwrite(&tuple.idx, sizeof(uint32_t), 1, file);
		fwrite(&tuple.val, sizeof(tuple.val), 1, file);
	}
	fclose(file);
}

bool checkFileExist(std::string filePath)
{
	return std::filesystem::exists(filePath);
}

void readLineData(std::ifstream& dataStream, bool* selectedGrid, std::unordered_map<uint32_t, std::vector<LineSet<Point>>>& waterLineSetsTable, int rowSize, int colSize, int sizeScale, float blockSize, bool useCatmullRom)
{
	Point vAllMin, vAllMax;
	vAllMin.x = FLT_MAX;
	vAllMin.z = FLT_MAX;
	vAllMax.x = FLT_MIN;
	vAllMax.z = FLT_MIN;

	int withHeight;
	dataStream.read(reinterpret_cast<char*>(&withHeight), sizeof(int));

	int lineNum;
	dataStream.read(reinterpret_cast<char*>(&lineNum), sizeof(int));

	for (int i = 0; i < lineNum; i++)
	{
		std::vector<Point> points;
		int count;
		dataStream.read(reinterpret_cast<char*>(&count), sizeof(int));
		if (i % 1000 == 0)
		{
			std::cout << "read line : " << i << "/" << lineNum << std::endl;
		}
		for (int i = 0; i < count; i++)
		{
			Point point;
			dataStream.read(reinterpret_cast<char*>(&point.x), sizeof(float));
			if (withHeight != 0) {
				float y;
				dataStream.read(reinterpret_cast<char*>(&y), sizeof(float));
			}
			dataStream.read(reinterpret_cast<char*>(&point.z), sizeof(float));
			points.push_back(point);
		}
		std::vector<Point> line;

		filterLinePoints(points, line);

		Point vMin, vMax;
		caculateLineBounding(line, vMin, vMax);

		vAllMin.x = std::min(vAllMin.x, vMin.x);
		vAllMin.z = std::min(vAllMin.z, vMin.z);
		vAllMax.x = std::max(vAllMax.x, vMax.x);
		vAllMax.z = std::max(vAllMax.z, vMax.z);

		if (useCatmullRom) {
			LineSet<Point> lineSet = sampleCatmullRom(line, 10);
			pushToLineSetsTable(selectedGrid, lineSet, waterLineSetsTable, rowSize, colSize, sizeScale, blockSize);
		}
		else {
			LineSet<Point> lineSet = { line, vMin, vMax };
			pushToLineSetsTable(selectedGrid, lineSet, waterLineSetsTable, rowSize, colSize, sizeScale, blockSize);
		}	
	}
}

void readLineData(std::ifstream& dataStream, bool* selectedGrid, std::unordered_map<uint32_t, std::vector<LineSet<Point3>>>& waterLineSetsTable, int rowSize, int colSize, int sizeScale, float blockSize, bool useCatmullRom)
{
	int withHeight;
	dataStream.read(reinterpret_cast<char*>(&withHeight), sizeof(int));

	int lineNum;
	dataStream.read(reinterpret_cast<char*>(&lineNum), sizeof(int));

	for (int i = 0; i < lineNum; i++)
	{
		std::vector<Point3> points;
		int count;
		dataStream.read(reinterpret_cast<char*>(&count), sizeof(int));
		if(i %1000 == 0)
		{
			std::cout << "read line : " << i << "/" << lineNum << std::endl;
		}
		for (int i = 0; i < count; i++)
		{
			Point3 point;
			dataStream.read(reinterpret_cast<char*>(&point.x), sizeof(float));
			if (withHeight != 0) {
				dataStream.read(reinterpret_cast<char*>(&point.y), sizeof(float));
			}
			else {
				point.y = 0.0f;
			}
			dataStream.read(reinterpret_cast<char*>(&point.z), sizeof(float));
			points.push_back(point);
		}

		Point3 vMin, vMax;
		caculateLineBounding(points, vMin, vMax);

		if (useCatmullRom) {
			LineSet<Point3> lineSet = sampleCatmullRom(points, 10);
			pushToLineSetsTable(selectedGrid, lineSet, waterLineSetsTable, rowSize, colSize, sizeScale, blockSize);
		}
		else {
			LineSet<Point3> lineSet = { points, vMin, vMax };
			pushToLineSetsTable(selectedGrid, lineSet, waterLineSetsTable, rowSize, colSize, sizeScale, blockSize);
		}	
	}
}

void CreateWaterLine(std::string dataPath, std::string outputPath, int rowSize, int colSize, bool* selectedGrid)
{
	std::ifstream dataStream(dataPath, std::ios::binary);
	if (dataStream)
	{
		std::vector<std::thread> threads;
		std::unordered_map<uint32_t, std::vector<LineSet<Point>>> waterLineSetsTable;
		readLineData(dataStream, selectedGrid, waterLineSetsTable, rowSize, colSize, 1, 32768.0f, true);

		ThreadPool pool;

		for (auto& table : waterLineSetsTable)
		{
			if (table.second.size())
			{
				pool.enqueue([&table, outputPath]() {
					int index = table.first;
					int pagex = index >> 16;
					int pagey = index & 0xffff;
					std::string filePath = outputPath + "water" + std::to_string(pagex) + "_" + std::to_string(pagey) + ".tuple";
					if (checkFileExist(filePath))
						return;
					std::vector<TwoTuples<uint8_t>> tuples;
					CalculateWaterHighPrecisionMesh(tuples, table.second, pagex, pagey, 32768.0f);
					if (tuples.size()) {
						writeMask(filePath, tuples, pagex, pagey);
					}
					}
				);
			}
		}
		pool.waitAll();
		pool.stop();
	}
}

void generatePolylineMesh(const std::vector<Point3>& polyline, float width, std::vector<Point3>& out_vertices, std::vector<uint32_t>& out_indices)
{
	out_vertices.clear();
	out_indices.clear();

	const int n = polyline.size();
	if (n < 2 || width <= 0.f) return;

	const float halfW = width * 0.5f;
	const float EPS = 1e-8f;

	std::vector<Point> tangents(n, { 0.f, 0.f });
	for (int i = 0; i < n; ++i) {
		Point t{ 0.f, 0.f };
		if (i + 1 < n) {
			Point fwd{ polyline[i + 1].x - polyline[i].x,
					  polyline[i + 1].z - polyline[i].z };
			if (vlen(fwd) > EPS) t = vadd(t, vnorm(fwd));
		}
		if (i >= 1) {
			Point bwd{ polyline[i].x - polyline[i - 1].x,
					  polyline[i].z - polyline[i - 1].z };
			if (vlen(bwd) > EPS) t = vadd(t, vnorm(bwd));
		}
		if (vlen(t) <= EPS) {
			for (int j = i + 1; j < n; ++j) {
				Point d{ polyline[j].x - polyline[i].x,
						polyline[j].z - polyline[i].z };
				if (vlen(d) > EPS) { t = vnorm(d); break; }
			}
			if (vlen(t) <= EPS) {
				for (int j = i; j-- > 0; ) {
					Point d{ polyline[i].x - polyline[j].x,
							polyline[i].z - polyline[j].z };
					if (vlen(d) > EPS) { t = vnorm(d); break; }
				}
			}
		}
		tangents[i] = vnorm(t);
	}

	std::vector<Point3> lefts;  lefts.reserve(n);
	std::vector<Point3> rights; rights.reserve(n);

	for (int i = 0; i < n; ++i) {
		const Point3& p = polyline[i];
		Point t = tangents[i];
		Point n2 = { -t.z, t.x };
		float L = vlen(n2);
		if (L <= EPS) n2 = { 1.f, 0.f };
		else          n2 = { n2.x / L, n2.z / L };

		Point off = vmul(n2, halfW);
		Point3 Lp{ p.x + off.x, p.y, p.z + off.z };
		Point3 Rp{ p.x - off.x, p.y, p.z - off.z };
		lefts.push_back(Lp);
		rights.push_back(Rp);
	}

	out_vertices.reserve(n * 2);
	for (int i = 0; i < n; ++i) {
		out_vertices.push_back(lefts[i]);
		out_vertices.push_back(rights[i]);
	}

	out_indices.reserve((n - 1) * 6);
	for (uint32_t i = 0; i < uint32_t(n - 1); ++i) {
		uint32_t Li = 2 * i;
		uint32_t Ri = 2 * i + 1;
		uint32_t Lip1 = 2 * (i + 1);
		uint32_t Rip1 = 2 * (i + 1) + 1;

		// 三角1
		out_indices.push_back(Li);
		out_indices.push_back(Ri);
		out_indices.push_back(Lip1);

		// 三角2
		out_indices.push_back(Ri);
		out_indices.push_back(Rip1);
		out_indices.push_back(Lip1);
	}
}

void generateCatmullRomPolylineMesh(const LineSet<Point3>& lineSet,
	float width, std::vector<RiverVertex>& out_vertices, std::vector<uint32_t>& out_indices)
{
	const auto& pts = lineSet.line;
	const size_t n = pts.size();
	if (n < 2 || width <= 0.0f)
		return;

	out_vertices.resize(2 * n);     // 每个点两个顶点：left/right
	std::vector<Point3> dirs(n);     // 每个点的前进方向（tangent
	std::vector<float> arcLen(n);

	const Point3 globalUp(0.0f, 1.0f, 0.0f);
	Point3 lastDir(1.0f, 0.0f, 0.0f); // 初始方向，防止退化

	// --------- 计算每个点的 forward 向量（基于相邻点） ---------
	for (size_t i = 0; i < n; ++i)
	{
		const Point3& cur = pts[i];

		const Point3& prev = (i > 0) ? pts[i - 1] : cur;
		const Point3& next = (i + 1 < n) ? pts[i + 1] : cur;

		Point3 f = next - prev;
		if (lengthSquared(f) > 1e-8f)
		{
			f = vnorm(f);
			lastDir = f;
		}
		else
		{
			// 当前点前进方向退化，则沿用上一点
			f = lastDir;
		}
		dirs[i] = f;
	}

	const float halfW = width * 0.5f;

	arcLen[0] = 0.0f;

	for (size_t i = 1; i < n; ++i)
	{
		float segLen = length(pts[i] - pts[i - 1]);
		arcLen[i] = arcLen[i - 1] + segLen;
	}

	// --------- 生成左右顶点 ---------
	for (size_t i = 0; i < n; ++i)
	{
		const Point3& center = pts[i];
		const Point3& forward = dirs[i];

		Point3 side = cross(globalUp, forward);

		if (lengthSquared(side) < 1e-6f)
		{
			Point3 altUp(0.0f, 0.0f, 1.0f);
			side = cross(altUp, forward);
			if (lengthSquared(side) < 1e-6f)
				side = Point3(1.0f, 0.0f, 0.0f);
		}
		side = vnorm(side);
		Point3 offset = side * halfW;

		out_vertices[2 * i + 0].position = center + offset; // left
		out_vertices[2 * i + 0].dir = { forward.x, forward.y };
		out_vertices[2 * i + 0].uv = { 0.0f, arcLen[i] / 40.0f };

		out_vertices[2 * i + 1].position = center - offset; // right
		out_vertices[2 * i + 1].dir = { forward.x, forward.y };
		out_vertices[2 * i + 1].uv = { 1.0f, arcLen[i] / 40.0f };
	}

	for (size_t i = 0; i + 1 < n; ++i)
	{
		unsigned int i0 = static_cast<unsigned int>(2 * i);       // left_i
		unsigned int i1 = static_cast<unsigned int>(2 * i + 1);   // right_i
		unsigned int i2 = static_cast<unsigned int>(2 * (i + 1)); // left_{i+1}
		unsigned int i3 = static_cast<unsigned int>(2 * (i + 1) + 1); // right_{i+1}

		// 第一个三角形
		out_indices.push_back(i0);
		out_indices.push_back(i1);
		out_indices.push_back(i2);

		// 第二个三角形
		out_indices.push_back(i1);
		out_indices.push_back(i3);
		out_indices.push_back(i2);
	}
}

void CreateWaterLineMesh(std::string dataPath, std::string outputPath, int rowSize, int colSize, bool* selectedGrid)
{
	std::ifstream dataStream(dataPath, std::ios::binary);
	if (dataStream)
	{
		std::unordered_map<uint32_t, std::vector<LineSet<Point3>>> waterLineSetsTable;
		readLineData(dataStream, selectedGrid, waterLineSetsTable, rowSize, colSize, 8, 4096.0f, true);

		for (auto& table : waterLineSetsTable)
		{
			if (!table.second.size())
				continue;
			int index = table.first;
			int pagex = index >> 16, pagey = index & 0xffff;	
			std::string path = outputPath + "water" + std::to_string(pagex) + "_" + std::to_string(pagey) + ".plane";
			FILE* file = nullptr;
			fopen_s(&file, path.c_str(), "wb");
			for (auto& lineset : table.second)
			{
				std::vector<RiverVertex> vert;
				std::vector<uint32_t> ind;
				generateCatmullRomPolylineMesh(lineset, 60.0f, vert, ind);
				int Num = (int)ind.size() / 3;
				fwrite(&Num, sizeof(int), 1, file);
				for (int idx : ind)
				{
					writeVertex(vert[idx], file);
				}
			}
			fclose(file);

		}
	}
}

void CreateRoadMask(std::unordered_map<uint32_t, std::vector<LineSet<Point3>>>& roadLineSetsTable, std::string outputPath)
{
	ThreadPool pool;
	for (auto& table : roadLineSetsTable)
	{
		if (table.second.size())
		{
			pool.enqueue([&table, outputPath]() {
				int index = table.first;
				int pagex = index >> 16;
				int pagey = index & 0xffff;
				std::string filePath = outputPath + "road" + std::to_string(pagex) + "_" + std::to_string(pagey) + ".tuple";

				std::vector<TwoTuples<uint32_t>> tuples;
				CalculateRoadHighPrecisionMesh(tuples, table.second, pagex, pagey, 32768.0f);
				if (tuples.size()) {
					writeMask(filePath, tuples, pagex, pagey);
				}
				}
			);
		}
	}
	pool.waitAll();
	pool.stop();
}

void CreateRoadLineMesh(std::unordered_map<int, std::vector< std::vector<Point3>>>& lineMap, std::string outputPath, int rowSize, int colSize, bool* selectedGrid)
{
	for (auto& table : lineMap)
	{
		if (!table.second.size())
			continue;

		int pagex = table.first >> 16, pagey = table.first & 0xffff;
		if (!selectedGrid[pagey * rowSize + pagex]) {
			continue;
		}
		std::string path = outputPath + "road" + std::to_string(pagex) + "_" + std::to_string(pagey) + ".mesh";
		FILE* file = nullptr;
		fopen_s(&file, path.c_str(), "wb");

		RoadSystemMeshes meshes;
		BuildRoadSystemFromLineSet(table.second, meshes);

		for (const auto& roadMesh : meshes.roadMeshes)
		{
			int Num = (int)roadMesh.indices.size() / 3;
			fwrite(&Num, sizeof(int), 1, file);
			for (int idx : roadMesh.indices)
			{
				writeVertex(roadMesh.vertices[idx], 0, file);
			}
		}

		for (const auto& interMesh : meshes.intersectionMeshes)
		{
			int Num = (int)interMesh.indices.size() / 3;
			fwrite(&Num, sizeof(int), 1, file);
			for (int idx : interMesh.indices)
			{
				writeVertex(interMesh.vertices[idx], 1, file);
			}
		}
		fclose(file);
	}
}

void SaveSampleLineToFile(const std::vector<std::vector<Point3>>& sampleLines, std::string outputPath)
{
	std::string path = outputPath + "SampleLine.bin";
	FILE* file = nullptr;
	fopen_s(&file, path.c_str(), "wb");
	for (const auto& line : sampleLines)
	{
		int Num = (int)line.size();
		fwrite(&Num, sizeof(int), 1, file);
		for (const auto& p : line)
		{
			writePoint(p, file);
		}
	}
	fclose(file);
}

void filterLineEndPoint(std::vector<std::vector<Point3>>& lines)
{
	for (auto& ls : lines)
	{
		if (ls.size() > 2 && length(ls[0] - ls[1]) < 5.0f)
		{
			ls.erase(ls.begin() + 1);
		}

		if (ls.size() > 2 && length(ls[ls.size() - 1] - ls[ls.size() - 2]) < 5.0f)
		{
			ls.erase(ls.begin() + ls.size() - 2);
		}
	}
}

void CreateRoadData(std::string dataPath, std::string outputPath, int rowSize, int colSize, bool* selectedGrid)
{
	std::ifstream dataStream(dataPath, std::ios::binary);
	if (dataStream)
	{
		std::vector<std::vector<Point3>> lines;
		readLinePointDataFromFile(dataStream, lines);
		filterLineEndPoint(lines);
		std::unordered_map<uint32_t, std::vector<LineSet<Point3>>> roadLineSetsTable;
		std::unordered_map<int, std::vector<std::vector<Point3>>> lineMap;
		std::vector<std::vector<Point3>> sampledLines;
		for (auto& line : lines) {
			std::vector<Point3> sampledLine;
			SampleCatmullRomPolyline(line, sampledLine, 0.1);
			Point3 vMin, vMax;
			caculateLineBounding(sampledLine, vMin, vMax);
			LineSet<Point3> lineSet = { sampledLine, vMin, vMax };
			pushToLineSetsTable(selectedGrid, lineSet, roadLineSetsTable, rowSize, colSize, 1, 32768.0f);
			line.clear();
			addToMap(sampledLine, line, lineMap, 1.0);
			sampledLines.push_back(sampledLine);
		}
		SaveSampleLineToFile(sampledLines, outputPath);
		CreateRoadMask(roadLineSetsTable, outputPath);
		//CreateRoadLineMesh(lineMap, outputPath, rowSize, colSize, selectedGrid);
	}
}

void CreateWaterData(std::string dataPath, std::string outputPath, int rowSize, int colSize, bool* selectedGrid)
{
	std::ifstream dataStream(dataPath, std::ios::binary);
	if (dataStream)
	{
		std::vector<std::vector<Point3>> lines;
		readLinePointDataFromFile(dataStream, lines);
		std::vector<std::vector<Point3>> sampledLines;
		for (auto& line : lines) {
			LineSet<Point3> lineSet = sampleCatmullRom(line, 10);
			sampledLines.push_back(lineSet.line);
		}

		SaveSampleLineToFile(sampledLines, outputPath);
	}
}
