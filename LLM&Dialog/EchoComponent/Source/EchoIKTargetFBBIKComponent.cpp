#include "EchoComponentStableHeaders.h"
#include "EchoIKTargetFBBIKComponent.h"
#include "Effect/EchoEffectHolder.h"
#include "EchoActorProFuncImp.h"
#include "Actor/EchoNodeComponent.h"
#include "EchoSkeletonInstance.h"
#include "EchoIKTargetPointComponent.h"
#include "EchoRoleTagPointPosComponent.h"
#include "Role/EchoRoleTagPointComponent.h"
#include "EchoTagPoint.h"


namespace Echo{



	DECLAREACTORCOMTYPE_CPP(IKTargetFBBIKComponent)

#define IKDATA(_pData) static_cast<IKTargetFBBIKComponentData*>(_pData)
#define IKCOMDATA(_pData) static_cast<EchoIK::TargetFBBIK*>(_pData)


	EDITOR_REFLECT_BEGIN(IKTargetFBBIKComponentInfo)
	EDITOR_REFLECT_MEMBER_UPDATE(m_FullBodyBipedIKName, "FullBodyBipedIKName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_BodyTargetPointCompName, "BodyTargetPointCompName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_LeftHandTargetPointCompName, "LeftHandTargetPointCompName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_LeftShoulderTargetPointCompName, "LeftShoulderTargetPointCompName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_RightHandTargetPointCompName, "RightHandTargetPointCompName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_RightShoulderTargetPointCompName, "RightShoulderTargetPointCompName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_LeftFootTargetPointCompName, "LeftFootTargetPointCompName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_LeftThighTargetPointCompName, "LeftThighTargetPointCompName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_RightFootTargetPointCompName, "RightFootTargetPointCompName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_RightThighTargetPointCompName, "RightThighTargetPointCompName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_LeftArmPointCompName, "LeftArmPointCompName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_LeftLegPointCompName, "LeftLegPointCompName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_RightArmPointCompName, "RightArmPointCompName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_RightLegPointCompName, "RightLegPointCompName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_END();
	_DEFINEINFO_VIRFUN(IKTargetFBBIKComponentInfo, IKBaseComponentInfo)


	void IKTargetFBBIKComponent::SetIKInfo(FinalIKInfoBase* pInfo) {
		if (!pInfo) return;
		SetIKInfoData(*(TargetFBBIKInfo*)pInfo);
	}

	void IKTargetFBBIKComponentInfo::SetTargetPointCompName(FBBEffectorType effectorIndex, const String& name, IKTargetFBBIKComponent* pComp) {
		switch (effectorIndex)
		{
		case FBBEffectorType::Body:
			return SetBodyTargetPointCompName(name, pComp);
			break;
		case FBBEffectorType::LeftShoulder:
			return SetLeftShoulderTargetPointCompName(name, pComp);
			break;
		case FBBEffectorType::RightShoulder:
			return SetRightShoulderTargetPointCompName(name, pComp);
			break;
		case FBBEffectorType::LeftThigh:
			return SetLeftThighTargetPointCompName(name, pComp);
			break;
		case FBBEffectorType::RightThigh:
			return SetRightThighTargetPointCompName(name, pComp);
			break;
		case FBBEffectorType::LeftHand:
			return SetLeftHandTargetPointCompName(name, pComp);
			break;
		case FBBEffectorType::RightHand:
			return SetRightHandTargetPointCompName(name, pComp);
			break;
		case FBBEffectorType::LeftFoot:
			return SetLeftFootTargetPointCompName(name, pComp);
			break;
		case FBBEffectorType::RightFoot:
			return SetRightFootTargetPointCompName(name, pComp);
			break;
		case FBBEffectorType::NumEffectors:
			break;
		default:
			break;
		}
	}
	const String& IKTargetFBBIKComponentInfo::GetTargetPointCompName(FBBEffectorType effectorIndex) {
		switch (effectorIndex)
		{
		case FBBEffectorType::Body:
			return GetBodyTargetPointCompName();
			break;
		case FBBEffectorType::LeftShoulder:
			return GetLeftShoulderTargetPointCompName();
			break;
		case FBBEffectorType::RightShoulder:
			return GetRightShoulderTargetPointCompName();
			break;
		case FBBEffectorType::LeftThigh:
			return GetLeftThighTargetPointCompName();
			break;
		case FBBEffectorType::RightThigh:
			return GetRightThighTargetPointCompName();
			break;
		case FBBEffectorType::LeftHand:
			return GetLeftHandTargetPointCompName();
			break;
		case FBBEffectorType::RightHand:
			return GetRightHandTargetPointCompName();
			break;
		case FBBEffectorType::LeftFoot:
			return GetLeftFootTargetPointCompName();
			break;
		case FBBEffectorType::RightFoot:
			return GetRightFootTargetPointCompName();
			break;
		case FBBEffectorType::NumEffectors:
			break;
		default:
			break;
		}
		static String g_cEmptyString = "";
		return g_cEmptyString;
	}

	void IKTargetFBBIKComponentInfo::SetBendGoalPointCompName(FBBChainType chainIndex, const String& name, IKTargetFBBIKComponent* pComp) {
		switch (chainIndex)
		{
		case FBBChainType::LeftArm:
			SetLeftArmPointCompName(name, pComp);
			break;
		case FBBChainType::LeftLeg:
			SetLeftLegPointCompName(name, pComp);
			break;
		case FBBChainType::RightArm:
			SetRightArmPointCompName(name, pComp);
			break;
		case FBBChainType::RightLeg:
			SetRightLegPointCompName(name, pComp);
			break;
		default:
			break;
		}
	}
	const String& IKTargetFBBIKComponentInfo::GetBendGoalPointCompName(FBBChainType chainIndex) {
		switch (chainIndex)
		{
		case FBBChainType::LeftArm:
			return GetLeftArmPointCompName();
			break;
		case FBBChainType::LeftLeg:
			return GetLeftLegPointCompName();
			break;
		case FBBChainType::RightArm:
			return GetRightArmPointCompName();
			break;
		case FBBChainType::RightLeg:
			return GetRightLegPointCompName();
			break;
		default:
			break;
		}
		static String g_cEmptyString = "";
		return g_cEmptyString;
	}

	IKTargetFBBIKComponent::IKTargetFBBIKComponent(ActorPtr pActor, ActorCOMMoveableType moveable) : IKBaseComponent(pActor, moveable) {
		m_pCompData = new IKTargetFBBIKComponentData();
	}
	IKTargetFBBIKComponent::~IKTargetFBBIKComponent() {  }
	const TargetFBBIKInfo& IKTargetFBBIKComponent::GetIKInfoData() {
		return IKDATA(m_pCompData)->mIKInfo;
	}
	bool IKTargetFBBIKComponent::IsCanCreate() {
		return CheckValidFBBIK(IKDATA(m_pCompData)->mIKInfo.ik);
	}

	void IKTargetFBBIKComponent::SetFullBodyBipedIKName(const Name& ikName) {
		if (IsExistIK()) return;
		IKDATA(m_pCompData)->mIKInfo.ik = ikName;
	}
	const Name& IKTargetFBBIKComponent::GetFullBodyBipedIKName() {
		return IKDATA(m_pCompData)->mIKInfo.ik;
	}

	void IKTargetFBBIKComponent::SetBodyTargetPointCompName(const String& name) { SetTargetPointCompName(FBBEffectorType::Body, name); }
	void IKTargetFBBIKComponent::SetLeftHandTargetPointCompName(const String& name){ SetTargetPointCompName(FBBEffectorType::LeftHand, name); }
	void IKTargetFBBIKComponent::SetLeftShoulderTargetPointCompName(const String& name){ SetTargetPointCompName(FBBEffectorType::LeftShoulder, name); }
	void IKTargetFBBIKComponent::SetRightHandTargetPointCompName(const String& name){ SetTargetPointCompName(FBBEffectorType::RightHand, name); }
	void IKTargetFBBIKComponent::SetRightShoulderTargetPointCompName(const String& name){ SetTargetPointCompName(FBBEffectorType::RightShoulder, name); }
	void IKTargetFBBIKComponent::SetLeftFootTargetPointCompName(const String& name){ SetTargetPointCompName(FBBEffectorType::LeftFoot, name); }
	void IKTargetFBBIKComponent::SetLeftThighTargetPointCompName(const String& name){ SetTargetPointCompName(FBBEffectorType::LeftThigh, name); }
	void IKTargetFBBIKComponent::SetRightFootTargetPointCompName(const String& name){ SetTargetPointCompName(FBBEffectorType::RightFoot, name); }
	void IKTargetFBBIKComponent::SetRightThighTargetPointCompName(const String& name){ SetTargetPointCompName(FBBEffectorType::RightThigh, name); }
	const String& IKTargetFBBIKComponent::GetBodyTargetPointCompName(){ return GetTargetPointCompName(FBBEffectorType::Body); }
	const String& IKTargetFBBIKComponent::GetLeftHandTargetPointCompName(){ return GetTargetPointCompName(FBBEffectorType::LeftHand); }
	const String& IKTargetFBBIKComponent::GetLeftShoulderTargetPointCompName(){ return GetTargetPointCompName(FBBEffectorType::LeftShoulder); }
	const String& IKTargetFBBIKComponent::GetRightHandTargetPointCompName(){ return GetTargetPointCompName(FBBEffectorType::RightHand); }
	const String& IKTargetFBBIKComponent::GetRightShoulderTargetPointCompName(){ return GetTargetPointCompName(FBBEffectorType::RightShoulder); }
	const String& IKTargetFBBIKComponent::GetLeftFootTargetPointCompName(){ return GetTargetPointCompName(FBBEffectorType::LeftFoot); }
	const String& IKTargetFBBIKComponent::GetLeftThighTargetPointCompName(){ return GetTargetPointCompName(FBBEffectorType::LeftThigh); }
	const String& IKTargetFBBIKComponent::GetRightFootTargetPointCompName(){ return GetTargetPointCompName(FBBEffectorType::RightFoot); }
	const String& IKTargetFBBIKComponent::GetRightThighTargetPointCompName(){ return GetTargetPointCompName(FBBEffectorType::RightThigh); }

	void IKTargetFBBIKComponent::SetLeftArmPointCompName(const String& name) {SetBendGoalPointCompName(FBBChainType::LeftArm, name);}
	void IKTargetFBBIKComponent::SetLeftLegPointCompName(const String& name) {SetBendGoalPointCompName(FBBChainType::LeftLeg, name);}
	void IKTargetFBBIKComponent::SetRightArmPointCompName(const String& name) {SetBendGoalPointCompName(FBBChainType::RightArm, name);}
	void IKTargetFBBIKComponent::SetRightLegPointCompName(const String& name) {SetBendGoalPointCompName(FBBChainType::RightLeg, name);}
	const String& IKTargetFBBIKComponent::GetLeftArmPointCompName() {return GetBendGoalPointCompName(FBBChainType::LeftArm);}
	const String& IKTargetFBBIKComponent::GetLeftLegPointCompName() {return GetBendGoalPointCompName(FBBChainType::LeftLeg);}
	const String& IKTargetFBBIKComponent::GetRightArmPointCompName() {return GetBendGoalPointCompName(FBBChainType::RightArm);}
	const String& IKTargetFBBIKComponent::GetRightLegPointCompName() {return GetBendGoalPointCompName(FBBChainType::RightLeg);}

	void IKTargetFBBIKComponent::SetTargetPointCompName(FBBEffectorType effectIndex, const String& name) {
		IKDATA(m_pCompData)->TargetPointCompNames[effectIndex] = name;
		ActorComponentPtr compPtr = FindComponent(name);
		SetFBBTarget(effectIndex, compPtr);
	}
	const String& IKTargetFBBIKComponent::GetTargetPointCompName(FBBEffectorType effectIndex) {
		return IKDATA(m_pCompData)->TargetPointCompNames[effectIndex];
	}
	
	void IKTargetFBBIKComponent::SetBendGoalPointCompName(FBBChainType chainIndex, const String& name) {
		IKDATA(m_pCompData)->BendGoalPointCompNames[chainIndex] = name;
		ActorComponentPtr compPtr = FindComponent(name);
		SetFBBBendGoal(chainIndex, compPtr);
	}
	const String& IKTargetFBBIKComponent::GetBendGoalPointCompName(FBBChainType chainIndex) {
		return IKDATA(m_pCompData)->BendGoalPointCompNames[chainIndex];
	}

	void IKTargetFBBIKComponent::SetSkeletonPosture(const std::map<Name, Quaternion>& boneMap, bool IsSet) {
		if (!IsExistIK() || boneMap.empty()) return;
		IKCOMDATA(m_pIKComponent)->setSkeletonPosture(boneMap, IsSet);
	}
	void IKTargetFBBIKComponent::ResetSkeletonPosture(const std::map<Name, Quaternion>& boneMap) {
		if (!IsExistIK() || boneMap.empty()) return;
		IKCOMDATA(m_pIKComponent)->resetSkeletonPosture(boneMap);
	}
	void IKTargetFBBIKComponent::SetFBBEffectorWeight(FBBEffectorType effectIndex, float positionWeight, float rotationWeight) {
		if (EchoIsNanOrInf(positionWeight) || EchoIsNanOrInf(rotationWeight)) return;
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->setFbbEffectorWeight(effectIndex, positionWeight, rotationWeight);
	}
	void IKTargetFBBIKComponent::SetFBBChainWeight(FBBChainType chainIndex, float weight) {
		if (EchoIsNanOrInf(weight)) return;
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->setFbbChainWeight(chainIndex, weight);
	}


	void IKTargetFBBIKComponent::SetFBBTarget(FBBEffectorType type, const ActorComponentPtr& pComp) {
		if (!IsExistIK()) {
			mTargetPtrs[type] = pComp;
		}
		else {
			ResetFBBTarget(type);
			mTargetPtrs[type] = pComp;
			ApplyTargetPtr(type);
		}
	}
	void IKTargetFBBIKComponent::ResetFBBTarget(FBBEffectorType type) {
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->resetFbbTarget(type);
		if (mTargetPtrs[type] == nullptr) return;
		if (mTargetPtrs[type]->getType() == IKTargetPointComponent::mType) {
			IKTargetPointComponent* pIKTargetPointComp = (IKTargetPointComponent*)mTargetPtrs[type].getPointer();
			pIKTargetPointComp->SetSkpeCallBack(nullptr, nullptr);
		}
	}

	void IKTargetFBBIKComponent::SetFBBBendGoal(FBBChainType type, const ActorComponentPtr& pComp) {
		if (!IsExistIK()) {
			mBendGoalPtrs[type] = pComp;
		}
		else {
			ResetFBBBendGoal(type);
			mBendGoalPtrs[type] = pComp;
			ApplyBendGoalPtr(type);
		}
	}
	
	void IKTargetFBBIKComponent::ResetFBBBendGoal(FBBChainType chainIndex) {
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->resetFbbBendGoal(chainIndex);
	}

	void IKTargetFBBIKComponent::PostCreateIK() {
		ApplyTargetPtrs();
		ApplyBendGoalPtrs();

		m_pIKComponent->mIKCallbackFuncTrigger.RegisterGetTagPointNameByCompCallback
		(
			[this](const ActorComponentPtr& compPtr)
			{
				return this->getTagPointNameByTargetComponent(compPtr);
			}
		);
	}

	void IKTargetFBBIKComponent::PreDestroyIK() {
		ResetFBBTargets();
		ResetFBBBendGoals();
		m_pIKComponent->mIKCallbackFuncTrigger.UnRegisterGetTagPointNameByCompCallback();
	}

	EchoIK::FullBodyBipedIK* IKTargetFBBIKComponent::GetFullBodyBipedIK() {
		if (!IsExistIK()) return nullptr;
		return IKCOMDATA(m_pIKComponent)->ik;
	}

	void IKTargetFBBIKComponent::ApplyTargetPtr(FBBEffectorType type) {
		if (mTargetPtrs[type] == nullptr) return;
		IKCOMDATA(m_pIKComponent)->setFbbTarget(type, mTargetPtrs[type], getTagPointNameByTargetComponent(mTargetPtrs[type]));
		if (mTargetPtrs[type]->getType() == IKTargetPointComponent::mType) {
			IKTargetPointComponent* pIKTargetPointComp = (IKTargetPointComponent*)mTargetPtrs[type].getPointer();
			auto OnSetSkpe = [this, type](IKTargetPointComponent* pComp) {
				if (pComp->GetIKPartType() == type) {
					this->SetSkeletonPosture(pComp->GetSkeletonPosture());
				}
			};
			auto OnResetSkpe = [this](IKTargetPointComponent* pComp) { this->ResetSkeletonPosture(pComp->GetSkeletonPosture()); };
			pIKTargetPointComp->SetSkpeCallBack(OnSetSkpe, OnResetSkpe);
		}
	}
	void IKTargetFBBIKComponent::ApplyBendGoalPtr(FBBChainType type) {
		if (mBendGoalPtrs[type] == nullptr) return;
		IKCOMDATA(m_pIKComponent)->setFbbBendGoal(type, mBendGoalPtrs[type], getTagPointNameByTargetComponent(mBendGoalPtrs[type]));

	}
	void IKTargetFBBIKComponent::ApplyTargetPtrs() {
		for (int i = 0; i != FBBEffectorType::NumEffectors; ++i) {
			ApplyTargetPtr((FBBEffectorType)i);
		}
	}
	void IKTargetFBBIKComponent::ApplyBendGoalPtrs() {
		for (int i = 0; i != FBBChainType::NumChains; ++i) {
			ApplyBendGoalPtr((FBBChainType)i);
		}
	}

	void IKTargetFBBIKComponent::ResetFBBTargets() {
		for (int i = 0; i != FBBEffectorType::NumEffectors; ++i) {
			ResetFBBTarget((FBBEffectorType)i);
		}
	}
	void IKTargetFBBIKComponent::ResetFBBBendGoals() {
		for (int i = 0; i != FBBChainType::NumChains; ++i) {
			ResetFBBBendGoal((FBBChainType)i);
		}
	}
}

namespace Echo {

