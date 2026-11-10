// --------------------------------------------------------------------------------------
// File:   EchoActorScriptComponent.h
// 
// Author: Chen ShiWen
//
// Date:   2022-7-4
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
// --------------------------------------------------------------------------------------

#ifndef __EchoActorScriptComponent_H__
#define __EchoActorScriptComponent_H__

#include "EchoComponentPrerequisites.h"
#include "EchoScriptComponent.h"
#include "EchoComDefineMacro.h"
//-----------------------------------------------------------------------------
//	Head File Begin
//-----------------------------------------------------------------------------

#define USE_VARIANT false

#if USE_VARIANT
#include <variant>
#endif

namespace Echo
{	
	// ----------------------------- Try using templates for code reuse ------------------------------------------------------------------
	template<class T>
	class ValueMap {
	public:
		T GetValue(const Name& name) {
			if (value_map.find(name) == value_map.end()) {
				return {};
			}
			return value_map[name];
		}

		void SetValue(const Name& name, const T& value) {
			if (value_map.find(name) == value_map.end()) return;
			value_map[name] = value;
		}

		void AddValue(const Name& name, const T& value) {
			if (value_map.find(name) == value_map.end()) {
				value_map.emplace(name, value);
			}
		}

		void RemoveValue(const Name& name) {
			if (value_map.find(name) != value_map.end()) {
				value_map.erase(name);
			}
		}

		const std::unordered_map<Name, T, NameIDHasher>& GetValueMap() const {
			return value_map;
		}

	public:
		void ExportJson(cJSON* jsonNode) const {
			for (const auto& pair : value_map) {
				WriteJSNode(jsonNode, pair.first.c_str(), pair.second);
			}
		}

		bool ImportJson(cJSON* jsonNode) {
			if (jsonNode == nullptr) return false;
			cJSON* values = jsonNode->child;
			while (values) {
				T value = {};
				ReadJSNode(values->parent, values->string, value);
				value_map.emplace(values->string, value);

				values = values->next;
			}
			return true;
		}

	public:
		static int reflectFlag;

	protected:
		std::unordered_map<Name, T, NameIDHasher> value_map;
	};

	// ------------------------------------------------------------------------------------------------------------------------------------------
	class ActorComponentInfo;

	class _EchoComponentExport ActorScriptComponent : public ScriptComponent
	{
		DECLAREACTORCOMTYPE_H(ActorScriptComponent)
	public:
		REGISTER_LUA_CLASS_KEY;
	public:
		void											init(ActorComponentInfo* info) override;
		void											copyTo(const ActorComponentPtr&  p_actor_component) override;

		// -----------------------------------------------------------------
		void											beginFrame(EventID id, ProtocolHandle handle) override;
		void											endFrame(EventID id, ProtocolHandle handle) override;
		void											preUpdateAnimation(EventID id, ProtocolHandle handle) override;
		void											postUpdateAnimation(EventID id, ProtocolHandle handle) override;

		void											preUpdateScene(EventID id, ProtocolHandle handle) override;
		void											postUpdateScene(EventID id, ProtocolHandle handle) override;
		void											preRenderScene(EventID id, ProtocolHandle handle) override;
		void											postRenderScene(EventID id, ProtocolHandle handle) override;
		// ------------------------------------------------ play pause and stop -----------------------------------------------
	private:
		void											EditorPlayImp() override;
		void											EditorPauseImp(bool pause) override;
		void											EditorStopImp() override;

		// ------------------------------------------------ script interface --------------------------------------------------
	public:
		float											GetFloatValue(const std::string& name);
		void											SetFloatValue(const std::string& name, float value);

		int												GetIntValue(const std::string& name);
		void											SetIntValue(const std::string& name, int value);

		bool											GetBoolValue(const std::string& name);
		void											SetBoolValue(const std::string& name, bool value);

		std::string										GetStringValue(const std::string& name);
		void											SetStringValue(const std::string& name, const std::string& value);

		Vector3											GetVector3Value(const std::string& name);
		void											SetVector3Value(const std::string& name, const Vector3& value);

		Quaternion										GetQuaternionValue(const std::string& name);
		void											SetQuaternionValue(const std::string& name, const Quaternion& value);

	public:
		ValueMap<float>									mFloatValueMap;
		ValueMap<int>									mIntValueMap;
		ValueMap<bool>									mBoolValueMap;
		ValueMap<std::string>							mStringValueMap;
		ValueMap<Vector3>								mVector3ValueMap;
		ValueMap<Quaternion>							mQuaternionValueMap;

	private:
		bool											mIsPause = true; ///< default is pause
	};

	// ----------------------------------------------------- class for initialize script component ----------------------------------------------
	class _EchoComponentExport ActorScriptComponentInfo : public ActorComponentInfo {
	public:
		ActorScriptComponentInfo() = default;
		ActorScriptComponentInfo(const Name& type, ActorCOMMoveableType movable);
		~ActorScriptComponentInfo() = default;

		void											ExportData(cJSON* jsonNode) override;
		void											ImportData(cJSON* jsonNode) override;
		void											CopyTo(ActorComponentInfo* p_actor_com_info) override;
		void											OverWrite() override;

	public:
		EDITOR_REFLECT
	
	public:
		std::string										m_ScriptPath = "";

	public:
		ValueMap<float>									mFloatValueMap;
		ValueMap<int>									mIntValueMap;
		ValueMap<bool>									mBoolValueMap;
		ValueMap<std::string>							mStringValueMap;
		ValueMap<Vector3>								mVector3ValueMap;
		ValueMap<Quaternion>							mQuaternionValueMap;
	};

	// 脚本组件工厂类
	DEFINECOMFACTORY(ActorScriptComponent,UNMOVEABLE)

	class EchoActorScriptComScript : public EchoActorComponentScript
	{
	public:
		REGISTER_LUA_CLASS_KEY;

	public:
		EchoActorScriptComScript() = default;
		EchoActorScriptComScript(const EchoActorScript& pActor, const std::string& name);
		EchoActorScriptComScript(const ActorComponentPtr& scriptCom);

	public:
		static EchoActorScriptComScript GetActorScriptCom(const EchoActorScript& pActor, const std::string& name);
		
		float GetFloatValue(const std::string& name);
		void SetFloatValue(const std::string& name, float value);

		int GetIntValue(const std::string& name);
		void SetIntValue(const std::string& name, int value);

		bool GetBoolValue(const std::string& name);
		void SetBoolValue(const std::string& name, bool value);

		std::string GetStringValue(const std::string& name);
		void SetStringValue(const std::string& name, const std::string& value);

		Vector3 GetVector3Value(const std::string& name);
		void SetVector3Value(const std::string& name, const Vector3& value);

		Quaternion GetQuaternionValue(const std::string& name);
		void SetQuaternionValue(const std::string& name, const Quaternion& value);
	};
} // namespace Echo

  //-----------------------------------------------------------------------------
  //	Head File End
  //-----------------------------------------------------------------------------
#endif