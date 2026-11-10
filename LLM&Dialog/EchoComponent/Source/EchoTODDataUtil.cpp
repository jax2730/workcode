#include "EchoComponentStableHeaders.h"
#include "EchoTODDataUtil.h"
#include "Actor/EchoActor.h"
#include "EchoTodTimelineComponent.h"
#include "EchoTODParamTrace.h"
#include "EchoSingleton.h"
#include "EchoTODComponentSystem.h"
#include "EchoTODComponentManager.h"

#include "EchoVideoConfigSystem.h"
#include "EchoPostProcess.h"
#include "EchoWorldManager.h"

#include "EchoOcean.h"
#include "EchoFFTOcean.h"
#include "EchoSphericalOcean.h"
#include "EchoCloud.h"
#include "EchoEnvTriggerEvent.h"
#include "EchoSky.h"
#include "EchoSun.h"
#include "EchoStars.h"
#include "EchoEnvironmentLight.h"
#include "EchoHDR.h"
#include "EchoVolumeLight.h"
#include "EchoDeferredRenderStrategy.h"
#include "EchoVolumetricLight.h"

#include "EchoLensFlare.h"
#include "EchoLensFlareComponent.h"

// TOD 组件
#include "EchoOceanComponent.h"
#include "EchoCloudMeshComponent.h"
#include "EchoCloudSphereComponent.h"
#include "EchoCloudPlaneComponent.h"
#include "EchoFogComponent.h"
#include "EchoSkyComponent.h"
#include "EchoSunComponent.h"
#include "EchoUnderWaterCausticsComponent.h"
#include "EchoPointlightManagerComponent.h"
#include "EchoWindPowerComponent.h"
#include "EchoEnvTriggerEventComponent.h"
#include "EchoDirecLightComponent.h"
#include "EchoStarsComponent.h"
#include "EchoDriftComponent.h"
#include "EchoWeatherCoverColorComponent.h"
#include "EchoEnvironmentLightComponent.h"
#include "EchoLightningComponent.h"
#include "EchoHDRComponent.h"
#include "EchoGlobalLightComponent.h"
#include "EchoVolumeLightComponent.h"
#include "EchoTODShadowComponent.h"
#include "EchoSphereFogComponent.h"
#include "EchoSphericalTerrainFog.h"
#include "EchoHDRAdaptiveComponent.h"

#include "EchoTimeLineInfo.h"
#include "EchoWorldSystem.h"
#include "EchoAreaManager.h"
#include "EchoSceneArtTemplateManager.h"
#include "EchoSceneArtTemplate.h"
#include "EchoSceneArtData.h"
#include "EchoSceneTriggerData.h"

#include "EchoEnvTriggerEventComponent.h"
#include "EchoLightFlareComponent.h"

namespace Echo
{
#ifdef ECHO_EDITOR
	struct StringToNumber
	{
		String	m_strParam;
		int		m_iUseSize;
	};

	std::vector<String> TODDataUtil::m_vActNames;
	std::vector<TODFrameData> TODDataUtil::m_vTimeData;
	std::vector<StringToNumber> TODDataUtil::m_vDataTemplates;
	std::vector<bool> TODDataUtil::m_Registry;
	std::map<Name, CalcTangentData> TODDataUtil::m_vTangentDatas;

	int CalcFrame(double time, const TodTimelineComponentPtr& pTod)
	{
		float currTime = static_cast<float>(time) * 0x15180;
		return static_cast<int>(currTime * pTod->mDeltaTime + 0.5f);
	};

	SceneManager* getSceneManager()
	{
		SceneManager* pSc = nullptr;
		Name activeName = WorldSystem::instance()->GetActiveWorld();
		if (TODSystem::Alive())
		{
			TODManager* pTodMgr = TODSystem::instance()->GetManager(activeName);
			if (pTodMgr)
				pSc = pTodMgr->getSceneManager();
		}
		return pSc;
	}

	const ActorComponentPtr& getComponentByType(Actor* pRoot, const Name& type)
	{
		String pType = type.toString();

		const ActorPtr& child = pRoot->getChildActor(pType.substr(0, pType.find("Component")) + "_TOD");
		return child->getComponent(pType);
	}

	void TODDataUtil::InitTimeData(TODVariableInfo* pTodData)
	{
		if (!pTodData)
		{
			assert(!"pTodData is Null");
			return;
		}

		m_vTimeData.resize(TodDataID::PARAM_TOTAL);
		m_vDataTemplates.resize(TodDataID::PARAM_TOTAL);
		m_Registry.resize(TodDataID::PARAM_TOTAL);

		RetrieveData("SunComponent",				TodDataID::PARAM_SUN_COLOR,							TodDataID::PARAM_SUN_COLOR_HDR_POWER,				pTodData);
		RetrieveData("SkyComponent",				TodDataID::PARAM_SKY_BOTTOM_COLOR,					TodDataID::PARAM_SKY_BRIGHT_INTENSITY,				pTodData);
		RetrieveData("StarsComponent",				TodDataID::PARAM_STAR_SIZE,							TodDataID::PARAM_STAR_COLOR_HDR_POWER,				pTodData);
		// RetrieveData("VolumetricComponent",			TodDataID::PARAM_VOLUMETRIC_LIGHT_SIZE,				TodDataID::PARAM_VOLUMETRIC_LIGHT_HDR_POWER,		pTodData);
		// RetrieveData("LensFlareComponent",		TodDataID::PARAM_LENS_FLARE_OPACITY,				TodDataID::PARAM_LENS_FLARE_OPACITY);
		RetrieveData("CloudSphereComponent",		TodDataID::PARAM_CLOUD_SPEED,						TodDataID::PARAM_CLOUD_TEXTURE,						pTodData);
		RetrieveData("CloudPlaneComponent",			TodDataID::PARAM_CLOUD_PLANE_U_ROLLSPEED,			TodDataID::PARAM_CLOUD_PLANE_SUNBACK_COLOR_POWER,	pTodData);
		RetrieveData("CloudMeshComponent",			TodDataID::PARAM_CLOUD_MESH_UP_OPACITY,				TodDataID::PARAM_CLOUD_MESH_DOWN_TEXTURE,			pTodData);
		RetrieveData("FogComponent",				TodDataID::PARAM_FOG_0_COLOR,						TodDataID::PARAM_FOG_3_HEIGHT2,						pTodData);
		RetrieveData("DirecLightComponent",			TodDataID::PARAM_MAIN_LIGHT_YAW,					TodDataID::PARAM_GROUND_LIGHT_HDR_POWER,			pTodData);
		RetrieveData("DirecLightComponent",			TodDataID::PARAM_PBR_MAIN_LIGHT_COLOR,				TodDataID::PARAM_ROLE_GROUND_LIGHT_HDR_POWER,		pTodData);
		RetrieveData("PointlightManagerComponent",	TodDataID::PARAM_GLOBAL_POINT_LIGHT_POWER,			TodDataID::PARAM_ROLE_POINT_LIGHT_POWER,			pTodData);
		RetrieveData("GlobalLightComponent",		TodDataID::PARAM_GLOBAL_SPECULAR_RATIO,				TodDataID::PARAM_GLOBAL_REFLECTION_RATIO,			pTodData);
		RetrieveData("HDRComponent",				TodDataID::PARAM_POST_PROCESS_HDR_EYEADAPTATION_0X, TodDataID::PARAM_POST_PROCESS_HDR_TONEMAPPING_3,	pTodData);
		RetrieveData("VolumeLightComponent",		TodDataID::PARAM_POST_PROCESS_VOLUME_LIGHT_AMOUNT,	TodDataID::PARAM_POST_PROCESS_VOLUME_LIGHT_COLOR,	pTodData);
		RetrieveData("WeatherCoverColorComponent",	TodDataID::PARAM_WEATHER_COVER_COLOR,				TodDataID::PARAM_WEATHER_COVER_COLOR_POWER,			pTodData);
		RetrieveData("OceanComponent",				TodDataID::PARAM_OCEAN_WIND_POWER,					TodDataID::PARAM_OCEAN_UNDERWATER_FOG_COLOR,		pTodData);
		RetrieveData("UnderWaterCausticsComponent", TodDataID::PARAM_UNDERWATER_CAUSTIC_COLOR,			TodDataID::PARAM_UNDERWATER_CAUSTIC_COLOR_POWER,	pTodData);
		RetrieveData("WindPowerComponent",			TodDataID::PARAM_WIND_POWER,						TodDataID::PARAM_WIND_VEG_SHAKE_FREQ,				pTodData);
		RetrieveData("TODShadowComponent",			TodDataID::PARAM_SHADOW_WEIGHT,						TodDataID::PARAM_SHADOW_WEIGHT, pTodData);
		RetrieveData("EnvironmentLightComponent",	TodDataID::PARAM_GLOBAL_ENVIRONMENT_EXPLOSURE,		TodDataID::PARAM_GLOBAL_ENVIRONMENT_EXPLOSURE, pTodData);

		//CheckData(pTodData);
	}

	void TODDataUtil::InsertTimeData(int id, const TODFrameData& pData)
	{
		if(m_vTimeData.size() <= id)
			m_vTimeData.resize(TodDataID::PARAM_TOTAL);

		m_vTimeData[id] = pData;
	}

	void TODDataUtil::SetTodTemplate(int id, const String& pParamStr, int pParamSize)
	{
		if (m_Registry.size() <= id)
		{
			m_vDataTemplates.resize(TodDataID::PARAM_TOTAL);
			m_Registry.resize(TodDataID::PARAM_TOTAL);
		}

		if (!m_Registry[id])
		{
			m_Registry[id] = true;
			m_vDataTemplates[id] = { pParamStr , pParamSize };
		}
	}

	void TODDataUtil::CreateTodActor(Actor* pRoot)
	{
		CreateTodComponent(pRoot);

		std::vector<ActorPtr> pTodActor;
		pRoot->getAllChildActors(pTodActor);

		TODComponentManager::instance()->closeAllTodCom();

		for (ActorPtr& act : pTodActor)
		{
			String id = act->getActorID();
			String comName = id.substr(0, id.find("_TOD")) + "Component";
			ActorComponent* com = act->getComponent(comName).get();
			SetComponentTemplate(com);
		}

		UseTODData(pRoot);

		m_vTangentDatas.clear();
	}

