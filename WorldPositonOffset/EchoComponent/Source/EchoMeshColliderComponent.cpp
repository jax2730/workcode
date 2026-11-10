#include "EchoComponentStableHeaders.h"
#include "EchoMeshColliderComponent.h"
#include "EchoProtocolComponent.h"
#include "EchoPhysXManager.h"
#include "EchoPhysXUtils.h"
#include "EchoMeshMgr.h"
#include "EchoLogManager.h"
#include "EchoPhysXMeshSerializer.h"
#include "Actor/EchoActorHelper.h"

namespace Echo {
	namespace EchoPhysics {
		enum class MeshType {
			eData,
			eGen,
		};
		struct MeshBase {
		public:
			virtual ~MeshBase() { mMeshPtr.setNull(); }
			bool isDataType() { return type == MeshType::eData; }
			bool isGenType() { return type == MeshType::eGen; }
			void setMeshPtr(const PhysXMeshResourceBasePtr& _MeshPtr) {
				mMeshPtr = _MeshPtr;
				ClearData();
			}
			void setMeshPtrNull() {
				ResetData();
				mMeshPtr.setNull();
			}
			virtual void ResetData() {}
			virtual void ClearData() {}
			MeshType type;
			PhysXMeshResourceBasePtr mMeshPtr;
		};
		struct DataMesh : public MeshBase {
			DataMesh(const DataMesh* pOther) {
				type = MeshType::eData;
				mMeshPtr = pOther->mMeshPtr;
				vbVec = pOther->vbVec;
				ibVec = pOther->ibVec;
			}
			DataMesh(const std::vector<Vector3>& _vbVec, const std::vector<uint16>& _ibVec) : vbVec(_vbVec), ibVec(_ibVec) {
				type = MeshType::eData;
			}
			virtual void ClearData() override { 
				if (mMeshPtr->getPhysXMeshType() == EchoGeometryType::eTRIANGLEMESH) {
					vbVec.clear();
					vbVec.shrink_to_fit();
					ibVec.clear();
					ibVec.shrink_to_fit();
				}
			}
			virtual void ResetData() override {
				if (!mMeshPtr.isNull()) {
					if (mMeshPtr->getPhysXMeshType() == EchoGeometryType::eTRIANGLEMESH) {
						PhysXTriangleMeshResource* pRes = (PhysXTriangleMeshResource*)mMeshPtr.getPointer();
						pRes->m_mesh->getVertexsAndIndices(vbVec, ibVec);
					}
				}
			}
			std::vector<Vector3> vbVec;
			std::vector<uint16> ibVec;
		};
		struct GenMesh : public MeshBase {
			GenMesh(const GenMesh* pOther) {
				type = MeshType::eGen;
				mMeshPtr = pOther->mMeshPtr;
				mDENFun = pOther->mDENFun;
			}
			GenMesh(std::function<void(std::vector<Vector3>& vbVec, std::vector<uint16>& ibVec)> _GenFun) : mDENFun(_GenFun) {
				type = MeshType::eGen;
			}
			std::function<void(std::vector<Vector3>& vbVec, std::vector<uint16>& ibVec)> mDENFun = nullptr;
		};
	}
	using namespace EchoPhysics;
}

