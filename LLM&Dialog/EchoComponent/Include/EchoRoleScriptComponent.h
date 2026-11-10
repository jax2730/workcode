#ifndef __EchoRoleScriptComponent_H__
#define __EchoRoleScriptComponent_H__

#include "EchoComponentPrerequisites.h"
#include "EchoProtocolComponent.h"
#include "EchoEnginePrerequisites.h"
#include "EchoScriptComponent.h"
//-----------------------------------------------------------------------------
//	Head File Begin
//-----------------------------------------------------------------------------

namespace Echo
{

	class _EchoComponentExport RoleScriptComponent : public ScriptComponent
	{
	public:
		RoleScriptComponent();

		virtual ~RoleScriptComponent();
	public:
		
		void setRoleObj(RoleObject* pRoleObj);
		

		virtual void beginFrame(EventID id, ProtocolHandle handle) override;
		virtual void endFrame(EventID id, ProtocolHandle handle) override;
		virtual void preUpdateAnimation(EventID id, ProtocolHandle handle) override;
		virtual void  postUpdateAnimation(EventID id, ProtocolHandle handle) override;

		virtual void  preUpdateScene(EventID id, ProtocolHandle handle) override;
		virtual void  postUpdateScene(EventID id, ProtocolHandle handle) override;
		virtual void								preRenderScene(EventID id, ProtocolHandle handle) override;
		virtual void								postRenderScene(EventID id, ProtocolHandle handle) override;
	private:
		RoleObject* m_pRoleObj = NULL;
	};
} // namespace Echo

//-----------------------------------------------------------------------------
//	Head File End
//-----------------------------------------------------------------------------
#endif