// --------------------------------------------------------------------------------------
// File:   Echo3DUIImageComponent.cpp
// 
// Author: Chen ShiWen
//
// Date:   2022-9-6
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
// --------------------------------------------------------------------------------------
#include "EchoComponentStableHeaders.h"
#include "3DUI/Echo3DUIImageComponent.h"

#include "EchoProtocolComponent.h"
#include "EchoActorProFuncImp.h"

#include "EchocJsonHelper.h"

#include "UI/GComponent.h"
#include "UI/GImage.h"

namespace Echo {
	// ------------------------------------------------ Echo3DUIImageComponentInfo ------------------------------------------------------------ //
	Echo3DUIImageComponentInfo::Echo3DUIImageComponentInfo(const Name & type, ActorCOMMoveableType movable)
		: Echo3DUIBaseComponentInfo(type, movable)
	{
	}

	void Echo3DUIImageComponentInfo::AfterExportData(cJSON* jsonNode)
	{
		reflect::TypeResolver<Echo3DUIImageComponentInfo>::get()->ExportJson(this, jsonNode, "");
	}

	void Echo3DUIImageComponentInfo::AfterImportData(cJSON* jsonNode)
	{
		reflect::TypeResolver<Echo3DUIImageComponentInfo>::get()->ImportJson(this, jsonNode, "");
	}

	void Echo3DUIImageComponentInfo::AfterCopyTo(ActorComponentInfo* comInfo)
	{
		Echo3DUIImageComponentInfo* pInfo = static_cast<Echo3DUIImageComponentInfo*>(comInfo);
		reflect::TypeResolver<Echo3DUIImageComponentInfo>::get()->copyTo(this, pInfo);
	}

	void Echo3DUIImageComponentInfo::AfterOverWrite() {
		reflect::TypeResolver<Echo3DUIImageComponentInfo>::get()->overWrite(mOriginComInfo, this);
	}

	EDITOR_REFLECT_BEGIN(Echo3DUIImageComponentInfo)
	EDITOR_REFLECT_MEMBER_UPDATE(mRate, "Rate", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_END();

	// ------------------------------------------------ Echo3DUIImageComponent --------------------------------------------------------------- //
	DECLAREACTORCOMTYPE_CPP(Echo3DUIImageComponent)

	Echo3DUIImageComponent::Echo3DUIImageComponent(ActorPtr  p_actor, ActorCOMMoveableType movable_type) 
		: Echo3DUIBaseComponent(p_actor, movable_type)
	{
        m_UI3DType = UI3D_IMAGECOMPONENT;
	}

	Echo3DUIImageComponent::~Echo3DUIImageComponent() {

	}

    void Echo3DUIImageComponent::AfterSetGObject()
	{
        //if(m_dummyMovable)
        //{
        //    ((UI2DRenderBase*)m_dummyMovable)->setRenderPriorityLookupIndex(m_priorityLookupIndex);
        //}

		/**
		 * @brief 应用Info中的数据
		*/
		applyRate();
	}

	void Echo3DUIImageComponent::applyRate()
	{
		//Echo3DUIImageComponentInfo* pInfo = static_cast<Echo3DUIImageComponentInfo*>(this->m_pActComInfo);
		//UI2DImage* pImageRender = static_cast<GImage*>(mGObject)->getImageRender();
		//Vector2 curRate = pInfo->getRate();
		//if (curRate != pImageRender->getBlendOffsetRate()) {
		//	pImageRender->setBlendOffsetRate(curRate);
		//}
	}

	// ----------------------------------------------------- Script -------------------------------------------------------------------- //
	//Echo3DUIImageComponent * Echo3DUIImageComponentScript::GetEcho3DUIImageComponent(ActorPtr  p_actor, const std::string & name)
	//{
	//	// return p_actor ? p_actor->GetComponent<Echo3DUIImageComponent>(Name(name)) : nullptr;
	//	return nullptr;
	//}

} /// Echo
