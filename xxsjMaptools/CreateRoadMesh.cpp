#include "KarstDataHelper.h"

struct PolylineRoad
{
	PolylineRoad() = default;
	PolylineRoad(LineSet<Point3> lineSet)
	{
		points = lineSet.line;
	}
	PolylineRoad(std::vector<Point3> pts)
	{
		points = pts;
	}
	std::vector<Vector3> points;
	float halfWidth = 5.0f;
};

struct RoadArm
{
	int   roadIndex;
	int   endpointIdx;
	Vector3  dir;
	float halfWidth;
};

struct Node
{
	Vector3              pos;
	std::vector<RoadArm> arms;
};

void BuildRoadMeshFromPolyline(const PolylineRoad& road, RoadMesh& outMesh, float uvScale = 0.1f)
{
	outMesh.vertices.clear();
	outMesh.indices.clear();

	const auto& pts = road.points;
	if (pts.size() < 2) return;

	const Vector3 up(0, 1, 0);

	std::vector<uint32_t> leftIdx(pts.size());
	std::vector<uint32_t> rightIdx(pts.size());

	float accDist = 0.0f;

	for (size_t i = 0; i < pts.size(); ++i)
	{
		Vector3 forward;
		if (i == 0)
			forward = vnorm(pts[1] - pts[0]);
		else if (i == pts.size() - 1)
			forward = vnorm(pts[i] - pts[i - 1]);
		else
		{
			Vector3 f1 = vnorm(pts[i] - pts[i - 1]);
			Vector3 f2 = vnorm(pts[i + 1] - pts[i]);
			forward = vnorm(f1 + f2);
		}

		Vector3 right = vnorm(cross(forward, up));
		Vector3 left = right * -1.0f;

		if (i > 0)
			accDist += length(pts[i] - pts[i - 1]);

		float vCoord = accDist * uvScale;

		{
			RoadVertex v;
			v.pos = pts[i] + left * road.halfWidth;
			v.normal = up;
			v.uv = Vector2(0.0f, vCoord);
			leftIdx[i] = static_cast<uint32_t>(outMesh.vertices.size());
			outMesh.vertices.push_back(v);
		}

		{
			RoadVertex v;
			v.pos = pts[i] + right * road.halfWidth;
			v.normal = up;
			v.uv = Vector2(1.0f, vCoord);
			rightIdx[i] = static_cast<uint32_t>(outMesh.vertices.size());
			outMesh.vertices.push_back(v);
		}
	}

	for (size_t i = 0; i + 1 < pts.size(); ++i)
	{
		uint32_t l0 = leftIdx[i];
		uint32_t r0 = rightIdx[i];
		uint32_t l1 = leftIdx[i + 1];
		uint32_t r1 = rightIdx[i + 1];

		outMesh.indices.push_back(l0);
		outMesh.indices.push_back(r0);
		outMesh.indices.push_back(l1);

		outMesh.indices.push_back(r0);
		outMesh.indices.push_back(r1);
		outMesh.indices.push_back(l1);
	}
}

int FindExistingNode(const std::vector<Node>& nodes, const Vector3& p, float mergeDist = 0.5f)
{
	for (int i = 0; i < (int)nodes.size(); ++i)
	{
		if (length(nodes[i].pos - p) < mergeDist)
			return i;
	}
	return -1;
}

void DetectEndpointNodes(const std::vector<PolylineRoad>& roads, std::vector<Node>& outNodes, float mergeDist = 0.5f)
{
	outNodes.clear();

	const int roadCount = (int)roads.size();
	if (roadCount == 0) return;

	for (int r = 0; r < roadCount; ++r)
	{
		const auto& pts = roads[r].points;
		if (pts.size() < 2) continue;

		int startIdx = 0;
		int endIdx = (int)pts.size() - 1;

		{
			const Vector3& p = pts[startIdx];

			int nodeIdx = FindExistingNode(outNodes, p, mergeDist);
			if (nodeIdx < 0)
			{
				Node n;
				n.pos = p;
				outNodes.push_back(n);
				nodeIdx = (int)outNodes.size() - 1;
			}

			Node& node = outNodes[nodeIdx];

			Vector3 dir = vnorm(pts[startIdx + 1] - pts[startIdx]);

			RoadArm arm;
			arm.roadIndex = r;
			arm.endpointIdx = startIdx;
			arm.dir = dir;
			arm.halfWidth = roads[r].halfWidth;
			node.arms.push_back(arm);
		}

		if (endIdx != startIdx)
		{
			const Vector3& p = pts[endIdx];

			int nodeIdx = FindExistingNode(outNodes, p, mergeDist);
			if (nodeIdx < 0)
			{
				Node n;
				n.pos = p;
				outNodes.push_back(n);
				nodeIdx = (int)outNodes.size() - 1;
			}

			Node& node = outNodes[nodeIdx];

			Vector3 dir = vnorm(pts[endIdx - 1] - pts[endIdx]);

			RoadArm arm;
			arm.roadIndex = r;
			arm.endpointIdx = endIdx;
			arm.dir = dir;
			arm.halfWidth = roads[r].halfWidth;
			node.arms.push_back(arm);
		}
	}
}

