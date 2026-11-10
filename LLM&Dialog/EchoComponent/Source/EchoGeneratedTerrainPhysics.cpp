#include "EchoComponentStableHeaders.h"
#include "EchoGeneratedTerrainPhysics.h"
#include "EchoPhysicsManager.h"
#include "EchoPhysXSystem.h"
#include "EchoPhysXScene.h"
#include "Actor/EchoNodeComponent.h"
#include "EchoPhysXManager.h"

namespace Echo
{
	void GeneratedTerrainPhysics::RegisterPhysicsTerrain() {
		ActorComponent* _this = getThisCompPtr();
		if (_this == nullptr) return;
		EchoPhysicsSystem::instance()->GetPhysicsManager(_this)->AddSphericalTerrain(this);
	}

	void GeneratedTerrainPhysics::UnregisterPhysicsTerrain() {
		ActorComponent* _this = getThisCompPtr();
		if (_this == nullptr) return;
		EchoPhysicsSystem::instance()->GetPhysicsManager(_this)->RemoveSphericalTerrain(this);
	}

	void GeneratedTerrainPhysics::SetIsUsePhysics(bool value) {
		if (m_bIsUsePhysics == value) {
			return;
		}
		m_bIsUsePhysics = value;
		ChangePhysics();
	}

	void GeneratedTerrainPhysics::SetMaterialId(int value) {
		m_MaterialId = value;
		if (m_ColliderRootPtr == nullptr) return;
		std::vector<MeshColliderComponent*> colliders;
		GetMeshColliders(colliders);
		for (MeshColliderComponent* pCollider : colliders) {
			if (pCollider != nullptr) {
				pCollider->SetMaterialId(m_MaterialId);
			}
		}
	}

	void GeneratedTerrainPhysics::SetFilterGroupType(PhysXGroup value) {
		m_FilterGroupType = value;
		if (m_ColliderRootPtr == nullptr) return;
		std::vector<MeshColliderComponent*> colliders;
		GetMeshColliders(colliders);
		for (MeshColliderComponent* pCollider : colliders) {
			if (pCollider != nullptr) {
				pCollider->SetFilterGroupType(m_FilterGroupType);
			}
		}
	}

	void GeneratedTerrainPhysics::SetCanInteractFilterGroupType(PhysXGroup value) {
		m_CanInteractFilterGroupType = value;
		if (m_ColliderRootPtr == nullptr) return;
		std::vector<MeshColliderComponent*> colliders;
		GetMeshColliders(colliders);
		for (MeshColliderComponent* pCollider : colliders) {
			if (pCollider != nullptr) {
				pCollider->SetCanInteractFilterGroupType(m_CanInteractFilterGroupType);
			}
		}
	}

	void GeneratedTerrainPhysics::SetMeshFlags(EchoMeshColliderFlags value) {
		m_MeshFlags = value;
		if (m_ColliderRootPtr == nullptr) return;
		std::vector<MeshColliderComponent*> colliders;
		GetMeshColliders(colliders);
		for (MeshColliderComponent* pCollider : colliders) {
			if (pCollider != nullptr) {
				pCollider->SetMeshFlags(m_MeshFlags);
			}
		}
	}
	
}

namespace Echo
{

