#include "EchoComponentStableHeaders.h"
#include "EchoIKCCDIKComponent.h"
#include "EchoIKTargetPointComponent.h"
#include "IK/EchoIKSolverCCDIK.h"
#include "IK/EchoIKCCDIK.h"

namespace Echo
{
	DECLAREACTORCOMTYPE_CPP(IKCCDIKComponent)
		EDITOR_REFLECT_BEGIN(IKCCDIKComponentInfo)
		EDITOR_REFLECT_MEMBER_UPDATE(m_FixTransform, "FixTransform", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_TargetName, "TragetName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_PositionWeight, "PositionWeight", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_RotationWeight, "RotationWeight", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_Tolerance, "Tolerance", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_MaxIteration, "MaxIteration", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_UseRotationLimits, "UseRotationLimmits", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_XY, "XY", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_Bones, "Bones", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_END();
	_DEFINEINFO_VIRFUN(IKCCDIKComponentInfo, IKBaseComponentInfo)


#define IKDATA(ptr) static_cast<IKCCDIKComponentData*>(ptr)
#define IKCOMDATA(ptr) static_cast<EchoIK::IKCCDIK*>(ptr)
		;
	IKCCDIKComponentInfo::IKCCDIKComponentInfo(const Name& comType, ActorCOMMoveableType moveable) 
		: IKBaseComponentInfo(comType, moveable){}

	IKCCDIKComponentInfo::IKCCDIKComponentInfo(){}

	IKCCDIKComponentInfo::~IKCCDIKComponentInfo(){}


	IKCCDIKComponent::IKCCDIKComponent(Echo::ActorPtr pActor, Echo::ActorCOMMoveableType movableType)
		:IKBaseComponent(pActor, movableType)
	{
		m_pCompData = new IKCCDIKComponentData();
	}

	IKCCDIKComponent::~IKCCDIKComponent(){}
	
	void Echo::IKCCDIKComponent::SetIKInfo(FinalIKInfoBase* pInfo)
	{
		if (pInfo == nullptr) return;
		SetIKInfoData(*((IKCCDIKInfo*)pInfo));
	}

	bool IKCCDIKComponent::IsCanCreate() {
		RoleIKComponent* pRoleIK = GetRoleIKComponent();
		if (!pRoleIK) return false;
		auto& bones = GetBones();
		if (bones.empty()) return false;
		for (auto& iter : bones) {
			if (iter.transform == Name::g_cEmptyName || pRoleIK->getBoneNodeByName(iter.transform) == nullptr) {
				return false;
			}
		}
		return true;
	}
	
	const Echo::IKCCDIKInfo& Echo::IKCCDIKComponent::GetIKInfoData()
	{
		return IKDATA(m_pCompData)->mIKInfo;
	}

	bool IKCCDIKComponent::GetFixTransform()
	{
		return IKDATA(m_pCompData)->mIKInfo.fixTransform;
	}

	const String& IKCCDIKComponent::GetTargetName()
	{
		return IKDATA(m_pCompData)->TargetName;
	}

	float IKCCDIKComponent::GetPositionWeight()
	{
		return IKDATA(m_pCompData)->mIKInfo.positionWeight;
	}
	float IKCCDIKComponent::GetRotationWeight() {
		return IKDATA(m_pCompData)->mIKInfo.rotationWeight;
	}

	float IKCCDIKComponent::GetTolerance()
	{
		return IKDATA(m_pCompData)->mIKInfo.tolerance;
	}

	int IKCCDIKComponent::GetMaxIteration()
	{
		return IKDATA(m_pCompData)->mIKInfo.maxIteration;
	}

	bool IKCCDIKComponent::GetUseRotationLimits()
	{
		return IKDATA(m_pCompData)->mIKInfo.useRotationLimits;
	}

	bool IKCCDIKComponent::GetXY()
	{
		return IKDATA(m_pCompData)->mIKInfo.check2d;
	}

	std::vector<Echo::NodeWeightInfo>& IKCCDIKComponent::GetBones()
	{
		return IKDATA(m_pCompData)->mIKInfo.bones;
	}

	void IKCCDIKComponent::SetFixTransform(bool value)
	{
		IKDATA(m_pCompData)->mIKInfo.fixTransform = value;
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->fixTransforms = value;
	}

	void IKCCDIKComponent::SetTargetName(const String& value)
	{
		IKDATA(m_pCompData)->TargetName = value;
		ActorComponentPtr compPtr = FindComponent(value);
		SetCCDTarget(compPtr);
	}

	void IKCCDIKComponent::SetPositionWeight(float value)
	{
		if (EchoIsNanOrInf(value)) return;
		IKDATA(m_pCompData)->mIKInfo.positionWeight = value;
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->GetIKSolver()->SetIKPositionWeight(value);
	}

	void IKCCDIKComponent::SetRotationWeight(float value) {
		if (EchoIsNanOrInf(value)) return;
		IKDATA(m_pCompData)->mIKInfo.rotationWeight = value;
		if (!IsExistIK()) return;
		((EchoIK::IKSolverCCDIK*)IKCOMDATA(m_pIKComponent)->GetIKSolver())->IKRotationWeight = value;
	}

	void IKCCDIKComponent::SetTolerance(float value)
	{
		if (EchoIsNanOrInf(value)) return;
		IKDATA(m_pCompData)->mIKInfo.tolerance = value;
		if (!IsExistIK()) return;
		static_cast<EchoIK::IKSolverCCDIK*>(IKCOMDATA(m_pIKComponent)->GetIKSolver())->tolerance = value;
	}

	void IKCCDIKComponent::SetMaxIteration(int value)
	{
		IKDATA(m_pCompData)->mIKInfo.maxIteration = value;
		if (!IsExistIK()) return;
		static_cast<EchoIK::IKSolverCCDIK*>(IKCOMDATA(m_pIKComponent)->GetIKSolver())->maxIterations = value;
	}

	void IKCCDIKComponent::SetUseRotationLimits(bool value)
	{
		IKDATA(m_pCompData)->mIKInfo.useRotationLimits = value;
		if (!IsExistIK()) return;
		static_cast<EchoIK::IKSolverCCDIK*>(IKCOMDATA(m_pIKComponent)->GetIKSolver())->useRotationLimits = value;
	}

	void IKCCDIKComponent::SetXY(bool value)
	{
		IKDATA(m_pCompData)->mIKInfo.check2d = value;
		if (!IsExistIK()) return;
		static_cast<EchoIK::IKSolverCCDIK*>(IKCOMDATA(m_pIKComponent)->GetIKSolver())->XY = value;
	}

	void IKCCDIKComponent::SetBones(const std::vector<NodeWeightInfo>& value)
	{
		if (!IsExistIK()) {
			IKDATA(m_pCompData)->mIKInfo.bones = value;
		}
		else if (IKDATA(m_pCompData)->mIKInfo.bones.size() == value.size()) {
			std::vector<NodeWeightInfo>& _bones = IKDATA(m_pCompData)->mIKInfo.bones;
			size_t size = _bones.size();
			for (int i = 0; i != size; ++i) {
				if (_bones[i].transform != value[i].transform) {
					return;
				}
			}
			_bones = value;
			std::vector<EchoIK::IKSolver::Bone*>& _ikbones = IKCOMDATA(m_pIKComponent)->GetIKSolver()->bones;
			for (int i = 0; i != size; ++i) {
				if (_ikbones[i]) _ikbones[i]->weight = _bones[i].weight;
			}
		}
	}

	void IKCCDIKComponent::SetCCDTarget(const ActorComponentPtr& pComp) {
		if (!IsExistIK()) {
			mTargetPtr = pComp;
		}
		else {
			ResetCCDTarget();
			mTargetPtr = pComp;
			ApplyTargetPtr();
		}
	}

	void IKCCDIKComponent::ResetCCDTarget() {
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->resetCCDTarget();
		if (mTargetPtr == nullptr) return;
		if (mTargetPtr->getType() == IKTargetPointComponent::mType) {
			IKTargetPointComponent* pIKTargetPointComp = (IKTargetPointComponent*)mTargetPtr.getPointer();
			pIKTargetPointComp->SetSkpeCallBack(nullptr, nullptr);
		}
	}
	void IKCCDIKComponent::SetSkeletonPosture(const std::map<Name, Quaternion>& boneMap) {
		if (!IsExistIK()) return;
		if (boneMap.empty()) {
			IKCOMDATA(m_pIKComponent)->resetSkeletonPosture();
			return;
		}
		IKCOMDATA(m_pIKComponent)->setSkeletonPosture(boneMap);

	}
	void IKCCDIKComponent::ResetSkeletonPosture() {
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->resetSkeletonPosture();
	}

	void IKCCDIKComponent::PostCreateIK() {
		ApplyTargetPtr();
	}

	void IKCCDIKComponent::PreDestroyIK() {
		ResetCCDTarget();
	}

	void IKCCDIKComponent::ApplyTargetPtr() {
		if (mTargetPtr == nullptr) return;
		IKCOMDATA(m_pIKComponent)->setCCDTarget(mTargetPtr, getTagPointNameByTargetComponent(mTargetPtr));
		if (mTargetPtr->getType() == IKTargetPointComponent::mType) {
			IKTargetPointComponent* pIKTargetPointComp = (IKTargetPointComponent*)mTargetPtr.getPointer();
			auto OnSetSkpe = [this](IKTargetPointComponent* pComp) { this->SetSkeletonPosture(pComp->GetSkeletonPosture()); };
			auto OnResetSkpe = [this](IKTargetPointComponent* pComp) { this->ResetSkeletonPosture(); };
			pIKTargetPointComp->SetSkpeCallBack(OnSetSkpe, OnResetSkpe);
		}
	}
}

namespace Echo {

