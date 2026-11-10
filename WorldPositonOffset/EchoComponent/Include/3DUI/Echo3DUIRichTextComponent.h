// --------------------------------------------------------------------------------------
// File:   Echo3DUIRichTextComponent.h
// 
// Author: Chen ShiWen
//
// Date:   2022-9-6
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
// --------------------------------------------------------------------------------------

#ifndef __ECHO_3DUI_RICHTEXT_COMPONENT_H__
#define __ECHO_3DUI_RICHTEXT_COMPONENT_H__

#include "EchoComponentPrerequisites.h"
#include "EchoPrerequisites.h"
#include "EchoComDefineMacro.h"

#include "3DUI/Echo3DUIBaseComponent.h"

namespace Echo {

	// ------------------------------------------------ Echo3DUIRichTextComponentInfo ------------------------------------------------------------ //
	class _EchoComponentExport Echo3DUIRichTextComponentInfo : public Echo3DUIBaseComponentInfo {
	public:
		Echo3DUIRichTextComponentInfo(const Name& type, ActorCOMMoveableType movable);
		~Echo3DUIRichTextComponentInfo();
	};

	// ------------------------------------------------ Echo3DUIRichTextComponent --------------------------------------------------------------- //
	class _EchoComponentExport Echo3DUIRichTextComponent : public Echo3DUIBaseComponent {
		DECLAREACTORCOMTYPE_H(Echo3DUIRichTextComponent)

	public:
		REGISTER_LUA_CLASS_KEY;
	};

	// -------------------------------------------------- Factory ---------------------------------------------------------------------- // 
	DEFINECOMFACTORY(Echo3DUIRichTextComponent, ALL_MOVEABLE)

	// ----------------------------------------------------- Script -------------------------------------------------------------------- //
	//class Echo3DUIRichTextComponentScript {
	//public:
	//	REGISTER_LUA_CLASS_KEY;

	//public:
	//	static Echo3DUIRichTextComponent* GetEcho3DUIRichTextComponent(ActorPtr  p_actor, const std::string& name);
	//};

} /// namespace Echo

#endif // !__ECHO_3DUI_RICHTEXT_COMPONENT_H__
