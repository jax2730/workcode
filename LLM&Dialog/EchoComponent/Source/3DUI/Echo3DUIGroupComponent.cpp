// --------------------------------------------------------------------------------------
// File:   Echo3DUIGroupComponent.cpp
// 
// Author: Chen ShiWen
//
// Date:   2022-9-6
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
// --------------------------------------------------------------------------------------
#include "EchoComponentStableHeaders.h"
#include "3DUI/Echo3DUIGroupComponent.h"

#include "EchoProtocolComponent.h"
#include "EchoActorProFuncImp.h"

#include "EchocJsonHelper.h"

#include "UI/GComponent.h"

namespace Echo {


	// ------------------------------------------------ Echo3DUIGroupComponentInfo ------------------------------------------------------------ //
	Echo3DUIGroupComponentInfo::Echo3DUIGroupComponentInfo(const Name & type, ActorCOMMoveableType movable)
		: Echo3DUIBaseComponentInfo(type, movable)
	{
	}

	Echo3DUIGroupComponentInfo::~Echo3DUIGroupComponentInfo()
	{
	}

	// ------------------------------------------------ Echo3DUIGroupComponent --------------------------------------------------------------- //
	DECLAREACTORCOMTYPE_CPP(Echo3DUIGroupComponent)

	Echo3DUIGroupComponent::Echo3DUIGroupComponent(ActorPtr  p_actor, ActorCOMMoveableType movable_type)
		: Echo3DUIBaseComponent(p_actor, movable_type)
	{
        m_UI3DType = UI3D_GROUPCOMPONENT;
	}

	Echo3DUIGroupComponent::~Echo3DUIGroupComponent() {

	}

	// ----------------------------------------------------- Script -------------------------------------------------------------------- //
	//Echo3DUIGroupComponent * Echo3DUIGroupComponentScript::GetEcho3DUIGroupComponent(ActorPtr  p_actor, const std::string & name)
	//{
	//	//return p_actor ? p_actor->GetComponent<Echo3DUIGroupComponent>(Name(name)) : nullptr;
	//	return nullptr;
	//}
} /// Echo
