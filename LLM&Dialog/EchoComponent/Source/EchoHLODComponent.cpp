// Only for windows
#include "EchoComponentStableHeaders.h"
#include "EchoHLODComponent.h"

#include "EchoImage.h"
#include "EchoEntity.h"
#include "EchoSubMesh.h"
#include "EchoMeshMgr.h"
#include "EchoActorEntityComponent.h"
#include "EchoDynamicVideoParamManager.h"
#include "EchoWorldManager.h"
#include "EchoVideoConfigSystem.h"
#include "EchoBuildingComponent.h"

namespace Echo
{
	// ----------------------------------- HLODComponent ----------------------------------- //

	DECLAREACTORCOMTYPE_CPP(HLODComponent)

	HLODComponent::HLODComponent(ActorPtr pActor, ActorCOMMoveableType type)
		:ProtocolActorComponent(pActor, type)
	{

	}

	HLODComponent::~HLODComponent()
	{

	}

	void HLODComponent::init(ActorComponentInfo* info)
	{
		ProtocolActorComponent::init(info);
		SetNotifiedUpdate(true);
		Echo::Name entityName;
		if (Root::instance()->getRuntimeMode() == Root::eRM_Editor)
		{
			entityName = Name(getCombinedName().append("||").append("HLODEntity"));
			//createReplaceHelper();
		}

		//GpuCull lod2 
		if (Root::instance()->m_bUseGpuPoolForHLOD)
		{
			if (info == nullptr)
			{
				return;
			}
			auto hlodInfo = static_cast<HLODComponentInfo*>(info);
			setCullMode(static_cast<CameraCullMode> (hlodInfo->m_CullMode));
			m_bCreateFinish = true;
			return;
		}

		EntityObjectDesc desc;
		desc.name = entityName;
		desc.pWorldMgr = getWorldManager();
		desc.pSceneMgr = getActor()->GetSceneManager();
		m_pEntityObject = new EntityObject(desc);
		m_pEntityObject->setEnableAutoLodMesh(false);
		m_pEntityObject->setIsHLODModel(true);
		m_pEntityObject->OnEntityCreateFinish = std::bind<void>(&HLODComponent::entityCreateFinished, this);

		if (info == nullptr)
		{
			return;
		}
		auto hlodInfo = static_cast<HLODComponentInfo*>(info);
		//Name helperName;
		//if (Echo::Root::instance()->getRuntimeMode() == Root::eRM_Editor)
		//	helperName = Name(getCombinedName() + "||" + "Helper");
		//m_pHLODObject = new HelperObject(helperName, HelperObjType_Box, false);
		//m_pHLODObject->setRenderType(ECHO_LINE_STRIP);
		//m_pHLODObject->setHelperColorType(HelperObjColorType_Green);
		//addChildToNode(m_pHLODObject->getSceneNode());
		if (!hlodInfo->m_MeshName.empty())
			loadEntity(Name(hlodInfo->m_MeshName), getActor()->isSyncLoadActor());
		else
			m_bCreateFinish = true;//如果有info资源且没有资源路径，认为创建完成
	}

	void HLODComponent::unInit()
	{
		SetNotifiedUpdate(false);
		ProtocolActorComponent::unInit();
		m_ActiveListenerSet.clear();
		if(m_pEntityObject)
			m_pEntityObject->clearAllEntity();
		SAFE_DELETE(m_pEntityObject);
		destroyReplaceHelper();
		useHLOD(false);
		SAFE_DELETE(m_pBuildingCreateListener);
	}

	void HLODComponent::bindNode()
	{
		// m_bAttachNode = true;
		if (m_pParentNodeCom.expired())
			return;
		if (m_pEntityObject != nullptr)
			addChildToNode(m_pEntityObject->getParentSceneNode());
		if (m_pReplaceObject && m_pReplaceObject->getSceneNode() && !m_pReplaceObject->getSceneNode()->getParent())
			addChildToNode(m_pReplaceObject->getSceneNode());
		m_pParentNodeCom->UpdateComRootNode(true);

		if (m_pActComInfo)
		{
			auto hlodInfo = static_cast<HLODComponentInfo*>(m_pActComInfo);

			setHLODRenderDis(hlodInfo->m_HLODDist);
			
			if (hlodInfo->m_DissolveFactor > std::numeric_limits<float>::epsilon())
			{
				setDissolveFactor(hlodInfo->m_DissolveFactor);
			}
			setRenderDistance(hlodInfo->m_CullDistance);
		}
	}

	void HLODComponent::disconnectCom()
	{
		setRelateColliderComponent(nullptr);
		if (m_pParentNodeCom.expired())
			return;
		if (m_pEntityObject != NULL)
			RemoveChildFromNode(m_pEntityObject->getParentSceneNode());
		if (m_pReplaceObject != nullptr)
			RemoveChildFromNode(m_pReplaceObject->getSceneNode());
		m_pParentNodeCom->UpdateComRootNode(true);
	}

