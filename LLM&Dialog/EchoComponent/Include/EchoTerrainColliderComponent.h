//-----------------------------------------------------------------------------
// File:   EchoTerrainColliderComponent.h
//
// Author: qiao_yuzhi
//
// Date:   2023-5-4
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef __EchoTerrainColliderComponent_h__ 
#define __EchoTerrainColliderComponent_h__

#include "EchoColliderComponent.h"
#include "EchoComDefineMacro.h"
#include "EchoComInfoDefineMacro.h"

namespace Echo {
	struct TerrainHolder;
	struct TerrainColliderComponentData : public ColliderComponentData {
		Index m_BlockTerrainIndex;
	};

	class _EchoComponentExport TerrainColliderComponent : public NodeColliderComponent {
	public:
		DECLAREACTORCOMTYPE_H(TerrainColliderComponent);
		REGISTER_LUA_CLASS_KEY;
	private:
		virtual EchoPhysicsComponentType GetPhysicsType() override { return EchoPhysicsComponentType::eTERRAIN; }
		virtual void ResetHelperColor() override;
		void GenerateVBIBCB(std::vector<Vector3>& vbVec, std::vector<uint16>& ibVec) override;
		EchoGeometry* GenerateGeometry(void* buffer) override;
	public:
		//与ColliderComponent完全不同的实现
		virtual void CreatePhysics() override;
		virtual void ReleasePhysics() override;

		virtual void init(ActorComponentInfo* info) override;
		virtual void unInit() override;
		virtual void connectCom() override;
		virtual void disconnectCom() override;
		virtual void visibleStateChanged(bool activeState) override;
	public:
		void SetBlockTerrainIndex(const Index& value);
		const Index& GetBlockTerrainIndex();
		virtual void SetCompData(ColliderComponentData* pData) override;
		virtual bool IsCanPhysics() override;
		virtual bool IsExistPhysics() override;
	private:
		std::weak_ptr<TerrainHolder> mBlockTerrain;
		friend class EchoPhysicsSystem;
		friend class EchoPhysicsManager;
	};

	class _EchoComponentExport TerrainColliderComponentInfo : public ColliderComponentInfo {
	public:
		TerrainColliderComponentInfo() {}
		TerrainColliderComponentInfo(const Name& comType, ActorCOMMoveableType MoveType);
		virtual ~TerrainColliderComponentInfo();
		virtual void CopyToData(ColliderComponentData* pData) override;
		_DECLCOMINFO_VIRFUN
	
	public://存储数据
		_DECLCOMINFO_PARAM_BEGIN(TerrainColliderComponent)
		_DECLCOMINFO_PARAM(Index, BlockTerrainIndex, Index(0,0), SetBlockTerrainIndex)
		_DECLCOMINFO_PARAM_END()
	};

	DEFINECOMFACTORY(TerrainColliderComponent, ALL_MOVEABLE)


}



#endif /*__EchoTerrainColliderComponent_h__*/
