#include "EchoComponentStableHeaders.h"
#include "EchoRoadManager.h"
#include "EchoPavementComponent.h"
#include "EchoPiecewiseBezierComponent.h"
#include "EchoCrossRoadComponent.h"
#include "EchoMeshSerializer.h"
#include "EchoShaderContentManager.h"
#include "EchoSphericalTerrainManager.h"
#include "EchoBiomeVegetation.h"
#include "EchoTorusTerrainNode.h"
#include "EchoMaterialManager.h"
#include "EchoMaterialV2Manager.h"
#include "EchoPlanetRoad.h"
#include "EchoWorldSystem.h"
#include "EchoWorldManager.h"

#define Vec3Compare(in, out, compare) \
{\
	if (in.x compare out.x) \
		{out.x = in.x;}\
	if (in.y compare out.y)\
		{out.y = in.y;}\
	if (in.z compare out.z)\
		{out.z = in.z;}\
}

namespace Echo
{
#ifdef ECHO_EDITOR
	void AllComponentData::UpdateBezierComponent()
	{
		for (auto& bezier : m_vecBeziers)
			bezier->frameWork();
	}

	void AllComponentData::UpdateCrossRoadComponent()
	{
		for (auto& crossrad : m_vecCrossRs)
			crossrad->frameWork();
	}
#endif // ECHO_EDITOR


	//=============================================================================================================

	void Vec3GEVec(const Vector3& i_vec, Vector3& o_vec)
	{
		if (i_vec == o_vec) return ;
		if (i_vec.x >= o_vec.x)
			o_vec.x  = i_vec.x;
		if (i_vec.y >= o_vec.y)
			o_vec.y  = i_vec.y;
		if (i_vec.z >= o_vec.z)
			o_vec.z  = i_vec.z;
	}

	//============================================================================================================================

	class PlanetTerrainTool : public PavementComponent::PavementFitPlanetTerrain
	{
	public:
		PlanetTerrainTool() :PavementComponent::PavementFitPlanetTerrain(nullptr) {};
		virtual~PlanetTerrainTool() {};

		virtual SphericalTerrain* getTerrainPtr() { return planet; };
	public:
		SphericalTerrain* planet = nullptr;
	};

	Echo::ProtocolResult PlanetRoadAddLoadingCount(void* data, Echo::ProtocolHandle handle)
	{
		Name worldName = *((Name*)data);
		if (!worldName.empty())
		{
			WorldManager* world = WorldSystem::instance()->GetWorldManager(worldName);
			if (world)
				world->addLoadObject(handle);
		}
		return Echo::PR_OK;
	}
	Echo::ProtocolResult PlanetRoadRemoveLoadingCount(void* data, Echo::ProtocolHandle handle)
	{
		Name worldName = *((Name*)data);
		if (!worldName.empty())
		{
			WorldManager* world = WorldSystem::instance()->GetWorldManager(worldName);
			if (world)
				world->removeLoadObject(handle, "PlanetRoad");
		}
		return Echo::PR_OK;
	}
	//============================================================================================================================

	RoadManager::RoadManager()
	{
		//m_WorkQueue = new RoadWorkQueue();
#ifdef ECHO_EDITOR
		m_pComs = new AllComponentData();
#else
		ListenerPick |= PavementPhyCreate_Listener;
#endif // ECHO_EDITOR

		Root::instance()->addFrameListener(this);

		for (int lod = 0; lod < LOD_LEVEL_NUM; ++lod)
		{
			m_pavementDrawCountLOD[lod] = 0;
			m_pavementTriangleCountLOD[lod] = 0;
		}
		{
			ProtocolComponentSystem::instance()->registerProtocolExtFunc(Echo::Name("CheckPlanetRoadMask"), &RoadManager::ProCom_CheckPlanetPosition_FitGorundRoad);
			ProtocolComponentSystem::instance()->registerProtocolExtFunc(Echo::Name("addBiomeVegetationData"), &RoadManager::addBiomeVegetationData);
			ProtocolComponentSystem::instance()->registerProtocolExtFunc(Echo::Name("delBiomeVegetationData"), &RoadManager::delBiomeVegetationData);
			ProtocolComponentSystem::instance()->registerProtocolExtFunc(Echo::Name("PlanetRoadAddLoadingCount"), &PlanetRoadAddLoadingCount);
			ProtocolComponentSystem::instance()->registerProtocolExtFunc(Echo::Name("PlanetRoadRemoveLoadingCount"), &PlanetRoadRemoveLoadingCount);
		}

		planetTerrainTool = new PlanetTerrainTool();
	}

	RoadManager::~RoadManager()
	{
		//if (m_WorkQueue)
		//	delete m_WorkQueue;

#ifdef ECHO_EDITOR
		if (m_pComs)
			delete m_pComs;
#endif // ECHO_EDITOR

		m_vecPavements.clear();
		Root::instance()->removeFrameListener(this);

		m_mapUtilKeyData.clear();
		m_mapPathMesh.clear();

		for (auto& [planet, data] : mPlanetFitGroundRoadMask)
		{
			if (!data->mTickIDs.empty())
			{
				for (auto& tickID : data->mTickIDs)
					RoadHelper::instance()->abortRequest(tickID);
			}
			SAFE_DELETE(data);
		}
		mPlanetFitGroundRoadMask.clear();
		mScenePlanetDataCache.clear();

		SAFE_DELETE(planetTerrainTool);

		{
			ProtocolComponentSystem::instance()->unregisterProtocolExtFunc(Echo::Name("CheckPlanetRoadMask"));
			ProtocolComponentSystem::instance()->unregisterProtocolExtFunc(Echo::Name("addBiomeVegetationData"));
			ProtocolComponentSystem::instance()->unregisterProtocolExtFunc(Echo::Name("delBiomeVegetationData"));
			//ProtocolComponentSystem::instance()->unregisterProtocolExtFunc(Echo::Name("PlanetRoadAddLoadingCount"));
			//ProtocolComponentSystem::instance()->unregisterProtocolExtFunc(Echo::Name("PlanetRoadRemoveLoadingCount"));
		}
	}

	bool RoadManager::frameEnded(const FrameEvent& evt)
	{
		(void)evt;
		m_pavementDrawCount = 0;
		m_pavementTriangleCount = 0;
		for (int lod = 0; lod < LOD_LEVEL_NUM; ++lod)
		{
			m_pavementDrawCountLOD[lod] = 0;
			m_pavementTriangleCountLOD[lod] = 0;
		}
#ifdef ECHO_EDITOR
#else
		std::vector<Name> needDelete;
		for (auto iter = m_mapWeight.begin(); iter != m_mapWeight.end(); ++iter)
		{
			if (iter->second < 60)
				iter->second++;
			else
				needDelete.push_back(iter->first);
		}

		for (auto& v : needDelete)
		{
			m_mapPathMesh.erase(v);
			m_mapWeight.erase(v);
		}
#endif
		//if (ListenerPick & PavementPhyCreate_Listener)
		{
			for (auto& pavement : m_vecPavements)
				pavement->frameWork();
		}
		if (usePlanetRoadMask)
			CheckScenePlanetRoadMaskUpdate();
		return true;
	}

	bool RoadManager::IsAdsorptionOperation()
	{
		return (ListenerPick & BezierLink_Listener) || (ListenerPick & CrossRoadLink_Listener);
	}

	void RoadManager::AdsorbedRoad()
	{
#ifdef ECHO_EDITOR
		if (m_pComs && m_BeginListener)
		{
			if (ListenerPick & BezierLink_Listener)
			{
				m_pComs->UpdateBezierComponent();
			}

			if (ListenerPick & CrossRoadLink_Listener)
			{
				m_pComs->UpdateCrossRoadComponent();
			}
		}
#endif // ECHO_EDITOR
	}

	PiecewiseBezierComponent* RoadManager::FindBezierInVec(String pEditorName, String pComName)
	{
		PiecewiseBezierComponent* pBezier = nullptr;
#ifdef ECHO_EDITOR
		if (m_pComs)
		{
			for (auto& mBezierCom : m_pComs->m_vecBeziers)
			{
				if (mBezierCom->getActor()->m_QtEditorName == pEditorName &&
					mBezierCom->getComName() == pComName)
				{
					pBezier = mBezierCom;
					break;
				}
			}
		}
#endif // ECHO_EDITOR
		return pBezier;
	}

	bool RoadManager::SetBezierVec(PiecewiseBezierComponent* p_bezier)
	{
#ifdef ECHO_EDITOR
		if (!m_pComs) return false;

		auto iter = std::find(m_pComs->m_vecBeziers.begin(), m_pComs->m_vecBeziers.end(), p_bezier);
		if (iter != m_pComs->m_vecBeziers.end())
		{
			m_pComs->m_vecBeziers.erase(iter);
			return false;
		}
		else
		{
			m_pComs->m_vecBeziers.push_back(p_bezier);
			return true;
		}
#endif // ECHO_EDITOR
		return false;
	}

	bool RoadManager::SetPavementVec(PavementComponent* p_pavement)
	{
		auto iter = std::find(m_vecPavements.begin(), m_vecPavements.end(), p_pavement);
		if (iter != m_vecPavements.end())
		{
			m_vecPavements.erase(iter);
			if (RoadManager::instance()->usePlanetRoadMask && (p_pavement->getFitGroundModel() || p_pavement->getVegetationCollection()))
				removePlanetFitGroundPavementAreaCache(p_pavement->weak_from_this());
			return false;
		}
		else
		{
			m_vecPavements.push_back(p_pavement);
			return true;
		}
	}

	bool RoadManager::SetCrossRoadVec(CrossRoadComponent* p_crossroad)
	{
#ifdef ECHO_EDITOR
		if (!m_pComs) return false;

		auto iter = std::find(m_pComs->m_vecCrossRs.begin(), m_pComs->m_vecCrossRs.end(), p_crossroad);
		if (iter != m_pComs->m_vecCrossRs.end())
		{
			m_pComs->m_vecCrossRs.erase(iter);
			return false;
		}
		else
		{
			m_pComs->m_vecCrossRs.push_back(p_crossroad);
			return true;
		}
#endif
		return false;
	}

	void RoadManager::ClearUtilKeyDatas(const String& n)
	{
		if (m_mapUtilKeyData.find(n) != m_mapUtilKeyData.end())
		{
			m_mapUtilKeyData[n]->clear();
		}
	}