	void HLODComponent::connectActor()
	{
		if (m_pActComInfo)
		{
			auto hlodInfo = static_cast<HLODComponentInfo*>(m_pActComInfo);
			setRelateColliderComName(hlodInfo->m_RelateColliderComName);
			if (getRelateColliderComName().empty())
			{
				setRelateColliderComponent(nullptr);
			}
			else if (m_pRelateColliderComponent == nullptr && getActor())
			{
				setRelateColliderComponent(getActor()->getComponent(m_strRelateColliderComName));
			}
			setHLODFactor(hlodInfo->m_HLODFactor);
			setCullFactor(hlodInfo->m_CullFactor);
		}

		{
			if (getActor()->getParentActor() && getActor()->getParentActor()->getIsDistanceBuilding())
			{
				getActor()->setIsDistanceBuilding(true);
				if (Root::instance()->m_bUseGpuPoolForHLOD /*&& getActor()->isStaticObject()*/)
				{
					if (getActor()->isStaticObject())
					{
						getActor()->setRootStaticObject(true);
						getActor()->setWholeLoad(true);
					}
					//setListener
					if (!m_pBuildingCreateListener)
					{
						m_pBuildingCreateListener = new BuildingCreateListener(this);
						getActor()->addActorLoadListener(m_pBuildingCreateListener);
					}
				}
			}
		}
	}

	void HLODComponent::visibleStateChanged(bool visibleState)
	{
		if (!ActorSystem::instance()->getMainActorManager()->IsEditorPlay())
			return;
		useHLOD(visibleState);

		//目前只有调试时才会使值不同，这里同步一下
		if (!ActorSystem::instance()->isHideRenderObject() && getHLODVisible() != m_bShowHlodMesh)
			setHLODVisible(m_bShowHlodMesh);
	}

	void HLODComponent::NotifiChildImp(int changeMask)
	{
		if (getCullMode() == DefaultCull)
		{
			mLastHLODLength[0] = 0.f;
			mLastHLODLength[1] = 0.f;
			mLastHlodDissolveLength = 0.f;

			// 立即更新
			m_FrameTick = Root::instance()->getNextFrameNumber() - 30;
		}
	}

	void HLODComponent::registerTickTimeBegin()
	{
		m_eventSet[TT_BeginFrame].insert(EID_EveryFrame);
		registerTickTimeEnd();
	}

	void HLODComponent::beginFrame(EventID eventid, ProtocolHandle handle)
	{
		switch (eventid)
		{
		case EID_EveryFrame:
		{
			if (!m_bEnable)
				return;

			const AxisAlignedBox& aabb = getBoudingBox();
			bool distanceCheck = true;
			float curHlodDistSq[3];
			calCullDistance(curHlodDistSq);

			float minSqureDis = FLT_MAX;
			for (auto& pageManager : getActor()->m_pPagedManager)
			{
				for (size_t i = 0; i < Target_Component_Cnt; ++i)
				{
					const DVector3 currPos = pageManager? pageManager->getTargetComponentPosition(Target_Component_Type(i)) : getSceneManager()->getMainCamera()->getPosition();
					float curSqureDis = aabb.SquareDistPointToAABB(currPos);
					minSqureDis = std::min(minSqureDis, curSqureDis);
					if (minSqureDis < curHlodDistSq[0] || minSqureDis > curHlodDistSq[2])
						distanceCheck = false;
				}
			}
			m_BuildingCullSq = curHlodDistSq[2];

			if (distanceCheck || (ActorSystem::instance()->getWaitActorBuildingFinish() && !m_pActor->isChildCreateFinish()))
			{
				useHLOD(true);
				if (distanceCheck)
				{
					if (minSqureDis > curHlodDistSq[1])
					{
						auto factor = (minSqureDis - curHlodDistSq[1]) / (curHlodDistSq[2] - curHlodDistSq[1]);
						setDissolveStrength(factor);
					}
					else
					{
						setDissolveStrength(0.0f);
					}
				}
			}
			else
			{
				useHLOD(false);
			}
		}
		default:
			break;
		}
		

	}

	void HLODComponent::updateBuildLod2Distence()
	{
		if (Root::instance()->m_bUseGpuPoolForHLOD && !m_BuildingComp.expired())
		{
			if (m_BuildingCullSq < std::numeric_limits<float>::epsilon())
			{
				float curHlodDistSq[3];
				calCullDistance(curHlodDistSq);
				m_BuildingCullSq = curHlodDistSq[2];
			}
			BuildingComponent* pBuildCompt = static_cast<BuildingComponent*>(m_BuildingComp.getPointer());
			pBuildCompt->setBuildingRenderDistance(std::sqrt(m_BuildingCullSq));
		}
	}

	void HLODComponent::calCullDistance(float curHlodDistSq[3])
	{
		VideoConfig* pVideoConfig = nullptr;
		if (getWorldManager())
			pVideoConfig = getWorldManager()->getVideoConfig();
		if (!pVideoConfig)
			pVideoConfig = VideoConfigSystem::instance()->getActiveVideoConfig();
		float fLodDisScale = DynamicVideoParamManager::instance()->getLodDisScale();
		if (pVideoConfig)
			fLodDisScale *= pVideoConfig->getSceneLodDisScale();
		float curHlodDist = m_fHLODDist * fLodDisScale;

		curHlodDistSq[0] = curHlodDist * curHlodDist;
		curHlodDistSq[1] = m_fDissolveDistSq;
		curHlodDistSq[2] = m_fCullDistSq;

		if (getCullMode() == CameraCullMode::DefaultCull)
		{
			if (m_FrameTick == 0)
				m_FrameTick = Root::instance()->getNextFrameNumber();
			const unsigned long runFrame = Root::instance()->getNextFrameNumber() - m_FrameTick;
			if (runFrame >= 30)
			{
				m_FrameTick = Root::instance()->getNextFrameNumber();
				resetLastHLODLength(pVideoConfig);
			}
			curHlodDistSq[0] = mLastHLODLength[0];
			//curHlodDistSq[1] = mLastHlodDissolveLength;
			curHlodDistSq[1] = mLastHLODLength[1];
			curHlodDistSq[2] = mLastHLODLength[1];
		}
	}

