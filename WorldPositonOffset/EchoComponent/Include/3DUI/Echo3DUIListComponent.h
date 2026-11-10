// --------------------------------------------------------------------------------------
// File:   Echo3DUIListComponent.h
// 
// Author: Chen ShiWen
//
// Date:   2022-9-6
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
// --------------------------------------------------------------------------------------

#ifndef __ECHO_3DUI_LIST_COMPONENT_H__
#define __ECHO_3DUI_LIST_COMPONENT_H__

#include "EchoComponentPrerequisites.h"
#include "EchoPrerequisites.h"
#include "EchoComDefineMacro.h"

#include "3DUI/Echo3DUIBaseComponent.h"

namespace Echo {

	// ------------------------------------------------ Echo3DUIListComponentInfo ------------------------------------------------------------ //
	class _EchoComponentExport Echo3DUIListComponentInfo : public Echo3DUIBaseComponentInfo {
	public:
		Echo3DUIListComponentInfo(const Name& type, ActorCOMMoveableType movable);
		~Echo3DUIListComponentInfo();
	};

	// ------------------------------------------------ Echo3DUIListComponent --------------------------------------------------------------- //
	class _EchoComponentExport Echo3DUIListComponent : public Echo3DUIBaseComponent {
		DECLAREACTORCOMTYPE_H(Echo3DUIListComponent)

	public:
		REGISTER_LUA_CLASS_KEY;
	};

	// -------------------------------------------------- Factory ---------------------------------------------------------------------- // 
	DEFINECOMFACTORY(Echo3DUIListComponent, ALL_MOVEABLE)

	// ----------------------------------------------------- Script -------------------------------------------------------------------- //
	//class Echo3DUIListComponentScript {
	//public:
	//	REGISTER_LUA_CLASS_KEY;

	//public:
	//	static Echo3DUIListComponent* GetEcho3DUIListComponent(ActorPtr  p_actor, const std::string& name);
	//};

} /// namespace Echo

#endif // !__ECHO_3DUI_LIST_COMPONENT_H__
