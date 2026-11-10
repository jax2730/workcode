//-----------------------------------------------------------------------------
// File:   EchoMeshColliderComponent.h
//
// Author: qiao_yuzhi
//
// Date:   2022-7-27
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef EchoMeshColliderComponent_h__
#define EchoMeshColliderComponent_h__

#include "EchoColliderComponent.h"
#include "EchoComDefineMacro.h"
#include "EchoPhysXResource.h"
namespace Echo
{
	enum class CookingOption : uint16 {
		NONE = 0,
		COOK_FOR_FASTER_SIMULATION = 1,
		ENABLE_MESH_CLEANING = 2,
		WELD_COLOCATED_VERTICES = 4,
		USE_FAST_MIDPHASE = 8,
		EVERYTHING = USE_FAST_MIDPHASE | WELD_COLOCATED_VERTICES | ENABLE_MESH_CLEANING | COOK_FOR_FASTER_SIMULATION
	};
	struct EchoMeshColliderFlag {
		enum Enum : uint8 {
			eNONE = 0,
			eDOUBLE_SIDED = (1 << 1)
		};
	};
	typedef uint8 EchoMeshColliderFlags;
	namespace EchoPhysics {
		struct MeshBase;
	};
};
namespace Echo
{
	struct MeshColliderComponentData : public ColliderComponentData {
		uint16 cookingOptions = 0;//弃用
		EchoMeshColliderFlags meshFlags = EchoMeshColliderFlag::eNONE;
		bool isConvex = false;
		Name meshName = Name("");//.meshphy
	};

	class _EchoComponentExport MeshColliderComponent : public NodeColliderComponent
	{
	public:
		DECLAREACTORCOMTYPE_H(MeshColliderComponent)
		REGISTER_LUA_CLASS_KEY
	public:
		virtual void init(ActorComponentInfo* info) override;
		virtual void unInit() override;
		virtual void copyTo(const ActorComponentPtr& actCom) override;
		void SetMeshGenFun(std::function<void(std::vector<Vector3>& vbVec, std::vector<uint16>& ibVec)> genFun, const String& resName = "");
		void SetMeshData(const std::vector<Vector3>& vbVec, const std::vector<uint16>& ibVec, const String& resName = "");
		void SetMeshName_mesh(const Name& value);
		void SetMeshName_meshphy(const Name& value);
		void ExportMeshphyToCooked();
		void ExportMeshphyToMesh(const char* filePath = nullptr);
		void SetPhysicsLocalAABB(const AxisAlignedBox& box);
		const AxisAlignedBox& GetPhysicsLocalAABB();
		Name GenerateMeshBaseResourceName();
	public:
		const Name& GetMeshName();
		void SetMeshName(const Name& meshphy);
		void SetIsConvex(bool value);
		bool GetIsConvex();
		void SetCookingOptions(uint16 value);//弃用
		uint16 GetCookingOptions();//弃用
		void SetMeshFlags(EchoMeshColliderFlags value);
		EchoMeshColliderFlags GetMeshFlags();
		virtual void SetCompData(ColliderComponentData* pData) override;
		virtual void SetIsUsePhysics(bool value) override;
		void SetIsUseAsyncRes(bool value) { m_bAsyncLoad = value; }
		bool GetIsUseAsyncRes() { return m_bAsyncLoad; }
	protected:
		virtual void ResetHelperColor() override;
		virtual void			GenerateVBIBCB(std::vector<Vector3>& vbVec, std::vector<uint16>& ibVec)	override;
		virtual EchoGeometry*	GenerateGeometry(void* buffer)	override;
		virtual bool			IsCanPhysics()					override;

		PhysXConvexMeshResourcePtr& GetConvexMeshPtr();
		PhysXTriangleMeshResourcePtr& GetTriangleMeshPtr();

		void					LoadPhysXResourcePtr();
		void					AsyncLoadPhysXResourcePtr();
		void					SyncLoadPhysXResourcePtr();
		void					MeshBaseCopyTo(EchoPhysics::MeshBase* pMeshBase);

		virtual void			ChangeIsTrigger() override;
		virtual bool			IsCanAttach(RigidBodyComponent* pRB) override;

		void AbortAsyncLoad();

		AxisAlignedBox mPhysicsLocalAABB = AxisAlignedBox(-0.5f, -0.5f, -0.5f, 0.5f, 0.5f, 0.5f);
		EchoPhysics::MeshBase* m_pMeshBase = nullptr;
		PhysXMeshResourceBasePtr mMeshPtr;
		BackgroundProcessTicket mAsyncLoadMeshPtrTicket = 0;
		int mAsyncLoadMeshPtrIndex = -1;
	};
	
	class _EchoComponentExport MeshColliderComponentInfo : public ColliderComponentInfo{
	public:
		MeshColliderComponentInfo() {}
		MeshColliderComponentInfo(const Name& comType, ActorCOMMoveableType MoveType) :ColliderComponentInfo(comType, MoveType) {};
		virtual ~MeshColliderComponentInfo() {};
		virtual void CopyToData(ColliderComponentData* pData) override;
		_DECLCOMINFO_VIRFUN
	public:
		_DECLCOMINFO_PARAM_BEGIN(MeshColliderComponent)
		_DECLCOMINFO_PARAM_SIM(bool, IsConvex, false, SetIsConvex)
		_DECLCOMINFO_PARAM_SIM(EchoMeshColliderFlags, MeshFlags, EchoMeshColliderFlag::eNONE, SetMeshFlags)
		_DECLCOMINFO_PARAM_SIM(uint16, CookingOptions, 0, SetCookingOptions)
		_DECLCOMINFO_PARAM(Name, MeshName, Name(""), SetMeshName)
		_DECLCOMINFO_PARAM_END()
	};

