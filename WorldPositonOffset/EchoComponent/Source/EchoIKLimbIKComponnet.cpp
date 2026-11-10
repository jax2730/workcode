#include "EchoComponentStableHeaders.h"
#include "EchoIKLimbIKComponnet.h"
#include "EchoIKTargetPointComponent.h"
#include "IK/EchoIK.h"

namespace Echo {

	static const std::vector<Echo::Name>& getDefaultBoneName()
	{
		static std::vector<Name> res;
		static bool init = false;
		if (init) return res;

		res.push_back(Name{ "" });
		res.push_back(Name{ "" });
		res.push_back(Name{ "" });

		init = true;
		return res;
	}

}

namespace Echo
{
	DECLAREACTORCOMTYPE_CPP(IKLimbIKComponent)
		EDITOR_REFLECT_BEGIN(IKLimbIKComponentInfo)
		EDITOR_REFLECT_MEMBER_UPDATE(m_FixTransform,"FixTransform",EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_IKPositionWeight, "PositionWeight", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_IKRotationWeight, "RotationWeight", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_MaintainRotationWeight, "MaintainRotationWeight", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_BendModifier, "BendModifier", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_BendModifierWeight, "BendModifierWeight", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_TargetName, "TargetNam", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_BendGoalName, "BendGoalName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_BoneName, "BoneName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_END();

	_DEFINEINFO_VIRFUN(IKLimbIKComponentInfo, IKBaseComponentInfo)

#define IKDATA(ptr) static_cast<IKLimbIKComponentData*>(ptr)
#define IKCOMDATA(ptr) static_cast<EchoIK::LimbIK*>(ptr)

	IKLimbIKComponent::IKLimbIKComponent(Echo::ActorPtr pActor, Echo::ActorCOMMoveableType movableType)
		:IKBaseComponent(pActor,movableType)
	{
		m_pCompData = new IKLimbIKComponentData();
	}

	IKLimbIKComponent::~IKLimbIKComponent(){}

	bool IKLimbIKComponent::IsCanCreate() {
		RoleIKComponent* pRoleIK = GetRoleIKComponent();
		if (!pRoleIK) return false;
		auto& bones = GetBoneName();
		if (bones.empty()) return false;
		for (auto& iter : bones) {
			if (iter == Name::g_cEmptyName || pRoleIK->getBoneNodeByName(iter) == nullptr) {
				return false;
			}
		}
		return true;
	}

	const Echo::LimbIKInfo& IKLimbIKComponent::GetIKInfoData()
	{
		return IKDATA(m_pCompData)->mIKInfo;
	}

	bool IKLimbIKComponent::GetFixTransform()
	{
		return IKDATA(m_pCompData)->mIKInfo.fixTransforms;
	}

	float IKLimbIKComponent::GetIKPositionWeight()
	{
		return IKDATA(m_pCompData)->mIKInfo.IKPositionWeight;
	}

	float IKLimbIKComponent::GetIKRotationWeight()
	{
		return IKDATA(m_pCompData)->mIKInfo.IKRotationWeight;
	}

	float IKLimbIKComponent::GetMaintainRotationWeight()
	{
		return IKDATA(m_pCompData)->mIKInfo.maintainRotationWeight;
	}

	int IKLimbIKComponent::GetBendModifier()
	{
		return IKDATA(m_pCompData)->mIKInfo.bendModifier;
	}

	float IKLimbIKComponent::GetBendModifierWeight()
	{
		return IKDATA(m_pCompData)->mIKInfo.bendModifierWeight;
	}

	const String& IKLimbIKComponent::GetTargetName()
	{
		return IKDATA(m_pCompData)->TargetName;
	}
	const String& IKLimbIKComponent::GetBendGoalName() {
		return IKDATA(m_pCompData)->BendGoalName;
	}

	std::vector<Echo::Name>& IKLimbIKComponent::GetBoneName()
	{
		return IKDATA(m_pCompData)->mIKInfo.m_vtBoneName;
	}

	void IKLimbIKComponent::SetFixTransform(bool value)
	{
		IKDATA(m_pCompData)->mIKInfo.fixTransforms = value;

		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->fixTransforms = value;
	}

	void IKLimbIKComponent::SetIKPositionWeight(float value)
	{
		IKDATA(m_pCompData)->mIKInfo.IKPositionWeight = value;

		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->GetIKSolver()->IKPositionWeight = value;
	}

	void IKLimbIKComponent::SetIKRotationWeight(float value)
	{
		IKDATA(m_pCompData)->mIKInfo.IKRotationWeight = value;

		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->GetIKSolver()->IKRotationWeight = value;
	}

	/*void IKLimbIKComponent::SetBendNormal(const Vector3& value)
	{
		IKDATA(m_pCompData)->mIKInfo.bendNormal = value;

		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->GetIKSolver()->bendNormal = value;
	}

	void IKLimbIKComponent::SetGoal(int value)
	{
		IKDATA(m_pCompData)->mIKInfo.goal = value;

		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->GetIKSolver()->goal = EchoIK::AvatarIKGoal(value);
	}*/

	void IKLimbIKComponent::SetMaintainRotationWeight(float value)
	{
		IKDATA(m_pCompData)->mIKInfo.maintainRotationWeight = value;

		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->GetIKSolver()->maintainRotationWeight = value;
	}

	void IKLimbIKComponent::SetBendModifier(int value)
	{
		IKDATA(m_pCompData)->mIKInfo.bendModifier = value;

		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->GetIKSolver()->bendModifier = EchoIK::IKSolverLimb::BendModifier(value);
	}

	void IKLimbIKComponent::SetBendModifierWeight(float value)
	{
		IKDATA(m_pCompData)->mIKInfo.bendModifierWeight = value;

		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->GetIKSolver()->bendModifierWeight = value;
	}

	void IKLimbIKComponent::SetTargetName(const String& value)
	{
		IKDATA(m_pCompData)->TargetName = value;
		ActorComponentPtr compPtr = FindComponent(value);
		SetLimbTarget(compPtr);
	}

	void IKLimbIKComponent::SetBendGoalName(const String& value) {
		IKDATA(m_pCompData)->BendGoalName = value;
		ActorComponentPtr compPtr = FindComponent(value);
		SetLimbBendGoal(compPtr);
	}

	void IKLimbIKComponent::SetBoneName(const std::vector<Name>& value)
	{
		if (IsExistIK()) return;
		IKDATA(m_pCompData)->mIKInfo.m_vtBoneName = value;
	}

	void IKLimbIKComponent::SetLimbTarget(const ActorComponentPtr& pComp) {
		if (!IsExistIK()) {
			mTargetPtr = pComp;
		}
		else {
			ResetLimbTarget();
			mTargetPtr = pComp;
			ApplyTargetPtr();
		}
	}
	void IKLimbIKComponent::ResetLimbTarget() {
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->resetLimbTarget();
		if (mTargetPtr == nullptr) return;
		if (mTargetPtr->getType() == IKTargetPointComponent::mType) {
			IKTargetPointComponent* pIKTargetPointComp = (IKTargetPointComponent*)mTargetPtr.getPointer();
			pIKTargetPointComp->SetSkpeCallBack(nullptr, nullptr);
		}
	}
	void IKLimbIKComponent::SetLimbBendGoal(const ActorComponentPtr& pComp) {
		if (!IsExistIK()) {
			mBendGoalPtr = pComp;
		}
		else {
			ResetLimbBendGoal();
			mBendGoalPtr = pComp;
			ApplyBendGoalPtr();
		}

	}

	void IKLimbIKComponent::ResetLimbBendGoal() {
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->resetLimbBendGoal();
	}

	void IKLimbIKComponent::SetSkeletonPosture(const std::map<Name, Quaternion>& boneMap) {
		if (!IsExistIK()) return;
		if (boneMap.empty()) {
			IKCOMDATA(m_pIKComponent)->resetSkeletonPosture();
			return;
		}
		IKCOMDATA(m_pIKComponent)->setSkeletonPosture(boneMap);

	}
	void IKLimbIKComponent::ResetSkeletonPosture() {
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->resetSkeletonPosture();
	}

	void IKLimbIKComponent::PostCreateIK()
	{
		ApplyTargetPtr();
		ApplyBendGoalPtr();
	}

	void IKLimbIKComponent::PreDestroyIK() {
		ResetLimbTarget();
		ResetLimbBendGoal();
	}

	void IKLimbIKComponent::ApplyTargetPtr() {
		if (mTargetPtr == nullptr) return;
		IKCOMDATA(m_pIKComponent)->setLimbTarget(mTargetPtr, getTagPointNameByTargetComponent(mTargetPtr));
		if (mTargetPtr->getType() == IKTargetPointComponent::mType) {
			IKTargetPointComponent* pIKTargetPointComp = (IKTargetPointComponent*)mTargetPtr.getPointer();
			auto OnSetSkpe = [this](IKTargetPointComponent* pComp) { this->SetSkeletonPosture(pComp->GetSkeletonPosture()); };
			auto OnResetSkpe = [this](IKTargetPointComponent* pComp) { this->ResetSkeletonPosture(); };
			pIKTargetPointComp->SetSkpeCallBack(OnSetSkpe, OnResetSkpe);
		}
	}
	void IKLimbIKComponent::ApplyBendGoalPtr() {
		if (mBendGoalPtr == nullptr) return;
		IKCOMDATA(m_pIKComponent)->setLimbBendGoal(mBendGoalPtr, getTagPointNameByTargetComponent(mBendGoalPtr));
	}

}

namespace Echo {

