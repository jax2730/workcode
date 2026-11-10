// --------------------------------------------------------------------------------------
// File:   Echo3DUIProgressBarComponent.cpp
// 
// Author: Chen ShiWen
//
// Date:   2022-9-6
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
// --------------------------------------------------------------------------------------
#include "EchoComponentStableHeaders.h"
#include "3DUI/Echo3DUIProgressBarComponent.h"

#include "EchoProtocolComponent.h"
#include "EchoActorProFuncImp.h"

#include "EchocJsonHelper.h"

#include "UI/GComponent.h"

namespace Echo {


	// ------------------------------------------------ Echo3DUIProgressBarComponentInfo ------------------------------------------------------------ //
	Echo3DUIProgressBarComponentInfo::Echo3DUIProgressBarComponentInfo(const Name & type, ActorCOMMoveableType movable) 
		: Echo3DUIBaseComponentInfo(type, movable)
	{
	}

	Echo3DUIProgressBarComponentInfo::~Echo3DUIProgressBarComponentInfo()
	{
	}

	// ------------------------------------------------ Echo3DUIProgressBarComponent --------------------------------------------------------------- //
	DECLAREACTORCOMTYPE_CPP(Echo3DUIProgressBarComponent)

	Echo3DUIProgressBarComponent::Echo3DUIProgressBarComponent(ActorPtr  p_actor, ActorCOMMoveableType movable_type) 
		: Echo3DUIBaseComponent(p_actor, movable_type)
	{
        m_UI3DType = UI3D_PROGRESSBARCOMPONENT;
	}

	Echo3DUIProgressBarComponent::~Echo3DUIProgressBarComponent() {

	}

	// ----------------------------------------------------- Script -------------------------------------------------------------------- //
	//Echo3DUIProgressBarComponent * Echo3DUIProgressBarComponentScript::GetEcho3DUIProgressBarComponent(ActorPtr  p_actor, const std::string & name)
	//{
	//	// return p_actor ? p_actor->GetComponent<Echo3DUIProgressBarComponent>(Name(name)) : nullptr;
	//	return nullptr;
	//}

} /// Echo
