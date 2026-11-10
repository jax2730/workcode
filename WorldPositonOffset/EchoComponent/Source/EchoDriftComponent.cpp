#include "EchoComponentStableHeaders.h"
#include "EchoDriftComponent.h"
#include "EchoTODComponentManager.h"
#include "EchoTODComponentSystem.h"
namespace Echo
{
	DECLAREACTORCOMTYPE_CPP(DriftComponent)

	DriftComponent::DriftComponent(ActorPtr  pActor, ActorCOMMoveableType type)
		: ActorComponent(pActor, type)
	{
	}

	DriftComponent::~DriftComponent()
	{

	}

	void DriftComponent::init(ActorComponentInfo* info)
	{
		if (info != nullptr)
		{
			DriftComponentInfo* driftInfo = static_cast<DriftComponentInfo*>(m_pActComInfo);
			m_nPriority = driftInfo->m_Priority;
			for (auto iter : driftInfo->m_DriftPropMap)
				createDrifts(iter.first, iter.second);
		}
		unuse();

		TODCOMPONENT_INITMGR(getDriftContext, m_tMgr, false)
	}
	void DriftComponent::unInit()
	{
		for (auto iter : m_mapDriftProp)
		{
			int realCount = getSceneManager()->getDriftManager()->getDriftsCount(this, iter.first);
			getSceneManager()->getDriftManager()->removeDrifts(this, iter.first, realCount);
		}
		m_mapDriftProp.clear();

		TODCOMPONENT_UNINITMGR(getDriftContext, m_tMgr, T_Drift);
	}
	void DriftComponent::copyTo(const ActorComponentPtr& newComp)
	{
	}
	void DriftComponent::visibleStateChanged(bool visibleState)
	{

		if (!visibleState)
		{
			for (auto iter : m_mapDriftProp)
			{
				int realCount = getSceneManager()->getDriftManager()->getDriftsCount(this, iter.first);
				getSceneManager()->getDriftManager()->removeDrifts(this, iter.first, realCount);
			}
			if(m_tMgr)
			   m_tMgr->popComponent(m_tMgr->getDriftContext(), this);
		}
		else
		{
			for (auto iter : m_mapDriftProp)
				createDrifts(iter.first, iter.second);
			if(m_tMgr)
				m_tMgr->pushComponent(m_tMgr->getDriftContext(), this);
		}
	}

	void DriftComponent::use()
	{
		m_bEnable = true;
		if (getRealVisibleState())
		{
			for (auto iter : m_mapDriftProp)
				createDrifts(iter.first, iter.second);
		}
	}

	void DriftComponent::unuse()
	{
		m_bEnable = false;
		if (getRealVisibleState())
		{
			for (auto iter : m_mapDriftProp)
			{
				int realCount = getSceneManager()->getDriftManager()->getDriftsCount(this, iter.first);
				getSceneManager()->getDriftManager()->removeDrifts(this, iter.first, realCount);
				iter.second.m_DriftCount = 0;
			}
		}
	}

	void DriftComponent::setPriority(const int& priority)
	{
		m_nPriority = priority;
		if(m_tMgr)
			m_tMgr->adjustComponentPriority(m_tMgr->getDriftContext(), this);
	}

	void DriftComponent::createDrifts(const Name& meshName, const DriftPropData& data)
	{
		createDriftsImpl(meshName, data.m_DriftCount);
		changeDriftsGeometryType(meshName, data.m_DriftGeometryType);
		changeDriftsAreaRange(meshName, data.m_AreaRange);
		changeDriftsAreaInnerRadius(meshName, data.m_InnerRadius);
		changeDriftsAreaOuterRadius(meshName, data.m_OuterRadius);
		changeDriftsSize(meshName, data.m_MinScale, data.m_MaxScale);
		changeDriftsVelocity(meshName, data.m_MinVelocity, data.m_MaxVelocity);
		changeDriftsRotateAxis(meshName, data.m_MinRotateAxis, data.m_MaxRotateAxis);
		changeDriftsRotateSpeed(meshName, data.m_MinRotateSpeed, data.m_MaxRotateSpeed);
		changeDriftsFadeInOffsetTime(meshName, data.m_FadeInTimeOffset);
		enableDriftsShadow(meshName, data.m_ShadowEnable);
	}

	void DriftComponent::createDriftsImpl(const Name& meshName, const int& count)
	{
		if (m_mapDriftProp.find(meshName) == m_mapDriftProp.end())
		{
			DriftPropData data;
			data.m_DriftCount = count;
			m_mapDriftProp.emplace(meshName, data);
		}

		changeDriftsCount(meshName, count);
	}

