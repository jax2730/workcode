#ifndef EchoVoxelTerrainComponent_h_
#define EchoVoxelTerrainComponent_h_

#include "EchoComponentPrerequisites.h"
#include "EchoProtocolComponent.h"
#include "Actor/EchoActor.h"
#include "EchoComDefineMacro.h"

#include "VoxelTerrain/EchoVoxelTerrain.h"
#include "EchoGeneratedTerrainPhysics.h"
#include "VoxelTerrain/EchoVoxelMisc.h"

namespace Echo
{
	class VoxelTerrain;
	class _EchoComponentExport VoxelTerrainComponent :public ActorComponent, public GeneratedTerrainPhysics_Voxel
	{
	public:
		DECLAREACTORCOMTYPE_H(VoxelTerrainComponent)
	public:	
		virtual void init(ActorComponentInfo* info) override;
		virtual void unInit() override;
		virtual void replay(ActorComponentInfo* info) override;
		virtual void copyTo(const ActorComponentPtr& actCom) override;
		virtual void connectCom() override;
		virtual void disconnectCom() override;
		virtual void NotifiChildImp(int changeMask) override;
		virtual void visibleStateChanged(bool activeState) override;
	public:
		const String& GetTerrainPath() { return m_sTerrainPath; }
		void SetTerrainPath(const String& name);

		void SetTerrain(SphericalTerrain* terrain) { m_Terrain = terrain; }

		void CreateTerrain();
		void DestroyTerrain();
		void ChangeTerrain();
		void OnCreateFinish();

		DMatrix4 GeVoxelWorldTransform() const;

#if ECHO_EDITOR
		bool m_FractalSynchronized{ false };
		void SpawnUnderGroundRooms(int targetCount, const Vector3& roomSize);
		void CollectCaveEntrancesAsync(VoxelFoliage::CollectEntranceListener* listener) const;
#endif
	private:
		virtual ActorComponent* getThisCompPtr() override;
		virtual bool IsCanPhysics() override;
		virtual void AddMeshColliderImpl(MeshColliderComponent* pCollider, const EchoIndex3& index) override;
		virtual void getAllBlockIndex(std::vector<EchoIndex3>&) override;
		virtual void getBlockIndex(const AxisAlignedBox&, std::vector<EchoIndex3>&) override;
		virtual AxisAlignedBox getBlockBound(const EchoIndex3&) override;
		virtual bool getIsNearby(const Vector3& worldPos) override;
		static void GenerateVBIBFunction(const EchoIndex3& index, VoxelTerrainComponent* _thisPtr, std::vector<Vector3>& vbVec, std::vector<uint16>& ibVec);
	private:
		String m_sTerrainPath;
		SphericalTerrain* m_Terrain = nullptr;
		VoxelTerrain* m_pVoxelTerrain = nullptr;
	};

	class _EchoComponentExport VoxelTerrainComponentInfo :public ActorComponentInfo
	{
	public:
		VoxelTerrainComponentInfo() {}
		VoxelTerrainComponentInfo(const Name& type, ActorCOMMoveableType moveable)
			:ActorComponentInfo(type, moveable) {
		};
		virtual ~VoxelTerrainComponentInfo() {}

		_DECLCOMINFO_VIRFUN
	public:
		_DECLCOMINFO_PARAM_BEGIN(VoxelTerrainComponent)
			_DECLCOMINFO_PARAM_SIM(bool, IsUsePhysics, true, SetIsUsePhysics)
			_DECLCOMINFO_PARAM(String, TerrainPath, String(""), SetTerrainPath)
			_DECLCOMINFO_PARAM_SIM(EchoMeshColliderFlags, MeshFlags, EchoMeshColliderFlag::eNONE, SetMeshFlags)
			_DECLCOMINFO_PARAM_SIM(int, MaterialId, -1, SetMaterialId)
			_DECLCOMINFO_PARAM_SIM(PhysXGroup, FilterGroupType, ePhysXGroup_Mesh, SetFilterGroupType)
			_DECLCOMINFO_PARAM_SIM(PhysXGroup, CanInteractFilterGroupType, ePhysXGroup_All, SetCanInteractFilterGroupType)
			_DECLCOMINFO_PARAM_END()
	};

	DEFINECOMFACTORY(VoxelTerrainComponent, ALL_MOVEABLE)
}
#endif // EchoVoxelTerrainComponent_h_
