// --------------------------------------------------------------------------------------
// File:   EchoActorScriptComponent.cpp
// 
// Author: Chen ShiWen
//
// Date:   2022-7-4
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
// --------------------------------------------------------------------------------------
#include "EchoComponentStableHeaders.h"
#define ELUA_LIB
#include "EchoActorScriptComponent.h"
#include "EchoStringConverter.h"
#include "EchoLogManager.h"
#include "EchoScriptManager.h"
//#include "eLuaFunction.h"
#include "Actor/EchoActor.h"
#include "EchoActorProFuncImp.h"

//-----------------------------------------------------------------------------
//	CPP File Begin
//-----------------------------------------------------------------------------
namespace Echo
{   
	DECLAREACTORCOMTYPE_CPP(ActorScriptComponent)

		// ----------------------------------------- 初始化脚本组件Info -------------------------------------------
		// --------------------------------------------------------------------------------------------------------
		EDITOR_REFLECT_BEGIN(ActorScriptComponentInfo)
		EDITOR_REFLECT_MEMBER_UPDATE(m_ScriptPath, "ScriptPath", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mFloatValueMap, "FloatValueArray", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mIntValueMap, "IntValueArray", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mBoolValueMap, "BoolValueArray", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mStringValueMap, "StringValueArray", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mVector3ValueMap, "Vector3ValueArray", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mQuaternionValueMap, "QuaternionValueArray", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_END();

	template <typename T>
	bool ReadJSNode(cJSON* jsonNode, const char* name, ValueMap<T>& valueMap) {
		cJSON* valueMapJson = cJSON_GetObjectItem(jsonNode, name);
		return valueMap.ImportJson(valueMapJson);
	}

	template <class T>
	void WriteJSNode(cJSON* jsonNode, const char* name, const ValueMap<T>& valueMap)
	{
		cJSON* valueMapJson = cJSON_CreateObject();
		cJSON_AddItemToObject(jsonNode, name, valueMapJson);
		valueMap.ExportJson(valueMapJson);
	}

	ActorScriptComponentInfo::ActorScriptComponentInfo(const Name& type, ActorCOMMoveableType	movable) :
		ActorComponentInfo(type, ActorCOMMoveableType::UNMOVEABLE)
	{}

	void ActorScriptComponentInfo::ExportData(cJSON * jsonNode)
	{
		reflect::TypeResolver<ActorScriptComponentInfo>::get()->ExportJson(this, jsonNode, "");
	}

	void ActorScriptComponentInfo::ImportData(cJSON * jsonNode)
	{
		reflect::TypeResolver<ActorScriptComponentInfo>::get()->ImportJson(this, jsonNode, "");
	}

	void ActorScriptComponentInfo::CopyTo(ActorComponentInfo * comInfo)
	{
		ActorScriptComponentInfo* targetInfo = static_cast<ActorScriptComponentInfo*>(comInfo);
		reflect::TypeResolver<ActorScriptComponentInfo>::get()->copyTo(this, targetInfo);
	}

	void ActorScriptComponentInfo::OverWrite() {
		if (mOriginComInfo != nullptr) {
			reflect::TypeResolver<ActorScriptComponentInfo>::get()->overWrite(mOriginComInfo, this);
		}
	}

	// ----------------------------------------- 脚本类 ---------------------------------------------------
	// ---------------------------------------------------------------------------------------------------
	ActorScriptComponent::ActorScriptComponent(ActorPtr  actor, ActorCOMMoveableType movable)
		: ScriptComponent(actor, movable)
	{
 
	}

	ActorScriptComponent::~ActorScriptComponent()
	{}

	void ActorScriptComponent::init(ActorComponentInfo * info)
	{
		ActorScriptComponentInfo* actorScriptComponentInfo = static_cast<ActorScriptComponentInfo*>(info);

		if (actorScriptComponentInfo) {
			this->mFloatValueMap = actorScriptComponentInfo->mFloatValueMap;
			this->mIntValueMap = actorScriptComponentInfo->mIntValueMap;
			this->mBoolValueMap = actorScriptComponentInfo->mBoolValueMap;
			this->mStringValueMap = actorScriptComponentInfo->mStringValueMap;
			this->mVector3ValueMap = actorScriptComponentInfo->mVector3ValueMap;
			this->mQuaternionValueMap = actorScriptComponentInfo->mQuaternionValueMap;

			this->loadScript(actorScriptComponentInfo->m_ScriptPath);
		}

		mIsPause = ActorSystem::instance()->getMainActorManager()->IsEditorPause();
	}

	void ActorScriptComponent::copyTo(const ActorComponentPtr&  targetComponent)
	{
		ActorScriptComponent* target = static_cast<ActorScriptComponent*>(targetComponent.get());

		target->mFloatValueMap = this->mFloatValueMap;
		target->mIntValueMap = this->mIntValueMap;
		target->mBoolValueMap = this->mBoolValueMap;
		target->mStringValueMap = this->mStringValueMap;
		target->mVector3ValueMap = this->mVector3ValueMap;
		target->mQuaternionValueMap = this->mQuaternionValueMap;
	}

	void ActorScriptComponent::beginFrame(EventID id, ProtocolHandle handle)
	{
		/*if (!mIsPause) {
			float dt = Root::instance()->getTimeSinceLastFrameClient();
			m_luaFunction[TT_BeginFrame](EchoActorScript(m_pActor), EchoActorScriptComScript(weak_from_this()), dt);
		}*/
	}

	void ActorScriptComponent::endFrame(EventID id, ProtocolHandle handle)
	{
		/*if (!mIsPause) {
			float dt = Root::instance()->getTimeSinceLastFrameClient();
			m_luaFunction[TT_EndFrame](EchoActorScript(m_pActor), EchoActorScriptComScript(weak_from_this()), dt);
		}*/
	}

	void ActorScriptComponent::preUpdateAnimation(EventID id, ProtocolHandle handle)
	{
		/*if (!mIsPause) {
			float dt = Root::instance()->getTimeSinceLastFrameClient();
			m_luaFunction[TT_PreUpdateAnimation](EchoActorScript(m_pActor), EchoActorScriptComScript(weak_from_this()), dt);
		}*/
	}

	void ActorScriptComponent::postUpdateAnimation(EventID id, ProtocolHandle handle)
	{
		/*if (!mIsPause) {
			float dt = Root::instance()->getTimeSinceLastFrameClient();
			m_luaFunction[TT_PostUpdateAnimation](EchoActorScript(m_pActor), EchoActorScriptComScript(weak_from_this()), dt);
		}*/
	}

	void ActorScriptComponent::preUpdateScene(EventID id, ProtocolHandle handle)
	{
		/*if (!mIsPause) {
			float dt = Root::instance()->getTimeSinceLastFrameClient();
			m_luaFunction[TT_PreUpdateScene](EchoActorScript(m_pActor), EchoActorScriptComScript(weak_from_this()), dt);
		}*/
	}

	void ActorScriptComponent::postUpdateScene(EventID id, ProtocolHandle handle)
	{
		/*if (!mIsPause) {
			float dt = Root::instance()->getTimeSinceLastFrameClient();
			m_luaFunction[TT_PostUpdateScene](EchoActorScript(m_pActor), EchoActorScriptComScript(weak_from_this()), dt);
		}*/
	}

	void ActorScriptComponent::preRenderScene(EventID id, ProtocolHandle handle)
	{
	/*	if (!mIsPause) {
			float dt = Root::instance()->getTimeSinceLastFrameClient();
			m_luaFunction[TT_PreRenderScene](EchoActorScript(m_pActor), EchoActorScriptComScript(weak_from_this()), dt);
		}*/
	}

	void ActorScriptComponent::postRenderScene(EventID id, ProtocolHandle handle)
	{
		/*if (!mIsPause) {
			float dt = Root::instance()->getTimeSinceLastFrameClient();
			m_luaFunction[TT_PostRenderScene](EchoActorScript(m_pActor), EchoActorScriptComScript(weak_from_this()), dt);
		}*/
	}

	void ActorScriptComponent::EditorPlayImp()
	{
		mIsPause = false;
	}

	void ActorScriptComponent::EditorPauseImp(bool pause)
	{
		mIsPause = pause;
	}

	void ActorScriptComponent::EditorStopImp()
	{
		mIsPause = true;
		
		ActorScriptComponentInfo* actorScriptComponentInfo = static_cast<ActorScriptComponentInfo*>(m_pActComInfo);

		if (actorScriptComponentInfo) {
			this->mFloatValueMap = actorScriptComponentInfo->mFloatValueMap;
			this->mIntValueMap = actorScriptComponentInfo->mIntValueMap;
			this->mBoolValueMap = actorScriptComponentInfo->mBoolValueMap;
			this->mStringValueMap = actorScriptComponentInfo->mStringValueMap;
			this->mVector3ValueMap = actorScriptComponentInfo->mVector3ValueMap;
			this->mQuaternionValueMap = actorScriptComponentInfo->mQuaternionValueMap;
		}
	}

	float ActorScriptComponent::GetFloatValue(const std::string & name)
	{
		return mFloatValueMap.GetValue(Name(name));
	}

	void ActorScriptComponent::SetFloatValue(const std::string & name, float value)
	{
		mFloatValueMap.SetValue(Name(name), value);
	}

	int ActorScriptComponent::GetIntValue(const std::string & name)
	{
		return mIntValueMap.GetValue(Name(name));
	}

	void ActorScriptComponent::SetIntValue(const std::string & name, int value)
	{
		mIntValueMap.SetValue(Name(name), value);
	}

	bool ActorScriptComponent::GetBoolValue(const std::string & name)
	{
		return mBoolValueMap.GetValue(Name(name));
	}

	void ActorScriptComponent::SetBoolValue(const std::string & name, bool value)
	{
		mBoolValueMap.SetValue(Name(name), value);
	}

	std::string ActorScriptComponent::GetStringValue(const std::string & name)
	{
		return mStringValueMap.GetValue(Name(name));
	}

	void ActorScriptComponent::SetStringValue(const std::string & name, const std::string & value)
	{
		mStringValueMap.SetValue(Name(name), value);
	}

	Vector3 ActorScriptComponent::GetVector3Value(const std::string & name)
	{
		return mVector3ValueMap.GetValue(Name(name));
	}

	void ActorScriptComponent::SetVector3Value(const std::string & name, const Vector3 & value)
	{
		mVector3ValueMap.SetValue(Name(name), value);
	}

	Quaternion ActorScriptComponent::GetQuaternionValue(const std::string & name)
	{
		return mQuaternionValueMap.GetValue(Name(name));
	}

	void ActorScriptComponent::SetQuaternionValue(const std::string & name, const Quaternion & value)
	{
		mQuaternionValueMap.SetValue(Name(name), value);
	}

	// ------------------------------------ 获取脚本组件指针类 -------------------------------------------
	// --------------------------------------------------------------------------------------------------
	EchoActorScriptComScript::EchoActorScriptComScript(const EchoActorScript& pActor, const std::string& name) :
		EchoActorComponentScript(pActor, name)
	{}

	EchoActorScriptComScript::EchoActorScriptComScript(const ActorComponentPtr& scriptCom)
		: EchoActorComponentScript(scriptCom){}

	EchoActorScriptComScript EchoActorScriptComScript::GetActorScriptCom(const EchoActorScript& pActor, const std::string & name)
	{
		return EchoActorScriptComScript(pActor, name);
	}

	float EchoActorScriptComScript::GetFloatValue(const std::string& name)
	{
		ActorScriptComponent* pCom = static_cast<ActorScriptComponent*>(m_Ptr.get());
		if (pCom) {
			return pCom->GetFloatValue(name);
		}
		return 0.0f;
	}

	void EchoActorScriptComScript::SetFloatValue(const std::string& name, float value)
	{
		ActorScriptComponent* pCom = static_cast<ActorScriptComponent*>(m_Ptr.get());
		if (pCom) {
			pCom->SetFloatValue(name, value);
		}
	}

	int EchoActorScriptComScript::GetIntValue(const std::string& name)
	{
		ActorScriptComponent* pCom = static_cast<ActorScriptComponent*>(m_Ptr.get());
		if (pCom) {
			return pCom->GetIntValue(name);
		}
		return 0;
	}

	void EchoActorScriptComScript::SetIntValue(const std::string& name, int value)
	{
		ActorScriptComponent* pCom = static_cast<ActorScriptComponent*>(m_Ptr.get());
		if (pCom) {
			pCom->SetIntValue(name, value);
		}
	}

	bool EchoActorScriptComScript::GetBoolValue(const std::string& name)
	{
		ActorScriptComponent* pCom = static_cast<ActorScriptComponent*>(m_Ptr.get());
		if (pCom) {
			return pCom->GetBoolValue(name);
		}
		return false;
	}

	void EchoActorScriptComScript::SetBoolValue(const std::string& name, bool value)
	{
		ActorScriptComponent* pCom = static_cast<ActorScriptComponent*>(m_Ptr.get());
		if (pCom) {
			pCom->SetBoolValue(name, value);
		}
	}

	std::string EchoActorScriptComScript::GetStringValue(const std::string& name)
	{
		ActorScriptComponent* pCom = static_cast<ActorScriptComponent*>(m_Ptr.get());
		if (pCom) {
			return pCom->GetStringValue(name);
		}
		return "";
	}

	void EchoActorScriptComScript::SetStringValue(const std::string& name, const std::string& value)
	{
		ActorScriptComponent* pCom = static_cast<ActorScriptComponent*>(m_Ptr.get());
		if (pCom) {
			pCom->SetStringValue(name, value);
		}
	}

	Vector3 EchoActorScriptComScript::GetVector3Value(const std::string& name)
	{
		ActorScriptComponent* pCom = static_cast<ActorScriptComponent*>(m_Ptr.get());
		if (pCom) {
			return pCom->GetVector3Value(name);
		}
		return Vector3::ZERO;
	}

	void EchoActorScriptComScript::SetVector3Value(const std::string& name, const Vector3& value)
	{
		ActorScriptComponent* pCom = static_cast<ActorScriptComponent*>(m_Ptr.get());
		if (pCom) {
			pCom->SetVector3Value(name, value);
		}
	}

	Quaternion EchoActorScriptComScript::GetQuaternionValue(const std::string& name)
	{
		ActorScriptComponent* pCom = static_cast<ActorScriptComponent*>(m_Ptr.get());
		if (pCom) {
			return pCom->GetQuaternionValue(name);
		}
		return Quaternion::IDENTITY;
	}

	void EchoActorScriptComScript::SetQuaternionValue(const std::string& name, const Quaternion& value)
	{
		ActorScriptComponent* pCom = static_cast<ActorScriptComponent*>(m_Ptr.get());
		if (pCom) {
			pCom->SetQuaternionValue(name, value);
		}
	}

	//size_t EchoActorScriptComScript::GetChildrenComponentSize(ActorComponentPtr  p_com)
	//{
	//	if (nullptr == p_com) return 0;
	//	return p_com->getChildActorComponents().size();
	//}

	//ActorComponentPtr  EchoActorScriptComScript::GetChildComponentByIndex(ActorComponentPtr  p_com, int index)
	//{
	//	if (nullptr == p_com) return nullptr;
	//	for (auto& p_child : p_com->getChildActorComponents()) {
	//		if (index-- == 0) {
	//			return p_child;
	//		}
	//	}
	//	return nullptr;
 //	}
} // namespace Echo

  //-----------------------------------------------------------------------------
  //	Cpp File End
  //-----------------------------------------------------------------------------
