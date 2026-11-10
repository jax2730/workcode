#include "EchoComponentStableHeaders.h"
#include "EchoCSGComponent.h"
#include "cJSON.h"
#include <stdio.h>
#include "EchoTimer.h"
#include "EchoMaterialManager.h"

namespace Echo
{
#define _COMPDATA(_pData) static_cast<CSGComponent*>(_pData)

	CSGComponentInfo::CSGComponentInfo(const Name& comType, ActorCOMMoveableType MoveType) :ActorComponentInfo(comType, MoveType) {}
	CSGComponentInfo::~CSGComponentInfo() {}

	EDITOR_REFLECT_BEGIN(CSGComponentInfo)
	EDITOR_REFLECT_END()

	DECLAREACTORCOMTYPE_CPP(CSGComponent)

	_DEFINEINFO_VIRFUN_BASE(CSGComponentInfo)

	CSGComponent::CSGComponent(ActorPtr pActor, ActorCOMMoveableType type) : NodeActorComponent(pActor, type)
	{
	}

	CSGComponent::~CSGComponent() {}

	void CSGComponent::init(ActorComponentInfo* info)
	{
	}

	bool CSGComponent::generateMeshFromArea(vector<Vector3>::type& prsData, vector<EchoGeometry*>::type& geometrys,Echo::String& destMeshPath,const Echo::String& templateMatPath)
	{
		vector<CSGNode*>::type trees;
		vector<CSGBrush*>::type brushs;

		for (size_t i = 0; i < geometrys.size(); i++)
		{
			Vector3 pos = prsData[i * 3];
			Vector3 euler = prsData[i * 3 + 1];
			Vector3 scale = prsData[i * 3 + 2];
			EchoGeometry* geo = geometrys[i];
			switch (geo->mType)
			{
			case eBOX:
			{
				EchoBoxGeometry* boxGeo = dynamic_cast<EchoBoxGeometry*>(geo);
				CSGBrush* brush = CSGShape::cubeCSG(pos, euler, scale, boxGeo->halfExtents);
				brush->updateBoundingBox();
				brushs.push_back(brush);

				CSGNode* node = new  CSGNode(brush->mPolygons);
				trees.push_back(node);
				break;
			}
			case eSPHERE:
			{
				EchoSphereGeometry* sphereGeo = dynamic_cast<EchoSphereGeometry*>(geo);
				CSGBrush* brush = CSGShape::sphereCSG(pos, euler, scale, sphereGeo->radius, 12.0f, 12.0f);
				brush->updateBoundingBox();
				brushs.push_back(brush);

				CSGNode* node = new  CSGNode(brush->mPolygons);
				trees.push_back(node);
				break;
			}
			case eCYLINDER:
			{
				EchoCylinderGeometry* cylinderGeo = dynamic_cast<EchoCylinderGeometry*>(geo);
				CSGBrush* brush = CSGShape::cylinderCSG(pos, euler, scale, cylinderGeo->radius, cylinderGeo->halfHeight);
				brush->updateBoundingBox();
				brushs.push_back(brush);

				CSGNode* node = new  CSGNode(brush->mPolygons);
				trees.push_back(node);
				break;
			}
			default:
				break;
			}
		}

		for (size_t i = 0; i < trees.size(); i++)
		{
			CSGNode* node = trees[i];
			for (size_t j = 0; j < trees.size(); j++)
			{
				if (i == j)
					continue;
				//bsp tree clip 是一个缓慢的过程,尝试使用aabb测试加速。
				if (!brushs[i]->getBoundingBox().intersects(brushs[j]->getBoundingBox()))
					continue;

				node->clipTo(*trees[j]);
			}
		}

		vector<CSGPolygon>::type allPolys;
		for (size_t i = 0; i < trees.size(); i++)
		{
			trees[i]->allPolygons(allPolys);
		}

		if (allPolys.size() <= 0)
		{
			return false;
		}

		CSGBrush* merged = new CSGBrush(allPolys);

		merged->generateDrawInfo();

		bool result = merged->exportToMesh(destMeshPath, templateMatPath);

		for (size_t i = 0; i < trees.size(); i++)
		{
			trees[i]->deleteTree(trees[i]);
			trees[i] = nullptr;
		}

		delete merged;
		return result;
	}

