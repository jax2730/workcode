#include "EchoComponentStableHeaders.h"
#include "PCG/EchoPCGComponent.h"

#include "PCG/EchoPCGSystem.h"
#include "PCG/EchoPCGManagedResource.h"

namespace Echo
{
	using namespace PCGV2;

	PCGComponent::PCGComponent(ActorPtr pActor, ActorCOMMoveableType type)
		: ActorComponent(pActor, type)
	{
	}

	PCGComponent::~PCGComponent()
	{
		CleanupAllManagedResources();
	}

	void PCGComponent::SetGraphPath(const std::string& path)
	{
		m_GraphPath = path;

		if (!m_GraphPath.empty())
		{
			try
			{
				m_Graph = PCGGraphManager::instance()->load(Name(m_GraphPath));
			}
			catch (...)
			{
				m_Graph.setNull();
			}
		}
		else
		{
			m_Graph.setNull();
		}
	}

	void PCGComponent::SetGraph(PCGV2::PCGGraphPtr graph)
	{
		m_Graph = graph;
		m_GraphPath = m_Graph->getName().toString();
	}

	void PCGComponent::AddToManagedResources(PCGManagedResource* resource)
	{
		if (resource && resource->IsValid())
		{
			m_GeneratedResources.push_back(resource);
		}
	}

	void PCGComponent::ForEachManagedResource(std::function<void(PCGManagedResource*)> func)
	{
		for (auto& resource : m_GeneratedResources)
		{
			if (resource)
			{
				func(resource);
			}
		}
	}

	void PCGComponent::PostProcessGraph()
	{
		CleanupUnusedManagedResources();
		m_bGenerationInProgress = false;
	}

	void PCGComponent::CleanupUnusedManagedResources()
	{
		auto cleanupBeginIt = std::remove_if(m_GeneratedResources.begin(), m_GeneratedResources.end(), [](PCGManagedResource* resource)
			{
				return !resource->IsValid();
			});
		std::for_each(cleanupBeginIt, m_GeneratedResources.end(), [](PCGManagedResource* resource)
			{
				delete resource;
			});

		m_GeneratedResources.erase(cleanupBeginIt, m_GeneratedResources.end());
	}

	void PCGComponent::CleanupAllManagedResources()
	{
		for (PCGManagedResource* resource : m_GeneratedResources)
		{
			delete resource;
		}
		m_GeneratedResources.clear();
	}

	void PCGComponent::Generate()
	{
		CleanupAllManagedResources();
		if (IsGraphPrepared())
		{
			GenerateInternal();
		}
	}

	bool PCGComponent::IsGraphPrepared()
	{
		return !m_Graph.isNull();
	}

	void PCGComponent::init(ActorComponentInfo* info)
	{
		if (info)
		{
			PCGComponentInfo* pcgInfo = static_cast<PCGComponentInfo*>(info);
			SetGraphPath(pcgInfo->GetGraphPath());
			SetBounds(pcgInfo->GetBounds());
		}
	}

	void PCGComponent::unInit()
	{
		CleanupAllManagedResources();
	}

	PCGComponentInfo* PCGComponent::GetComponentInfo() const
	{
		return static_cast<PCGComponentInfo*>(m_pActComInfo);
	}

	PCGV2::PCGSystem* PCGComponent::GetPCGSystem() const
	{
		return PCGV2::PCGSystem::GetInstance();
	}

	PCGTaskId PCGComponent::GenerateInternal()
	{
		m_CurrentGenerationTask = GetPCGSystem()->ScheduleComponent(this->weak_from_this());

		return m_CurrentGenerationTask;
	}

	PCGComponentInfo::PCGComponentInfo(const Name& type, ActorCOMMoveableType movable)
		: ActorComponentInfo(type, movable)
	{

	}

	DECLAREACTORCOMTYPE_CPP(PCGComponent)
		EDITOR_REFLECT_BEGIN(PCGComponentInfo)
		EDITOR_REFLECT_MEMBER_UPDATE(m_GraphPath, "GraphPath", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_Bounds, "Bounds", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_END();
	_DEFINEINFO_VIRFUN_BASE(PCGComponentInfo)
}
