#include "EchoComponentStableHeaders.h"
#include "EchoFogComponent.h"
#include "EchoTODVariableInfo.h"
#include "EchoSceneManager.h"
#include "EchoTODComponentManager.h"
#include "EchoTODComponentSystem.h"
#include "EchoTODDataUtil.h"

namespace Echo {
	
	DECLAREACTORCOMTYPE_CPP(FogComponent)

		EDITOR_REFLECT_BEGIN(FogComponentInfo)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mFirstNearFogColor, "FirstNearFogColor", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mFirstFarFogColor, "FirstFarFogColor", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mSecondNearFogColor, "SecondNearFogColor", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mSecondFarFogColor, "SecondFarFogColor", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mThirdNearFogColor, "ThirdNearFogColor", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mThirdFarFogColor, "ThirdFarFogColor", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mFourthNearFogColor, "FourthNearFogColor", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mFourthFarFogColor, "FourthFarFogColor", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mNearFogColorIntensity, "NearFogColorIntensity", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mFarFogColorIntensity, "FarFogColorIntensity", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mNearDensity, "NearDensity", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mFarDensity, "FarDensity", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mNearTopHeight, "NearTopHeight", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mFarTopHeight, "FarTopHeight", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mNearRampStart, "NearRampStart", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mFarRampStart, "FarRampStart", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mNearRampEnd, "NearRampEnd", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mFarRampEnd, "FarRampEnd", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mPriority, "Priority", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mTransFormTime, "TransTime", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_END()

    Name firstNearColorName = Name("FirstNearFogColor");
	Name firstNearColorPowerName = Name("FirstNearFogColorPower");
    Name firstFarColorName = Name("FirstFarFogColor");
	Name firstFarColorPowerName = Name("FirstFarFogColorPower");
	
	Name secondNearColorName = Name("SecondNearFogColor");
	Name secondNearColorPowerName = Name("SecondNearFogColorPower");
	Name secondFarColorName = Name("SecondFarFogColor");
	Name secondFarColorPowerName = Name("SecondFarFogColorPower");

	Name thirdNearColorName = Name("ThirdNearFogColor");
	Name thirdNearColorPowerName = Name("ThirdNearFogColorPower");
    Name thirdFarColorName = Name("ThirdFarFogColor");
	Name thirdFarColorPowerName = Name("ThirdFarFogColorPower");

	Name fourthNearColorName = Name("FourthNearFogColor");
	Name fourthNearColorPowerName = Name("FourthNearFogColorPower");
	Name fourthFarColorName = Name("FourthFarFogColor");
	Name fourthFarColorPowerName = Name("FourthFarFogColorPower");

	Name nearDensityFirstName = Name("NearDensity0");
	Name nearDensitySecondName = Name("NearDensity1");
	Name nearDensityThirdName = Name("NearDensity2");
	Name nearDensityFourthName = Name("NearDensity3");

	Name farDensityFirstName = Name("FarDensity0");
	Name farDensitySecondName = Name("FarDensity1");
	Name farDensityThirdName = Name("FarDensity2");
	Name farDensityFourthName = Name("FarDensity3");

	Name nearTopHeightFirstName = Name("NearTopHeight0");
	Name nearTopHeightSecondName = Name("NearTopHeight1");
	Name nearTopHeightThirdName = Name("NearTopHeight2");
	Name nearTopHeightFourthName = Name("NearTopHeight3");

	Name farTopHeightFirstName = Name("FarTopHeight0");
	Name farTopHeightSecondName = Name("FarTopHeight1");
	Name farTopHeightThirdName = Name("FarTopHeight2");
	Name farTopHeightFourthName = Name("FarTopHeight3");

	Name nearRampStartFirstName = Name("NearRampStart0");
	Name nearRampStartSecondName = Name("NearRampStart1");
	Name nearRampStartThirdName = Name("NearRampStart2");
	Name nearRampStartFourthName = Name("NearRampStart3");

	Name farRampStartFirstName = Name("FarRampStart0");
	Name farRampStartSecondName = Name("FarRampStart1");
	Name farRampStartThirdName = Name("FarRampStart2");
	Name farRampStartFourthName = Name("FarRampStart3");

