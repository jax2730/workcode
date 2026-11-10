// --------------------------------------------------------------------------------------
// File:   Echo3DUITextComponent.cpp
// 
// Author: Chen ShiWen
//
// Date:   2022-9-6
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
// --------------------------------------------------------------------------------------
#include "EchoComponentStableHeaders.h"
#include "3DUI/Echo3DUITextComponent.h"

#include "EchoProtocolComponent.h"
#include "EchoActorProFuncImp.h"

#include "EchocJsonHelper.h"

#include "UI/GComponent.h"

#include "UI/GTextField.h"

namespace Echo {

	// ------------------------------------------------ Echo3DUITextComponentInfo ------------------------------------------------------------ //
	Echo3DUITextComponentInfo::Echo3DUITextComponentInfo(const Name & type, ActorCOMMoveableType movable) 
		: Echo3DUIBaseComponentInfo(type, movable)
	{
	}

	Echo3DUITextComponentInfo::~Echo3DUITextComponentInfo()
	{
	}

	// ------------------------------------------------ Echo3DUITextComponent --------------------------------------------------------------- //
	DECLAREACTORCOMTYPE_CPP(Echo3DUITextComponent)

	Echo3DUITextComponent::Echo3DUITextComponent(ActorPtr  p_actor, ActorCOMMoveableType movable_type) 
		: Echo3DUIBaseComponent(p_actor, movable_type) 
	{
        m_UI3DType = UI3D_TEXTCOMPONENT;
	}

	Echo3DUITextComponent::~Echo3DUITextComponent() {

	}

    void Echo3DUITextComponent::AfterSetGObject()
	{
        //if(m_dummyMovable)
        //{
        //    ((UI2DRenderBase*)m_dummyMovable)->setRenderPriorityLookupIndex(m_priorityLookupIndex);
        //}
	}
    
	// ----------------------------------------------------- Script -------------------------------------------------------------------- //
	//Echo3DUITextComponentScript::Echo3DUITextComponentScript(const EchoActorScript& pActor, const std::string& name) :
	//	Echo3DUIBaseComponentScript(pActor, name)
	//{
	//}
	//
	//Echo3DUITextComponentScript Echo3DUITextComponentScript::GetEcho3DUITextComponent(const EchoActorScript& pActor, const std::string & name)
	//{
	//	return 	Echo3DUITextComponentScript(pActor, name);
	//}



} /// Echo
