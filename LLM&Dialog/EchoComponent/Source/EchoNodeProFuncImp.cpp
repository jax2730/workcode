#include "EchoComponentStableHeaders.h"
#include "EchoNodeProFuncImp.h"
#include "EchoActorProFuncImp.h"
#include "Actor/EchoNodeComponent.h"

namespace Echo 
{
	void EchoNodeProFuncImp::initProtocolFunction() 
	{
		/*auto p = ProtocolComponentSystem::instance();
		p->registerFactory(Name("NodeComponent"), new NodeComponentFactory());*/
	}

	void EchoNodeProFuncImp::qtNotInitProtocolFunction()
	{
		//QT生成不同的NodeComponent这里不要直接用宏
		//auto p = ProtocolComponentSystem::instance();
		//p->registerFactory(Name("NodeComponent"), new NodeComponentFactory());
	}

	void EchoNodeProFuncImp::uninitProtocolFunction()
	{
	}

	NodeComponent * EchoNodeScript::Script_GetNodeComponent(Actor * pActor, const std::string & name)
	{
		ProtocolComponentSystem* proComponentMgr = ProtocolComponentSystem::instance();
		Echo::ProtocolHandle comHandle = 0;
		Echo::ActorComTypeDesc desc;
		desc.actHandle = (Echo::ProtocolHandle)pActor;
		desc.comType = "NodeComponent";
		desc.comName = name;
		Echo::ProtocolHandle descHandle = (Echo::ProtocolHandle)&desc;
		Echo::ProtocolResult result = proComponentMgr->callProtocolFunc(PFID_Actor_GetComponent, &comHandle, descHandle);
		if (result != PR_OK)
			return NULL;
		if (comHandle == 0)
			return NULL;
		return (NodeComponent*)comHandle;
	}
}
