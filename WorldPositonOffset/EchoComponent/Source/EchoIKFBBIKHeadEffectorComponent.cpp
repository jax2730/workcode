#include "EchoComponentStableHeaders.h"
#include "EchoIKFBBIKHeadEffectorComponent.h"
#include "Effect/EchoEffectHolder.h"
#include "EchoActorProFuncImp.h"
#include "IK/EchoFBBIKHeadEffector.h"
#include "EchoIKTargetPointComponent.h"
#include "IK/EchoFullBodyBipedIK.h"

namespace Echo {

	DECLAREACTORCOMTYPE_CPP(IKFBBIKHeadEffectorComponent)

#define IKDATA(_pData) static_cast<IKFBBIKHeadEffectorComponentData*>(_pData)
#define IKCOMDATA(_pData) static_cast<EchoIK::FBBIKHeadEffector*>(_pData)

		EDITOR_REFLECT_BEGIN(IKFBBIKHeadEffectorComponentInfo)
		EDITOR_REFLECT_MEMBER_UPDATE(m_PositionWeight, "PositionWeight", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_BodyWeight, "BodyWeight", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_ThighWeight, "ThighWeight", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_RotationWeight, "RotationWeight", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_BodyClampWeight, "BodyClampWeight", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_HeadClampWeight, "HeadClampWeight", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_BendWeight, "BendWeight", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_CCDWeight, "CCDWeight", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_Roll, "Roll", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_Damper, "Damper", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_PostStretchWeight, "PostStretchWeight", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_MaxStretch, "MaxStretch", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_StretchDamper, "StretchDamper", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_ChestDirectionWeight, "ChestDirectionWeight", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_HandsPullBody, "HandsPullBody", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_FixHead, "FixHead", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_FullBodyBipedIKName, "FullBodyBipedIKName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_BendBones, "BendBones", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_CCDBones, "CCDBones", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_StretchBones, "StretchBones", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_ChestDirection, "ChestDirection", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_ChestBones, "ChestBones", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_TargetNodeCompName, "TargetNodeCompName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_END();
	_DEFINEINFO_VIRFUN(IKFBBIKHeadEffectorComponentInfo, IKBaseComponentInfo)


	
	IKFBBIKHeadEffectorComponent::IKFBBIKHeadEffectorComponent(ActorPtr pActor, ActorCOMMoveableType moveable) : IKBaseComponent(pActor, moveable) {
		m_pCompData = new IKFBBIKHeadEffectorComponentData();
	}
	IKFBBIKHeadEffectorComponent::~IKFBBIKHeadEffectorComponent() {}
	void IKFBBIKHeadEffectorComponent::SetIKInfo(FinalIKInfoBase* pInfo) {
		if (!pInfo) return;
		SetIKInfoData(*(FBBIKHeadEffectorInfo*)pInfo);
	}
	const FBBIKHeadEffectorInfo& IKFBBIKHeadEffectorComponent::GetIKInfoData() {
		return IKDATA(m_pCompData)->mIKInfo;
	}

	bool IKFBBIKHeadEffectorComponent::IsCanCreate() {
		RoleIKComponent* pRoleIK = GetRoleIKComponent();
		if (!pRoleIK) return false;
		auto& bones = GetCCDBones();
		for (auto& iter : bones) {
			if (iter == Name::g_cEmptyName || pRoleIK->getBoneNodeByName(iter) == nullptr) {
				return false;
			}
		}
		auto& bbones = GetBendBones();
		for (auto& iter : bbones) {
			if (iter.transform == Name::g_cEmptyName || pRoleIK->getBoneNodeByName(iter.transform) == nullptr) {
				return false;
			}
		}
		auto& sbones = GetStretchBones();
		for (auto& iter : sbones) {
			if (iter == Name::g_cEmptyName || pRoleIK->getBoneNodeByName(iter) == nullptr) {
				return false;
			}
		}
		auto& cbones = GetChestBones();
		for (auto& iter : cbones) {
			if (iter == Name::g_cEmptyName || pRoleIK->getBoneNodeByName(iter) == nullptr) {
				return false;
			}
		}
		return CheckValidFBBIK(IKDATA(m_pCompData)->mIKInfo.ik);
	}

	void IKFBBIKHeadEffectorComponent::SetPositionWeight(float value){ 
		IKDATA(m_pCompData)->mIKInfo.positionWeight = value; 
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->positionWeight = value;
	} 
	float IKFBBIKHeadEffectorComponent::GetPositionWeight(){ return IKDATA(m_pCompData)->mIKInfo.positionWeight; }
	void IKFBBIKHeadEffectorComponent::SetBodyWeight(float value){ 
		IKDATA(m_pCompData)->mIKInfo.bodyWeight = value; 
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->bodyWeight = value;
	} 
	float IKFBBIKHeadEffectorComponent::GetBodyWeight(){ return IKDATA(m_pCompData)->mIKInfo.bodyWeight; }
	void IKFBBIKHeadEffectorComponent::SetThighWeight(float value){ 
		IKDATA(m_pCompData)->mIKInfo.thighWeight = value; 
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->thighWeight = value;
	} 
	float IKFBBIKHeadEffectorComponent::GetThighWeight(){ return IKDATA(m_pCompData)->mIKInfo.thighWeight; }
	void IKFBBIKHeadEffectorComponent::SetRotationWeight(float value){ 
		IKDATA(m_pCompData)->mIKInfo.rotationWeight = value; 
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->rotationWeight = value;
	} 
	float IKFBBIKHeadEffectorComponent::GetRotationWeight(){ return IKDATA(m_pCompData)->mIKInfo.rotationWeight; }
	void IKFBBIKHeadEffectorComponent::SetBodyClampWeight(float value){ 
		IKDATA(m_pCompData)->mIKInfo.bodyClampWeight = value; 
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->bodyClampWeight = value;
	} 
	float IKFBBIKHeadEffectorComponent::GetBodyClampWeight(){ return IKDATA(m_pCompData)->mIKInfo.bodyClampWeight; }
	void IKFBBIKHeadEffectorComponent::SetHeadClampWeight(float value){ 
		IKDATA(m_pCompData)->mIKInfo.headClampWeight = value; 
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->headClampWeight = value;
	} 
	float IKFBBIKHeadEffectorComponent::GetHeadClampWeight(){ return IKDATA(m_pCompData)->mIKInfo.headClampWeight; }
	void IKFBBIKHeadEffectorComponent::SetBendWeight(float value){ 
		IKDATA(m_pCompData)->mIKInfo.bendWeight = value; 
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->bendWeight = value;
	} 
	float IKFBBIKHeadEffectorComponent::GetBendWeight(){ return IKDATA(m_pCompData)->mIKInfo.bendWeight; }
	void IKFBBIKHeadEffectorComponent::SetCCDWeight(float value){ 
		IKDATA(m_pCompData)->mIKInfo.CCDWeight = value; 
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->CCDWeight = value;
	} 
	float IKFBBIKHeadEffectorComponent::GetCCDWeight(){ return IKDATA(m_pCompData)->mIKInfo.CCDWeight; }
	void IKFBBIKHeadEffectorComponent::SetRoll(float value){ 
		IKDATA(m_pCompData)->mIKInfo.roll = value; 
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->roll = value;
	} 
	float IKFBBIKHeadEffectorComponent::GetRoll(){ return IKDATA(m_pCompData)->mIKInfo.roll; }
	void IKFBBIKHeadEffectorComponent::SetDamper(float value){ 
		IKDATA(m_pCompData)->mIKInfo.damper = value; 
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->damper = value;
	} 
	float IKFBBIKHeadEffectorComponent::GetDamper(){ return IKDATA(m_pCompData)->mIKInfo.damper; }
	void IKFBBIKHeadEffectorComponent::SetPostStretchWeight(float value){ 
		IKDATA(m_pCompData)->mIKInfo.postStretchWeight = value; 
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->postStretchWeight = value;
	} 
	float IKFBBIKHeadEffectorComponent::GetPostStretchWeight(){ return IKDATA(m_pCompData)->mIKInfo.postStretchWeight; }
	void IKFBBIKHeadEffectorComponent::SetMaxStretch(float value){ 
		IKDATA(m_pCompData)->mIKInfo.maxStretch = value; 
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->maxStretch = value;
	} 
	float IKFBBIKHeadEffectorComponent::GetMaxStretch(){ return IKDATA(m_pCompData)->mIKInfo.maxStretch; }
	void IKFBBIKHeadEffectorComponent::SetStretchDamper(float value){ 
		IKDATA(m_pCompData)->mIKInfo.stretchDamper = value; 
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->stretchDamper = value;
	} 
	float IKFBBIKHeadEffectorComponent::GetStretchDamper(){ return IKDATA(m_pCompData)->mIKInfo.stretchDamper; }
	void IKFBBIKHeadEffectorComponent::SetChestDirectionWeight(float value){ 
		IKDATA(m_pCompData)->mIKInfo.chestDirectionWeight = value;
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->chestDirectionWeight = value;
	} 
	float IKFBBIKHeadEffectorComponent::GetChestDirectionWeight(){ return IKDATA(m_pCompData)->mIKInfo.chestDirectionWeight; }
	void IKFBBIKHeadEffectorComponent::SetHandsPullBody(bool value){ 
		IKDATA(m_pCompData)->mIKInfo.handsPullBody = value; 
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->handsPullBody = value;
	} 
	bool IKFBBIKHeadEffectorComponent::GetHandsPullBody(){ return IKDATA(m_pCompData)->mIKInfo.handsPullBody; }
	void IKFBBIKHeadEffectorComponent::SetFixHead(bool value){ 
		IKDATA(m_pCompData)->mIKInfo.fixHead = value; 
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->fixHead = value;
	} 
	bool IKFBBIKHeadEffectorComponent::GetFixHead(){ return IKDATA(m_pCompData)->mIKInfo.fixHead; }
	void IKFBBIKHeadEffectorComponent::SetFullBodyBipedIKName(const Name& value){ 
		if (IsExistIK()) return;
		IKDATA(m_pCompData)->mIKInfo.ik = value; 
	} 
	const Name& IKFBBIKHeadEffectorComponent::GetFullBodyBipedIKName(){ return IKDATA(m_pCompData)->mIKInfo.ik; }
	void IKFBBIKHeadEffectorComponent::SetBendBones(const std::vector<NodeWeightInfo>& value){ 
		if (!IsExistIK()) {
			IKDATA(m_pCompData)->mIKInfo.bendBones = value;
		}
		else {
			if (IKDATA(m_pCompData)->mIKInfo.bendBones.size() != value.size()) return;
			for (int i = 0; i != value.size(); ++i) {
				if (IKDATA(m_pCompData)->mIKInfo.bendBones[i].transform != value[i].transform) return;
			}
			for (int i = 0; i != value.size(); ++i) {
				IKDATA(m_pCompData)->mIKInfo.bendBones[i].weight = value[i].weight;
				IKCOMDATA(m_pIKComponent)->bendBones[i]->weight = value[i].weight;
			}
		}
	} 
	const std::vector<NodeWeightInfo>& IKFBBIKHeadEffectorComponent::GetBendBones(){ return IKDATA(m_pCompData)->mIKInfo.bendBones; }
	void IKFBBIKHeadEffectorComponent::SetCCDBones(const std::vector<Name>& value){ 
		if (IsExistIK()) return;
		IKDATA(m_pCompData)->mIKInfo.CCDBones = value; 
	} 
	const std::vector<Name>& IKFBBIKHeadEffectorComponent::GetCCDBones(){ return IKDATA(m_pCompData)->mIKInfo.CCDBones; }
	void IKFBBIKHeadEffectorComponent::SetStretchBones(const std::vector<Name>& value){ 
		if (IsExistIK()) return;
		IKDATA(m_pCompData)->mIKInfo.stretchBones = value; 
	} 
	const std::vector<Name>& IKFBBIKHeadEffectorComponent::GetStretchBones(){ return IKDATA(m_pCompData)->mIKInfo.stretchBones; }
	void IKFBBIKHeadEffectorComponent::SetChestDirection(const Vector3& value){ 
		IKDATA(m_pCompData)->mIKInfo.chestDirection = value; 
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->chestDirection = value;
	} 
	const Vector3& IKFBBIKHeadEffectorComponent::GetChestDirection(){ return IKDATA(m_pCompData)->mIKInfo.chestDirection; }
	void IKFBBIKHeadEffectorComponent::SetChestBones(const std::vector<Name>& value){ 
		if (IsExistIK()) return;
		IKDATA(m_pCompData)->mIKInfo.chestBones = value; 
	} 
	const std::vector<Name>& IKFBBIKHeadEffectorComponent::GetChestBones(){ return IKDATA(m_pCompData)->mIKInfo.chestBones; }

	void IKFBBIKHeadEffectorComponent::SetTargetNodeCompName(const String& value) {
		IKDATA(m_pCompData)->TargetNodeCompName = value;
		ActorComponentPtr compPtr = FindComponent(value);
		SetWorldTargetNode(compPtr);
	}
	const String& IKFBBIKHeadEffectorComponent::GetTargetNodeCompName() {
		return IKDATA(m_pCompData)->TargetNodeCompName;
	}
	void IKFBBIKHeadEffectorComponent::SetWorldTargetNode(const ActorComponentPtr& pComp) {
		mTargetPtr = pComp;
		if (IsExistIK()) {
			ApplyTargetPtr();
		}
	}

	void IKFBBIKHeadEffectorComponent::SetWorldTargetNode(WorldNode* pNode) {
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->setWorldTargetNode(pNode);
	}

	EchoIK::FullBodyBipedIK* IKFBBIKHeadEffectorComponent::GetFullBodyBipedIK() {
		if (!IsExistIK()) return nullptr;
		return IKCOMDATA(m_pIKComponent)->ik;
	}

	void IKFBBIKHeadEffectorComponent::PostCreateIK() {
		ApplyTargetPtr();
	}

	void IKFBBIKHeadEffectorComponent::ApplyTargetPtr() {
		if (mTargetPtr == nullptr) return;
		IKCOMDATA(m_pIKComponent)->setWorldTargetNode(mTargetPtr);
	}
}

namespace Echo {

