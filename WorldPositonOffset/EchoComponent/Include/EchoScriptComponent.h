#ifndef __EchoScriptComponent_H__
#define __EchoScriptComponent_H__

#include "EchoComponentPrerequisites.h"
#include "Actor/EchoProtocolActorComponent.h"
#include "EchoScriptManager.h"
//#include "eLuaFunction.h"


//-----------------------------------------------------------------------------
//	Head File Begin
//-----------------------------------------------------------------------------

namespace Echo
{
	//=============================================================================
	// 1. ScripComponentManager tick run the scrip thread ; query return now / lock ?
	// 2. LUA call must be pack ...
	class _EchoComponentExport ScriptComponent : public ProtocolActorComponent
	{
	public:
		ScriptComponent(ActorPtr actor, ActorCOMMoveableType movableType);

		virtual ~ScriptComponent();
	public:
		void loadScript(const std::string& scriptName);
		std::string getStrScriptPath() { return m_strScriptPath; }

		void											init(ActorComponentInfo* info) override {};
		//void											uninit() {};

		void											registerTickTimeBegin() override;
		virtual void									beginFrame(EventID, ProtocolHandle) override;
		virtual void									endFrame(EventID, ProtocolHandle) override;
		virtual void									preUpdateAnimation(EventID, ProtocolHandle) override;
		virtual void									postUpdateAnimation(EventID, ProtocolHandle) override;

		virtual void									preUpdateScene(EventID, ProtocolHandle) override;
		virtual void									postUpdateScene(EventID, ProtocolHandle) override;
		virtual void									preRenderScene(EventID, ProtocolHandle) override;
		virtual void									postRenderScene(EventID, ProtocolHandle) override;;
	protected:
		std::string										m_strScriptName;
		std::string										m_strScriptPath;
		//eLuaFunction									m_luaFunction[TT_Count];
	};
} // namespace Echo

//-----------------------------------------------------------------------------
//	Head File End
//-----------------------------------------------------------------------------
#endif