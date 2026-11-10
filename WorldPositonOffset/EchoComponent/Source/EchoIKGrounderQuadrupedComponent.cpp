#include "EchoComponentStableHeaders.h"
#include "EchoIKGrounderQuadrupedComponent.h"
#include "Effect/EchoEffectHolder.h"
#include "EchoActorProFuncImp.h"
#include "IK/EchoGrounderQuadruped.h"
#include "EchoPhysXAsset.h"

namespace Echo {

	DECLAREACTORCOMTYPE_CPP(IKGrounderQuadrupedComponent)

#define IKDATA(_pData) static_cast<IKGrounderQuadrupedComponentData*>(_pData)
#define IKCOMDATA(_pData) static_cast<EchoIK::GrounderQuadruped*>(_pData)

	EDITOR_REFLECT_BEGIN(IKGrounderQuadrupedComponentInfo)
	EDITOR_REFLECT_MEMBER_UPDATE(m_Weight, "Weight", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_RootRotationWeight, "RootRotationWeight", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_MinRootRotation, "MinRootRotation", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_MaxRootRotation, "MaxRootRotation", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_RootRotationSpeed, "RootRotationSpeed", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_MaxLegOffset, "MaxLegOffset", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_MaxForeLegOffset, "MaxForeLegOffset", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_MaintainHeadRotationWeight, "MaintainHeadRotationWeight", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_Pelvis, "Pelvis", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_LastSpineBone, "LastSpineBone", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_Head, "Head", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_LegNames, "LegNames", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_ForeLegNames, "ForeLegNames", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_Solver, "Solver", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_ForeLegSolver, "ForeLegSolver", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_END();
	_DEFINEINFO_VIRFUN(IKGrounderQuadrupedComponentInfo, IKBaseComponentInfo)


	IKGrounderQuadrupedComponent::IKGrounderQuadrupedComponent(ActorPtr pActor, ActorCOMMoveableType moveable) : IKBaseComponent(pActor, moveable) {
		m_pCompData = new IKGrounderQuadrupedComponentData();
	}
	IKGrounderQuadrupedComponent::~IKGrounderQuadrupedComponent() { }

	void IKGrounderQuadrupedComponent::SetIKInfo(FinalIKInfoBase* pInfo) {
		if (!pInfo) return;
		SetIKInfoData(*(GrounderQuadrupedInfo*)pInfo);
	}

	bool IKGrounderQuadrupedComponent::IsCanCreate() {
		RoleIKComponent* pRoleIK = GetRoleIKComponent();
		if (!pRoleIK) return false;
		if (GetPelvis() == Name::g_cEmptyName || pRoleIK->getBoneNodeByName(GetPelvis()) == nullptr) return false;
		if (GetLastSpineBone() == Name::g_cEmptyName || pRoleIK->getBoneNodeByName(GetLastSpineBone()) == nullptr) return false;
		if (GetHead() == Name::g_cEmptyName || pRoleIK->getBoneNodeByName(GetHead()) == nullptr) return false;

		auto& legs = GetLegNames();
		if (legs.empty()) return false;
		for (auto& iter : legs) {
			if (iter == Name::g_cEmptyName || !CheckValidLimb(iter)) {
				return false;
			}
		}
		auto& forelegs = GetForeLegNames();
		if (forelegs.empty()) return false;
		for (auto& iter : forelegs) {
			if (iter == Name::g_cEmptyName || !CheckValidLimb(iter)) {
				return false;
			}
		}
		return true;
	}

	void IKGrounderQuadrupedComponent::SetWeight(float value){ 
		IKDATA(m_pCompData)->mIKInfo.weight = value; 
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->setWeight(value);
	} 
	float IKGrounderQuadrupedComponent::GetWeight(){ return IKDATA(m_pCompData)->mIKInfo.weight; }
	void IKGrounderQuadrupedComponent::SetRootRotationWeight(float value){ 
		IKDATA(m_pCompData)->mIKInfo.rootRotationWeight = value; 
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->rootRotationWeight = value;
	} 
	float IKGrounderQuadrupedComponent::GetRootRotationWeight(){ return IKDATA(m_pCompData)->mIKInfo.rootRotationWeight; }
	void IKGrounderQuadrupedComponent::SetMinRootRotation(float value){ 
		IKDATA(m_pCompData)->mIKInfo.minRootRotation = value; 
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->minRootRotation = value;
	} 
	float IKGrounderQuadrupedComponent::GetMinRootRotation(){ return IKDATA(m_pCompData)->mIKInfo.minRootRotation; }
	void IKGrounderQuadrupedComponent::SetMaxRootRotation(float value){ 
		IKDATA(m_pCompData)->mIKInfo.maxRootRotation = value; 
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->maxRootRotation = value;
	} 
	float IKGrounderQuadrupedComponent::GetMaxRootRotation(){ return IKDATA(m_pCompData)->mIKInfo.maxRootRotation; }
	void IKGrounderQuadrupedComponent::SetRootRotationSpeed(float value){ 
		IKDATA(m_pCompData)->mIKInfo.rootRotationSpeed = value; 
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->rootRotationSpeed = value;
	} 
	float IKGrounderQuadrupedComponent::GetRootRotationSpeed(){ return IKDATA(m_pCompData)->mIKInfo.rootRotationSpeed; }
	void IKGrounderQuadrupedComponent::SetMaxLegOffset(float value){ 
		IKDATA(m_pCompData)->mIKInfo.maxLegOffset = value; 
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->maxLegOffset = value;
	} 
	float IKGrounderQuadrupedComponent::GetMaxLegOffset(){ return IKDATA(m_pCompData)->mIKInfo.maxLegOffset; }
	void IKGrounderQuadrupedComponent::SetMaxForeLegOffset(float value){ 
		IKDATA(m_pCompData)->mIKInfo.maxForeLegOffset = value; 
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->maxForeLegOffset = value;
	} 
	float IKGrounderQuadrupedComponent::GetMaxForeLegOffset(){ return IKDATA(m_pCompData)->mIKInfo.maxForeLegOffset; }
	void IKGrounderQuadrupedComponent::SetMaintainHeadRotationWeight(float value){ 
		IKDATA(m_pCompData)->mIKInfo.maintainHeadRotationWeight = value; 
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->maintainHeadRotationWeight = value;
	} 
	float IKGrounderQuadrupedComponent::GetMaintainHeadRotationWeight(){ return IKDATA(m_pCompData)->mIKInfo.maintainHeadRotationWeight; }
	void IKGrounderQuadrupedComponent::SetPelvis(const Name& value){ 
		if (IsExistIK()) return;
		IKDATA(m_pCompData)->mIKInfo.pelvis = value; 
	} 
	const Name& IKGrounderQuadrupedComponent::GetPelvis(){ return IKDATA(m_pCompData)->mIKInfo.pelvis; }
	void IKGrounderQuadrupedComponent::SetLastSpineBone(const Name& value){ 
		if (IsExistIK()) return;
		IKDATA(m_pCompData)->mIKInfo.lastSpineBone = value; 
	} 
	const Name& IKGrounderQuadrupedComponent::GetLastSpineBone(){ return IKDATA(m_pCompData)->mIKInfo.lastSpineBone; }
	void IKGrounderQuadrupedComponent::SetHead(const Name& value){ 
		if (IsExistIK()) return;
		IKDATA(m_pCompData)->mIKInfo.head = value; 
	} 
	const Name& IKGrounderQuadrupedComponent::GetHead(){ return IKDATA(m_pCompData)->mIKInfo.head; }
	void IKGrounderQuadrupedComponent::SetLegNames(const std::vector<Name>& value){ 
		if (IsExistIK()) return;
		IKDATA(m_pCompData)->mIKInfo.legNames = value; 
	} 
	const std::vector<Name>& IKGrounderQuadrupedComponent::GetLegNames(){ return IKDATA(m_pCompData)->mIKInfo.legNames; }
	void IKGrounderQuadrupedComponent::SetForeLegNames(const std::vector<Name>& value){ 
		if (IsExistIK()) return;
		IKDATA(m_pCompData)->mIKInfo.forelegNames = value; 
	} 
	const std::vector<Name>& IKGrounderQuadrupedComponent::GetForeLegNames(){ return IKDATA(m_pCompData)->mIKInfo.forelegNames; }
	void IKGrounderQuadrupedComponent::SetSolver(const GroundingInfo& value){ 
		IKDATA(m_pCompData)->mIKInfo.solver = value; 
		if (!IsExistIK()) return;
		if (!IKCOMDATA(m_pIKComponent)->solver) return;
		IKCOMDATA(m_pIKComponent)->solver->LoadInfo(&value);
	} 
	const GroundingInfo& IKGrounderQuadrupedComponent::GetSolver(){ return IKDATA(m_pCompData)->mIKInfo.solver; }
	void IKGrounderQuadrupedComponent::SetForeLegSolver(const GroundingInfo& value){ 
		IKDATA(m_pCompData)->mIKInfo.forelegSolver = value; 
		if (!IsExistIK()) return;
		if (!IKCOMDATA(m_pIKComponent)->forelegSolver) return;
		IKCOMDATA(m_pIKComponent)->forelegSolver->LoadInfo(&value);
	} 
	const GroundingInfo& IKGrounderQuadrupedComponent::GetForeLegSolver(){ return IKDATA(m_pCompData)->mIKInfo.forelegSolver; }

}

namespace Echo {

