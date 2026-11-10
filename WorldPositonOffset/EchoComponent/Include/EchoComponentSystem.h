#ifndef __EchoComponentSystem_H__
#define __EchoComponentSystem_H__

#include "EchoComponentPrerequisites.h"
#include "EchoSingleton.h"
#include "EchoProtocolComponent.h"

namespace Echo
{
	enum ComSystemRegisterList
	{
		COMSYSTEMREMEMPTYLIST		= 0,
		//Component System里会注册的相关功能，可以通过组合独立注册使用
		ECHO3UICOMREGISTER			= 1 << 0, //引擎编辑器使用的3DUI组件，客户端时不注册
		ECHOTESTSCRIPTREGISTER		= 1 << 1, //引擎编辑器测试的胶水脚本，客户端时不注册
		ECHOCOMKEYBOARDREGISTER		= 1 << 2, //场景编辑器测试的KeyBoard映射，客户端时不注册
		
		QTANDECHODIFFPROFUNCTION	= 1 << 3, //场景编辑器和其他运行模式不同的注册函数

		SUTOECHOCOMREGISTER			= 1 << 10, //SU to Echo 的组件，客户端时不注册

		//根据已有的项目，组合好需要注册的功能可以直接使用
		PADBO2COMREGISTERLIST		= ECHO3UICOMREGISTER | ECHOTESTSCRIPTREGISTER | SUTOECHOCOMREGISTER ,
		QTEDITORCOMREGISTERLIST		= ECHO3UICOMREGISTER | ECHOTESTSCRIPTREGISTER | SUTOECHOCOMREGISTER | QTANDECHODIFFPROFUNCTION | ECHOCOMKEYBOARDREGISTER  ,
		SUCLIENTCOMREGISTERLIST
	};

	class _EchoComponentExport ComponentSystem : public Singleton<ComponentSystem>
	{
	public:
		ComponentSystem();
		~ComponentSystem();

		//QT Init : SceneActor在QT的场景编辑器里相关逻辑，与其他编辑器和运行时逻辑不同。
		//qtInit为true时，不会注册相关需要组件和注册函数。需要在场景编辑器里单独注册。
		//Echo 3DUI : 3DUI使用的FGUI对应的component，现在引擎和客户端会各有一份。
		//echo3DUI为true时，使用EchoComponent中的3DUI组件。
		void					init(bool qtInit = false,bool echo3DUI = false);
		void					init(ComSystemRegisterList list);
		void                    shutDown();
		void					deinit();
		//现有结构的临时写法，需要移到WorldManager
		void                    initManager(const Name& name);
		void                    deinitManager(const Name& name);

	public:
		inline bool				getIsQtInit() const { return (mRegisterList & QTANDECHODIFFPROFUNCTION); }
		inline bool				getIsEcho3DUI() const { return (mRegisterList & ECHO3UICOMREGISTER); }
		inline bool             getIsEchoEditor() const { return (mRegisterList & SUTOECHOCOMREGISTER); }
	private:
		void					initProtocolFunction();
		void					uninitProtocolFunction();

		void					qtNotInitProtocolFunction();
		void					qtNotUninitProtocolFunction();

		void					registerFactory();
		void					Echo3DUIRegisterFactory();
		void					registerSUToEchoComFactory();
	private:
		int						mRegisterList;
	};
} // namespace Echo

//-----------------------------------------------------------------------------
//	Head File End
//-----------------------------------------------------------------------------
#endif
