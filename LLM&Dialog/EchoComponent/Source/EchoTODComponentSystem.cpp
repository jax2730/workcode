#include "EchoComponentStableHeaders.h"
#include "EchoTODComponentSystem.h"
#include "EchoWorldSystem.h"

namespace Echo {
	TODComponentSystem::TODComponentSystem()
	{
	}
	TODComponentSystem::~TODComponentSystem()
	{
		for (const auto& iter : m_mapTODComponentManager)
		{
			TODComponentManager*  tMgr = m_mapTODComponentManager[iter.first];
			if (tMgr)
			{
				delete tMgr;
				tMgr = nullptr;
			}
			m_mapTODComponentManager[iter.first] = nullptr;
		}
	}

	TODComponentManager* TODComponentSystem::createTODComponentManager(const Name& name, ProtocolComponentManager* pm)
	{
		if(m_mapTODComponentManager.count(name) != 0)
		{
			if (m_mapTODComponentManager[name]->m_pProtocolComponentManager == nullptr)
				m_mapTODComponentManager[name]->m_pProtocolComponentManager = pm;
			return m_mapTODComponentManager[name];
		}
		TODComponentManager* tMgr = new TODComponentManager(name,pm);
		m_mapTODComponentManager.emplace(name,tMgr);

		return tMgr;
	}
	void TODComponentSystem::destroyTODComponentManager(TODComponentManager* tm)
	{
		if (tm == nullptr)
			return;
	     
		if(m_mapTODComponentManager.count(tm->mName) != 0)
		{
			 m_mapTODComponentManager.erase(tm->mName);
		}

		//tm->deinitTodComData();
		delete tm;
		tm = nullptr;
	}
	void TODComponentSystem::destroyTODComponentManager(const Name& name)
	{
		if (m_mapTODComponentManager.count(name) != 0)
		{
			TODComponentManager* tMgr = m_mapTODComponentManager[name];
			m_mapTODComponentManager[name] = nullptr;
			if (tMgr)
			{
				delete tMgr;
				tMgr = nullptr;
			}
			m_mapTODComponentManager.erase(name);
		}

		//tm->deinitTodComData();
	}
	TODComponentManager* TODComponentSystem::getTODComponentManager(const Name& name)
	{
		if(m_mapTODComponentManager.count(name) == 0)
		    return nullptr;
		return m_mapTODComponentManager[name];
	}
	TODComponentManager* TODComponentSystem::getMainTODComponentManager()
	{
		if (m_mapTODComponentManager.count(g_MainTodComponentName) == 0)
		{
			createTODComponentManager(g_MainActorManagerName,nullptr);
		}
		return m_mapTODComponentManager[g_MainTodComponentName];
	}
	void TODComponentSystem::setTodComMgrTickWay(bool isUseEchoTick)
	{
		for (const auto& iter : m_mapTODComponentManager)
		{
			if (iter.second == nullptr)
				continue;
			iter.second->setIsUseEchoTick(isUseEchoTick);
		}
	}
	TODComponentManager* TODComponentSystem::getActiveTodComManager()
	{
		auto iter = m_mapTODComponentManager.find(WorldSystem::instance()->GetActiveWorld());
		if (iter != m_mapTODComponentManager.end()) {
			return iter->second;
		}
		return nullptr;
	}
}
