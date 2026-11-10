#ifndef __ECHO_NODEPROFUNCIMP_H__
#define __ECHO_NODEPROFUNCIMP_H__

#include "EchoComponentPrerequisites.h"
#include "EchoProtocolComponent.h"
#include "Actor/EchoNodeComponent.h"

namespace Echo {

	class _EchoComponentExport EchoNodeProFuncImp
	{
	public:
		//-----------------------------------------------------------------------------
		static void								initProtocolFunction();
		static void								qtNotInitProtocolFunction();
		static void								uninitProtocolFunction();
	};

	class EchoNodeScript
	{
	public:
		REGISTER_LUA_CLASS_KEY;
		//static bool								Script_ShowBox(Echo::ProtocolHandle actHandle, const std::string& actID, bool isShow);
		static NodeComponent*						Script_GetNodeComponent(Actor* pActor, const std::string& name);
	};
}

#endif 