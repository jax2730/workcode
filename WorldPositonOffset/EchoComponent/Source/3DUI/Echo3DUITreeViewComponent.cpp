// --------------------------------------------------------------------------------------
// File:   Echo3DUITreeViewComponent.cpp
// 
// Author: Chen ShiWen
//
// Date:   2022-9-6
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
// --------------------------------------------------------------------------------------
#include "EchoComponentStableHeaders.h"
#include "3DUI/Echo3DUITreeViewComponent.h"

#include "EchoProtocolComponent.h"
#include "EchoActorProFuncImp.h"

#include "EchocJsonHelper.h"

#include "UI/GComponent.h"

namespace Echo {

	// ------------------------------------------------ Echo3DUITreeViewComponentInfo ------------------------------------------------------------ //
	Echo3DUITreeViewComponentInfo::Echo3DUITreeViewComponentInfo(const Name & type, ActorCOMMoveableType movable)
		: Echo3DUIBaseComponentInfo(type, movable)
	{
	}

	Echo3DUITreeViewComponentInfo::~Echo3DUITreeViewComponentInfo()
	{
	}

	// ------------------------------------------------ Echo3DUITreeViewComponent --------------------------------------------------------------- //
	DECLAREACTORCOMTYPE_CPP(Echo3DUITreeViewComponent)

	Echo3DUITreeViewComponent::Echo3DUITreeViewComponent(ActorPtr  p_actor, ActorCOMMoveableType movable_type) 
		: Echo3DUIBaseComponent(p_actor, movable_type) 
	{
        m_UI3DType = UI3D_TREEVIEWCOMPONENT;
	}

	Echo3DUITreeViewComponent::~Echo3DUITreeViewComponent() {

	}

	// ----------------------------------------------------- Script -------------------------------------------------------------------- //
	//Echo3DUITreeViewComponent * Echo3DUITreeViewComponentScript::GetEcho3DUITreeViewComponent(ActorPtr  p_actor, const std::string & name)
	//{
	//	// return p_actor ? p_actor->GetComponent<Echo3DUITreeViewComponent>(Name(name)) : nullptr;
	//	return nullptr;
	//}

} /// Echo
