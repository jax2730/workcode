// --------------------------------------------------------------------------------------
// File:   Echo3DUINodeComponent.cpp
// 
// Author: Chen ShiWen
//
// Date:   2022-9-6
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
// --------------------------------------------------------------------------------------
#include "EchoComponentStableHeaders.h"
#include "3DUI/Echo3DUINodeComponent.h"

#include "EchoProtocolComponent.h"
#include "EchoActorProFuncImp.h"

#include "EchocJsonHelper.h"

#include "UI/GComponent.h"

namespace Echo {


	// ------------------------------------------------ Echo3DUINodeComponentInfo ------------------------------------------------------------ //
	Echo3DUINodeComponentInfo::Echo3DUINodeComponentInfo(const Name & type, ActorCOMMoveableType movable) 
		: Echo3DUIBaseComponentInfo(type, movable)
	{
	}

	Echo3DUINodeComponentInfo::~Echo3DUINodeComponentInfo()
	{
	}

	// ------------------------------------------------ Echo3DUINodeComponent --------------------------------------------------------------- //
	DECLAREACTORCOMTYPE_CPP(Echo3DUINodeComponent)

	Echo3DUINodeComponent::Echo3DUINodeComponent(ActorPtr  p_actor, ActorCOMMoveableType movable_type) 
		: Echo3DUIBaseComponent(p_actor, movable_type)
	{
        m_UI3DType = UI3D_NODECOMPONENT;
	}

	Echo3DUINodeComponent::~Echo3DUINodeComponent() {

	}

	// ----------------------------------------------------- Script -------------------------------------------------------------------- //
	//Echo3DUINodeComponentScript::Echo3DUINodeComponentScript(const EchoActorScript& pActor, const std::string& name) : 
	//	Echo3DUIBaseComponentScript(pActor, name)
	//{
	//}

	//Echo3DUINodeComponentScript Echo3DUINodeComponentScript::GetEcho3DUINodeComponent(const EchoActorScript& pActor, const std::string& name)
	//{
	//	return Echo3DUINodeComponentScript(pActor, name);
	//}

} /// Echo
