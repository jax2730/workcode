// --------------------------------------------------------------------------------------
// File:   Echo3DUIProgressBarComponent.h
// 
// Author: Chen ShiWen
//
// Date:   2022-9-6
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
// --------------------------------------------------------------------------------------

#ifndef __ECHO_3DUI_PROGRESSBAR_COMPONENT_H__
#define __ECHO_3DUI_PROGRESSBAR_COMPONENT_H__

#include "EchoComponentPrerequisites.h"
#include "EchoPrerequisites.h"
#include "EchoComDefineMacro.h"

#include "3DUI/Echo3DUIBaseComponent.h"

namespace Echo {

	// ------------------------------------------------ Echo3DUIProgressBarComponentInfo ------------------------------------------------------------ //
	class _EchoComponentExport Echo3DUIProgressBarComponentInfo : public Echo3DUIBaseComponentInfo {
	public:
		Echo3DUIProgressBarComponentInfo(const Name& type, ActorCOMMoveableType movable);
		~Echo3DUIProgressBarComponentInfo();
	};

	// ------------------------------------------------ Echo3DUIProgressBarComponent --------------------------------------------------------------- //
	class _EchoComponentExport Echo3DUIProgressBarComponent : public Echo3DUIBaseComponent {
		DECLAREACTORCOMTYPE_H(Echo3DUIProgressBarComponent)

	public:
		REGISTER_LUA_CLASS_KEY;
	};

	// -------------------------------------------------- Factory ---------------------------------------------------------------------- // 
	DEFINECOMFACTORY(Echo3DUIProgressBarComponent, ALL_MOVEABLE)

	// ----------------------------------------------------- Script -------------------------------------------------------------------- //
	//class Echo3DUIProgressBarComponentScript {
	//public:
	//	REGISTER_LUA_CLASS_KEY;

	//public:
	//	static Echo3DUIProgressBarComponent* GetEcho3DUIProgressBarComponent(ActorPtr  p_actor, const std::string& name);
	//};

} /// namespace Echo

#endif // !__ECHO_3DUI_PROGRESSBAR_COMPONENT_H__
