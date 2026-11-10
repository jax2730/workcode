#include "EchoComponentStableHeaders.h"
#include "EchoPavementComponent.h"
#include "EchoPiecewiseBezierComponent.h"
#include "EchoLogManager.h"
#include "EchoPiecewiseBezierUtil.h"
#include "EchoRoadManager.h"
#include "EchoPavementObject.h"
#include "Actor/EchoActor.h"
#include "EchoPlanetRoadResource.h"
#include "EchoPlanetRoad.h"

namespace Echo
{
	void WriteJSNode(JSNODE pRoot, const char* name, const std::vector<PavementComponentInfo::LayerDatas_*>& value) {
		cJSON* jsonArray = cJSON_CreateArray();
		
		for (auto& v : value)
		{
			cJSON* jsonNode = cJSON_CreateObject();
			cJSON_AddItemToArray(jsonArray, jsonNode);
			v->ExportJson(jsonNode);
		}
		cJSON_AddItemToObject(pRoot, name, jsonArray);
	}

	bool ReadJSNode(JSNODE pRoot, const char* name, std::vector<PavementComponentInfo::LayerDatas_*>& value) {
		auto jsonNode = Echo::cJSON_GetObjectItem(pRoot, name);
		if (jsonNode && jsonNode->type == cJSON_Array)
		{
			for (auto& v : value)
				delete v;
			value.clear();

			int count = cJSON_GetArraySize(jsonNode);
			value.reserve(count);

			int i = 0;
			for (cJSON* _child = jsonNode->child; _child != nullptr; _child = _child->next)
			{
				value.push_back(new PavementComponentInfo::LayerDatas_());
				value[i]->ImportJson(_child);
				i++;
			}
			
			return true;
		}
		return false;
	}

	void WriteJSNode(JSNODE pRoot, const char* name, const std::vector<PavementMeshBaseData>& dataVec)
	{
		//暂时先不导出新版数据，只在使用新模式时动态生成新版数据并使用
		//return;

		if (dataVec.empty())
			return;

		cJSON* dataArray = cJSON_CreateArray();
		cJSON_AddItemToObject(pRoot, name, dataArray);

		for (auto& data : dataVec)
		{
			cJSON* dataJson = cJSON_CreateObject();
			cJSON_AddItemToArray(dataArray, dataJson);

			WriteJSNode(dataJson, "GroupName", data.groupName);
			WriteJSNode(dataJson, "MeshName", data.meshName);
			WriteJSNode(dataJson, "PartIndex", data.partIndex);
			WriteJSNode(dataJson, "UpdateType", data.updateType);
			WriteJSNode(dataJson, "PartLength", data.partLength);
			WriteJSNode(dataJson, "SpacingStep", data.m_SpacingStep);
			WriteJSNode(dataJson, "CurveModel", data.m_IsCurveModel);
			WriteJSNode(dataJson, "PierModel", data.m_IsPierModel);
			WriteJSNode(dataJson, "RenderDistance", data.m_RenderDistance);
			WriteJSNode(dataJson, "Lod1Distance", data.m_Lod1Distance);
			WriteJSNode(dataJson, "Lod2Distance", data.m_Lod2Distance);
			WriteJSNode(dataJson, "Offset", data.offset);
			WriteJSNode(dataJson, "Quaternion", data.quat);
			WriteJSNode(dataJson, "Scale", data.scale);
			WriteJSNode(dataJson, "SurfaceType", data.m_surfaceType);
		}
	}

	bool ReadJSNode(JSNODE pRoot, const char* name, std::vector<PavementMeshBaseData>& dataVec)
	{
		cJSON* dataArray = cJSON_GetObjectItem(pRoot, name);
		if (dataArray && dataArray->type == cJSON_Array)
		{
			for (JSNODE dataJson = dataArray->child; dataJson; dataJson = dataJson->next)
			{
				PavementMeshBaseData data;

				ReadJSNode(dataJson, "GroupName", data.groupName);
				ReadJSNode(dataJson, "MeshName", data.meshName);
				ReadJSNode(dataJson, "PartIndex", data.partIndex);
				ReadJSNode(dataJson, "UpdateType", data.updateType);
				ReadJSNode(dataJson, "PartLength", data.partLength);
				ReadJSNode(dataJson, "SpacingStep", data.m_SpacingStep);
				ReadJSNode(dataJson, "CurveModel", data.m_IsCurveModel);
				if (Math::FloatEqual(data.m_SpacingStep, 0.0f))
				{
					data.m_IsCurveModel = true;
				}
				ReadJSNode(dataJson, "PierModel", data.m_IsPierModel);
				ReadJSNode(dataJson, "RenderDistance", data.m_RenderDistance);
				ReadJSNode(dataJson, "Lod1Distance", data.m_Lod1Distance);
				ReadJSNode(dataJson, "Lod2Distance", data.m_Lod2Distance);
				ReadJSNode(dataJson, "Offset", data.offset);
				ReadJSNode(dataJson, "Quaternion", data.quat);
				ReadJSNode(dataJson, "Scale", data.scale);
				ReadJSNode(dataJson, "SurfaceType", data.m_surfaceType);
				
				dataVec.push_back(data);
			}
			
		}
		return true;
	}

	void PavementComponentInfo::LayerDatas_::ExportJson(cJSON* jsonNode) const
	{
		cJSON_AddNumberToObject(jsonNode, "UpdateType", m_UpdateWay);
		cJSON_AddNumberToObject(jsonNode, "MaxPillarSum", m_MaxPillarSum);

		cJSON* array_id = cJSON_CreateArray();
		cJSON_AddItemToObject(jsonNode, "Ids", array_id);
		for (auto& map_v : m_Meshdatas)
			cJSON_AddNumberToObject(array_id, "", map_v->m_PickPlace);
		
		cJSON* array_data = cJSON_CreateArray();
		cJSON_AddItemToObject(jsonNode, "Meshs", array_data);
		for (auto& map_v : m_Meshdatas)
			cJSON_AddStringToObject(array_data, "", map_v->m_MeshName.c_str());

		cJSON* array_change = cJSON_CreateArray();
		cJSON_AddItemToObject(jsonNode, "RoadChanges", array_change);
		for (auto& data : m_Meshdatas)
		{
			WriteJSNode(array_change, "", data->m_OffsetPoint);
			WriteJSNode(array_change, "", data->m_ScalePoint);
			WriteJSNode(array_change, "", data->m_RotatePoint);
		}

		cJSON* array_weight = cJSON_CreateArray();
		cJSON_AddItemToObject(jsonNode, "Weighted", array_weight);
		for (auto& data : m_Meshdatas)
			WriteJSNode(array_weight, "", data->m_Weighted);

		if (m_PillarCount.size() != 0)
		{
			cJSON* array_height = cJSON_CreateArray();
			cJSON_AddItemToObject(jsonNode, "PillarCount", array_height);
			for (auto& heights : m_PillarCount)
			{
				cJSON* array_h = cJSON_CreateArray();
				cJSON_AddItemToArray(array_height, array_h);
				for (auto& height : heights)
					WriteJSNode(array_h, "", height);
			}
		}

		cJSON_AddNumberToObject(jsonNode, "Spacing Step", m_SpacingStep);
		cJSON_AddNumberToObject(jsonNode, "Pick Model", m_PickModel);
		//cJSON_AddNumberToObject(jsonNode, "CullDistance", m_CullDistance);
		cJSON_AddNumberToObject(jsonNode, "RenderDistance", m_RenderDistance);
		cJSON_AddNumberToObject(jsonNode, "PartLength", m_PartLength);
		cJSON_AddNumberToObject(jsonNode, "Lod1Distance", m_Lod1Distance);
		cJSON_AddNumberToObject(jsonNode, "Lod2Distance", m_Lod2Distance);
		cJSON_AddNumberToObject(jsonNode, "Sort Type", m_SortNumber);
	}

	bool PavementComponentInfo::LayerDatas_::OldDataRead(cJSON* jsonNode)
	{
		cJSON* v_data = cJSON_GetObjectItem(jsonNode, "V");
		if (v_data)
		{
			ReadJSNode(v_data, "UpdateType", m_UpdateWay);
			ReadJSNode(v_data, "MaxPillarSum", m_MaxPillarSum);
			ReadJSNode(v_data, "Spacing Step", m_SpacingStep);
			ReadJSNode(v_data, "Pick Model", m_PickModel);
			//ReadJSNode(v_data, "CullDistance", m_CullDistance);
			ReadJSNode(v_data, "RenderDistance", m_RenderDistance);
			ReadJSNode(v_data, "PartLength", m_PartLength);
			ReadJSNode(v_data, "Lod1Distance", m_Lod1Distance);
			ReadJSNode(v_data, "Lod2Distance", m_Lod2Distance);
			ReadJSNode(v_data, "Sort Type", m_SortNumber);

			m_Meshdatas.push_back(new MeshDatas_());
			ReadJSNode(v_data, "SubMesh", m_Meshdatas[0]->m_MeshName);
			ReadJSNode(v_data, "Offset", m_Meshdatas[0]->m_OffsetPoint);
			ReadJSNode(v_data, "Scale", m_Meshdatas[0]->m_ScalePoint);
			ReadJSNode(v_data, "Rotation", m_Meshdatas[0]->m_RotatePoint);
		}

		return true;
	}
 
	void PavementComponentInfo::CopyDataTo(const std::vector<LayerDatas_*>& in_data, std::vector<LayerDatas_*>& layers)
	{
		for (auto& vdata : in_data)
		{
			LayerDatas_* data = new LayerDatas_();

			//data->m_CullDistance = vdata->m_CullDistance;
			data->m_RenderDistance = vdata->m_RenderDistance;
			data->m_PartLength = vdata->m_PartLength;
			data->m_Lod1Distance = vdata->m_Lod1Distance;
			data->m_Lod2Distance = vdata->m_Lod2Distance;
			data->m_SpacingStep	 = vdata->m_SpacingStep;
			data->m_MaxRotation	 = vdata->m_MaxRotation;
			data->m_MinRotation	 = vdata->m_MinRotation;
			data->m_SortNumber	 = vdata->m_SortNumber;
			data->m_MaxPillarSum = vdata->m_MaxPillarSum;
			data->m_PickModel	 = vdata->m_PickModel;
			data->m_NewStoT		 = vdata->m_NewStoT;
			data->m_UpdateWay	 = vdata->m_UpdateWay;

			for (auto& mesh_d : vdata->m_Meshdatas)
			{
				MeshDatas_* mesh = new MeshDatas_();
				mesh->m_MeshName=mesh_d->m_MeshName;
				mesh->m_OffsetPoint = mesh_d->m_OffsetPoint;
				mesh->m_ScalePoint = mesh_d->m_ScalePoint;
				mesh->m_RotatePoint = mesh_d->m_RotatePoint;
				mesh->m_Weighted = mesh_d->m_Weighted;
				mesh->m_PickPlace = mesh_d->m_PickPlace;
				mesh->m_PickType = mesh_d->m_PickType;
				data->m_Meshdatas.push_back(mesh);
			}

			for (auto& VecHeight : vdata->m_PillarCount)
			{
				std::vector<int> Heights;
				for (auto& height : VecHeight)
					Heights.push_back(height);
				data->m_PillarCount.push_back(Heights);
			}

			layers.push_back(data);
		}
	}

	void PavementComponentInfo::SetPillarHeight(size_t layer_index, size_t index, const std::vector<int>& Counts)
	{
		if (mLayerDatas.size() <= layer_index) return;
		auto& pillar_c = mLayerDatas[layer_index]->m_PillarCount;

		while (pillar_c.size() <= index)
		{
			std::vector<int> int_da;
			pillar_c.push_back(int_da);
		}

		pillar_c[index].clear();
		for (size_t i = 0; i < Counts.size(); ++i)
			pillar_c[index].push_back(Counts[i]);
	}