	DEFINECOMFACTORY(MeshColliderComponent,ALL_MOVEABLE)

};
namespace Echo
{
	struct MeshColliderUNMOVEABLEComponentData : public ColliderComponentData {
		uint16 cookingOptions = 0;
		EchoMeshColliderFlags meshFlags = EchoMeshColliderFlag::eNONE;
		bool isConvex = false;
		Name meshName = Name("");//.meshphy
	};

	class _EchoComponentExport MeshColliderUNMOVEABLEComponent : public ColliderComponent
	{
	public:
		DECLAREACTORCOMTYPE_H(MeshColliderUNMOVEABLEComponent)
			REGISTER_LUA_CLASS_KEY
	public:
		virtual void init(ActorComponentInfo* info) override;
		virtual void unInit() override;
		virtual void copyTo(const ActorComponentPtr& actCom) override;
		void SetMeshGenFun(std::function<void(std::vector<Vector3>& vbVec, std::vector<uint16>& ibVec)> genFun, const String& resName = "");
		void SetMeshData(const std::vector<Vector3>& vbVec, const std::vector<uint16>& ibVec, const String& resName = "");
		void SetMeshName_mesh(const Name& value);
		void SetMeshName_meshphy(const Name& value);
		void ExportMeshphyToCooked();
		void ExportMeshphyToMesh(const char* filePath = nullptr);
		void SetPhysicsLocalAABB(const AxisAlignedBox& box);
		const AxisAlignedBox& GetPhysicsLocalAABB();
		Name GenerateMeshBaseResourceName();
	public:
		const Name& GetMeshName();
		void SetMeshName(const Name& meshphy);
		void SetIsConvex(bool value);
		bool GetIsConvex();
		void SetCookingOptions(uint16 value);
		uint16 GetCookingOptions();
		void SetMeshFlags(EchoMeshColliderFlags value);
		EchoMeshColliderFlags GetMeshFlags();
		virtual void SetCompData(ColliderComponentData* pData) override;
		virtual void SetIsUsePhysics(bool value) override;
		void SetIsUseAsyncRes(bool value) { m_bAsyncLoad = value; }
		bool GetIsUseAsyncRes() { return m_bAsyncLoad; }
	protected:
		virtual void ResetHelperColor() override;
		virtual void			GenerateVBIBCB(std::vector<Vector3>& vbVec, std::vector<uint16>& ibVec)	override;
		virtual EchoGeometry* GenerateGeometry(void* buffer)	override;
		virtual bool			IsCanPhysics()					override;

		PhysXConvexMeshResourcePtr& GetConvexMeshPtr();
		PhysXTriangleMeshResourcePtr& GetTriangleMeshPtr();

		void					LoadPhysXResourcePtr();
		void					AsyncLoadPhysXResourcePtr();
		void					SyncLoadPhysXResourcePtr();
		void					MeshBaseCopyTo(EchoPhysics::MeshBase* pMeshBase);

		virtual void			ChangeIsTrigger() override;
		virtual bool			IsCanAttach(RigidBodyComponent* pRB) override;
		void AbortAsyncLoad();

		AxisAlignedBox mPhysicsLocalAABB = AxisAlignedBox(-0.5f, -0.5f, -0.5f, 0.5f, 0.5f, 0.5f);
		EchoPhysics::MeshBase* m_pMeshBase = nullptr;
		PhysXMeshResourceBasePtr mMeshPtr;
		BackgroundProcessTicket mAsyncLoadMeshPtrTicket = 0;
		int mAsyncLoadMeshPtrIndex = -1;
	};

	class _EchoComponentExport MeshColliderUNMOVEABLEComponentInfo : public ColliderComponentInfo {
	public:
		MeshColliderUNMOVEABLEComponentInfo() {}
		MeshColliderUNMOVEABLEComponentInfo(const Name& comType, ActorCOMMoveableType MoveType) :ColliderComponentInfo(comType, MoveType) {};
		virtual ~MeshColliderUNMOVEABLEComponentInfo() {};
		virtual void CopyToData(ColliderComponentData* pData) override;
		_DECLCOMINFO_VIRFUN
	public:
		_DECLCOMINFO_PARAM_BEGIN(MeshColliderUNMOVEABLEComponent)
			_DECLCOMINFO_PARAM_SIM(bool, IsConvex, false, SetIsConvex)
			_DECLCOMINFO_PARAM_SIM(EchoMeshColliderFlags, MeshFlags, EchoMeshColliderFlag::eNONE, SetMeshFlags)
			_DECLCOMINFO_PARAM_SIM(uint16, CookingOptions, 0, SetCookingOptions)
			_DECLCOMINFO_PARAM(Name, MeshName, Name(""), SetMeshName)
			_DECLCOMINFO_PARAM_END()
	};

	DEFINECOMFACTORY(MeshColliderUNMOVEABLEComponent, UNMOVEABLE)

};
#endif // EchoMeshColliderComponent_h__
