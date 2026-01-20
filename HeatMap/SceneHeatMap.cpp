///////////////////////////////////////////////////////////////////////////////
//
// @file SceneHeatMap.cpp
// @author luoshuquan
// @date 2020/04/03 Friday
// @version 1.0
//
// @copyright Copyright (c) PixelSoft Corporation. All rights reserved.
//
///////////////////////////////////////////////////////////////////////////////

#include "SceneHeatMap.h"
#include "EchoEngine.h"
#include "EchoStringConverter.h"
#include "EchoWorldSystem.h"
#include "EchoWorldManager.h"
#include "EchoSceneCommonInfoMgr.h"
#include "EchoSceneCommonResource.h"
#include "EchoPageStrategy.h"
#include "EchoPageManager.h"
#include "EchoSubMesh.h"
#include "EchoMesh.h"
#include "EchoMeshMgr.h"
#include "EchoMaterialManager.h"
#include "EchoTextureManager.h"
#include "EchoResourceManagerFactory.h"
#include "EchoImage.h"
#include "EchoMountainInfo.h"
#include "EchoMountainManager.h"
#include "EchoSubMountain.h"
#include "EchoBuildingObject.h"
#include "EchoRoleObject.h"
//#include "RTree.h"
//#include "TimerManager.h"
//#include "EchoCameraObj.h"
//#include "EchoClientInfo.h"

#define PI 3.1415926535
/*
namespace {

	using namespace std;
	struct node
	{
		int x, z;
		bool operator==(const node& n) {
			if (x == n.x && z == n.z) return true;
			return false;
		}
	};
	struct RTRect
	{
		RTRect() {}

		RTRect(int a_minX, int a_minY, int a_maxX, int a_maxY)
		{
			min[0] = a_minX;
			min[1] = a_minY;

			max[0] = a_maxX;
			max[1] = a_maxY;
		}

		int min[2];
		int max[2];
		inline int getArea() {
			return (max[0] - min[0])*(max[1] - min[1]);
		}
		bool operator==(const RTRect& RT)
		{
			if (this->max[0] == RT.max[0] &&this->max[1] == RT.max[1] &&this->min[0] == RT.min[0] && this->min[1] == RT.min[1])
				return true;
			else return false;
		}
		RTRect operator * (const float& n) {
			RTRect rec;
			for (int i = 0; i < 2; i++) {
				rec.max[i] = this->max[i] * n;
				rec.min[i] = this->min[i] * n;
			}
			return rec;
		}

	};

	typedef RTRect ValueType;
	typedef RTree<ValueType, int, 2, float> MyRTree;
	RTRect m_curRec;
	std::vector<RTRect> searchResult;
	std::vector<RTRect> branchRects;

	struct RTreeData
	{
		RTRect rec;
		std::vector<node*> points;
		int speed;
	};

	MyRTree tree[3];
	MyRTree branchTree;

	bool testcallback(ValueType value) {
		for (int index = 0; index < 2; ++index)
		{
			if (value.min[index] > m_curRec.max[index] ||
				m_curRec.min[index] > value.max[index])
			{
				return true;  // 
			}
		}
		searchResult.push_back(value);

		return true;
	}

	bool branchCallback(ValueType value) {
		branchRects.push_back(value);
		return true;
	}

} // end namespace
*/
struct SimpleObjInfo
{
	Echo::AxisAlignedBox aabb;
	unsigned int tri_count = 0u;

};
struct subMountainInfo
{
	std::vector<SimpleObjInfo> meshinfo;
	Echo::Matrix4 meshMatrix;
};
typedef void(SceneHeatMapPrivate::*PROC_FUNC)(const Echo::cJSON *);

struct fucType {
	std::unordered_set<std::string> str;
	PROC_FUNC func;
};
class SceneHeatMapPrivate
{
public:
	SceneHeatMapPrivate()
	{
		initfunc();
	}
	~SceneHeatMapPrivate() {
		if (hmap != nullptr) {
		for (int i = 0; i < 3; ++i) {
			delete[] (hmap[i]);
			hmap[i] = nullptr;
		}
		delete[] hmap;
		hmap = nullptr;

		}
	}
	const float HMapMaxSize = 4096.f;
	const float TriCount = 1000.f;
	float maxTri = 45000;
	float minTri = 20000;  //
	const float min_S = 200;  //
	const int step = 400;  //
	float** hmap = nullptr;
	int hm_width = 0,
		hm_height = 0;
	float pixelSize = 1.0;

	bool Lod[3]{ 0 };
	float hm_maxTri[3]{ 0.f };

	std::string picName{""};

	const char* datapath = nullptr;
	int times = 0;

	//EchoCameraObj * m_pCameraObj = nullptr;