	void IKGrounderQuadrupedComponent::SetIKInfoData(const GrounderQuadrupedInfo& info) {
		SetWeight(info.weight);
		SetRootRotationWeight(info.rootRotationWeight);
		SetMinRootRotation(info.minRootRotation);
		SetMaxRootRotation(info.maxRootRotation);
		SetRootRotationSpeed(info.rootRotationSpeed);
		SetMaxLegOffset(info.maxLegOffset);
		SetMaxForeLegOffset(info.maxForeLegOffset);
		SetMaintainHeadRotationWeight(info.maintainHeadRotationWeight);
		SetPelvis(info.pelvis);
		SetLastSpineBone(info.lastSpineBone);
		SetHead(info.head);
		SetLegNames(info.legNames);
		SetForeLegNames(info.forelegNames);
		SetSolver(info.solver);
		SetForeLegSolver(info.forelegSolver);
	}
	const GrounderQuadrupedInfo& IKGrounderQuadrupedComponent::GetIKInfoData() {
		return IKDATA(m_pCompData)->mIKInfo;
	}
	void IKGrounderQuadrupedComponent::UseDefaultInitialize() {
		SetWeight(1.0f);
		SetRootRotationWeight(1.0f);
		SetMinRootRotation(-25.0f);
		SetMaxRootRotation(45.0f);

		SetRootRotationSpeed(5.0f);
		SetMaxLegOffset(0.5f);
		SetMaxForeLegOffset(0.5f);
		SetMaintainHeadRotationWeight(0.5f);


		GroundingInfo groundingInfo;
		groundingInfo.layers = ePhysXGroup_StaticWorld;
		groundingInfo.maxStep = 0.5f;
		groundingInfo.heightOffset = 0.03f;
		groundingInfo.footSpeed = 2.5f;
		groundingInfo.footRadius = 0.15f;
		groundingInfo.footCenterOffset = 0.0f;
		groundingInfo.prediction = 0.05f;
		groundingInfo.footRotationWeight = 1.0f;
		groundingInfo.footRotationSpeed = 7.0f;
		groundingInfo.footRotationFree = true;
		groundingInfo.maxFootRotationAngle = 45.0f;
		groundingInfo.rotateSolver = true;
		groundingInfo.pelvisSpeed = 10.0f;
		groundingInfo.pelvisDamper = 0.1f;
		groundingInfo.lowerPelvisWeight = 1.0f;
		groundingInfo.liftPelvisWeight = 0.0f;
		groundingInfo.rootSphereCastRadius = 0.1f;
		groundingInfo.overstepFallsDown = true;
		groundingInfo.quality = 1;
		SetSolver(groundingInfo);
		SetForeLegSolver(groundingInfo);

		SetPelvis(Name("Bip001 Pelvis"));
		SetLastSpineBone(Name("Bip001 Spine2"));
		SetHead(Name("Bip001 Neck2"));

		std::vector<Name> nullLegs;
		SetLegNames(nullLegs);
		SetForeLegNames(nullLegs);
	}


