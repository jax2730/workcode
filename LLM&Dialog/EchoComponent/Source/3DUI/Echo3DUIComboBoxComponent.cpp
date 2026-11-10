// --------------------------------------------------------------------------------------
// File:   Echo3DUIComboBoxComponent.cpp
// 
// Author: Chen ShiWen
//
// Date:   2022-9-6
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
// --------------------------------------------------------------------------------------
#include "EchoComponentStableHeaders.h"
#include "3DUI/Echo3DUIComboBoxComponent.h"

#include "EchoProtocolComponent.h"
#include "EchoActorProFuncImp.h"

#include "EchocJsonHelper.h"

#include "UI/GComponent.h"

#include "3DUI/Echo3DUIComponent.h"

namespace Echo {


	// ------------------------------------------------ Echo3DUIComboBoxComponentInfo ------------------------------------------------------------ //
	Echo3DUIComboBoxComponentInfo::Echo3DUIComboBoxComponentInfo(const Name & type, ActorCOMMoveableType movable) 
		: Echo3DUIBaseComponentInfo(type, movable)
	{
	}

	Echo3DUIComboBoxComponentInfo::~Echo3DUIComboBoxComponentInfo()
	{
	}

	// ------------------------------------------------ Echo3DUIComboBoxComponent --------------------------------------------------------------- //
	DECLAREACTORCOMTYPE_CPP(Echo3DUIComboBoxComponent)

	Echo3DUIComboBoxComponent::Echo3DUIComboBoxComponent(ActorPtr  p_actor, ActorCOMMoveableType movable_type)
		: Echo3DUIBaseComponent(p_actor, movable_type)
	{
        m_UI3DType = UI3D_COMBOBOXCOMPONENT;
	}

	Echo3DUIComboBoxComponent::~Echo3DUIComboBoxComponent() {

	}
    
	// ----------------------------------------------------- Script -------------------------------------------------------------------- //
	//Echo3DUIComboBoxComponent * Echo3DUIComboBoxComponentScript::GetEcho3DUIComboBoxComponent(ActorPtr  p_actor, const std::string & name)
	//{
	//	// return p_actor ? p_actor->GetComponent<Echo3DUIComboBoxComponent>(Name(name)) : nullptr;
	//	return nullptr;
	//}

} /// Echo
