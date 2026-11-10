#include "EchoComponentStableHeaders.h"
#include "EchoIKPenetrationAvoidanceComponent.h"
#include "IK/EchoPenetrationAvoidance.h"

namespace Echo {

	static const std::vector<Echo::Avoider>& GetDefaultAvoiders()
	{
		static std::vector<Avoider> dea;
		static bool init = false;
		if (init == true) {
			return dea;
		}

		{//0
			Avoider avo;
			EffectorLinkInfo eff;

			eff.effector = EchoIK::FullBodyBipedEffector::LeftHand;
			eff.weight = 1.f;
			avo.effectors.push_back(eff);
			eff.effector = EchoIK::FullBodyBipedEffector::LeftShoulder;
			eff.weight = 0.4f;
			avo.effectors.push_back(eff);
			eff.effector = EchoIK::FullBodyBipedEffector::RightShoulder;
			eff.weight = 0.1f;
			avo.effectors.push_back(eff);
			eff.effector = EchoIK::FullBodyBipedEffector::Body;
			eff.weight = 0.2f;
			avo.effectors.push_back(eff);

			avo.raycastFromNameVec.push_back(Name{ "Bip001 Pelvis" });
			avo.raycastToName = Name{ "Bip001 L Hand" };
			avo.layers = 6;
			avo.smoothTimeOut = 0.3f;
			avo.smoothTimeIn = 0.03f;
			avo.raycastRadius = 0.1f;

			dea.push_back(avo);
		}
		{//1
			Avoider avo;
			EffectorLinkInfo eff;

			eff.effector = EchoIK::FullBodyBipedEffector::RightHand;
			eff.weight = 1.f;
			avo.effectors.push_back(eff);
			eff.effector = EchoIK::FullBodyBipedEffector::RightShoulder;
			eff.weight = 0.4f;
			avo.effectors.push_back(eff);
			eff.effector = EchoIK::FullBodyBipedEffector::LeftShoulder;
			eff.weight = 0.1f;
			avo.effectors.push_back(eff);
			eff.effector = EchoIK::FullBodyBipedEffector::Body;
			eff.weight = 0.2f;
			avo.effectors.push_back(eff);

			avo.raycastFromNameVec.push_back(Name{ "Bip001 Pelvis" });
			avo.raycastToName = Name{ "Bip001 R Hand" };
			avo.layers = 6;
			avo.smoothTimeOut = 0.3f;
			avo.smoothTimeIn = 0.03f;
			avo.raycastRadius = 0.1f;

			dea.push_back(avo);
		}
		{//2
			Avoider avo;
			EffectorLinkInfo eff;

			eff.effector = EchoIK::FullBodyBipedEffector::LeftFoot;
			eff.weight = 1.f;
			avo.effectors.push_back(eff);

			avo.raycastFromNameVec.push_back(Name{ "Bip001" });
			avo.raycastToName = Name{ "Bip001 L Foot" };
			avo.layers = 6;
			avo.smoothTimeOut = 0.3f;
			avo.smoothTimeIn = 0.03f;
			avo.raycastRadius = 0.1f;

			dea.push_back(avo);
		}
		{
			Avoider avo;
			EffectorLinkInfo eff;

			eff.effector = EchoIK::FullBodyBipedEffector::RightFoot;
			eff.weight = 1.f;
			avo.effectors.push_back(eff);

			avo.raycastFromNameVec.push_back(Name{ "Bip001" });
			avo.raycastToName = Name{ "Bip001 R Foot" };
			avo.layers = 6;
			avo.smoothTimeOut = 0.3f;
			avo.smoothTimeIn = 0.03f;
			avo.raycastRadius = 0.1f;

			dea.push_back(avo);
		}
		init = true;
		return dea;
	}

	static void DataToIK_Global(std::vector<EchoIK::PenetrationAvoidance::Avoider>& ikAvoiders, const std::vector<Avoider>& infoAvoiders)
	{
		ikAvoiders.resize(infoAvoiders.size());
		for (int i = 0; i < ikAvoiders.size(); ++i) {
			//Raycast from
			ikAvoiders[i].raycastFromName = infoAvoiders[i].raycastFromNameVec;

			//Raycast to
			ikAvoiders[i].raycastToName = infoAvoiders[i].raycastToName;

			ikAvoiders[i].raycastRadius = infoAvoiders[i].raycastRadius;

			ikAvoiders[i].effectors.resize(infoAvoiders[i].effectors.size());
			for (int j = 0; j < ikAvoiders[i].effectors.size(); ++j) {
				ikAvoiders[i].effectors[j].effector = EchoIK::FullBodyBipedEffector(infoAvoiders[i].effectors[j].effector);
				ikAvoiders[i].effectors[j].weight = infoAvoiders[i].effectors[j].weight;
			}

			ikAvoiders[i].smoothTimeIn = infoAvoiders[i].smoothTimeIn;
			ikAvoiders[i].smoothTimeOut = infoAvoiders[i].smoothTimeOut;
			ikAvoiders[i].layers = infoAvoiders[i].layers;
		}
	}


}

