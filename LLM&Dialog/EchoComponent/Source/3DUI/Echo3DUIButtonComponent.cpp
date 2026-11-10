// --------------------------------------------------------------------------------------
// File:   Echo3DUIButtonComponent.cpp
// 
// Author: Chen ShiWen
//
// Date:   2022-9-6
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
// --------------------------------------------------------------------------------------
#include "EchoComponentStableHeaders.h"
#include "3DUI/Echo3DUIButtonComponent.h"

#include "EchoProtocolComponent.h"
#include "EchoActorProFuncImp.h"

#include "EchocJsonHelper.h"

#include "UI/GComponent.h"
#include "EchoResourceFolderManager.h"

namespace Echo {

	// ------------------------------------------------ Echo3DUIButtonComponentInfo ------------------------------------------------------------ //


	Echo3DUIButtonComponentInfo::Echo3DUIButtonComponentInfo(const Name & type, ActorCOMMoveableType movable) 
		: Echo3DUIBaseComponentInfo(type, movable)
	{
	}

	EDITOR_REFLECT_BEGIN(Echo3DUIButtonComponentInfo)
	EDITOR_REFLECT_MEMBER_UPDATE(m_ScriptPath, "ScriptPath", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_END();

	void Echo3DUIButtonComponentInfo::AfterExportData(cJSON * jsonNode)
	{
		reflect::TypeResolver<Echo3DUIButtonComponentInfo>::get()->ExportJson(this, jsonNode, "");
	}

	void Echo3DUIButtonComponentInfo::AfterImportData(cJSON * jsonNode)
	{
		reflect::TypeResolver<Echo3DUIButtonComponentInfo>::get()->ImportJson(this, jsonNode, "");
	}

	void Echo3DUIButtonComponentInfo::AfterCopyTo(ActorComponentInfo * actorComInfo)
	{
		Echo3DUIButtonComponentInfo* target = static_cast<Echo3DUIButtonComponentInfo*>(actorComInfo);
		reflect::TypeResolver<Echo3DUIButtonComponentInfo>::get()->copyTo(this, target);
	}

	void Echo3DUIButtonComponentInfo::AfterOverWrite() {
		reflect::TypeResolver<Echo3DUIButtonComponentInfo>::get()->overWrite(mOriginComInfo, this);
	} 

	// ------------------------------------------------ Echo3DUIButtonComponent --------------------------------------------------------------- //
	DECLAREACTORCOMTYPE_CPP(Echo3DUIButtonComponent)

	Echo3DUIButtonComponent::Echo3DUIButtonComponent(ActorPtr  p_actor, ActorCOMMoveableType movable_type) 
		: Echo3DUIBaseComponent(p_actor, movable_type)
	{
        m_UI3DType = UI3D_BUTTONCOMPONENT;
	}

	Echo3DUIButtonComponent::~Echo3DUIButtonComponent() {
		if (this->mGObject) {
			/// fgui event remove
			//UIEventDispatcher* event_dispatcher = static_cast<UIEventDispatcher*>(this->mGObject);

			//event_dispatcher->removeEventListener(UIEventType::RollOver);
			//event_dispatcher->removeEventListener(UIEventType::RollOut);
			//event_dispatcher->removeEventListener(UIEventType::Click);
			//event_dispatcher->removeEventListener(UIEventType::Exit);
		}
	}

	bool Echo3DUIButtonComponent::RegisterScriptFunctionsFromLua(const std::string & script_path)
	{
		if (script_path.empty()) return false;

		//std::size_t start_pos = script_path.find_last_of("/\\");
		//std::size_t end_pos = script_path.find_last_of(".");

		//if (start_pos == std::string::npos || end_pos == std::string::npos) return false;

		//std::string script_name = script_path.substr(start_pos + 1, end_pos - start_pos - 1);

		/*this->m_LuaFunctions[Button3DUIEventType::BE_Click] = eLuaFunction((script_name + ".OnClick").c_str());
		this->m_LuaFunctions[Button3DUIEventType::BE_RollOut] = eLuaFunction((script_name + ".OnRollOut").c_str());
		this->m_LuaFunctions[Button3DUIEventType::BE_RollOver] = eLuaFunction((script_name + ".OnRollOver").c_str());
		this->m_LuaFunctions[Button3DUIEventType::BE_Exit] = eLuaFunction((script_name + ".OnExit").c_str());
		this->m_LuaFunctions[Button3DUIEventType::BE_TouchBegin] = eLuaFunction((script_name + ".OnTouchBegin").c_str());
		this->m_LuaFunctions[Button3DUIEventType::BE_TouchEnd] = eLuaFunction((script_name + ".OnTouchEnd").c_str());

		return GetScriptManager()->LoadScriptModule(ResourceFolderManager::instance()->GetDefaultRootFolder() + script_path);*/
		return false;
	}

	void Echo3DUIButtonComponent::AfterSetGObject()
	{
		// 注册Button脚本函数。
		if (this->mGObject) 
		{
			//// 加载脚本
			//Echo3DUIButtonComponentInfo* buttonComInfo = static_cast<Echo3DUIButtonComponentInfo*>(this->m_pActComInfo);
			//this->RegisterScriptFunctionsFromLua(buttonComInfo->m_ScriptPath);

			//// 注册fgui事件
			//UIEventDispatcher* event_dispatcher = static_cast<UIEventDispatcher*>(this->mGObject);

			//event_dispatcher->addEventListener(UIEventType::RollOver, REG_CALLBACK_1(Echo3DUIButtonComponent::OnRollOver, this));
			//event_dispatcher->addEventListener(UIEventType::RollOut, REG_CALLBACK_1(Echo3DUIButtonComponent::OnRollOut, this));
			//event_dispatcher->addEventListener(UIEventType::Click, REG_CALLBACK_1(Echo3DUIButtonComponent::OnClick, this));
			//event_dispatcher->addEventListener(UIEventType::Exit, REG_CALLBACK_1(Echo3DUIButtonComponent::OnButtonExit, this));
			//event_dispatcher->addEventListener(UIEventType::TouchBegin, REG_CALLBACK_1(Echo3DUIButtonComponent::OnTouchBegin, this));
			//event_dispatcher->addEventListener(UIEventType::TouchEnd, REG_CALLBACK_1(Echo3DUIButtonComponent::OnTouchEnd, this));
		}
	}

	void Echo3DUIButtonComponent::OnRollOver(EventContext * context)
	{
		//this->m_LuaFunctions[Button3DUIEventType::BE_RollOver](EchoActorScript(this->m_pActor), Echo3DUIButtonComponentScript(weak_from_this()));
	}

	void Echo3DUIButtonComponent::OnRollOut(EventContext * context)
	{
		//this->m_LuaFunctions[Button3DUIEventType::BE_RollOut](EchoActorScript(this->m_pActor), Echo3DUIButtonComponentScript(weak_from_this()));
	}

	void Echo3DUIButtonComponent::OnClick(EventContext * context)
	{
		//this->m_LuaFunctions[Button3DUIEventType::BE_Click](EchoActorScript(this->m_pActor), Echo3DUIButtonComponentScript(weak_from_this()));
	}

	void Echo3DUIButtonComponent::OnButtonExit(EventContext * context)
	{
		//this->m_LuaFunctions[Button3DUIEventType::BE_Exit](EchoActorScript(this->m_pActor), Echo3DUIButtonComponentScript(weak_from_this()));
	}

	void Echo3DUIButtonComponent::OnTouchBegin(EventContext * context)
	{
		//this->m_LuaFunctions[Button3DUIEventType::BE_TouchBegin](EchoActorScript(this->m_pActor), Echo3DUIButtonComponentScript(weak_from_this()));
	}

	void Echo3DUIButtonComponent::OnTouchEnd(EventContext * context)
	{
		//this->m_LuaFunctions[Button3DUIEventType::BE_TouchEnd](EchoActorScript(this->m_pActor), Echo3DUIButtonComponentScript(weak_from_this()));
	}



	// ----------------------------------------------------- Script -------------------------------------------------------------------- //
	//Echo3DUIButtonComponentScript::Echo3DUIButtonComponentScript(const EchoActorScript& pActor, const std::string& name) :
	//	EchoActorComponentScript(pActor, name)
	//{}

	//Echo3DUIButtonComponentScript::Echo3DUIButtonComponentScript(const ActorComponentPtr& com) : 
	//	EchoActorComponentScript(com) 
	//{}

	//Echo3DUIButtonComponentScript Echo3DUIButtonComponentScript::GetEcho3DUIButtonComponent(const EchoActorScript& pActor, const std::string& name)
	//{
	//	return Echo3DUIButtonComponentScript(pActor, name);
	//}

} /// Echo