	int RoadManager::CalculateCrossRoadPointAndVec(const MeshPtr& p_mesh,
		const std::vector<std::vector<Vector3>>& p_vertexData, std::vector<Vector3>& o_vec)
	{
		/*
		y  x		  Front
		| /				|
		|/___ z  Left --+-- Right
		*				|
					   Back
		*/

		int result = 0;

		AxisAlignedBox entity_box = p_mesh->getAABB();
		float left_key = entity_box.vMin.z;
		float right_key = entity_box.vMax.z;
		float front_key = entity_box.vMax.x;
		float back_key = entity_box.vMin.x;

		Vector3 left_min, left_max;
		Vector3 right_min, right_max;
		Vector3 front_min, front_max;
		Vector3 back_min, back_max;

		Vector3 left, right, front, back;
		Vector3 left_vec, right_vec, front_vec, back_vec;

		left_min = right_min = front_min = back_min = entity_box.vMax;
		left_max = right_max = front_max = back_max = entity_box.vMin;

		const float hight = entity_box.vMin.y;

		for (auto& vecs : p_vertexData)
		{
			for (auto& data : vecs)
			{
				if (abs(data.x - front_key) < 1e-3)
				{
					Vec3Compare(data, front_min, <= );
					Vec3Compare(data, front_max, >= );
				}
				else if (abs(data.x - back_key) < 1e-3)
				{
					Vec3Compare(data, back_min, <= );
					Vec3Compare(data, back_max, >= );
				}

				if (abs(data.z - left_key) < 1e-3)
				{
					Vec3Compare(data, left_min, <= );
					Vec3Compare(data, left_max, >= );
				}
				else if (abs(data.z - right_key) < 1e-3)
				{
					Vec3Compare(data, right_min, <= );
					Vec3Compare(data, right_max, >= );
				}
			}
		}

		float lx = (entity_box.vMax.x - entity_box.vMin.x);

		if (abs((left_max.x - left_min.x) - lx) >= 1.f)
			result |= Left;

		if (abs((right_max.x - right_min.x) - lx) >= 1.f)
			result |= Right;

		float lz = (entity_box.vMax.z - entity_box.vMin.z);

		if (abs((front_max.z - front_min.z) - lz) >= 1.f)
			result |= Front;

		if (abs((back_max.z - back_min.z) - lz) >= 1.f)
			result |= Back;

		left = left_min + (left_max - left_min) / 2.f;
		left.z = left_key;
		left.y = hight;
		right = right_min + (right_max - right_min) / 2.f;
		right.z = right_key;
		right.y = hight;
		front = front_min + (front_max - front_min) / 2.f;
		front.x = front_key;
		front.y = hight;
		back = back_min + (back_max - back_min) / 2.f;
		back.x = back_key;
		back.y = hight;

		o_vec.push_back(left);
		o_vec.push_back(Vector3(0.0f, 0.f, -1.0f));
		o_vec.push_back(right);
		o_vec.push_back(Vector3(0.0f, 0.f, 1.0f));
		o_vec.push_back(front);
		o_vec.push_back(Vector3(1.0f, 0.f, 0.0f));
		o_vec.push_back(back);
		o_vec.push_back(Vector3(-1.0f, 0.f, 0.0f));
		o_vec.push_back(left_min);
		o_vec.push_back(left_max);
		o_vec.push_back(right_min);
		o_vec.push_back(right_max);
		o_vec.push_back(front_min);
		o_vec.push_back(front_max);
		o_vec.push_back(back_min);
		o_vec.push_back(back_max);

		return result;
	}

	bool RoadManager::GetPathMeshData(const Name& path, std::shared_ptr<PavementTool::PrivateMeshData>& data, bool needSubData)
	{
		auto iter = m_mapPathMesh.find(path);
		if (iter != m_mapPathMesh.end())
		{
			if (needSubData && iter->second->SubData.size() == 0) return false;

			data = iter->second;
			auto i = m_mapWeight.find(path);
			if (i != m_mapWeight.end())
				m_mapWeight.erase(i);
			return true;
		}
		else
			return false;
	}

	void RoadManager::SetPathMeshData(const Name& path, std::shared_ptr<PavementTool::PrivateMeshData>& data)
	{
		if (m_mapPathMesh.find(path) == m_mapPathMesh.end() && m_mapPathMesh.size() >= 10)
			m_mapPathMesh.erase(m_mapPathMesh.begin());
		m_mapPathMesh[path] = data;
	}

