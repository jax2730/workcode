// --------------------------------------------------------------------------------------
// File:   Echo3DUIListComponent.cpp
// 
// Author: Chen ShiWen
//
// Date:   2022-9-6
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
// --------------------------------------------------------------------------------------
#include "EchoComponentStableHeaders.h"
#include "3DUI/Echo3DUIListComponent.h"

#include "EchoProtocolComponent.h"
#include "EchoActorProFuncImp.h"

#include "EchocJsonHelper.h"

#include "UI/GComponent.h"

namespace Echo {


	// ------------------------------------------------ Echo3DUIListComponentInfo ------------------------------------------------------------ //
	Echo3DUIListComponentInfo::Echo3DUIListComponentInfo(const Name & type, ActorCOMMoveableType movable) 
		: Echo3DUIBaseComponentInfo(type, movable)
	{
	}

	Echo3DUIListComponentInfo::~Echo3DUIListComponentInfo()
	{
	}

	// ------------------------------------------------ Echo3DUIListComponent --------------------------------------------------------------- //
	DECLAREACTORCOMTYPE_CPP(Echo3DUIListComponent)

	Echo3DUIListComponent::Echo3DUIListComponent(ActorPtr  p_actor, ActorCOMMoveableType movable_type)
		: Echo3DUIBaseComponent(p_actor, movable_type) 
	{
        m_UI3DType = UI3D_LISTCOMPONENT;
	}

	Echo3DUIListComponent::~Echo3DUIListComponent() {

	}

	// ----------------------------------------------------- Script -------------------------------------------------------------------- //
	//Echo3DUIListComponent * Echo3DUIListComponentScript::GetEcho3DUIListComponent(ActorPtr  p_actor, const std::string & name)
	//{
	//	// return p_actor ? p_actor->GetComponent<Echo3DUIListComponent>(Name(name)) : nullptr;
	//	return nullptr;
	//}

} /// Echo