	void IKGrounderQuadrupedComponentInfo::CopyToCompData(IKBaseComponentData* pData) {
		if (!pData) return;
		IKBaseComponentInfo::CopyToCompData(pData);
		IKDATA(pData)->mIKInfo.weight = GetWeight();
		IKDATA(pData)->mIKInfo.rootRotationWeight = GetRootRotationWeight();
		IKDATA(pData)->mIKInfo.minRootRotation = GetMinRootRotation();
		IKDATA(pData)->mIKInfo.maxRootRotation = GetMaxRootRotation();
		IKDATA(pData)->mIKInfo.rootRotationSpeed = GetRootRotationSpeed();
		IKDATA(pData)->mIKInfo.maxLegOffset = GetMaxLegOffset();
		IKDATA(pData)->mIKInfo.maxForeLegOffset = GetMaxForeLegOffset();
		IKDATA(pData)->mIKInfo.maintainHeadRotationWeight = GetMaintainHeadRotationWeight();
		IKDATA(pData)->mIKInfo.pelvis = GetPelvis();
		IKDATA(pData)->mIKInfo.lastSpineBone = GetLastSpineBone();
		IKDATA(pData)->mIKInfo.head = GetHead();
		IKDATA(pData)->mIKInfo.legNames = GetLegNames();
		IKDATA(pData)->mIKInfo.forelegNames = GetForeLegNames();
		IKDATA(pData)->mIKInfo.solver = GetSolver();
		IKDATA(pData)->mIKInfo.forelegSolver = GetForeLegSolver();

	}