	void TODDataUtil::CreateTodComActor(Actor* pRoot)
	{
		CreateTodComponent(pRoot);

		String CreatedCom = EnvTriggerEventComponent::mType.toString();
		String ActName = "EnvTriggerEvent_TOD";
		ActorPtr pAct = pRoot->getChildActor(ActName);
		if (pAct->GetActComSize() <= 1)
		{
			pAct->setActorID(ActName);
			pAct->addComponent(Name(CreatedCom)
				, CreatedCom, pAct->getRootNodeCom(), nullptr
				, pAct->getRootNodeCom()->getGroupID(), false);
		}

		static std::vector<String> comStr =
		{
			LightningComponent::mType.toString(),
			LensFlareComponent::mType.toString()
		};

		for (const String& comname : comStr)
		{
			ActName = comname.substr(0, comname.find("Component")) + "_TOD";
			pAct = pRoot->getChildActor(ActName);
			if (pAct->GetActComSize() <= 1)
			{
				pAct->setActorID(ActName);
				pAct->addComponent(Name(comname)
					, comname, pAct->getRootNodeCom(), nullptr
					, pAct->getRootNodeCom()->getGroupID(), false);
			}
		}
	}

	void TODDataUtil::UseTODData(Actor* pRoot)
	{
		std::vector<ActorPtr> pTodActor;
		pRoot->getAllChildActors(pTodActor);

		SetTodComponentData(pRoot);

		// 设置关键点斜率
		for (ActorPtr& act : pTodActor)
		{
			String id = act->getActorID();
			String comName = id.substr(0, id.find("_TOD")) + "Component";
			ActorComponent* com = act->getComponent(comName).get();
			//TODComponentManager::instance()->setTodComIsUse(com, true);

			TodTimelineComponentPtr tcom = act->getComponent("TodTimelineComponent").getStaticCastWeakPtr<TodTimelineComponent>();

			if (tcom.expired()) continue;

			int pFOffset = CalcFrame(0.01f, tcom);

			const comMapping& comMaps = tcom->getComToAttribute();
			auto mapIter = comMaps.find(comName);
			if (mapIter != comMaps.end())
			{
				for (const attributeData& d : mapIter->second)
				{
					if (!d.attributeName.empty() && !comName.empty())
					{
						Name pTangentName = Name(comName + "::" + d.attributeName.toString());
						CalcTangentData& pData = m_vTangentDatas[pTangentName];
						if (!pData.m_vDatas.empty())
						{
							for (int pFrameI = 0; pFrameI < pData.m_vDatas.size(); ++pFrameI)
							{
								int pFrame = CalcFrame(pData.m_vTimes[pFrameI], tcom);

								for (int st = d.st; st < d.en; ++st)
								{
									int pV4Index = st - d.st;
									float leftHand = (pData.m_vDatas[pFrameI][pV4Index] - pData.m_vLeftHand[pFrameI][pV4Index]) / pFOffset;
									float rightHand = (pData.m_vRightHand[pFrameI][pV4Index] - pData.m_vDatas[pFrameI][pV4Index]) / pFOffset;
									tcom->AddKeyWithHandTangent(st, pFrame, pData.m_vDatas[pFrameI][pV4Index], leftHand, rightHand);
								}
							}
						}
					}
				}
			}

			TODComponentManager::instance()->setTodComIsUse(com, true);

			if (tcom->getType() == TodTimelineComponent::mType)
				tcom.getStaticCastWeakPtr<TodTimelineComponent>()->setDayTime(11, 0);
			else
			{
				tcom->remakeTime();
				tcom->remakeDayTime();
			}
		}
		m_vTangentDatas.clear();
	}

	void TODDataUtil::CheckData(TODVariableInfo* pTodData)
	{
		if (!pTodData)
		{
			assert(!"pTodData is Null");
			return;
		}

		for (int i = 0; i < pTodData->getVariableCount(); ++i)
		{
			if (m_vTimeData[i].m_strCom.empty()) continue;

			TODVariableInfo::SVariableInfo varInfo;
			if (pTodData->getVariableInfo(i, varInfo))
			{
				int count = varInfo.pParamTrace->getKeyCount();
				for (int fram = 0; fram < count; ++fram)
				{
					double time = varInfo.pParamTrace->getKeyTime(fram);
					float data[4] = { 0.f };
					if (varInfo.pParamTrace->getKeyValue(fram, data))
					{
						if (m_vTimeData[i].m_vTimes[fram] != time ||
							m_vTimeData[i].m_vDatas[fram] != Vector4(data))
						{
							assert(!"Error In Copy TOD Data");
						}
					}
				}
			}
		}
	}

	void TODDataUtil::RetrieveData(const String& pComName, int pBegin, int pEnd, TODVariableInfo* pTodData)
	{
		if (std::find(m_vActNames.begin(), m_vActNames.end(), pComName) == m_vActNames.end())
			m_vActNames.push_back(pComName);
		int index = pBegin;
		do
		{
			TODFrameData pComData;
			pComData.m_strCom = pComName;

			TODVariableInfo::SVariableInfo varInfo;
			if (pTodData->getVariableInfo(index, varInfo))
			{
				int count = varInfo.pParamTrace->getKeyCount();
				for (int fram = 0; fram < count; ++fram)
				{
					double time = varInfo.pParamTrace->getKeyTime(fram);
					if (time < 0.0) time = 0.0;
					if (varInfo.pParamTrace->getType() == ParamTrace::PT_STRING)
					{
						String data = "";
						if (varInfo.pParamTrace->getKeyValue(fram, data))
						{
							pComData.m_vTimes.push_back(time);
							pComData.m_strDatas.push_back(data);
						}
					}
					else
					{
						float data[4] = { 0.f, 0.f, 0.f, 1.f };
						if (varInfo.pParamTrace->getKeyValue(fram, data))
						{
							if (index == TodDataID::PARAM_GLOBAL_SPECULAR_RATIO)
								data[0] *= 0.1f;

							pComData.m_vTimes.push_back(time);
							pComData.m_vDatas.push_back(Vector4(data));
						}

						double left = time - 0.01;
						double right = time + 0.01;

						Vector4 pTangetData(data);
						if (left >= 0.0)
							varInfo.pParamTrace->lerpFloat3(left, pTangetData.ptr());
						pComData.m_vLeftDatas.push_back(Vector4(pTangetData));

						pTangetData = Vector4(data);
						if(right <= 1.0)
							varInfo.pParamTrace->lerpFloat3(right, pTangetData.ptr());
						pComData.m_vRightDatas.push_back(Vector4(pTangetData));
					}
				}
			}
			InsertTimeData(index, pComData);
			index++;
		} while (index <= pEnd);
	}

	void TODDataUtil::CreateTodComponent(Actor* pRoot)
	{
		String CreatedCom = m_vTimeData[0].m_strCom;
		String ActName = CreatedCom.substr(0, CreatedCom.find("Component")) + "_TOD";

		String pRootName = pRoot->getActorIDStr();

		for (int i = 0; i < m_vTimeData.size(); ++i)
		{
			if (m_vTimeData[i].m_strCom.empty())
				continue;

			if (CreatedCom != m_vTimeData[i].m_strCom)
			{
				CreatedCom = m_vTimeData[i].m_strCom;
				ActName = CreatedCom.substr(0, CreatedCom.find("Component")) + "_TOD";
			}

			ActorPtr pAct = pRoot->getChildActor(ActName);

			if (pAct->GetActComSize() <= 1)
			{
				pAct->setActorID(ActName);
				pAct->addComponent(Name(CreatedCom)
					, CreatedCom, pAct->getRootNodeCom(), nullptr
					, pAct->getRootNodeCom()->getGroupID(), false);

				TodTimelineComponentPtr pTimeCom = pAct->addComponent(Name("TodTimelineComponent")
					, "TodTimelineComponent", pAct->getRootNodeCom(), nullptr
					, pAct->getRootNodeCom()->getGroupID(), false
				).getStaticCastWeakPtr<TodTimelineComponent>();

				pTimeCom->m_TimelineFilePath = "actors/weather/" + pRootName + "_" + ActName.substr(0, ActName.find("_TOD")) + ".ttl";
				TodTimelineComponentInfo* pInfo = static_cast<TodTimelineComponentInfo*>(pTimeCom->m_pActComInfo);
				pInfo->m_TimelineFilePath = pTimeCom->m_TimelineFilePath;
			}
		}
	}

	void TODDataUtil::SetTodComponentData(Actor* pRoot)
	{
		String CreatedCom = m_vTimeData[0].m_strCom;
		String ActName = CreatedCom.substr(0, CreatedCom.find("Component")) + "_TOD";

		TodTimelineComponentPtr pTodTimeline = TodTimelineComponentPtr();
		ActorComponentPtr pTodCom = ActorComponentPtr();
		for (int i = 0; i < m_vTimeData.size(); ++i)
		{
			if (m_vTimeData[i].m_strCom.empty())
				continue;

			if (CreatedCom != m_vTimeData[i].m_strCom)
			{
				CreatedCom = m_vTimeData[i].m_strCom;
				ActName = CreatedCom.substr(0, CreatedCom.find("Component")) + "_TOD";
			}

			ActorPtr pAct = pRoot->getChildActor(ActName);

			pTodCom = pAct->getComponent(CreatedCom);
			pTodTimeline = pAct->getComponent("TodTimelineComponent").getStaticCastWeakPtr<TodTimelineComponent>();

			std::vector<double>& pTimes = m_vTimeData[i].m_vTimes;
			std::vector<Vector4>& pDatas = m_vTimeData[i].m_vDatas;
			std::vector<String>& pStrDatas = m_vTimeData[i].m_strDatas;

			if (pTodTimeline.expired() || pTodCom.expired())
				continue;

			if (m_vDataTemplates[i].m_strParam.empty())
				continue;

			float offset = 0.f;

			float CrossEnumValue[5] = { 0.f };
			for (int tIndex = 0; tIndex < pTimes.size(); ++tIndex)
			{
				pTodTimeline->setTimelineFrame(CalcFrame(pTimes[tIndex], pTodTimeline));

				// 若存在将三个枚举值组成一个Vector3传递时候，触发此逻辑
				if (m_vDataTemplates[i].m_iUseSize < 0)
				{
					int CrossNum = abs(m_vDataTemplates[i].m_iUseSize);
					for (int num = 0; num < CrossNum; ++num)
						CrossEnumValue[num] = m_vTimeData[i + num].m_vDatas[tIndex][0];

					pTodTimeline->setTodAttributeData(pTodCom->getCombinedName(), m_vDataTemplates[i].m_strParam,
						CrossEnumValue, CrossNum, offset);
					continue;
				}

				if (!pDatas.empty())
					pTodTimeline->setTodAttributeData(pTodCom->getCombinedName(), m_vDataTemplates[i].m_strParam,
						pDatas[tIndex].ptr(), m_vDataTemplates[i].m_iUseSize, offset);
				else if (!pStrDatas.empty() && !pStrDatas[tIndex].empty())
					pTodTimeline->setTodStringAttributeData(pTodCom->getCombinedName(), m_vDataTemplates[i].m_strParam, pStrDatas[tIndex]);

				Name pName = Name(CreatedCom + "::" + m_vDataTemplates[i].m_strParam);
				if (pStrDatas.empty() &&
					m_vTangentDatas[pName].m_vDatas.empty())
				{
					m_vTangentDatas[pName] = {
						m_vTimeData[i].m_vTimes,
						m_vTimeData[i].m_vDatas,
						m_vTimeData[i].m_vLeftDatas,
						m_vTimeData[i].m_vRightDatas
					};
				}
			}
		}

		m_vTimeData.clear();
	}