	Echo::FloatRect range;
	//key: typename form Json
	std::map<std::string, fucType> all_func;
	// key: mesh name
	//key: typename form Json
	std::map<Echo::String, PROC_FUNC> proc_func;
	// value: mesh info
	std::unordered_map<Echo::Name, std::vector<SimpleObjInfo>, Echo::NameIDHasher> mesh_info;
	std::unordered_map<Echo::Name, std::vector<subMountainInfo>, Echo::NameIDHasher> mountain_info;
	std::unordered_map<Echo::Name, std::vector<SimpleObjInfo>, Echo::NameIDHasher> building_info;
	std::unordered_map<Echo::Name, std::vector<SimpleObjInfo>, Echo::NameIDHasher> role_info;

	void initfunc() {
		all_func["Entity"] = { {"Entity","Entities"},&SceneHeatMapPrivate::processEntities };
		all_func["Building"] = { {"Building","Buildings"},&SceneHeatMapPrivate::processBuilding };
		all_func["Mountain Object"] = { {"Mountain Object","Mountain","Mountainobj","Mountain obj", "Mountain Objects"}, &SceneHeatMapPrivate::processMountainObjects };
		all_func["Role"] = { {"Role"}, &SceneHeatMapPrivate::processRole };
	}

	bool GetObjMatrix(Echo::cJSON* pChild, Echo::Matrix4 &mat)
	{
		using namespace Echo;
		cJSON *pPos = cJSON_GetObjectItem(pChild, "Pos");
		if (nullptr == pPos) return false;
		cJSON *pScale = cJSON_GetObjectItem(pChild, "Scale");
		if (nullptr == pScale) return false;
		cJSON *pOri = cJSON_GetObjectItem(pChild, "Ori");
		if (nullptr == pOri) return false;

		Vector3 pos = Echo::StringConverter::parseVector3(pPos->valuestring);
		Vector3 scale = Echo::StringConverter::parseVector3(pScale->valuestring);
		Quaternion ori = Echo::StringConverter::parseQuaternion(pOri->valuestring);
		mat.makeTransform(pos, scale, ori);

		return true;
	}

	int getAllSubTriCount(Echo::MeshPtr ptrMesh) {
		int tricount = 0;
		auto sub_num = ptrMesh->getNumSubMesh();
		for (decltype(sub_num) s = 0; s < sub_num; ++s)
		{
			Echo::SubMesh * sub_mesh = ptrMesh->getSubMesh(s);
			tricount += sub_mesh->m_TriangleCount;
		}
		return tricount;
	}

/*
	void InsertNodeToRTree(MyRTree& T,float f = 0.2f) 
	{
		T.Search(m_curRec.min, m_curRec.max, testcallback);
		int n = searchResult.size();
		if (0 == n)
		{
			if(!(m_curRec.getArea() > 4000*4000))
				T.Insert(m_curRec.min, m_curRec.max, m_curRec);
			return;
		}
		bool flag = false;
		for (auto it_r = searchResult.begin(); it_r != searchResult.end(); ++it_r)
		{
			//int curRec_S = (m_curRec.max[0] - m_curRec.min[0]) * (m_curRec.max[1] - m_curRec.min[1]);
			//int value_S = (it_r->max[0] - it_r->min[0]) * (it_r->max[1] - it_r->min[1]);
			int w = Min(it_r->max[0], m_curRec.max[0]) - Max(it_r->min[0], m_curRec.min[0]);
			int h = Min(it_r->max[1], m_curRec.max[1]) - Max(it_r->min[1], m_curRec.min[1]);
			int overlap_S = w * h;
			//
			if (!(overlap_S*1.f / m_curRec.getArea()*1.f > f || overlap_S*1.f / it_r->getArea()*1.f > f || m_curRec.getArea() < min_S || it_r->getArea() < min_S)) continue;

			for (int index = 0; index < 2; ++index)
			{
				m_curRec.min[index] = Min(it_r->min[index], m_curRec.min[index]);
				m_curRec.max[index] = Max(it_r->max[index], m_curRec.max[index]);
			}
			T.Remove(it_r->min, it_r->max, *it_r);
			flag = true;

		}
		searchResult.clear();
		if(flag)
			InsertNodeToRTree(T,f);
		else 
			T.Insert(m_curRec.min, m_curRec.max, m_curRec);
	}
*/
	//
	void setPicData(const Echo::Vector3* objPoints, Echo::Matrix4 objMatrix,SimpleObjInfo info, int lod) {
		Echo::Vector3 tmpPoints{ 0.0,0.0,0.0 };
		float maxL = 0.0, maxR = 0.0, maxT = 0.0, maxB = 0.0;
		tmpPoints = objMatrix * objPoints[0];
		maxL = maxR = tmpPoints.x;
		maxT = maxB = tmpPoints.z;
		for (int i = 1; i < 8; ++i) {
			tmpPoints = objMatrix *objPoints[i];
			if (tmpPoints.x < maxL) maxL = tmpPoints.x;
			if (tmpPoints.x > maxR) maxR = tmpPoints.x;
			if (tmpPoints.z < maxT) maxT = tmpPoints.z;
			if (tmpPoints.z > maxB) maxB = tmpPoints.z;
		}
		Echo::FloatRect tmp{ maxL / pixelSize,maxT / pixelSize,maxR / pixelSize,maxB / pixelSize };
		tmp.left -= range.left;
		tmp.right -= range.left;
		tmp.top -= range.top;
		tmp.bottom -= range.top;

		int xStart = /*m_curRec.min[0] =*/ Echo::Math::Clamp(int(tmp.left + 0.5f), 0, hm_width - 1);
		int xEnd =/* m_curRec.max[0] = */Echo::Math::Clamp(int(tmp.right + 0.5f), 0, hm_width - 1);
		int yStart = /*m_curRec.min[1] =*/ Echo::Math::Clamp(int(tmp.top + 0.5f), 0, hm_height - 1);
		int yEnd =/* m_curRec.max[1] = */Echo::Math::Clamp(int(tmp.bottom + 0.5f), 0, hm_height - 1);

		//bool flag = true;
		for (int y = yStart; y <= yEnd; ++y) {
			for (int x = xStart; x <= xEnd; ++x) {
				int index = hm_width*y + x;
				if (index > hm_width*hm_height || index < 0)
					continue;
				hmap[lod][index] += info.tri_count / TriCount;
				if (hmap[lod][index] > hm_maxTri[lod]) hm_maxTri[lod] = hmap[lod][index];
				//if (flag && (hmap[lod][index] > (minTri  / TriCount - lod))) {
				//	flag = false;
				//	InsertNodeToRTree(tree[lod],0.f);
				//}
			}
		}
	}