	void IKGrounderQuadrupedComponentInfo::CompDataToInfo(IKBaseComponentData* pData, IKBaseComponent* _pComp) {
		if (!pData) return;
		IKBaseComponentInfo::CompDataToInfo(pData, _pComp);
		IKGrounderQuadrupedComponent* pComp = (IKGrounderQuadrupedComponent*)_pComp;
		SetWeight(IKDATA(pData)->mIKInfo.weight, pComp);
		SetRootRotationWeight(IKDATA(pData)->mIKInfo.rootRotationWeight, pComp);
		SetMinRootRotation(IKDATA(pData)->mIKInfo.minRootRotation, pComp);
		SetMaxRootRotation(IKDATA(pData)->mIKInfo.maxRootRotation, pComp);
		SetRootRotationSpeed(IKDATA(pData)->mIKInfo.rootRotationSpeed, pComp);
		SetMaxLegOffset(IKDATA(pData)->mIKInfo.maxLegOffset, pComp);
		SetMaxForeLegOffset(IKDATA(pData)->mIKInfo.maxForeLegOffset, pComp);
		SetMaintainHeadRotationWeight(IKDATA(pData)->mIKInfo.maintainHeadRotationWeight, pComp);
		SetPelvis(IKDATA(pData)->mIKInfo.pelvis, pComp);
		SetLastSpineBone(IKDATA(pData)->mIKInfo.lastSpineBone, pComp);
		SetHead(IKDATA(pData)->mIKInfo.head, pComp);
		SetLegNames(IKDATA(pData)->mIKInfo.legNames, pComp);
		SetForeLegNames(IKDATA(pData)->mIKInfo.forelegNames, pComp);
		SetSolver(IKDATA(pData)->mIKInfo.solver, pComp);
		SetForeLegSolver(IKDATA(pData)->mIKInfo.forelegSolver, pComp);

	}
}