	void TODDataUtil::SetComponentTemplate(ActorComponent*& pCom)
	{
		SceneManager* sMgr = TODComponentSystem::instance()->getActiveTodComManager()->GetSceneManager();

		Ocean* pOcean = sMgr->getOcean();
		if (pOcean && pCom->getComNameToString() == "OceanComponent")
		{
			OceanComponent* oceanCom = static_cast<OceanComponent*>(pCom);
			
			const String& pReflectTextureName = pOcean->getReflectTextureName();

			float  pOceanHeight = pOcean->getOceanHeight();
			float  pWavefrequency = pOcean->getWaveFrequency();
			float  pWaveAmplitude = pOcean->getWaveAmplitude();
			float  pBumpScale = pOcean->getBumpScale();
			float  pRefractAmount = pOcean->getRefractAmount();
			float  pBorderSoft = pOcean->getBorderSoft();
			float  pSpecularPower = pOcean->getSpecularPower();
			float  pSpecularStrength = pOcean->getSpecularStrength();
			float  pFoamBias = pOcean->getFoamBias();
			float  pFoamSoft = pOcean->getFoamSoft();
			float  pFoamTiling = pOcean->getFoamTilling();
			float  pUnderWaterFogDensity = pOcean->getUnderWaterFogDensity();
			float  pUnderWaterFogStart = pOcean->getUnderWaterFogStart();
			float  pUnderWaterFogEnd = pOcean->getUnderWaterFogEnd();
			//bool pForceUnderWater = pOcean->isForceUnderWater();

			oceanCom->setReflectTextureName(pReflectTextureName);
			oceanCom->setWaveFrequency(pWavefrequency);
			oceanCom->setOceanHeight(pOceanHeight);
			oceanCom->setWaveAmplitude(pWaveAmplitude);
			oceanCom->setBumpScale(pBumpScale);
			oceanCom->setRefractAmount(pRefractAmount);
			oceanCom->setBorderSoft(pBorderSoft);
			oceanCom->setSpecularPower(pSpecularPower);
			oceanCom->setSpecularStrength(pSpecularStrength);
			oceanCom->setFoamBias(pFoamBias);
			oceanCom->setFoamSoft(pFoamSoft);
			oceanCom->setFoamTilling(pFoamTiling);
			oceanCom->setUnderWaterFogDensity(pUnderWaterFogDensity);
			oceanCom->setUnderWaterFogStart(pUnderWaterFogStart);
			oceanCom->setUnderWaterFogEnd(pUnderWaterFogEnd);

			if (oceanCom->m_pActComInfo)
			{
				OceanComponentInfo* pInfo = static_cast<OceanComponentInfo*>(oceanCom->m_pActComInfo);
				pInfo->mContext = oceanCom->getContext();
			}
		}

		Cloud* pCloud = sMgr->getCloud();
		if (pCloud && pCom->getComNameToString() == "CloudMeshComponent")
		{
			CloudMeshComponent* cloudMeshCom = static_cast<CloudMeshComponent*>(pCom);

			std::vector<bool>			isVisible;
			float pCloudBorderSoft = 0.f;

			std::vector<String>			m_vMeshFile;
			std::vector<String>			m_vCloudTexName;
			std::vector<float>			m_vTopHeight;
			std::vector<float>			m_vBottomHeight;
			std::vector<float>			m_vScale;
			std::vector<float>			m_vCloudUSpeed;
			std::vector<float>			m_vCloudVSpeed;
			std::vector<float>			m_vCloudUVTiling;
			std::vector<unsigned int>	m_vUseOriginalUV;
			std::vector<unsigned int>	m_vUseSunBackColor;
			std::vector<unsigned int>	m_vUseDistort;
			std::vector<float>			m_vCloudDistortSpeed;
			std::vector<float>			m_vDistortAmplitude;
			std::vector<float>			m_vDistortScale;
			std::vector<int32>			m_vRenderPriority;
			bool						m_bVisible = false;

			m_vMeshFile.resize(3);
			m_vCloudTexName.resize(3);
			m_vTopHeight.resize(3);
			m_vBottomHeight.resize(3);
			m_vScale.resize(3);
			m_vCloudUSpeed.resize(3);
			m_vCloudVSpeed.resize(3);
			m_vCloudUVTiling.resize(3);
			m_vUseOriginalUV.resize(3);
			m_vUseSunBackColor.resize(3);
			m_vUseDistort.resize(3);
			m_vCloudDistortSpeed.resize(3);
			m_vDistortAmplitude.resize(3);
			m_vDistortScale.resize(3);
			m_vRenderPriority.resize(3);
			isVisible.resize(3);

			auto setCloudMesh = [&](CloudMesh* pCloud, int pIndex) {
				isVisible[pIndex] = pCloud->getVisible();
				pCloudBorderSoft = pCloud->getCloudBorderSoft();

				Vector2 vHeight = pCloud->getCloudHeight();
				m_vTopHeight[pIndex] = vHeight.y;
				m_vBottomHeight[pIndex] = vHeight.x;

				m_vMeshFile[pIndex] = pCloud->getMeshFile();
				m_vCloudTexName[pIndex] = pCloud->getCloudTextureName();
				m_vScale[pIndex] = pCloud->getScale();
				m_vCloudUSpeed[pIndex] = pCloud->getCloudUSpeed();
				m_vCloudVSpeed[pIndex] = pCloud->getCloudVSpeed();
				m_vCloudUVTiling[pIndex] = pCloud->getCloudUVTilling();
				m_vUseOriginalUV[pIndex] = pCloud->getUseOriginalUV() ? 1 : 0;
				m_vUseSunBackColor[pIndex] = pCloud->getUseSunBackColor() ? 1 : 0;
				m_vUseDistort[pIndex] = pCloud->getUseDistort() ? 1 : 0;
				m_vCloudDistortSpeed[pIndex] = pCloud->getDistortSpeed();
				m_vDistortAmplitude[pIndex] = pCloud->getDistortAmplitude();
				m_vDistortScale[pIndex] = pCloud->getDistortScale();
				m_vRenderPriority[pIndex] = pCloud->getRenderPriority();

				m_bVisible |= isVisible[pIndex];
			};

			CloudMesh* pUp = pCloud->getCloudMeshUp();
			if (pUp)
				setCloudMesh(pUp, 0);

			CloudMesh* pMid = pCloud->getCloudMeshMid();
			if (pMid)
				setCloudMesh(pMid, 1);

			CloudMesh* pDown = pCloud->getCloudMeshDown();
			if (pDown)
				setCloudMesh(pDown, 2);

			cloudMeshCom->setCloudVisible(m_bVisible);
			//cloudMeshCom->setCloudVisible(isVisible);
			cloudMeshCom->setCloudBorderSoft(pCloudBorderSoft);
			cloudMeshCom->setMeshName(m_vMeshFile);
			cloudMeshCom->setTextureName(m_vCloudTexName);
			cloudMeshCom->setTopHeight(m_vTopHeight);
			cloudMeshCom->setBottomHeight(m_vBottomHeight);
			cloudMeshCom->setCloudMeshScale(m_vScale);
			cloudMeshCom->setURollSpeed(m_vCloudUSpeed);
			cloudMeshCom->setVRollSpeed(m_vCloudVSpeed);
			cloudMeshCom->setUVTilling(m_vCloudUVTiling);
			cloudMeshCom->setIsUseOriginalUV(m_vUseOriginalUV);
			cloudMeshCom->setIsUseDistort(m_vUseDistort);
			cloudMeshCom->setIsShowBackColor(m_vUseSunBackColor);
			cloudMeshCom->setDistortSpeed(m_vCloudDistortSpeed);
			cloudMeshCom->setDistortAmplitude(m_vDistortAmplitude);
			cloudMeshCom->setDistortScale(m_vDistortScale);
			cloudMeshCom->setRenderPriority(m_vRenderPriority);

			if (cloudMeshCom->m_pActComInfo)
			{
				CloudMeshComponentInfo* pInfo = static_cast<CloudMeshComponentInfo*>(cloudMeshCom->m_pActComInfo);
				pInfo->mContext = cloudMeshCom->mContext;
			}
		}

		if (pCloud && pCom->getComNameToString() == "CloudSphereComponent")
		{
			CloudSphereComponent* cloudSphereCom = static_cast<CloudSphereComponent*>(pCom);
			CloudSphere* pCloudSphere = pCloud->getCloudSphere();
			if (pCloudSphere)
			{
				const String& pTexName = pCloudSphere->getCloudTextureName();
				const String& pMeshFile = pCloudSphere->getMeshFile();
				bool isVisible = pCloudSphere->getVisible();

				cloudSphereCom->setTextureName(pTexName);
				cloudSphereCom->setMeshName(pMeshFile);
				cloudSphereCom->setCloudSphereVisible(isVisible);

				if (cloudSphereCom->m_pActComInfo)
				{
					CloudSphereComponentInfo* pInfo = static_cast<CloudSphereComponentInfo*>(cloudSphereCom->m_pActComInfo);
					pInfo->mContext = cloudSphereCom->mContext;
				}
			}
		}

		if (pCloud && pCom->getComNameToString() == "CloudPlaneComponent")
		{
			CloudPlaneComponent* cloudPlaneCom = static_cast<CloudPlaneComponent*>(pCom);
			CloudPlane* pCloudPlane = pCloud->getCloudPlane();

			if (pCloudPlane)
			{
				bool isVisible = pCloudPlane->getVisible();
				const String& pTexName = pCloudPlane->getCloudTextureName();
				const String& pStarsTexName = pCloudPlane->getStarsTextureName();
				float pHeight = pCloudPlane->getCloudHeight().x;

				cloudPlaneCom->setCloudPlaneVisible(isVisible);
				cloudPlaneCom->setTextureName(pTexName);
				cloudPlaneCom->setStarsTextureName(pStarsTexName);
				cloudPlaneCom->setHeight(pHeight);

				if (cloudPlaneCom->m_pActComInfo)
				{
					CloudPlaneComponentInfo* pInfo = static_cast<CloudPlaneComponentInfo*>(cloudPlaneCom->m_pActComInfo);
					pInfo->mContext = cloudPlaneCom->mContext;
				}
			}

		}
	}

