#include "EchoComponentStableHeaders.h"
#include "EchoTODComponentManager.h"

#include "EchoSceneManager.h" 
#include "EchoSun.h"
#include "EchoRenderStrategy.h"
#include "EchoPostProcess.h"
#include "EchoSky.h"
#include "EchoOcean.h"
#include "EchoFFTOcean.h"
#include "EchoWorldManager.h"
#include "EchoTODManager.h"
#include "EchoSceneArtData.h"
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
#include "EchoDriftComponent.h"
#include "EchoEnvTriggerEvent.h"
#include "EchoSceneTriggerData.h"
#include "EchoStarsComponent.h"
#include "EchoStars.h"
#include "EchoWeatherCoverColorComponent.h"
#include "EchoEngine.h"
#include "EchoTODComponentSystem.h"
#include "EchoVolumeLightComponent.h"
#include "EchoTODShadowComponent.h"
#include "EchoSphereFogComponent.h"
#include "EchoHDRAdaptiveComponent.h"
#include "EchoVideoConfigSystem.h"
#include "EchoSphericalTerrainManager.h"
#include "EchoSphericalTerrainComponent.h"
#include "EchoPlanetConverterComponent.h"
#include "EchoSphericalTerrainFog.h"
#include "EchoControlTodChangeComponent.h"
#include "EchoASSAO.h"
#include "EchoSphericalOcean.h"
#include "EchoSphericalOceanComponent.h"
#include "EchoDeferredRenderStrategy.h"
#include "EchoTODDataUtil.h"

//stfid不存在时需要给一个报错信息的log

namespace Echo
{

	Name TODComponentManagerType = Name("TODComponentManager");

	TODComponentManager::TODComponentManager(const Name& name, ProtocolComponentManager* pMgr)
		:ProtocolComponent(pMgr)
		,mName(name)
	{
		registerTickTimeBegin();

		mFinalComVec.reserve(TodComType::T_End);
		mFinalComVec.resize(TodComType::T_End,nullptr);
		mTodTransTime.reserve(TodComType::T_End);
		mTodTransTime.resize(TodComType::T_End, 0.f);
		for (int i = 0; i < TodComType::T_End; ++i)
		{
			mTodTransState[i] = false;
			mIsTodTrans[i] = false;
			mTodComIsChange[i] = false;;
		}
		for (int i = T_CloudMesh; i <= T_CloudSphere; i++)
			mCloudState[i- T_CloudMesh] = Cloud_None;

		initTodTransData();

		mWeatherManager = WeatherSystem::instance()->GetManager(mName);
	    mTodManager = TODSystem::instance()->GetManager(mName);
		if (mTodManager)
			mSceneManager = mTodManager->getSceneManager();

#ifdef ECHO_EDITOR
		mIsEffectiveModify = true;
#else
		mIsEffectiveModify = false;
#endif // ECHO_EDITOR
	}

	TODComponentManager::TODComponentManager(const Name& name, const TodComManagerHandle& mgrHandle_)
		:ProtocolComponent(mgrHandle_.mProtocolComManager)
		, mName(name)
		,mWorldManager(mgrHandle_.mWorldManager)
		,mSceneManager(mgrHandle_.mSceneManager)
		,mWeatherManager(mgrHandle_.mWeatherManager)
		,mTodManager(mgrHandle_.mTodManager)
	{
		registerTickTimeBegin();
        mFinalComVec.reserve(TodComType::T_End);
		mFinalComVec.resize(TodComType::T_End,nullptr);
		mTodTransTime.reserve(TodComType::T_End);
		mTodTransTime.resize(TodComType::T_End, 0.f);
		for (int i = 0; i < TodComType::T_End; ++i)
		{
			mTodTransState[i] = false;
			mIsTodTrans[i] = false;
			mTodComIsChange[i] = false;;
		}
		for (int i = T_CloudMesh; i <= T_CloudSphere; i++)
			mCloudState[i- T_CloudMesh] = Cloud_None;

		initTodTransData();
	}
	TODComponentManager* TODComponentManager::instance()
	{
		return 	TODComponentSystem::instance()->getMainTODComponentManager();
	}

	void TODComponentManager::getProtocolType(Name& type)
	{
		type = TODComponentManagerType;
	}
	void TODComponentManager::getProtocolTypeToString(std::string& typeStr)
	{
		typeStr = TODComponentManagerType.c_str();
	}
	TODComponentManager::~TODComponentManager()
	{
		mWorldManager = nullptr;
		uninitTodTransData();
		deinitTodComData();
	}
	void TODComponentManager::setIsUseSecondGroup(bool isUse_)
	{
		mIsUseSecondGroup = isUse_;
		if (mEnvTriggerContext.mActiveCom)
		{
			mEnvTriggerContext.mActiveCom->_setIsUseSecondTriggerGroup(isUse_);
		}
	}
	void TODComponentManager::SetManagerByHandle(const TodComManagerHandle& mgrHandle_)
	{
		    mWorldManager = mgrHandle_.mWorldManager;
			mSceneManager = mgrHandle_.mSceneManager;
			mWeatherManager = mgrHandle_.mWeatherManager;
			mTodManager = mgrHandle_.mTodManager;
			m_pProtocolComponentManager = mgrHandle_.mProtocolComManager;
	}
	void TODComponentManager::deinitTodComData()
	{
		mSceneTriggerData = nullptr;
		mCurrSceneArtData = nullptr;
		mTransState = S_NoneTrans;
		mIsChangeTodStfID = false;
		mIsEffectiveModify = true;
		closeAllTodCom();
		clearAllTodComVec();
		clearFinalComVec();
		deleteCloudSphere();
		deleteCloudMesh();
		deleteCloudPlane();
		deleteSky();
		deleteSun();
		deleteOcean();
		deleteStars();
		clearUpTodTexturePtrMap();
		clearCloudMeshPtrMap();
		mIsEffectiveModify = false;
	}
	void TODComponentManager::beginFrame(EventID eventId_, ProtocolHandle)
	{
        if (!mTodManager)
			return;

		float elapsedTime_ = mIsUseEchoTick ? (float)(mTodManager->getTimeElapsed()) : Echo::Root::instance()->getTimeSinceLastFrameClient();
		if (mEnvTriggerContext.mIsUseCurrType && mEnvTriggerContext.mActiveCom)
		{ 
			switch (eventId_)
			{
			case EID_EveryFrame:
			{
				mEnvTriggerContext.mActiveCom->update(elapsedTime_);
				break;
			}
			default:
				break;
			}
		}
	}
	void TODComponentManager::preUpdateScene(EventID eventId_, ProtocolHandle)
	{
		if (!mTodManager)
			return;

		mTimeElapsed = mIsUseEchoTick ? (float)(mTodManager->getTimeElapsed()) : Echo::Root::instance()->getTimeSinceLastFrameClient();

		//if (mEnvTriggerContext.mIsUseCurrType && mEnvTriggerContext.mActiveCom)
		//{ 
		//	switch (eventId_)
		//	{
		//	case EID_EveryFrame:
		//	{
		//		mEnvTriggerContext.mActiveCom->update(mTimeElapsed);
		//		break;
		//	}
		//	default:
		//		break;
		//	}
		//}


		//demo
		//if (mSceneManager)
		//{
		//	PlanetManager* pMgr = mSceneManager->getPlanetManager();
		//	if (pMgr)
		//	{
		//		SphericalTerrain* terrain = pMgr->getNearestPlanet();
		//		Camera* pCam = mSceneManager->getMainCamera();
		//		if (terrain && pCam)
		//		{
		//			DVector3 camPos = pCam->getPosition();
		//			Vector3 gravityDir = terrain->getGravityWs(camPos) * -1.f;
		//			gravityDir.normalise();
		//			useAllGravityType();
		//
		//			//float radius = terrain->getRadius();
		//			float eleHeight = (float)terrain->getElevation(camPos);
		//
		//			Vector3 camDir = camPos - terrain->m_pos;
		//			camDir.normalise();
		//			Vector3 groundPos = terrain->getGroundPos(camPos);
		//			setTODGravityData(gravityDir, eleHeight, groundPos);
		//		}
		//	}
		//	else
		//		unUseAllGravityType();
		//}

		// 未发生过渡或者stf未加载完
		if (mTransState == S_NoneTrans || mIsChangeTodStfID == true)
			return;
		
		if (!mIsTodDataLoadFinish)
			return;


		if ((TODSystem::instance()->showLog(LogType::LT_CloudMesh) || TODSystem::instance()->showLog(LogType::LT_CloudSphere) || TODSystem::instance()->showLog(LogType::LT_CloudPlane)) && GetSceneManager()->getName() == g_MainTodComponentName)
			LogManager::instance()->logMessage(" Insert Time : " + std::to_string(mTimeElapsed) + " useEchoTick : " + std::to_string(mIsUseEchoTick));


		if ((TODSystem::instance()->showLog(LogType::LT_CloudMesh) || TODSystem::instance()->showLog(LogType::LT_CloudSphere) || TODSystem::instance()->showLog(LogType::LT_CloudPlane)) && mName == g_MainTodComponentName)
			LogManager::instance()->logMessage("[ TODComponentManager ] ----- PreUpdateSceneBegin ----- [Begin]");

		switch (eventId_)
		{
		case EID_EveryFrame:
		{
			_dealWithFirstTick();
			if (mIsChangeWeatherByClient)
			{
				_setTodDayTimeAttribute(TODSystem::instance()->mCancelModification ? false : true);
				_getTransFinalState(false);
			}
			if (mIsUpdateTod)
			{
				updateTod();
			}
			if (mIsUpdateCloud)
			{
				updateCloud();
			}
			if (!mIsUpdateCloud && !mIsUpdateTod)
			{
				setTransStateEnd();
			}
			break;
		}
		default:
			break;
		}

		if ((TODSystem::instance()->showLog(LogType::LT_CloudMesh) || TODSystem::instance()->showLog(LogType::LT_CloudSphere) || TODSystem::instance()->showLog(LogType::LT_CloudPlane)) && mName == g_MainTodComponentName)
			LogManager::instance()->logMessage("[ TODComponentManager ] ----- PreUpdateSceneBegin ----- [End]");
	}
	
	void TODComponentManager::registerTickTimeBegin()
	{
		m_eventSet[TT_PreUpdateScene].insert(EID_EveryFrame);

		m_eventSet[TT_BeginFrame].insert(EID_EveryFrame);
		registerTickTimeEnd();
	}



	void TODComponentManager::setTransState(TodComTransState state_)
	{
		mTransState = state_;
		mTodElapsed = 0.f;
		mCloudElapsed = 0.f;
		mElapsedTime = 0.f;
		/*if (mTodManager && mTodManager->judgeIsFirstUpdate())
		{
			mTodElapsed = 600.f;
			mCloudElapsed = 600.f;
		}*/

		if (state_ == S_NoneTrans)
			return;
		mIsFirstTodTick = true;
		getWeatherTransStartState();
	}

	/*void TODComponentManager::setWeatherTypeByCom(int uType, float transTime,  const std::string& weatherName, bool isChangeSameAttr, bool bTransition, bool isDay)
	{
		if (weatherName.empty())
			return;
		setWeatherTypeByCom(uType, transTime, isChangeSameAttr, bTransition, isDay);
	}*/
	void  TODComponentManager::setWeatherTypeByCom(int uType, float transTime, bool isChangeSameAttr, bool bTransition, bool isDay)
	{												// uType = stf ID
		mIsTransSameAttr = isChangeSameAttr;
	
		if (mIsTransSameAttr)
			mIsChangeTodStfID = true;
		else
		{
			if (mTodId != uType)
				mIsChangeTodStfID = true;
			else
				mIsChangeTodStfID = false;
		}

		if (uType < 0)
			mIsChangeTodStfID = false;

		mTodId = uType;
		mIsChangeWeatherByClient = true;

		std::stringstream ss;
		ss << "SetWeatherType by Com " << std::endl;
		ss << "Is Change Tod Stf Id : " << mIsChangeTodStfID << std::endl;
		LogManager::instance()->logMessage(ss.str());

		if (mWorldManager != nullptr && mIsChangeTodStfID)
			mWorldManager->setWeatherType(uType, transTime, bTransition, isDay);
	}

