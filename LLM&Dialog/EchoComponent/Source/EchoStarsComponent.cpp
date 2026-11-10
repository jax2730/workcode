#include "EchoComponentStableHeaders.h"
#include "EchoStarsComponent.h"
#include "EchoStars.h"
#include "EchoTODComponentManager.h"
#include "EchoLogManager.h"
#include "EchoRenderStrategy.h"
#include "EchoPostProcess.h"
#include "EchoTODComponentSystem.h"
#include "EchoTODDataUtil.h"


namespace Echo {

	DECLAREACTORCOMTYPE_CPP(StarsComponent);

	EDITOR_REFLECT_BEGIN(StarsComponentInfo)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mStarsSize, "StarsSize", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mStarsIntensity, "StarsIntensity", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mStarsHDRIntensity, "StarsHDRIntensity", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mIsShowLowerPart, "IsShowLowerPart", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mPriority, "StarsPriority", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_END()

	Name starsSizeName_ = Name("StarsSize");
	Name starsIntensityName_ = Name("StarsIntensity");
	Name starsHDRIntensityName_ = Name("StarsHDRIntensity");

	StarsComponentInfo::StarsComponentInfo(const Name& comType, ActorCOMMoveableType type)
		:ActorComponentInfo(comType,type)
	{
	}
	void StarsComponentInfo::ExportData(cJSON* jsonNode)
	{
		reflect::TypeResolver<StarsComponentInfo>::get()->ExportJson(this, jsonNode, "");
	}
	void StarsComponentInfo::ImportData(cJSON* jsonNode)
	{
		reflect::TypeResolver<StarsComponentInfo>::get()->ImportJson(this, jsonNode, "");
	}
	void StarsComponentInfo::CopyTo(ActorComponentInfo* comInfo)
	{
		if (comInfo == nullptr)
			return;
		StarsComponentInfo* starsInfo = static_cast<StarsComponentInfo*>(comInfo);
		reflect::TypeResolver<StarsComponentInfo>::get()->copyTo(this, starsInfo);
	}
	void StarsComponentInfo::OverWrite()
	{
		if (mOriginComInfo != nullptr) 
		{
			reflect::TypeResolver<StarsComponentInfo>::get()->overWrite(mOriginComInfo, this);
		}
	}

	StarsComponent::StarsComponent(ActorPtr pActor, ActorCOMMoveableType movableType)
		: ActorComponent(pActor, movableType)
	{
#ifdef ECHO_EDITOR
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_STAR_SIZE,				starsSizeName_.toString(),			1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_STAR_INTENSITY,		starsIntensityName_.toString(),		1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_STAR_COLOR_HDR_POWER,	starsHDRIntensityName_.toString(),	1);
#endif // ECHO_EDITOR
	}

	StarsComponent::~StarsComponent()
	{
	}

	void StarsComponent::init(ActorComponentInfo* info)
	{
		TODCOMPONENT_INITMGR(getStarsContext, m_tMgr, false);

		StarsComponentInfo* starsInfo = static_cast<StarsComponentInfo*>(info);
		if (starsInfo == nullptr)
			return;
		mContext = starsInfo->mContext;
	}
	void StarsComponent::unInit()
	{
		mStars = nullptr;
		TODCOMPONENT_UNINITMGR(getStarsContext, m_tMgr, T_End);
	}
	void StarsComponent::visibleStateChanged(bool isVisible)
	{
		if (!m_tMgr)
			return;
		if (isVisible)
			m_tMgr->pushComponent(m_tMgr->getStarsContext(), this);
		else
			m_tMgr->popComponent(m_tMgr->getStarsContext(), this);
	}
	void StarsComponent::setTimelineAttribute(std::vector<float>& data,const Name& name_)
	{
		if (!m_tMgr)
			return;
		TODContext<StarsComponent, Stars>& starsContext = m_tMgr->getStarsContext();
		auto activeCom = starsContext.mActiveCom;
		if (activeCom == nullptr || activeCom != this)
			return;

		m_tMgr->setTodComChangeState(T_Stars);
		if (name_ == starsSizeName_) {
			setStarsSize(data[0]);
		}
		else if (name_ == starsIntensityName_) {
			setStarsIntensity(data[0]);
		}
		else if (name_ == starsHDRIntensityName_) {
			setStarsHDRIntensity(data[0]);
		}
	}
	void StarsComponent::use()
	{
		if (!m_tMgr)
			return;
		mStars = m_tMgr->getStars();
		if (mStars == nullptr)
			return;
		if (m_tMgr->getIsWriteLog())
		{
			std::string logInfo = "";
			logInfo.append("Stars : use").append(getCombinedName());
			LogManager::getSingleton().logMessage(Echo::LML_CRITICAL, logInfo.data());
		}
		setStarsAttribute();
		setPriority(mContext.mPriority);
		mStars->init();
	}
	void StarsComponent::unuse()
	{
		if (!m_tMgr)
			return;
		if(mStars)
		   mStars->deinit();
		mStars = nullptr;
		if (m_tMgr->getIsWriteLog())
		{
			std::string logInfo = "";
			logInfo.append("Stars : unuse").append(getCombinedName());
			LogManager::getSingleton().logMessage(Echo::LML_CRITICAL, logInfo.data());
		}
	}
	void StarsComponent::setStarsAttribute()
	{
		if (mStars == nullptr)
			return;
		mStars->setStarSize(mContext.mStarsSize);
		mStars->setStarIntensity(mContext.mStarsIntensity);
		setStarsHDRIntensity(mContext.mStarsHDRIntensity);
		mStars->setShowLowerPart(mContext.mIsShowLowerPart);
	}
	void StarsComponent::setStarsSize(float size_)
	{
		mContext.mStarsSize = size_;
		if (mStars)
		{
			mStars->setStarSize(size_);
		}
	}
	void StarsComponent::setStarsIntensity(float intensity_)
	{
		mContext.mStarsIntensity = intensity_;
		if(mStars)
		{
			mStars->setStarIntensity(intensity_);
		}
	}
	void StarsComponent::setStarsHDRIntensity(float HDRIntensity_)
	{
		mContext.mStarsHDRIntensity = HDRIntensity_;

		bool bEnableHDR = false;

		RenderSystem* pRenderSystem = Root::instance()->getRenderSystem();
		if (pRenderSystem)
		{
			RenderStrategy* pStrategy = pRenderSystem->getCurRenderStrategy();
			if (pStrategy && pStrategy->getPostProcessChain())
			{
				bEnableHDR = pStrategy->getPostProcessChain()->isEnable(PostProcess::HDR);
			}
		}
		if (mStars)
			mStars->setHDRPower(bEnableHDR ? HDRIntensity_ : 1.f);
	}
	void StarsComponent::setIsUseLowerStarsPart(bool isUse_)
	{
		mContext.mIsShowLowerPart = isUse_;
		if (mStars)
			mStars->setShowLowerPart(isUse_);
	}
	void StarsComponent::setPriority(int priority_)
	{
		mContext.mPriority = priority_;
		if(m_tMgr)
			m_tMgr->adjustComponentPriority(m_tMgr->getStarsContext(), this);
	}
}