	void GetObjPosition(std::vector<SimpleObjInfo> objInfo, Echo::Matrix4 objMatrix)
	{
		int i = 0;
		while (i<3) {
			if (Lod[i] == false) { ++i; continue; }
			int lod = i;
			if (objInfo.empty()) return;
			while (objInfo.size()-1 < lod && lod >= 0) {
				lod--;
			}
			auto iter = objInfo.at(lod);
			const Echo::Vector3* objPoints = iter.aabb.getAllCorners();
			setPicData(objPoints, objMatrix, iter, i);
			++i;
		}
	}

	/*
	void pickPoints(MyRTree& T,cJSON* Rectangles) 
	{
		T.Search(m_curRec.min, m_curRec.max, testcallback);
		
		for (auto it_result = searchResult.begin(); it_result != searchResult.end(); ++it_result) {
			RTreeData* data = new RTreeData;
			data->rec = *it_result*pixelSize;
			int s = it_result->max[0] - it_result->min[0];
			if (s < 6) {
				delete data;
				data = nullptr;
				continue;
			}
			cJSON* rectangle = cJSON_CreateObject();
			cJSON* rec = cJSON_CreateObject();
			cJSON_AddNumberToObject(rec, "xStart", data->rec.min[0] + (range.left)*pixelSize);
			cJSON_AddNumberToObject(rec, "zStart", data->rec.min[1] + (range.top)*pixelSize);
			cJSON_AddNumberToObject(rec, "xEnd", data->rec.max[0] + (range.left)*pixelSize);
			cJSON_AddNumberToObject(rec, "zEnd", data->rec.max[1] + (range.top)*pixelSize);

			int period;
			if (it_result->getArea() > 100000) {
				s = 20;
				data->speed = 50;
				period = 4.0;
			}
			else {
				data->speed = 30;
				s = 10;
				period = 3.0;
			}
			cJSON* speed = cJSON_CreateNumber(data->speed);
			cJSON* points = cJSON_CreateArray();

			RTRect tmpRec(it_result->min[0] + range.left, 
						it_result->min[1] + range.left,
						it_result->max[0] + range.left, 
						it_result->max[1] + range.left);
			int h = tmpRec.max[1] - tmpRec.min[1];
			int w = tmpRec.max[0] - tmpRec.min[0];
			double radian;
			if (h < w) {
				int xstart = tmpRec.min[0];
				radian = xstart / ((double)w / period)*PI;
				double y = sin(radian) * (h / 2.0) + tmpRec.min[1] + h / 2;
				for (; xstart < tmpRec.max[0]; xstart += s) {
					//
					radian = xstart / ((double)w / period)*PI;
					y = sin(radian) * (h / 2.0) + tmpRec.min[1] + h / 2;
					cJSON* pos = cJSON_CreateObject();
					cJSON_AddNumberToObject(pos, "x", xstart*pixelSize);
					cJSON_AddNumberToObject(pos, "z", y*pixelSize);
					cJSON_AddItemToArray(points, pos);
				}
			}
			else {
				int zstart = tmpRec.min[1];
				radian = zstart / ((double)h / 4.0)*PI;
				double x = sin(radian) * (w / 2.0) + tmpRec.min[0] + w / 2;
				for (; zstart < tmpRec.max[1]; zstart += s) {
					//
					radian = zstart / ((double)h / 4.0)*PI;
					x = sin(radian) * (w / 2.0) + tmpRec.min[0] + w / 2;
					cJSON* pos = cJSON_CreateObject();
					cJSON_AddNumberToObject(pos, "x", x*pixelSize);
					cJSON_AddNumberToObject(pos, "z", zstart*pixelSize);
					cJSON_AddItemToArray(points, pos);
				}
			}
			delete data;
			cJSON_AddItemToObject(rectangle, "rec", rec);
			cJSON_AddItemToObject(rectangle, "points", points);
			cJSON_AddItemToObject(rectangle, "speed", speed);
			cJSON_AddItemToArray(Rectangles, rectangle);
			T.Remove(it_result->min, it_result->max, *it_result);
		}
		searchResult.clear();
	}

	//void LoadJson(std::string filename) {
	//	m_pCameraObj->LoadCameraInfo(filename.c_str());
	//}

	bool OutPutRTreeData(const Echo::String &imgPath) {
		int lod = 0;
		int level[3] = { 1,0,0 };
		while (lod < 3) {
			if (Lod[lod] == false) {
				++lod; continue;
			}

			tree[lod].GetBranchsRec(level[lod], branchCallback);
			for (auto it_branch = branchRects.begin(); it_branch != branchRects.end(); ++it_branch)
			{
				m_curRec.min[0] = it_branch->min[0];
				m_curRec.min[1] = it_branch->min[1];
				m_curRec.max[0] = it_branch->max[0];
				m_curRec.max[1] = it_branch->max[1];
				InsertNodeToRTree(branchTree, 0.4f);
			}
			branchRects.clear();

			cJSON* root = cJSON_CreateObject();
			cJSON* JsonVersion = cJSON_CreateNumber(1.0);
			cJSON* Rectangles = cJSON_CreateArray();
			//
			int row = 0;
			for (int zStart = 0; zStart < hm_height; zStart += step) {
				m_curRec.min[1] = zStart;
				m_curRec.max[1] = zStart + step;
				++row;
				if (0 == row % 2) {
					for (int xStart = hm_width; xStart > 0; xStart--) {
						m_curRec.min[0] = xStart - step;
						m_curRec.max[0] = xStart;
						pickPoints(branchTree, Rectangles);
					}
				}
				else {
					for (int xStart = 0; xStart < hm_width; xStart += step) {
						m_curRec.min[0] = xStart;
						m_curRec.max[0] = xStart + step;
						pickPoints(branchTree, Rectangles);
					}
				}
			}
			cJSON_AddItemToObject(root, "JsonVersion", JsonVersion);
			cJSON_AddItemToObject(root, "Rectangles", Rectangles);
			char* datastream = cJSON_Print(root);
			std::string filename = imgPath + "recData#" + Echo::StringConverter::toString(lod) + '#' + Echo::StringConverter::toString(minTri) + ".json";
			ofstream file(filename);
			if (!file) return false;
			file << datastream;
			file.close();
			cJSON_Delete(root);
			branchTree.RemoveAll();
			tree[lod].RemoveAll();

			++lod;

		}
		return true;

	}
*/
	void OutPutPNG(const Echo::String &imgPath)
	{
		using namespace std;
		Echo::Image hmapImg;
		hmapImg.setHeight(hm_height);
		hmapImg.setWidth(hm_width);
		//hmapImg.setFormat(Echo::PixelFormat::PF_R8G8B8A8);
		hmapImg.setFormat(Echo::PixelFormat::PF_R8G8B8);
		hmapImg.allocMemory();
		Echo::ColorValue color;
		int lod = 0;
		while (lod < 3) {
			if (Lod[lod] == false) {
				++lod; continue;
			}
			for (int z = 0; z < hm_height; ++z) {
				for (int x = 0; x < hm_width; ++x) {
					hmapImg.setColourAt(Echo::ColorValue::Black, x, z, 0);
					float tmp = hmap[lod][z*hm_width + x] / hm_maxTri[lod];
					tmp = Echo::Math::Clamp(tmp, 0.f, 1.f);
					tmp = Echo::Math::Pow(tmp, 1.f / 2.2f);
					//color.a = tmp;
					//color.r = color.g = color.b = tmp;
					float c = hmap[lod][z*hm_width + x] * TriCount;
					if (c > maxTri)
					{
						color.r = tmp;
						color.g = color.b = 0;
					}
					else if (c < minTri)
					{
						color.b = tmp;
						color.r = color.g = 0;
					}
					else
					{
						color.g = tmp;
						color.r = color.b = 0;
					}
					hmapImg.setColourAt(color, x, z, 0);
				}
			}
			int num = int(hm_maxTri[lod] * TriCount);
			if ("" == picName) picName = "ALL";
			std::string name = imgPath + picName + '#' + Echo::StringConverter::toString(lod) + '#' + Echo::StringConverter::toString(num) + "red" + Echo::StringConverter::toString(maxTri) + ".png";
			hmapImg.save(name);
			hm_maxTri[lod] = 0;
			memset(hmap[lod], 0, (hm_height*hm_width) * sizeof(float));
			Lod[lod] = false;
			++lod;
		}
		hmapImg.freeMemory();
		proc_func.clear();
		picName = "";
	}