	void IKFBBIKHeadEffectorComponent::SetCompData(IKBaseComponentData* pData) {
		if (!pData) return;
		IKBaseComponent::SetCompData(pData);
		SetTargetNodeCompName(IKDATA(pData)->TargetNodeCompName);
	}
	void IKFBBIKHeadEffectorComponentInfo::CopyToCompData(IKBaseComponentData* pData) {
		if (!pData) return;
		IKBaseComponentInfo::CopyToCompData(pData);

		IKDATA(pData)->mIKInfo.positionWeight = GetPositionWeight();
		IKDATA(pData)->mIKInfo.bodyWeight = GetBodyWeight();
		IKDATA(pData)->mIKInfo.thighWeight = GetThighWeight();
		IKDATA(pData)->mIKInfo.rotationWeight = GetRotationWeight();
		IKDATA(pData)->mIKInfo.bodyClampWeight = GetBodyClampWeight();
		IKDATA(pData)->mIKInfo.headClampWeight = GetHeadClampWeight();
		IKDATA(pData)->mIKInfo.bendWeight = GetBendWeight();
		IKDATA(pData)->mIKInfo.CCDWeight = GetCCDWeight();
		IKDATA(pData)->mIKInfo.roll = GetRoll();
		IKDATA(pData)->mIKInfo.damper = GetDamper();
		IKDATA(pData)->mIKInfo.postStretchWeight = GetPostStretchWeight();
		IKDATA(pData)->mIKInfo.maxStretch = GetMaxStretch();
		IKDATA(pData)->mIKInfo.stretchDamper = GetStretchDamper();
		IKDATA(pData)->mIKInfo.chestDirectionWeight = GetChestDirectionWeight();
		IKDATA(pData)->mIKInfo.handsPullBody = GetHandsPullBody();
		IKDATA(pData)->mIKInfo.fixHead = GetFixHead();
		IKDATA(pData)->mIKInfo.ik = GetFullBodyBipedIKName();
		IKDATA(pData)->mIKInfo.bendBones = GetBendBones();
		IKDATA(pData)->mIKInfo.CCDBones = GetCCDBones();
		IKDATA(pData)->mIKInfo.stretchBones = GetStretchBones();
		IKDATA(pData)->mIKInfo.chestDirection = GetChestDirection();
		IKDATA(pData)->mIKInfo.chestBones = GetChestBones();

		IKDATA(pData)->TargetNodeCompName = GetTargetNodeCompName();
	}

