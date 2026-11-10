#include "EchoComponentStableHeaders.h"
#include "EchoIKGrounderIKComponent.h"
#include "Effect/EchoEffectHolder.h"
#include "EchoActorProFuncImp.h"
#include "IK/EchoGrounderIK.h"
#include "EchoPhysXAsset.h"

namespace Echo {

	DECLAREACTORCOMTYPE_CPP(IKGrounderIKComponent)

#define IKDATA(_pData) static_cast<IKGrounderIKComponentData*>(_pData)
#define IKCOMDATA(_pData) static_cast<EchoIK::GrounderIK*>(_pData)

	EDITOR_REFLECT_BEGIN(IKGrounderIKComponentInfo)
	EDITOR_REFLECT_MEMBER_UPDATE(m_Weight, "Weight", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_RootRotationWeight, "RootRotationWeight", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_RootRotationSpeed, "RootRotationSpeed", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_MaxRootRotationAngle, "MaxRootRotationAngle", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_Pelvis, "Pelvis", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_LegNames, "LegNames", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_Solver, "Solver", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_END();
	_DEFINEINFO_VIRFUN(IKGrounderIKComponentInfo, IKBaseComponentInfo)


	IKGrounderIKComponent::IKGrounderIKComponent(ActorPtr pActor, ActorCOMMoveableType moveable) : IKBaseComponent(pActor, moveable) {
		m_pCompData = new IKGrounderIKComponentData();
	}
	IKGrounderIKComponent::~IKGrounderIKComponent() { }
	void IKGrounderIKComponent::SetIKInfo(FinalIKInfoBase* pInfo) {
		if (!pInfo) return;
		SetIKInfoData(*(GrounderIKInfo*)pInfo);
	}
	bool IKGrounderIKComponent::IsCanCreate() {
		RoleIKComponent* pRoleIK = GetRoleIKComponent();
		if (!pRoleIK) return false;
		if (GetPelvis() == Name::g_cEmptyName || pRoleIK->getBoneNodeByName(GetPelvis()) == nullptr) return false;
		auto& legs = GetLegNames();
		if (legs.empty()) return false;
		for (auto& iter : legs) {
			if (iter == Name::g_cEmptyName || !CheckValidLimb(iter)) {
				return false;
			}
		}
		return true;
	}

	void IKGrounderIKComponent::SetWeight(float value){
		if (EchoIsNanOrInf(value)) return;
		IKDATA(m_pCompData)->mIKInfo.weight = value; 
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->weight = value;
	} 
	float IKGrounderIKComponent::GetWeight(){ return IKDATA(m_pCompData)->mIKInfo.weight; }
	void IKGrounderIKComponent::SetRootRotationWeight(float value){
		if (EchoIsNanOrInf(value)) return;
		IKDATA(m_pCompData)->mIKInfo.rootRotationWeight = value; 
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->rootRotationWeight = value;
	} 
	float IKGrounderIKComponent::GetRootRotationWeight(){ return IKDATA(m_pCompData)->mIKInfo.rootRotationWeight; }
	void IKGrounderIKComponent::SetRootRotationSpeed(float value){ 
		IKDATA(m_pCompData)->mIKInfo.rootRotationSpeed = value; 
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->rootRotationSpeed = value;
	} 
	float IKGrounderIKComponent::GetRootRotationSpeed(){ return IKDATA(m_pCompData)->mIKInfo.rootRotationSpeed; }
	void IKGrounderIKComponent::SetMaxRootRotationAngle(float value){ 
		IKDATA(m_pCompData)->mIKInfo.maxRootRotationAngle = value; 
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->maxRootRotationAngle = value;
	} 
	float IKGrounderIKComponent::GetMaxRootRotationAngle(){ return IKDATA(m_pCompData)->mIKInfo.maxRootRotationAngle; }
	void IKGrounderIKComponent::SetPelvis(const Name& value){ 
		if (IsExistIK()) return;
		IKDATA(m_pCompData)->mIKInfo.pelvis = value; 
	} 
	const Name& IKGrounderIKComponent::GetPelvis(){ return IKDATA(m_pCompData)->mIKInfo.pelvis; }
	void IKGrounderIKComponent::SetLegNames(const std::vector<Name>& value){ 
		if (IsExistIK()) return;
		IKDATA(m_pCompData)->mIKInfo.legNames = value; 
	} 
	const std::vector<Name>& IKGrounderIKComponent::GetLegNames(){ return IKDATA(m_pCompData)->mIKInfo.legNames; }
	void IKGrounderIKComponent::SetSolver(const GroundingInfo& value){ 
		IKDATA(m_pCompData)->mIKInfo.solver = value;
		if (!IsExistIK()) return;
		if (!IKCOMDATA(m_pIKComponent)->solver) return;
		IKCOMDATA(m_pIKComponent)->solver->LoadInfo(&value);
	} 
	const GroundingInfo& IKGrounderIKComponent::GetSolver(){ return IKDATA(m_pCompData)->mIKInfo.solver; }

}

namespace Echo {