	void GeneratedTerrainPhysics_Spherical::GetMeshColliders(std::vector<MeshColliderComponent*>& colliders)
	{
		for (auto& iter : m_SyncColliderMap) {
			if (iter.second != nullptr) {
				MeshColliderComponent* pCollider = (MeshColliderComponent*)iter.second.getPointer();
				colliders.push_back(pCollider);
			}
		}

		for (auto& iter : m_AsyncColliderMap) {
			if (iter.second != nullptr) {
				MeshColliderComponent* pCollider = (MeshColliderComponent*)iter.second.getPointer();
				colliders.push_back(pCollider);
			}
		}

	}

	
	void GeneratedTerrainPhysics_Spherical::CreatePhysics() {
		ActorComponent* _this = getThisCompPtr();
		if (_this == nullptr) return;
		if (!IsCanPhysics() || !GetIsUsePhysics() || !_this->getRealVisibleState()) return;

		DestroyPhysics();

		auto root = _this->getActor()->AddComponent<NodeComponent>(_this->getComName() + "_Physics", _this->getParentNodeCom());
		m_ColliderRootPtr = root.getStaticCastWeakPtr<ActorComponent>();
		m_LastPosition = Vector3::MIN;

		UpdateLoadData();
	}

	
	void GeneratedTerrainPhysics_Spherical::DestroyPhysics() {
		ActorComponent* _this = getThisCompPtr();
		if (_this == nullptr) return;
		if (m_ColliderRootPtr == nullptr) return;

		m_bIsNearby = false;
		ClearPhysics();
		_this->getActor()->delComponent(m_ColliderRootPtr);
		m_ColliderRootPtr = nullptr;
	}

	
	void GeneratedTerrainPhysics_Spherical::ClearPhysics() {
		ActorComponent* _this = getThisCompPtr();
		if (_this == nullptr) return;
		for (auto& iter : m_SyncColliderMap) {
			_this->getActor()->delComponent(iter.second);
		}
		m_SyncColliderMap.clear();
		for (auto& iter : m_AsyncColliderMap) {
			_this->getActor()->delComponent(iter.second);
		}
		m_AsyncColliderMap.clear();
		m_LastPosition = Vector3::MIN;
		m_LastAsyncPosition = Vector3::MIN;
	}

	
	void GeneratedTerrainPhysics_Spherical::UpdateLoadData() {
		if (m_ColliderRootPtr == nullptr) return;
		m_vSyncLoadSize = 300.0f;
		m_vAsyncLoadSize = 512.0f;
		m_vKeepLoadSize = 640.0f;
		m_fPredictTime = 1.0f;
		m_fUpdateSyncDistanceSqu = 10000.0f;
		m_fUpdateAsyncDistanceSqu = 10000.0f;
		m_fMaxSpeed = 128.0f;
	}

	
	ActorComponentPtr GeneratedTerrainPhysics_Spherical::AddMeshCollider(int index, bool bAsync, bool bUse) {
		ActorComponent* _this = getThisCompPtr();
		if (_this == nullptr) return nullptr;
		ActorComponentPtr compPtr = _this->getActor()->addComponent(MeshColliderComponent::mType, std::to_string(index), m_ColliderRootPtr);
		MeshColliderComponent* pCollider = (MeshColliderComponent*)compPtr.getPointer();
		pCollider->SetIsUsePhysXPage(false);
		pCollider->SetMaterialId(m_MaterialId);
		pCollider->SetFilterGroupType(m_FilterGroupType);
		pCollider->SetCanInteractFilterGroupType(m_CanInteractFilterGroupType);
		pCollider->SetMeshFlags(m_MeshFlags);
		AddMeshColliderImpl(pCollider, index);
		pCollider->SetIsUseAsyncRes(bAsync);
		if (bUse) {
			pCollider->SetIsUsePhysics(true);
		}
		return compPtr;
	}

	
	void GeneratedTerrainPhysics_Spherical::getBlockLoadBound(AxisAlignedBox& sync, AxisAlignedBox& async, AxisAlignedBox& keep) {
		AxisAlignedBox aAsyncLoadBox(m_LastAsyncPosition - m_vAsyncLoadSize, m_LastAsyncPosition + m_vAsyncLoadSize);
		AxisAlignedBox aSyncLoadBox(m_LastPosition - m_vSyncLoadSize, m_LastPosition + m_vSyncLoadSize);
		AxisAlignedBox aKeepLoadBox(m_LastPosition - m_vKeepLoadSize, m_LastPosition + m_vKeepLoadSize);
		{
			AxisAlignedBox aSyncAsyncLoadBox(m_LastPosition - m_vAsyncLoadSize, m_LastPosition + m_vAsyncLoadSize);
			aAsyncLoadBox.merge(aSyncAsyncLoadBox);
		}
		async = aAsyncLoadBox;
		sync = aSyncLoadBox;
		keep = aKeepLoadBox;
	}

	
	void GeneratedTerrainPhysics_Spherical::UpdatePhysics() {
		if (m_ColliderRootPtr == nullptr || !m_SyncColliderMap.empty()) return;
		m_bIsNearby = true;
		std::vector<int> leavesIndexs;
		getAllBlockIndex(leavesIndexs);

		for (int index : leavesIndexs) {
			ActorComponentPtr compPtr = AddMeshCollider(index, false, true);
			m_SyncColliderMap.insert(std::make_pair(index, compPtr));
		}
	}

	
	void GeneratedTerrainPhysics_Spherical::UpdatePhysics(const Vector3& worldPos, const Vector3& predictWorldDirection, float speed, bool bForceAsync) {
		ActorComponent* _this = getThisCompPtr();
		if (_this == nullptr) return;
		if (m_ColliderRootPtr == nullptr) return;
		bool isNearby = getIsNearby(worldPos);
		if (m_bIsNearby != isNearby) {
			if (m_bIsNearby) {
				ClearPhysics();
			}
			m_bIsNearby = isNearby;
		}
		if (!m_bIsNearby) {
			return;
		}
		Vector3 localPos = _this->convertWorldToLocalPosition(worldPos);
		Vector3 localPredictDirection = _this->getWorldRotation().Inverse() * predictWorldDirection;
		float curSpeed = Math::Min(speed, m_fMaxSpeed);
		Vector3 aLocalPos = localPos + localPredictDirection * curSpeed * m_fPredictTime;

		if ((localPos - m_LastPosition).LengthSq() < m_fUpdateSyncDistanceSqu && (aLocalPos - m_LastAsyncPosition).LengthSq() < m_fUpdateAsyncDistanceSqu && (m_LastForceAsync == bForceAsync)) {
			return;
		}
		m_LastPosition = localPos;
		m_LastAsyncPosition = aLocalPos;
		m_LastForceAsync = bForceAsync;

		AxisAlignedBox aAsyncLoadBox;
		AxisAlignedBox aSyncLoadBox;
		AxisAlignedBox aKeepLoadBox;
		getBlockLoadBound(aSyncLoadBox, aAsyncLoadBox, aKeepLoadBox);

		std::vector<int> sAsyncLoadIndexs;
		std::vector<int> sSyncLoadIndexs;

		{
			std::vector<int> indexs;
			getBlockIndex(aAsyncLoadBox, indexs);

			sAsyncLoadIndexs.reserve(indexs.size());
			sSyncLoadIndexs.reserve(indexs.size());

			if (bForceAsync) {
				sAsyncLoadIndexs = indexs;
			}
			else {
				for (const auto& index : indexs) {
					if (getBlockBound(index).intersects(aSyncLoadBox)) {
						sSyncLoadIndexs.push_back(index);
					}
					else {
						sAsyncLoadIndexs.push_back(index);
					}
				}
			}
		}

		std::map<int, ActorComponentPtr> newAsyncColliderMap;
		std::map<int, ActorComponentPtr> newSyncColliderMap;

		std::vector<int> willAsyncLoadIndexs;

		for (int index : sAsyncLoadIndexs) {
			auto asyncIter = m_AsyncColliderMap.find(index);
			if (asyncIter != m_AsyncColliderMap.end()) {
				newAsyncColliderMap.insert(std::make_pair(asyncIter->first, asyncIter->second));
				m_AsyncColliderMap.erase(asyncIter);
				continue;
			}
			auto syncIter = m_SyncColliderMap.find(index);
			if (syncIter != m_SyncColliderMap.end()) {
				MeshColliderComponent* pCollider = (MeshColliderComponent*)syncIter->second.get();
				if (pCollider) {
					pCollider->SetIsUseAsyncRes(true);
				}
				newAsyncColliderMap.insert(std::make_pair(syncIter->first, syncIter->second));
				m_SyncColliderMap.erase(syncIter);
				continue;
			}
			ActorComponentPtr compPtr = AddMeshCollider(index, true, false);
			newAsyncColliderMap.insert(std::make_pair(index, compPtr));
			willAsyncLoadIndexs.push_back(index);
		}

		for (int index : sSyncLoadIndexs) {
			auto syncIter = m_SyncColliderMap.find(index);
			if (syncIter != m_SyncColliderMap.end()) {
				newSyncColliderMap.insert(std::make_pair(syncIter->first, syncIter->second));
				m_SyncColliderMap.erase(syncIter);
				continue;
			}
			auto asyncIter = m_AsyncColliderMap.find(index);
			if (asyncIter != m_AsyncColliderMap.end()) {
				MeshColliderComponent* pCollider = (MeshColliderComponent*)asyncIter->second.get();
				if (pCollider) {
					pCollider->SetIsUseAsyncRes(false);
					pCollider->SetIsUsePhysics(true);
				}
				newSyncColliderMap.insert(std::make_pair(asyncIter->first, asyncIter->second));
				m_AsyncColliderMap.erase(asyncIter);
				continue;
			}
			ActorComponentPtr compPtr = AddMeshCollider(index, false, true);
			newSyncColliderMap.insert(std::make_pair(index, compPtr));
		}

		for (auto& iter : m_SyncColliderMap) {
			if (aKeepLoadBox.intersects(getBlockBound(iter.first))) {
				MeshColliderComponent* pCollider = (MeshColliderComponent*)iter.second.getPointer();
				if (pCollider) {
					pCollider->SetIsUseAsyncRes(true);
				}
				newAsyncColliderMap.insert(std::make_pair(iter.first, iter.second));
			}
			else {
				_this->getActor()->delComponent(iter.second);
			}
		}
		m_SyncColliderMap.swap(newSyncColliderMap);

		for (auto& iter : m_AsyncColliderMap) {
			if (aKeepLoadBox.intersects(getBlockBound(iter.first))) {
				newAsyncColliderMap.insert(std::make_pair(iter.first, iter.second));
			}
			else {
				_this->getActor()->delComponent(iter.second);
			}
		}
		m_AsyncColliderMap.swap(newAsyncColliderMap);

		std::vector<std::pair<float, int>> sortAsyncLoadIndexs;
		for (int index : willAsyncLoadIndexs) {
			float disSqu = getBlockBound(index).squaredDistance(m_LastAsyncPosition);
			sortAsyncLoadIndexs.push_back(std::make_pair(disSqu, index));
		}
		std::sort(sortAsyncLoadIndexs.begin(), sortAsyncLoadIndexs.end());
		for (auto& iter : sortAsyncLoadIndexs) {
			((MeshColliderComponent*)m_AsyncColliderMap.find(iter.second)->second.getPointer())->SetIsUsePhysics(true);
		}
	}

}