	void processEntities(const Echo::cJSON * entity_arr)
	{
		using namespace Echo;

		for (cJSON *pChild = entity_arr->child; pChild != nullptr; pChild = pChild->next)
		{
			Matrix4 meshMatrix;

			if (!GetObjMatrix(pChild, meshMatrix)) continue;
			cJSON *pRes = cJSON_GetObjectItem(pChild, "res");
			if (nullptr == pRes) continue;

			Name resName(pRes->valuestring);
			std::vector<SimpleObjInfo> info;
			auto it = mesh_info.find(resName);
			if (it != mesh_info.end()) {
				info = it->second;
			}
			else
			{
				MeshPtr ptrMesh = MeshMgr::instance()->getByName(resName);
				if (ptrMesh.isNull())
				{
					try {
						ptrMesh = MeshMgr::instance()->prepare(resName);
					}
					catch (...) {}
				}
				if (ptrMesh.isNull())
					continue;

				SimpleObjInfo lodinfo;
				lodinfo.tri_count = getAllSubTriCount(ptrMesh);
				lodinfo.aabb = ptrMesh->getAABB();
				if (lodinfo.aabb.isNull() || lodinfo.aabb.isInfinite()) continue;
				info.push_back(lodinfo);

				for (auto it_lod = ptrMesh->m_vtLodMesh.begin(); it_lod != ptrMesh->m_vtLodMesh.end(); ++it_lod) {
					MeshPtr ptr = *it_lod;
					lodinfo.aabb = ptr->getAABB();
					if (lodinfo.aabb.isNull() || lodinfo.aabb.isInfinite()) continue;
					lodinfo.tri_count = getAllSubTriCount(ptr);
					info.push_back(lodinfo);

				}
				mesh_info[resName] = info;

				if (ptrMesh->m_bVirMesh)
				{
					if(ptrMesh->mRealMesh.isNull()) continue;
					const Name &resRealName = ptrMesh->mRealMesh->getName();
					mesh_info[resRealName] = info;
				}
			}
			GetObjPosition(info, meshMatrix);
		}

		MeshMgr::instance()->destoryUnreferenced();
		MaterialManager::instance()->destoryUnreferenced();
		TextureManager::instance()->destoryUnreferenced();
	}

