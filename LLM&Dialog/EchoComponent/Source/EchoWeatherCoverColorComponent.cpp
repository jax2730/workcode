#include "EchoComponentStableHeaders.h"
#include "EchoWeatherCoverColorComponent.h"
#include "EchoWeatherManager.h"
#include "EchoTODComponentManager.h"
#include "EchoLogManager.h"
#include "EchoSceneManager.h"
#include "EchoTODComponentSystem.h"
#include "EchoTODDataUtil.h"

namespace Echo {


	EDITOR_REFLECT_BEGIN(WeatherCoverColorComponentInfo)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mCoverColor, "CoverColor", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mCoverColorIntensity, "CoverColorIntensity", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mIsUseCoverColor, "IsUseCoverColor", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mPriority, "Priority", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mOverlayPos, "OverlayPos", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mOverlayRadius, "OverlayRadius", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_END()

		DECLAREACTORCOMTYPE_CPP(WeatherCoverColorComponent)


	Name weatherCoverColorName_			= Name("WeatherCoverColor");
	Name weatherCoverColorIntensity_	= Name("WeatherCoverColorIntensity");

	WeatherCoverColorComponentInfo::WeatherCoverColorComponentInfo(const Name& comType, ActorCOMMoveableType type)
		:ActorComponentInfo(comType,type)
	{
	}
	WeatherCoverColorComponentInfo::~WeatherCoverColorComponentInfo()
	{
	}
	void WeatherCoverColorComponentInfo::ExportData(cJSON* jsonNode)
	{
		reflect::TypeResolver<WeatherCoverColorComponentInfo>::get()->ExportJson(this, jsonNode, "");
	}
	void WeatherCoverColorComponentInfo::ImportData(cJSON* jsonNode)
	{
		reflect::TypeResolver<WeatherCoverColorComponentInfo>::get()->ImportJson(this, jsonNode, "");
	}
	void WeatherCoverColorComponentInfo::CopyTo(ActorComponentInfo* comInfo)
	{
		if (comInfo == nullptr)
			return;
		WeatherCoverColorComponentInfo* coverColorInfo = static_cast<WeatherCoverColorComponentInfo*>(comInfo);
		reflect::TypeResolver<WeatherCoverColorComponentInfo>::get()->copyTo(this, coverColorInfo);

	}
	void WeatherCoverColorComponentInfo::Overlay(ActorComponentInfo* srcInfo)
	{
	}
	void WeatherCoverColorComponentInfo::OverWrite()
	{
		if (mOriginComInfo != nullptr)
		{
			reflect::TypeResolver<WeatherCoverColorComponentInfo>::get()->overWrite(mOriginComInfo, this);
		}
	}