	void HLODComponent::EditorPlayImp()
	{
		useHLOD(true);
	}

	void HLODComponent::EditorPauseImp(bool pause)
	{

	}

	void HLODComponent::EditorStopImp()
	{
		if (!isEnable())
			return;
		useHLOD(false);
		setHLODVisible(true);
		setDissolveStrength(0.0f);
	}

	void HLODComponent::setHLODVisible(bool isVisible)
	{
		if (m_bHLODMeshExist)
		{
			if (m_pEntityObject != nullptr)
			{
				if (ActorSystem::instance()->isHideRenderObject())
				{
					m_pEntityObject->setVisible(false);
				}
				else
				{
					m_pEntityObject->setVisible(isVisible);
				}
			}
		}
		else
		{
			if (m_pReplaceObject != nullptr)
			{
				if (ActorSystem::instance()->isHideRenderObject())
				{
					m_pReplaceObject->setVisible(false);
				}
				else
				{
					m_pReplaceObject->setVisible(isVisible);
				}
			}
		}

		if (m_pRelateColliderComponent)
		{
			m_pRelateColliderComponent->setRuntimeVisible(isVisible);
		}

		for (auto& iter : m_ActiveListenerSet)
		{
			iter->HLODComActiveCallBack(isVisible);
		}
	}

	bool HLODComponent::getHLODVisible()
	{
		if (Root::instance()->m_bUseGpuPoolForHLOD)
		{
			return m_bShowHlodMesh;
		}
		if (m_pEntityObject == nullptr)
			return false;
		return m_pEntityObject->getVisible();
	}

	// ============================================ TODO =====================================================
	void HLODComponent::updateCombinedName()
	{

		Echo::Name entityName = Name(getCombinedName().append("||").append("HLODEntity"));
		Echo::Name helperName = Name(getCombinedName().append("||").append("HLODReplaceObject"));
		if (m_pEntityObject != NULL)
			m_pEntityObject->setName(entityName);
		if (m_pReplaceObject != NULL)
			m_pReplaceObject->setName(helperName);
	}

	CameraCullMode HLODComponent::getCullMode()
	{
		return (CameraCullMode)m_fCullMode;
	}

	void HLODComponent::setCullMode(int mode)
	{
		m_fCullMode = mode;
		if (mode == CameraCullMode::DefaultCull)
		{
			if (mLastHLODLength[1] <= std::numeric_limits<float>::epsilon())
				resetLastHLODLength();
		}
		else
		{
			if (m_pEntityObject != NULL)
				m_pEntityObject->setRenderingDistance(std::sqrt(m_fCullDistSq));
			if (m_pReplaceObject != NULL)
				m_pReplaceObject->SetRanderingDistance(std::sqrt(m_fCullDistSq));
		}
		m_FrameTick = 0;
	}

	float HLODComponent::getRenderDistance()
	{
		return std::sqrt(m_fCullDistSq);
	}

	void HLODComponent::setRenderDistance(float dis)
	{
		if (m_fCullMode == CameraCullMode::DistanceCull)
		{
			if (m_pEntityObject != NULL)
				m_pEntityObject->setRenderingDistance(dis);
			if (m_pReplaceObject != NULL)
				m_pReplaceObject->SetRanderingDistance(dis);
		}
		m_fCullDistSq = dis * dis;
		m_fDissolveDist = dis * m_fDissolveFactor;
		m_fDissolveDistSq = m_fDissolveDist * m_fDissolveDist;
	}

	float HLODComponent::getTODRenderDis()
	{
		if (m_pEntityObject == NULL)
			return 0.f;
		return m_pEntityObject->getRenderingDistanceForTOD();
	}

	void HLODComponent::setTODRenderDis(float dis)
	{
		if (m_pEntityObject == NULL)
			return;
		m_pEntityObject->setRenderingDistanceForTOD(dis);
	}

	void HLODComponent::setHLODRenderDis(float dis)
	{
		m_fHLODDist = dis;
		//m_fHLODDistSq = m_fHLODDist * m_fHLODDist;
	}

	float HLODComponent::getHLODRenderDis()
	{
		/*if (getCullMode() == CameraCullMode::DefaultCull)
			return std::sqrt(mLastHLODLength[0]);*/
		return m_fHLODDist;
	}

	float HLODComponent::getFactorCullLength()
	{
		if (mLastHLODLength[1] == 0.f)
			resetLastHLODLength();
		return std::sqrt(mLastHLODLength[1]);
	}

	void HLODComponent::setDissolveRenderDis(float dis)
	{
		m_fDissolveDist = dis;
		m_fDissolveDistSq = m_fDissolveDist * m_fDissolveDist;
	}

	float HLODComponent::getDissolveRenderDis()
	{
		/*if (getCullMode() == CameraCullMode::DefaultCull)
			return std::sqrt(mLastHLODLength[1]) * m_fDissolveFactor;*/
		return m_fDissolveDist;
	}

	void HLODComponent::setDissolveStrength(float val)
	{
		if (val > 1.0f)
			val = 1.0f;
		if (val < 0.0f)
			val = 0.0f;

		if (Math::FloatEqual(mDissolveStrength, val))
			return;

		if (m_pEntityObject)
		{
			mDissolveStrength = val;
			m_pEntityObject->setStippleTransparency(val);
		}
	}

	void HLODComponent::setCastShadow(bool isEnable)
	{
		if (m_pEntityObject == NULL)
			return;
		m_pEntityObject->enableCastShadow(isEnable);
	}

	bool HLODComponent::getCastShadow()
	{
		if (m_pEntityObject == NULL)
			return false;
		return m_pEntityObject->IsCastShadow();
	}

