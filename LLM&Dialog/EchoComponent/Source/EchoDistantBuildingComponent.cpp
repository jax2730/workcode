#include "EchoComponentStableHeaders.h"
#include "EchoDistantBuildingComponent.h"

#include "EchoImposterComponent.h"
#include "EchoHLODComponent.h"
#include "EchoBuildingComponent.h"

#include "EchoVideoConfig.h"
#include "EchoVideoConfigSystem.h"
#include "EchoWorldManager.h"

namespace Echo
{
#ifdef ECHO_EDITOR
	bool DistantBuildingComponent::m_renderHelper = true;
#endif // ECHO_EDITOR

	static void _UpdataVideoConfig(ActorPtr pActor, const _db_UpdateFactorConfig& cfg)
	{
		if (cfg.mBuilding)
		{
			cfg.mBuilding->setBuildingCullMode(DefaultCull);

			cfg.mBuilding->setBuildingLodFactorLevel1(0.f);
			cfg.mBuilding->setBuildingLodFactorLevel2(0.f);
			cfg.mBuilding->setBuildingRenderFactor(0.f);
		}
		if (cfg.mHlod)
		{
			cfg.mHlod->setCullMode(DefaultCull);

			cfg.mHlod->setHLODFactor(0.f);
			cfg.mHlod->setCullFactor(0.f);
			cfg.mHlod->setDissolveFactor(0.9f);
			cfg.mHlod->resetLastHLODLength();
		}
		if (cfg.mImposter)
		{
			cfg.mImposter->SetCullMode(DefaultCull);

			cfg.mImposter->SetImposterFactor(0.f);
			cfg.mImposter->SetCullFactor(0.f);
		}
	}

	static void _UpdateDistanceFactor(ActorPtr pActor, const _db_UpdateFactorConfig& cfg)
	{
		float size = pActor->getWorldBounds().getSize().length();
		float lod1dis = cfg.mLod1Factor * size;
		float lod2dis = cfg.mLod2Factor * size;
		float hloddis = cfg.mHlodFactor * size;
		float mixfact = (cfg.mHlodFactor + ((cfg.mImposterFactor - cfg.mHlodFactor) * 0.9f)) / std::max(cfg.mImposterFactor,0.0001f);
		float imposterdis = cfg.mImposterFactor * size;
		float culldis = cfg.mCullFactor * size;

		if (cfg.mImposter == nullptr)
		{
			// 用来剔除hlod
			imposterdis = culldis; 
		}
		if (cfg.mHlod == nullptr)
		{
			// 用来剔除灰模
			hloddis = imposterdis;
		}

		// 设值
		if (cfg.mBuilding)
		{
			cfg.mBuilding->setBuildingCullMode(DistanceCull);

			cfg.mBuilding->setBuildingLodDistanceLevel1(lod1dis);
			cfg.mBuilding->setBuildingLodDistanceLevel2(lod2dis);
			cfg.mBuilding->setBuildingRenderDistance(hloddis);
		}
		if (cfg.mHlod)
		{
			cfg.mHlod->setCullMode(DistanceCull);

			cfg.mHlod->setHLODRenderDis(hloddis);
			cfg.mHlod->setRenderDistance(imposterdis);
			cfg.mHlod->setDissolveFactor(mixfact);
			cfg.mHlod->resetLastHLODLength();
		}
		if (cfg.mImposter)
		{
			cfg.mImposter->SetCullMode(DistanceCull);

			cfg.mImposter->SetEnableDistance(imposterdis);
			cfg.mImposter->SetCullDistance(culldis);
		}

	}

