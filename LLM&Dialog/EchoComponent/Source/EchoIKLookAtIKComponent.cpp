#include "EchoComponentStableHeaders.h"
#include "EchoIKLookAtIKComponent.h"
#include "Effect/EchoEffectHolder.h"
#include "EchoActorProFuncImp.h"
#include "IK/EchoLookAtIK.h"
#include "EchoIKPointComponent.h"
#include "EchoIKTargetPointComponent.h"

namespace Echo {

	DECLAREACTORCOMTYPE_CPP(IKLookAtIKComponent)

#define IKDATA(_pData) static_cast<IKLookAtIKComponentData*>(_pData)
#define IKCOMDATA(_pData) static_cast<EchoIK::LookAtIK*>(_pData)

	EDITOR_REFLECT_BEGIN(IKLookAtIKComponentInfo)
	EDITOR_REFLECT_MEMBER_UPDATE(m_FixTransforms, "FixTransforms", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_Weight, "Weight", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_BodyWeight, "BodyWeight", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_HeadWeight, "HeadWeight", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_EyesWeight, "EyesWeight", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_ClampWeight, "ClampWeight", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_ClampWeightHead, "ClampWeightHead", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_ClampWeightEyes, "ClampWeightEyes", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_ClampSmoothing, "ClampSmoothing", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_Head, "Head", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_Spine, "Spine", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_Eyes, "Eyes", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_TargetNodeCompName, "TargetNodeCompName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_END();
	_DEFINEINFO_VIRFUN(IKLookAtIKComponentInfo, IKBaseComponentInfo)


	IKLookAtIKComponent::IKLookAtIKComponent(ActorPtr pActor, ActorCOMMoveableType moveable) : IKBaseComponent(pActor, moveable) {
		m_pCompData = new IKLookAtIKComponentData();
	}
	IKLookAtIKComponent::~IKLookAtIKComponent() { }
	void IKLookAtIKComponent::SetIKInfo(FinalIKInfoBase* pInfo) {
		if (!pInfo) return;
		SetIKInfoData(*(LookAtIKInfo*)pInfo);
	}

	bool IKLookAtIKComponent::IsCanCreate() {
		RoleIKComponent* pRoleIK = GetRoleIKComponent();
		if (!pRoleIK) return false;
		auto& spines = GetSpine();
		if (spines.empty()) return false;
		for (auto& iter : spines) {
			if (iter == Name::g_cEmptyName || pRoleIK->getBoneNodeByName(iter) == nullptr) {
				return false;
			}
		}
		auto& eyes = GetEyes();
		if (!eyes.empty()) {
			for (auto& iter : eyes) {
				if (iter == Name::g_cEmptyName || pRoleIK->getBoneNodeByName(iter) == nullptr) {
					return false;
				}
			}
		}
		const Name& head = GetHead();
		if (head == Name::g_cEmptyName || pRoleIK->getBoneNodeByName(head) == nullptr) {
			return false;
		}
		return true;
	}

	void IKLookAtIKComponent::SetFixTransforms(bool value){ 
		IKDATA(m_pCompData)->mIKInfo.fixTransforms = value; 
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->fixTransforms = value;
	} 
	bool IKLookAtIKComponent::GetFixTransforms(){ return IKDATA(m_pCompData)->mIKInfo.fixTransforms; }
	void IKLookAtIKComponent::SetWeight(float value){ 
		if (EchoIsNanOrInf(value)) return;
		IKDATA(m_pCompData)->mIKInfo.weight = value; 
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->setPositionWeight(value);
	} 
	float IKLookAtIKComponent::GetWeight(){ return IKDATA(m_pCompData)->mIKInfo.weight; }
	void IKLookAtIKComponent::SetBodyWeight(float value){ 
		if (EchoIsNanOrInf(value)) return;
		IKDATA(m_pCompData)->mIKInfo.bodyWeight = value; 
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->GetIKSolver()->bodyWeight = value;
	} 
	float IKLookAtIKComponent::GetBodyWeight(){ return IKDATA(m_pCompData)->mIKInfo.bodyWeight; }
	void IKLookAtIKComponent::SetHeadWeight(float value){ 
		if (EchoIsNanOrInf(value)) return;
		IKDATA(m_pCompData)->mIKInfo.headWeight = value; 
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->GetIKSolver()->headWeight = value;
	} 
	float IKLookAtIKComponent::GetHeadWeight(){ return IKDATA(m_pCompData)->mIKInfo.headWeight; }
	void IKLookAtIKComponent::SetEyesWeight(float value){ 
		if (EchoIsNanOrInf(value)) return;
		IKDATA(m_pCompData)->mIKInfo.eyesWeight = value; 
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->GetIKSolver()->eyesWeight = value;
	} 
	float IKLookAtIKComponent::GetEyesWeight(){ return IKDATA(m_pCompData)->mIKInfo.eyesWeight; }
	void IKLookAtIKComponent::SetClampWeight(float value){ 
		if (EchoIsNanOrInf(value)) return;
		IKDATA(m_pCompData)->mIKInfo.clampWeight = value; 
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->GetIKSolver()->clampWeight = value;
	} 
	float IKLookAtIKComponent::GetClampWeight(){ return IKDATA(m_pCompData)->mIKInfo.clampWeight; }
	void IKLookAtIKComponent::SetClampWeightHead(float value){ 
		if (EchoIsNanOrInf(value)) return;
		IKDATA(m_pCompData)->mIKInfo.clampWeightHead = value; 
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->GetIKSolver()->clampWeightHead = value;
	} 
	float IKLookAtIKComponent::GetClampWeightHead(){ return IKDATA(m_pCompData)->mIKInfo.clampWeightHead; }
	void IKLookAtIKComponent::SetClampWeightEyes(float value){ 
		if (EchoIsNanOrInf(value)) return;
		IKDATA(m_pCompData)->mIKInfo.clampWeightEyes = value; 
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->GetIKSolver()->clampWeightEyes = value;
	} 
	float IKLookAtIKComponent::GetClampWeightEyes(){ return IKDATA(m_pCompData)->mIKInfo.clampWeightEyes; }
	void IKLookAtIKComponent::SetClampSmoothing(int value){ 
		IKDATA(m_pCompData)->mIKInfo.clampSmoothing = value; 
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->GetIKSolver()->clampSmoothing = value;
	} 
	int IKLookAtIKComponent::GetClampSmoothing(){ return IKDATA(m_pCompData)->mIKInfo.clampSmoothing; }
	void IKLookAtIKComponent::SetHead(const Name& value){ 
		if (IsExistIK()) return;
		IKDATA(m_pCompData)->mIKInfo.head = value; 
	} 
	const Name& IKLookAtIKComponent::GetHead(){ return IKDATA(m_pCompData)->mIKInfo.head; }
	void IKLookAtIKComponent::SetSpine(const std::vector<Name>& value){ 
		if (IsExistIK()) return;
		IKDATA(m_pCompData)->mIKInfo.spine = value; 
	} 
	const std::vector<Name>& IKLookAtIKComponent::GetSpine(){ return IKDATA(m_pCompData)->mIKInfo.spine; }
	void IKLookAtIKComponent::SetEyes(const std::vector<Name>& value){ 
		if (IsExistIK()) return; 
		IKDATA(m_pCompData)->mIKInfo.eyes = value; 
	} 
	const std::vector<Name>& IKLookAtIKComponent::GetEyes(){ return IKDATA(m_pCompData)->mIKInfo.eyes; }

	void IKLookAtIKComponent::SetTargetNodeCompName(const String& value) {
		IKDATA(m_pCompData)->TargetNodeCompName = value;
		ActorComponentPtr compPtr = FindComponent(value);
		SetTargetNode(compPtr);
	}
	const String& IKLookAtIKComponent::GetTargetNodeCompName() {
		return IKDATA(m_pCompData)->TargetNodeCompName;
	}

	void IKLookAtIKComponent::SetTargetNode(const ActorComponentPtr& pComp) {
		mTargetPtr = pComp;
		if (IsExistIK()) {
			ApplyTargetPtr();
		}
	}
	void IKLookAtIKComponent::SetTargetNode(WorldNode* pNode) {
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->setTargetNode(pNode, Name::g_cEmptyName);
	}
	void IKLookAtIKComponent::PostCreateIK() {
		ApplyTargetPtr();
	}

	void IKLookAtIKComponent::ApplyTargetPtr() {
		if (mTargetPtr == nullptr) return;
		IKCOMDATA(m_pIKComponent)->setTargetNode(mTargetPtr, getTagPointNameByTargetComponent(mTargetPtr));
	}
}

namespace Echo {

