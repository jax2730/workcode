#pragma once
#include "EchoComponentPrerequisites.h"

#include "PCG/EchoPCGSettings.h"
#include "PCG/EchoPCGElement.h"
#include "PCG/EchoPCGContext.h"

namespace Echo
{
	class InstancedStaticMeshComponent;

	namespace PCGV2
	{
		class PCGMeshSelectorBase : public PCGObject
		{
		public:
			PCGMeshSelectorBase();

			PCG_ABSTRACT_OBJECT_BODY(PCGMeshSelectorBase)

		};

		struct PCGMeshSelectorWeightedEntry
		{
			PCG_STRUCT_BODY()

			std::string meshPath;

			float weight = 1.0f;
		};

		class PCGMeshSelectorWeighted : public PCGMeshSelectorBase
		{
		public:
			PCGMeshSelectorWeighted();

			PCG_OBJECT_BODY(PCGMeshSelectorWeighted)

			std::vector<PCGMeshSelectorWeightedEntry> m_Entries;
		};

		struct PCGMeshInstanceList
		{
			std::vector<PCGTransform> instances;
		};

		struct PCGStaticMeshSpawnerContext : public PCGContext
		{

		};

		class PCGStaticMeshSpawnerSettings : public PCGSettings
		{
		public:
			PCGStaticMeshSpawnerSettings();

			virtual ~PCGStaticMeshSpawnerSettings();

			PCG_OBJECT_BODY(PCGStaticMeshSpawnerSettings)

			virtual PCGElement* GetElement() const override;

			virtual Name GetDefaultNodeTitle() const override { return Name("Static Mesh Spawner"); }

			virtual PCGSettings* Clone() const override;

		protected:

			virtual std::vector<PCGPinProperties> InputPinProperties() const override;
			virtual std::vector<PCGPinProperties> OutputPinProperties() const override;

			std::shared_ptr<PCGMeshSelectorBase> m_MeshSelector;
			friend class PCGStaticMeshSpawnerElement;
		};

		class PCGStaticMeshSpawnerElement : public PCGElement
		{
		public:
			static PCGElement* GetInstance();

		protected:
			bool PrepareDataInternal(PCGContext* inContext) override;

			bool ExecuteInternal(PCGContext* inContext) override;

		private:

			void SpawnStaticMeshInstances(PCGStaticMeshSpawnerContext* context, const PCGMeshInstanceList& instanceList);

			InstancedStaticMeshComponent* CreatePCGManagedISMC(EchoWeakPtr<PCGComponent> sourceComponent, ActorPtr targetActor);
		};
	}
}