	static void _UpdateSamllObjectFactor(ActorPtr pActor, const _db_UpdateFactorConfig& cfg)
	{
		float lod1factor = cfg.mLod1Factor;
		float lod2factor = cfg.mLod2Factor;
		float hlodfactor = cfg.mHlodFactor;
		float mixfact = (cfg.mHlodFactor + ((cfg.mImposterFactor - cfg.mHlodFactor) * 0.9f)) / std::max(cfg.mImposterFactor, 0.00001f);
		float imposterfactor = cfg.mImposterFactor;
		float cullfactor = cfg.mCullFactor;

		if (cfg.mImposter == nullptr)
		{
			// 用来剔除hlod
			imposterfactor = cullfactor;
		}
		if (cfg.mHlod == nullptr)
		{
			// 用来剔除灰模
			hlodfactor = imposterfactor;
		}

		if (cfg.mBuilding)
		{
			cfg.mBuilding->setBuildingCullMode(DefaultCull);

			cfg.mBuilding->setBuildingLodFactorLevel1(lod1factor);
			cfg.mBuilding->setBuildingLodFactorLevel2(lod2factor);
			cfg.mBuilding->setBuildingRenderFactor(hlodfactor);
		}
		if (cfg.mHlod)
		{
			cfg.mHlod->setCullMode(DefaultCull);

			cfg.mHlod->setHLODFactor(hlodfactor);
			cfg.mHlod->setCullFactor(imposterfactor);
			cfg.mHlod->setDissolveFactor(mixfact);
			cfg.mHlod->resetLastHLODLength();
		}
		if (cfg.mImposter)
		{
			cfg.mImposter->SetCullMode(DefaultCull);
			cfg.mImposter->SetImposterFactor(imposterfactor);
			cfg.mImposter->SetCullFactor(cullfactor);
		}
	}

	static void _UpdateSamllObjectFactor(DistantBuildingComponent* disCom, _db_UpdateFactorConfig& cfg)
	{
		if (!disCom) return;
		if (!disCom->getActor()->m_IsAnimationTimelineActor) return;

		if (cfg.mUpdateKey & DistantBuildingComponent::Building)
		{
			float lod1factor = cfg.mLod1Factor;
			float lod2factor = cfg.mLod2Factor;
			float discullfactor = cfg.mHlodFactor * 0.9f;

			disCom->setLodLevel1Factor(lod1factor);
			disCom->setLodLevel2Factor(lod2factor);

			if (cfg.mBuilding)
			{
				cfg.mBuilding->setBuildingCullMode(DefaultCull);

				cfg.mBuilding->setBuildingLodFactorLevel1(lod1factor);
				cfg.mBuilding->setBuildingLodFactorLevel2(lod2factor);
				cfg.mBuilding->setBuildingRenderFactor(discullfactor);

				cfg.mUpdateKey &= ~DistantBuildingComponent::Building;
			}
		}

		if (cfg.mUpdateKey & DistantBuildingComponent::HLOD)
		{
			float hlodfactor = cfg.mHlodFactor;
			float hlodcullfactor = cfg.mImposterFactor * 0.9f;

			disCom->setHlodFactor(hlodfactor);

			if (cfg.mHlod)
			{
				cfg.mHlod->setCullMode(DefaultCull);

				cfg.mHlod->setHLODFactor(hlodfactor);
				//cfg.mHlod->setDissolveFactor(imposterfactor);
				cfg.mHlod->setCullFactor(hlodcullfactor);
				cfg.mHlod->resetLastHLODLength();

				cfg.mUpdateKey &= ~DistantBuildingComponent::HLOD;
			}
		}

		if (cfg.mUpdateKey & DistantBuildingComponent::Imposter)
		{
			float imposterfactor = cfg.mImposterFactor;
			float cullfactor = cfg.mCullFactor;

			disCom->setImposterFactor(imposterfactor);
			disCom->setCullFactor(cullfactor);

			if (cfg.mImposter)
			{
				cfg.mImposter->SetCullMode(DefaultCull);
				cfg.mImposter->SetImposterFactor(imposterfactor);
				//cfg.mImposter->SetGradientFactor(imposterfactor);
				cfg.mImposter->SetCullFactor(cullfactor);

				cfg.mUpdateKey &= ~DistantBuildingComponent::Imposter;
			}
		}
	}

	// 获得参考距离
	//static float _GetReferenceDistance(ActorPtr pActor, float factor, DistantBuildingFactorType type)
	//{
	//	if (type == DistantBuildingFactorType::ByDistance)
	//	{
	//		float size = pActor->getWorldBounds().getSize().length();
	//		return size * factor;
	//	}
	//	if (type == DistantBuildingFactorType::BySmallObject || type == DistantBuildingFactorType::ByVideoConfig)
	//	{
	//		if (factor < 0.00001f)
	//			return 0.f;

	//		float size = pActor->getLocalBounds().getHalfSize().squaredLength();
	//		return size / factor;
	//	}
	//	return 0.f;
	//}

