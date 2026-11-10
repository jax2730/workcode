//-----------------------------------------------------------------------------
// File:   EchoGeneratedTerrainPhysics.h
//
// Author: qiao_yuzhi
//
// Date:   2025-9-15
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef EchoGeneratedTerrainPhysics_h__
#define EchoGeneratedTerrainPhysics_h__

#include "EchoComponentPrerequisites.h"
#include "EchoProtocolComponent.h"
#include "Actor/EchoActor.h"

#include "EchoComDefineMacro.h"
#include "EchoMeshColliderComponent.h"
#include "EchoIndex.h"

namespace Echo
{
	class _EchoComponentExport GeneratedTerrainPhysics {
	public:
		GeneratedTerrainPhysics() {}
		virtual ~GeneratedTerrainPhysics() {}

		bool GetIsUsePhysics() { return m_bIsUsePhysics; }
		virtual void SetIsUsePhysics(bool value);
		int GetMaterialId() { return m_MaterialId; }
		virtual void SetMaterialId(int value);
		PhysXGroup GetFilterGroupType() { return m_FilterGroupType; }
		virtual void SetFilterGroupType(PhysXGroup value);
		PhysXGroup GetCanInteractFilterGroupType() { return m_CanInteractFilterGroupType; }
		virtual void SetCanInteractFilterGroupType(PhysXGroup value);
		EchoMeshColliderFlags GetMeshFlags() { return m_MeshFlags; }
		virtual void SetMeshFlags(EchoMeshColliderFlags value);

		virtual void CreatePhysics() = 0;
		virtual void DestroyPhysics() = 0;
		inline void ChangePhysics() { DestroyPhysics(); CreatePhysics(); }
		virtual void ClearPhysics() = 0;
		virtual void UpdatePhysics(const Vector3& position, const Vector3& predictDirection, float speed, bool bForceAsync) = 0;
		virtual void UpdatePhysics() = 0;
		virtual bool IsCanPhysics() = 0;
		virtual bool getIsNearby(const Vector3& localPos) { return true; }
		virtual ActorComponent* getThisCompPtr() = 0;

		inline bool IsExistPhysics() { return m_ColliderRootPtr != nullptr; }
	protected:
		void RegisterPhysicsTerrain();
		void UnregisterPhysicsTerrain();
		virtual void GetMeshColliders(std::vector<MeshColliderComponent*>& colliders) = 0;

		ActorComponentPtr m_ColliderRootPtr = nullptr;
		bool m_bIsUsePhysics = true;
		EchoMeshColliderFlags m_MeshFlags = EchoMeshColliderFlag::eNONE;
		int m_MaterialId = -1;
		PhysXGroup m_FilterGroupType = ePhysXGroup_Mesh;
		PhysXGroup m_CanInteractFilterGroupType = ePhysXGroup_All;
	};

	class _EchoComponentExport GeneratedTerrainPhysics_Spherical : public GeneratedTerrainPhysics {
	public:
		GeneratedTerrainPhysics_Spherical() {}
		virtual ~GeneratedTerrainPhysics_Spherical() {}

		virtual void CreatePhysics() override;
		virtual void DestroyPhysics() override;
		virtual void ClearPhysics() override;
		virtual void UpdatePhysics(const Vector3& position, const Vector3& predictDirection, float speed, bool bForceAsync) override;
		virtual void UpdatePhysics() override;
	protected:
		virtual void GetMeshColliders(std::vector<MeshColliderComponent*>& colliders) override;
		virtual void UpdateLoadData();
		virtual void getBlockLoadBound(AxisAlignedBox& sync, AxisAlignedBox& async, AxisAlignedBox& keep);
		ActorComponentPtr AddMeshCollider(int index, bool bAsync, bool bUse);
	protected:
		virtual void AddMeshColliderImpl(MeshColliderComponent* pCollider, int index) = 0;
		virtual void getAllBlockIndex(std::vector<int>&) = 0;
		virtual void getBlockIndex(const AxisAlignedBox&, std::vector<int>&) = 0;
		virtual AxisAlignedBox getBlockBound(int) = 0;
	protected:

		bool m_bIsNearby = false;
		bool m_LastForceAsync = false;
		std::map<int, ActorComponentPtr> m_SyncColliderMap;
		std::map<int, ActorComponentPtr> m_AsyncColliderMap;
		float m_vAsyncLoadSize = 0.0f;
		float m_vSyncLoadSize = 0.0f;
		float m_vKeepLoadSize = 0.0f;
		float m_fUpdateSyncDistanceSqu = 0.0f;
		float m_fUpdateAsyncDistanceSqu = 0.0f;
		float m_fMaxSpeed = 0.0f;
		float m_fPredictTime = 0.0f;
		Vector3 m_LastPosition = Vector3::MIN;
		Vector3 m_LastAsyncPosition = Vector3::MIN;

	};

	class _EchoComponentExport GeneratedTerrainPhysics_Voxel : public GeneratedTerrainPhysics {
	public:
		GeneratedTerrainPhysics_Voxel() {}
		virtual ~GeneratedTerrainPhysics_Voxel() {}

		virtual void CreatePhysics() override;
		virtual void DestroyPhysics() override;
		virtual void ClearPhysics() override;
		virtual void UpdatePhysics(const Vector3& position, const Vector3& predictDirection, float speed, bool bForceAsync) override;
		virtual void UpdatePhysics() override;
	protected:
		virtual void GetMeshColliders(std::vector<MeshColliderComponent*>& colliders) override;
		virtual void UpdateLoadData();
		virtual void getBlockLoadBound(AxisAlignedBox& sync, AxisAlignedBox& async, AxisAlignedBox& keep);
		ActorComponentPtr AddMeshCollider(const EchoIndex3& index, bool bAsync, bool bUse);
	protected:
		virtual void AddMeshColliderImpl(MeshColliderComponent* pCollider, const EchoIndex3& index) = 0;
		virtual void getAllBlockIndex(std::vector<EchoIndex3>&) = 0;
		virtual void getBlockIndex(const AxisAlignedBox&, std::vector<EchoIndex3>&) = 0;
		virtual AxisAlignedBox getBlockBound(const EchoIndex3&) = 0;
	protected:

		bool m_bIsNearby = false;
		bool m_LastForceAsync = false;
		std::map<EchoIndex3, ActorComponentPtr> m_SyncColliderMap;
		std::map<EchoIndex3, ActorComponentPtr> m_AsyncColliderMap;
		float m_vAsyncLoadSize = 0.0f;
		float m_vSyncLoadSize = 0.0f;
		float m_vKeepLoadSize = 0.0f;
		float m_fUpdateSyncDistanceSqu = 0.0f;
		float m_fUpdateAsyncDistanceSqu = 0.0f;
		float m_fMaxSpeed = 0.0f;
		float m_fPredictTime = 0.0f;
		Vector3 m_LastPosition = Vector3::MIN;
		Vector3 m_LastAsyncPosition = Vector3::MIN;

	};

}

#endif // EchoGeneratedTerrainPhysics_h__