	bool CSGComponent::generateMeshFromArea(vector<Vector3>::type& prsData, vector<EchoGeometry*>::type& geometrys,Echo::String& destMeshPath,const Echo::String& outerMatPath,const Echo::String& innerMatPath)
	{
		if (destMeshPath.empty() || outerMatPath.empty() || innerMatPath.empty())
		{
			return false;
		}

		vector<CSGNode*>::type trees;
		vector<CSGBrush*>::type brushs;

		for (size_t i = 0; i < geometrys.size(); i++)
		{
			Vector3 pos = prsData[i * 3];
			Vector3 euler = prsData[i * 3 + 1];
			Vector3 scale = prsData[i * 3 + 2];
			EchoGeometry* geo = geometrys[i];
			switch (geo->mType)
			{
			case eBOX:
			{
				EchoBoxGeometry* boxGeo = dynamic_cast<EchoBoxGeometry*>(geo);
				CSGBrush* brush = CSGShape::cubeCSG(pos, euler, scale, boxGeo->halfExtents);
				brush->updateBoundingBox();
				brushs.push_back(brush);

				CSGNode* node = new  CSGNode(brush->mPolygons);
				trees.push_back(node);
				break;
			}
			case eSPHERE:
			{
				EchoSphereGeometry* sphereGeo = dynamic_cast<EchoSphereGeometry*>(geo);
				CSGBrush* brush = CSGShape::sphereCSG(pos, euler, scale, sphereGeo->radius, 12.0f, 12.0f);
				brush->updateBoundingBox();
				brushs.push_back(brush);

				CSGNode* node = new  CSGNode(brush->mPolygons);
				trees.push_back(node);
				break;
			}
			case eCYLINDER:
			{
				EchoCylinderGeometry* cylinderGeo = dynamic_cast<EchoCylinderGeometry*>(geo);
				CSGBrush* brush = CSGShape::cylinderCSG(pos, euler, scale, cylinderGeo->radius, cylinderGeo->halfHeight);
				brush->updateBoundingBox();
				brushs.push_back(brush);

				CSGNode* node = new  CSGNode(brush->mPolygons);
				trees.push_back(node);
				break;
			}
			default:
				break;
			}
		}

		for (size_t i = 0; i < trees.size(); i++)
		{
			CSGNode* node = trees[i];
			for (size_t j = 0; j < trees.size(); j++)
			{
				if (i == j)
					continue;
				//bsp tree clip 是一个缓慢的过程,尝试使用aabb测试加速。
				if (!brushs[i]->getBoundingBox().intersects(brushs[j]->getBoundingBox()))
					continue;

				node->clipTo(*trees[j]);
			}
		}

		vector<CSGPolygon>::type allPolys;
		for (size_t i = 0; i < trees.size(); i++)
		{
			trees[i]->allPolygons(allPolys);
		}

		if (allPolys.size() <= 0)
		{
			return false;
		}

		CSGBrush* merged = new CSGBrush(allPolys);

		merged->generateDrawInfo();

		bool result = merged->exportToMesh(destMeshPath, outerMatPath, innerMatPath);

		for (size_t i = 0; i < trees.size(); i++)
		{
			trees[i]->deleteTree(trees[i]);
			trees[i] = nullptr;
		}

		delete merged;
		return result;
	}