	void RoadManager::SetWeightZero(const Name& path)
	{
		m_mapWeight[path] = 0;
	}
	//String			mVSDXGenerateMask = "#define DIFFUSE_TEX\n";
	//String			mPSDXGenerateMask = "#define DIFFUSE_TEX\n";
	std::shared_ptr<PavementTool::PrivateMeshData> RoadManager::prepareMeshData(const Name& meshName)
	{
		std::shared_ptr<PavementTool::PrivateMeshData> it_data;
		if (GetPathMeshData(meshName, it_data, true))
		{
			return it_data;
		}
		else
		{
			it_data = std::shared_ptr<PavementTool::PrivateMeshData>(new PavementTool::PrivateMeshData());
			MeshPtr baseMesh;
			try
			{
				baseMesh = MeshMgr::instance()->prepare(meshName);
				//运行时已经load过的mesh，暂时屏蔽
				//if (Root::instance()->getRuntimeMode() == Root::eRM_Client && baseMesh->isLoaded())
				//	return nullptr;
			}
			catch (...)
			{
				LogManager::instance()->logMessage("RoadManager::prepareMeshData(): " + meshName.toString() + "not exist");
				return nullptr;
			};
			//计算创建物理所需顶点
			MeshPtr realmesh = baseMesh;
			{
				Name meshphyName = Mesh::getDstPhyMeshName(realmesh->getName());
				while (!realmesh->mRealMesh.isNull())
				{
					realmesh = realmesh->mRealMesh;
					meshphyName = Mesh::getDstPhyMeshName(realmesh->getName());
				}
				//Name meshphyName;
				//meshphyName = Mesh::getDstPhyMeshName(baseMesh->getName());
				//tempmesh = baseMesh->mRealMesh;
				//while (!tempmesh.isNull())
				//{
				//	meshphyName = Mesh::getDstPhyMeshName(tempmesh->getName());
				//	tempmesh = tempmesh->mRealMesh;
				//}
				PhysXTriangleMeshResourcePtr res;
				try
				{
					res = PhysXTriangleMeshResourceManager::instance()->load(meshphyName);
				}
				catch (...)
				{
					LogManager::instance()->logMessage("RoadManager::prepareMeshData(): " + meshphyName.toString() + "not exist");

				}

				if (!res.isNull() && res->ExistPhysXMesh())
				{
					res->m_mesh->getVertexsAndIndices(it_data->PhyData.vertexVec, it_data->PhyData.indexVec);
				}
			}
			auto getMeshData = [&](const Mesh& meshTemp, const Mesh& realTemp)
			{
				uint32 num = meshTemp.getNumSubMesh();
				std::vector<PavementTool::SubMeshData_> tempVec;
				tempVec.reserve(num);
				for (uint32 sub = 0; sub < num; ++sub)
				{
					Echo::SubMesh* submesh = meshTemp.getSubMesh(sub);

					MeshVertexIndexData data;
					if (realTemp.getSubMesh(sub))
						realTemp.getSubMesh(sub)->getVertexIndexData(data);
					else
						LogManager::instance()->logMessage("RoadManager::prepareMeshData(): get real submesh VBIB by index error, meshName: " + meshTemp.getName().toString());

					PavementTool::SubMeshData_ subdata;

					try
					{
						const MaterialWrapper& materialWrapper = submesh->getMaterialWrapper();

						if (materialWrapper.isV1())
						{
							Material* material = materialWrapper.getV1();
							
							MaterialPtr road_material = MaterialManager::instance()->createManual();
							subdata.road_material = road_material;
							road_material->clone(material, true, false);

							memcpy(&road_material->m_desc.RenderState, &material->m_desc.RenderState, sizeof(material->m_desc.RenderState));

							uint32 elementSize = 0;
							road_material->m_desc.IADesc.ElementArray[0].nStream = 0;
							road_material->m_desc.IADesc.ElementArray[0].eType = ECHO_FLOAT3;
							road_material->m_desc.IADesc.ElementArray[0].Semantics = ECHO_POSITION0;
							road_material->m_desc.IADesc.ElementArray[0].nOffset = elementSize;
							elementSize += 3 * sizeof(float);
							road_material->m_desc.IADesc.ElementArray[1].nStream = 0;
							road_material->m_desc.IADesc.ElementArray[1].eType = ECHO_FLOAT3;
							road_material->m_desc.IADesc.ElementArray[1].Semantics = ECHO_NORMAL0;
							road_material->m_desc.IADesc.ElementArray[1].nOffset = elementSize;
							elementSize += 3 * sizeof(float);
							road_material->m_desc.IADesc.ElementArray[3].nStream = 0;
							road_material->m_desc.IADesc.ElementArray[3].eType = ECHO_FLOAT4;
							road_material->m_desc.IADesc.ElementArray[3].Semantics = ECHO_TANGENT0;
							road_material->m_desc.IADesc.ElementArray[3].nOffset = elementSize;
							elementSize += 4 * sizeof(float);
							road_material->m_desc.IADesc.ElementArray[2].nStream = 0;
							road_material->m_desc.IADesc.ElementArray[2].eType = ECHO_FLOAT2;
							road_material->m_desc.IADesc.ElementArray[2].Semantics = ECHO_TEXCOORD0;
							road_material->m_desc.IADesc.ElementArray[2].nOffset = elementSize;
							elementSize += 2 * sizeof(float);

							road_material->m_desc.IADesc.nElementCount = 4;
							road_material->m_desc.IADesc.nStreamCount = 1;
							road_material->m_desc.IADesc.StreamArray[0].nStride = elementSize; //vertexSize

							std::string mVSDX, mPSDX;
							ShaderContentManager::instance()->GetOrLoadShaderContent(Name(material->mVSName), mVSDX);
							ShaderContentManager::instance()->GetOrLoadShaderContent(Name(material->mPSName), mPSDX);
							road_material->setShaderName(material->mVSName, material->mPSName);
							road_material->setShaderSrcCode(mVSDX, mPSDX);
							road_material->setMacroShaderSrcCode(/*mVSDXGenerateMask + */material->mMaterialMacro, /*mPSDXGenerateMask + */material->mMaterialMacro);

							road_material->setMaterialPassMask(material->getMaterialPassMask());
							//road_material->setMaterialPointShadowPassMask(material->getMaterialPointShadowPassMask());
							road_material->setMaterialRipplePassMask(material->getMaterialRipplePassMask());
							road_material->setMaterialSoftPassMask(material->getMaterialSoftPassMask());

							road_material->createPass(NormalPass);
							road_material->prepareRes();
						}
						else if (materialWrapper.isV2())
						{
							MaterialV2* material = materialWrapper.getV2();
							
							MaterialV2Ptr road_material = MaterialV2Manager::instance()->createManual();
							subdata.road_material = road_material;

							Mat::MaterialInfo materialInfo = material->getMaterialInfo();
							materialInfo.attributes.vertexCompression = false;

							road_material->createFromMaterialInfo(materialInfo, true);

							ECHO_IA_DESC iaDesc = road_material->getVertexInputAttributeDesc();

							uint32 elementSize = 0;
							iaDesc.ElementArray[0].nStream = 0;
							iaDesc.ElementArray[0].eType = ECHO_FLOAT3;
							iaDesc.ElementArray[0].Semantics = ECHO_POSITION0;
							iaDesc.ElementArray[0].nOffset = elementSize;
							elementSize += 3 * sizeof(float);
							iaDesc.ElementArray[1].nStream = 0;
							iaDesc.ElementArray[1].eType = ECHO_FLOAT3;
							iaDesc.ElementArray[1].Semantics = ECHO_NORMAL0;
							iaDesc.ElementArray[1].nOffset = elementSize;
							elementSize += 3 * sizeof(float);
							iaDesc.ElementArray[3].nStream = 0;
							iaDesc.ElementArray[3].eType = ECHO_FLOAT4;
							iaDesc.ElementArray[3].Semantics = ECHO_TANGENT0;
							iaDesc.ElementArray[3].nOffset = elementSize;
							elementSize += 4 * sizeof(float);
							iaDesc.ElementArray[2].nStream = 0;
							iaDesc.ElementArray[2].eType = ECHO_FLOAT2;
							iaDesc.ElementArray[2].Semantics = ECHO_TEXCOORD0;
							iaDesc.ElementArray[2].nOffset = elementSize;
							elementSize += 2 * sizeof(float);

							iaDesc.nElementCount = 4;
							iaDesc.nStreamCount = 1;
							iaDesc.StreamArray[0].nStride = elementSize; //vertexSize

							std::vector<ShaderMacrosItem> vsMacros = road_material->getCustomVSMacros();
							std::vector<ShaderMacrosItem> psMacros = road_material->getCustomPSMacros();

							//vsMacros.insert(vsMacros.begin(), ShaderMacrosItem::fromDefineString(mVSDXGenerateMask));
							//psMacros.insert(psMacros.begin(), ShaderMacrosItem::fromDefineString(mPSDXGenerateMask));

							road_material->setCustomVSMacros(vsMacros);
							road_material->setCustomPSMacros(psMacros);

							road_material->setVertexInputAttributeDesc(iaDesc);

							road_material->prepareRes();
						}
						else
						{
							MaterialPtr road_material = MaterialManager::instance()->createManual();
							subdata.road_material = road_material;
						}
					}
					catch (...)
					{
						subdata.road_material = nullptr;
						LogManager::instance()->logMessage("[RoadManager::prepareMeshData] get mesh materialV2 failed!  meshName:"
							+ meshName.toString() + __FUNCTION__);
					}
					for (size_t j = 0; j < data.vertices.size(); ++j)
					{
						PavementTool::Vertex_ value(data.vertices[j], data.normals[j]);

						subdata.road_vertexes.push_back(value);
						subdata.road_uv.push_back(data.uvs[j]);
						subdata.road_tangent.push_back(data.tangents[j]);
					}

					for (size_t j = 0; j < data.indices.size(); ++j)
						subdata.road_indices.push_back(data.indices[j]);

					tempVec.push_back(subdata);

					it_data->vertexNumMax = it_data->vertexNumMax >= data.vertices.size() ? it_data->vertexNumMax : data.vertices.size();
				}
				it_data->SubData.push_back(tempVec);
			};

			//虚拟mesh的顶点数据是在真实mesh中获取的，所以要用真实mesh来获取顶点数据
			if (Root::instance()->getRuntimeMode() == Root::eRM_Client && realmesh->isLoaded())
			{
				//运行时mesh在load阶段会释放掉缓存的顶点数据，需要重新序列化一遍

				auto rePrepareMesh = [&](const Name& meshName, const Mesh& _baseMesh) {

					Name curMeshName = Mesh::getCurPlatformMeshName(meshName);
					DataStreamPtr stream(Root::instance()->GetMemoryDataStream(curMeshName.c_str()));
					//DataStreamPtr stream(Root::instance()->GetMemoryDataStream(meshName.c_str()));

					if (stream.isNull() || stream->size() == 0)
					{
						LogManager::instance()->logMessage("[RoadManager::prepareMeshData] get mesh resource failed!  meshName:"
							+ meshName.toString() + __FUNCTION__);
						return false;
					}
					LogManager::instance()->logMessage("[RoadManager::prepareMeshData] get mesh resource  meshName:"
						+ meshName.toString() + __FUNCTION__);
					try
					{
						//这里tempMesh一定是_baseMesh或者_baseMesh的真实mesh，tempMesh用于下一步获取顶点数据
						Mesh tempMesh(meshName);
						MeshSerializer serializer;
						serializer.importMesh(stream, &tempMesh);
						if (!tempMesh.m_bMergeMesh)
							tempMesh.loadInfoFromJson();
						tempMesh.dealDataForEditor();
						getMeshData(_baseMesh, tempMesh);
						return true;
					}
					catch (...)
					{
						LogManager::instance()->logMessage("[RoadManager::prepareMeshData] Mesh serializer import mesh resource failed!  meshName:"
							+ meshName.toString() + __FUNCTION__);
						return false;
					}
				};

				rePrepareMesh(realmesh->getName(), *baseMesh.getPointer());

				for (auto& lodmesh : baseMesh->m_vtLodMesh)
				{
					MeshPtr realLodmesh = lodmesh;
					{
						while (!realLodmesh->mRealMesh.isNull())
						{
							realLodmesh = realLodmesh->mRealMesh;
						}
					}
					rePrepareMesh(realLodmesh->getName(), *lodmesh.getPointer());
				}

			}
			else
			{
				getMeshData(*baseMesh.getPointer(), *baseMesh.getPointer());
				for (auto& lodmesh : baseMesh->m_vtLodMesh)
				{
					getMeshData(*lodmesh.getPointer(), *lodmesh.getPointer());
				}
			}
			if (it_data->vertexNumMax == 0)
			{
				return nullptr;
			}

			

			it_data->mesh = baseMesh;

			SetPathMeshData(meshName, it_data);

		}
		return it_data;
	}

	//--------------------------------------------------------------------------------------------------------------------------------------------------

	Echo::ProtocolResult RoadManager::addBiomeVegetationData(void* p, Echo::ProtocolHandle handle)
	{
		_RegisterData* data = (_RegisterData*)p;
		RoadManager::instance()->registerScenePlanet(data->scenePtr, data->sphPtr);
		return Echo::PR_OK;
	}

	Echo::ProtocolResult RoadManager::delBiomeVegetationData(void* p, Echo::ProtocolHandle handle)
	{
		_RegisterData* data = (_RegisterData*)p;
		RoadManager::instance()->unregisterScenePlanet(data->scenePtr, data->sphPtr);
		return Echo::PR_OK;
	}

	void RoadManager::registerScenePlanet(SceneManager* scene, SphericalTerrain* planet)
	{
		if (!planet || !scene)
			return;
		if (mPlanetFitGroundRoadMask.find(planet) != mPlanetFitGroundRoadMask.end())
		{
			LogManager::instance()->logMessage("RoadManager::registerScenePlanet() planet is registered");
			return;
		}

		tthread::lock_guard<tthread::recursive_mutex> lock(mLock);
		mPlanetFitGroundRoadMask[planet] = new RoadManager::PlanetRoadFitGroundAreaMap();
		mScenePlanetDataCache[scene->getName()].mPlanetCache.insert(planet);
	}

	void RoadManager::unregisterScenePlanet(SceneManager* scene, SphericalTerrain* planet)
	{
		if (!planet || !scene)
			return;
		if (mPlanetFitGroundRoadMask.find(planet) == mPlanetFitGroundRoadMask.end())
		{
			LogManager::instance()->logMessage("RoadManager::unregisterScenePlanet() can not find planet to unregistered");
			return;
		}
		//if (mPlanetFitGroundRoadMask[planet]->mTickID != 0)
		//	RoadHelper::instance()->abortRequest(mPlanetFitGroundRoadMask[planet]->mTickID);
		if (!mPlanetFitGroundRoadMask[planet]->mTickIDs.empty())
		{
			for (auto& tickID : mPlanetFitGroundRoadMask[planet]->mTickIDs)
				RoadHelper::instance()->abortRequest(tickID);
		}

		tthread::lock_guard<tthread::recursive_mutex> lock(mLock);
		SAFE_DELETE(mPlanetFitGroundRoadMask[planet]);
		mPlanetFitGroundRoadMask.erase(planet);
		const Name& sceneName = scene->getName();
		mScenePlanetDataCache[sceneName].mPlanetCache.erase(planet);
	}

	void RoadManager::CheckScenePlanetRoadMaskUpdate()
	{
		return;
		for (auto& [sceneName, sceneData] : mScenePlanetDataCache)
		{
			SceneManager* scene = Root::instance()->getSceneManager(sceneName);
			if (!scene || sceneData.mPlanetCache.empty())
				continue;
			Camera* camera = scene->getMainCamera();
			if (camera->getCameraUsage() != CameraUsage::SceneCamera)
				continue;
			Vector3 camPos = camera->getDerivedPosition();
			AxisAlignedBox camBox(camPos - 256.0f, camPos + 256.0f);
			for (auto& planet : sceneData.mPlanetCache)
			{
				UpdatScenePlanetMaskData(planet, camBox);
			}
		}
	}