	void IKFBBIKHeadEffectorComponentInfo::CompDataToInfo(IKBaseComponentData* pData, IKBaseComponent* _pComp) {
		if (!pData) return;
		IKBaseComponentInfo::CompDataToInfo(pData, _pComp);
		IKFBBIKHeadEffectorComponent* pComp = (IKFBBIKHeadEffectorComponent*)_pComp;
		SetPositionWeight(IKDATA(pData)->mIKInfo.positionWeight, pComp);
		SetBodyWeight(IKDATA(pData)->mIKInfo.bodyWeight, pComp);
		SetThighWeight(IKDATA(pData)->mIKInfo.thighWeight, pComp);
		SetRotationWeight(IKDATA(pData)->mIKInfo.rotationWeight, pComp);
		SetBodyClampWeight(IKDATA(pData)->mIKInfo.bodyClampWeight, pComp);
		SetHeadClampWeight(IKDATA(pData)->mIKInfo.headClampWeight, pComp);
		SetBendWeight(IKDATA(pData)->mIKInfo.bendWeight, pComp);
		SetCCDWeight(IKDATA(pData)->mIKInfo.CCDWeight, pComp);
		SetRoll(IKDATA(pData)->mIKInfo.roll, pComp);
		SetDamper(IKDATA(pData)->mIKInfo.damper, pComp);
		SetPostStretchWeight(IKDATA(pData)->mIKInfo.postStretchWeight, pComp);
		SetMaxStretch(IKDATA(pData)->mIKInfo.maxStretch, pComp);
		SetStretchDamper(IKDATA(pData)->mIKInfo.stretchDamper, pComp);
		SetChestDirectionWeight(IKDATA(pData)->mIKInfo.chestDirectionWeight, pComp);
		SetHandsPullBody(IKDATA(pData)->mIKInfo.handsPullBody, pComp);
		SetFixHead(IKDATA(pData)->mIKInfo.fixHead, pComp);
		SetFullBodyBipedIKName(IKDATA(pData)->mIKInfo.ik, pComp);
		SetBendBones(IKDATA(pData)->mIKInfo.bendBones, pComp);
		SetCCDBones(IKDATA(pData)->mIKInfo.CCDBones, pComp);
		SetStretchBones(IKDATA(pData)->mIKInfo.stretchBones, pComp);
		SetChestDirection(IKDATA(pData)->mIKInfo.chestDirection, pComp);
		SetChestBones(IKDATA(pData)->mIKInfo.chestBones, pComp);

		SetTargetNodeCompName(IKDATA(pData)->TargetNodeCompName, pComp);
	}

