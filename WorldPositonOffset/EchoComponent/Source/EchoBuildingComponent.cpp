#include "EchoComponentStableHeaders.h"
#include "EchoBuildingComponent.h"
#include "EchoActorEntityComponent.h"
#include "EchoDynamicVideoParamManager.h"
#include "EchoComInfoDefineMacro.h"
#include "EchoEntityObject.h"
#include "EchoVideoConfigSystem.h"
#include "EchoWorldManager.h"
#include "EchoDistantBuildingComponent.h"

namespace Echo
{
	DECLAREACTORCOMTYPE_CPP(BuildingComponent); 
	EDITOR_REFLECT_BEGIN(BuildingComponentInfo)
	EDITOR_REFLECT_MEMBER_UPDATE(m_BuildingSimpleMeshName, "BuildingSimpleMeshName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_BuildingLodDistanceLevel1, "BuildingLodDistanceLevel1", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_BuildingLodDistanceLevel2, "BuildingLodDistanceLevel2", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_BuildingRenderDistance, "BuildingRenderDistance", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_BuildingLodFactorLevel1, "BuildingLodFactorLevel1", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_BuildingLodFactorLevel2, "BuildingLodFactorLevel2", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_BuildingRenderFactor, "BuildingRenderFactor", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_BuildingCullMode, "BuildingCullMode", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_BuildingUpdateLodByBlock, "BuildingUpdateLodByBlock", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_DistantBuildingComName, "DistantBuildingComponentName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_END();

	BuildingComponent::BuildingComponent(ActorPtr pActor, ActorCOMMoveableType type)
		:ProtocolActorComponent(pActor, type)
	{

	}

	BuildingComponent::~BuildingComponent()
	{

	}

	void BuildingComponent::init(ActorComponentInfo* info)
	{
		setActorComActiveType(ActorComActiveBits::RENDER_BIT);

		SetNotifiedUpdate(true);
		ProtocolActorComponent::init(info);

		m_pActorBuilding = new ActorBuilding(getSceneManager());
		m_pActorBuilding->setVisible(false);
		m_pActorBuilding->setName(Name(getCombinedName().append("||Building")));
		addChildToNode(m_pActorBuilding->getMainSceneNode());
		UpdateComRootNode(true);
		useBuildingRendering(true);

		if (ActorSystem::instance()->isHideRenderObject())
		{
			m_pActorBuilding->setVisible(false);
		}

		if (getActor() && !getActor()->isCreateFinish())
			getActor()->addActorLoadListener(this);

		if (m_pActComInfo)
		{
			BuildingComponentInfo* buildInfo = static_cast<BuildingComponentInfo*>(m_pActComInfo);

			setBuildingSimpleMesh(buildInfo->m_BuildingSimpleMeshName);
			setBuildingLodDistanceLevel1(buildInfo->m_BuildingLodDistanceLevel1);
			setBuildingLodDistanceLevel2(buildInfo->m_BuildingLodDistanceLevel2);
			setBuildingRenderDistance(buildInfo->m_BuildingRenderDistance);

			setBuildingCullMode(buildInfo->m_BuildingCullMode);
			setBuildingLodFactorLevel1(buildInfo->m_BuildingLodFactorLevel1);
			setBuildingLodFactorLevel2(buildInfo->m_BuildingLodFactorLevel2);
			setBuildingRenderFactor(buildInfo->m_BuildingRenderFactor);

			setBuildingUpdateLoadByBlock(buildInfo->m_BuildingUpdateLodByBlock);
		}
	}

	void BuildingComponent::connectCom()
	{
		if (m_pActorBuilding)
		{
#ifdef ECHO_EDITOR
			batchEntities();
#endif

			for (auto& iter : m_mapEntityComs)
			{
				if (iter.first)
					updateEntityComTransformImpl(iter.first);
				iter.second = 0;
			}
		}
		m_nTransformCnt = 0;

	}

	void BuildingComponent::disconnectCom()
	{
		removeAllEntityCom();
	}

	void BuildingComponent::unInit()
	{
		SetNotifiedUpdate(false);
		getActor()->removeActorLoadListener(this);
		RemoveChildFromNode(m_pActorBuilding->getMainSceneNode());
		SAFE_DELETE(m_pActorBuilding);
		m_mapEntityComs.clear();
	}

	void BuildingComponent::connectActor()
	{
		if (m_pActComInfo)
		{
			BuildingComponentInfo* buildInfo = static_cast<BuildingComponentInfo*>(m_pActComInfo);

			setBuildingCullMode(buildInfo->m_BuildingCullMode);
			setBuildingLodFactorLevel1(buildInfo->m_BuildingLodFactorLevel1);
			setBuildingLodFactorLevel2(buildInfo->m_BuildingLodFactorLevel2);
			setBuildingRenderFactor(buildInfo->m_BuildingRenderFactor);
		}

		if (getActor()->getParentActor() && getActor()->getParentActor()->getIsDistanceBuilding())
		{
			getActor()->setIsDistanceBuilding(true);
		}

		findDistantBuildingCom();
		if (!m_disBuildingCom.expired())
		{
			m_disBuildingCom->setBuildingCom(weak_from_this());
			setDistantBuildingComName(m_disBuildingCom->getCombinedName());
#ifdef ECHO_EDITOR
			m_disBuildingCom->m_IsUpdateByBlock = m_pActorBuilding->getUpdateLodByBlock();
#endif // ECHO_EDITOR
		}
	}

	void BuildingComponent::NotifiChildImp(int changeMask)
	{
		++m_nTransformCnt;
		m_bNeedUpdateTrans = true;

		if(m_bBuildingCullMode == DefaultCull)
		{
			for (int i = 0; i < 3; ++i)
				m_fLastBuildingLodLength[i] = 0.f;
			// 立即更新
			m_FrameTick = Root::instance()->getNextFrameNumber() - 30;
		}
	}

	void BuildingComponent::visibleStateChanged(bool visibleState)
	{
		if(m_pActorBuilding)
		{
			if (ActorSystem::instance()->isHideRenderObject())
			{
				m_pActorBuilding->setVisible(false);
				return;
			}
			if (isUsingBuildingRendering())
			{
				m_pActorBuilding->setVisible(visibleState);
			}
		}
	}

	void BuildingComponent::registerTickTimeBegin()
	{
		m_eventSet[TT_BeginFrame].insert(EID_EveryFrame);
		registerTickTimeEnd();
	}

	void BuildingComponent::beginFrame(EventID eventid, ProtocolHandle handle)
	{
		switch (eventid)
		{
		case EID_EveryFrame:
		{
			if (m_pActorBuilding && m_bBuildingCullMode == CameraCullMode::DefaultCull)
			{
				if (m_FrameTick == 0)
					m_FrameTick = Root::instance()->getNextFrameNumber();

				const unsigned long runFrame = Root::instance()->getNextFrameNumber() - m_FrameTick;
				if (runFrame >= 30)	// 每30帧判断一次videoConfig文件设置
				{
					m_FrameTick = Root::instance()->getNextFrameNumber();
					float res_1	= resetLastLodFactorDistance(0);
					float res_2	= resetLastLodFactorDistance(1);
					float res_3	= resetLastLodFactorDistance(2);

					if (res_1 != m_fLastBuildingLodLength[0])
					{
						m_fLastBuildingLodLength[0] = res_1;
						m_pActorBuilding->setLodDis(LOD_LEVEL_1, m_fLastBuildingLodLength[0]);
					}

					if (res_2 != m_fLastBuildingLodLength[1])
					{
						m_fLastBuildingLodLength[1] = res_2;
						m_pActorBuilding->setLodDis(LOD_LEVEL_2, m_fLastBuildingLodLength[1]);
					}

					if (res_3 != m_fLastBuildingLodLength[2])
					{
						m_fLastBuildingLodLength[2] = res_3;
						m_pActorBuilding->setRenderDistance(m_fLastBuildingLodLength[2]);
					}
				}
			}

			if (m_pActorBuilding && m_bNeedUpdateTrans)
			{
				for (auto& iter : m_mapEntityComs)
				{
					if (iter.first && iter.second > m_nTransformCnt)
						updateEntityComTransformImpl(iter.first);
					iter.second = 0;
				}
				m_nTransformCnt = 0;
				m_bNeedUpdateTrans = false;
			}

		}
		default:
			break;
		}
	}

	bool BuildingComponent::OnActorCreateFinish()
	{
		if (m_pActorBuilding == nullptr)
			return false;
		m_pActorBuilding->setInitFinish();
		return true;
	}

	AxisAlignedBox BuildingComponent::getLoadBoundingBox()
	{
		Vector3 v3Distance = { 1000.0f, 1000.0f, 1000.0f };

		BuildingComponentInfo* pInfo = nullptr;
		if (m_pActComInfo != nullptr)
			pInfo = static_cast<BuildingComponentInfo*>(m_pActComInfo);
		if (pInfo != nullptr)
		{
			if (pInfo->m_BuildingCullMode == CameraCullMode::DefaultCull)
			{
				if (pInfo->m_BuildingRenderFactor == m_fLastBuildingLodFactor[2] && m_fLastBuildingLodLength[2] != 0.f)
					v3Distance = Vector3(m_fLastBuildingLodLength[2]);
				else
				{
					float cullFactor = pInfo->m_BuildingRenderFactor;
					if (cullFactor <= std::numeric_limits<float>::epsilon())
					{
						VideoConfig* pVideoConfig = nullptr;
						if (getWorldManager())
							pVideoConfig = getWorldManager()->getVideoConfig();
						if (!pVideoConfig)
							pVideoConfig = VideoConfigSystem::instance()->getActiveVideoConfig();
						if (pVideoConfig)
							cullFactor = pVideoConfig->getCurrentVideoConfig()->renderHLODfactor * 0.9f;
					}
					AxisAlignedBox aabb = getActor()->getExactWorldLoadBounds();
					if (!aabb.isFinite())
						aabb = getActor()->getWorldBounds();
					float cullLength = aabb.getHalfSize().squaredLength() / cullFactor;
					v3Distance = Vector3(std::sqrt(cullLength));
				}
			}
			else
				v3Distance = Vector3(std::sqrt(pInfo->m_BuildingRenderDistance));
		}

		const Vector3& minBox = getWorldAABB().getMinimum();
		const Vector3& maxBox = getWorldAABB().getMaximum();
		AxisAlignedBox loadBox(minBox - v3Distance, maxBox + v3Distance);
		return loadBox;
	}

	void BuildingComponent::addEntityCom(const EchoWeakPtr<EntityComponent>& pEntityCom)
	{
		if (m_pActorBuilding == nullptr || pEntityCom.Expired())
			return;

		m_bBuildingControlling = true;
		pEntityCom->visibleStateChanged(false);
		m_bBuildingControlling = false;

		EntityObject* obj = pEntityCom->getEntityObject();

		if (obj && obj->m_pActorBuilding == m_pActorBuilding)
			return;

		auto iter = m_mapEntityComs.find(pEntityCom);
		if (iter != m_mapEntityComs.end())
			return;
		m_mapEntityComs.emplace(pEntityCom, 1);

		if (obj)
		{
			m_pActorBuilding->addBuildBlock(obj);
			//m_pActorBuilding->notifyFinishOne(obj);
			if (getActor() && getActor()->isCreateFinish())
			{
				m_pActorBuilding->setInitFinish();
			}
		}
	}

	void BuildingComponent::removeEntityCom(const EchoWeakPtr<EntityComponent>& pEntityCom, bool forceRemove)
	{
		if (m_pActorBuilding == NULL || pEntityCom.Expired())
			return;

		if (!forceRemove && !(m_ParentVisibleState == 0))
			return;

		EntityObject* obj = pEntityCom->getEntityObject();
		if (obj && obj->m_pActorBuilding != m_pActorBuilding)
			return;

		auto iter = m_mapEntityComs.find(pEntityCom);
		if (iter == m_mapEntityComs.end())
			return;
		m_mapEntityComs.erase(iter);

		if (obj)
		{
			m_pActorBuilding->removeBuildBlock(obj);
		}

		m_bBuildingControlling = true;
		pEntityCom->visibleStateChanged(pEntityCom->getRealVisibleState());
		m_bBuildingControlling = false;
	}

	void BuildingComponent::updateEntityComTransform(const EchoWeakPtr<EntityComponent>& pEntityCom)
	{
		if (m_pActorBuilding == NULL || pEntityCom.Expired())
			return;

		auto iter = m_mapEntityComs.find(pEntityCom);
		if (iter == m_mapEntityComs.end())
			return;

		++(iter->second);
		m_bNeedUpdateTrans = true;
	}

	bool BuildingComponent::hasEntityComponent(const EchoWeakPtr<EntityComponent>& pCom)
	{
		return m_mapEntityComs.find(pCom) != m_mapEntityComs.end();
	}

	void BuildingComponent::useBuildingRendering(bool use)
	{
		if (m_pActorBuilding == nullptr)
			return;

		if (m_bUsingBuildRender == use)
			return;
		m_bUsingBuildRender = use;

		m_bBuildingControlling = true;

		for (auto& iter : m_mapEntityComs)
		{
			if (iter.first)
				iter.first->setRuntimeVisible(!use);
		}

		m_pActorBuilding->setVisible(use);

		m_bBuildingControlling = false;
	}

	void BuildingComponent::setBuildingSimpleMesh(const String& meshName)
	{
		m_nMeshName = meshName;
		if (m_pActorBuilding)
		{
			if (!meshName.empty())
			{
				Name nMeshName(meshName);
				m_pActorBuilding->setSimpleMesh(nMeshName);
				m_pActorBuilding->setUseSimpleRender(false);
				m_pActorBuilding->setUseSimpleShadow(true);
			}
			else
			{
				m_pActorBuilding->setUseSimpleRender(true);
				m_pActorBuilding->setUseSimpleShadow(true);
			}
		}
	}

	const String& BuildingComponent::getBuildingSimpleMeshName()
	{
		return m_nMeshName;
	}

	void BuildingComponent::setBuildingLodDistanceLevel1(float val)
	{
		m_fBuildingLodDistanceLevel1 = val;
		if (m_pActorBuilding)
		{
			m_pActorBuilding->setLodDis(LOD_LEVEL_1, val);
		}
	}

	float BuildingComponent::getBuildingLodDistanceLevel1()
	{
		return m_fBuildingLodDistanceLevel1;
	}

	void BuildingComponent::setBuildingLodDistanceLevel2(float val)
	{
		m_fBuildingLodDistanceLevel2 = val;
		if (m_pActorBuilding)
		{
			m_pActorBuilding->setLodDis(LOD_LEVEL_2, val);
		}
	}

	float BuildingComponent::getBuildingLodDistanceLevel2()
	{
		return m_fBuildingLodDistanceLevel2;
	}

	void BuildingComponent::setBuildingRenderDistance(float distance)
	{
		m_fBuildingRenderDistance = distance;
		if (m_pActorBuilding)
		{
			m_pActorBuilding->setRenderDistance(distance);
		}
	}

	void BuildingComponent::setBuildingUpdateLoadByBlock(bool val)
	{
		if (m_pActorBuilding)
		{
			m_pActorBuilding->setUpdateLodByBlock(val);

#ifdef ECHO_EDITOR
			if (m_disBuildingCom.expired())
				findDistantBuildingCom();
			if (!m_disBuildingCom.expired())
				m_disBuildingCom->m_IsUpdateByBlock = m_pActorBuilding->getUpdateLodByBlock();
#endif // ECHO_EDITOR
		}
	}

	bool BuildingComponent::getBuildingUpdateLoadByBlock() const
	{
		if (m_pActorBuilding)
		{
			return m_pActorBuilding->getUpdateLodByBlock();
		}
		return false;
	}

	float BuildingComponent::getFactorCullLength()
	{
		if(m_fLastBuildingLodLength[2] == 0.f)
			m_fLastBuildingLodLength[2] = resetLastLodFactorDistance(2);
		return m_fLastBuildingLodLength[2];
	}

	void BuildingComponent::setBuildingLodFactorLevel1(float val)
	{
		setBuildingFactor(0, val);
	}

	void BuildingComponent::setBuildingLodFactorLevel2(float val)
	{
		setBuildingFactor(1, val);
	}

	void BuildingComponent::setBuildingRenderFactor(float val)
	{
		setBuildingFactor(2, val);
	}

	void BuildingComponent::setBuildingFactor(int id, float val)
	{
		if (val < std::numeric_limits<float>::epsilon())
			val = 0.f;
		m_fBuildingFactor[id] = val;
		if (m_fLastBuildingLodFactor[id] != val || m_fLastBuildingLodLength[id] == 0.f)
		{
			m_fLastBuildingLodFactor[id] = val;
			m_fLastBuildingLodLength[id] = 0.f;
			m_fLastBuildingLodLength[id] = resetLastLodFactorDistance(id);
			if (m_pActorBuilding && m_bBuildingCullMode == CameraCullMode::DefaultCull)
			{
				if (id == 2)
					m_pActorBuilding->setRenderDistance(m_fLastBuildingLodLength[id]);
				else if(id == 1)
					m_pActorBuilding->setLodDis(LOD_LEVEL_2, m_fLastBuildingLodLength[id]);
				else if (id == 0)
					m_pActorBuilding->setLodDis(LOD_LEVEL_1, m_fLastBuildingLodLength[id]);
			}
		}
	}

	void BuildingComponent::setDistantBuildingComName(const String& name)
	{
		if (m_pActComInfo)
			static_cast<BuildingComponentInfo*>(m_pActComInfo)->m_DistantBuildingComName = getActor()->getRelativeNameFromAbsoluteName(name);
	}

	void BuildingComponent::setBuildingCullMode(int mode)
	{
		m_bBuildingCullMode = mode;
		if (m_pActorBuilding && m_bBuildingCullMode == CameraCullMode::DefaultCull)
		{
			m_pActorBuilding->setRenderDistance(m_fLastBuildingLodLength[2]);
			m_pActorBuilding->setLodDis(LOD_LEVEL_2, m_fLastBuildingLodLength[1]);
			m_pActorBuilding->setLodDis(LOD_LEVEL_1, m_fLastBuildingLodLength[0]);
			m_FrameTick = 0;
		}
		else if (m_pActorBuilding && m_bBuildingCullMode != CameraCullMode::DefaultCull)
		{
			m_pActorBuilding->setRenderDistance(m_fBuildingRenderDistance);
			m_pActorBuilding->setLodDis(LOD_LEVEL_2, m_fBuildingLodDistanceLevel2);
			m_pActorBuilding->setLodDis(LOD_LEVEL_1, m_fBuildingLodDistanceLevel1);
		}
	}

	bool BuildingComponent::getIsUpdateByBlock()
	{
		if (m_pActorBuilding)
			return m_pActorBuilding->getUpdateLodByBlock();
		return false;
	}

	void BuildingComponent::updateCombinedName()
	{
		if (m_pActorBuilding == NULL)
			return;
		Echo::Name buildingName = Name(getCombinedName().append("||Building"));
		m_pActorBuilding->setName(buildingName);
	}

#ifdef  ECHO_EDITOR


	std::set<Name> collectNeedComTypeSet = { Name("EntityComponent")/*, Name("RoleComponent"),Name("HLODComponent")*/ };
	std::set<Name> exceptNeedChildTypeSet = { Name("BuildingComponent") };

	void BuildingComponent::batchEntities()
	{
		std::deque<ActorPtr> childActorList;
		for (auto& childActor : getActor()->getChildActors())
		{
			if (childActor.second.expired())
				continue;
			childActorList.push_back(childActor.second);
		}
		while (!childActorList.empty())
		{
			ActorPtr childActor = childActorList.front();
			childActorList.pop_front();
			childActor->setBuildingCom(weak_from_this());
			for (auto& coltype : collectNeedComTypeSet)
			{
				auto batchComs = childActor->getComponentsByType(coltype, false);
				for (auto& collectCom : batchComs)
				{
					if (collectCom.expired())
						continue;
					if (collectCom->getType() == COMPONENTTYPE(EntityComponent))
					{
						addEntityCom(collectCom->weak_from_this());
					}
					//else if (collectCom->getType() == COMPONENTTYPE(HLODComponent))
					//{
					//	HLODComponent* hlodComPtr = static_cast<HLODComponent*>(collectCom.getPointer());
					//	Entity* entity = hlodComPtr->getentityob();
					//	if (entity == nullptr)
					//	{
					//		continue;
					//	}
					//}
				}
			}

			bool cascade = true;
			for (auto& comType : exceptNeedChildTypeSet)
			{
				auto&& retCom = childActor->getFirstComByType(comType);
				if (!retCom.expired())
				{
					cascade = false;
					break;
				}
			}
			if (cascade)
			{
				for (auto& grandChildActor : childActor->getChildActors())
				{
					if (grandChildActor.second.expired())
						continue;
					childActorList.push_back(grandChildActor.second);
				}
			}
		}
	}

#endif //  ECHO_EDITOR

	void BuildingComponent::removeAllEntityCom()
	{
		if (m_pActorBuilding == NULL)
			return;

		if (!(m_ParentVisibleState == 0))
			return;

		for (auto& iter : m_mapEntityComs)
		{
			const EntityComponentPtr& pEntityCom = iter.first;
			if (pEntityCom.expired())
				continue;
			EntityObject* obj = pEntityCom->getEntityObject();
			if (obj == nullptr || obj->m_pActorBuilding != m_pActorBuilding)
				continue;

			m_pActorBuilding->removeBuildBlock(obj);

			m_bBuildingControlling = true;
			pEntityCom->visibleStateChanged(pEntityCom->getRealVisibleState());
			m_bBuildingControlling = false;
		}
		m_mapEntityComs.clear();
	}

	float BuildingComponent::resetLastLodFactorDistance(int i)
	{
		VideoConfig* pVideoConfig = nullptr;
		if (getWorldManager())
			pVideoConfig = getWorldManager()->getVideoConfig();
		if (!pVideoConfig)
			pVideoConfig = VideoConfigSystem::instance()->getActiveVideoConfig();
		if (!pVideoConfig) return 0.f;

		float distance = m_fLastBuildingLodLength[i];
		float defaultFactor = 0.f;
		bool changeLength = false;

		if (m_fBuildingFactor[i] <= std::numeric_limits<float>::epsilon())
		{
			if (i == 0)
				defaultFactor = pVideoConfig->getCurrentVideoConfig()->renderLOD1factor;
			else if (i == 1)
				defaultFactor = pVideoConfig->getCurrentVideoConfig()->renderLOD2factor;
			else
				defaultFactor = pVideoConfig->getCurrentVideoConfig()->renderHLODfactor * 0.9f;
			if (defaultFactor != m_fLastBuildingLodFactor[i])
			{
				m_fLastBuildingLodFactor[i] = defaultFactor;
				changeLength = true;
			}
		}

		if (m_fLastBuildingLodLength[i] <= std::numeric_limits<float>::epsilon())
			changeLength = true;

		if (changeLength)
		{
			AxisAlignedBox aabb = getActor()->getStaticObjectExactWorldLoadBounds();
			if (!aabb.isFinite())
				aabb = getActor()->getWorldBounds();
			distance = aabb.getHalfSize().squaredLength() / m_fLastBuildingLodFactor[i];
			distance = std::sqrt(distance);
		}

		return distance;
	}

	void BuildingComponent::updateEntityComTransformImpl(const EchoWeakPtr<EntityComponent>& pEntityCom)
	{
		if (m_pActorBuilding == NULL || pEntityCom.Expired())
			return;

		EntityObject* obj = pEntityCom->getEntityObject();
		if (obj)
		{
			m_pActorBuilding->updateLodBuildTransform(obj);
		}
	}

	void BuildingComponent::findDistantBuildingCom()
	{
		int num = 0;
		ActorPtr act = getActor();
		if (act.expired()) return;
		ActorComponentPtr com = act->getComponent("DistantBuildingComponent");
		String comname = "";
		if (m_pActComInfo)
			comname = static_cast<BuildingComponentInfo*>(m_pActComInfo)->m_DistantBuildingComName;

		if (!comname.empty())
			com = act->getActorComByRelativeName(comname).seleteCom;
		else
		{
			while (!act.expired() && act->getIsDistanceBuilding() && com.expired() && num < 10)
			{
				act = act->getParentActor();
				com = act->getComponent("DistantBuildingComponent");
				num++;
			}
		}

		if (num >= 10 || com.expired())
			LogManager::instance()->logMessage("[BuildingComponent] DistantBuildingComponent query error");
		else
			m_disBuildingCom = com.getDynamicCastWeakPtr<DistantBuildingComponent>();
	}

	BuildingComponentInfo::BuildingComponentInfo(const Name& type, ActorCOMMoveableType moveable)
		:ActorComponentInfo(type, moveable)
	{

	}

	BuildingComponentInfo::~BuildingComponentInfo()
	{

	}

#ifdef ECHO_EDITOR
	bool BuildingComponentInfo::getNeedCollectedResource(std::unordered_multimap<ResourceType, std::string>& resMap)
	{
		if (m_BuildingSimpleMeshName.empty()) {
			return false;
		}
		resMap.insert(std::make_pair(ResourceType::Mesh, m_BuildingSimpleMeshName));
		return true;
	}
#endif //ECHO_EDITOR

	_DEFINEINFO_VIRFUN_BASE(BuildingComponentInfo);
}
