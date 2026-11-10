#include "EchoComponentStableHeaders.h"
#include "EchoCapsuleColliderComponent.h"
#include "EchoRole.h"
#include "EchocJsonHelper.h"
#include "EchoPhysXScene.h"
#include "EchoPhysXAsset.h"
#include "Actor/EchoActorHelper.h"
namespace Echo
{

	DECLAREACTORCOMTYPE_CPP(CapsuleColliderComponent)

#define _COMPDATA(_pData) static_cast<CapsuleColliderComponentData*>(_pData)

	typedef CapsuleColliderComponentInfo	InfoType;
	typedef CapsuleColliderComponentInfo*	InfoPtrType;
	Echo::CapsuleColliderComponentInfo::CapsuleColliderComponentInfo(const Name& comType, ActorCOMMoveableType MoveType) :ColliderComponentInfo(comType, MoveType)
	{
	}

	Echo::CapsuleColliderComponentInfo::~CapsuleColliderComponentInfo()
	{
	}

	EDITOR_REFLECT_BEGIN(CapsuleColliderComponentInfo)
	EDITOR_REFLECT_MEMBER_UPDATE(m_Radius, "Radius", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_Height, "Height", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_END();
	_DEFINEINFO_VIRFUN(CapsuleColliderComponentInfo, ColliderComponentInfo)


	void Echo::CapsuleColliderComponentInfo::CopyToData(ColliderComponentData* pData) {
		if (!pData) return;
		ColliderComponentInfo::CopyToData(pData);
		_COMPDATA(pData)->radius = m_Radius;
		_COMPDATA(pData)->height = m_Height;
	}

	CapsuleColliderComponent::CapsuleColliderComponent(ActorPtr pActor, ActorCOMMoveableType type):NodeColliderComponent(pActor,type)
	{
		m_pCompData = new CapsuleColliderComponentData();
	}

	CapsuleColliderComponent::~CapsuleColliderComponent(){}

	float CapsuleColliderComponent::GetRadius() {
		return _COMPDATA(m_pCompData)->radius;
	}
	float CapsuleColliderComponent::GetHeight() {
		return _COMPDATA(m_pCompData)->height;
	}
	void CapsuleColliderComponent::SetRadius(float radius) {
		_COMPDATA(m_pCompData)->radius = radius;
#ifdef ECHO_EDITOR
		if (ActorHelper::getForceSave() && m_pActComInfo)
		{
			CapsuleColliderComponentInfo* colliderInfo = static_cast<CapsuleColliderComponentInfo*>(m_pActComInfo);
			colliderInfo->SetRadius(radius);
		}
#endif
		ChangeShape();
	}
	void CapsuleColliderComponent::SetHeight(float height) {
		_COMPDATA(m_pCompData)->height = height;
#ifdef ECHO_EDITOR
		if (ActorHelper::getForceSave() && m_pActComInfo)
		{
			CapsuleColliderComponentInfo* colliderInfo = static_cast<CapsuleColliderComponentInfo*>(m_pActComInfo);
			colliderInfo->SetHeight(height);
		}
#endif
		ChangeShape();
	}

	void CapsuleColliderComponent::SetDirectionX() {
		setRotation(Quaternion::IDENTITY);
	}
	void CapsuleColliderComponent::SetDirectionY() {
		setRotation({ 0.0f, 0.0f, 0.707107f, 0.707107f });
	}
	void CapsuleColliderComponent::SetDirectionZ() {
		setRotation({ 0.0f, -0.707107f, 0.0f, 0.707107f });
	}

	void CapsuleColliderComponent::ResetHelperColor() {
		if (IsExistPhysics()) {
			SetHelperColorType((HelperObjColorType)PhysXColor::gCapsule);
		}
		else {
			SetHelperColorType((HelperObjColorType)PhysXColor::gNotExistPhysics);
		}
	}

	void CapsuleColliderComponent::GenerateVBIBCB(std::vector<Vector3>& vbVec, std::vector<uint16>& ibVec)
	{
#ifdef USE_PHYSICS_HELPEROBJ
		m_pHelperObj->getSceneNode()->_setDerivedScale(Vector3::UNIT_SCALE);
#endif // USE_PHYSICS_HELPEROBJ
		vbVec.clear(); ibVec.clear();
		vbVec.resize(256); 
		float radius, halfHeight;
		GetRealRadiusHalfHeight(radius, halfHeight);

		for (int i = 0; i < 16 ; ++i)// 其中 0~31 32 ~ 63 为纵切面的点 64 ~ 127 为横切面的点 
		{
			float	x = 0 ,
					y = radius*( 1 - i / 16.0f ),
					z = sqrt(radius*radius - y * y) ;
			vbVec[i + 00] = { x,y,z + halfHeight };		vbVec[i + 00 + 128] = { x,y,-z - halfHeight };
			vbVec[31- i] = { x,-y,z + halfHeight };		vbVec[31 - i + 128] = { x,-y,-z - halfHeight };

			vbVec[i + 32] = { y,x,z + halfHeight };		vbVec[i + 32 + 128] = { y,x,-z - halfHeight };
			vbVec[63 - i] = { -y,x,z + halfHeight };	vbVec[63 - i + 128] = { -y,x,-z - halfHeight };

			vbVec[i + 64] = { z,y,x + halfHeight };		vbVec[i + 64 + 128] = { z,y,-x - halfHeight };
			vbVec[95 - i] = { z,-y,x + halfHeight };	vbVec[95 - i + 128] = { z,-y,-x - halfHeight };

			vbVec[111 - i] = { -y,-z,x + halfHeight };	vbVec[111 - i + 128] = { -y,-z,-x - halfHeight };
			vbVec[112 + i] = { -y,z,x + halfHeight };	vbVec[112 + i + 128] = { -y,z,-x - halfHeight };
		}
		// 添加连接线
		ibVec.push_back(0);		ibVec.push_back(128);
		ibVec.push_back(31);	ibVec.push_back(128+31);
		ibVec.push_back(32);	ibVec.push_back(128+32);
		ibVec.push_back(63);	ibVec.push_back(128+63);
		for (int i = 0; i < 31; ++i)
		{
			ibVec.push_back(i);					ibVec.push_back(i + 1);
			ibVec.push_back(32 + i);			ibVec.push_back(32 + i + 1);

			ibVec.push_back(128 + i);			ibVec.push_back(128 + i + 1);
			ibVec.push_back(128 + 32 + i);		ibVec.push_back(128 + 32 + i + 1);
		}
		for (int i = 0; i < 63; ++i)
		{
			ibVec.push_back(64 + i);				ibVec.push_back(64 + i + 1);
			ibVec.push_back(128 + 64 + i);			ibVec.push_back(128 + 64 + i + 1);
		}
		ibVec.push_back(64);
		ibVec.push_back(127);
		ibVec.push_back(64+128);
		ibVec.push_back(255);

		for (auto & ve3 : vbVec)
		{
			std::swap(ve3.x, ve3.z);
		}

// 		// 方向
// 		int dir = static_cast <InfoPtrType> (m_pActComInfo) ->direction;
// 		if ('x' == dir)
// 		{
// 			for (auto & ve3 : vbVec)
// 			{
// 				std::swap(ve3.x, ve3.z);
// 			}
// 		}
// 		else if ('y' == dir)
// 		{
// 			for (auto & ve3 : vbVec)
// 			{
// 				std::swap(ve3.y, ve3.z);
// 			}
// 		}
// 		// 偏移
// 		for (auto & ve3 : vbVec)
// 			ve3 += center;
			
	}

	EchoGeometry * CapsuleColliderComponent::GenerateGeometry(void* buffer)
	{
		float radius, halfHeight;
		GetRealRadiusHalfHeight(radius, halfHeight);
		EchoCapsuleGeometry* res = new(buffer) EchoCapsuleGeometry(radius, halfHeight);
		res->upDirection = EchoCapsuleGeometry::UpDirection::eX;
		return res;
	}

	void CapsuleColliderComponent::GetRealRadiusHalfHeight(float& _radius, float& _halfHeight) {

		Vector3 scale = getWorldScale();
		scale.x = Abs(scale.x);
		scale.y = Abs(scale.y);
		scale.z = Abs(scale.z);
		float radius = _COMPDATA(m_pCompData)->radius * std::max(scale.z, scale.y);
		float height = _COMPDATA(m_pCompData)->height * scale.x;
		height *= 0.5f;
		height -= radius;
		height = std::max(height, 0.0001f);
		_radius = Abs(radius);
		_halfHeight = Abs(height);
	}

	void CapsuleColliderComponent::SetCompData(ColliderComponentData* pData) {
		if (!pData) return;
		SetCompDataBase(pData);
		SetRadius(_COMPDATA(pData)->radius);
		SetHeight(_COMPDATA(pData)->height);
	}

	bool CapsuleColliderComponent::IsCanPhysics() {
		float radius, halfHeight;
		GetRealRadiusHalfHeight(radius, halfHeight);
		return !EchoAlmostZero(radius);
	}

}; // end namespace Echo

