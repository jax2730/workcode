// --------------------------------------------------------------------------------------
// File:   Echo3DUIGraphComponent.cpp
// 
// Author: Chen ShiWen
//
// Date:   2022-9-6
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
// --------------------------------------------------------------------------------------
#include "EchoComponentStableHeaders.h"
#include "3DUI/Echo3DUIGraphComponent.h"

#include "EchoProtocolComponent.h"
#include "EchoActorProFuncImp.h"

#include "EchocJsonHelper.h"

#include "UI/GComponent.h"

namespace Echo {


	// ------------------------------------------------ Echo3DUIGraphComponentInfo ------------------------------------------------------------ //
	Echo3DUIGraphComponentInfo::Echo3DUIGraphComponentInfo(const Name & type, ActorCOMMoveableType movable) 
		: Echo3DUIBaseComponentInfo(type, movable)
	{
	}

	Echo3DUIGraphComponentInfo::~Echo3DUIGraphComponentInfo()
	{
	}

	// ------------------------------------------------ Echo3DUIGraphComponent --------------------------------------------------------------- //
	DECLAREACTORCOMTYPE_CPP(Echo3DUIGraphComponent)

	Echo3DUIGraphComponent::Echo3DUIGraphComponent(ActorPtr  p_actor, ActorCOMMoveableType movable_type) 
		: Echo3DUIBaseComponent(p_actor, movable_type) 
	{
        m_UI3DType = UI3D_GRAPHCOMPONENT;
	}

	Echo3DUIGraphComponent::~Echo3DUIGraphComponent() {

	}

    void Echo3DUIGraphComponent::AfterSetGObject()
	{
        //if(m_dummyMovable)
        //{
        //    ((UI2DRenderBase*)m_dummyMovable)->setRenderPriorityLookupIndex(m_priorityLookupIndex);
        //}
	}

	// ----------------------------------------------------- Script -------------------------------------------------------------------- //
	//Echo3DUIGraphComponent * Echo3DUIGraphComponentScript::GetEcho3DUIGraphComponent(ActorPtr  p_actor, const std::string & name)
	//{
	//	// return p_actor ? p_actor->GetComponent<Echo3DUIGraphComponent>(Name(name)) : nullptr;
	//	return nullptr;
	//}

} /// Echo