	void IKGrounderIKComponent::SetIKInfoData(const GrounderIKInfo& info) {
		SetWeight(info.weight);
		SetRootRotationWeight(info.rootRotationWeight);
		SetRootRotationSpeed(info.rootRotationSpeed);
		SetMaxRootRotationAngle(info.maxRootRotationAngle);
		SetPelvis(info.pelvis);
		SetLegNames(info.legNames);
		SetSolver(info.solver);
	}
	const GrounderIKInfo& IKGrounderIKComponent::GetIKInfoData() {
		return IKDATA(m_pCompData)->mIKInfo;
	}
	void IKGrounderIKComponent::UseDefaultInitialize() {
		SetWeight(1.0f);
		SetRootRotationWeight(1.0f);
		SetRootRotationSpeed(5.0f);
		SetMaxRootRotationAngle(45.0f);
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
		SetPelvis(Name("Bip001 Pelvis"));
		SetLegNames(std::vector<Name>());
	}

	void IKGrounderIKComponentInfo::CopyToCompData(IKBaseComponentData* pData) {
		if (!pData) return;
		IKBaseComponentInfo::CopyToCompData(pData);
		IKDATA(pData)->mIKInfo.weight = GetWeight();
		IKDATA(pData)->mIKInfo.rootRotationWeight = GetRootRotationWeight();
		IKDATA(pData)->mIKInfo.rootRotationSpeed = GetRootRotationSpeed();
		IKDATA(pData)->mIKInfo.maxRootRotationAngle = GetMaxRootRotationAngle();
		IKDATA(pData)->mIKInfo.pelvis = GetPelvis();
		IKDATA(pData)->mIKInfo.legNames = GetLegNames();
		IKDATA(pData)->mIKInfo.solver = GetSolver();

	}
	void IKGrounderIKComponentInfo::CompDataToInfo(IKBaseComponentData* pData, IKBaseComponent* _pComp) {
		if (!pData) return;
		IKBaseComponentInfo::CompDataToInfo(pData, _pComp);
		IKGrounderIKComponent* pComp = (IKGrounderIKComponent*)_pComp;
		SetWeight(IKDATA(pData)->mIKInfo.weight, pComp);
		SetRootRotationWeight(IKDATA(pData)->mIKInfo.rootRotationWeight, pComp);
		SetRootRotationSpeed(IKDATA(pData)->mIKInfo.rootRotationSpeed, pComp);
		SetMaxRootRotationAngle(IKDATA(pData)->mIKInfo.maxRootRotationAngle, pComp);
		SetPelvis(IKDATA(pData)->mIKInfo.pelvis, pComp);
		SetLegNames(IKDATA(pData)->mIKInfo.legNames, pComp);
		SetSolver(IKDATA(pData)->mIKInfo.solver, pComp);

	}

}