	void processMountainObjects(const Echo::cJSON * mountain_arr)
	{
		using namespace Echo;
		for (cJSON * pChild = mountain_arr->child; pChild != nullptr; pChild = pChild->next) {

			Echo::Matrix4 mountainMatrix;
			if (!GetObjMatrix(pChild, mountainMatrix)) continue;
			cJSON *pRes = cJSON_GetObjectItem(pChild, "res");
			if (nullptr == pRes) continue;

			Name resName(pRes->valuestring);
			std::vector<subMountainInfo> mountainInfo;
			auto it = mountain_info.find(resName);
			if (it != mountain_info.end()) {
				mountainInfo = it->second;
			}
			else
			{
				MountainInfoPtr ptrMountain = MountainManager::instance()->getByName(resName);
				if (ptrMountain.isNull())
				{
					try {
						ptrMountain = MountainManager::instance()->prepare(resName);
					}
					catch (...) {}
				}
				if (ptrMountain.isNull())
					continue;

				auto sub_num = ptrMountain->getNumSubMountain();
				for (decltype(sub_num) s = 0; s < sub_num; ++s)
				{
					EntitySubMountain* subMountain = dynamic_cast<EntitySubMountain*>(ptrMountain->getSubMountain(s));
					subMountainInfo submountainInfo;
					submountainInfo.meshMatrix.makeTransform(subMountain->getPosition(), subMountain->getScale(), subMountain->getOrientation());

					MeshPtr ptrMesh = MeshMgr::instance()->getByName(Name(subMountain->meshName));
					if (ptrMesh.isNull())
					{
						try {
							ptrMesh = MeshMgr::instance()->prepare(Name(subMountain->meshName));
						}
						catch (...) {}
					}
					if (ptrMesh.isNull())
						continue;

					std::vector<SimpleObjInfo> info;
					auto it = mesh_info.find(ptrMesh->getName());
					if (it != mesh_info.end()) {
						info = it->second;
					}
					else
					{
						SimpleObjInfo lodinfo;
						lodinfo.tri_count = getAllSubTriCount(ptrMesh);
						lodinfo.aabb = ptrMesh->getAABB();
						if (lodinfo.aabb.isNull() || lodinfo.aabb.isInfinite()) continue;
						info.push_back(lodinfo);

						for (auto it_lod = ptrMesh->m_vtLodMesh.begin(); it_lod != ptrMesh->m_vtLodMesh.end(); ++it_lod) {
							MeshPtr ptr = *it_lod;
							lodinfo.aabb = ptr->getAABB();
							if (lodinfo.aabb.isNull() || lodinfo.aabb.isInfinite()) continue;
							lodinfo.tri_count = getAllSubTriCount(ptr);
							info.push_back(lodinfo);

						}
						mesh_info[ptrMesh->getName()] = info;

						if (ptrMesh->m_bVirMesh)
						{
							if (ptrMesh->mRealMesh.isNull()) continue;
							const Name &resRealName = ptrMesh->mRealMesh->getName();
							mesh_info[resRealName] = info;
						}

					}
					submountainInfo.meshinfo = info;
					mountainInfo.push_back(submountainInfo);

				}  // end for
				mountain_info[resName] = mountainInfo;

			}  // end else

			for (auto it = mountainInfo.begin(); it != mountainInfo.end(); ++it) {
				GetObjPosition(it->meshinfo, mountainMatrix*it->meshMatrix);
			}

		}  // end for
		MeshMgr::instance()->destoryUnreferenced();
		MaterialManager::instance()->destoryUnreferenced();
		TextureManager::instance()->destoryUnreferenced();
		MountainManager::instance()->destoryUnreferenced();
	}

