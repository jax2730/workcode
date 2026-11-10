// --------------------------------------------------------------------------------------
// File:   Echo3DUISliderComponent.h
// 
// Author: Chen ShiWen
//
// Date:   2022-9-6
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
// --------------------------------------------------------------------------------------

#ifndef __ECHO_3DUI_SLIDER_COMPONENT_H__
#define __ECHO_3DUI_SLIDER_COMPONENT_H__

#include "EchoComponentPrerequisites.h"
#include "EchoPrerequisites.h"
#include "EchoComDefineMacro.h"

#include "3DUI/Echo3DUIBaseComponent.h"

namespace Echo {

	// ------------------------------------------------ Echo3DUISliderComponentInfo ------------------------------------------------------------ //
	class _EchoComponentExport Echo3DUISliderComponentInfo : public Echo3DUIBaseComponentInfo {
	public:
		Echo3DUISliderComponentInfo(const Name& type, ActorCOMMoveableType movable);
		~Echo3DUISliderComponentInfo();
	};

	// ------------------------------------------------ Echo3DUISliderComponent --------------------------------------------------------------- //
	class _EchoComponentExport Echo3DUISliderComponent : public Echo3DUIBaseComponent {
		DECLAREACTORCOMTYPE_H(Echo3DUISliderComponent)

	public:
		REGISTER_LUA_CLASS_KEY;
	};

	// -------------------------------------------------- Factory ---------------------------------------------------------------------- // 
	DEFINECOMFACTORY(Echo3DUISliderComponent, ALL_MOVEABLE)

	// ----------------------------------------------------- Script -------------------------------------------------------------------- //
	//class Echo3DUISliderComponentScript {
	//public:
	//	REGISTER_LUA_CLASS_KEY;

	//public:
	//	static Echo3DUISliderComponent* GetEcho3DUISliderComponent(ActorPtr  p_actor, const std::string& name);
	//};

} /// namespace Echo

#endif // !__ECHO_3DUI_SLIDER_COMPONENT_H__
