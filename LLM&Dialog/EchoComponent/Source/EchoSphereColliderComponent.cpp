#include "EchoComponentStableHeaders.h"
#include "EchoSphereColliderComponent.h"
#include "EchoRole.h"
#include "EchocJsonHelper.h"
#include "EchoPhysXScene.h"
#include "EchoPhysXAsset.h"
#include "Actor/EchoActorHelper.h"
#include <cmath>
#include "EchoMatrix3.h"

#define HELPER_ACCURACY 64 

namespace Echo
{
	DECLAREACTORCOMTYPE_CPP(SphereColliderComponent)

#define _COMPDATA(_pData) static_cast<SphereColliderComponentData*>(_pData)

	typedef SphereColliderComponentInfo	InfoType;
	typedef SphereColliderComponentInfo*	InfoPtrType;


	// MakeUnitBallPoints 会在 x轴 y轴 z轴 做 圆环
	// accuracy 表示一个圆环需要多少个点
	std::vector<Vector3> MakeUnitBallPoints(size_t accuracy)
	{
		if (accuracy < 3)
			return{};
		std::vector<Vector3> result(accuracy * 3);
		constexpr float Pi = 3.1415926535f;
		// 垂直于x轴的圆环
		{
			result[0] = { 0,1,0 };	// 垂直于x轴的圆环的第一个点
			float C = cos((2 * Pi) / accuracy), S = sin((2 * Pi) / accuracy);
			Matrix3	RotationMatrix(1, 0, 0,
				0, C, -S,
				0, S, C);
			for (int i = 1; i < accuracy; ++i)	// 求出 垂直于x轴的圆环的每一个点
			{
				result[i] = RotationMatrix * result[i - 1];
			}
		}
		float C = cos(Pi / 2), S = sin(Pi / 2);
		// 垂直于y轴的圆环
		{
			Matrix3	RotationMatrix(C, 0, S,
				0, 1, 0,
				-S, 0, C);
			for (size_t i = accuracy; i < accuracy * 2; ++i)
			{
				result[i] = RotationMatrix * result[i - accuracy];
			}
		}
		// 垂直于Z轴的圆环
		{
			Matrix3	RotationMatrix(C, -S, 0,
				S, C, 0,
				0, 0, 1);
			for (size_t i = accuracy * 2; i < accuracy * 3; ++i)
			{
				result[i] = RotationMatrix * result[i - accuracy * 2];
			}
		}
		return result;
	}
	std::vector<int> MakeLine(int accuracy)
	{
		std::vector<int> result;
		for (int i = 0; i < 3; ++i)
		{
			int beginIndex = accuracy * i;
			for (int j = 0; j < accuracy - 1; ++j)
			{
				result.push_back(j + beginIndex);
				result.push_back(j + beginIndex + 1);
			}
			result.push_back(beginIndex + accuracy - 1);
			result.push_back(beginIndex);
		}
		return result;
	}
	const std::vector<Vector3> unitBallPoints = MakeUnitBallPoints(HELPER_ACCURACY);
	const std::vector<int>	   pointsLine = MakeLine(HELPER_ACCURACY);

	SphereColliderComponentInfo::SphereColliderComponentInfo(const Name& comType, ActorCOMMoveableType MoveType) :ColliderComponentInfo(comType, MoveType)
	{
	}

	SphereColliderComponentInfo::~SphereColliderComponentInfo()
	{
	}

	EDITOR_REFLECT_BEGIN(SphereColliderComponentInfo)
	EDITOR_REFLECT_MEMBER_UPDATE(m_Radius, "Radius", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_END();
	_DEFINEINFO_VIRFUN(SphereColliderComponentInfo, ColliderComponentInfo)

	
	void SphereColliderComponentInfo::CopyToData(ColliderComponentData* pData) {
		if (!pData) return;
		ColliderComponentInfo::CopyToData(pData);
		_COMPDATA(pData)->radius = m_Radius;
	}




	SphereColliderComponent::SphereColliderComponent(ActorPtr pActor,ActorCOMMoveableType type):NodeColliderComponent(pActor,type)
	{
		m_pCompData = new SphereColliderComponentData();
	}

	SphereColliderComponent::~SphereColliderComponent()
	{
	}

	float SphereColliderComponent::GetRadius() {
		return _COMPDATA(m_pCompData)->radius;
	}
	void SphereColliderComponent::SetRadius(float radius) {
		_COMPDATA(m_pCompData)->radius = radius;
#ifdef ECHO_EDITOR
		if (ActorHelper::getForceSave() && m_pActComInfo)
		{
			SphereColliderComponentInfo* colliderInfo = static_cast<SphereColliderComponentInfo*>(m_pActComInfo);
			colliderInfo->SetRadius(radius);
		}
#endif
		ChangeShape();
	}

	void SphereColliderComponent::ResetHelperColor() {
		if (IsExistPhysics()) {
			SetHelperColorType((HelperObjColorType)PhysXColor::gSphere);
		}
		else {
			SetHelperColorType((HelperObjColorType)PhysXColor::gNotExistPhysics);
		}
	}

	void SphereColliderComponent::GenerateVBIBCB(std::vector<Vector3>& vbVec, std::vector<uint16>& ibVec)
	{
#ifdef USE_PHYSICS_HELPEROBJ
		m_pHelperObj->getSceneNode()->_setDerivedScale(Vector3::UNIT_SCALE);
#endif // USE_PHYSICS_HELPEROBJ
		ibVec.clear();
		vbVec.resize(HELPER_ACCURACY * 3);
		ibVec.resize(HELPER_ACCURACY * 3 * 2);
		std::copy(unitBallPoints.begin(), unitBallPoints.end(), vbVec.begin());
		std::copy(pointsLine.begin(), pointsLine.end(), ibVec.begin());
		// 半径 和 位移
		float   radius = GetRealRadius();
		for (auto & ve3 : vbVec )
		{
			ve3 *= radius;
		}
	}
	EchoGeometry * SphereColliderComponent::GenerateGeometry(void* buffer)
	{
		return new(buffer) EchoSphereGeometry(GetRealRadius());
	}

	float SphereColliderComponent::GetRealRadius() {
		Vector3 scaleXYZ = getWorldScale();
		scaleXYZ.x = Abs(scaleXYZ.x);
		scaleXYZ.y = Abs(scaleXYZ.y);
		scaleXYZ.z = Abs(scaleXYZ.z);
		float scale = std::max(scaleXYZ.x, scaleXYZ.y);
		scale = std::max(scale, scaleXYZ.z);
		return  Abs(_COMPDATA(m_pCompData)->radius * scale);
	}

	void SphereColliderComponent::SetCompData(ColliderComponentData* pData) {
		if (!pData) return;
		SetCompDataBase(pData);
		SetRadius(_COMPDATA(pData)->radius);
	}

	bool SphereColliderComponent::IsCanPhysics() {
		float radius = GetRealRadius();
		return !EchoAlmostZero(radius);
	}

} // end namesapce Echo
