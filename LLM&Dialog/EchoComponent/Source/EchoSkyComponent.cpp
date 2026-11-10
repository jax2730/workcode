// --------------------------------------------------------------------------------------
// File:   EchoSkyComponent.cpp
// 
// Author: Chen ShiWen
//
// Date:   2022-12-7
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
// --------------------------------------------------------------------------------------
#include "EchoComponentStableHeaders.h"
#include "EchoSkyComponent.h"

#include "EchoSky.h"
#include "EchoRenderStrategy.h"
#include "EchoPostProcess.h"
#include "EchoTODComponentManager.h"
#include "EchoTODComponentSystem.h"
#include "EchoLogManager.h"
#include "EchoTODDataUtil.h"

namespace Echo {

	DECLAREACTORCOMTYPE_CPP(SkyComponent);

	Name skyTopColorName_ = Name("TopColor");
	Name skyBottomColorName_ = Name("BottomColor");
	Name skyColourRatioName_ = Name("ColourRatio");
	Name skyFogHeightName_ = Name("FogHeight");
	Name skyTopColorName_w = Name("BrightIntensity");
	Name skyBottomColorName_w = Name("BrightRange");

	SkyComponent::SkyComponent(ActorPtr pActor, ActorCOMMoveableType movableType)
		: ActorComponent(pActor, movableType)
	{
#ifdef ECHO_EDITOR
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_SKY_BOTTOM_COLOR,		skyBottomColorName_.toString(),		4);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_SKY_TOP_COLOR,			skyTopColorName_.toString(),		4);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_SKY_COLOR_RATIO,		skyColourRatioName_.toString(),		1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_SKY_FOG_HEIGHT,		skyFogHeightName_.toString(),		1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_SKY_BRIGHT_RANGE,		skyBottomColorName_w.toString(),	1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_SKY_BRIGHT_INTENSITY,	skyTopColorName_w.toString(),		1);
#endif // ECHO_EDITOR
	}

	SkyComponent::~SkyComponent()
	{
	}
	
	void SkyComponent::init(ActorComponentInfo* info) 
	{
		SkyComponentInfo* comInfo = static_cast<SkyComponentInfo*>(info);
		if (comInfo != nullptr)
			mContext = comInfo->mContext;
		TODCOMPONENT_INITMGR(getSkyContext, m_tMgr, true);
	}

	void SkyComponent::unInit()
	{
		TODCOMPONENT_UNINITMGR(getSkyContext, m_tMgr, T_Sky);
	}

	void SkyComponent::use()
	{
		if (!m_tMgr)
			return;
		m_Sky = m_tMgr->getSky();
		if (m_Sky == nullptr)
			return;
		if (m_tMgr->getIsWriteLog())
		{
			std::string logInfo = "";
			logInfo.append("Sky : use").append(getCombinedName());
			LogManager::getSingleton().logMessage(Echo::LML_CRITICAL, logInfo.data());
		}
		setBottomColor(mContext.getBottomColor());
		setTopColor(mContext.getTopColor());
		setFogHeight(mContext.getFogHeight());
		setColourRatio(mContext.getColourRatio());
		setPGSkyEnable(mContext.getPGSkyEnable());
		setPGSkyIndex(mContext.getPGSkyIndex());
	}

	void SkyComponent::unuse()
	{
		m_Sky = nullptr;
		
		if (m_tMgr && m_tMgr->getIsWriteLog())
		{
			std::string logInfo = "";
			logInfo.append("Sky : unuse").append(getCombinedName());
			LogManager::getSingleton().logMessage(Echo::LML_CRITICAL, logInfo.data());
		}
	}

	void SkyComponent::setTopColor(const ColorValue & topColor)
	{
		if (m_Sky)
			m_Sky->setSkyTopColour(topColor);
		mContext.setTopColor(topColor);
	}

	void SkyComponent::setBottomColor(const ColorValue & bottomColor)
	{
		if (m_Sky)
			m_Sky->setSkyBottomColour(bottomColor);
		mContext.setBottomColor(bottomColor);
	}

	void SkyComponent::setColourRatio(float colourRatio)
	{
		if (m_Sky)
			m_Sky->setSkyColourRatio(colourRatio);
	
		mContext.setColourRatio(colourRatio);
	}

	void SkyComponent::setFogHeight(float fogHeight)
	{
		if (m_Sky)
			m_Sky->setSkyFogHeight(fogHeight);
		mContext.setFogHeight(fogHeight);
	}

	void SkyComponent::setPriority(int priority)
	{
		mContext.setPriority(priority);
		if(m_tMgr)
			m_tMgr->adjustComponentPriority(m_tMgr->getSkyContext(), this);
	}

	void SkyComponent::setPGSkyEnable(bool enable){
		if (m_Sky)
		{
			if (enable)
				m_Sky->useProgramGeneratedSky(mContext.mPGSkyIndex, false);
			else
				m_Sky->useProgramGeneratedSky(-1, false);
		}
		mContext.setPGSkyEnable(enable);
	}

	void SkyComponent::setPGSkyIndex(int index) {
		if (m_Sky && mContext.getPGSkyEnable())
		{
			m_Sky->useProgramGeneratedSky(index, false);
		}
		mContext.setPGSkyIndex(index);
	}

	void SkyComponent::setAnimeSpeed(float speed)
	{
		if (m_Sky)
			m_Sky->setAnimeSpeed(speed);
		mContext.setAnimeSpeed(speed);
	}

	void SkyComponent::setLayer1Color(const ColorValue& color)
	{
		if (m_Sky)
			m_Sky->setLayer1Color(color);
		mContext.setLayer1Color(color);
	}

	void SkyComponent::setLayer2Color(const ColorValue& color)
	{
		if (m_Sky)
			m_Sky->setLayer2Color(color);
		mContext.setLayer2Color(color);
	}

	void SkyComponent::setTimelineAttribute(std::vector<float>& data, const Name& name_) {

		if (!m_tMgr)
			return;
		TODContext<SkyComponent, Sky>& skyContext = m_tMgr->getSkyContext();
		auto activeCom = skyContext.mActiveCom;
		if (activeCom == nullptr || activeCom != this)
			return;

		m_tMgr->setTodComChangeState(T_Sky);
		if (name_ == skyTopColorName_) {
			this->setTopColor(ColorValue(data[0], data[1], data[2], data[3]));
		}
		else if (name_ == skyBottomColorName_) {
			this->setBottomColor(ColorValue(data[0],data[1],data[2],data[3]));
		}
		else if (name_ == skyColourRatioName_) {
			this->setColourRatio(data[0]);
		}
		else if (name_ == skyFogHeightName_) {
			this->setFogHeight(data[0]);
		}
		else if (name_ == skyTopColorName_w)
		{
			ColorValue v = mContext.getTopColor();
			this->setTopColor(ColorValue(v[0], v[1], v[2], data[0]));
		}
		else if (name_ == skyBottomColorName_w)
		{
			ColorValue v = mContext.getBottomColor();
			this->setBottomColor(ColorValue(v[0], v[1], v[2], data[0]));
		}
	}

	void SkyComponent::visibleStateChanged(bool isVisible)
	{
		if (!m_tMgr)
			return;
		if (isVisible)
			m_tMgr->pushComponent(m_tMgr->getSkyContext(), this);
		else
			m_tMgr->popComponent(m_tMgr->getSkyContext(), this);
	}

	// ---------------------------------------------------------------------------------------
	EDITOR_REFLECT_BEGIN(SkyComponentInfo)
	EDITOR_REFLECT_MEMBER_UPDATE(mContext.mTopColor, "TopColor", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mContext.mBottomColor, "BottomColor", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mContext.mColourRatio, "ColourRatio", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mContext.mFogHeight, "FogHeight", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mContext.mPriority, "Priority", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mContext.mTransFormTime, "TransTime", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mContext.mEnablePGSky, "PGSkyEnable", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mContext.mPGSkyIndex, "PGSkyIndex", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_END();

	SkyComponentInfo::SkyComponentInfo(const Name & comType, ActorCOMMoveableType type)
		: ActorComponentInfo(comType, type)
	{
	}

	void SkyComponentInfo::ExportData(cJSON * jsonNode)
	{
		reflect::TypeResolver<SkyComponentInfo>::get()->ExportJson(this, jsonNode, "");
	}

	void SkyComponentInfo::ImportData(cJSON * jsonNode)
	{
		reflect::TypeResolver<SkyComponentInfo>::get()->ImportJson(this, jsonNode, "");
	}

	void SkyComponentInfo::CopyTo(ActorComponentInfo * comInfo)
	{
		if (comInfo == nullptr)
			return;
		SkyComponentInfo* targetInfo = static_cast<SkyComponentInfo*>(comInfo);
		reflect::TypeResolver<SkyComponentInfo>::get()->copyTo(this, targetInfo);
	}

	void SkyComponentInfo::OverWrite() {
		if (mOriginComInfo != nullptr) {
			reflect::TypeResolver<SkyComponentInfo>::get()->overWrite(mOriginComInfo, this);
		}
	}
}
