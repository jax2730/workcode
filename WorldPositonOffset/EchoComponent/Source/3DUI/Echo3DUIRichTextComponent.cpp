// --------------------------------------------------------------------------------------
// File:   Echo3DUIRichTextComponent.cpp
// 
// Author: Chen ShiWen
//
// Date:   2022-9-6
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
// --------------------------------------------------------------------------------------
#include "EchoComponentStableHeaders.h"
#include "3DUI/Echo3DUIRichTextComponent.h"

#include "EchoProtocolComponent.h"
#include "EchoActorProFuncImp.h"

#include "EchocJsonHelper.h"

#include "UI/GComponent.h"

namespace Echo {

	// ------------------------------------------------ Echo3DUIRichTextComponentInfo ------------------------------------------------------------ //
	Echo3DUIRichTextComponentInfo::Echo3DUIRichTextComponentInfo(const Name & type, ActorCOMMoveableType movable) 
		: Echo3DUIBaseComponentInfo(type, movable)
	{
	}

	Echo3DUIRichTextComponentInfo::~Echo3DUIRichTextComponentInfo()
	{

	}

	// ------------------------------------------------ Echo3DUIRichTextComponent --------------------------------------------------------------- //
	DECLAREACTORCOMTYPE_CPP(Echo3DUIRichTextComponent)

	Echo3DUIRichTextComponent::Echo3DUIRichTextComponent(ActorPtr  p_actor, ActorCOMMoveableType movable_type) 
		: Echo3DUIBaseComponent(p_actor, movable_type)
	{
        m_UI3DType = UI3D_RICHTEXTCOMPONENT;
	}

	Echo3DUIRichTextComponent::~Echo3DUIRichTextComponent() {

	}

	// ----------------------------------------------------- Script -------------------------------------------------------------------- //
	//Echo3DUIRichTextComponent * Echo3DUIRichTextComponentScript::GetEcho3DUIRichTextComponent(ActorPtr  p_actor, const std::string & name)
	//{
	//	// return p_actor ? p_actor->GetComponent<Echo3DUIRichTextComponent>(Name(name)) : nullptr;
	//	return nullptr;
	//}

} /// Echo