	void IKFBBIKHeadEffectorComponent::SetIKInfoData(const FBBIKHeadEffectorInfo& info) {
		SetPositionWeight(info.positionWeight);
		SetBodyWeight(info.bodyWeight);
		SetThighWeight(info.thighWeight);
		SetRotationWeight(info.rotationWeight);
		SetBodyClampWeight(info.bodyClampWeight);
		SetHeadClampWeight(info.headClampWeight);
		SetBendWeight(info.bendWeight);
		SetCCDWeight(info.CCDWeight);
		SetRoll(info.roll);
		SetDamper(info.damper);
		SetPostStretchWeight(info.postStretchWeight);
		SetMaxStretch(info.maxStretch);
		SetStretchDamper(info.stretchDamper);
		SetChestDirectionWeight(info.chestDirectionWeight);
		SetHandsPullBody(info.handsPullBody);
		SetFixHead(info.fixHead);
		SetFullBodyBipedIKName(info.ik);
		SetBendBones(info.bendBones);
		SetCCDBones(info.CCDBones);
		SetStretchBones(info.stretchBones);
		SetChestDirection(info.chestDirection);
		SetChestBones(info.chestBones);
	}
	void IKFBBIKHeadEffectorComponent::UseDefaultInitialize() {

		SetPositionWeight(1.0f);
		SetBodyWeight(1.0f);
		SetThighWeight(1.0f);
		SetHandsPullBody(true);
		SetRotationWeight(1.0f);
		SetBodyClampWeight(0.0f);
		SetHeadClampWeight(0.0f);
		SetBendWeight(1.0f);
		SetCCDWeight(0.0f);
		SetRoll(0.0f);
		SetDamper(300.0f);
		SetPostStretchWeight(1.0f);
		SetMaxStretch(1.0f);
		SetStretchDamper(0.0f);
		SetFixHead(false);
		SetChestDirection({ 0,0,1 });
		SetChestDirectionWeight(1.0f);

		SetFullBodyBipedIKName(Name::g_cEmptyName);

		std::vector<Name> newCCDBones(3);
		newCCDBones[0] = Name("Bip001 Spine1");
		newCCDBones[1] = Name("Bip001 Spine2");
		newCCDBones[2] = Name("Bip001 Neck");
		SetCCDBones(newCCDBones);

		std::vector<NodeWeightInfo> newBendBones(3);
		newBendBones[0].transform = Name("Bip001 Spine1");
		newBendBones[1].transform = Name("Bip001 Spine2");
		newBendBones[2].transform = Name("Bip001 Neck");
		newBendBones[0].weight = 1.0f;
		newBendBones[1].weight = 1.0f;
		newBendBones[2].weight = 1.0f;
		SetBendBones(newBendBones);

		std::vector<Name> nullBones;
		SetChestBones(nullBones);
		SetStretchBones(nullBones);

		SetTargetNodeCompName(String(""));
	}
}