	void IKLookAtIKComponent::SetIKInfoData(const LookAtIKInfo& info) {
		SetFixTransforms(info.fixTransforms);
		SetWeight(info.weight);
		SetBodyWeight(info.bodyWeight);
		SetHeadWeight(info.headWeight);
		SetEyesWeight(info.eyesWeight);
		SetClampWeight(info.clampWeight);
		SetClampWeightHead(info.clampWeightHead);
		SetClampWeightEyes(info.clampWeightEyes);
		SetClampSmoothing(info.clampSmoothing);
		SetHead(info.head);
		SetSpine(info.spine);
		SetEyes(info.eyes);
	}
	const LookAtIKInfo& IKLookAtIKComponent::GetIKInfoData() {
		return IKDATA(m_pCompData)->mIKInfo;
	}
	void IKLookAtIKComponent::UseDefaultInitialize() {
		SetFixTransforms(false);
		SetWeight(1.0f);
		SetBodyWeight(1.0f);
		SetHeadWeight(1.0f);
		SetEyesWeight(1.0f);
		SetClampWeight(0.0f);
		SetClampWeightHead(0.0f);
		SetClampWeightEyes(0.0f);
		SetClampSmoothing(2);
		Name newHead("Bip001 Head");
		SetHead(newHead);
		std::vector<Name> newSpine;
		newSpine.push_back(Name("Bip001 Spine1"));
		newSpine.push_back(Name("Bip001 Spine2"));
		newSpine.push_back(Name("Bip001 Neck"));
		SetSpine(newSpine);
		std::vector<Name> newEyes;
		SetEyes(newEyes);
		SetTargetNodeCompName("");
	}