	void PavementComponentInfo::DataToNewModel()
	{

		if (mLayerDatas.empty() || !mPavementData.empty())
			return;

		NameGenerator group("Pavement");
		for (auto& layer : mLayerDatas)
		{
			for (auto& meshData : layer->m_Meshdatas)
			{
				PavementMeshBaseData data;
				data.groupName = Name(group.generate());
				data.meshName = meshData->m_MeshName;
				data.partIndex = meshData->m_PickPlace - 1;

				data.updateType = layer->m_UpdateWay;
				
				data.partLength = layer->m_PartLength;
				data.m_SpacingStep = layer->m_SpacingStep;

				data.m_IsCurveModel = layer->m_SpacingStep == 0.0f;//旧版本间隔生成没有随道路曲线扭曲的功能，所以旧版数据这里都算作不随曲线扭曲
				data.m_IsPierModel = layer->m_MaxPillarSum != 0.0f;

				data.m_RenderDistance = layer->m_RenderDistance;
				data.m_Lod1Distance = layer->m_Lod1Distance;
				data.m_Lod2Distance = layer->m_Lod2Distance;

				data.offset = meshData->m_OffsetPoint;
				data.quat.FromEuler(meshData->m_RotatePoint);
				data.scale = meshData->m_ScalePoint;

				mPavementData.push_back(data);
			}
		}
		//return;
		for (auto& layer : mLayerDatas)
		{
			for (auto& meshData : layer->m_Meshdatas)
			{
				SAFE_DELETE(meshData);
			}
			layer->m_Meshdatas.clear();
			SAFE_DELETE(layer);
		}
		mLayerDatas.clear();

	}


	bool PavementComponentInfo::LayerDatas_::ImportJson(cJSON* jsonNode)
	{
		cJSON* _js_id_array = cJSON_GetObjectItem(jsonNode, "Ids");
		cJSON* _js_mesh = cJSON_GetObjectItem(jsonNode, "Meshs");
		cJSON* _js_change = cJSON_GetObjectItem(jsonNode, "RoadChanges");
		cJSON* _js_weight = cJSON_GetObjectItem(jsonNode, "Weighted");
		cJSON* _js_pillar = cJSON_GetObjectItem(jsonNode, "PillarCount");

		if ((!_js_id_array || _js_id_array->type != cJSON_Array) &&
			(!_js_mesh || _js_mesh->type != cJSON_Array) &&
			(!_js_change || _js_change->type != cJSON_Array) &&
			(!_js_weight || _js_weight->type != cJSON_Array) &&
			(!_js_pillar || _js_pillar->type != cJSON_Array))
		{
			return OldDataRead(jsonNode);
		}

		ReadJSNode(jsonNode, "UpdateType", m_UpdateWay);
		ReadJSNode(jsonNode, "MaxPillarSum", m_MaxPillarSum);
		ReadJSNode(jsonNode, "Spacing Step", m_SpacingStep);
		ReadJSNode(jsonNode, "Pick Model", m_PickModel);
		ReadJSNode(jsonNode, "RenderDistance", m_RenderDistance);
		ReadJSNode(jsonNode, "PartLength", m_PartLength);
		ReadJSNode(jsonNode, "Lod1Distance", m_Lod1Distance);
		ReadJSNode(jsonNode, "Lod2Distance", m_Lod2Distance);
		ReadJSNode(jsonNode, "Sort Type", m_SortNumber);
		
		int count = cJSON_GetArraySize(_js_id_array);
		m_Meshdatas.reserve(count);
		for (int i = 0; i < count; ++i)
			m_Meshdatas.push_back(new MeshDatas_());

		std::vector<int> ids(count, 0);

		int i = 0;
		if(_js_id_array && _js_id_array->child)
		for (cJSON* _child = _js_id_array->child; _child != nullptr; _child = _child->next)
		{
			m_Meshdatas[i]->m_PickPlace = _child->valueint;
			i++;
		}

		i = 0;
		if (_js_mesh && _js_mesh->child)
		for (cJSON* _child = _js_mesh->child; _child != nullptr; _child = _child->next)
		{
			Name meshname;
			meshname = _child->valuestring;
			m_Meshdatas[i]->m_MeshName = meshname;
			i++;
		}

		i = 0;
		for (cJSON* _child = _js_weight->child; _child != nullptr; _child = _child->next)
		{
			m_Meshdatas[i]->m_Weighted = (float)_child->valuedouble;
			i++;
		}

		i = 0;
		if (_js_change && _js_change->child)
		for (cJSON* _child = _js_change->child; _child != nullptr; _child = _child->next)
		{
			std::vector<Vector3> rChange(3, Vector3::ZERO);

			cJSON* value = _child->child;
			m_Meshdatas[i]->m_OffsetPoint.x = (float)value->valuedouble;
			m_Meshdatas[i]->m_OffsetPoint.y = (float)value->next->valuedouble;
			m_Meshdatas[i]->m_OffsetPoint.z = (float)value->next->next->valuedouble;

			_child = _child->next;
			value = _child->child;
			m_Meshdatas[i]->m_ScalePoint.x = (float)value->valuedouble;
			m_Meshdatas[i]->m_ScalePoint.y = (float)value->next->valuedouble;
			m_Meshdatas[i]->m_ScalePoint.z = (float)value->next->next->valuedouble;

			_child = _child->next;
			value = _child->child;
			m_Meshdatas[i]->m_RotatePoint.x = (float)value->valuedouble;
			m_Meshdatas[i]->m_RotatePoint.y = (float)value->next->valuedouble;
			m_Meshdatas[i]->m_RotatePoint.z = (float)value->next->next->valuedouble;

			i++;
		}

		i = 0;
		if(_js_pillar && _js_pillar->child)
		for (cJSON* _child = _js_pillar->child; _child != nullptr; _child = _child->next)
		{
			std::vector<int> pillar_c;
			for (cJSON* _value = _child->child; _value != nullptr; _value = _value->next)
				pillar_c.push_back(_value->valueint);
			m_PillarCount.push_back(pillar_c);
		}

		return true;
	}

	////////////////////////////////////////////////////////////
	// PavementComponentInfo
	EDITOR_REFLECT_BEGIN(PavementComponentInfo)

