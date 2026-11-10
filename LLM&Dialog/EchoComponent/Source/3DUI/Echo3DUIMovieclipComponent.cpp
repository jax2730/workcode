// --------------------------------------------------------------------------------------
// File:   Echo3DUIMovieclipComponent.cpp
// 
// Author: Chen ShiWen
//
// Date:   2022-9-6
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
// --------------------------------------------------------------------------------------
#include "EchoComponentStableHeaders.h"
#include "3DUI/Echo3DUIMovieclipComponent.h"

#include "EchoProtocolComponent.h"
#include "EchoActorProFuncImp.h"

#include "EchocJsonHelper.h"

#include "UI/GComponent.h"

namespace Echo {

	// ------------------------------------------------ Echo3DUIMovieclipComponentInfo ------------------------------------------------------------ //
	Echo3DUIMovieclipComponentInfo::Echo3DUIMovieclipComponentInfo(const Name & type, ActorCOMMoveableType movable) 
		: Echo3DUIBaseComponentInfo(type, movable)
	{
	}

	Echo3DUIMovieclipComponentInfo::~Echo3DUIMovieclipComponentInfo()
	{

	}

	// ------------------------------------------------ Echo3DUIMovieclipComponent --------------------------------------------------------------- //
	DECLAREACTORCOMTYPE_CPP(Echo3DUIMovieclipComponent)

	Echo3DUIMovieclipComponent::Echo3DUIMovieclipComponent(ActorPtr  p_actor, ActorCOMMoveableType movable_type) 
		: Echo3DUIBaseComponent(p_actor, movable_type)
	{
        m_UI3DType = UI3D_MOVIECLIPCOMPONENT;
	}

	Echo3DUIMovieclipComponent::~Echo3DUIMovieclipComponent() {

	}

    void Echo3DUIMovieclipComponent::AfterSetGObject()
	{
        //if(m_dummyMovable)
        //{
        //    ((UI2DRenderBase*)m_dummyMovable)->setRenderPriorityLookupIndex(m_priorityLookupIndex);
        //}
	}

	// ----------------------------------------------------- Script -------------------------------------------------------------------- //
	//Echo3DUIMovieclipComponent * Echo3DUIMovieclipComponentScript::GetEcho3DUIMovieclipComponent(ActorPtr  p_actor, const std::string & name)
	//{
	//	// return p_actor ? p_actor->GetComponent<Echo3DUIMovieclipComponent>(Name(name)) : nullptr;
	//	return nullptr;
	//}

} /// Echo