namespace Echo
{

	void GeneratedTerrainPhysics_Voxel::GetMeshColliders(std::vector<MeshColliderComponent*>& colliders)
	{
		for (auto& iter : m_SyncColliderMap) {
			if (iter.second != nullptr) {
				MeshColliderComponent* pCollider = (MeshColliderComponent*)iter.second.getPointer();
				colliders.push_back(pCollider);
			}
		}

		for (auto& iter : m_AsyncColliderMap) {
			if (iter.second != nullptr) {
				MeshColliderComponent* pCollider = (MeshColliderComponent*)iter.second.getPointer();
				colliders.push_back(pCollider);
			}
		}

	}


	void GeneratedTerrainPhysics_Voxel::CreatePhysics() {
		ActorComponent* _this = getThisCompPtr();
		if (_this == nullptr) return;
		if (!IsCanPhysics() || !GetIsUsePhysics() || !_this->getRealVisibleState()) return;

		DestroyPhysics();

		auto root = _this->getActor()->AddComponent<NodeComponent>(_this->getComName() + "_Physics", _this->getParentNodeCom());
		m_ColliderRootPtr = root.getStaticCastWeakPtr<ActorComponent>();
		m_LastPosition = Vector3::MIN;

		UpdateLoadData();
	}


	void GeneratedTerrainPhysics_Voxel::DestroyPhysics() {
		ActorComponent* _this = getThisCompPtr();
		if (_this == nullptr) return;
		if (m_ColliderRootPtr == nullptr) return;

		m_bIsNearby = false;
		ClearPhysics();
		_this->getActor()->delComponent(m_ColliderRootPtr);
		m_ColliderRootPtr = nullptr;
	}


