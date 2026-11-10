#pragma once

#include "EchoComponentPrerequisites.h"


namespace Echo
{
	class InstancedStaticMeshComponent;

	namespace PCGV2
	{
		class PCGManagedResource
		{
		public:
			virtual ~PCGManagedResource() = default;

			virtual bool Release();

			virtual bool IsValid() const = 0;
		};

		// A wrapper for PCG generated component
		class PCGManagedComponent : public PCGManagedResource
		{
		public:
			virtual ~PCGManagedComponent();

			PCGManagedComponent(ActorComponentPtr component);

			virtual void SetComponent(ActorComponentPtr component);

			virtual bool IsValid() const;

			virtual bool Release();

		protected:
			ActorComponentPtr m_GeneratedComponent;
		};

		// A wrapper for PCG generated InstancedStaticMeshComponent
		class PCGManagedInstancedStaticMeshComponent : public PCGManagedComponent
		{
		public:
			PCGManagedInstancedStaticMeshComponent(ActorComponentPtr component);

			virtual void SetComponent(ActorComponentPtr component);

			InstancedStaticMeshComponent* GetComponent() const;

		private:

			mutable InstancedStaticMeshComponent* m_CachedRawComponent = nullptr;
		};
	}
}
