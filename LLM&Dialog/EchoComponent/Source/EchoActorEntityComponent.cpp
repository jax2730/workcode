#include "EchoComponentStableHeaders.h"
#include "EchoActorEntityComponent.h"

#include "EchoEntity.h"

#include "EchoActorProFuncImp.h"
#include "EchoMeshMgr.h"
#include "EchoLogManager.h"
#include "EchoEntityObject.h"
#include "EchoResourceFolderManager.h"
#include "EchoEntityBatchComponent.h"
#include "EchoBuildingComponent.h"
#include "Actor/EchoActorHelper.h"
#include "EchoDataStream.h"
#include "EchoMeshSerializer.h"

namespace Echo
{
	DECLAREACTORCOMTYPE_CPP(EntityComponent)
	EntityComponent::EntityComponent(ActorPtr  pActor, ActorCOMMoveableType type)
					: NodeActorComponent(pActor, type)
	{
		setActorComActiveType(ActorComActiveBits::RENDER_BIT);
		pActor->setIsNeedAddInRoom(true);

		EntityObjectDesc desc;
		desc.pWorldMgr = getWorldManager();
		desc.pSceneMgr = getSceneManager();
		m_pEntityObject = new EntityObject(desc);
	}

//=============================================================================
EntityComponent::~EntityComponent()
{
}

void EntityComponent::init(ActorComponentInfo * info)
{
}

void EntityComponent::initRenderObject(ActorComponentInfo* info)
{
	if (m_pEntityObject)
	{
#ifdef ECHO_EDITOR
		m_pEntityObject->setName(Name(getCombinedName().append("||").append("Entity")));
		m_pEntityObject->setModuleName("Actor");
#endif
		m_pEntityObject->OnEntityCreateFinish = std::bind<void>(&EntityComponent::onEntityCreateFinish, this);
	}

	//auto it = std::bind<void>(&EntityComponent::onEntityCreateFinish, this);
	//m_pEntityObject->m_vtEntityCreateFinishList.push_back(it);
	m_pActComInfo = info;
	EntityComponentInfo* mInfo = static_cast<EntityComponentInfo*>(m_pActComInfo);
	if (mInfo == nullptr)
		return;
	if (!mInfo->m_MeshName.empty())
		loadEntity(Name(mInfo->m_MeshName), getActor()->isSyncLoadActor());
	else
		m_bCreateFinish = true;//如果有info资源且没有资源路径，认为创建完成
}

void EntityComponent::unInit()
{
	//if (!getActor().expired())
	//	getActor()->removeObjectFromRoom(m_pEntityObject);
	SetNotifiedUpdate(false);
	m_setEntityComListeners.clear();
	if (m_pEntityObject)
	{
		m_pEntityObject->clearAllEntity();
	}
}

void EntityComponent::replay(ActorComponentInfo * info)
{
	// TODO
}

void EntityComponent::bindNode()
{
	if (m_pEntityObject == nullptr || m_bAttachNode)
		return;

	m_bAttachNode = true;
	UpdateComRootNode();

	if (m_bNotifiedTransformUpdate && getActor() && getActor()->getBuildingCom())
	{
		BuildingComponent* pBuildingCom = static_cast<BuildingComponent*>(getActor()->getBuildingCom().getPointer());
		pBuildingCom->updateEntityComTransform(weak_from_this());
	}

}

void EntityComponent::connectActor()
{
	if (isCreateFinish() && getActor() && getActor()->getBuildingCom())
	{
		BuildingComponent* pBuildingCom = static_cast<BuildingComponent*>(getActor()->getBuildingCom().getPointer());
		pBuildingCom->addEntityCom(weak_from_this());
		SetNotifiedUpdate(true);
	}
}

void EntityComponent::disconnectCom()
{
	m_bAttachNode = false;
}

void EntityComponent::disconnectActor()
{
	if (!getActor()->getEntityObjBatchCom().expired())
	{
		EntityBatchComponent* pCom = static_cast<EntityBatchComponent*>(getActor()->getEntityObjBatchCom().getPointer());
		pCom->removeComFromBatch(this);
	}

	if (getActor()->getBuildingCom())
	{
		BuildingComponent* pBuildingCom = static_cast<BuildingComponent*>(getActor()->getBuildingCom().getPointer());
		pBuildingCom->removeEntityCom(weak_from_this(), true);
		SetNotifiedUpdate(false);
	}
}

void EntityComponent::copyTo(const ActorComponentPtr&   newComp)
{
}
void EntityComponent::visibleStateChanged(bool visibleState)
{
	if (ActorSystem::instance()->isHideRenderObject())
	{
		if (m_pEntityObject)
			m_pEntityObject->setVisible(false);
		return;
	}
	if (getActor())
	{
		// 如果有Building组件，则可见性由Building组件管理
		if (getActor()->getBuildingCom())
		{
			BuildingComponent* pBuildingCom = static_cast<BuildingComponent*>(getActor()->getBuildingCom().getPointer());

			if (!pBuildingCom->isBuildingControlling())
			{
				if (pBuildingCom->hasEntityComponent(weak_from_this()))
				{
					if (!visibleState)
					{
						pBuildingCom->removeEntityCom(weak_from_this());
					}
					else
					{
						return;
					}
				}
				else if (visibleState && isCreateFinish())
				{
					pBuildingCom->addEntityCom(weak_from_this());
					return;
				}
			}
		}

		// 如果有EntityObjectBatch组件，则可见性由EntityObjectBatch组件管理
		EchoWeakPtr<EntityBatchComponent> batchCom = getActor()->getEntityObjBatchCom().getStaticCastWeakPtr<EntityBatchComponent>();
		if(!batchCom.expired() && batchCom->isEnable() && batchCom->getBatchVisible())
			return;
	}
		
	if(m_pEntityObject)
		m_pEntityObject->setVisible(visibleState);
}

void EntityComponent::createComRootNodeImpl()
{
	mComRootNode = m_pEntityObject->getSceneNode();
}

void EntityComponent::DeleteComRootNodeImpl()
{
	SAFE_DELETE(m_pEntityObject);
	mComRootNode = nullptr;
}

void EntityComponent::NotifiChildImp(int changeMask)
{
	if (getActor() && getActor()->getBuildingCom())
	{
		BuildingComponent* pBuildingCom = static_cast<BuildingComponent*>(getActor()->getBuildingCom().getPointer());
		pBuildingCom->updateEntityComTransform(weak_from_this());
	}
}

void EntityComponent::updateActiveTypeState(ActorComActiveFlags inVal, ActorComActiveFlags inChangeMask)
{
	if (m_pEntityObject)
	{
		if (inChangeMask & ActorComActiveBits::CASTSHADOW_BIT)
			m_pEntityObject->enableCastShadow(!(inVal & ActorComActiveBits::CASTSHADOW_BIT));
		if(inChangeMask & ActorComActiveBits::SHOWENTITY_BIT)
			m_pEntityObject->enableShowEntity(!(inVal & ActorComActiveBits::SHOWENTITY_BIT));
	}
}


void EntityComponent::updatePortalCullType(PortalCullType cullType)
{
	if (m_pEntityObject)
	{
		m_pEntityObject->setPortalCullType(cullType);
	}
}


void EntityComponent::changeEntity(const std::string & path , bool synchronous)
{
	Name namePath(path);

	loadEntity(namePath,synchronous);
	//m_pEntityObject->changeEntity(namePath, synchronous);
}

void EntityComponent::setActor(const ActorPtr&  pActor)
{
	//if (pActor == nullptr)
		//return;
	if (m_pEntityObject != NULL)
	{
		m_bAttachNode = false;
	}
	
	m_pActor = pActor;
	if (getParentNodeCom() == nullptr)
		return;

	if (m_pEntityObject != nullptr && !m_bAttachNode)
	{
		m_bAttachNode = true;
		UpdateComRootNode();
	}
}

void EntityComponent::setQueryMask(Echo::uint32 flags)
{
#ifdef ECHO_EDITOR
	m_QtFlags = flags;
#endif
	if (m_pEntityObject == nullptr)
		return;
	m_pEntityObject->setQueryFlags(flags);
}

#ifdef ECHO_EDITOR
void EntityComponent::getErrorMessageImpl(String& message)
{
	if (!hasErrorMessage())
		return;
	if (m_pEntityObject == nullptr)
		return;
	m_pEntityObject->getErrorMessage(message);
}
bool EntityComponent::hasErrorMessageImpl()
{
	bool ret = false;
	if (m_pEntityObject == nullptr)
		return ret;
	ret = m_pEntityObject->hasErrorMessage();
	if (ret)
	{
		REPORT_ERROR_MESSAGE_INSTANCE(ActorComponent, this);
	}
	return ret;
}
#endif

void EntityComponent::updateCombinedName()
{
	if (m_pEntityObject == NULL)
		return;
	Echo::Name entityName = Name(getCombinedName().append("||").append("Entity"));
	m_pEntityObject->setName(entityName);
}

CameraCullMode EntityComponent::getCullMode()
{
	if (m_pEntityObject == NULL)
		return CameraCullMode::DefaultCull;
	return m_pEntityObject->getCameraCullMode();
}

void EntityComponent::setCullMode(int mode/* = DefaultCull*/)
{
	if (m_pEntityObject == NULL)
		return;
#ifdef ECHO_EDITOR
	if (ActorHelper::getForceSave() && m_pActComInfo)
	{
		EntityComponentInfo* entityComInfo = static_cast<EntityComponentInfo*>(m_pActComInfo);
		entityComInfo->m_CullMode = mode;
	}
#endif
	m_pEntityObject->setCameraCullMode(static_cast<CameraCullMode>(mode));
}

float EntityComponent::getRenderDistance()
{
	if (m_pEntityObject == NULL)
		return 0.f;
	return m_pEntityObject->getRenderingDistance();
}

void EntityComponent::setRenderDistance(float dis)
{
	if (m_pEntityObject == NULL)
		return;
#ifdef ECHO_EDITOR
	if (ActorHelper::getForceSave() && m_pActComInfo)
	{
		EntityComponentInfo* minfo = static_cast<EntityComponentInfo*>(m_pActComInfo);
		minfo->m_CullDistance = dis;
	}
#endif
	m_pEntityObject->setRenderingDistance(dis);
}

float EntityComponent::getTODRenderDis()
{
	if (m_pEntityObject == NULL)
		return 0.f;
	return m_pEntityObject->getRenderingDistanceForTOD();
}

void EntityComponent::setTODRenderDis(float dis)
{
	if (m_pEntityObject == NULL)
		return;

#ifdef ECHO_EDITOR
	if (ActorHelper::getForceSave() && m_pActComInfo)
	{
		EntityComponentInfo* entityComInfo = static_cast<EntityComponentInfo*>(m_pActComInfo);
		entityComInfo->m_TodCullDistance = dis;
	}
#endif
	m_pEntityObject->setRenderingDistanceForTOD(dis);
}

float EntityComponent::getDissolveStrength()
{
	if (m_pEntityObject == NULL)
		return 0;
	return m_pEntityObject->getStippleTransparency();
}

void EntityComponent::setDissolveStrength(float val)
{
	if (!m_pEntityObject)
		return;

#ifdef ECHO_EDITOR
	if (ActorHelper::getForceSave() && m_pActComInfo)
	{
		EntityComponentInfo* entityComInfo = static_cast<EntityComponentInfo*>(m_pActComInfo);
		entityComInfo->m_DissolveStrength = val;
	}
#endif
	if (val != 0.0f)
	{
		if (val > 1.0f)
			val = 1.0f;
		//mDissolveStrength = val;
		m_pEntityObject->setStippleTransparency(val);
	}
	else
		m_pEntityObject->resetStippleTransparency();
}

void EntityComponent::setCastShadow(bool isEnable)
{
	if (m_pEntityObject == NULL)
		return;

#ifdef ECHO_EDITOR
	if (ActorHelper::getForceSave() && m_pActComInfo)
	{
		EntityComponentInfo* entityComInfo = static_cast<EntityComponentInfo*>(m_pActComInfo);
		entityComInfo->m_CastShadow = isEnable;
	}
#endif
	m_pEntityObject->enableCastShadow(isEnable);
}

bool EntityComponent::getCastShadow()
{
	if (m_pEntityObject == NULL)
		return false;
	return m_pEntityObject->IsCastShadow();
}

void EntityComponent::setLodDisLevel1(float val)
{
	if (m_pEntityObject == NULL)
		return;
#ifdef ECHO_EDITOR
	if (ActorHelper::getForceSave() && m_pActComInfo)
	{
		EntityComponentInfo* entityComInfo = static_cast<EntityComponentInfo*>(m_pActComInfo);
		entityComInfo->m_LodDis_Level1 = val;
	}
#endif
	m_pEntityObject->setLodDis(LOD_LEVEL_1,val);
}

float EntityComponent::getLodDisLevel1()
{
	if (m_pEntityObject == NULL)
		return 0.0f;
	return m_pEntityObject->getLodDis(LOD_LEVEL_1);
}

void EntityComponent::setLodDisLevel2(float val)
{
	if (m_pEntityObject == NULL)
		return;
#ifdef ECHO_EDITOR
	if (ActorHelper::getForceSave() && m_pActComInfo)
	{
		EntityComponentInfo* entityComInfo = static_cast<EntityComponentInfo*>(m_pActComInfo);
		entityComInfo->m_LodDis_Level2 = val;
	}
#endif
	m_pEntityObject->setLodDis(LOD_LEVEL_2, val);
}

float EntityComponent::getLodDisLevel2()
{
	if (m_pEntityObject == NULL)
		return 0.0f;
	return m_pEntityObject->getLodDis(LOD_LEVEL_2);
}

void EntityComponent::loadEntity(const Name & name, bool synchronous)
{
	if (name.empty() || m_pEntityObject == NULL)
		return;

	mEntityPath = name;
	m_bCreateFinish = false; // 加载前需要把创建完成改成false;
	m_pEntityObject->clearAllEntity();
	m_pEntityObject->changeEntity(name, synchronous);
	if (ActorSystem::instance()->isHideRenderObject())
	{
		if (m_pEntityObject)
			m_pEntityObject->setVisible(false);
	}
	updateCombinedName();
	if (m_pActComInfo)
	{
		EntityComponentInfo* mInfo = static_cast<EntityComponentInfo*>(m_pActComInfo);
		setCullMode(static_cast<CameraCullMode> (mInfo->m_CullMode));
		setRenderDistance(mInfo->m_CullDistance);
		setTODRenderDis(mInfo->m_TodCullDistance);
		setDissolveStrength(mInfo->m_DissolveStrength);
		setEnityFpsState(mInfo->m_FpsState);
		setLodDisLevel1(mInfo->m_LodDis_Level1);
		setLodDisLevel2(mInfo->m_LodDis_Level2);
		setCastShadow(mInfo->m_CastShadow);
#ifdef ECHO_EDITOR
		if (ActorHelper::getForceSave())
		{
			mInfo->m_MeshName = name.toString();
		}
#endif
	}
#ifdef ECHO_EDITOR
	m_pEntityObject->setQueryFlags(m_QtFlags);
	if (m_pEntityObject->hasErrorMessage())
	{
		REPORT_ERROR_MESSAGE_INSTANCE(ActorComponent, this);
	}
#endif
}

//void EntityComponent::removeEntity()
//{
//	SAFE_DELETE(m_pEntityObject);
//}


void EntityComponent::setEnityFpsState(const int& state_)
{
	if (m_pEntityObject == nullptr)
		return;
#ifdef ECHO_EDITOR
	if (ActorHelper::getForceSave() && m_pActComInfo)
	{
		EntityComponentInfo* entityComInfo = static_cast<EntityComponentInfo*>(m_pActComInfo);
		entityComInfo->m_FpsState = state_;
	}
#endif
	ENUM_FP_STATE stateTemp = static_cast<ENUM_FP_STATE>(state_);
	m_pEntityObject->setFPState(stateTemp);
}

int EntityComponent::getEnityFpsState()
{
	if (m_pEntityObject == nullptr)
		return 0;
	return (int)m_pEntityObject->getFPState();
}

Echo::Entity* EntityComponent::getEntity()
{
	if (m_pEntityObject == nullptr)
		return nullptr;
	return m_pEntityObject->getEntity(LOD_LEVEL_0);
}

Echo::Entity* EntityComponent::getCurEntity()
{
	if (m_pEntityObject == nullptr)
		return nullptr;
	return m_pEntityObject->getCurEntity();
}


Echo::Entity* EntityComponent::getLowestEntity()
{
	if (m_pEntityObject == nullptr)
		return nullptr;
	return m_pEntityObject->getLowestEntity();
}

Echo::AxisAlignedBox EntityComponent::getLoadBoundingBox()
{
	Vector3 v3Distance = { 100000.0f, 100000.0f, 100000.0f };
	if (m_pActComInfo != nullptr)
		v3Distance = Vector3(static_cast<EntityComponentInfo*>(m_pActComInfo)->GetCullDistance());
	v3Distance = { 100000.0f, 100000.0f, 100000.0f };
	const Vector3& minBox = getWorldAABB().getMinimum();
	const Vector3& maxBox = getWorldAABB().getMaximum();
	AxisAlignedBox loadBox(minBox - v3Distance, maxBox + v3Distance);
	return loadBox;
}


void EntityComponent::setMatNames(const std::map<uint32, st_MaterialInfo>& vtMatNames)
{
	if (m_pEntityObject)
	{
		m_pEntityObject->setMatNames(vtMatNames);
	}
}


void EntityComponent::setCustomUniform(uint8 uniformIndex, uint8 paramIndex, float paramVal, int lodLevel /*= -1*/,int subIndex, EM_UNIFORM_TYPE uniformType /*= UNIFORM_TYPE_PS*/)
{
	if (m_pEntityObject)
	{
		m_pEntityObject->setCustomUniform(uniformIndex, paramIndex, paramVal, lodLevel,subIndex, uniformType);
	}
}

void EntityComponent::resetCustomUniform(uint8 uniformIndex, uint8 paramIndex, int lodLevel /*= -1*/, int subIndex, EM_UNIFORM_TYPE uniformType /*= UNIFORM_TYPE_PS*/)
{
	if (m_pEntityObject)
	{
		m_pEntityObject->resetCustomUniform(uniformIndex, paramIndex, lodLevel,subIndex, uniformType);
	}
}

float EntityComponent::getCustomUniform(uint8 uniformIndex, uint8 paramIndex, int lodLevel /*= -1*/, int subIndex, EM_UNIFORM_TYPE uniformType /*= UNIFORM_TYPE_PS*/)
{
	if (m_pEntityObject)
	{
		return m_pEntityObject->getCustomUniform(uniformIndex, paramIndex, lodLevel,subIndex, uniformType);
	}
	return 0;
}

void EntityComponent::onEntityCreateFinish()
{
	//if(!getActor().expired() && m_pEntityObject)
	//	getActor()->mergeBoundBox(m_pEntityObject->getWorldBox());
	
	//if (!getActor().expired())
	//	getActor()->addObjectToRoom(m_pEntityObject);

	if (Root::instance()->getRuntimeMode() == Root::eRM_Editor)
		UpdateBounds();
	m_bCreateFinish = true;
	if (!m_pActor.expired())
	{
		m_pActor->notifyComponentFinish(weak_from_this());
	}

	for (auto& listener : m_setEntityComListeners)
		listener->EntityCreateFinishCallBack();
	m_setEntityComListeners.clear();
	
	if (!getActor()->getEntityObjBatchCom().expired())
	{
		EntityBatchComponent* pCom= static_cast<EntityBatchComponent*>(getActor()->getEntityObjBatchCom().getPointer());
		pCom->addComToBatch(this);
	}

	if (getActor()->getBuildingCom())
	{
		BuildingComponent* pBuildingCom = static_cast<BuildingComponent*>(getActor()->getBuildingCom().getPointer());
		pBuildingCom->addEntityCom(weak_from_this());
		SetNotifiedUpdate(true);
	}

#ifdef ECHO_EDITOR
	if (m_pEntityObject->hasErrorMessage())
	{
		REPORT_ERROR_MESSAGE_INSTANCE(ActorComponent, this);
	}
#endif // ECHO_EDITOR

}

void EntityComponent::addEntityCreateListener(EntityComListener* listener)
{
	if (listener == nullptr)
		return;
	m_setEntityComListeners.emplace(listener);
}

void EntityComponent::removeEntityCreateListener(EntityComListener* listener)
{
	if (listener == nullptr)
		return;
	m_setEntityComListeners.erase(listener);
}

bool EntityComponent::isCreateFinish()
{
	return m_bCreateFinish;
}

void EntityComponent::setClientLogMessage(const Name& logstr)
{
	if (!m_pEntityObject)
		return;
	m_pEntityObject->mClientLogMessage = logstr;
}

void EntityComponent::setAutoLodLevel(bool val)
{
	if (!m_pEntityObject)
		return;
	m_pEntityObject->setEnableAutoLodLevel(val);
}

//_DEFINFO_DATA_WRITE_READ_FUN(NodeComponentInfo)




EDITOR_REFLECT_BEGIN(EntityComponentInfo)
EDITOR_REFLECT_MEMBER_UPDATE(m_MeshName, "MeshName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
EDITOR_REFLECT_MEMBER_UPDATE(m_CullMode, "CullMode", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
EDITOR_REFLECT_MEMBER_UPDATE(m_CullDistance, "CullDistance", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
EDITOR_REFLECT_MEMBER_UPDATE(m_TodCullDistance, "TodCullDistance", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
EDITOR_REFLECT_MEMBER_UPDATE(m_DissolveStrength, "DissolveStrength", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
EDITOR_REFLECT_MEMBER_UPDATE(m_FpsState, "FpsState", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
EDITOR_REFLECT_MEMBER_UPDATE(m_CastShadow, "CastShadow", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
EDITOR_REFLECT_MEMBER_UPDATE(m_LodDis_Level1, "LodDis_Level1", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
EDITOR_REFLECT_MEMBER_UPDATE(m_LodDis_Level2, "LodDis_Level2", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
//EDITOR_REFLECT_MEMBER_UPDATE(mNodeComponet, "NodeComponent", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
EDITOR_REFLECT_END();



EntityComponentInfo::EntityComponentInfo(const Name& comType, ActorCOMMoveableType moveType)
	:ActorComponentInfo(comType, moveType)
{

}
EntityComponentInfo::~EntityComponentInfo()
{
}

#ifdef ECHO_EDITOR
	bool EntityComponentInfo::getNeedCollectedResource(std::unordered_multimap<ResourceType, std::string>& resMap)
	{
		if (m_MeshName.empty()) {
			return false;
		}
		resMap.insert(std::make_pair(ResourceType::Mesh, m_MeshName));
		return true;
	}
#endif

_DEFINEINFO_VIRFUN_BASE(EntityComponentInfo)

//void EntityComponentInfo::ExportData(cJSON * jsonNode)
//{
//	reflect::TypeResolver<EntityComponentInfo>::get()->ExportJson(this, jsonNode, "");
//}
//
//void EntityComponentInfo::ImportData(cJSON * jsonNode)
//{
//	reflect::TypeResolver<EntityComponentInfo>::get()->ImportJson(this, jsonNode, "");
//}
//
//void EntityComponentInfo::CopyTo(ActorComponentInfo * comInfo)
//{
//	EntityComponentInfo* pInfo = static_cast<EntityComponentInfo*> (comInfo);
//	reflect::TypeResolver<EntityComponentInfo>::get()->copyTo(this, pInfo);
//}
//
//void EntityComponentInfo::Overlay(ActorComponentInfo* srcInfo)
//{
//	reflect::TypeResolver<EntityComponentInfo>::get()->overWrite(this, srcInfo);
//}
//
//
//void EntityComponentInfo::OverWrite()
//{
//	if (mOriginComInfo != nullptr)
//	{
//		reflect::TypeResolver<EntityComponentInfo>::get()->overWrite(mOriginComInfo, this);
//	}
//}

EchoEntityScript EchoEntityScript::Script_GetEntityComponent(const EchoActorScript& pActor, const std::string& name)
	{
		return EchoEntityScript(pActor, name);
	}

EntityComponentFactory::EntityComponentFactory()
{
	mType = Echo::Name("EntityComponent");
	mMoveableType = ALL_MOVEABLE;
	bIsEditorCom = true;
	mPathGizmoIcon = mType;
	auto&& pcm = ProtocolComponentSystem::instance();
	if (pcm)
	{
		pcm->registerProtocolExtFunc(Name("OutputACSEntityLodInfo"), ProCom_OutputLodInfo);
	}
}

EntityComponentFactory::~EntityComponentFactory()
{
	ProtocolComponentSystem::instance()->unregisterProtocolExtFunc(Name("OutputACSEntityLodInfo"));
}

namespace
{
	DataStreamPtr& operator << (DataStreamPtr& outstream, const char* str)
	{
		EchoZoneScoped;

		outstream->writeData(str, strlen(str));
		return outstream;
	}

	DataStreamPtr& operator << (DataStreamPtr& outstream, const String& str)
	{
		EchoZoneScoped;

		outstream->writeData(str.c_str(), str.size());
		return outstream;
	}

	template< class T>
	DataStreamPtr& operator << (DataStreamPtr& outstream, const T& t)
	{
		EchoZoneScoped;

		String str = StringConverter::toString(t);
		return outstream << str;
	}
}

void EntityComponentFactory::outputMeshLodInfoImpl(DataStreamPtr& dataStream, const Name& meshName, std::set<Name>& meshFindSet, uint32& maxTriNum)
{
	if (meshFindSet.find(meshName) != meshFindSet.end())
	{
		return;
	}
	meshFindSet.emplace(meshName);

	Mesh* pMesh = nullptr;
	Echo::MeshPtr meshPtr = Echo::MeshMgr::instance()->getByName(meshName);

	bool meshExist = false;
	bool fileExist = true;

	if (!meshPtr.isNull())
	{
		pMesh = meshPtr.get();
		meshExist = true;
	}
	else
	{
		pMesh = new Mesh(meshName);
	}

	if (!meshExist)
	{
		DataStreamPtr stream(Root::instance()->GetMemoryDataStream(meshName.c_str()));

		if (stream.isNull() || stream->size() == 0)
		{
			LogManager::instance()->logMessage("[VegAM]Mesh serializer import mesh resource failed!  meshName:"
				+ meshName.toString() + __FUNCTION__);
			fileExist = false;
		}
		else
		{
			try
			{
				Echo::MeshSerializer serializer;
				serializer.importMesh(stream, pMesh);
				if (!pMesh->m_bMergeMesh)
					pMesh->loadInfoFromJson();
			}
			catch (...)
			{
				LogManager::instance()->logMessage("[VegAM]Mesh serializer import mesh resource failed!  meshName:"
					+ meshName.toString() + __FUNCTION__);
				fileExist = false;
			}
		}
	}

	if (pMesh->m_bVirMesh && !pMesh->getRealMeshName().empty())
	{
		outputMeshLodInfoImpl(dataStream, pMesh->getRealMeshName(), meshFindSet, maxTriNum);
		if (!meshExist)
		{
			SAFE_DELETE(pMesh);
		}
		return;
	}

	String infoStr = "Mesh Name : " + meshName.toString();

	if (!fileExist)
	{
		infoStr += "(File Not Exist!!!)\n";
	}
	else
	{
		uint32 currTriNum = 0;
		for (uint32 iter = 0; iter < pMesh->getNumSubMesh(); ++iter)
		{
			SubMesh* pSubMesh = pMesh->getSubMesh(iter);
			if (pSubMesh)
				currTriNum += pSubMesh->m_TriangleCount;
		}

		infoStr += "\nTriangle Count : " + std::to_string(currTriNum);
		if (currTriNum > maxTriNum)
		{
			infoStr += "(Larger Lod Triangle Count!!!)";
		}
		infoStr += "\n";
		maxTriNum = currTriNum;
	}

	dataStream << infoStr.c_str();

	if (!meshExist)
	{
		SAFE_DELETE(pMesh);
	}
}

void EntityComponentFactory::outputMeshLodInfo(DataStreamPtr& dataStream, const Name& meshName, std::set<Name>& meshFindSet)
{
	if (meshFindSet.find(meshName) != meshFindSet.end())
	{
		return;
	}

	Name resMeshName[3];
	resMeshName[0] = meshName;
	Mesh::getLodMeshName(resMeshName[0], resMeshName[1], resMeshName[2]);
	uint32 maxTriNum = UINT_MAX;
	for (int i = LOD_LEVEL_0; i < LOD_LEVEL_NUM; ++i)
	{
		outputMeshLodInfoImpl(dataStream, resMeshName[i], meshFindSet, maxTriNum);
	}
	dataStream << "---------seprator---------\n";
}

Echo::ProtocolResult EntityComponentFactory::ProCom_OutputLodInfo(void* pDataStream, Echo::ProtocolHandle handle)
{
	DataStreamPtr dataStream = *(DataStreamPtr*)pDataStream;
	auto&& pFactory = ActorComFactoryManager::getSingleton().GetActorComFactory(Name("EntityComponent"));
	if (pFactory == nullptr)
	{
		LogManager::instance()->logMessage("EntityComponent not found in the scene (Query failed)");
		return PR_Falied;
	}

	std::set<Name> meshFindSet;

	for (auto& pComponent : pFactory->GetComponentSet())
	{
		if (pComponent == nullptr)
			continue;
		EntityComponent* entityCom = static_cast<EntityComponent*>(pComponent.get());
		EntityObject* pEntityObject = entityCom->getEntityObject();
		const Name& meshName = entityCom->mEntityPath;
		if (pEntityObject && !meshName.empty())
		{
			outputMeshLodInfo(dataStream, meshName, meshFindSet);
		}
	}

	return PR_OK;
}

}; // namespace Echo 
