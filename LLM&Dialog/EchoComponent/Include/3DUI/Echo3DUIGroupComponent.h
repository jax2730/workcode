// --------------------------------------------------------------------------------------
// File:   Echo3DUIGroupComponent.h
// 
// Author: Chen ShiWen
//
// Date:   2022-9-6
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
// --------------------------------------------------------------------------------------

#ifndef __ECHO_3DUI_GROUP_COMPONENT_H__
#define __ECHO_3DUI_GROUP_COMPONENT_H__

#include "EchoComponentPrerequisites.h"
#include "EchoPrerequisites.h"
#include "EchoComDefineMacro.h"

#include "3DUI/Echo3DUIBaseComponent.h"

namespace Echo {

	// ------------------------------------------------ Echo3DUIGroupComponentInfo ------------------------------------------------------------ //
	class _EchoComponentExport Echo3DUIGroupComponentInfo : public Echo3DUIBaseComponentInfo {
	public:
		Echo3DUIGroupComponentInfo(const Name& type, ActorCOMMoveableType movable);
		~Echo3DUIGroupComponentInfo();
	};

	// ------------------------------------------------ Echo3DUIGroupComponent --------------------------------------------------------------- //
	class _EchoComponentExport Echo3DUIGroupComponent : public Echo3DUIBaseComponent {
		DECLAREACTORCOMTYPE_H(Echo3DUIGroupComponent)

	public:
		REGISTER_LUA_CLASS_KEY;
	};

	// -------------------------------------------------- Factory ---------------------------------------------------------------------- // 
	DEFINECOMFACTORY(Echo3DUIGroupComponent, ALL_MOVEABLE)

	// ----------------------------------------------------- Script -------------------------------------------------------------------- //
	//class Echo3DUIGroupComponentScript {
	//public:
	//	REGISTER_LUA_CLASS_KEY;

	//public:
	//	static Echo3DUIGroupComponent* GetEcho3DUIGroupComponent(ActorPtr  p_actor, const std::string& name);
	//};

} /// namespace Echo

#endif // !__ECHO_3DUI_GROUP_COMPONENT_H__