	void processBuilding(const Echo::cJSON *building_arr)
	{
		using namespace Echo;
		for (cJSON * pChild = building_arr->child; pChild != nullptr; pChild = pChild->next)
		{
			Echo::Matrix4 buildingMatrix;
			if (!GetObjMatrix(pChild, buildingMatrix)) continue;
			cJSON *pRes = cJSON_GetObjectItem(pChild, "res");
			if (nullptr == pRes) continue;

			Name resName(pRes->valuestring);
			std::vector<SimpleObjInfo> buildingInfo;
			auto it = building_info.find(resName);
			if (it != building_info.end()) {
				buildingInfo = it->second;
			}
			else
			{
				SimpleObjInfo lodinfo;
				BuildingObject * ptrBuilding = new Echo::BuildingObject(resName);
				ptrBuilding->loadFromJson(resName.c_str(), true);
				if (ptrBuilding->getDrawCount() == 0)
				{
					delete ptrBuilding;
					return;
				}
				ptrBuilding->updateBounds();
				lodinfo.aabb.merge(ptrBuilding->getBoundingBox());
				if (lodinfo.aabb.isNull() || lodinfo.aabb.isInfinite())
				{
					delete ptrBuilding;
					return;
				}
				int lod = 0;
				while (ptrBuilding->getTriangleCount(lod) != 0) {
					if (lod > 2) break;
					lodinfo.tri_count = ptrBuilding->getTriangleCount(lod);
					buildingInfo.push_back(lodinfo);
					++lod;
				}
				if (0 == buildingInfo.size()) {
					delete ptrBuilding;
					return;
				}
				building_info[resName] = buildingInfo;
				delete ptrBuilding;
			}
			GetObjPosition(buildingInfo, buildingMatrix);
		}
		MeshMgr::instance()->destoryUnreferenced();
		MaterialManager::instance()->destoryUnreferenced();
		TextureManager::instance()->destoryUnreferenced();
	}