namespace Echo {

#define MeshColliderComponent_PhysXResourceLoad(MESHCOLLIDERCOMPONENT,MESHCOLLIDERCOMPONENTPTR)\
	MeshColliderComponent_LoadPhysXResourcePtr(MESHCOLLIDERCOMPONENT, MESHCOLLIDERCOMPONENTPTR)\
	MeshColliderComponent_AsyncLoadPhysXResourcePtr(MESHCOLLIDERCOMPONENT, MESHCOLLIDERCOMPONENTPTR)\
	MeshColliderComponent_SyncLoadPhysXResourcePtr(MESHCOLLIDERCOMPONENT, MESHCOLLIDERCOMPONENTPTR)\
	MeshColliderComponent_MeshBaseCopyTo(MESHCOLLIDERCOMPONENT, MESHCOLLIDERCOMPONENTPTR)\

#define MeshColliderComponent_LoadPhysXResourcePtr(MESHCOLLIDERCOMPONENT,MESHCOLLIDERCOMPONENTPTR)\
	void MESHCOLLIDERCOMPONENT::LoadPhysXResourcePtr() {\
		AbortAsyncLoad();\
		mAsyncLoadMeshPtrIndex = -1;\
		if (!mMeshPtr.isNull()) {\
			if (!GetMeshName().empty()) {\
				PhysXResourceManager::instance()->AddCachePhysXResource(mMeshPtr);\
			}\
			mMeshPtr.setNull();\
		}\
		if (!getRealVisibleState() || !GetIsUsePhysics()) return;\
		if (IsAsync()) {\
			AsyncLoadPhysXResourcePtr();\
		}\
		else {\
			SyncLoadPhysXResourcePtr();\
		}\
	}

#define MeshColliderComponent_AsyncLoadPhysXResourcePtr(MESHCOLLIDERCOMPONENT,MESHCOLLIDERCOMPONENTPTR)\
	void MESHCOLLIDERCOMPONENT::AsyncLoadPhysXResourcePtr() {\
		if (m_pMeshBase) {\
			if (m_pMeshBase->mMeshPtr.isNull()) {\
				Name resName = GenerateMeshBaseResourceName();\
				MESHCOLLIDERCOMPONENT* _thisPtr = this;\
				EchoPhysics::MeshBase* pMeshBase = m_pMeshBase;\
				GeneratePhysXMeshFunc genCallback = nullptr;\
				PhysXResourceManager::CallbackFunc loadCallback = [_thisPtr, pMeshBase](PhysXMeshResourceBasePtr& meshResource) {\
					if ((_thisPtr->mMeshPtr == meshResource) || meshResource.isNull() || (_thisPtr->m_pMeshBase != pMeshBase) || ((meshResource->getPhysXMeshType() == EchoGeometryType::eCONVEXMESH) != _thisPtr->GetIsConvex()))  { _thisPtr->mAsyncLoadMeshPtrIndex = -1; return;}\
					if (!meshResource->ExistPhysXMesh()) { _thisPtr->mAsyncLoadMeshPtrIndex = -2; return;}\
					_thisPtr->m_pMeshBase->setMeshPtr(meshResource);\
					_thisPtr->mMeshPtr = meshResource;\
					_thisPtr->mAsyncLoadMeshPtrIndex = -1;\
					if (_thisPtr->GetIsUsePhysics()) {\
						_thisPtr->ReleasePhysics();\
						_thisPtr->AddToPhysicsUpdateQueue();\
						_thisPtr->AddToEditorUpdateQueue();\
					}\
				};\
				if (m_pMeshBase->isDataType()) {\
					DataMesh* m_pMeshData = (DataMesh*)m_pMeshBase;\
					if (!m_pMeshData->vbVec.empty()) {\
						genCallback = [_thisPtr](std::vector<Vector3>& vbVec, std::vector<uint16>& ibVec) {\
							if ((_thisPtr == nullptr) || _thisPtr->m_pMeshBase == nullptr || !_thisPtr->m_pMeshBase->isDataType()) return;\
							DataMesh* m_pMeshData = (DataMesh*)_thisPtr->m_pMeshBase;\
							vbVec = m_pMeshData->vbVec;\
							ibVec = m_pMeshData->ibVec;\
						};\
					}\
				}\
				else if (m_pMeshBase->isGenType()) {\
					GenMesh* m_pMeshData = (GenMesh*)m_pMeshBase;\
					if (m_pMeshData->mDENFun != nullptr) {\
						genCallback = [_thisPtr](std::vector<Vector3>& vbVec, std::vector<uint16>& ibVec) {\
							if ((_thisPtr == nullptr) || _thisPtr->m_pMeshBase == nullptr || !_thisPtr->m_pMeshBase->isGenType()) return;\
							GenMesh* m_pMeshData = (GenMesh*)_thisPtr->m_pMeshBase;\
							m_pMeshData->mDENFun(vbVec, ibVec);\
						};\
					}\
				}\
				if (genCallback != nullptr) {\
					if (GetIsConvex()) {\
						auto asyncLoadRet = PhysXResourceManager::instance()->AsyncLoadConvex(resName, genCallback, loadCallback);\
						mAsyncLoadMeshPtrTicket = asyncLoadRet.first;\
						mAsyncLoadMeshPtrIndex = asyncLoadRet.second;\
					}\
					else {\
						auto asyncLoadRet = PhysXResourceManager::instance()->AsyncLoad(resName, genCallback, loadCallback);\
						mAsyncLoadMeshPtrTicket = asyncLoadRet.first;\
						mAsyncLoadMeshPtrIndex = asyncLoadRet.second;\
					}\
				}\
			}\
			else {\
				mMeshPtr = m_pMeshBase->mMeshPtr;\
				if (GetIsUsePhysics()) {\
					ReleasePhysics();\
					AddToPhysicsUpdateQueue();\
					AddToEditorUpdateQueue();\
				}\
			}\
		}\
		else {\
			if (!GetMeshName().empty()) {\
				MESHCOLLIDERCOMPONENT* _thisPtr = this;\
				auto loadCallback = [_thisPtr](PhysXMeshResourceBasePtr& meshResource) {\
					if ((_thisPtr->mMeshPtr == meshResource) || meshResource.isNull() || (meshResource->getName() != _thisPtr->GetMeshName()) || ((meshResource->getPhysXMeshType() == EchoGeometryType::eCONVEXMESH) != _thisPtr->GetIsConvex()))  { _thisPtr->mAsyncLoadMeshPtrIndex = -1; return;}\
					if (!meshResource->ExistPhysXMesh()) { _thisPtr->mAsyncLoadMeshPtrIndex = -2; return; }\
					_thisPtr->mMeshPtr = meshResource;\
					_thisPtr->mAsyncLoadMeshPtrIndex = -1;\
					if (_thisPtr->m_IsChangeSurfaceType) {\
						_thisPtr->SetSurfaceType(meshResource->m_surfaceType);\
					}\
					if (_thisPtr->GetIsUsePhysics()) {\
						_thisPtr->ReleasePhysics();\
						_thisPtr->AddToPhysicsUpdateQueue();\
						_thisPtr->AddToEditorUpdateQueue();\
					}\
				};\
				if (GetIsConvex()) {\
					auto asyncLoadRet = PhysXResourceManager::instance()->AsyncLoadConvex(GetMeshName(), loadCallback);\
					mAsyncLoadMeshPtrTicket = asyncLoadRet.first;\
					mAsyncLoadMeshPtrIndex = asyncLoadRet.second;\
				}\
				else {\
					auto asyncLoadRet = PhysXResourceManager::instance()->AsyncLoad(GetMeshName(), loadCallback);\
					mAsyncLoadMeshPtrTicket = asyncLoadRet.first;\
					mAsyncLoadMeshPtrIndex = asyncLoadRet.second;\
				}\
			}\
		}\
	}\
	void MESHCOLLIDERCOMPONENT::AbortAsyncLoad() {\
		if (mAsyncLoadMeshPtrIndex < 0) return;\
		PhysXResourceManager::instance()->AbortLoad(mAsyncLoadMeshPtrTicket, mAsyncLoadMeshPtrIndex);\
		mAsyncLoadMeshPtrTicket = 0;\
		mAsyncLoadMeshPtrIndex = -1;\
	}\

#define MeshColliderComponent_SyncLoadPhysXResourcePtr(MESHCOLLIDERCOMPONENT,MESHCOLLIDERCOMPONENTPTR)\
	void MESHCOLLIDERCOMPONENT::SyncLoadPhysXResourcePtr() {\
		if (m_pMeshBase) {\
			if (m_pMeshBase->mMeshPtr.isNull()) {\
				Name resName = GenerateMeshBaseResourceName();\
				GeneratePhysXMeshFunc genCallback = nullptr;\
				if (m_pMeshBase->isDataType()) {\
					DataMesh* m_pMeshData = (DataMesh*)m_pMeshBase;\
					if (!m_pMeshData->vbVec.empty()) {\
						MESHCOLLIDERCOMPONENT* _thisPtr = this;\
						genCallback = [_thisPtr](std::vector<Vector3>& vbVec, std::vector<uint16>& ibVec) {\
							if (_thisPtr == nullptr || _thisPtr->m_pMeshBase == nullptr || !_thisPtr->m_pMeshBase->isDataType()) return;\
							DataMesh* m_pMeshData = (DataMesh*)_thisPtr->m_pMeshBase;\
							vbVec = m_pMeshData->vbVec;\
							ibVec = m_pMeshData->ibVec;\
						};\
					}\
				}\
				else if (m_pMeshBase->isGenType()) {\
					GenMesh* m_pMeshData = (GenMesh*)m_pMeshBase;\
					if (m_pMeshData->mDENFun != nullptr) {\
						MESHCOLLIDERCOMPONENT* _thisPtr = this;\
						genCallback = [_thisPtr](std::vector<Vector3>& vbVec, std::vector<uint16>& ibVec) {\
							if (_thisPtr == nullptr || _thisPtr->m_pMeshBase == nullptr || !_thisPtr->m_pMeshBase->isGenType()) return;\
							GenMesh* m_pMeshData = (GenMesh*)_thisPtr->m_pMeshBase;\
							m_pMeshData->mDENFun(vbVec, ibVec);\
						};\
					}\
				}\
				if (genCallback != nullptr) {\
					PhysXMeshResourceBasePtr res;\
					if (GetIsConvex()) {\
						res = PhysXConvexMeshResourceManager::instance()->createGenerateResource(resName, genCallback);\
					}\
					else {\
						res = PhysXTriangleMeshResourceManager::instance()->createGenerateResource(resName, genCallback);\
					}\
					res->load();\
					if (res->ExistPhysXMesh()) {\
						m_pMeshBase->setMeshPtr(res);\
					}\
					else {\
						mAsyncLoadMeshPtrIndex = -2;\
					}\
				}\
			}\
			mMeshPtr = m_pMeshBase->mMeshPtr;\
		}\
		else {\
			if (!GetMeshName().empty()) {\
				try {\
					PhysXMeshResourceBasePtr res;\
					if (GetIsConvex()) {\
						res = PhysXConvexMeshResourceManager::instance()->load(GetMeshName());\
					}\
					else {\
						res = PhysXTriangleMeshResourceManager::instance()->load(GetMeshName());\
					}\
					if (!res.isNull())\
					{\
						if (res->ExistPhysXMesh())\
						{\
							mMeshPtr = res;\
							if (m_IsChangeSurfaceType) {\
								SetSurfaceType(res->m_surfaceType);\
							}\
						}\
						else \
						{\
							mAsyncLoadMeshPtrIndex = -2;\
						}\
					}\
				}\
				catch (...) {\
					LogManager::instance()->logMessage("-error-\t" + GetMeshName().toString() + " resource load faild!");\
				}\
			}\
		}\
		if (GetIsUsePhysics()) {\
			ReleasePhysics();\
			AddToPhysicsUpdateQueue();\
			AddToEditorUpdateQueue();\
		}\
	}

#define MeshColliderComponent_MeshBaseCopyTo(MESHCOLLIDERCOMPONENT,MESHCOLLIDERCOMPONENTPTR)\
	void MESHCOLLIDERCOMPONENT::MeshBaseCopyTo(EchoPhysics::MeshBase* pMeshBase) {\
		if (pMeshBase == nullptr) return;\
		SAFE_DELETE(m_pMeshBase);\
		if (pMeshBase->isDataType()) {\
			DataMesh* pMeshData = (DataMesh*)pMeshBase;\
			m_pMeshBase = new DataMesh(pMeshData);\
		}\
		else if (pMeshBase->isGenType()) {\
			GenMesh* pMeshData = (GenMesh*)pMeshBase;\
			m_pMeshBase = new GenMesh(pMeshData);\
		}\
		LoadPhysXResourcePtr();\
	}

}

