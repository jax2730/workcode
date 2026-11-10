#include "EchoComponentStableHeaders.h"
#include "EchoTerrainColliderComponent.h"
#include "Effect/EchoEffectHolder.h"
#include "EchoActorProFuncImp.h"
#include "EchoPhysXScene.h"
#include "EchoPhysXAsset.h"
#include "EchoPhysXManager.h"

#include "EchocJsonHelper.h"
namespace Echo{

	DECLAREACTORCOMTYPE_CPP(TerrainColliderComponent)

#define _COMPDATA(_pData) static_cast<TerrainColliderComponentData*>(_pData)

	TerrainColliderComponentInfo::TerrainColliderComponentInfo(const Name& comType, ActorCOMMoveableType MoveType):ColliderComponentInfo(comType, MoveType) {}
	TerrainColliderComponentInfo::~TerrainColliderComponentInfo() {}

	EDITOR_REFLECT_BEGIN(TerrainColliderComponentInfo)
	EDITOR_REFLECT_MEMBER_UPDATE(m_BlockTerrainIndex, "BlockTerrainIndex", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_END();

	_DEFINEINFO_VIRFUN(TerrainColliderComponentInfo, ColliderComponentInfo)


	void TerrainColliderComponentInfo::CopyToData(ColliderComponentData* pData) {
		if (!pData) return;
		ColliderComponentInfo::CopyToData(pData);
		_COMPDATA(pData)->m_BlockTerrainIndex = m_BlockTerrainIndex;
	}

	TerrainColliderComponent::TerrainColliderComponent(ActorPtr pActor, ActorCOMMoveableType type) : NodeColliderComponent(pActor,type) {
		m_pCompData = new TerrainColliderComponentData();
		//SetNotifiedPriority(0);
	}
	TerrainColliderComponent::~TerrainColliderComponent() {}

	void TerrainColliderComponent::CreatePhysics() {
		const Index& index = GetBlockTerrainIndex();
		mBlockTerrain = GetPhysicsManager()->GetPhysXWorldManager()->GetBlockTerrain_All(index.x, index.z);
		if (!mBlockTerrain.expired() && mBlockTerrain.lock()->pShape != nullptr) {
			mBlockTerrain.lock()->pShape->SetUserData(this);
			mBlockTerrain.lock()->pShape->SetUserType((int)PhysXUserTypeEnum::ActorComponent);

			SetSurfaceType(mBlockTerrain.lock()->pStatic->GetSurfaceType());
			SetFilterGroupType(ePhysXGroup_Terrain);
			SetCanInteractFilterGroupType(ePhysXGroup_All);

			setWorldTransform(mBlockTerrain.lock()->pStatic->GetTransform());
		}
		m_pShape = nullptr;
		m_pAttachStatic = nullptr;
		m_pAttachRigidBody = nullptr;
		ResetHelperColor();
	}
	void TerrainColliderComponent::ReleasePhysics() {
		if(mBlockTerrain.expired()) return;
		mBlockTerrain.lock()->pShape->SetUserData(nullptr);
		mBlockTerrain.lock()->pShape->SetUserType((int)PhysXUserTypeEnum::Undefined);
		mBlockTerrain.reset();
		m_pShape = nullptr;
		m_pAttachStatic = nullptr;
		m_pAttachRigidBody = nullptr;
		ResetHelperColor();
	}
	void TerrainColliderComponent::init(ActorComponentInfo* info) {
		PhysicsBaseComponent::init(info);
		if (info) {
			static_cast<TerrainColliderComponentInfo*>(info)->CopyToData(m_pCompData);
			SetCompData(m_pCompData);
		}
	}
	void TerrainColliderComponent::unInit() {
		mListenerList.clear();
#ifdef USE_PHYSICS_HELPEROBJ
		ReleaseHelper();
#endif // USE_PHYSICS_HELPEROBJ
		ReleasePhysics();
		PhysicsBaseComponent::unInit();
	}
	void TerrainColliderComponent::connectCom() {
		PhysicsBaseComponent::connectCom();
	}
	void TerrainColliderComponent::disconnectCom() {
		PhysicsBaseComponent::disconnectCom();
	}

	void TerrainColliderComponent::visibleStateChanged(bool activeState) {
		if (!GetPhysicsManager()) return;
		if (activeState) {
			AddToPhysicsUpdateQueue();
			AddToEditorUpdateQueue();
		}
		else {
			ReleasePhysics();
			RemoveFromPhysicsUpdateQueue();
		}
	}

	void TerrainColliderComponent::ResetHelperColor() {
		if (IsExistPhysics()) {
			SetHelperColorType((HelperObjColorType)PhysXColor::gTerrain);
		}
		else {
			SetHelperColorType((HelperObjColorType)PhysXColor::gNotExistPhysics);
		}
	}

	void TerrainColliderComponent::GenerateVBIBCB(std::vector<Vector3>& vbVec, std::vector<uint16>& ibVec) {
		vbVec.clear();
		ibVec.clear();

		if (!IsExistPhysics()) { 
			AddToEditorUpdateQueue(); 
			return; 
		}
		float cellsize;
		std::vector<std::vector<float>> heights;
		mBlockTerrain.lock()->pShape->GetHeightFieldData(cellsize, heights);
		if (heights.empty()) return;
		uint32 nbRows = (uint32)heights.size();
		uint32 nbColumns = (uint32)heights[0].size();
		if (nbRows < 256 && nbColumns < 256) {
			if (nbRows < 2 || nbColumns < 2) return;

			vbVec.resize(nbRows * nbColumns);
			uint32 nbRowsLast = nbRows - 1;
			uint32 nbColumnsLast = nbColumns - 1;
			for (uint32 i = 0; i != nbRows; ++i) {
				for (uint32 j = 0; j != nbColumns; ++j) {
					uint32 index = i * nbColumns + j;
					vbVec[index] = Vector3(i * cellsize, heights[i][j], j * cellsize);
					if (j != nbColumnsLast) {
						ibVec.push_back(index);
						ibVec.push_back(index + 1);
					}
					if (i != nbRowsLast) {
						ibVec.push_back(index);
						ibVec.push_back(index + nbColumns);
					}
				}
			}
		}
		else {
			uint32 nbRowsNew = 256;
			uint32 nbColumnsNew = 256;
			uint32 intRow = nbRows / nbRowsNew;
			uint32 intColumn = nbColumns / nbColumnsNew;
			
			auto SampleHeight = [&](uint32 x, uint32 z) -> float {
				uint32 realRow = x * intRow;
				uint32 realColumn = z * intColumn;
				if (realRow >= nbRows) {
					realRow = nbRows - 1;
				}
				if (realColumn >= nbColumns) {
					realColumn = nbColumns - 1;
				}
				return heights[realRow][realColumn];
			};
			auto GetCellSizeRow = [&](uint32 index) -> float {
				uint32 realRow = index * intRow;
				if (realRow >= nbRows) {
					realRow = nbRows - 1;
				}
				return cellsize * realRow;
			};
			auto GetCellSizeColumn = [&](uint32 index) -> float {
				uint32 realColumn = index * intColumn;
				if (realColumn >= nbColumns) {
					realColumn = nbColumns - 1;
				}
				return cellsize * realColumn;
			};

			uint32 nbRowsLast = nbRowsNew - 1;
			uint32 nbColumnsLast = nbColumnsNew - 1;
			vbVec.resize(nbRowsNew * nbColumnsNew);
			for (uint32 i = 0; i != nbRowsNew; ++i) {
				for (uint32 j = 0; j != nbColumnsNew; ++j) {
					uint32 index = i * nbColumnsNew + j;
					vbVec[index] = Vector3(GetCellSizeRow(i), SampleHeight(i,j), GetCellSizeColumn(j));
					if (j != nbColumnsLast) {
						ibVec.push_back(index);
						ibVec.push_back(index + 1);
					}
					if (i != nbRowsLast) {
						ibVec.push_back(index);
						ibVec.push_back(index + nbColumnsNew);
					}
				}
			}
		}
	}
	
	EchoGeometry* TerrainColliderComponent::GenerateGeometry(void* buffer) {
		//不实现
		return nullptr;
	}

	void TerrainColliderComponent::SetCompData(ColliderComponentData* pData) {
		if (!pData) return;
		SetCompDataBase(pData);
		SetBlockTerrainIndex(_COMPDATA(pData)->m_BlockTerrainIndex);
	}
	bool TerrainColliderComponent::IsCanPhysics() {
		const Index& index = GetBlockTerrainIndex();
		return GetPhysicsManager()->GetPhysXWorldManager()->HasBlockTerrain_All(index.x, index.z);
	}
	bool TerrainColliderComponent::IsExistPhysics() {
		return !mBlockTerrain.expired();
	}
	void TerrainColliderComponent::SetBlockTerrainIndex(const Index& value) {
		_COMPDATA(m_pCompData)->m_BlockTerrainIndex = value;
	}
	const Index& TerrainColliderComponent::GetBlockTerrainIndex() {
		return _COMPDATA(m_pCompData)->m_BlockTerrainIndex;
	}

}
