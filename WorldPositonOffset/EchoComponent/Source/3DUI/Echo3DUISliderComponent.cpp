// --------------------------------------------------------------------------------------
// File:   Echo3DUISliderComponent.cpp
// 
// Author: Chen ShiWen
//
// Date:   2022-9-6
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
// --------------------------------------------------------------------------------------
#include "EchoComponentStableHeaders.h"
#include "3DUI/Echo3DUISliderComponent.h"

#include "EchoProtocolComponent.h"
#include "EchoActorProFuncImp.h"

#include "EchocJsonHelper.h"

#include "UI/GComponent.h"

namespace Echo {


	// ------------------------------------------------ Echo3DUISliderComponentInfo ------------------------------------------------------------ //
	Echo3DUISliderComponentInfo::Echo3DUISliderComponentInfo(const Name & type, ActorCOMMoveableType movable) 
		: Echo3DUIBaseComponentInfo(type, movable)
	{
	}

	Echo3DUISliderComponentInfo::~Echo3DUISliderComponentInfo()
	{
	}

	// ------------------------------------------------ Echo3DUISliderComponent --------------------------------------------------------------- //
	DECLAREACTORCOMTYPE_CPP(Echo3DUISliderComponent)

	Echo3DUISliderComponent::Echo3DUISliderComponent(ActorPtr  p_actor, ActorCOMMoveableType movable_type) 
		: Echo3DUIBaseComponent(p_actor, movable_type)
	{
        m_UI3DType = UI3D_SLIDERCOMPONENT;
	}

	Echo3DUISliderComponent::~Echo3DUISliderComponent() {

	}

	// ----------------------------------------------------- Script -------------------------------------------------------------------- //
	//Echo3DUISliderComponent * Echo3DUISliderComponentScript::GetEcho3DUISliderComponent(ActorPtr  p_actor, const std::string & name)
	//{
	//	// return p_actor ? p_actor->GetComponent<Echo3DUISliderComponent>(Name(name)) : nullptr;
	//	return nullptr;
	//}

} /// Echo