namespace Echo
{
#define _COMPDATA(_pData) static_cast<MeshColliderComponentData*>(_pData)

	DECLAREACTORCOMTYPE_CPP(MeshColliderComponent)

	EDITOR_REFLECT_BEGIN(MeshColliderComponentInfo)
	EDITOR_REFLECT_MEMBER_UPDATE(m_IsConvex, "IsConvex", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_MeshFlags, "MeshFlags", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_CookingOptions, "CookingOptions", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_MeshName, "MeshName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_END();
	_DEFINEINFO_VIRFUN(MeshColliderComponentInfo, ColliderComponentInfo)


	void MeshColliderComponentInfo::CopyToData(ColliderComponentData* pData) {
		if (!pData) return;
		ColliderComponentInfo::CopyToData(pData);
		_COMPDATA(pData)->isConvex = m_IsConvex;
		_COMPDATA(pData)->cookingOptions = m_CookingOptions;
		_COMPDATA(pData)->meshFlags = m_MeshFlags;
		_COMPDATA(pData)->meshName = m_MeshName;
	}

	MeshColliderComponent::MeshColliderComponent(ActorPtr pActor, ActorCOMMoveableType type) : NodeColliderComponent(pActor, type) {
		m_pCompData = new MeshColliderComponentData();
	}
	MeshColliderComponent::~MeshColliderComponent() {
		if (mAsyncLoadMeshPtrIndex > -1) {
			LogManager::instance()->logMessage(LogMessageLevel::LML_CRITICAL, "\t --error-- MeshCollider Not AbortAsyncLoad.");
		}
	};

	void MeshColliderComponent::init(ActorComponentInfo* info) {
		NodeColliderComponent::init(info);
		if (getActor() != nullptr) {
			SetPhysicsLocalAABB(getActor()->getExactLocalLoadBounds());
		}
	}
	void MeshColliderComponent::unInit() {
		AbortAsyncLoad();
		SAFE_DELETE(m_pMeshBase);
		mMeshPtr.setNull();
		ColliderComponent::unInit();
	}
	void MeshColliderComponent::copyTo(const ActorComponentPtr& actCom) {
		if (!actCom.get()) return;
		ColliderComponent::copyTo(actCom);
		MeshColliderComponent* pOther = (MeshColliderComponent*)actCom.getPointer();
		pOther->MeshBaseCopyTo(m_pMeshBase);
	}
	void MeshColliderComponent::SetMeshGenFun(std::function<void(std::vector<Vector3>& vbVec, std::vector<uint16>& ibVec)> genFun, const String& resName) {
		SAFE_DELETE(m_pMeshBase);
		m_pMeshBase = new GenMesh(genFun);
		if (resName.empty()) {
			_COMPDATA(m_pCompData)->meshName = Name::g_cEmptyName;
		}
		else {
			_COMPDATA(m_pCompData)->meshName = Name("collision/mesh/" + resName + ".meshphy");
		}
		LoadPhysXResourcePtr();
	}
	void MeshColliderComponent::SetMeshData(const std::vector<Vector3>& vbVec, const std::vector<uint16>& ibVec, const String& resName) {
		SAFE_DELETE(m_pMeshBase);
		m_pMeshBase = new DataMesh(vbVec,ibVec);
		if (resName.empty()) {
			_COMPDATA(m_pCompData)->meshName = Name::g_cEmptyName;
		}
		else {
			_COMPDATA(m_pCompData)->meshName = Name("collision/mesh/" + resName + ".meshphy");
		}
		LoadPhysXResourcePtr();
	}

	void MeshColliderComponent::SetMeshName_mesh(const Name& value) {

		if (Root::instance()->getRuntimeMode() == Root::RuntimeMode::eRM_Client) {
			return;
		}
		
		auto getDstPhyMeshName = [&](const std::pair<uint32, Name>& srcPhyMesh, const Name& _srcMesh) -> Name {
			std::string phyMeshName = _srcMesh.toString() + "phy";
			if (phyMeshName.find(".meshphy") == phyMeshName.npos) {
				return Name::g_cEmptyName;
			}
			std::replace(phyMeshName.begin(), phyMeshName.end(), '\\', '/');
			std::replace(phyMeshName.begin(), phyMeshName.end(), '/', '_');
			phyMeshName.insert(0, "collision/mesh/");
			Name dstPhyMesh = Name(phyMeshName);
			if (!Root::instance()->TestResExist(dstPhyMesh.c_str(), false, "Physics")) {
				try {
					MeshPtr phyMesh = MeshMgr::instance()->getByName(srcPhyMesh.second);
					if (phyMesh.isNull() || !phyMesh->isLoaded())
					{
						phyMesh = MeshMgr::instance()->load(srcPhyMesh.second);
					}
					if (phyMesh.isNull() || !phyMesh->isLoaded())
					{
						return dstPhyMesh;
					}
					//phyMesh->exportToBinary(dstPhyMesh.c_str(), true, true);
					PhysXMeshSerializer serializer;
					MeshToPhysXMeshData toData;
					toData.pMesh = phyMesh;
					toData.surfaceType = srcPhyMesh.first;
					serializer.exportPhysXMesh(toData, dstPhyMesh.toString());
				}
				catch (...)
				{
					LogManager::instance()->logMessage("-error-\t" + srcPhyMesh.second.toString() + " resource load faild!");
				}
			}
			return dstPhyMesh;
			};

		try {
			MeshPtr res = MeshMgr::instance()->getByName(value);
			if (res.isNull() || !res->isLoaded())
			{
				res = MeshMgr::instance()->load(value);
			}
			if (res.isNull() || !res->isLoaded())
			{
				return;
			}
			std::multimap<uint32, Name> mapPhyMesh;
			res->getPhyMeshName(mapPhyMesh);
			if (!mapPhyMesh.empty()) {
				SetMeshName(getDstPhyMeshName(*mapPhyMesh.begin(), value));
			}
			else {
				SetMeshName(getDstPhyMeshName(std::make_pair(0, value), value));
			}

		}
		catch (...) {
			LogManager::instance()->logMessage("-error-\t" + value.toString() + " resource load faild!");
		}
	}
	void MeshColliderComponent::SetMeshName_meshphy(const Name& value) {
		SetMeshName(value);
	}

	void MeshColliderComponent::ExportMeshphyToCooked() {
		if (!mMeshPtr.isNull()) {
			mMeshPtr->exportToCooked();
		}
	}

	void MeshColliderComponent::ExportMeshphyToMesh(const char* fileName) {
		if (!mMeshPtr.isNull()) {
			mMeshPtr->exportToMesh(fileName);
		}
	}
	void MeshColliderComponent::SetPhysicsLocalAABB(const AxisAlignedBox& box) {
		if (box.isNull() || box.isInfinite()) {
			Vector3 extent(0.5f, 0.5f, 0.5f);
			mPhysicsLocalAABB.setExtents(-extent, extent);
		}
		else {
			mPhysicsLocalAABB = box;
		}
	}
	Name MeshColliderComponent::GenerateMeshBaseResourceName() {
		if (m_pMeshBase != nullptr && GetMeshName().empty()) {
			return Name("collision/mesh/" + getComName() + "_" + std::to_string((unsigned long long)(void*)(m_pMeshBase)));
		}
		else {
			return GetMeshName();
		}
	}
	const AxisAlignedBox& MeshColliderComponent::GetPhysicsLocalAABB() {
		return mPhysicsLocalAABB;
	}
	const Name& MeshColliderComponent::GetMeshName() {
		return _COMPDATA(m_pCompData)->meshName;
	}
	void MeshColliderComponent::SetMeshName(const Name& name) {
		_COMPDATA(m_pCompData)->meshName = name;
		SAFE_DELETE(m_pMeshBase);
#ifdef ECHO_EDITOR
		if (ActorHelper::getForceSave() && m_pActComInfo)
		{
			MeshColliderComponentInfo* colliderInfo = static_cast<MeshColliderComponentInfo*>(m_pActComInfo);
			colliderInfo->SetMeshName(name);
		}
#endif
		LoadPhysXResourcePtr();
	}
	void MeshColliderComponent::SetIsConvex(bool value) {
		if (GetIsConvex() != value) {
			_COMPDATA(m_pCompData)->isConvex = value;
#ifdef ECHO_EDITOR
			if (ActorHelper::getForceSave() && m_pActComInfo)
			{
				MeshColliderComponentInfo* colliderInfo = static_cast<MeshColliderComponentInfo*>(m_pActComInfo);
				colliderInfo->SetIsConvex(value);
			}
#endif
			if (!GetIsConvex()) {
				SetIsTrigger(false);
			}
			ResetHelperColor();
			if (m_pMeshBase) {
				m_pMeshBase->setMeshPtrNull();
			}
			ChangeAttach();
			LoadPhysXResourcePtr();
		}
	}
	bool MeshColliderComponent::GetIsConvex() {
		return _COMPDATA(m_pCompData)->isConvex;
	}
	void MeshColliderComponent::SetCookingOptions(uint16 value) {
		_COMPDATA(m_pCompData)->cookingOptions = value;
#ifdef ECHO_EDITOR
		if (ActorHelper::getForceSave() && m_pActComInfo)
		{
			MeshColliderComponentInfo* colliderInfo = static_cast<MeshColliderComponentInfo*>(m_pActComInfo);
			colliderInfo->SetCookingOptions(value);
		}
#endif
	}
	uint16 MeshColliderComponent::GetCookingOptions() {
		return _COMPDATA(m_pCompData)->cookingOptions;
	}

	void MeshColliderComponent::SetMeshFlags(EchoMeshColliderFlags value) {
		bool change = _COMPDATA(m_pCompData)->meshFlags != value;
		_COMPDATA(m_pCompData)->meshFlags = value;
#ifdef ECHO_EDITOR
		if (ActorHelper::getForceSave() && m_pActComInfo)
		{
			MeshColliderComponentInfo* colliderInfo = static_cast<MeshColliderComponentInfo*>(m_pActComInfo);
			colliderInfo->SetMeshFlags(value);
		}
#endif
		if (change && IsExistPhysics()) {
			ChangeShape();
		}
	}
	EchoMeshColliderFlags MeshColliderComponent::GetMeshFlags() {
		return _COMPDATA(m_pCompData)->meshFlags;
	}

	void MeshColliderComponent::ResetHelperColor() {
		if (IsExistPhysics()) {
			if (GetIsConvex()) {
				SetHelperColorType((HelperObjColorType)PhysXColor::gConvexMesh);
			}
			else {
				SetHelperColorType((HelperObjColorType)PhysXColor::gTriangleMesh);
			}
		}
		else {
			SetHelperColorType((HelperObjColorType)PhysXColor::gNotExistPhysics);
		}
	}

	void MeshColliderComponent::GenerateVBIBCB(std::vector<Vector3>& vbVec, std::vector<uint16>& ibVec)
	{
		vbVec.clear();
		ibVec.clear();
		if (mMeshPtr.isNull()) return;
		mMeshPtr->getDebugRenderLines(vbVec, ibVec);
	}
	EchoGeometry * MeshColliderComponent::GenerateGeometry(void* buffer)
	{
		if (mMeshPtr.isNull()) return nullptr;
		if (GetIsConvex()) {
			EchoConvexMeshGeometry* geometry = nullptr;
			PhysXConvexMeshResourcePtr resource = GetConvexMeshPtr();
			if (resource->m_mesh) {
				geometry = new(buffer) EchoConvexMeshGeometry(resource->m_mesh, getWorldScale());
			}
			return geometry;
		}
		else {
			EchoTriangleMeshGeometry* geometry = nullptr;
			PhysXTriangleMeshResourcePtr resource = GetTriangleMeshPtr();
			if (resource->m_mesh) {
				geometry = new(buffer) EchoTriangleMeshGeometry(resource->m_mesh, getWorldScale());
				geometry->flags = (PhysXMeshGeometryFlag)GetMeshFlags();
			}
			return geometry;
		}
	}

	bool MeshColliderComponent::IsCanPhysics() {
		if (mMeshPtr.isNull()) return false;
		return mMeshPtr->ExistPhysXMesh();
	}
	void MeshColliderComponent::ChangeIsTrigger() {
		if (!m_pShape) return;
		if (!GetIsConvex()) {
			m_pCompData->isTrigger = false;
		}
		m_pShape->EnableTrigger(m_pCompData->isTrigger);
	}

	bool MeshColliderComponent::IsCanAttach(RigidBodyComponent* pRB) {
		return pRB->GetIsKinematic() || GetIsConvex();
	}

	void MeshColliderComponent::SetCompData(ColliderComponentData* pData) {
		if (!pData) return;
		if (_COMPDATA(pData)->surfaceType == 0) {
			m_IsChangeSurfaceType = true;
		}
		else {
			m_IsChangeSurfaceType = false;
		}
		SetCompDataBase(pData);
		SetIsConvex(_COMPDATA(pData)->isConvex);
		SetCookingOptions(_COMPDATA(pData)->cookingOptions);
		SetMeshFlags(_COMPDATA(pData)->meshFlags);
		SetMeshName(_COMPDATA(pData)->meshName);
	}

	PhysXConvexMeshResourcePtr& MeshColliderComponent::GetConvexMeshPtr() {
		return (PhysXConvexMeshResourcePtr&)mMeshPtr;
	}
	PhysXTriangleMeshResourcePtr& MeshColliderComponent::GetTriangleMeshPtr() {
		return (PhysXTriangleMeshResourcePtr&)mMeshPtr;
	}

	MeshColliderComponent_PhysXResourceLoad(MeshColliderComponent, MeshColliderComponentPtr)

	void MeshColliderComponent::SetIsUsePhysics(bool value) {
		m_IsUsePhysics = value;
		if (!value) {
			if (IsExistPhysics()) {
				ReleasePhysics();
			}
			else {
				RemoveFromPhysicsUpdateQueue();
			}
			AbortAsyncLoad();
			if (!mMeshPtr.isNull()) {
				if (!GetMeshName().empty()) {
					PhysXResourceManager::instance()->AddCachePhysXResource(mMeshPtr);
				}
				mMeshPtr.setNull();
			}
		}
		else {
			if (!IsExistPhysics()) {
				if (mMeshPtr.isNull()) {
					if (mAsyncLoadMeshPtrIndex != -2) {
						LoadPhysXResourcePtr();
					}
				}
				else {
					AddToPhysicsUpdateQueue();
				}
			}
			//else {
			//	AttachSelfLink();
			//}
		}
	}
#undef _COMPDATA
}; // end namespace Echo
//========================================================================================================================================
namespace Echo
{
#define _COMPDATA(_pData) static_cast<MeshColliderUNMOVEABLEComponentData*>(_pData)