	void HLODComponent::setLodDisLevel1(float val)
	{
		if (m_pEntityObject == NULL)
			return;
		m_pEntityObject->setLodDis(LOD_LEVEL_1, val);
	}

	float HLODComponent::getLodDisLevel1()
	{
		if (m_pEntityObject == NULL)
			return 0.0f;
		return m_pEntityObject->getLodDis(LOD_LEVEL_1);
	}

	void HLODComponent::setLodDisLevel2(float val)
	{
		if (m_pEntityObject == NULL)
			return;
		m_pEntityObject->setLodDis(LOD_LEVEL_2, val);
	}

	float HLODComponent::getLodDisLevel2()
	{
		if (m_pEntityObject == NULL)
			return 0.0f;
		return m_pEntityObject->getLodDis(LOD_LEVEL_2);
	}

	void HLODComponent::setEnityFpsState(const int& state_)
	{
		if (m_pEntityObject == nullptr)
			return;
		ENUM_FP_STATE stateTemp = static_cast<ENUM_FP_STATE>(state_);
		m_pEntityObject->setFPState(stateTemp);
	}

	bool HLODComponent::mergeHLOD(HLODMergeSettings& settings)
	{
		auto pm = ProtocolComponentSystem::instance();
		if (!pm)
			return false;
		auto result =  pm->callProtocolFunc(Name("MergeHLOD"), this, (ProtocolHandle)&settings);
		if (result == PR_OK)
			return true;
		else
			return false;
	}

	void HLODComponent::addHLODComActiveListener(HLODComActiveListener* listener)
	{
		m_ActiveListenerSet.emplace(listener);
	}


	void HLODComponent::removeHLODComActiveListener(HLODComActiveListener* listener)
	{
		m_ActiveListenerSet.erase(listener);
	}
	// =============================================== FEATURE =======================================================

	void HLODComponent::setEntityPath(const Name& path)
	{
		mEntityPath = path;
	}

	void HLODComponent::useHLOD(bool use)
	{
		if (!isEnable() || m_bShowHlodMesh == use)
		{
			return;
		}
		if (ActorSystem::instance()->isHideRenderObject())
		{
			setHLODVisible(false);
			return;
		}
		m_bShowHlodMesh = use;

		setHLODVisible(m_bShowHlodMesh);
		//not gpu cull : pass
		//gpu cull && !use : pass  (When "use" is set to "false", it indicates that the building should be displayed, even when using gpucull)
		if (!Root::instance()->m_bUseGpuPoolForHLOD || !use)
		{
			if (!getActor().expired())
			{
				for (auto& childActor : getActor()->getChildActors())
				{
					auto&& rootCom = childActor.second->getRootNodeCom();
					if (rootCom.expired())
						continue;
					rootCom->setRuntimeVisible(!use);
				}
			}

		}

		if (Root::instance()->m_bUseGpuPoolForHLOD && use && !m_BuildingComp.expired())
		{
			updateBuildLod2Distence();
		}

		if(!use)
			setDissolveStrength(0.0f);

	//	const ActorPtr& ParentActor = getActor();
	//	const std::vector<ActorPtr>& vectorPtr = ParentActor->getChildActors();
	//	for (const ActorPtr& item : vectorPtr)
	//	{
	//		item->setVisiable(!visible);
	//	}
	//	// Get the EntityComponent from the current Root Component
	//	// TODO: 
	//	{
	//		std::vector<ActorComponentPtr> ComPtrList = getComponentsByType(Name("EntityComponent"), true);
	//		for (ActorComponentPtr& ptr : ComPtrList)
	//		{
	//			EntityComponent* p = dynamic_cast<EntityComponent*>(ptr.get());
	//			// Entity* item;
	//			//if (p->m_pEntityObject)
	//			//{
	//			//	p->m_pEntityObject->setVisible(!visible);
	//			//}
	//			///*if (p->getEntity())
	//			//{
	//			//	p->getEntity()->setVisible(!visible);
	//			//}*/
	//			p->setVisible(!visible);
	//		}
	//	}
	}

	void HLODComponent::enableHLOD(bool enable)
	{
		m_bEnable = enable;
	}

	bool HLODComponent::isEnable() const
	{
		return m_bEnable;
	}

