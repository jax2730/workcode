#include "EchoComponentStableHeaders.h"
#include "EchoEntityBatchComponent.h"
#include "EchoComDefineMacro.h"
#include "EchoActorEntityComponent.h"
#include "EchoHLODComponent.h"
#include "EchoActorRoleComponent.h"
#include "EchoDynamicVideoParamManager.h"
#include "EchoComInfoDefineMacro.h"


//#include "EchoHelperObj.h"

namespace Echo
{
	DECLAREACTORCOMTYPE_CPP(EntityBatchComponent);
	EDITOR_REFLECT_BEGIN(EntityBatchComponentInfo)
	EDITOR_REFLECT_MEMBER_UPDATE(m_EntityBatchDist, "BatchRenderDis", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_EntityBatchFactor, "BatchRenderFactor", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_EnableDis, "EnableDistance", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_EnableDisFactor, "EnableDistanceFactor", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_TodCullDistance, "TodCullDistance", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_DissolveStrength, "DissolveStrength", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_CastShadow, "CastShadow", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_LodDis_Level1, "LodDis_Level1", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_LodDis_Level2, "LodDis_Level2", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_BoundBox, "RenderBoundBox", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_END();

	Name g_EntityComType("EntityComponent");

	static AxisAlignedBox getLocalAABB(ActorComponent* pCom)
	{
		Echo::AxisAlignedBox aabb = pCom->getWorldAABB();
		if (!aabb.isNull())
		{
			Echo::Vector3 scale(1, 1, 1);
			Echo::Vector3 pos(0, 0, 0);
			scale = pCom->getWorldScale();
			scale = Echo::Vector3::UNIT_SCALE / scale;
			pos = pCom->getWorldPosition();
			aabb = Echo::AxisAlignedBox(aabb.getMinimum() - pos, aabb.getMaximum() - pos);
			aabb.scale(scale);
		}
		else
		{
			aabb = AxisAlignedBox::BOX_NULL;
		}
		return aabb;
	}

	EntityBatchComponentInfo::EntityBatchComponentInfo(const Name& type, ActorCOMMoveableType moveable)
		:ActorComponentInfo(type, moveable)
	{

	}

	EntityBatchComponentInfo::~EntityBatchComponentInfo()
	{

	}

	_DEFINEINFO_VIRFUN_BASE(EntityBatchComponentInfo);

	EntityBatchComponent::EntityBatchComponent(ActorPtr pActor, ActorCOMMoveableType type)
		:ProtocolActorComponent(pActor, type)
	{

	}

	EntityBatchComponent::~EntityBatchComponent()
	{

	}

	void EntityBatchComponent::init(ActorComponentInfo* info)
	{
		ProtocolActorComponent::init(info);
		m_pEntityObjectBatch = new EntityObjectBatch(getActor()->GetSceneManager());
		//m_pEntityObjectBatch->addRenderListener(this);
		enableEntityBatch(m_bGlobalEnable);
		if (info == nullptr)
			return;

		EntityBatchComponentInfo* pBatchInfo = static_cast<EntityBatchComponentInfo*>(info);
		//if(Root::instance()->getRuntimeMode() == Root::eRM_Client)
		//	useEntityBatch(true);
		m_fEntityBatchFactor = pBatchInfo->m_EntityBatchFactor;
		setBatchRenderDis(pBatchInfo->m_EntityBatchDist);
		setBatchEnableFactor(pBatchInfo->m_EnableDisFactor);
		setDissolveStrength(pBatchInfo->m_DissolveStrength);
		setRenderBoundingBox(pBatchInfo->m_BoundBox);
		setLodDisLevel1(pBatchInfo->m_LodDis_Level1);
		setLodDisLevel2(pBatchInfo->m_LodDis_Level2);
		setCastShadow(pBatchInfo->m_CastShadow);
	}

	void EntityBatchComponent::unInit()
	{
		ProtocolActorComponent::unInit();
		//if (mRequestID > 0)
		//{
		//	EntityObjectBatchMgr::instance()->abortRequest(mRequestID);
		//	mRequestID = 0;
		//}
		//useEntityBatch(false);
		//enableEntityBatch(false);
		//m_pEntityObjectBatch->removeRenderListener(this);
		SAFE_DELETE(m_pEntityObjectBatch);
		//mbBatchFinish = false;
	}

	void EntityBatchComponent::bindNode()
	{
		if (m_pParentNodeCom.expired())
			return;
		if (m_pEntityObjectBatch != nullptr)
			addChildToNode(m_pEntityObjectBatch->getParentSceneNode());
		auto it = std::bind<void>(&EntityBatchComponent::OnNodeTransformUpdate, this);
		getActor()->getRootNodeCom()->getSceneNode()->addTransformUpdateFunc(this, it);
		m_pParentNodeCom->UpdateComRootNode(true);
#ifdef ECHO_EDITOR
		setRenderBoundingBox(getLocalAABB(getActor()->getRootNodeCom().getPointer()));
#endif
	}

	void EntityBatchComponent::connectActor()
	{
#ifdef ECHO_EDITOR
		//if (Root::instance()->getRuntimeMode() == Root::eRM_Editor)
		{
			batchEntitys();
			enableEntityBatch(ActorSystem::instance()->getMainActorManager()->IsEditorPlay());
			//m_pEntityObjectBatch->updateBoundingBox();
			//m_pEntityObjectBatch->setPosition(m_pEntityObjectBatch->getBoundingBox().getCenter());
			//enableEntityBatch(true);
			//useEntityBatch(true);
			//HelperObject* helper = new HelperObject(Name("Batchtt"), HelperObjType_Box);
			//helper->setRenderType(ECHO_LINE_STRIP);
			//helper->setHelperColorType(HelperObjColorType_white);
			//helper->setScale(m_pEntityObjectBatch->getBoundingBox().getSize());
			//helper->setOrientation(m_pEntityObjectBatch->getOrientation());
			//helper->setPosition(m_pEntityObjectBatch->getPosition());
			
		}
#endif
	}

	void EntityBatchComponent::disconnectCom()
	{
		if (m_pParentNodeCom.expired())
			return;
		getActor()->getRootNodeCom()->getSceneNode()->removeTransformUpdateFunc(this);
		if (m_pEntityObjectBatch != NULL)
			RemoveChildFromNode(m_pEntityObjectBatch->getParentSceneNode());
		m_pParentNodeCom->UpdateComRootNode(true);
	}

	void EntityBatchComponent::visibleStateChanged(bool visibleState)
	{
		if (!ActorSystem::instance()->getMainActorManager()->IsEditorPlay())
			return;
		useEntityBatch(visibleState);
	}

	void EntityBatchComponent::registerTickTimeBegin()
	{
		m_eventSet[TT_BeginFrame].insert(EID_EveryFrame);
		registerTickTimeEnd();
	}

	void EntityBatchComponent::beginFrame(EventID eventid, ProtocolHandle handle)
	{
		switch (eventid)
		{
		case EID_EveryFrame:
		{

			if (!isEnable())
				return;


			const AxisAlignedBox& aabb = getActor()->getWorldBounds();
			bool distanceCheck = true;
			float fLodDisScale = DynamicVideoParamManager::instance()->getLodDisScale();
			float curDist = m_fEnableDis * fLodDisScale;
			float minSqureDis = FLT_MAX;
			for (auto& pageManager : getActor()->m_pPagedManager)
			{
				for (size_t i = 0; i < Target_Component_Cnt; ++i)
				{
					const Vector3 currPos = pageManager ? pageManager->getTargetComponentPosition(Target_Component_Type(i)) : getSceneManager()->getMainCamera()->getPosition();
					float curSqureDis = aabb.SquareDistPointToAABB(currPos);
					minSqureDis = std::min(minSqureDis, curSqureDis);
					if (curSqureDis < curDist * curDist)
						distanceCheck = false;
				}
			}

			if (distanceCheck)
			{
				useEntityBatch(true);
			}
			else
			{
				useEntityBatch(false);
			}

			//if (!m_bEnable)
			//	return;
			//if(Root::instance()->getRuntimeMode() == Root::eRM_Editor)
			//	setRenderBoundingBox(getLocalAABB(getActor()->getRootNodeCom().getPointer()));
			//if (!mbBatchFinish)
			//{
			//	if (mRequestID == 0 && getActor()->isCreateFinish())
			//	{
			//		st_EntityObjBatchRequest req;
			//		req.pCom = this;
			//		mRequestID = EntityObjectBatchMgr::instance()->addRequest(req, 0);
			//	}
			//	return;
			//}

		}
		default:
			break;
		}
	}

	void EntityBatchComponent::EditorPlayImp()
	{
		enableEntityBatch(true);
	}

	void EntityBatchComponent::EditorPauseImp(bool pause)
	{

	}

	void EntityBatchComponent::EditorStopImp()
	{
		if (!isEnable())
			return;
		enableEntityBatch(false);
	}


	void EntityBatchComponent::OnNodeTransformUpdate()
	{
		if (m_pEntityObjectBatch == NULL)
			return;
		m_pEntityObjectBatch->setPosition(getActor()->getWorldPosition());
		m_pEntityObjectBatch->setOrientation(getActor()->getWorldRotation());
		m_pEntityObjectBatch->updateBoundingBox();
	}

	void EntityBatchComponent::setRenderBoundingBox(const AxisAlignedBox& box)
	{
		if (m_pEntityObjectBatch == NULL)
			return;

		m_pEntityObjectBatch->setBoundBox(box);
		if (Root::instance()->getRuntimeMode() == Root::eRM_Editor)
		{
			//EntityBatchComponentInfo* pComponentInfo = static_cast<EntityBatchComponentInfo*>(m_pActComInfo);
			COMPONENT_INFO_PARAM_CHANGE(EntityBatchComponentInfo, static_cast<EntityBatchComponentInfo*>(m_pActComInfo), m_BoundBox, box);
		}
	}

	CameraCullMode EntityBatchComponent::getCullMode()
	{
		if (m_pEntityObjectBatch)
			return m_pEntityObjectBatch->getCameraCullMode();
		return DefaultCull;
	}

	void EntityBatchComponent::setCullMode(int mode)
	{
		if (m_pEntityObjectBatch)
			m_pEntityObjectBatch->setCameraCullMode((CameraCullMode)mode);
	}

	float	EntityBatchComponent::getTODRenderDis()
	{
		if (m_pEntityObjectBatch == NULL)
			return 1000.0f;

		return m_pEntityObjectBatch->getRenderingDistanceForTOD();
	}
	void	EntityBatchComponent::setTODRenderDis(float dis)
	{
		if (m_pEntityObjectBatch == NULL)
			return;

		m_pEntityObjectBatch->setRenderingDistanceForTOD(dis);
	}

	void EntityBatchComponent::setCastShadow(bool isEnable)
	{
		if (m_pEntityObjectBatch == NULL)
			return;
		m_pEntityObjectBatch->enableCastShadow(isEnable);
	}

	bool EntityBatchComponent::getCastShadow()
	{
		if (m_pEntityObjectBatch == NULL)
			return false;
		return m_pEntityObjectBatch->IsCastShadow();

	}

	void EntityBatchComponent::setLodDisLevel1(float val)
	{
		if (m_pEntityObjectBatch == NULL)
			return;
		m_pEntityObjectBatch->setLodDis(LOD_LEVEL_1, val);
	}

	float EntityBatchComponent::getLodDisLevel1()
	{
		if (m_pEntityObjectBatch == NULL)
			return 0.0f;
		return m_pEntityObjectBatch->getLodDis(LOD_LEVEL_1);
	}

	void EntityBatchComponent::setLodDisLevel2(float val)
	{
		if (m_pEntityObjectBatch == NULL)
			return;
		m_pEntityObjectBatch->setLodDis(LOD_LEVEL_2, val);
	}

	float EntityBatchComponent::getLodDisLevel2()
	{
		if (m_pEntityObjectBatch == NULL)
			return 0.0f;
		return m_pEntityObjectBatch->getLodDis(LOD_LEVEL_2);
	}

	float EntityBatchComponent::getDissolveStrength()
	{
		if (m_pEntityObjectBatch == NULL)
			return 0;
		return m_pEntityObjectBatch->getStippleTransparency();
	}

	void EntityBatchComponent::setDissolveStrength(float val)
	{
		if (!m_pEntityObjectBatch)
			return;
		if (val != 0.0f)
		{
			if (val > 1.0f)
				val = 1.0f;
			//mDissolveStrength = val;
			m_pEntityObjectBatch->setStippleTransparency(val);
		}
		else
			m_pEntityObjectBatch->resetStippleTransparency();
	}

	void	EntityBatchComponent::setBatchRenderDis(float dis)
	{
		if (m_pEntityObjectBatch == NULL)
			return;

		m_fEntityBatchDist = dis;
		m_pEntityObjectBatch->setRenderingDistance(dis);
	}
	float	EntityBatchComponent::getBatchRenderDis()
	{
		return m_fEntityBatchDist;
	}

	void EntityBatchComponent::setBatchEnableDis(float dis)
	{
		m_fEnableDis = dis;
	}

	float EntityBatchComponent::getBatchEnableDis()
	{
		return m_fEnableDis;
	}

	void EntityBatchComponent::setBatchEnableFactor(float factor)
	{
		if (factor < std::numeric_limits<float>::epsilon())
			factor = std::numeric_limits<float>::epsilon();

		m_fEnableDisFactor = factor;
		AxisAlignedBox aabb = getWorldAABB();
		setBatchRenderDis((aabb.getSize() * m_fEnableDisFactor).Length());
	}

	float EntityBatchComponent::getBatchEnableFactor()
	{
		return m_fEnableDisFactor;
	}

	void EntityBatchComponent::setShadowRenderObject(EntityObject* obj, const ActorComponentPtr& pCom)
	{
		if(m_pEntityObjectBatch)
			m_pEntityObjectBatch->setShadowRenderObject(obj, pCom);
	}
	EntityObject* EntityBatchComponent::getShadowRenderObject()
	{
		if (!m_pEntityObjectBatch)
			return nullptr;
		return m_pEntityObjectBatch->getShadowRenderObject();
	}

	void EntityBatchComponent::addComToBatch(ActorComponent* pCom)
	{
		if (m_pEntityObjectBatch == NULL)
			return;
		m_bUpdateFinish = false;
		if (pCom->getType() == g_EntityComType)
		{
			EntityComponent* pEntityCom = static_cast<EntityComponent*>(pCom);
			EntityObject* obj = pEntityCom->getEntityObject();
			if (obj)
			{
				if(isEnable())
					obj->setVisible(false);
				m_pEntityObjectBatch->addEntityObjectToBatch(obj);
			}
			
		}
	}

	void EntityBatchComponent::removeComFromBatch(ActorComponent* pCom)
	{
		if (m_pEntityObjectBatch == NULL)
			return;
		
		if (pCom->getType() == g_EntityComType)
		{
			EntityComponent* pEntityCom = static_cast<EntityComponent*>(pCom);
			EntityObject* obj = pEntityCom->getEntityObject();
			if (obj)
			{
				m_pEntityObjectBatch->removeEntityObjectToBatch(pEntityCom->getEntityObject());
				obj->setVisible(true);
			}
		}
	}

#ifdef ECHO_EDITOR
	std::set<Name> collectComTypeSet = { Name("EntityComponent")/*, Name("RoleComponent"),Name("HLODComponent")*/};
	std::set<Name> exceptChildTypeSet = { Name("InOutsideDivisionComponent"), Name("EntityBatchComponent") };

	void EntityBatchComponent::batchEntitys()
	{
		//m_pEntityObjectBatch->updateBoundingBox();
		//只收集所有子actor的entity
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
			for (auto& coltype : collectComTypeSet)
			{
				auto batchComs = childActor->getComponentsByType(coltype, false);
				for (auto& collectCom : batchComs)
				{
					if (collectCom->getType() == COMPONENTTYPE(EntityComponent))
					{
						EntityComponent* EntityComPtr = static_cast<EntityComponent*>(collectCom.getPointer());
						if (EntityComPtr == nullptr)
						{
							continue;
						}
						addComToBatch(EntityComPtr);
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
			for (auto& comType : exceptChildTypeSet)
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
#endif

	void EntityBatchComponent::useEntityBatch(bool use)
	{
		if (m_pEntityObjectBatch == NULL)
			return;

		if (!isEnable())
			return;

		if (m_bUpdateFinish && use == m_bBatchVisible)
		{
			return;
		}
		//if (use == m_bBatchVisible)
		//	return;
		m_bUpdateFinish = true;
		m_bBatchVisible = use;
		m_pEntityObjectBatch->setBatchVisible(use);
//#ifdef ECHO_EDITOR
//		m_pEntityObjectBatch->setEntityObjsVisible(!use);
//#endif
	}
	void EntityBatchComponent::enableEntityBatch(bool enable)
	{
		if (m_pEntityObjectBatch == NULL)
			return;

		if (m_bEnable == enable)
			return;
		m_bUpdateFinish = true;
		m_bEnable = enable;
		m_bBatchVisible = enable;
		m_pEntityObjectBatch->setBatchVisible(enable);
//#ifdef ECHO_EDITOR
//		{
//			m_bBatchVisible = !enable;
//			m_pEntityObjectBatch->setVisible(!enable);
//			m_pEntityObjectBatch->setEntityObjsVisible(enable);
//		}
//#else
//		{
//			m_bBatchVisible = enable;
//			m_pEntityObjectBatch->setVisible(enable);
//			m_pEntityObjectBatch->setEntityObjsVisible(!enable);
//		}
//#endif
	}
	bool EntityBatchComponent::isEnable() const
	{
		return m_bGlobalEnable && m_bEnable;
	}

	void EntityBatchComponent::setEntityBatchFactor(float factor)
	{
		if (factor < std::numeric_limits<float>::epsilon())
			factor = std::numeric_limits<float>::epsilon();
		m_fEntityBatchFactor = factor;
		AxisAlignedBox aabb = getWorldAABB();
		setBatchRenderDis((aabb.getSize() * m_fEntityBatchFactor).Length());
	}


	EntityBatchComponentFactory::EntityBatchComponentFactory()
		:ActorComFactory()
	{
		mType = Echo::Name("EntityBatchComponent");
		mMoveableType = ALL_MOVEABLE;
		bIsEditorCom = true;
		mPathGizmoIcon = Echo::Name("EntityBatchComponent");
		auto&& pcm = ProtocolComponentSystem::instance();
		if (pcm)
			pcm->registerProtocolExtFunc(Name("EnableBatchEntityObj"), ProCom_EnableBatchEntityObj);
	}

	EntityBatchComponentFactory::~EntityBatchComponentFactory()
	{
		ProtocolComponentSystem::instance()->unregisterProtocolExtFunc(Name("EnableBatchEntityObj"));
	}

	Echo::ProtocolResult EntityBatchComponentFactory::ProCom_EnableBatchEntityObj(void* enable, Echo::ProtocolHandle handle)
	{
		auto&& pFactory = ActorComFactoryManager::getSingleton().GetActorComFactory(Name("EntityBatchComponent"));
		if (pFactory == nullptr)
			return PR_Falied;
		bool enb = *(bool*)enable;
		EntityBatchComponent::m_bGlobalEnable = enb;
		for (auto& pComponent : pFactory->GetComponentSet())
		{
			if (pComponent == nullptr)
				continue;
			auto&& batchCom = static_cast<EntityBatchComponent*>(pComponent.get());
			batchCom->enableEntityBatch(enb);
			//batchCom->setRuntimeVisible(enb);
		}
		return PR_OK;
	}

}
