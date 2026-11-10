// --------------------------------------------------------------------------------------
// File:   Echo3DUILabelComponent.h
// 
// Author: Chen ShiWen
//
// Date:   2022-9-6
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
// --------------------------------------------------------------------------------------

#ifndef __ECHO_3DUI_INPUTTEXT_COMPONENT_H__
#define __ECHO_3DUI_INPUTTEXT_COMPONENT_H__

#include "EchoComponentPrerequisites.h"
#include "EchoPrerequisites.h"
#include "EchoComDefineMacro.h"

#include "3DUI/Echo3DUIBaseComponent.h"

namespace Echo {

	// ------------------------------------------------ Echo3DUILabelComponentInfo ------------------------------------------------------------ //
	class _EchoComponentExport Echo3DUILabelComponentInfo : public Echo3DUIBaseComponentInfo {
	public:
		Echo3DUILabelComponentInfo(const Name& type, ActorCOMMoveableType movable);
		~Echo3DUILabelComponentInfo();
	};

	// ------------------------------------------------ Echo3DUILabelComponent --------------------------------------------------------------- //
	class _EchoComponentExport Echo3DUILabelComponent : public Echo3DUIBaseComponent {
		DECLAREACTORCOMTYPE_H(Echo3DUILabelComponent)

	public:
		REGISTER_LUA_CLASS_KEY;
	};

	// -------------------------------------------------- Factory ---------------------------------------------------------------------- // 
	DEFINECOMFACTORY(Echo3DUILabelComponent, ALL_MOVEABLE)

	// ----------------------------------------------------- Script -------------------------------------------------------------------- //
	//class Echo3DUILabelComponentScript {
	//public:
	//	REGISTER_LUA_CLASS_KEY;

	//public:
	//	static Echo3DUILabelComponent* GetEcho3DUILabelComponent(ActorPtr  p_actor, const std::string& name);
	//};

} /// namespace Echo

#endif // !__ECHO_3DUI_INPUTTEXT_COMPONENT_H__