	void RoadManager::addPlanetFitGroundPavementAreaCache(const ActorComponentPtr& com, SphericalTerrain* planet, const AxisAlignedBox& worldBox)
	{
		return;
		if (!RoadManager::instance()->usePlanetRoadMask)
			return;
		if (com.expired() || !planet || !planet->isCreateFinish() || worldBox.isNull())
		{
			LogManager::instance()->logMessage("RoadManager::addPlanetFitGroundPavementAreaCache(): com/planet or worldBox is null/empty");
			return;
		}
		SceneManager* scene = com->getSceneManager();
		if (!scene)
			return;
		const Name& sceneName = scene->getName();
		if (mScenePlanetDataCache[sceneName].mPlanetCache.find(planet) == mScenePlanetDataCache[sceneName].mPlanetCache.end())
		{
			LogManager::instance()->logMessage("RoadManager::addPlanetFitGroundPavementAreaCache() planet is unregistered");
			return;
		}
		if(com->getType() == COMPONENTTYPE(PavementComponent))
		{
			PavementComponent* pavement = (PavementComponent*)com.get();
			if (!pavement->isPavementsCreateFinish())
				return;
		}
		//if (mPlanetFitGroundRoadMask.find(planet) != mPlanetFitGroundRoadMask.end())
		{
			PlanetRoadFitGroundAreaMap* data = mPlanetFitGroundRoadMask[planet];
			std::vector<int> indexs = planet->getLeavesInWs(worldBox);
			if(!indexs.empty())
			{
				data->needupdate = true;
				std::map<int, RoadManager::FitGroundMask>& maskData = data->maskData[data->writeIndex];
				for (auto& index : indexs)
				{
					data->roadCache[index].insert(com);
					//if (maskData.find(index) != maskData.end())
						maskData[index].needUpdate = true;
				}
			}
		}
		return;
	}

	void RoadManager::removePlanetFitGroundPavementAreaCache(const ActorComponentPtr& com)
	{
		return;
		if (com.expired())
			return;
		SceneManager* scene = com->getSceneManager();
		if (!scene)
			return;
		for (auto& [planet, data] : mPlanetFitGroundRoadMask)
		{
			bool isFind = false;
			for (auto& [index, pavements] : data->roadCache)
			{
				if (pavements.find(com) != pavements.end())
				{
					isFind = true;
					pavements.erase(com);
					std::map<int, RoadManager::FitGroundMask>& mask = data->maskData[data->writeIndex];
					if (mask.find(index) != mask.end())
						mask[index].needUpdate = true;
				}
			}
			if (isFind)
			{
				data->needupdate = true;
				break;
			}
		}
	}

	void RoadManager::UpdatScenePlanetMaskData(SphericalTerrain* planet, const AxisAlignedBox& worldBox)
	{
		if (!planet || !planet->isCreateFinish())
			return;
		RoadManager::PlanetRoadFitGroundAreaMap* data = mPlanetFitGroundRoadMask[planet];
		bool updateByPos = true;
		{
			Vector3 pos = worldBox.getCenter();
			if (planet->getGravityDirection(pos).length() > (planet->getMaxRealRadius() + 800.0f))
			{
				updateByPos = false;
				//return;
			}
			Vector3& posCache = data->posCache;
			if (posCache == Vector3::ZERO)
			{
				posCache = pos;
			}
			else
			{
				const float t = 0.3f;
				Vector2 xy, lastxy;
				if (planet->isSphere())
				{
					int face, lastface;
					SphericalTerrainQuadTreeNode::MapToCube(pos, face, xy);
					SphericalTerrainQuadTreeNode::MapToCube(posCache, lastface, lastxy);
					float updateSize = (float)SphericalTerrainQuadTreeNode::getLengthOnCube(planet->m_maxDepth) * t;
					if (face == lastface && (xy - lastxy).Length() < updateSize)
						updateByPos = false;
					//return;
				}
				else if (planet->isTorus())
				{
					xy = TorusTerrainNode::MapToPlane(pos);
					lastxy = TorusTerrainNode::MapToPlane(posCache);
					Vector2 updateSize = 1.0f / Vector2(static_cast<float>(planet->m_numSlice[0]), static_cast<float>(planet->m_numSlice[1]));
					updateSize *= TorusTerrainNode::getLengthOnSlice(planet->m_maxDepth);
					updateSize *= t;
					if ((xy - lastxy).Length() < updateSize.Length())
						updateByPos = false;
					//return;
				}
				if (updateByPos)
					posCache = pos;
			}
		}
		std::set<ActorComponentPtr> pavementSet;
		std::map<int, RoadManager::FitGroundMask>& maskData = data->maskData[data->writeIndex];

		if (!updateByPos || data->needupdate)
		{
			std::vector<int> delcache;
			for (auto& [index, maskCache] : maskData)
			{
				if (maskCache.needUpdate)
				{
					maskCache.needUpdate = false;
					for (auto& com : data->roadCache[index])
					{
						pavementSet.insert(com);
					}
				}
				else if (!maskCache.needCheck)
				{
					delcache.push_back(index);
				}
			}
			if (!delcache.empty())
			{
				for (auto& i : delcache)
					maskData.erase(i);
			}
			if (!pavementSet.empty())
			{
				updatePlanetPavementMaskCache(planet, pavementSet);
			}
			data->needupdate = false;
			return;
		}
		std::vector<int> indexs = planet->getLeavesInWs(worldBox);

		//if (indexs.empty() && !maskData.empty())
		//{
		//	//tthread::lock_guard<tthread::recursive_mutex> lock(mLock);
		//	maskData.clear();
		//}
		//else
		if (!indexs.empty())
		{
			//std::vector<int> delIndexs;
			//for (auto& [index, maskCache] : maskData)
			//{
			//	if (std::find(indexs.begin(), indexs.end(), index) == indexs.end() || !maskCache.needCheck)
			//		delIndexs.push_back(index);
			//	//maskCache.needUpdate = false;
			//}
			//if (!delIndexs.empty())
			//{
			//	for (auto& index : delIndexs)
			//	{
			//		maskData.erase(index);
			//	}
			//}
			for (auto& index : indexs)
			{
				auto iter = maskData.find(index);
				if (iter == maskData.end())
					continue;
				if (!iter->second.needUpdate)
					continue;
				if (data->roadCache.find(index) == data->roadCache.end())
					continue;
				iter->second.needUpdate = false;
				for (auto& com : data->roadCache[index])
				{
					pavementSet.insert(com);
				}
			}
			if (!pavementSet.empty())
			{
				updatePlanetPavementMaskCache(planet, pavementSet);
			}
		}
	}

	Echo::ProtocolResult RoadManager::ProCom_CheckPlanetPosition_FitGorundRoad(void* data, Echo::ProtocolHandle handle)
	{
		VegRoadQueryPackage* cache = (VegRoadQueryPackage*)data;
		RoadManager::instance()->CheckPlanetPosition_FitGorundRoad(cache);
		return Echo::PR_OK;
	}

	struct MaskData
	{
		ProceduralSphere::NodeInfo areaNode;
		int		areaID = 0;

		Vector2	areaIndex = Vector2::ZERO;//映射到外接立方体上对应面的坐标（-1，1）映射转到（0，2）映射
		Vector2 areaPosition = Vector2::ZERO;//地块id对应区域(0,100)映射下取值
	};

	MaskData getPlanetMaskData(SphericalTerrain* planet, const Vector3& worldPos)
	{
		MaskData data;
		if (!planet || !planet->isCreateFinish())
			return data;

		{
			Vector3 pos = planet->ToLocalSpace(worldPos);
			if(planet->isSphere())
			{
				//计算星球映射立方体的面和在该面上[-1,1]的映射
				SphericalTerrainQuadTreeNode::MapToCube(pos, data.areaNode.slice, data.areaIndex);
				//[-1,1]转到[0,2]的映射
				data.areaIndex += 1.0f;
				//星球四叉树节点最大深度下，在[0,2]映射下单位映射长度
				float partLength = (float)SphericalTerrainQuadTreeNode::getLengthOnCube(planet->m_maxDepth);
				//星球四叉树节点最大深度下，映射值在最大深度对应的地块，在映射面上的索引
				data.areaNode.x = Math::IFloor(data.areaIndex.x / partLength);
				data.areaNode.y = Math::IFloor(data.areaIndex.y / partLength);
				//星球四叉树节点最大深度下，映射索引对应的地块id
				data.areaID = data.areaNode.slice * SphericalTerrainQuadTreeNode::getTileCountOverLevel(planet->m_maxDepth) +
					SphericalTerrainQuadTreeNode::getTileCountOverLevel(planet->m_maxDepth - 1) + data.areaNode.x + data.areaNode.y * (1 << planet->m_maxDepth);
				//和星球四叉树节点最大深度保持一致
				data.areaNode.depth = planet->m_maxDepth;
				//计算在立方体映射面上对应映射值，在四叉树最大深度下对应地块上，按100*100规格划分的位置映射的索引(x,y)
				data.areaPosition = partLength * Vector2(static_cast<float>(data.areaNode.x), static_cast<float>(data.areaNode.y));
				data.areaPosition = data.areaIndex - data.areaPosition;
				float partSize = (2.0f / 100) / (1 << planet->m_maxDepth);
				data.areaPosition /= partSize;
			}
			else if (planet->isTorus())
			{
				data.areaIndex = TorusTerrainNode::MapToPlane(pos);

				const Vector2 sliceXy = data.areaIndex * Vector2(static_cast<float>(planet->m_numSlice[0]), static_cast<float>(planet->m_numSlice[1]));
				const int sliceX = std::clamp(static_cast<int>(std::floorf(sliceXy.x)), 0, planet->m_numSlice[0] - 1);
				const int sliceY = std::clamp(static_cast<int>(std::floorf(sliceXy.y)), 0, planet->m_numSlice[1] - 1);
				data.areaNode.slice = sliceX + sliceY * planet->m_numSlice[0];

				data.areaNode.depth = planet->m_maxDepth;

				const int sliceIndexX = data.areaNode.slice % planet->m_numSlice[0];
				const int sliceIndexY = data.areaNode.slice / planet->m_numSlice[0];
				Vector2 size = 1.0f / Vector2(static_cast<float>(planet->m_numSlice[0]), static_cast<float>(planet->m_numSlice[1]));
				Vector2 offset = size * Vector2(static_cast<float>(sliceIndexX), static_cast<float>(sliceIndexY));
				size *= TorusTerrainNode::getLengthOnSlice(data.areaNode.depth);
				data.areaPosition = data.areaIndex - offset;
				data.areaNode.x = Math::IFloor(data.areaPosition.x / size.x);
				data.areaNode.y = Math::IFloor(data.areaPosition.y / size.y);
				data.areaPosition.x -= data.areaNode.x * size.x;
				data.areaPosition.y -= data.areaNode.y * size.y;

				data.areaID = data.areaNode.slice * SphericalTerrainQuadTreeNode::getTileCountOverLevel(planet->m_maxDepth) +
					SphericalTerrainQuadTreeNode::getTileCountOverLevel(planet->m_maxDepth - 1) + data.areaNode.x + data.areaNode.y * (1 << planet->m_maxDepth);

				size *= 1.0f / 100.0f;
				data.areaPosition /= size;
			}
		}
		return data;
	}
#ifdef ECHO_EDITOR
	void RoadManager::CalculateAllPlanetRoadMask(SphericalTerrain* planet, bool sync)
	{
		if (!planet || mPlanetFitGroundRoadMask.find(planet) == mPlanetFitGroundRoadMask.end())
			return;
		if (!planet->mRoad || planet->mRoad->mPlanetRoadData.isNull())
			return;
		//if (planet->m_terrainData.isNull() || planet->m_terrainData->mPlanetRoad.isNull())
		//	return;
		//const PlanetRoadGroup& roadgroup = planet->m_terrainData->mPlanetRoad->getPlanetRoadGroup();
		const PlanetRoadGroup& roadgroup = planet->mRoad->mPlanetRoadData->getPlanetRoadGroup();
		if (roadgroup.roads.empty())
			return;
		//auto begin = std::chrono::steady_clock::now();
		for (auto& road : roadgroup.roads)
		{
			updatePlanetRoadMaskCache(planet, road, true, sync);
		}
		//auto time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - begin).count();
		//LogManager::instance()->logMessage("RoadManager::CalculateAllPlanetRoadMask() use time (ms): " + std::to_string(time));
	}
#endif
	void RoadManager::CheckPlanetPosition_FitGorundRoad(VegRoadQueryPackage* inputData)
	{
		if (!inputData || !inputData->sphPtr)
			return;

		tthread::lock_guard<tthread::recursive_mutex> lock(mLock);

		if (mPlanetFitGroundRoadMask.find(inputData->sphPtr) == mPlanetFitGroundRoadMask.end())
			return;
		RoadManager::PlanetRoadFitGroundAreaMap* data = mPlanetFitGroundRoadMask[inputData->sphPtr];

		std::map<int, RoadManager::FitGroundMask>& maskDataCache = data->maskData[data->readIndex];
		if (maskDataCache.empty())
			return;

		for (auto& element : inputData->elements)
		{
			MaskData maskData = getPlanetMaskData(inputData->sphPtr, element.first);
			if (maskDataCache.find(maskData.areaID) != maskDataCache.end() && maskDataCache[maskData.areaID].needCheck)
			{
				int x = (int)maskData.areaPosition.x;
				int y = (int)maskData.areaPosition.y;
				try {
					if (maskDataCache[maskData.areaID].mask[x].test((size_t)y))
					{
						element.second = false;
					}
				}
				catch (std::exception& e)
				{
					LogManager::instance()->logMessage(LML_CRITICAL, e.what());
				}
			}
		}
		return;
	}

