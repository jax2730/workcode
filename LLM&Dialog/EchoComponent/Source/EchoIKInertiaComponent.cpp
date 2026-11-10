#include "EchoComponentStableHeaders.h"
#include "EchoIKInertiaComponent.h"
#include "IK/EchoInertia.h"

namespace Echo {

	//static void TransBodyInfoToBodyVec(std::vector<EchoIK::Inertia::Body>& bodyVec, const std::vector<InertiaBodyInfo>& bodyInfoVec)
	//{
	//	bodyVec.clear();
	//	bodyVec.resize(bodyInfoVec.size());
	//	for (int i = 0; i < bodyInfoVec.size(); ++i) {
	//		TransBodyInfoToBody(bodyVec[i], bodyInfoVec[i]);
	//	}
	//}
	//
	//static void TransBodyToBodyInfoVec(const std::vector<EchoIK::Inertia::Body>& bodyVec, std::vector<InertiaBodyInfo>& bodyInfoVec)
	//{
	//	bodyInfoVec.clear();
	//	bodyInfoVec.resize(bodyVec.size());
	//	for (int i = 0; i < bodyVec.size(); ++i) {
	//		TransBodyToBodyInfo(bodyVec[i], bodyInfoVec[i]);
	//	}
	//}
	//
	//static void TransLimitInfoToLimitVec(std::vector<EchoIK::Inertia::OffsetLimits>& limitVec, const std::vector<InertiaOffsetLimitsInfo>& limitInfoVec)
	//{
	//	limitVec.clear();
	//	limitVec.resize(limitInfoVec.size());
	//	for (int i = 0; i < limitInfoVec.size(); ++i) {
	//		TransOffsetLimitsInfoToOffsetLimits(limitVec[i], limitInfoVec[i]);
	//	}
	//}
	//
	//static void TransLimitToLimitInfoVec(const std::vector<EchoIK::Inertia::OffsetLimits>& limitVec, std::vector<InertiaOffsetLimitsInfo>& limitInfoVec)
	//{
	//	limitInfoVec.clear();
	//	limitInfoVec.resize(limitVec.size());
	//	for (int i = 0; i < limitVec.size(); ++i) {
	//		TransOffsetLimitsToOffsetLimitsInfo(limitVec[i], limitInfoVec[i]);
	//	}
	//}
	//
	//static std::vector<Echo::EchoIK::Inertia::Body> getBodies(IKInertiaComponentData* pData)
	//{
	//	static std::vector<EchoIK::Inertia::Body> res;
	//	TransBodyInfoToBodyVec(res, pData->mIKInfo.bodies);
	//	return res;
	//}
	//
	//static std::vector<Echo::EchoIK::Inertia::OffsetLimits> getLimits(IKInertiaComponentData* pData)
	//{
	//	static std::vector<EchoIK::Inertia::OffsetLimits> res;
	//	TransLimitInfoToLimitVec(res, pData->mIKInfo.limits);
	//	return res;
	//}
	//
	//static void setBody(IKInertiaComponentData* pData, const std::vector<EchoIK::Inertia::Body>& bodyVec)
	//{
	//	TransBodyToBodyInfoVec(bodyVec, pData->mIKInfo.bodies);
	//}
	//
	//static void setLimits(IKInertiaComponentData* pData, const std::vector<EchoIK::Inertia::OffsetLimits>& limitsVec)
	//{
	//	TransLimitToLimitInfoVec(limitsVec, pData->mIKInfo.limits);
	//}


