// --------------------------------------------------------------------------------------
// File:   Echo3DUITreeViewComponent.h
// 
// Author: Chen ShiWen
//
// Date:   2022-9-6
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
// --------------------------------------------------------------------------------------

#ifndef __ECHO_3DUI_TREEVIEW_COMPONENT_H__
#define __ECHO_3DUI_TREEVIEW_COMPONENT_H__

#include "EchoComponentPrerequisites.h"
#include "EchoPrerequisites.h"
#include "EchoComDefineMacro.h"

#include "3DUI/Echo3DUIBaseComponent.h"

namespace Echo {

	// ------------------------------------------------ Echo3DUITreeViewComponentInfo ------------------------------------------------------------ //
	class _EchoComponentExport Echo3DUITreeViewComponentInfo : public Echo3DUIBaseComponentInfo {
	public:
		Echo3DUITreeViewComponentInfo(const Name& type, ActorCOMMoveableType movable);
		~Echo3DUITreeViewComponentInfo();
	};

	// ------------------------------------------------ Echo3DUITreeViewComponent --------------------------------------------------------------- //
	class _EchoComponentExport Echo3DUITreeViewComponent : public Echo3DUIBaseComponent {
		DECLAREACTORCOMTYPE_H(Echo3DUITreeViewComponent)

	public:
		REGISTER_LUA_CLASS_KEY;
	};

	// -------------------------------------------------- Factory ---------------------------------------------------------------------- // 
	DEFINECOMFACTORY(Echo3DUITreeViewComponent, ALL_MOVEABLE)

	// ----------------------------------------------------- Script -------------------------------------------------------------------- //
	//class Echo3DUITreeViewComponentScript {
	//public:
	//	REGISTER_LUA_CLASS_KEY;

	//public:
	//	static Echo3DUITreeViewComponent* GetEcho3DUITreeViewComponent(ActorPtr  p_actor, const std::string& name);
	//};

} /// namespace Echo

#endif // !__ECHO_3DUI_TREEVIEW_COMPONENT_H__