	void GeneratedTerrainPhysics_Voxel::ClearPhysics() {
		ActorComponent* _this = getThisCompPtr();
		if (_this == nullptr) return;
		for (auto& iter : m_SyncColliderMap) {
			_this->getActor()->delComponent(iter.second);
		}
		m_SyncColliderMap.clear();
		for (auto& iter : m_AsyncColliderMap) {
			_this->getActor()->delComponent(iter.second);
		}
		m_AsyncColliderMap.clear();
		m_LastPosition = Vector3::MIN;
		m_LastAsyncPosition = Vector3::MIN;
	}


	void GeneratedTerrainPhysics_Voxel::UpdateLoadData() {
		if (m_ColliderRootPtr == nullptr) return;
		m_vSyncLoadSize = 300.0f;
		m_vAsyncLoadSize = 512.0f;
		m_vKeepLoadSize = 640.0f;
		m_fPredictTime = 1.0f;
		m_fUpdateSyncDistanceSqu = 10000.0f;
		m_fUpdateAsyncDistanceSqu = 10000.0f;
		m_fMaxSpeed = 128.0f;
	}


	ActorComponentPtr GeneratedTerrainPhysics_Voxel::AddMeshCollider(const EchoIndex3& index, bool bAsync, bool bUse) {
		ActorComponent* _this = getThisCompPtr();
		if (_this == nullptr) return nullptr;
		ActorComponentPtr compPtr = _this->getActor()->addComponent(MeshColliderComponent::mType, index.ToString(), m_ColliderRootPtr);
		MeshColliderComponent* pCollider = (MeshColliderComponent*)compPtr.getPointer();
		pCollider->SetIsUsePhysXPage(false);
		pCollider->SetMaterialId(m_MaterialId);
		pCollider->SetFilterGroupType(m_FilterGroupType);
		pCollider->SetCanInteractFilterGroupType(m_CanInteractFilterGroupType);
		pCollider->SetMeshFlags(m_MeshFlags);
		AddMeshColliderImpl(pCollider, index);
		pCollider->SetIsUseAsyncRes(bAsync);
		if (bUse) {
			pCollider->SetIsUsePhysics(true);
		}
		return compPtr;
	}