	static const LVConfig* _GetVideoConfig(ActorPtr pActor)
	{
		VideoConfig* pVideoConfig = nullptr;
		
		if (pActor->getWorldManager())
			pVideoConfig = pActor->getWorldManager()->getVideoConfig();
		if (!pVideoConfig)
			pVideoConfig = VideoConfigSystem::instance()->getActiveVideoConfig();

		return pVideoConfig->getCurrentVideoConfig() ;
	}
// -------------------------------------------------------------------------------------------------------------------------
	DECLAREACTORCOMTYPE_CPP(DistantBuildingComponent);
	EDITOR_REFLECT_BEGIN(DistantBuildingComponentInfo)
		EDITOR_REFLECT_MEMBER_UPDATE(m_ImposterActorName, "m_ImposterActorName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_HlodActorName,	  "m_HlodActorName",	 EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_BuildingActorName, "m_BuildingActorName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)

		EDITOR_REFLECT_MEMBER_UPDATE(m_lod1Factor,		"m_lod1Factor",		EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_lod2Factor,		"m_lod2Factor",		EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_BuildCullFactor, "m_BuildCullFactor", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_HlodFactor,		"m_HlodFactor",		EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_HlodCullFactor,	"m_HlodCullFactor", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_ImposterFactor,	"m_ImposterFactor",	EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_CullFactor,		"m_CullFactor",		EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		//EDITOR_REFLECT_MEMBER_UPDATE(m_FactorType,		"m_FactorType",		EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_Enable,			"m_Enable",			EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_END();
// -------------------------------------------------------------------------------------------------------------------------
	DistantBuildingComponent::DistantBuildingComponent(ActorPtr pActor, ActorCOMMoveableType type)
		:ProtocolActorComponent(pActor, type)
	{
		pActor->setIsDistanceBuilding(true);
	}

	DistantBuildingComponent::~DistantBuildingComponent()
	{
#ifdef ECHO_EDITOR
		deleteHelper();
#endif // ECHO_EDITOR
	}
	void DistantBuildingComponent::init(ActorComponentInfo* info)
	{
		if (!info)
			return;

		ProtocolActorComponent::init(info);
		SetNotifiedUpdate(true);

		DistantBuildingComponentInfo* linfo = static_cast<DistantBuildingComponentInfo*> (info);

		m_BuildingActorName = linfo->m_BuildingActorName;
		m_ImposterActorName = linfo->m_ImposterActorName;
		m_HlodActorName = linfo->m_HlodActorName;

		m_lod1Factor = linfo->m_lod1Factor;
		m_lod2Factor = linfo->m_lod2Factor;
		m_BuildCullFactor = linfo->m_BuildCullFactor;
		m_HlodFactor = linfo->m_HlodFactor;
		m_HlodCullFactor = linfo->m_HlodCullFactor;
		m_ImposterFactor = linfo->m_ImposterFactor;
		m_CullFactor = linfo->m_CullFactor;
		//setFactorType(linfo->m_FactorType);
		m_Enable = linfo->m_Enable;
	}

	void DistantBuildingComponent::unInit()
	{
		ProtocolActorComponent::unInit();
		SetNotifiedUpdate(false);
	}

	void DistantBuildingComponent::registerTickTimeBegin()
	{
		m_eventSet[TT_BeginFrame].insert(EID_EveryFrame);
		registerTickTimeEnd();
	}

	void DistantBuildingComponent::beginFrame(EventID eventid, ProtocolHandle handle)
	{

		switch (eventid)
		{
		case EID_EveryFrame:
		{
#ifdef ECHO_EDITOR
			if (!m_renderHelper)
				deleteHelper();
			else
			{
				createHelper();

				AxisAlignedBox curAABB = getActor()->getWorldBounds();
				float boxLen = curAABB.getHalfSize().length();
				auto  pCamera = Root::instance()->getMainSceneManager()->getMainCamera();
				float cameraLen = std::sqrt(curAABB.SquareDistPointToAABB(pCamera->getPosition()));
				m_CameraFactor = std::pow(boxLen / cameraLen, 2.f);

				if (m_helperBox->getName().toString().find(getCombinedName()) != String::npos)
					m_helperBox->setName(Name(getCombinedName().append("||").append("DistantBuildingBoxObject")));

				if (m_CameraFactor >= getLodLevel1Factor(true))
					m_helperBox->setHelperColorType(HelperObjColorType_Red);
				else if (m_CameraFactor >= getLodLevel2Factor(true))
					m_helperBox->setHelperColorType(helperObjColorType_Yellow);
				else if (m_CameraFactor >= getHlodFactor(true))
					m_helperBox->setHelperColorType(HelperObjColorType_Green);
				else if (m_CameraFactor >= getImposterFactor(true))
					m_helperBox->setHelperColorType(HelperObjColorType_Blue);
				else if (m_CameraFactor >= getCullFactor(true))
					m_helperBox->setHelperColorType(HelperObjColorType_black);
			}
#endif // ECHO_EDITOR

			if (m_factorConfig.mUpdateKey)
			{
				if (m_factorConfig.mUpdateKey & DistantBuildingComponent::Building)
					m_factorConfig.mBuilding = getBuildingComponent();
				if (m_factorConfig.mUpdateKey & DistantBuildingComponent::HLOD)
					m_factorConfig.mHlod = getHLODComponent();
				if (m_factorConfig.mUpdateKey & DistantBuildingComponent::Imposter)
					m_factorConfig.mImposter = getImposterComponent();

				_UpdateSamllObjectFactor(this, m_factorConfig);
			}
		}
		default:
			break;
		}

		
	}

	void DistantBuildingComponent::EditorStopImp()
	{
#ifdef ECHO_EDITOR
		SAFE_DELETE(m_helperBox)
#endif // ECHO_EDITOR
	}

	void DistantBuildingComponent::NotifiChildImp(int changeMask)
	{
#ifdef ECHO_EDITOR
		updateHelper();
#endif
	}

	void DistantBuildingComponent::showEditorHelper(bool isShow)
	{
#ifdef ECHO_EDITOR
		m_renderHelper = isShow;
#endif // ECHO_EDITOR
	}

	//void DistantBuildingComponent::connectActor()
	//{
	//	UpdateFactor();
	//}

	void DistantBuildingComponent::setImposterActor(ActorPtr pActor)
	{
		m_ImposterActorName.clear();
		if (pActor.Expired())
			return;
		ActorPtr thisActor = getActor();
		m_ImposterActorName = thisActor->getRelativeNameFromAbsoluteName(pActor->getCombinedID());
	}

	void DistantBuildingComponent::setHlodActor(ActorPtr pActor)
	{
		m_HlodActorName.clear();
		if (pActor.Expired())
			return;
		ActorPtr thisActor = getActor();
		m_HlodActorName = thisActor->getRelativeNameFromAbsoluteName(pActor->getCombinedID());
	}

	void DistantBuildingComponent::setBuildingActor(ActorPtr pActor)
	{
		m_BuildingActorName.clear();
		if (pActor.Expired())
			return;
		ActorPtr thisActor = getActor();
		m_BuildingActorName = thisActor->getRelativeNameFromAbsoluteName(pActor->getCombinedID());
	}

	void DistantBuildingComponent::setImposterActor(const std::string& relationActor)
	{
		m_ImposterActorName = relationActor;
	}

	void DistantBuildingComponent::setHlodActor(const std::string& relationActor)
	{
		m_HlodActorName = relationActor;
	}

	void DistantBuildingComponent::setBuildingActor(const std::string& relationActor)
	{
		m_BuildingActorName = relationActor;
	}

	ActorPtr DistantBuildingComponent::getImposterActor()
	{
		ActorPtr thisActor = getActor();
		if (m_ImposterActorName.empty())
			return nullptr;
		return thisActor->getActorComByRelativeName(m_ImposterActorName).seleteActor;
	}

	ImposterComponent* DistantBuildingComponent::getImposterComponent()
	{
		ActorPtr lActor = getImposterActor();
		if (lActor == nullptr)
			return nullptr;

		return lActor->GetFirstComByType<ImposterComponent>().get();
	}

	ActorPtr DistantBuildingComponent::getHlodActor()
	{
		ActorPtr thisActor = getActor();
		if (m_HlodActorName.empty())
			return nullptr;
		return thisActor->getActorComByRelativeName(m_HlodActorName).seleteActor;
	}

	HLODComponent* DistantBuildingComponent::getHLODComponent()
	{
		ActorPtr lActor = getHlodActor();
		if (lActor == nullptr)
			return nullptr;
		return lActor->GetFirstComByType<HLODComponent>().get();
	}

	ActorPtr DistantBuildingComponent::getBuildingActor()
	{
		ActorPtr thisActor = getActor();
		if (m_BuildingActorName.empty())
			return nullptr;
		return thisActor->getActorComByRelativeName(m_BuildingActorName).seleteActor;
	}

	BuildingComponent* DistantBuildingComponent::getBuildingComponent()
	{
		ActorPtr lActor = getBuildingActor();
		if (lActor == nullptr)
			return nullptr;
		return lActor->GetFirstComByType<BuildingComponent>().get();
	}

	void DistantBuildingComponent::setLodLevel1Factor(float fact)
	{
		m_lod1Factor = fact;
		// UpdateFactor();
	}

	void DistantBuildingComponent::setLodLevel2Factor(float fact)
	{
		m_lod2Factor = fact;
		// UpdateFactor();
	}

	void DistantBuildingComponent::setHlodFactor(float fact)
	{
		m_HlodFactor = fact;
		// UpdateFactor();
		m_BuildCullFactor = fact * 0.9f;
	}

	void DistantBuildingComponent::setImposterFactor(float fact)
	{
		m_ImposterFactor = fact;
		// UpdateFactor();
		m_HlodCullFactor = fact * 0.9f;
	}
	void DistantBuildingComponent::setCullFactor(float fact)
	{
		m_CullFactor = fact;
		// UpdateFactor();
	}
	void DistantBuildingComponent::setBuildCullFactor(float fact)
	{
		m_BuildCullFactor = fact;
	}
	void DistantBuildingComponent::setHlodCullFactor(float fact)
	{
		m_HlodCullFactor = fact;
	}

	//void DistantBuildingComponent::setFactorType(int type)
	//{
	//	if (type < 0 || type >= (int)DistantBuildingFactorType::Count)
	//		m_FactorType = DistantBuildingFactorType::ByVideoConfig;
	//	else 
	//		m_FactorType = (DistantBuildingFactorType)type;
	//}
	void DistantBuildingComponent::enableFactor(bool enable)
	{
		m_Enable = enable;
		// UpdateFactor();
	}

	void DistantBuildingComponent::showFactor(bool visit)
	{
		m_ShowFactor = visit;
	}
	//void DistantBuildingComponent::UpdateFactor()
	//{
	//	if (!m_Enable)
	//		return;
	//	ActorPtr thisActor = getActor();
	//	_db_UpdateFactorConfig cfg;
	//	{
	//		cfg.mBuilding = getBuildingComponent();
	//		cfg.mHlod = getHlodComponent();
	//		cfg.mImposter = getImposterComponent();

	//		cfg.mLod1Factor = m_lod1Factor;
	//		cfg.mLod2Factor = m_lod2Factor;
	//		cfg.mHlodFactor = m_HlodFactor;
	//		cfg.mImposterFactor = m_ImposterFactor;
	//		cfg.mCullFactor = m_CullFactor;
	//	}

	//	//if (m_FactorType == DistantBuildingFactorType::ByVideoConfig)
	//	//{
	//	//	_UpdataVideoConfig(thisActor, cfg);
	//	//}
	//	//else if (m_FactorType == DistantBuildingFactorType::ByDistance)
	//	//{
	//	//	_UpdateDistanceFactor(thisActor, cfg);
	//	//}
	//	//else if (m_FactorType == DistantBuildingFactorType::BySmallObject)
	//	//{
	//	_UpdateSamllObjectFactor(thisActor, cfg);
	//	//}
	//}
// -------------------------------------------------------------------------------------------------------------------------
	DistantBuildingComponentInfo::DistantBuildingComponentInfo(const Name& type, ActorCOMMoveableType moveable)
		:ActorComponentInfo(type, moveable)
	{
	}
	DistantBuildingComponentInfo::~DistantBuildingComponentInfo()
	{
	}
	_DEFINEINFO_VIRFUN_BASE(DistantBuildingComponentInfo);

	VideoConfig* DistantBuildingComponent::getVideoConfig()
	{
		VideoConfig* pVideoConfig = nullptr;
		if (getWorldManager())
			pVideoConfig = getWorldManager()->getVideoConfig();
		if (!pVideoConfig)
			pVideoConfig = VideoConfigSystem::instance()->getActiveVideoConfig();
		return pVideoConfig;
	}

	float DistantBuildingComponent::getLodLevel1Factor(bool isReal)
	{
		if (isReal && m_lod1Factor <= std::numeric_limits<float>::epsilon())
		{
			if (m_realLod1Factor <= std::numeric_limits<float>::epsilon())
			{
				VideoConfig* pVideoConfig = getVideoConfig();
				if (pVideoConfig)
					m_realLod1Factor = pVideoConfig->getCurrentVideoConfig()->renderLOD1factor;
			}
			return m_realLod1Factor;
		}
			
		return m_lod1Factor;
	}

	float DistantBuildingComponent::getLodLevel2Factor(bool isReal)
	{
		if (isReal && m_lod2Factor <= std::numeric_limits<float>::epsilon())
		{
			if (m_realLod2Factor <= std::numeric_limits<float>::epsilon())
			{
				VideoConfig* pVideoConfig = getVideoConfig();
				if (pVideoConfig)
					m_realLod2Factor = pVideoConfig->getCurrentVideoConfig()->renderLOD2factor;
			}
			return m_realLod2Factor;
		}
		return m_lod2Factor;
	}

	float DistantBuildingComponent::getBuildCullFactor()
	{
		return m_BuildCullFactor;
	}

	float DistantBuildingComponent::getHlodFactor(bool isReal)
	{
		if (isReal && m_HlodFactor <= std::numeric_limits<float>::epsilon())
		{
			if (m_realHlodFactor <= std::numeric_limits<float>::epsilon())
			{
				VideoConfig* pVideoConfig = getVideoConfig();
				if (pVideoConfig)
					m_realHlodFactor = pVideoConfig->getCurrentVideoConfig()->renderHLODfactor;
			}
			return m_realHlodFactor;
		}
		return m_HlodFactor;
	}

	float DistantBuildingComponent::getHlodCullFactor()
	{
		return m_HlodCullFactor;
	}

	float DistantBuildingComponent::getImposterFactor(bool isReal)
	{
		if (isReal && m_ImposterFactor <= std::numeric_limits<float>::epsilon())
		{
			if (m_realImposterFactor <= std::numeric_limits<float>::epsilon())
			{
				VideoConfig* pVideoConfig = getVideoConfig();
				if (pVideoConfig)
					m_realImposterFactor = pVideoConfig->getCurrentVideoConfig()->renderImposterfactor;
			}
			return m_realImposterFactor;
		}
		return m_ImposterFactor;
	}

	float DistantBuildingComponent::getCullFactor(bool isReal)
	{
		if (isReal && m_CullFactor <= std::numeric_limits<float>::epsilon())
		{
			if (m_realCullFactor <= std::numeric_limits<float>::epsilon())
			{
				VideoConfig* pVideoConfig = getVideoConfig();
				if (pVideoConfig)
					m_realCullFactor = pVideoConfig->getCurrentVideoConfig()->cullImposterfactor;
			}
			return m_realCullFactor;
		}
		return m_CullFactor;
	}

	//DistantBuildingFactorType DistantBuildingComponent::getFactorType()
	//{
	//	return m_FactorType;
	//}

	bool DistantBuildingComponent::isEnableFactor()
	{
		return m_Enable;
	}

	bool DistantBuildingComponent::isShowFactor()
	{
		return m_ShowFactor;
	}

	void DistantBuildingComponent::setClientFactorConfig(const _db_UpdateFactorConfig& data)
	{
		_db_UpdateFactorConfig fcData = data;
		fcData.mLod1Factor *= fcData.mLod1Factor;
		fcData.mLod2Factor *= fcData.mLod2Factor;
		fcData.mHlodFactor *= fcData.mHlodFactor;
		fcData.mImposterFactor *= fcData.mImposterFactor;
		fcData.mCullFactor *= fcData.mCullFactor;
		//setFactorConfig(fcData);
		m_factorConfig = fcData;
	}

#ifdef ECHO_EDITOR
	void DistantBuildingComponent::createHelper()
	{
		if (!getActor()) return;
		
		if (!m_helperBox)
		{
			AxisAlignedBox curAABB = getActor()->getWorldBounds();
			m_helperBox = new HelperObject(Name(getCombinedName().append("||").append("DistantBuildingBoxObject")));
			m_helperBox->setHelperColorType(HelperObjColorType_white);
			m_helperBox->setRenderType(ECHO_LINE_STRIP);
			m_helperBox->setVisible(true);

			updateHelper();

			/*if (m_pParentNodeCom)
				m_pParentNodeCom->UpdateComRootNode(true);*/
		}
	}

	void DistantBuildingComponent::updateHelper()
	{
		if (m_helperBox)
		{
			if (getActor())
			{
				AxisAlignedBox curAABB = getActor()->getWorldBounds();
				if (m_helperBox->getSceneNode())
				{
					m_helperBox->getSceneNode()->_setDerivedScale(curAABB.getSize());
					m_helperBox->getSceneNode()->_setDerivedPosition(curAABB.getCenter());
					m_helperBox->getSceneNode()->_update(true);
				}
			}
			if (getImposterComponent())
				m_helperBox->SetRanderingDistance(getImposterComponent()->getFactorCullLength());
			else if (getHLODComponent())
				m_helperBox->SetRanderingDistance(getHLODComponent()->getFactorCullLength());
			else if (getBuildingComponent())
				m_helperBox->SetRanderingDistance(getBuildingComponent()->getFactorCullLength());
			else
				m_helperBox->setVisible(false);
		}
	}

	void DistantBuildingComponent::deleteHelper()
	{
		SAFE_DELETE(m_helperBox);
	}
#endif // ECHO_EDITOR


	/*float DistantBuildingComponent::getLodLevel1ReferenceDistance()
	{
		ActorPtr thisActor = getActor();
		if (thisActor.Expired())
			return 0.f;

		if (m_FactorType == DistantBuildingFactorType::ByVideoConfig)
		{
			const LVConfig* pcfg = _GetVideoConfig(thisActor);
			if (!pcfg)
				return 0.f;
			return _GetReferenceDistance(thisActor, pcfg->renderLOD1factor, m_FactorType);
		}
		return _GetReferenceDistance(thisActor, m_lod1Factor, m_FactorType);
	}

	float DistantBuildingComponent::getLodLevel2ReferenceDistance()
	{
		ActorPtr thisActor = getActor();
		if (thisActor.Expired())
			return 0.f;

		if (m_FactorType == DistantBuildingFactorType::ByVideoConfig)
		{
			const LVConfig* pcfg = _GetVideoConfig(thisActor);
			if (!pcfg)
				return 0.f;
			return _GetReferenceDistance(thisActor, pcfg->renderLOD2factor, m_FactorType);
		}
		return _GetReferenceDistance(thisActor, m_lod2Factor, m_FactorType);
	}

	float DistantBuildingComponent::getHlodReferenceDistance()
	{
		ActorPtr thisActor = getActor();
		if (thisActor.Expired())
			return 0.f;

		if (m_FactorType == DistantBuildingFactorType::ByVideoConfig)
		{
			const LVConfig* pcfg = _GetVideoConfig(thisActor);
			if (!pcfg)
				return 0.f;
			return _GetReferenceDistance(thisActor, pcfg->renderHLODfactor, m_FactorType);
		}
		return _GetReferenceDistance(thisActor, m_HlodFactor, m_FactorType);
	}

	float DistantBuildingComponent::getImposterReferenceDistance()
	{
		ActorPtr thisActor = getActor();
		if (thisActor.Expired())
			return 0.f;
		if (m_FactorType == DistantBuildingFactorType::ByVideoConfig)
		{
			const LVConfig* pcfg = _GetVideoConfig(thisActor);
			if (!pcfg)
				return 0.f;
			return _GetReferenceDistance(thisActor, pcfg->renderImposterfactor, m_FactorType);
		}
		return _GetReferenceDistance(thisActor, m_ImposterFactor, m_FactorType);
	}

	float DistantBuildingComponent::getCullReferenceDistance()
	{
		ActorPtr thisActor = getActor();
		if (thisActor.Expired())
			return 0.f;
		if (m_FactorType == DistantBuildingFactorType::ByVideoConfig)
		{
			const LVConfig* pcfg = _GetVideoConfig(thisActor);
			if (!pcfg)
				return 0.f;
			return _GetReferenceDistance(thisActor, pcfg->cullImposterfactor, m_FactorType);
		}
		return _GetReferenceDistance(thisActor, m_CullFactor, m_FactorType);
	}*/
}