	void RoadManager::updatePlanetPavementMaskCache(SphericalTerrain* mPlanet, const std::set<ActorComponentPtr>& pavementVec, bool sync)
	{
		if (pavementVec.empty() || !mPlanet)
			return;
		if (mPlanetFitGroundRoadMask.find(mPlanet) == mPlanetFitGroundRoadMask.end())
			return;
		RoadManager::PlanetRoadFitGroundAreaMap* mData = mPlanetFitGroundRoadMask[mPlanet];
		//if (!mData->mTickIDs.empty())
		//	RoadHelper::instance()->abortRequest(mData->mTickIDs.back());

		RoadRangeData data;
		data.planet = mPlanet;
		{
			for (auto& com : pavementVec)
			{
				if (com.expired())
					continue;
				PavementComponent* pavement = com.getStaticCastWeakPtr<PavementComponent>().get();
				if ((pavement->getFitGroundModel() || pavement->getVegetationCollection()) && pavement->getBezierCom())
				{
					data.boxs.push_back(pavement->getWorldAABB());
					std::vector<std::array<Vector3, 4>> beziers;
					std::vector<std::array<Vector3, 2>> normals;
					size_t beziercount = pavement->getBezierCom()->getPointCount();
					beziercount = beziercount == 0 ? beziercount : beziercount - 1;
					for (size_t i = 0; i < beziercount; ++i)
					{
						PiecewiseBezierPoint*  point1 = pavement->getBezierCom()->getPoint(i);
						PiecewiseBezierPoint*  point2 = pavement->getBezierCom()->getPoint(i + 1);
						if (!point1 || !point2 || (point1->FitGorundLevel != 0 && point2->FitGorundLevel != 0))
							continue;
						std::array<Vector3, 4> bezier;
						std::array<Vector3, 2> normal;

						normal[0] = pavement->convertLocalToWorldPosition(point1->normal) - pavement->getWorldPosition();//point1->normal;
						normal[1] = pavement->convertLocalToWorldPosition(point2->normal) - pavement->getWorldPosition(); //point2->normal;

						bezier[0] = point1->point;
						bezier[1] = point1->destination_control;
						bezier[3] = point2->point;
						bezier[2] = point2->source_control;

						bezier[1] = pavement->convertLocalToWorldPosition(bezier[0] + bezier[1]);// -bezier[0];
						bezier[2] = pavement->convertLocalToWorldPosition(bezier[3] + bezier[2]);// -bezier[3];

						bezier[0] = pavement->convertLocalToWorldPosition(bezier[0]);
						bezier[3] = pavement->convertLocalToWorldPosition(bezier[3]);

						bezier[1] -= bezier[0];
						bezier[2] -= bezier[3];

						if (point1->FitGorundLevel > -1)
						{
							Vector3 normal;
							bezier[0] = mPlanet->getSurfaceFinestWs(bezier[0], &normal);
							bezier[0] += normal * (point1->FitGorundLevel * RoadManager::instance()->m_pavementFitGroundLevelHight);
						}
						if (point2->FitGorundLevel > -1)
						{
							Vector3 normal;
							bezier[3] = mPlanet->getSurfaceFinestWs(bezier[3], &normal);
							bezier[3] += normal * (point2->FitGorundLevel * RoadManager::instance()->m_pavementFitGroundLevelHight);
						}

						beziers.push_back(bezier);
						normals.push_back(normal);
					}
					const PavementComponent::PavementGroup& pavementGroups = pavement->getPavementGroups();
					for (auto& [id, pavementData] : pavementGroups)
					{
						if (m_mapPathMesh.find(pavementData.first.meshName) == m_mapPathMesh.end())
							continue;

						PavementTool::PrivateMeshData* meshData = m_mapPathMesh[pavementData.first.meshName].get();
						AxisAlignedBox box = meshData->mesh->getAABB();
						Matrix4 meshtransform;
						meshtransform.makeTransform(pavementData.first.offset, pavementData.first.scale, pavementData.first.quat);
						box.transformAffine(meshtransform);

						if (box.vMin.y > 1e-4f || box.vMax.y < 1e-4f)
							continue;

						RoadBezierData roadData;
						roadData.beziers = beziers;
						roadData.normals = normals;

						roadData.Lwidth = box.vMin.z;
						roadData.Rwidth = box.vMax.z;

						//roadData.worldquat = pavement->getWorldRotation();

						data.roads.push_back(roadData);
					}
				}
			}
		}
		if (sync)
		{
			std::vector<Vector3> vbVec;
			std::vector<uint16> ibVec;
			CalculateRoadRange(data, vbVec, ibVec);
			std::vector<Vector3> result = CalculatePlanetMask(mPlanet, vbVec, ibVec);
			RoadManager::PlanetRoadFitGroundAreaMap* tempData = mPlanetFitGroundRoadMask[mPlanet];
			std::map<int, RoadManager::FitGroundMask>& maskData = tempData->maskData[tempData->writeIndex];
			for (auto& cache : result)
			{
				int	areaID = (int)cache.x;
				//if (maskData.find(areaID) != maskData.end())
				{
					maskData[areaID].needCheck = true;
					maskData[areaID].mask[(int)cache.y][(int)cache.z] = true;
				}
			}
			{
				tthread::lock_guard<tthread::recursive_mutex> lock(mLock);
				++tempData->writeIndex;
				tempData->writeIndex &= 1;
				++tempData->readIndex;
				tempData->readIndex &= 1;
				tempData->maskData[tempData->writeIndex] = tempData->maskData[tempData->readIndex];
			}
			/*if (mPlanet->m_vegHandle)
			{
				for (auto& box : boxVec)
					mPlanet->m_vegHandle->recheckPosition(box);
			}*/
		}
		else
			mData->mTickIDs.push_back(RoadHelper::instance()->UpdatePlanetPavementMask(data));

	}

	void RoadManager::updatePlanetBuildingMaskCache(SphericalTerrain* mPlanet, bool isRemoveVeg)
	{
		if (mPlanetFitGroundRoadMask.find(mPlanet) == mPlanetFitGroundRoadMask.end() || !mPlanet->isCreateFinish())
			return;
		
		auto convertLocalToWorldPosition = [](const Vector3& LocalPos, const Vector3& parentWorldPos, const Quaternion& parentWorldQuat)
		{
			return parentWorldPos + parentWorldQuat * LocalPos;
		};
		BuildingRangeData buildData;
		buildData.planet = mPlanet;
		buildData.removeVeg = true;
		std::vector<OrientedBox> buildBoxs = mPlanet->getFlatAreas();
		for (auto& buildbox : buildBoxs)
		{
			BuildingRangeData::BuildingData data;
			data.worldPos = convertLocalToWorldPosition(buildbox.center, mPlanet->m_pos, mPlanet->m_rot);
			data.worldQuat = mPlanet->m_rot * buildbox.orientation;
			data.box.setExtents(-buildbox.extent, buildbox.extent);
			buildData.builds.push_back(data);
			if (buildData.builds.size() > 100)
			{
				mPlanetFitGroundRoadMask[mPlanet]->mTickIDs.push_back(RoadHelper::instance()->UpdatePlanetBuildingMask(buildData));
				buildData.builds.clear();
			}
		}
		if (!buildData.builds.empty())
			mPlanetFitGroundRoadMask[mPlanet]->mTickIDs.push_back(RoadHelper::instance()->UpdatePlanetBuildingMask(buildData));
	}

