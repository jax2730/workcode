// --------------------------------------------------------------------------------------
// File:   Echo3DUIScrollBarComponent.h
// 
// Author: Chen ShiWen
//
// Date:   2022-9-6
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
// --------------------------------------------------------------------------------------

#ifndef __ECHO_3DUI_SCROLLBAR_COMPONENT_H__
#define __ECHO_3DUI_SCROLLBAR_COMPONENT_H__

#include "EchoComponentPrerequisites.h"
#include "EchoPrerequisites.h"
#include "EchoComDefineMacro.h"

#include "3DUI/Echo3DUIBaseComponent.h"

namespace Echo {

	// ------------------------------------------------ Echo3DUIScrollBarComponentInfo ------------------------------------------------------------ //
	class _EchoComponentExport Echo3DUIScrollBarComponentInfo : public Echo3DUIBaseComponentInfo {
	public:
		Echo3DUIScrollBarComponentInfo(const Name& type, ActorCOMMoveableType movable);
		~Echo3DUIScrollBarComponentInfo();
	};

	// ------------------------------------------------ Echo3DUIScrollBarComponent --------------------------------------------------------------- //
	class _EchoComponentExport Echo3DUIScrollBarComponent : public Echo3DUIBaseComponent {
		DECLAREACTORCOMTYPE_H(Echo3DUIScrollBarComponent)

	public:
		REGISTER_LUA_CLASS_KEY;
	};

	// -------------------------------------------------- Factory ---------------------------------------------------------------------- // 
	DEFINECOMFACTORY(Echo3DUIScrollBarComponent, ALL_MOVEABLE)

	// ----------------------------------------------------- Script -------------------------------------------------------------------- //
	//class Echo3DUIScrollBarComponentScript {
	//public:
	//	REGISTER_LUA_CLASS_KEY;

	//public:
	//	static Echo3DUIScrollBarComponent* GetEcho3DUIScrollBarComponent(ActorPtr  p_actor, const std::string& name);
	//};

} /// namespace Echo

#endif // !__ECHO_3DUI_SCROLLBAR_COMPONENT_H__