	void DriftComponent::removeDrifts(const Name& meshName)
	{
		if (m_mapDriftProp.find(meshName) == m_mapDriftProp.end())
			return;
		int realCount = getSceneManager()->getDriftManager()->getDriftsCount(this, meshName);
		removeDriftsCount(meshName, realCount);
		m_mapDriftProp.erase(meshName);
	}

	void DriftComponent::addDriftsCount(const Name& meshName, const int& count)
	{
		if (m_mapDriftProp.find(meshName) == m_mapDriftProp.end())
			return;
		if (getRuntimeVisibleState() && m_bEnable)
			getSceneManager()->getDriftManager()->addDrifts(this, meshName, count);
	}

	void DriftComponent::removeDriftsCount(const Name& meshName, const int& count)
	{
		if (m_mapDriftProp.find(meshName) == m_mapDriftProp.end())
			return;
		if (getRuntimeVisibleState() && m_bEnable)
			getSceneManager()->getDriftManager()->removeDrifts(this, meshName, count);
	}

	void DriftComponent::changeDriftsGeometryType(const Name& meshName, const DriftGeometryType& type)
	{
		if (m_mapDriftProp.find(meshName) == m_mapDriftProp.end())
			return;

		DriftPropData& data = m_mapDriftProp[meshName];

		data.m_DriftGeometryType = type;
		getSceneManager()->getDriftManager()->changeDriftsGeometryType(meshName, type);
	}

	void DriftComponent::changeDriftsMesh(const Name& oldMeshName, const Name& newMeshName)
	{
		if (m_mapDriftProp.find(oldMeshName) == m_mapDriftProp.end())
			return;
		DriftPropData data = m_mapDriftProp[oldMeshName];
		removeDrifts(oldMeshName);
		createDrifts(newMeshName, data);
	}

	void DriftComponent::changeDriftsCount(const Name& meshName, const int& count)
	{
		if (m_mapDriftProp.find(meshName) == m_mapDriftProp.end())
			return;
		DriftPropData& data = m_mapDriftProp[meshName];
		int realCount = getSceneManager()->getDriftManager()->getDriftsCount(this, meshName);
		if (realCount == count)
			return;
		int differ = count - realCount;
		if (differ > 0)
			addDriftsCount(meshName, differ);
		else
			removeDriftsCount(meshName, -differ);

		data.m_DriftCount = count;
	}

	void DriftComponent::changeDriftsAreaRange(const Name& meshName, const float& areaSize)
	{
		if (areaSize < 0.0f)
			return;
		if (m_mapDriftProp.find(meshName) == m_mapDriftProp.end())
			return;
		DriftPropData& data = m_mapDriftProp[meshName];

		data.m_AreaRange = areaSize;
		getSceneManager()->getDriftManager()->changeDriftsAreaSize(meshName, areaSize);
	}

	void DriftComponent::changeDriftsAreaInnerRadius(const Name& meshName, const float& innerRadius)
	{
		if (innerRadius < 0.0f)
			return;
		if (m_mapDriftProp.find(meshName) == m_mapDriftProp.end())
			return;
		DriftPropData& data = m_mapDriftProp[meshName];

		data.m_InnerRadius = innerRadius;
		getSceneManager()->getDriftManager()->changeDriftsInnerRadius(meshName, innerRadius);
	}

	void DriftComponent::changeDriftsAreaOuterRadius(const Name& meshName, const float& outerRadius)
	{
		if (outerRadius < 0.0f)
			return;
		if (m_mapDriftProp.find(meshName) == m_mapDriftProp.end())
			return;
		DriftPropData& data = m_mapDriftProp[meshName];

		data.m_OuterRadius = outerRadius;
		getSceneManager()->getDriftManager()->changeDriftsOuterRadius(meshName, outerRadius);
	}

	void DriftComponent::changeDriftsSize(const Name& meshName, const float& min, const float& max)
	{
		if (min < 0.0f || max < 0.0f || min > max)
			return;
		if (m_mapDriftProp.find(meshName) == m_mapDriftProp.end())
			return;
		DriftPropData& data = m_mapDriftProp[meshName];
		data.m_MinScale = min;
		data.m_MaxScale = max;
		getSceneManager()->getDriftManager()->changeDriftsSize(meshName, min, max);
	}