	void Echo::IKCCDIKComponent::SetCompData(IKBaseComponentData* pData)
	{
		if (pData == nullptr) return;
		IKBaseComponent::SetCompData(pData);
		SetTargetName(IKDATA(pData)->TargetName);
	}

	void Echo::IKCCDIKComponent::SetIKInfoData(const IKCCDIKInfo& info)
	{
		SetFixTransform(info.fixTransform);
		SetPositionWeight(info.positionWeight);
		SetRotationWeight(info.rotationWeight);
		SetTolerance(info.tolerance);
		SetMaxIteration(info.maxIteration);
		SetUseRotationLimits(info.useRotationLimits);
		SetXY(info.check2d);
		SetBones(info.bones);
	}
	void Echo::IKCCDIKComponent::UseDefaultInitialize()
	{
		SetFixTransform(false);
		SetTargetName(String(""));
		SetPositionWeight(1.f);
		SetTolerance(0.f);
		SetMaxIteration(4);
		SetUseRotationLimits(true);
		SetXY(false);
		SetBones({});

	}


	void IKCCDIKComponentInfo::CopyToCompData(IKBaseComponentData* pData)
	{
		if (pData == nullptr) return;
		IKBaseComponentInfo::CopyToCompData(pData);

		IKDATA(pData)->TargetName = GetTargetName();

		IKDATA(pData)->mIKInfo.fixTransform = GetFixTransform();
		IKDATA(pData)->mIKInfo.positionWeight = GetPositionWeight();
		IKDATA(pData)->mIKInfo.positionWeight = GetRotationWeight();
		IKDATA(pData)->mIKInfo.tolerance = GetTolerance();
		IKDATA(pData)->mIKInfo.maxIteration = GetMaxIteration();
		IKDATA(pData)->mIKInfo.useRotationLimits = GetUseRotationLimits();
		IKDATA(pData)->mIKInfo.check2d = GetXY();
		IKDATA(pData)->mIKInfo.bones = GetBones();
	}

	void IKCCDIKComponentInfo::CompDataToInfo(IKBaseComponentData* pData, IKBaseComponent* pComp)
	{
		if (pData == nullptr) return;
		IKBaseComponentInfo::CompDataToInfo(pData, pComp);
		IKCCDIKComponent* _pComp = static_cast<IKCCDIKComponent*>(pComp);

		SetTargetName(IKDATA(pData)->TargetName, _pComp);

		SetFixTransform(IKDATA(pData)->mIKInfo.fixTransform, _pComp);
		SetPositionWeight(IKDATA(pData)->mIKInfo.positionWeight, _pComp);
		SetRotationWeight(IKDATA(pData)->mIKInfo.rotationWeight, _pComp);
		SetTolerance(IKDATA(pData)->mIKInfo.tolerance, _pComp);
		SetMaxIteration(IKDATA(pData)->mIKInfo.maxIteration, _pComp);
		SetUseRotationLimits(IKDATA(pData)->mIKInfo.useRotationLimits, _pComp);
		SetXY(IKDATA(pData)->mIKInfo.check2d, _pComp);
		SetBones(IKDATA(pData)->mIKInfo.bones, _pComp);
	}

}