	inline const std::vector<InertiaBodyInfo>& defaultBodies() {
		static std::vector<InertiaBodyInfo> res;
		static bool init = false;

		if (init) return res;

		InertiaBodyInfo body;
		EffectorLinkInfo effector;

		// Body 0
		//Bip001 spine 1
		body.transformName = Name("Bip001 Spine1");
		body.speed = 10.f;
		body.acceleration = 25.f;
		body.matchVelocity = 0.3f;
		body.gravity = 0.f;
		body.effectorLinks.clear();
		effector.effector = EchoIK::FullBodyBipedEffector::LeftShoulder;
		effector.weight = 1.f;
		body.effectorLinks.push_back(effector);
		effector.effector = EchoIK::FullBodyBipedEffector::RightShoulder;
		effector.weight = 1.f;
		body.effectorLinks.push_back(effector);
		effector.effector = EchoIK::FullBodyBipedEffector::LeftHand;
		effector.weight = 0.3f;
		body.effectorLinks.push_back(effector);
		effector.effector = EchoIK::FullBodyBipedEffector::RightHand;
		effector.weight = 0.3f;
		body.effectorLinks.push_back(effector);
		effector.effector = EchoIK::FullBodyBipedEffector::Body;
		effector.weight = 0.4f;
		body.effectorLinks.push_back(effector);

		res.push_back(body);

		// Body 1
		//Bip001 L hand
		body.transformName = Name("Bip001 L Hand");
		body.speed = 6.f;
		body.acceleration = 20.f;
		body.matchVelocity = 0.5f;
		body.gravity = 0.f;
		body.effectorLinks.clear();
		effector.effector = EchoIK::FullBodyBipedEffector::LeftHand;
		effector.weight = 0.4f;
		body.effectorLinks.push_back(effector);
		effector.effector = EchoIK::FullBodyBipedEffector::LeftShoulder;
		effector.weight = 0.1f;
		body.effectorLinks.push_back(effector);

		res.push_back(body);

		// Body 2
		//Bip001 R hand
		body.transformName = Name("Bip001 R Hand");
		body.speed = 6.f;
		body.acceleration = 20.f;
		body.matchVelocity = 0.f;
		body.gravity = 0.f;
		body.effectorLinks.clear();
		effector.effector = EchoIK::FullBodyBipedEffector::RightHand;
		effector.weight = 0.4f;
		body.effectorLinks.push_back(effector);
		effector.effector = EchoIK::FullBodyBipedEffector::RightShoulder;
		effector.weight = 0.1f;
		body.effectorLinks.push_back(effector);

		res.push_back(body);

		// Body 3
		//Bip001 spine
		body.transformName = Name("Bip001 Spine");
		body.speed = 14.f;

		body.acceleration = 25.f;
		body.matchVelocity = 0.5f;
		body.gravity = 0.f;
		body.effectorLinks.clear();
		effector.effector = EchoIK::FullBodyBipedEffector::LeftThigh;
		effector.weight = 0.5f;
		body.effectorLinks.push_back(effector);
		effector.effector = EchoIK::FullBodyBipedEffector::RightThigh;
		effector.weight = 0.5f;
		body.effectorLinks.push_back(effector);

		res.push_back(body);

		init = true;

		return res;
	}
	inline const std::vector<InertiaOffsetLimitsInfo>& defaultLimits() {
		static std::vector<InertiaOffsetLimitsInfo> res;
		static bool init = false;

		if (init) return res;

		//0
		InertiaOffsetLimitsInfo limit/*=new EchoIK::Inertia::OffsetLimits()*/;
		limit.effector = EchoIK::FullBodyBipedEffector::RightHand;
		limit.spring = 0.7f;
		limit.x = true;
		limit.y = true;
		limit.z = false;
		limit.minX = 0.f;
		limit.maxX = 0.3f;
		limit.minY = 0.f;
		limit.maxY = 0.3f;
		limit.minZ = 0.f;
		limit.maxZ = 0.f;
		res.push_back(limit);

		//1
		//limit = new EchoIK::Inertia::OffsetLimits();
		limit.effector = EchoIK::FullBodyBipedEffector::LeftHand;
		limit.spring = 0.7f;
		limit.x = true;
		limit.y = false;
		limit.z = false;
		limit.minX = -0.3f;
		limit.maxX = 0.f;
		limit.minY = 0.f;
		limit.maxY = 0.f;
		limit.minZ = 0.f;
		limit.maxZ = 0.f;
		res.push_back(limit);

		//2
		//limit = new EchoIK::Inertia::OffsetLimits();
		limit.effector = EchoIK::FullBodyBipedEffector::Body;
		limit.spring = 0.3f;
		limit.x = false;
		limit.y = true;
		limit.z = false;
		limit.minX = 0.f;
		limit.maxX = 0.f;
		limit.minY = 0.3f;
		limit.minZ = 0.f;
		limit.maxZ = 0.f;
		res.push_back(limit);

		//3
		//limit = new EchoIK::Inertia::OffsetLimits();
		limit.effector = EchoIK::FullBodyBipedEffector::LeftShoulder;
		limit.spring = 0.f;
		limit.x = false;
		limit.y = true;
		limit.z = false;
		limit.minX = 0.f;
		limit.maxX = 0.f;
		limit.minY = 0.f;
		limit.maxY = 0.3f;
		limit.minZ = 0.f;
		limit.maxZ = 0.f;
		res.push_back(limit);
		//4
		//limit = new EchoIK::Inertia::OffsetLimits();
		limit.effector = EchoIK::FullBodyBipedEffector::RightShoulder;
		limit.spring = 0.f;
		limit.x = false;
		limit.y = true;
		limit.z = false;
		limit.minX = 0.f;
		limit.maxX = 0.f;
		limit.minY = 0.f;
		limit.maxY = 0.3f;
		limit.minZ = 0.f;
		limit.maxZ = 0.f;
		res.push_back(limit);

		//5
		//limit = new EchoIK::Inertia::OffsetLimits();
		limit.effector = EchoIK::FullBodyBipedEffector::LeftFoot;
		limit.spring = 0.f;
		limit.x = false;
		limit.y = true;
		limit.z = false;
		limit.minX = 0.f;
		limit.maxX = 0.f;
		limit.minY = 0.f;
		limit.maxY = 0.3f;
		limit.minZ = 0.f;
		limit.maxZ = 0.f;
		res.push_back(limit);

		//6
		//limit = new EchoIK::Inertia::OffsetLimits();
		limit.effector = EchoIK::FullBodyBipedEffector::RightFoot;
		limit.spring = 0.f;
		limit.x = false;
		limit.y = true;
		limit.z = false;
		limit.minX = 0.f;
		limit.maxX = 0.f;
		limit.minY = 0.f;
		limit.maxY = 0.3f;
		limit.minZ = 0.f;
		limit.maxZ = 0.f;
		res.push_back(limit);

		init = true;

		return res;
	}

}