void ApplyEndpointShrink(const std::vector<Node>& nodes,
	std::vector<PolylineRoad>& roadsForRender,
	float shrinkDist)
{
	const int nodeCount = (int)nodes.size();
	for (int n = 0; n < nodeCount; ++n)
	{
		const Node& node = nodes[n];
		for (const auto& arm : node.arms)
		{
			int r = arm.roadIndex;
			int idx = arm.endpointIdx;

			if (r < 0 || r >= (int)roadsForRender.size()) continue;
			auto& pts = roadsForRender[r].points;
			if (idx < 0 || idx >= (int)pts.size()) continue;

			int neighborIdx = (idx == 0) ? 1 : (idx - 1);
			if (neighborIdx < 0 || neighborIdx >= (int)pts.size()) continue;

			Vector3 p0 = pts[idx];
			Vector3 p1 = pts[neighborIdx];
			float segLen = length(p1 - p0);
			if (segLen <= 1e-5f) continue;

			Vector3 newPos = node.pos + arm.dir * shrinkDist;
			pts[idx] = newPos;
			if (shrinkDist >= segLen)
			{
				if(idx == 0)
				{
					int offset = 1;
					while (shrinkDist >= segLen && offset + 1 < pts.size() && offset >= 0)
					{
						segLen += length(pts[offset + 1] - pts[offset]);
						pts.erase(pts.begin() + offset);
						offset++;
					}
				}
				else if (idx == (int)pts.size() - 1)
				{
					int offset = pts.size() - 2;
					while (shrinkDist >= segLen && offset < pts.size() && offset >= 1)
					{
						segLen += length(pts[offset] - pts[offset - 1]);
						pts.erase(pts.begin() + offset);
						offset--;
					}
				}
			}
		}
	}
}

struct IntersectionBuilder
{
	static void BuildIntersectionPatch(const Node& node, float shrinkDist, RoadMesh& outMesh)
	{
		outMesh.vertices.clear();
		outMesh.indices.clear();

		const auto& arms = node.arms;
		if (arms.size() < 2) return;

		struct ArmInfo
		{
			RoadArm arm;
			float angle;
			Vector3 leftPoint;
			Vector3 rightPoint;
		};

		std::vector<ArmInfo> list;
		list.reserve(arms.size());
		const Vector3 up(0, 1, 0);

		for (auto& a : arms)
		{
			Vector3 dir = vnorm(a.dir);
			Vector3 centerOnRoad = node.pos + dir * shrinkDist;

			Vector3 right = vnorm(cross(dir, up));
			Vector3 left = right * -1.0f;

			ArmInfo info;
			info.arm = a;
			info.angle = std::atan2(dir.z, dir.x);
			info.leftPoint = centerOnRoad + left * a.halfWidth;
			info.rightPoint = centerOnRoad + right * a.halfWidth;

			list.push_back(info);
		}

		std::sort(list.begin(), list.end(),
			[](const ArmInfo& a, const ArmInfo& b)
			{
				return a.angle < b.angle;
			});

		std::vector<Vector3> polyPoints;
		polyPoints.reserve(list.size() * 2);
		for (auto& info : list)
		{
			polyPoints.push_back(info.leftPoint);
			polyPoints.push_back(info.rightPoint);
		}

		float lift = 0.01f;

		uint32_t centerIndex = 0;
		RoadVertex centerV;
		centerV.pos = node.pos;
		centerV.pos.y += lift;
		centerV.normal = Vector3(0, 1, 0);
		centerV.uv = Vector2(0.5f, 0.5f);
		outMesh.vertices.push_back(centerV);

		float uvScale = 0.1f;
		for (size_t i = 0; i < polyPoints.size(); ++i)
		{
			RoadVertex v;
			v.pos = polyPoints[i];
			v.pos.y += lift;
			v.normal = Vector3(0, 1, 0);

			float u = 0.5f + (polyPoints[i].x - node.pos.x) * uvScale;
			float w = 0.5f + (polyPoints[i].z - node.pos.z) * uvScale;
			v.uv = Vector2(u, w);

			outMesh.vertices.push_back(v);
		}

		uint32_t count = static_cast<uint32_t>(polyPoints.size());
		for (uint32_t i = 0; i < count; ++i)
		{
			uint32_t i0 = centerIndex;
			uint32_t i1 = 1 + i;
			uint32_t i2 = 1 + ((i + 1) % count);

			outMesh.indices.push_back(i0);
			outMesh.indices.push_back(i1);
			outMesh.indices.push_back(i2);
		}
	}
};