	DECLAREACTORCOMTYPE_CPP(MeshColliderUNMOVEABLEComponent)

		EDITOR_REFLECT_BEGIN(MeshColliderUNMOVEABLEComponentInfo)
		EDITOR_REFLECT_MEMBER_UPDATE(m_IsConvex, "IsConvex", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_MeshFlags, "MeshFlags", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_CookingOptions, "CookingOptions", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_MeshName, "MeshName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_END();
	_DEFINEINFO_VIRFUN(MeshColliderUNMOVEABLEComponentInfo, ColliderComponentInfo)


		void MeshColliderUNMOVEABLEComponentInfo::CopyToData(ColliderComponentData* pData) {
		if (!pData) return;
		ColliderComponentInfo::CopyToData(pData);
		_COMPDATA(pData)->isConvex = m_IsConvex;
		_COMPDATA(pData)->cookingOptions = m_CookingOptions;
		_COMPDATA(pData)->meshFlags = m_MeshFlags;
		_COMPDATA(pData)->meshName = m_MeshName;
	}

	MeshColliderUNMOVEABLEComponent::MeshColliderUNMOVEABLEComponent(ActorPtr pActor, ActorCOMMoveableType type) : ColliderComponent(pActor, type) {
		m_pCompData = new MeshColliderUNMOVEABLEComponentData();
	}
	MeshColliderUNMOVEABLEComponent::~MeshColliderUNMOVEABLEComponent() {
		if (mAsyncLoadMeshPtrIndex > -1) {
			LogManager::instance()->logMessage(LogMessageLevel::LML_CRITICAL, "\t --error-- MeshCollider Not AbortAsyncLoad.");
		}
	};
	void MeshColliderUNMOVEABLEComponent::init(ActorComponentInfo* info) {
		ColliderComponent::init(info);
		if (getActor() != nullptr) {
			SetPhysicsLocalAABB(getActor()->getExactLocalLoadBounds());
		}
	}
	void MeshColliderUNMOVEABLEComponent::unInit() {
		AbortAsyncLoad();
		SAFE_DELETE(m_pMeshBase);
		mMeshPtr.setNull();
		ColliderComponent::unInit();
	}
	void MeshColliderUNMOVEABLEComponent::copyTo(const ActorComponentPtr& actCom) {
		if (!actCom.get()) return;
		ColliderComponent::copyTo(actCom);
		MeshColliderUNMOVEABLEComponent* pOther = (MeshColliderUNMOVEABLEComponent*)actCom.getPointer();
		pOther->MeshBaseCopyTo(m_pMeshBase);
	}
	void MeshColliderUNMOVEABLEComponent::SetMeshGenFun(std::function<void(std::vector<Vector3>& vbVec, std::vector<uint16>& ibVec)> genFun, const String& resName) {
		SAFE_DELETE(m_pMeshBase);
		m_pMeshBase = new GenMesh(genFun);
		if (resName.empty()) {
			_COMPDATA(m_pCompData)->meshName = Name::g_cEmptyName;
		}
		else {
			_COMPDATA(m_pCompData)->meshName = Name("collision/mesh/" + resName + ".meshphy");
		}
		LoadPhysXResourcePtr();
	}
	void MeshColliderUNMOVEABLEComponent::SetMeshData(const std::vector<Vector3>& vbVec, const std::vector<uint16>& ibVec, const String& resName) {
		SAFE_DELETE(m_pMeshBase);
		m_pMeshBase = new DataMesh(vbVec, ibVec);
		if (resName.empty()) {
			_COMPDATA(m_pCompData)->meshName = Name::g_cEmptyName;
		}
		else {
			_COMPDATA(m_pCompData)->meshName = Name("collision/mesh/" + resName + ".meshphy");
		}
		LoadPhysXResourcePtr();
	}

	void MeshColliderUNMOVEABLEComponent::SetMeshName_mesh(const Name& value) {

		if (Root::instance()->getRuntimeMode() == Root::RuntimeMode::eRM_Client) {
			return;
		}

		auto getDstPhyMeshName = [&](const std::pair<uint32, Name>& srcPhyMesh, const Name& _srcMesh) -> Name {
			std::string phyMeshName = _srcMesh.toString() + "phy";
			if (phyMeshName.find(".meshphy") == phyMeshName.npos) {
				return Name::g_cEmptyName;
			}
			std::replace(phyMeshName.begin(), phyMeshName.end(), '\\', '/');
			std::replace(phyMeshName.begin(), phyMeshName.end(), '/', '_');
			phyMeshName.insert(0, "collision/mesh/");
			Name dstPhyMesh = Name(phyMeshName);
			if (!Root::instance()->TestResExist(dstPhyMesh.c_str(), false, "Physics")) {
				try {
					MeshPtr phyMesh = MeshMgr::instance()->getByName(srcPhyMesh.second);
					if (phyMesh.isNull() || !phyMesh->isLoaded())
					{
						phyMesh = MeshMgr::instance()->load(srcPhyMesh.second);
					}
					if (phyMesh.isNull() || !phyMesh->isLoaded())
					{
						return dstPhyMesh;
					}
					//phyMesh->exportToBinary(dstPhyMesh.c_str(), true, true);
					PhysXMeshSerializer serializer;
					MeshToPhysXMeshData toData;
					toData.pMesh = phyMesh;
					toData.surfaceType = srcPhyMesh.first;
					serializer.exportPhysXMesh(toData, dstPhyMesh.toString());
				}
				catch (...)
				{
					LogManager::instance()->logMessage("-error-\t" + srcPhyMesh.second.toString() + " resource load faild!");
				}
			}
			return dstPhyMesh;
			};

		try {
			MeshPtr res = MeshMgr::instance()->getByName(value);
			if (res.isNull() || !res->isLoaded())
			{
				res = MeshMgr::instance()->load(value);
			}
			if (res.isNull() || !res->isLoaded())
			{
				return;
			}
			std::multimap<uint32, Name> mapPhyMesh;
			res->getPhyMeshName(mapPhyMesh);
			if (!mapPhyMesh.empty()) {
				SetMeshName(getDstPhyMeshName(*mapPhyMesh.begin(), value));
			}
			else {
				SetMeshName(getDstPhyMeshName(std::make_pair(0, value), value));
			}

		}
		catch (...) {
			LogManager::instance()->logMessage("-error-\t" + value.toString() + " resource load faild!");
		}
	}
	void MeshColliderUNMOVEABLEComponent::SetMeshName_meshphy(const Name& value) {
		SetMeshName(value);
	}

	void MeshColliderUNMOVEABLEComponent::ExportMeshphyToCooked() {
		if (!mMeshPtr.isNull()) {
			mMeshPtr->exportToCooked();
		}
	}

	void MeshColliderUNMOVEABLEComponent::ExportMeshphyToMesh(const char* fileName) {
		if (!mMeshPtr.isNull()) {
			mMeshPtr->exportToMesh(fileName);
		}
	}
	void MeshColliderUNMOVEABLEComponent::SetPhysicsLocalAABB(const AxisAlignedBox& box) {
		if (box.isNull() || box.isInfinite()) {
			Vector3 extent(0.5f, 0.5f, 0.5f);
			mPhysicsLocalAABB.setExtents(-extent, extent);
		}
		else {
			mPhysicsLocalAABB = box;
		}
	}
	Name MeshColliderUNMOVEABLEComponent::GenerateMeshBaseResourceName() {
		if (m_pMeshBase != nullptr && GetMeshName().empty()) {
			return Name("collision/mesh/" + getComName() + "_" + std::to_string((unsigned long long)(void*)(m_pMeshBase)));
		}
		else {
			return GetMeshName();
		}
	}
	const AxisAlignedBox& MeshColliderUNMOVEABLEComponent::GetPhysicsLocalAABB() {
		return mPhysicsLocalAABB;
	}
	const Name& MeshColliderUNMOVEABLEComponent::GetMeshName() {
		return _COMPDATA(m_pCompData)->meshName;
	}
	void MeshColliderUNMOVEABLEComponent::SetMeshName(const Name& name) {
		_COMPDATA(m_pCompData)->meshName = name;
		SAFE_DELETE(m_pMeshBase);
#ifdef ECHO_EDITOR
		if (ActorHelper::getForceSave() && m_pActComInfo)
		{
			MeshColliderComponentInfo* colliderInfo = static_cast<MeshColliderComponentInfo*>(m_pActComInfo);
			colliderInfo->SetMeshName(name);
		}
#endif
		LoadPhysXResourcePtr();
	}
	void MeshColliderUNMOVEABLEComponent::SetIsConvex(bool value) {
		if (GetIsConvex() != value) {
			_COMPDATA(m_pCompData)->isConvex = value;
#ifdef ECHO_EDITOR
			if (ActorHelper::getForceSave() && m_pActComInfo)
			{
				MeshColliderComponentInfo* colliderInfo = static_cast<MeshColliderComponentInfo*>(m_pActComInfo);
				colliderInfo->SetIsConvex(value);
			}
#endif
			if (!GetIsConvex()) {
				SetIsTrigger(false);
			}
			ResetHelperColor();
			if (m_pMeshBase) {
				m_pMeshBase->setMeshPtrNull();
			}
			ChangeAttach();
			LoadPhysXResourcePtr();
		}
	}
	bool MeshColliderUNMOVEABLEComponent::GetIsConvex() {
		return _COMPDATA(m_pCompData)->isConvex;
	}
	void MeshColliderUNMOVEABLEComponent::SetCookingOptions(uint16 value) {
		_COMPDATA(m_pCompData)->cookingOptions = value;
#ifdef ECHO_EDITOR
		if (ActorHelper::getForceSave() && m_pActComInfo)
		{
			MeshColliderComponentInfo* colliderInfo = static_cast<MeshColliderComponentInfo*>(m_pActComInfo);
			colliderInfo->SetCookingOptions(value);
		}
#endif
	}
	uint16 MeshColliderUNMOVEABLEComponent::GetCookingOptions() {
		return _COMPDATA(m_pCompData)->cookingOptions;
	}

	void MeshColliderUNMOVEABLEComponent::SetMeshFlags(EchoMeshColliderFlags value) {
		bool change = _COMPDATA(m_pCompData)->meshFlags != value;
		_COMPDATA(m_pCompData)->meshFlags = value;
#ifdef ECHO_EDITOR
		if (ActorHelper::getForceSave() && m_pActComInfo)
		{
			MeshColliderComponentInfo* colliderInfo = static_cast<MeshColliderComponentInfo*>(m_pActComInfo);
			colliderInfo->SetMeshFlags(value);
		}
#endif
		if (change && IsExistPhysics()) {
			ChangeShape();
		}
	}
	EchoMeshColliderFlags MeshColliderUNMOVEABLEComponent::GetMeshFlags() {
		return _COMPDATA(m_pCompData)->meshFlags;
	}
	void MeshColliderUNMOVEABLEComponent::ResetHelperColor() {
		if (IsExistPhysics()) {
			if (GetIsConvex()) {
				SetHelperColorType((HelperObjColorType)PhysXColor::gConvexMesh);
			}
			else {
				SetHelperColorType((HelperObjColorType)PhysXColor::gTriangleMesh);
			}
		}
		else {
			SetHelperColorType((HelperObjColorType)PhysXColor::gNotExistPhysics);
		}
	}

	void MeshColliderUNMOVEABLEComponent::GenerateVBIBCB(std::vector<Vector3>& vbVec, std::vector<uint16>& ibVec)
	{
		vbVec.clear();
		ibVec.clear();
		if (mMeshPtr.isNull()) return;
		mMeshPtr->getDebugRenderLines(vbVec, ibVec);
	}
	EchoGeometry* MeshColliderUNMOVEABLEComponent::GenerateGeometry(void* buffer)
	{
		if (mMeshPtr.isNull()) return nullptr;
		if (GetIsConvex()) {
			EchoConvexMeshGeometry* geometry = nullptr;
			PhysXConvexMeshResourcePtr resource = GetConvexMeshPtr();
			if (resource->m_mesh) {
				geometry = new(buffer) EchoConvexMeshGeometry(resource->m_mesh, getWorldScale());
			}
			return geometry;
		}
		else {
			EchoTriangleMeshGeometry* geometry = nullptr;
			PhysXTriangleMeshResourcePtr resource = GetTriangleMeshPtr();
			if (resource->m_mesh) {
				geometry = new(buffer) EchoTriangleMeshGeometry(resource->m_mesh, getWorldScale());
				geometry->flags = (PhysXMeshGeometryFlag)GetMeshFlags();
			}
			return geometry;
		}
	}

	bool MeshColliderUNMOVEABLEComponent::IsCanPhysics() {
		if (mMeshPtr.isNull()) return false;
		return mMeshPtr->ExistPhysXMesh();
	}
	void MeshColliderUNMOVEABLEComponent::ChangeIsTrigger() {
		if (!m_pShape) return;
		if (!GetIsConvex()) {
			m_pCompData->isTrigger = false;
		}
		m_pShape->EnableTrigger(m_pCompData->isTrigger);
	}

	bool MeshColliderUNMOVEABLEComponent::IsCanAttach(RigidBodyComponent* pRB) {
		return pRB->GetIsKinematic() || GetIsConvex();
	}

	void MeshColliderUNMOVEABLEComponent::SetCompData(ColliderComponentData* pData) {
		if (!pData) return;
		if (_COMPDATA(pData)->surfaceType == 0) {
			m_IsChangeSurfaceType = true;
		}
		else {
			m_IsChangeSurfaceType = false;
		}
		SetCompDataBase(pData);
		SetIsConvex(_COMPDATA(pData)->isConvex);
		SetCookingOptions(_COMPDATA(pData)->cookingOptions);
		SetMeshFlags(_COMPDATA(pData)->meshFlags);
		SetMeshName(_COMPDATA(pData)->meshName);
	}

	PhysXConvexMeshResourcePtr& MeshColliderUNMOVEABLEComponent::GetConvexMeshPtr() {
		return (PhysXConvexMeshResourcePtr&)mMeshPtr;
	}
	PhysXTriangleMeshResourcePtr& MeshColliderUNMOVEABLEComponent::GetTriangleMeshPtr() {
		return (PhysXTriangleMeshResourcePtr&)mMeshPtr;
	}

	MeshColliderComponent_PhysXResourceLoad(MeshColliderUNMOVEABLEComponent, MeshColliderUNMOVEABLEComponentPtr)

	void MeshColliderUNMOVEABLEComponent::SetIsUsePhysics(bool value) {
		m_IsUsePhysics = value;
		if (!value) {
			if (IsExistPhysics()) {
				ReleasePhysics();
			}
			else {
				RemoveFromPhysicsUpdateQueue();
			}
			AbortAsyncLoad();
			if (!mMeshPtr.isNull()) {
				if (!GetMeshName().empty()) {
					PhysXResourceManager::instance()->AddCachePhysXResource(mMeshPtr);
				}
				mMeshPtr.setNull();
			}
		}
		else {
			if (!IsExistPhysics()) {
				if (mMeshPtr.isNull()) {
					if (mAsyncLoadMeshPtrIndex != -2) {
						LoadPhysXResourcePtr();
					}
				}
				else {
					AddToPhysicsUpdateQueue();
				}
			}
			//else {
			//	AttachSelfLink();
			//}
		}
	}

#undef _COMPDATA
}; // end namespace Echo