	void RoadManager::updatePlanetRoadMaskCache(SphericalTerrain* mPlanet, const PlanetRoadData& road, bool isRemoveVeg, bool sync)
	{
		//updateMaskCache(mPlanet, road.roadbox, mPlanet->m_pos, mPlanet->m_rot, true);
		//return;
		if ((int)road.beziers.size() <= 1 || road.datas.empty() || road.roadbox.isNull())
			return;
		if (mPlanetFitGroundRoadMask.find(mPlanet) == mPlanetFitGroundRoadMask.end())
			return;
		RoadManager::PlanetRoadFitGroundAreaMap* mData = mPlanetFitGroundRoadMask[mPlanet];
		RoadRangeData data;
		data.planet = mPlanet;
		data.removeVeg = isRemoveVeg;

		AxisAlignedBox box = road.roadbox;
		Matrix4 meshtransform;
		meshtransform.makeTransform(mPlanet->m_pos, Vector3::UNIT_SCALE, mPlanet->m_rot);
		box.transformAffine(meshtransform);
		data.boxs.push_back(box);
		std::vector<std::array<Vector3, 4>> beziers;
		std::vector<std::array<Vector3, 2>> normals;
		auto convertPlanetLocalToWorldPosition = [](const Vector3& planetLocalPos, const Vector3& planetWorldPos, const Quaternion& planetWorldQuat)
		{
			return planetWorldPos + planetWorldQuat * planetLocalPos;
		};
		size_t beziercount = road.beziers.size();
		beziercount = beziercount == 0 ? beziercount : beziercount - 1;
		for (size_t i = 0; i < beziercount; ++i)
		{
			const PlanetRoadBezierPoint* point1 = &road.beziers[i];
			const PlanetRoadBezierPoint* point2 = &road.beziers[i + 1];
			std::array<Vector3, 4> bezier;
			std::array<Vector3, 2> normal;

			normal[0] = convertPlanetLocalToWorldPosition(point1->normal, mPlanet->m_pos, mPlanet->m_rot) - mPlanet->m_pos;
			normal[1] = convertPlanetLocalToWorldPosition(point2->normal, mPlanet->m_pos, mPlanet->m_rot) - mPlanet->m_pos;

			bezier[0] = point1->point;
			bezier[1] = point1->destination_control;
			bezier[3] = point2->point;
			bezier[2] = point2->source_control;

			bezier[1] = convertPlanetLocalToWorldPosition(bezier[0] + bezier[1], mPlanet->m_pos, mPlanet->m_rot);
			bezier[2] = convertPlanetLocalToWorldPosition(bezier[3] + bezier[2], mPlanet->m_pos, mPlanet->m_rot);

			bezier[0] = convertPlanetLocalToWorldPosition(bezier[0], mPlanet->m_pos, mPlanet->m_rot);
			bezier[3] = convertPlanetLocalToWorldPosition(bezier[3], mPlanet->m_pos, mPlanet->m_rot);

			bezier[1] -= bezier[0];
			bezier[2] -= bezier[3];

			beziers.push_back(bezier);
			normals.push_back(normal);
		}
		RoadBezierData roadData;
		roadData.beziers = beziers;
		roadData.normals = normals;

		roadData.Lwidth = -road.width;
		roadData.Rwidth = road.width;

		data.roads.push_back(roadData);

		if (sync)
		{
			std::vector<Vector3> vbVec;
			std::vector<uint16> ibVec;
			CalculateRoadRange(data, vbVec, ibVec);
			std::vector<Vector3> result = CalculatePlanetMask(mPlanet, vbVec, ibVec);
			RoadManager::PlanetRoadFitGroundAreaMap* tempData = mPlanetFitGroundRoadMask[mPlanet];
			std::map<int, RoadManager::FitGroundMask>& maskData = tempData->maskData[tempData->writeIndex];
			for (auto& cache : result)
			{
				int	areaID = (int)cache.x;
				//if (maskData.find(areaID) != maskData.end())
				{
					maskData[areaID].needCheck = true;
					maskData[areaID].mask[(int)cache.y][(int)cache.z] = isRemoveVeg;
				}
			}
			{
				tthread::lock_guard<tthread::recursive_mutex> lock(mLock);
				++tempData->writeIndex;
				tempData->writeIndex &= 1;
				++tempData->readIndex;
				tempData->readIndex &= 1;
				tempData->maskData[tempData->writeIndex] = tempData->maskData[tempData->readIndex];
			}
			if (mPlanet->m_vegHandle)
			{
				for (auto& box : data.boxs)
					mPlanet->m_vegHandle->recheckPosition(box);
			}
		}
		else
			mData->mTickIDs.push_back(RoadHelper::instance()->UpdatePlanetPavementMask(data));
	}

	void RoadManager::updateMaskCache(SphericalTerrain* mPlanet, const AxisAlignedBox& localBox, const Vector3& worldPos, const Quaternion& worldQuat, bool isRemoveVeg)
	{
		//auto start = std::chrono::steady_clock::now();

		if (mPlanetFitGroundRoadMask.find(mPlanet) == mPlanetFitGroundRoadMask.end())
			return;

		auto convertLocalToWorldPosition = [](const Vector3& LocalPos, const Vector3& parentWorldPos, const Quaternion& parentWorldQuat)
		{
			return parentWorldPos + parentWorldQuat * LocalPos;
		};

		std::vector<Vector3> vbVec;
		std::vector<uint16> ibVec;

		{
			Vector3 p1 = convertLocalToWorldPosition(localBox.vMin, worldPos, worldQuat);
			Vector3 p2 = convertLocalToWorldPosition(Vector3(localBox.vMin.x, localBox.vMin.y, localBox.vMax.z), worldPos, worldQuat);
			Vector3 p3 = convertLocalToWorldPosition(Vector3(localBox.vMax.x, localBox.vMin.y, localBox.vMax.z), worldPos, worldQuat);
			Vector3 p4 = convertLocalToWorldPosition(Vector3(localBox.vMax.x, localBox.vMin.y, localBox.vMin.z), worldPos, worldQuat);

			vbVec.push_back(p1);
			vbVec.push_back(p2);
			vbVec.push_back(p3);
			vbVec.push_back(p4);

			ibVec.push_back(0);
			ibVec.push_back(1);
			ibVec.push_back(3);

			ibVec.push_back(1);
			ibVec.push_back(2);
			ibVec.push_back(3);

			/*{
				static HelperObject* helper = nullptr;
				if (helper == nullptr)
				{
					helper = new HelperObject(Name("test"), HelperObjType_CustomPolygon, true, nullptr, Root::instance()->getMainSceneManager());
					helper->setRenderType(ECHO_TRIANGLE_LIST);
				}
				helper->changeCustomPolygon(vbVec, ibVec);
			}*/
		}

		std::vector<Vector3> result = CalculatePlanetMask(mPlanet, vbVec, ibVec);
		RoadManager::PlanetRoadFitGroundAreaMap* tempData = mPlanetFitGroundRoadMask[mPlanet];
		std::map<int, RoadManager::FitGroundMask>& maskData = tempData->maskData[tempData->writeIndex];
		for (auto& cache : result)
		{
			int	areaID = (int)cache.x;
			maskData[areaID].needCheck = true;
			maskData[areaID].mask[(int)cache.y][(int)cache.z] = isRemoveVeg;
		}
		{
			tthread::lock_guard<tthread::recursive_mutex> lock(mLock);
			++tempData->writeIndex;
			tempData->writeIndex &= 1;
			++tempData->readIndex;
			tempData->readIndex &= 1;
			tempData->maskData[tempData->writeIndex] = tempData->maskData[tempData->readIndex];
		}
		if (mPlanet->m_vegHandle)
		{
			AxisAlignedBox box = localBox;
			Matrix4 meshtransform;
			meshtransform.makeTransform(worldPos, Vector3::UNIT_SCALE, worldQuat);
			box.transformAffine(meshtransform);
			mPlanet->m_vegHandle->recheckPosition(box);
		}

		//auto end = std::chrono::steady_clock::now();
		//auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
		//LogManager::instance()->logMessage("RoadManager::updateMaskCache用时：" + std::to_string(duration_ms) + " ms");
	}

	void RoadManager::CalculateRoadRange(const RoadRangeData& data, std::vector<Vector3>& vbVec, std::vector<uint16>& ibVec)
	{
		if (!data.planet || data.roads.empty())
			return;
		planetTerrainTool->planet = data.planet;

		for (auto& roadData : data.roads)
		{
			const float step = abs(roadData.Rwidth);
			Matrix4 left, right;// , _planet;
			left.makeTransform(std::move(Vector3(0.0f, 0.0f, roadData.Lwidth)), Vector3::UNIT_SCALE, Quaternion::IDENTITY);
			right.makeTransform(std::move(Vector3(0.0f, 0.0f, roadData.Rwidth)), Vector3::UNIT_SCALE, Quaternion::IDENTITY);
			//_planet.makeTransform(data.planet->m_pos, Vector3::UNIT_SCALE, data.planet->m_rot);

			int beziercount = (int)roadData.beziers.size();
			for (int index = 0; index < beziercount; ++index)
			{
				const auto& bezierData = roadData.beziers[index];
				float alllength = PiecewiseBezierUtil::LengthFromTime(bezierData[0], bezierData[1], bezierData[3], bezierData[2], 1.0f);

				int count = Math::IFloor(alllength / step);
				if (count <= 0)
					count = 1;
				float partStep = alllength / count;
				int vbcount = (int)vbVec.size();

				std::unordered_map<float, float>tempdata;
				for (int i = 0; i <= count; ++i)
				{
					float length = i * partStep;

					vbVec.push_back(PiecewiseBezierUtil::GetBezierPositionAndMatrix3FromLength(bezierData[0], bezierData[1], bezierData[2], bezierData[3],
						length, roadData.normals[index][0], roadData.normals[index][1], left, Matrix4::IDENTITY));
					vbVec.push_back(PiecewiseBezierUtil::GetBezierPositionAndMatrix3FromLength(bezierData[0], bezierData[1], bezierData[2], bezierData[3],
						length, roadData.normals[index][0], roadData.normals[index][1], right, Matrix4::IDENTITY));

					if (i < count)
					{
						ibVec.push_back(vbcount + i * 2);
						ibVec.push_back(vbcount + i * 2 + 1);
						ibVec.push_back(vbcount + i * 2 + 3);

						ibVec.push_back(vbcount + i * 2);
						ibVec.push_back(vbcount + i * 2 + 3);
						ibVec.push_back(vbcount + i * 2 + 2);
					}

				}
			}
		}
	}