	void GeneratedTerrainPhysics_Voxel::getBlockLoadBound(AxisAlignedBox& sync, AxisAlignedBox& async, AxisAlignedBox& keep) {
		AxisAlignedBox aAsyncLoadBox(m_LastAsyncPosition - m_vAsyncLoadSize, m_LastAsyncPosition + m_vAsyncLoadSize);
		AxisAlignedBox aSyncLoadBox(m_LastPosition - m_vSyncLoadSize, m_LastPosition + m_vSyncLoadSize);
		AxisAlignedBox aKeepLoadBox(m_LastPosition - m_vKeepLoadSize, m_LastPosition + m_vKeepLoadSize);
		{
			AxisAlignedBox aSyncAsyncLoadBox(m_LastPosition - m_vAsyncLoadSize, m_LastPosition + m_vAsyncLoadSize);
			aAsyncLoadBox.merge(aSyncAsyncLoadBox);
		}
		async = aAsyncLoadBox;
		sync = aSyncLoadBox;
		keep = aKeepLoadBox;
	}


	void GeneratedTerrainPhysics_Voxel::UpdatePhysics() {
		if (m_ColliderRootPtr == nullptr || !m_SyncColliderMap.empty()) return;
		m_bIsNearby = true;
		std::vector<EchoIndex3> leavesIndexs;
		getAllBlockIndex(leavesIndexs);

		for (EchoIndex3 index : leavesIndexs) {
			ActorComponentPtr compPtr = AddMeshCollider(index, false, true);
			m_SyncColliderMap.insert(std::make_pair(index, compPtr));
		}
	}