	void processRole(const Echo::cJSON *role_arr)
	{
		using namespace Echo;
		for (cJSON * pChild = role_arr->child; pChild != nullptr; pChild = pChild->next)
		{
			Echo::Matrix4 roleMatrix;
			if (!GetObjMatrix(pChild, roleMatrix)) continue;
			cJSON *pRes = cJSON_GetObjectItem(pChild, "res");
			if (nullptr == pRes) continue;
			cJSON *pCurrentEquip = cJSON_GetObjectItem(pChild, "CurrentEquip");
			if (nullptr == pCurrentEquip) continue;

			Name resName(pRes->valuestring);
			Name curEquip(pCurrentEquip->valuestring);
			std::vector<SimpleObjInfo> roleInfo;
			auto it = role_info.find(resName);
			if (it != role_info.end()) {
				roleInfo = it->second;
			}
			else
			{
				SimpleObjInfo lodinfo;
				RoleObject* ptrRoleObj = new RoleObject(resName, resName, nullptr, true);
				ptrRoleObj->getSkeletonComponent()->changeEquip(curEquip, true);
				ptrRoleObj->getSceneNode()->needUpdateBounds();
				ptrRoleObj->getSceneNode()->_updateBounds();
				lodinfo.aabb = ptrRoleObj->getSceneNode()->getWorldAABB();
				if (lodinfo.aabb.isNull() || lodinfo.aabb.isInfinite())
				{
					delete ptrRoleObj;
					return;
				}

				int lod = 0;
				while (ptrRoleObj->getEntityComponent()->getTriangleCount(lod) != 0) {
					if (lod > 2) break;
					lodinfo.tri_count = ptrRoleObj->getEntityComponent()->getTriangleCount(lod);
					roleInfo.push_back(lodinfo);
					++lod;
				}
				if (0 == roleInfo.size()) {
					delete ptrRoleObj;
					return;
				}
				role_info[resName] = roleInfo;
				delete ptrRoleObj;

			}
			GetObjPosition(roleInfo, roleMatrix);
		}
		MeshMgr::instance()->destoryUnreferenced();
		MaterialManager::instance()->destoryUnreferenced();
		TextureManager::instance()->destoryUnreferenced();
	}


}; // SceneHeatMapPrivate

SceneHeatMap::SceneHeatMap()
	: dp(new SceneHeatMapPrivate)
{ }

SceneHeatMap::~SceneHeatMap()
{
	delete dp;	dp = nullptr;
}

void SceneHeatMap::setup()
{
	dp->range = Echo::WorldSystem::instance()->GetMainWorldManager()->getTerrainRange();
	float worldWidth = dp->range.right - dp->range.left;
	float worldHeight = dp->range.bottom - dp->range.top;

	if (worldHeight > dp->HMapMaxSize || worldWidth > dp->HMapMaxSize) {
		dp->pixelSize = (worldWidth > worldHeight ? worldWidth : worldHeight) / dp->HMapMaxSize*1.0f;
	}
	dp->hm_height = int(worldHeight / dp->pixelSize);
	dp->hm_width = int(worldWidth / dp->pixelSize);

	dp->range.left = dp->range.left / dp->pixelSize;
	dp->range.right = dp->range.right / dp->pixelSize;
	dp->range.top = dp->range.top / dp->pixelSize;
	dp->range.bottom = dp->range.bottom / dp->pixelSize;

	//dp->hmap = new float*[3];
	//for (int i = 0; i < 3; ++i) {
	//	dp->hmap[i] = new float[dp->hm_height*dp->hm_width];
	//	memset(dp->hmap[i], 0, (dp->hm_height*dp->hm_width) * sizeof(float));
	//}

}

namespace
{
	using namespace Echo;

	std::string _generatePageFilename(const Echo::String & rel_path, const Echo::Name & section, Echo::PageID pageID)
	{
		Echo::StringStream  str;
		str << rel_path << section.c_str();
		str << std::setw(8) << std::setfill('0') << std::hex << pageID << ".page";
		std::string strName;
		str >> strName;
		return  strName;
	}

}

void SceneHeatMap::setPath(const char * _datapath)
{
	dp->datapath = _datapath;
}

//void timerCallBack() {
//	cout << "test" << endl;
//}

