// --------------------------------------------------------------------------------------
// File:   Echo3DUIScrollBarComponent.cpp
// 
// Author: Chen ShiWen
//
// Date:   2022-9-6
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
// --------------------------------------------------------------------------------------
#include "EchoComponentStableHeaders.h"
#include "3DUI/Echo3DUIScrollBarComponent.h"

#include "EchoProtocolComponent.h"
#include "EchoActorProFuncImp.h"

#include "EchocJsonHelper.h"

#include "UI/GComponent.h"

namespace Echo {


	// ------------------------------------------------ Echo3DUIScrollBarComponentInfo ------------------------------------------------------------ //
	Echo3DUIScrollBarComponentInfo::Echo3DUIScrollBarComponentInfo(const Name & type, ActorCOMMoveableType movable) 
		: Echo3DUIBaseComponentInfo(type, movable)
	{
	}

	Echo3DUIScrollBarComponentInfo::~Echo3DUIScrollBarComponentInfo()
	{
	}

	// ------------------------------------------------ Echo3DUIScrollBarComponent --------------------------------------------------------------- //
	DECLAREACTORCOMTYPE_CPP(Echo3DUIScrollBarComponent)

	Echo3DUIScrollBarComponent::Echo3DUIScrollBarComponent(ActorPtr  p_actor, ActorCOMMoveableType movable_type)
		: Echo3DUIBaseComponent(p_actor, movable_type) 
	{
        m_UI3DType = UI3D_SCROLLBARCOMPONENT;
	}

	Echo3DUIScrollBarComponent::~Echo3DUIScrollBarComponent() {

	}

	// ----------------------------------------------------- Script -------------------------------------------------------------------- //
	//Echo3DUIScrollBarComponent * Echo3DUIScrollBarComponentScript::GetEcho3DUIScrollBarComponent(ActorPtr  p_actor, const std::string & name)
	//{
	//	// return p_actor ? p_actor->GetComponent<Echo3DUIScrollBarComponent>(Name(name)) : nullptr;
	//	return nullptr;
	//}

} /// Echo
