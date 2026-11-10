#include "EchoComponentStableHeaders.h"
#include "PCG/EchoInstancedStaticMeshComponent.h"

namespace Echo
{
	DECLAREACTORCOMTYPE_CPP(InstancedStaticMeshComponent);

	InstancedStaticMeshComponent::InstancedStaticMeshComponent(ActorPtr pActor, ActorCOMMoveableType type)
		: ActorComponent(pActor, type)
	{
		m_SceneManager = pActor->GetSceneManager();
	}

	InstancedStaticMeshComponent::~InstancedStaticMeshComponent()
	{
		DestroyGeneratedInstances();
	}

	uint32 InstancedStaticMeshComponent::AddInstance(const PCGTransform& transform, bool bWorldSpace /*= false*/)
	{
		PCGTransform localTransform = transform;
		if (bWorldSpace) {
			localTransform.translation = convertWorldToLocalPosition(transform.translation);
			localTransform.rotation = convertWorldToLocalRotation(transform.rotation);
			localTransform.scale = convertWorldToLocalScale(transform.scale);
		}
		m_InstanceTransforms.push_back(std::move(localTransform));

		return static_cast<uint32>(m_InstanceTransforms.size() - 1);
	}

	std::vector<uint32> InstancedStaticMeshComponent::AddInstances(const std::vector<PCGTransform>& transforms, bool bWorldSpace /*= false*/)
	{
		const uint32 count = static_cast<uint32>(transforms.size());

		std::vector<uint32> newInstanceIndices;
		newInstanceIndices.reserve(count);

		uint32 instanceIndex = static_cast<uint32>(m_InstanceTransforms.size());

		for (const auto& transform : transforms)
		{
			newInstanceIndices.push_back(instanceIndex);
			PCGTransform localTransform = bWorldSpace ? PCGTransform::Multiply(getWorldMatrix().inverse(), transform) : transform;
			m_InstanceTransforms.push_back(std::move(localTransform));
			instanceIndex++;
		}
		return newInstanceIndices;
	}

	void InstancedStaticMeshComponent::ClearInstances()
	{
		DestroyGeneratedInstances();
		m_InstanceTransforms.clear();
	}

	void InstancedStaticMeshComponent::SpawnStaticMeshInstances()
	{
		DestroyGeneratedInstances();

		Actor* actor = getActorPtr();
		Name entityName = Name(actor->getCombinedID() + +"&&InstancedStaticMeshComponent");
		SceneNode* actorRootNode = (SceneNode*)actor->getRootNodeCom()->getSceneNode();
		m_GeneratedInstances.reserve(m_InstanceTransforms.size());
		for (const auto& transform : m_InstanceTransforms)
		{
			Entity* entity = CreateStaticMeshEntityInternal(entityName);
			SceneNode* entityNode = m_SceneManager->createSceneNode();
			entityNode->setPosition(transform.translation);
			entityNode->setOrientation(transform.rotation);
			entityNode->setScale(transform.scale);
			entityNode->attachObject(entity);
			actorRootNode->addChild(entityNode);

			m_GeneratedInstances.emplace_back(entityNode, entity);
		}
		actorRootNode->_update(true);
		actorRootNode->_updateBounds();
	}

	void InstancedStaticMeshComponent::DestroyGeneratedInstances()
	{
		for (GeneratedInstance& generated : m_GeneratedInstances)
		{
			if (generated.entity)
			{
				generated.entity->detachFromParent();
				m_SceneManager->destroyEntity(generated.entity);
			}

			delete generated.sceneNode;
		}

		m_GeneratedInstances.clear();
	}

	Entity* InstancedStaticMeshComponent::CreateStaticMeshEntityInternal(const Name& entityName)
	{
		SceneManager* scene = getSceneManager();
		Entity* entity = scene->createEntity(entityName, m_Mesh, true, SceneEntity, true);

		// TODO: config entity settings
		entity->setCheckCulled(true);
		entity->setFPState(FP_STATE_NONE);
		entity->setCameraCullMode(CameraCullMode::DistanceCull);
		entity->enableCastShadow(false);
		// entity->setRenderingDistance(mRenderDistance);
		return entity;
	}

	InstancedStaticMeshComponentInfo::InstancedStaticMeshComponentInfo(const Name& type, ActorCOMMoveableType movable)
		: ActorComponentInfo(type, movable)
	{

	}
	_DEFINEINFO_VIRFUN_BASE(InstancedStaticMeshComponentInfo);
}