	void HLODComponent::loadEntity(const Name& path, bool synchronous)
	{
		if (Root::instance()->m_bUseGpuPoolForHLOD)
		{
			if (m_pActComInfo != nullptr)
			{
				auto hlodInfo = static_cast<HLODComponentInfo*>(m_pActComInfo);

				setCullMode(static_cast<CameraCullMode> (hlodInfo->m_CullMode));
				setRenderDistance(hlodInfo->m_CullDistance);
				setTODRenderDis(hlodInfo->m_TodCullDistance);
				setDissolveStrength(hlodInfo->m_DissolveStrength);
				setEnityFpsState(hlodInfo->m_FpsState);
				setLodDisLevel1(hlodInfo->m_LodDis_Level1);
				setLodDisLevel2(hlodInfo->m_LodDis_Level2);
				setCastShadow(hlodInfo->m_CastShadow);
				hlodMeshExist(true);
				enableHLOD(true);
			}

			return;
		}

		if (path.empty())
			return;


		mEntityPath = path;
		m_bCreateFinish = false; // 加载前需要把创建完成改成false;
		m_pEntityObject->clearAllEntity();
		// 这里需要传空来清空掉一些未完全清除的信息
		// m_pEntityObject->changeEntity(Name(), synchronous);
		m_pEntityObject->changeEntity(path, synchronous);
		updateCombinedName();
		if (m_pActComInfo != nullptr)
		{
			auto hlodInfo = static_cast<HLODComponentInfo*>(m_pActComInfo);

			setCullMode(static_cast<CameraCullMode> (hlodInfo->m_CullMode));
			setRenderDistance(hlodInfo->m_CullDistance);
			setTODRenderDis(hlodInfo->m_TodCullDistance);
			setDissolveStrength(hlodInfo->m_DissolveStrength);
			setEnityFpsState(hlodInfo->m_FpsState);
			setLodDisLevel1(hlodInfo->m_LodDis_Level1);
			setLodDisLevel2(hlodInfo->m_LodDis_Level2);
			setCastShadow(hlodInfo->m_CastShadow);
		}
#ifdef ECHO_EDITOR
		m_pEntityObject->setQueryFlags(m_QtFlags);
#endif
		// m_pEntityObject->setVisible(false);

		if (synchronous)
		{
			if (m_pEntityObject->getLowestEntity() == nullptr)
				hlodMeshExist(false);
			else
				hlodMeshExist(true);
			enableHLOD(true);
			useHLOD(ActorSystem::instance()->getMainActorManager()->IsEditorPlay());
		}
		if (Root::instance()->getRuntimeMode() == Root::eRM_Client)
		{
			m_pEntityObject->setVisible(m_bShowHlodMesh);
		}
	}

	void HLODComponent::createEntity(EM_LOD_LEVEL lodLevel, const MeshPtr& pMesh)
	{
		if (pMesh.isNull())
			return;
		if(m_pEntityObject)
			m_pEntityObject->createEntity(lodLevel, pMesh);
	}

	void HLODComponent::createReplaceHelper()
	{
		if (m_pReplaceObject)
			return;
		Echo::Name helperName;
		if (Root::instance()->getRuntimeMode() == Root::eRM_Editor)
			helperName = Name(getCombinedName().append("||").append("HLODReplaceObject"));
		m_pReplaceObject = new HelperObject(helperName);
		m_pReplaceObject->setHelperColorType(HelperObjColorType_Red);
		m_pReplaceObject->setRenderType(ECHO_LINE_STRIP);
		m_pReplaceObject->setVisible(false);
	}

	void HLODComponent::destroyReplaceHelper()
	{
		SAFE_DELETE(m_pReplaceObject);
	}

	void HLODComponent::showNormalMap(bool bShowNormalMap)
	{
		if (m_pEntityObject != NULL)
		{
			SubMesh* subMesh = m_pEntityObject->getEntity()->getMesh()->getSubMesh(0);
			const MaterialWrapper& pMaterial = subMesh->getMaterialWrapper();

			if (pMaterial.isV1())
			{
				Material* pMatV1 = pMaterial.getV1();
				if (bShowNormalMap)
				{
					pMatV1->m_matInfo.genmask = "%BUMP_MAP%PBR_SIMPLE";
				}
				else
				{
					pMatV1->m_matInfo.genmask = "%PBR_SIMPLE";
				}
				pMatV1->LoadShaderConfig(pMatV1->m_matInfo.shadername.c_str());
				pMatV1->rebuildMaterial();
			}
			else if (pMaterial.isV2())
			{
				MaterialV2* pMatV2 = pMaterial.getV2();
				std::vector<Name> newVariantMask;
				if (bShowNormalMap)
				{
					newVariantMask.emplace_back(Name("BUMP_MAP"));
					newVariantMask.emplace_back(Name("PBR_SIMPLE"));
				}
				else
				{
					newVariantMask.emplace_back(Name("PBR_SIMPLE"));
				}
				pMatV2->updateVariantMask(newVariantMask);
			}
		}
	}

	void HLODComponent::setHLODFactor(float factor)
	{
		if (factor < std::numeric_limits<float>::epsilon())
			factor = 0.f;
		m_fHLODFactor = factor;
		if (mLastHLODFactor[0] != factor || mLastHLODLength[0] == 0.f)
		{
			mLastHLODFactor[0] = factor;
			mLastHLODLength[0] = 0.f;
			resetLastHLODLength();
		}
	}

	void HLODComponent::setDissolveFactor(float factor)
	{
		if (factor < std::numeric_limits<float>::epsilon())
			factor = std::numeric_limits<float>::epsilon();
		m_fDissolveFactor = factor;
		//AxisAlignedBox aabb = getWorldAABB();
		//setDissolveRenderDis((aabb.getSize() * m_fDissolveFactor).Length());
		m_fDissolveDist = getRenderDistance() * m_fDissolveFactor;
		m_fDissolveDistSq = m_fDissolveDist * m_fDissolveDist;
	}

	void HLODComponent::setCullFactor(float factor)
	{
		if (factor < std::numeric_limits<float>::epsilon())
			factor = 0.f;
		m_fCullFactor = factor;
		if(mLastHLODFactor[1] != factor || mLastHLODLength[1] == 0.f)
		{
			mLastHLODFactor[1] = factor;
			mLastHLODLength[1] = 0.f;
			mLastHlodDissolveLength = 0.f;
			resetLastHLODLength();
		}
	}

	void HLODComponent::setReplaceBox(const AxisAlignedBox& box)
	{
		m_replaceBox = box;
		if (m_pReplaceObject != nullptr)
		{
			m_pReplaceObject->setScale(box.getSize());
			m_pReplaceObject->setPosition(box.getCenter());
		}
		UpdateComRootNode(true, true);
	}