	// number : 0 第一段颜色		type :	0 近
	//			1 第二段颜色				1 远
	//			2 第三段颜色		
	//			3 第四段颜色		
	float GetFogColorPower(ActorComponentInfo* info, TimeLineInfoPtr& pTimelineInfo, int frame, int number, int type, String str)
	{
		float result = -1.f;
		const std::vector<EchoHermite*> pHermites = pTimelineInfo->getHermiteData();
		const comMapping pComToAttribute = pTimelineInfo->getComToAttribute();
		for (const auto& comIter : pComToAttribute)
		{
			if (result != -1.f) break;
			for (const auto& attrIter : pComToAttribute.at(comIter.first))
			{
				if (attrIter.st >= pHermites.size()) continue;

				// 查询颜色强度
				if ((type == 0 && attrIter.attributeName == "NearFogColorIntensity") ||
					(type == 1 && attrIter.attributeName == "FarFogColorIntensity")	 )
				{
					int index = attrIter.st + number;
					if (index < attrIter.en)
					{
						EchoFloatHermite* pHermite = static_cast<EchoFloatHermite*>(pHermites[index]);
						result = pHermite->Interpolate((float)frame);

						LogManager::getSingleton().logMessage(Echo::LML_CRITICAL, "GetFogColorPower : __File__ :" + str);

						break;
					}
				}
			}
		}

		if (result == -1.f)
		{
			FogComponentInfo* fogInfo = static_cast<FogComponentInfo*>(info);
			if (type == 0)
				result = fogInfo->mContext.mNearFogColorIntensity[number];
			else if (type == 1)
				result = fogInfo->mContext.mFarFogColorIntensity[number];
		}

		return result;
	}

	void TODDataUtil::ModifyTimelineFileData(Actor* act, const String& path)
	{
		if (path.empty())
			return;
		TimeLineInfoPtr pTimelineInfo;
		try
		{
			pTimelineInfo = TimeLineInfoManager::instance()->load(Name(path));
		}
		catch (...)
		{
			pTimelineInfo = TimeLineInfoManager::instance()->createManual(Name(path));
			if (pTimelineInfo.isNull())
			{
				LogManager::getSingleton().logMessage(Echo::LML_CRITICAL, "TODDataUtil::ModifyTimelineFileData :File not exit: " + path);
				return;
			}
			else
				pTimelineInfo->load();
		}
		bool changeColor = false;
		std::vector<String> pStrToNumber = { "First", "Second", "Third", "Fourth" };
		const std::vector<EchoHermite*> pHermites = pTimelineInfo->getHermiteData();
		const comMapping pComToAttribute = pTimelineInfo->getComToAttribute();
		for (const auto& comIter : pComToAttribute)
		{
			ActorComponentPtr com = act->getComponent(comIter.first);
			if (com.expired() || com == nullptr)
				continue;

			ActorComponentInfo* info = com->m_pActComInfo;

			for (const auto& attrIter : pComToAttribute.at(comIter.first))
			{
				if (attrIter.st >= pHermites.size()) continue;
				
				if (attrIter.attributeName == "LightningSunColor")
				{
					for (int i = attrIter.st; i < attrIter.en; ++i)
					{
						for (const auto& key : pHermites[i]->mKeyMap)
						{
							HermiteFloatKey* floatKey = reinterpret_cast<HermiteFloatKey*>(key.second);
							floatKey->value = std::pow(floatKey->value, 0.454545f);
							changeColor = true;
						}
					}
					continue;
				}

				int number = -1;
				int type = -1;

				// 防止旧数据未存储isColor标志信息的情况
				if (attrIter.attributeName == "FirstNearFogColor"	|| attrIter.attributeName == "FirstFarFogColor"		||
					attrIter.attributeName == "SecondNearFogColor"	|| attrIter.attributeName == "SecondFarFogColor"	||
					attrIter.attributeName == "ThirdNearFogColor"	|| attrIter.attributeName == "ThirdFarFogColor"		||
					attrIter.attributeName == "FourthNearFogColor"	|| attrIter.attributeName == "FourthFarFogColor"	)
				{
					String attributeStr = attrIter.attributeName.toString();
					if (attributeStr.find("Far") != std::string::npos)
						type = 1;
					else
						type = 0;

					auto i = std::find_if(pStrToNumber.begin(), pStrToNumber.end(), [attributeStr](String key) {return attributeStr.find(key) != String::npos; });
					number = (int)std::distance(pStrToNumber.begin(), i);
					if (i == pStrToNumber.end())
						number = -1;
				}
				
				if (number == -1 || type == -1)
					continue;

				for (int i = attrIter.st; i < attrIter.en; ++i)
				{
					for (auto& key : pHermites[i]->mKeyMap)
					{
						HermiteFloatKey* floatKey = reinterpret_cast<HermiteFloatKey*>(key.second);
						float ColorPower = GetFogColorPower(info, pTimelineInfo, key.first, number, type, path);
						float Color = static_cast<float>(std::pow(floatKey->value, 2.2f) * std::pow(ColorPower, 6.f / 11.f));
						floatKey->value = static_cast<int>(Color * 255 + 0.5f) / 255.f;
						changeColor = true;
					}
				}
			}
		}

		if (changeColor)
			pTimelineInfo.getPointer()->saveTimelineFile(path);
	}
#endif

	TODUtil::TODUtil()
	{
		TODComponentManager* tcMgr = nullptr;

		if (TODComponentSystem::Alive() && WorldSystem::Alive())
			tcMgr = TODComponentSystem::instance()->getTODComponentManager(WorldSystem::instance()->GetActiveWorld());

		if (tcMgr)
			area = new AreaManager(tcMgr->GetSceneManager());
#ifdef ECHO_EDITOR
		mSATMgr = new SceneArtTemplateManager(nullptr);
#endif
	}

	TODUtil::~TODUtil()
	{
		SAFE_DELETE(area)
#ifdef ECHO_EDITOR
		SAFE_DELETE(mSATMgr)
#endif
	}

