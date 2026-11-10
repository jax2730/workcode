// --------------------------------------------------------------------------------------
// File:   Echo3DUIButtonComponent.h
// 
// Author: Chen ShiWen
//
// Date:   2022-9-6
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
// --------------------------------------------------------------------------------------

#ifndef __ECHO_3DUI_BUTTON_COMPONENT_H__
#define __ECHO_3DUI_BUTTON_COMPONENT_H__

//#define  ELUA_LIB

#include "EchoComponentPrerequisites.h"
#include "EchoPrerequisites.h"
#include "EchoComDefineMacro.h"

#include "3DUI/Echo3DUIBaseComponent.h"

#include "EchoScriptManager.h"
//#include "eLuaFunction.h"

#include "Event/EventContext.h"

#include "UI/GImage.h"
#include "UI/GTextField.h"

namespace Echo {

	class Echo3DUIButtonComponentInfo;
	// ------------------------------------------------ Echo3DUIButtonComponent --------------------------------------------------------------- //
	enum Button3DUIEventType {
		BE_Click = 0,

		BE_RollOver,
		BE_RollOut,

		BE_TouchBegin,
		BE_TouchEnd,

		BE_Exit,

		BE_COUNT
	};

	class _EchoComponentExport Echo3DUIButtonComponent : public Echo3DUIBaseComponent {
		DECLAREACTORCOMTYPE_H(Echo3DUIButtonComponent)

	public:
		REGISTER_LUA_CLASS_KEY;

	public:
		bool RegisterScriptFunctionsFromLua(const std::string& script_path);

	private:
		virtual void AfterSetGObject()override;

	private:
		void OnRollOver(EventContext* context);
		void OnRollOut(EventContext* context);
		void OnClick(EventContext* context);
		void OnButtonExit(EventContext* context);
		void OnTouchBegin(EventContext* context);
		void OnTouchEnd(EventContext* context);

		//eLuaFunction m_LuaFunctions[Button3DUIEventType::BE_COUNT];
	};

	// ------------------------------------------------ Echo3DUIButtonComponentInfo ------------------------------------------------------------ //
	class _EchoComponentExport Echo3DUIButtonComponentInfo : public Echo3DUIBaseComponentInfo {
	public:
		Echo3DUIButtonComponentInfo() = default;
		Echo3DUIButtonComponentInfo(const Name& type, ActorCOMMoveableType movable);

	private:
		virtual void AfterExportData(cJSON* jsonNode) override;
		virtual void AfterImportData(cJSON* jsonNode) override;
		virtual void AfterCopyTo(ActorComponentInfo* actorComInfo) override;
		virtual void AfterOverWrite() override;

	public:
		EDITOR_REFLECT

	public:
		std::string m_ScriptPath = "";
	};

	// -------------------------------------------------- Factory ---------------------------------------------------------------------- // 
	DEFINECOMFACTORY(Echo3DUIButtonComponent, ALL_MOVEABLE)

	// ----------------------------------------------------- Script -------------------------------------------------------------------- //
	//class Echo3DUIButtonComponentScript : public EchoActorComponentScript {
	//public:
	//	REGISTER_LUA_CLASS_KEY;

	//public:
	//	Echo3DUIButtonComponentScript() = default;
	//	Echo3DUIButtonComponentScript(const EchoActorScript& pActor, const std::string& name);
	//	Echo3DUIButtonComponentScript(const ActorComponentPtr& com);

	//public:
	//	static Echo3DUIButtonComponentScript GetEcho3DUIButtonComponent(const EchoActorScript& pActor, const std::string& name);
	//};

} /// namespace Echo

#endif // !__ECHO_3DUI_BUTTON_COMPONENT_H__