	void IKLookAtIKComponent::SetCompData(IKBaseComponentData* pData) {
		if (!pData) return;
		IKBaseComponent::SetCompData(pData);
		SetTargetNodeCompName(IKDATA(pData)->TargetNodeCompName);
	}


	void IKLookAtIKComponentInfo::CopyToCompData(IKBaseComponentData* pData) {
		if (!pData) return;
		IKBaseComponentInfo::CopyToCompData(pData);
		IKDATA(pData)->mIKInfo.fixTransforms = GetFixTransforms();
		IKDATA(pData)->mIKInfo.weight = GetWeight();
		IKDATA(pData)->mIKInfo.bodyWeight = GetBodyWeight();
		IKDATA(pData)->mIKInfo.headWeight = GetHeadWeight();
		IKDATA(pData)->mIKInfo.eyesWeight = GetEyesWeight();
		IKDATA(pData)->mIKInfo.clampWeight = GetClampWeight();
		IKDATA(pData)->mIKInfo.clampWeightHead = GetClampWeightHead();
		IKDATA(pData)->mIKInfo.clampWeightEyes = GetClampWeightEyes();
		IKDATA(pData)->mIKInfo.clampSmoothing = GetClampSmoothing();
		IKDATA(pData)->mIKInfo.head = GetHead();
		IKDATA(pData)->mIKInfo.spine = GetSpine();
		IKDATA(pData)->mIKInfo.eyes = GetEyes();
		IKDATA(pData)->TargetNodeCompName = GetTargetNodeCompName();

	}
	void IKLookAtIKComponentInfo::CompDataToInfo(IKBaseComponentData* pData, IKBaseComponent* _pComp) {
		if (!pData) return;
		IKBaseComponentInfo::CompDataToInfo(pData, _pComp);
		IKLookAtIKComponent* pComp = (IKLookAtIKComponent*)_pComp;
		SetFixTransforms(IKDATA(pData)->mIKInfo.fixTransforms, pComp);
		SetWeight(IKDATA(pData)->mIKInfo.weight, pComp);
		SetBodyWeight(IKDATA(pData)->mIKInfo.bodyWeight, pComp);
		SetHeadWeight(IKDATA(pData)->mIKInfo.headWeight, pComp);
		SetEyesWeight(IKDATA(pData)->mIKInfo.eyesWeight, pComp);
		SetClampWeight(IKDATA(pData)->mIKInfo.clampWeight, pComp);
		SetClampWeightHead(IKDATA(pData)->mIKInfo.clampWeightHead, pComp);
		SetClampWeightEyes(IKDATA(pData)->mIKInfo.clampWeightEyes, pComp);
		SetClampSmoothing(IKDATA(pData)->mIKInfo.clampSmoothing, pComp);
		SetHead(IKDATA(pData)->mIKInfo.head, pComp);
		SetSpine(IKDATA(pData)->mIKInfo.spine, pComp);
		SetEyes(IKDATA(pData)->mIKInfo.eyes, pComp);
		SetTargetNodeCompName(IKDATA(pData)->TargetNodeCompName, pComp);
	}

}
