#include "EchoComponentStableHeaders.h"
#include "EchoVoxelTerrainComponent.h"
#include "EchoPhysicsManager.h"
#include "EchoPhysXSystem.h"
#include "EchoPhysXScene.h"
#include "EchoMeshColliderComponent.h"
#include "Actor/EchoNodeComponent.h"
#include "VoxelTerrain/EchoVoxelWorld.h"

namespace Echo
{
	DECLAREACTORCOMTYPE_CPP(VoxelTerrainComponent)
		EDITOR_REFLECT_BEGIN(VoxelTerrainComponentInfo)
		EDITOR_REFLECT_MEMBER_UPDATE(m_IsUsePhysics, "IsUsePhysics", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_TerrainPath, "TerrainPath", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_MeshFlags, "MeshFlags", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_MaterialId, "MaterialId", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_FilterGroupType, "FilterGroupType", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_CanInteractFilterGroupType, "CanInteractFilterGroupType", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_END();
	_DEFINEINFO_VIRFUN_BASE(VoxelTerrainComponentInfo)

	VoxelTerrainComponent::VoxelTerrainComponent(ActorPtr pActor, ActorCOMMoveableType movableType)
		:ActorComponent(pActor, movableType)
	{

	}

	VoxelTerrainComponent::~VoxelTerrainComponent()
	{
	}

	void VoxelTerrainComponent::init(ActorComponentInfo* info)
	{
		if (info) {
			VoxelTerrainComponentInfo* pInfo = static_cast<VoxelTerrainComponentInfo*>(info);
			m_sTerrainPath = pInfo->GetTerrainPath();
			m_bIsUsePhysics = pInfo->GetIsUsePhysics();
			m_MeshFlags = pInfo->GetMeshFlags();
			m_MaterialId = pInfo->GetMaterialId();
			m_FilterGroupType = pInfo->GetFilterGroupType();
			m_CanInteractFilterGroupType = pInfo->GetCanInteractFilterGroupType();
		}
		SetNotifiedUpdate(true);
		RegisterPhysicsTerrain();
	}

	void VoxelTerrainComponent::unInit()
	{
		SetNotifiedUpdate(false);
		UnregisterPhysicsTerrain();
	}

	void VoxelTerrainComponent::replay(ActorComponentInfo* info)
	{
		if (info) {
			VoxelTerrainComponentInfo* pInfo = static_cast<VoxelTerrainComponentInfo*>(info);
			SetTerrainPath(pInfo->GetTerrainPath());
			SetIsUsePhysics(pInfo->GetIsUsePhysics());
			SetMeshFlags(pInfo->GetMeshFlags());
			SetMaterialId(pInfo->GetMaterialId());
			SetFilterGroupType(pInfo->GetFilterGroupType());
			SetCanInteractFilterGroupType(pInfo->GetCanInteractFilterGroupType());
		}
	}

	void VoxelTerrainComponent::copyTo(const ActorComponentPtr& actCom)
	{
		if (actCom == nullptr) return;
		VoxelTerrainComponent* pComp = static_cast<VoxelTerrainComponent*>(actCom.getPointer());
		pComp->SetTerrainPath(GetTerrainPath());
		pComp->SetIsUsePhysics(GetIsUsePhysics());
		pComp->SetMeshFlags(GetMeshFlags());
		pComp->SetMaterialId(GetMaterialId());
		pComp->SetFilterGroupType(GetFilterGroupType());
		pComp->SetCanInteractFilterGroupType(GetCanInteractFilterGroupType());

	}

	void VoxelTerrainComponent::connectCom()
	{
		// CreateTerrain();
	}

	void VoxelTerrainComponent::disconnectCom()
	{
		DestroyTerrain();
	}

	void VoxelTerrainComponent::NotifiChildImp(int changeMask)
	{
		if (m_pVoxelTerrain) {
			const DVector3 pos = getWorldPosition();
			const Quaternion rot = getWorldRotation();
			const Vector3 scl = getWorldScale();
			DMatrix4 world;
			world.makeTransform(pos, scl, rot);
			m_pVoxelTerrain->GetVoxelWorld()->SetWorldTransform(world);
		}
	}

	void VoxelTerrainComponent::visibleStateChanged(bool activeState)
	{
		if (m_pVoxelTerrain)
		{
			m_pVoxelTerrain->GetVoxelWorld()->SetVisible(activeState);
		}
		ChangePhysics();
	}

	void VoxelTerrainComponent::SetTerrainPath(const String& name) {
		if (m_sTerrainPath == name) {
			return;
		}
		m_sTerrainPath = name;
		ChangeTerrain();
	}

	void VoxelTerrainComponent::OnCreateFinish()
	{
		ChangePhysics();
	}

	DMatrix4 VoxelTerrainComponent::GeVoxelWorldTransform() const
	{
		if (m_pVoxelTerrain != nullptr)
		{
			return m_pVoxelTerrain->GetVoxelWorld()->GetWorldTransform();
		}
		return DMatrix4::IDENTITY;
	}
#if ECHO_EDITOR
	void VoxelTerrainComponent::SpawnUnderGroundRooms(int targetCount, const Vector3& roomSize)
	{
		if (!m_pVoxelTerrain) return;
		m_pVoxelTerrain->GetVoxelWorld()->SpawnRooms(targetCount, roomSize);
	}

	void VoxelTerrainComponent::CollectCaveEntrancesAsync(VoxelFoliage::CollectEntranceListener* listener) const
	{
		if (!listener) return;
		if (!m_pVoxelTerrain) return;
		m_pVoxelTerrain->GetVoxelWorld()->CollectCaveEntrancesAsync(listener);
	}
#endif

	void VoxelTerrainComponent::CreateTerrain()
	{
		if (m_pVoxelTerrain != nullptr)
			return;

		Echo::SceneManager* pSceneManager = Root::instance()->getMainSceneManager();
		if (pSceneManager == nullptr)
			return;

		m_pVoxelTerrain = new VoxelTerrain(pSceneManager);
		if (m_pVoxelTerrain)
		{
#if ECHO_EDITOR
			m_pVoxelTerrain->SetFractalSynchronized(m_FractalSynchronized);
#endif
			m_pVoxelTerrain->random(m_Terrain);

			const DVector3 pos = getWorldPosition();
			const Quaternion rot = getWorldRotation();
			const Vector3 scl = getWorldScale();
			DMatrix4 world;
			world.makeTransform(pos, scl, rot);
			m_pVoxelTerrain->GetVoxelWorld()->SetWorldTransform(world);

			OnCreateFinish();
		}
	}

	void VoxelTerrainComponent::DestroyTerrain()
	{
		if (m_pVoxelTerrain == nullptr)
			return;
		DestroyPhysics();
		SAFE_DELETE(m_pVoxelTerrain);
	}

	void VoxelTerrainComponent::ChangeTerrain()
	{
		DestroyTerrain();
		CreateTerrain();
	}
	ActorComponent* VoxelTerrainComponent::getThisCompPtr() {
		return this;
	}
	bool VoxelTerrainComponent::getIsNearby(const Vector3& pos) {
		if (!m_pVoxelTerrain)
		{
			return false;
		}
		VoxelWorld* world = m_pVoxelTerrain->GetVoxelWorld();
		VoxelIntBox worldBounds = world->GetWorldBounds();
		IntVector3 iPos = world->GlobalToLocal(pos);
		uint64 sqrDist = worldBounds.ComputeSquaredDistanceFromBoxToPoint(iPos);
		return sqrDist < Math::Sqr(600.f * world->m_VoxelSizeInv);
	}
	bool VoxelTerrainComponent::IsCanPhysics() {
		return m_pVoxelTerrain != nullptr;
	}
	void VoxelTerrainComponent::AddMeshColliderImpl(MeshColliderComponent* pCollider, const EchoIndex3& index) {
		pCollider->SetMeshGenFun(std::bind<void>(&VoxelTerrainComponent::GenerateVBIBFunction, index, this, std::placeholders::_1, std::placeholders::_2));
		pCollider->setPosition(m_pVoxelTerrain->GetVoxelWorld()->GetFinestChunkPosition(index));
	}
	void VoxelTerrainComponent::getAllBlockIndex(std::vector<EchoIndex3>& leavesIndexs) {
		leavesIndexs = m_pVoxelTerrain->GetVoxelWorld()->GetFinestChunkIndex(AxisAlignedBox::BOX_INFINITE);
	}
	void VoxelTerrainComponent::getBlockIndex(const AxisAlignedBox& localBound, std::vector<EchoIndex3>& indexs) {
		indexs = m_pVoxelTerrain->GetVoxelWorld()->GetFinestChunkIndex(localBound);
	}
	AxisAlignedBox VoxelTerrainComponent::getBlockBound(const EchoIndex3& index) {
		return m_pVoxelTerrain->GetVoxelWorld()->GetFinestChunkBounds(index);
	}
	void VoxelTerrainComponent::GenerateVBIBFunction(const EchoIndex3& index, VoxelTerrainComponent* _thisPtr, std::vector<Vector3>& vbVec, std::vector<uint16>& ibVec)
	{
		if (_thisPtr != nullptr)
		{
			if (_thisPtr->m_pVoxelTerrain == nullptr) return;
			_thisPtr->m_pVoxelTerrain->GetVoxelWorld()->CreateFinestChunkGeometry(index, ibVec, vbVec);
		}
	}
}
