// --------------------------------------------------------------------------------------
// File:   Echo3DUIGraphComponent.h
// 
// Author: Chen ShiWen
//
// Date:   2022-9-6
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
// --------------------------------------------------------------------------------------

#ifndef __ECHO_3DUI_GRAPH_COMPONENT_H__
#define __ECHO_3DUI_GRAPH_COMPONENT_H__

#include "EchoComponentPrerequisites.h"
#include "EchoPrerequisites.h"
#include "EchoComDefineMacro.h"

#include "3DUI/Echo3DUIBaseComponent.h"

namespace Echo {

	class Echo3DUIGraphComponentInfo;
	// ------------------------------------------------ Echo3DUIGraphComponent --------------------------------------------------------------- //
	class _EchoComponentExport Echo3DUIGraphComponent : public Echo3DUIBaseComponent {
		DECLAREACTORCOMTYPE_H(Echo3DUIGraphComponent)

	public:
		REGISTER_LUA_CLASS_KEY;

        virtual void AfterSetGObject()override;
	};

	// ------------------------------------------------ Echo3DUIGraphComponentInfo ------------------------------------------------------------ //
	class _EchoComponentExport Echo3DUIGraphComponentInfo : public Echo3DUIBaseComponentInfo {
	public:
		Echo3DUIGraphComponentInfo(const Name& type, ActorCOMMoveableType movable);
		~Echo3DUIGraphComponentInfo();
	};

	// -------------------------------------------------- Factory ---------------------------------------------------------------------- // 
	DEFINECOMFACTORY(Echo3DUIGraphComponent, ALL_MOVEABLE)

	// ----------------------------------------------------- Script -------------------------------------------------------------------- //
	//class Echo3DUIGraphComponentScript {
	//public:
	//	REGISTER_LUA_CLASS_KEY;

	//public:
	//	static Echo3DUIGraphComponent* GetEcho3DUIGraphComponent(ActorPtr  p_actor, const std::string& name);
	//};

} /// namespace Echo

#endif // !__ECHO_3DUI_GRAPH_COMPONENT_H__
