#include "EchoComponentStableHeaders.h"
#include "EchoIKGrounderFBBIKComponent.h"
#include "Effect/EchoEffectHolder.h"
#include "EchoActorProFuncImp.h"
#include "IK/EchoGrounderFBBIK.h"
#include "EchoPhysXAsset.h"

namespace Echo {

	DECLAREACTORCOMTYPE_CPP(IKGrounderFBBIKComponent)

#define IKDATA(_pData) static_cast<IKGrounderFBBIKComponentData*>(_pData)
#define IKCOMDATA(_pData) static_cast<EchoIK::GrounderFBBIK*>(_pData)

	EDITOR_REFLECT_BEGIN(IKGrounderFBBIKComponentInfo)
	EDITOR_REFLECT_MEMBER_UPDATE(m_Weight, "Weight", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_SpineBend, "SpineBend", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_SpineSpeed, "SpineSpeed", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_FullBodyBipedIKName, "FullBodyBipedIKName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_Solver, "Solver", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_Spine, "Spine", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_END();
	_DEFINEINFO_VIRFUN(IKGrounderFBBIKComponentInfo, IKBaseComponentInfo)


	IKGrounderFBBIKComponent::IKGrounderFBBIKComponent(ActorPtr pActor, ActorCOMMoveableType moveable) : IKBaseComponent(pActor, moveable) {
		m_pCompData = new IKGrounderFBBIKComponentData();
	}
	IKGrounderFBBIKComponent::~IKGrounderFBBIKComponent() { }
	void IKGrounderFBBIKComponent::SetIKInfo(FinalIKInfoBase* pInfo) {
		if (!pInfo) return;
		SetIKInfoData(*(GrounderFBBIKInfo*)pInfo);
	}
	const GrounderFBBIKInfo& IKGrounderFBBIKComponent::GetIKInfoData() {
		return IKDATA(m_pCompData)->mIKInfo;
	}
	
	bool IKGrounderFBBIKComponent::IsCanCreate() {
		return !GetSpine().empty() && CheckValidFBBIK(IKDATA(m_pCompData)->mIKInfo.ik);
	}

	void IKGrounderFBBIKComponent::SetFullBodyBipedIKName(const Name& ikName) {
		if (IsExistIK()) return;
		IKDATA(m_pCompData)->mIKInfo.ik = ikName;
	}
	const Name& IKGrounderFBBIKComponent::GetFullBodyBipedIKName() {
		return IKDATA(m_pCompData)->mIKInfo.ik;
	}

	void IKGrounderFBBIKComponent::SetWeight(float value) {
		if (EchoIsNanOrInf(value)) return;
		IKDATA(m_pCompData)->mIKInfo.weight = value;
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->weight = value;
	}
	float IKGrounderFBBIKComponent::GetWeight() {
		return IKDATA(m_pCompData)->mIKInfo.weight;
	}
	void IKGrounderFBBIKComponent::SetSpineBend(float value) {
		if (EchoIsNanOrInf(value)) return;
		IKDATA(m_pCompData)->mIKInfo.spineBend = value;
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->spineBend = value;
	}
	float IKGrounderFBBIKComponent::GetSpineBend() {
		return IKDATA(m_pCompData)->mIKInfo.spineBend;
	}
	void IKGrounderFBBIKComponent::SetSpineSpeed(float value) {
		if (EchoIsNanOrInf(value)) return;
		IKDATA(m_pCompData)->mIKInfo.spineSpeed = value;
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->spineSpeed = value;
	}
	float IKGrounderFBBIKComponent::GetSpineSpeed() {
		return IKDATA(m_pCompData)->mIKInfo.spineSpeed;
	}
	void IKGrounderFBBIKComponent::SetSolver(const GroundingInfo& value) {
		IKDATA(m_pCompData)->mIKInfo.solver = value;
		if (!IsExistIK()) return;
		if (!IKCOMDATA(m_pIKComponent)->solver) return;
		IKCOMDATA(m_pIKComponent)->solver->LoadInfo(&value);
	}
	const GroundingInfo& IKGrounderFBBIKComponent::GetSolver() {
		return IKDATA(m_pCompData)->mIKInfo.solver;
	}
	void IKGrounderFBBIKComponent::SetSpine(const std::vector<SpineEffectorInfo>& value) {
		if (!IsExistIK()) {
			IKDATA(m_pCompData)->mIKInfo.spine = value;
		}
		else {
			if (IKDATA(m_pCompData)->mIKInfo.spine.size() != value.size()) return;
			for (int i = 0; i != value.size(); ++i) {
				if (IKDATA(m_pCompData)->mIKInfo.spine[i].effectorType != value[i].effectorType) return;
			}
			for (int i = 0; i != value.size(); ++i) {
				IKDATA(m_pCompData)->mIKInfo.spine[i].horizontalWeight = value[i].horizontalWeight;
				IKDATA(m_pCompData)->mIKInfo.spine[i].verticalWeight = value[i].verticalWeight;
				IKCOMDATA(m_pIKComponent)->spine[i]->horizontalWeight = value[i].horizontalWeight;
				IKCOMDATA(m_pIKComponent)->spine[i]->verticalWeight = value[i].verticalWeight;
			}
		}
	}
	const std::vector<SpineEffectorInfo>& IKGrounderFBBIKComponent::GetSpine() {
		return IKDATA(m_pCompData)->mIKInfo.spine;
	}

	EchoIK::FullBodyBipedIK* IKGrounderFBBIKComponent::GetFullBodyBipedIK() {
		if (!IsExistIK()) return nullptr;
		return IKCOMDATA(m_pIKComponent)->ik;
	}
}

namespace Echo {