namespace Echo
{
	DECLAREACTORCOMTYPE_CPP(IKPenetrationAvoidanceComponent)
		EDITOR_REFLECT_BEGIN(IKPenetrationAvoidanceComponentInfo)
		EDITOR_REFLECT_MEMBER_UPDATE(m_FullBodyBipedIKName, "FullBodyBipedIKName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_Weight,"weight", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_Avoider,"avoiders", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_END();
	_DEFINEINFO_VIRFUN(IKPenetrationAvoidanceComponentInfo, IKBaseComponentInfo);

#define IKDATA(ptr) static_cast<IKPenetrationAvoidanceComponentData*>(ptr)
#define IKCOMDATA(ptr) static_cast<EchoIK::PenetrationAvoidance*>(ptr)

	Echo::IKPenetrationAvoidanceComponent::IKPenetrationAvoidanceComponent(Echo::ActorPtr pActor, Echo::ActorCOMMoveableType movableType)
		:IKBaseComponent(pActor, movableType)
	{
		m_pCompData = new IKPenetrationAvoidanceComponentData();
	}
	
	Echo::IKPenetrationAvoidanceComponent::~IKPenetrationAvoidanceComponent(){ }

	
	void Echo::IKPenetrationAvoidanceComponent::SetIKInfo(FinalIKInfoBase* pInfo)
	{
		if (pInfo == nullptr) return;
		SetIKInfoData(*(PenetrationAvoidanceInfo*)pInfo);
	}
	
	bool IKPenetrationAvoidanceComponent::IsCanCreate() {
		RoleIKComponent* pRoleIK = GetRoleIKComponent();
		if (!pRoleIK) return false;
		auto& avoiders = GetAvoider();
		if (avoiders.empty()) return false;
		for (auto& iter : avoiders) {
			if (iter.raycastToName == Name::g_cEmptyName || pRoleIK->getBoneNodeByName(iter.raycastToName) == nullptr) {
				return false;
			}
			for (auto& iter2 : iter.raycastFromNameVec) {
				if (iter2 == Name::g_cEmptyName || pRoleIK->getBoneNodeByName(iter2) == nullptr) {
					return false;
				}
			}
		}
		return CheckValidFBBIK(IKDATA(m_pCompData)->mIKInfo.ik);
	}
	
	const Echo::PenetrationAvoidanceInfo& Echo::IKPenetrationAvoidanceComponent::GetIKInfoData(){
		return IKDATA(m_pCompData)->mIKInfo;
	}
	
	const std::vector<Avoider>& Echo::IKPenetrationAvoidanceComponent::GetAvoider(){
		return IKDATA(m_pCompData)->mIKInfo.avoiders;
	}
	
	void Echo::IKPenetrationAvoidanceComponent::SetAvoider(const std::vector<Avoider>& value){
		IKDATA(m_pCompData)->mIKInfo.avoiders = value;
		if (!IsExistIK()) return;
		DataToIK_Global(IKCOMDATA(m_pIKComponent)->avoiders, IKDATA(m_pCompData)->mIKInfo.avoiders);
		IKCOMDATA(m_pIKComponent)->loadRaycastFrom();
		IKCOMDATA(m_pIKComponent)->loadRaycastTo();
	}
	
	float IKPenetrationAvoidanceComponent::GetWeight(){
		return IKDATA(m_pCompData)->mIKInfo.weight;
	}

	void IKPenetrationAvoidanceComponent::SetWeight(float value)
	{
		IKDATA(m_pCompData)->mIKInfo.weight = value;
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->weight = value;
	}

	void IKPenetrationAvoidanceComponent::SetFullBodyBipedIKName(const Name& ikName) {
		if (IsExistIK()) return;
		IKDATA(m_pCompData)->mIKInfo.ik = ikName;
	}
	const Name& IKPenetrationAvoidanceComponent::GetFullBodyBipedIKName() {
		return IKDATA(m_pCompData)->mIKInfo.ik;
	}

	EchoIK::FullBodyBipedIK* IKPenetrationAvoidanceComponent::GetFullBodyBipedIK() {
		if (!IsExistIK()) return nullptr;
		return IKCOMDATA(m_pIKComponent)->ik;
	}

	Echo::IKPenetrationAvoidanceComponentInfo::IKPenetrationAvoidanceComponentInfo()
	{
	
	}

	Echo::IKPenetrationAvoidanceComponentInfo::IKPenetrationAvoidanceComponentInfo(const Name& comType, ActorCOMMoveableType moveable)
		: IKBaseComponentInfo(comType, moveable) {

	}
	
	Echo::IKPenetrationAvoidanceComponentInfo::~IKPenetrationAvoidanceComponentInfo()
	{
	
	}
	

}

namespace Echo {

	void Echo::IKPenetrationAvoidanceComponent::SetIKInfoData(const PenetrationAvoidanceInfo& info)
	{
		SetFullBodyBipedIKName(info.ik);
		SetWeight(info.weight);
		SetAvoider(info.avoiders);
	}


	void Echo::IKPenetrationAvoidanceComponent::UseDefaultInitialize() {
		SetFullBodyBipedIKName(Name::g_cEmptyName);
		SetWeight(1.0f);
		SetAvoider(GetDefaultAvoiders());
	}

	void Echo::IKPenetrationAvoidanceComponentInfo::CopyToCompData(IKBaseComponentData* pData)
	{
		if (pData == nullptr) return;
		IKBaseComponentInfo::CopyToCompData(pData);

		IKDATA(pData)->mIKInfo.ik = GetFullBodyBipedIKName();
		IKDATA(pData)->mIKInfo.weight = GetWeight();
		IKDATA(pData)->mIKInfo.avoiders = GetAvoider();
	}

	void Echo::IKPenetrationAvoidanceComponentInfo::CompDataToInfo(IKBaseComponentData* pData, IKBaseComponent* pComp)
	{
		if (pData == nullptr) return;
		IKBaseComponentInfo::CompDataToInfo(pData, pComp);
		IKPenetrationAvoidanceComponent* _pComp = static_cast<IKPenetrationAvoidanceComponent*>(pComp);
		SetFullBodyBipedIKName(IKDATA(pData)->mIKInfo.ik, _pComp);
		SetWeight(IKDATA(pData)->mIKInfo.weight, _pComp);
		SetAvoider(IKDATA(pData)->mIKInfo.avoiders, _pComp);
	}

}