	//----------------Test Unit------------------------------
	void CSGComponent::mergeBrush()
	{
		FILE* file = fopen("E:/TestArea/shuiti_area_test_1.area", "r");
		if (!file)
		{
			return;
		}

		fseek(file, 0, SEEK_END);
		long fileSize = ftell(file);
		fseek(file, 0, SEEK_SET);

		char* buffer = (char*)malloc(fileSize + 1);
		if (!buffer)
		{
			fclose(file);
			return;
		}

		fread(buffer, 1, fileSize, file);
		fclose(file);
		buffer[fileSize] = '\0';
		cJSON* root = cJSON_Parse(buffer);
		if (root == nullptr)
		{
			std::cout << "json parse error!" << std::endl;
		}

		Timer timer;

		vector<Vector3>::type prsv;
		vector<EchoGeometry*>::type geoms;

		vector<CSGNode*>::type trees;


		cJSON* meshPathNode = cJSON_GetObjectItem(root, "meshPath");
		Echo::String meshPath = meshPathNode->valuestring;

		cJSON* outerMatNode = cJSON_GetObjectItem(root, "outerMatPath");
		Echo::String outerMatPath = outerMatNode->valuestring;
		cJSON* innerMatNode = cJSON_GetObjectItem(root, "innerMatPath");
		Echo::String innerMatPath = innerMatNode->valuestring;

		int i;
		cJSON* areas = cJSON_GetObjectItem(root, "areas");
		for (i = 0; i < cJSON_GetArraySize(areas); i++)
		{
			cJSON* subitem = cJSON_GetArrayItem(areas, i);
			cJSON* area_position = cJSON_GetObjectItem(subitem, "position");
			Vector3 localPos;
			localPos.x = static_cast<float>(cJSON_GetArrayItem(area_position, 0)->valuedouble);
			localPos.y = static_cast<float>(cJSON_GetArrayItem(area_position, 1)->valuedouble);
			localPos.z = static_cast<float>(cJSON_GetArrayItem(area_position, 2)->valuedouble);

			cJSON* eulerAngles = cJSON_GetObjectItem(subitem, "eulerAngles");
			Vector3 euler;
			euler.x = static_cast<float>(cJSON_GetArrayItem(eulerAngles, 0)->valuedouble);
			euler.y = static_cast<float>(cJSON_GetArrayItem(eulerAngles, 1)->valuedouble);
			euler.z = static_cast<float>(cJSON_GetArrayItem(eulerAngles, 2)->valuedouble);

			cJSON* scale = cJSON_GetObjectItem(subitem, "scale");
			Vector3 localScale;
			localScale.x = static_cast<float>(cJSON_GetArrayItem(scale, 0)->valuedouble);
			localScale.y = static_cast<float>(cJSON_GetArrayItem(scale, 1)->valuedouble);
			localScale.z = static_cast<float>(cJSON_GetArrayItem(scale, 2)->valuedouble);

			cJSON* geom = cJSON_GetObjectItem(subitem, "geom");
			cJSON* gType = cJSON_GetObjectItem(geom, "type");
			int type = gType->valueint;

			switch (type)
			{
			case 0:
			{
				Vector3 size;
				size.x = static_cast<float>(cJSON_GetObjectItem(geom, "long")->valuedouble);
				size.y = static_cast<float>(cJSON_GetObjectItem(geom, "height")->valuedouble);
				size.z = static_cast<float>(cJSON_GetObjectItem(geom, "wide")->valuedouble);

				EchoBoxGeometry* ebg = new EchoBoxGeometry(size);

				prsv.push_back(localPos);
				prsv.push_back(euler);
				prsv.push_back(localScale);

				geoms.push_back(ebg);

				break;
			}
			case 1:
			{
				float radius = static_cast<float>(cJSON_GetObjectItem(geom, "radius")->valuedouble);

				EchoSphereGeometry* ebg = new EchoSphereGeometry(radius);

				prsv.push_back(localPos);
				prsv.push_back(euler);
				prsv.push_back(localScale);

				geoms.push_back(ebg);

				break;
			}
			case 3:
			{
				float radius = static_cast<float>(cJSON_GetObjectItem(geom, "radius")->valuedouble);
				float height = static_cast<float>(cJSON_GetObjectItem(geom, "height")->valuedouble);

				EchoCylinderGeometry* ebg = new EchoCylinderGeometry(radius, height);

				prsv.push_back(localPos);
				prsv.push_back(euler);
				prsv.push_back(localScale);

				geoms.push_back(ebg);

				break;
			}
			default:
				break;
			}

		}

		cJSON_Delete(root);

		generateMeshFromArea(prsv, geoms, meshPath, outerMatPath, innerMatPath);
	}
	//----------------Test Unit------------------------------
}
