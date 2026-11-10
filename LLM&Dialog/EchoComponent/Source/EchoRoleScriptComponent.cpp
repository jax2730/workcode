#include "EchoComponentStableHeaders.h"
#define ELUA_LIB
#include "EchoRoleScriptComponent.h"
#include "EchoStringConverter.h"
#include "EchoRoleObject.h"

//-----------------------------------------------------------------------------
//	CPP File Begin
//-----------------------------------------------------------------------------
namespace Echo
{
	RoleScriptComponent::RoleScriptComponent()
		:ScriptComponent(NULL,UNMOVEABLE)
	{
		loadScript("echores/script/rolecontrol.escript");
	}

	RoleScriptComponent::~RoleScriptComponent()
	{

	}

	void RoleScriptComponent::setRoleObj(RoleObject* pRoleObj)
	{
		m_pRoleObj = pRoleObj;
	}

	void RoleScriptComponent::beginFrame(EventID id, ProtocolHandle handle)
	{
		//float dt = Root::instance()->getTimeSinceLastFrameClient();
		
		//m_luaFunction[TT_BeginFrame](m_pRoleObj,dt);
	}

	void RoleScriptComponent::endFrame(EventID id, ProtocolHandle handle)
	{

	}

	void RoleScriptComponent::preUpdateAnimation(EventID id, ProtocolHandle handle)
	{

	}

	void RoleScriptComponent::postUpdateAnimation(EventID id, ProtocolHandle handle)
	{

	}

	void RoleScriptComponent::preUpdateScene(EventID id, ProtocolHandle handle)
	{

	}

	void RoleScriptComponent::postUpdateScene(EventID id, ProtocolHandle handle)
	{

	}

	void RoleScriptComponent::preRenderScene(EventID id, ProtocolHandle handle)
	{

	}

	void RoleScriptComponent::postRenderScene(EventID id, ProtocolHandle handle)
	{

	}

} // namespace Echo

//-----------------------------------------------------------------------------
//	Cpp File End
//-----------------------------------------------------------------------------