	void DriftComponent::changeDriftsVelocity(const Name& meshName, const Vector3& min, const Vector3& max)
	{
		if (min > max)
			return;
		if (m_mapDriftProp.find(meshName) == m_mapDriftProp.end())
			return;
		DriftPropData& data = m_mapDriftProp[meshName];
		data.m_MinVelocity = min;
		data.m_MaxVelocity = max;
		getSceneManager()->getDriftManager()->changeDriftsVelocity(meshName, min, max);
	}

	void DriftComponent::changeDriftsRotateAxis(const Name& meshName, const Vector3& min, const Vector3& max)
	{
		if (min > max)
			return;
		if (m_mapDriftProp.find(meshName) == m_mapDriftProp.end())
			return;
		DriftPropData& data = m_mapDriftProp[meshName];
		data.m_MinRotateAxis = min;
		data.m_MaxRotateAxis = max;
		getSceneManager()->getDriftManager()->changeDriftsRotateAxis(meshName, min, max);
	}

	void DriftComponent::changeDriftsRotateSpeed(const Name& meshName, const float& min, const float& max)
	{
		if (min > max)
			return;
		if (m_mapDriftProp.find(meshName) == m_mapDriftProp.end())
			return;
		DriftPropData& data = m_mapDriftProp[meshName];
		data.m_MinRotateSpeed = min;
		data.m_MaxRotateSpeed = max;
		getSceneManager()->getDriftManager()->changeDriftsRotateSpeed(meshName, min, max);
	}

	void DriftComponent::changeDriftsFadeInOffsetTime(const Name& meshName, const float& range)
	{
		if (m_mapDriftProp.find(meshName) == m_mapDriftProp.end())
			return;
		DriftPropData& data = m_mapDriftProp[meshName];
		data.m_FadeInTimeOffset = range;
		getSceneManager()->getDriftManager()->setDriftsFadeInOffsetTime(meshName, range);
	}

	void DriftComponent::enableDriftsShadow(const Name& meshName, const bool& bShadowEnable)
	{
		if (m_mapDriftProp.find(meshName) == m_mapDriftProp.end())
			return;
		DriftPropData& data = m_mapDriftProp[meshName];

		data.m_ShadowEnable = bShadowEnable;
		getSceneManager()->getDriftManager()->enableDriftsShadow(meshName, data.m_ShadowEnable);
	}

	void DriftComponent::setFadeInTime(const float& time)
	{
		for (auto& iter : m_mapDriftProp)
		{
			getSceneManager()->getDriftManager()->setDriftsFadeInTime(iter.first, time);
		}
	}

	void DriftComponent::setDriftsPropData(const std::map<Name, DriftPropData>& dataMap)
	{
		for (auto iter = m_mapDriftProp.begin(); iter != m_mapDriftProp.end();)
		{
			if (dataMap.find(iter->first) == dataMap.end())
			{
				changeDriftsCount(iter->first, 0);
				iter = m_mapDriftProp.erase(iter);
			}
			else
			{
				++iter;
			}
		}

		for (auto& iter : dataMap)
			createDrifts(iter.first, iter.second);
	}

