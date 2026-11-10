// --------------------------------------------------------------------------------------
// File:   Echo3DUITextComponent.h
// 
// Author: Chen ShiWen
//
// Date:   2022-9-6
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
// --------------------------------------------------------------------------------------

#ifndef __ECHO_3DUI_TEXT_COMPONENT_H__
#define __ECHO_3DUI_TEXT_COMPONENT_H__

#include "EchoComponentPrerequisites.h"
#include "EchoPrerequisites.h"
#include "EchoComDefineMacro.h"

#include "3DUI/Echo3DUIBaseComponent.h"

#include "UI/GTextField.h"

namespace Echo {

	// ------------------------------------------------ Echo3DUITextComponentInfo ------------------------------------------------------------ //
	class _EchoComponentExport Echo3DUITextComponentInfo : public Echo3DUIBaseComponentInfo {
	public:
		Echo3DUITextComponentInfo(const Name& type, ActorCOMMoveableType movable);
		~Echo3DUITextComponentInfo();
	};

	// ------------------------------------------------ Echo3DUITextComponent --------------------------------------------------------------- //
	class _EchoComponentExport Echo3DUITextComponent : public Echo3DUIBaseComponent {
		DECLAREACTORCOMTYPE_H(Echo3DUITextComponent)

        virtual void AfterSetGObject()override;
	public:
		REGISTER_LUA_CLASS_KEY;
	};

	// -------------------------------------------------- Factory ---------------------------------------------------------------------- // 
	DEFINECOMFACTORY(Echo3DUITextComponent, ALL_MOVEABLE)

	// ----------------------------------------------------- Script -------------------------------------------------------------------- //
	//class Echo3DUITextComponentScript : public Echo3DUIBaseComponentScript {
	//public:
	//	REGISTER_LUA_CLASS_KEY;

	//public:
	//	Echo3DUITextComponentScript() = default;
	//	Echo3DUITextComponentScript(const EchoActorScript& pActor, const std::string& name);

	//public:
	//	static Echo3DUITextComponentScript GetEcho3DUITextComponent(const EchoActorScript&  p_actor, const std::string& name);
	//};

} /// namespace Echo

#endif // !__ECHO_3DUI_TEXT_COMPONENT_H__