	void TODComponentManager::getWeatherTransStartState()
	{
		
		if (mSceneManager)
		{
			Cloud* pCloud = mSceneManager->getCloud();
			if (pCloud)
				_getCloudData(pCloud);

			if(mDirecData)
			   _getSceneDirecLightData(*mDirecData);

			if (mFogData)
				_getFogAttribute(*mFogData);

			if (mSkyData)
				_getSkyAttribute(*mSkyData);
			
			if (mSunData)
				_getSunAttribute(*mSunData);

			if (mSphereFogData)
				_getSphereFogAttribute(*mSphereFogData);

#define GetWeatherTransStartState(Name)	\
			if (m##Name##Handle)	\
				_get##Name##Attribute(*m##Name##Handle);

			GetWeatherTransStartState(EnvironmentLight)
			GetWeatherTransStartState(GlobalLight)
			GetWeatherTransStartState(HDR)
			GetWeatherTransStartState(HDRAdaptive)
			GetWeatherTransStartState(Ocean)
			GetWeatherTransStartState(PointLight)
			GetWeatherTransStartState(Stars)
			GetWeatherTransStartState(UnderWaterCaustics)
			GetWeatherTransStartState(WeatherCoverColor)
			GetWeatherTransStartState(TODShadow)
			GetWeatherTransStartState(SphericalOcean)
#undef GetWeatherTransStartState
		}

	}

	void TODComponentManager::addDirecLight(DirecLightComponent* item)
	{
	 	if (item == nullptr)
			return;
		addTODItem<DirecLightComponent>(item, mDirecVec);
		updateDirecLight(nullptr);
	}


	void TODComponentManager::addFog(FogComponent * item)
	{
		if (item == nullptr)
			return;
		addTODItem<FogComponent>(item, mFogVec);
		updateFog(nullptr);
	}


	void TODComponentManager::deleteDireclight(DirecLightComponent *item)
	{
		if (item == nullptr)
			return;

		auto iter = std::find(mDirecVec.begin(), mDirecVec.end(), item);
		if (iter != mDirecVec.end())
		   mDirecVec.erase(iter);

		updateDirecLight(item);

		if (item == mCurrActiveDirecCom)
			mCurrActiveDirecCom = nullptr;
	}

	void TODComponentManager::deleteFog(FogComponent *item)
	{
		if (item == nullptr)
			return;

		auto iter = std::find(mFogVec.begin(), mFogVec.end(), item);
		if (iter != mFogVec.end())
			mFogVec.erase(iter);

		updateFog(item);

		if (item == mCurrActiveFogCom)
			mCurrActiveFogCom = nullptr;
	}

	void TODComponentManager::DirecLightPriorityChanged(DirecLightComponent *item)
	{
		if (item == nullptr)
			return;
		TODItemPriorityChanged<DirecLightComponent>(item, mDirecVec);
		updateDirecLight(nullptr);
	}

	void TODComponentManager::FogPriorityChanged(FogComponent * item)
	{
		if (item == nullptr)
			return;
		TODItemPriorityChanged<FogComponent>(item, mFogVec);
		updateFog(nullptr);
	}

	void TODComponentManager::setFogIsUse(bool isUse_)
	{
		mIsUseFog = isUse_;
		updateFog(nullptr);
	}

	void TODComponentManager::setCurrSunDirection(const Vector3 & dir_)
	{
		if (mSunContext.mActiveCom == nullptr || !mSunContext.mIsUseCurrType)
		{
			if (mSunContext.mDefaultSceneObj != nullptr)
				mSunContext.mDefaultSceneObj->setSunDirection(dir_);
			else
			{
				if (mSceneManager)
				{
					Sun* sun = mSceneManager->getSun();
					if (sun)
						sun->setSunDirection(dir_);
				}
			}
		}	
		else
		{
			if(mSunContext.mActiveCom != nullptr)
			   mSunContext.mActiveCom->setDirection(dir_);
		}
			
	}

	void TODComponentManager::initTodTransData()
	{
		mSkyData = new SkyComponentContext();
		mFogData = new FogHandle();
		mDirecData = new DirecLightHandle();
		mSunData = new sunHandle();
		mCloudMeshHandle = new CloudMeshComponentContext();
		mCloudSphereHandle = new CloudSphereComponentContext();
		mCloudPlaneHandle = new CloudPlaneComponentContext();
		mSphereFogData = new SphereFogComHandle();

#define INIT_TODTRANSDATA(Name, Type)	\
			m##Name##Handle = new Type();	\
			mFinal##Name##Handle = new Type();

		INIT_TODTRANSDATA(EnvironmentLight, EnvironmentLightHandle)
		INIT_TODTRANSDATA(GlobalLight, GlobalLightHandle)
		INIT_TODTRANSDATA(HDR, HDRHandle)
		INIT_TODTRANSDATA(HDRAdaptive, HDRAdaptiveHandle)
		INIT_TODTRANSDATA(Ocean, OceanComponentContext)
		INIT_TODTRANSDATA(PointLight, pLightManagerHandle)
		INIT_TODTRANSDATA(Stars, StarsComponentHandle)
		INIT_TODTRANSDATA(UnderWaterCaustics, CausticsHandle)
		INIT_TODTRANSDATA(WeatherCoverColor, WeatherCoverColorComData)
		INIT_TODTRANSDATA(TODShadow, TodShadowDataHandle)
		INIT_TODTRANSDATA(SphericalOcean, SphericalOceanContext)
#undef INIT_TODTRANSDATA

		//记录过渡状态最终的属性值
		mFinalSkyData = new SkyComponentContext();
		mFinalFogData = new FogHandle();
        mFinalDirecData = new DirecLightHandle();
		mFinalSunData = new sunHandle();
		mFinalCloudMeshHandle = new CloudMeshComponentContext();
		mFinalCloudSphereHandle = new CloudSphereComponentContext();
		mFinalCloudPlaneHandle = new CloudPlaneComponentContext();
		mFinalSphereFogData = new SphereFogComHandle();
	}

	void TODComponentManager::uninitTodTransData()
	{
		SAFE_DELETE(mSkyData)
		SAFE_DELETE(mFogData)
		SAFE_DELETE(mDirecData)
		SAFE_DELETE(mSunData)
		SAFE_DELETE(mCloudMeshHandle)
		SAFE_DELETE(mCloudSphereHandle)
		SAFE_DELETE(mCloudPlaneHandle)
		SAFE_DELETE(mSphereFogData)
		SAFE_DELETE(mEnvironmentLightHandle)
		SAFE_DELETE(mGlobalLightHandle)
		SAFE_DELETE(mHDRHandle)
		SAFE_DELETE(mHDRAdaptiveHandle)
		SAFE_DELETE(mOceanHandle)
		SAFE_DELETE(mPointLightHandle)
		SAFE_DELETE(mStarsHandle)
		SAFE_DELETE(mUnderWaterCausticsHandle)
		SAFE_DELETE(mWeatherCoverColorHandle)
		SAFE_DELETE(mTODShadowHandle)
		SAFE_DELETE(mSphericalOceanHandle)

		SAFE_DELETE(mFinalSkyData)
		SAFE_DELETE(mFinalFogData)
		SAFE_DELETE(mFinalDirecData)
		SAFE_DELETE(mFinalSunData)
		SAFE_DELETE(mFinalCloudMeshHandle)
		SAFE_DELETE(mFinalCloudSphereHandle)
		SAFE_DELETE(mFinalCloudPlaneHandle)
		SAFE_DELETE(mFinalSphereFogData)
		SAFE_DELETE(mFinalEnvironmentLightHandle)
		SAFE_DELETE(mFinalGlobalLightHandle)
		SAFE_DELETE(mFinalHDRHandle)
		SAFE_DELETE(mFinalHDRAdaptiveHandle)
		SAFE_DELETE(mFinalOceanHandle)
		SAFE_DELETE(mFinalPointLightHandle)
		SAFE_DELETE(mFinalStarsHandle)
		SAFE_DELETE(mFinalUnderWaterCausticsHandle)
		SAFE_DELETE(mFinalWeatherCoverColorHandle)
		SAFE_DELETE(mFinalTODShadowHandle)
		SAFE_DELETE(mFinalSphericalOceanHandle)
	}

	void TODComponentManager::setDirecLightIsUse(bool isUse_)
	{
		mIsUseDirecLight = isUse_;
		updateDirecLight(nullptr);
	}

	void  TODComponentManager::setTodComMgrSceneTriggerData(SceneTriggerData* sceneTriggerData_)
	{
		mSceneTriggerData = sceneTriggerData_;
	}


	void TODComponentManager::updateFog(FogComponent* fogCom_)
	{
		if ( (mIsFogTranslation && mIsFirstTodTick) || !mIsEffectiveModify)
			return;
		if (mIsUseFog)
		{
			if (mFogVec.empty())
			{
				if(mCurrActiveFogCom)
					setDefauleFogHandle();
				mIsFogRealTime = false;
				mCurrActiveFogCom = nullptr;
                return;
			}
				
			if (mFogHandle == nullptr)
				getDefaultFogHandle();

			if (mIsFogRealTime && mCurrActiveFogCom == fogCom_)
			{
				mIsFogRealTime = false;
				mCurrActiveFogCom = nullptr;
			}

			if (!mIsFogRealTime && mCurrActiveFogCom != mFogVec[0])
				mCurrActiveFogCom = mFogVec[0];

			if (!mIsChangeTodStfID && mCurrActiveFogCom)
			{
				mCurrActiveFogCom->setMainSceneFog();
				if (mIsWriteLog)
				{
					std::string logInfo = "";
					logInfo.append("fog : use").append(mCurrActiveFogCom->getCombinedName());
					LogManager::getSingleton().logMessage(Echo::LML_CRITICAL, logInfo.data());
				}
			}
			else
				mIsFogActive = false;
		}
		else
		{
			if (mCurrActiveFogCom != nullptr)
				setDefauleFogHandle();
			mCurrActiveFogCom = nullptr;
			mIsFogRealTime = false;
		}	
	}

	void TODComponentManager::updateDirecLight(DirecLightComponent* direcLightCom_)
	{
		if ( (mIsDirecLightTranslation && mIsFirstTodTick) || !mIsEffectiveModify)
			return;
		if (mIsUseDirecLight)
		{
			if (mDirecVec.empty())
			{
				if(mCurrActiveDirecCom)
					setDefaultDirecLightHandle();
				mIsDirecRealTime = false;
				mCurrActiveDirecCom = nullptr;
				return;
			}

			if (mDirecLightHandle == nullptr)
				getDefaultDirecLightHandle();

			if (mIsDirecRealTime && mCurrActiveDirecCom == direcLightCom_)
			{
				mIsDirecRealTime = false;
				mCurrActiveDirecCom = nullptr;
			}


			if (!mIsDirecRealTime && mCurrActiveDirecCom != mDirecVec[0])
                mCurrActiveDirecCom = mDirecVec[0];
              
			if (!mIsChangeTodStfID && mCurrActiveDirecCom)
			{
                mCurrActiveDirecCom->setScreenAttribute();
				if (mIsWriteLog)
				{
					std::string logInfo = "";
					logInfo.append("direclight : use").append(mCurrActiveDirecCom->getCombinedName());
					LogManager::getSingleton().logMessage(Echo::LML_CRITICAL, logInfo.data());
				}
			}
			else
				mIsDirecActive = false;
		}
		else
		{
			if(mCurrActiveDirecCom != nullptr)
			   setDefaultDirecLightHandle();
			mCurrActiveDirecCom = nullptr;
			mIsDirecRealTime = false;
		}
	}

	void TODComponentManager::setDefaultDirecLightHandle()
	{
		if (mSceneManager == nullptr)
			return;

		if (mDirecLightHandle)
		{
			if (mSceneManager->getSun() != nullptr)
				mSceneManager->getSun()->setSunDirection(mDirecLightHandle->mSunDirection);
			mSceneManager->setMainLightDirection(mDirecLightHandle->mMainLightDirection);
			mSceneManager->setMainLightShadowDirection(mDirecLightHandle->mMainLightShadowDirection);
			mSceneManager->setPBRMainLightColor(mDirecLightHandle->mPBRMainLightColor);
			mSceneManager->setPBRSkyLightColour(mDirecLightHandle->mPBRSkyLightColor);
			mSceneManager->setPBRGroundLightColour(mDirecLightHandle->mPBRGroundLightColor);
			mSceneManager->setRoleMainLightColor(mDirecLightHandle->mRoleMainLightColor);
			mSceneManager->setRoleSkyLightColour(mDirecLightHandle->mRoleSkyLightColor);
			mSceneManager->setRoleGroundLightColour(mDirecLightHandle->mRoleGroundLightColor);

			mSceneManager->setMainLightColor(mDirecLightHandle->mMainLightColor);
			mSceneManager->setSkyLightColour(mDirecLightHandle->mMainSkyLightColor);
			mSceneManager->setGroundLightColour(mDirecLightHandle->mMainGroundLightColor);

			mSceneManager->setPBRAmbientBlendThreshold(mDirecLightHandle->mPBRAmbientBlendThreshold);
			mSceneManager->setRoleAmbientBlendThreshold(mDirecLightHandle->mRoleAmbientBlendThreshold);

			std::array<float, 3> percent;
			percent[0] = (mDirecLightHandle->m_ViewLightPercent);
			percent[1] = (mDirecLightHandle->m_PBRViewLightPercent);
			percent[2] = (mDirecLightHandle->m_RoleViewLightPercent);
			mSceneManager->setViewLightPercent(percent);

			if (mIsWriteLog)
			{
				LogManager::getSingleton().logMessage(Echo::LML_CRITICAL, "MainSceneManager :: direclight active");
			}

			delete(mDirecLightHandle);
			mDirecLightHandle = nullptr;
		}
	}

	void TODComponentManager::getDefaultDirecLightHandle()
	{
		if (mSceneManager == nullptr)
			return;

		if (mDirecLightHandle == nullptr)
		{
			mDirecLightHandle = new DirecLightHandle();
			_getSceneDirecLightData(*mDirecLightHandle);
		}
	}

	void TODComponentManager::getDefaultFogHandle()
	{
		if (mSceneManager == nullptr)
			return;
		if (mFogHandle == nullptr)
		{
			mFogHandle = new FogHandle();
			mSceneManager->getFogData(mFogHandle->mFogMode, mFogHandle->mFogColor, mFogHandle->mDensity, mFogHandle->mRampStart, mFogHandle->mRampEnd, mFogHandle->mTopHeight);
		}
	}

	void TODComponentManager::setDefauleFogHandle()
	{
		if (mSceneManager == nullptr)
			return;
		if (mFogHandle)
		{
			mSceneManager->setFog(mFogHandle->mFogMode, mFogHandle->mFogColor, mFogHandle->mDensity, mFogHandle->mRampStart, mFogHandle->mRampEnd, mFogHandle->mTopHeight);
			if (mIsWriteLog)
			{
				LogManager::getSingleton().logMessage(Echo::LML_CRITICAL, "MainSceneManager :: fog active");
			}
			delete mFogHandle;
			mFogHandle = nullptr;
		}
	}

	void TODComponentManager::createCloudSphere()
	{
		if (mCloudSphere != nullptr)
			return;
		if (mSceneManager)
		{
			mCloudSphere = new CloudSphere();
			mCloudSphere->_notifyManager(mSceneManager);
			mCloudSphere->setQueryFlags(0);
		}
	}

	void TODComponentManager::deleteCloudSphere()
	{
		if (mCloudSphere)
		{
			useDefaultObj(mCloudSphereContext);
			delete mCloudSphere;
		}
		mCloudSphere = nullptr;
	}

	void TODComponentManager::createCloudPlane()
	{
		if (mCloudPlane != nullptr)
			return;
		if (mSceneManager)
		{
			mCloudPlane = new CloudPlane();
			mCloudPlane->_notifyManager(mSceneManager);
			mCloudPlane->setQueryFlags(0);
		}
	}

	void TODComponentManager::deleteCloudPlane()
	{
		if (mCloudPlane)
		{
			useDefaultObj(mCloudPlaneContext);
            delete mCloudPlane;
		}
			
		mCloudPlane = nullptr;
	}

	void TODComponentManager::createCloudMesh()
	{
		if (mSceneManager == nullptr)
			return;
		if (mCloudMeshUp == nullptr)
		{
			mCloudMeshUp = new CloudMesh("CloudUpMesh");
			if (mCloudMeshUp)
			{
				mCloudMeshUp->_notifyManager(mSceneManager);
				mCloudMeshUp->setQueryFlags(0);
			}
		}
		if (mCloudMeshMid == nullptr)
		{
			mCloudMeshMid = new CloudMesh("CloudMidMesh");
			if (mCloudMeshMid)
			{
				mCloudMeshMid->_notifyManager(mSceneManager);
				mCloudMeshMid->setQueryFlags(0);
			}

		}
		if (mCloudMeshDown == nullptr)
		{
			mCloudMeshDown = new CloudMesh("CloudDownMesh");
			if (mCloudMeshDown)
			{
				mCloudMeshDown->_notifyManager(mSceneManager);
				mCloudMeshDown->setQueryFlags(0);
			}
		}

	}

	void TODComponentManager::deleteCloudMesh()
	{
		useDefaultObj(mCloudMeshContext);
		SAFE_DELETE(mCloudMeshUp);
		SAFE_DELETE(mCloudMeshMid);
		SAFE_DELETE(mCloudMeshDown);
	}

	void TODComponentManager::createSky()
	{
		if (mSky != nullptr)
			return;
		if (mSceneManager == nullptr)
			return;

		mSky = new Sky();

		if (mSky) {
			mSky->_notifyManager(mSceneManager);
			mSky->setQueryFlags(0);
		}
	}

	void TODComponentManager::deleteSky()
	{
		if (mSky == nullptr)
			return;

		useDefaultObj(mSkyContext);

		Sky*  tempSky = nullptr;
		if(mSceneManager)
		   tempSky = mSceneManager->getSky();
		if (mSky)
			delete mSky;
		mSky = nullptr;
		if(mSceneManager)
			mSceneManager->setSky(tempSky);
	}

	void TODComponentManager::createSun()
	{
		if (mSun != nullptr)
			return;
	
		if (mSceneManager == nullptr)
			return;
		mSun = new Sun();
		mSun->_notifyManager(mSceneManager);
		mSun->setQueryFlags(0);
	}

	void TODComponentManager::deleteSun()
	{
		if (mSun == nullptr)
			return;

		useDefaultObj(mSunContext);

		Sun* tempSun = nullptr;
		if (mSceneManager)
			tempSun = mSceneManager->getSun();
		if (mSun)
			delete mSun;
		mSun = nullptr;
		if(mSceneManager)
			mSceneManager->setSun(tempSun);
	}

	void TODComponentManager::createOcean()
	{
		if (mOcean != nullptr)
			return;
		if (mSceneManager == nullptr)
			return;
		if (Root::instance()->m_bFFTOcean)
			mOcean = new FFTOcean();
		else
			mOcean = new Ocean();
		mOcean->_notifyManager(mSceneManager);
		mOcean->setQueryFlags(0);
	}
	void TODComponentManager::deleteOcean()
	{
		if (mOcean == nullptr)
			return;

		useDefaultObj(mOceanContext);

		Ocean* tempOcean = nullptr;
		if (mSceneManager)
			tempOcean = mSceneManager->getOcean();
		if (mOcean)
			delete mOcean;
		mOcean = nullptr;
		if (mSceneManager)
			mSceneManager->setOcean(tempOcean);
	}

	void TODComponentManager::createStars()
	{
		if (mStars != nullptr || mSceneManager == nullptr)
			return;
		mStars = new Stars();
		if (mStars)
		{
			mStars->_notifyManager(mSceneManager);
			mStars->setQueryFlags(0);
		}
	}

	void TODComponentManager::deleteStars()
	{
		if (mStars == nullptr)
			return;

		useDefaultObj(mStarsContext);

		Stars* tempStars = nullptr;
		if (mSceneManager)
			tempStars = mSceneManager->getStars();
		if (mStars)
			delete mStars;
		mStars = nullptr;
		if (mSceneManager)
			mSceneManager->setStars(tempStars);
	
	}

	HDR* TODComponentManager::getHDR()
	{
		RenderSystem* pRenderSystem = Root::instance()->getRenderSystem();
		RenderStrategy* pStrategy = nullptr;
		PostProcessChain* pProcessChain = nullptr;
		HDR* pHDR = nullptr;
		if (!pRenderSystem) return nullptr;
		pStrategy = pRenderSystem->getCurRenderStrategy();

		if (!pStrategy) return nullptr;
		pProcessChain = pStrategy->getPostProcessChain();

		if (!pProcessChain) return nullptr;
		pHDR = static_cast<HDR*>(pProcessChain->getPostProcess(PostProcess::Type::HDR));

		return pHDR;
	}

	SphericalOcean* TODComponentManager::getCurrentSphericalOcean()
	{
		if (!mSceneManager) return nullptr;

		PlanetManager* pMgr = mSceneManager->getPlanetManager();
		if (!pMgr) return nullptr;

		SphericalTerrain* planet = pMgr->getNearestPlanetToSurface();
		if (!planet) return nullptr;

		return planet->getOcean();
	}

	void TODComponentManager::setDirecLightRealTime(bool isRealTime, DirecLightComponent* direcCom)
	{
		if (direcCom == nullptr)
		   return;
		mIsDirecRealTime = isRealTime;		
		if (isRealTime && direcCom)
		{
			if (!mIsUseDirecLight)
			{
				mIsUseDirecLight = true;
				getDefaultDirecLightHandle(); 
			}
			mCurrActiveDirecCom = direcCom;
		}
		updateDirecLight(nullptr);
	}

	bool TODComponentManager::getDirecLightRealTime(DirecLightComponent* direcCom)
	{
		if(direcCom != mCurrActiveDirecCom)
		   return false;
		return mIsDirecRealTime;
	}

	void TODComponentManager::setFogRealTime(bool isRealTime, FogComponent* fogCom)
	{
		if (fogCom == nullptr)
			return;
		mIsFogRealTime = isRealTime;
		if (isRealTime && fogCom != nullptr)
		{
			if (!mIsUseFog)
			{
				mIsUseFog = true;
				getDefaultFogHandle();
			}
			mCurrActiveFogCom = fogCom;
		}
		updateFog(nullptr);
	}

	bool TODComponentManager::getFogRealTime(FogComponent* fogCom_)
	{
		if (fogCom_ != mCurrActiveFogCom)
			return false;
		return mIsFogRealTime;
	}
	// ----------------------------------------------------------------------------------------------
	void TODComponentManager::saveDefaultObj(TODContext<SkyComponent, Sky>& context)
	{
		// 如果没有保存过默认的, 则保存一下。
		if (nullptr == context.mDefaultSceneObj) {
		
			if (mSceneManager == nullptr)
				return;
			context.mDefaultSceneObj = mSceneManager->getSky();
			if (context.mDefaultSceneObj) {
				// 将默认解绑
				context.mDefaultSceneObj->deinit();
			}
			createSky();
			if (mSky)
			{
				mSceneManager->setSky(mSky);
				mSky->init();
			}
		}
	}

	void TODComponentManager::useDefaultObj(TODContext<SkyComponent, Sky>& context)
	{
    	if (mSceneManager == nullptr)
			return;
		if (context.mDefaultSceneObj) {

			if (mSky)
			{
				mSky->deinit();
			}

			context.mDefaultSceneObj->init();
			mSceneManager->setSky(context.mDefaultSceneObj);
			context.mDefaultSceneObj = nullptr;
			context.mActiveCom = nullptr;
			context.mIsImmediateCom = false;
		}
	}

	// ----------------------------------------------------------------------------------------------
	void TODComponentManager::saveDefaultObj(TODContext<CloudSphereComponent, CloudSphere>& context) {
		
		if (mSceneManager == nullptr)
			return;
		
		Cloud* mCloud = mSceneManager->getCloud();

		if (mCloud && context.mDefaultSceneObj == nullptr) {
			context.mDefaultSceneObj = mCloud->getCloudSphere();
			if (context.mDefaultSceneObj) {
				context.mDefaultSceneObj->deinit();
			}

			createCloudSphere();
			if (mCloudSphere)
			{
				mCloud->setCloudSphere(mCloudSphere);
				mCloudSphere->init();
			}
	}
	}

	void TODComponentManager::useDefaultObj(TODContext<CloudSphereComponent, CloudSphere>& context) {
		
		if (mSceneManager == nullptr)
			return;
		Cloud* mCloud = mSceneManager->getCloud();
		if (mCloud && context.mDefaultSceneObj) {
			
			if (mCloudSphere)
				mCloudSphere->deinit();

			mCloud->setCloudSphere(context.mDefaultSceneObj);
			context.mDefaultSceneObj->init();

			if (mTodManager)
				mTodManager->setCurrentCloudDiffuseNameByCom(eCloudType_Sphere, "Default");
			context.mDefaultSceneObj = nullptr;

			context.mActiveCom = nullptr;
			context.mIsImmediateCom = false;
		}
	}

	// ----------------------------------------------------------------------------------------------
	void TODComponentManager::saveDefaultObj(TODContext<CloudPlaneComponent, CloudPlane>& context) {
		
		if (mSceneManager == nullptr)
			return;
		
		Cloud* mCloud = mSceneManager->getCloud();

		if (mCloud && context.mDefaultSceneObj == nullptr) {
			context.mDefaultSceneObj = mCloud->getCloudPlane();
			if (context.mDefaultSceneObj) {
				context.mDefaultSceneObj->deinit();
			}

			createCloudPlane();
			if (mCloudPlane)
			{
				mCloud->setCloudPlane(mCloudPlane);
				mCloudPlane->init();
			}
		}
	}

	void TODComponentManager::useDefaultObj(TODContext<CloudPlaneComponent, CloudPlane>& context) {
		
		if (mSceneManager == nullptr)
			return;

		Cloud* mCloud = mSceneManager->getCloud();
		if (mCloud && context.mDefaultSceneObj) {

			if (mCloudPlane)
				mCloudPlane->deinit();
		
			mCloud->setCloudPlane(context.mDefaultSceneObj);
			context.mDefaultSceneObj->init();
			context.mDefaultSceneObj = nullptr;

			context.mActiveCom = nullptr;
			context.mIsImmediateCom = false;
		}
	}

	void TODComponentManager::saveDefaultObj(TODContext<CloudMeshComponent, CloudMeshHandle>& context)
	{
		if (mSceneManager == nullptr)
			return;
		Cloud* mCloud = mSceneManager->getCloud();

		if (mCloud && context.mDefaultSceneObj == nullptr) {
			context.mDefaultSceneObj = new CloudMeshHandle();
			context.mDefaultSceneObj->mCloudUpMesh = mCloud->getCloudMeshUp();
			context.mDefaultSceneObj->mCloudMidMesh = mCloud->getCloudMeshMid();
			context.mDefaultSceneObj->mCloudDownMesh = mCloud->getCloudMeshDown();

			if (context.mDefaultSceneObj) {
				if(context.mDefaultSceneObj->mCloudUpMesh)
				   context.mDefaultSceneObj->mCloudUpMesh->deinit();
				if(context.mDefaultSceneObj->mCloudMidMesh)
				   context.mDefaultSceneObj->mCloudMidMesh->deinit();
				if(context.mDefaultSceneObj->mCloudDownMesh)
				   context.mDefaultSceneObj->mCloudDownMesh->deinit();

				createCloudMesh();

				if (mCloudMeshUp)
				{
					mCloudMeshUp->init();
					mCloud->setCloudMeshUp(mCloudMeshUp);
				}

				if (mCloudMeshMid)
				{
					mCloudMeshMid->init();
					mCloud->setCloudMeshMid(mCloudMeshMid);
				}

				if (mCloudMeshDown)
				{
					mCloudMeshDown->init();
					mCloud->setCloudMeshDown(mCloudMeshDown);
				}
			}
		}
	}
	void TODComponentManager::useDefaultObj(TODContext<CloudMeshComponent, CloudMeshHandle>& context)
	{
		if (mSceneManager == nullptr)
			return;
		Cloud* mCloud = mSceneManager->getCloud();

		if (mCloud && context.mDefaultSceneObj)
		{

			if (mCloudMeshUp)
				mCloudMeshUp->deinit();
			if (mCloudMeshMid)
				mCloudMeshMid->deinit();
			if (mCloudMeshDown)
				mCloudMeshDown->deinit();

			if (context.mDefaultSceneObj->mCloudUpMesh)
			{
				mCloud->setCloudMeshUp(context.mDefaultSceneObj->mCloudUpMesh);
				context.mDefaultSceneObj->mCloudUpMesh->init();
			}
			if (context.mDefaultSceneObj->mCloudMidMesh)
			{
				mCloud->setCloudMeshMid(context.mDefaultSceneObj->mCloudMidMesh);
				context.mDefaultSceneObj->mCloudMidMesh->init();
			}
			if (context.mDefaultSceneObj->mCloudDownMesh)
			{
				mCloud->setCloudMeshDown(context.mDefaultSceneObj->mCloudDownMesh);
				context.mDefaultSceneObj->mCloudDownMesh->init();
			}


			if (mTodManager)
			{
				mTodManager->setCurrentCloudDiffuseNameByCom(eCloudType_Mesh_Up, "Default");
				mTodManager->setCurrentCloudDiffuseNameByCom(eCloudType_Mesh_Mid, "Default");
				mTodManager->setCurrentCloudDiffuseNameByCom(eCloudType_Mesh_Down, "Default");
			}


			if (context.mDefaultSceneObj)
			{
				context.mDefaultSceneObj->mCloudUpMesh = nullptr;
				context.mDefaultSceneObj->mCloudMidMesh = nullptr;
				context.mDefaultSceneObj->mCloudDownMesh = nullptr;
				delete context.mDefaultSceneObj;
				context.mDefaultSceneObj = nullptr;
			}
			context.mActiveCom = nullptr;
			context.mIsImmediateCom = false;
		}
	}


	void TODComponentManager::saveDefaultObj(TODContext<SunComponent, Sun>& context) {
		if (context.mDefaultSceneObj == nullptr) {

			if (mSceneManager == nullptr)
				return;

			context.mDefaultSceneObj = mSceneManager->getSun();
			mSunDefaultColor = mSceneManager->getSunColor();
			if (context.mDefaultSceneObj) {
				context.mDefaultSceneObj->deinit();
			}

			createSun();
			Vector3 dir = Vector3::ZERO;
			if(context.mDefaultSceneObj)
				dir = context.mDefaultSceneObj->getSunDirection();
			mSceneManager->setSun(mSun);
			mSun->init();
		    mSun->setSunDirection(dir);
		}
	}

	void TODComponentManager::useDefaultObj(TODContext<SunComponent, Sun>& context) {
		if (context.mDefaultSceneObj) {
			if (mSceneManager == nullptr)
				return;
			if (mSun)
				mSun->deinit();

			mSceneManager->setSun(context.mDefaultSceneObj);
			context.mDefaultSceneObj->init();

			mSceneManager->setSunColor(mSunDefaultColor);
			mSceneManager->getSun()->setSunDirection(mSceneManager->getMainLightDirection());
			context.mDefaultSceneObj = nullptr;
			context.mActiveCom = nullptr;
			context.mIsImmediateCom = false;
		}
	}

	void TODComponentManager::saveDefaultObj(TODContext<EnvironmentLightComponent, EnvironmentLightHandle>& context) {
		//如果在组件启用前，场景中存在默认的指针，把该指针保存到context中，再把该指针从场景中解绑
		if (context.mDefaultSceneObj == nullptr) {
			context.mDefaultSceneObj = new EnvironmentLightHandle();
			auto envInstance = EnvironmentLight::instance();
			//将场景中的数据保存到handle中并解绑指针
			context.mDefaultSceneObj->mEnvironmentExplosure = envInstance->getEnvironmentExplosure();
			context.mDefaultSceneObj->mEnvironmentTextureName = envInstance->getGlobalEnvironmentTexName();
			context.mDefaultSceneObj->mEnvironmentShcFileName = envInstance->getGlobalSphericalHarmonicsFileName();
			envInstance->setUpdateSource(EnvironmentLight::DataFromComponent);//设置sceneManager中该指针为空指针
			
		}
	}

	void TODComponentManager::useDefaultObj(TODContext<EnvironmentLightComponent, EnvironmentLightHandle>& context) {
		if (context.mDefaultSceneObj) {		
			auto envInstance = EnvironmentLight::instance();
			envInstance->setGlobalEnvironmentTexName(context.mDefaultSceneObj->mEnvironmentTextureName,EnvironmentLight::DataFromComponent);
			envInstance->setGlobalSphericalHarmonicsFileName(context.mDefaultSceneObj->mEnvironmentShcFileName, EnvironmentLight::DataFromComponent);
			envInstance->setGlobalEnvironmentExplosure(context.mDefaultSceneObj->mEnvironmentExplosure, EnvironmentLight::DataFromComponent);

			envInstance->setUpdateSource(EnvironmentLight::DataFromEditor);
			delete context.mDefaultSceneObj;
			context.mDefaultSceneObj = nullptr;
			context.mActiveCom = nullptr;
			context.mIsImmediateCom = false;
		}
	}
	void TODComponentManager::saveDefaultObj(TODContext<OceanComponent, Ocean>& context)
	{
		if (mSceneManager == nullptr)
			return;
		Ocean* sceneOcean = mSceneManager->getOcean();

		if (sceneOcean == nullptr || !mIsOcean)
		{
			createOcean();
			if (mOcean)
			{
				mSceneManager->setOcean(mOcean);
				if (!mIsOceanInit)//因为场景没有海洋的时候会走两遍这边,第二遍会重复init海洋组件
				{
					mOcean->init();
					mIsOceanInit = true;
				}
			}
			mIsOcean = false;//如果场景里本身没有海洋,则不再走saveDefault
			return;
		}
		if (nullptr == context.mDefaultSceneObj) {
			context.mDefaultSceneObj = mSceneManager->getOcean();
			mWaterShallowColor = mSceneManager->getWaterShallowColor();
			mWaterDeepColor = mSceneManager->getWaterDeepColor();
			mWaterFoamColor = mSceneManager->getWaterFoamColor();
			mWaterReflectColor = mSceneManager->getWaterReflectColor();
			mUnderWaterColor = mSceneManager->getUnderWaterColorValue();
			if (context.mDefaultSceneObj) {
				context.mDefaultSceneObj->deinit();
			}
			createOcean();
			if (mOcean)
			{
				mSceneManager->setOcean(mOcean);
				mOcean->init();
			}
		}
	}

	void TODComponentManager::useDefaultObj(TODContext<OceanComponent, Ocean>& context)
	{
		if (mSceneManager == nullptr)
			return;
		if (context.mDefaultSceneObj) {
			if (mOcean)
			{
				mOcean->deinit();
			}
			context.mDefaultSceneObj->init();
			mSceneManager->setOcean(context.mDefaultSceneObj);
			mSceneManager->setWaterShallowColor(mWaterShallowColor);
			mSceneManager->setWaterDeepColor(mWaterDeepColor);
			mSceneManager->setWaterFoamColor(mWaterFoamColor);
			mSceneManager->setWaterReflectColor(mWaterReflectColor);
			mSceneManager->setUnderWaterColor(mUnderWaterColor);
			context.mDefaultSceneObj = nullptr;

			context.mActiveCom = nullptr; 
			context.mIsImmediateCom = false;
		}
		else if (!mIsOcean)
		{
			if (mOcean) {
				mOcean->deinit();
				mIsOceanInit = false;
			}
			mSceneManager->setOcean(nullptr);
		}
	}
	void TODComponentManager::saveDefaultObj(TODContext<WindPowerComponent, WindPowerHandle>& context) {
		//如果在组件启用前，场景中存在默认的指针，把该指针保存到context中，再把该指针从场景中解绑
		if (context.mDefaultSceneObj == nullptr) {
			context.mDefaultSceneObj = new WindPowerHandle();

			if (mSceneManager == nullptr)
				return;
			//将场景中的数据保存到handle中并解绑指针
			Vector4 WindParas = mSceneManager->getWindParams();
			Vector4 TreeParas = mSceneManager->getGlobalTreeAnimationParams();
			Vector4 VegParas = mSceneManager->getGlobalVegetationAnimationParams();
			context.mDefaultSceneObj->windDirection = Vector3(WindParas.x, WindParas.y, WindParas.z);
			context.mDefaultSceneObj->windAngle = 180 * Math::ASin(WindParas.z).valueRadians() / Math::PI;
			context.mDefaultSceneObj->windPower = WindParas.z;
			context.mDefaultSceneObj->treeSwayAmp = TreeParas.x;
			context.mDefaultSceneObj->treeSwayFreq = TreeParas.y;
			context.mDefaultSceneObj->treeShakeAmp = TreeParas.z;
			context.mDefaultSceneObj->treeShakeFreq = TreeParas.w;
			context.mDefaultSceneObj->vegSwayAmp = VegParas.x;
			context.mDefaultSceneObj->vegSwayFreq = VegParas.y;
			context.mDefaultSceneObj->vegShakeAmp = VegParas.z;
			context.mDefaultSceneObj->vegShakeFreq = VegParas.w;
		}
	}
	void TODComponentManager::useDefaultObj(TODContext<WindPowerComponent, WindPowerHandle>& context) {
		if (context.mDefaultSceneObj) {
		  
			if (mSceneManager == nullptr)
				return;
			Vector4 WindParas;
			Vector4 TreeParas;
			Vector4 VegParas;
			WindParas.x = context.mDefaultSceneObj->windDirection.x;
			WindParas.y = context.mDefaultSceneObj->windDirection.y;
			WindParas.z = context.mDefaultSceneObj->windDirection.z;
			WindParas.w = context.mDefaultSceneObj->windPower;
			TreeParas.x = context.mDefaultSceneObj->treeSwayAmp;
			TreeParas.y = context.mDefaultSceneObj->treeSwayFreq;
			TreeParas.z = context.mDefaultSceneObj->treeShakeAmp;
			TreeParas.w = context.mDefaultSceneObj->treeShakeFreq;
			VegParas.x = context.mDefaultSceneObj->vegSwayAmp;
			VegParas.y = context.mDefaultSceneObj->vegSwayFreq;
			VegParas.z = context.mDefaultSceneObj->vegShakeAmp;
			VegParas.w = context.mDefaultSceneObj->vegShakeFreq;
			mSceneManager->setWindParams(WindParas);
			mSceneManager->setGlobalTreeAnimationParams(TreeParas);
			mSceneManager->setGlobalVegetationAnimationParams(VegParas);
			delete context.mDefaultSceneObj;
			context.mDefaultSceneObj = nullptr;
			context.mActiveCom = nullptr;
			context.mIsImmediateCom = false;
		}
	}
	void TODComponentManager::saveDefaultObj(TODContext<UnderWaterCausticsComponent, CausticsHandle>& context)
	{

		if (mSceneManager == nullptr)
			return;
		if (context.mDefaultSceneObj == nullptr) {
			context.mDefaultSceneObj = new CausticsHandle();
			auto CausticsParams = mSceneManager->getCausticParams();
			context.mDefaultSceneObj->StartDistance = CausticsParams.x;
			context.mDefaultSceneObj->EndDistance = CausticsParams.y;
			context.mDefaultSceneObj->Density = CausticsParams.z;
			context.mDefaultSceneObj->DisturbStrength = CausticsParams.w;
		    context.mDefaultSceneObj->isForceUnderWater = mSceneManager->isForceUnderWater();
			context.mDefaultSceneObj->Color = mSceneManager->getCausticColour();
		}
	}
	void TODComponentManager::useDefaultObj(TODContext<UnderWaterCausticsComponent, CausticsHandle>& context)
	{
		if (mSceneManager == nullptr)
			return;
		if (context.mDefaultSceneObj) {
			Vector4 CausticsParams;
			CausticsParams.x = context.mDefaultSceneObj->StartDistance;
			CausticsParams.y = context.mDefaultSceneObj->EndDistance;
			CausticsParams.z = context.mDefaultSceneObj->Density;
			CausticsParams.w = context.mDefaultSceneObj->DisturbStrength;
			mSceneManager->setCausticParams(CausticsParams);
			mSceneManager->setCausticColour(context.mDefaultSceneObj->Color);

			if (context.mDefaultSceneObj->isForceUnderWater) {
				if (mWorldManager) {
					mWorldManager->setUnderWaterEnable(true);
				}
				else {
					VideoConfigSystem::instance()->getActiveVideoConfig()->setUnderWaterEnable(true);
				}
			}

			mSceneManager->setIsForceUnderWater(context.mDefaultSceneObj->isForceUnderWater);

			delete context.mDefaultSceneObj;
			context.mDefaultSceneObj = nullptr;
			context.mActiveCom = nullptr;
			context.mIsImmediateCom = false;
		}
	}


	void TODComponentManager::saveDefaultObj(TODContext<PointlightManagerComponent, pLightManagerHandle>& context)
	{
		if (mSceneManager == nullptr)
			return;
		if (context.mDefaultSceneObj == nullptr) {
			context.mDefaultSceneObj = new pLightManagerHandle();
			
			context.mDefaultSceneObj->mIsUseRolePointLight = mSceneManager->getRolePointLightVisible();
			context.mDefaultSceneObj->mLightPower = mSceneManager->getLightPower();
			context.mDefaultSceneObj->mRolePointLightPower = mSceneManager->getRolePointLightPower();
			context.mDefaultSceneObj->mRolePointLightColorLinear = mSceneManager->getRolePointLightColor();
			context.mDefaultSceneObj->mSelfEmissiveLightPower = mSceneManager->getSeltEmissiveLightPower();
		}
	}
	void TODComponentManager::useDefaultObj(TODContext<PointlightManagerComponent, pLightManagerHandle>& context)
	{
		if (mSceneManager == nullptr)
			return;
		if (context.mDefaultSceneObj) {
			mSceneManager->setLightPower(context.mDefaultSceneObj->mLightPower);
			mSceneManager->setRolePointLightPower(context.mDefaultSceneObj->mRolePointLightPower);
			mSceneManager->setRolePointLightColor(ColorValue::linearToGamma(context.mDefaultSceneObj->mRolePointLightColorLinear));
			mSceneManager->setRolePointLightVisible(context.mDefaultSceneObj->mIsUseRolePointLight);
			mSceneManager->setSelfEmissiveLightPower(context.mDefaultSceneObj->mSelfEmissiveLightPower);
			delete context.mDefaultSceneObj;
			context.mDefaultSceneObj = nullptr;
		}
	}

	void TODComponentManager::saveDefaultObj(TODContext<LightningComponent, LightningHandle>& context)
	{
		if (context.mDefaultSceneObj == nullptr)
		{
			context.mDefaultSceneObj = new LightningHandle();
			if (mWeatherManager)
			{
				WeatherParams LightningParams = mWeatherManager->getSpecialLightningParams();
				context.mDefaultSceneObj->version = LightningParams.version;
				context.mDefaultSceneObj->mLightningLife = LightningParams.mLightningLife;
				context.mDefaultSceneObj->mLightningInterval = LightningParams.mLightningInterval;
				context.mDefaultSceneObj->mLightningLimitYawAngle = LightningParams.mLightningLimitYawAngle;
				context.mDefaultSceneObj->mLightningLimitPitchRange = Vector2(LightningParams.mLightningDirection.z, LightningParams.mLightningDirection.w);
				context.mDefaultSceneObj->mLightningUseBeam = LightningParams.mLightningUseBeam;
				context.mDefaultSceneObj->mLightningColor = LightningParams.mLightningColor;
				context.mDefaultSceneObj->mLightningColorPower = LightningParams.mLightningColorPower;
				context.mDefaultSceneObj->mUseSunColor = LightningParams.mUseSunColor;
				context.mDefaultSceneObj->mUseSunBackColor = LightningParams.mUseSunBackColor;
				context.mDefaultSceneObj->mCloudSunBackColour = LightningParams.mCloudSunBackColour;
				context.mDefaultSceneObj->mLightningTexture = LightningParams.mLightningTexture;
				context.mDefaultSceneObj->mLightningUseParticle = LightningParams.mLightningUseParticle;
				context.mDefaultSceneObj->mLightningParticleIsLoop = LightningParams.mLightningParticleIsLoop;
				context.mDefaultSceneObj->mLightningParticleOffsetRange = LightningParams.mLightningParticleOffsetRange;
				context.mDefaultSceneObj->mLightningParticleScaleRange = LightningParams.mLightningParticleScaleRange;
				context.mDefaultSceneObj->mLightningParticle = LightningParams.mLightningParticle;
				context.mDefaultSceneObj->mLightningSound = LightningParams.mLightningSound;
				context.mDefaultSceneObj->m_BellVecs = LightningParams.mLightningBells;
			}
		}
	}
	void TODComponentManager::useDefaultObj(TODContext<LightningComponent, LightningHandle>& context)
	{
		if (context.mDefaultSceneObj)
		{
			if (mWeatherManager)
			{
				auto& obj = context.mDefaultSceneObj;
				LightningType type = LT_SPECIAL;
				mWeatherManager->setLightningLifeTime(type, obj->mLightningLife);
				mWeatherManager->setLightningDelayed(type, obj->mLightningDelayed);

				mWeatherManager->setLightningInterval(type, obj->mLightningInterval);
				mWeatherManager->setLightningLimitYawAngle(type, obj->mLightningLimitYawAngle);

				mWeatherManager->setLightningLimitPitchRange(type, obj->mLightningLimitPitchRange);
				mWeatherManager->setLightningUseBeam(type, obj->mLightningUseBeam);

				mWeatherManager->setLightningColor(type, obj->mLightningColor);
				mWeatherManager->setLightningColorPower(type, obj->mLightningColorPower);
				mWeatherManager->setLightningUseSunColor(type, obj->mUseSunColor);

				mWeatherManager->setLightningSunColor(type, obj->mSunColor);
				mWeatherManager->setLightningUseSunBackColor(type, obj->mUseSunBackColor);

				mWeatherManager->setLightningSunBackColour(type, obj->mCloudSunBackColour * obj->mLightningSunBackColourPower);
				auto clearEmptyPath = [&](const std::vector<String>& names)
				{
					std::vector<String> temp;
					for (int i = 0; i < names.size(); i++)
						if (names[i].size() > 3)//有效的文件应该至少有后缀，.ogg,.dds,.pef等
						{
							temp.emplace_back(names[i]);
						}
					return temp;
				};
				if (!clearEmptyPath(obj->mLightningTexture).empty())
				{
					mWeatherManager->setLightningTextureName(type, clearEmptyPath(obj->mLightningTexture));
				}
				mWeatherManager->setLightningUseParticle(type, obj->mLightningUseParticle);
				mWeatherManager->setLightningParticleLoop(type, obj->mLightningParticleIsLoop);

				mWeatherManager->setLightningParticleOffsetRange(type, obj->mLightningParticleOffsetRange);
				mWeatherManager->setLightningParticleScaleRange(type, obj->mLightningParticleScaleRange);
				if (!clearEmptyPath(obj->mLightningParticle).empty())
				{
					mWeatherManager->setLightningParticleName(type, clearEmptyPath(obj->mLightningParticle));
				}
				if (!clearEmptyPath(obj->mLightningSound).empty())
				{
					mWeatherManager->setLightningSound(type, clearEmptyPath(obj->mLightningSound));
				}
				mWeatherManager->setLightningVisible(LT_NORMAL, obj->mNormalLightningVisible);
				mWeatherManager->setLightningBells(LT_SPECIAL, obj->m_BellVecs);
				
			}
			delete context.mDefaultSceneObj;
			context.mDefaultSceneObj = nullptr;
			context.mActiveCom = nullptr;
			context.mIsImmediateCom = false;
		}
	}
	void TODComponentManager::saveDefaultObj(TODContext<GlobalLightComponent, GlobalLightHandle>& context)
	{
		if (mSceneManager == nullptr)
			return;
		if (context.mDefaultSceneObj == nullptr)
		{
			context.mDefaultSceneObj = new GlobalLightHandle();
			float globalReflectRatio = mSceneManager->getGlobalReflectionRatio();
			float globalSpecularRatio = mSceneManager->getGlobalSpecularRatio();
			context.mDefaultSceneObj->mGlobalReflectRatio = globalReflectRatio;
			context.mDefaultSceneObj->mGlobalSpecularRatio = globalSpecularRatio;
		}
	}
	void TODComponentManager::useDefaultObj(TODContext<GlobalLightComponent, GlobalLightHandle>& context)
	{
		if (mSceneManager == nullptr)
			return;
		if (context.mDefaultSceneObj)
		{
			mSceneManager->setGlobalReflectionRatio(context.mDefaultSceneObj->mGlobalReflectRatio);
			mSceneManager->setGlobalSpecularRatio(context.mDefaultSceneObj->mGlobalSpecularRatio);
			delete context.mDefaultSceneObj;
			context.mDefaultSceneObj = nullptr;
			context.mActiveCom = nullptr;
			context.mIsImmediateCom = false;
		}
	}
	void TODComponentManager::saveDefaultObj(TODContext<HDRComponent, HDRHandle>& context)
	{
		if (context.mDefaultSceneObj == nullptr)
		{
			context.mDefaultSceneObj = new HDRHandle();

			HDR* pHDR = HDRComponent::getHDR();

			if (pHDR != nullptr)
			{
				context.mDefaultSceneObj->EyeAdaption = pHDR->getEyeAdaptationParams();
				context.mDefaultSceneObj->Bright = pHDR->getBrightParams();
				context.mDefaultSceneObj->EyeToneMapping = pHDR->getToneMappingParams();
				context.mDefaultSceneObj->EyeToneMappingCurve = pHDR->getToneMappingCurve();
				context.mDefaultSceneObj->EyeToneMappingColor = pHDR->getColorBalance();

				RenderSystem* pRenderSystem = Root::instance()->getRenderSystem();
				if (!pRenderSystem)
					return;
				RenderStrategy* pStrategy = pRenderSystem->getCurRenderStrategy();
				if (!pStrategy)
					return;
				PostProcessChain* pProcessChain = pStrategy->getPostProcessChain();
				if (!pProcessChain)
					return;

				context.mDefaultSceneObj->mIsUseHDR = pProcessChain->isEnable(PostProcess::HDR);
			}
		}
	}
	void TODComponentManager::useDefaultObj(TODContext<HDRComponent, HDRHandle>& context)
	{
		if (context.mDefaultSceneObj)
		{
			HDR* pHDR = HDRComponent::getHDR();
			if (pHDR)
			{
				bool isActive = Root::instance()->getActiveSceneManager() == mSceneManager;
				if (isActive)
				{
					HDRHandle* data = context.mDefaultSceneObj;
					HDRComponent::setIsOpenHDR(data->mIsUseHDR);
					pHDR->setEyeAdaptationParam0x(data->EyeAdaption.x);
					pHDR->setEyeAdaptationParam0y(data->EyeAdaption.y);
					pHDR->setEyeAdaptationParam0z(data->EyeAdaption.z);
					pHDR->setEyeAdaptationParam0w(data->EyeAdaption.w);

					pHDR->setBrightParam0x(data->Bright.x);
					pHDR->setBrightParam0y(data->Bright.y);
					pHDR->setBrightParam0z(data->Bright.z);
					pHDR->setBrightParam0w(data->Bright.w);

					pHDR->setToneMappingParam0x(data->EyeToneMapping.x);
					pHDR->setToneMappingParam0y(data->EyeToneMapping.y);
					pHDR->setToneMappingParam0z(data->EyeToneMapping.z);

					if (data->EyeToneMappingCurve.size() == 5)//防止下标溢出
						pHDR->setToneMappingCurve(
							data->EyeToneMappingCurve[0],
							data->EyeToneMappingCurve[1],
							data->EyeToneMappingCurve[2],
							data->EyeToneMappingCurve[3],
							data->EyeToneMappingCurve[4]);

					pHDR->setToneMappingColorBalance(
						data->EyeToneMappingColor.r,
						data->EyeToneMappingColor.g,
						data->EyeToneMappingColor.b);
				}
				}
				
			delete context.mDefaultSceneObj;
			context.mDefaultSceneObj = nullptr;
			context.mActiveCom = nullptr;
			context.mIsImmediateCom = false;
		}
	}
	void TODComponentManager::saveDefaultObj(TODContext<EnvTriggerEventComponent, TriggrtEventComponentData>& context)
	{
		if (context.mDefaultSceneObj == nullptr)
		{
			context.mDefaultSceneObj = new TriggrtEventComponentData();
			if (mWeatherManager)
			{
               context.mDefaultSceneObj->mIsRaining = mWeatherManager->isRaining();
			   context.mDefaultSceneObj->mIsSnowing = mWeatherManager->isCoverSnow();
			}
			
			if (mSceneTriggerData)
			{
				mSceneTriggerData->setIsUseTodCom(true);
				mSceneTriggerData->leaveArea();
			}
		}
	}
	void TODComponentManager::useDefaultObj(TODContext<EnvTriggerEventComponent, TriggrtEventComponentData>& context)
	{
		if (context.mDefaultSceneObj)
		{
			if (mWeatherManager)
			{
                mWeatherManager->setRaining(context.mDefaultSceneObj->mIsRaining);
			    mWeatherManager->setCoverSnow(context.mDefaultSceneObj->mIsSnowing);
			}
			
			delete context.mDefaultSceneObj;
			context.mDefaultSceneObj = nullptr;
			if (mSceneTriggerData)
			{
				mSceneTriggerData->setIsUseTodCom(false);
			}
		}
	}

	void TODComponentManager::saveDefaultObj(TODContext<DriftComponent, DriftPropData>& context)
	{

	}
	void TODComponentManager::useDefaultObj(TODContext<DriftComponent, DriftPropData>& context)
	{

	}


	void TODComponentManager::saveDefaultObj(TODContext<StarsComponent, Stars>& context )
	{
		if (context.mDefaultSceneObj == nullptr)
		{
			if (mSceneManager == nullptr)
				return;
			context.mDefaultSceneObj = mSceneManager->getStars();
			if (context.mDefaultSceneObj) {
				context.mDefaultSceneObj->deinit();
				createStars();
				if (mStars)
				{
					mSceneManager->setStars(mStars);
				}
			}
		}
	}
	void TODComponentManager::useDefaultObj(TODContext<StarsComponent, Stars>& context)
	{
		if (mSceneManager == nullptr)
			return;
		if (context.mDefaultSceneObj) {

			/*if (mStars)
			{
				mStars->deinit();
			}*/
			context.mDefaultSceneObj->init();
			mSceneManager->setStars(context.mDefaultSceneObj);
			context.mDefaultSceneObj = nullptr;
			context.mActiveCom = nullptr;
			context.mIsImmediateCom = false;
		}
	}


	void  TODComponentManager::saveDefaultObj(TODContext<WeatherCoverColorComponent, WeatherCoverColorComData>& context)
	{
		if (context.mDefaultSceneObj == nullptr)
		{
			context.mDefaultSceneObj = new WeatherCoverColorComData();
			if (context.mDefaultSceneObj && mWeatherManager)
			{
				context.mDefaultSceneObj->mCoverColor = mWeatherManager->getCoverColor();
				context.mDefaultSceneObj->mCoverColorIntensity = mWeatherManager->getCoverColorIntensity();
				context.mDefaultSceneObj->mIsUseCoverColor = mWeatherManager->getIsUseCoverColor();
			}


		}
	}
	void  TODComponentManager::useDefaultObj(TODContext<WeatherCoverColorComponent, WeatherCoverColorComData>& context)
	{
		if (context.mDefaultSceneObj != nullptr)
		{
			if (mWeatherManager)
			{
				mWeatherManager->setUseCoverColor(context.mDefaultSceneObj->mIsUseCoverColor);
				mWeatherManager->setCoverColor(context.mDefaultSceneObj->mCoverColor);
				mWeatherManager->setCoverColorIntensity(context.mDefaultSceneObj->mCoverColorIntensity, context.mDefaultSceneObj->mCoverColorIntensity);
			}
			delete context.mDefaultSceneObj;
			context.mDefaultSceneObj = nullptr;
		}
	}

	void TODComponentManager::saveDefaultObj(TODContext<VolumeLightComponent, VolumeLightComponentContext>& context)
	{
		if (context.mDefaultSceneObj == nullptr)
		{
			context.mDefaultSceneObj = new VolumeLightComponentContext();
			VolumeLight* vLight = VolumeLightComponent::getVolumeLight();
			if (vLight)
			{
				Vector4 tempParam[2];
				vLight->getSunShaftsParams(tempParam);
				context.mDefaultSceneObj->mRaysCustomColor.r = std::pow(tempParam[0].x,0.4545f);
				context.mDefaultSceneObj->mRaysCustomColor.g = std::pow(tempParam[0].y, 0.4545f);
				context.mDefaultSceneObj->mRaysCustomColor.b = std::pow(tempParam[0].z, 0.4545f);
				context.mDefaultSceneObj->mRaysCustomColor.a = tempParam[0].w;

				context.mDefaultSceneObj->mRaysAmount = tempParam[1].z;
				context.mDefaultSceneObj->mRaysColorIntensity = tempParam[1].w;
				context.mDefaultSceneObj->mRaysAttenuation = vLight->getRayAttenuation();
			}
		
			RenderSystem* pRenderSystem = Root::instance()->getRenderSystem();
			if (!pRenderSystem)
				return;
			RenderStrategy* pStrategy = pRenderSystem->getCurRenderStrategy();
			if (!pStrategy)
				return;
			PostProcessChain* pProcessChain = pStrategy->getPostProcessChain();
			if (!pProcessChain)
				return;
			context.mDefaultSceneObj->mIsUseVolumeLight = pProcessChain->isEnable(PostProcess::Type::VolumeLight);
		}
	}

	void TODComponentManager::useDefaultObj(TODContext<VolumeLightComponent, VolumeLightComponentContext>& context)
	{
		if (context.mDefaultSceneObj)
		{

			VolumeLight* volumelight = VolumeLightComponent::getVolumeLight();
			bool isActive = Root::instance()->getActiveSceneManager() == mSceneManager;
			if (volumelight && isActive)
			{
				VolumeLightComponent::setIsOpenVolumeLight(context.mDefaultSceneObj->mIsUseVolumeLight);
				volumelight->setSunShaftsParams(
					context.mDefaultSceneObj->mRaysAmount, context.mDefaultSceneObj->mRaysAttenuation,
					context.mDefaultSceneObj->mRaysColorIntensity, context.mDefaultSceneObj->mRaysCustomColor.r,
					context.mDefaultSceneObj->mRaysCustomColor.g, context.mDefaultSceneObj->mRaysCustomColor.b);
			}

			delete context.mDefaultSceneObj;
			context.mDefaultSceneObj = nullptr;
		}
	}


	void  TODComponentManager::saveDefaultObj(TODContext<TODShadowComponent, TodShadowDataHandle>& context)
	{
		if (!mSceneManager)
			return;
		if (context.mDefaultSceneObj == nullptr)
		{
			context.mDefaultSceneObj = new TodShadowDataHandle();
			context.mDefaultSceneObj->mShadowWeight = mSceneManager->getShadowWeight();

			RenderSystem* system = Root::instance()->getRenderSystem();
			if (!system) return;

			RenderStrategy* strategy = system->getCurRenderStrategy();
			if (!strategy) return;

			if (system->getCurrentRenderStrategyType() == RS_Deferred)
			{
				auto deferred = dynamic_cast<const DeferredRenderStrategy*>(strategy);
				if (!deferred) return;
				context.mDefaultSceneObj->m_AssaoShadowStrength = deferred->getAOShadowStrength();
				context.mDefaultSceneObj->m_AssaoShadowPower = deferred->getAOShadowPower();
				return;
			}

			PostProcessChain* effects = strategy->getPostProcessChain();
			if (!effects) return;

			ASSAO* ao = dynamic_cast<ASSAO*>(effects->getPostProcess(PostProcess::SSAO));
			if (!ao)
				return;
			context.mDefaultSceneObj->m_AssaoShadowStrength = ao->getShadowStrength();
			context.mDefaultSceneObj->m_AssaoShadowPower = ao->getShadowPower();
		}
	}
	void  TODComponentManager::useDefaultObj(TODContext<TODShadowComponent, TodShadowDataHandle>& context)
	{
		if (context.mDefaultSceneObj != nullptr)
		{
			if (mSceneManager)
				mSceneManager->setShadowWeight(context.mDefaultSceneObj->mShadowWeight);

			RenderSystem* system = Root::instance()->getRenderSystem();
			RenderStrategy* strategy = nullptr;
			PostProcessChain* effects = nullptr;
			ASSAO* ao = nullptr;
			if (system)
                strategy = system->getCurRenderStrategy();

			if (strategy && system->getCurrentRenderStrategyType() == RS_Deferred)
			{
				auto deferred = dynamic_cast<DeferredRenderStrategy*>(strategy);
				if (!deferred) return;
				deferred->setAOShadowStrength(context.mDefaultSceneObj->m_AssaoShadowStrength);
				deferred->setAOShadowPower(context.mDefaultSceneObj->m_AssaoShadowPower);
			}
			else
			{
				if (strategy)
					effects = strategy->getPostProcessChain();
				if (effects)
					ao = dynamic_cast<ASSAO*>(effects->getPostProcess(PostProcess::SSAO));
				if (ao)
				{
					ao->setShadowStrength(context.mDefaultSceneObj->m_AssaoShadowStrength);
					ao->setShadowPower(context.mDefaultSceneObj->m_AssaoShadowPower);
				}
			}

			delete context.mDefaultSceneObj;
			context.mDefaultSceneObj = nullptr;
		}
	}

	void TODComponentManager::saveDefaultObj(TODContext<HDRAdaptiveComponent, HDRAdaptiveHandle>& context)
	{
		if (context.mDefaultSceneObj == nullptr)
		{
			context.mDefaultSceneObj = new HDRAdaptiveHandle();
			HDR* pHDR = HDRComponent::getHDR();
			if (pHDR != nullptr)
			{
				Echo::Vector4 eyeAdaptive = pHDR->getEyeAdaptationParams();
				context.mDefaultSceneObj->mEyeAdaptionX = eyeAdaptive.x;
				context.mDefaultSceneObj->mEyeAdaptionY = eyeAdaptive.y;
				context.mDefaultSceneObj->mEyeAdaptionZ = eyeAdaptive.z;
				context.mDefaultSceneObj->mEyeAdaptionW = eyeAdaptive.w;
			}
		}
	}

	void TODComponentManager::useDefaultObj(TODContext<HDRAdaptiveComponent, HDRAdaptiveHandle>& context)
	{
		if (context.mDefaultSceneObj)
		{
		
			HDR* pHDR = HDRComponent::getHDR();

			bool isActive = Root::instance()->getActiveSceneManager() == mSceneManager;
			if (pHDR != nullptr && isActive)
			{
				const auto& data = context.mDefaultSceneObj;
				pHDR->setEyeAdaptationParam0x(data->mEyeAdaptionX);
				pHDR->setEyeAdaptationParam0y(data->mEyeAdaptionY);
				pHDR->setEyeAdaptationParam0z(data->mEyeAdaptionZ);
				pHDR->setEyeAdaptationParam0w(data->mEyeAdaptionW);
			}

			delete context.mDefaultSceneObj;
			context.mDefaultSceneObj = nullptr;
		}
	}

	//添加逻辑
	void  TODComponentManager::saveDefaultObj(TODContext<SphereFogComponent, SphereFogComHandle>& context)
	{
		RenderSystem* mRenderSystem = Root::instance()->getRenderSystem();

		if (!mRenderSystem)
			return;
		RenderStrategy* pStrategy = mRenderSystem->getCurRenderStrategy();
		if (!pStrategy)
			return;
		PostProcessChain* pProcessChain = pStrategy->getPostProcessChain();
		if (!pProcessChain)
			return;
		SphericalTerrainFog* mFog = static_cast<SphericalTerrainFog*> (pProcessChain->getPostProcess(PostProcess::Type::SphericalTerrainFog));
		if (mFog)
			mFog->setIsUseTod(true);
	}

	void  TODComponentManager::useDefaultObj(TODContext<SphereFogComponent, SphereFogComHandle>& context)
	{
		RenderSystem* mRenderSystem = Root::instance()->getRenderSystem();
		if (!mRenderSystem)
			return;
		RenderStrategy* pStrategy = mRenderSystem->getCurRenderStrategy();
		if (!pStrategy)
			return;
		PostProcessChain* pProcessChain = pStrategy->getPostProcessChain();
		if (!pProcessChain)
			return;

		SphericalTerrainFog* mFog = static_cast<SphericalTerrainFog*> (pProcessChain->getPostProcess(PostProcess::Type::SphericalTerrainFog));
		if (mFog)
		{
			mFog->setIsUseTod(false);
		}
	}

	void TODComponentManager::saveDefaultObj(TODContext<SphericalOceanComponent, SphericalOceanContext>& context)
	{
		SphericalOcean* sOcean = getCurrentSphericalOcean();

		if (!sOcean)
		{
			mHaveSphericalOcean = false;
			return;
		}

		mHaveSphericalOcean = true;
		if (nullptr == context.mDefaultSceneObj)
		{
			context.mDefaultSceneObj = new SphericalOceanContext();
			_getSphericalOceanAttribute(*context.mDefaultSceneObj);
		}
	}

	void TODComponentManager::useDefaultObj(TODContext<SphericalOceanComponent, SphericalOceanContext>& context)
	{
		SphericalOcean* sOcean = getCurrentSphericalOcean();

		if (!sOcean || !mHaveSphericalOcean) return;

		SphereOceanShadeSettings shadeSettings;
		sOcean->getShadeSetting(shadeSettings);

		shadeSettings.m_underWaterColor = context.mDefaultSceneObj->mUnderWaterColor;
		// TODO
		shadeSettings.deepColor = context.mDefaultSceneObj->mWaterDeepColor;
		shadeSettings.m_foamColor = context.mDefaultSceneObj->mWaterFoamColor;
		shadeSettings.m_reflectColor = context.mDefaultSceneObj->mWaterReflectColor;
		shadeSettings.shallowColor = context.mDefaultSceneObj->mWaterShallowColor;

		sOcean->setShadeSetting(shadeSettings);
	}

	void TODComponentManager::setTransStateEnd()
	{
		mTransState = S_NoneTrans;
		mIsEffectiveModify = false;
		if (mUnUseAllGravity)
			unUseAllGravityType();
#ifdef ECHO_EDITOR
		mIsEffectiveModify = true;
#endif
	}

	void TODComponentManager::setFinalTodCom(ActorComponent* todCom_, TodComType type_)
	{
		if (mSceneManager == nullptr)
			return;
		if (todCom_ == nullptr && mIsTodTrans[type_] == false)
			return;
		if (todCom_)
			mIsTodTrans[type_] = true;
		else
			mIsTodTrans[type_] = false;
	
		mFinalComVec[type_] = todCom_;
	
		mTodTransState[type_] = true;


		if (!mSceneManager->getSun() || !mSceneManager->getSky() || !mSceneManager->getCloud() || !mSceneManager->getStars()
			||!mSceneManager->getLensFlare() )
		{
			mIsTodDataLoadFinish = false;
			return;
		}

		if (type_ == T_CloudMesh)
		{
			Cloud* pCloud = mSceneManager->getCloud();
				
			mIsUpdateCloud = true;
			pCloud->setCloudFadingOut(eCloudType_Mesh_Up, true);
			pCloud->setCloudFadingOut(eCloudType_Mesh_Mid, true);
			pCloud->setCloudFadingOut(eCloudType_Mesh_Down, true);

			bool isUseCloudMesh_ = _judgeIsUseCloudMesh(pCloud);

			if (mCloudState[type_ - T_CloudMesh] == Cloud_None)
			{
				if (!isUseCloudMesh_)
				{
					mCloudState[type_ - T_CloudMesh] = Cloud_No_Out;
					if (mCloudMeshHandle && mCloudState[0] == Cloud_No_Out)
					{
						mCloudMeshHandle->mOpacity[0] = 0.f;
						mCloudMeshHandle->mOpacity[1] = 0.f;
						mCloudMeshHandle->mOpacity[2] = 0.f;
					}
				}
				else
					mCloudState[type_ - T_CloudMesh] = Cloud_Pre;
			}
			else if (mCloudState[type_ - T_CloudMesh] == Cloud_In || mCloudState[type_ - T_CloudMesh] == Cloud_No_Out)
				mCloudState[type_ - T_CloudMesh] = Cloud_Out;
			else if (mCloudState[type_ - T_CloudMesh] == Cloud_Out)
			{
				// | 白 | 黑 | 红 | 移动在极短时间内从白穿越黑到红，当进入红场景时候
				// 如果当时场景正在做从白到黑的淡出效果，此逻辑会导致场景突变到红场景，而取消过渡效果
				mCloudState[type_ - T_CloudMesh] = Cloud_No_Out;	
			}

			if (mFinalComVec[T_CloudMesh])
			{
				CloudMeshComponent* cloudMeshCom_ = static_cast<CloudMeshComponent*>(mFinalComVec[T_CloudMesh]);
				cloudMeshCom_->setTransFormTime(mTodTransTime[T_CloudMesh]);
				//mTodTransTime[T_CloudMesh] = cloudMeshCom_->getTransFormTime();
			}

		}
		else if (type_ == T_CloudSphere)
		{
			Cloud* pCloud = mSceneManager->getCloud();
			mIsUpdateCloud = true;
			pCloud->setCloudFadingOut(eCloudType_Sphere, true);

			if (mCloudState[type_ - T_CloudMesh] == Cloud_None)
			{
				CloudSphere* cloudSphere_ = pCloud->getCloudSphere();
				if (cloudSphere_ && (cloudSphere_->getVisible() == false || cloudSphere_->getCloudOpacity() == 0.f))
				{
					mCloudState[type_ - T_CloudMesh] = Cloud_No_Out;
					if (mCloudSphereHandle && mCloudState[2] == Cloud_No_Out)
						mCloudSphereHandle->mOpacity = 0.f;
				}
				else
					mCloudState[type_ - T_CloudMesh] = Cloud_Pre;
			}
			else if (mCloudState[type_ - T_CloudMesh] == Cloud_In || mCloudState[type_ - T_CloudMesh] == Cloud_No_Out)
				mCloudState[type_ - T_CloudMesh] = Cloud_Out;
			else if (mCloudState[type_ - T_CloudMesh] == Cloud_Out)
			{
				mCloudState[type_ - T_CloudMesh] = Cloud_No_Out;
			}

			if (mFinalComVec[T_CloudSphere])
			{
				CloudSphereComponent* cloudSphereCom_ = static_cast<CloudSphereComponent*>(mFinalComVec[T_CloudSphere]);
				cloudSphereCom_->setTransFormTime(mTodTransTime[T_CloudSphere]);
				//mTodTransTime[T_CloudSphere] = cloudSphereCom_->getTransFormTime();
			}
		}
		else if (type_ == T_CloudPlane)
		{
			Cloud* pCloud = mSceneManager->getCloud();

			mIsUpdateCloud = true;

			if (mCloudState[type_ - T_CloudMesh] == Cloud_None)
			{
				CloudPlane* cloudPlane_ = pCloud->getCloudPlane();
				if (cloudPlane_ && (cloudPlane_->getVisible() == false || cloudPlane_->getCloudOpacity() == 0.f))
				{
					mCloudState[type_ - T_CloudMesh] = Cloud_No_Out;
					if (mCloudPlaneHandle && mCloudState[1] == Cloud_No_Out)
						mCloudPlaneHandle->mOpacity = 0.f;
				}
				else
					mCloudState[type_ - T_CloudMesh] = Cloud_Pre;
			}
			else if (mCloudState[type_ - T_CloudMesh] == Cloud_In || mCloudState[type_ - T_CloudMesh] == Cloud_No_Out)
				mCloudState[type_ - T_CloudMesh] = Cloud_Out;
			else if (mCloudState[type_ - T_CloudMesh] == Cloud_Out)
			{
				mCloudState[type_ - T_CloudMesh] = Cloud_No_Out;
			}

			if (mFinalComVec[T_CloudPlane])
			{
				CloudPlaneComponent* cloudPlaneCom_ = static_cast<CloudPlaneComponent*>(mFinalComVec[T_CloudPlane]);
				cloudPlaneCom_->setTransFormTime(mTodTransTime[T_CloudPlane]);
				//mTodTransTime[T_CloudPlane] = cloudPalneCom_->getTransFormTime();
			}
		}
		else if (type_ == T_Sky)
		{
			mIsUpdateTod = true;
			if ( mFinalComVec[T_Sky])
			{
				SkyComponent* skyCom_ =  static_cast<SkyComponent*>(mFinalComVec[T_Sky]);
				skyCom_->setTransFormTime(mTodTransTime[T_Sky]);
				//mTodTransTime[T_Sky] = skyCom_->getTransFormTime();
			}
		}
		else if (type_ == T_Fog)
		{
			mIsUpdateTod = true;
			if (mFinalComVec[T_Fog])
			{
				FogComponent* fogCom_ = static_cast<FogComponent*>(mFinalComVec[T_Fog]);
				fogCom_->setTransFormTime(mTodTransTime[T_Fog]);
				//mTodTransTime[T_Fog] = fogCom_->getTransFormTime();
			}
		}
		else if (type_ == T_Sun)
		{	
			mIsUpdateTod = true;
			if (mFinalComVec[T_Sun])
			{
				SunComponent* sunCom_ = static_cast<SunComponent*>(mFinalComVec[T_Sun]);
				sunCom_->setTransFormTime(mTodTransTime[T_Sun]);
				//mTodTransTime[T_Sun] = sunCom_->getTransFormTime();
			}
		}
		else if (type_ == T_DirecLight)
		{
			mIsUpdateTod = true;
			if (mFinalComVec[T_DirecLight])
			{
				DirecLightComponent* direcCom_ = static_cast<DirecLightComponent*>(mFinalComVec[T_DirecLight]);
				direcCom_->setTransFormTime(mTodTransTime[T_DirecLight]);
				//mTodTransTime[T_DirecLight] = direcCom_->getTransFormTime();
			}
		}
		else if (type_ == T_SphereFog)
		{

			if (mFinalComVec[T_SphereFog])
			{
				SphereFogComponent* sphereFogCom_ = static_cast<SphereFogComponent*>(mFinalComVec[T_SphereFog]);
				mTodTransTime[T_SphereFog] = sphereFogCom_->getTransFormTime();
			}

			//当进入无球体雾区域 或者 从无球体雾区域进入有球体雾区域时，不处理过渡，由底层处理
			if (!mFinalComVec[T_SphereFog] || !mIsUseTodSphereFog)
			{
				mIsUseTodSphereFog = !mIsUseTodSphereFog;
				mTodTransState[type_] = false;
				//是否要通知球体雾管理器（通知条件 ：从无到有 || 从有到无）
				mIsNotifyFogMgr = true;
			}
			else
				mIsUpdateTod = true;
		}
#define ELSE_IF(EnumId, ComType)	\
		else if (type_ == EnumId)	\
		{	\
			mIsUpdateTod = true;\
			if (mFinalComVec[EnumId])	\
			{	\
				ComType* pCom_ = static_cast<ComType*>(mFinalComVec[EnumId]);	\
                pCom_->setTransFormTime(mTodTransTime[EnumId]);\
			}\
		}

		ELSE_IF(T_EnvironmentLight, EnvironmentLightComponent)
		ELSE_IF(T_GlobalLight, GlobalLightComponent)
		ELSE_IF(T_HDR, HDRComponent)
		ELSE_IF(T_HDRAdaptive, HDRAdaptiveComponent)
		ELSE_IF(T_Ocean, OceanComponent)
		ELSE_IF(T_PointLight, PointlightManagerComponent)
		ELSE_IF(T_Stars, StarsComponent)
		ELSE_IF(T_UnderWaterCaustics, UnderWaterCausticsComponent)
		ELSE_IF(T_WeatherCoverColor, WeatherCoverColorComponent)
		ELSE_IF(T_Shadow, TODShadowComponent)
		ELSE_IF(T_SphericalOcean, SphericalOceanComponent)
#undef ELSE_IF
		
	}
	void TODComponentManager::updateCloud()
	{
		if (mIsUpdateCloud == false)
			return;

		if (mSceneManager == nullptr)
			return;
		Cloud* pCloud = mSceneManager->getCloud();
		if (pCloud == nullptr)
			return;

		bool isTranslation_ = false;
		for (int i = T_CloudMesh; i <= T_CloudSphere; ++i)
		{
			if (mTodTransState[i] == false)
				continue;

			float timePercent_ = 2.f;
			if (mTodTransTime[i] > std::numeric_limits<float>::epsilon())
			{
				timePercent_ = mCloudElapsed / mTodTransTime[i];
			}

			timePercent_ = std::min(2.f, timePercent_);
			   
			float realPercent = timePercent_;
			//处理时间百分比
			CloudFadingState&  cloudFadeState_ = mCloudState[i - T_CloudMesh];
			if (realPercent > 1.f)
			{
				if (cloudFadeState_ == Cloud_Pre || cloudFadeState_ == Cloud_Out || cloudFadeState_ == Cloud_No_Out)
					realPercent = 1.f;
				else if (cloudFadeState_ == Cloud_In)
					realPercent -= 1.f;
			}
			//realPercent = std::min(1.f, realPercent);
		
			realPercent = Math::Cos((realPercent + 1) * Math::PI) * 0.5f + 0.5f;

			if ((TODSystem::instance()->showLog(LogType::LT_CloudMesh) || TODSystem::instance()->showLog(LogType::LT_CloudSphere) || TODSystem::instance()->showLog(LogType::LT_CloudPlane)) && GetSceneManager()->getName() == g_MainTodComponentName)
			{
				stringstream ss;
				ss << "realPercent : " << realPercent << " calPercent : " << mCloudElapsed / mTodTransTime[i];
				ss << " mTransTimeByClient : " << mTransTimeByClient << " mTodTransTime[i] : " << mTodTransTime[i];
				LogManager::instance()->logMessage(ss.str());
			}

			if (cloudFadeState_ == Cloud_Pre || cloudFadeState_ == Cloud_Out)
			{
				cloudFadeState_ = Cloud_Out;
				if (i == T_CloudSphere)
					_cloudSphereFadeOut(realPercent, pCloud);
				else if (i == T_CloudMesh)
					_cloudMeshFadeOut(realPercent, pCloud);
				else if (i == T_CloudPlane)
					_cloudPlaneFadeOut(realPercent, pCloud);
			}
			else if (cloudFadeState_ == Cloud_In || cloudFadeState_ == Cloud_No_Out)
			{
				if (i == T_CloudSphere)
					_cloudSphereFadeIn(realPercent, pCloud);
				else if (i == T_CloudMesh)
					_cloudMeshFadeIn(realPercent, pCloud);
				else if (i == T_CloudPlane)
					_cloudPlaneFadeIn(realPercent, pCloud);
			}
		/*	else if (cloudFadeState_ == Cloud_No_Out)
			{
				if (i == T_CloudSphere)
					_cloudSphereFadeIn(realPercent, pCloud);
				else if (i == T_CloudMesh)
					_cloudMeshFadeIn(realPercent, pCloud);
				else if (i == T_CloudPlane)
					_cloudPlaneFadeIn(realPercent, pCloud);
			}*/

			mTodComIsChange[i] = false;
			if (cloudFadeState_ == Cloud_None)
			{
				mCloudState[i - T_CloudMesh] = Cloud_None;
				mTodTransState[i] = false;
			}
			isTranslation_ |= mTodTransState[i];
		}

		mCloudElapsed += mTimeElapsed;
		if (!isTranslation_)
		{
			mIsUpdateCloud = false;
		}

	}
	void TODComponentManager::updateTod()
	{
		bool isTranslation_ = false;

		for (int i = T_DirecLight; i < T_End; ++i)
		{
			if (mTodTransState[i] == false)
				continue;
			float timePercent_ = 1.f;
			if (mTodTransTime[i] > std::numeric_limits<float>::epsilon())
			{
			   timePercent_ = mTodElapsed / mTodTransTime[i];
			}

			timePercent_ = std::min(1.f, timePercent_);
			timePercent_ = Math::Cos((timePercent_ + 1) * Math::PI) * 0.5f + 0.5f;

			if (i == T_Sky)
				_setSkyAttribute(timePercent_);
			else if (i == T_Fog)
				_setFogAttribute(timePercent_);
			else if (i == T_DirecLight)
				_setDirecLightAttribute(timePercent_);
			else if (i == T_Sun)
				_setSunAttribute(timePercent_);
			else if (i == T_SphereFog)
				_setSphereFogAttribute(timePercent_);
#define ELSE_IF(EnumId, Name)	\
			else if (i == EnumId)	\
				_set##Name##Attribute(timePercent_);

			ELSE_IF(T_EnvironmentLight, EnvironmentLight)
			ELSE_IF(T_GlobalLight, GlobalLight)
			ELSE_IF(T_HDR, HDR)
			ELSE_IF(T_HDRAdaptive, HDRAdaptive)
			ELSE_IF(T_Ocean, Ocean)
			ELSE_IF(T_PointLight, PointLight)
			ELSE_IF(T_Stars, Stars)
			ELSE_IF(T_UnderWaterCaustics, UnderWaterCaustics)
			ELSE_IF(T_WeatherCoverColor, WeatherCoverColor)
			ELSE_IF(T_Shadow, TODShadow)
			ELSE_IF(T_SphericalOcean, SphericalOcean)
#undef ELSE_IF

			mTodComIsChange[i] = false;
			if (timePercent_ == 1.f)
			{
				mTodTransState[i] = false;
			}
			isTranslation_ |= mTodTransState[i];
		}

		mTodElapsed += mTimeElapsed;

		if (!isTranslation_)
		{
			mIsUpdateTod = false;
		}
	}

	void TODComponentManager::clearComInFinalComVec(ActorComponent* todCom_, TodComType  type_)
	{
		if (type_ < T_End)
		{
			if (mFinalComVec[type_] == todCom_)
				mFinalComVec[type_] = nullptr;
		}
	}

	void TODComponentManager::clearFinalComVec()
	{
		for (int i = 0; i < T_End; ++i)
			mFinalComVec[i] = nullptr;
	}

	float TODComponentManager::getTodTransTime()
	{
		float transTime = FLT_MAX;

		if ( mSkyContext.mIsTranslation && mTodTransTime[T_Sky] != 0.f)
		{
			transTime = std::min(transTime, mTodTransTime[T_Sky]);
		}
		if ( mSunContext.mIsTranslation && mTodTransTime[T_Sun] != 0.f)
		{
			transTime = std::min(transTime, mTodTransTime[T_Sun]);
		}
		if ( mIsDirecLightTranslation && mTodTransTime[T_DirecLight] != 0.f)
		{
			transTime = std::min(transTime, mTodTransTime[T_DirecLight]);
		}
		if ( mIsFogTranslation && mTodTransTime[T_Fog] != 0.f)
		{
			transTime = std::min(transTime, mTodTransTime[T_Fog]);
		}
		if (transTime == FLT_MAX)
			transTime = 0.f;
		return transTime;
	}

	float TODComponentManager::getCloudTransTime()
	{
		float transTime = FLT_MAX;

		if ( mCloudMeshContext.mIsTranslation && mTodTransTime[T_CloudMesh] != 0.f)
		{
			float transTime_ = mTodTransTime[T_CloudMesh] * 2;
			if (mCloudState[T_CloudMesh] == Cloud_In || mCloudState[T_CloudMesh] == Cloud_No_Out)
			{
				transTime_ = mTodTransTime[T_CloudMesh];
				//isDouble_ = false;
			}
			transTime = std::min(transTime, transTime_);
		}
		if ( mCloudSphereContext.mIsTranslation && mTodTransTime[T_CloudSphere] != 0.f)
		{
			float transTime_ = mTodTransTime[T_CloudSphere] * 2;
			if (mCloudState[T_CloudSphere] == Cloud_In || mCloudState[T_CloudSphere] == Cloud_No_Out)
			{
                 transTime_ = mTodTransTime[T_CloudSphere];
				 //isDouble_ = false;
			}
				
			transTime = std::min(transTime, transTime_);
		}
		if ( mCloudPlaneContext.mIsTranslation && mTodTransTime[T_CloudPlane] != 0.f)
		{
			float transTime_ = mTodTransTime[T_CloudPlane] * 2;
			if (mCloudState[T_CloudPlane] == Cloud_In || mCloudState[T_CloudPlane] == Cloud_No_Out)
			{
				transTime_ = mTodTransTime[T_CloudPlane];
				//isDouble_ = false;
			}
			transTime = std::min(transTime, transTime_);
		}
		if (transTime == FLT_MAX)
			transTime = 0.f;
		return transTime;
	}

	void TODComponentManager::addTodComVec(TodTimelineComponent* todTime_)
	{
		if (todTime_ == nullptr)
			return;
		auto iter = std::find(mTodTimelineVec.begin(), mTodTimelineVec.end(), todTime_);
		if (iter != mTodTimelineVec.end())
			return;
		mTodTimelineVec.emplace_back(todTime_);
	}

	void TODComponentManager::delTodComVec(TodTimelineComponent* todTime_)
	{
		auto iter = std::find(mTodTimelineVec.begin(), mTodTimelineVec.end(), todTime_);
		if (iter == mTodTimelineVec.end())
			return;
		mTodTimelineVec.erase(iter);
	}

	void TODComponentManager::clearAllTodComVec()
	{
		mTodTimelineVec.clear();
	}

	void TODComponentManager::addTodTexturePtr(const std::string& name_, const TexturePtr& texturePtr)
	{
		if (mTexturePtrMap.find(name_) == mTexturePtrMap.end())
			mTexturePtrMap.insert(std::make_pair(name_, texturePtr));
		else
			mTexturePtrMap[name_] = texturePtr;
	}

	void TODComponentManager::deleteTodTexturePtr(const std::string& name_)
	{
		if (mTexturePtrMap.find(name_) == mTexturePtrMap.end())
			return;
		mTexturePtrMap.erase(name_);
	}

	void TODComponentManager::clearUpTodTexturePtrMap()
	{
		for (auto& iter : mTexturePtrMap)
			iter.second = nullptr;
		mTexturePtrMap.clear();
	}

	void TODComponentManager::getTodTexturePtrByName(const std::string& name_, TexturePtr& texPtr)
	{
		std::map<std::string, TexturePtr>::const_iterator iter = mTexturePtrMap.find(name_);
		if (iter != mTexturePtrMap.end())
		{
			texPtr = iter->second;
		}
	}

	void TODComponentManager::addCloudMeshPtr(const String& name, const MeshPtr& mesh)
	{
		mMeshPtrMap[name] = mesh;
	}

	void TODComponentManager::deleteCloudMeshPtr(const String& name)
	{
		if (mMeshPtrMap.find(name) == mMeshPtrMap.end())
			return;
		mMeshPtrMap.erase(name);
	}

	void TODComponentManager::clearCloudMeshPtrMap()
	{
		mMeshPtrMap.clear();
	}

	void TODComponentManager::getCloudMeshPtrPtrByName(const String& name, MeshPtr& mesh)
	{
		auto iter = mMeshPtrMap.find(name);
		if (iter != mMeshPtrMap.end())
			mesh = iter->second;
	}

	void TODComponentManager::setTodDayTime(double t)
	{
		mTodTime = t;
		if (mTransState != S_NoneTrans)
			return;
		if (mIsStopUpdate) return;
		_setTodDayTimeAttribute(true);
		if (mWeatherManager && mTodManager)
			mWeatherManager->updateWeatherParam(0.f);
	}

	void TODComponentManager::setOverlayPosAndRadius(const Vector3& pos_, float inRadius_, float outRadius_)
	{
		mOverlayInRadiu = inRadius_;
		mOverlayOutRadiu = outRadius_;
		mOverlayPosition = pos_;
	}

	void TODComponentManager::setIsUseOverlay(bool isUse)
	{
		mIsUseOverlay = isUse;
		if (isUse)
			setOverlayPosAndRadius(mOverlayPosition,mOverlayInRadiu,mOverlayOutRadiu);
	}

	void TODComponentManager::setTodPlaySpeed(float s)
	{
		for (const auto& timeIter : mTodTimelineVec)
		{
			if (timeIter == nullptr)
				continue;
			timeIter->setTodPlaySpeed(s);
		}
	}

	std::vector<TodTimelineComponent*>& TODComponentManager::getTodComVec()
	{
		return mTodTimelineVec;
	}

	void TODComponentManager::enterOrLeaveClientArea()
	{
		if (mTransState == S_NoneTrans)
			return;
		mIsTodDataLoadFinish = true;
		int id = 0;

		//修改为枚举
		CallProtocolFunc(Echo::Name("StopTodTimelinePlay"), nullptr, id);

		if (mIsUseDirecLight && !mDirecVec.empty())
		{
			if (mIsTransSameAttr || (!mIsTransSameAttr && mCurrActiveDirecCom != mDirecVec[id]))
			   setFinalTodCom(mDirecVec[id], T_DirecLight);
		}
		else
			setFinalTodCom(nullptr, T_DirecLight);

		if (mIsUseFog && !mFogVec.empty())
		{
			if (mIsTransSameAttr || (!mIsTransSameAttr && mCurrActiveFogCom != mFogVec[id]))
				setFinalTodCom(mFogVec[id], T_Fog);
		}
		else
			setFinalTodCom(nullptr, T_Fog);


		//开启相同组件过渡时 || (不开启相同组件过渡 && 当前优先级容器持有的第一个指针与当前生效的指针不同时)设置过渡组件指针
#define SETFINALTODCOM(Context, EnumId)	\
		if (Context.mIsUseCurrType && !Context.mComponentVec.empty())\
        {\
            if(mIsTransSameAttr || (!mIsTransSameAttr && Context.mActiveCom !=Context.mComponentVec[id]))\
		        setFinalTodCom(Context.mComponentVec[id], EnumId);	\
		}\
		else \
			setFinalTodCom(nullptr, EnumId);

		SETFINALTODCOM(mCloudMeshContext, T_CloudMesh)
		SETFINALTODCOM(mCloudPlaneContext, T_CloudPlane)
		SETFINALTODCOM(mCloudSphereContext, T_CloudSphere)
        SETFINALTODCOM(mSkyContext, T_Sky)
		SETFINALTODCOM(mSunContext, T_Sun)
		SETFINALTODCOM(mSphereFogContext, T_SphereFog)

		SETFINALTODCOM(mEnvironmentLightContext, T_EnvironmentLight)
		SETFINALTODCOM(mGlobalLightContext, T_GlobalLight)
		SETFINALTODCOM(mHDRContext, T_HDR)
		SETFINALTODCOM(mHDRAdaptiveContext, T_HDRAdaptive)
		SETFINALTODCOM(mOceanContext, T_Ocean)
		SETFINALTODCOM(mPointlightMgrContext, T_PointLight)
		SETFINALTODCOM(mStarsContext, T_Stars)
		SETFINALTODCOM(mUnderWaterCausticsContext, T_UnderWaterCaustics)
		SETFINALTODCOM(mCoverColorContext, T_WeatherCoverColor)
		SETFINALTODCOM(mTODShadowContext, T_Shadow)
		SETFINALTODCOM(mSphericalOceanContext, T_SphericalOcean)
#undef SETFINALTODCOM

        //处理下相关逻辑
		// 与AreaManager的过渡判断同步，AreaManager的淡出结束条件为平面云与球体云都淡出结束，才能结束淡出逻辑
		if ((mTodTransState[T_CloudSphere] && mCloudState[T_CloudSphere] != Cloud_No_Out && mCloudState[T_CloudSphere] != Cloud_None) ||
			(mTodTransState[T_CloudPlane]  && mCloudState[T_CloudPlane] != Cloud_No_Out && mCloudState[T_CloudPlane] != Cloud_None))
		{
			mCloudState[T_CloudSphere] = Cloud_Pre;
			mCloudState[T_CloudPlane] = Cloud_Pre;
		}

		if(!mIsChangeTodStfID)
		   setTransFormTimeFromClient(mTransTimeByClient);
	}


	void TODComponentManager::closeAllTodCom()
	{
		setAllTodComIsUse(false);

		//clearAllTodComVec();
	}

	void TODComponentManager::setTodComIsUse(Echo::ActorComponent* comp, bool b)
	{

		if (comp->getType() == CloudMeshComponent::mType)
		{
			if (getCloudMeshContext().mIsUseCurrType != b)
			{
				setIsUseThisType(getCloudMeshContext(), b);
			}
		}
		else if (comp->getType() == CloudSphereComponent::mType)
		{
			if (getCloudSphereContext().mIsUseCurrType != b)
			{
				setIsUseThisType(getCloudSphereContext(), b);
			}
		}
		else if (comp->getType() == CloudPlaneComponent::mType)
		{
			if (getCloudPlaneContext().mIsUseCurrType != b)
				setIsUseThisType(getCloudPlaneContext(), b);
		}
		else if (comp->getType() == DirecLightComponent::mType)
		{
			if (getDirecLightIsUse() != b)
				setDirecLightIsUse(b);
		}
		else if (comp->getType() == FogComponent::mType)
		{
			if (getFogIsUse() != b)
				setFogIsUse(b);
		}
		else if (comp->getType() == SkyComponent::mType)
		{
			if (getSkyContext().mIsUseCurrType != b)
				setIsUseThisType(getSkyContext(), b);
		}
		else if (comp->getType() == SunComponent::mType)
		{
			if (getSunContext().mIsUseCurrType != b)
				setIsUseThisType(getSunContext(), b);
		}
		else if (comp->getType() == OceanComponent::mType)
		{
			if(mOceanContext.mIsUseCurrType != b)
			   setIsUseThisType(mOceanContext, b);
		}
		else if (comp->getType() == UnderWaterCausticsComponent::mType)
		{
			if(mUnderWaterCausticsContext.mIsUseCurrType != b)
			   setIsUseThisType(mUnderWaterCausticsContext, b);
		}
		else if (comp->getType() == PointlightManagerComponent::mType)
		{
			if(mPointlightMgrContext.mIsUseCurrType != b)
			   setIsUseThisType(mPointlightMgrContext, b);
		}
		else if (comp->getType() == WindPowerComponent::mType)
		{
			if (mWindPowerContext.mIsUseCurrType != b)
			   setIsUseThisType(mWindPowerContext, b);
		}
		else if (comp->getType() == TodTimelineComponent::mType)
		{
			TodTimelineComponent* todTime_ = static_cast<TodTimelineComponent*>(comp);
			addTodComVec(todTime_);
		}
		else if (comp->getType() == EnvTriggerEventComponent::mType)
		{
			if(mEnvTriggerContext.mIsUseCurrType != b)
			    setIsUseThisType(mEnvTriggerContext,b);
		}
		else if (comp->getType() == DriftComponent::mType)
		{
			if (mDriftContext.mIsUseCurrType != b)
			{
				setIsUseThisType(mDriftContext, b);
				static_cast<DriftComponent*>(comp)->setFadeInTime(mTransTimeByClient / 2.0f);
			}
		}
		else if (comp->getType() == StarsComponent::mType)
		{
			if(mStarsContext.mIsUseCurrType != b)
				setIsUseThisType(mStarsContext, b);
		}
		else if(comp->getType() == WeatherCoverColorComponent::mType)
		{
			if (mCoverColorContext.mIsUseCurrType != b)
				setIsUseThisType(mCoverColorContext,b);
		}
		else if (comp->getType() == EnvironmentLightComponent::mType)
		{
			if (mEnvironmentLightContext.mIsUseCurrType != b)
				setIsUseThisType(mEnvironmentLightContext, b);
		}
		else if (comp->getType() == LightningComponent::mType)
		{
			if (mLightningContext.mIsUseCurrType != b)
				setIsUseThisType(mLightningContext, b);
		}
		else if (comp->getType() == HDRComponent::mType)
		{
			if (mHDRContext.mIsUseCurrType != b)
				setIsUseThisType(mHDRContext, b);
		}
		else if (comp->getType() == GlobalLightComponent::mType)
		{
			if (mGlobalLightContext.mIsUseCurrType != b)
				setIsUseThisType(mGlobalLightContext, b);
		}
		else if (comp->getType() == VolumeLightComponent::mType)
		{
			if (mVolumeLightContext.mIsUseCurrType != b)
				setIsUseThisType(mVolumeLightContext, b);
		}
		else if (comp->getType() == TODShadowComponent::mType)
		{
			if (mTODShadowContext.mIsUseCurrType != b)
				setIsUseThisType(mTODShadowContext, b);
		}
		else if (comp->getType() == SphereFogComponent::mType)
		{
			if (mSphereFogContext.mIsUseCurrType != b)
				setIsUseThisType(mSphereFogContext, b);
		}
		else if (comp->getType() == HDRAdaptiveComponent::mType)
		{
			if (mHDRAdaptiveContext.mIsUseCurrType != b)
				setIsUseThisType(mHDRAdaptiveContext, b);
		}
		else if (comp->getType() == SphericalOceanComponent::mType)
		{
			if (mSphericalOceanContext.mIsUseCurrType != b)
				setIsUseThisType(mSphericalOceanContext, b);
		}

	}

	// ----------------------------------------------------------------------------------------------
	
	Echo::ProtocolResult EchoTodComponentMgrProFuncImp::ProCom_TodChangeCompleted(void* data, ProtocolHandle  nemeHandle)
	{
		Name* levelName_ = (Name*)(nemeHandle);
		TODComponentManager* tMgr = TODComponentSystem::instance()->getTODComponentManager(*levelName_);
		if (tMgr == nullptr)
			return  Echo::PR_Falied;

		if (tMgr->mIsChangeTodStfID == false)
			return PR_Falied;
		if (data == nullptr)
			return PR_Falied;

		tMgr->_StfLoadFinishCallBack(data);
		return PR_OK;
	}
	Echo::ProtocolResult EchoTodComponentMgrProFuncImp::ProCom_TodResetTodComDate(void* date, ProtocolHandle)
	{
		if (date == nullptr)
			return Echo::PR_Falied;
		Name* levelName_ = static_cast<Name*>(date);

		TODComponentManager* manger = TODComponentSystem::instance()->getTODComponentManager(*levelName_);
		if (!manger) 
			return Echo::PR_Falied;
		if (manger->mTransState != S_NoneTrans)
			return Echo::PR_Falied;
		if (manger->getCloudMeshContext().mIsUseCurrType && manger->getCloudMeshContext().mActiveCom)
		{
			manger->getCloudMeshContext().mActiveCom->unuse();
            manger->getCloudMeshContext().mActiveCom->use();
		}
			

		if (manger->getCloudPlaneContext().mIsUseCurrType && manger->getCloudPlaneContext().mActiveCom)
		{
			manger->getCloudPlaneContext().mActiveCom->unuse();
			manger->getCloudPlaneContext().mActiveCom->use();
		}
		
		if (manger->getCloudSphereContext().mIsUseCurrType && manger->getCloudSphereContext().mActiveCom)
		{
			manger->getCloudSphereContext().mActiveCom->unuse();
			manger->getCloudSphereContext().mActiveCom->use();
		}
		
		if (manger->getDirecLightIsUse() && manger->getCurrActiveDirecLightCom())
		{
			manger->getCurrActiveDirecLightCom()->setScreenAttribute();
		}

		if (manger->getFogIsUse() && manger->getCurrActiveFogCom())
			manger->getCurrActiveFogCom()->setMainSceneFog();

		if (manger->getSkyContext().mIsUseCurrType && manger->getSkyContext().mActiveCom)
		{
			manger->getSkyContext().mActiveCom->unuse();
			manger->getSkyContext().mActiveCom->use();
		}
		
		if (manger->getSunContext().mIsUseCurrType && manger->getSunContext().mActiveCom)
		{
			manger->getSunContext().mActiveCom->unuse();
			manger->getSunContext().mActiveCom->use();
		}

		TODContext<EnvironmentLightComponent, EnvironmentLightHandle>& enLightContext_ = manger->getEnvironmentLightContext();
		if (enLightContext_.mIsUseCurrType && enLightContext_.mActiveCom)
		{
			enLightContext_.mActiveCom->unuse();
			enLightContext_.mActiveCom->use();
		}

		TODContext<OceanComponent, Ocean>& oceanContext_ = manger->getOceanContext();
		if (oceanContext_.mIsUseCurrType && oceanContext_.mActiveCom)
		{
			oceanContext_.mActiveCom->unuse();
			oceanContext_.mActiveCom->use();
		}

		TODContext<WindPowerComponent, WindPowerHandle>& windPowerContext_ = manger->getWindPowerContext();
		if (windPowerContext_.mIsUseCurrType && windPowerContext_.mActiveCom)
		{
			windPowerContext_.mActiveCom->unuse();
			windPowerContext_.mActiveCom->use();
		}

		TODContext<UnderWaterCausticsComponent, CausticsHandle>& underWaterContext_ = manger->getUnderWaterCausticsContext();
		if (underWaterContext_.mIsUseCurrType && underWaterContext_.mActiveCom)
		{
			underWaterContext_.mActiveCom->unuse();
			underWaterContext_.mActiveCom->use();
		}

		TODContext<PointlightManagerComponent, pLightManagerHandle>& pointlightMgrContext_ = manger->getPointlightMgrContext();
		if (pointlightMgrContext_.mIsUseCurrType && pointlightMgrContext_.mActiveCom)
		{
			pointlightMgrContext_.mActiveCom->unuse();
			pointlightMgrContext_.mActiveCom->use();
		}

		TODContext<StarsComponent, Stars>& starsContext_ = manger->getStarsContext();
		if (starsContext_.mIsUseCurrType && starsContext_.mActiveCom)
		{
			starsContext_.mActiveCom->unuse();
			starsContext_.mActiveCom->use();
		}

		TODContext<WeatherCoverColorComponent, WeatherCoverColorComData>& weatherCoverColorContext_ = manger->getWeatherCoverColorContext();
		if (weatherCoverColorContext_.mIsUseCurrType && weatherCoverColorContext_.mActiveCom)
		{
			/*weatherCoverColorContext_.mActiveCom->unuse();
			weatherCoverColorContext_.mActiveCom->use();*/
			weatherCoverColorContext_.mActiveCom->resetCoverColor();
		}

		TODContext<HDRComponent, HDRHandle>& hdrContext_ = manger->getHDRContext();
		if (hdrContext_.mIsUseCurrType && hdrContext_.mActiveCom)
		{
			hdrContext_.mActiveCom->unuse();
			hdrContext_.mActiveCom->use();
		}

		TODContext<GlobalLightComponent, GlobalLightHandle>& globalLightContext_ = manger->getGlobalLightContext();
		if (globalLightContext_.mIsUseCurrType && globalLightContext_.mActiveCom)
		{
			globalLightContext_.mActiveCom->unuse();
			globalLightContext_.mActiveCom->use();
		}

		TODContext<VolumeLightComponent, VolumeLightComponentContext>& volumeLightContext_ = manger->getVolumeLightContext();
		if (volumeLightContext_.mIsUseCurrType && volumeLightContext_.mActiveCom)
		{
			volumeLightContext_.mActiveCom->unuse();
			volumeLightContext_.mActiveCom->use();
		}

		TODContext<TODShadowComponent, TodShadowDataHandle>& todShaderContext_ = manger->getTODShadowContext();
		if (todShaderContext_.mIsUseCurrType && todShaderContext_.mActiveCom)
		{
			todShaderContext_.mActiveCom->unuse();
			todShaderContext_.mActiveCom->use();
		}

		//TODContext<LightningComponent, LightningHandle>& lightningContext_ = manger->getLightningContext();
		//if (lightningContext_.mIsUseCurrType && lightningContext_.mActiveCom)
		//{
		//	//lightningContext_.mActiveCom->unuse();
		//	//lightningContext_.mActiveCom->use();
		//	lightningContext_.mActiveCom->resetLightning();
		//}

		TODContext<SphereFogComponent, SphereFogComHandle>& planetConContext_ = manger->getSphereFogContext();
		if (planetConContext_.mIsUseCurrType && planetConContext_.mActiveCom)
		{
			planetConContext_.mActiveCom->unuse();
			planetConContext_.mActiveCom->use();
		}

		TODContext<HDRAdaptiveComponent, HDRAdaptiveHandle>& hdrAdaptiveContext = manger->getHDRAdaptiveContext();
		if (hdrAdaptiveContext.mIsUseCurrType && hdrAdaptiveContext.mActiveCom)
		{
			hdrAdaptiveContext.mActiveCom->unuse();
			hdrAdaptiveContext.mActiveCom->use();
		}

		TODContext<SphericalOceanComponent, SphericalOceanContext>& sphOceanContext = manger->getSphericalOceanContext();
		if (sphOceanContext.mIsUseCurrType && sphOceanContext.mActiveCom)
		{
			sphOceanContext.mActiveCom->unuse();
			sphOceanContext.mActiveCom->use();
		}
		return Echo::PR_OK;
	}
	
	void EchoTodComponentMgrProFuncImp::initProtocolFunction()
	{
		ProtocolComponentSystem* p = ProtocolComponentSystem::instance();
		p->registerProtocolFunc(ProtocolFuncID::PFID_SetTodComData, &EchoTodComponentMgrProFuncImp::ProCom_TodChangeCompleted);
	    p->registerProtocolFunc(ProtocolFuncID::PFID_ResetTodComAttribute, &EchoTodComponentMgrProFuncImp::ProCom_TodResetTodComDate);
		p->registerProtocolFunc(ProtocolFuncID::PFID_CloseAllTodCom, &EchoTodComponentMgrProFuncImp::ProCom_CloseAllTodCom);
		p->registerProtocolFunc(ProtocolFuncID::PFID_GetAreaMgrSceneTriggerData, &EchoTodComponentMgrProFuncImp::ProCom_GetAreaManagerSceneDataHelper);
		p->registerProtocolFunc(ProtocolFuncID::PFID_SetTodComTickWay, &EchoTodComponentMgrProFuncImp::ProCom_SetTodComTickWay);
		p->registerProtocolFunc(ProtocolFuncID::PFID_GetTodActiveData, &EchoTodComponentMgrProFuncImp::ProCom_ShowTodCurrActiveComData);
		p->registerProtocolFunc(ProtocolFuncID::PFID_ProcessingCacheData, &EchoTodComponentMgrProFuncImp::ProCom_ProcessCacheData);
		p->registerProtocolFunc(ProtocolFuncID::PFID_ShowLogData, &EchoTodComponentMgrProFuncImp::ProCom_ShowLogData);
		p->registerProtocolFunc(ProtocolFuncID::PFID_ReplaceSTF, &EchoTodComponentMgrProFuncImp::ProCom_ReplaceSTF);
	}
	void EchoTodComponentMgrProFuncImp::uninitProtocolFunction()
	{
		ProtocolComponentSystem* p = ProtocolComponentSystem::instance();
		p->unregisterProtocolFunc(ProtocolFuncID::PFID_SetTodComData);
		p->unregisterProtocolFunc(ProtocolFuncID::PFID_ResetTodComAttribute);
		p->unregisterProtocolFunc(ProtocolFuncID::PFID_CloseAllTodCom);
		p->unregisterProtocolFunc(ProtocolFuncID::PFID_GetAreaMgrSceneTriggerData);
		p->unregisterProtocolFunc(ProtocolFuncID::PFID_SetTodComTickWay);
		p->unregisterProtocolFunc(ProtocolFuncID::PFID_GetTodActiveData);
		p->unregisterProtocolFunc(ProtocolFuncID::PFID_ProcessingCacheData);
		p->unregisterProtocolFunc(ProtocolFuncID::PFID_ShowLogData);
		p->unregisterProtocolFunc(ProtocolFuncID::PFID_ReplaceSTF);

		if (TODUtil::Alive()) delete TODUtil::instance();
	}

	Echo::ProtocolResult EchoTodComponentMgrProFuncImp::ProCom_CloseAllTodCom(void* data, ProtocolHandle)
	{
		if (data == nullptr)
			return Echo::PR_Falied;
		Name* levelName_ = static_cast<Name*>(data);

		TODComponentManager* tMgr = TODComponentSystem::instance()->getTODComponentManager(*levelName_);
		if (tMgr == nullptr)
			return  Echo::PR_Falied;
		tMgr->deinitTodComData();
		tMgr->mIsOcean = true;//关闭所有组件之后需要重新判断当前场景是否存在海洋
		return  Echo::PR_OK;
	}

	Echo::ProtocolResult EchoTodComponentMgrProFuncImp::ProCom_GetAreaManagerSceneDataHelper(void* data, ProtocolHandle nameHandle)
	{
		if (data == nullptr)
			return Echo::PR_Falied;
		Name* levelName_ = (Name*)(nameHandle);
		TODComponentManager* tMgr = TODComponentSystem::instance()->getTODComponentManager(*levelName_);
		if (tMgr == nullptr)
			return  Echo::PR_Falied;
		tMgr->setTodComMgrSceneTriggerData(static_cast<SceneTriggerData*>(data));
		return Echo::PR_OK;
	}

	Echo::ProtocolResult EchoTodComponentMgrProFuncImp::ProCom_SetTodComTickWay(void* data, ProtocolHandle)
	{
		if (data == nullptr)
			return  Echo::PR_Falied;

		int isUseEchoTick_ = *static_cast<int*>(data);
		if (isUseEchoTick_ == 0)
			TODComponentSystem::instance()->setTodComMgrTickWay(false);
		else if(isUseEchoTick_ == 1)
			TODComponentSystem::instance()->setTodComMgrTickWay(true);
		return Echo::PR_OK;
	}

	Echo::ProtocolResult EchoTodComponentMgrProFuncImp::ProCom_ShowTodCurrActiveComData(void* data, ProtocolHandle)
	{
		TODComponentManager* tMgr = TODComponentSystem::instance()->getActiveTodComManager();
		if (tMgr == nullptr)
			return  Echo::PR_Falied;

		tMgr->showTodActiveData();
		
		return Echo::PR_OK;
	}

	Echo::ProtocolResult EchoTodComponentMgrProFuncImp::ProCom_ProcessCacheData(void* data, ProtocolHandle)
	{
		if (data == nullptr)
			return PR_Falied;

		std::string* levelName = (std::string*)data;
		TODComponentManager* tMgr = TODComponentSystem::instance()->getTODComponentManager(Name(*levelName));
		if (tMgr == nullptr)
			return PR_Falied;
		tMgr->processCacheData();
		return PR_OK;
	}

	Echo::ProtocolResult EchoTodComponentMgrProFuncImp::ProCom_ShowLogData(void* data, ProtocolHandle)
	{
		if (data == nullptr)
		{
			TODSystem::instance()->clearLogSiwtch();
			return PR_OK;
		}

		std::string* type = static_cast<std::string*>(data);

		if ((*type) == "CloudMesh")
			TODSystem::instance()->setLogSwitch(LogType::LT_CloudMesh);
		else if ((*type) == "CloudSphere")
			TODSystem::instance()->setLogSwitch(LogType::LT_CloudSphere);
		else if ((*type) == "CloudPlane")
			TODSystem::instance()->setLogSwitch(LogType::LT_CloudPlane);
		else if ((*type) == "Fog")
			TODSystem::instance()->setLogSwitch(LogType::LT_Fog);

		return PR_OK;
	}

	Echo::ProtocolResult EchoTodComponentMgrProFuncImp::ProCom_ReplaceSTF(void* data, ProtocolHandle handle)
	{
		if (!TODUtil::Alive())
			new TODUtil();

		if (!TODUtil::instance()->GetState())
			TODUtil::instance()->SetReplaceTODState(true);

		bool useDefault = (bool)handle;

		TODUtil::instance()->ReplaceTOD(useDefault);

		return PR_OK;
	}


	void TODComponentManager::setAllTodComIsUse(bool isUse_)
	{
		if (mCloudMeshContext.mIsUseCurrType != isUse_)
			setIsUseThisType(mCloudMeshContext, isUse_);

		if (mCloudSphereContext.mIsUseCurrType != isUse_)
			setIsUseThisType(mCloudSphereContext, isUse_);

		if (mCloudPlaneContext.mIsUseCurrType != isUse_)
			setIsUseThisType(mCloudPlaneContext, isUse_);

		if (mIsUseDirecLight != isUse_)
			setDirecLightIsUse(isUse_);

		if (mIsUseFog != isUse_)
			setFogIsUse(isUse_);

		if (mSkyContext.mIsUseCurrType != isUse_)
			setIsUseThisType(mSkyContext, isUse_);

		if (mSunContext.mIsUseCurrType != isUse_)
			setIsUseThisType(mSunContext, isUse_);

		if (mOceanContext.mIsUseCurrType != isUse_)
			setIsUseThisType(mOceanContext, isUse_);

		if (mUnderWaterCausticsContext.mIsUseCurrType != isUse_)
			setIsUseThisType(mUnderWaterCausticsContext, isUse_);

		if (mPointlightMgrContext.mIsUseCurrType != isUse_)
			setIsUseThisType(mPointlightMgrContext, isUse_);

		if (mWindPowerContext.mIsUseCurrType != isUse_)
			setIsUseThisType(mWindPowerContext, isUse_);

		if (mEnvTriggerContext.mIsUseCurrType != isUse_)
			setIsUseThisType(mEnvTriggerContext, isUse_);

		if (mDriftContext.mIsUseCurrType != isUse_)
			setIsUseThisType(mDriftContext, isUse_);

		if (mStarsContext.mIsUseCurrType != isUse_)
			setIsUseThisType(mStarsContext, isUse_);

		if (mCoverColorContext.mIsUseCurrType != isUse_)
			setIsUseThisType(mCoverColorContext, isUse_);

		if(mEnvironmentLightContext.mIsUseCurrType != isUse_)
			setIsUseThisType(mEnvironmentLightContext, isUse_);

		if (mLightningContext.mIsUseCurrType != isUse_)
			setIsUseThisType(mLightningContext, isUse_);

		if (mHDRContext.mIsUseCurrType != isUse_)
			setIsUseThisType(mHDRContext, isUse_);

		if(mGlobalLightContext.mIsUseCurrType != isUse_)
			setIsUseThisType(mGlobalLightContext, isUse_);

		if (mVolumeLightContext.mIsUseCurrType != isUse_)
			setIsUseThisType(mVolumeLightContext, isUse_);

		if (mTODShadowContext.mIsUseCurrType != isUse_)
			setIsUseThisType(mTODShadowContext, isUse_);

		if(mSphereFogContext.mIsUseCurrType != isUse_)
			setIsUseThisType(mSphereFogContext, isUse_);
		if (mHDRAdaptiveContext.mIsUseCurrType != isUse_)
			setIsUseThisType(mHDRAdaptiveContext, isUse_);
		if (mSphericalOceanContext.mIsUseCurrType != isUse_)
			setIsUseThisType(mSphericalOceanContext, isUse_);
	}

	void TODComponentManager::showTodActiveData()
	{
		
		std::string logData_ = "\n weatherData:{ \n";

		if (!mSceneManager)
		{
			logData_.append("Scene Init False!!!");
			logData_.append("\n}\n");
			LogManager::getSingleton().logMessage(Echo::LML_NORMAL, logData_.data());
			return;
		}

		logData_.append("Stf Id :").append(std::to_string(mTodId).data()).append("\n");
		if (mCloudMeshContext.mActiveCom)
		{
			logData_.append("CloudMeshCom : ").append(mCloudMeshContext.mActiveCom->getCombinedName());
			CloudMeshComponent* meshCom_ = static_cast<CloudMeshComponent*>(mCloudMeshContext.mActiveCom);
			logData_.append(" || texture : ");

			for (const std::string& texName_ : meshCom_->getTextureName())
				logData_.append(texName_.c_str()).append(" | ");
			logData_.append("\n");
		}

		if (mCloudSphereContext.mActiveCom)
		{
			logData_.append("CloudSphereCom : ").append(mCloudSphereContext.mActiveCom->getCombinedName());
			CloudSphereComponent* sphereCom_ = static_cast<CloudSphereComponent *>(mCloudSphereContext.mActiveCom);
			logData_.append(" || texture : ").append(sphereCom_->getTextureName()).append("\n");
		}

		if (mCloudPlaneContext.mActiveCom)
		{
			logData_.append("CloudPlaneCom : ").append(mCloudPlaneContext.mActiveCom->getCombinedName());
			CloudPlaneComponent* planeCom_ = static_cast<CloudPlaneComponent*>(mCloudPlaneContext.mActiveCom);
			logData_.append(" || cloudTexture : ").append(planeCom_->getTextureName());
			logData_.append(" || starsTexture : ").append(planeCom_->getStarsTextureName()).append("\n");
		}
			

		if (mCurrActiveDirecCom)
			logData_.append("DirecLightCom : ").append(mCurrActiveDirecCom->getCombinedName()).append("\n");

		if (mCurrActiveFogCom)
			logData_.append("FogCom : ").append(mCurrActiveFogCom->getCombinedName()).append("\n");

		if (mSkyContext.mActiveCom)
			logData_.append("SkyCom : ").append(mSkyContext.mActiveCom->getCombinedName()).append("\n");

		if (mSunContext.mActiveCom)
			logData_.append("SunCom : ").append(mSunContext.mActiveCom->getCombinedName()).append("\n");

		if (mOceanContext.mActiveCom)
			logData_.append("OceanCom : ").append(mOceanContext.mActiveCom->getCombinedName()).append("\n");

		if (mUnderWaterCausticsContext.mActiveCom)
			logData_.append("UnderWaterCom : ").append(mUnderWaterCausticsContext.mActiveCom->getCombinedName()).append("\n");

		if (mPointlightMgrContext.mActiveCom)
			logData_.append("PointLightMgrCom : ").append(mPointlightMgrContext.mActiveCom->getCombinedName()).append("\n");

		if (mWindPowerContext.mActiveCom)
			logData_.append("WindPowerMgrCom : ").append(mWindPowerContext.mActiveCom->getCombinedName()).append("\n");

		if (mEnvTriggerContext.mActiveCom)
			logData_.append("WeatherParticleCom : ").append(mEnvTriggerContext.mActiveCom->getCombinedName()).append("\n");

		if (mDriftContext.mActiveCom)
			logData_.append("DriftCom : ").append(mDriftContext.mActiveCom->getCombinedName()).append("\n");

		if (mStarsContext.mActiveCom)
			logData_.append("StarsCom : ").append(mStarsContext.mActiveCom->getCombinedName()).append("\n");

		if (mCoverColorContext.mActiveCom)
			logData_.append("WeatherCoverColorCom : ").append(mCoverColorContext.mActiveCom->getCombinedName()).append("\n");

		if (mEnvironmentLightContext.mActiveCom )
			logData_.append("EnvironmentLightCom : ").append(mEnvironmentLightContext.mActiveCom->getCombinedName()).append("\n");

		if (mLightningContext.mActiveCom )
			logData_.append("LightningCom : ").append(mLightningContext.mActiveCom->getCombinedName()).append("\n");

		if (mHDRContext.mActiveCom)
			logData_.append("HDRCom : ").append(mHDRContext.mActiveCom->getCombinedName()).append("\n");

		if (mGlobalLightContext.mActiveCom)
			logData_.append("GlobalLightCom : ").append(mGlobalLightContext.mActiveCom->getCombinedName()).append("\n");

		if (mVolumeLightContext.mActiveCom)
			logData_.append("VolumeLightCom : ").append(mVolumeLightContext.mActiveCom->getCombinedName()).append("\n");

		if (mTODShadowContext.mActiveCom)
			logData_.append("TODShadowCom : ").append(mTODShadowContext.mActiveCom->getCombinedName()).append("\n");

		if (mHDRAdaptiveContext.mActiveCom)
			logData_.append("HDRAdaptiveCom : ").append(mHDRAdaptiveContext.mActiveCom->getCombinedName()).append("\n");

		if (mSphereFogContext.mActiveCom)
			logData_.append("SphereFogCom : ").append(mSphereFogContext.mActiveCom->getCombinedName()).append("\n");

		if (mEnvTriggerContext.mActiveCom)
			logData_.append("EnvTriggerCom : ").append(mEnvTriggerContext.mActiveCom->getCombinedName()).append("\n");
		if (mSphericalOceanContext.mActiveCom)
		{
			String value = "SphericalOceanCom : ";
			value += mSphericalOceanContext.mActiveCom->getCombinedName() + "\n";
			logData_.append(value);
		}
		logData_.append("----------\n");
		Cloud* cloud_ = mSceneManager->getCloud();
	    if( cloud_)
		{
			CloudMesh* cloudUpMesh_ = cloud_->getCloudMeshUp();
			if (cloudUpMesh_)
			{
				logData_.append(std::string(("Cloud Up Mesh fading state: ")));
				logData_.append(cloudUpMesh_->getCloudFadingOut() ? "true" : "false");
				std::string tempName_ = "";
				cloudUpMesh_->getCloudEffectTextureName(tempName_);
				logData_.append(" || texture : ").append(tempName_);
				logData_.append("\n");
			}

			CloudMesh* cloudMidMesh_ = cloud_->getCloudMeshMid();
			if (cloudMidMesh_)
			{
				logData_.append(std::string(("Cloud Mid Mesh fading state: ")));
				logData_.append(cloudMidMesh_->getCloudFadingOut() ? "true" : "false");
				std::string tempName_ = "";
				cloudMidMesh_->getCloudEffectTextureName(tempName_);
				logData_.append(" || texture : ").append(tempName_);
				logData_.append("\n");
			}

			CloudMesh* cloudDownMesh_ = cloud_->getCloudMeshDown();
			if (cloudDownMesh_)
			{
				logData_.append("Cloud Down Mesh fading state: ");
				logData_.append(cloudDownMesh_->getCloudFadingOut() ? "true" : "false");
				std::string tempName_ = "";
				cloudDownMesh_->getCloudEffectTextureName(tempName_);
				logData_.append(" || texture : ").append(tempName_);
				logData_.append("\n");
			}

			CloudSphere* cloudSphere_ = cloud_->getCloudSphere();
			if (cloudSphere_)
			{
				logData_.append(std::string(("Cloud Sphere fading state: ")));
				logData_.append(cloudSphere_->getCloudFadingOut() ? "true" : "false");
				std::string tempName_ = "";
				cloudSphere_->getCloudEffectTextureName(tempName_);
				logData_.append(" || texture : ").append(tempName_);
				logData_.append("\n");
			}


			CloudPlane* cloudPlane_ = cloud_->getCloudPlane();
			if (cloudPlane_)
			{
				logData_.append("cloud plane : ");
				logData_.append("cloudTexure : ").append(cloudPlane_->getCloudTextureName());
				logData_.append(" || starsTexture").append(cloudPlane_->getStarsTextureName()).append("\n");
			}
		}

		logData_.append("SceneName : ").append(mName.c_str()).append("\n");
		logData_.append("Trans state : ").append(mTransState == S_Trans ? "Trans" : "No Trans").append("\n");
		if(mTodManager)
		   logData_.append("Is Moon Rise : ").append(mTodManager->isMoonRise() ? "true" : "false");


		std::stringstream ss;
		ss << logData_;
		ss <<"\nIs Use Custom Gracvity : " << mSceneManager->getIsUseGravityDir() << std::endl;
		ss << "Gravity Dir : " << mSceneManager->getGravityDir() << std::endl;
		ss << "Gravity Level : "<< mSceneManager->getGravityHeight() << std::endl;
		ss << "Gravity Pos : " << mSceneManager->getGravityPos() << std::endl;

		if (mTodManager)
		{
			std::string colorName_;
			mTodManager->getActiveColorChartName(colorName_);

			//获取一下是否开启colorGrading

			ss << "Is Use Custom ColorChart : " << mTodManager->isUseCustomColorChart() << std::endl;
			ss << "ColorChart Nane : " << colorName_ << std::endl;

			const std::multimap<std::string, int>&  errorData_ =  mTodManager->getErrorId();
			for (const std::multimap<std::string, int>::value_type& value : errorData_)
			{
				ss << "Weather Id  Path is Empty() : " << value.second << " Area Name : " << value.first<<std::endl;
			}
		}

		ss << " time : " << mTodTime << std::endl;
		ss << "\n}\n";

		LogManager::getSingleton().logMessage(Echo::LML_NORMAL, ss.str());
	}

	void TODComponentManager::processCacheData()
	{
		if (mIsTodDataLoadFinish == false)
		{
			getWeatherTransStartState();
			enterOrLeaveClientArea();
			mIsTodDataLoadFinish = true;
		}
	}

	void TODComponentManager::_setTodDayTimeAttribute(bool bSetTod_)
	{
		//非激活场景不处理天气相关的tick
		if (Root::instance()->getActiveSceneManager() != mSceneManager)
			return;

		if (mTodManager && mSceneManager && bSetTod_)
		{
			mTodManager->setStaticTime(mTodTime);		// mTodTime更类似于百分比或比值类参数
		}
		for (const auto& timeIter : mTodTimelineVec)
		{
			if (timeIter == nullptr)
				continue;
			timeIter->currTime = static_cast<float>(mTodTime) * 60 * 60 * 24;
			timeIter->updateTime();
			timeIter->setComponentAttribute();
		}
	}

	bool TODComponentManager::_judgeIsUseCloudMesh(Cloud* pCloud)
	{
		if (pCloud == nullptr)
			return false;
		CloudMesh* upCloud = pCloud->getCloudMeshUp();

		if (upCloud)
		{
			if (upCloud->getVisible() == false)
				mIsUpCloudMeshUse = false;
			else
			{
				if (upCloud->getMeshFile().empty())
					mIsUpCloudMeshUse = false;
				else
					mIsUpCloudMeshUse = true;
			}
		}
		CloudMesh* midCloud = pCloud->getCloudMeshMid();
		if (midCloud)
		{
			if (midCloud->getVisible() == false)
				mIsMidCloudMeshUse = false;
			else
			{
				if (midCloud->getMeshFile().empty())
					mIsMidCloudMeshUse = false;
				else
					mIsMidCloudMeshUse = true;
			}
		}
		CloudMesh* downCloud = pCloud->getCloudMeshDown();
		if (downCloud)
		{
			if (downCloud->getVisible() == false)
				mIsDownCloudMeshUse = false;
			else
			{
				if (downCloud->getMeshFile().empty())
					mIsDownCloudMeshUse = false;
				else
					mIsDownCloudMeshUse = true;
			}
		}
		if (!mIsDownCloudMeshUse && !mIsMidCloudMeshUse && !mIsUpCloudMeshUse)
			return false;
		return true;
	}

	void TODComponentManager::_dealWithFirstTick()
	{
		if (mIsFirstTodTick)
		{

			mIsEffectiveModify = true;

			mIsFirstTodTick = false;
			if (mIsChangeTodStfID)
				mIsChangeTodStfID = false;

			{
				updateDirecLight(nullptr);
				if (mFinalDirecData && mTodTransState[T_DirecLight])
					_getSceneDirecLightData(*mFinalDirecData);
			}

			{
				updateFog(nullptr);
				if (mFinalFogData && mTodTransState[T_Fog])
					_getFogAttribute(*mFinalFogData);
			}
			
			{
				SkyComponent* skyCom_ = static_cast<SkyComponent*>(mFinalComVec[T_Sky]);
				updateContext(mSkyContext, skyCom_);
				if (mFinalSkyData && mTodTransState[T_Sky])
					_getSkyAttribute(*mFinalSkyData);
			}
			
			{
				SunComponent* sunCom_ = static_cast<SunComponent*>(mFinalComVec[T_Sun]);
				updateContext(mSunContext, sunCom_);
				if (mFinalSunData && mTodTransState[T_Sun])
					_getSunAttribute(*mFinalSunData);
			}

			{
				SphereFogComponent* SphereFogCom_ = static_cast<SphereFogComponent*>(mFinalComVec[T_SphereFog]);
				updateContext(mSphereFogContext, SphereFogCom_);
				if (mFinalSphereFogData && mTodTransState[T_SphereFog])
					_getSphereFogAttribute(*mFinalSphereFogData);

				if (mIsNotifyFogMgr)
				{
					mIsNotifyFogMgr = false;
					RenderSystem* mRenderSystem = Root::instance()->getRenderSystem();
					if (!mRenderSystem)
						return;
					RenderStrategy* pStrategy = mRenderSystem->getCurRenderStrategy();
					if (!pStrategy)
						return;
					PostProcessChain* pProcessChain = pStrategy->getPostProcessChain();
					if (!pProcessChain)
						return;

					SphericalTerrainFog* mFog = static_cast<SphericalTerrainFog*> (pProcessChain->getPostProcess(PostProcess::Type::SphericalTerrainFog));
					if (mFog)
					{
						mFog->setIsUseTod(mIsUseTodSphereFog);
						mFog->setTransTime(mTransTimeByClient);
					}
				}
			}

#define UpdataTODComponent(Name, Context, Type, EnumId)	\
			{	\
				Type*	pCom = static_cast<Type*>(mFinalComVec[EnumId]);	\
				updateContext(Context, pCom);	\
				if (m##Name##Handle && mTodTransState[EnumId])	\
					_get##Name##Attribute(*mFinal##Name##Handle);	\
			}

			UpdataTODComponent(EnvironmentLight, mEnvironmentLightContext, EnvironmentLightComponent, T_EnvironmentLight)
			UpdataTODComponent(GlobalLight, mGlobalLightContext, GlobalLightComponent, T_GlobalLight)
			UpdataTODComponent(HDR, mHDRContext, HDRComponent, T_HDR)
			UpdataTODComponent(HDRAdaptive, mHDRAdaptiveContext, HDRAdaptiveComponent, T_HDRAdaptive)
			UpdataTODComponent(Ocean, mOceanContext, OceanComponent, T_Ocean)
			UpdataTODComponent(PointLight, mPointlightMgrContext, PointlightManagerComponent, T_PointLight)
			UpdataTODComponent(Stars, mStarsContext, StarsComponent, T_Stars)
			UpdataTODComponent(UnderWaterCaustics, mUnderWaterCausticsContext, UnderWaterCausticsComponent, T_UnderWaterCaustics)
			UpdataTODComponent(WeatherCoverColor, mCoverColorContext, WeatherCoverColorComponent, T_WeatherCoverColor)
			UpdataTODComponent(TODShadow, mTODShadowContext, TODShadowComponent, T_Shadow)
			UpdataTODComponent(SphericalOcean, mSphericalOceanContext, SphericalOceanComponent, T_SphericalOcean)
#undef UpdataTODComponent

			if (mSceneManager)
			{
				Cloud* pCloud = mSceneManager->getCloud();
				if (pCloud == nullptr)
					return;
				if (mCloudState[2] == Cloud_No_Out)
				{
					pCloud->setCloudFadingOut(eCloudType_Sphere, false);
					CloudSphereComponent* cloudSphere_ = static_cast<CloudSphereComponent*>(mFinalComVec[T_CloudSphere]);
					updateContext(mCloudSphereContext, cloudSphere_);
					if (mFinalCloudSphereHandle)
						_getCloudSphereData(*mFinalCloudSphereHandle, pCloud);
					pCloud->setCloudOpacity(eCloudType_Sphere, 0.f);
				}
				if (mCloudState[0] == Cloud_No_Out)
				{
					pCloud->setCloudFadingOut(eCloudType_Mesh_Up, false);
					pCloud->setCloudFadingOut(eCloudType_Mesh_Mid, false);
					pCloud->setCloudFadingOut(eCloudType_Mesh_Down, false);
					CloudMeshComponent* cloudMesh_ = static_cast<CloudMeshComponent*>(mFinalComVec[T_CloudMesh]);
					updateContext(mCloudMeshContext, cloudMesh_);
					if (mFinalCloudMeshHandle)
						_getCloudMeshData(*mFinalCloudMeshHandle,pCloud);
					pCloud->setCloudOpacity(eCloudType_Mesh_Up, 0.f);
					pCloud->setCloudOpacity(eCloudType_Mesh_Mid, 0.f);
					pCloud->setCloudOpacity(eCloudType_Mesh_Down, 0.f);
				}
				if (mCloudState[1] == Cloud_No_Out)
				{
					CloudPlaneComponent* cloudPlane_ = static_cast<CloudPlaneComponent*>(mFinalComVec[T_CloudPlane]);
					updateContext(mCloudPlaneContext, cloudPlane_);
					if (mFinalCloudPlaneHandle)
						_getCloudPlaneData(*mFinalCloudPlaneHandle,pCloud);
					pCloud->setCloudOpacity(eCloudType_Plane, 0.f);
				}

				if (mCloudState[2] == Cloud_Pre)
					pCloud->getCloudSphere()->setVisible(mCloudSphereHandle->mCloudSphereVisible);
				if (mCloudState[0] == Cloud_Pre)
				{
					pCloud->getCloudMeshUp()->setVisible(mCloudMeshHandle->mCloudRealVisible[0]);
					pCloud->getCloudMeshMid()->setVisible(mCloudMeshHandle->mCloudRealVisible[1]);
					pCloud->getCloudMeshDown()->setVisible(mCloudMeshHandle->mCloudRealVisible[2]);
				}
				if (mCloudState[1] == Cloud_Pre)
					pCloud->getCloudPlane()->setVisible(mCloudPlaneHandle->mCloudPlaneVisible);
			}

			// 初始化不走过渡逻辑的Tod组件
			_setTodComAttributeInFirstTrans();

			mIsEffectiveModify = false;
		}
	}

	void TODComponentManager::_getTransFinalState(bool isGetAll)
	{
		if (mFinalDirecData)
			_getSceneDirecLightData(*mFinalDirecData);
		if (mFinalFogData)
			_getFogAttribute(*mFinalFogData);
		if (mFinalSkyData)
			_getSkyAttribute(*mFinalSkyData);
		if (mFinalSunData)
			_getSunAttribute(*mFinalSunData);
		if (mFinalSphereFogData && isGetAll)
			_getSphereFogAttribute(*mFinalSphereFogData);

#define TOD_IF(Name)	\
		if(mFinal##Name##Handle)	\
			_get##Name##Attribute(*mFinal##Name##Handle);

		TOD_IF(EnvironmentLight)
		TOD_IF(GlobalLight)
		TOD_IF(HDR)
		TOD_IF(HDRAdaptive)
		TOD_IF(Ocean)
		TOD_IF(PointLight)
		TOD_IF(Stars)
		TOD_IF(UnderWaterCaustics)
		TOD_IF(WeatherCoverColor)
		TOD_IF(TODShadow)
		TOD_IF(SphericalOcean)
#undef	TOD_IF

		if (mSceneManager)
		{
			Cloud* pCloud = mSceneManager->getCloud();
			if (pCloud == nullptr)
				return;
			if (mFinalCloudSphereHandle)
				_getCloudSphereData(*mFinalCloudSphereHandle, pCloud);
			if (mFinalCloudMeshHandle)
				_getCloudMeshData(*mFinalCloudMeshHandle, pCloud,false);
			if (mFinalCloudPlaneHandle)
				_getCloudPlaneData(*mFinalCloudPlaneHandle, pCloud);
		}
	}
	void TODComponentManager::_setSphereFogAttribute(float percent)
	{
		RenderSystem* mRenderSystem = Root::instance()->getRenderSystem();
		if (!mRenderSystem)
			return;
		RenderStrategy* pStrategy = mRenderSystem->getCurRenderStrategy();
		if (!pStrategy)
			return;
		PostProcessChain* pProcessChain = pStrategy->getPostProcessChain();
		if (!pProcessChain)
			return;

		SphericalTerrainFog* mFog = static_cast<SphericalTerrainFog*> (pProcessChain->getPostProcess(PostProcess::Type::SphericalTerrainFog));

		if (!mFog)
			return;

		SphereFogComHandle sphereFogHandle;

		if (mTodComIsChange[T_SphereFog])
		{
			SphereFogComHandle currSphereFogHandle;

			mFog->getTodFogParams(currSphereFogHandle.mHorizonColor, currSphereFogHandle.mFogColor, currSphereFogHandle.mFogExpTerm, currSphereFogHandle.mFogExpScale,
				currSphereFogHandle.mFogStart, currSphereFogHandle.mFogEnd, currSphereFogHandle.mColorChangeGradient, currSphereFogHandle.mFogMaxAlpha, currSphereFogHandle.mFogSkyParam);
			sphereFogHandle.mFogColor = comlerp<ColorValue>(mSphereFogData->mFogColor, currSphereFogHandle.mFogColor, percent);
			sphereFogHandle.mFogExpTerm = comlerp<float>(mSphereFogData->mFogExpTerm, currSphereFogHandle.mFogExpTerm, percent);
			sphereFogHandle.mFogExpScale = comlerp<float>(mSphereFogData->mFogExpScale, currSphereFogHandle.mFogExpScale, percent);
			sphereFogHandle.mFogMaxAlpha = comlerp<float>(mSphereFogData->mFogMaxAlpha, currSphereFogHandle.mFogMaxAlpha, percent);
			sphereFogHandle.mHorizonColor = comlerp<ColorValue>(mSphereFogData->mHorizonColor, currSphereFogHandle.mHorizonColor, percent);
			sphereFogHandle.mColorChangeGradient = comlerp<float>(mSphereFogData->mColorChangeGradient, currSphereFogHandle.mColorChangeGradient, percent);
			sphereFogHandle.mFogStart = comlerp<float>(mSphereFogData->mFogStart, currSphereFogHandle.mFogStart, percent);
			sphereFogHandle.mFogEnd = comlerp<float>(mSphereFogData->mFogEnd, currSphereFogHandle.mFogEnd, percent);
			sphereFogHandle.mFogSkyParam = comlerp<Vector4>(mSphereFogData->mFogSkyParam, currSphereFogHandle.mFogSkyParam, percent);
		}
		else if (mFinalSphereFogData)
		{
			sphereFogHandle.mFogColor = comlerp<ColorValue>(mSphereFogData->mFogColor, mFinalSphereFogData->mFogColor, percent);
			sphereFogHandle.mFogExpTerm = comlerp<float>(mSphereFogData->mFogExpTerm, mFinalSphereFogData->mFogExpTerm, percent);
			sphereFogHandle.mFogExpScale = comlerp<float>(mSphereFogData->mFogExpScale, mFinalSphereFogData->mFogExpScale, percent);
			sphereFogHandle.mFogMaxAlpha = comlerp<float>(mSphereFogData->mFogMaxAlpha, mFinalSphereFogData->mFogMaxAlpha, percent);
			sphereFogHandle.mHorizonColor = comlerp<ColorValue>(mSphereFogData->mHorizonColor, mFinalSphereFogData->mHorizonColor, percent);
			sphereFogHandle.mColorChangeGradient = comlerp<float>(mSphereFogData->mColorChangeGradient, mFinalSphereFogData->mColorChangeGradient, percent);
			sphereFogHandle.mFogSkyParam = comlerp<Vector4>(mSphereFogData->mFogSkyParam, mFinalSphereFogData->mFogSkyParam, percent);
			sphereFogHandle.mFogStart = comlerp<float>(mSphereFogData->mFogStart, mFinalSphereFogData->mFogStart, percent);
			sphereFogHandle.mFogEnd = comlerp<float>(mSphereFogData->mFogEnd, mFinalSphereFogData->mFogEnd, percent);
		}

		mFog->setTodFogParams(sphereFogHandle.mHorizonColor, sphereFogHandle.mFogColor, sphereFogHandle.mFogExpTerm, sphereFogHandle.mFogExpScale,
			               sphereFogHandle.mFogStart, sphereFogHandle.mFogEnd, sphereFogHandle.mColorChangeGradient, sphereFogHandle.mFogMaxAlpha, sphereFogHandle.mFogSkyParam);
	}
	void TODComponentManager::_getSphereFogAttribute(SphereFogComHandle& pData)
	{
		RenderSystem*  mRenderSystem = Root::instance()->getRenderSystem();
		if (!mRenderSystem)
			return;
		RenderStrategy* pStrategy = mRenderSystem->getCurRenderStrategy();
		if (!pStrategy)
			return;
		PostProcessChain* pProcessChain = pStrategy->getPostProcessChain();
		if (!pProcessChain)
			return;
		SphericalTerrainFog*  mFog = static_cast<SphericalTerrainFog*> (pProcessChain->getPostProcess(PostProcess::Type::SphericalTerrainFog));
		if (!mFog)
			return;

		mFog->getTodFogParams(pData.mHorizonColor, pData.mFogColor,
			pData.mFogExpTerm, pData.mFogExpScale, pData.mFogStart, pData.mFogEnd, pData.mColorChangeGradient, pData.mFogMaxAlpha, pData.mFogSkyParam);
	}
	void TODComponentManager::_setSphericalOceanAttribute(float percent)
	{
		SphericalOcean* sOcean = getCurrentSphericalOcean();
		if (!sOcean) return;

		SphericalOceanContext sphericalOceanHandle;

		if (mTodComIsChange[T_SphericalOcean])
		{
			SphericalOceanContext currSphericalOceanHandle;
			_getSphericalOceanAttribute(currSphericalOceanHandle);

			sphericalOceanHandle.mUnderWaterColor =
				comlerp<ColorValue>(mSphericalOceanHandle->mUnderWaterColor, currSphericalOceanHandle.mUnderWaterColor, percent);
			sphericalOceanHandle.mUnderWaterFogColor =
				comlerp<ColorValue>(mSphericalOceanHandle->mUnderWaterFogColor, currSphericalOceanHandle.mUnderWaterFogColor, percent);
			sphericalOceanHandle.mWaterDeepColor =
				comlerp<ColorValue>(mSphericalOceanHandle->mWaterDeepColor, currSphericalOceanHandle.mWaterDeepColor, percent);
			sphericalOceanHandle.mWaterFoamColor =
				comlerp<ColorValue>(mSphericalOceanHandle->mWaterFoamColor, currSphericalOceanHandle.mWaterFoamColor, percent);
			sphericalOceanHandle.mWaterReflectColor =
				comlerp<ColorValue>(mSphericalOceanHandle->mWaterReflectColor, currSphericalOceanHandle.mWaterReflectColor, percent);
			sphericalOceanHandle.mWaterShallowColor =
				comlerp<ColorValue>(mSphericalOceanHandle->mWaterShallowColor, currSphericalOceanHandle.mWaterShallowColor, percent);

		}
		else if (mFinalSphereFogData)
		{
			sphericalOceanHandle.mUnderWaterColor =
				comlerp<ColorValue>(mSphericalOceanHandle->mUnderWaterColor, mFinalSphericalOceanHandle->mUnderWaterColor, percent);
			sphericalOceanHandle.mUnderWaterFogColor =
				comlerp<ColorValue>(mSphericalOceanHandle->mUnderWaterFogColor, mFinalSphericalOceanHandle->mUnderWaterFogColor, percent);
			sphericalOceanHandle.mWaterDeepColor =
				comlerp<ColorValue>(mSphericalOceanHandle->mWaterDeepColor, mFinalSphericalOceanHandle->mWaterDeepColor, percent);
			sphericalOceanHandle.mWaterFoamColor =
				comlerp<ColorValue>(mSphericalOceanHandle->mWaterFoamColor, mFinalSphericalOceanHandle->mWaterFoamColor, percent);
			sphericalOceanHandle.mWaterReflectColor =
				comlerp<ColorValue>(mSphericalOceanHandle->mWaterReflectColor, mFinalSphericalOceanHandle->mWaterReflectColor, percent);
			sphericalOceanHandle.mWaterShallowColor =
				comlerp<ColorValue>(mSphericalOceanHandle->mWaterShallowColor, mFinalSphericalOceanHandle->mWaterShallowColor, percent);
		}

		SphereOceanShadeSettings shadeSettings;
		sOcean->getShadeSetting(shadeSettings);

		shadeSettings.m_underWaterColor = sphericalOceanHandle.mUnderWaterColor;
		// TODO
		shadeSettings.deepColor = sphericalOceanHandle.mWaterDeepColor;
		shadeSettings.m_foamColor = sphericalOceanHandle.mWaterFoamColor;
		shadeSettings.m_reflectColor = sphericalOceanHandle.mWaterReflectColor;
		shadeSettings.shallowColor = sphericalOceanHandle.mWaterShallowColor;

		sOcean->setShadeSetting(shadeSettings);
	}
	void TODComponentManager::_getSphericalOceanAttribute(SphericalOceanContext& pData)
	{
		SphericalOcean* sOcean = getCurrentSphericalOcean();
		if (!sOcean) return;

		SphereOceanShadeSettings shadeSettings;
		sOcean->getShadeSetting(shadeSettings);

		pData.mUnderWaterColor = shadeSettings.m_underWaterColor;
		//TODO
		pData.mWaterDeepColor = shadeSettings.deepColor;	
		pData.mWaterFoamColor = shadeSettings.m_foamColor;	
		pData.mWaterReflectColor = shadeSettings.m_reflectColor;
		pData.mWaterShallowColor = shadeSettings.shallowColor;
	}
	void TODComponentManager::_setSkyAttribute(float percent)
	{
		if (mSceneManager == nullptr)
			return;

		Sky* sky_ = mSceneManager->getSky();
		if (sky_ == nullptr || !mSkyData)
			return;

		if (mTodComIsChange[T_Sky])
		{
			sky_->setSkyBottomColour(comlerp<ColorValue>(mSkyData->mBottomColor, sky_->getSkyBottomColour(), percent));
			sky_->setSkyTopColour(comlerp<ColorValue>(mSkyData->mTopColor, sky_->getSkyTopColour(), percent));
			sky_->setSkyFogHeight(comlerp<float>(mSkyData->mFogHeight, sky_->getSkyFogHeight(), percent));
			sky_->setSkyColourRatio(comlerp<float>(mSkyData->mColourRatio, sky_->getSkyColourRatio(), percent));
		}
		else if(mFinalSkyData) 
		{
			sky_->setSkyBottomColour(comlerp<ColorValue>(mSkyData->mBottomColor,mFinalSkyData->mBottomColor, percent));
			sky_->setSkyTopColour(comlerp<ColorValue>(mSkyData->mTopColor, mFinalSkyData->mTopColor, percent));
			sky_->setSkyFogHeight(comlerp<float>(mSkyData->mFogHeight, mFinalSkyData->mFogHeight, percent));
			sky_->setSkyColourRatio(comlerp<float>(mSkyData->mColourRatio, mFinalSkyData->mColourRatio, percent));
		}

	}
	void TODComponentManager::_setFogAttribute(float percent)
	{

		if (mSceneManager == nullptr || !mFogData)
			return;

		if (mTodComIsChange[T_Fog])
		{
			FogHandle  fogHandle_;
			mSceneManager->getFogData(fogHandle_.mFogMode, fogHandle_.mFogColor,
				fogHandle_.mDensity, fogHandle_.mRampStart, fogHandle_.mRampEnd, fogHandle_.mTopHeight);
			for (int i = 0; i < 8; ++i)
			{
				fogHandle_.mFogColor[i] = comlerp<ColorValue>(mFogData->mFogColor[i], fogHandle_.mFogColor[i], percent);
				fogHandle_.mDensity[i] = comlerp<float>(mFogData->mDensity[i], fogHandle_.mDensity[i], percent);
				fogHandle_.mRampStart[i] = comlerp<float>(mFogData->mRampStart[i], fogHandle_.mRampStart[i], percent);
				fogHandle_.mRampEnd[i] = comlerp<float>(mFogData->mRampEnd[i], fogHandle_.mRampEnd[i], percent);
				fogHandle_.mTopHeight[i] = comlerp<float>(mFogData->mTopHeight[i], fogHandle_.mTopHeight[i], percent);
			}
			mSceneManager->setFog(fogHandle_.mFogMode, fogHandle_.mFogColor,
				fogHandle_.mDensity, fogHandle_.mRampStart, fogHandle_.mRampEnd, fogHandle_.mTopHeight);
		}
		else if(mFinalFogData)
		{
			FogHandle  fogHandle_;
			for (int i = 0; i < 8; ++i)
			{
				fogHandle_.mFogColor[i] = comlerp<ColorValue>(mFogData->mFogColor[i], mFinalFogData->mFogColor[i], percent);
				fogHandle_.mDensity[i] = comlerp<float>(mFogData->mDensity[i], mFinalFogData->mDensity[i], percent);
				fogHandle_.mRampStart[i] = comlerp<float>(mFogData->mRampStart[i], mFinalFogData->mRampStart[i], percent);
				fogHandle_.mRampEnd[i] = comlerp<float>(mFogData->mRampEnd[i], mFinalFogData->mRampEnd[i], percent);
				fogHandle_.mTopHeight[i] = comlerp<float>(mFogData->mTopHeight[i], mFinalFogData->mTopHeight[i], percent);
			}
			fogHandle_.mFogMode = mFinalFogData->mFogMode;
			mSceneManager->setFog(fogHandle_.mFogMode, fogHandle_.mFogColor,
				fogHandle_.mDensity, fogHandle_.mRampStart, fogHandle_.mRampEnd, fogHandle_.mTopHeight);
		}

		if (TODSystem::instance()->showLog(LogType::LT_Fog) && GetSceneManager()->getName() == g_MainTodComponentName)
		{
			if (mFinalFogData && !mTodComIsChange[T_Fog])
			{
				StringStream ss;
				ss << mFogData->mFogColor[0].r << " ----> " << mFinalFogData->mFogColor[0].r << " percent : " << percent;
				LogManager::instance()->logMessage("[Fog] Change Data : " + ss.str());
			}
		}
	}
	void TODComponentManager::_setDirecLightAttribute(float percent)
	{
		if (mSceneManager == nullptr || !mDirecData)
			return;

		if (mTodComIsChange[T_DirecLight])
		{
			DirecLightHandle direcHandle_;
			_getSceneDirecLightData(direcHandle_);
			mSceneManager->setPBRMainLightColor(comlerp<ColorValue>(mDirecData->mPBRMainLightColor, direcHandle_.mPBRMainLightColor, percent));
			mSceneManager->setPBRSkyLightColour(comlerp<ColorValue>(mDirecData->mPBRSkyLightColor, direcHandle_.mPBRSkyLightColor, percent));
			mSceneManager->setPBRGroundLightColour(comlerp<ColorValue>(mDirecData->mPBRGroundLightColor, direcHandle_.mPBRGroundLightColor, percent));
			mSceneManager->setRoleMainLightColor(comlerp<ColorValue>(mDirecData->mRoleMainLightColor, direcHandle_.mRoleMainLightColor, percent));
			mSceneManager->setRoleSkyLightColour(comlerp<ColorValue>(mDirecData->mRoleSkyLightColor, direcHandle_.mRoleSkyLightColor, percent));
			mSceneManager->setRoleGroundLightColour(comlerp<ColorValue>(mDirecData->mRoleGroundLightColor, direcHandle_.mRoleGroundLightColor, percent));
			mSceneManager->setMainLightColor(comlerp<ColorValue>(mDirecData->mMainLightColor, direcHandle_.mMainLightColor, percent));
			mSceneManager->setSkyLightColour(comlerp<ColorValue>(mDirecData->mMainSkyLightColor, direcHandle_.mMainSkyLightColor, percent));
			mSceneManager->setGroundLightColour(comlerp<ColorValue>(mDirecData->mMainGroundLightColor, direcHandle_.mMainGroundLightColor, percent));
			mSceneManager->setMainLightShadowDirection(comlerp<Vector3>(mDirecData->mMainLightShadowDirection, direcHandle_.mMainLightShadowDirection, percent));
			mSceneManager->setMainLightDirection(comlerp<Vector3>(mDirecData->mMainLightDirection, direcHandle_.mMainLightDirection, percent));
			mSceneManager->setPBRAmbientBlendThreshold(comlerp<float>(mDirecData->mPBRAmbientBlendThreshold, direcHandle_.mPBRAmbientBlendThreshold, percent));
			mSceneManager->setRoleAmbientBlendThreshold(comlerp<float>(mDirecData->mRoleAmbientBlendThreshold, direcHandle_.mRoleAmbientBlendThreshold, percent));

			std::array<float, 3> percent_;
			percent_[0] = (comlerp(mDirecData->m_ViewLightPercent, direcHandle_.m_ViewLightPercent, percent));
			percent_[1] = (comlerp(mDirecData->m_PBRViewLightPercent, direcHandle_.m_PBRViewLightPercent, percent));
			percent_[2] = (comlerp(mDirecData->m_RoleViewLightPercent, direcHandle_.m_RoleViewLightPercent, percent));
			mSceneManager->setViewLightPercent(percent_);

			if (mSceneManager->getSun() != nullptr && mDirecData && mFinalDirecData)
				mSceneManager->getSun()->setSunDirection(comlerp<Vector3>(mDirecData->mSunDirection, direcHandle_.mSunDirection, percent));
		}
		else if(mFinalDirecData)
		{
			mSceneManager->setPBRMainLightColor(comlerp<ColorValue>(mDirecData->mPBRMainLightColor, mFinalDirecData->mPBRMainLightColor, percent));
			mSceneManager->setPBRSkyLightColour(comlerp<ColorValue>(mDirecData->mPBRSkyLightColor, mFinalDirecData->mPBRSkyLightColor, percent));
			mSceneManager->setPBRGroundLightColour(comlerp<ColorValue>(mDirecData->mPBRGroundLightColor, mFinalDirecData->mPBRGroundLightColor, percent));
			mSceneManager->setRoleMainLightColor(comlerp<ColorValue>(mDirecData->mRoleMainLightColor, mFinalDirecData->mRoleMainLightColor, percent));
			mSceneManager->setRoleSkyLightColour(comlerp<ColorValue>(mDirecData->mRoleSkyLightColor, mFinalDirecData->mRoleSkyLightColor, percent));
			mSceneManager->setRoleGroundLightColour(comlerp<ColorValue>(mDirecData->mRoleGroundLightColor, mFinalDirecData->mRoleGroundLightColor, percent));
			mSceneManager->setMainLightColor(comlerp<ColorValue>(mDirecData->mMainLightColor, mFinalDirecData->mMainLightColor, percent));
			mSceneManager->setSkyLightColour(comlerp<ColorValue>(mDirecData->mMainSkyLightColor, mFinalDirecData->mMainSkyLightColor, percent));
			mSceneManager->setGroundLightColour(comlerp<ColorValue>(mDirecData->mMainGroundLightColor, mFinalDirecData->mMainGroundLightColor, percent));
			mSceneManager->setMainLightShadowDirection(comlerp<Vector3>(mDirecData->mMainLightShadowDirection, mFinalDirecData->mMainLightShadowDirection, percent));
			mSceneManager->setMainLightDirection(comlerp<Vector3>(mDirecData->mMainLightDirection, mFinalDirecData->mMainLightDirection, percent));
			mSceneManager->setPBRAmbientBlendThreshold(comlerp<float>(mDirecData->mPBRAmbientBlendThreshold, mFinalDirecData->mPBRAmbientBlendThreshold, percent));
			mSceneManager->setRoleAmbientBlendThreshold(comlerp<float>(mDirecData->mRoleAmbientBlendThreshold, mFinalDirecData->mRoleAmbientBlendThreshold, percent));

			std::array<float, 3> percent_;
			percent_[0] = (comlerp(mDirecData->m_ViewLightPercent, mFinalDirecData->m_ViewLightPercent, percent));
			percent_[1] = (comlerp(mDirecData->m_PBRViewLightPercent, mFinalDirecData->m_PBRViewLightPercent, percent));
			percent_[2] = (comlerp(mDirecData->m_RoleViewLightPercent, mFinalDirecData->m_RoleViewLightPercent, percent));
			mSceneManager->setViewLightPercent(percent_);

			if (mSceneManager->getSun() != nullptr && mDirecData && mFinalDirecData)
				mSceneManager->getSun()->setSunDirection(comlerp<Vector3>(mDirecData->mSunDirection, mFinalDirecData->mSunDirection, percent));
		}
	}
	void TODComponentManager::_setSunAttribute(float percent)
	{
		if (mSceneManager == nullptr)
			return;
		Sun* sun_ = mSceneManager->getSun();
		if (sun_ == nullptr && !mSunData)
			return;

		if (mTodComIsChange[T_Sun])
		{
			mSceneManager->setSunColor(comlerp<ColorValue>(mSunData->mSunColor, mSceneManager->getSunColor(), percent));
			sun_->setEmissionColour(comlerp<ColorValue>(mSunData->mEmmittingColor, sun_->getEmissionColour(), percent));
			sun_->setSunInnerRadius(comlerp<float>(mSunData->mInRadius, sun_->getSunInnerRadius(), percent));
			sun_->setSunOuterRadius(comlerp<float>(mSunData->mOutRadius, sun_->getSunOuterRadius(), percent));
			sun_->setSunOpacity(comlerp<float>(mSunData->mOpacity, sun_->getSunOpacity(), percent));
		}
		else if(mFinalSunData)
		{
			const ColorValue& color_ = mFinalSunData->mSunColor;
			mSceneManager->setSunColor(comlerp<ColorValue>(mSunData->mSunColor, color_, percent));
			sun_->setEmissionColour(comlerp<ColorValue>(mSunData->mEmmittingColor, mFinalSunData->mEmmittingColor, percent));
			sun_->setSunInnerRadius(comlerp<float>(mSunData->mInRadius, mFinalSunData->mInRadius, percent));
			sun_->setSunOuterRadius(comlerp<float>(mSunData->mOutRadius, mFinalSunData->mOutRadius, percent));
			sun_->setSunOpacity(comlerp<float>(mSunData->mOpacity, mFinalSunData->mOpacity, percent));
		}
	}
	void TODComponentManager::_setDriftAttribute(float percent)
	{
	}

	void TODComponentManager::_setEnvironmentLightAttribute(float percent)
	{
		if (mSceneManager == nullptr)
			return;

		EnvironmentLight* pEnvirLight = mSceneManager->getEnvironmentLight();
		if (!pEnvirLight || !mEnvironmentLightHandle)
			return;

		if (mTodComIsChange[T_EnvironmentLight])
		{
			float lerp = comlerp<float>(mEnvironmentLightHandle->mEnvironmentExplosure, pEnvirLight->getEnvironmentExplosure(), percent);
			pEnvirLight->setGlobalEnvironmentExplosure(lerp, EnvironmentLight::DataFromComponent);
		}
		else if(mFinalEnvironmentLightHandle)
		{
			float lerp = comlerp<float>(mEnvironmentLightHandle->mEnvironmentExplosure, mFinalEnvironmentLightHandle->mEnvironmentExplosure, percent);
			pEnvirLight->setGlobalEnvironmentExplosure(lerp, EnvironmentLight::DataFromComponent);
		}
	}

	void TODComponentManager::_getEnvironmentLightAttribute(EnvironmentLightHandle& pData)
	{
		if (mSceneManager == nullptr)
			return;
		EnvironmentLight* pEnvirLight = mSceneManager->getEnvironmentLight();
		if (pEnvirLight)
		{
			pData.mEnvironmentExplosure = pEnvirLight->getEnvironmentExplosure();
		}
	}

	void TODComponentManager::_setGlobalLightAttribute(float percent)
	{
		if (mSceneManager == nullptr || !mGlobalLightHandle)
			return;

		if (mTodComIsChange[T_GlobalLight])
		{
			GlobalLightHandle globalHandle;
			_getGlobalLightAttribute(globalHandle);
			mSceneManager->setGlobalSpecularRatio(comlerp<float>(mGlobalLightHandle->mGlobalSpecularRatio, globalHandle.mGlobalSpecularRatio, percent));
			mSceneManager->setGlobalReflectionRatio(comlerp<float>(mGlobalLightHandle->mGlobalReflectRatio, globalHandle.mGlobalReflectRatio, percent));
		}
		else if (mFinalGlobalLightHandle)
		{
			mSceneManager->setGlobalSpecularRatio(comlerp<float>(mGlobalLightHandle->mGlobalSpecularRatio, mFinalGlobalLightHandle->mGlobalSpecularRatio, percent));
			mSceneManager->setGlobalReflectionRatio(comlerp<float>(mGlobalLightHandle->mGlobalReflectRatio, mFinalGlobalLightHandle->mGlobalReflectRatio, percent));
		}
	}

	void TODComponentManager::_getGlobalLightAttribute(GlobalLightHandle& pData)
	{
		if (mSceneManager == nullptr)
			return;

		pData.mGlobalReflectRatio = mSceneManager->getGlobalReflectionRatio();
		pData.mGlobalSpecularRatio = mSceneManager->getGlobalSpecularRatio();
	}

	void TODComponentManager::_setHDRAttribute(float percent)
	{
		HDR* pHDR = getHDR();

		if (!pHDR || !mHDRHandle) return;

		if (mTodComIsChange[T_HDR])
		{
			pHDR->setBrightParam0x(comlerp<float>(mHDRHandle->Bright.x, pHDR->getBrightParams().x, percent));
			pHDR->setToneMappingParam0x(comlerp<float>(mHDRHandle->EyeToneMapping.x, pHDR->getToneMappingParams().x, percent));

			std::vector<float> v = mHDRHandle->EyeToneMappingCurve;
			v[4] = comlerp<float>(v[4], pHDR->getToneMappingCurve()[4], percent);
			pHDR->setToneMappingCurve(v[0], v[1], v[2], v[3], v[4]);
		}
		else if (mFinalHDRHandle)
		{
			pHDR->setBrightParam0x(comlerp<float>(mHDRHandle->Bright.x, mFinalHDRHandle->Bright.x, percent));
			pHDR->setToneMappingParam0x(comlerp<float>(mHDRHandle->EyeToneMapping.x, mFinalHDRHandle->EyeToneMapping.x, percent));
			std::vector<float> v = mHDRHandle->EyeToneMappingCurve;
			v[4] = comlerp<float>(v[4], mFinalHDRHandle->EyeToneMappingCurve[4], percent);
			pHDR->setToneMappingCurve(v[0], v[1], v[2], v[3], v[4]);
		}
	}

	void TODComponentManager::_getHDRAttribute(HDRHandle& pData)
	{
		HDR* pHDR = getHDR();

		if (!pHDR) return;

		pData.Bright = pHDR->getBrightParams();
		pData.EyeToneMapping = pHDR->getToneMappingParams();
		pData.EyeToneMappingCurve = pHDR->getToneMappingCurve();
	}

	void TODComponentManager::_setHDRAdaptiveAttribute(float percent)
	{
		HDR* pHDR = getHDR();

		if (!pHDR || !mHDRAdaptiveHandle) return;

		if (mTodComIsChange[T_HDRAdaptive])
		{
			Vector4 eyeAdaptation = pHDR->getEyeAdaptationParams();
			pHDR->setEyeAdaptationParam0x(comlerp<float>(mHDRAdaptiveHandle->mEyeAdaptionX, eyeAdaptation.x, percent));
			pHDR->setEyeAdaptationParam0y(comlerp<float>(mHDRAdaptiveHandle->mEyeAdaptionY, eyeAdaptation.y, percent));
			pHDR->setEyeAdaptationParam0z(comlerp<float>(mHDRAdaptiveHandle->mEyeAdaptionZ, eyeAdaptation.z, percent));
			pHDR->setEyeAdaptationParam0w(comlerp<float>(mHDRAdaptiveHandle->mEyeAdaptionW, eyeAdaptation.w, percent));
		}
		else if (mFinalHDRAdaptiveHandle)
		{
			pHDR->setEyeAdaptationParam0x(comlerp<float>(mHDRAdaptiveHandle->mEyeAdaptionX, mFinalHDRAdaptiveHandle->mEyeAdaptionX, percent));
			pHDR->setEyeAdaptationParam0y(comlerp<float>(mHDRAdaptiveHandle->mEyeAdaptionY, mFinalHDRAdaptiveHandle->mEyeAdaptionY, percent));
			pHDR->setEyeAdaptationParam0z(comlerp<float>(mHDRAdaptiveHandle->mEyeAdaptionZ, mFinalHDRAdaptiveHandle->mEyeAdaptionZ, percent));
			pHDR->setEyeAdaptationParam0w(comlerp<float>(mHDRAdaptiveHandle->mEyeAdaptionW, mFinalHDRAdaptiveHandle->mEyeAdaptionW, percent));
		}
	}

	void TODComponentManager::_getHDRAdaptiveAttribute(HDRAdaptiveHandle& pData)
	{
		HDR* pHDR = getHDR();

		if (!pHDR) return;

		Vector4 eyeAdaptation = pHDR->getEyeAdaptationParams();
		pData.mEyeAdaptionX = eyeAdaptation.x;
		pData.mEyeAdaptionY = eyeAdaptation.y;
		pData.mEyeAdaptionZ = eyeAdaptation.z;
		pData.mEyeAdaptionW = eyeAdaptation.w;
	}

	void TODComponentManager::_setOceanAttribute(float percent)
	{
		if (mSceneManager == nullptr)
			return;

		Ocean* pOcean = mSceneManager->getOcean();
		if (!pOcean || !mOceanHandle) return;

		if (mTodComIsChange[T_Ocean])
		{
			mSceneManager->setWaterShallowColor(comlerp<ColorValue>(mOceanHandle->getWaterShallowColor(), mSceneManager->getWaterShallowColor(), percent));
			mSceneManager->setWaterDeepColor(comlerp<ColorValue>(mOceanHandle->getWaterDeepColor(), mSceneManager->getWaterDeepColor(), percent));
			mSceneManager->setWaterFoamColor(comlerp<ColorValue>(mOceanHandle->getWaterFoamColor(), mSceneManager->getWaterFoamColor(), percent));
			mSceneManager->setWaterReflectColor(comlerp<ColorValue>(mOceanHandle->getWaterReflectColor(), mSceneManager->getWaterReflectColor(), percent));
			pOcean->setUnderWaterFogColor(comlerp<ColorValue>(mOceanHandle->getUnderWaterFogColor(), pOcean->getUnderWaterFogColor(), percent));
			mSceneManager->setUnderWaterColor(comlerp<ColorValue>(mOceanHandle->getUnderWaterColor(), mSceneManager->getUnderWaterColorValue(), percent));
		}
		else if (mFinalOceanHandle)
		{
			mSceneManager->setWaterShallowColor(comlerp<ColorValue>(mOceanHandle->getWaterShallowColor(), mFinalOceanHandle->getWaterShallowColor(), percent));
			mSceneManager->setWaterDeepColor(comlerp<ColorValue>(mOceanHandle->getWaterDeepColor(), mFinalOceanHandle->getWaterDeepColor(), percent));
			mSceneManager->setWaterFoamColor(comlerp<ColorValue>(mOceanHandle->getWaterFoamColor(), mFinalOceanHandle->getWaterFoamColor(), percent));
			mSceneManager->setWaterReflectColor(comlerp<ColorValue>(mOceanHandle->getWaterReflectColor(), mFinalOceanHandle->getWaterReflectColor(), percent));
			pOcean->setUnderWaterFogColor(comlerp<ColorValue>(mOceanHandle->getUnderWaterFogColor(), mFinalOceanHandle->getUnderWaterFogColor(), percent));
			mSceneManager->setUnderWaterColor(comlerp<ColorValue>(mOceanHandle->getUnderWaterColor(), mFinalOceanHandle->getUnderWaterColor(), percent));
		}
	}

	void TODComponentManager::_getOceanAttribute(OceanComponentContext& pData)
	{
		if (mSceneManager == nullptr) return;

		Ocean* pOcean = mSceneManager->getOcean();
		if (!pOcean) return;

		pData.setWaterShallowColor(mSceneManager->getWaterShallowColor());
		pData.setWaterDeepColor(mSceneManager->getWaterDeepColor());
		pData.setWaterFoamColor(mSceneManager->getWaterFoamColor());
		pData.setWaterReflectColor(mSceneManager->getWaterReflectColor());
		pData.setUnderWaterFogColor(pOcean->getUnderWaterFogColor());
		pData.setUnderWaterColor(mSceneManager->getUnderWaterColorValue());
	}

	void TODComponentManager::_setPointLightAttribute(float percent)
	{
		if (!mSceneManager || !mPointLightHandle) return;

		if (mTodComIsChange[T_PointLight])
		{
			mSceneManager->setLightPower(comlerp<float>(mPointLightHandle->mLightPower, mSceneManager->getLightPower(), percent));
			mSceneManager->setRolePointLightPower(comlerp<float>(mPointLightHandle->mRolePointLightPower, mSceneManager->getRolePointLightPower(), percent));
			mSceneManager->setRolePointLightColor(comlerp<ColorValue>(mPointLightHandle->mRolePointLightColor, mSceneManager->getRolePointLightColor(), percent));
		}
		else if (mFinalPointLightHandle)
		{
			mSceneManager->setLightPower(comlerp<float>(mPointLightHandle->mLightPower, mFinalPointLightHandle->mLightPower, percent));
			mSceneManager->setRolePointLightPower(comlerp<float>(mPointLightHandle->mRolePointLightPower, mFinalPointLightHandle->mRolePointLightPower, percent));
			mSceneManager->setRolePointLightColor(comlerp<ColorValue>(mPointLightHandle->mRolePointLightColor, mFinalPointLightHandle->mRolePointLightColor, percent));
		}
	}

	void TODComponentManager::_getPointLightAttribute(pLightManagerHandle& pData)
	{
		if (!mSceneManager) return;

		pData.mLightPower = mSceneManager->getLightPower();
		pData.mRolePointLightPower = mSceneManager->getRolePointLightPower();
		pData.mRolePointLightColor = mSceneManager->getRolePointLightColor();
	}

	void TODComponentManager::_setStarsAttribute(float percent)
	{
		if (!mSceneManager) return;

		Stars* pStars = mSceneManager->getStars();

		if (!pStars || !mStarsHandle) return;

		if (mTodComIsChange[T_Stars])
		{
			pStars->setStarSize(comlerp<float>(mStarsHandle->mStarsSize, pStars->getStarSize(), percent));
			pStars->setStarIntensity(comlerp<float>(mStarsHandle->mStarsIntensity, pStars->getStarIntensity(), percent));
			pStars->setHDRPower(comlerp<float>(mStarsHandle->mStarsHDRIntensity, pStars->getHDRPower(), percent));
		}
		else if (mFinalStarsHandle)
		{
			pStars->setStarSize(comlerp<float>(mStarsHandle->mStarsSize, mFinalStarsHandle->mStarsSize, percent));
			pStars->setStarIntensity(comlerp<float>(mStarsHandle->mStarsIntensity, mFinalStarsHandle->mStarsIntensity, percent));
			pStars->setHDRPower(comlerp<float>(mStarsHandle->mStarsHDRIntensity, mFinalStarsHandle->mStarsHDRIntensity, percent));
		}
	}

	void TODComponentManager::_getStarsAttribute(StarsComponentHandle& pData)
	{
		if (!mSceneManager) return;

		Stars* pStars = mSceneManager->getStars();

		if (!pStars) return;

		pData.mStarsSize = pStars->getStarSize();
		pData.mStarsIntensity = pStars->getStarIntensity();
		pData.mStarsHDRIntensity = pStars->getHDRPower();
	}

	void TODComponentManager::_setUnderWaterCausticsAttribute(float percent)
	{
		if (!mSceneManager || !mUnderWaterCausticsHandle) return;

		if (mTodComIsChange[T_UnderWaterCaustics])
		{
			mSceneManager->setCausticColour(comlerp<ColorValue>(mUnderWaterCausticsHandle->Color, mSceneManager->getCausticColour(), percent));
		}
		else if (mFinalUnderWaterCausticsHandle)
		{
			mSceneManager->setCausticColour(comlerp<ColorValue>(mUnderWaterCausticsHandle->Color, mFinalUnderWaterCausticsHandle->Color, percent));
		}
	}

	void TODComponentManager::_getUnderWaterCausticsAttribute(CausticsHandle& pData)
	{
		if (!mSceneManager) return;
		pData.Color = mSceneManager->getCausticColour();
	}

	void TODComponentManager::_setWeatherCoverColorAttribute(float percent)
	{
		if (!mWeatherCoverColorHandle) return;

		WeatherManager* weather = nullptr;
		if (mSceneManager && mSceneManager->getWeatherManager())
			weather = mSceneManager->getWeatherManager();
		else
			weather = WeatherSystem::instance()->GetManager(mName);

		if (!weather) return;

		if (mTodComIsChange[T_WeatherCoverColor])
		{
			weather->setCoverColor(comlerp<ColorValue>(mWeatherCoverColorHandle->mCoverColor, weather->getCoverColor(), percent));
			weather->setCoverColorIntensity(weather->getCoverColorIntensity(),
				comlerp<float>(mWeatherCoverColorHandle->mCoverColorIntensity, weather->getCoverColorIntensity(), percent));
		}
		else if (mFinalWeatherCoverColorHandle)
		{
			weather->setCoverColor(comlerp<ColorValue>(mWeatherCoverColorHandle->mCoverColor, mFinalWeatherCoverColorHandle->mCoverColor, percent));
			weather->setCoverColorIntensity(mFinalWeatherCoverColorHandle->mCoverColorIntensity,
				comlerp<float>(mWeatherCoverColorHandle->mCoverColorIntensity, mFinalWeatherCoverColorHandle->mCoverColorIntensity, percent));
		}
	}

	void TODComponentManager::_getWeatherCoverColorAttribute(WeatherCoverColorComData& pData)
	{
		WeatherManager* weather = nullptr;
		if (mSceneManager && mSceneManager->getWeatherManager())
			weather = mSceneManager->getWeatherManager();
		else
			weather = WeatherSystem::instance()->GetManager(mName);

		if (!weather) return;

		pData.mCoverColor = weather->getCoverColor();
		pData.mCoverColorIntensity = weather->getCoverColorIntensity();
	}


	void TODComponentManager::_setTODShadowAttribute(float precent)
	{
		if (!mSceneManager || !mTODShadowHandle) return;

		if (mTodComIsChange[T_Shadow])
		{
			mSceneManager->setShadowWeight(comlerp<float>(mTODShadowHandle->mShadowWeight, mSceneManager->getShadowWeight(), precent));
		}
		else if (mFinalTODShadowHandle)
		{
			mSceneManager->setShadowWeight(comlerp<float>(mTODShadowHandle->mShadowWeight, mFinalTODShadowHandle->mShadowWeight, precent));
		}
	}

	void TODComponentManager::_getTODShadowAttribute(TodShadowDataHandle& pData)
	{
		if (!mSceneManager) return;

		pData.mShadowWeight = mSceneManager->getShadowWeight();
	}

	void TODComponentManager::_cloudSphereFadeIn(float percent, Cloud* pCloud)
	{
		if (pCloud == nullptr  || mCloudSphereHandle == nullptr)
			return;

		//pCloud->setCloudFadingOut(eCloudType_Sphere, true);
		float opacity;
		if (mTodComIsChange[T_CloudSphere])
		{
			opacity = comlerp<float>(mCloudSphereHandle->mOpacity, pCloud->getCloudOpacity(eCloudType_Sphere), percent);
		}
		else if(mFinalCloudSphereHandle)
		{
			opacity = comlerp(mCloudSphereHandle->mOpacity, mFinalCloudSphereHandle->mOpacity, percent);
			_setCloudSphereAttribute(mFinalCloudSphereHandle, pCloud);
		}

		if (TODSystem::instance()->showLog(LogType::LT_CloudSphere) && GetSceneManager()->getName() == g_MainTodComponentName)
		{
			stringstream ss;
			ss << "Sphere opacity : " << opacity
				<< " insert percent : " << percent;
			LogManager::instance()->logMessage("[ TODComponentManager ] ----- _cloudSphereFadeIn ----- [" + ss.str() + "]");
		}

		pCloud->setCloudOpacity(eCloudType_Sphere, opacity);
		if (percent >= 1.f - std::numeric_limits<float>::epsilon())
		{
			mCloudState[2] = Cloud_None;
		}
	}
	void TODComponentManager::_cloudSphereFadeOut(float percent, Cloud* pCloud)
	{
		
		if (pCloud == nullptr || mCloudSphereHandle == nullptr)
			return;
		
		float opacity1 = comlerp(mCloudSphereHandle->mOpacity, 0.f, percent);
		pCloud->setCloudFadingOut(eCloudType_Sphere, true);
		_setCloudSphereAttribute(mCloudSphereHandle,pCloud);
		pCloud->setCloudOpacity(eCloudType_Sphere, opacity1);

		if (TODSystem::instance()->showLog(LogType::LT_CloudSphere) && GetSceneManager()->getName() == g_MainTodComponentName)
		{
			stringstream ss;
			ss << "Sphere opacity : " << opacity1
				<< " insert percent : " << percent;
			LogManager::instance()->logMessage("[ TODComponentManager ] ----- _cloudSphereFadeOut ----- [" + ss.str() + "]");
		}

		if ( percent >= 1.f - std::numeric_limits<float>::epsilon())
		{
			pCloud->setCloudFadingOut(eCloudType_Sphere, false);
			CloudSphereComponent* cloudSphereCom_ = static_cast<CloudSphereComponent*>(mFinalComVec[T_CloudSphere]);
			pCloud->setCloudOpacity(eCloudType_Sphere, mCloudSphereHandle->mOpacity);

			mIsEffectiveModify = true;
			updateContext(mCloudSphereContext, cloudSphereCom_);
			mIsEffectiveModify = false;

			//修改过渡状态
			mCloudState[2] = Cloud_In;

			if (cloudSphereCom_)
			{
				std::string logData;
				logData.append("CloudSphereFadingout : ").append(cloudSphereCom_->getComNameToString());
				LogManager::getSingleton().logMessage(logData.data());
			}
			if (mFinalCloudSphereHandle)
			{
				_getCloudSphereData(*mFinalCloudSphereHandle, pCloud);
				mFinalCloudSphereHandle->mOpacity = pCloud->getCloudOpacity(eCloudType_Sphere);
			}
			pCloud->setCloudOpacity(eCloudType_Sphere, 0.f);
			mCloudSphereHandle->mOpacity = 0.f;
		}

	}
	void TODComponentManager::_setCloudSphereAttribute(CloudSphereComponentContext* sphereContext, Cloud* pCloud)
	{
		if (sphereContext && pCloud)
		{
			pCloud->setCloudColour(eCloudType_Sphere, sphereContext->mColour);
			pCloud->setCloudSunBackColour(eCloudType_Sphere, sphereContext->mBackColour);
			pCloud->setCloudUSpeed(eCloudType_Sphere, sphereContext->mSpeed);
			pCloud->setCloudEmendation(eCloudType_Sphere, sphereContext->mEmendation);
		}
	}
	void TODComponentManager::_cloudPlaneFadeIn(float percent, Cloud* pCloud)
	{
		if (pCloud == nullptr || mCloudPlaneHandle == nullptr)
			return;

		float opacity;
		if (mTodComIsChange[T_CloudPlane])
		{
			opacity = comlerp<float>(mCloudPlaneHandle->mOpacity, pCloud->getCloudOpacity(eCloudType_Plane), percent);
		}
		else if(mFinalCloudPlaneHandle)
		{
			opacity = comlerp(mCloudPlaneHandle->mOpacity, mFinalCloudPlaneHandle->mOpacity, percent);
			_setCloudPlaneAttribute(mFinalCloudPlaneHandle,pCloud);
		}
		if (TODSystem::instance()->showLog(LogType::LT_CloudPlane) && GetSceneManager()->getName() == g_MainTodComponentName)
		{
			stringstream ss;
			ss << "Plane opacity : " << opacity
				<< " insert percent : " << percent;
			LogManager::instance()->logMessage("[ TODComponentManager ] ----- _cloudPlaneFadeIn ----- [" + ss.str() + "]");
		}

		pCloud->setCloudOpacity(eCloudType_Plane, opacity);
		if (percent >= 1.f - std::numeric_limits<float>::epsilon())
		{
			mCloudState[1] = Cloud_None;
		}
	}
	void TODComponentManager::_cloudPlaneFadeOut(float percent,Cloud* pCloud)
	{
		if (pCloud == nullptr || mCloudPlaneHandle == nullptr)
			return;

		_setCloudPlaneAttribute(mCloudPlaneHandle, pCloud);
		float opacity2 = comlerp(mCloudPlaneHandle->mOpacity, 0.f, percent);
		pCloud->setCloudOpacity(eCloudType_Plane, opacity2);

		if (TODSystem::instance()->showLog(LogType::LT_CloudPlane) && GetSceneManager()->getName() == g_MainTodComponentName)
		{
			stringstream ss;
			ss << "Plane opacity : " << opacity2
				<< " insert percent : " << percent;
			LogManager::instance()->logMessage("[ TODComponentManager ] ----- _cloudPlaneFadeOut ----- [" + ss.str() + "]");
		}

		if (percent >= 1.f - std::numeric_limits<float>::epsilon())
		{
			pCloud->setCloudOpacity(eCloudType_Plane, mCloudPlaneHandle->mOpacity);
			CloudPlaneComponent* cloudPlaneCom_ = static_cast<CloudPlaneComponent*>(mFinalComVec[T_CloudPlane]);

			mIsEffectiveModify = true;
			updateContext(mCloudPlaneContext, cloudPlaneCom_);
			mIsEffectiveModify = false;

			if (cloudPlaneCom_)
			{
				std::string logData;
				logData.append("CloudPlaneFadingout : ").append(cloudPlaneCom_->getComNameToString());
				LogManager::getSingleton().logMessage(logData.data());
			}

			if (mFinalCloudPlaneHandle)
			{
				_getCloudPlaneData(*mFinalCloudPlaneHandle,pCloud);
				mFinalCloudPlaneHandle->mOpacity = pCloud->getCloudOpacity(eCloudType_Plane);
			}
			pCloud->setCloudOpacity(eCloudType_Plane,0.f);

			mCloudState[1] = Cloud_In;
			mCloudPlaneHandle->mOpacity = 0.f;
		}
		
	}
	void TODComponentManager::_setCloudPlaneAttribute(CloudPlaneComponentContext* planeContext,Cloud* pCloud)
	{
		if (planeContext && pCloud)
		{
			pCloud->setCloudColour(eCloudType_Plane, planeContext->mColour);
			pCloud->setCloudSunBackColour(eCloudType_Plane, planeContext->mBackColor);
			pCloud->setCloudEmendation(eCloudType_Plane, planeContext->mEmendation);
			pCloud->setCloudUSpeed(eCloudType_Plane, planeContext->mURollSpeed);
			pCloud->setCloudVSpeed(eCloudType_Plane, planeContext->mVRollSpeed);
			pCloud->setCloudUVTilling(eCloudType_Plane, planeContext->mUVTilling);
		}
	}
	void TODComponentManager::_setCloudMeshData(const CloudMeshComponentContext& rhs, float percent,Cloud* pCloud)
	{
		if (pCloud == nullptr)
			return;
		_setCloudMeshDataBytype(rhs, eCloudType_Mesh_Up,pCloud);
		_setCloudMeshDataBytype(rhs, eCloudType_Mesh_Mid,pCloud);
		_setCloudMeshDataBytype(rhs, eCloudType_Mesh_Down,pCloud);
	}
	void TODComponentManager::_setCloudMeshDataBytype(const CloudMeshComponentContext& pData, int type_,Cloud* pCloud)
	{
		if (pCloud == nullptr)
			return;
		CloudType type = static_cast<CloudType>(type_);

		int index = type - eCloudType_Mesh_Up;
	
		_setCloudMeshTemplateDataByType(pData,type_,pCloud);

		pCloud->setCloudVisible(type, pData.mCloudRealVisible[index]);
		pCloud->setCloudSunBackColour(type,pData.mBackColor[index]);
		pCloud->setCloudTextureName(type, pData.mTextureName[index]);
		pCloud->setCloudEmendation(type, pData.mEmendation[index]);
		pCloud->setCloudColour(type,pData.mColor[index]);

		pCloud->updateCloudMaterial(type);
	}
	void TODComponentManager::_setCloudMeshTemplateDataByType(const CloudMeshComponentContext& pData, int type_, Cloud* pCloud)
	{
		if (pCloud == nullptr)
			return;
		CloudType type = static_cast<CloudType>(type_);

		int index = type - eCloudType_Mesh_Up;


		/*std::string logData;
		logData.append("TodComponentManager : ").append(" type : ").append(std::to_string(type).append(" Height : x -- ")
			.append(std::to_string(pData.mTopHeight[index]))).append(" y-- ").append(std::to_string(pData.mBottomHeight[index]));
		LogManager::getSingleton().logMessage(logData.data());*/

		pCloud->setCloudUseOriginalUV(type, pData.mIsUseOriginalUV[index]);
		pCloud->setCloudUseSunBackColor(type, pData.mIsShowBackColor[index]);
		pCloud->setCloudUseDistort(type, pData.mIsUseDistort[index]);
		pCloud->setCloudMeshFile(type, pData.mMeshName[index]);
		pCloud->setCloudHeight(type, Vector2(pData.mBottomHeight[index], pData.mTopHeight[index]));
		pCloud->setCloudScale(type, pData.mCloudMeshScale[index]);
		pCloud->setCloudUSpeed(type, pData.mURollSpeed[index]);
		pCloud->setCloudVSpeed(type, pData.mVRollSpeed[index]);
		pCloud->setCloudUVTilling(type, pData.mUVTilling[index]);
		pCloud->setCloudDistortSpeed(type, pData.mDistortSpeed[index]);
		pCloud->setCloudDistortAmplitude(type, pData.mDistortAmplitude[index]);
		pCloud->setCloudDistortScale(type, pData.mDistortScale[index]);
		pCloud->setCloudRenderPriority(type, pData.mRenderPriority[index]);
	}
	void TODComponentManager::_getCloudMeshDataByType(CloudMeshComponentContext& pData, int type,Cloud* pCloud, bool isGetAllData)
	{
		if (pCloud == nullptr)
			return;

		CloudMesh* cloudMesh_ = nullptr;
		if (type == eCloudType_Mesh_Up)
			cloudMesh_ = pCloud->getCloudMeshUp();
		else if (type == eCloudType_Mesh_Mid)
			cloudMesh_ = pCloud->getCloudMeshMid();
		else if (type == eCloudType_Mesh_Down)
			cloudMesh_ = pCloud->getCloudMeshDown();
		if (cloudMesh_ == nullptr)
			return;
		int idx = type - eCloudType_Mesh_Up;

		if (isGetAllData)
		{
			pData.mIsUseOriginalUV[idx] = cloudMesh_->getUseOriginalUV();
			pData.mIsShowBackColor[idx] = cloudMesh_->getUseSunBackColor();
			pData.mIsUseDistort[idx] = cloudMesh_->getUseDistort();
			pData.mMeshName[idx] = cloudMesh_->getMeshFile();
			pData.mBottomHeight[idx] = cloudMesh_->getCloudHeight().x;
			pData.mTopHeight[idx] = cloudMesh_->getCloudHeight().y;
			pData.mCloudMeshScale[idx] = cloudMesh_->getScale();
			pData.mURollSpeed[idx] = cloudMesh_->getCloudUSpeed();
			pData.mVRollSpeed[idx] = cloudMesh_->getCloudVSpeed();
			pData.mUVTilling[idx] = cloudMesh_->getCloudUVTilling();
			pData.mDistortSpeed[idx] = cloudMesh_->getDistortSpeed();
			pData.mDistortAmplitude[idx] = cloudMesh_->getDistortAmplitude();
			pData.mDistortScale[idx] = cloudMesh_->getDistortScale();
			pData.mRenderPriority[idx] = cloudMesh_->getRenderPriority();

		}
		pData.mBackColor[idx] = cloudMesh_->getCloudSunBackColour();
		pData.mEmendation[idx] = cloudMesh_->getCloudEmendation();
		pData.mColor[idx] = cloudMesh_->getCloudColour();
		cloudMesh_->getCloudEffectTextureName(pData.mTextureName[idx]);
		pData.mOpacity[idx] = cloudMesh_->getCloudOpacity();
		if (cloudMesh_->getMeshFile().empty() || cloudMesh_->getVisible() == false)
			pData.mOpacity[idx] = 0.f;
		pData.mCloudRealVisible[idx] = cloudMesh_->getVisible();
	}
	void TODComponentManager::_cloudMeshFadeIn(float percent,Cloud* pCloud)
	{
		if (pCloud == nullptr || mCloudMeshHandle == nullptr)
			return;

		std::vector<float> opacity_;
		opacity_.reserve(3);
		opacity_.resize(3);

		
		if (mTodComIsChange[T_CloudMesh])
		{
			opacity_[0] = comlerp<float>(mCloudMeshHandle->mOpacity[0], pCloud->getCloudOpacity(eCloudType_Mesh_Up), percent);
			opacity_[1] = comlerp<float>(mCloudMeshHandle->mOpacity[1], pCloud->getCloudOpacity(eCloudType_Mesh_Mid), percent);
			opacity_[2] = comlerp<float>(mCloudMeshHandle->mOpacity[2], pCloud->getCloudOpacity(eCloudType_Mesh_Down), percent);
			if (mFinalCloudMeshHandle)
			{
                _setCloudMeshTemplateDataByType(*mFinalCloudMeshHandle, eCloudType_Mesh_Up, pCloud);
				_setCloudMeshTemplateDataByType(*mFinalCloudMeshHandle, eCloudType_Mesh_Mid, pCloud);
				_setCloudMeshTemplateDataByType(*mFinalCloudMeshHandle, eCloudType_Mesh_Down, pCloud);
			}
				
		}
		else if(mFinalCloudMeshHandle)
		{
			opacity_[0] = comlerp<float>(mCloudMeshHandle->mOpacity[0], mFinalCloudMeshHandle->mOpacity[0], percent);
			opacity_[1] = comlerp<float>(mCloudMeshHandle->mOpacity[1], mFinalCloudMeshHandle->mOpacity[1], percent);
			opacity_[2] = comlerp<float>(mCloudMeshHandle->mOpacity[2], mFinalCloudMeshHandle->mOpacity[2], percent);
			if (mFinalComVec[T_CloudMesh] != nullptr)
			{
				_setCloudMeshData(*mFinalCloudMeshHandle, percent,pCloud);
			}
			else
			{
				if (mCurrSceneArtData)
				{
					_setCloudSeaParamByType(pCloud, static_cast<int>(eCloudType_Mesh_Up), SceneArtData::eCloudSeaUp, mCurrSceneArtData);
					_setCloudSeaParamByType(pCloud, static_cast<int>(eCloudType_Mesh_Mid), SceneArtData::eCloudSeaMid, mCurrSceneArtData);
					_setCloudSeaParamByType(pCloud, static_cast<int>(eCloudType_Mesh_Down), SceneArtData::eCloudSeaDown, mCurrSceneArtData);
				}
			}
		}
		pCloud->setCloudOpacity(eCloudType_Mesh_Up, opacity_[0]);
		pCloud->setCloudOpacity(eCloudType_Mesh_Mid, opacity_[1]);
		pCloud->setCloudOpacity(eCloudType_Mesh_Down, opacity_[2]);

		if (TODSystem::instance()->showLog(LogType::LT_CloudMesh) &&  GetSceneManager()->getName() == g_MainTodComponentName)
		{
			stringstream ss;
			ss << pCloud->getCloudMeshUp() << "  Up opacity : " << opacity_[0] << "\n"
				<< pCloud->getCloudMeshMid() << "  Mid opacity : " << opacity_[1] << "\n"
				<< pCloud->getCloudMeshDown() << "  Down opacity : " << opacity_[2] << "\n"
				<< "percent : " << percent;
			LogManager::instance()->logMessage("[ TODComponentManager ] ----- _cloudMeshFadeIn ----- [" + ss.str() + "]");
		}



		if (percent >= 1.f - std::numeric_limits<float>::epsilon())
		{
			mCloudState[0] = Cloud_None;
			_setCloudTodTimelineState(true);
		}
	}
	void TODComponentManager::_cloudMeshFadeOut(float percent,Cloud* pCloud)
	{
		if (pCloud == nullptr || mCloudMeshHandle == nullptr)
			return;
		

		float opacity1 = comlerp<float>(mCloudMeshHandle->mOpacity[0], 0.f, percent);
		pCloud->setCloudFadingOut(eCloudType_Mesh_Up, true);
		pCloud->setCloudOpacity(eCloudType_Mesh_Up, opacity1);

		float opacity2 = comlerp<float>(mCloudMeshHandle->mOpacity[1], 0.f, percent);
		pCloud->setCloudFadingOut(eCloudType_Mesh_Mid, true);
		pCloud->setCloudOpacity(eCloudType_Mesh_Mid, opacity2);

		float opacity3 = comlerp<float>(mCloudMeshHandle->mOpacity[2], 0.f, percent);
		pCloud->setCloudFadingOut(eCloudType_Mesh_Down, true);
		pCloud->setCloudOpacity(eCloudType_Mesh_Down, opacity3);

		_setCloudMeshData(*mCloudMeshHandle, percent, pCloud);

		if (TODSystem::instance()->showLog(LogType::LT_CloudMesh) && GetSceneManager()->getName() == g_MainTodComponentName)
		{
			stringstream ss;

			ss  <<pCloud->getCloudMeshUp() << "  Up opacity : " << opacity1 << "\n"
				<<pCloud->getCloudMeshMid() <<"  Mid opacity : " << opacity2 << "\n"
				<<pCloud->getCloudMeshDown()<<"  Down opacity : " << opacity3 << "\n"
				<< "percent : " << percent;
			LogManager::instance()->logMessage("[ TODComponentManager ] ----- _cloudMeshFadeOut ----- [" + ss.str() + "]");
		}

		if (percent >= 1.f - std::numeric_limits<float>::epsilon())
		{
			pCloud->setCloudFadingOut(eCloudType_Mesh_Up, false);
			pCloud->setCloudFadingOut(eCloudType_Mesh_Mid, false);
			pCloud->setCloudFadingOut(eCloudType_Mesh_Down, false);

			pCloud->setCloudOpacity(eCloudType_Mesh_Up, mCloudMeshHandle->mOpacity[0]);
			pCloud->setCloudOpacity(eCloudType_Mesh_Mid, mCloudMeshHandle->mOpacity[1]);
			pCloud->setCloudOpacity(eCloudType_Mesh_Down, mCloudMeshHandle->mOpacity[2]);
				
			CloudMeshComponent*  cloudMeshCom_ = static_cast<CloudMeshComponent*>(mFinalComVec[T_CloudMesh]);

			mIsEffectiveModify = true;
			updateContext(mCloudMeshContext, cloudMeshCom_);
			mIsEffectiveModify = false;

			if (cloudMeshCom_)
			{
				std::string logData;
			    logData.append("CloudMeshFadingout : ").append(cloudMeshCom_->getComNameToString());
			    LogManager::getSingleton().logMessage(logData.data());
			}

			if (mFinalCloudMeshHandle)
			{
				_getCloudMeshData(*mFinalCloudMeshHandle,pCloud);
			
				mFinalCloudMeshHandle->mOpacity[0] = pCloud->getCloudOpacity(eCloudType_Mesh_Up);
				mFinalCloudMeshHandle->mOpacity[1] = pCloud->getCloudOpacity(eCloudType_Mesh_Mid);
				mFinalCloudMeshHandle->mOpacity[2] = pCloud->getCloudOpacity(eCloudType_Mesh_Down);
			}

			//过渡状态修改
			mCloudState[0] = Cloud_In;

			//透明度更改 
			pCloud->setCloudOpacity(eCloudType_Mesh_Up, 0.f);
			pCloud->setCloudOpacity(eCloudType_Mesh_Mid, 0.f);
			pCloud->setCloudOpacity(eCloudType_Mesh_Down, 0.f);
			mCloudMeshHandle->mOpacity[0] = 0.f;
			mCloudMeshHandle->mOpacity[1] = 0.f;
			mCloudMeshHandle->mOpacity[2] = 0.f;
		}
	
	}
	void TODComponentManager::_getSceneAttributeData()
	{
		if (mSceneManager == nullptr)
			return;
		Sky* sky_ = mSceneManager->getSky();
		if (sky_ && mSkyData)
		{
			mSkyData->mBottomColor = sky_->getSkyBottomColour();
			mSkyData->mTopColor = sky_->getSkyTopColour();
			mSkyData->mFogHeight = sky_->getSkyFogHeight();
			mSkyData->mColourRatio = sky_->getSkyColourRatio();
		}
		if (mFogData)
		{
			mSceneManager->getFogData(mFogData->mFogMode, mFogData->mFogColor,
				mFogData->mDensity, mFogData->mRampStart, mFogData->mRampEnd, mFogData->mTopHeight);
		}
		if(mDirecData)
		  _getSceneDirecLightData(*mDirecData);

		Sun* sun_ = mSceneManager->getSun();
		if (sun_ && mSunData)
		{
			mSunData->mSunColor = mSceneManager->getSunColor();
			mSunData->mEmmittingColor = sun_->getEmissionColour();
			mSunData->mInRadius = sun_->getSunInnerRadius();
			mSunData->mOutRadius = sun_->getSunOuterRadius();
			mSunData->mOpacity = sun_->getSunOpacity();
		}
	}
	void TODComponentManager::setTodComChangeState(TodComType type_)
	{
		if (mTodTransState[type_])
			mTodComIsChange[type_] = true;
	}
	void TODComponentManager::setTODGravityData(const Vector3& gravityDir_,  float gravityHeight_, const Vector3& gravityPos_)
	{
		if (mSceneManager == nullptr) 
			return;
		mSceneManager->setGravityDir(gravityDir_);
		mSceneManager->setGravityPos(gravityPos_);
		mSceneManager->setGravityHeight(gravityHeight_);

		setGravityData();
	}

	void TODComponentManager::setGravity(const Vector3& gravity_)
	{
		if (mSceneManager)
			mSceneManager->setGravityAccel(gravity_);
	}

	void TODComponentManager::setIsUseGravityDirByType(int type_, bool isUse_)
	{
		if (mSceneManager == nullptr)
			return;
		mSceneManager->setIsUseGravityDirByType(static_cast<TODGravityState>(type_), isUse_);
	}

	void TODComponentManager::setIsUseGravityPosByType(int type_, bool isUse_)
	{
		if (mSceneManager == nullptr)
			return;
		mSceneManager->setIsUseGravityPosByType(static_cast<TODGravityState>(type_), isUse_);
	}

	void TODComponentManager::setIsUseGravityHeightByType(int type_, bool isUse_)
	{
		if (mSceneManager == nullptr)
			return;
		mSceneManager->setIsUseGravityDirByType(static_cast<TODGravityState>(type_), isUse_);
	}
	void TODComponentManager::useAllGravityType()
	{
		if (mSceneManager == nullptr)
			return;
		mSceneManager->setIsUseGravityDir(true);
		for (int i = 0; i < Use_State_End; ++i)
		{
			mSceneManager->setIsUseGravityDirByType(static_cast<TODGravityState>(i), true);
			mSceneManager->setIsUseGravityPosByType(static_cast<TODGravityState>(i), true);
			mSceneManager->setIsUseGravityHeightByType(static_cast<TODGravityState>(i), true);
		}
	}
	void TODComponentManager::unUseAllGravityType()
	{
		if (mSceneManager == nullptr)
			return;

		if (mIsEffectiveModify)
		{
			mUnUseAllGravity = true;
			return;
		}

		mSceneManager->setIsUseGravityDir(false);
		for (int i = 0; i < Use_State_End; ++i)
		{
			mSceneManager->setIsUseGravityDirByType(static_cast<TODGravityState>(i), false);
			mSceneManager->setIsUseGravityPosByType(static_cast<TODGravityState>(i), false);
			mSceneManager->setIsUseGravityHeightByType(static_cast<TODGravityState>(i), false);
		}
	}
	void TODComponentManager::setGravityData()
	{
		if (mSceneManager == nullptr)
			return;
		Vector3 currentPos = mSceneManager->getGravityDir();
		Quaternion qu;
		qu = Quaternion::FromToRotation(Vector3::UNIT_Y, currentPos);
		
		Matrix3 matrix = matrix.IDENTITY;
		qu.ToRotationMatrix(matrix);
		Sky* sky = mSceneManager->getSky();
		if (sky)
		{
			if(mSceneManager->getIsUseGravityDirByType(Use_State_Sky))
				sky->setRotation(matrix);
			else
				sky->setRotation(Matrix3::IDENTITY);
		}
		Cloud* cloud = mSceneManager->getCloud();
		if (cloud)
		{
			auto cloudPlane = cloud->getCloudPlane();
			auto cloudMeshUp = cloud->getCloudMeshUp();
			auto cloudMeshMid = cloud->getCloudMeshMid();
			auto cloudMeshDown = cloud->getCloudMeshDown();
			auto cloudSphere = cloud->getCloudSphere();

			if (cloudPlane)
			{
				if (mSceneManager->getIsUseGravityDirByType(Use_State_CloudPlane))
					cloudPlane->setRotation(matrix);
				else
					cloudPlane->setRotation(Matrix3::IDENTITY);
			}

			if (cloudMeshUp)
			{
				if (mSceneManager->getIsUseGravityDirByType(Use_State_CloudMesh) && mSceneManager->getIsUseGravityPosByType(Use_State_CloudMesh))
					cloudMeshUp->setRotation(matrix);
				else
					cloudMeshUp->setRotation(Matrix3::IDENTITY);
			}

			if (cloudMeshMid)
			{
				if (mSceneManager->getIsUseGravityDirByType(Use_State_CloudMesh) && mSceneManager->getIsUseGravityPosByType(Use_State_CloudMesh))
					cloudMeshMid->setRotation(matrix);
				else
					cloudMeshMid->setRotation(Matrix3::IDENTITY);
			}

			if (cloudMeshDown)
			{
				if (mSceneManager->getIsUseGravityDirByType(Use_State_CloudMesh) && mSceneManager->getIsUseGravityPosByType(Use_State_CloudMesh))
					cloudMeshDown->setRotation(matrix);
				else
					cloudMeshDown->setRotation(Matrix3::IDENTITY);
			}

			if (cloudSphere)
			{
				if (mSceneManager->getIsUseGravityDirByType(Use_State_CloudSphere))
					cloudSphere->setRotation(matrix);
				else
					cloudSphere->setRotation(Matrix3::IDENTITY);
			}
		}
		

	}
	void TODComponentManager::setTransFormTimeFromClient(float time_)
	{
		mTransTimeByClient = time_;
		mTransTimeByClient = std::max(mTransTimeByClient, 0.002f);
		setTodTransTime(time_);
		setCloudTransTime(time_);
	}
	void TODComponentManager::setTodTransTime(float time_)
	{
		for (int i = T_DirecLight; i < T_End; ++i)
		{
			mTodTransTime[i] = time_;
		}
	}
	void TODComponentManager::setCloudTransTime(float time_)
	{
		for (int i = T_CloudMesh; i <= T_CloudSphere; ++i)
		{
			/*if (mCloudState[i - T_CloudMesh] == Cloud_No_Out)
				mTodTransTime[i] = time_;
			else*/
			mTodTransTime[i] = time_ / 2;
		}
	}
	void TODComponentManager::_getSkyAttribute(SkyComponentContext& mSkyData_)
	{
		if (mSceneManager == nullptr)
			return;
		Sky* sky_ = mSceneManager->getSky();
		if (sky_)
		{
			mSkyData_.mBottomColor = sky_->getSkyBottomColour();
			mSkyData_.mTopColor = sky_->getSkyTopColour();
			mSkyData_.mFogHeight = sky_->getSkyFogHeight();
			mSkyData_.mColourRatio = sky_->getSkyColourRatio();
		}
	}
	void TODComponentManager::_getFogAttribute(FogHandle& mFogData_)
	{
		if (mSceneManager == nullptr)
			return;
		mSceneManager->getFogData(mFogData_.mFogMode, mFogData_.mFogColor,
			mFogData_.mDensity, mFogData_.mRampStart, mFogData_.mRampEnd, mFogData_.mTopHeight);

		if (TODSystem::instance()->showLog(LogType::LT_Fog) && GetSceneManager()->getName() == g_MainTodComponentName)
		{
			String name = "Init FogData";
			if (&mFogData_ == mFinalFogData)
				name = "Final FogData";
			StringStream ss;
			ss << mFogData_.mFogColor[0].r;
			LogManager::instance()->logMessage("[Fog] Change Data : " + name + " " + ss.str());
		}
	}
	
	void TODComponentManager::_getSceneDirecLightData(DirecLightHandle& direcHandle_)
	{
		if (mSceneManager == nullptr)
			return;
		direcHandle_.mPBRMainLightColor = mSceneManager->getPBRMainLightColor();
		direcHandle_.mPBRSkyLightColor = mSceneManager->getPBRSkyLightColour();
		direcHandle_.mPBRGroundLightColor = mSceneManager->getPBRGroundLightColour();
		direcHandle_.mRoleMainLightColor = mSceneManager->getRoleMainLightColor();
		direcHandle_.mRoleSkyLightColor = mSceneManager->getRoleSkyLightColour();
		direcHandle_.mRoleGroundLightColor = mSceneManager->getRoleGroundLightColour();
		direcHandle_.mMainLightDirection = mSceneManager->getMainLightDirectionOrigin();
		direcHandle_.mMainLightShadowDirection = mSceneManager->getMainLightShadowDirection();
		direcHandle_.mMainLightColor = mSceneManager->getMainLightColor();
		direcHandle_.mMainSkyLightColor = mSceneManager->getSkyLightColour();
		direcHandle_.mMainGroundLightColor = mSceneManager->getGroundLightColour();
		direcHandle_.mPBRAmbientBlendThreshold = mSceneManager->getPBRAmbientBlendThreshold();
		direcHandle_.mRoleAmbientBlendThreshold = mSceneManager->getRoleAmbientBlendThreshold();
		direcHandle_.m_ViewLightPercent = mSceneManager->getViewLightPercent();
		direcHandle_.m_PBRViewLightPercent = mSceneManager->getPBRViewLightPercent();
		direcHandle_.m_RoleViewLightPercent = mSceneManager->getRoleViewLightPercent();
		if (mSceneManager->getSun() != nullptr)
			direcHandle_.mSunDirection = mSceneManager->getSun()->getSunDirection();
	}
	void TODComponentManager::_getSunAttribute(sunHandle& mSunData_)
	{
		if (mSceneManager == nullptr)
			return;
		Sun* sun_ = mSceneManager->getSun();
		if (sun_)
		{
			mSunData_.mSunColor = mSceneManager->getSunColor();
			mSunData_.mEmmittingColor = sun_->getEmissionColour();
			mSunData_.mInRadius = sun_->getSunInnerRadius();
			mSunData_.mOutRadius = sun_->getSunOuterRadius();
			mSunData_.mOpacity = sun_->getSunOpacity();
		}
	}
	
	void TODComponentManager::_setCloudTodTimelineState(bool isUse)
	{
		if (mFinalComVec[T_CloudMesh])
		{
			CloudMeshComponent* cloudMeshCom_ = static_cast<CloudMeshComponent*>(mFinalComVec[T_CloudMesh]);
			cloudMeshCom_->setIsUpdateTodTimeline(isUse);
		}
	}
	void TODComponentManager::_getCloudData(Cloud* pCloud)
	{
		if(mCloudSphereHandle)
		  _getCloudSphereData(*mCloudSphereHandle,pCloud);
		if(mCloudMeshHandle)
		  _getCloudMeshData(*mCloudMeshHandle, pCloud);
		if(mCloudPlaneHandle)
		  _getCloudPlaneData(*mCloudPlaneHandle, pCloud);
	
	}
	void TODComponentManager::_getCloudMeshData(CloudMeshComponentContext& cloudMeshData_, Cloud* pCloud, bool isGetAllData)
	{
		if (pCloud == nullptr)
			return;

		_getCloudMeshDataByType(cloudMeshData_, eCloudType_Mesh_Up,pCloud, isGetAllData);
		_getCloudMeshDataByType(cloudMeshData_, eCloudType_Mesh_Mid, pCloud, isGetAllData);
		_getCloudMeshDataByType(cloudMeshData_, eCloudType_Mesh_Down, pCloud, isGetAllData);
	
	}
	void TODComponentManager::_getCloudSphereData(CloudSphereComponentContext& cloudSphereData_, Cloud* pCloud)
	{
		if (pCloud == nullptr)
			return;

		CloudSphere* sphereCloud_ = pCloud->getCloudSphere();
		if (sphereCloud_)
		{
			cloudSphereData_.mColour = sphereCloud_->getCloudColour();
			cloudSphereData_.mBackColour = sphereCloud_->getCloudSunBackColour();
			cloudSphereData_.mEmendation = sphereCloud_->getCloudEmendation();
			cloudSphereData_.mSpeed = sphereCloud_->getCloudSpeed();
			cloudSphereData_.mOpacity = sphereCloud_->getCloudOpacity();
			if (sphereCloud_->getVisible() == false)
				cloudSphereData_.mOpacity = 0.f;
		}
	}
	void TODComponentManager::_getCloudPlaneData(CloudPlaneComponentContext& cloudPlaneData_,Cloud* pCloud)
	{
		if (pCloud == nullptr)
			return;
		CloudPlane* planeCloud_ = pCloud->getCloudPlane();
		if (planeCloud_)
		{
			cloudPlaneData_.mColour = planeCloud_->getCloudColour();
			cloudPlaneData_.mBackColor = planeCloud_->getCloudSunBackColour();
			cloudPlaneData_.mEmendation = planeCloud_->getCloudEmendation();
			//cloudPlaneData_.mHeight = planeCloud_->getCloudHeight().x;
			cloudPlaneData_.mURollSpeed = planeCloud_->getCloudUSpeed();
			cloudPlaneData_.mVRollSpeed = planeCloud_->getCloudVSpeed();
			cloudPlaneData_.mUVTilling = planeCloud_->getCloudUVTilling();
			cloudPlaneData_.mOpacity = planeCloud_->getCloudOpacity();
			if (planeCloud_->getVisible() == false)
				cloudPlaneData_.mOpacity = 0.f;
		}
	}

	void TODComponentManager::_setTodComAttributeInFirstTrans()
	{
		updateContext<WindPowerComponent, WindPowerHandle>(mWindPowerContext,nullptr);
		updateContext<DriftComponent, DriftPropData>(mDriftContext,nullptr);
		updateContext<EnvTriggerEventComponent, TriggrtEventComponentData>(mEnvTriggerContext, nullptr);
		updateContext<LightningComponent, LightningHandle>(mLightningContext,nullptr);
		updateContext<VolumeLightComponent, VolumeLightComponentContext>(mVolumeLightContext,nullptr);
	}

	void TODComponentManager::_setCloudSeaParamByType(Cloud* pCloud, int type, uint32 index, SceneArtData* pData)
	{
		if (!pCloud || !pData
			|| index >= SceneArtData::eCloudSeaMax)
		{
			return;
		}
		CloudType type_ = static_cast<CloudType>(type);
		pCloud->setCloudUseOriginalUV(type_, pData->mCloudSeaUseOriUV[index]);
		pCloud->setCloudUseSunBackColor(type_, pData->mCloudSeaUseSunBackColor[index]);
		pCloud->setCloudUseDistort(type_, pData->mCloudSeaUseDistort[index]);
		pCloud->setCloudMeshFile(type_, pData->mCloudSeaMeshFile[index]);
		pCloud->setCloudTextureName(type_, pData->mCloudSeaDiffuse[index]);
		pCloud->setCloudHeight(type_, pData->mCloudSeaHeight[index]);
		pCloud->setCloudScale(type_, pData->mCloudSeaScale[index]);
		pCloud->setCloudUSpeed(type_, pData->mCloudSeaUSpeed[index]);
		pCloud->setCloudVSpeed(type_, pData->mCloudSeaVSpeed[index]);
		pCloud->setCloudUVTilling(type_, pData->mCloudSeaTilling[index]);
		pCloud->setCloudDistortSpeed(type_, pData->mCloudSeaDistortSpeed[index]);
		pCloud->setCloudDistortAmplitude(type_, pData->mCloudSeaDistortAmp[index]);
		pCloud->setCloudDistortScale(type_, pData->mCloudSeaDistortScale[index]);
		pCloud->setCloudRenderPriority(type_, pData->mCloudSeaRenderPriority[index]);

		pCloud->setCloudVisible(type_ , pData->getCloudSeaVisible() && (!pData->mCloudSeaMeshFile[index].empty()));
		pCloud->updateCloudMaterial(type_);
	}

	void TODComponentManager::_StfLoadFinishCallBack(void* data)
	{
		if (!data)
			return;
		SceneArtData* artData = static_cast<SceneArtData*>(data);
		
		float transTime_ = mTransTimeByClient;
		setSceneArtData(artData);
	
		if (mTransState != S_NoneTrans)
		{
			mIsChangeTodStfID = false;
			setTransFormTimeFromClient(mTransTimeByClient);

			if (mTodManager)
				mTodManager->setTranslationTime(transTime_ - 0.001f);
			if (artData)
			{
				artData->setTodTransitionTime(transTime_ - 0.001f);
				artData->setCloudSeaTransitionTime(transTime_ / 2.f - 0.0005f);
			}
			if (mWeatherManager && getIsEffectCoverColor())
				mWeatherManager->setTransTime(transTime_);
		}
	}

	void TODComponentManager::_AddControlTodComVec(ControlTodChangeComponent* com)
	{
		addTODItem<ControlTodChangeComponent>(com,mControlComVec);
	}

	void TODComponentManager::_RemoveControlTodComVec(ControlTodChangeComponent* com)
	{
		std::vector<ControlTodChangeComponent*>::const_iterator iter  = std::find(mControlComVec.begin(), mControlComVec.end(), com);

		if (iter != mControlComVec.end())
		{
			mControlComVec.erase(iter);
		}
	}

	ControlTodChangeComponent* TODComponentManager::_GetFirstControlCom()
	{
		if(mControlComVec.empty())
		   return nullptr;
		return mControlComVec[0];
	}

};