namespace Echo
{
	DECLAREACTORCOMTYPE_CPP(IKInertiaComponent)
		EDITOR_REFLECT_BEGIN(IKInertiaComponentInfo)
		EDITOR_REFLECT_MEMBER_UPDATE(m_FullBodyBipedIKName, "FullBodyBipedIKName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_Weight, "Weight", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_Bodies, "Bodies", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_Limits, "Limits", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_END();

	_DEFINEINFO_VIRFUN(IKInertiaComponentInfo, IKBaseComponentInfo)


#define IKDATA(ptr) static_cast<IKInertiaComponentData*>(ptr)
#define IKCOMDATA(ptr) static_cast<EchoIK::Inertia*>(ptr)


	IKInertiaComponent::IKInertiaComponent(Echo::ActorPtr pActor, Echo::ActorCOMMoveableType movableType)
		:IKBaseComponent(pActor, movableType)
	{
		m_pCompData = new IKInertiaComponentData();
	}

	IKInertiaComponent::~IKInertiaComponent(){}

	void IKInertiaComponent::SetIKInfo(FinalIKInfoBase* pInfo)
	{
		if (pInfo == nullptr) return;
		SetIKInfoData(*((IKInertiaInfo*)pInfo));
	}

	bool IKInertiaComponent::IsCanCreate() {
		RoleIKComponent* pRoleIK = GetRoleIKComponent();
		if (!pRoleIK) return false;
		auto& bodies = GetBodies();
		if (bodies.empty()) return false;
		for (auto& iter : bodies) {
			if (iter.transformName == Name::g_cEmptyName || pRoleIK->getBoneNodeByName(iter.transformName) == nullptr) {
				return false;
			}
		}
		return CheckValidFBBIK(IKDATA(m_pCompData)->mIKInfo.ik);
	}

	void IKInertiaComponent::SetFullBodyBipedIKName(const Name& ikName) {
		if (IsExistIK()) return;
		IKDATA(m_pCompData)->mIKInfo.ik = ikName;
	}
	const Name& IKInertiaComponent::GetFullBodyBipedIKName() {
		return IKDATA(m_pCompData)->mIKInfo.ik;
	}

	float IKInertiaComponent::GetWeight()
	{
		return IKDATA(m_pCompData)->mIKInfo.weight;
	}

	void IKInertiaComponent::SetWeight(float value)
	{
		IKDATA(m_pCompData)->mIKInfo.weight= value;
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->weight = value;
	}

	const std::vector<InertiaBodyInfo>& IKInertiaComponent::GetBodies()
	{
		return IKDATA(m_pCompData)->mIKInfo.bodies;
	}

	void IKInertiaComponent::SetBodies(const std::vector<InertiaBodyInfo>& value)
	{
		if (!IsExistIK()) {
			IKDATA(m_pCompData)->mIKInfo.bodies = value;
		}
		else if (IKDATA(m_pCompData)->mIKInfo.bodies.size() == value.size()) {
			std::vector<InertiaBodyInfo>& _bodies = IKDATA(m_pCompData)->mIKInfo.bodies;
			size_t size = _bodies.size();
			for (int i = 0; i != size; ++i) {
				if ((_bodies[i].transformName != value[i].transformName) || (_bodies[i].effectorLinks.size() != value[i].effectorLinks.size())) {
					return;
				}
			}
			_bodies = value;
			std::vector<EchoIK::Inertia::Body>& _ikbodies = IKCOMDATA(m_pIKComponent)->bodies;
			for (int i = 0; i != size; ++i) {
				EchoIK::Inertia::TransBodyInfoToBody(_ikbodies[i], _bodies[i]);
			}
		}
	}

	const std::vector<InertiaOffsetLimitsInfo>& IKInertiaComponent::GetLimits()
	{
		return IKDATA(m_pCompData)->mIKInfo.limits;
	}

	void IKInertiaComponent::SetLimits(const std::vector<InertiaOffsetLimitsInfo>& value)
	{
		if (!IsExistIK()) {
			IKDATA(m_pCompData)->mIKInfo.limits = value;
		}
		else if (IKDATA(m_pCompData)->mIKInfo.limits.size() == value.size()) {
			size_t size = value.size();
			IKDATA(m_pCompData)->mIKInfo.limits = value;
			std::vector<EchoIK::Inertia::OffsetLimits>& _iklimits = IKCOMDATA(m_pIKComponent)->limits;
			for (int i = 0; i != size; ++i) {
				EchoIK::Inertia::TransOffsetLimitsInfoToOffsetLimits(_iklimits[i], value[i]);
			}
		}
	}

	EchoIK::FullBodyBipedIK* IKInertiaComponent::GetFullBodyBipedIK() {
		if (!IsExistIK()) return nullptr;
		return IKCOMDATA(m_pIKComponent)->ik;
	}


}

namespace Echo {

