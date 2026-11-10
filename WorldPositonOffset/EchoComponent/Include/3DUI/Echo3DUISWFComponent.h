// --------------------------------------------------------------------------------------
// File:   Echo3DUISWFComponent.h
// 
// Author: Chen ShiWen
//
// Date:   2022-9-6
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
// --------------------------------------------------------------------------------------

#ifndef __ECHO_3DUI_SWF_COMPONENT_H__
#define __ECHO_3DUI_SWF_COMPONENT_H__

#include "EchoComponentPrerequisites.h"
#include "EchoPrerequisites.h"
#include "EchoComDefineMacro.h"

#include "3DUI/Echo3DUIBaseComponent.h"

namespace Echo {

	// ------------------------------------------------ Echo3DUISWFComponentInfo ------------------------------------------------------------ //
	class _EchoComponentExport Echo3DUISWFComponentInfo : public Echo3DUIBaseComponentInfo {
	public:
		Echo3DUISWFComponentInfo(const Name& type, ActorCOMMoveableType movable);
		~Echo3DUISWFComponentInfo();
	};

	// ------------------------------------------------ Echo3DUISWFComponent --------------------------------------------------------------- //
	class _EchoComponentExport Echo3DUISWFComponent : public Echo3DUIBaseComponent {
		DECLAREACTORCOMTYPE_H(Echo3DUISWFComponent)

	public:
		REGISTER_LUA_CLASS_KEY;
	};

	// -------------------------------------------------- Factory ---------------------------------------------------------------------- // 
	DEFINECOMFACTORY(Echo3DUISWFComponent, ALL_MOVEABLE)

	// ----------------------------------------------------- Script -------------------------------------------------------------------- //
	//class Echo3DUISWFComponentScript {
	//public:
	//	REGISTER_LUA_CLASS_KEY;

	//public:
	//	static Echo3DUISWFComponent* GetEcho3DUISWFComponent(ActorPtr  p_actor, const std::string& name);
	//};

} /// namespace Echo

#endif // !__ECHO_3DUI_SWF_COMPONENT_H__