	void HLODComponent::hlodMeshExist(bool exist)
	{
		m_bHLODMeshExist = exist;
		if(m_pEntityObject != nullptr)
			m_pEntityObject->setVisible(exist);
#ifdef  ECHO_EDITOR
		if (exist)
		{
			destroyReplaceHelper();
		}
		else if (m_pReplaceObject == nullptr)
		{
			createReplaceHelper();
			if (m_pReplaceObject && m_pReplaceObject->getSceneNode() && !m_pReplaceObject->getSceneNode()->getParent())
				addChildToNode(m_pReplaceObject->getSceneNode());
			UpdateComRootNode(true);
		}
#endif //  ECHO_EDITOR
		if (!m_bHLODMeshExist)
		{
			std::string msg = "hlod mesh file :" + mEntityPath.toString() + " not exists!!!";
			LogManager::instance()->logMessage(LML_CRITICAL, msg.c_str());
		}
	}

	void HLODComponent::setRelateColliderComName(const String& strVal)
	{
		m_strRelateColliderComName = strVal;
	}

	const String& HLODComponent::getRelateColliderComName() const
	{
		return m_strRelateColliderComName;
	}

	AxisAlignedBox HLODComponent::getBoudingBox()
	{
		AxisAlignedBox aabb = getActor()->getExactWorldLoadBounds();
		if (!aabb.isFinite())
			aabb = getActor()->getWorldBounds();
		return aabb;
	}

	void HLODComponent::resetLastHLODLength(VideoConfig* vd)
	{
		VideoConfig* pVideoConfig = vd;
		if (!pVideoConfig)
		{
			if (getWorldManager())
				pVideoConfig = getWorldManager()->getVideoConfig();
			if (!pVideoConfig)
				pVideoConfig = VideoConfigSystem::instance()->getActiveVideoConfig();
		}

		if (!pVideoConfig) return;

		bool changeLength[2] = { false,false };
		float hlodFactor[2] = { 0.f, 0.f };
		float dissolveFactor = 0.f;
		if (m_fHLODFactor <= std::numeric_limits<float>::epsilon())
		{
			if (pVideoConfig)
				hlodFactor[0] = pVideoConfig->getCurrentVideoConfig()->renderHLODfactor;
			if (mLastHLODFactor[0] != hlodFactor[0])
			{
				mLastHLODFactor[0] = hlodFactor[0];
				changeLength[0] = true;
			}
		}

		if (m_fCullFactor <= std::numeric_limits<float>::epsilon())
		{
			if (pVideoConfig)
				dissolveFactor = pVideoConfig->getCurrentVideoConfig()->renderImposterfactor;
			hlodFactor[1] = dissolveFactor * 0.9f;
			if (mLastHLODFactor[1] != hlodFactor[1])
			{
				mLastHLODFactor[1] = hlodFactor[1];
				changeLength[1] = true;
			}
		}
		
		if(mLastHLODLength[0] <= std::numeric_limits<float>::epsilon())
			changeLength[0] = true;
		if (mLastHLODLength[1] <= std::numeric_limits<float>::epsilon())
			changeLength[1] = true;

		if (changeLength[0] || changeLength[1])
		{
			Vector3 aabbSize = getBoudingBox().getHalfSize();

			if (changeLength[0])
				mLastHLODLength[0] = aabbSize.squaredLength() / mLastHLODFactor[0];
			if (changeLength[1])
			{
				mLastHLODLength[1] = aabbSize.squaredLength() / mLastHLODFactor[1];
				mLastHlodDissolveLength = aabbSize.squaredLength() / dissolveFactor;
			}
			if (changeLength[1])
			{
				if (m_pEntityObject != NULL)
					m_pEntityObject->setRenderingDistance(std::sqrt(mLastHLODLength[1]));
				if (m_pReplaceObject != NULL)
					m_pReplaceObject->SetRanderingDistance(std::sqrt(mLastHLODLength[1]));
			}
		}
	}

	void HLODComponent::showEditorHelper(bool isShow)
	{
		if (m_pReplaceObject != nullptr)
			m_pReplaceObject->setVisible(isShow);
	}

	bool HLODComponent::isCreateFinish()
	{
		return m_bCreateFinish;
	}

	Entity* HLODComponent::getLowestEntity()
	{
		if(m_pEntityObject == nullptr)
			return nullptr;
		return m_pEntityObject->getLowestEntity();
	}

	AxisAlignedBox HLODComponent::getLoadBoundingBox()
	{
		Vector3 v3Distance = { 1000.0f, 1000.0f, 1000.0f };

		HLODComponentInfo* pInfo = nullptr;
		if (m_pActComInfo != nullptr)
			pInfo = static_cast<HLODComponentInfo*>(m_pActComInfo);
		if (pInfo != nullptr)
		{
			if (getCullMode() == CameraCullMode::DefaultCull)
			{
				if (pInfo->m_CullFactor == mLastHLODFactor[1] && mLastHLODLength[1] != 0.f)
					v3Distance = Vector3(std::sqrt(mLastHLODLength[1]));
				else
				{
					float cullFactor = pInfo->m_CullFactor;
					if (cullFactor <= std::numeric_limits<float>::epsilon())
					{
						VideoConfig* pVideoConfig = nullptr;
						if (getWorldManager())
							pVideoConfig = getWorldManager()->getVideoConfig();
						if (!pVideoConfig)
							pVideoConfig = VideoConfigSystem::instance()->getActiveVideoConfig();
						if (pVideoConfig)
							cullFactor = pVideoConfig->getCurrentVideoConfig()->renderImposterfactor * 0.9f;
					}
					float cullLength = getBoudingBox().getHalfSize().squaredLength() / cullFactor;
					v3Distance = Vector3(std::sqrt(cullLength));
				}
			}
			else
				v3Distance = Vector3(std::sqrt(pInfo->m_CullDistance));
		}

		const Vector3& minBox = getWorldAABB().getMinimum();
		const Vector3& maxBox = getWorldAABB().getMaximum();
		AxisAlignedBox loadBox(minBox - v3Distance, maxBox + v3Distance);
		return loadBox;
	}