	void IKLimbIKComponent::SetCompData(IKBaseComponentData* pData)
	{
		if (pData == nullptr) return;
		IKBaseComponent::SetCompData(pData);
		SetTargetName(IKDATA(pData)->TargetName);
		SetBendGoalName(IKDATA(pData)->BendGoalName);
	}

	void IKLimbIKComponent::SetIKInfo(FinalIKInfoBase* pInfo)
	{
		if (pInfo == nullptr) return;
		SetIKInfoData(*((LimbIKInfo*)pInfo));
	}

	void IKLimbIKComponent::SetIKInfoData(const LimbIKInfo& info)
	{
		SetFixTransform(info.fixTransforms);
		SetIKPositionWeight(info.IKPositionWeight);
		SetIKRotationWeight(info.IKRotationWeight);
		SetMaintainRotationWeight(info.maintainRotationWeight);
		SetBendModifier(info.bendModifier);
		SetBendModifierWeight(info.bendModifierWeight);
		SetBoneName(info.m_vtBoneName);
	}

	void IKLimbIKComponent::UseDefaultInitialize()
	{
		SetFixTransform(true);
		SetIKPositionWeight(1.f);
		SetIKRotationWeight(1.f);
		SetMaintainRotationWeight(0.f);
		SetBendModifier(EchoIK::IKSolverLimb::BendModifier::Animation);
		SetBendModifierWeight(1.f);
		SetBoneName(getDefaultBoneName());

		SetTargetName("");
		SetBendGoalName("");
	}