Vector3 CatmullRom(const Vector3& p0, const Vector3& p1, const Vector3& p2, const Vector3& p3, float t)
{
	// t ∈ [0,1]
	float t2 = t * t;
	float t3 = t2 * t;

	// 标准 Catmull–Rom
	Vector3 r;
	r.x = 0.5f * ((2.0f * p1.x) +
		(-p0.x + p2.x) * t +
		(2.0f * p0.x - 5.0f * p1.x + 4.0f * p2.x - p3.x) * t2 +
		(-p0.x + 3.0f * p1.x - 3.0f * p2.x + p3.x) * t3);
	r.y = 0.5f * ((2.0f * p1.y) +
		(-p0.y + p2.y) * t +
		(2.0f * p0.y - 5.0f * p1.y + 4.0f * p2.y - p3.y) * t2 +
		(-p0.y + 3.0f * p1.y - 3.0f * p2.y + p3.y) * t3);
	r.z = 0.5f * ((2.0f * p1.z) +
		(-p0.z + p2.z) * t +
		(2.0f * p0.z - 5.0f * p1.z + 4.0f * p2.z - p3.z) * t2 +
		(-p0.z + 3.0f * p1.z - 3.0f * p2.z + p3.z) * t3);
	return r;
}

Vector3 Lerp(const Vector3& a, const Vector3& b, float t)
{
	return a * (1.0f - t) + b * t;
}

void SampleCatmullRomPolyline(const std::vector<Vector3>& ctrl, std::vector<Vector3>& out, float step, float smooth)
{
	out.clear();
	if (ctrl.size() < 2)
	{
		out = ctrl;
		return;
	}

	std::vector<Vector3> c;
	c.reserve(ctrl.size() + 2);
	c.push_back(ctrl.front());
	for (auto& p : ctrl) c.push_back(p);
	c.push_back(ctrl.back());

	int n = (int)c.size();

	out.push_back(ctrl.front());

	for (int i = 1; i < n - 2; ++i)
	{
		const Vector3& p0 = c[i - 1];
		const Vector3& p1 = c[i];
		const Vector3& p2 = c[i + 1];
		const Vector3& p3 = c[i + 2];

		for (float t = step; t < 1.0f; t += step)
		{
			Vector3 straight = Lerp(p1, p2, t);
			Vector3 curve = CatmullRom(p0, p1, p2, p3, t);
			Vector3 finalPos = Lerp(straight, curve, smooth);

			out.push_back(finalPos);
		}

		out.push_back(p2);
	}

	if (!out.empty())
		out.back() = ctrl.back();
}


void BuildRoadSystem(const std::vector<PolylineRoad>& roads, RoadSystemMeshes& outSystem, float mergeDist = 0.05f, float shrinkDist = 10.0f)
{
	outSystem.roadMeshes.clear();
	outSystem.intersectionMeshes.clear();

	std::vector<Node> nodes;
	DetectEndpointNodes(roads, nodes, mergeDist);

	std::vector<PolylineRoad> renderRoads = roads;
	ApplyEndpointShrink(nodes, renderRoads, shrinkDist);

	for (const auto& r : renderRoads)
	{
		RoadMesh m;
		BuildRoadMeshFromPolyline(r, m);
		outSystem.roadMeshes.push_back(std::move(m));
	}

	for (auto& node : nodes)
	{
		RoadMesh m;
		IntersectionBuilder::BuildIntersectionPatch(node, shrinkDist, m);
		if (!m.vertices.empty())
			outSystem.intersectionMeshes.push_back(std::move(m));
	}
}

void BuildRoadSystemFromLineSet(std::vector<std::vector<Point3>>& lineSets, RoadSystemMeshes& outMeshes)
{
	std::vector<PolylineRoad> roads;

	for (auto& ls : lineSets)
	{
		PolylineRoad road(ls);
		roads.push_back(std::move(road));
	}

	BuildRoadSystem(roads, outMeshes);
}
