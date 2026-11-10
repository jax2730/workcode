#include "EchoComponentStableHeaders.h"
#define ELUA_LIBActorScriptComponent

#define  ELUA_LIB
#include "EchoScriptComponent.h"
#include "EchoResourceFolderManager.h"

//-----------------------------------------------------------------------------
//	CPP File Begin
//-----------------------------------------------------------------------------
namespace Echo
{
	ScriptComponent::ScriptComponent(ActorPtr actor, ActorCOMMoveableType movableType)
		:ProtocolActorComponent(actor, movableType)
	{
		registerTickTimeBegin();
	}

	ScriptComponent::~ScriptComponent()
	{
	}

	void ScriptComponent::loadScript(const std::string& scriptName)
	{
		if (scriptName.empty())
		{
			return;
		}
		m_strScriptPath = scriptName;
		std::size_t posstart = scriptName.find_last_of("/\\");
		std::size_t posend = scriptName.find_last_of(".");

		if (posend == std::string::npos || posstart == std::string::npos)
		{
			return;
		}
		m_strScriptName = scriptName.substr(posstart + 1, posend - posstart - 1);

		/*m_luaFunction[TT_BeginFrame] = eLuaFunction((m_strScriptName + ".beginFrame").c_str());
		m_luaFunction[TT_EndFrame] = eLuaFunction((m_strScriptName + ".endFrame").c_str());
		m_luaFunction[TT_PreUpdateAnimation] = eLuaFunction((m_strScriptName + ".preUpdateAnimation").c_str());
		m_luaFunction[TT_PostUpdateAnimation] = eLuaFunction((m_strScriptName + ".postUpdateAnimation").c_str());
		m_luaFunction[TT_PreUpdateScene] = eLuaFunction((m_strScriptName + ".preUpdateScene").c_str());
		m_luaFunction[TT_PostUpdateScene] = eLuaFunction((m_strScriptName + ".postUpdateScene").c_str());
		m_luaFunction[TT_PreRenderScene] = eLuaFunction((m_strScriptName + ".preRenderScene").c_str());

		m_luaFunction[TT_PostRenderScene] = eLuaFunction((m_strScriptName + ".postRenderScene").c_str());

		GetScriptManager()->LoadScriptModule(scriptName);*/
	}

	void ScriptComponent::registerTickTimeBegin()
	{
		/*	m_eventSet[TT_BeginFrame].insert(EID_EveryFrame);
			m_eventSet[TT_EndFrame].insert(EID_EveryFrame);
			m_eventSet[TT_PreUpdateAnimation].insert(EID_EveryFrame);
			m_eventSet[TT_PostUpdateAnimation].insert(EID_EveryFrame);
			m_eventSet[TT_PreRenderScene].insert(EID_EveryFrame);
			m_eventSet[TT_PostRenderScene].insert(EID_EveryFrame);
			m_eventSet[TT_PreUpdateScene].insert(EID_EveryFrame);
			m_eventSet[TT_PostUpdateScene].insert(EID_EveryFrame);
			registerTickTimeEnd();*/
	}

	void ScriptComponent::beginFrame(EventID, ProtocolHandle)
	{
		//m_luaFunction[TT_BeginFrame]();
	}

	void ScriptComponent::endFrame(EventID, ProtocolHandle)
	{
		//m_luaFunction[TT_EndFrame]();
	}

	void ScriptComponent::preUpdateAnimation(EventID, ProtocolHandle)
	{
		//m_luaFunction[TT_PreUpdateAnimation]();
	}

	void ScriptComponent::postUpdateAnimation(EventID, ProtocolHandle)
	{
		//m_luaFunction[TT_PostUpdateAnimation]();
	}

	void ScriptComponent::preUpdateScene(EventID, ProtocolHandle)
	{
		//m_luaFunction[TT_PreUpdateScene]();
	}

	void ScriptComponent::postUpdateScene(EventID, ProtocolHandle)
	{
		//m_luaFunction[TT_PostUpdateScene]();
	}

	void ScriptComponent::preRenderScene(EventID, ProtocolHandle)
	{
		//m_luaFunction[TT_PreRenderScene]();
	}

	void ScriptComponent::postRenderScene(EventID, ProtocolHandle)
	{
		//m_luaFunction[TT_PostRenderScene]();
	}
} // namespace Echo

//-----------------------------------------------------------------------------
//	Cpp File End
//-----------------------------------------------------------------------------