	EDITOR_REFLECT_MEMBER_UPDATE(mVersion, "Version", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mLayerDatas, "Meshs", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mPavementName, "PavementName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mBindComName, "BindComponent", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(isCreatePhysics, "IsCreatePhysics", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)

	EDITOR_REFLECT_MEMBER_UPDATE(mPavementData, "PavementData", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(isFitGround, "FitGroundModel", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mVegetationCollection, "VegetationCollection", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)

	EDITOR_REFLECT_END();

	PavementComponentInfo::PavementComponentInfo(
		const Name& type, ActorCOMMoveableType moveable)
		: ActorComponentInfo(type, moveable)
	{
	}

	PavementComponentInfo::~PavementComponentInfo()
	{
		for (auto& v : mLayerDatas)
			if (!v->m_Meshdatas.empty())
				delete v;
		mLayerDatas.clear();
	}

	void PavementComponentInfo::ExportData(cJSON* jsonNode)
	{
		// 消除Info内部的空数据
		for (auto iter = mLayerDatas.begin(); iter != mLayerDatas.end();)
		{
			if ((*iter)->m_Meshdatas.empty())
			{
				delete (*iter);
				iter = mLayerDatas.erase(iter);
			}
			else
			{
				for (auto mesh_iter = (*iter)->m_Meshdatas.begin(); mesh_iter != (*iter)->m_Meshdatas.end();)
				{
					if ((*mesh_iter)->m_MeshName.empty())
					{
						delete (*mesh_iter);
						mesh_iter = (*iter)->m_Meshdatas.erase(mesh_iter);
					}
					else
						mesh_iter++;
				}

				if ((*iter)->m_Meshdatas.empty())
				{
					delete (*iter);
					iter = mLayerDatas.erase(iter);
				}
				else
					iter++;
			}
		}

		reflect::TypeResolver<PavementComponentInfo>::get()->ExportJson(this, jsonNode, "");
	}

	void PavementComponentInfo::ImportData(cJSON* jsonNode)
	{
		reflect::TypeResolver<PavementComponentInfo>::get()->ImportJson(this, jsonNode, "");

		DataToNewModel();
	}

	void PavementComponentInfo::CopyTo(ActorComponentInfo* info)
	{
		if (info == nullptr)
			return;

		DataToNewModel();

		PavementComponentInfo* pInfo = static_cast<PavementComponentInfo*> (info);
		reflect::TypeResolver<PavementComponentInfo>::get()->copyTo(this, pInfo);

		

		/*pInfo->mLayerDatas.clear();
		CopyDataTo(mLayerDatas, pInfo->mLayerDatas);*/

	}

	void PavementComponentInfo::OverWrite()
	{
		if (mOriginComInfo != nullptr)
		{
			PavementComponentInfo* m_pInfo = static_cast<PavementComponentInfo*>(mOriginComInfo);
			reflect::TypeResolver<PavementComponentInfo>::get()->overWrite(m_pInfo, this);

			/*m_pInfo->mLayerDatas.clear();
			CopyDataTo(mLayerDatas, m_pInfo->mLayerDatas);*/

			DataToNewModel();
		}
	}
#ifdef ECHO_EDITOR
	bool PavementComponentInfo::getNeedCollectedResource(std::unordered_multimap<ResourceType, std::string>& resMap)
	{
		/*if (mLayerDatas.empty()) {
			return false;
		}s
		for (auto& data : mLayerDatas)
		{
			if (data->m_Meshdatas.empty())
				continue;
			for (auto& iter : data->m_Meshdatas)
			{
				if (!iter->m_MeshName.empty())
					resMap.insert(std::make_pair(ResourceType::Mesh, iter->m_MeshName.toString()));
			}
		}*/
		if (mPavementData.empty())
			return false;
		for (auto& data : mPavementData)
		{
			if (data.meshName.empty())
				continue;
			resMap.insert(std::make_pair(ResourceType::Mesh, data.meshName.toString()));
		}

		return true;
	}
#endif
	///////////////////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////////////////
	// PavementComponent
	DECLAREACTORCOMTYPE_CPP(PavementComponent)
		PavementComponent::PavementComponent(ActorPtr _actor,
			ActorCOMMoveableType movableType /*= ALL_MOVEABLE*/)
		: NodeActorComponent(_actor, movableType)
	{
		RoadManager::instance()->SetPavementVec(this);
		//mDefaultFitTerrain = new PavementFitPlanetTerrain(weak_from_this());
	}

	PavementComponent::~PavementComponent()
	{
		if (!mBezier.expired())
			mBezier->clearRoadIns();
		RoadHelper::instance()->setComVaild(mNamePath);
		RoadManager::instance()->SetPavementVec(this);
		//SAFE_DELETE(mDefaultFitTerrain);
	}

	void PavementComponent::init(ActorComponentInfo* info)
	{
		if (info)
		{
			stringstream ss;
			ss << this;
			mNamePath = ss.str();
			RoadHelper::instance()->setComVaild(mNamePath, true, this);
		}
	}

	void PavementComponent::unInit()
	{
		clearAllPavements();
	}

	void PavementComponent::initBezier(ActorComponentPtr mbez)
	{
		if (!m_pActComInfo) return;

		if (mbez == nullptr || !mbez->m_pActComInfo)
		{
			mBezier = nullptr;
			//mBeCurve = nullptr;
			return;
		}
		if (mbez->getType() == Name("PiecewiseBezierComponent"))
		{
			mBezier = mbez.getStaticCastWeakPtr<PiecewiseBezierComponent>();
			for (int i = 0; i < mBezier->getPointCount(); ++i)
				mPickAngles.push_back(mBezier->getRotateAngle(i));
		}
	}

	bool PavementComponent::initPavementUtil(size_t num, size_t InfoNum)
	{
		std::vector<MeshPtr> meshs;
		std::vector<PavementTool::LodMeshDataCache> datas;
		PavementComponentInfo* pInfo = GetPavementInfo();

		try { //并不关注SubData数据是否存在，只考虑是否加载过meshptr
			//auto& map_d = RoadManager::instance()->m_mapPathMesh;
			if (pInfo->mLayerDatas.size() > InfoNum && !(Root::instance()->UsePavementTestNewMode && pInfo->mLayerDatas[InfoNum]->m_SpacingStep == 0.0f))
			{
				bool needBreak = false;
				auto& LayerData = pInfo->mLayerDatas[InfoNum];
				bool needSubData = false;

				for (auto& MeshData : LayerData->m_Meshdatas)
				{
					std::shared_ptr<PavementTool::PrivateMeshData> it_data;
					if (RoadManager::instance()->GetPathMeshData(MeshData->m_MeshName, it_data, needSubData))
					{
						PavementTool::LodMeshDataCache data;
						meshs.push_back(it_data->mesh);
						data = it_data->SubData;
						datas.push_back(data);
					}
					else
					{
						if (Root::instance()->getRuntimeMode() == Root::eRM_Client)
						{
							bool needData = true;
							if (LayerData->m_SpacingStep != 0.f) needData = false;
							RoadHelper::instance()->addMeshRequest(mNamePath, MeshData->m_MeshName, num, InfoNum, needData);
							needBreak = true;
						}
						else
						{
							PavementTool::LodMeshDataCache data;
							MeshPtr mesh = Echo::MeshMgr::instance()->load(MeshData->m_MeshName);
							meshs.push_back(mesh);
							datas.push_back(data);
						}
					}
				}
				if (needBreak) return true;
			}
		}
		catch (...)
		{
			auto& LayerData = pInfo->mLayerDatas[InfoNum];
			for (auto& MeshData : LayerData->m_Meshdatas)
			{
				String v = MeshData->m_MeshName.toString();
				if (v.empty()) return false;
				size_t index = v.rfind('/');
				String name = String(std::next(v.begin(), index + 1), v.end());
				String path = String(v.begin(), std::next(v.begin(), index));
				mErrorMeshs.push_back(Name(name));
				mErrorMeshs.push_back(Name(path));
			}
			delete pInfo->mLayerDatas[InfoNum];
			pInfo->mLayerDatas.erase(pInfo->mLayerDatas.begin() + InfoNum);
			return false;
		}

		LoadedMeshAndUpdate(meshs, num, InfoNum, datas);

		return true;
	}

	void PavementComponent::tryInitPavementUtil(size_t num, size_t InfoNum)
	{
		bool needInit = true;
		PavementComponentInfo* pInfo = GetPavementInfo();
		if (pInfo == nullptr) return;
		if (pInfo->mLayerDatas.size() > InfoNum)
		{
			//auto& map_d = RoadManager::instance()->m_mapPathMesh;
			auto& LayerData = pInfo->mLayerDatas[InfoNum];
			bool needSubData = LayerData->m_SpacingStep == 0;
			for (auto& MeshData : LayerData->m_Meshdatas)
			{
				std::shared_ptr<PavementTool::PrivateMeshData> it_data;
				if (!RoadManager::instance()->GetPathMeshData(MeshData->m_MeshName, it_data, needSubData))
				{
					needInit = false;
					break;
				}
			}
			if (needInit)
			{
				initPavementUtil(num, InfoNum);
			}
		}
	}

	void PavementComponent::copyTo(const ActorComponentPtr& actCom)
	{
		PavementComponentPtr pavement = actCom.getStaticCastWeakPtr<PavementComponent>();
		pavement->mNoReBuild = this->mNoReBuild;

	}

	void PavementComponent::createComRootNodeImpl()
	{
		mComRootNode = getSceneManager()->createSceneNode();
	}

	Name PavementComponent::getErrorInfo(size_t i)
	{
		if (!m_pActComInfo) return Name();
		if (mErrorMeshs.size() > i + 1)
			return mErrorMeshs[i] + Name(" in ") + mErrorMeshs[i + 1];
		else return Name("");
	}

	void PavementComponent::delErrorInfo(size_t i)
	{
		if (!m_pActComInfo) return;
		if (mErrorMeshs.size() > i)
			mErrorMeshs.erase(std::next(mErrorMeshs.begin(), i));
		if (mErrorMeshs.size() > i)
			mErrorMeshs.erase(std::next(mErrorMeshs.begin(), i));
	}

	int PavementComponent::findErrorIndex(Name value)
	{
		if (!m_pActComInfo) return 0;
		int result = -1;
		const auto& iter = std::find(mErrorMeshs.begin(), mErrorMeshs.end(), value);
		if (iter != mErrorMeshs.end())
			result = int(iter - mErrorMeshs.begin());
		return result;
	}

	int PavementComponent::getPartNumber(size_t part_index)
	{
		if (!m_pActComInfo) return 0;
		if (!mBezier.expired())
		{
			float length = mBezier->getPiecewiseLength(part_index, false);
			float roadlength = mRoadUtil[GetPavementInfo()->RoadIndex]->GetRealLength(part_index);
			float result = length / roadlength;
			if (result - float(int(result)) > 0.5)
				result++;
			return int(result);
		}
		return 0;
	}

	int PavementComponent::getSegmentCount()
	{
		if (!m_pActComInfo) return 0;
		if (!mBezier.expired())
			return (int)mBezier->getPointCount() - 1;
		else
			return 0;
	}

	void PavementComponent::setDefaultSegment()
	{
		mRealCreateNumber = -1;
		mRealCreateSegment = -1;

		mNowCreateNumber = -1;
		mNowCreateSegment = -1;

		mRealCreateLength = 0.f;

		for (auto& v : mPhyCreateNumberInPart)
			v = -1;
	}

	void PavementComponent::AddAngleInIndex(size_t index, float value)
	{
		mPickAngles.insert(std::next(mPickAngles.begin(), index), value);
	}

	void PavementComponent::insertAngle(size_t index, float value)
	{
		while (index >= mPickAngles.size())
			mPickAngles.push_back(0.f);
		mPickAngles[index] = value;
	}

	void PavementComponent::changeAngle(std::vector<float>& vec)
	{
		mPickAngles.clear();
		mPickAngles.insert(mPickAngles.begin(), vec.begin(), vec.end());
	}


	size_t PavementComponent::getInfoDataCount() 
	{ 
		if (m_pActComInfo)
			return GetPavementInfo()->mLayerDatas.size();
		else
			return 0;
	}
	String PavementComponent::getBindComName() 
	{ 
		if (m_pActComInfo)
			return GetPavementInfo()->getBindComName();
		else
			return "";
	}

	Name PavementComponent::PavementName(size_t meshNum) 
	{ 
		if (m_pActComInfo)
			return GetPavementInfo()->mPavementName;
		else
			return Name();
	}
	//--------------------------------------------------------------------------------------------

	void PavementComponent::frameWork() 
	{
		if (isWaitDel)
			clearAllPavements();
		//if (m_pActComInfo && !GetPavementInfo()->getBindComName().empty() && !Root::instance()->UsePavementTestNewMode)
		//{
		//	for (size_t num = 0; num < mRoadUtil.size() && mBezier != nullptr; ++num)
		//		if (mRoadUtil[num]->GetBezierUtil())
		//			updateModelPhy(num, this->getWorldPosition());
		//}
	};

	void PavementComponent::needUpdateBezier()
	{
		// 在调用connectCom接口时候发现，此接口会多次调用，防止多次更新带来的性能开销而加上的
		if (!connectFirst) connectFirst = true;
	}

	/*
		在道路计算法线为第0套操作时候，每次移动控制点会导致此控制点之后的所有的有关法线的计算都失效
		因此，需要判断当前道路中是否有需要第0套计算法线的方案，有则默认更新道路所有段落。
	*/
	bool PavementComponent::needUpdateAll()
	{
		if (!m_pActComInfo) return false;
		bool result = false;
		for (auto& data : GetPavementInfo()->mLayerDatas)
			result |= (data->m_UpdateWay == 0);

		return result;
	}

	void PavementComponent::bindActor()
	{
		return;

		/*if (!m_pActComInfo) return;
		if (!connectFirst) return;

		PavementComponentInfo* pInfo = GetPavementInfo();
		if (!GetPavementInfo()->getBindComName().empty())
		{
			for (const auto& _com : getActor()->getComponents())
			{
				if (_com.second->getComName() == pInfo->getBindComName())
				{
					initBezier(_com.second);
					bezierAddRoadIns();
					break;
				}
			}

			if (mBezier == nullptr)
			{
				LogManager::instance()->logMessage(LML_TRIVIAL, "PavementComponent Binding Failed ");
				return;
			}

			for (int i = 0; i < pInfo->mLayerDatas.size(); ++i)
			{
				if (!initPavementUtil((size_t)i, (size_t)i))
				{ 
					i-- ;
				}
			}
			connectFirst = false;
		}*/
	}

	void PavementComponent::connectCom()
	{
		if (!m_pActComInfo) return;
		if (!connectFirst) return;

		PavementComponentInfo* pInfo = GetPavementInfo();
		if (!pInfo->getBindComName().empty())
		{
			PiecewiseBezierComponentPtr bezier = getActor()->GetComponent<PiecewiseBezierComponent>(pInfo->getBindComName());
			if (bezier != nullptr)
			{
				mBezier = bezier;
				mBezier->addRoadIns(weak_from_this());
				for (int i = 0; i < mBezier->getPointCount(); ++i)
					mPickAngles.push_back(mBezier->getRotateAngle(i));
			}
			else
			{
				LogManager::instance()->logMessage(LML_TRIVIAL, "PavementComponent Binding Failed ");
				return;
			}

			if (!Root::instance()->UsePavementTestNewMode)
			{
				for (int i = 0; i < pInfo->mLayerDatas.size(); ++i)
				{
					if (!initPavementUtil((size_t)i, (size_t)i))
					{
						i--;
					}
				}
			}
			if (Root::instance()->UsePavementTestNewMode)//else
			{
				fitGround = pInfo->isFitGround;
				mVegetationCollection = pInfo->mVegetationCollection;

				mPavements.clear();
				for (auto& data : pInfo->mPavementData)
				{
					createPavement(data, pInfo->isCreatePhysics ? UpdateType::UpdateAll : UpdateType::UpdateRender, getActor()->isSyncLoadActor());
				}
				//updateAllPavement(pInfo->isCreatePhysics ? UpdateType::UpdateAll : UpdateType::UpdateRender, getActor()->isSyncLoadActor());
			}

			connectFirst = false;
		}
	}

	AxisAlignedBox PavementComponent::getLoadBoundingBox()
	{
		/*if (m_pActComInfo == nullptr)
		{
			return ActorComponent::getLoadBoundingBox();
		}*/

		float d = 0.0f;
		for (auto& [id, data] : mPavements)
		{
			d = Math::Max(data.first.m_RenderDistance, d);
		}
		if (d == 0.0f) d = 1000.0f;
		const Vector3& minBox = getWorldAABB().getMinimum();
		const Vector3& maxBox = getWorldAABB().getMaximum();
		AxisAlignedBox loadBox(minBox - d, maxBox + d);
		return loadBox;
	}

	//--------------------------------------------------------------------------------------------

	void PavementComponent::update(size_t num)
	{
		if (!m_pActComInfo) return;
		if (mBezier == nullptr || !mNeedUpdate)
			return;
		
		for (size_t mesh_num = 0; mesh_num < mRoadUtil.size(); ++mesh_num)
		{
			if (num != -1) mesh_num = num;
			PavementModelUtil* pTool = mRoadUtil[mesh_num];
			pTool->DefaultAllObjects(Vector3::ZERO);
			
			// 预计算所有控制点法线信息
			if (pTool->GetUpdateWay() == 0)
				mBezier->getBezierTool()->CalculateControlNormal();

			mBezier->setActionType(PiecewiseTool::CREATE_ACTION);
			if (mBezier->getPointCount() > 0)
			{
				for (size_t pick_index = 0; pick_index < mBezier->getPointCount() - 1; ++pick_index)
				{
					updatePiecewiseRoad(pick_index, mesh_num);
				}
			}
			if (num != -1) break;
		}

		for (size_t i = 0; i < mMeshAddScene.size(); ++i)
		{
			if (num != -1) i = num;
			if(mMeshAddScene[i])
			{
				addChildToNode(mRoadUtil[i]->GetSceneNode());
				mMeshAddScene[i] = false;
				UpdateComRootNode(true, true);
			}
			if (num != -1) break;
		}

		for (auto& m : mRoadUtil)
			for (auto& mesh : m->GetMeshDatas())
				RoadManager::instance()->SetWeightZero(mesh->mMeshData->only_mesh->mesh->getName());
	}

	void PavementComponent::updatePiecewiseRoad(size_t pick_index, size_t mesh_num)
	{
		if (!m_pActComInfo) return;

		Echo::ProtocolComponentSystem::instance()->callProtocolFunc(Echo::PFID_EditorComponent_ChangeProperty, nullptr, 0);
		PavementComponentInfo* pInfo = GetPavementInfo();

		auto UpdataFunc = [&](size_t i, size_t pick_index, PavementModelUtil* pTool, float step)->void
		{
			if (pInfo->mLayerDatas[i]->m_PillarCount.size() > pick_index)
				pTool->InitIndexPillarHeight(pick_index, pInfo->mLayerDatas[i]->m_PillarCount[pick_index]);
			addRequestModelStep(i, pick_index, step, this->getWorldPosition());
		};

		if (!mNeedUpdate) return;
		if (mesh_num != -1 && mesh_num >= mRoadUtil.size()) return;
		for (size_t i = 0; i < mRoadUtil.size(); ++i)
		{
			if (mesh_num != -1) i = mesh_num;
			PavementModelUtil* pTool = mRoadUtil[i];
			if (!pTool->MeshDataNotNull())
			{
				//initPavementUtil(i, i);
				if (mesh_num != -1) break;
				continue;
			}
			
			pTool->InitRotateAngle(mPickAngles);
			pTool->SetWorldValue(this->getWorldScale(), this->getWorldRotation());

			PiecewiseTool::PiecewiseBezier pick_place = mBezier->getBezierTool()->CalculatePlace(pick_index);

			std::pair<float, float> real_step = pTool->UpdateMoveObject(pick_index, mBezier->getActionType());

			if (pInfo->mLayerDatas[i]->m_SpacingStep == 0.f)
			{
				if (pTool->GetModeMoveCount() != 0)
				{
					if(!Root::instance()->UsePavementTestNewMode)
					{
						if (pick_place & PiecewiseTool::Begin)
						{
							if (Root::instance()->getRuntimeMode() == Root::eRM_Client)
								addRequestModelPick(i, pick_index, this->getWorldPosition());
							else
								pTool->UpdatePickModelPart(pick_index, this->getWorldPosition());
						}
						else
						{
							if (Root::instance()->getRuntimeMode() == Root::eRM_Client)
								addRequestModelPick(i, pick_index, this->getWorldPosition());
							else
								pTool->UpdatePickModelPart(pick_index - 1, this->getWorldPosition());

							if (!(pick_place & PiecewiseTool::End) &&
								!(mBezier->getActionType() & PiecewiseTool::DELETE_ACTION))
							{
								if (Root::instance()->getRuntimeMode() == Root::eRM_Client)
								{
									//addRequestModelPick(i, pick_index, this->getWorldPosition());
								}
								else
									pTool->UpdatePickModelPart(pick_index, this->getWorldPosition());
							}
						}
					}
					else
					{
						//updatePavement(pick_index, i, UpdateType::UpdateRender, getActor()->isSyncLoadActor());
					}
				}
			}
			else
			{
				if (pTool->GetSceneNodeCount() != 0)
				{
					if (real_step.first != 0 && pick_place & PiecewiseTool::Begin)
					{
						if (Root::instance()->getRuntimeMode() == Root::eRM_Client)
							UpdataFunc(i, pick_index, pTool, real_step.first);
						else
						{
							pTool->BatchRendering(int(pick_index), real_step.first, this->getWorldPosition());
							if (pInfo->mLayerDatas[i]->m_MaxPillarSum != 0)
								pInfo->SetPillarHeight(i, pick_index, pTool->GetPillarCount()[pick_index]);
						}
					}
					else
					{
						if (real_step.second != 0)
						{
							if (Root::instance()->getRuntimeMode() == Root::eRM_Client)
								UpdataFunc(i, pick_index - 1, pTool, real_step.second);
							else
							{
								pTool->BatchRendering(int(pick_index - 1), real_step.second, this->getWorldPosition());
								if (pInfo->mLayerDatas[i]->m_MaxPillarSum != 0)
									pInfo->SetPillarHeight(i, pick_index - 1, pTool->GetPillarCount()[pick_index - 1]);
							}
						}
						if (!(pick_place & PiecewiseTool::End) && real_step.first != 0)
						{
							if (Root::instance()->getRuntimeMode() == Root::eRM_Client)
								UpdataFunc(i, pick_index, pTool, real_step.first);
							else
							{
								pTool->BatchRendering(int(pick_index), real_step.first, this->getWorldPosition());
								if (pInfo->mLayerDatas[i]->m_MaxPillarSum != 0)
									pInfo->SetPillarHeight(i, pick_index, pTool->GetPillarCount()[pick_index]);
							}
						}
					}
				}
			}
			
			if (mesh_num != -1) break;
		}
	}

	void PavementComponent::updateRealTime(int pick_index, int pick_num)
	{
		if (!m_pActComInfo) return;
		if (!mNeedUpdate) return ;

		PavementComponentInfo* pInfo = GetPavementInfo();

		if (pick_index == 0 && pick_num == 0)
		{
			for (auto& road : mRoadUtil)
				road->DefaultAllObjects(Vector3::ZERO);
			mRealCreateSegment = mNowCreateSegment = -1;
			mRealCreateNumber = mNowCreateNumber = -1;
		}

		if (int(pick_index) >= mBezier->getPointCount() - 1) return ;

		float reallength = mRoadUtil[pInfo->RoadIndex]->GetRealLength(pick_index);

		if (pick_index > mRealCreateSegment || (pick_index == mRealCreateSegment && pick_num > mRealCreateNumber))
		{
			for (size_t i = 0; i < mRoadUtil.size(); ++i)
			{
				if (pick_index > mRealCreateSegment && pick_num == 0)
					mPhyCreateNumberInPart[i] = 0;

				PavementModelUtil* pTool = mRoadUtil[i];
				float real_step = pTool->GetRealStep(pick_index);

				if (pTool->GetMeshDataSize() == 0) return ;

				if (pick_num == 0)
				{
					pTool->InitRotateAngle(mPickAngles);
					real_step = pTool->UpdateIndexObject(pick_index);
				}

				if (pInfo->mLayerDatas[i]->m_SpacingStep == 0.f)
				{
					if (!Root::instance()->UsePavementTestNewMode)
					{
						if (pTool->GetModeMoveCount() != 0)
						{
							pTool->UpdatePickModelPart(pick_index, this->getWorldPosition(), pick_num);
							mRealCreateLength = (pick_num + 1) * reallength;
						}
					}
					else
					{
						//updatePavement(pick_index, i, UpdateType::UpdateRender, true);
					}
				}
				else
				{
					int createNumber = int(mRealCreateLength / pInfo->mLayerDatas[i]->m_SpacingStep) + 1;
					if (pTool->GetSceneNodeCount() != 0)
					{
						for (int number = mPhyCreateNumberInPart[i] + 1; number < createNumber; ++number)
							pTool->BatchRendering(pick_index, real_step, this->getWorldPosition(), number);
						mPhyCreateNumberInPart[i] = createNumber - 1;
					}
				}
			}

			mRealCreateSegment = mNowCreateSegment = pick_index;
			mRealCreateNumber = mNowCreateNumber = pick_num;
		}
		/*else
		{
			for (size_t i = 0; i < mRoadUtil.size(); ++i)
			{
				PavementModelUtil* pTool = mRoadUtil[i];
				if (pTool->GetMeshDataSize() == 0) return;
				float real_step = pTool->GetRealStep(pick_index);

				if (pInfo->mLayerDatas[i]->m_SpacingStep == 0.f)
				{
					if (pTool->GetModeMoveCount() != 0)
					{
						pTool->ResizeRealTimeModeSize(pick_index, pick_num);
					}
				}
				else
				{
					int num = int(reallength * (pick_num + 1) / pInfo->mLayerDatas[i]->m_SpacingStep) - 1;
					pTool->ResizeSegmentModeSize(pick_index, num, real_step, this->getWorldPosition());
				}
			}

			mNowCreateSegment = pick_index;
			mNowCreateNumber = pick_num;
		}*/

		for (auto& phymodel : mRoadPhyUtil)
			phymodel->DeleteAllModelPhy();
	}

	void PavementComponent::updateModelPhy(size_t num, const Vector3& offset)
	{
		if (!m_pActComInfo) return;
		if (num >= mRoadPhyUtil.size()) return;
		PavementPhyUtil* ppTool = mRoadPhyUtil[num];

		if (mBezier->getEditing())
		{
			for (size_t phy_index = 0; phy_index < ppTool->GetPhyModelCount(); ++phy_index)
				ppTool->DeleteModelPhy(phy_index);
		}
		else
		{
			Camera* MainCamera = getSceneManager()->getMainCamera();
			AxisAlignedBox RoleBox;
			Vector3 RolePos = MainCamera->getDerivedPosition();
			DBMatrix4 matrix = this->getWorldMatrix().inverse();
			RolePos = matrix * RolePos + offset;
			RoleBox.merge(RolePos - 50.f);
			RoleBox.merge(RolePos + 50.f);

			if (!ppTool->DecideDeletePhyMode(RoleBox, offset))
				ppTool->DecideCreatePhyMode(RoleBox, offset);
		}
	}

	void PavementComponent::LoadedMeshAndUpdate(std::vector<MeshPtr>& meshs, size_t num, size_t InfoNum, const std::vector<PavementTool::LodMeshDataCache>& datas)
	{
		if (!m_pActComInfo) return;
		PavementComponentInfo* pInfo = GetPavementInfo();

		if (pInfo->mLayerDatas.size() <= InfoNum) return;

		while (num >= mRoadUtil.size())
		{
			addRoadUtilSize();
		}
		
		PavementModelUtil* roadtool = mRoadUtil[num];
		PavementPhyUtil* roadphytool = mRoadPhyUtil[num];

		auto& LayerData = pInfo->mLayerDatas[InfoNum];
		roadtool->InitBezierUtil(mBezier->getBezierTool());
		roadtool->InitStepAndSumType(LayerData->m_SpacingStep,
			LayerData->m_MaxPillarSum, LayerData->m_UpdateWay);
		roadtool->InitRotateAngle(mPickAngles);
		roadtool->ClearMeshDatas();
		roadtool->SetPickModel(LayerData->m_PickModel);
		roadtool->setRenderDistance(LayerData->m_RenderDistance);
		roadtool->setLodDistance(LOD_LEVEL_1, LayerData->m_Lod1Distance);
		roadtool->setLodDistance(LOD_LEVEL_2, LayerData->m_Lod2Distance);

		for (size_t i = 0; i < meshs.size(); ++i)
		{
			std::vector<Vector3> Vec3Data;
			Vec3Data.push_back(LayerData->m_Meshdatas[i]->m_OffsetPoint);
			Vec3Data.push_back(LayerData->m_Meshdatas[i]->m_ScalePoint);
			Vec3Data.push_back(LayerData->m_Meshdatas[i]->m_RotatePoint);
			roadtool->AddChangeValue(Vec3Data);
			roadtool->SetWeightAndPlace(i,
				LayerData->m_Meshdatas[i]->m_Weighted,
				LayerData->m_Meshdatas[i]->m_PickPlace);
			// 考虑是否需要SubData，以及存入的meshptr是否存在SubData
			if (datas.size() > i && !datas[i].empty())
				roadtool->InitMeshPtr(i, meshs[i], datas[i]);
			else
			{
				PavementTool::LodMeshDataCache data;
				roadtool->InitMeshPtr(i, meshs[i], data);
			}
		}

		roadphytool->InitComponentPtr(weak_from_this());
		roadphytool->InitPavementModelUtil(roadtool);

		//if (!Root::instance()->UsePavementTestNewMode)
			update(num);
		//else
		//{
		//	updateLayerPavement(num, UpdateType::UpdateRender, getActor()->isSyncLoadActor());
		//}
	}

	//--------------------------------------------------------------------------------------------

	void PavementComponent::bezierAddRoadIns()
	{
		if (mBezier != nullptr)
			mBezier->addRoadIns(weak_from_this());
		/*if (mBeCurve != nullptr)
			mBeCurve->addRoadIns(weak_from_this());*/
	}

	void PavementComponent::bezierCleaRoadIns()
	{
		if (!m_pActComInfo) return;
		if (mBezier != nullptr)
			mBezier->clearRoadIns(weak_from_this());
		GetPavementInfo()->setBindComName("");
		/*if (mBeCurve != nullptr)
			mBeCurve->clearRoadIns(weak_from_this());*/
	}

	void PavementComponent::deleteRoadDataUtil(size_t num)
	{
		if (!m_pActComInfo) return;
		if (mRoadUtil.size() <= num) return;
		mRoadUtil[num]->RemoveAllNode();
		RemoveChildFromNode(mRoadUtil[num]->GetSceneNode());
		
		delete mRoadUtil[num];
		delete mRoadPhyUtil[num];

		mRoadUtil.erase(std::next(mRoadUtil.begin(), num));
		mRoadPhyUtil.erase(std::next(mRoadPhyUtil.begin(), num));
		mRoadVisible.erase(std::next(mRoadVisible.begin(), num));
		mNoReBuild.erase(std::next(mNoReBuild.begin(), num));
		mMeshAddScene.erase(std::next(mMeshAddScene.begin(), num));
		mPhyCreateNumberInPart.erase(std::next(mPhyCreateNumberInPart.begin(), num));
		/*if(Root::instance()->UsePavementTestNewMode)
		{
			for (auto& pavement : mPavements[num])
			{
				SAFE_DELETE(pavement);
			}
			mPavements.erase(std::next(mPavements.begin(), num));
		}*/
		UpdateComRootNode(true);
	}

	void PavementComponent::deleteRoadModel(size_t num)
	{
		if (!m_pActComInfo) return;
		for (size_t phy_index = 0; phy_index < mRoadPhyUtil[num]->GetPhyModelCount(); ++phy_index)
			mRoadPhyUtil[num]->DeleteModelPhy(phy_index);

		deleteRoadDataUtil(num);
	}

	void PavementComponent::removeAllRoadModel()
	{
		if (!m_pActComInfo) return;
		while (mRoadUtil.size() > 0)
			deleteRoadModel(mRoadUtil.size() - 1);
	}

	size_t PavementComponent::addRoadUtilSize()
	{
		if (!m_pActComInfo) return 0;
		size_t utilsize = mRoadUtil.size();
		mRoadUtil.push_back(new PavementModelUtil(nullptr, getSceneManager()));
		mRoadPhyUtil.push_back(new PavementPhyUtil());
		mRoadVisible.push_back(false);
		mMeshAddScene.push_back(true);
		mPhyCreateNumberInPart.push_back(-1);
		if (mNoReBuild.size() < mRoadUtil.size())
			mNoReBuild.push_back(true);
		//if (Root::instance()->UsePavementTestNewMode)
		//	mPavements.resize(mPavements.size() + 1);
		return utilsize;
	}

	void PavementComponent::addRequestModelPick(size_t index, size_t pick_index, const Vector3& offset)
	{
		if (!m_pActComInfo) return;
		PavementModelUtil* pTool = mRoadUtil[index];
		PiecewiseBezierUtil* BezierUtil = pTool->GetBezierUtil();

		std::shared_ptr<PavementTool::PavementUtilMesh> MeshData = nullptr;
		auto& m_mapData = RoadManager::instance()->m_mapUtilKeyData;
		if (m_mapData.find(pTool->getPathName()) != m_mapData.end())
			MeshData = FindPlaceUtilMeshData(pick_index, *(m_mapData[pTool->getPathName()]));
		else return;

		PavementTool::AsyncModelData aMData;
		aMData.from = BezierUtil->GetPoint(pick_index);
		aMData.from_des = BezierUtil->GetDestination(pick_index);
		aMData.to = BezierUtil->GetPoint(pick_index + 1);
		aMData.to_sou = BezierUtil->GetSource(pick_index + 1);
		aMData.from_angle = pTool->GetAngle(pick_index);
		aMData.to_angle = pTool->GetAngle(pick_index + 1);

		Vector3 point, tangent, normal;
		if (GetPavementInfo()->mLayerDatas[index]->m_UpdateWay == 0)
			BezierUtil->GetBfData(pick_index, point, tangent, normal);

		PavementTool::CalcualteNormalData_ cnor;
		cnor.bf_point = point;
		cnor.bf_tangent = tangent;
		cnor.bf_normal = normal;
		cnor.normal1 = BezierUtil->GetNormal(pick_index);
		cnor.normal2 = BezierUtil->GetNormal(pick_index + 1);

		auto id = RoadHelper::instance()->addCalculatePointReq(pTool->getPathName(), MeshData, pick_index, offset
			, pTool->GetUpdateWay(), BezierUtil->GetPartLength(pick_index), aMData, cnor, getSceneManager());

		pTool->AddRequests(id, pick_index, false);
	}

	// 会开启读取info内已经存储好的向下沿伸路墩高度数据的接口
	void PavementComponent::addRequestModelStep(size_t index, size_t pick_index, float step, const Vector3& offset)
	{
		if (!m_pActComInfo) return;
		PavementModelUtil* pTool = mRoadUtil[index];
		PiecewiseBezierUtil* BezierUtil = pTool->GetBezierUtil();

		std::shared_ptr<PavementTool::PavementUtilMesh> MeshData = nullptr;
		auto& m_mapData = RoadManager::instance()->m_mapUtilKeyData;
		if (m_mapData.find(pTool->getPathName()) != m_mapData.end())
			MeshData = FindPlaceUtilMeshData(pick_index, *(m_mapData[pTool->getPathName()]));
		else return;

		PavementTool::AsyncModelData aMData;
		aMData.from = BezierUtil->GetPoint(pick_index);
		aMData.from_des = BezierUtil->GetDestination(pick_index);
		aMData.to = BezierUtil->GetPoint(pick_index + 1);
		aMData.to_sou = BezierUtil->GetSource(pick_index + 1);
		aMData.from_angle = pTool->GetAngle(pick_index);
		aMData.to_angle = pTool->GetAngle(pick_index + 1);
		//aMData.mComName = mBezier->getCombinedName();

		aMData.mWorldOri = this->getWorldRotation();
		aMData.mWorldPosition = this->getWorldPosition();
		aMData.mWorldScale = this->getWorldScale();

		Vector3 point, tangent, normal;
		if (GetPavementInfo()->mLayerDatas[index]->m_UpdateWay == 0)
			BezierUtil->GetBfData(pick_index, point, tangent, normal);

		PavementTool::CalcualteNormalData_ cnor;
		cnor.bf_point = point;
		cnor.bf_tangent = tangent;
		cnor.bf_normal = normal;
		cnor.normal1 = BezierUtil->GetNormal(pick_index);
		cnor.normal2 = BezierUtil->GetNormal(pick_index + 1);

		auto id = RoadHelper::instance()->addCalculateStepReq(pTool->getPathName(), MeshData, pick_index, BezierUtil->GetPartLength(pick_index),
			step, pTool->GetUpdateWay(), pTool->GetPickModel(), pTool->GetCreateLength(), aMData, cnor, getSceneManager());

		pTool->AddRequests(id, pick_index, true);
	}

	

	void PavementComponent::randomCreateRoadUtil(size_t index)
	{
		if (!mBezier || !mBezier->m_pActComInfo || mBezier->getPointCount() == 0) return;
		if (!m_pActComInfo) return;
		if (!mNoReBuild[index])
		{
			auto pFindName = [](std::vector<float>& values, float& sum, float weight)->size_t
			{
				bool needChange = false;
				float raValue = float(rand() % 100) / 100.f * sum;
				size_t i = 0;
				float n_sum = 0.f;
				for (auto& v : values)
				{
					if (raValue <= v)
						needChange = true;

					if (needChange)
						v = v - weight / 10.f > 0 ? v - weight / 10.f : 0.f;
					else
						i++;
					n_sum += v;
				}
				sum = n_sum;
				if (i == values.size()) --i;
				return i;
			};

			PavementComponentInfo* pInfo = GetPavementInfo();
			PavementInfoData* layer_d = pInfo->mLayerDatas[index];
			std::vector<PavementInfoLayerData*>& mesh_ds = pInfo->mLayerDatas[index]->m_Meshdatas;

			srand((unsigned int)time(0));

			// 顺序判断
			std::vector<float> weights;
			std::vector<Name> mesh_names;
			for (auto& p_mesh_d : mesh_ds)
			{
				auto iter = std::find(mesh_names.begin(), mesh_names.end(), p_mesh_d->m_MeshName);
				if (iter == mesh_names.end())
				{
					mesh_names.push_back(p_mesh_d->m_MeshName);
					weights.push_back(p_mesh_d->m_Weighted);
				}
				else
				{
					size_t p_index = iter - mesh_names.begin();
					weights[p_index] += p_mesh_d->m_Weighted;
				}

				delete p_mesh_d;
			}
			mesh_ds.clear();

			float weight_sum = 0.f;
			for (auto& weight : weights)
				weight = weight_sum = weight_sum + weight;

			auto& lenvec = mBezier->getLengthVec();
			float lens = mBezier->getSegmentLengths();
			for (size_t len_i = 0; len_i < lenvec.size(); ++len_i)
			{
				if (lenvec[len_i] == 0.f) break;
				float randvalue = (float)(rand() % 100) / 100.f;
				PavementInfoLayerData* data = new PavementInfoLayerData();
				
				data->m_Weighted = lenvec[len_i] / lens * 10.f;
				data->m_RotatePoint.x = layer_d->m_MinRotation.x + (layer_d->m_MaxRotation.x - layer_d->m_MinRotation.x) * randvalue;
				randvalue = (float)(rand() % 100) / 100.f;
				data->m_RotatePoint.y = layer_d->m_MinRotation.y + (layer_d->m_MaxRotation.y - layer_d->m_MinRotation.y) * randvalue;
				randvalue = (float)(rand() % 100) / 100.f;
				data->m_RotatePoint.z = layer_d->m_MinRotation.z + (layer_d->m_MaxRotation.z - layer_d->m_MinRotation.z) * randvalue;
				data->m_PickPlace = (int)len_i + 1;
				size_t name_index = pFindName(weights, weight_sum, data->m_Weighted);
				data->m_MeshName = mesh_names[name_index];

				mesh_ds.push_back(data);
			}
		}
	}

	PavementComponentInfo* PavementComponent::GetPavementInfo()
	{
		if (m_pActComInfo)
			return static_cast<PavementComponentInfo*>(m_pActComInfo);
		else
			return nullptr;
	}

	void PavementComponent::dealComponentChange(const ActorComponentPtr& com)
	{
		if (com.expired())
			return;
		if (com.get() == mBezier && m_pActComInfo)
		{
			PavementComponentInfo* info = (PavementComponentInfo*)m_pActComInfo;
			info->mBindComName = com->getComName();
		}
	}

	void PavementComponent::dealComponentDelete(const ActorComponentPtr& com)
	{
		if (com.expired())
			return;
		if (com.get() == mBezier)
		{
			mBezier = nullptr;
			if(m_pActComInfo)
			{
				PavementComponentInfo* info = (PavementComponentInfo*)m_pActComInfo;
				info->mBindComName.clear();
			}
		}
	}

	void PavementComponent::clearAllPavements()
	{
		//组件析构前删除所有的异步请求
		for (auto& [group, data] : mPavements)
		{
			for (auto& object : data.second)
			{
				object->abortAllRequests();
			}
		}

		for (auto& [pavement, colliderVec] : mPavementPhys)
		{
			for (auto& collider : colliderVec)
			{
				getActor()->delComponent(collider->weak_from_this());
			}
		}
		mPavementPhys.clear();
		mPavements.clear();
	}

	void PavementComponent::setBezierCom(const ActorComponentPtr& bezier)
	{
		if (bezier.expired() || bezier->getType() != COMPONENTTYPE(PiecewiseBezierComponent))
			return;
		mBezier = bezier.getStaticCastWeakPtr<PiecewiseBezierComponent>();
		mBezier->addRoadIns(weak_from_this());
		for (int i = 0; i < mBezier->getPointCount(); ++i)
			mPickAngles.push_back(mBezier->getRotateAngle(i));
	}

	void PavementComponent::setBezierComByName(const Name& bezierName)
	{
		if (bezierName.empty() || getActor().expired())
			return;
		ActorComponentPtr bezier = getActor()->getComponent(bezierName.c_str());
		if (!bezier.expired() && bezier->getType() == COMPONENTTYPE(PiecewiseBezierComponent))
		{
			mBezier = bezier.getStaticCastWeakPtr<PiecewiseBezierComponent>();
			mBezier->addRoadIns(weak_from_this());
			for (int i = 0; i < mBezier->getPointCount(); ++i)
				mPickAngles.push_back(mBezier->getRotateAngle(i));
		}
	}

	bool PavementComponent::IsPavementGroup(const Name& group)
	{
		if (group.empty())
			return false;
		if (mPavements.find(group) != mPavements.end())
			return true;
		else
			return false;
	}

	void PavementComponent::ChangePavementGroupName(const Name& oldName, const Name& newName)
	{
		if (!IsPavementGroup(oldName))
		{
			LogManager::instance()->logMessage("PavementComponent::ChangePavementGroupName() old name is not exist!");
			return;
		}
		if (newName.empty() || IsPavementGroup(newName))
		{
			LogManager::instance()->logMessage("PavementComponent::ChangePavementGroupName() new name is empty or exist!");
			return;
		}
		mPavements[newName] = mPavements[oldName];
		mPavements.erase(oldName);
	}

	static std::vector<PavementMeshBaseData> CreatePavementDataFromTemplate(const PlanetRoadAssembly& inRoadTemplate)
	{
		std::vector<PavementMeshBaseData> pavementDataList;
		NameGenerator pavementName("Pavement");
		for (const PlanetRoadDesc& roadData : inRoadTemplate.groups)
		{
			PavementMeshBaseData pavementData;
			pavementData.groupName = pavementName.generate();
			pavementData.meshName = roadData.meshName;
			pavementData.offset = roadData.offset;
			pavementData.quat = roadData.quat;
			pavementData.scale = roadData.scale;
			pavementData.partLength = roadData.partLength;
			pavementData.m_SpacingStep = roadData.m_SpacingStep;
			pavementData.m_RenderDistance = roadData.m_RenderDistance;
			pavementData.m_Lod1Distance = roadData.m_Lod1Distance;
			pavementData.m_Lod2Distance = roadData.m_Lod2Distance;
			pavementData.m_surfaceType = roadData.m_surfaceType;
			pavementDataList.push_back(pavementData);
		}
		return pavementDataList;
	}

	bool PavementComponent::InitializeFromTemplate(int32 templateID)
	{
		// TODO: currently only record the template ID, need to implement the full initialization logic and replace the old initialization logic
		mCurrentTemplateID = templateID;

		//PavementComponentInfo* info = static_cast<PavementComponentInfo*>(m_pActComInfo);

		//PlanetRoadAssembly roadTemplate;
		//if (!PlanetRoadResourceManager::instance()->m_PlanetRoadMeshLibrary.GetAssemblyByID(templateID, roadTemplate))
		//{
		//	return false;
		//}

		//clearAllPavements();
		//info->mPavementData = CreatePavementDataFromTemplate(roadTemplate);

		//for (const PavementMeshBaseData& pavementData : info->mPavementData)
		//{
		//	createPavement(pavementData, PavementComponent::UpdateRender, true);
		//}
		return true;
	}

	bool PavementComponent::ReplaceTemplate(int32 newTemplateID)
	{
		PavementComponentInfo* info = static_cast<PavementComponentInfo*>(m_pActComInfo);

		PlanetRoadAssembly newTemplate;
		if (!PlanetRoadResourceManager::instance()->m_PlanetRoadMeshLibrary.GetAssemblyByID(newTemplateID, newTemplate))
		{
			return false;
		}

		int32 roadID = getRoadID();

		if (roadID >= 0)
		{
			SphericalTerrain* planet = ((PavementComponent::PavementFitPlanetTerrain*)getPavementFitTerrain())->getTerrainPtr();
			if (planet)
			{
				PlanetRoadPtr roadRes = planet->mRoad->mPlanetRoadData;
				PlanetRoadGroup& roadGroup = roadRes->getPlanetRoadGroupRef();
				//roadGroup.roads
				auto it = std::find_if(roadGroup.roads.begin(), roadGroup.roads.end(),
					[&](const PlanetRoadData& road) { return road.roadID == roadID; });

				if (it != roadGroup.roads.end())
				{
					PlanetRoadData& roadData = *it;
					roadData.SetAssemblyID(newTemplateID);
				}
			}
		}

		clearAllPavements();
		info->mPavementData = CreatePavementDataFromTemplate(newTemplate);
		mCurrentTemplateID = newTemplateID;

		for (const PavementMeshBaseData& pavementData : info->mPavementData)
		{
			createPavement(pavementData, PavementComponent::UpdateRender, true);
		}

		return true;
	}

	void PavementComponent::SetBaseMesh(const Name& meshName, const Name& group)
	{
		if (!IsPavementGroup(group))
		{
			LogManager::instance()->logMessage("PavementComponent::SetBaseMesh() group name is not exist or empty!");
			return;
		}
		mPavements[group].first.meshName = meshName;
		for (auto& pavement : mPavements[group].second)
		{
			if (!pavement)
				continue;
			pavement->setBaseMesh(meshName);
		}
	}

	void PavementComponent::SetMeshOffset(const Vector3& pos, const Name& group)
	{
		if (!IsPavementGroup(group))
		{
			LogManager::instance()->logMessage("PavementComponent::SetMeshOffset() group name is not exist or empty!");
			return;
		}
		mPavements[group].first.offset = pos;
		for (auto& pavement : mPavements[group].second)
		{
			if (!pavement)
				continue;
			pavement->setMeshOffset(pos);
		}
	}

	void PavementComponent::SetMeshQuat(const Quaternion& quat, const Name& group)
	{
		if (!IsPavementGroup(group))
		{
			LogManager::instance()->logMessage("PavementComponent::SetMeshQuat() group name is not exist or empty!");
			return;
		}
		mPavements[group].first.quat = quat;
		for (auto& pavement : mPavements[group].second)
		{
			if (!pavement)
				continue;
			pavement->setMeshQuat(quat);
		}
	}

	void PavementComponent::SetMeshScale(const Vector3& scale, const Name& group)
	{
		if (!IsPavementGroup(group))
		{
			LogManager::instance()->logMessage("PavementComponent::SetMeshScale() group name is not exist or empty!");
			return;
		}
		mPavements[group].first.scale = scale;
		for (auto& pavement : mPavements[group].second)
		{
			if (!pavement)
				continue;
			pavement->setMeshScale(scale);
		}
	}

	void PavementComponent::SetBaseMeshTransform(const Vector3& pos, const Quaternion& quat, const Vector3& scale, const Name& group)
	{
		if (!IsPavementGroup(group))
		{
			LogManager::instance()->logMessage("PavementComponent::SetBaseMesh() group name is not exist or empty!");
			return;
		}
		mPavements[group].first.offset = pos;
		mPavements[group].first.quat = quat;
		mPavements[group].first.scale = scale;
		Pavement::Transform data{ pos, quat, scale };
		for (auto& pavement : mPavements[group].second)
		{
			if (!pavement)
				continue;
			pavement->setBaseMeshTransform(data);
		}
	}

	void PavementComponent::SetGroupPartIndex(int partIndex, const Name& group)
	{
		if (mBezier.expired())
		{
			LogManager::instance()->logMessage("PavementComponent::SetGroupPartIndex() the Bezier Component  is NULL!");
			return;
		}
		if (!IsPavementGroup(group))
		{
			LogManager::instance()->logMessage("PavementComponent::SetGroupPartIndex() group name is not exist or empty!");
			return;
		}
		int partsize = mBezier->getPointCount() == 0 ? 0 : (int)mBezier->getPointCount() - 1;
		if (partIndex >= partsize || partIndex < -1)
			partIndex = partsize - 1;
		mPavements[group].first.partIndex = partIndex;
	}

	void PavementComponent::SetBezierUpdateType(int type, const Name& group)
	{
		if (!IsPavementGroup(group))
		{
			LogManager::instance()->logMessage("PavementComponent::SetBaseMesh() group name is not exist or empty!");
			return;
		}
		mPavements[group].first.updateType = type;
		for (auto& pavement : mPavements[group].second)
		{
			if (!pavement)
				continue;
			pavement->setBezierUpdateType(type);
		}
	}

	void PavementComponent::SetPartLength(float distance, const Name& group)
	{
		if (!IsPavementGroup(group))
		{
			LogManager::instance()->logMessage("PavementComponent::SetBaseMesh() group name is not exist or empty!");
			return;
		}
		mPavements[group].first.partLength = distance;
		for (auto& pavement : mPavements[group].second)
		{
			if (!pavement)
				continue;
			pavement->setPartitionDistance(distance);
		}
	}

	void PavementComponent::SetStepLength(float distance, const Name& group)
	{
		if (!IsPavementGroup(group))
		{
			LogManager::instance()->logMessage("PavementComponent::SetBaseMesh() group name is not exist or empty!");
			return;
		}
		mPavements[group].first.m_SpacingStep = distance;
		if (Math::FloatEqual(distance, 0.0f))
		{
			mPavements[group].first.m_IsCurveModel = true;
			mPavements[group].first.m_SpacingStep = false;
		}
		for (auto& pavement : mPavements[group].second)
		{
			if (!pavement)
				continue;
			pavement->setStepLength(distance);
		}
	}

	void PavementComponent::SetCurveModel(bool isCurveModel, const Name& group)
	{
		if (!IsPavementGroup(group))
		{
			LogManager::instance()->logMessage("PavementComponent::SetBaseMesh() group name is not exist or empty!");
			return;
		}
		for (auto& pavement : mPavements[group].second)
		{
			if (!pavement)
				continue;
			if (pavement->setCurveModel(isCurveModel))
				mPavements[group].first.m_IsCurveModel = isCurveModel;
		}
	}

	void PavementComponent::SetPierModel(bool isPierModel, const Name& group)
	{
		if (!IsPavementGroup(group))
		{
			LogManager::instance()->logMessage("PavementComponent::SetBaseMesh() group name is not exist or empty!");
			return;
		}
		if (!mPavements[group].first.m_IsCurveModel)
			mPavements[group].first.m_IsPierModel = isPierModel;
		for (auto& pavement : mPavements[group].second)
		{
			if (!pavement)
				continue;
			pavement->setPierModel(isPierModel);
		}
	}

	void PavementComponent::SetRenderDistance(float distance, const Name& group)
	{
		if (!IsPavementGroup(group))
		{
			LogManager::instance()->logMessage("PavementComponent::SetBaseMesh() group name is not exist or empty!");
			return;
		}
		mPavements[group].first.m_RenderDistance = distance;
		for (auto& pavement : mPavements[group].second)
		{
			if (!pavement)
				continue;
			pavement->setRenderDistance(distance);
		}
	}

	void PavementComponent::SetLod1Distance(float distance, const Name& group)
	{
		if (!IsPavementGroup(group))
		{
			LogManager::instance()->logMessage("PavementComponent::SetBaseMesh() group name is not exist or empty!");
			return;
		}
		mPavements[group].first.m_Lod1Distance = distance;
		for (auto& pavement : mPavements[group].second)
		{
			if (!pavement)
				continue;
			pavement->setLodDistance(LOD_LEVEL_1, distance);
		}
	}

	void PavementComponent::SetLod2Distance(float distance, const Name& group)
	{
		if (!IsPavementGroup(group))
		{
			LogManager::instance()->logMessage("PavementComponent::SetBaseMesh() group name is not exist or empty!");
			return;
		}
		mPavements[group].first.m_Lod2Distance = distance;
		for (auto& pavement : mPavements[group].second)
		{
			if (!pavement)
				continue;
			pavement->setLodDistance(LOD_LEVEL_2, distance);
		}
	}

	void PavementComponent::SetSurfaceType(uint32 type, const Name& group)
	{
		if (!IsPavementGroup(group))
		{
			LogManager::instance()->logMessage("PavementComponent::SetSurfaceType() group name is not exist or empty!");
			return;
		}
		mPavements[group].first.m_surfaceType = type;
		for (auto& pavement : mPavements[group].second)
		{
			if (!pavement)
				continue;
			pavement->mPhySurfaceType = type;
			auto iter = mPavementPhys.find(pavement);
			if (iter != mPavementPhys.end())
			{
				for (auto& phyCom : mPavementPhys[pavement])
				{
					phyCom->SetSurfaceType(type);
				}
			}
		}
	}

	const Name& PavementComponent::GetBaseMeshName(const Name& group)
	{
		if (IsPavementGroup(group))
		{
			return mPavements[group].first.meshName;
		}
			
		return Name::g_cEmptyName;
	}

	const Vector3& PavementComponent::GetBaseMeshOffset(const Name& group)
	{
		if (IsPavementGroup(group))
		{
			return mPavements[group].first.offset;
		}
		
		return Vector3::ZERO;
	}

	const Quaternion& PavementComponent::GetBaseMeshQuat(const Name& group)
	{
		if (IsPavementGroup(group))
		{
			return mPavements[group].first.quat;
		}
		
		return Quaternion::IDENTITY;
	}

	const Vector3& PavementComponent::GetBaseMeshScale(const Name& group)
	{
		if (IsPavementGroup(group))
		{
			return mPavements[group].first.scale;
		}
		
		return Vector3::UNIT_SCALE;
	}

	int PavementComponent::GetGroupPartIndex(const Name& group)
	{
		if (IsPavementGroup(group))
		{
			return mPavements[group].first.partIndex;
		}
		return -1;
	}

	int PavementComponent::GetBezierUpdatType(const Name& group)
	{
		if (IsPavementGroup(group))
		{
			return mPavements[group].first.updateType;
		}
		return 2;
	}

	float PavementComponent::GetPartLength(const Name& group)
	{
		if (IsPavementGroup(group))
		{
			return mPavements[group].first.partLength;
		}
		return 0.0f;
	}

	float PavementComponent::GetStepLength(const Name& group)
	{
		if (IsPavementGroup(group))
		{
			return mPavements[group].first.m_SpacingStep;
		}
		return 0.0f;
	}

	bool PavementComponent::GetIsCurveModel(const Name& group)
	{
		if (IsPavementGroup(group))
		{
			return mPavements[group].first.m_IsCurveModel;
		}
		return false;
	}

	bool PavementComponent::GetIdPierModel(const Name& group)
	{
		if (IsPavementGroup(group))
		{
			return mPavements[group].first.m_IsPierModel;
		}
		return false;
	}

	float PavementComponent::GetRenderDistance(const Name& group)
	{
		if (IsPavementGroup(group))
		{
			return mPavements[group].first.m_RenderDistance;
		}
		return 0.0f;
	}

	float PavementComponent::GetLod1Distance(const Name& group)
	{
		if (IsPavementGroup(group))
		{
			return mPavements[group].first.m_Lod1Distance;
		}
		return 0.0f;
	}

	float PavementComponent::GetLod2Distance(const Name& group)
	{
		if (IsPavementGroup(group))
		{
			return mPavements[group].first.m_Lod2Distance;
		}
		return 0.0f;
	}

	uint32 PavementComponent::GetSurfaceType(const Name& group)
	{
		if (IsPavementGroup(group))
		{
			return mPavements[group].first.m_surfaceType;
		}
		return 0;
	}

	void PavementComponent::createPavement(const PavementMeshBaseData& data, UpdateType type, bool sync)
	{
		if (!Root::instance()->UsePavementTestNewMode)
			return;
		if (mBezier.expired())
		{
			LogManager::instance()->logMessage("PavementComponent::createPavement() the Bezier Component  is NULL!");
			return;
		}
		if (data.groupName.empty())
		{
			LogManager::instance()->logMessage("PavementComponent::createPavement() the pavement name is empty!");
			return;
		}
		if (mPavements.find(data.groupName) != mPavements.end())
		{
			LogManager::instance()->logMessage("PavementComponent::createPavement() the pavement name already exists!");
			return;
		}
		mPavements[data.groupName].first = data;
		updateGroupPavement(data.groupName, type, sync);
	}

	void PavementComponent::deletePavement(const Name& group)
	{
		if (!Root::instance()->UsePavementTestNewMode)
			return;

		if (mPavements.find(group) == mPavements.end())
			return;
		for (auto& pavement : mPavements[group].second)
		{
			if (mPavementPhys.find(pavement) != mPavementPhys.end())
			{
				for (auto& collider : mPavementPhys[pavement])
				{
					getActor()->delComponent(collider->weak_from_this());
				}
				mPavementPhys.erase(pavement);
			}
		}
		mPavements.erase(group);
	}

	void PavementComponent::updateAllPavement(UpdateType type, bool sync)
	{
		if (!Root::instance()->UsePavementTestNewMode)
			return;

		for (auto& [groupName, data] : mPavements)
		{
			updateGroupPavement(groupName, type, sync);
		}
	}

	void PavementComponent::updateGroupPavement(const Name& group, UpdateType type, bool sync)
	{
		if (!Root::instance()->UsePavementTestNewMode)
			return;
		if (mBezier.expired())
		{
			LogManager::instance()->logMessage("PavementComponent::updateGroupPavement() the Bezier Component  is NULL!");
			return;
		}
		if (mPavements.find(group) == mPavements.end())
		{
			LogManager::instance()->logMessage("PavementComponent::updateGroupPavement() error group name. can not find.");
			return;
		}
		size_t partsize = mBezier->getPointCount() == 0 ? 0 : mBezier->getPointCount() - 1;
		//for (auto& [group, datas] : mPavements)
		//{
			for (size_t index = 0; index < partsize; ++index)
			{
				updatePavement(index, group, type, sync);
			}
		//}
	}

	void PavementComponent::updateIndexPavement(size_t index, UpdateType type, bool sync)
	{
		if (!Root::instance()->UsePavementTestNewMode)
			return;

		if (mBezier.expired())
		{
			LogManager::instance()->logMessage("PavementComponent::updateIndexPavement() the Bezier Component  is NULL!");
			return;
		}
		size_t partsize = mBezier->getPointCount() == 0 ? 0 : mBezier->getPointCount() - 1;
		if (index >= partsize)
		{
			LogManager::instance()->logMessage("PavementComponent::updateIndexPavement() update pavement with error index!");
			return;
		}
		for (auto& [name, datas] : mPavements)
		{
			updatePavement(index, name, type, sync);
		}
	}

	void PavementComponent::updatePavement(size_t index, const Name& group, PavementComponent::UpdateType type /*= UpdateRender*/, bool sync /*= false*/)
	{
		if (!Root::instance()->UsePavementTestNewMode)
			return;
		if (mBezier.expired())
		{
			LogManager::instance()->logMessage("PavementComponent::updatePavement() the Bezier Component  is NULL!");
			return;
		}
		if (group.empty())
		{
			LogManager::instance()->logMessage("PavementComponent::updatePavement() the group name is empty!");
			return;
		}
		if (mPavements.find(group) == mPavements.end())
		{
			LogManager::instance()->logMessage("PavementComponent::updatePavement() error group name. can not find.");
			return;
		}

		size_t partsize = mBezier->getPointCount() == 0 ? 0 : mBezier->getPointCount() - 1;
		if (index >= partsize)
		{
			LogManager::instance()->logMessage("PavementComponent::updatePavement() update pavement with error index!");
			return;
		}
		PavementMeshBaseData& pavementData = mPavements[group].first;
		std::vector<std::shared_ptr<Pavement>>& pavementVec = mPavements[group].second;

		//这里如果之前已作用生成的段被删了，默认取最后一段
		if (pavementData.partIndex > (int)partsize || pavementData.partIndex < -1)
			pavementData.partIndex = (int)partsize;

		while (pavementVec.size() > partsize)
		{
			pavementVec.pop_back();
		}
		pavementVec.resize(partsize);

		if (pavementData.partIndex == -1 || pavementData.partIndex == index)
		{
			PavementDesc desc;

			desc.parent = this;
			desc.pavementName = pavementData.groupName;
			desc.meshName = pavementData.meshName;

			BezierDataCache bezierdata;
			bezierdata.mControllerPoint[0] = mBezier->getPoint(index)->point;
			bezierdata.mControllerPoint[1] = mBezier->getPoint(index)->destination_control;
			bezierdata.mControllerPoint[3] = mBezier->getPoint(index + 1)->point;
			bezierdata.mControllerPoint[2] = mBezier->getPoint(index + 1)->source_control;
			bezierdata.mControllerNormal[0] = mBezier->getPoint(index)->normal;
			bezierdata.mControllerNormal[1] = mBezier->getPoint(index + 1)->normal;
			bezierdata.mControllerFitGroundLevel[0] = mBezier->getPoint(index)->FitGorundLevel;
			bezierdata.mControllerFitGroundLevel[1] = mBezier->getPoint(index + 1)->FitGorundLevel;
			bezierdata.AxisXAngle[0] = mBezier->getPoint(index)->AxisXAngle;
			bezierdata.AxisXAngle[1] = mBezier->getPoint(index + 1)->AxisXAngle;
			bezierdata.mLength = mBezier->getPiecewiseLength(index, false);
			bezierdata.mType = pavementData.updateType;
			desc.bezierdata = bezierdata;

			desc.segmentLength = pavementData.partLength;
			desc.spacingStep = pavementData.m_SpacingStep;

			desc.synchronous = sync;
			desc.updateType = type;

			desc.renderDistance = pavementData.m_RenderDistance;
			desc.lod1_distance = pavementData.m_Lod1Distance;
			desc.lod2_distance = pavementData.m_Lod2Distance;

			desc.position = pavementData.offset;
			desc.rotation = pavementData.quat;
			desc.scale = pavementData.scale;

			desc.m_IsCurveModel = pavementData.m_IsCurveModel;
			desc.m_IsPierModel = pavementData.m_IsPierModel;

			desc.isFitGround = fitGround;

			desc.m_surfaceType = pavementData.m_surfaceType;

			if (pavementVec[index])
			{
				pavementVec[index]->setPavementDesc(desc);
				pavementVec[index]->update(desc.synchronous, desc.updateType);
			}
			else
			{
				pavementVec[index] = std::make_shared<Pavement>(desc);
				pavementVec[index]->updatePavement();	//构造完Pavement后必须调用一次
			}
		}
		else if (pavementVec[index])
		{
			std::weak_ptr<Pavement> pavement = pavementVec[index];

			if (mPavementPhys.find(pavement) != mPavementPhys.end())
			{
				for (auto& collider : mPavementPhys[pavement])
				{
					getActor()->delComponent(collider->weak_from_this());
				}
				mPavementPhys.erase(pavement);
			}
			pavementVec[index].reset();
		}

		return;
	}

	void PavementComponent::updatePavementPhyObj(std::shared_ptr<Pavement> sharedpavement)
	{
		std::weak_ptr<Pavement> querypavement = sharedpavement;
		std::vector<MeshColliderComponent*>& phyVec = mPavementPhys[querypavement];
		size_t oldsize = phyVec.size();
		size_t newsize;

		if (sharedpavement->getIsCurveModel())
		{
			//扭曲模型
			newsize = sharedpavement->getPavementPhyDataCache().size();
		}
		else
		{
			//非扭曲模型
			newsize = sharedpavement->getPavementTransformDataCache().size();
		}
		//重新组织物理组件MeshColliderComponent的数量,暂时先采用每次更新动态创建删除物理组件的策略
		while (newsize != oldsize && oldsize >= 0)
		{
			if (newsize > oldsize)
			{
				ActorComponentPtr phyCom = getActor()->addComponent(COMPONENTTYPE(MeshColliderComponent), this->getComName() + "_PavementPhy", weak_from_this(), nullptr, getGroupID());
				phyVec.push_back((MeshColliderComponent*)phyCom.get());
				++oldsize;
			}
			else
			{
				getActor()->delComponent(phyVec[oldsize - 1]->weak_from_this());
				phyVec.pop_back();
				--oldsize;
			}
		}

		for (size_t index = 0; index < newsize; ++index)
		{
			phyVec[index]->SetSurfaceType(sharedpavement->mPhySurfaceType);
			if(sharedpavement->getIsCurveModel())
			{
				const PavementPhyData& phyData = sharedpavement->getPavementPhyDataCache();
				phyVec[index]->SetMeshData(phyData[index].vertexVec, phyData[index].indexVec);
				phyVec[index]->SetPhysicsLocalAABB(phyData[index].mBox);
				phyVec[index]->setPosition(Vector3::ZERO);
				phyVec[index]->setRotation(Quaternion::IDENTITY);
				phyVec[index]->setScale(Vector3::UNIT_SCALE);
			}
			else
			{
				const Pavement::TransformVec& transformData = sharedpavement->getPavementTransformDataCache();
				Name phyName = Mesh::getDstPhyMeshName(sharedpavement->getMeshName());
				phyVec[index]->SetMeshName_meshphy(phyName);
				phyVec[index]->setPosition(transformData[index].position);
				phyVec[index]->setRotation(transformData[index].rotation);
				phyVec[index]->setScale(transformData[index].scale);
			}
		}
	}

	bool PavementComponent::saveRoadFile(cJSON* root)
	{
		if (m_pActComInfo == nullptr) return false;

		if (mBezier.expired())
		{
			LogManager::instance()->logMessage("PavementComponent::saveRoadFile() the Bezier Component  is NULL!");
			return false;
		}

		cJSON* _js_node = cJSON_CreateArray();
		cJSON_AddItemToObject(root, mBezier->getComName().c_str(), _js_node);
		cJSON_AddStringToObject(root, "Version", "1.0");

		for (auto& [group, data] : mPavements)
		{
			cJSON* _js_Road = cJSON_CreateObject();
			cJSON_AddItemToObject(_js_node, mBezier->getComName().c_str(), _js_Road);

			cJSON_AddNumberToObject(_js_Road, "UpdateType", data.first.updateType);
			cJSON_AddNumberToObject(_js_Road, "MaxPillarSum", 0);

			cJSON* array_id = cJSON_CreateArray();
			cJSON_AddItemToObject(_js_Road, "Ids", array_id);
			//for (auto& map_v : data->m_Meshdatas)
				cJSON_AddNumberToObject(array_id, "", data.first.partIndex);

			cJSON* array_data = cJSON_CreateArray();
			cJSON_AddItemToObject(_js_Road, "Meshs", array_data);
			//for (auto& map_v : data->m_Meshdatas)
			cJSON_AddStringToObject(array_data, "", data.first.meshName.c_str());

			cJSON* array_change = cJSON_CreateArray();
			cJSON_AddItemToObject(_js_Road, "RoadChanges", array_change);
			//for (auto& data : data->m_Meshdatas)
			{
				WriteJSNode(array_change, "", data.first.offset);
				WriteJSNode(array_change, "", data.first.scale);
				WriteJSNode(array_change, "", data.first.quat.ToEuler());
			}

			cJSON* array_weight = cJSON_CreateArray();
			cJSON_AddItemToObject(_js_Road, "Weighted", array_weight);
			//for (auto& data : data->m_Meshdatas)
			WriteJSNode(array_weight, "", 1.0f);

			/*if (data->m_PillarCount.size() != 0)
			{
				cJSON* array_height = cJSON_CreateArray();
				cJSON_AddItemToObject(_js_Road, "PillarCount", array_height);
				for (auto& heights : data->m_PillarCount)
				{
					cJSON* array_h = cJSON_CreateArray();
					cJSON_AddItemToArray(array_height, array_h);
					for (auto& height : heights)
						WriteJSNode(array_h, "", height);
				}
			}*/

			cJSON_AddNumberToObject(_js_Road, "Spacing Step", data.first.m_SpacingStep);
			cJSON_AddNumberToObject(_js_Road, "Pick Model", false);
		}

		return true;
	}

	void PavementComponent::setFitGroundModel(bool isFitGround)
	{
		if (fitGround == isFitGround)
			return;
		fitGround = isFitGround;
		/*if (fitGround && !mFitTerrain)
		{
			PavementFitPlanetTerrain* terrain = (PavementFitPlanetTerrain*)mDefaultFitTerrain;
			for (auto& planet : getSceneManager()->getPlanetManager()->m_SphericalTerrainSet)
			{
				if (!terrain->getTerrainPtr())
				{
					terrain->setTerrainPtr(planet);
				}
				else
				{
					DVector3 pos = getWorldPosition();
					if (pos.squaredDistance(terrain->getTerrainPtr()->m_pos) > pos.squaredDistance(planet->m_pos))
						terrain->setTerrainPtr(planet);
				}
			}
		}*/
		for (auto& [group,data] : mPavements)
		{
			for(auto& pavement : data.second)
			{
				if (!pavement)
					continue;
				pavement->setFitGroundModel(fitGround);
			}
		}
		if (!fitGround && !mFitTerrain)
		{
			RoadManager::instance()->removePlanetFitGroundPavementAreaCache(this->weak_from_this());
		}
	}

	void PavementComponent::setPavementFitTerrain(PavementFitTerrain* terrain)
	{
		mFitTerrain = terrain;

		if (!terrain)
			return;

		if (mVegetationCollection && mFitTerrain->mType == PavementFitTerrain::planetTerrain)
		{
			SphericalTerrain* mPlanet = ((PavementComponent::PavementFitPlanetTerrain*)mFitTerrain)->getTerrainPtr();
			if (!mPlanet)
				return;
			RoadManager::instance()->addPlanetFitGroundPavementAreaCache(weak_from_this(), mPlanet, getWorldAABB());
		}
	}

	void PavementComponent::setVegetationCollection(bool isCollection)
	{
		mVegetationCollection = isCollection;
	}

	bool PavementComponent::isPavementsCreateFinish()
	{
		if (mPavements.empty())
			return false;
		for (const auto& group : mPavements)
		{
			for (const auto& pavement : group.second.second)
			{
				if (!pavement)
					continue;
				if (pavement->getIsCalculating())
					return false;
			}
		}
		return true;
	}

	void PavementComponent::PavementFitPlanetTerrain::getTerrainData(const Vector3& worldPos, PavementFitTerrain::TerrainData& data)
	{
		//if (com.expired())
		//	return;
		SphericalTerrain* mPlanet = getTerrainPtr();
		if (!mPlanet)
			return;
		if (mPlanet && !mPlanet->isCreateFinish())
		{
			LogManager::instance()->logMessage("PavementComponent::PavementFitPlanetTerrain::getTerrainData():: planet is not create finished");
			return;
		}
		data.terrainHeight = mPlanet->getCurrentHeight(worldPos);
		data.terrainPos = mPlanet->getSurfaceFinestWs(worldPos, &data.terrainNormal);
		data.terrainNormal.normalise();

	}

	//SphericalTerrain* PavementComponent::PavementFitPlanetTerrain::getTerrainPtr()
	//{
	//	if (com.expired())
	//		return nullptr;
	//	SphericalTerrain* mPlanet = com->getActorManager()->getSceneManager()->getPlanetManager()->m_nearestPlanet;
	//	if (!mPlanet && !com->getActorManager()->getSceneManager()->getPlanetManager()->m_SphericalTerrainSet.empty())
	//	{
	//		for (auto& planet : com->getSceneManager()->getPlanetManager()->m_SphericalTerrainSet)
	//		{
	//			if (!mPlanet)
	//			{
	//				mPlanet = planet;
	//			}
	//			else
	//			{
	//				DVector3 pos = com->getWorldPosition();
	//				if (pos.squaredDistance(mPlanet->m_pos) > pos.squaredDistance(planet->m_pos))
	//					mPlanet = planet;
	//			}
	//		}
	//	}
	//	assert(mPlanet && "PavementComponent::PavementFitPlanetTerrain::getTerrainPtr():: There is no planet.");
	//	/*if (mPlanet && !mPlanet->isCreateFinish())
	//	{
	//		LogManager::instance()->logMessage("PavementComponent::PavementFitPlanetTerrain::getTerrainPtr():: planet is not create finished");
	//		return nullptr;
	//	}*/
	//	return mPlanet;
	//}

}
