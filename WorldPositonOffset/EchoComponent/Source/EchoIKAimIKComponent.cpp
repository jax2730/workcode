#include "EchoComponentStableHeaders.h"
#include "EchoIKAimIKComponent.h"
#include "Effect/EchoEffectHolder.h"
#include "EchoActorProFuncImp.h"
#include "IK/EchoAimIK.h"
#include "EchoIKPointComponent.h"
#include "EchoIKTargetPointComponent.h"
#include "IK/EchoRotationLimitAngle.h"
#include "IK/EchoIKSolverAim.h"
#include "IK/EchoIKSolver.h"

namespace Echo {

	DECLAREACTORCOMTYPE_CPP(IKAimIKComponent)

#define IKDATA(_pData) static_cast<IKAimIKComponentData*>(_pData)
#define IKCOMDATA(_pData) static_cast<EchoIK::AimIK*>(_pData)


		EDITOR_REFLECT_BEGIN(IKAimIKComponentInfo)
		EDITOR_REFLECT_MEMBER_UPDATE(m_FixTransforms, "FixTransforms", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_Weight, "Weight", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_PoleWeight, "PoleWeight", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_Tolerance, "Tolerance", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_MaxIterations, "MaxIterations", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_ClampWeight, "ClampWeight", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_ClampSmoothing, "ClampSmoothing", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_UseRotationLimits, "UseRotationLimits", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_XY, "XY", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_Axis, "Axis", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_PoleAxis, "PoleAxis", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_Bones, "Bones", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_TargetNodeCompName, "TargetNodeCompName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_AimNodeCompName, "AimNodeCompName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_PoleTargetNodeName, "PoleTargetNodeName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_END();
	_DEFINEINFO_VIRFUN(IKAimIKComponentInfo, IKBaseComponentInfo)


	
	void IKAimIKComponent::SetIKInfo(FinalIKInfoBase* pInfo) {
		if (!pInfo) return;
		SetIKInfoData(*(AimIKInfo*)pInfo);
	}

	IKAimIKComponent::IKAimIKComponent(ActorPtr pActor, ActorCOMMoveableType moveable) : IKBaseComponent(pActor, moveable) {
		m_pCompData = new IKAimIKComponentData();
	}
	IKAimIKComponent::~IKAimIKComponent() {}
	const AimIKInfo& IKAimIKComponent::GetIKInfoData() {
		return IKDATA(m_pCompData)->mIKInfo;
	}
	
	bool IKAimIKComponent::IsCanCreate() {
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
	void IKAimIKComponent::SetFixTransforms(bool value) { 
		IKDATA(m_pCompData)->mIKInfo.fixTransforms = value; 
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->fixTransforms = value;
	}
	bool IKAimIKComponent::GetFixTransforms() { return IKDATA(m_pCompData)->mIKInfo.fixTransforms; }
	void IKAimIKComponent::SetWeight(float value){
		if (EchoIsNanOrInf(value)) return;
		IKDATA(m_pCompData)->mIKInfo.weight = value; 
		if (!IsExistIK()) return;
		auto pIKSolverAim = IKCOMDATA(m_pIKComponent)->GetIKSolver();
		if (!pIKSolverAim) return;
		pIKSolverAim->SetIKPositionWeight(value);
	}
	float IKAimIKComponent::GetWeight(){ return IKDATA(m_pCompData)->mIKInfo.weight; }
	void IKAimIKComponent::SetPoleWeight(float value){
		if (EchoIsNanOrInf(value)) return;
		IKDATA(m_pCompData)->mIKInfo.poleWeight = value; 
		if (!IsExistIK()) return;
		auto pIKSolverAim = IKCOMDATA(m_pIKComponent)->GetIKSolver();
		if (!pIKSolverAim) return;
		pIKSolverAim->poleWeight = value;
	}
	float IKAimIKComponent::GetPoleWeight(){ return IKDATA(m_pCompData)->mIKInfo.poleWeight; }
	void IKAimIKComponent::SetTolerance(float value){
		if (EchoIsNanOrInf(value)) return;
		IKDATA(m_pCompData)->mIKInfo.tolerance = value; 
		if (!IsExistIK()) return;
		auto pIKSolverAim = IKCOMDATA(m_pIKComponent)->GetIKSolver();
		if (!pIKSolverAim) return;
		pIKSolverAim->tolerance = value;
	}
	float IKAimIKComponent::GetTolerance(){ return IKDATA(m_pCompData)->mIKInfo.tolerance; }
	void IKAimIKComponent::SetMaxIterations(int value){ 
		IKDATA(m_pCompData)->mIKInfo.maxIterations = value; 
		if (!IsExistIK()) return;
		auto pIKSolverAim = IKCOMDATA(m_pIKComponent)->GetIKSolver();
		if (!pIKSolverAim) return;
		pIKSolverAim->maxIterations = value;
	}
	int IKAimIKComponent::GetMaxIterations(){ return IKDATA(m_pCompData)->mIKInfo.maxIterations; }
	void IKAimIKComponent::SetClampWeight(float value){ 
		IKDATA(m_pCompData)->mIKInfo.clampWeight = value; 
		if (!IsExistIK()) return;
		auto pIKSolverAim = IKCOMDATA(m_pIKComponent)->GetIKSolver();
		if (!pIKSolverAim) return;
		pIKSolverAim->clampWeight = value;
	}
	float IKAimIKComponent::GetClampWeight(){ return IKDATA(m_pCompData)->mIKInfo.clampWeight; }
	void IKAimIKComponent::SetClampSmoothing(int value){ 
		IKDATA(m_pCompData)->mIKInfo.clampSmoothing = value;
		if (!IsExistIK()) return;
		auto pIKSolverAim = IKCOMDATA(m_pIKComponent)->GetIKSolver();
		if (!pIKSolverAim) return;
		pIKSolverAim->clampSmoothing = value;
	}
	int IKAimIKComponent::GetClampSmoothing(){ return IKDATA(m_pCompData)->mIKInfo.clampSmoothing; }
	void IKAimIKComponent::SetUseRotationLimits(bool value){ 
		IKDATA(m_pCompData)->mIKInfo.useRotationLimits = value; 
		if (!IsExistIK()) return;
		auto pIKSolverAim = IKCOMDATA(m_pIKComponent)->GetIKSolver();
		if (!pIKSolverAim) return;
		pIKSolverAim->useRotationLimits = value;
	}
	bool IKAimIKComponent::GetUseRotationLimits(){ return IKDATA(m_pCompData)->mIKInfo.useRotationLimits; }
	void IKAimIKComponent::SetXY(bool value){ 
		IKDATA(m_pCompData)->mIKInfo.XY = value; 
		if (!IsExistIK()) return;
		auto pIKSolverAim = IKCOMDATA(m_pIKComponent)->GetIKSolver();
		if (!pIKSolverAim) return;
		pIKSolverAim->XY = value;
	}
	bool IKAimIKComponent::GetXY(){ return IKDATA(m_pCompData)->mIKInfo.XY; }
	void IKAimIKComponent::SetAxis(const Vector3& value){ 
		IKDATA(m_pCompData)->mIKInfo.axis = value;
		if (!IsExistIK()) return;
		auto pIKSolverAim = IKCOMDATA(m_pIKComponent)->GetIKSolver();
		if (!pIKSolverAim) return;
		pIKSolverAim->axis = value;
	}
	const Vector3& IKAimIKComponent::GetAxis(){ return IKDATA(m_pCompData)->mIKInfo.axis; }
	void IKAimIKComponent::SetPoleAxis(const Vector3& value){ 
		IKDATA(m_pCompData)->mIKInfo.poleAxis = value; 
		if (!IsExistIK()) return;
		auto pIKSolverAim = IKCOMDATA(m_pIKComponent)->GetIKSolver();
		if (!pIKSolverAim) return;
		pIKSolverAim->poleAxis = value;
	}
	const Vector3& IKAimIKComponent::GetPoleAxis(){ return IKDATA(m_pCompData)->mIKInfo.poleAxis; }
	
	void IKAimIKComponent::SetTargetNodeCompName(const String& value) {
		IKDATA(m_pCompData)->TargetNodeCompName = value;
		ActorComponentPtr compPtr = FindComponent(value);
		SetTargetNode(compPtr);
	}
	const String& IKAimIKComponent::GetTargetNodeCompName() {
		return IKDATA(m_pCompData)->TargetNodeCompName;
	}
	void IKAimIKComponent::SetAimNodeCompName(const String& value) {
		IKDATA(m_pCompData)->AimNodeCompName = value;
		ActorComponentPtr compPtr = FindComponent(value);
		SetAimNode(compPtr);
	}
	const String& IKAimIKComponent::GetAimNodeCompName() {
		return IKDATA(m_pCompData)->AimNodeCompName;
	}
	void IKAimIKComponent::SetPoleTargetNodeName(const String& value) {
		IKDATA(m_pCompData)->PoleTargetNodeName = value;
		ActorComponentPtr compPtr = FindComponent(value);
		SetPoleTargetNode(compPtr);
	}
	const String& IKAimIKComponent::GetPoleTargetNodeName() {
		return IKDATA(m_pCompData)->PoleTargetNodeName;
	}

	void IKAimIKComponent::SetBones(const std::vector<NodeWeightInfo>& value) {
		if (!IsExistIK()) {
			IKDATA(m_pCompData)->mIKInfo.bones = value;
		}
		else {
			if (IKDATA(m_pCompData)->mIKInfo.bones.size() != value.size()) return;
			for (int i = 0; i != value.size(); ++i) {
				if (IKDATA(m_pCompData)->mIKInfo.bones[i].transform != value[i].transform) return;
			}
			auto pIKSolverAim = IKCOMDATA(m_pIKComponent)->GetIKSolver();
			if (pIKSolverAim->bones.size() != value.size()) return;
			for (int i = 0; i != value.size(); ++i) {
				IKDATA(m_pCompData)->mIKInfo.bones[i].weight = value[i].weight;
				if (pIKSolverAim) pIKSolverAim->bones[i]->weight = value[i].weight;
			}
		}
	}
	const std::vector<NodeWeightInfo>& IKAimIKComponent::GetBones(){ 
		return IKDATA(m_pCompData)->mIKInfo.bones; 
	}

	void IKAimIKComponent::SetTargetNode(WorldNode* pNode) {
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->setTargetNode(pNode);
	}
	void IKAimIKComponent::SetAimNode(WorldNode* pNode) {
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->setAimNode(pNode);
	}
	void IKAimIKComponent::SetPoleTargetNode(WorldNode* pNode) {
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->setPoleTargetNode(pNode);
	}
	void IKAimIKComponent::PostCreateIK() {
		ApplyTargetPtr();
		ApplyAimPtr();
		ApplyPoleTargetPtr();
	}

	void IKAimIKComponent::SetTargetNode(const ActorComponentPtr& pComp) {
		mTargetPtr = pComp;
		if (IsExistIK()) {
			ApplyTargetPtr();
		}
	}
	void IKAimIKComponent::SetAimNode(const ActorComponentPtr& pComp) {
		mAimPtr = pComp;
		if (IsExistIK()) {
			ApplyAimPtr();
		}
	}
	void IKAimIKComponent::SetPoleTargetNode(const ActorComponentPtr& pComp) {
		mPoleTargetPtr = pComp;
		if (IsExistIK()) {
			ApplyPoleTargetPtr();
		}
	}

	void IKAimIKComponent::ApplyTargetPtr() {
		if (mTargetPtr == nullptr) return;
		IKCOMDATA(m_pIKComponent)->setTargetNode(mTargetPtr, getTagPointNameByTargetComponent(mTargetPtr));
	}
	void IKAimIKComponent::ApplyAimPtr() {
		if (mAimPtr == nullptr) return;
		IKCOMDATA(m_pIKComponent)->setAimNode(mAimPtr, getTagPointNameByTargetComponent(mAimPtr));
	}
	void IKAimIKComponent::ApplyPoleTargetPtr() {
		if (mPoleTargetPtr == nullptr) return;
		IKCOMDATA(m_pIKComponent)->setPoleTargetNode(mPoleTargetPtr, getTagPointNameByTargetComponent(mPoleTargetPtr));
	}
}

namespace Echo {

	void IKAimIKComponent::SetCompData(IKBaseComponentData* pData) {
		if (!pData) return;
		IKBaseComponent::SetCompData(pData);
		SetTargetNodeCompName(IKDATA(pData)->TargetNodeCompName);
		SetAimNodeCompName(IKDATA(pData)->AimNodeCompName);
		SetPoleTargetNodeName(IKDATA(pData)->PoleTargetNodeName);
	}

	void IKAimIKComponent::SetIKInfoData(const AimIKInfo& info) {
		SetFixTransforms(info.fixTransforms);
		SetWeight(info.weight);
		SetPoleWeight(info.poleWeight);
		SetTolerance(info.tolerance);
		SetMaxIterations(info.maxIterations);
		SetClampWeight(info.clampWeight);
		SetClampSmoothing(info.clampSmoothing);
		SetUseRotationLimits(info.useRotationLimits);
		SetXY(info.XY);
		SetAxis(info.axis);
		SetPoleAxis(info.poleAxis);
		SetBones(info.bones);
	}
	void IKAimIKComponent::UseDefaultInitialize() {

		SetTargetNodeCompName("");
		SetAimNodeCompName("");
		SetPoleTargetNodeName("");

		SetFixTransforms(false);
		SetAxis({ 0,0,1 });
		SetPoleAxis({ 0,1,0 });
		SetWeight(1.0f);
		SetPoleWeight(0.0f);
		SetTolerance(0.1f);
		SetMaxIterations(4);
		SetClampWeight(0.0f);
		SetClampSmoothing(2);
		SetUseRotationLimits(true);
		SetXY(false);

		std::vector<NodeWeightInfo> newBones(5);
		{
			NodeWeightInfo& info = newBones[0];
			info.transform = Name("Bip001 Spine1");
			info.weight = 0.773f;
		}
		{
			NodeWeightInfo& info = newBones[1];
			info.transform = Name("Bip001 Spine2");
			info.weight = 0.883f;
		}
		{
			NodeWeightInfo& info = newBones[2];
			info.transform = Name("Bip001 Neck");
			info.weight = 0.93f;
		}
		{
			NodeWeightInfo& info = newBones[3];
			info.transform = Name("Bip001 R UpperArm");
			info.weight = 1.f;
		}
		{
			NodeWeightInfo& info = newBones[4];
			info.transform = Name("Bip001 R Hand");
			info.weight = 1.f;
		}
		SetBones(newBones);
	}

	void IKAimIKComponentInfo::CopyToCompData(IKBaseComponentData* pData) {
		if (!pData) return;
		IKBaseComponentInfo::CopyToCompData(pData);
		IKDATA(pData)->mIKInfo.fixTransforms = GetFixTransforms();
		IKDATA(pData)->mIKInfo.weight = GetWeight();
		IKDATA(pData)->mIKInfo.poleWeight = GetPoleWeight();
		IKDATA(pData)->mIKInfo.tolerance = GetTolerance();
		IKDATA(pData)->mIKInfo.maxIterations = GetMaxIterations();
		IKDATA(pData)->mIKInfo.clampWeight = GetClampWeight();
		IKDATA(pData)->mIKInfo.clampSmoothing = GetClampSmoothing();
		IKDATA(pData)->mIKInfo.useRotationLimits = GetUseRotationLimits();
		IKDATA(pData)->mIKInfo.XY = GetXY();
		IKDATA(pData)->mIKInfo.axis = GetAxis();
		IKDATA(pData)->mIKInfo.poleAxis = GetPoleAxis();
		IKDATA(pData)->mIKInfo.bones = GetBones();

		IKDATA(pData)->TargetNodeCompName = GetTargetNodeCompName();
		IKDATA(pData)->AimNodeCompName = GetAimNodeCompName();
		IKDATA(pData)->PoleTargetNodeName = GetPoleTargetNodeName();

	}
	void IKAimIKComponentInfo::CompDataToInfo(IKBaseComponentData* pData, IKBaseComponent* _pComp) {
		if (!pData) return;
		IKBaseComponentInfo::CompDataToInfo(pData, _pComp);
		IKAimIKComponent* pComp = (IKAimIKComponent*)_pComp;
		
		SetFixTransforms(IKDATA(pData)->mIKInfo.fixTransforms, pComp);
		SetWeight(IKDATA(pData)->mIKInfo.weight, pComp);
		SetPoleWeight(IKDATA(pData)->mIKInfo.poleWeight, pComp);
		SetTolerance(IKDATA(pData)->mIKInfo.tolerance, pComp);
		SetMaxIterations(IKDATA(pData)->mIKInfo.maxIterations, pComp);
		SetClampWeight(IKDATA(pData)->mIKInfo.clampWeight, pComp);
		SetClampSmoothing(IKDATA(pData)->mIKInfo.clampSmoothing, pComp);
		SetUseRotationLimits(IKDATA(pData)->mIKInfo.useRotationLimits, pComp);
		SetXY(IKDATA(pData)->mIKInfo.XY, pComp);
		SetAxis(IKDATA(pData)->mIKInfo.axis, pComp);
		SetPoleAxis(IKDATA(pData)->mIKInfo.poleAxis, pComp);
		SetBones(IKDATA(pData)->mIKInfo.bones, pComp);

		SetTargetNodeCompName(IKDATA(pData)->TargetNodeCompName, pComp);
		SetAimNodeCompName(IKDATA(pData)->AimNodeCompName, pComp);
		SetPoleTargetNodeName(IKDATA(pData)->PoleTargetNodeName, pComp);
	}

}
