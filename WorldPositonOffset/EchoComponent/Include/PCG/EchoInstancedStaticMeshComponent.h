#pragma once

#include "EchoComponentPrerequisites.h"

#include "Actor/EchoActorComponent.h"
#include "EchoComDefineMacro.h"

#include "PCG/Data/EchoPCGDataCommon.h"

namespace Echo
{
	using PCGV2::PCGTransform;

	class _EchoComponentExport InstancedStaticMeshComponent : public ActorComponent
	{
		DECLAREACTORCOMTYPE_H(InstancedStaticMeshComponent)
	public:
		REGISTER_LUA_CLASS_KEY;

		struct GeneratedInstance
		{
			SceneNode* sceneNode = nullptr;
			Entity* entity = nullptr;

			GeneratedInstance(SceneNode* inSceneNode, Entity* inEntity)
				: sceneNode(inSceneNode), entity(inEntity)
			{
			}
		};

		void SetMesh(MeshPtr mesh) { m_Mesh = mesh; }

		const MeshPtr& GetMesh() const { return m_Mesh; }

		uint32 AddInstance(const PCGTransform& transform, bool bWorldSpace = false);

		std::vector<uint32> AddInstances(const std::vector<PCGTransform>& transforms, bool bWorldSpace = false);

		void ClearInstances();

		void SpawnStaticMeshInstances();

	private:
		void DestroyGeneratedInstances();

		Entity* CreateStaticMeshEntityInternal(const Name& entityName);

		SceneManager* m_SceneManager;

		MeshPtr m_Mesh;

		std::vector<PCGTransform> m_InstanceTransforms;

		std::vector<GeneratedInstance> m_GeneratedInstances;
	};

	class _EchoComponentExport InstancedStaticMeshComponentInfo : public ActorComponentInfo
	{
	public:
		InstancedStaticMeshComponentInfo(const Name& type, ActorCOMMoveableType movable);

		_DECLCOMINFO_VIRFUN;
	};

	DEFINECOMFACTORY(InstancedStaticMeshComponent, ALL_MOVEABLE)
}