	Name nearRampEndFirstName = Name("NearRampEnd0");
	Name nearRampEndSecondName = Name("NearRampEnd1");
	Name nearRampEndThirdName = Name("NearRampEnd2");
	Name nearRampEndFourthName = Name("NearRampEnd3");

	Name farRampEndFirstName = Name("FarRampEnd0");
	Name farRampEndSecondName = Name("FarRampEnd1");
	Name farRampEndThirdName = Name("FarRampEnd2");
	Name farRampEndFourthName = Name("FarRampEnd3");



	FogComponentInfo::FogComponentInfo(const Name& comType, ActorCOMMoveableType type)
		:ActorComponentInfo(comType, type)
	{
	}
	FogComponentInfo::~FogComponentInfo()
	{
	}
	void FogComponentInfo::ExportData(cJSON * jsonNode)
	{
		reflect::TypeResolver<FogComponentInfo>::get()->ExportJson(this, jsonNode, "");
	}
	void FogComponentInfo::ImportData(cJSON * jsonNode)
	{
		reflect::TypeResolver<FogComponentInfo>::get()->ImportJson(this, jsonNode, "");
	}
	void FogComponentInfo::CopyTo(ActorComponentInfo * comInfo)
	{
		if (comInfo == nullptr)
			return;
		FogComponentInfo* fogInfo = static_cast<FogComponentInfo*>(comInfo);
		reflect::TypeResolver<FogComponentInfo>::get()->copyTo(this, fogInfo);
	}
	void FogComponentInfo::Overlay(ActorComponentInfo* srcInfo)
	{
	}
	void FogComponentInfo::OverWrite()
	{
		if (mOriginComInfo != nullptr)
		{
			reflect::TypeResolver<FogComponentInfo>::get()->overWrite(mOriginComInfo, this);
		}
	}
	FogComponent::FogComponent(ActorPtr pActor, ActorCOMMoveableType movableType)
		: ActorComponent(pActor, movableType)
	{	
#ifdef ECHO_EDITOR
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_FOG_0_COLOR,			firstNearColorName.toString(),			4);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_FOG_0_COLOR_POWER,		firstNearColorPowerName.toString(),		1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_FOG_0_RAMP_START,		nearRampStartFirstName.toString(),		1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_FOG_0_RAMP_END,		nearRampEndFirstName.toString(),		1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_FOG_0_GLOBAL_DENSITY,	nearDensityFirstName.toString(),		1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_FOG_0_HEIGHT,			nearTopHeightFirstName.toString(),		1);

		TODDataUtil::SetTodTemplate(TodDataID::PARAM_FOG_0_COLOR2,			firstFarColorName.toString(),			4);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_FOG_0_COLOR_POWER2,	firstFarColorPowerName.toString(),		1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_FOG_0_RAMP_START2,		farRampStartFirstName.toString(),		1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_FOG_0_RAMP_END2,		farRampEndFirstName.toString(),			1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_FOG_0_GLOBAL_DENSITY2,	farDensityFirstName.toString(),			1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_FOG_0_HEIGHT2,			farTopHeightFirstName.toString(),		1);


		TODDataUtil::SetTodTemplate(TodDataID::PARAM_FOG_1_COLOR,			secondNearColorName.toString(),			4);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_FOG_1_COLOR_POWER,		secondNearColorPowerName.toString(),	1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_FOG_1_RAMP_START,		nearRampStartSecondName.toString(),		1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_FOG_1_RAMP_END,		nearRampEndSecondName.toString(),		1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_FOG_1_GLOBAL_DENSITY,	nearDensitySecondName.toString(),		1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_FOG_1_HEIGHT,			nearTopHeightSecondName.toString(),		1);

		TODDataUtil::SetTodTemplate(TodDataID::PARAM_FOG_1_COLOR2,			secondFarColorName.toString(),			4);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_FOG_1_COLOR_POWER2,	secondFarColorPowerName.toString(),		1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_FOG_1_RAMP_START2,		farRampStartSecondName.toString(),		1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_FOG_1_RAMP_END2,		farRampEndSecondName.toString(),		1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_FOG_1_GLOBAL_DENSITY2,	farDensitySecondName.toString(),		1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_FOG_1_HEIGHT2,			farTopHeightSecondName.toString(),		1);

		TODDataUtil::SetTodTemplate(TodDataID::PARAM_FOG_2_COLOR,			thirdNearColorName.toString(),			4);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_FOG_2_COLOR_POWER,		thirdNearColorPowerName.toString(),		1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_FOG_2_RAMP_START,		nearRampStartThirdName.toString(),		1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_FOG_2_RAMP_END,		nearRampEndThirdName.toString(),		1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_FOG_2_GLOBAL_DENSITY,	nearDensityThirdName.toString(),		1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_FOG_2_HEIGHT,			nearTopHeightThirdName.toString(),		1);

		TODDataUtil::SetTodTemplate(TodDataID::PARAM_FOG_2_COLOR2,			thirdFarColorName.toString(),			4);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_FOG_2_COLOR_POWER2,	thirdFarColorPowerName.toString(),		1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_FOG_2_RAMP_START2,		farRampStartThirdName.toString(),		1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_FOG_2_RAMP_END2,		farRampEndThirdName.toString(),			1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_FOG_2_GLOBAL_DENSITY2,	farDensityThirdName.toString(),			1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_FOG_2_HEIGHT2,			farTopHeightThirdName.toString(),		1);

		TODDataUtil::SetTodTemplate(TodDataID::PARAM_FOG_3_COLOR,			fourthNearColorName.toString(),			4);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_FOG_3_COLOR_POWER,		fourthNearColorPowerName.toString(),	1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_FOG_3_RAMP_START,		nearRampStartFourthName.toString(),		1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_FOG_3_RAMP_END,		nearRampEndFourthName.toString(),		1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_FOG_3_GLOBAL_DENSITY,	nearDensityFourthName.toString(),		1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_FOG_3_HEIGHT,			nearTopHeightFourthName.toString(),		1);

		TODDataUtil::SetTodTemplate(TodDataID::PARAM_FOG_3_COLOR2,			fourthFarColorName.toString(),			4);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_FOG_3_COLOR_POWER2,	fourthFarColorPowerName.toString(),		1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_FOG_3_RAMP_START2,		farRampStartFourthName.toString(),		1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_FOG_3_RAMP_END2,		farRampEndFourthName.toString(),		1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_FOG_3_GLOBAL_DENSITY2,	farDensityFourthName.toString(),		1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_FOG_3_HEIGHT2,			farTopHeightFourthName.toString(),		1);