	void SetSkyJSONData(TODComponentManager* tcMgr, const String& skyPath)
	{
		if (!tcMgr) return;

		Echo::DataStreamPtr pDataStream(Echo::Root::instance()->GetFileDataStream(skyPath.c_str()));
		if (!pDataStream.isNull())
		{
			size_t nSize = pDataStream->size();
			char* pData = new char[nSize + 1];
			memset(pData, 0, nSize + 1);
			pDataStream->read(pData, nSize);

			JSNODE hRoot = cJSON_Parse(pData);
			delete[] pData;
			pDataStream->close();

			if (0 != hRoot)
			{
				cJSON* hInfo = cJSON_GetObjectItem(hRoot, "Sky");

				tcMgr->createCloudSphere();
				CloudSphere* cloudSphere = tcMgr->getComCloudSphere();
				if (cloudSphere)
				{
					bool cloudSphereVisible = false;
					String cloudSphereTexture, cloudSphereMesh;

					ReadJSNode(hInfo, "CloudSphereVisible", cloudSphereVisible);
					ReadJSNode(hInfo, "CloudTexture", cloudSphereTexture);
					ReadJSNode(hInfo, "CloudSphereMeshFile", cloudSphereMesh);

					cloudSphere->setCloudFadingOut(false);
					cloudSphere->setVisible(cloudSphereVisible);
					cloudSphere->setMeshFile(cloudSphereMesh);
					cloudSphere->setCloudTextureName(cloudSphereTexture);
				}

				tcMgr->createCloudPlane();
				CloudPlane* cloudPlane = tcMgr->getComCloudPlane();
				if (cloudPlane)
				{
					bool cloudPlaneVisible = false;
					String cloudPlaneTexture, starsTexture;
					float cloudHeight = 0.f;

					ReadJSNode(hInfo, "CloudPlaneVisible", cloudPlaneVisible);
					ReadJSNode(hInfo, "CloudTexture2", cloudPlaneTexture);
					ReadJSNode(hInfo, "CloudPlaneHeight", cloudHeight);
					ReadJSNode(hInfo, "StarsTexture", starsTexture);

					cloudPlane->setVisible(cloudPlaneVisible);
					cloudPlane->setCloudTextureName(cloudPlaneTexture);
					cloudPlane->setStarsTextureName(starsTexture);
					cloudPlane->setCloudHeight(Vector2(cloudHeight, cloudHeight));
				}

				tcMgr->createSun();
				Sun* sun = tcMgr->getSun();
				if (sun)
				{
					String sunTexture, moonTexture[Echo::Sun::MAX_MOON_PHASE_CNT];
					ReadJSNode(hInfo, "SunTexture", sunTexture);
					for (int i = 0; i < Echo::Sun::MAX_MOON_PHASE_CNT; i++)
					{
						String sCount = "MoonTex" + StringConverter::toString(i);
						ReadJSNode(hInfo, sCount.c_str(), moonTexture[i]);
					}

					sun->setSunTextureName(sunTexture);
					for (Echo::uint32 i = 0; i < Echo::Sun::MAX_MOON_PHASE_CNT; i++)
						sun->setMoonTextureName(i, moonTexture[i]);
				}

				tcMgr->createStars();
				Stars* stars = tcMgr->getStars();
				if (stars)
				{
					bool starsShowLowerPart = false;
					ReadJSNode(hInfo, "StarsShowLowerPart", starsShowLowerPart);

					stars->setShowLowerPart(starsShowLowerPart);
				}

				WeatherManager* wMgr = WeatherSystem::instance()->GetMainManager();
				if (wMgr)
				{
					float lightningLifeTime = 0.f, lightningLimitYawAngle = 0.f;
					bool lightningDelayed = false, lightningModelUseParticle = false;
					Vector2 lightningInterval, lightningParticleOffsetRange, lightningParticleScaleRange;
					std::vector<Echo::String> lightningTexture;
					std::vector<Echo::String> lightningParticle;
					std::vector<Echo::String> lightningSound;
					std::vector<Echo::String> lightningBankName;
					std::vector<Echo::String> lightningEventName;

					ReadJSNode(hInfo, "LightningLifeTime", lightningLifeTime);
					ReadJSNode(hInfo, "LightningDelayed", lightningDelayed);
					ReadJSNode(hInfo, "LightningInterval", lightningInterval);
					ReadJSNode(hInfo, "LightningLimitYawAngle", lightningLimitYawAngle);
					ReadJSNode(hInfo, "LightningUseParticle", lightningModelUseParticle);
					ReadJSNode(hInfo, "LightningParticleOffsetRange", lightningParticleOffsetRange);
					ReadJSNode(hInfo, "LightningParticleScaleRange", lightningParticleScaleRange);

					lightningTexture.resize(Echo::Lightning::MAX_LIGHTNING_TEXTURE_CNT);
					for (int i = 0; i < Echo::Lightning::MAX_LIGHTNING_TEXTURE_CNT; i++)
					{
						String sCount = "LightningTexture" + StringConverter::toString(i);
						ReadJSNode(hInfo, sCount.c_str(), lightningTexture[i]);
					}
					lightningParticle.resize(Echo::Lightning::MAX_LIGHTNING_TEXTURE_CNT);
					for (int j = 0; j < Echo::Lightning::MAX_LIGHTNING_TEXTURE_CNT; j++)
					{
						String sCount = "LightningParticle" + StringConverter::toString(j);
						ReadJSNode(hInfo, sCount.c_str(), lightningParticle[j]);
					}
					lightningSound.resize(Echo::Lightning::MAX_LIGHTNING_SOUND_CNT);
					lightningBankName.resize(Echo::Lightning::MAX_LIGHTNING_SOUND_CNT);
					lightningEventName.resize(Echo::Lightning::MAX_LIGHTNING_SOUND_CNT);
					for (int k = 0; k < Echo::Lightning::MAX_LIGHTNING_SOUND_CNT; k++)
					{
						String sCount = "LightningSound" + StringConverter::toString(k);
						ReadJSNode(hInfo, sCount.c_str(), lightningSound[k]);
						//新的音效
						sCount = "LightningBank" + StringConverter::toString(k);
						ReadJSNode(hInfo, sCount.c_str(), lightningBankName[k]);
						sCount = "LightningEvent" + StringConverter::toString(k);
						ReadJSNode(hInfo, sCount.c_str(), lightningEventName[k]);
					}

					wMgr->setLightningTextureName(LT_NORMAL, lightningTexture);
					wMgr->setLightningParticleName(LT_NORMAL, lightningParticle);
					wMgr->setLightningLifeTime(LT_NORMAL, lightningLifeTime);
					wMgr->setLightningInterval(LT_NORMAL, lightningInterval);
					wMgr->setLightningLimitYawAngle(LT_NORMAL, lightningLimitYawAngle);
					//新的音效
					//_setLightningBells(nullptr, "");
					wMgr->setLightningSound(LT_NORMAL, lightningSound);
					wMgr->setLightningDelayed(LT_NORMAL, lightningDelayed);
					wMgr->setLightningUseParticle(LT_NORMAL, lightningModelUseParticle);
					wMgr->setLightningParticleOffsetRange(LT_NORMAL, lightningParticleOffsetRange);
					wMgr->setLightningParticleScaleRange(LT_NORMAL, lightningParticleScaleRange);
				}

				if (Echo::TODSystem::Alive())
				{
					Echo::TODManager* todMgr = Echo::TODSystem::instance()->GetMainManager();
					if (todMgr)
					{
						float mainLightPitchLimit = 0.f;
						String colorGradingTexture;
						ReadJSNode(hInfo, "MainLightPitchLimit", mainLightPitchLimit);
						ReadJSNode(hInfo, "TodColorGrading", colorGradingTexture);

						todMgr->setMainLightPitchLimit(mainLightPitchLimit);
						todMgr->setDefaultColorChart(colorGradingTexture);
					}
				}

				SceneManager* scMgr = tcMgr->GetSceneManager();
				if (scMgr)
				{
					bool volumetricLightEnabled = false;
					ReadJSNode(hInfo, "VolumetricLightVisible", volumetricLightEnabled);

					VolumetricLight* pVLight = scMgr->getVolumetricLight();
					if (pVLight)
						pVLight->setEnabled(volumetricLightEnabled);
				}

				if (scMgr)
				{
					LensFlare* flare_ = scMgr->getLensFlare();
					if (flare_)
					{
						bool lensFlareEnabled = false;
						String lensFlareTexture;
						Vector2 lensFlareDynamicScaleRange;
						unsigned int elementRandom[Echo::LensFlare::MAX_ELEMENT_CNT] = { 0 };
						unsigned int elementTexIndex[Echo::LensFlare::MAX_ELEMENT_CNT] = { 0 };
						bool elementRotate[Echo::LensFlare::MAX_ELEMENT_CNT] = { false };
						ColorValue elementColour[Echo::LensFlare::MAX_ELEMENT_CNT];
						Vector2 elementLocation[Echo::LensFlare::MAX_ELEMENT_CNT];
						Vector2 elementSize[Echo::LensFlare::MAX_ELEMENT_CNT];
						Vector2 elementOpacity[Echo::LensFlare::MAX_ELEMENT_CNT];
						ReadJSNode(hInfo, "LensFlareVisible", lensFlareEnabled);
						ReadJSNode(hInfo, "LensFlareTexture", lensFlareTexture);
						ReadJSNode(hInfo, "LensFlareScaleRange", lensFlareDynamicScaleRange);
						for (int i = 0; i < Echo::LensFlare::MAX_ELEMENT_CNT; i++)
						{
							String sCount = "LensFlareElement" + Echo::StringConverter::toString(i);

							ReadJSNode(hInfo, (sCount + "random").c_str(), elementRandom[i]);
							ReadJSNode(hInfo, (sCount + "texture").c_str(), elementTexIndex[i]);
							ReadJSNode(hInfo, (sCount + "rotate").c_str(), elementRotate[i]);
							ReadJSNode(hInfo, (sCount + "color").c_str(), elementColour[i]);
							ReadJSNode(hInfo, (sCount + "locationRange").c_str(), elementLocation[i]);
							ReadJSNode(hInfo, (sCount + "sizeRange").c_str(), elementSize[i]);
							ReadJSNode(hInfo, (sCount + "opacityRange").c_str(), elementOpacity[i]);
						}

						std::vector<LensFlareParam> paramVec;
						for (int i = 0; i < Echo::LensFlare::MAX_ELEMENT_CNT; i++)
						{
							Echo::uint32 random = elementRandom[i];
							for (Echo::uint32 j = 0; j < random; j++)
							{
								LensFlareParam param;

								param.textureIndex = elementTexIndex[i];
								param.color = elementColour[i];
								param.rotate = elementRotate[i];

								Echo::Vector2 locationRange = elementLocation[i];
								if (locationRange.x > locationRange.y)
								{
									locationRange.y = locationRange.x;
								}
								param.location = Echo::Math::RangeRandom(locationRange.x, locationRange.y);

								Echo::Vector2 sizeRange = elementSize[i];
								if (sizeRange.x > sizeRange.y)
								{
									sizeRange.y = sizeRange.x;
								}
								param.size = Echo::Math::RangeRandom(sizeRange.x, sizeRange.y);

								Echo::Vector2 opacityRange = elementOpacity[i];
								if (opacityRange.x > opacityRange.y)
								{
									opacityRange.y = opacityRange.x;
								}
								param.opacity = Echo::Math::RangeRandom(opacityRange.x, opacityRange.y);

								paramVec.push_back(param);
							}
						}
						flare_->setElements(paramVec);
						flare_->setTextureName(lensFlareTexture);
					}
				}

				cJSON_Delete(hRoot);
			}
			TODContext<EnvTriggerEventComponent, TriggrtEventComponentData>& context = tcMgr->getEnvTriggerContext();
			tcMgr->setIsUseThisType(context, false);
		}
	}