	void GeneratedTerrainPhysics_Voxel::UpdatePhysics(const Vector3& worldPos, const Vector3& predictWorldDirection, float speed, bool bForceAsync) {
		ActorComponent* _this = getThisCompPtr();
		if (_this == nullptr) return;
		if (m_ColliderRootPtr == nullptr) return;
		bool isNearby = getIsNearby(worldPos);
		if (m_bIsNearby != isNearby) {
			if (m_bIsNearby) {
				ClearPhysics();
			}
			m_bIsNearby = isNearby;
		}
		if (!m_bIsNearby) {
			return;
		}
		Vector3 localPos = _this->convertWorldToLocalPosition(worldPos);
		Vector3 localPredictDirection = _this->getWorldRotation().Inverse() * predictWorldDirection;
		float curSpeed = Math::Min(speed, m_fMaxSpeed);
		Vector3 aLocalPos = localPos + localPredictDirection * curSpeed * m_fPredictTime;

		if ((localPos - m_LastPosition).LengthSq() < m_fUpdateSyncDistanceSqu && (aLocalPos - m_LastAsyncPosition).LengthSq() < m_fUpdateAsyncDistanceSqu && (m_LastForceAsync == bForceAsync)) {
			return;
		}
		m_LastPosition = localPos;
		m_LastAsyncPosition = aLocalPos;
		m_LastForceAsync = bForceAsync;

		AxisAlignedBox aAsyncLoadBox;
		AxisAlignedBox aSyncLoadBox;
		AxisAlignedBox aKeepLoadBox;
		getBlockLoadBound(aSyncLoadBox, aAsyncLoadBox, aKeepLoadBox);

		std::vector<EchoIndex3> sAsyncLoadIndexs;
		std::vector<EchoIndex3> sSyncLoadIndexs;

		{
			std::vector<EchoIndex3> indexs;
			getBlockIndex(aAsyncLoadBox, indexs);

			sAsyncLoadIndexs.reserve(indexs.size());
			sSyncLoadIndexs.reserve(indexs.size());

			if (bForceAsync) {
				sAsyncLoadIndexs = indexs;
			}
			else {
				for (const auto& index : indexs) {
					if (getBlockBound(index).intersects(aSyncLoadBox)) {
						sSyncLoadIndexs.push_back(index);
					}
					else {
						sAsyncLoadIndexs.push_back(index);
					}
				}
			}
		}

		std::map<EchoIndex3, ActorComponentPtr> newAsyncColliderMap;
		std::map<EchoIndex3, ActorComponentPtr> newSyncColliderMap;

		std::vector<EchoIndex3> willAsyncLoadIndexs;

		for (EchoIndex3 index : sAsyncLoadIndexs) {
			auto asyncIter = m_AsyncColliderMap.find(index);
			if (asyncIter != m_AsyncColliderMap.end()) {
				newAsyncColliderMap.insert(std::make_pair(asyncIter->first, asyncIter->second));
				m_AsyncColliderMap.erase(asyncIter);
				continue;
			}
			auto syncIter = m_SyncColliderMap.find(index);
			if (syncIter != m_SyncColliderMap.end()) {
				MeshColliderComponent* pCollider = (MeshColliderComponent*)syncIter->second.get();
				if (pCollider) {
					pCollider->SetIsUseAsyncRes(true);
				}
				newAsyncColliderMap.insert(std::make_pair(syncIter->first, syncIter->second));
				m_SyncColliderMap.erase(syncIter);
				continue;
			}
			ActorComponentPtr compPtr = AddMeshCollider(index, true, false);
			newAsyncColliderMap.insert(std::make_pair(index, compPtr));
			willAsyncLoadIndexs.push_back(index);
		}

		for (EchoIndex3 index : sSyncLoadIndexs) {
			auto syncIter = m_SyncColliderMap.find(index);
			if (syncIter != m_SyncColliderMap.end()) {
				newSyncColliderMap.insert(std::make_pair(syncIter->first, syncIter->second));
				m_SyncColliderMap.erase(syncIter);
				continue;
			}
			auto asyncIter = m_AsyncColliderMap.find(index);
			if (asyncIter != m_AsyncColliderMap.end()) {
				MeshColliderComponent* pCollider = (MeshColliderComponent*)asyncIter->second.get();
				if (pCollider) {
					pCollider->SetIsUseAsyncRes(false);
					pCollider->SetIsUsePhysics(true);
				}
				newSyncColliderMap.insert(std::make_pair(asyncIter->first, asyncIter->second));
				m_AsyncColliderMap.erase(asyncIter);
				continue;
			}
			ActorComponentPtr compPtr = AddMeshCollider(index, false, true);
			newSyncColliderMap.insert(std::make_pair(index, compPtr));
		}

		for (auto& iter : m_SyncColliderMap) {
			if (aKeepLoadBox.intersects(getBlockBound(iter.first))) {
				MeshColliderComponent* pCollider = (MeshColliderComponent*)iter.second.getPointer();
				if (pCollider) {
					pCollider->SetIsUseAsyncRes(true);
				}
				newAsyncColliderMap.insert(std::make_pair(iter.first, iter.second));
			}
			else {
				_this->getActor()->delComponent(iter.second);
			}
		}
		m_SyncColliderMap.swap(newSyncColliderMap);

		for (auto& iter : m_AsyncColliderMap) {
			if (aKeepLoadBox.intersects(getBlockBound(iter.first))) {
				newAsyncColliderMap.insert(std::make_pair(iter.first, iter.second));
			}
			else {
				_this->getActor()->delComponent(iter.second);
			}
		}
		m_AsyncColliderMap.swap(newAsyncColliderMap);

		std::vector<std::pair<float, EchoIndex3>> sortAsyncLoadIndexs;
		for (EchoIndex3 index : willAsyncLoadIndexs) {
			float disSqu = getBlockBound(index).squaredDistance(m_LastAsyncPosition);
			sortAsyncLoadIndexs.push_back(std::make_pair(disSqu, index));
		}
		std::sort(sortAsyncLoadIndexs.begin(), sortAsyncLoadIndexs.end());
		for (auto& iter : sortAsyncLoadIndexs) {
			((MeshColliderComponent*)m_AsyncColliderMap.find(iter.second)->second.getPointer())->SetIsUsePhysics(true);
		}
	}

}