	void IKGrounderFBBIKComponentInfo::CopyToCompData(IKBaseComponentData* pData) {
		if (!pData) return;
		IKBaseComponentInfo::CopyToCompData(pData);
		IKDATA(pData)->mIKInfo.weight = GetWeight();
		IKDATA(pData)->mIKInfo.spineBend = GetSpineBend();
		IKDATA(pData)->mIKInfo.spineSpeed = GetSpineSpeed();
		IKDATA(pData)->mIKInfo.ik = GetFullBodyBipedIKName();
		IKDATA(pData)->mIKInfo.solver = GetSolver();
		IKDATA(pData)->mIKInfo.spine = GetSpine();
	}
	void IKGrounderFBBIKComponentInfo::CompDataToInfo(IKBaseComponentData* pData, IKBaseComponent* _pComp) {
		if (!pData) return;
		IKBaseComponentInfo::CompDataToInfo(pData, _pComp);
		IKGrounderFBBIKComponent* pComp = (IKGrounderFBBIKComponent*)_pComp;
		SetWeight(IKDATA(pData)->mIKInfo.weight, pComp);
		SetSpineBend(IKDATA(pData)->mIKInfo.spineBend, pComp);
		SetSpineSpeed(IKDATA(pData)->mIKInfo.spineSpeed, pComp);
		SetFullBodyBipedIKName(IKDATA(pData)->mIKInfo.ik, pComp);
		SetSolver(IKDATA(pData)->mIKInfo.solver, pComp);
		SetSpine(IKDATA(pData)->mIKInfo.spine, pComp);
	}

	void IKGrounderFBBIKComponent::SetIKInfoData(const GrounderFBBIKInfo& info) {
		SetWeight(info.weight);
		SetSpineBend(info.spineBend);
		SetSpineSpeed(info.spineSpeed);
		SetFullBodyBipedIKName(info.ik);
		SetSolver(info.solver);
		SetSpine(info.spine);
	}

	void IKGrounderFBBIKComponent::UseDefaultInitialize() {
		SetWeight(1.0f);
		SetSpineBend(2.0f);
		SetSpineSpeed(3.0f);
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
		SetFullBodyBipedIKName(Name::g_cEmptyName);
		std::vector<SpineEffectorInfo> newSpine(5);
		{
			SpineEffectorInfo& effector = newSpine[0];
			effector.effectorType = EchoIK::FullBodyBipedEffector::LeftShoulder;
			effector.horizontalWeight = 1.0f;
			effector.verticalWeight = 0.3f;
		}
		{
			SpineEffectorInfo& effector = newSpine[1];
			effector.effectorType = EchoIK::FullBodyBipedEffector::RightShoulder;
			effector.horizontalWeight = 1.0f;
			effector.verticalWeight = 0.3f;
		}
		{
			SpineEffectorInfo& effector = newSpine[2];
			effector.effectorType = EchoIK::FullBodyBipedEffector::Body;
			effector.horizontalWeight = -0.3f;
			effector.verticalWeight = 0.3f;
		}
		{
			SpineEffectorInfo& effector = newSpine[3];
			effector.effectorType = EchoIK::FullBodyBipedEffector::LeftHand;
			effector.horizontalWeight = 0.5f;
			effector.verticalWeight = 0.f;
		}
		{
			SpineEffectorInfo& effector = newSpine[4];
			effector.effectorType = EchoIK::FullBodyBipedEffector::RightHand;
			effector.horizontalWeight = 0.5f;
			effector.verticalWeight = 0.f;
		}
		SetSpine(newSpine);
	}

}