	void IKLimbIKComponentInfo::CopyToCompData(IKBaseComponentData* pData)
	{
		if (!pData) return;
		IKBaseComponentInfo::CopyToCompData(pData);
		IKDATA(pData)->mIKInfo.fixTransforms = GetFixTransform();
		IKDATA(pData)->mIKInfo.IKPositionWeight = GetIKPositionWeight();
		IKDATA(pData)->mIKInfo.IKRotationWeight = GetIKRotationWeight();
		IKDATA(pData)->mIKInfo.maintainRotationWeight = GetMaintainRotationWeight();
		IKDATA(pData)->mIKInfo.bendModifier = GetBendModifier();
		IKDATA(pData)->mIKInfo.bendModifierWeight = GetBendModifierWeight();
		IKDATA(pData)->mIKInfo.m_vtBoneName = GetBoneName();
		
		IKDATA(pData)->TargetName = GetTargetName();
		IKDATA(pData)->BendGoalName = GetBendGoalName();
	}

	void IKLimbIKComponentInfo::CompDataToInfo(IKBaseComponentData* pData, IKBaseComponent* pComp)
	{
		if (!pData) return;
		IKBaseComponentInfo::CompDataToInfo(pData, pComp);
		auto _pComp = static_cast<IKLimbIKComponent*>(pComp);

		SetFixTransform(IKDATA(pData)->mIKInfo.fixTransforms, _pComp);
		SetIKPositionWeight(IKDATA(pData)->mIKInfo.IKPositionWeight, _pComp);
		SetIKRotationWeight(IKDATA(pData)->mIKInfo.IKRotationWeight, _pComp);
		SetMaintainRotationWeight(IKDATA(pData)->mIKInfo.maintainRotationWeight, _pComp);
		SetBendModifier(IKDATA(pData)->mIKInfo.bendModifier, _pComp);
		SetBendModifierWeight(IKDATA(pData)->mIKInfo.bendModifierWeight, _pComp);
		SetBoneName(IKDATA(pData)->mIKInfo.m_vtBoneName, _pComp);
		
		SetTargetName(IKDATA(pData)->TargetName, _pComp);
		SetBendGoalName(IKDATA(pData)->BendGoalName, _pComp);
	}

}