	void TODUtil::ReplaceTOD(bool useDefault)
	{
		if (!replaceTOD) return;
		if (!TODComponentSystem::Alive() || !WorldSystem::Alive()) return;

		Name activeName = WorldSystem::instance()->GetActiveWorld();
		if (TODSystem::Alive())
		{
			TODManager* pTodMgr = TODSystem::instance()->GetManager(activeName);
			if (pTodMgr)
				pTodMgr->enablePlay(false);
		}

		TODComponentManager* tcMgr = TODComponentSystem::instance()->getTODComponentManager(activeName);
		if (tcMgr)
		{
			if (!useDefault)
			{
				tcMgr->setIsStopUpdate(true);
				tcMgr->setAllTodComIsUse(false);
			}
			else
			{
				tcMgr->setIsStopUpdate(false);
				tcMgr->setAllTodComIsUse(true);
			}
		}

#ifdef ECHO_EDITOR
		TODComponentSystem::instance()->stopUI = true;
#endif // ECHO_EDITOR

		if (area)
		{
			if (EnvironmentLight::Alive())
			{
				if (!useDefault)
					environmentLightState = static_cast<int>(EnvironmentLight::instance()->getUpdateSource());
				EnvironmentLight::instance()->setUpdateSource(EnvironmentLight::DataFromEditor);
			}

			bool fog = false;
			String weatherPath = StringTable::instance()->getString("TOD", "replaceSTFfile");
			if (useDefault)
			{
				weatherPath = StringTable::instance()->getString("TOD", "DefaultSTFfile");
				fog = true;
			}

			String skyPath = StringTable::instance()->getString("TOD", "replaceSkyJSON");
			SetSkyJSONData(tcMgr, skyPath);
			area->setTransitionArtTemplate(false);
			area->applySceneArtTemplate(weatherPath);

			RenderSystem* pRenderSystem = Root::instance()->getRenderSystem();
			if (pRenderSystem) {
				PostProcess* process = pRenderSystem->getCurRenderStrategy()->getPostProcessChain()->getPostProcess(PostProcess::Type::SphericalTerrainFog);
				if (process != nullptr)
				{
					SphericalTerrainFog* pFog = dynamic_cast<SphericalTerrainFog*>(process);
					pFog->enableFog(fog);
					pFog->enableAtmosphere(fog);
				}
			}
		}
		if (tcMgr->GetSceneManager())
		{
			TODManager* pTodMgr = tcMgr->GetSceneManager()->getTODManager();
			if (pTodMgr)
			{
				pTodMgr->setTranslationTime(0.f);
				pTodMgr->updateData(0.0, true);
			}
		}
		if (useDefault)
		{
			tcMgr->setIsStopUpdate(false);
			if (EnvironmentLight::Alive())
				EnvironmentLight::instance()->setUpdateSource(static_cast<EnvironmentLight::UpdateSource>(environmentLightState));
		}

		replaceTOD = false;
	}
#ifdef ECHO_EDITOR
	void SetSkyJSONData(Actor* pRoot, const String& skyPath)
	{
		TODComponentManager* tcMgr = TODComponentManager::instance();
		if (!tcMgr) return;

		Echo::DataStreamPtr pDataStream(Echo::Root::instance()->GetFileDataStream(skyPath.c_str()));
		if (!pDataStream.isNull())
		{
			size_t nSize = pDataStream->size();
			char* pData = new char[nSize + 1];
			memset(pData, 0, nSize + 1);
			pDataStream->read(pData, nSize);

			JSNODE hRoot = cJSON_Parse(pData);
			delete[] pData;
			pDataStream->close();

			if (0 != hRoot)
			{
				cJSON* hInfo = cJSON_GetObjectItem(hRoot, "Sky");

				CloudSphereComponent* cloudSphere = static_cast<CloudSphereComponent*>
					(getComponentByType(pRoot, CloudSphereComponent::mType).get());

				if (cloudSphere)
				{
					bool cloudSphereVisible = false;
					String cloudSphereTexture, cloudSphereMesh;

					ReadJSNode(hInfo, "CloudSphereVisible", cloudSphereVisible);
					ReadJSNode(hInfo, "CloudTexture", cloudSphereTexture);
					ReadJSNode(hInfo, "CloudSphereMeshFile", cloudSphereMesh);

					cloudSphere->setVisible(cloudSphereVisible);
					cloudSphere->setMeshName(cloudSphereMesh);
					cloudSphere->setTextureName(cloudSphereTexture);
				}

				CloudPlaneComponent* cloudPlane = static_cast<CloudPlaneComponent*>
					(getComponentByType(pRoot, CloudPlaneComponent::mType).get());

				if (cloudPlane)
				{
					bool cloudPlaneVisible = false;
					String cloudPlaneTexture, starsTexture;
					float cloudHeight = 0.f;

					ReadJSNode(hInfo, "CloudPlaneVisible", cloudPlaneVisible);
					ReadJSNode(hInfo, "CloudTexture2", cloudPlaneTexture);
					ReadJSNode(hInfo, "CloudPlaneHeight", cloudHeight);
					ReadJSNode(hInfo, "StarsTexture", starsTexture);

					cloudPlane->setVisible(cloudPlaneVisible);
					cloudPlane->setTextureName(cloudPlaneTexture);
					cloudPlane->setStarsTextureName(starsTexture);
					cloudPlane->setHeight(cloudHeight);
				}

				SunComponent* sun = static_cast<SunComponent*>
					(getComponentByType(pRoot, SunComponent::mType).get());

				if (sun)
				{
					String sunTexture;
					std::vector<String> moonTexture;
					moonTexture.resize(Echo::Sun::MAX_MOON_PHASE_CNT);
					ReadJSNode(hInfo, "SunTexture", sunTexture);
					for (int i = 0; i < Echo::Sun::MAX_MOON_PHASE_CNT; i++)
					{
						String sCount = "MoonTex" + StringConverter::toString(i);
						ReadJSNode(hInfo, sCount.c_str(), moonTexture[i]);
					}

					sun->setSonTexture(sunTexture);
					sun->setMoonTextures(moonTexture);
				}

				StarsComponent* stars = static_cast<StarsComponent*>
					(getComponentByType(pRoot, StarsComponent::mType).get());
				if (stars)
				{
					bool starsShowLowerPart = false;
					ReadJSNode(hInfo, "StarsShowLowerPart", starsShowLowerPart);

					stars->setIsUseLowerStarsPart(starsShowLowerPart);
				}

				WeatherManager* wMgr = WeatherSystem::instance()->GetMainManager();
				if (wMgr)
				{
					float lightningLifeTime = 0.f, lightningLimitYawAngle = 0.f;
					bool lightningDelayed = false, lightningModelUseParticle = false;
					Vector2 lightningInterval, lightningParticleOffsetRange, lightningParticleScaleRange;
					std::vector<Echo::String> lightningTexture;
					std::vector<Echo::String> lightningParticle;
					std::vector<Echo::String> lightningSound;
					std::vector<Echo::String> lightningBankName;
					std::vector<Echo::String> lightningEventName;

					ReadJSNode(hInfo, "LightningLifeTime", lightningLifeTime);
					ReadJSNode(hInfo, "LightningDelayed", lightningDelayed);
					ReadJSNode(hInfo, "LightningInterval", lightningInterval);
					ReadJSNode(hInfo, "LightningLimitYawAngle", lightningLimitYawAngle);
					ReadJSNode(hInfo, "LightningUseParticle", lightningModelUseParticle);
					ReadJSNode(hInfo, "LightningParticleOffsetRange", lightningParticleOffsetRange);
					ReadJSNode(hInfo, "LightningParticleScaleRange", lightningParticleScaleRange);

					lightningTexture.resize(Echo::Lightning::MAX_LIGHTNING_TEXTURE_CNT);
					for (int i = 0; i < Echo::Lightning::MAX_LIGHTNING_TEXTURE_CNT; i++)
					{
						String sCount = "LightningTexture" + StringConverter::toString(i);
						ReadJSNode(hInfo, sCount.c_str(), lightningTexture[i]);
					}
					lightningParticle.resize(Echo::Lightning::MAX_LIGHTNING_TEXTURE_CNT);
					for (int j = 0; j < Echo::Lightning::MAX_LIGHTNING_TEXTURE_CNT; j++)
					{
						String sCount = "LightningParticle" + StringConverter::toString(j);
						ReadJSNode(hInfo, sCount.c_str(), lightningParticle[j]);
					}
					lightningSound.resize(Echo::Lightning::MAX_LIGHTNING_SOUND_CNT);
					lightningBankName.resize(Echo::Lightning::MAX_LIGHTNING_SOUND_CNT);
					lightningEventName.resize(Echo::Lightning::MAX_LIGHTNING_SOUND_CNT);
					for (int k = 0; k < Echo::Lightning::MAX_LIGHTNING_SOUND_CNT; k++)
					{
						String sCount = "LightningSound" + StringConverter::toString(k);
						ReadJSNode(hInfo, sCount.c_str(), lightningSound[k]);
						//新的音效
						sCount = "LightningBank" + StringConverter::toString(k);
						ReadJSNode(hInfo, sCount.c_str(), lightningBankName[k]);
						sCount = "LightningEvent" + StringConverter::toString(k);
						ReadJSNode(hInfo, sCount.c_str(), lightningEventName[k]);
					}

					wMgr->setLightningTextureName(LT_NORMAL, lightningTexture);
					wMgr->setLightningParticleName(LT_NORMAL, lightningParticle);
					wMgr->setLightningLifeTime(LT_NORMAL, lightningLifeTime);
					wMgr->setLightningInterval(LT_NORMAL, lightningInterval);
					wMgr->setLightningLimitYawAngle(LT_NORMAL, lightningLimitYawAngle);
					//新的音效
					//_setLightningBells(nullptr, "");
					wMgr->setLightningSound(LT_NORMAL, lightningSound);
					wMgr->setLightningDelayed(LT_NORMAL, lightningDelayed);
					wMgr->setLightningUseParticle(LT_NORMAL, lightningModelUseParticle);
					wMgr->setLightningParticleOffsetRange(LT_NORMAL, lightningParticleOffsetRange);
					wMgr->setLightningParticleScaleRange(LT_NORMAL, lightningParticleScaleRange);
				}

				if (Echo::TODSystem::Alive())
				{
					Echo::TODManager* todMgr = Echo::TODSystem::instance()->GetMainManager();
					if (todMgr)
					{
						float mainLightPitchLimit = 0.f;
						String colorGradingTexture;
						ReadJSNode(hInfo, "MainLightPitchLimit", mainLightPitchLimit);
						ReadJSNode(hInfo, "TodColorGrading", colorGradingTexture);

						todMgr->setMainLightPitchLimit(mainLightPitchLimit);
						todMgr->setDefaultColorChart(colorGradingTexture);
					}
				}

				SceneManager* scMgr = tcMgr->GetSceneManager();
				if (scMgr)
				{
					bool volumetricLightEnabled = false;
					ReadJSNode(hInfo, "VolumetricLightVisible", volumetricLightEnabled);

					VolumetricLight* pVLight = scMgr->getVolumetricLight();
					if (pVLight)
						pVLight->setEnabled(volumetricLightEnabled);
				}

				LensFlareComponent* lfCom = static_cast<LensFlareComponent*>
					(getComponentByType(pRoot, LensFlareComponent::mType).get());

				if (lfCom)
				{
					bool lensFlareEnabled = false;
					String lensFlareTexture;
					Vector2 lensFlareDynamicScaleRange;
					unsigned int elementRandom[Echo::LensFlare::MAX_ELEMENT_CNT] = { 0 };
					unsigned int elementTexIndex[Echo::LensFlare::MAX_ELEMENT_CNT] = { 0 };
					bool elementRotate[Echo::LensFlare::MAX_ELEMENT_CNT] = { false };
					ColorValue elementColour[Echo::LensFlare::MAX_ELEMENT_CNT];
					Vector2 elementLocation[Echo::LensFlare::MAX_ELEMENT_CNT];
					Vector2 elementSize[Echo::LensFlare::MAX_ELEMENT_CNT];
					Vector2 elementOpacity[Echo::LensFlare::MAX_ELEMENT_CNT];
					ReadJSNode(hInfo, "LensFlareVisible", lensFlareEnabled);
					ReadJSNode(hInfo, "LensFlareTexture", lensFlareTexture);
					ReadJSNode(hInfo, "LensFlareScaleRange", lensFlareDynamicScaleRange);
					for (int i = 0; i < Echo::LensFlare::MAX_ELEMENT_CNT; i++)
					{
						String sCount = "LensFlareElement" + Echo::StringConverter::toString(i);

						ReadJSNode(hInfo, (sCount + "random").c_str(), elementRandom[i]);
						ReadJSNode(hInfo, (sCount + "texture").c_str(), elementTexIndex[i]);
						ReadJSNode(hInfo, (sCount + "rotate").c_str(), elementRotate[i]);
						ReadJSNode(hInfo, (sCount + "color").c_str(), elementColour[i]);
						ReadJSNode(hInfo, (sCount + "locationRange").c_str(), elementLocation[i]);
						ReadJSNode(hInfo, (sCount + "sizeRange").c_str(), elementSize[i]);
						ReadJSNode(hInfo, (sCount + "opacityRange").c_str(), elementOpacity[i]);
					}

					std::vector<LensFlareParam> paramVec;
					for (int i = 0; i < Echo::LensFlare::MAX_ELEMENT_CNT; i++)
					{
						Echo::uint32 random = elementRandom[i];
						for (Echo::uint32 j = 0; j < random; j++)
						{
							LensFlareParam param;

							param.textureIndex = elementTexIndex[i];
							param.color = elementColour[i];
							param.rotate = elementRotate[i];

							Echo::Vector2 locationRange = elementLocation[i];
							if (locationRange.x > locationRange.y)
							{
								locationRange.y = locationRange.x;
							}
							param.location = Echo::Math::RangeRandom(locationRange.x, locationRange.y);

							Echo::Vector2 sizeRange = elementSize[i];
							if (sizeRange.x > sizeRange.y)
							{
								sizeRange.y = sizeRange.x;
							}
							param.size = Echo::Math::RangeRandom(sizeRange.x, sizeRange.y);

							Echo::Vector2 opacityRange = elementOpacity[i];
							if (opacityRange.x > opacityRange.y)
							{
								opacityRange.y = opacityRange.x;
							}
							param.opacity = Echo::Math::RangeRandom(opacityRange.x, opacityRange.y);

							paramVec.push_back(param);
						}
					}

					lfCom->setLensFlareParamsVec(paramVec);
					lfCom->setLensFlareTexture(lensFlareTexture);
				}

				cJSON_Delete(hRoot);
			}
			TODContext<EnvTriggerEventComponent, TriggrtEventComponentData>& context = tcMgr->getEnvTriggerContext();
			tcMgr->setIsUseThisType(context, false);
		}
	}

