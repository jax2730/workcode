#include "EchoComponentStableHeaders.h"
#include "PCG/Elements/EchoPCGStaticMeshSpawner.h"

#include "PCG/EchoPCGComponent.h"
#include "PCG/EchoInstancedStaticMeshComponent.h"
#include "PCG/EchoPCGManagedResource.h"
#include "PCG/Data/EchoPCGPointData.h"

namespace Echo
{
	namespace PCGV2
	{
		PCGMeshSelectorBase::PCGMeshSelectorBase()
			: PCGObject()
		{
		}

		REGISTER_PCG_CLASS(PCGMeshSelectorBase)
		PCGClass* PCGMeshSelectorBase::StaticClass()
		{
			BEGIN_PCG_CLASS(PCGMeshSelectorBase)
			END_PCG_CLASS()
		}

		REGISTER_PCG_CLASS(PCGMeshSelectorWeightedEntry)
		PCGClass* PCGMeshSelectorWeightedEntry::StaticClass()
		{
			BEGIN_PCG_CLASS(PCGMeshSelectorWeightedEntry)
				Class.AddFilePathProperty("MeshPath", STRUCT_OFFSET(PCGMeshSelectorWeightedEntry, meshPath))
					->SetFilter(MakeFileFilterString("Mesh Files(.mesh)", "*.mesh"))
					->SetInitialDirectory("model/obj");

				Class.AddProperty("Weight", PCGPropertyType::FLOAT, STRUCT_OFFSET(PCGMeshSelectorWeightedEntry, weight));
			END_PCG_CLASS()
		}

		PCGMeshSelectorWeighted::PCGMeshSelectorWeighted()
			: PCGMeshSelectorBase()
		{

		}

		REGISTER_PCG_CLASS(PCGMeshSelectorWeighted)
		PCGClass* PCGMeshSelectorWeighted::StaticClass()
		{
			BEGIN_PCG_CLASS(PCGMeshSelectorWeighted)
				PCGProperty* entryElementProp = Class.AddStructProperty("Entry_Ele", 0, PCGMeshSelectorWeightedEntry::StaticClass(), PCGPropertyFlags::Transient)
					->SetDisplayName("WeightedEntry");
				Class.AddArrayProperty<PCGMeshSelectorWeightedEntry>("Entries", STRUCT_OFFSET(PCGMeshSelectorWeighted, m_Entries), entryElementProp);
			END_PCG_CLASS()
		}

		PCGStaticMeshSpawnerSettings::PCGStaticMeshSpawnerSettings()
			: PCGSettings()
		{
			m_MeshSelector = std::make_shared<PCGMeshSelectorWeighted>();
		}

		PCGStaticMeshSpawnerSettings::~PCGStaticMeshSpawnerSettings()
		{
		}

		REGISTER_PCG_CLASS(PCGStaticMeshSpawnerSettings)
		PCGClass* PCGStaticMeshSpawnerSettings::StaticClass()
		{
			BEGIN_PCG_CLASS(PCGStaticMeshSpawnerSettings)
				Class.AddSharedPtrProperty<PCGMeshSelectorBase>("MeshSelector", STRUCT_OFFSET(PCGStaticMeshSpawnerSettings, m_MeshSelector));
				PCGSettings::AppendPublicPCGSettingsProperties<PCGStaticMeshSpawnerSettings>(Class);
				RegisterPCGSettings("PCGStaticMeshSpawnerSettings");
			END_PCG_CLASS()
		}

		PCGElement* PCGStaticMeshSpawnerSettings::GetElement() const
		{
			return PCGStaticMeshSpawnerElement::GetInstance();
		}

		PCGSettings* PCGStaticMeshSpawnerSettings::Clone() const
		{
			return CloneT<PCGStaticMeshSpawnerSettings>();
		}

		std::vector<PCGPinProperties> PCGStaticMeshSpawnerSettings::InputPinProperties() const
		{
			std::vector<PCGPinProperties> properties;
			properties.emplace_back(PCGPinConstants::DefaultInputLabel, PCGDataType::Point);
			return properties;
		}

		std::vector<PCGPinProperties> PCGStaticMeshSpawnerSettings::OutputPinProperties() const
		{
			std::vector<PCGPinProperties> properties;
			return properties;
		}

		PCGElement* PCGStaticMeshSpawnerElement::GetInstance()
		{
			static PCGStaticMeshSpawnerElement* instance = new PCGStaticMeshSpawnerElement();
			return instance;
		}

		bool PCGStaticMeshSpawnerElement::PrepareDataInternal(PCGContext* inContext)
		{
			//assert(inContext);
			//PCGStaticMeshSpawnerContext* context = static_cast<PCGStaticMeshSpawnerContext*>(inContext);
			//std::vector<PCGTaggedData> inputs = context->inputData.GetInputs();
			//std::vector<PCGTaggedData>& outputs = context->outputData.taggedDataList;
			return true;
		}

		bool PCGStaticMeshSpawnerElement::ExecuteInternal(PCGContext* inContext)
		{
			EchoWeakPtr<PCGComponent> pcgComponent = inContext->sourceComponent;
			PCGStaticMeshSpawnerContext* context = static_cast<PCGStaticMeshSpawnerContext*>(inContext);
			ActorPtr targetActor = pcgComponent->getActor();

			MeshPtr cubeMesh = HelperObjectPreloading::instance()->GetCubeMesh();

			std::vector<PCGTaggedData> inputs = context->inputData.GetInputsByPin(PCGPinConstants::DefaultInputLabel);
			for (auto& input : inputs)
			{
				const PCGPointData* pointData = input.data;

				if (pointData && !pointData->IsEmpty())
				{
					const std::vector<PCGPoint>& points = pointData->GetPoints();
					InstancedStaticMeshComponent* ISMC = CreatePCGManagedISMC(pcgComponent, targetActor);
					ISMC->SetMesh(cubeMesh);

					for (const auto& point : points)
					{
						ISMC->AddInstance(point.transform, true);
					}
					ISMC->SpawnStaticMeshInstances();
				}
			}

			return true;
		}

		void PCGStaticMeshSpawnerElement::SpawnStaticMeshInstances(PCGStaticMeshSpawnerContext* context, const PCGMeshInstanceList& instanceList)
		{
		}

		InstancedStaticMeshComponent* PCGStaticMeshSpawnerElement::CreatePCGManagedISMC(EchoWeakPtr<PCGComponent> sourceComponent, ActorPtr targetActor)
		{
			static ActorComFactory* ISMCFactory = ActorComFactoryManager::getSingleton().GetActorComFactory(COMPONENTTYPE(InstancedStaticMeshComponent));
			static InstancedStaticMeshComponentInfo* info = static_cast<InstancedStaticMeshComponentInfo*>(ISMCFactory->createComponentInfo());
			ActorComponentPtr rootComponent = targetActor->getRootNodeCom();
			ActorComponentPtr ISMComponentPtr = targetActor->addComponent(COMPONENTTYPE(InstancedStaticMeshComponent), "InstancedStaticMeshComponent", rootComponent, info);
			PCGManagedInstancedStaticMeshComponent* pcgManagedISMComponent = new PCGManagedInstancedStaticMeshComponent(ISMComponentPtr);
			sourceComponent->AddToManagedResources(pcgManagedISMComponent);
			return pcgManagedISMComponent->GetComponent();
		}
	}
}
