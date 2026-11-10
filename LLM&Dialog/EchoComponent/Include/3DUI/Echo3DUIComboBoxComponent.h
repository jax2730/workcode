// --------------------------------------------------------------------------------------
// File:   Echo3DUIComboBoxComponent.h
// 
// Author: Chen ShiWen
//
// Date:   2022-9-6
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
// --------------------------------------------------------------------------------------

#ifndef __ECHO_3DUI_COMBOBOX_COMPONENT_H__
#define __ECHO_3DUI_COMBOBOX_COMPONENT_H__

#include "EchoComponentPrerequisites.h"
#include "EchoPrerequisites.h"
#include "EchoComDefineMacro.h"

#include "3DUI/Echo3DUIBaseComponent.h"

#include "UI/GComboBox.h"

namespace Echo {

	// ------------------------------------------------ Echo3DUIComboBoxComponentInfo ------------------------------------------------------------ //
	class _EchoComponentExport Echo3DUIComboBoxComponentInfo : public Echo3DUIBaseComponentInfo {
	public:
		Echo3DUIComboBoxComponentInfo(const Name& type, ActorCOMMoveableType movable);
		~Echo3DUIComboBoxComponentInfo();
	};

	// ------------------------------------------------ Echo3DUIComboBoxComponent --------------------------------------------------------------- //
	class _EchoComponentExport Echo3DUIComboBoxComponent : public Echo3DUIBaseComponent {
		DECLAREACTORCOMTYPE_H(Echo3DUIComboBoxComponent)

	public:
		REGISTER_LUA_CLASS_KEY;
	};

	// -------------------------------------------------- Factory ---------------------------------------------------------------------- // 
	DEFINECOMFACTORY(Echo3DUIComboBoxComponent, ALL_MOVEABLE)

	// ----------------------------------------------------- Script -------------------------------------------------------------------- //
	//class Echo3DUIComboBoxComponentScript {
	//public:
	//	REGISTER_LUA_CLASS_KEY;

	//public:
	//	static Echo3DUIComboBoxComponent* GetEcho3DUIComboBoxComponent(ActorPtr  p_actor, const std::string& name);
	//};

} /// namespace Echo

#endif // !__ECHO_3DUI_COMBOBOX_COMPONENT_H__