	void IKInertiaComponentInfo::CopyToCompData(IKBaseComponentData* pData)
	{
		if (!pData) return;
		IKBaseComponentInfo::CopyToCompData(pData);

		IKDATA(pData)->mIKInfo.ik = GetFullBodyBipedIKName();
		IKDATA(pData)->mIKInfo.weight = GetWeight();
		IKDATA(pData)->mIKInfo.bodies = GetBodies();
		IKDATA(pData)->mIKInfo.limits = GetLimits();
	}

	void IKInertiaComponentInfo::CompDataToInfo(IKBaseComponentData* pData, IKBaseComponent* pComp)
	{
		if (!pData) return;
		IKBaseComponentInfo::CompDataToInfo(pData, pComp);
		IKInertiaComponent* _pComp = static_cast<IKInertiaComponent*>(pComp);
		SetFullBodyBipedIKName(IKDATA(pData)->mIKInfo.ik, _pComp);
		SetWeight(IKDATA(pData)->mIKInfo.weight, _pComp);
		SetBodies(IKDATA(pData)->mIKInfo.bodies, _pComp);
		SetLimits(IKDATA(pData)->mIKInfo.limits, _pComp);
	}

	void IKInertiaComponent::UseDefaultInitialize()
	{
		SetFullBodyBipedIKName(Name::g_cEmptyName);
		SetWeight(1.f);
		SetBodies(defaultBodies());
		SetLimits(defaultLimits());
	}

	void IKInertiaComponent::SetIKInfoData(const IKInertiaInfo& info)
	{
		SetFullBodyBipedIKName(info.ik);
		SetWeight(info.weight);
		SetBodies(info.bodies);
		SetLimits(info.limits);
	}

	const Echo::IKInertiaInfo& IKInertiaComponent::GetIKInfoData()
	{
		return IKDATA(m_pCompData)->mIKInfo;
	}

}