	void TODUtil::InitSTFID()
	{
		String pStfTablePath = StringTable::instance()->getString("TOD", "STFConfig");

		DataStreamPtr pDataStream(Root::instance()->GetFileDataStream(pStfTablePath.c_str()));
		if (pDataStream.isNull())
		{
			LogManager::getSingleton().logMessage("-error-\tcan't find file! " + pStfTablePath, LML_CRITICAL);
			return;
		}

		size_t pMemorySize = pDataStream->size();
		if (0 == pMemorySize)
		{
			LogManager::getSingleton().logMessage("-error-\tthe file is empty! " + pStfTablePath, LML_CRITICAL);
			return;
		}

		char* pData = new char[pMemorySize + 1];
		memset(pData, 0, pMemorySize + 1);
		pDataStream->read(pData, pMemorySize);

		auto removeSpace = [](String& str)
		{
			str.erase(std::remove_if(str.begin(), str.end(), [](unsigned char c) { return std::isspace(c); }), str.end());
		};

		std::vector<String> pDataLine;
		String value = pData;
		size_t pos = value.find('\n');
		while (pos != String::npos)
		{
			String v = value.substr(0, pos);
			removeSpace(v);
			pDataLine.push_back(v);
			value = value.substr(pos + 1, value.size());
			pos = value.find('\n');
		}
		pDataLine.push_back(value);

		String insertID = "";
		for (const String& str : pDataLine)
		{
			pos = str.find(':');
			if (pos == String::npos && str.find('[') != String::npos)
			{
				insertID = str;
				continue;
			}

			if (pos != String::npos)
			{
				String name = str.substr(0, pos);
				removeSpace(name);
				String v = str.substr(pos + 1, str.size());
				removeSpace(v);
				mSTFMap[insertID][name] = v;
			}
		}

		delete[] pData;
		pDataStream->close();
	}

	void TODUtil::SelectSTFID(const String& id, const String& name)
	{
		if (mSTFMap.empty()) return;

		if (mSTFMap.find(id) == mSTFMap.end() || mSTFMap[id].find(name) == mSTFMap[id].end()) return;

		String stfpath = mSTFMap[id][name];

		mSceneArtTemplate = mSATMgr->loadTemplate(stfpath, true, true);
	}

	bool TODUtil::GetTodStatusData(TODVariableInfo*& data)
	{
		bool result = false;

		if (mSceneArtTemplate)
		{
			SceneArtData* pArtData = mSceneArtTemplate->getArtData();
			if (!pArtData) return result;

			int useTodID = 0;
			mSceneManager = getSceneManager();
			if (!mSceneManager) return result;

			//TOD
			TODManager* pTodMgr = mSceneManager->getTODManager();
			if (pTodMgr)
			{
				const String& todFileName = pArtData->getTodFileName(useTodID);
				if (!todFileName.empty())
				{
					data = pTodMgr->loadTod(todFileName);
					result = true;
				}
			}
		}
		return result;
	}

	bool TODUtil::UseSTF(Actor* pRoot)
	{
		String skyPath = StringTable::instance()->getString("TOD", "DefaultSkyJson");
		if (!skyPath.empty())
		{
			SetSkyJSONData(pRoot, skyPath);
			return true;
		}
		return false;
	}

	void TODUtil::applyComData(Actor* pRoot)
	{
		std::vector<ActorPtr> pTodActor;
		pRoot->getAllChildActors(pTodActor);

		TODComponentManager::instance()->closeAllTodCom();

		for (ActorPtr& act : pTodActor)
		{
			String id = act->getActorID();
			String comName = id.substr(0, id.find("_TOD")) + "Component";
			ActorComponent* com = act->getComponent(comName).get();
			SetComponentData(com);
		}
	}

