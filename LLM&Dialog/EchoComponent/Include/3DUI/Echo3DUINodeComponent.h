// --------------------------------------------------------------------------------------
// File:   Echo3DUINodeComponent.h
// 
// Author: Chen ShiWen
//
// Date:   2022-9-6
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
// --------------------------------------------------------------------------------------

#ifndef __ECHO_3DUI_NODE_COMPONENT_H__
#define __ECHO_3DUI_NODE_COMPONENT_H__

#include "EchoComponentPrerequisites.h"
#include "EchoPrerequisites.h"
#include "EchoComDefineMacro.h"

#include "3DUI/Echo3DUIBaseComponent.h"

namespace Echo {

	// ------------------------------------------------ Echo3DUINodeComponentInfo ------------------------------------------------------------ //
	class _EchoComponentExport Echo3DUINodeComponentInfo : public Echo3DUIBaseComponentInfo {
	public:
		Echo3DUINodeComponentInfo(const Name& type, ActorCOMMoveableType movable);
		~Echo3DUINodeComponentInfo();
	};

	// ------------------------------------------------ Echo3DUINodeComponent --------------------------------------------------------------- //
	class _EchoComponentExport Echo3DUINodeComponent : public Echo3DUIBaseComponent {
		DECLAREACTORCOMTYPE_H(Echo3DUINodeComponent)

	public:
		REGISTER_LUA_CLASS_KEY;
	};

	// -------------------------------------------------- Factory ---------------------------------------------------------------------- // 
	DEFINECOMFACTORY(Echo3DUINodeComponent, ALL_MOVEABLE)

	// ----------------------------------------------------- Script -------------------------------------------------------------------- //
	//class Echo3DUINodeComponentScript : public Echo3DUIBaseComponentScript {
	//public:
	//	REGISTER_LUA_CLASS_KEY;

	//public:
	//	Echo3DUINodeComponentScript() = default;
	//	Echo3DUINodeComponentScript(const EchoActorScript& pActor, const std::string& name);

	//public:
	//	static Echo3DUINodeComponentScript GetEcho3DUINodeComponent(const EchoActorScript& pActor, const std::string& name);
	//};

} /// namespace Echo

#endif // !__ECHO_3DUI_NODE_COMPONENT_H__
