// --------------------------------------------------------------------------------------
// File:   Echo3DUISWFComponent.cpp
// 
// Author: Chen ShiWen
//
// Date:   2022-9-6
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
// --------------------------------------------------------------------------------------
#include "EchoComponentStableHeaders.h"
#include "3DUI/Echo3DUISWFComponent.h"

#include "EchoProtocolComponent.h"
#include "EchoActorProFuncImp.h"

#include "EchocJsonHelper.h"

#include "UI/GComponent.h"

namespace Echo {


	// ------------------------------------------------ Echo3DUISWFComponentInfo ------------------------------------------------------------ //
	Echo3DUISWFComponentInfo::Echo3DUISWFComponentInfo(const Name & type, ActorCOMMoveableType movable) 
		: Echo3DUIBaseComponentInfo(type, movable)
	{
	}

	Echo3DUISWFComponentInfo::~Echo3DUISWFComponentInfo()
	{
	}

	// ------------------------------------------------ Echo3DUISWFComponent --------------------------------------------------------------- //
	DECLAREACTORCOMTYPE_CPP(Echo3DUISWFComponent)

	Echo3DUISWFComponent::Echo3DUISWFComponent(ActorPtr  p_actor, ActorCOMMoveableType movable_type) 
		: Echo3DUIBaseComponent(p_actor, movable_type)
	{
        m_UI3DType = UI3D_SWFCOMPONENT;
	}

	Echo3DUISWFComponent::~Echo3DUISWFComponent() {

	}

	// ----------------------------------------------------- Script -------------------------------------------------------------------- //
	//Echo3DUISWFComponent * Echo3DUISWFComponentScript::GetEcho3DUISWFComponent(ActorPtr  p_actor, const std::string & name)
	//{
	//	// return p_actor ? p_actor->GetComponent<Echo3DUISWFComponent>(Name(name)) : nullptr;
	//	return nullptr;
	//}

} /// Echo