	void TODUtil::SetComponentData(ActorComponent*& pCom)
	{
		if (!pCom || !mSceneArtTemplate) return;

		SceneArtData* pArtData = mSceneArtTemplate->getArtData();
		if (!pArtData) return;

		if (mSceneManager)
		{
			WeatherManager* pWeatherMgr = WeatherSystem::instance()->GetManager(mSceneManager->getName());

			// 声音
			if (pWeatherMgr)
				pWeatherMgr->setSound(pArtData->getWeatherSoundFiles(), pArtData->getWeatherMusicFiles(), pArtData->getWeatherSoundFadeIn(), pArtData->getWeatherSoundFadeInTime(),
					pArtData->getWeatherSoundFadeOut(), pArtData->getWeatherSoundFadeOutTime(), pArtData->getSoundAreaVolumePrecent());
		}

		if (pCom->getType() == OceanComponent::mType)
		{
			OceanComponent* oceanCom = static_cast<OceanComponent*>(pCom);

			//海洋
			float  pOceanHeight = pArtData->getOceanHeight();
			float  pWavefrequency = pArtData->getWaveFrequency();
			float  pWaveAmplitude = pArtData->getWaveAmplitude();
			float  pBumpScale = pArtData->getBumpScale();
			float  pRefractAmount = pArtData->getRefractAmount();
			float  pBorderSoft = pArtData->getBorderSoft();
			float  pSpecularPower = pArtData->getSpecularPower();
			float  pSpecularStrength = pArtData->getSpecularStrength();
			float  pFoamBias = pArtData->getFoamBias();
			float  pFoamSoft = pArtData->getFoamSoft();
			float  pFoamTiling = pArtData->getFoamTilling();
			float  pUnderWaterFogDensity = pArtData->getUnderWaterFogDensity();
			float  pUnderWaterFogStart = pArtData->getUnderWaterFogStart();
			float  pUnderWaterFogEnd = pArtData->getUnderWaterFogEnd();

			oceanCom->setOceanHeight(pOceanHeight);
			oceanCom->setWaveFrequency(pWavefrequency);
			oceanCom->setWaveAmplitude(pWaveAmplitude);
			oceanCom->setBumpScale(pBumpScale);
			oceanCom->setRefractAmount(pRefractAmount);
			oceanCom->setBorderSoft(pBorderSoft);
			oceanCom->setReflectTextureName(pArtData->getReflectTexName());

			oceanCom->setSpecularPower(pSpecularPower);
			oceanCom->setSpecularStrength(pSpecularStrength);

			oceanCom->setFoamBias(pFoamBias);
			oceanCom->setFoamSoft(pFoamSoft);
			oceanCom->setFoamTilling(pFoamTiling);

			oceanCom->setUnderWaterFogDensity(pUnderWaterFogDensity);
			oceanCom->setUnderWaterFogStart(pUnderWaterFogStart);
			oceanCom->setUnderWaterFogEnd(pUnderWaterFogEnd);

			if (oceanCom->m_pActComInfo)
			{
				OceanComponentInfo* pInfo = static_cast<OceanComponentInfo*>(oceanCom->m_pActComInfo);
				pInfo->mContext = oceanCom->getContext();
			}

			Ocean* pOcean = mSceneManager->getOcean();
			Root* pRoot = Root::instance();
			if (pRoot->m_bFFTOcean && pOcean)
			{
				FFTOcean* pfftOcean = (FFTOcean*)(pOcean);
				pfftOcean->setUVScale(pArtData->getUVScale());
				pfftOcean->setPhillipsA(pArtData->getPhillipsA());
				pfftOcean->setLambda(pArtData->getLambda());
				pfftOcean->setHeightScale(pArtData->getHeightScale());
				pfftOcean->setBubblesScale(pArtData->getBubblesScale());
				pfftOcean->setBubblesThreshold(pArtData->getBubblesThreshold());
				pfftOcean->setWindScale(pArtData->getFFTWindScale());
				pfftOcean->setTimeScale(pArtData->getTimeScale());
				pfftOcean->setWindDirection(pArtData->getFFTWindDirection());
				pfftOcean->setBubblesColor(pArtData->getOceanBubblesColor());
			}

			SphericalOcean* planetOcean = mSceneManager->getPlanetManager()->getCurrentOcean();
			if (planetOcean && planetOcean->getIsUseTod())
			{
				planetOcean->setOceanHeight(pArtData->getOceanHeight());
				planetOcean->setWaveFrequency(pArtData->getWaveFrequency());
				planetOcean->setWaveAmplitude(pArtData->getWaveAmplitude());
				planetOcean->setBumpScale(pArtData->getBumpScale());
				planetOcean->setRefractAmount(pArtData->getRefractAmount());
				planetOcean->setBorderSoft(pArtData->getBorderSoft());
				planetOcean->setReflectTexture(pArtData->getReflectTexName());
				planetOcean->setSpecularPower(pArtData->getSpecularPower());
				planetOcean->setSpecularStrength(pArtData->getSpecularStrength());
				planetOcean->setFoamBias(pArtData->getFoamBias());
				planetOcean->setFoamSoft(pArtData->getFoamSoft());
				planetOcean->setFoamTilling(pArtData->getFoamTilling());
				planetOcean->setUnderWaterFogDensity(pArtData->getUnderWaterFogDensity());
				planetOcean->setUnderWaterFogStart(pArtData->getUnderWaterFogStart());
				planetOcean->setUnderWaterFogEnd(pArtData->getUnderWaterFogEnd());
			}
		}
		else if (pCom->getType() == UnderWaterCausticsComponent::mType)
		{
			UnderWaterCausticsComponent* unWaterCom = static_cast<UnderWaterCausticsComponent*>(pCom);

			unWaterCom->setIsForceUnderWater(pArtData->isForceUnderWater());
			Vector4 temp = pArtData->getCausticParams();
			unWaterCom->setGlobalStartDistance(temp.x);
			unWaterCom->setGlobalEndDistance(temp.y);
			unWaterCom->setGlobalDensity(temp.z);
			unWaterCom->setGlobalDisturbStrength(temp.w);

			if (unWaterCom->m_pActComInfo)
			{
				UnderWaterCausticsComponentInfo* pInfo = static_cast<UnderWaterCausticsComponentInfo*>(unWaterCom->m_pActComInfo);
				pInfo->mCausticsHandle = unWaterCom->getContext();
			}
		}
		else if (pCom->getType() == EnvTriggerEventComponent::mType)
		{
			EnvTriggerEventComponent* pETEcom = static_cast<EnvTriggerEventComponent*>(pCom);

			if (pETEcom)
			{
				pETEcom->setRainState(pArtData->isRain());
				pETEcom->setSnowState(pArtData->isCoverSnow());

				SceneTriggerData* pTriggerData = mSceneArtTemplate->getTriggerData();
				if (pTriggerData)
				{
					std::vector<EnvTriggerEventDesc> desc;
					std::vector<String> name;
					std::vector<float> yRotateAngle;

					for (const auto& pEvent : pTriggerData->getAreaVec())
					{
						for (int i = 0; i < pEvent->getTriggerEventCount(); ++i)
						{
							EnvTriggerEventDesc* tri = pEvent->getTriggerEventDesc(i);
							desc.push_back(*tri);
							name.push_back(tri->ObjectName.toString());
							float hd = tri->YRotate.GetYawRadian();
							yRotateAngle.push_back(Math::fRad2Deg * hd);

							pETEcom->addTriggerEvent();
							pETEcom->setTriggerEventData(std::make_pair(i, *tri));
							pETEcom->resetEvent(i);
						}
					}
					pETEcom->setTriggerEventAllData(desc);
					pETEcom->setTriggerEventName(name);
					pETEcom->setYRotateAngle(yRotateAngle);
				}

				if (pETEcom->m_pActComInfo)
				{
					EnvTriggerEventComponentInfo* pInfo = static_cast<EnvTriggerEventComponentInfo*>(pETEcom->m_pActComInfo);
					pInfo->mContext = pETEcom->getContext();
				}

				TODComponentManager::instance()->setTodComIsUse(pETEcom, true);
			}
		}
		else if (pCom->getType() == CloudMeshComponent::mType)
		{
			//云海
			CloudMeshComponent* cloudMeshCom = static_cast<CloudMeshComponent*>(pCom);

			std::vector<bool>			isVisible;
			float pCloudBorderSoft = 0.f;

			std::vector<String>			m_vMeshFile;
			std::vector<String>			m_vCloudTexName;
			std::vector<float>			m_vTopHeight;
			std::vector<float>			m_vBottomHeight;
			std::vector<float>			m_vScale;
			std::vector<float>			m_vCloudUSpeed;
			std::vector<float>			m_vCloudVSpeed;
			std::vector<float>			m_vCloudUVTiling;
			std::vector<unsigned int>	m_vUseOriginalUV;
			std::vector<unsigned int>	m_vUseSunBackColor;
			std::vector<unsigned int>	m_vUseDistort;
			std::vector<float>			m_vCloudDistortSpeed;
			std::vector<float>			m_vDistortAmplitude;
			std::vector<float>			m_vDistortScale;
			std::vector<int32>			m_vRenderPriority;
			bool						m_bVisible = pArtData->getCloudSeaVisible();

			m_vMeshFile.resize(3);
			m_vCloudTexName.resize(3);
			m_vTopHeight.resize(3);
			m_vBottomHeight.resize(3);
			m_vScale.resize(3);
			m_vCloudUSpeed.resize(3);
			m_vCloudVSpeed.resize(3);
			m_vCloudUVTiling.resize(3);
			m_vUseOriginalUV.resize(3);
			m_vUseSunBackColor.resize(3);
			m_vUseDistort.resize(3);
			m_vCloudDistortSpeed.resize(3);
			m_vDistortAmplitude.resize(3);
			m_vDistortScale.resize(3);
			m_vRenderPriority.resize(3);
			isVisible.resize(3);

			auto setCloudMesh = [&](int pIndex) {

				isVisible[pIndex] = pArtData->getCloudSeaVisible();

				m_vCloudTexName[pIndex] = pArtData->getCloudSeaDiffuse(pIndex);
				m_vMeshFile[pIndex] = pArtData->getCloudSeaMeshFile(pIndex);

				Vector2 vHeight = pArtData->getCloudSeaHeight(pIndex);
				m_vTopHeight[pIndex] = vHeight.y;
				m_vBottomHeight[pIndex] = vHeight.x;

				m_vScale[pIndex] = pArtData->getCloudSeaScale(pIndex);
				m_vCloudUSpeed[pIndex] = pArtData->getCloudSeaUSpeed(pIndex);
				m_vCloudVSpeed[pIndex] = pArtData->getCloudSeaVSpeed(pIndex);
				m_vCloudUVTiling[pIndex] = pArtData->getCloudSeaTilling(pIndex);
				m_vUseOriginalUV[pIndex] = pArtData->isCloudSeaUseOriUV(pIndex) ? 1 : 0;
				m_vUseSunBackColor[pIndex] = pArtData->isCloudSeaUseSunBackColor(pIndex) ? 1 : 0;
				m_vUseDistort[pIndex] = pArtData->isCloudSeaUseDistort(pIndex) ? 1 : 0;
				m_vCloudDistortSpeed[pIndex] = pArtData->getCloudSeaDistortSpeed(pIndex);
				m_vDistortAmplitude[pIndex] = pArtData->getCloudSeaDistortAmp(pIndex);
				m_vDistortScale[pIndex] = pArtData->getCloudSeaDistortScale(pIndex);
				m_vRenderPriority[pIndex] = pArtData->getCloudSeaRenderPriority(pIndex);

				pCloudBorderSoft = pArtData->getCloudSeaBorderSoft();

				m_bVisible |= isVisible[pIndex];
			};

			setCloudMesh(0);
			setCloudMesh(1);
			setCloudMesh(2);

			cloudMeshCom->setCloudVisible(m_bVisible);
			//cloudMeshCom->setCloudVisible(isVisible);
			cloudMeshCom->setCloudBorderSoft(pCloudBorderSoft);
			cloudMeshCom->setMeshName(m_vMeshFile);
			cloudMeshCom->setTextureName(m_vCloudTexName);
			cloudMeshCom->setTopHeight(m_vTopHeight);
			cloudMeshCom->setBottomHeight(m_vBottomHeight);
			cloudMeshCom->setCloudMeshScale(m_vScale);
			cloudMeshCom->setURollSpeed(m_vCloudUSpeed);
			cloudMeshCom->setVRollSpeed(m_vCloudVSpeed);
			cloudMeshCom->setUVTilling(m_vCloudUVTiling);
			cloudMeshCom->setIsUseOriginalUV(m_vUseOriginalUV);
			cloudMeshCom->setIsUseDistort(m_vUseDistort);
			cloudMeshCom->setIsShowBackColor(m_vUseSunBackColor);
			cloudMeshCom->setDistortSpeed(m_vCloudDistortSpeed);
			cloudMeshCom->setDistortAmplitude(m_vDistortAmplitude);
			cloudMeshCom->setDistortScale(m_vDistortScale);
			cloudMeshCom->setRenderPriority(m_vRenderPriority);

			if (cloudMeshCom->m_pActComInfo)
			{
				CloudMeshComponentInfo* pInfo = static_cast<CloudMeshComponentInfo*>(cloudMeshCom->m_pActComInfo);
				pInfo->mContext = cloudMeshCom->mContext;
			}
		}
		else if (pCom->getType() == LightningComponent::mType)
		{
			LightningComponent* pLightCom = static_cast<LightningComponent*>(pCom);
			//闪电
			pLightCom->setIsUseNormalLightning(pArtData->getNormalLightningVisible());

			pLightCom->setIsUseSpecialLightning(pArtData->getlightningVisible());
			pLightCom->setLightningLifeTime(pArtData->getLightningLife());
			pLightCom->setLightningDelayed(pArtData->isLightningDelayed());
			pLightCom->setLightningInterval(pArtData->getLightningInterval());
			pLightCom->setLightningLimitYawAngle(pArtData->getLightningLimitYawAngle());
			pLightCom->setLightningLimitPitchRange(pArtData->getLightningLimitPitchRange());
			pLightCom->setLightningUseBeam(pArtData->isLightningUseBeam());
			pLightCom->setLightningColor(pArtData->getLightningColor());
			pLightCom->setLightningColorPower(pArtData->getLightningColorPower());
			pLightCom->setUseSunColor(pArtData->isLightningUseSunColor());
			pLightCom->setSunColor(pArtData->getLightningSunColor());
			pLightCom->setUseSunBackColor(pArtData->isLightningUseSunBackColor());
			pLightCom->setSunBackColor(pArtData->getLightningSunBackColour());
			pLightCom->setSunBackColorPower(pArtData->getLightningSunBackColourPower());

			pLightCom->setGlobalLightningTextureName(pArtData->getLightningTextures());
			pLightCom->setIsUseParticle(pArtData->isLightningUseParticle());
			pLightCom->setIsParticleLoop(pArtData->isLightningParticleIsLoop());
			pLightCom->setParticleOffsetRange(pArtData->getLightningParticleOffsetRange());
			pLightCom->setParticleScaleRange(pArtData->getLightningParticleScaleRange());
			pLightCom->setParticlesName(pArtData->getLightningParticles());
			pLightCom->setLightningSoundName(pArtData->getLightningSoundFiles());

			if (pLightCom->m_pActComInfo)
			{
				LightningComponentInfo* pInfo = static_cast<LightningComponentInfo*>(pLightCom->m_pActComInfo);
				pInfo->mLightningHandle = pLightCom->getContext();
			}

			TODComponentManager::instance()->setTodComIsUse(pLightCom, true);
		}
		else if (pCom->getType() == WeatherCoverColorComponent::mType)
		{
			WeatherCoverColorComponent* wCom = static_cast<WeatherCoverColorComponent*>(pCom);
			wCom->setIsUseCoverColor(pArtData->getUseCoverColor());
		}

	}
#endif
}