	void HLODComponent::entityCreateFinished()
	{
		if (m_pEntityObject == nullptr || m_pEntityObject->getLowestEntity() == nullptr)
			hlodMeshExist(false);
		else
			hlodMeshExist(true);
		UpdateComRootNode(true);

		m_bCreateFinish = true;
		if (!m_pActor.expired())
		{
			m_pActor->notifyComponentFinish(weak_from_this());
		}

		if (m_bHLODMeshExist)
		{
			setHLODFactor(getHLODFactor());
			setHLODRenderDis(m_fHLODDist);
			if (getDissolveFactor() > std::numeric_limits<float>::epsilon())
				setDissolveFactor(getDissolveFactor());
			else
				setDissolveRenderDis(getDissolveRenderDis());
			setCullFactor(getCullFactor());
			setRenderDistance(getRenderDistance());
		}
		else if (Root::instance()->getRuntimeMode() == Root::eRM_Client)
		{
			useHLOD(false);
			enableHLOD(false);
			return;
		}

		enableHLOD(true);
		useHLOD(ActorSystem::instance()->getMainActorManager()->IsEditorPlay());
		//float range = getWorldAABB().getSize().length();
		//range *= m_fHLODFactor;
		//const Vector3& minBox = getWorldAABB().getMinimum();
		//const Vector3& maxBox = getWorldAABB().getMaximum();

		//AxisAlignedBox hlodBox(minBox - range, maxBox + range);
		//m_pHLODObject->setScale(hlodBox.getSize());
	}

	void HLODComponent::setRelateColliderComponent(const ActorComponentPtr& comPtr)
	{
		m_pRelateColliderComponent = comPtr;
	}

	const ActorComponentPtr& HLODComponent::getRelateColliderComponent() const
	{
		return m_pRelateColliderComponent;
	}

	


	BuildingCreateListener::BuildingCreateListener(HLODComponent* pHost)
		: m_pHost(pHost)
	{
	}

	bool BuildingCreateListener::OnActorCreateFinish()
	{
		std::vector<ActorComponentPtr> compVec = mActor->getComponentsByType<BuildingComponent>(true);
		if (compVec.size() > 0)
		{
			m_pHost->m_BuildingComp = compVec[0];
			m_pHost->updateBuildLod2Distence();
		}
		return true;
	}



	// ----------------------------------- HLODComponentInfo ----------------------------------- //