void SceneHeatMap::generate(Echo::WorldManager *worldmgr)
{
	using namespace Echo;

	//dp->OutPutPNG(dp->datapath);
	//dp->OutPutRTreeData(dp->datapath);

	//TimerManager *timerMng = new TimerManager();
	//TimerObj* t = new TimerObj(*timerMng);
	//t->Strat(1000, &timerCallBack, t_type::CIRCLE);

	//EchoClientInfo::instance()->setup(Echo::Root::instance(), Echo::GetEchoEngine()->getCamera());
	//EchoClientInfo::instance()->sendClientInfo();
	//return;

	dp->hmap = new float*[3];
	for (int i = 0; i < 3; ++i) {
		dp->hmap[i] = new float[dp->hm_height*dp->hm_width];
		memset(dp->hmap[i], 0, (dp->hm_height*dp->hm_width) * sizeof(float));
	}

	Echo::SceneCommonInfoMgr * sceCommMgr = worldmgr->getSceneInfoMgr();
   const Echo::SceneCommonResourcePtr & ptrSceRes = sceCommMgr->getSceneCommonRes();
	const std::map<Echo::Name, Echo::SceneCommonResource::DistributionMap2D*> & disMaps = ptrSceRes->getWorldSectionMaps();
	Echo::String res_path = worldmgr->getPageManager()->getPageRelativePath();
	for (auto it : disMaps)
	{
		int x_start = it.second->originX,
			x_end = it.second->originX + (int)it.second->xLength;
		int y_start = it.second->originY,
			y_end = it.second->originY + (int)it.second->yLength;
		for (auto y = y_start; y < y_end; ++y)
		{
			for (auto x = x_start; x < x_end; ++x)
			{
				if (!it.second->exists(x, y))
					continue;

				Echo::PageID iD = Echo::PageStrategyData::calculatePageID(x, y);

				std::string pageResName = _generatePageFilename(res_path, it.first, iD);
				DataStreamPtr ptrStream(Root::instance()->GetFileDataStream(pageResName.c_str()));
				if (ptrStream.isNull()) continue;

				std::unique_ptr<char[]> contents(new char[ptrStream->size() + 1]);
				ptrStream->readData(contents.get(), ptrStream->size());
				contents[ptrStream->size()] = '\0';

				cJSON *pRoot = cJSON_Parse(contents.get());
				if (nullptr == pRoot) continue;

				for (auto it : dp->proc_func)
				{
					const cJSON *pNode = cJSON_GetObjectItem(pRoot, it.first.c_str());
					if (pNode != nullptr && pNode->type == cJSON_Array)
						(dp->*it.second)(pNode);
				}

				cJSON_Delete(pRoot);
			}
		}

		ResourceManagerFactory::instance()->destoryUnreferencedResource();
	}


	//dp->m_pCameraObj = new EchoCameraObj();
	//dp->m_pCameraObj->SetWorldManager(worldmgr,Echo::GetEchoEngine()->getCamera());


	//dp->OutPutRTreeData(dp->datapath);
	dp->OutPutPNG(dp->datapath);
}

void SceneHeatMap::parsecommand(const char * command)
{
	using namespace Echo;
	std::string cmd{ command };
	std::unordered_set<std::string> type;
	std::string tmp;
	bool lodflag = false;
	bool allflag = false;
	for (int i = 0; i < cmd.length(); ++i) {
		char ch = cmd[i];
		if (ch != ' ') tmp += ch;
		if (ch == ' ' || cmd[i + 1] == '\0') {
			transform(tmp.begin(), tmp.end(), tmp.begin(), ::toupper);
			if (tmp[0] == 'L'&& '0' <= tmp[1] && tmp[1] <= '2') {
				for (int i = 1; i < tmp.size(); ++i) {
					switch (tmp[i])
					{
					case '0':
						dp->Lod[0] = true; lodflag = true;
						break;
					case '1':
						dp->Lod[1] = true; lodflag = true;
						break;
					case '2':
						dp->Lod[2] = true; lodflag = true;
						break;
					default:
						break;
					}
				}
				tmp.clear();
				continue;
			}
			if (tmp[0] == 'M' && tmp[1] == 'A' && tmp[2] == 'X')
			{
				std::string maxStr = tmp.substr(3, tmp.size() - 3);
				StringStream ss;
				ss << maxStr;
				ss >> dp->maxTri;
				ss.clear();
				tmp.clear();
				continue;
			}
			if (tmp[0] == 'M' && tmp[1] == 'I' && tmp[2] == 'N')
			{
				std::string minStr = tmp.substr(3, tmp.size() - 3);
				StringStream ss;
				ss << minStr;
				ss >> dp->minTri;
				ss.clear();
				tmp.clear();
				continue;
			}
			if ("ALL" == tmp) {
				type.clear();
				type.insert("ALL");
				allflag = true;
			}
			if(allflag == false) type.insert(tmp);
			tmp.clear();
		}

	} // end for
	if (lodflag == false) dp->Lod[0] = true;
	if (type.empty())
		type.insert("ALL");
	for (auto it = type.begin(); it != type.end(); ++it) {
		if (*it == "ALL") {
			for (auto iter_all = dp->all_func.begin(); iter_all != dp->all_func.end(); ++iter_all)
				dp->proc_func[iter_all->first] = iter_all->second.func;
			break;
		}
		else {
			std::string tmp = *it;
			size_t n = tmp.find('-');
			if (n == -1)
				transform(tmp.begin() + 1, tmp.end(), tmp.begin() + 1, ::tolower);
			else {
				transform(tmp.begin() + 1, tmp.begin() + 1 + n, tmp.begin() + 1, ::tolower);
				transform(tmp.begin() + n + 2, tmp.end(), tmp.begin() + n + 2, ::tolower);
				tmp[n] = ' ';

			}
			for (auto iter_all = dp->all_func.begin(); iter_all != dp->all_func.end(); ++iter_all) {
				auto iter_str = iter_all->second.str.find(tmp);
				if (iter_str == iter_all->second.str.end()) continue;
				if (dp->picName != "") dp->picName += '_';
				dp->picName += tmp;
				dp->proc_func[iter_all->first] = iter_all->second.func;
				break;

			}
		} // end else
	}// end for
}



