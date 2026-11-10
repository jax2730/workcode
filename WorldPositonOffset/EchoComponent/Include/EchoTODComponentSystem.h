#ifndef __EchoTodComponentSystem_H__
#define __EchoTodComponentSystem_H__
#include "EchoEnginePrerequisites.h"
#include "Actor/EchoActorManager.h"
#include "EchoTODComponentManager.h"


namespace Echo 
{
	static const Name g_MainTodComponentName = Name("Main");

	class _EchoComponentExport TODComponentSystem : public Singleton<TODComponentSystem>
	{
	public:
		TODComponentSystem();
		~TODComponentSystem();

		TODComponentManager*            createTODComponentManager(const Name& name, ProtocolComponentManager* tm);
		void							destroyTODComponentManager(TODComponentManager* tm);
		void							destroyTODComponentManager(const Name& name);
		TODComponentManager*            getTODComponentManager(const Name& name);
		TODComponentManager*            getMainTODComponentManager();

		void                            setTodComMgrTickWay(bool isUseEchoTick);

		TODComponentManager*            getActiveTodComManager();

#ifdef ECHO_EDITOR
		bool							stopUI = false;
#endif

	private:
		std::map<Name, TODComponentManager*>		m_mapTODComponentManager;
	};

	// isTranslation 判断当前组件是否支持过渡操作
	#define TODCOMPONENT_INITMGR(ContentFunc, Mgr, isTranslation)						\
		{																				\
			Name sceneName = g_MainActorManagerName;									\
			if (m_pActor)																\
			{																			\
				SceneManager* sMgr = m_pActor->GetSceneManager();						\
				if (sMgr)																\
					sceneName = sMgr->getName();										\
			}																			\
			Mgr = TODComponentSystem::instance()->getTODComponentManager(sceneName);	\
			if(Mgr)																		\
			{																			\
				Mgr->ContentFunc().mIsTranslation = isTranslation;						\
				if (getRealVisibleState())												\
					Mgr->pushComponent(Mgr->ContentFunc(), this);						\
			}																			\
		}
	
	// isTranslation 判断当前组件是否支持过渡操作
	#define TODCOMPONENT_INITSCENEMGR(ContentFunc, Mgr, isTranslation, sMgr)			\
	{																					\
			Name sceneName = g_MainActorManagerName;									\
			if (m_pActor)																\
			{																			\
				sMgr = m_pActor->GetSceneManager();										\
				if (sMgr)																\
					sceneName = sMgr->getName();										\
			}																			\
			Mgr = TODComponentSystem::instance()->getTODComponentManager(sceneName);	\
			if(Mgr)																		\
			{																			\
				Mgr->ContentFunc().mIsTranslation = isTranslation;						\
				if (getRealVisibleState())												\
					Mgr->pushComponent(Mgr->ContentFunc(), this);						\
			}																			\
		}
	
	// type为TodComType类型，若存在则填入存在类型，不存在为T_End
	#define TODCOMPONENT_UNINITMGR(ContentFunc, Mgr, type)		\
	{															\
		if(Mgr)													\
		{														\
			Mgr->popComponent(Mgr->ContentFunc(), this);		\
			Mgr->clearComInFinalComVec(this, type);				\
		}														\
		Mgr = nullptr;											\
	}
	
}
#endif