	void IKTargetFBBIKComponentInfo::CopyToCompData(IKBaseComponentData* pData) {
		if (!pData) return;
		IKBaseComponentInfo::CopyToCompData(pData);
		IKDATA(pData)->mIKInfo.ik = GetFullBodyBipedIKName();
		IKDATA(pData)->TargetPointCompNames[FBBEffectorType::Body] = GetBodyTargetPointCompName();
		IKDATA(pData)->TargetPointCompNames[FBBEffectorType::LeftShoulder] = GetLeftShoulderTargetPointCompName();
		IKDATA(pData)->TargetPointCompNames[FBBEffectorType::RightShoulder] = GetRightShoulderTargetPointCompName();
		IKDATA(pData)->TargetPointCompNames[FBBEffectorType::LeftThigh] = GetLeftThighTargetPointCompName();
		IKDATA(pData)->TargetPointCompNames[FBBEffectorType::RightThigh] = GetRightThighTargetPointCompName();
		IKDATA(pData)->TargetPointCompNames[FBBEffectorType::LeftHand] = GetLeftHandTargetPointCompName();
		IKDATA(pData)->TargetPointCompNames[FBBEffectorType::RightHand] = GetRightHandTargetPointCompName();
		IKDATA(pData)->TargetPointCompNames[FBBEffectorType::LeftFoot] = GetLeftFootTargetPointCompName();
		IKDATA(pData)->TargetPointCompNames[FBBEffectorType::RightFoot] = GetRightFootTargetPointCompName();

		IKDATA(pData)->BendGoalPointCompNames[FBBChainType::LeftArm] = GetLeftArmPointCompName();
		IKDATA(pData)->BendGoalPointCompNames[FBBChainType::LeftLeg] = GetLeftLegPointCompName();
		IKDATA(pData)->BendGoalPointCompNames[FBBChainType::RightArm] = GetRightArmPointCompName();
		IKDATA(pData)->BendGoalPointCompNames[FBBChainType::RightLeg] = GetRightLegPointCompName();
	}
	void IKTargetFBBIKComponentInfo::CompDataToInfo(IKBaseComponentData* pData, IKBaseComponent* _pComp) {
		if (!pData) return;
		IKBaseComponentInfo::CompDataToInfo(pData, _pComp);
		IKTargetFBBIKComponent* pComp = (IKTargetFBBIKComponent*)_pComp;
		SetFullBodyBipedIKName(IKDATA(pData)->mIKInfo.ik, pComp);
		SetBodyTargetPointCompName(IKDATA(pData)->TargetPointCompNames[FBBEffectorType::Body], pComp);
		SetLeftShoulderTargetPointCompName(IKDATA(pData)->TargetPointCompNames[FBBEffectorType::LeftShoulder], pComp);
		SetRightShoulderTargetPointCompName(IKDATA(pData)->TargetPointCompNames[FBBEffectorType::RightShoulder], pComp);
		SetLeftThighTargetPointCompName(IKDATA(pData)->TargetPointCompNames[FBBEffectorType::LeftThigh], pComp);
		SetRightThighTargetPointCompName(IKDATA(pData)->TargetPointCompNames[FBBEffectorType::RightThigh], pComp);
		SetLeftHandTargetPointCompName(IKDATA(pData)->TargetPointCompNames[FBBEffectorType::LeftHand], pComp);
		SetRightHandTargetPointCompName(IKDATA(pData)->TargetPointCompNames[FBBEffectorType::RightHand], pComp);
		SetLeftFootTargetPointCompName(IKDATA(pData)->TargetPointCompNames[FBBEffectorType::LeftFoot], pComp);
		SetRightFootTargetPointCompName(IKDATA(pData)->TargetPointCompNames[FBBEffectorType::RightFoot], pComp);

		SetLeftArmPointCompName(IKDATA(pData)->BendGoalPointCompNames[FBBChainType::LeftArm], pComp);
		SetLeftLegPointCompName(IKDATA(pData)->BendGoalPointCompNames[FBBChainType::LeftLeg], pComp);
		SetRightArmPointCompName(IKDATA(pData)->BendGoalPointCompNames[FBBChainType::RightArm], pComp);
		SetRightLegPointCompName(IKDATA(pData)->BendGoalPointCompNames[FBBChainType::RightLeg], pComp);
	}
	void IKTargetFBBIKComponent::SetCompData(IKBaseComponentData* pData) {
		if (!pData) return;
		IKBaseComponent::SetCompData(pData);
		for (int i = 0; i != FBBEffectorType::NumEffectors; ++i) {
			SetTargetPointCompName((FBBEffectorType)i, IKDATA(pData)->TargetPointCompNames[i]);
		}
		for (int i = 0; i != FBBChainType::NumChains; ++i) {
			SetBendGoalPointCompName((FBBChainType)i, IKDATA(pData)->BendGoalPointCompNames[i]);
		}
	}

	void IKTargetFBBIKComponent::SetIKInfoData(const TargetFBBIKInfo& info) {
		SetFullBodyBipedIKName(info.ik);
	}
	void IKTargetFBBIKComponent::UseDefaultInitialize() {
		SetFullBodyBipedIKName(Name::g_cEmptyName);
		for (int i = 0; i != FBBEffectorType::NumEffectors; ++i) {
			SetTargetPointCompName((FBBEffectorType)i, "");
		}
		for (int i = 0; i != FBBChainType::NumChains; ++i) {
			SetBendGoalPointCompName((FBBChainType)i, "");
		}
	}


}