#endif // ECHO_EDITOR
	}

	FogComponent::~FogComponent()
	{
	}

	void FogComponent::init(ActorComponentInfo * info)
	{
		Name sceneName = g_MainActorManagerName;
		if (m_pActor)
		{
		    m_sMgr = m_pActor->GetSceneManager();
			if (m_sMgr)
				sceneName = m_sMgr->getName();
		}
		m_tMgr = TODComponentSystem::instance()->getTODComponentManager(sceneName);
		if (m_tMgr && getRealVisibleState())
			m_tMgr->addFog(this);

		if (info != nullptr)
        {
           FogComponentInfo* mFogInfo = static_cast<FogComponentInfo*>(info);
		   mContext = mFogInfo->mContext;
        }
		setFirstNearFogColor();
		setFirstFarFogColor();
		setSecondFarFogColor();
		setSecondNearFogColor();
		setThirdNearFogColor();
		setThirdFarFogColor();
		setFourthNearFogColor();
		setFourthFarFogColor();
		setNearDensity(mContext.mNearDensity);
		setFarDensity(mContext.mFarDensity);
		setNearRampStart(mContext.mNearRampStart);
		setFarRampStart(mContext.mFarRampStart);
		setNearRampEnd(mContext.mNearRampEnd);
		setFarRampEnd(mContext.mFarRampEnd);
		setNearTopHeight(mContext.mNearTopHeight);
		setFarTopHeight(mContext.mFarTopHeight);
	}
	void FogComponent::unInit()
	{
		if (!m_tMgr)
			return;
		m_tMgr->deleteFog(this);
		m_tMgr->clearComInFinalComVec(this, T_Fog);
		m_tMgr = nullptr;
	}
	void FogComponent::copyTo(const ActorComponentPtr& newObj)
	{
		
	}
	void FogComponent::setTimelineAttribute(std::vector<float>& data, const Name& name_)
	{
		if (!m_tMgr ||  m_tMgr->getFogIsUse() == false)
			return;
		if (m_tMgr->getCurrActiveFogCom() != this)
			return;

		m_tMgr->setTodComChangeState(T_Fog);
		if (name_ == firstNearColorName)
		{
			mContext.mFirstNearFogColor = ColorValue(data[0], data[1], data[2], data[3]);
			setFirstNearFogColor();
		}
		else if (name_ == firstNearColorPowerName)
		{
			mContext.mNearFogColorIntensity.x = data[0];
			setFirstNearFogColor();
		}
		else if (name_ == firstFarColorName)
		{
			mContext.mFirstFarFogColor = ColorValue(data[0], data[1], data[2], data[3]);
			setFirstFarFogColor();
		}
		else if (name_ == firstFarColorPowerName)
		{
			mContext.mFarFogColorIntensity.x = data[0];
			setFirstFarFogColor();
		}
		else if (name_ == secondNearColorName)
		{
			mContext.mSecondNearFogColor = ColorValue(data[0], data[1], data[2], data[3]);
			setSecondNearFogColor();
		}
		else if (name_ == secondNearColorPowerName)
		{
			mContext.mNearFogColorIntensity.y = data[0];
			setSecondNearFogColor();
		}
		else if (name_ == secondFarColorName)
		{
			mContext.mSecondFarFogColor = ColorValue(data[0], data[1], data[2], data[3]);
			setSecondFarFogColor();
		}
		else if (name_ == secondFarColorPowerName)
		{
			mContext.mFarFogColorIntensity.y = data[0];
			setSecondFarFogColor();
		}
		else if (name_ == thirdNearColorName)
		{
			mContext.mThirdNearFogColor = ColorValue(data[0], data[1], data[2], data[3]);
			setThirdNearFogColor();
		}
		else if (name_ == thirdNearColorPowerName)
		{
			mContext.mNearFogColorIntensity.z = data[0];
			setThirdNearFogColor();
		}
		else if (name_ == thirdFarColorName)
		{
			mContext.mThirdFarFogColor = ColorValue(data[0], data[1], data[2], data[3]);
			setThirdFarFogColor();
		}
		else if (name_ == thirdFarColorPowerName)
		{
			mContext.mFarFogColorIntensity.z = data[0];
			setThirdFarFogColor();
		}
		else if (name_ == fourthNearColorName)
		{
			mContext.mFourthNearFogColor = ColorValue(data[0], data[1], data[2], data[3]);
			setFourthNearFogColor();
		}
		else if (name_ == fourthNearColorPowerName)
		{
			mContext.mNearFogColorIntensity.w = data[0];
			setFourthNearFogColor();
		}
		else if (name_ == fourthFarColorName)
		{
			mContext.mFourthFarFogColor = ColorValue(data[0], data[1], data[2], data[3]);
			setFourthFarFogColor();
		}
		else if (name_ == fourthFarColorPowerName)
		{
			mContext.mFarFogColorIntensity.w = data[0];
			setFourthFarFogColor();
		}

		else if (name_ == nearDensityFirstName)
		{
			float* pNearDensity = mContext.mNearDensity.ptr();
			*(pNearDensity) = data[0];
			setNearDensity(mContext.mNearDensity);
		}
		else if (name_ == nearDensitySecondName)
		{
			float* pNearDensity = mContext.mNearDensity.ptr();
			*(pNearDensity + 1) = data[0];
			setNearDensity(mContext.mNearDensity);
		}
		else if (name_ == nearDensityThirdName)
		{
			float* pNearDensity = mContext.mNearDensity.ptr();
			*(pNearDensity + 2) = data[0];
			setNearDensity(mContext.mNearDensity);
		}
		else if (name_ == nearDensityFourthName)
		{
			float* pNearDensity = mContext.mNearDensity.ptr();
			*(pNearDensity + 3) = data[0];
			setNearDensity(mContext.mNearDensity);
		}

		else if (name_ == farDensityFirstName)
		{
			float* pFarDensity = mContext.mFarDensity.ptr();
			*(pFarDensity) = data[0];
			setFarDensity(mContext.mFarDensity);
		}
		else if (name_ == farDensitySecondName)
		{
			float* pFarDensity = mContext.mFarDensity.ptr();
			*(pFarDensity + 1) = data[0];
			setFarDensity(mContext.mFarDensity);
		}
		else if (name_ == farDensityThirdName)
		{
			float* pFarDensity = mContext.mFarDensity.ptr();
			*(pFarDensity + 2) = data[0];
			setFarDensity(mContext.mFarDensity);
		}
		else if (name_ == farDensityFourthName)
		{
			float* pFarDensity = mContext.mFarDensity.ptr();
			*(pFarDensity + 3) = data[0];
			setFarDensity(mContext.mFarDensity);
		}
		else if (name_ == nearTopHeightFirstName)
		{
			float* pNearTopHeight = mContext.mNearTopHeight.ptr();
			*(pNearTopHeight ) = data[0];
			setNearTopHeight(mContext.mNearTopHeight);
		}
		else if (name_ == nearTopHeightSecondName)
		{
			float* pNearTopHeight = mContext.mNearTopHeight.ptr();
			*(pNearTopHeight + 1) = data[0];
			setNearTopHeight(mContext.mNearTopHeight);
		}
		else if (name_ == nearTopHeightThirdName)
		{
			float* pNearTopHeight = mContext.mNearTopHeight.ptr();
			*(pNearTopHeight + 2) = data[0];
			setNearTopHeight(mContext.mNearTopHeight);
		}
		else if (name_ == nearTopHeightFourthName)
		{
			float* pNearTopHeight = mContext.mNearTopHeight.ptr();
			*(pNearTopHeight + 3) = data[0];
			setNearTopHeight(mContext.mNearTopHeight);
		}
		else if (name_ == farTopHeightFirstName)
		{
			float* pFarTopHeight = mContext.mFarTopHeight.ptr();
			*(pFarTopHeight ) = data[0];
			setFarTopHeight(mContext.mFarTopHeight);
		}
		else if (name_ == farTopHeightSecondName)
		{
			float* pFarTopHeight = mContext.mFarTopHeight.ptr();
			*(pFarTopHeight + 1) = data[0];
			setFarTopHeight(mContext.mFarTopHeight);
		}
		else if (name_ == farTopHeightThirdName)
		{
			float* pFarTopHeight = mContext.mFarTopHeight.ptr();
			*(pFarTopHeight + 2) = data[0];
			setFarTopHeight(mContext.mFarTopHeight);
		}
		else if (name_ == farTopHeightFourthName)
		{
			float* pFarTopHeight = mContext.mFarTopHeight.ptr();
			*(pFarTopHeight + 3) = data[0];
			setFarTopHeight(mContext.mFarTopHeight);
		}
		else if (name_ == nearRampStartFirstName)
		{
			float* pNearRampStart = mContext.mNearRampStart.ptr();
			*(pNearRampStart ) = data[0];
			setNearRampStart(mContext.mNearRampStart);
		}
		else if (name_ == nearRampStartSecondName)
		{
			float* pNearRampStart = mContext.mNearRampStart.ptr();
			*(pNearRampStart + 1) = data[0];
			setNearRampStart(mContext.mNearRampStart);
		}
		else if (name_ == nearRampStartThirdName)
		{
			float* pNearRampStart = mContext.mNearRampStart.ptr();
			*(pNearRampStart + 2) = data[0];
			setNearRampStart(mContext.mNearRampStart);
		}
		else if (name_ == nearRampStartFourthName)
		{
			float* pNearRampStart = mContext.mNearRampStart.ptr();
			*(pNearRampStart + 3) = data[0];
			setNearRampStart(mContext.mNearRampStart);
		}
		else if (name_ == farRampStartFirstName)
		{
			float* pFarRampStart = mContext.mFarRampStart.ptr();
			*(pFarRampStart ) = data[0];
			setFarRampStart(mContext.mFarRampStart);
		}
		else if (name_ == farRampStartSecondName)
		{
			float* pFarRampStart = mContext.mFarRampStart.ptr();
			*(pFarRampStart + 1) = data[0];
			setFarRampStart(mContext.mFarRampStart);
		}
		else if (name_ == farRampStartThirdName)
		{
			float* pFarRampStart = mContext.mFarRampStart.ptr();
			*(pFarRampStart + 2) = data[0];
			setFarRampStart(mContext.mFarRampStart);
		}
		else if (name_ == farRampStartFourthName)
		{
			float* pFarRampStart = mContext.mFarRampStart.ptr();
			*(pFarRampStart + 3) = data[0];
			setFarRampStart(mContext.mFarRampStart);
		}
		else if (name_ == nearRampEndFirstName)
		{
			float* pNearRampEnd = mContext.mNearRampEnd.ptr();
			*(pNearRampEnd) = data[0];
			setNearRampEnd(mContext.mNearRampEnd);
		}
		else if (name_ == nearRampEndSecondName)
		{
			float* pNearRampEnd = mContext.mNearRampEnd.ptr();
			*(pNearRampEnd + 1) = data[0];
			setNearRampEnd(mContext.mNearRampEnd);
		}
		else if (name_ == nearRampEndThirdName)
		{
			float* pNearRampEnd = mContext.mNearRampEnd.ptr();
			*(pNearRampEnd + 2) = data[0];
			setNearRampEnd(mContext.mNearRampEnd);
		}
		else if (name_ == nearRampEndFourthName)
		{
			float* pNearRampEnd = mContext.mNearRampEnd.ptr();
			*(pNearRampEnd + 3) = data[0];
			setNearRampEnd(mContext.mNearRampEnd);
		}

		else if (name_ == farRampEndFirstName)
		{
			float* pFarRampEnd = mContext.mFarRampEnd.ptr();
			*(pFarRampEnd ) = data[0];
			setFarRampEnd(mContext.mFarRampEnd);
		}
		else if (name_ == farRampEndSecondName)
		{
			float* pFarRampEnd = mContext.mFarRampEnd.ptr();
			*(pFarRampEnd + 1) = data[0];
			setFarRampEnd(mContext.mFarRampEnd);
		}
		else if (name_ == farRampEndThirdName)
		{
			float* pFarRampEnd = mContext.mFarRampEnd.ptr();
			*(pFarRampEnd + 2) = data[0];
			setFarRampEnd(mContext.mFarRampEnd);
		}
		else if (name_ == farRampEndFourthName)
		{
			float* pFarRampEnd = mContext.mFarRampEnd.ptr();
			*(pFarRampEnd + 3) = data[0];
			setFarRampEnd(mContext.mFarRampEnd);
		}
		
	}
	void FogComponent::visibleStateChanged(bool isVisible)
	{
		if (!m_tMgr)
			return;
		if (isVisible)
			m_tMgr->addFog(this);
		else
			m_tMgr->deleteFog(this);
	}
	void FogComponent::setInitFirstNearFogColor(const ColorValue& color_)
	{
		mContext.mFirstNearFogColor = color_;
		setFirstNearFogColor();
	}
	void FogComponent::setInitFirstNearFogPower(float power_)
	{
		mContext.mNearFogColorIntensity.x = power_;
		setFirstNearFogColor();
	}
	void FogComponent::setInitFirstFarFogColor(const ColorValue& color_)
	{
		mContext.mFirstFarFogColor = color_;
		setFirstFarFogColor();
	}
	void FogComponent::setInitFirstFarFogPower(float power_)
	{
		mContext.mFarFogColorIntensity.x = power_;
		setFirstFarFogColor();
	}
	void FogComponent::setInitSecondNearFogColor(const ColorValue& color_)
	{
		mContext.mSecondNearFogColor = color_;
		setSecondNearFogColor();
	}
	void FogComponent::setInitSecondNearFogPower(float power_)
	{
		mContext.mNearFogColorIntensity.y = power_;
		setSecondNearFogColor();
	}
	void FogComponent::setInitSecondFarFogColor(const ColorValue& color_)
	{
		mContext.mSecondFarFogColor = color_;
		setSecondFarFogColor();
	}
	void FogComponent::setInitSecondFarFogPower(float power_)
	{
		mContext.mFarFogColorIntensity.y = power_;
		setSecondFarFogColor();
	}
	void FogComponent::setInitThirdNearFogColor(const ColorValue& color_)
	{
		mContext.mThirdNearFogColor = color_;
		setThirdNearFogColor();
	}
	void FogComponent::setInitThirdNearFogPower(float power_)
	{
		mContext.mNearFogColorIntensity.z = power_;
		setThirdNearFogColor();
	}
	void FogComponent::setInitThirdFarFogColor(const ColorValue& color_)
	{
		mContext.mThirdFarFogColor = color_;
		setThirdFarFogColor();
	}
	void FogComponent::setInitThirdFarFogPower(float power_)
	{
		mContext.mFarFogColorIntensity.z = power_;
		setThirdFarFogColor();
	}
	void FogComponent::setInitFourthNearFogColor(const ColorValue& color_)
	{
		mContext.mFourthNearFogColor = color_;
		setFourthNearFogColor();
	}
	void FogComponent::setInitFourthNearFogPower(float power_)
	{
		mContext.mNearFogColorIntensity.w = power_;
		setFourthNearFogColor();
	}
	void FogComponent::setInitFourthFarFogColor(const ColorValue& color_)
	{
		mContext.mFourthFarFogColor = color_;
		setFourthFarFogColor();
	}
	void FogComponent::setInitFourthFarFogPower(float power_)
	{
		mContext.mFarFogColorIntensity.w = power_;
		setFourthFarFogColor();
	}
	void FogComponent::setFirstNearFogColor()
	{
		mFogColor[0] = ColorValue::gammaToLinear(mContext.mFirstNearFogColor) * mContext.mNearFogColorIntensity.x;
		if (m_tMgr && m_tMgr->getCurrActiveFogCom() == this)
			setMainSceneFog();
	}
	void FogComponent::setFirstFarFogColor()
	{
		mFogColor[4] = ColorValue::gammaToLinear(mContext.mFirstFarFogColor) * mContext.mFarFogColorIntensity.x;
		if (m_tMgr &&  m_tMgr->getCurrActiveFogCom() == this)
			setMainSceneFog();
	}
	void FogComponent::setSecondNearFogColor()
	{
		mFogColor[1] = ColorValue::gammaToLinear(mContext.mSecondNearFogColor) * mContext.mNearFogColorIntensity.y;
		if (m_tMgr &&  m_tMgr->getCurrActiveFogCom() == this)
			setMainSceneFog();
	}
	void FogComponent::setSecondFarFogColor()
	{
		mFogColor[5] = ColorValue::gammaToLinear(mContext.mSecondFarFogColor) * mContext.mFarFogColorIntensity.y;
		if (m_tMgr &&  m_tMgr->getCurrActiveFogCom() == this)
			setMainSceneFog();
	}
	void FogComponent::setThirdNearFogColor()
	{
		mFogColor[2] = ColorValue::gammaToLinear(mContext.mThirdNearFogColor) * mContext.mNearFogColorIntensity.z;
		if ( m_tMgr &&  m_tMgr->getCurrActiveFogCom() == this)
			setMainSceneFog();
	}
	void FogComponent::setThirdFarFogColor()
	{
		mFogColor[6] = ColorValue::gammaToLinear(mContext.mThirdFarFogColor) * mContext.mFarFogColorIntensity.z;
		if ( m_tMgr &&  m_tMgr->getCurrActiveFogCom() == this)
			setMainSceneFog();
	}
	void FogComponent::setFourthNearFogColor()
	{
		mFogColor[3] = ColorValue::gammaToLinear(mContext.mFourthNearFogColor) * mContext.mNearFogColorIntensity.w;
		if ( m_tMgr &&  m_tMgr->getCurrActiveFogCom() == this)
			setMainSceneFog();
	}
	void FogComponent::setFourthFarFogColor()
	{
		mFogColor[7] = ColorValue::gammaToLinear(mContext.mFourthFarFogColor) * mContext.mFarFogColorIntensity.w;
		if ( m_tMgr && m_tMgr->getCurrActiveFogCom() == this)
			setMainSceneFog();
	}
	void FogComponent::setNearDensity(const Vector4& nearDensity_)
	{
		mContext.mNearDensity = nearDensity_;
		float* pNearDensity_ = mContext.mNearDensity.ptr();
		for (int i = 0; i < 4; i++)
			mDensity[i] = *(pNearDensity_ + i);
		
		if (m_tMgr && m_tMgr->getCurrActiveFogCom() == this)
			setMainSceneFog();
	}
	void FogComponent::setFarDensity(const Vector4& farDensity_)
	{
		mContext.mFarDensity = farDensity_;
		float* pFarDensity_ = mContext.mFarDensity.ptr();
		for (int i = 4; i < 8; i++)
			mDensity[i] = *(pFarDensity_ + i - 4);
		if (m_tMgr && m_tMgr->getCurrActiveFogCom() == this)
			setMainSceneFog();
	}
	void FogComponent::setNearTopHeight(const Vector4& nearTopHeight_)
	{
		mContext.mNearTopHeight = nearTopHeight_;
		float* pNearTopHeight_ = mContext.mNearTopHeight.ptr();
		for (int i = 0; i < 4; i++)
			mTopHeight[i] = *(pNearTopHeight_+i);
		if (m_tMgr &&  m_tMgr->getCurrActiveFogCom() == this)
			setMainSceneFog();
	}
	void FogComponent::setFarTopHeight(const Vector4& farTopHeight_)
	{
		mContext.mFarTopHeight = farTopHeight_;
		float* pFarTopHeight_ = mContext.mFarTopHeight.ptr();
		for (int i = 4; i < 8; i++)
			mTopHeight[i] = *(pFarTopHeight_ + i - 4);
		
		if (m_tMgr && m_tMgr->getCurrActiveFogCom() == this)
			setMainSceneFog();
	}
	void FogComponent::setNearRampStart(const Vector4& nearRampStart_)
	{
		mContext.mNearRampStart = nearRampStart_;
		float* pNearRampStart_ = mContext.mNearRampStart.ptr();
		for (int i = 0; i < 4; i++)
			mRampStart[i] = *(pNearRampStart_ + i);
		
		if (m_tMgr && m_tMgr->getCurrActiveFogCom() == this)
			setMainSceneFog();
	}
	void FogComponent::setFarRampStart(const Vector4& farRampStart_)
	{
		mContext.mFarRampStart = farRampStart_;
		float* pFarRampStart_ = mContext.mFarRampStart.ptr();
		for (int i = 4; i < 8; i++)
			mRampStart[i] = *(pFarRampStart_ + i - 4);

		if (m_tMgr && m_tMgr->getCurrActiveFogCom() == this)
			setMainSceneFog();
	}
	void FogComponent::setNearRampEnd(const Vector4& nearRampEnd_)
	{
		mContext.mNearRampEnd = nearRampEnd_;
		float* pNearRampEnd_ = mContext.mNearRampEnd.ptr();
		for (int i = 0; i < 4; i++)
			mRampEnd[i] = *(pNearRampEnd_ + i);
		
		if (m_tMgr && m_tMgr->getCurrActiveFogCom() == this)
			setMainSceneFog();
	}
	void FogComponent::setFarRampEnd(const Vector4& farRampEnd_)
	{
		mContext.mFarRampEnd = farRampEnd_;
		float* pFarRampEnd_ = mContext.mFarRampEnd.ptr();
		for (int i = 4; i < 8; i++)
			mRampEnd[i] = *(pFarRampEnd_ + i - 4);
		
		if (m_tMgr && m_tMgr->getCurrActiveFogCom() == this)
			setMainSceneFog();
	}
	void FogComponent::setMainSceneFog()
	{
		if (!m_tMgr || !m_tMgr->getFogIsUse() || !m_sMgr)
			return;
		FogMode modeType_ = m_sMgr->getFogMode();
		m_sMgr->setFog(modeType_, mFogColor, mDensity, mRampStart, mRampEnd, mTopHeight);
	}
	void FogComponent::setRealTime(bool isUse_)
	{
		if(m_tMgr)
		   m_tMgr->setFogRealTime(isUse_,this);
	}
	bool   FogComponent::getRealTime()
	{
		if (!m_tMgr)
			return false;
		return m_tMgr->getFogRealTime(this);
	}

	int FogComponent::getPriority()
	{
		return mContext.mPriority;
	}
	void FogComponent::setPriority(int priority_)
	{
		mContext.mPriority = priority_;
		if(m_tMgr)
		   m_tMgr->FogPriorityChanged(this);
	}
	
};
