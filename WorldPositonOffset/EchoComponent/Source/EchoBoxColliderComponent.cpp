#include "EchoComponentStableHeaders.h"
#include "EchoBoxColliderComponent.h"
#include "Effect/EchoEffectHolder.h"
#include "EchoActorProFuncImp.h"
#include "EchoPhysXScene.h"
#include "EchoPhysXAsset.h"
#include "Actor/EchoActorHelper.h"
#include "EchocJsonHelper.h"
namespace Echo{

	DECLAREACTORCOMTYPE_CPP(BoxColliderComponent)

#define _COMPDATA(_pData) static_cast<BoxColliderComponentData*>(_pData)

	BoxColliderComponentInfo::BoxColliderComponentInfo(const Name& comType, ActorCOMMoveableType MoveType):ColliderComponentInfo(comType, MoveType) {}
	BoxColliderComponentInfo::~BoxColliderComponentInfo() {}

	EDITOR_REFLECT_BEGIN(BoxColliderComponentInfo)
	EDITOR_REFLECT_MEMBER_UPDATE(m_HalfSize, "HalfSize", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_END();

	_DEFINEINFO_VIRFUN(BoxColliderComponentInfo, ColliderComponentInfo)

	
	void BoxColliderComponentInfo::CopyToData(ColliderComponentData* pData) {
		if (!pData) return;
		ColliderComponentInfo::CopyToData(pData);
		_COMPDATA(pData)->halfSize = m_HalfSize;
	}

	BoxColliderComponent::BoxColliderComponent(ActorPtr pActor, ActorCOMMoveableType type) : NodeColliderComponent(pActor,type) {
		m_pCompData = new BoxColliderComponentData();
	}
	BoxColliderComponent::~BoxColliderComponent() {}

	void BoxColliderComponent::ResetHelperColor() {
		if (IsExistPhysics()) {
			SetHelperColorType((HelperObjColorType)PhysXColor::gBox);
		}
		else {
			SetHelperColorType((HelperObjColorType)PhysXColor::gNotExistPhysics);
		}
	}
	
	void BoxColliderComponent::GenerateVBIBCB(std::vector<Vector3>& vbVec, std::vector<uint16>& ibVec) {
		vbVec.clear();
		ibVec.clear();

		Vector3 halfSize = _COMPDATA(m_pCompData)->halfSize;

		std::vector<Vector3> mVbVec = {
			Vector3(1.0f, 1.0f, 1.0f),
			Vector3(-1.0f, 1.0f, 1.0f),
			Vector3(-1.0f, 1.0f, -1.0f),
			Vector3(1.0f, 1.0f, -1.0f),
			Vector3(1.0f, -1.0f, 1.0f),
			Vector3(-1.0f, -1.0f, 1.0f),
			Vector3(-1.0f, -1.0f, -1.0f),
			Vector3(1.0f, -1.0f, -1.0f)
		};
		std::vector<uint16> mIbVec = { 0,1,1,2,2,3,3,0,4,5,5,6,6,7,7,4,0,4,1,5,2,6,3,7 };

		ibVec.insert(ibVec.end(), mIbVec.begin(), mIbVec.end());
		for (Vector3& vb : mVbVec) {
			vb *= halfSize;
		}
		vbVec.insert(vbVec.end(), mVbVec.begin(), mVbVec.end());
	}
	
	EchoGeometry* BoxColliderComponent::GenerateGeometry(void* buffer) {
		Vector3&& size = GetRealHalfSize();
		return new(buffer) EchoBoxGeometry(size);
	}

	Vector3 BoxColliderComponent::GetRealHalfSize() {
		Vector3 scale = _COMPDATA(m_pCompData)->halfSize * getWorldScale();
		scale.x = Abs(scale.x);
		scale.y = Abs(scale.y);
		scale.z = Abs(scale.z);
		return scale;
	}
	
	void BoxColliderComponent::SetHalfSize(const Vector3& size) {
		_COMPDATA(m_pCompData)->halfSize = size;
#ifdef ECHO_EDITOR
		if (ActorHelper::getForceSave() && m_pActComInfo)
		{
			BoxColliderComponentInfo* colliderInfo = static_cast<BoxColliderComponentInfo*>(m_pActComInfo);
			colliderInfo->SetHalfSize(size);
		}
#endif
		ChangeShape();
	}
	const Vector3& BoxColliderComponent::GetHalfSize() {
		return _COMPDATA(m_pCompData)->halfSize;
	}

	void BoxColliderComponent::SetCompData(ColliderComponentData* pData) {
		if (!pData) return;
		SetCompDataBase(pData);
		SetHalfSize(_COMPDATA(pData)->halfSize);
	}
	bool BoxColliderComponent::IsCanPhysics() {
		Vector3&& size = GetRealHalfSize();
		return !(EchoAlmostZero(size.x) || EchoAlmostZero(size.y) || EchoAlmostZero(size.z));
	}

}
