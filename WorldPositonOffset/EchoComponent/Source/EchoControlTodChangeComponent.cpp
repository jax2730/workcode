#include "EchoComponentStableHeaders.h"
#include "EchoControlTodChangeComponent.h"
#include "EchoParticleSystemComponent.h"
#include "EchoTodTimelineComponent.h"
#include "EchoSceneComponentManager.h"
#include "EchoSky.h"
#include "EchoSun.h"
#include "EchoOcean.h"
#include "EchoRenderStrategy.h"
#include "EchoPostProcess.h"
#include "EchoTODManager.h"
#include "EchoSceneComponentManager.h"
#include "EchoStarfieldComponent.h"
#include "EchoCloudMeshComponent.h"
#include "EchoDirecLightComponent.h"
#include "EchoHelperObjProFuncImp.h"
#include "EchoSunComponent.h"
#include "EchoSkyComponent.h"
#include "EchoOceanComponent.h"
#include "EchoOcean.h"
#include "EchoCloudSphereComponent.h"
#include "EchoCloudPlaneComponent.h"
#include "EchoFogComponent.h"
#include "EchoCloud.h" 
#include "EchoBoxComponent.h"
#include "EchoEnvironmentLight.h"
#include "EchoEnvironmentLightComponent.h"
#include "EchoTodTimelineComponent.h"
#include "EchoWindPowerComponent.h"
#include "EchoPointlightManagerComponent.h"
#include "EchoUnderWaterCausticsComponent.h"
#include "EchoLightningComponent.h"
#include "EchoGlobalLightComponent.h"
#include "EchoHDRComponent.h"
#include "EchoEnvTriggerEventComponent.h"
#include "EchoEnvTriggerEvent.h"
#include "EchoTODComponentManager.h"
#include "EchoTODComponentSystem.h"
#include "EchoDriftComponent.h"
#include "EchoHDRAdaptiveComponent.h"
#include "EchoSphereFogComponent.h"
#include "EchoStarsComponent.h"
#include "EchoTODShadowComponent.h"
#include "EchoVolumeLightComponent.h"
#include "EchoWeatherCoverColorComponent.h"

namespace Echo 
{
	DECLAREACTORCOMTYPE_CPP(ControlTodChangeComponent)


