// --------------------------------------------------------------------------------------
// File:   Echo3DUILoaderComponent.h
// 
// Author: Chen ShiWen
//
// Date:   2022-9-6
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
// --------------------------------------------------------------------------------------

#ifndef __ECHO_3DUI_LOADER_COMPONENT_H__
#define __ECHO_3DUI_LOADER_COMPONENT_H__

#include "EchoComponentPrerequisites.h"
#include "EchoPrerequisites.h"
#include "EchoComDefineMacro.h"

#include "3DUI/Echo3DUIBaseComponent.h"

#include "UI/GLoader.h"

namespace Echo {

	// ------------------------------------------------ Echo3DUILoaderComponentInfo ------------------------------------------------------------ //
	class _EchoComponentExport Echo3DUILoaderComponentInfo : public Echo3DUIBaseComponentInfo {
	public:
		Echo3DUILoaderComponentInfo(const Name& type, ActorCOMMoveableType movable);
	};

	// ------------------------------------------------ Echo3DUILoaderComponent --------------------------------------------------------------- //
	class _EchoComponentExport Echo3DUILoaderComponent : public Echo3DUIBaseComponent {
		DECLAREACTORCOMTYPE_H(Echo3DUILoaderComponent)

	public:
		REGISTER_LUA_CLASS_KEY;

	private:
        virtual void AfterSetGObject()override;
	};

	// -------------------------------------------------- Factory ---------------------------------------------------------------------- // 
	DEFINECOMFACTORY(Echo3DUILoaderComponent, ALL_MOVEABLE)

	// ----------------------------------------------------- Script -------------------------------------------------------------------- //
	//class Echo3DUILoaderComponentScript {
	//public:
	//	REGISTER_LUA_CLASS_KEY;

	//public:
	//	static Echo3DUILoaderComponent* GetEcho3DUILoaderComponent(ActorPtr  p_actor, const std::string& name);
	//};

} /// namespace Echo

#endif // !__ECHO_3DUI_LOADER_COMPONENT_H__
