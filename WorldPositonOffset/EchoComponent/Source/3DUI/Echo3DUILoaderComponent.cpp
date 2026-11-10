// --------------------------------------------------------------------------------------
// File:   Echo3DUILoaderComponent.cpp
// 
// Author: Chen ShiWen
//
// Date:   2022-9-6
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
// --------------------------------------------------------------------------------------
#include "EchoComponentStableHeaders.h"
#include "3DUI/Echo3DUILoaderComponent.h"

#include "EchoProtocolComponent.h"
#include "EchoActorProFuncImp.h"

#include "EchocJsonHelper.h"

#include "UI/GComponent.h"

#include "3DUI/Echo3DUIComponent.h"

namespace Echo {

	// ------------------------------------------------ Echo3DUILoaderComponentInfo ------------------------------------------------------------ //
	Echo3DUILoaderComponentInfo::Echo3DUILoaderComponentInfo(const Name & type, ActorCOMMoveableType movable) 
		: Echo3DUIBaseComponentInfo(type, movable)
	{
	}

	// ------------------------------------------------ Echo3DUILoaderComponent --------------------------------------------------------------- //
	DECLAREACTORCOMTYPE_CPP(Echo3DUILoaderComponent)

	Echo3DUILoaderComponent::Echo3DUILoaderComponent(ActorPtr  p_actor, ActorCOMMoveableType movable_type) 
		: Echo3DUIBaseComponent(p_actor, movable_type)
	{
        m_UI3DType = UI3D_LOADERCOMPONENT;
	}

	Echo3DUILoaderComponent::~Echo3DUILoaderComponent() {

	}

	// ----------------------------------------------------- Script -------------------------------------------------------------------- //
	//Echo3DUILoaderComponent * Echo3DUILoaderComponentScript::GetEcho3DUILoaderComponent(ActorPtr  p_actor, const std::string & name)
	//{
	//	// return p_actor ? p_actor->GetComponent<Echo3DUILoaderComponent>(Name(name)) : nullptr;
	//	return nullptr;
	//}

	void Echo3DUILoaderComponent::AfterSetGObject()
	{
        //if(m_dummyMovable)
        //{
        //    ((UI2DRenderBase*)m_dummyMovable)->setRenderPriorityLookupIndex(m_priorityLookupIndex);
        //}
	}
} /// Echo
