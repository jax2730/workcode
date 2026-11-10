// --------------------------------------------------------------------------------------
// File:   Echo3DUIInputTextComponent.h
// 
// Author: Chen ShiWen
//
// Date:   2022-9-6
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
// --------------------------------------------------------------------------------------

#ifndef __ECHO_3DUI_LABEL_COMPONENT_H__
#define __ECHO_3DUI_LABEL_COMPONENT_H__

#include "EchoComponentPrerequisites.h"
#include "EchoPrerequisites.h"
#include "EchoComDefineMacro.h"

#include "3DUI/Echo3DUIBaseComponent.h"

#include "UI/GTextInput.h"

namespace Echo {

	// ------------------------------------------------ Echo3DUIInputTextComponentInfo ------------------------------------------------------------ //
	class _EchoComponentExport Echo3DUIInputTextComponentInfo : public Echo3DUIBaseComponentInfo {
	public:
		Echo3DUIInputTextComponentInfo(const Name& type, ActorCOMMoveableType movable);
		~Echo3DUIInputTextComponentInfo();
	};

	// ------------------------------------------------ Echo3DUIInputTextComponent --------------------------------------------------------------- //
	class _EchoComponentExport Echo3DUIInputTextComponent : public Echo3DUIBaseComponent {
		DECLAREACTORCOMTYPE_H(Echo3DUIInputTextComponent)

	public:
		REGISTER_LUA_CLASS_KEY;

	public:
		/// ------------------------------------------------- In order the script to call ---------------------------------------------- /// 
		std::string GetText();
		void SetText(const std::string& text);

	private:
		virtual void AfterSetGObject()override;
        virtual void afterChangeScene3DUIStyle(UI2DStyle newStyle) override;
        virtual void PostReleaseGObject()override;
	private:
		std::string m_Text;
	};

	// -------------------------------------------------- Factory ---------------------------------------------------------------------- // 
	DEFINECOMFACTORY(Echo3DUIInputTextComponent, ALL_MOVEABLE)

	// ----------------------------------------------------- Script -------------------------------------------------------------------- //
	//class Echo3DUIInputTextComponentScript : public Echo3DUIBaseComponentScript {
	//public:
	//	REGISTER_LUA_CLASS_KEY;

	//public:
	//	Echo3DUIInputTextComponentScript() = default;
	//	Echo3DUIInputTextComponentScript(const EchoActorScript& pActor, const std::string& name);

	//public:
	//	static Echo3DUIInputTextComponentScript GetEcho3DUIInputTextComponent(const EchoActorScript& p_actor, const std::string& name);

	//	std::string GetText();
	//	void SetText(const std::string& text);
	//};

} /// namespace Echo

#endif // !__ECHO_3DUI_LABEL_COMPONENT_H__
