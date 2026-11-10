//-----------------------------------------------------------------------------
// File:   EchoPCGGraphComponentManager.cpp
//
// Author: zhou_guangyuan
//
// Date:   2025-4-2
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include "EchoComponentStableHeaders.h"
#include "EchoPCGGraphComponentManager.h"

#include "EchoPCGGraphComponent.h"
#include "EchoPCGObject.h"
#include "EchoSphericalTerrainManager.h"
#include "EchoSphericalTerrainPCGGenScheduler.h"
#include "EchoActorEntityComponent.h"
#include "EchoEntityObject.h"
#include "EchoPCGBaseElement.h"

namespace Echo
{
	bool gCalculateActorEntityTriangleCount(const ActorPtr& pActor, uint32& triangleCount) {
		std::vector<EntityComponentPtr> entitys = pActor->getComponentsByType<EntityComponent, EntityComponent>(true);
		bool bDraw = false;
		triangleCount = 0;
		for (auto& entity : entitys) {
			EntityObject* pObj = entity->getEntityObject();
			if (pObj) {
				if (!pObj->getCulled()) {
					triangleCount += pObj->getResourceTriangleCount();
					bDraw = true;
				}
				pObj->setCulled(true);
			}
		}
		return bDraw;
	}
}

namespace Echo
{
	PCGGraphComponentManager::PCGGraphComponentManager()
	{
		Echo::Root::instance()->addFrameListener(this);
	}


	PCGGraphComponentManager::~PCGGraphComponentManager()
	{
		Echo::Root::instance()->removeFrameListener(this);
	}


	void PCGGraphComponentManager::AddComponent(PCGGraphComponent* component)
	{
		if (component)
		{
			m_Components.insert(component);
		}
	}

	void PCGGraphComponentManager::RemoveComponent(PCGGraphComponent* component)
	{
		auto it = m_Components.find(component);
		if (it != m_Components.end())
		{
			m_Components.erase(it);
		}
	}

	bool PCGGraphComponentManager::frameEnded(const FrameEvent& evt)
	{
		for (auto* component : m_Components)
		{
			if (component->NeedExcutePCGGraph())
			{
				component->ExcutePCGGraph();
			}
		}
		return true;
	}

	bool PCGSystem::AddComponentManager(Name name, PCGGraphComponentManager* manager)
	{
		if (!manager)
		{
			return false;
		}

		auto it = m_ComponentManagerMap.find(name);
		if (it != m_ComponentManagerMap.end())
		{
			return false;
		}

		m_ComponentManagerMap[name] = manager;
		return true;
	}

	void PCGSystem::RemoveComponentManager(Name name)
	{
		auto it = m_ComponentManagerMap.find(name);
		if (it != m_ComponentManagerMap.end())
		{
			m_ComponentManagerMap.erase(it);
		}
	}

	Echo::PCGGraphComponentManager* PCGSystem::getComponentManager(Name name)
	{
		auto it = m_ComponentManagerMap.find(name);
		return it != m_ComponentManagerMap.end() ? it->second : nullptr;
	}

	PCGObjectManager* PCGSystem::GetPCGObjectManager()
	{
		return PCGObjectManager::GetInstance();
	}

	PCGSystem::PCGSystem()
	{
		// Create default component manager for empty name
		m_ComponentManagerMap[Name("")] = new PCGGraphComponentManager();
		SphericalTerrain::gNewPlanetGenObjectScheduler = SphericalTerrainPCGGenScheduler::gCreateFunction;
		PCG::PCGGeneralObjectCache::gGetActorTriangleCountFun = gCalculateActorEntityTriangleCount;
	}


	PCGSystem::~PCGSystem()
	{
		for (auto& iter : m_ComponentManagerMap)
		{
			SAFE_DELETE(iter.second);
		}
		m_ComponentManagerMap.clear();
	}


}

