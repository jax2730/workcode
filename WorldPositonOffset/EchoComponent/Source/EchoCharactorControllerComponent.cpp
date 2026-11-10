#include "EchoComponentStableHeaders.h"
#include "EchoCharactorControllerComponent.h"

#include "EchoPhysXCharacterController.h"
#include "EchoActorRoleComponent.h"
#include "IK/EchoIKCommon.h"
#include "EchoColliderComponent.h"
#include "EchoPhysXManager.h"
namespace Echo {

#define _COMPDATA(_pData) static_cast<CharatorControllerComponentData*>(_pData)
	Echo::Name CharatorControllerComponent::mType("CharatorControllerComponent");

EDITOR_REFLECT_BEGIN(CharatorControllerComponentInfo)
//EDITOR_REFLECT_MEMBER_UPDATE(m_FilterGroupType, "FilterGroupType", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
EDITOR_REFLECT_MEMBER_UPDATE(m_SlopeLimit, "SlopeLimit", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
EDITOR_REFLECT_MEMBER_UPDATE(m_StepOffset, "StepOffset", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
EDITOR_REFLECT_MEMBER_UPDATE(m_SkinWidth, "SkinWidth", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
EDITOR_REFLECT_MEMBER_UPDATE(m_MinMoveDistance, "MinMoveDistance", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
EDITOR_REFLECT_MEMBER_UPDATE(m_Radius, "Radius", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
EDITOR_REFLECT_MEMBER_UPDATE(m_MiddleHeight, "MiddleHeight", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
EDITOR_REFLECT_MEMBER_UPDATE(m_Center, "Center", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
EDITOR_REFLECT_MEMBER_UPDATE(m_CenterModel, "CenterModel", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
EDITOR_REFLECT_MEMBER_UPDATE(m_IsCurrentController, "IsCurrentController", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
//EDITOR_REFLECT_MEMBER_UPDATE(m_ControllerRoleCompName, "ControllerRoleCompName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
EDITOR_REFLECT_END();

	_DEFINEINFO_VIRFUN(CharatorControllerComponentInfo, ColliderComponentInfo)

	void CharatorControllerComponentInfo::CopyToData(ColliderComponentData* _pData) {
		if (!_pData) return;
		ColliderComponentInfo::CopyToData(_pData);
		CharatorControllerComponentData* pData = static_cast<CharatorControllerComponentData*>(_pData);
		pData->slopeLimit		= m_SlopeLimit;
		pData->stepOffset		= m_StepOffset;
		pData->skinWidth		= m_SkinWidth;
		pData->minMoveDistance	= m_MinMoveDistance;
		pData->radius			= m_Radius;
		pData->middleHeight		= m_MiddleHeight;
		pData->center			= m_Center;
		pData->centerModel		= m_CenterModel;
	}

	static constexpr uint16 baseSize = 64;

	// 该单位球（半径为1 球心为{0,0,0}）共有192个点 其中按索引顺序每64个点为一个圆 法向量分别为 x轴 y轴 z轴
	// 法向量为x轴的点 按索引顺序 从{0,0,1}点 开始 按照逆时针(x轴正方向指向自己)均匀取点
	// 法向量为y轴的点 为法向量为x轴的所有点 绕z轴旋转 pi/2 度
	// 法向量为z轴的点 为法向量为x轴的所有点 绕y轴旋转 pi/2 度
	std::vector<Vector3> makeUnitBallPoints()
	{
		constexpr float  PI = 3.141592653589793f;

		std::vector<Vector3> result(baseSize * 3);
		result[0] = { 0,0,1 };
		float C = std::cos((PI * 2) / baseSize);
		float S = std::sin((PI * 2) / baseSize);
		Matrix3 R = { 1, 0, 0,
			0, C, -S,
			0, S, C };
		// x 
		for (int i = 1; i < baseSize; ++i)
			result[i] = R * result[i - 1];
		C = 0;
		S = 1;
		// y
		R = { C, -S, 0,
			S, C, 0,
			0, 0, 1 };
		for (int i = 0; i < baseSize; ++i)
			result[baseSize + i] = R*result[i];
		// z 
		R = { C , 0 , S ,
			0 , 1 , 0 ,
			-S , 0 , C };
		for (int i = 0; i < baseSize; ++i)
			result[baseSize * 2 + i] = R*result[i];

		return result;
	}
	const std::vector<Vector3> unitBallPoints = makeUnitBallPoints();

	std::vector<uint16> makeLinkLine()
	{
		std::vector<uint16> result;
		for (uint16 i = 0; i < 2; ++i)
		{
			uint16 begin = baseSize * 2 * i;
			for (uint16 j = 0; j < baseSize / 2 - 1; ++j)
			{
				result.push_back(begin + j);
				result.push_back(begin + j + 1);
			}
			//result.push_back(begin + baseSize / 2 -1);
			//result.push_back(begin + 0);

			for (uint16 j = baseSize / 2; j < baseSize * 1.5 - 1; ++j)
			{
				result.push_back(begin + j);
				result.push_back(begin + j + 1);
			}
			result.push_back(begin + baseSize * 3 / 2 - 1);
			result.push_back(begin + baseSize / 2);

			for (uint16 j = baseSize * 3 / 2; j < baseSize * 2 - 1; ++j)
			{
				result.push_back(begin + j);
				result.push_back(begin + j + 1);
			}
			//result.push_back(begin + baseSize * 2 - 1);
			//result.push_back(begin + baseSize * 3/2);
		}
		result.push_back(0), result.push_back(baseSize * 5 / 2 - 1);
		result.push_back(baseSize / 2 - 1), result.push_back(baseSize * 2);

		result.push_back(baseSize * 3 / 2), result.push_back(baseSize * 4 - 1);
		result.push_back(baseSize * 2 - 1), result.push_back(baseSize * 7 / 2);
		return result;
	}
	const std::vector<uint16> linkLine = makeLinkLine();

	CharatorControllerComponent::CharatorControllerComponent(ActorPtr pActor, ActorCOMMoveableType type) :ColliderComponent(pActor, type) {
		m_pCompData = new CharatorControllerComponentData();
	}
	CharatorControllerComponent:: ~CharatorControllerComponent(){}
	
	void CharatorControllerComponent::connectCom() {
		PhysicsBaseComponent::connectCom();
		ComponentUpdatePhysics();
	}
	void CharatorControllerComponent::disconnectCom() {
		ReleasePhysics();
		RemoveSelfLink();
		PhysicsBaseComponent::disconnectCom();
	}
	void CharatorControllerComponent::connectActor() {
		PhysicsBaseComponent::connectActor();
	}
	void CharatorControllerComponent::disconnectActor() {
		PhysicsBaseComponent::disconnectActor();
	}

	void CharatorControllerComponent::init(ActorComponentInfo * info)
	{
		PhysicsBaseComponent::init(info);
		if (info) {
			static_cast<CharatorControllerComponentInfo*>(info)->CopyToData(m_pCompData);
			SetCompData(m_pCompData);
			SetIsCurrentController(((CharatorControllerComponentInfo*)info)->m_IsCurrentController);
		}
		SetNotifiedUpdate(true);
	}
	void CharatorControllerComponent::unInit() {
		GetPhysicsManager()->RemoveColliderFromUpdateGeometry(this);
		mListenerList.clear();
#ifdef USE_PHYSICS_HELPEROBJ
		ReleaseHelper();
#endif // USE_PHYSICS_HELPEROBJ
		ReleasePhysics();
		SetNotifiedUpdate(false);
		if (EchoPhysicsSystem::instance()->GetCurrentCharactorController() == this) EchoPhysicsSystem::instance()->SetCurrentCharactorController(nullptr);
		PhysicsBaseComponent::unInit();
	}
	
	void CharatorControllerComponent::replay(ActorComponentInfo * info)
	{
		if (!info) return;
		ColliderComponent::replay(info);
		SetIsCurrentController(((CharatorControllerComponentInfo*)info)->m_IsCurrentController);
	}

	void CharatorControllerComponent::NotifiChildImp(int changeMask) {
		if ((uint8)changeMask == WorldNode::TSC_NONE) return;
		if (((uint8)changeMask & WorldNode::TSC_SCALE) == WorldNode::TSC_SCALE) {
			ChangeGeometry();
		}
		if (m_IsUpdatePhysicsTransform) {
			ComponentUpdatePhysics();
		}
	}

	void CharatorControllerComponent::visibleStateChanged(bool activeState) {
		PhysicsBaseComponent::visibleStateChanged(activeState);
	}

	void CharatorControllerComponent::SetRealSize(float radius, float middleHeight) {
		const float kMinScale = 0.00001F;
		const float kMinSize = 0.00001F;
		if (radius < 0 || middleHeight < 0) return;
		Vector3 scale = getWorldScale();

		float sh = std::max(Abs(scale.y), kMinScale);
		float newMiddleHeight = std::max(middleHeight / sh, kMinSize);
		float sr = std::max(std::max(Abs(scale.x), Abs(scale.z)), kMinScale);
		float newRadius = std::max(radius / sr, kMinScale);
		
		SetMiddleHeight(newMiddleHeight);
		SetRadius(newRadius);
	}
	void CharatorControllerComponent::GetRealSize(float& _radius, float& _middleHeight) {
		const float kMinScale = 0.00001F;
		const float kMinSize = 0.00001F;

		float r = GetRadius();
		float mh = GetMiddleHeight();
		if (r < 0 || mh < 0) return;
		Vector3 scale = getWorldScale();

		float sh = std::max(Abs(scale.y), kMinScale);
		float middleHeight = std::max(mh * sh, kMinSize);
		float sr = std::max(std::max(Abs(scale.x), Abs(scale.z)), kMinScale);
		float radius = std::max(r * sr, kMinScale);

		_middleHeight = middleHeight;
		_radius = radius;
	}

	uint32 CharatorControllerComponent::Move(float dtime, const Vector3 & dist, bool IsMove)
	{
		if (m_pChactrl == nullptr) {
			return 0;
		}
		uint32 res = m_pChactrl->move(dist, dtime, _COMPDATA(m_pCompData)->minMoveDistance);
		if (IsMove) {
			PhysicsUpdateComponent();
		}
		m_pChactrl->UpdateHit();
		UpdateHelperPosition();
		return res;
	}

	bool CharatorControllerComponent::PhysicsUpdateComponent() {
		DVector3 newPos = GetCCTWorldPosition();
		if (newPos.isEqual(getWorldPosition(), 1e-4)) return false;
		m_IsUpdatePhysicsTransform = false;
		getParentComponentNode()->setWorldPosition(newPos);
		m_IsUpdatePhysicsTransform = true;
		return true;
	}
	bool CharatorControllerComponent::ComponentUpdatePhysics() {
		SetCCTWorldPosition(getWorldPosition());
		return true;
	}

	void CharatorControllerComponent::CreatePhysics() {
		if (!GetPhysicsManager()->IsPhysicsUpdating()) return;
		if (m_pChactrl != nullptr)
			return;
		ComponentUpdatePhysics();
		float radius, middleHeight;
		GetRealSize(radius, middleHeight);
		PhysXControllerShapeCapsule capsuleShape;
		capsuleShape.radius = radius;
		capsuleShape.middleHeight = middleHeight;
		capsuleShape.climbingMode = 1;
		PhysXControllerDesc desc(capsuleShape);
		desc.position = mCctPosition;
		desc.slopeLimit = std::max(0.f, std::cos(_COMPDATA(m_pCompData)->slopeLimit / 180 * ECHO_PI));
		desc.stepOffset = _COMPDATA(m_pCompData)->stepOffset;
		desc.contactOffset = _COMPDATA(m_pCompData)->skinWidth;
		desc.userData = this;
		desc.userType = (int)PhysXUserTypeEnum::ActorComponent;
		desc.scaleCoeff = GetPhysicsManager()->GetControllerShapeScale();
		desc.nonWalkableMode = mNonWalkableMode ? PhysXControllerNonWalkableMode::ePREVENT_CLIMBING_AND_FORCE_SLIDING : PhysXControllerNonWalkableMode::ePREVENT_CLIMBING;
		
		m_pChactrl = GetPhysicsManager()->GetScene()->AddCharacterController(desc);
		if (!m_pChactrl) return;
		m_pChactrl->SetControllerCallBack(GetPhysicsManager()->mPhysXCCTCallBack);
		m_pChactrl->setUpDirection(mUpDirection);
		m_pShape = m_pChactrl->GetPhysXShape();
		m_pShape->SetUserData(this);
		m_pShape->SetUserType((int)PhysXUserTypeEnum::ActorComponent);
		ChangeFilterGroup();
		ChangeIsTrigger();
		AttachSelfLink();
		ResetHelperColor();
	}
	void CharatorControllerComponent::ReleasePhysics() {
		if (!IsExistPhysics())
			return;
		mCctPosition = m_pChactrl->getPosition();
		DetachSelfLink();
		m_pShape = nullptr;
		GetPhysicsManager()->RemovePhysXActor(m_pChactrl);
		m_pChactrl = nullptr;
		ResetHelperColor();
	}
	bool CharatorControllerComponent::IsCanPhysics() {
		float radius, middleHeight;
		GetRealSize(radius, middleHeight);
		return !EchoAlmostZero(radius);
	}
	bool CharatorControllerComponent::IsExistPhysics() {
		return m_pChactrl != nullptr;
	}

#ifdef USE_PHYSICS_HELPEROBJ
	void CharatorControllerComponent::UpdateComponentInEditor() {
		if (m_IsHelperVisible) {
			if (!m_pHelperObj && IsCanPhysics()) {
				CreateHelper();
			}
			if (m_pHelperObj) {
				UpdateHelper();
				UpdateHelperPosition();
				UpdateHelperRotation();
			}
		}
		else if (m_pHelperObj) {
			ReleaseHelper();
		}
	}
#endif // USE_PHYSICS_HELPEROBJ

	DVector3 CharatorControllerComponent::GetWorldCenterPosition() {
		return GetPositionCCT();
	}
	DVector3 CharatorControllerComponent::GetWorldFootPosition(bool IsSkin) {
		return GetWorldCenterPosition() - DVector3(CalCenterOffset(IsSkin));
	}
	DVector3 CharatorControllerComponent::GetWorldHeadPosition(bool IsSkin) {
		return GetWorldCenterPosition() + DVector3(CalCenterOffset(IsSkin));
	}
	void CharatorControllerComponent::SetWorldCenterPosition(const DVector3& position, bool IsMove) {
		SetPositionCCT(position);
		if (IsMove) {
			PhysicsUpdateComponent();
		}
	}
	void CharatorControllerComponent::SetWorldFootPosition(const DVector3& position, bool IsMove, bool IsSkin) {
		SetWorldCenterPosition(position + DVector3(CalCenterOffset(IsSkin)), IsMove);
	}
	void CharatorControllerComponent::SetWorldHeadPosition(const DVector3& position, bool IsMove, bool IsSkin) {
		SetWorldCenterPosition(position - DVector3(CalCenterOffset(IsSkin)), IsMove);
	}
	const Vector3& CharatorControllerComponent::GetUpDirection() {
		return mUpDirection;
	}
	void CharatorControllerComponent::SetUpDirection(const Vector3& up) {
		DVector3 oldPos = GetCCTWorldPosition();
		mUpDirection = up.GetNormalized();
		if (m_pChactrl) m_pChactrl->setUpDirection(mUpDirection);
		SetCCTWorldPosition(oldPos);
		UpdateHelperRotation();
	}

	void CharatorControllerComponent::SetSlopeLimit(float slopLimit)
	{
		slopLimit = Math::Clamp(slopLimit, 0.0f, 90.0f);
		_COMPDATA(m_pCompData)->slopeLimit = slopLimit;

		if (m_pChactrl)
			m_pChactrl->setSlopeLimit(std::max(0.f, std::cos(_COMPDATA(m_pCompData)->slopeLimit / 180 * ECHO_PI)));
	}

	float CharatorControllerComponent::GetSlopeLimit()
	{
		return _COMPDATA(m_pCompData)->slopeLimit;
	}

	void CharatorControllerComponent::SetStepOffset(float stepOffset)
	{
		stepOffset = Math::Clamp(stepOffset, 0.0001f, GetRadius() * 2.f + GetMiddleHeight());//GetRealHeight();
		_COMPDATA(m_pCompData)->stepOffset = stepOffset;

		if (m_pChactrl)	
			m_pChactrl->setStepOffset(_COMPDATA(m_pCompData)->stepOffset);
	}

	float CharatorControllerComponent::GetStepOffset()
	{
		return _COMPDATA(m_pCompData)->stepOffset;
	}

	void CharatorControllerComponent::SetSkinWidth(float skinWidth)
	{
		skinWidth = Math::Clamp(skinWidth, 1e-6f, FLT_MAX);
		DVector3 oldPos = GetCCTWorldPosition();
		_COMPDATA(m_pCompData)->skinWidth = skinWidth;
		if (m_pChactrl) m_pChactrl->setContactOffset(skinWidth);
		SetCCTWorldPosition(oldPos);
	}

	float CharatorControllerComponent::GetSkinWidth()
	{
		return _COMPDATA(m_pCompData)->skinWidth;
	}

	void CharatorControllerComponent::SetMinMoveDistance(float minMoveDistance)
	{
		minMoveDistance = Math::Clamp(minMoveDistance, 0.0f, FLT_MAX);
		_COMPDATA(m_pCompData)->minMoveDistance = minMoveDistance;
	}

	float CharatorControllerComponent::GetMinMoveDistance()
	{
		return _COMPDATA(m_pCompData)->minMoveDistance;
	}

	void CharatorControllerComponent::SetNonWalkableMode(bool value) {
		mNonWalkableMode = value;
		if (m_pChactrl) {
			m_pChactrl->setNonWalkableMode(mNonWalkableMode ? PhysXControllerNonWalkableMode::ePREVENT_CLIMBING_AND_FORCE_SLIDING : PhysXControllerNonWalkableMode::ePREVENT_CLIMBING);
		}
	}

	bool CharatorControllerComponent::GetNonWalkableMode() {
		return mNonWalkableMode;
	}

	void CharatorControllerComponent::SetCenter(const Vector3 & center)
	{
		DVector3 oldPos = GetCCTWorldPosition();
		_COMPDATA(m_pCompData)->center = center;
		SetCCTWorldPosition(oldPos);
	}

	const Vector3 & CharatorControllerComponent::GetCenter()
	{
		return _COMPDATA(m_pCompData)->center;
	}

	void CharatorControllerComponent::SetCenterModel(uint32 model) {
		DVector3 oldPos = GetCCTWorldPosition();
		_COMPDATA(m_pCompData)->centerModel = model;
		SetCCTWorldPosition(oldPos);
	}
	uint32 CharatorControllerComponent::GetCenterModel() {
		return _COMPDATA(m_pCompData)->centerModel;
	}

	void CharatorControllerComponent::SetRadius(float radius)
	{
		radius = Math::Clamp(radius, 0.00001f, FLT_MAX);
		//DVector3 oldPos = GetCCTWorldPosition();
		_COMPDATA(m_pCompData)->radius = radius;
		//SetCCTWorldPosition(oldPos);
		ChangeGeometry();
	}

	float CharatorControllerComponent::GetRadius()
	{
		return _COMPDATA(m_pCompData)->radius;
	}

	void CharatorControllerComponent::SetMiddleHeight(float middleHeight) {

		middleHeight = Math::Clamp(middleHeight, 0.00001f, FLT_MAX);
		//DVector3 oldPos = GetCCTWorldPosition();
		_COMPDATA(m_pCompData)->middleHeight = middleHeight;
		//SetCCTWorldPosition(oldPos);
		ChangeGeometry();
	}
	float CharatorControllerComponent::GetMiddleHeight() {
		return _COMPDATA(m_pCompData)->middleHeight;
	}
	void CharatorControllerComponent::SetRealHeight(float height) {
		if (height < 0.f) return;
		float radius, middleHeight;
		GetRealSize(radius, middleHeight);
		float newMiddleHeight = std::max(height - 2.0f * radius, 0.0f);
		SetRealSize(radius, newMiddleHeight);
		AddToEditorUpdateQueue();
	}
	float CharatorControllerComponent::GetRealHeigth() {
		float radius, middleHeight;
		GetRealSize(radius, middleHeight);
		return radius * 2.0f + middleHeight;
	}

	void CharatorControllerComponent::ChangeIsTrigger() {
		if (m_pChactrl) {
			m_pChactrl->SetIsTrigger(GetIsTrigger());
		}
	}

	void CharatorControllerComponent::ChangeFilterGroup() {
		if (m_pChactrl) {
			ColliderComponent::ChangeFilterGroup();
			InvalidateCache();
		}
	}
	void CharatorControllerComponent::SetControllerCallBack(const EchoControllerCallBackPtr& pCallBack) {
		m_pCallBack = pCallBack;
	}
	const EchoControllerCallBackPtr& CharatorControllerComponent::GetControllerCallBack() {
		return m_pCallBack;
	}
	void CharatorControllerComponent::InvalidateCache() {
		if (m_pChactrl) {
			m_pChactrl->invalidateCache();
		}
	}
	void CharatorControllerComponent::SetIsCurrentController(bool value) {
		if (GetIsCurrentController()) {
			if (!value) {
				EchoPhysicsSystem::instance()->SetCurrentCharactorController(nullptr);
			}
		}
		else if (value) {
			EchoPhysicsSystem::instance()->SetCurrentCharactorController(this);
		}
	}
	bool CharatorControllerComponent::GetIsCurrentController() {
		return m_IsCurrentController;
	}
	void CharatorControllerComponent::SyncPhysXGlobalPose() {
		if (m_pChactrl) {
			m_pChactrl->SyncGlobalPose();
		}
	}
	void CharatorControllerComponent::ResetHelperColor() {
		if (IsExistPhysics()) {
			SetHelperColorType((HelperObjColorType)PhysXColor::gCharactorController);
		}
		else {
			SetHelperColorType((HelperObjColorType)PhysXColor::gNotExistPhysics);
		}
	}
	void CharatorControllerComponent::GenerateVBIBCB(std::vector<Vector3>& vbVec, std::vector<uint16>& ibVec) {
		vbVec.clear();
		ibVec.clear();
		
		float radius, middleHeight;
		GetRealSize(radius, middleHeight);

		std::vector<Vector3>& result = vbVec;
		result.resize(baseSize * 4);
		auto resultIter = result.begin();
		// y -
		resultIter = std::copy(unitBallPoints.begin(), unitBallPoints.begin() + baseSize / 2, resultIter);
		resultIter = std::copy(unitBallPoints.begin() + baseSize, unitBallPoints.begin() + baseSize * 2, resultIter);
		resultIter = std::copy(unitBallPoints.begin() + baseSize * 2, unitBallPoints.begin() + baseSize * 5 / 2, resultIter);
		// y + 
		resultIter = std::copy(unitBallPoints.begin() + baseSize / 2, unitBallPoints.begin() + baseSize, resultIter);
		resultIter = std::copy(unitBallPoints.begin() + baseSize, unitBallPoints.begin() + baseSize * 2, resultIter);
		resultIter = std::copy(unitBallPoints.begin() + baseSize * 5 / 2, unitBallPoints.begin() + baseSize * 3, resultIter);


		// 位置 和 缩放
		for (auto& ve3 : result)
			ve3 *= radius;

		float dis = middleHeight;
		for (size_t i = 0; i < result.size() / 2; ++i)
			result[i].y -= dis * 0.5f;
		for (size_t i = result.size() / 2; i < result.size(); ++i)
			result[i].y += dis * 0.5f;

		//Quaternion quat = GetPhysicsWorldRotation();

		//for (auto& ve3 : result) {
		//	ve3 *= quat;
		//	ve3 += center;
		//}
		ibVec = linkLine;
	}

	EchoGeometry* CharatorControllerComponent::GenerateGeometry(void* buffer) {
		return nullptr;
	}
	void CharatorControllerComponent::ChangeGeometry() {
		AddToEditorUpdateQueue();
		if (m_pChactrl == nullptr) return;
		DVector3 oldPos = GetCCTWorldPosition();
		float radius, middleHeight;
		GetRealSize(radius, middleHeight);
		PhysXControllerShapeCapsule capsuleShape;
		capsuleShape.radius = radius;
		capsuleShape.middleHeight = middleHeight;
		capsuleShape.climbingMode = 1;
		//只改变CCT的值，不改变实际物理值
		m_pChactrl->changeShape(capsuleShape, false);
		SetCCTWorldPosition(oldPos);
		GetPhysicsManager()->AddColliderToUpdateGeometry(this);
	}
	void CharatorControllerComponent::ChangeGeometryImpl() {
		if (!m_pChactrl) return;
		m_pChactrl->updateKinematicProxy();
	}
	void CharatorControllerComponent::SetCCTWorldPosition(const DVector3& position) {
		DVector3 phyPos = position;
		UseCenterModel(phyPos, true);
		SetPositionCCT(phyPos);
	}
	DVector3 CharatorControllerComponent::GetCCTWorldPosition() {
		DVector3 position = GetPositionCCT();
		UseCenterModel(position, false);
		return position;
	}

	void CharatorControllerComponent::SetPositionCCT(const DVector3& position) {
		if (m_pChactrl) {
			m_pChactrl->setPosition(position);
		}
		else {
			mCctPosition = position;
		}
		UpdateHelperPosition();
	}
	const DVector3& CharatorControllerComponent::GetPositionCCT() {
		if (m_pChactrl) {
			mCctPosition = m_pChactrl->getPosition();
		}
		return mCctPosition;
	}

	void CharatorControllerComponent::UseCenterModel(DVector3& position, bool toPhy) {
		float mult = toPhy ? 1.0f : -1.0f;
		position += mult * GetCenter() * getWorldRotation();
		switch (GetCenterModel()) {
			case CCT_Top: {
				position -= mult * CalCenterOffset(true);
				break;
			}
			case CCT_Bottom: {
				position += mult * CalCenterOffset(true);
				break;
			}
			default: {
				break;
			}
		}
	}

	const Vector3& CharatorControllerComponent::CalCenterOffset(bool bSkin) {
		float radius, middleHeight;
		if (m_pChactrl != nullptr) {
			PhysXControllerShapeCapsule capsuleShape;
			m_pChactrl->getControllerShape(capsuleShape);
			radius = capsuleShape.radius;
			middleHeight = capsuleShape.middleHeight;
		}
		else
		{
			GetRealSize(radius, middleHeight);
		}
		static Vector3 gTempOffset = Vector3::ZERO;
		gTempOffset = GetUpDirection() * (middleHeight * 0.5f + radius + (bSkin ? GetSkinWidth() : 0.0f));
		return gTempOffset;
	}

	void CharatorControllerComponent::ResetCCTWorldPosition() {
		if (!IsExistPhysics()) return;
		Move(0.0f, -GetUpDirection() * GetSkinWidth(), true);
		m_pChactrl->ClearHit();
	}

	Quaternion CharatorControllerComponent::GetPhysicsWorldRotation() {
		Vector3 upDirection = GetUpDirection();
		Quaternion rotation;
		rotation.FromAxesToAxes(Vector3::UP, upDirection);
		return rotation;
	}

	void CharatorControllerComponent::SetCompData(ColliderComponentData* _pData) {
		if (!_pData) return;
		SetCompDataBase(_pData);
		CharatorControllerComponentData* pData = static_cast<CharatorControllerComponentData*>(_pData);
		SetSlopeLimit(pData->slopeLimit);
		SetStepOffset(pData->stepOffset);
		SetSkinWidth(pData->skinWidth);
		SetMinMoveDistance(pData->minMoveDistance);
		SetCenter(pData->center);
		SetRadius(pData->radius);
		SetMiddleHeight(pData->middleHeight);
		SetCenterModel(pData->centerModel);
	}
	PhysXCharacterController* CharatorControllerComponent::GetPhysXCharacterController() {
		return m_pChactrl;
	}

	CharatorControllerComponent * EchoCharatorControllerScript::Script_GetCharatorControllerComponent(Actor * pActor, const std::string & name)
	{
		ProtocolComponentSystem* proComponentMgr = ProtocolComponentSystem::instance();
		ProtocolHandle comHandle = 0;
		ActorComTypeDesc desc;
		desc.actHandle = (Echo::ProtocolHandle)pActor;
		desc.comType = CharatorControllerComponent::mType;
		desc.comName = name;
		Echo::ProtocolHandle descHandle = (Echo::ProtocolHandle) &desc;
		Echo::ProtocolResult result = proComponentMgr->callProtocolFunc(PFID_Actor_GetComponent, &comHandle, descHandle);

		if (result != PR_OK)
			return nullptr;
		if (comHandle == 0)
			return nullptr;

		return (CharatorControllerComponent*)comHandle;
	}


	PhysicsActor* CharatorControllerComponent::_getActorPhysX() {
		return m_pChactrl;
	}

	DVector3 CharatorControllerComponent::getWorldPositionPhysX() {
		return GetWorldCenterPosition();
	}
	Quaternion CharatorControllerComponent::getWorldRotationPhysX() {
		return GetPhysicsWorldRotation();
	}
	DTransform CharatorControllerComponent::getWorldTransformPhysX() {
		return DTransform(getWorldPositionPhysX(), getWorldRotationPhysX());
	}


	void CharatorControllerComponent::UpdateHelperPosition() {
#ifdef USE_PHYSICS_HELPEROBJ
		if (m_pHelperObj) {
			m_pHelperObj->setPosition(GetPositionCCT());
		}
#endif // USE_PHYSICS_HELPEROBJ
	}
	void CharatorControllerComponent::UpdateHelperRotation() {
#ifdef USE_PHYSICS_HELPEROBJ
		if (m_pHelperObj) {
			m_pHelperObj->setOrientation(GetPhysicsWorldRotation());
		}
#endif // USE_PHYSICS_HELPEROBJ
	}

}; // namespace Echo


namespace Echo {

}

namespace Echo {
	extern void PhysXRaycastHitToEchoRaycastHit(PhysXRaycastHit& physXhit, EchoRaycastHit& hit, EchoSceneQueryFlags flags);
	extern bool EchoPhysXSceneQueryFilterCallBack(const EchoSceneQueryFilter& filter, PhysicsActor* actor, void* shapeUserData);

	//Raycast
	bool CharatorControllerComponent::Raycast(const DVector3& origin, const Vector3& direction, float maxDistance, PhysXGroup layerMask, const EchoSceneQueryFilter& filter) {
		if (!m_pChactrl) return false;
		PhysXRaycastHit physXhit;
		if (!m_pChactrl->GetTouchedShapesRelease()) {
			return m_pChactrl->Raycast(origin, direction, maxDistance, physXhit, layerMask, filter ? std::bind<bool>(EchoPhysXSceneQueryFilterCallBack, filter, std::placeholders::_1, std::placeholders::_2) : SceneQueryFunction(nullptr));
		}
		else {
			return GetPhysicsManager()->Raycast(origin, direction, maxDistance, layerMask, filter);
		}
	}
	bool CharatorControllerComponent::Raycast(const DVector3& origin, const Vector3& direction, EchoRaycastHit& hit, float maxDistance, PhysXGroup layerMask, const EchoSceneQueryFilter& filter) {
		if (!m_pChactrl) return false;
		if (!m_pChactrl->GetTouchedShapesRelease()) {
			PhysXRaycastHit physXhit;
			bool res = m_pChactrl->Raycast(origin, direction, maxDistance, physXhit, layerMask, filter ? std::bind<bool>(EchoPhysXSceneQueryFilterCallBack, filter, std::placeholders::_1, std::placeholders::_2) : SceneQueryFunction(nullptr));
			if (res) PhysXRaycastHitToEchoRaycastHit(physXhit, hit, EchoSceneQueryFlag::eSURFACE_TYPE);
			return res;
		}
		else {
			return GetPhysicsManager()->Raycast(origin, direction, hit, maxDistance, layerMask, filter);
		}
	}
	bool CharatorControllerComponent::Raycast(const DVector3& origin, const Vector3& direction, std::vector<EchoRaycastHit>& hits, float maxDistance, PhysXGroup layerMask, const EchoSceneQueryFilter& filter) {
		if (!m_pChactrl) return false;
		if (!m_pChactrl->GetTouchedShapesRelease()) {
			std::vector<PhysXRaycastHit> physXhits;
			bool res = m_pChactrl->Raycast(origin, direction, maxDistance, physXhits, layerMask, filter ? std::bind<bool>(EchoPhysXSceneQueryFilterCallBack, filter, std::placeholders::_1, std::placeholders::_2) : SceneQueryFunction(nullptr));
			if (res) {
				int size = (int)physXhits.size();
				hits.resize(size);
				for (int i = 0; i != size; ++i) {
					PhysXRaycastHitToEchoRaycastHit(physXhits[i], hits[i], EchoSceneQueryFlag::eSURFACE_TYPE);
				}
			}
			return res;
		}
		else {
			return GetPhysicsManager()->Raycast(origin, direction, hits, maxDistance, layerMask, filter);
		}
	}
	//Sweep
	bool CharatorControllerComponent::GeometryCast(const EchoGeometry* geometry, const DTransform& pose, const Vector3& direction, float maxDistance, PhysXGroup layerMask, const EchoSceneQueryFilter& filter) {
		if (!m_pChactrl) return false;
		if (!geometry) return false;
		if (!m_pChactrl->GetTouchedShapesRelease()) {
			PhysXRaycastHit physXhit;
			return m_pChactrl->Sweep(*geometry, pose, direction, maxDistance, physXhit, layerMask, filter ? std::bind<bool>(EchoPhysXSceneQueryFilterCallBack, filter, std::placeholders::_1, std::placeholders::_2) : SceneQueryFunction(nullptr));
		}
		else {
			return GetPhysicsManager()->GeometryCast(geometry, pose, direction, maxDistance, layerMask, filter);
		}
	}
	bool CharatorControllerComponent::GeometryCast(const EchoGeometry* geometry, const DTransform& pose, const Vector3& direction, EchoRaycastHit& hit, float maxDistance, PhysXGroup layerMask, const EchoSceneQueryFilter& filter) {
		if (!m_pChactrl) return false;
		if (!geometry) return false;
		if (!m_pChactrl->GetTouchedShapesRelease()) {
			PhysXRaycastHit physXhit;
			bool res = m_pChactrl->Sweep(*geometry, pose, direction, maxDistance, physXhit, layerMask, filter ? std::bind<bool>(EchoPhysXSceneQueryFilterCallBack, filter, std::placeholders::_1, std::placeholders::_2) : SceneQueryFunction(nullptr));
			if (res) PhysXRaycastHitToEchoRaycastHit(physXhit, hit, EchoSceneQueryFlag::eSURFACE_TYPE);
			return res;
		}
		else {
			return GetPhysicsManager()->GeometryCast(geometry, pose, direction, hit, maxDistance, layerMask, filter);
		}
	}
	bool CharatorControllerComponent::GeometryCast(const EchoGeometry* geometry, const DTransform& pose, const Vector3& direction, std::vector<EchoRaycastHit>& hits, float maxDistance, PhysXGroup layerMask, const EchoSceneQueryFilter& filter) {
		if (!m_pChactrl) return false;
		if (!geometry) return false;
		if (!m_pChactrl->GetTouchedShapesRelease()) {
			std::vector<PhysXRaycastHit> physXhits;
			bool res = m_pChactrl->Sweep(*geometry, pose, direction, maxDistance, physXhits, layerMask, filter ? std::bind<bool>(EchoPhysXSceneQueryFilterCallBack, filter, std::placeholders::_1, std::placeholders::_2) : SceneQueryFunction(nullptr));
			if (res) {
				int size = (int)physXhits.size();
				hits.resize(size);
				for (int i = 0; i != size; ++i) {
					PhysXRaycastHitToEchoRaycastHit(physXhits[i], hits[i], EchoSceneQueryFlag::eSURFACE_TYPE);
				}
			}
			return res;
		}
		else {
			return GetPhysicsManager()->GeometryCast(geometry, pose, direction, hits, maxDistance, layerMask, filter);
		}
	}
	//Overlap
	bool CharatorControllerComponent::OverlapGeometry(const EchoGeometry* geometry, const DTransform& pose, PhysXGroup layerMask, const EchoSceneQueryFilter& filter) {
		if (!m_pChactrl) return false;
		if (!geometry) return false;
		if (!m_pChactrl->GetTouchedShapesRelease()) {
			PhysXSceneQueryHit physXHit;
			return m_pChactrl->Overlap(*geometry, pose, physXHit, layerMask, filter ? std::bind<bool>(EchoPhysXSceneQueryFilterCallBack, filter, std::placeholders::_1, std::placeholders::_2) : SceneQueryFunction(nullptr));
		}
		else {
			return GetPhysicsManager()->OverlapGeometry(geometry, pose, layerMask, filter);
		}
	}
	bool CharatorControllerComponent::OverlapGeometry(const EchoGeometry* geometry, const DTransform& pose, EchoSceneQueryHit& hit, PhysXGroup layerMask, const EchoSceneQueryFilter& filter) {
		if (!m_pChactrl) return false;
		if (!geometry) return false;
		if (!m_pChactrl->GetTouchedShapesRelease()) {
			PhysXSceneQueryHit physXHit;
			bool res = m_pChactrl->Overlap(*geometry, pose, physXHit, layerMask, filter ? std::bind<bool>(EchoPhysXSceneQueryFilterCallBack, filter, std::placeholders::_1, std::placeholders::_2) : SceneQueryFunction(nullptr));
			if (res) EchoPhysicsManager::PhysXPtrToEchoPtr(physXHit.actor, physXHit.shape, hit.rigidbody, hit.collider);
			return res;
		}
		else {
			return GetPhysicsManager()->OverlapGeometry(geometry, pose, hit, layerMask, filter);
		}
	}
	bool CharatorControllerComponent::OverlapGeometry(const EchoGeometry* geometry, const DTransform& pose, std::vector<EchoSceneQueryHit>& hits, PhysXGroup layerMask, const EchoSceneQueryFilter& filter) {
		if (!m_pChactrl) return false;
		if (!geometry) return false;
		if (!m_pChactrl->GetTouchedShapesRelease()) {
			std::vector<PhysXSceneQueryHit> physXhits;
			bool res = m_pChactrl->Overlap(*geometry, pose, physXhits, layerMask, filter ? std::bind<bool>(EchoPhysXSceneQueryFilterCallBack, filter, std::placeholders::_1, std::placeholders::_2) : SceneQueryFunction(nullptr));
			if (res) {
				int size = (int)physXhits.size();
				hits.resize(size);
				for (int i = 0; i != size; ++i) {
					EchoPhysicsManager::PhysXPtrToEchoPtr(physXhits[i].actor, physXhits[i].shape, hits[i].rigidbody, hits[i].collider);
				}
			}
			return res;
		}
		else {
			return GetPhysicsManager()->OverlapGeometry(geometry, pose, hits, layerMask, filter);
		}
	}


}

namespace Echo {

	class PhysXMathCctMoveTask : public PhysXTask {
	public:
		PhysXMathCctMoveTask(PhysXMathCharacterController* cct) : pCct(cct) {}
		virtual ~PhysXMathCctMoveTask() {}
		virtual void TaskExecute() override {
			mResult = pCct->move(mDist, mDtime, mMinMoveDis, mGroup);
		}
		virtual void TaskFinish() override {
			if (mCallback) mCallback();
		}
	public:
		float mMinMoveDis = 0.0f;
		float mDtime = 0.0f;
		Vector3 mDist = Vector3::ZERO;
		PhysXGroupEnum mGroup = ePhysXGroup_All;
		uint32 mResult = 0;
		PhysXMathCharacterController* pCct = nullptr;
		EchoMathCctMoveTask::FinishCallback mCallback = nullptr;
	};

	EchoMathCctMoveTask::EchoMathCctMoveTask(PhysXMathCharacterController* pCharl) : m_pCharl(pCharl) {
		m_pPrivate = new PhysXMathCctMoveTask(m_pCharl);
	}
	EchoMathCctMoveTask::~EchoMathCctMoveTask() {
		SAFE_DELETE(m_pPrivate);
	}
	
	void EchoMathCctMoveTask::Move(const Vector3& dist, float elapsedTime, float minDist, PhysXGroupEnum canInteractGroup) {
		m_pPrivate->mDist = dist;
		m_pPrivate->mDtime = elapsedTime;
		m_pPrivate->mMinMoveDis = minDist;
		m_pPrivate->mGroup = canInteractGroup;
		m_pPrivate->PrepareTask();
	}
	uint32 EchoMathCctMoveTask::GetResult() {
		return m_pPrivate->mResult;
	}
	void EchoMathCctMoveTask::Wait() {
		if (m_pPrivate->IsPrepare()) {
			m_pPrivate->SynchroTask();
		}
	}
	bool EchoMathCctMoveTask::IsMoving() const {
		return m_pPrivate->IsPrepare();
	}
	void EchoMathCctMoveTask::SetFinishCallback(const FinishCallback& callback) {
		m_pPrivate->mCallback = callback;
	}
}

namespace Echo {

	class MathCharacterControllerTask : public PhysXTask {
	public:
		MathCharacterControllerTask(MathCharacterController* cct) : pCct(cct) {}
		virtual ~MathCharacterControllerTask() {}
		virtual void TaskExecute() override {
			pCct->Move_Async(mDtime, mDist);
		}
		virtual void TaskFinish() override {
			pCct->Move_Sync();
			if (mCallback) {
				mCallback(pCct);
			}
		}
	public:
		float mDtime = 0.0f;
		Vector3 mDist = Vector3::ZERO;
		MathCharacterController* pCct = nullptr;
		std::function<void(MathCharacterController*)> mCallback = nullptr;
	};
	void MathCharacterController::SetFinishCallback(const std::function<void(MathCharacterController*)>& callback) {
		m_pTask->mCallback = callback;
	}
	bool MathCharacterController::IsPrepareMove() {
		return m_pTask->IsPrepare();
	}
	void MathCharacterController::PrepareMove(float dtime, const Vector3& dist) {
		if (IsPrepareMove())
			m_pTask->CancelTask();
		m_pTask->mDist = dist;
		m_pTask->mDtime = dtime;
		m_pTask->PrepareTask();
	}
	uint32 MathCharacterController::SynchroMove() {
		if (IsPrepareMove())
			m_pTask->SynchroTask();
		return mResult;
	}

	uint32 MathCharacterController::Move(float dtime, const Vector3& dist)
	{
		if (m_pChactrl == nullptr) {
			return 0;
		}
		Move_Async(dtime, dist);
		Move_Sync();
		return mResult;
	}

	void MathCharacterController::Move_Async(float dtime, const Vector3& dist) {
		mResult = m_pChactrl->move(dist, dtime, mMinMoveDistance, mCanInteractFilterGroupType);
	}
	void MathCharacterController::Move_Sync() {
		m_pChactrl->UpdateHit();
		if (m_pHelperObj) {
			m_pHelperObj->setPosition(m_pChactrl->getPosition());
		}
	}

	MathCharacterController::MathCharacterController(EchoPhysicsManager* pMgr) : m_pManager(pMgr) {
		m_pTask = new MathCharacterControllerTask(this);
		CreatePhysics();
		mWorldRotation = GetPhysicsWorldRotation();
		m_pManager->AddMathCharacterController(this);
	}
	MathCharacterController::~MathCharacterController() {
		SAFE_DELETE(m_pTask);
		ReleasePhysics();
		ReleaseHelper();
		m_pManager->RemoveMathCharacterController(this);
	}

	void MathCharacterController::SetRealSize(float radius, float middleHeight) {
		const float kMinScale = 0.00001F;
		const float kMinSize = 0.00001F;
		if (radius < 0 || middleHeight < 0) return;
		Vector3 scale = getWorldScale();

		float sh = std::max(Abs(scale.y), kMinScale);
		float newMiddleHeight = std::max(middleHeight / sh, kMinSize);
		float sr = std::max(std::max(Abs(scale.x), Abs(scale.z)), kMinScale);
		float newRadius = std::max(radius / sr, kMinScale);

		SetMiddleHeight(newMiddleHeight);
		SetRadius(newRadius);
	}
	void MathCharacterController::GetRealSize(float& _radius, float& _middleHeight) {
		const float kMinScale = 0.00001F;
		const float kMinSize = 0.00001F;

		float r = GetRadius();
		float mh = GetMiddleHeight();
		if (r < 0 || mh < 0) return;
		Vector3 scale = getWorldScale();

		float sh = std::max(Abs(scale.y), kMinScale);
		float middleHeight = std::max(mh * sh, kMinSize);
		float sr = std::max(std::max(Abs(scale.x), Abs(scale.z)), kMinScale);
		float radius = std::max(r * sr, kMinScale);

		_middleHeight = middleHeight;
		_radius = radius;
	}


	void MathCharacterController::CreatePhysics() {
		if (m_pChactrl != nullptr)
			return;

		float radius, middleHeight;
		GetRealSize(radius, middleHeight);
		PhysXControllerShapeCapsule capsuleShape;
		capsuleShape.radius = radius;
		capsuleShape.middleHeight = middleHeight;
		capsuleShape.climbingMode = 1;
		PhysXControllerDesc desc(capsuleShape);
		desc.position = DVector3::ZERO;
		desc.slopeLimit = std::max(0.f, std::cos(mSlopeLimit / 180 * ECHO_PI));
		desc.stepOffset = mStepOffset;
		desc.contactOffset = mSkinWidth;
		desc.userData = this;
		desc.userType = (int)PhysXUserTypeEnum::Undefined;
		desc.scaleCoeff = m_pManager->GetControllerShapeScale();
		desc.nonWalkableMode = mNonWalkableMode ? PhysXControllerNonWalkableMode::ePREVENT_CLIMBING_AND_FORCE_SLIDING : PhysXControllerNonWalkableMode::ePREVENT_CLIMBING;

		m_pChactrl = m_pManager->GetScene()->AddMathCharacterController(desc);
		if (!m_pChactrl) return;
		m_pChactrl->SetControllerCallBack(m_pManager->mPhysXMathCCTCallBack);
		m_pChactrl->setUpDirection(mUpDirection);
		m_pChactrl->setCacheShapeHit(true);
	}
	void MathCharacterController::ReleasePhysics() {
		if (m_pChactrl == nullptr)
			return;
		m_pManager->GetScene()->RemoveMathCharacterController(m_pChactrl);
		m_pChactrl = nullptr;
	}

	DVector3 MathCharacterController::GetWorldCenterPosition() {
		return m_pChactrl->getPosition();
	}
	DVector3 MathCharacterController::GetWorldFootPosition(bool IsSkin) {
		return GetWorldCenterPosition() - DVector3(CalCenterOffset(IsSkin));
	}
	DVector3 MathCharacterController::GetWorldHeadPosition(bool IsSkin) {
		return GetWorldCenterPosition() + DVector3(CalCenterOffset(IsSkin));
	}
	void MathCharacterController::SetWorldCenterPosition(const DVector3& position) {
		m_pChactrl->setPosition(position);
		if (m_pHelperObj) {
			m_pHelperObj->setPosition(position);
		}
	}
	void MathCharacterController::SetWorldFootPosition(const DVector3& position, bool IsSkin) {
		SetWorldCenterPosition(position + DVector3(CalCenterOffset(IsSkin)));
	}
	void MathCharacterController::SetWorldHeadPosition(const DVector3& position, bool IsSkin) {
		SetWorldCenterPosition(position - DVector3(CalCenterOffset(IsSkin)));
	}
	const Vector3& MathCharacterController::GetUpDirection() {
		return mUpDirection;
	}
	void MathCharacterController::SetUpDirection(const Vector3& up) {
		DVector3 oldPos = GetCCTWorldPosition();
		mUpDirection = up.GetNormalized();
		m_pChactrl->setUpDirection(mUpDirection);
		SetCCTWorldPosition(oldPos);
		mWorldRotation = GetPhysicsWorldRotation();
		if (m_pHelperObj) {
			m_pHelperObj->setOrientation(mWorldRotation);
		}
	}

	void MathCharacterController::SetSlopeLimit(float slopLimit)
	{
		slopLimit = Math::Clamp(slopLimit, 0.0f, 90.0f);
		mSlopeLimit = slopLimit;
		m_pChactrl->setSlopeLimit(std::max(0.f, std::cos(mSlopeLimit / 180 * ECHO_PI)));
	}

	float MathCharacterController::GetSlopeLimit()
	{
		return mSlopeLimit;
	}

	void MathCharacterController::SetStepOffset(float stepOffset)
	{
		stepOffset = Math::Clamp(stepOffset, 0.0001f, GetRadius() * 2.f + GetMiddleHeight());//GetRealHeight();
		mStepOffset = stepOffset;
		m_pChactrl->setStepOffset(mStepOffset);
	}

	float MathCharacterController::GetStepOffset()
	{
		return mStepOffset;
	}

	void MathCharacterController::SetSkinWidth(float skinWidth)
	{
		skinWidth = Math::Clamp(skinWidth, 1e-6f, FLT_MAX);
		DVector3 oldPos = GetCCTWorldPosition();
		mSkinWidth = skinWidth;
		m_pChactrl->setContactOffset(skinWidth);
		SetCCTWorldPosition(oldPos);
	}

	float MathCharacterController::GetSkinWidth()
	{
		return mSkinWidth;
	}

	void MathCharacterController::SetMinMoveDistance(float minMoveDistance)
	{
		minMoveDistance = Math::Clamp(minMoveDistance, 0.0f, FLT_MAX);
		mMinMoveDistance = minMoveDistance;
	}

	float MathCharacterController::GetMinMoveDistance()
	{
		return mMinMoveDistance;
	}

	void MathCharacterController::SetNonWalkableMode(bool value) {
		mNonWalkableMode = value;
		m_pChactrl->setNonWalkableMode(mNonWalkableMode ? PhysXControllerNonWalkableMode::ePREVENT_CLIMBING_AND_FORCE_SLIDING : PhysXControllerNonWalkableMode::ePREVENT_CLIMBING);
	}

	bool MathCharacterController::GetNonWalkableMode() {
		return mNonWalkableMode;
	}

	void MathCharacterController::SetCenter(const Vector3& center)
	{
		DVector3 oldPos = GetCCTWorldPosition();
		mCenter = center;
		SetCCTWorldPosition(oldPos);
	}

	const Vector3& MathCharacterController::GetCenter()
	{
		return mCenter;
	}

	void MathCharacterController::SetCenterModel(uint32 model) {
		DVector3 oldPos = GetCCTWorldPosition();
		mCenterModel = model;
		SetCCTWorldPosition(oldPos);
	}
	uint32 MathCharacterController::GetCenterModel() {
		return mCenterModel;
	}

	void MathCharacterController::SetRadius(float radius)
	{
		radius = Math::Clamp(radius, 0.00001f, FLT_MAX);
		DVector3 oldPos = GetCCTWorldPosition();
		mRadius = radius;
		ChangeGeometry();
		SetCCTWorldPosition(oldPos);
	}

	float MathCharacterController::GetRadius()
	{
		return mRadius;
	}

	void MathCharacterController::SetMiddleHeight(float middleHeight) {

		middleHeight = Math::Clamp(middleHeight, 0.00001f, FLT_MAX);
		DVector3 oldPos = GetCCTWorldPosition();
		mMiddleHeight = middleHeight;
		ChangeGeometry();
		SetCCTWorldPosition(oldPos);
	}
	float MathCharacterController::GetMiddleHeight() {
		return mMiddleHeight;
	}
	void MathCharacterController::SetCanInteractFilterGroupType(PhysXGroup value) {
		mCanInteractFilterGroupType = value;
	}
	PhysXGroup MathCharacterController::GetCanInteractFilterGroupType() {
		return mCanInteractFilterGroupType;
	}
	void MathCharacterController::SetRealHeight(float height) {
		if (height < 0.f) return;
		float radius, middleHeight;
		GetRealSize(radius, middleHeight);
		float newMiddleHeight = std::max(height - 2.0f * radius, 0.0f);
		SetRealSize(radius, newMiddleHeight);
	}
	float MathCharacterController::GetRealHeigth() {
		float radius, middleHeight;
		GetRealSize(radius, middleHeight);
		return radius * 2.0f + middleHeight;
	}

	void MathCharacterController::SetControllerCallBack(EchoMathControllerCallBack* pCallBack) {
		m_pCallBack = pCallBack;
	}
	EchoMathControllerCallBack* MathCharacterController::GetControllerCallBack() const {
		return m_pCallBack;
	}
	void MathCharacterController::InvalidateCache() {
		if (m_pChactrl) {
			m_pChactrl->invalidateCache();
		}
	}
	void MathCharacterController::SetCCTWorldPosition(const DVector3& position) {
		DVector3 phyPos = position;
		UseCenterModel(phyPos, true);
		SetWorldCenterPosition(phyPos);
	}
	DVector3 MathCharacterController::GetCCTWorldPosition() {
		DVector3 position;
		position = m_pChactrl->getPosition();
		UseCenterModel(position, false);
		return position;
	}

	void MathCharacterController::UseCenterModel(DVector3& position, bool toPhy) {
		float mult = toPhy ? 1.0f : -1.0f;
		position += mult * GetCenter() * getWorldRotation();
		switch (GetCenterModel()) {
		case CCT_Top: {
			position -= mult * CalCenterOffset(true);
			break;
		}
		case CCT_Bottom: {
			position += mult * CalCenterOffset(true);
			break;
		}
		default: {
			break;
		}
		}
	}

	const Vector3& MathCharacterController::CalCenterOffset(bool bSkin) {
		static Vector3 gTempOffset = Vector3::ZERO;
		float radius, middleHeight;
		GetRealSize(radius, middleHeight);
		gTempOffset = GetUpDirection() * (middleHeight * 0.5f + radius + (bSkin ? GetSkinWidth() : 0.0f));
		return gTempOffset;
	}

	Quaternion MathCharacterController::GetPhysicsWorldRotation() {
		Vector3 upDirection = GetUpDirection();
		Quaternion rotation;
		rotation.FromAxesToAxes(Vector3::UP, upDirection);
		return rotation;
	}

	void MathCharacterController::ChangeGeometry() {
		float radius, middleHeight;
		GetRealSize(radius, middleHeight);
		PhysXControllerShapeCapsule capsuleShape;
		capsuleShape.radius = radius;
		capsuleShape.middleHeight = middleHeight;
		capsuleShape.climbingMode = 1;
		m_pChactrl->changeShape(capsuleShape);
	}

	PhysXMathCharacterController* MathCharacterController::GetPhysXMathCharacterController() {
		return m_pChactrl;
	}
	
	void MathCharacterController::CreateHelper() {
		if (m_pHelperObj) return;
		m_pHelperObj = new HelperObject(Name::g_cEmptyName, HelperObjType_Box, false, nullptr, m_pManager->GetSceneManager());
		m_pHelperObj->setVisible(m_IsHelperVisible);
		m_pHelperObj->setRenderType(ECHO_DRAW::ECHO_LINE_LIST);
		m_pHelperObj->setOrientation(Quaternion::IDENTITY);
		m_pHelperObj->setScale(Vector3::UNIT_SCALE);
		m_pHelperObj->setHelperColorType((HelperObjColorType)PhysXColor::gMathCharactorController);
		m_pHelperObj->setPosition(GetWorldCenterPosition());
		m_pHelperObj->setOrientation(mWorldRotation);
	}
	void MathCharacterController::ReleaseHelper() {
		if (m_pHelperObj == NULL) return;
		delete m_pHelperObj;
		m_pHelperObj = NULL;
	}
	void MathCharacterController::UpdateHelper() {
		if (!m_pHelperObj || !m_pHelperObj->getVisible()) {
			return;
		}
		std::vector<Vector3> vbVec;
		std::vector<uint16> ibVec;
		GenerateVBIBCB(vbVec, ibVec);
		if (vbVec.empty() || ibVec.empty()) {
			m_pHelperObj->clearBuffer();
		}
		m_pHelperObj->changeCustomPolygon(vbVec, ibVec);
	}
	void MathCharacterController::GenerateVBIBCB(std::vector<Vector3>& vbVec, std::vector<uint16>& ibVec) {
	
		float radius, middleHeight;
		GetRealSize(radius, middleHeight);
		//DVector3 dcenter = DVector3::ZERO;
		//UseCenterModel(dcenter, true);
		//Vector3 center = dcenter;
		//Vector3 center = Vector3::ZERO;

		std::vector<Vector3>& result = vbVec;
		result.resize(baseSize * 4);
		auto resultIter = result.begin();
		// y -
		resultIter = std::copy(unitBallPoints.begin(), unitBallPoints.begin() + baseSize / 2, resultIter);
		resultIter = std::copy(unitBallPoints.begin() + baseSize, unitBallPoints.begin() + baseSize * 2, resultIter);
		resultIter = std::copy(unitBallPoints.begin() + baseSize * 2, unitBallPoints.begin() + baseSize * 5 / 2, resultIter);
		// y + 
		resultIter = std::copy(unitBallPoints.begin() + baseSize / 2, unitBallPoints.begin() + baseSize, resultIter);
		resultIter = std::copy(unitBallPoints.begin() + baseSize, unitBallPoints.begin() + baseSize * 2, resultIter);
		resultIter = std::copy(unitBallPoints.begin() + baseSize * 5 / 2, unitBallPoints.begin() + baseSize * 3, resultIter);

		for (auto& ve3 : result)
			ve3 *= radius;

		float dis = middleHeight;
		for (size_t i = 0; i < result.size() / 2; ++i)
			result[i].y -= dis * 0.5f;
		for (size_t i = result.size() / 2; i < result.size(); ++i)
			result[i].y += dis * 0.5f;

		ibVec = linkLine;
	}
	void MathCharacterController::SetHelperVisible(bool value) {
		m_IsHelperVisible = value;
		if (m_IsHelperVisible) CreateHelper();
		if (!m_pHelperObj) return;
#ifdef PHYSICS_RUNTIME
		if (!m_IsHelperVisible) {
			ReleaseHelper();
			return;
		}
#else
		m_pHelperObj->setVisible(value);
#endif
		UpdateHelper();
	}
	bool MathCharacterController::GetHelperVisible() {
		return m_IsHelperVisible;
	}
	void MathCharacterController::ResetHelperColor() {
		if (!m_pHelperObj) {
			return;
		}
		m_pHelperObj->setHelperColorType((HelperObjColorType)PhysXColor::gMathCharactorController);
	}
	void MathCharacterController::SetCompData(CharatorControllerComponentData* pData) {
		if (pData == nullptr) return;
		SetCanInteractFilterGroupType(pData->canInteractFilterGroupType);
		SetCenter(pData->center);
		SetCenterModel(pData->centerModel);
		SetMiddleHeight(pData->middleHeight);
		SetMinMoveDistance(pData->minMoveDistance);
		SetRadius(pData->radius);
		SetSkinWidth(pData->skinWidth);
		SetSlopeLimit(pData->slopeLimit);
		SetStepOffset(pData->stepOffset);
	}

	//Raycast
	bool MathCharacterController::Raycast(const DVector3& origin, const Vector3& direction, float maxDistance, PhysXGroup layerMask, const EchoSceneQueryFilter& filter) {
		if (!m_pChactrl) return false;
		PhysXRaycastHit physXhit;
		if (!m_pChactrl->GetTouchedShapesRelease()) {
			return m_pChactrl->Raycast(origin, direction, maxDistance, physXhit, layerMask, filter ? std::bind<bool>(EchoPhysXSceneQueryFilterCallBack, filter, std::placeholders::_1, std::placeholders::_2) : SceneQueryFunction(nullptr));
		}
		else {
			return GetPhysicsManager()->Raycast(origin, direction, maxDistance, layerMask, filter);
		}
	}
	bool MathCharacterController::Raycast(const DVector3& origin, const Vector3& direction, EchoRaycastHit& hit, float maxDistance, PhysXGroup layerMask, const EchoSceneQueryFilter& filter) {
		if (!m_pChactrl) return false;
		if (!m_pChactrl->GetTouchedShapesRelease()) {
			PhysXRaycastHit physXhit;
			bool res = m_pChactrl->Raycast(origin, direction, maxDistance, physXhit, layerMask, filter ? std::bind<bool>(EchoPhysXSceneQueryFilterCallBack, filter, std::placeholders::_1, std::placeholders::_2) : SceneQueryFunction(nullptr));
			if (res) PhysXRaycastHitToEchoRaycastHit(physXhit, hit, EchoSceneQueryFlag::eSURFACE_TYPE);
			return res;
		}
		else {
			return GetPhysicsManager()->Raycast(origin, direction, hit, maxDistance, layerMask, filter);
		}
	}
	bool MathCharacterController::Raycast(const DVector3& origin, const Vector3& direction, std::vector<EchoRaycastHit>& hits, float maxDistance, PhysXGroup layerMask, const EchoSceneQueryFilter& filter) {
		if (!m_pChactrl) return false;
		if (!m_pChactrl->GetTouchedShapesRelease()) {
			std::vector<PhysXRaycastHit> physXhits;
			bool res = m_pChactrl->Raycast(origin, direction, maxDistance, physXhits, layerMask, filter ? std::bind<bool>(EchoPhysXSceneQueryFilterCallBack, filter, std::placeholders::_1, std::placeholders::_2) : SceneQueryFunction(nullptr));
			if (res) {
				int size = (int)physXhits.size();
				hits.resize(size);
				for (int i = 0; i != size; ++i) {
					PhysXRaycastHitToEchoRaycastHit(physXhits[i], hits[i], EchoSceneQueryFlag::eSURFACE_TYPE);
				}
			}
			return res;
		}
		else {
			return GetPhysicsManager()->Raycast(origin, direction, hits, maxDistance, layerMask, filter);
		}
	}
	//Sweep
	bool MathCharacterController::GeometryCast(const EchoGeometry* geometry, const DTransform& pose, const Vector3& direction, float maxDistance, PhysXGroup layerMask, const EchoSceneQueryFilter& filter) {
		if (!m_pChactrl) return false;
		if (!geometry) return false;
		if (!m_pChactrl->GetTouchedShapesRelease()) {
			PhysXRaycastHit physXhit;
			return m_pChactrl->Sweep(*geometry, pose, direction, maxDistance, physXhit, layerMask, filter ? std::bind<bool>(EchoPhysXSceneQueryFilterCallBack, filter, std::placeholders::_1, std::placeholders::_2) : SceneQueryFunction(nullptr));
		}
		else {
			return GetPhysicsManager()->GeometryCast(geometry, pose, direction, maxDistance, layerMask, filter);
		}
	}
	bool MathCharacterController::GeometryCast(const EchoGeometry* geometry, const DTransform& pose, const Vector3& direction, EchoRaycastHit& hit, float maxDistance, PhysXGroup layerMask, const EchoSceneQueryFilter& filter) {
		if (!m_pChactrl) return false;
		if (!geometry) return false;
		if (!m_pChactrl->GetTouchedShapesRelease()) {
			PhysXRaycastHit physXhit;
			bool res = m_pChactrl->Sweep(*geometry, pose, direction, maxDistance, physXhit, layerMask, filter ? std::bind<bool>(EchoPhysXSceneQueryFilterCallBack, filter, std::placeholders::_1, std::placeholders::_2) : SceneQueryFunction(nullptr));
			if (res) PhysXRaycastHitToEchoRaycastHit(physXhit, hit, EchoSceneQueryFlag::eSURFACE_TYPE);
			return res;
		}
		else {
			return GetPhysicsManager()->GeometryCast(geometry, pose, direction, hit, maxDistance, layerMask, filter);
		}
	}
	bool MathCharacterController::GeometryCast(const EchoGeometry* geometry, const DTransform& pose, const Vector3& direction, std::vector<EchoRaycastHit>& hits, float maxDistance, PhysXGroup layerMask, const EchoSceneQueryFilter& filter) {
		if (!m_pChactrl) return false;
		if (!geometry) return false;
		if (!m_pChactrl->GetTouchedShapesRelease()) {
			std::vector<PhysXRaycastHit> physXhits;
			bool res = m_pChactrl->Sweep(*geometry, pose, direction, maxDistance, physXhits, layerMask, filter ? std::bind<bool>(EchoPhysXSceneQueryFilterCallBack, filter, std::placeholders::_1, std::placeholders::_2) : SceneQueryFunction(nullptr));
			if (res) {
				int size = (int)physXhits.size();
				hits.resize(size);
				for (int i = 0; i != size; ++i) {
					PhysXRaycastHitToEchoRaycastHit(physXhits[i], hits[i], EchoSceneQueryFlag::eSURFACE_TYPE);
				}
			}
			return res;
		}
		else {
			return GetPhysicsManager()->GeometryCast(geometry, pose, direction, hits, maxDistance, layerMask, filter);
		}
	}
	//Overlap
	bool MathCharacterController::OverlapGeometry(const EchoGeometry* geometry, const DTransform& pose, PhysXGroup layerMask, const EchoSceneQueryFilter& filter) {
		if (!m_pChactrl) return false;
		if (!geometry) return false;
		if (!m_pChactrl->GetTouchedShapesRelease()) {
			PhysXSceneQueryHit physXHit;
			return m_pChactrl->Overlap(*geometry, pose, physXHit, layerMask, filter ? std::bind<bool>(EchoPhysXSceneQueryFilterCallBack, filter, std::placeholders::_1, std::placeholders::_2) : SceneQueryFunction(nullptr));
		}
		else {
			return GetPhysicsManager()->OverlapGeometry(geometry, pose, layerMask, filter);
		}
	}
	bool MathCharacterController::OverlapGeometry(const EchoGeometry* geometry, const DTransform& pose, EchoSceneQueryHit& hit, PhysXGroup layerMask, const EchoSceneQueryFilter& filter) {
		if (!m_pChactrl) return false;
		if (!geometry) return false;
		if (!m_pChactrl->GetTouchedShapesRelease()) {
			PhysXSceneQueryHit physXHit;
			bool res = m_pChactrl->Overlap(*geometry, pose, physXHit, layerMask, filter ? std::bind<bool>(EchoPhysXSceneQueryFilterCallBack, filter, std::placeholders::_1, std::placeholders::_2) : SceneQueryFunction(nullptr));
			if (res) EchoPhysicsManager::PhysXPtrToEchoPtr(physXHit.actor, physXHit.shape, hit.rigidbody, hit.collider);
			return res;
		}
		else {
			return GetPhysicsManager()->OverlapGeometry(geometry, pose, hit, layerMask, filter);
		}
	}
	bool MathCharacterController::OverlapGeometry(const EchoGeometry* geometry, const DTransform& pose, std::vector<EchoSceneQueryHit>& hits, PhysXGroup layerMask, const EchoSceneQueryFilter& filter) {
		if (!m_pChactrl) return false;
		if (!geometry) return false;
		if (!m_pChactrl->GetTouchedShapesRelease()) {
			std::vector<PhysXSceneQueryHit> physXhits;
			bool res = m_pChactrl->Overlap(*geometry, pose, physXhits, layerMask, filter ? std::bind<bool>(EchoPhysXSceneQueryFilterCallBack, filter, std::placeholders::_1, std::placeholders::_2) : SceneQueryFunction(nullptr));
			if (res) {
				int size = (int)physXhits.size();
				hits.resize(size);
				for (int i = 0; i != size; ++i) {
					EchoPhysicsManager::PhysXPtrToEchoPtr(physXhits[i].actor, physXhits[i].shape, hits[i].rigidbody, hits[i].collider);
				}
			}
			return res;
		}
		else {
			return GetPhysicsManager()->OverlapGeometry(geometry, pose, hits, layerMask, filter);
		}
	}
}