	std::vector<Vector3> RoadManager::CalculatePlanetMask(SphericalTerrain* mPlanet, const std::vector<Vector3>& worldPositions, const std::vector<uint16>& indexVec)
	{
		std::vector<Vector3> result;

		if (!mPlanet || !mPlanet->isCreateFinish())
			return result;
		/*{
			static HelperObject* helper = nullptr;
			if (helper == nullptr)
			{
				helper = new HelperObject(Name("test"), HelperObjType_CustomPolygon, true, nullptr, Root::instance()->getMainSceneManager());
				helper->setRenderType(ECHO_TRIANGLE_LIST);
			}
			helper->changeCustomPolygon(worldPositions, indexVec);
		}*/
		Vector2 partSize;
		if (mPlanet->isSphere())//几何星球
		{
			partSize = std::move(Vector2((float)SphericalTerrainQuadTreeNode::getLengthOnCube(mPlanet->m_maxDepth)));
		}
		else if (mPlanet->isTorus())//环形星球
		{
			partSize = 1.0f / Vector2(static_cast<float>(mPlanet->m_numSlice[0]), static_cast<float>(mPlanet->m_numSlice[1]));
			partSize *= TorusTerrainNode::getLengthOnSlice(mPlanet->m_maxDepth);
		}

		auto CalculateSameSliceResult = [&result](const Vector2& p1, const Vector2& p2, const Vector2& p3, int minX, int minY, int planetSlice, int planetDepth)
		{
			float fmax_x, fmin_x, fmax_y, fmin_y;
			fmax_x = Math::Max(p1.x, Math::Max(p2.x, p3.x));
			fmin_x = Math::Min(p1.x, Math::Min(p2.x, p3.x));
			fmax_y = Math::Max(p1.y, Math::Max(p2.y, p3.y));
			fmin_y = Math::Min(p1.y, Math::Min(p2.y, p3.y));

			for (int x = Math::IFloor(fmin_x); x <= Math::IFloor(fmax_x) + 1; ++x)
			{
				for (int y = Math::IFloor(fmin_y); y <= Math::IFloor(fmax_y) + 1; ++y)
				{
					if (Math::pointInTri2D(std::move(Vector2((float)x + 0.2f, (float)y + 0.2f)), p1, p2, p3) ||
						Math::pointInTri2D(std::move(Vector2((float)x + 0.8f, (float)y + 0.2f)), p1, p2, p3) ||
						Math::pointInTri2D(std::move(Vector2((float)x + 0.2f, (float)y + 0.8f)), p1, p2, p3) ||
						Math::pointInTri2D(std::move(Vector2((float)x + 0.8f, (float)y + 0.8f)), p1, p2, p3) ||
						(x == Math::IFloor(p1.x) && y == Math::IFloor(p1.y)) ||
						(x == Math::IFloor(p2.x) && y == Math::IFloor(p2.y)) ||
						(x == Math::IFloor(p3.x) && y == Math::IFloor(p3.y)))
					{
						int area_x = minX + x / 100;
						int area_y = minY + y / 100;
						int areaTempID = planetSlice * SphericalTerrainQuadTreeNode::getTileCountOverLevel(planetDepth) +
							SphericalTerrainQuadTreeNode::getTileCountOverLevel(planetDepth - 1) + area_x + area_y * (1 << planetDepth);
						result.push_back(Vector3((float)areaTempID, (float)(x % 100), (float)(y % 100)));
					}
				}
			}
		};

		auto CalculateDifferentSliceResult = [&result, &partSize, &mPlanet](int slice, const Plane& plane, Vector3 pos1, Vector3 pos2, Vector3 pos3)
		{
			float dist1, dist2, dist3;
			if (plane.IntersectRay(Vector3::ZERO, pos1, dist1))
				pos1 *= dist1;
			if (plane.IntersectRay(Vector3::ZERO, pos2, dist2))
				pos2 *= dist2;
			if (plane.IntersectRay(Vector3::ZERO, pos3, dist3))
				pos3 *= dist3;

			Vector2 p1, p2, p3;
			p1.x = pos1.Dot(SphericalTerrainQuadTreeNode::FaceRight[slice]);
			p1.y = pos1.Dot(SphericalTerrainQuadTreeNode::FaceUp[slice]);
			p2.x = pos2.Dot(SphericalTerrainQuadTreeNode::FaceRight[slice]);
			p2.y = pos2.Dot(SphericalTerrainQuadTreeNode::FaceUp[slice]);
			p3.x = pos3.Dot(SphericalTerrainQuadTreeNode::FaceRight[slice]);
			p3.y = pos3.Dot(SphericalTerrainQuadTreeNode::FaceUp[slice]);

			p1 += 1.f;
			p2 += 1.f;
			p3 += 1.f;

			int imax_x, imin_x, imax_y, imin_y;
			imax_x = Math::IFloor(100.f * Math::Max(p1.x, Math::Max(p2.x, p3.x)) / partSize.x) + 1;
			imin_x = Math::IFloor(100.f * Math::Min(p1.x, Math::Min(p2.x, p3.x)) / partSize.x);
			imax_y = Math::IFloor(100.f * Math::Max(p1.y, Math::Max(p2.y, p3.y)) / partSize.y) + 1;
			imin_y = Math::IFloor(100.f * Math::Min(p1.y, Math::Min(p2.y, p3.y)) / partSize.y);

			for (int x = Math::Max(imin_x, 0); x <= Math::Min(imax_x, 100 * (1 << mPlanet->m_maxDepth)); ++x)
			{
				for (int y = Math::Max(imin_y, 0); y <= Math::Min(imax_y, 100 * (1 << mPlanet->m_maxDepth)); ++y)
				{
					int point_x = Math::IFloor(x * partSize.x / 100.f);
					int point_y = Math::IFloor(y * partSize.y / 100.f);
					
					if (Math::pointInTri2D(std::move(Vector2((float)point_x + 0.2f, (float)point_y + 0.2f)), p1, p2, p3) ||
						Math::pointInTri2D(std::move(Vector2((float)point_x + 0.8f, (float)point_y + 0.2f)), p1, p2, p3) ||
						Math::pointInTri2D(std::move(Vector2((float)point_x + 0.2f, (float)point_y + 0.8f)), p1, p2, p3) ||
						Math::pointInTri2D(std::move(Vector2((float)point_x + 0.8f, (float)point_y + 0.8f)), p1, p2, p3) ||
						(point_x == Math::IFloor(p1.x) && point_y == Math::IFloor(p1.y)) ||
						(point_x == Math::IFloor(p2.x) && point_y == Math::IFloor(p2.y)) ||
						(point_x == Math::IFloor(p3.x) && point_y == Math::IFloor(p3.y)))
					{
						int area_x = x / 100;
						int area_y = y / 100;
						int areaTempID = slice * SphericalTerrainQuadTreeNode::getTileCountOverLevel(mPlanet->m_maxDepth) +
							SphericalTerrainQuadTreeNode::getTileCountOverLevel(mPlanet->m_maxDepth - 1) + area_x + area_y * (1 << mPlanet->m_maxDepth);
						result.push_back(Vector3((float)areaTempID, (float)(x % 100), (float)(y % 100)));
					}
				}
			}
		};

		int size = (int)indexVec.size();
		for (int index = 0; index < size; ++index)
		{
			if (index >= (size - 2) ||
				indexVec[index] >= (int)worldPositions.size() ||
				indexVec[index + 1] >= (int)worldPositions.size() ||
				indexVec[index + 2] >= (int)worldPositions.size())
				break;
			Vector3 pos1 = worldPositions[indexVec[index]];
			Vector3 pos2 = worldPositions[indexVec[++index]];
			Vector3 pos3 = worldPositions[indexVec[++index]];

			MaskData point1 = getPlanetMaskData(mPlanet, pos1);
			MaskData point2 = getPlanetMaskData(mPlanet, pos2);
			MaskData point3 = getPlanetMaskData(mPlanet, pos3);

			if (point1.areaNode.slice != point2.areaNode.slice || point2.areaNode.slice != point3.areaNode.slice || point1.areaNode.slice != point3.areaNode.slice)
			{
				Plane plane1, plane2, plane3;
				plane1.FromPointNormal(SphericalTerrainQuadTreeNode::FaceForward[point1.areaNode.slice], SphericalTerrainQuadTreeNode::FaceForward[point1.areaNode.slice]);
				plane2.FromPointNormal(SphericalTerrainQuadTreeNode::FaceForward[point2.areaNode.slice], SphericalTerrainQuadTreeNode::FaceForward[point2.areaNode.slice]);
				plane3.FromPointNormal(SphericalTerrainQuadTreeNode::FaceForward[point3.areaNode.slice], SphericalTerrainQuadTreeNode::FaceForward[point3.areaNode.slice]);

				pos1 = mPlanet->ToLocalSpace(pos1);
				pos2 = mPlanet->ToLocalSpace(pos2);
				pos3 = mPlanet->ToLocalSpace(pos3);

				CalculateDifferentSliceResult(point1.areaNode.slice, plane1, pos1, pos2, pos3);
				CalculateDifferentSliceResult(point2.areaNode.slice, plane2, pos1, pos2, pos3);
				CalculateDifferentSliceResult(point3.areaNode.slice, plane3, pos1, pos2, pos3);
			}
			else
			{
				int min_x, min_y;
				if (point1.areaID != point2.areaID || point1.areaID != point3.areaID || point2.areaID != point3.areaID)
				{
					min_x = Math::Min(point1.areaNode.x, Math::Min(point2.areaNode.x, point3.areaNode.x));
					min_y = Math::Min(point1.areaNode.y, Math::Min(point2.areaNode.y, point3.areaNode.y));

					point1.areaPosition.x += (point1.areaNode.x - min_x) * 100;
					point2.areaPosition.x += (point2.areaNode.x - min_x) * 100;
					point3.areaPosition.x += (point3.areaNode.x - min_x) * 100;

					point1.areaPosition.y += (point1.areaNode.y - min_y) * 100;
					point2.areaPosition.y += (point2.areaNode.y - min_y) * 100;
					point3.areaPosition.y += (point3.areaNode.y - min_y) * 100;
				}
				else
				{
					if (point1.areaIndex.distance(point2.areaIndex) > partSize.length()
						|| point1.areaIndex.distance(point3.areaIndex) > partSize.length()
						|| point2.areaIndex.distance(point3.areaIndex) > partSize.length())
						continue;
					min_x = point1.areaNode.x;
					min_y = point1.areaNode.y;
				}

				CalculateSameSliceResult(point1.areaPosition, point2.areaPosition, point3.areaPosition, min_x, min_y, point1.areaNode.slice, mPlanet->m_maxDepth);
			}

		}
		return result;

	}