	EDITOR_REFLECT_BEGIN(ControlTodChangeComponentInfo)
	    EDITOR_REFLECT_MEMBER_UPDATE(mContext.mCloudMeshName, "CloudMeshName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mCloudPlaneName, "CloudPlaneName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mCloudSphereName, "CloudSphereName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mDirecLightName, "DirecLightName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mFogName, "FogName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mSkyName, "SkyName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mSunName, "SunName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mOceanName, "OceanName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mWindPowerName, "WindPowerName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mUnderWaterCausticsName, "UnderWaterCausticsName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mLightningName, "LightningName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mTodTimelineName, "TodTimelineName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mEnvironmentLightName,"EnvironmentLightName",EditorUI_DRAG,nullptr,reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mStarfieldName,"StarfieldName",EditorUI_DRAG,nullptr,reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mBoxName, "BoxName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mSceneName, "SceneName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mParticleName, "ParticleName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mCameraPosition, "CameraPosition", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mSceneID, "SceneID", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mTodTransTime, "TodTransTime", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mParticleNameVec, "ParticleNameVec", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mCloudTransTime, "CloudTransTime", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mPointlightMgrName, "PointlightMgrName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mGlobalLightName, "GlobalLightName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mHDRName, "HDRName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mIsChangeScene, "IsChangeSceneByCom", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mEnvTriggerName, "EnvTriggerName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)

		EDITOR_REFLECT_MEMBER_UPDATE(mContext.m_TodActorName, "TodActorName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.m_StfID, "StfID", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.m_Priority, "Priority", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.m_isEffectTod, "isEffectTod", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_END()

	ControlTodChangeComponentInfo::ControlTodChangeComponentInfo(const Name& comType, ActorCOMMoveableType type)
		:ActorComponentInfo(comType,type)
	{
	}
	ControlTodChangeComponentInfo::~ControlTodChangeComponentInfo()
	{
	}
	void ControlTodChangeComponentInfo::ExportData(cJSON* jsonNode)
	{
		reflect::TypeResolver<ControlTodChangeComponentInfo>::get()->ExportJson(this, jsonNode, "");
	}
	void ControlTodChangeComponentInfo::ImportData(cJSON* jsonNode)
	{
		reflect::TypeResolver<ControlTodChangeComponentInfo>::get()->ImportJson(this, jsonNode, "");
	}
	void ControlTodChangeComponentInfo::CopyTo(ActorComponentInfo* comInfo)
	{
		if (comInfo == nullptr)
			return;
		ControlTodChangeComponentInfo* copyInfo = static_cast<ControlTodChangeComponentInfo*>(comInfo);
		reflect::TypeResolver<ControlTodChangeComponentInfo>::get()->copyTo(this, copyInfo);
	}
	void ControlTodChangeComponentInfo::Overlay(ActorComponentInfo* srcInfo)
	{
	}
	void ControlTodChangeComponentInfo::OverWrite()
	{
		if (mOriginComInfo != nullptr)
		{
			reflect::TypeResolver<ControlTodChangeComponentInfo>::get()->overWrite(mOriginComInfo, this);
		}
	}
	ControlTodChangeComponent::ControlTodChangeComponent(EchoWeakPtr<Actor> pActor, ActorCOMMoveableType isMoveable)
		:ProtocolActorComponent(pActor,isMoveable)
	{
	}

	ControlTodChangeComponent::~ControlTodChangeComponent()
	{
	}

	AxisAlignedBox ControlTodChangeComponent::getLoadBoundingBox()
	{
		AxisAlignedBox loadBox(Vector3(-10000.f), Vector3(10000.f));
		return loadBox;
	}

	void ControlTodChangeComponent::init(ActorComponentInfo* info)
	{
		mTodComVec.reserve(Vec_End);
		mTodComVec.resize(Vec_End);
		Name sceneName = g_MainActorManagerName;
		if (m_pActor)
		{
			SceneManager* sMgr = m_pActor->GetSceneManager();
			if (sMgr)
				sceneName = sMgr->getName();
		}
		m_tMgr = TODComponentSystem::instance()->getTODComponentManager(sceneName);
		if (info != nullptr)
		{
			ProtocolActorComponent::init(info);
			mContext = static_cast<ControlTodChangeComponentInfo*>(info)->mContext;
		}
		
		setTodActorFilePath(mContext.m_TodActorName);
	}
	void ControlTodChangeComponent::unInit()
	{
		ProtocolActorComponent::unInit();
		for (size_t i = 0; i < Vec_End; i++)
			mTodComVec[i] = nullptr;
		for (size_t i = 0; i < mParticleComVec.size(); ++i)
			mParticleComVec[i] = nullptr;

		if (m_tMgr)
			m_tMgr->_RemoveControlTodComVec(this);
	}
	void ControlTodChangeComponent::copyTo(const ActorComponentPtr& actCom)
	{
	}
	void ControlTodChangeComponent::registerTickTimeBegin()
	{
		m_eventSet[TT_BeginFrame].insert(EID_EveryFrame);
		registerTickTimeEnd();
	}
	void ControlTodChangeComponent::connectCom()
	{
		if (m_pActComInfo == nullptr)
			return;
		initTodVecState(m_pActComInfo);
		initParticleCom();
		initTransTime();
	}
	void ControlTodChangeComponent::beginFrame(EventID eventId, ProtocolHandle)
	{
		switch (eventId)
		{
		case EID_EveryFrame:
		{
			if (isPosInEpitaxyBox() && !mIsTrigger)
			{
				mIsTrigger = true;
				if (m_TodActor)
					m_TodActor->setVisiable(true);
				if (m_tMgr)
					m_tMgr->_AddControlTodComVec(this);
				updateActiveTod();
				switchEffect(false);
				mIsTriggerStarfield = false;
			}
			else if (isPosInBox() && mContext.mIsChangeScene)
			{
				changeScene();
				switchEffect(false);
				mIsTriggerStarfield = false;
			}
			else if (mIsTrigger && !isPosInEpitaxyBox())
			{
				if (isPosInTripleEpitaxyBox() && mIsTriggerStarfield == false) {
					mIsTriggerStarfield = true;
					switchEffect(true);
				}
				else if(!isPosInTripleEpitaxyBox() && mIsTriggerStarfield == true){
					switchEffect(false);
					mIsTriggerStarfield = false;
				}
				if (m_TodActor)
					m_TodActor->setVisiable(false);
				if (m_tMgr)
				{
					bool isCurrCom_ = this == m_tMgr->_GetFirstControlCom();
					m_tMgr->_RemoveControlTodComVec(this);
					if(isCurrCom_)
					   updateActiveTod();
				}
			
				mIsTrigger = false;
			}
			break;
		}
		default:
				break;
		}
	}
	
	void ControlTodChangeComponent::initParticleCom()
	{
		const ActorPtr& currActor = getActor();
		if (currActor == nullptr)
			return;
		bool isExit = false;
		size_t sz = mContext.mParticleNameVec.size();
		mParticleComVec.reserve(sz);
		mParticleComVec.resize(sz);
		for (size_t i = 0 ; i < sz ; ++ i)
		{
			if (mContext.mParticleNameVec[i] == mContext.mParticleName)
				isExit = true;
			ActorComponent*  currCom_ = currActor->getComponent(mContext.mParticleNameVec[i]).get();
			mParticleComVec[i] = static_cast<ParticleSystemComponent*>(currCom_);
		}

		if(!isExit)
		{
			mParticleComVec.reserve(sz + 1);
			mParticleComVec.resize(sz + 1);
			mParticleComVec[sz] = static_cast<ParticleSystemComponent*>(currActor->getComponent(mContext.mParticleName).get());
			mContext.mParticleNameVec.reserve(sz + 1);
			mContext.mParticleNameVec.resize(sz + 1);
			mContext.mParticleNameVec[sz] = mContext.mParticleName;
			if (m_pActComInfo)
			{
				ControlTodChangeComponentInfo* conInfo = static_cast<ControlTodChangeComponentInfo*>(m_pActComInfo);
				conInfo->mContext.mParticleNameVec.reserve(sz + 1);
				conInfo->mContext.mParticleNameVec.resize(sz + 1);
				conInfo->mContext.mParticleNameVec[sz] = mContext.mParticleName;
			}
		}
	}
	void ControlTodChangeComponent::setIsUseImmediateMode(bool isUse)
	{
		if (!m_tMgr)
			return;
		setIsUseCloudImmediateMode(isUse);
		if (mTodComVec[DirecLight_Type] != nullptr)
		{
			static_cast<DirecLightComponent*>(mTodComVec[DirecLight_Type])->setRealTime(isUse);
		}
		if (mTodComVec[Fog_Type] != nullptr)
		{
			static_cast<FogComponent*>(mTodComVec[Fog_Type])->setRealTime(isUse);
		}
		if (mTodComVec[Sky_Type] != nullptr)
		{
			if (isUse)
				m_tMgr->enableImmediateComponent(m_tMgr->getSkyContext(), static_cast<SkyComponent*>(mTodComVec[Sky_Type]));
			else
				m_tMgr->cancelImmediateComponent(m_tMgr->getSkyContext());
		}
		if (mTodComVec[Sun_Type] != nullptr)
		{
			if (isUse)
				m_tMgr->enableImmediateComponent(m_tMgr->getSunContext(), static_cast<SunComponent*>(mTodComVec[Sun_Type]));
			else
				m_tMgr->cancelImmediateComponent(m_tMgr->getSkyContext());
		}
		if (mTodComVec[Ocean_Type] != nullptr)
		{
			if (isUse)
				m_tMgr->enableImmediateComponent(m_tMgr->getOceanContext(), static_cast<OceanComponent*>(mTodComVec[Ocean_Type]));
			else
				m_tMgr->cancelImmediateComponent(m_tMgr->getOceanContext());
		}
		if (mTodComVec[Wind_type] != nullptr)
		{
			if (isUse)
				m_tMgr->enableImmediateComponent(m_tMgr->getWindPowerContext(), static_cast<WindPowerComponent*>(mTodComVec[Wind_type]));
			else
				m_tMgr->cancelImmediateComponent(m_tMgr->getWindPowerContext());
		}
		if (mTodComVec[Caustics_type] != nullptr)
		{
			if (isUse)
				m_tMgr->enableImmediateComponent(m_tMgr->getUnderWaterCausticsContext(), static_cast<UnderWaterCausticsComponent*>(mTodComVec[Caustics_type]));
			else
				m_tMgr->cancelImmediateComponent(m_tMgr->getUnderWaterCausticsContext());
		}
		if (mTodComVec[PointLightMgr_type] != nullptr)
		{
			if (isUse)
				m_tMgr->enableImmediateComponent(m_tMgr->getPointlightMgrContext(), static_cast<PointlightManagerComponent*>(mTodComVec[PointLightMgr_type]));
			else
				m_tMgr->cancelImmediateComponent(m_tMgr->getPointlightMgrContext());
		}
		if (mTodComVec[Lightning_type] != nullptr)
		{
			if (isUse)
				m_tMgr->enableImmediateComponent(m_tMgr->getLightningContext(), static_cast<LightningComponent*>(mTodComVec[Lightning_type]));
			else
				m_tMgr->cancelImmediateComponent(m_tMgr->getLightningContext());
		}
		if (mTodComVec[GlobalLight_type] != nullptr)
		{
			if (isUse)
				m_tMgr->enableImmediateComponent(m_tMgr->getGlobalLightContext(), static_cast<GlobalLightComponent*>(mTodComVec[GlobalLight_type]));
			else
				m_tMgr->cancelImmediateComponent(m_tMgr->getGlobalLightContext());
		}
		if (mTodComVec[HDR_type] != nullptr)
		{
			if (isUse)
				m_tMgr->enableImmediateComponent(m_tMgr->getHDRContext(), static_cast<HDRComponent*>(mTodComVec[HDR_type]));
			else
				m_tMgr->cancelImmediateComponent(m_tMgr->getHDRContext());
		}
		if (mTodComVec[EnvTrigger_type] != nullptr)
		{
			if (isUse)
				m_tMgr->enableImmediateComponent(m_tMgr->getEnvTriggerContext(), static_cast<EnvTriggerEventComponent*>(mTodComVec[EnvTrigger_type]));
			else
				m_tMgr->cancelImmediateComponent(m_tMgr->getEnvTriggerContext());
		}
	}
	void ControlTodChangeComponent::setIsUseTod(bool isUse)
	{
		if (!m_tMgr)
			return;
		setIsUseCloud(isUse);
		if (mTodComVec[Fog_Type])
			m_tMgr->setFogIsUse(isUse);
		if (mTodComVec[DirecLight_Type])
			m_tMgr->setDirecLightIsUse(isUse);
		if (mTodComVec[Sky_Type])
			m_tMgr->setIsUseThisType(m_tMgr->getSkyContext(), isUse);
		if (mTodComVec[Sun_Type])
			m_tMgr->setIsUseThisType(m_tMgr->getSunContext(), isUse);
		if (mTodComVec[Ocean_Type])
			m_tMgr->setIsUseThisType(m_tMgr->getOceanContext(), isUse);
		if (mTodComVec[Wind_type])
			m_tMgr->setIsUseThisType(m_tMgr->getWindPowerContext(), isUse);
		if (mTodComVec[Caustics_type])
			m_tMgr->setIsUseThisType(m_tMgr->getUnderWaterCausticsContext(), isUse);
		if(mTodComVec[Lightning_type])
			m_tMgr->setIsUseThisType(m_tMgr->getLightningContext(), isUse);
		if (mTodComVec[PointLightMgr_type])
			m_tMgr->setIsUseThisType(m_tMgr->getPointlightMgrContext(), isUse);
		if (mTodComVec[GlobalLight_type])
			m_tMgr->setIsUseThisType(m_tMgr->getGlobalLightContext(), isUse);
		if (mTodComVec[HDR_type])
			m_tMgr->setIsUseThisType(m_tMgr->getHDRContext(), isUse);
		if(mTodComVec[EnvTrigger_type])
			m_tMgr->setIsUseThisType(m_tMgr->getEnvTriggerContext(), isUse);
	}
	void ControlTodChangeComponent::setIsUseCloud(bool isUse)
	{
		if (!m_tMgr)
			return;
		if(mTodComVec[CloudMesh_Type])
			m_tMgr->setIsUseThisType(m_tMgr->getCloudMeshContext(), isUse);
		if(mTodComVec[CloudPlane_Type])
			m_tMgr->setIsUseThisType(m_tMgr->getCloudPlaneContext(), isUse);
		if(mTodComVec[CloudSphere_Type])
			m_tMgr->setIsUseThisType(m_tMgr->getCloudSphereContext(), isUse);
	}	
	void ControlTodChangeComponent::setIsUseCloudImmediateMode(bool isUse)
	{
		if (!m_tMgr)
			return;
		if (mTodComVec[CloudMesh_Type] != nullptr)
		{
			if (isUse)
				m_tMgr->enableImmediateComponent(m_tMgr->getCloudMeshContext(), static_cast<CloudMeshComponent*>(mTodComVec[CloudMesh_Type]));
			else
				m_tMgr->cancelImmediateComponent(m_tMgr->getCloudMeshContext());
		}
		if (mTodComVec[CloudPlane_Type] != nullptr)
		{
			if (isUse)
				m_tMgr->enableImmediateComponent(m_tMgr->getCloudPlaneContext(), static_cast<CloudPlaneComponent*>(mTodComVec[CloudPlane_Type]));
			else
				m_tMgr->cancelImmediateComponent(m_tMgr->getCloudPlaneContext());
		}
		if (mTodComVec[CloudSphere_Type] != nullptr)
		{
			if (isUse)
				m_tMgr->enableImmediateComponent(m_tMgr->getCloudSphereContext(), static_cast<CloudSphereComponent*>(mTodComVec[CloudSphere_Type]));
			else
				m_tMgr->cancelImmediateComponent(m_tMgr->getCloudSphereContext());
		}
	}

	void ControlTodChangeComponent::switchEffect(bool isUse)
	{
		if (mTodComVec[Starfield_Type] != nullptr)
		{
			StarfieldComponent* mCom = static_cast<StarfieldComponent*>(mTodComVec[Starfield_Type]);
			mCom->switchStarfield(isUse);
		}
	}

	void ControlTodChangeComponent::setStarfieldName(const std::string& starfieldName_)
	{
		if (starfieldName_.empty())
		{
			mContext.mStarfieldName = "";
			mTodComVec[Starfield_Type] = nullptr;
			return;
		}
		
		const ActorPtr& currActor = getActor();
		if (currActor == nullptr)return;
		ActorComponentPtr currCom = currActor->getComponent(starfieldName_);
		if (currCom == nullptr || currCom->getType() !=  StarfieldComponent::mType )
			return;
		mTodComVec[Starfield_Type] = currCom.get();
		mContext.mStarfieldName = starfieldName_;
	}

	void ControlTodChangeComponent::setEnvironmentLightName(const std::string& envLightName_)
	{
		if (envLightName_.empty())
		{
			mContext.mEnvironmentLightName  = "";
			mTodComVec[EnvironmentLight_Type] = nullptr;
			return;
		}

		const ActorPtr& currActor = getActor();
		if (currActor == nullptr)
			return;
		ActorComponentPtr currCom = currActor->getComponent(envLightName_);
		if (currCom == nullptr || currCom->getType() != EnvironmentLightComponent::mType)
			return;
		mTodComVec[EnvironmentLight_Type] = currCom.get();
		mContext.mEnvironmentLightName = envLightName_;
	}
	void ControlTodChangeComponent::setSceneName(const std::string& sceneName_)
	{
		mContext.mSceneName = sceneName_;
	}
	void ControlTodChangeComponent::setCloudMeshName(const std::string& cloudMeshName_)
	{
		if (cloudMeshName_ .empty())
		{
			mContext.mCloudMeshName =  "";
			mTodComVec[CloudMesh_Type] = nullptr;
			return;
	    }
		const ActorPtr& currActor = getActor();
		if (currActor == nullptr)
			return;
		ActorComponentPtr currCom = currActor->getComponent(cloudMeshName_);
		if (currCom == nullptr || currCom->getType() != CloudMeshComponent::mType)
			return;
        mContext.mCloudMeshName = cloudMeshName_;
		mTodComVec[CloudMesh_Type] = currCom.get();
	}


	void ControlTodChangeComponent::setCloudPlaneName(const std::string& cloudPlaneName_)
	{
		if (cloudPlaneName_ .empty())
		{
			mContext.mCloudPlaneName = "";
			mTodComVec[CloudPlane_Type] = nullptr;
			return;
		}

		const ActorPtr& currActor = getActor();
		if (currActor == nullptr)
			return;
		ActorComponentPtr currCom = currActor->getComponent(cloudPlaneName_);
		if (currCom == nullptr || currCom->getType() != CloudPlaneComponent::mType)
			return;
		mTodComVec[CloudPlane_Type] = currCom.get();
		mContext.mCloudPlaneName = cloudPlaneName_;
	}

	void ControlTodChangeComponent::setCloudSphereName(const std::string& sphereName_)
	{
		if (sphereName_ .empty())
		{
			mContext.mCloudSphereName = "";
			mTodComVec[CloudSphere_Type] = nullptr;
			return;
		}
		const ActorPtr& currActor = getActor();
		if (currActor == nullptr)
			return;
		ActorComponentPtr currCom = currActor->getComponent(sphereName_);
		if (currCom == nullptr || currCom->getType() != CloudSphereComponent::mType)
			return;
		mTodComVec[CloudSphere_Type] = currCom.get();
		mContext.mCloudSphereName = sphereName_;
	}
	
	void ControlTodChangeComponent::setDirecLightName(const std::string& direcLightName_)
	{
		if (direcLightName_.empty())
		{
			mContext.mDirecLightName = "";
			mTodComVec[DirecLight_Type] = nullptr;
			return;
		}
		const ActorPtr& currActor = getActor();
		if (currActor == nullptr)
			return;
		ActorComponentPtr currCom = currActor->getComponent(direcLightName_);
		if (currCom == nullptr || currCom->getType() != DirecLightComponent::mType)
			return;
		mTodComVec[DirecLight_Type] = currCom.get();
		mContext.mDirecLightName = direcLightName_;
	}

	void ControlTodChangeComponent::setFogName(const std::string& fogName_)
	{
		if (fogName_.empty())
		{
			mContext.mFogName = "";
			mTodComVec[Fog_Type] = nullptr;
			return;
		}
		const ActorPtr& currActor = getActor();
		if (currActor == nullptr)
			return;
		ActorComponentPtr currCom = currActor->getComponent(fogName_);
		if (currCom == nullptr || currCom->getType() != FogComponent::mType)
			return;
		mTodComVec[Fog_Type] = currCom.get();
		mContext.mFogName = fogName_;
	}
	
	void ControlTodChangeComponent::setSkyName(const std::string& skyName_)
	{
		if (skyName_.empty())
		{
			mContext.mSkyName = "";
			mTodComVec[Sky_Type] = nullptr;
			return;
		}
		const ActorPtr& currActor = getActor();
		if (currActor == nullptr)
			return;
		ActorComponentPtr currCom = currActor->getComponent(skyName_);
		if (currCom == nullptr || currCom->getType() != SkyComponent::mType)
			return;
		mTodComVec[Sky_Type] = currCom.get();
		mContext.mSkyName = skyName_;
	}
	
	void ControlTodChangeComponent::setSunName(const std::string& sunName)
	{
		if (sunName.empty())
		{
			mContext.mSunName = "";
			mTodComVec[Sun_Type] = nullptr;
			return;
		}
		const ActorPtr& currActor = getActor();
		if (currActor == nullptr)
			return;
		ActorComponentPtr currCom = currActor->getComponent(sunName);
		if (currCom == nullptr || currCom->getType() != SunComponent::mType)
			return;
		mTodComVec[Sun_Type] = currCom.get();
		mContext.mSunName = sunName;
	}

	void ControlTodChangeComponent::setOceanName(const String& oceanName)
	{
		if (oceanName.empty())
		{
			mContext.mSunName = "";
			mTodComVec[Ocean_Type] = nullptr;
			return;
		}
		const ActorPtr& currActor = getActor();
		if (currActor == nullptr)
			return;
		ActorComponentPtr currCom = currActor->getComponent(oceanName);
		if (currCom == nullptr || currCom->getType() != OceanComponent::mType)
			return;
		mTodComVec[Ocean_Type] = currCom.get();
		mContext.mOceanName = oceanName;
	}
	void ControlTodChangeComponent::setWindPowerName(const String& windPowerName)
	{
		if (windPowerName.empty())
		{
			mContext.mWindPowerName = "";
			mTodComVec[Wind_type] = nullptr;
			return;
		}
		const ActorPtr& currActor = getActor();
		if (currActor == nullptr)
			return;
		ActorComponentPtr currCom = currActor->getComponent(windPowerName);
		if (currCom == nullptr || currCom->getType() != WindPowerComponent::mType)
			return;
		mTodComVec[Wind_type] = currCom.get();
		mContext.mWindPowerName = windPowerName;
	}
	void ControlTodChangeComponent::setCausticsName(const String& CausticsName)
	{
		if (CausticsName.empty())
		{
			mContext.mUnderWaterCausticsName = "";
			mTodComVec[Caustics_type] = nullptr;
			return;
		}
		const ActorPtr& currActor = getActor();
		if (currActor == nullptr)
		return;
		ActorComponentPtr currCom = currActor->getComponent(CausticsName);
		if (currCom == nullptr || currCom->getType() != UnderWaterCausticsComponent::mType)
			return;
		mTodComVec[Caustics_type] = currCom.get();
		mContext.mUnderWaterCausticsName = CausticsName;
	}
	void ControlTodChangeComponent::setLightningName(const String& LightningName)
	{
		if (LightningName.empty())
		{
			mContext.mLightningName = "";
			mTodComVec[Lightning_type] = nullptr;
			return;
		}
		const ActorPtr& currActor = getActor();
		if (currActor == nullptr)
			return;
		ActorComponentPtr currCom = currActor->getComponent(LightningName);
		if (currCom == nullptr || currCom->getType() != LightningComponent::mType)
			return;
		mTodComVec[Lightning_type] = currCom.get();
		mContext.mLightningName = LightningName;
	}
	void ControlTodChangeComponent::setTodTimelineName(const std::string& todTimelineName_)
	{
		if (todTimelineName_.empty())
		{
			mContext.mTodTimelineName = "";
			mTodComVec[TodTime_Type] = nullptr;
			return;
		}
		const ActorPtr& currActor = getActor();
		if (currActor == nullptr)
			return;
		ActorComponentPtr currCom = currActor->getComponent(todTimelineName_);
		if (currCom == nullptr || currCom->getType() != TodTimelineComponent::mType)
			return;
		mTodComVec[TodTime_Type] = currCom.get();
		mContext.mTodTimelineName = todTimelineName_;
	}
	
	void ControlTodChangeComponent::setBoxName(const std::string& boxName)
	{
		if (boxName.empty())
		{
			mContext.mBoxName = "";
			mTodComVec[Box_Type] = nullptr;
			return;
		}
		const ActorPtr& currActor = getActor();
		if (currActor == nullptr)
			return;
		ActorComponentPtr currCom = currActor->getComponent(boxName);
		if (currCom == nullptr || currCom->getType() != BoxComponent::mType)
			return;
		mTodComVec[Box_Type] = currCom.get();
		mContext.mBoxName = boxName;
	}
	
	void ControlTodChangeComponent::setParticleName(const std::pair<std::string, size_t>& particleData)
	{
		if (particleData.second >= mParticleComVec.size())
			return;
		if (particleData.first.empty())
		{
			mContext.mParticleNameVec[particleData.second] = "";
			mParticleComVec[particleData.second] = nullptr;
			return;
		}
		const ActorPtr& currActor = getActor();
		if (currActor == nullptr)
			return;
		ActorComponent* currCom = currActor->getComponent(particleData.first).get();
		if (currCom == nullptr || currCom->getType() != ParticleSystemComponent::mType)
			return;
		mParticleComVec[particleData.second] = static_cast<ParticleSystemComponent*>(currCom);
		mContext.mParticleNameVec[particleData.second] = particleData.first;
	}

	void ControlTodChangeComponent::setParticleSize(size_t sz)
	{
		if (sz >= mParticleComVec.size())
		{
			mParticleComVec.reserve(sz);
			mParticleComVec.resize(sz);
		}
		else
		{
			for (size_t i = sz; i < mParticleComVec.size(); ++i)
			{
				mParticleComVec[i] = nullptr;
			}
			mParticleComVec.erase(mParticleComVec.begin() + sz, mParticleComVec.end());
		}
		if (sz >= mContext.mParticleNameVec.size())
		{
			mContext.mParticleNameVec.reserve(sz);
			mContext.mParticleNameVec.resize(sz);
		}
		else
		{
			mContext.mParticleNameVec.erase(mContext.mParticleNameVec.begin() + sz, mContext.mParticleNameVec.end());
		}
	}

	void ControlTodChangeComponent::setParticleNameVec(const std::vector<std::string>& particleName_)
	{
		mContext.mParticleNameVec = particleName_;
	}

	void ControlTodChangeComponent::setCameraPosition(const Vector3& cameraPosition_)
	{
		mContext.mCameraPosition = cameraPosition_;
	}
	void ControlTodChangeComponent::setPointlightManagerName(const String& pLightMgrName)
	{
		if (pLightMgrName.empty())
		{
			mContext.mPointlightMgrName = "";
			mTodComVec[PointLightMgr_type] = nullptr;
			return;
		}
		const ActorPtr& currActor = getActor();
		if (currActor == nullptr)
			return;
		ActorComponentPtr currCom = currActor->getComponent(pLightMgrName);
		if (currCom == nullptr || currCom->getType() != PointlightManagerComponent::mType)
			return;
		mTodComVec[PointLightMgr_type] = currCom.get();
		mContext.mPointlightMgrName = pLightMgrName;
	}
	void ControlTodChangeComponent::setGlobalLightName(const String& globalLightName)
	{
		if (globalLightName.empty())
		{
			mContext.mGlobalLightName = "";
			mTodComVec[GlobalLight_type] = nullptr;
			return;
		}
		const ActorPtr& currActor = getActor();
		if (currActor == nullptr)
			return;
		ActorComponentPtr currCom = currActor->getComponent(globalLightName);
		if (currCom == nullptr || currCom->getType() != GlobalLightComponent::mType)
			return;
		mTodComVec[GlobalLight_type] = currCom.get();
		mContext.mGlobalLightName = globalLightName;
	}

	void ControlTodChangeComponent::setHDRName(const String& HDRName)
	{
		if (HDRName.empty())
		{
			mContext.mHDRName = "";
			mTodComVec[HDR_type] = nullptr;
			return;
		}
		const ActorPtr& currActor = getActor();
		if (currActor == nullptr)
			return;
		ActorComponentPtr currCom = currActor->getComponent(HDRName);
		if (currCom == nullptr || currCom->getType() != HDRComponent::mType)
			return;
		mTodComVec[HDR_type] = currCom.get();
		mContext.mHDRName = HDRName;
	}
	void ControlTodChangeComponent::setEnvTriggerName(const String& envTriggerName)
	{
		if (envTriggerName.empty())
		{
			mContext.mEnvTriggerName = "";
			mTodComVec[EnvTrigger_type] = nullptr;
			return;
		}
		const ActorPtr& currActor = getActor();
		if (currActor == nullptr)
			return;
		ActorComponentPtr currCom = currActor->getComponent(envTriggerName);
		if (currCom == nullptr || currCom->getType() != EnvTriggerEventComponent::mType)
			return;
		mTodComVec[EnvTrigger_type] = currCom.get();
		mContext.mEnvTriggerName = envTriggerName;
	}
	void ControlTodChangeComponent::setSceneId(uint32 sceneId_)
	{
		mContext.mSceneID = sceneId_;
	}

	template <class T>
	void SetComPriority(const Echo::ActorPtr& act, int priority)
	{
		if (act.expired())
			return;
		auto component = act->GetFirstComByType<T>();
		if (component.expired()) return;
		component->setPriority(priority);
	}

	void ControlTodChangeComponent::SetPriority(const Echo::ActorPtr& ptr, int priority)
	{
		if (ptr.expired()) return;
		for (auto& actor : ptr->getChildActors())
		{
			SetComPriority<TodTimelineComponent>(actor.second, priority);
			SetComPriority<CloudMeshComponent>(actor.second, priority);
			SetComPriority<CloudPlaneComponent>(actor.second, priority);
			SetComPriority<CloudSphereComponent>(actor.second, priority);
			SetComPriority<DirecLightComponent>(actor.second, priority);
			SetComPriority<FogComponent>(actor.second, priority);
			SetComPriority<SkyComponent>(actor.second, priority);
			SetComPriority<SunComponent>(actor.second, priority);
			SetComPriority<OceanComponent>(actor.second, priority);
			SetComPriority<UnderWaterCausticsComponent>(actor.second, priority);
			SetComPriority<PointlightManagerComponent>(actor.second, priority);
			SetComPriority<WindPowerComponent>(actor.second, priority);
			SetComPriority<EnvTriggerEventComponent>(actor.second, priority);
			SetComPriority<DriftComponent>(actor.second, priority);
			SetComPriority<StarsComponent>(actor.second, priority);
			SetComPriority<WeatherCoverColorComponent>(actor.second, priority);
			SetComPriority<LightningComponent>(actor.second, priority);
			SetComPriority<HDRComponent>(actor.second, priority);
			SetComPriority<GlobalLightComponent>(actor.second, priority);
			SetComPriority<VolumeLightComponent>(actor.second, priority);
			SetComPriority<TODShadowComponent>(actor.second, priority);
			SetComPriority<SphereFogComponent>(actor.second, priority);
			SetComPriority<HDRAdaptiveComponent>(actor.second, priority);
		}
	}



	void ControlTodChangeComponent::initTodVecState(ActorComponentInfo* info)
	{
		if (info == nullptr)
			return;
		ControlTodChangeComponentInfo* controlInfo_ = static_cast<ControlTodChangeComponentInfo*>(info);
		mContext = controlInfo_->mContext;
		setCloudMeshName(controlInfo_->mContext.mCloudMeshName);
		setCloudPlaneName(controlInfo_->mContext.mCloudPlaneName);
		setCloudSphereName(controlInfo_->mContext.mCloudSphereName);
		setDirecLightName(controlInfo_->mContext.mDirecLightName);
		setFogName(controlInfo_->mContext.mFogName);
		setSkyName(controlInfo_->mContext.mSkyName);
		setSunName(controlInfo_->mContext.mSunName);
		setCausticsName(controlInfo_->mContext.mUnderWaterCausticsName);
		setOceanName(controlInfo_->mContext.mOceanName);
		setWindPowerName(controlInfo_->mContext.mWindPowerName);
		setTodTimelineName(controlInfo_->mContext.mTodTimelineName);
		setBoxName(controlInfo_->mContext.mBoxName);
		setSceneId(controlInfo_->mContext.mSceneID);
		setSceneName(controlInfo_->mContext.mSceneName);
		setLightningName(controlInfo_->mContext.mLightningName);
		setCameraPosition(controlInfo_->mContext.mCameraPosition);
		setPointlightManagerName(controlInfo_->mContext.mPointlightMgrName);
		setGlobalLightName(controlInfo_->mContext.mGlobalLightName);
		setHDRName(controlInfo_->mContext.mHDRName);
		setEnvTriggerName(controlInfo_->mContext.mEnvTriggerName);
	}
	void ControlTodChangeComponent::initTransTime()
	{
	    if (mTodComVec[DirecLight_Type])
		{
			DirecLightComponent* direcCom_ = static_cast<DirecLightComponent*>(mTodComVec[DirecLight_Type]);
			direcCom_->setTransFormTime(mContext.mTodTransTime);

		}
		if (mTodComVec[Fog_Type])
		{
			FogComponent* fogCom_ = static_cast<FogComponent*>(mTodComVec[Fog_Type]);
			fogCom_->setTransFormTime(mContext.mTodTransTime);

		}
		if (mTodComVec[Sky_Type])
		{
			SkyComponent* skyCom_ = static_cast<SkyComponent*>(mTodComVec[Sky_Type]);
			skyCom_->setTransFormTime(mContext.mTodTransTime);

		}
		if (mTodComVec[Sun_Type])
		{
			SunComponent* sunCom_ = static_cast<SunComponent*>(mTodComVec[Sun_Type]);
			sunCom_->setTransFormTime(mContext.mTodTransTime);

		}

		if (mTodComVec[CloudMesh_Type])
		{
			CloudMeshComponent* cloudMeshCom_ = static_cast<CloudMeshComponent*>(mTodComVec[CloudMesh_Type]);
			cloudMeshCom_->setTransFormTime(mContext.mCloudTransTime);
		}

		if (mTodComVec[CloudSphere_Type])
		{
			CloudSphereComponent* cloudSphereCom_ = static_cast<CloudSphereComponent*>(mTodComVec[CloudSphere_Type]);
			cloudSphereCom_->setTransFormTime(mContext.mCloudTransTime);
		}

		if (mTodComVec[CloudPlane_Type])
		{
			CloudPlaneComponent* cloudPlaneCom_ = static_cast<CloudPlaneComponent*>(mTodComVec[CloudPlane_Type]);
			cloudPlaneCom_->setTransFormTime(mContext.mCloudTransTime);
		}
	}
	void ControlTodChangeComponent::changeTodEffection(bool isUse)
	{
		changeCloudMeshEffection(isUse);
		changeCloudEffection(isUse);
		changeCloudOtherEffection(isUse);
	}
	void ControlTodChangeComponent::changeCloudMeshEffection(bool isUse)
	{
		if (mTodComVec[Box_Type] == nullptr)
			return;
		BoxComponent* boxCom_ = static_cast<BoxComponent*>(mTodComVec[Box_Type]);
		int priority_ = boxCom_->getPriority();
		
		if (mTodComVec[CloudMesh_Type] != nullptr)
		{
			CloudMeshComponent* cloudMeshCom = static_cast<CloudMeshComponent*>(mTodComVec[CloudMesh_Type]);
			if (isUse)
				cloudMeshCom->setPriority(priority_);
		}
	}
	void ControlTodChangeComponent::changeCloudEffection(bool isUse)
	{
		if (mTodComVec[Box_Type] == nullptr)
			return;
		BoxComponent* boxCom_ = static_cast<BoxComponent*>(mTodComVec[Box_Type]);
		int priority_ = boxCom_->getPriority();
	
		if (mTodComVec[CloudPlane_Type] != nullptr)
		{
			CloudPlaneComponent* cloudPlaneCom = static_cast<CloudPlaneComponent*>(mTodComVec[CloudPlane_Type]);
			if (isUse)
				cloudPlaneCom->setPriority(priority_);
		}

		if (mTodComVec[CloudSphere_Type] != nullptr)
		{
			CloudSphereComponent* cloudSphereCom = static_cast<CloudSphereComponent*>(mTodComVec[CloudSphere_Type]);
			if (isUse)
				cloudSphereCom->setPriority(priority_);
		}
	}
	void ControlTodChangeComponent::changeCloudOtherEffection(bool isUse)
	{
		for(ParticleSystemComponent* particle_ : mParticleComVec)
		{
			if (particle_)
			{
				particle_->PlayParticle(!isUse);
				particle_->SetPalyTime(0.f);
			}
		}
	}
	void ControlTodChangeComponent::changeScene()
	{
		SceneComponentManager* sceneMgr = SceneComponentManager::instance();
		if (sceneMgr == nullptr || mContext.mSceneName.empty())
		{
			return;
		}
		if (m_tMgr)
		{
			m_tMgr->setTransState(S_NoneTrans);
			m_tMgr->clearFinalComVec();
		}
		sceneMgr->setChangeSceneState(true);
		sceneMgr->setChangeSceneID(mContext.mSceneID);
		sceneMgr->setChangeSceneName(mContext.mSceneName);
		sceneMgr->setCameraPosition(mContext.mCameraPosition);
	}
	bool ControlTodChangeComponent::isPosInBox()
	{
		if (mTodComVec[Box_Type] == nullptr)
			return false;
		BoxComponent* boxCom_ = static_cast<BoxComponent*>(mTodComVec[Box_Type]);

		SceneManager* scMgr = getSceneManager();
		if (!scMgr)
			return false;
		const Vector3 cameraPos_ = scMgr->getMainCamera()->getPosition();
		return boxCom_->isPosInBox(cameraPos_);
	}
	bool ControlTodChangeComponent::isPosInEpitaxyBox()
	{
		if (mTodComVec[Box_Type] == nullptr)
			return false;
		BoxComponent* boxCom_ = static_cast<BoxComponent*>(mTodComVec[Box_Type]);
		SceneManager* scMgr = getSceneManager();
		if (!scMgr)
			return false;
		const Vector3 cameraPos_ = scMgr->getMainCamera()->getPosition();
		return boxCom_->isPosInEpitaxyBox(cameraPos_);
	}
	bool ControlTodChangeComponent::isPosInTripleEpitaxyBox()
	{
		if (mTodComVec[Box_Type] == nullptr)
			return false;
		BoxComponent* boxCom_ = static_cast<BoxComponent*>(mTodComVec[Box_Type]);
		SceneManager* scMgr = getSceneManager();
		if (!scMgr)
			return false;
		const Vector3 cameraPos_ = scMgr->getMainCamera()->getPosition();
		return boxCom_->isPosInTripleEpitaxyBox(cameraPos_);
	}


	void ControlTodChangeComponent::EditorPauseImp(bool pause)
	{
		mIsTrigger = false;
	}
	void ControlTodChangeComponent::EditorStopImp()
	{
		mIsTrigger = false;
	}

	void ControlTodChangeComponent::setTodActorFilePath(const std::string& path_)
	{
		mContext.m_TodActorName = path_;
		//写成同步测试
		if (!mContext.m_TodActorName.empty() && getActorManager())
		{
			m_TodActor = getActorManager()->createActor(Name(mContext.m_TodActorName), "", true);
			m_pActor->addActor(m_pActor->getRootNodeCom(), m_TodActor);
			m_TodActor->setVisiable(false);
		}
	}

	void ControlTodChangeComponent::updateActiveTod()
	{
		if (!m_tMgr)
			return;
		ControlTodChangeComponent*  com_ = m_tMgr->_GetFirstControlCom();
		if (com_)
			com_->_updateActiveTod(true);
		else
			_updateActiveTod(false);
	}

	void ControlTodChangeComponent::_updateActiveTod(bool isUpdateActor)
	{
		if (!m_tMgr)
			return;
		
		m_tMgr->setTransState(S_Trans);
		if(!mContext.m_isEffectTod)		// 判断是否为镜头效果，非镜头效果则关闭所有组件的生效状态（已被过渡状态拦截）
		   m_tMgr->closeAllTodCom();
		m_tMgr->setTransFormTimeFromClient(mContext.mTodTransTime);
		m_tMgr->setWeatherTypeByCom(mContext.m_StfID); // 设置stf的id数据
		if (isUpdateActor)
		{
			if (!m_TodActor.expired())
			{
				SetPriority(m_TodActor, mContext.m_Priority);

				for (auto& p : m_TodActor->getComponents())
				{
					m_tMgr->setTodComIsUse(p.second.get(), true);
				}

				for (auto& actor : m_TodActor->getChildActors())
				{
					for (auto& p : actor.second->getComponents())
					{
						m_tMgr->setTodComIsUse(p.second.get(), true);
					}
				}
			}
		}
		m_tMgr->enterOrLeaveClientArea();
		
	}
}

