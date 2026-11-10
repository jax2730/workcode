//-----------------------------------------------------------------------------
// File:   EchoPCGGraphComponentManager.h
//
// Author: zhou_guangyuan
//
// Date:   2025-4-2
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#pragma once

#include "EchoComponentPrerequisites.h"

namespace Echo
{
	class PCGGraphComponent;
	class PCGObjectManager;

	class _EchoComponentExport PCGGraphComponentManager : public FrameListener
	{
	public:
		PCGGraphComponentManager();
		~PCGGraphComponentManager();

		void AddComponent(PCGGraphComponent* component);
		void RemoveComponent(PCGGraphComponent* component);
		const std::set<PCGGraphComponent*>& GetAllComponent() const { return m_Components; }

		virtual bool frameEnded(const FrameEvent& evt);
	protected:

		std::set<PCGGraphComponent*> m_Components;
	};

	class _EchoComponentExport PCGSystem : public Singleton<PCGSystem>
	{
	public:
		PCGSystem();
		~PCGSystem();

		bool AddComponentManager(Name name, PCGGraphComponentManager* manager);
		void RemoveComponentManager(Name name);

		PCGGraphComponentManager* getComponentManager(Name name = Name(""));

		PCGObjectManager* GetPCGObjectManager();

	private:
		std::map<Name, PCGGraphComponentManager*> m_ComponentManagerMap;
	};
}
