#include "EchoComponentStableHeaders.h"
#include "PCG/EchoPCGManagedResource.h"

#include "PCG/EchoInstancedStaticMeshComponent.h"


namespace Echo
{
	namespace PCGV2
	{
		bool PCGManagedResource::Release()
		{
			return true;
		}

		PCGManagedComponent::~PCGManagedComponent()
		{
			Release();
		}

		PCGManagedComponent::PCGManagedComponent(ActorComponentPtr component)
			: m_GeneratedComponent(component)
		{

		}

		void PCGManagedComponent::SetComponent(ActorComponentPtr component)
		{
			m_GeneratedComponent = component;
		}

		bool PCGManagedComponent::IsValid() const
		{
			return m_GeneratedComponent && !m_GeneratedComponent.expired();
		}

		bool PCGManagedComponent::Release()
		{
			if (IsValid())
			{
				ActorPtr ownedActor = m_GeneratedComponent->getActor();
				if (ownedActor && !ownedActor.expired())
				{
					ownedActor->delComponent(m_GeneratedComponent);
				}
				SetComponent(nullptr);
			}
			return true;
		}

		PCGManagedInstancedStaticMeshComponent::PCGManagedInstancedStaticMeshComponent(ActorComponentPtr component)
			: PCGManagedComponent(component)
		{

		}

		void PCGManagedInstancedStaticMeshComponent::SetComponent(ActorComponentPtr component)
		{
			PCGManagedComponent::SetComponent(component);
			m_CachedRawComponent = nullptr;
		}

		InstancedStaticMeshComponent* PCGManagedInstancedStaticMeshComponent::GetComponent() const
		{
			if (!m_CachedRawComponent)
			{
				if (!m_GeneratedComponent.expired())
				{
					m_CachedRawComponent = static_cast<InstancedStaticMeshComponent*>(m_GeneratedComponent.get());
				}
				else
				{
					m_CachedRawComponent = nullptr;
				}
			}
			return m_CachedRawComponent;
		}


	}
}