	void RoadManager::operationCompleted_PlanetMask(WorkQueue::RequestID tickID, SphericalTerrain* mPlanet, const std::vector<Vector3>& data, const std::vector<AxisAlignedBox>& boxVec, bool isRemove)
	{
		if (mPlanetFitGroundRoadMask.find(mPlanet) == mPlanetFitGroundRoadMask.end())
			return;
		RoadManager::PlanetRoadFitGroundAreaMap* tempData = mPlanetFitGroundRoadMask[mPlanet];
		auto iter = std::find(tempData->mTickIDs.begin(), tempData->mTickIDs.end(), tickID);
		if (tempData->mTickIDs.end() == iter)
			return;
		tempData->mTickIDs.erase(iter);

		std::map<int, RoadManager::FitGroundMask>& maskData = tempData->maskData[tempData->writeIndex];
		for (auto& cache : data)
		{
			int	areaID = (int)cache.x;
			//if (maskData.find(areaID) != maskData.end())
			{
				maskData[areaID].needCheck = true;
				maskData[areaID].mask[(int)cache.y][(int)cache.z] = isRemove;
			}
		}
		{
			tthread::lock_guard<tthread::recursive_mutex> lock(mLock);
			++tempData->writeIndex;
			tempData->writeIndex &= 1;
			++tempData->readIndex;
			tempData->readIndex &= 1;
			tempData->maskData[tempData->writeIndex] = tempData->maskData[tempData->readIndex];
		}
		if (mPlanet->m_vegHandle)
		{
			for (auto& box : boxVec)
				mPlanet->m_vegHandle->recheckPosition(box);
		}
	}
#ifdef ECHO_EDITOR
	void RoadManager::UpdateAllPavementPhysics(bool sync)
	{
		if (Root::instance()->UsePavementTestNewMode)
		{
			for (auto& pavement : m_vecPavements)
			{
				pavement->updateAllPavement(PavementComponent::UpdatePhysics, sync);
			}
		}
	}
#endif

#ifdef ECHO_EDITOR
	namespace
	{
		const size_t BVH_MAX_TRIANGLE = 32;
		const size_t BVH_MAX_DIVISION = 32;
	}

	bool RoadManager::GetPointPickEntity(MovableObject* moveable, const Ray& ray, Vector3& pos, float& dis)
	{
		bool res = false;
		EntityObject* ent = static_cast<EntityObject*>(moveable);
		if (!Echo::Math::intersects(ray, ent->getWorldBoundingBox()).first)
			return res;
		Entity* entity = static_cast<Entity*>(moveable);
		Echo::MeshPtr ptrMesh = entity->getMesh();

		String meshname = ptrMesh->getName().toString();
		size_t itr1 = meshname.find("_lod1");
		size_t itr2 = meshname.find("_lod2");
		if (itr1 != size_t(-1) || itr2 != size_t(-1))
		{
			String name(meshname.begin(), meshname.end() - 10);
			name += ".mesh";
			meshname = name;
			ptrMesh = Echo::MeshMgr::instance()->load(Name(meshname));
		}

		auto iter = m_mapMeshBVH.find(ptrMesh->getName());
		Echo::Matrix4 mt4World = entity->_getParentNodeFullTransform();
		/*Vector3 worldof = entity->getWorldPosition();
		mt4World[3][0] -= worldof.x;
		mt4World[3][1] -= worldof.y;
		mt4World[3][2] -= worldof.z;*/

		Vector3 hit_position = Vector3::ZERO;
		if (iter != m_mapMeshBVH.end())
			res |= iter->second->getPickHeight(ray, hit_position, dis, mt4World);
		else
		{
			TreeManager* MeshNode = new TreeManager(ptrMesh);
			res |= MeshNode->getPickHeight(ray, hit_position, dis, mt4World);
			m_mapMeshBVH[ptrMesh->getName()] = MeshNode;
		}

		if (res)
			pos = hit_position;
		return res;
	}

	TreeManager::TreeManager(MeshPtr& mesh)
	{
		std::vector<int> pIndexs;
		std::vector<BVH_Node::Triangle> pTriangles;

		// 获取Mesh内部顶点与索引信息
		Echo::Vector3 pos[3];
		for (unsigned short s = 0; s < mesh->getNumSubMesh(); ++s)
		{
			Echo::SubMesh* submesh = mesh->getSubMesh(s);
			MeshVertexIndexData data;
			submesh->getVertexIndexData(data);

			m_vDatas.push_back(data.vertices);
			pIndexs = data.indices;

			for (size_t i = 0; i < pIndexs.size(); i += 3)
				pTriangles.push_back({ (uint16)pIndexs[i], (uint16)pIndexs[i + 1] , (uint16)pIndexs[i + 2], uint16(m_vDatas.size() - 1) });
		}

		AxisAlignedBox ab = mesh->getAABB();
		Vector3 abv = {
			std::abs(ab.vMax.x - ab.vMin.x),
			std::abs(ab.vMax.y - ab.vMin.y),
			std::abs(ab.vMax.z - ab.vMin.z),
		};
		float max_l = std::max(abv.x, std::max(abv.y, abv.z));

		m_pRoot = new BVH_Node(pTriangles, this, max_l);
	}

	TreeManager::~TreeManager()
	{
		if (m_pRoot) delete m_pRoot;
	}

	bool TreeManager::getPickHeight(const Echo::Ray& ray, Vector3& hit_pos, float& hit_dis, const Echo::Matrix4& mt4World)
	{
		if (!m_pRoot) return false;
		const Echo::Matrix4 mt4InvWorld = mt4World.inverse();
		Echo::Matrix3 mt3World;
		mt4World.extract3x3Matrix(mt3World);
		const Echo::Matrix3 mt3InvWorld = mt3World.Inverse();
		Echo::Vector3 v3InvOri = mt4InvWorld * ray.getOrigin();
		Echo::Vector3 v3InvDir = mt3InvWorld * ray.getDirection();
		//v3InvDir = v3InvDir.normalisedCopy();
		Echo::Ray rayLocal(v3InvOri, v3InvDir);
		if (m_pRoot->getPickHeight(rayLocal, hit_dis))
		{
			Vector3 pos = rayLocal.getPoint(hit_dis);
			hit_pos = mt4World * pos;
			Vector3 h_pos = ray.getPoint(hit_dis);
			hit_pos = h_pos;
			return true;
		}
		else
			return false;
	}

	TreeManager::BVH_Node::BVH_Node(std::vector<Triangle>& pTriangles, TreeManager* pThis, float mDivisionValue)
	{
		std::vector<std::vector<Vector3>>& pVertexs = pThis->m_vDatas;

		for (size_t i = 0; i < pTriangles.size(); ++i)
		{
			Triangle& tra = pTriangles[i];

			uint16 subnum, data;
			Triangle::DataUnbinding(tra.a, subnum, data);
			m_Box.merge(pVertexs[subnum][data]);

			Triangle::DataUnbinding(tra.b, subnum, data);
			m_Box.merge(pVertexs[subnum][data]);

			Triangle::DataUnbinding(tra.c, subnum, data);
			m_Box.merge(pVertexs[subnum][data]);
		}

		p_Datas = pThis;

		if (pTriangles.size() <= BVH_MAX_TRIANGLE || mDivisionValue <= BVH_MAX_DIVISION)
		{
			m_vTriangles = pTriangles;
			return;
		}

		// 计算划分八叉树的划分点
		m_Center = {
			m_Box.vMin.x + (m_Box.vMax.x - m_Box.vMin.x) / 2.f,
			m_Box.vMin.y + (m_Box.vMax.y - m_Box.vMin.y) / 2.f,
			m_Box.vMin.z + (m_Box.vMax.z - m_Box.vMin.z) / 2.f
		};

		// 创建八叉树各存储三角形信息
		std::vector<std::vector<Triangle>> pChildTriangles;
		for (size_t i = 0; i < 8; ++i)
		{
			std::vector<Triangle> triangles;
			pChildTriangles.push_back(triangles);
		}

		for (size_t i = 0; i < pTriangles.size(); ++i)
		{
			auto& triangle = pTriangles[i];
			int index[3] = { 0,0,0 };

			// 去除重复在八叉树内插入相同三角。
			std::vector<int> real_index;

			for (size_t j = 0; j < 3; ++j)
			{
				// 计算单个顶点坐标所处八叉树位置
				for (size_t index_off = 0; index_off < 3; ++index_off)
				{
					uint16 subnum, subdata;
					Triangle::DataUnbinding(triangle.index[j], subnum, subdata);
					Vector3 vec_d = pVertexs[subnum][subdata];
					float data = vec_d[index_off];
					if (data >= m_Center[index_off])
						index[j] += (int)pow(2.0, index_off);
				}


				bool isSame = false;
				for (size_t pk = j; pk > 0; --pk)
				{
					if (index[j] == index[pk - 1])
						isSame = true;
				}

				if (!isSame) real_index.push_back(index[j]);
			}

			for (size_t j = 0; j < real_index.size(); ++j)
				pChildTriangles[real_index[j]].push_back(triangle);
		}

		for (size_t i = 0; i < 8; ++i)
			if (!pChildTriangles[i].empty())
				m_pChildNode[i] = new TreeManager::BVH_Node(pChildTriangles[i], pThis, mDivisionValue / 2.f);
	}

	TreeManager::BVH_Node::~BVH_Node()
	{
		for (size_t i = 0; i < 8; ++i)
			if (m_pChildNode[i])
				delete m_pChildNode[i];
	}

	bool TreeManager::BVH_Node::getPickHeight(const Echo::Ray& ray, float& hit_distance)
	{
		std::pair<bool, float> res = Echo::Math::intersects(ray, m_Box);
		if (!res.first || res.second < 0.f || res.second > hit_distance)
			return false;
		bool result = false;
		if (m_vTriangles.empty())
		{
			for (size_t i = 0; i < 8; ++i)
				if (m_pChildNode[i])
					result |= m_pChildNode[i]->getPickHeight(ray, hit_distance);
		}
		else
		{
			for (size_t i = 0; i < m_vTriangles.size(); ++i)
			{
				Triangle triangle = m_vTriangles[i];
				uint16 subnum, subdata;
				Triangle::DataUnbinding(triangle.a, subnum, subdata);
				const Vector3& pos1 = p_Datas->m_vDatas[subnum][subdata];
				Triangle::DataUnbinding(triangle.b, subnum, subdata);
				const Vector3& pos2 = p_Datas->m_vDatas[subnum][subdata];
				Triangle::DataUnbinding(triangle.c, subnum, subdata);
				const Vector3& pos3 = p_Datas->m_vDatas[subnum][subdata];

				std::pair<bool, float> hit = Echo::Math::intersects(ray, pos1, pos2, pos3, true, true);

				if (hit.first && hit.second < hit_distance)
				{
					hit_distance = hit.second;
					result = true;
				}
			}
		}

		return result;
	}

#endif // ECHO_EDITOR

}