	WeatherCoverColorComponent::WeatherCoverColorComponent(Echo::ActorPtr pActor, Echo::ActorCOMMoveableType movableType)
		:ActorComponent(pActor, movableType)
	{
#ifdef ECHO_EDITOR
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_WEATHER_COVER_COLOR,			weatherCoverColorName_.toString(),		4);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_WEATHER_COVER_COLOR_POWER,		weatherCoverColorIntensity_.toString(),	1);
#endif // ECHO_EDITOR
	}
	WeatherCoverColorComponent::~WeatherCoverColorComponent()
	{

	}

	void WeatherCoverColorComponent::init(ActorComponentInfo* info)
	{
		TODCOMPONENT_INITMGR(getWeatherCoverColorContext, m_tMgr, false);

		if (info == nullptr)
			return;
		WeatherCoverColorComponentInfo* coverColorComInfo_ = static_cast<WeatherCoverColorComponentInfo*>(info);
		mContext = coverColorComInfo_->mContext;
	}
	void WeatherCoverColorComponent::unInit()
	{
		TODCOMPONENT_UNINITMGR(getWeatherCoverColorContext, m_tMgr, T_End);
	}
	void WeatherCoverColorComponent::copyTo(const ActorComponentPtr&)
	{
	}
	void WeatherCoverColorComponent::setTimelineAttribute(std::vector<float>& data,const Name& name_)
	{
		if (!m_tMgr)
			return;
		WeatherCoverColorComponent* activeCom = m_tMgr->getWeatherCoverColorContext().mActiveCom;
		if (activeCom == nullptr || activeCom != this)
			return;
		m_tMgr->setTodComChangeState(T_WeatherCoverColor);
		if (name_ == weatherCoverColorName_)
		{
			setCoverColor(ColorValue(data[0], data[1], data[2], data[3]));
		}
		else if (name_ == weatherCoverColorIntensity_)
		{
			setCoverColorIntensity(data[0]);
		}
	}
	void WeatherCoverColorComponent::visibleStateChanged(bool isVisible)
	{
		if (!m_tMgr)
			return;
		if (isVisible)
			m_tMgr->pushComponent(m_tMgr->getWeatherCoverColorContext(), this);
		else
			m_tMgr->popComponent(m_tMgr->getWeatherCoverColorContext(), this);
	}
	void WeatherCoverColorComponent::use()
	{
		if (m_tMgr && m_tMgr->getIsWriteLog())
		{
			std::string logInfo = "";
			logInfo.append("WeatherCover : use").append(getCombinedName());
			LogManager::getSingleton().logMessage(Echo::LML_CRITICAL, logInfo.data());
		}

		setIsUseCoverColor(mContext.mIsUseCoverColor);
		setCoverColor(mContext.mCoverColor);
		setCoverColorIntensity(mContext.mCoverColorIntensity);
		//setOverlayData();
	}
	void WeatherCoverColorComponent::unuse()
	{
	}
	void WeatherCoverColorComponent::resetCoverColor()
	{
		setCoverColor(mContext.mCoverColor);
		setCoverColorIntensity(mContext.mCoverColorIntensity);
		//setOverlayData();
	}
	void WeatherCoverColorComponent::setOverlayPosition(const Vector3& position_)
	{
		mContext.mOverlayPos = position_;
		if (m_tMgr && m_tMgr->getWeatherCoverColorContext().mActiveCom == this)
			setOverlayData();
	}
	void WeatherCoverColorComponent::setOverlayRadius(float radius_)
	{
		mContext.mOverlayRadius = radius_;
		if (m_tMgr && m_tMgr->getWeatherCoverColorContext().mActiveCom == this)
			setOverlayData();
	}
	void WeatherCoverColorComponent::setOverlayData()
	{
		if (!m_tMgr)
			return;

		SceneManager* sMgr = m_tMgr->GetSceneManager();

		if (!sMgr)
			return;

		sMgr->setWeatherCoverParams(Vector4(mContext.mOverlayPos.x, mContext.mOverlayPos.y, mContext.mOverlayPos.z,mContext.mOverlayRadius));
	}
	void WeatherCoverColorComponent::setTransFormTime(float transTime_)
	{
		mContext.mTransTime = transTime_;
	
		if (m_tMgr && m_tMgr->getWeatherCoverColorContext().mActiveCom == this)
		{
			WeatherManager* wMgr = m_tMgr->GetWeatherManager();
			if (wMgr)
				wMgr->setTransTime(transTime_);
		}

	}
	void WeatherCoverColorComponent::setIsUseCoverColor(bool isUseCoverColor_)
	{
		mContext.mIsUseCoverColor = isUseCoverColor_;

		if (m_tMgr && m_tMgr->getWeatherCoverColorContext().mActiveCom == this)
		{
			WeatherManager* wMgr = m_tMgr->GetWeatherManager();
			if (wMgr)
				wMgr->setUseCoverColor(isUseCoverColor_);
		}
		
	}
	void WeatherCoverColorComponent::setCoverColor(const ColorValue& coverColor_)
	{
		mContext.mCoverColor = coverColor_;

		if (m_tMgr && m_tMgr->getWeatherCoverColorContext().mActiveCom == this)
		{
			WeatherManager* wMgr = m_tMgr->GetWeatherManager();
			if (wMgr)
				wMgr->setCoverColor(coverColor_);
		}
	}
	void WeatherCoverColorComponent::setCoverColorIntensity(float intensity_)
	{
		mContext.mCoverColorIntensity = intensity_;
		if (m_tMgr && m_tMgr->getWeatherCoverColorContext().mActiveCom == this)
		{
			WeatherManager* wMgr = m_tMgr->GetWeatherManager();
			if (wMgr)
				wMgr->setCoverColorIntensity(intensity_, intensity_);
		}
	}
	void WeatherCoverColorComponent::setPriority(int priority_)
	{
		mContext.mPriority = priority_;
		if(m_tMgr)
			m_tMgr->adjustComponentPriority(m_tMgr->getWeatherCoverColorContext(), this);
	}
}
