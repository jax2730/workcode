// --------------------------------------------------------------------------------------
// File:   Echo3DUILabelComponent.cpp
// 
// Author: Chen ShiWen
//
// Date:   2022-9-6
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
// --------------------------------------------------------------------------------------
#include "EchoComponentStableHeaders.h"
#include "3DUI/Echo3DUILabelComponent.h"

#include "EchoProtocolComponent.h"
#include "EchoActorProFuncImp.h"

#include "EchocJsonHelper.h"

#include "UI/GComponent.h"

namespace Echo {


	// ------------------------------------------------ Echo3DUILabelComponentInfo ------------------------------------------------------------ //
	Echo3DUILabelComponentInfo::Echo3DUILabelComponentInfo(const Name & type, ActorCOMMoveableType movable) 
		: Echo3DUIBaseComponentInfo(type, movable)
	{
	}

	Echo3DUILabelComponentInfo::~Echo3DUILabelComponentInfo()
	{
	}

	// ------------------------------------------------ Echo3DUILabelComponent --------------------------------------------------------------- //
	DECLAREACTORCOMTYPE_CPP(Echo3DUILabelComponent)

	Echo3DUILabelComponent::Echo3DUILabelComponent(ActorPtr  p_actor, ActorCOMMoveableType movable_type) 
		: Echo3DUIBaseComponent(p_actor, movable_type)
	{
        m_UI3DType = UI3D_LABELCOMPONENT;
	}

	Echo3DUILabelComponent::~Echo3DUILabelComponent() {

	}

	// ----------------------------------------------------- Script -------------------------------------------------------------------- //
	//Echo3DUILabelComponent * Echo3DUILabelComponentScript::GetEcho3DUILabelComponent(ActorPtr  p_actor, const std::string & name)
	//{
	//	// return p_actor ? p_actor->GetComponent<Echo3DUILabelComponent>(Name(name)) : nullptr;
	//	return nullptr;
	//}

} /// Echo
