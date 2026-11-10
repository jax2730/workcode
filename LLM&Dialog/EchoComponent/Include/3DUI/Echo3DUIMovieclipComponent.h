// --------------------------------------------------------------------------------------
// File:   Echo3DUIMovieclipComponent.h
// 
// Author: Chen ShiWen
//
// Date:   2022-9-6
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
// --------------------------------------------------------------------------------------

#ifndef __ECHO_3DUI_MOVIECLIP_COMPONENT_H__
#define __ECHO_3DUI_MOVIECLIP_COMPONENT_H__

#include "EchoComponentPrerequisites.h"
#include "EchoPrerequisites.h"
#include "EchoComDefineMacro.h"

#include "3DUI/Echo3DUIBaseComponent.h"

namespace Echo {

	// ------------------------------------------------ Echo3DUIMovieclipComponentInfo ------------------------------------------------------------ //
	class _EchoComponentExport Echo3DUIMovieclipComponentInfo : public Echo3DUIBaseComponentInfo {
	public:
		Echo3DUIMovieclipComponentInfo(const Name& type, ActorCOMMoveableType movable);
		~Echo3DUIMovieclipComponentInfo();
	};

	// ------------------------------------------------ Echo3DUIMovieclipComponent --------------------------------------------------------------- //
	class _EchoComponentExport Echo3DUIMovieclipComponent : public Echo3DUIBaseComponent {
		DECLAREACTORCOMTYPE_H(Echo3DUIMovieclipComponent)

	public:
		REGISTER_LUA_CLASS_KEY;

        virtual void AfterSetGObject()override;
	};

	// -------------------------------------------------- Factory ---------------------------------------------------------------------- // 
	DEFINECOMFACTORY(Echo3DUIMovieclipComponent, ALL_MOVEABLE)

	// ----------------------------------------------------- Script -------------------------------------------------------------------- //
	//class Echo3DUIMovieclipComponentScript {
	//public:
	//	REGISTER_LUA_CLASS_KEY;

	//public:
	//	static Echo3DUIMovieclipComponent* GetEcho3DUIMovieclipComponent(ActorPtr  p_actor, const std::string& name);
	//};

} /// namespace Echo

#endif // !__ECHO_3DUI_MOVIECLIP_COMPONENT_H__