	EDITOR_REFLECT_BEGIN(DriftComponentInfo)
	EDITOR_REFLECT_MEMBER_UPDATE(m_Priority, "DriftPriority", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_END();

	DriftComponentInfo::DriftComponentInfo(const Name& comType, ActorCOMMoveableType moveType)
		: ActorComponentInfo(comType, moveType)
	{
	}
	DriftComponentInfo::~DriftComponentInfo()
	{
	}

	void DriftComponentInfo::ExportData(cJSON* jsonNode)
	{
		reflect::TypeResolver<DriftComponentInfo>::get()->ExportJson(this, jsonNode, "");
		cJSON* driftArray = cJSON_CreateArray();
		cJSON_AddItemToObject(jsonNode, "DriftArray", driftArray);
		for (auto iter : m_DriftPropMap)
		{
			cJSON* driftJson = cJSON_CreateObject();
			cJSON_AddStringToObject(driftJson, "MeshName", iter.first.c_str());
			int geoType = iter.second.m_DriftGeometryType;
			WriteJSNode(driftJson, "DriftGeometryType", geoType);
			WriteJSNode(driftJson, "DriftCastShadow", iter.second.m_ShadowEnable);
			WriteJSNode(driftJson, "DriftCount", iter.second.m_DriftCount);
			WriteJSNode(driftJson, "DriftAreaRange", iter.second.m_AreaRange);
			WriteJSNode(driftJson, "DriftInnerRadius", iter.second.m_InnerRadius);
			WriteJSNode(driftJson, "DriftOuterRadius", iter.second.m_OuterRadius);
			WriteJSNode(driftJson, "DriftFadeInTimeOffset", iter.second.m_FadeInTimeOffset);
			WriteJSNode(driftJson, "DriftMinSize", iter.second.m_MinScale);
			WriteJSNode(driftJson, "DriftMaxSize", iter.second.m_MaxScale);
			WriteJSNode(driftJson, "DriftMinVelocity", iter.second.m_MinVelocity);
			WriteJSNode(driftJson, "DriftMaxVelocity", iter.second.m_MaxVelocity);
			WriteJSNode(driftJson, "DriftMinRotateAxis", iter.second.m_MinRotateAxis);
			WriteJSNode(driftJson, "DriftMaxRotateAxis", iter.second.m_MaxRotateAxis);
			WriteJSNode(driftJson, "DriftMinRotateSpeed", iter.second.m_MinRotateSpeed);
			WriteJSNode(driftJson, "DriftMaxRotateSpeed", iter.second.m_MaxRotateSpeed);
			cJSON_AddItemToArray(driftArray, driftJson);
		}
	}

	void DriftComponentInfo::ImportData(cJSON* jsonNode)
	{
		reflect::TypeResolver<DriftComponentInfo>::get()->ImportJson(this, jsonNode, "");
		cJSON* driftArray = cJSON_GetObjectItem(jsonNode, "DriftArray");
		int arraySize = cJSON_GetArraySize(driftArray);
		for (int i = 0; i < arraySize; ++i)
		{
			cJSON* driftJson = cJSON_GetArrayItem(driftArray, i);
			if (driftJson == nullptr)
				continue;
			Name meshName;
			ReadJSNode(driftJson, "MeshName", meshName);
			DriftPropData data;
			int geoType = 0;
			ReadJSNode(driftJson, "DriftGeometryType", geoType);
			data.m_DriftGeometryType = (DriftGeometryType)geoType;
			ReadJSNode(driftJson, "DriftCastShadow", data.m_ShadowEnable);
			ReadJSNode(driftJson, "DriftCount", data.m_DriftCount);
			ReadJSNode(driftJson, "DriftAreaRange", data.m_AreaRange);
			ReadJSNode(driftJson, "DriftInnerRadius", data.m_InnerRadius);
			ReadJSNode(driftJson, "DriftOuterRadius", data.m_OuterRadius);
			ReadJSNode(driftJson, "DriftFadeInTimeOffset", data.m_FadeInTimeOffset);
			ReadJSNode(driftJson, "DriftMinSize", data.m_MinScale);
			ReadJSNode(driftJson, "DriftMaxSize", data.m_MaxScale);
			ReadJSNode(driftJson, "DriftMinVelocity", data.m_MinVelocity);
			ReadJSNode(driftJson, "DriftMaxVelocity", data.m_MaxVelocity);
			ReadJSNode(driftJson, "DriftMinRotateAxis", data.m_MinRotateAxis);
			ReadJSNode(driftJson, "DriftMaxRotateAxis", data.m_MaxRotateAxis);
			ReadJSNode(driftJson, "DriftMinRotateSpeed", data.m_MinRotateSpeed);
			ReadJSNode(driftJson, "DriftMaxRotateSpeed", data.m_MaxRotateSpeed);
			m_DriftPropMap.emplace(meshName, data);
		}
	}

	void DriftComponentInfo::CopyTo(ActorComponentInfo* comInfo)
	{
		DriftComponentInfo*  driftInfo = static_cast<DriftComponentInfo*>(comInfo);
		driftInfo->m_Priority = m_Priority;
		driftInfo->m_DriftPropMap = m_DriftPropMap;
	}

	void DriftComponentInfo::Overlay(ActorComponentInfo* comInfo)
	{

	}

	void DriftComponentInfo::OverWrite()
	{
		if (mOriginComInfo != nullptr)
		{
			DriftComponentInfo* oriInfo = static_cast<DriftComponentInfo*> (mOriginComInfo);
			reflect::TypeResolver<DriftComponentInfo>::get()->overWrite(oriInfo, this);
			m_DriftPropMap = oriInfo->m_DriftPropMap;
		}
	}

#ifdef ECHO_EDITOR
	bool DriftComponentInfo::getNeedCollectedResource(std::unordered_multimap<ResourceType, std::string>& resMap)
	{
		if (m_DriftPropMap.empty()) {
			return false;
		}
		for (auto& iter : m_DriftPropMap)
		{
			if (iter.first.empty())
				continue;
			resMap.insert(std::make_pair(ResourceType::Mesh, iter.first.c_str()));
		}
		return true;
	}
#endif
}