	EDITOR_REFLECT_BEGIN(HLODComponentInfo)	
	EDITOR_REFLECT_MEMBER_UPDATE(m_MeshName, "MeshName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_RelateColliderComName, "RelateColliderComName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_HLODDist, "HLODDistance", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_CullMode, "CullMode", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_CullDistance, "CullDistance", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_TodCullDistance, "TodCullDistance", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_DissolveStrength, "DissolveStrength", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_FpsState, "FpsState", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_CastShadow, "CastShadow", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_LodDis_Level1, "LodDis_Level1", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_LodDis_Level2, "LodDis_Level2", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_HLODFactor, "HLODFactor", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_DissolveFactor, "DissolveFactor", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_CullFactor, "CullFactor", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_END();

	HLODComponentInfo::HLODComponentInfo(const Name& type, ActorCOMMoveableType movable)
		:ActorComponentInfo(type, movable)
	{
	}

	HLODComponentInfo::~HLODComponentInfo()
	{
	}

#ifdef ECHO_EDITOR
	bool HLODComponentInfo::getNeedCollectedResource(std::unordered_multimap<ResourceType, std::string>& resMap)
	{
		if (m_MeshName.empty()) {
			return false;
		}
		resMap.insert(std::make_pair(ResourceType::Mesh, m_MeshName.c_str()));
		return true;
	}
#endif // ECHO_EDITOR

	_DEFINEINFO_VIRFUN_BASE(HLODComponentInfo)

	HLODComponentFactory::HLODComponentFactory() 
		:ActorComFactory() 
	{ 
		mType = Echo::Name("HLODComponent"); 
		mMoveableType = ALL_MOVEABLE; 
		bIsEditorCom = true; 
		mPathGizmoIcon = Echo::Name("HLODComponent"); 
		auto&& pcm = ProtocolComponentSystem::instance();
		if(pcm)
		{
			pcm->registerProtocolExtFunc(Name("EnableHLODCom"), ProCom_EnableHLOD);
			pcm->registerProtocolExtFunc(Name("DisplayMissHLODLog"), ProCom_DisplayMissHLODLog);
			pcm->registerProtocolExtFunc(Name("DisplayAllHLODAABBAndDisLog"), ProCom_DisplayAllHLODAABBAndDisLog);
		}
	}

	HLODComponentFactory::~HLODComponentFactory() 
	{
		ProtocolComponentSystem::instance()->unregisterProtocolExtFunc(Name("EnableHLODCom"));
		ProtocolComponentSystem::instance()->unregisterProtocolExtFunc(Name("DisplayMissHLODLog"));
		ProtocolComponentSystem::instance()->unregisterProtocolExtFunc(Name("DisplayAllHLODAABBAndDisLog"));
	}

	Echo::ProtocolResult HLODComponentFactory::ProCom_EnableHLOD(void* enable, Echo::ProtocolHandle handle)
	{
		auto&& pFactory = ActorComFactoryManager::getSingleton().GetActorComFactory(Name("HLODComponent"));
		if (pFactory == nullptr)
			return PR_Falied;
		for (auto& pComponent : pFactory->GetComponentSet())
		{
			if (pComponent == nullptr)
				continue;
			auto&& hlodCom = static_cast<HLODComponent*>(pComponent.get());
			bool enb = *(bool*)enable;
			hlodCom->enableHLOD(enb);
			if (enb)
			{
				hlodCom->createReplaceHelper();
				if (hlodCom->m_pReplaceObject && hlodCom->m_pReplaceObject->getSceneNode() && !hlodCom->m_pReplaceObject->getSceneNode()->getParent())
					hlodCom->addChildToNode(hlodCom->m_pReplaceObject->getSceneNode());
				hlodCom->UpdateComRootNode(true);
				hlodCom->useHLOD(ActorSystem::instance()->getMainActorManager()->IsEditorPlay());
			}
			else
			{
				if (Root::instance()->getRuntimeMode() == Root::eRM_Client)
					hlodCom->destroyReplaceHelper();
			}
		}
		return PR_OK;
	}

#define DEF_MAX_LOOP_PER_COM_INFO_LOG 10

	void DisplaySetInfomation(const Echo::String& title, const std::set<std::string>& infos)
	{
		int cnt = 0;
		Echo::String info = title + " | size : " + std::to_string(infos.size()) + " :\n { ";
		for (const String& str : infos)
		{
			info += "\n" + str;

			++cnt;
			if (cnt >= DEF_MAX_LOOP_PER_COM_INFO_LOG)
			{
				LogManager::instance()->logMessage(info);
				cnt = 0;
				info = "";
			}
		}
		LogManager::instance()->logMessage(info + "\n}");
		cnt = 0;
	}

	Echo::ProtocolResult HLODComponentFactory::ProCom_DisplayMissHLODLog(void* pos, Echo::ProtocolHandle handle)
	{
		auto&& pFactory = ActorComFactoryManager::getSingleton().GetActorComFactory(Name("HLODComponent"));
		if (pFactory == nullptr)
		{
			LogManager::instance()->logMessage("HLOD Component not found in the scene (Query failed)");
			return PR_Falied;
		}

		std::set<std::string> lostHLODMesh;
		std::set<std::string> noExitHLOD;
		std::set<std::string> lostHLODPath;
		for (auto& pComponent : pFactory->GetComponentSet())
		{
			if (pComponent == nullptr)
				continue;
			auto&& hlodCom = static_cast<HLODComponent*>(pComponent.get());

			if (!hlodCom->m_bHLODMeshExist)
			{
				String info;
				info += "Component name is : " + hlodCom->getCombinedName();
				info += " Mesh path is : " + hlodCom->mEntityPath.toString();
				noExitHLOD.insert(info);
				if (!hlodCom->mEntityPath.empty())
					lostHLODMesh.insert(hlodCom->mEntityPath.toString());
			}
			else if (hlodCom->mEntityPath.empty())
				lostHLODPath.insert(hlodCom->getCombinedName());
		}

		if (noExitHLOD.empty() && lostHLODPath.empty())
			LogManager::instance()->logMessage("There are no non-existent HLOD");
		else
		{
			DisplaySetInfomation("non-existent HLOD", noExitHLOD);
			DisplaySetInfomation("File Path is Empty", lostHLODPath);
			DisplaySetInfomation("No Mesh for HLOD", lostHLODMesh);
		}

		return PR_OK;
	}

	Echo::ProtocolResult HLODComponentFactory::ProCom_DisplayAllHLODAABBAndDisLog(void* pos, Echo::ProtocolHandle handle)
	{
		auto&& pFactory = ActorComFactoryManager::getSingleton().GetActorComFactory(Name("HLODComponent"));
		if (pFactory == nullptr)
		{
			LogManager::instance()->logMessage("HLOD Component not found in the scene (Query failed)");
			return PR_Falied;
		}

		std::set<std::string> allHLODInfo;
		for (auto& pComponent : pFactory->GetComponentSet())
		{
			if (pComponent == nullptr)
				continue;
			auto&& hlodCom = static_cast<HLODComponent*>(pComponent.get());

			AxisAlignedBox box = hlodCom->getBoudingBox();
			VideoConfig* pVideoConfig;
			if (hlodCom->getWorldManager())
				pVideoConfig = hlodCom->getWorldManager()->getVideoConfig();
			else
				pVideoConfig = VideoConfigSystem::instance()->getMainVideoConfig();
			float fLodDisScale = DynamicVideoParamManager::instance()->getLodDisScale();
			if (pVideoConfig)
				fLodDisScale *= pVideoConfig->getSceneLodDisScale();
			float curHlodDist = hlodCom->m_fHLODDist * fLodDisScale;

			String info;//hlodCom->getLoadBoundingBox()
			info += "Component name is : " + hlodCom->getCombinedName();
			info += " Mesh path is : " + hlodCom->mEntityPath.toString();
			info += "\n AxisAlignedBox : min : " + StringConverter::toString(box.getMinimum());
			info += " max : " + StringConverter::toString(box.getMaximum());
			info += " HLOD distance : " + StringConverter::toString(curHlodDist);
			allHLODInfo.insert(info);
		}

		DisplaySetInfomation("all HLOD", allHLODInfo);
		
		return PR_OK;
	}

#undef DEF_MAX_LOOP_PER_COM_INFO_LOG
}
