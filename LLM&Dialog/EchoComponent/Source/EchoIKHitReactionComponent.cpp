#include "EchoComponentStableHeaders.h"
#include "EchoIKHitReactionComponent.h"
#include "IK/EchoHitReaction.h"

namespace Echo {
	const std::vector<Echo::HitReaHitPointBoneInfo>& DefaultHitBonePoints()
	{
		static std::vector<HitReaHitPointBoneInfo> res;
		static bool init = false;
		if (init) return res;
		HitReaHitPointBoneInfo info;
		info.name = Name{ "Head" };
		info.crossFadeTime = 0.0f;
		info.aroundCenterOfMass = { {0.0f,0.0f},{0.2f,-35.f},{0.5f,1.5f},{0.8f,0.f} };

		HitReaBoneLinkInfo bone;
		bone.boneName = Name{ "Bip001 Head" };
		bone.weight = 1.f;

		info.boneLinks.push_back(bone);

		res.push_back(info);

		init = true;

		return res;
	}

	const std::vector<Echo::HitReaHitPointEffectorInfo>& DefaultHitEffectorPoints()
	{
		static std::vector<HitReaHitPointEffectorInfo> res;
		static bool init = false;
		if (init) return res;
		HitReaHitPointEffectorInfo boneInfo;
		EffectorLinkInfo effInfo;

		//0
		boneInfo.name = Name{ "L Forearm" };
		boneInfo.colliderName = Name{ "" };
		boneInfo.crossFadeTime = 0.0f;
		boneInfo.offsetInForceDirection = { {0.0f,0.0f},{0.2f,0.19f},{0.4f,0.015f},{0.8f,0.f} };
		boneInfo.offsetInUpDirection = { { 0.0f,0.0f }, { 0.2f,-0.08f }, { 0.4f,0.03f }, { 0.8f,0.f } };

		effInfo.effector = EchoIK::FullBodyBipedEffector::LeftHand;
		effInfo.weight = 0.7f;
		boneInfo.effectorLinks.push_back(effInfo);
		effInfo.effector = EchoIK::FullBodyBipedEffector::LeftShoulder;
		effInfo.weight = 0.5f;
		boneInfo.effectorLinks.push_back(effInfo);
		effInfo.effector = EchoIK::FullBodyBipedEffector::RightHand;
		effInfo.weight = 0.25f;
		boneInfo.effectorLinks.push_back(effInfo);

		res.push_back(boneInfo);

		//1
		boneInfo.name = Name{ "Head" };
		boneInfo.colliderName = Name{ "" };
		boneInfo.crossFadeTime = 0.0f;
		boneInfo.offsetInForceDirection = { {0.0f,0.0f},{0.2f,0.15f},{0.4f,0.015f},{0.8f,0.f} };
		boneInfo.offsetInUpDirection = { };

		effInfo.effector = EchoIK::FullBodyBipedEffector::LeftShoulder;
		effInfo.weight = 1.f;
		boneInfo.effectorLinks.push_back(effInfo);
		effInfo.effector = EchoIK::FullBodyBipedEffector::RightShoulder;
		effInfo.weight = 1.f;
		boneInfo.effectorLinks.push_back(effInfo);
		effInfo.effector = EchoIK::FullBodyBipedEffector::LeftHand;
		effInfo.weight = 0.5f;
		boneInfo.effectorLinks.push_back(effInfo);
		effInfo.effector = EchoIK::FullBodyBipedEffector::RightHand;
		effInfo.weight = 0.5f;
		boneInfo.effectorLinks.push_back(effInfo);

		res.push_back(boneInfo);

		//2
		boneInfo.name = Name{ "" };
		boneInfo.colliderName = Name{ "" };
		boneInfo.crossFadeTime = 0.0f;
		boneInfo.offsetInForceDirection = { {0.0f,0.0f},{0.2f,0.19f},{0.4f,0.015f},{0.8f,0.f} };
		boneInfo.offsetInUpDirection = { };

		effInfo.effector = EchoIK::FullBodyBipedEffector::Body;
		effInfo.weight = 1.f;
		boneInfo.effectorLinks.push_back(effInfo);
		effInfo.effector = EchoIK::FullBodyBipedEffector::LeftHand;
		effInfo.weight = 0.5f;
		boneInfo.effectorLinks.push_back(effInfo);
		effInfo.effector = EchoIK::FullBodyBipedEffector::RightHand;
		effInfo.weight = 0.5f;
		boneInfo.effectorLinks.push_back(effInfo);

		res.push_back(boneInfo);

		//3
		boneInfo.name = Name{ "Hands down" };
		boneInfo.colliderName = Name{ "" };
		boneInfo.crossFadeTime = 0.0f;
		boneInfo.offsetInForceDirection = { };
		boneInfo.offsetInUpDirection = { {0.0f,0.0f},{0.2f,-0.6f},{0.4f,-0.2f},{0.8f,0.f} };

		effInfo.effector = EchoIK::FullBodyBipedEffector::LeftHand;
		effInfo.weight = 0.5f;
		boneInfo.effectorLinks.push_back(effInfo);
		effInfo.effector = EchoIK::FullBodyBipedEffector::RightHand;
		effInfo.weight = 1.f;
		boneInfo.effectorLinks.push_back(effInfo);

		res.push_back(boneInfo);

		//4 
		boneInfo.name = Name{ "Chest" };
		boneInfo.colliderName = Name{ "" };
		boneInfo.crossFadeTime = 0.0f;
		boneInfo.offsetInForceDirection = { {0.0f,0.0f},{0.2f,0.14f},{0.4f,0.015f},{0.8f,0.f} };
		boneInfo.offsetInUpDirection = {  };

		effInfo.effector = EchoIK::FullBodyBipedEffector::LeftShoulder;
		effInfo.weight = 1.f;
		boneInfo.effectorLinks.push_back(effInfo);
		effInfo.effector = EchoIK::FullBodyBipedEffector::RightShoulder;
		effInfo.weight = 1.f;
		boneInfo.effectorLinks.push_back(effInfo);
		effInfo.effector = EchoIK::FullBodyBipedEffector::LeftHand;
		effInfo.weight = 0.75f;
		boneInfo.effectorLinks.push_back(effInfo);
		effInfo.effector = EchoIK::FullBodyBipedEffector::RightHand;
		effInfo.weight = 0.75f;
		boneInfo.effectorLinks.push_back(effInfo);

		res.push_back(boneInfo);

		init = true;
		return res;
	}

	static void GetEffectorHitPoints_Global(IKHitReactionComponentData* pData, std::vector<Echo::EchoIK::HitReaction::HitPointEffector>& res)
	{
		// Copy function for vector to curve
		auto CopyCurve = [](AnimationCurve& destCurve, const std::vector<Vector2>& srcData)
			{
				for (const auto& point : srcData)
				{
					destCurve.AddKey(KeyframeTpl<float>(point.x, point.y));
				}
			};

		auto& mIKInfo = pData->mIKInfo;

		res.resize(mIKInfo.effectorHitPoints.size());
		for (int i = 0; i < mIKInfo.effectorHitPoints.size(); ++i) {
			res[i].name = mIKInfo.effectorHitPoints[i].name;//Name
			// Collider Names for multiple selection mode
			res[i].colliderName = mIKInfo.effectorHitPoints[i].colliderName;//Collider Name
			res[i].crossFadeTime = mIKInfo.effectorHitPoints[i].crossFadeTime;//CrossFadeTime
			res[i].multipleSelectionMode = mIKInfo.effectorHitPoints[i].multipleSelectionMode;
			res[i].colliderNameVec = mIKInfo.effectorHitPoints[i].colliderNameVec;

			//Animation curve
			auto& forceInfo = mIKInfo.effectorHitPoints[i].offsetInForceDirection;
			for (int j = 0; j < forceInfo.size(); ++j)
			{
				res[i].offsetInForceDirection.AddKey(KeyframeTpl<float>(forceInfo[j].x, forceInfo[j].y));
			}

			auto& upInfo = mIKInfo.effectorHitPoints[i].offsetInUpDirection;
			for (int j = 0; j < upInfo.size(); ++j)
			{
				res[i].offsetInUpDirection.AddKey(KeyframeTpl<float>(upInfo[j].x, upInfo[j].y));
			}

			res[i].hyperbolicMode = mIKInfo.effectorHitPoints[i].hyperbolicMode;
			// In force direction curver [Lower,Upper]
			CopyCurve(res[i].offsetInForceDirectionLowerLimit, mIKInfo.effectorHitPoints[i].offsetInForceDirectionLowerLimit);
			CopyCurve(res[i].offsetInForceDirectionUpperLimit, mIKInfo.effectorHitPoints[i].offsetInForceDirectionUpperLimit);
			// In up direction curver [Lower,Upper]
			CopyCurve(res[i].offsetInUpDirectionLowerLimit, mIKInfo.effectorHitPoints[i].offsetInUpDirectionLowerLimit);
			CopyCurve(res[i].offsetInUpDirectionUpperLimit, mIKInfo.effectorHitPoints[i].offsetInUpDirectionUpperLimit);

			//Effector links
			res[i].effectorLinks.resize(mIKInfo.effectorHitPoints[i].effectorLinks.size());
			for (int j = 0; j < mIKInfo.effectorHitPoints[i].effectorLinks.size(); ++j)
			{
				res[i].effectorLinks[j].weight = mIKInfo.effectorHitPoints[i].effectorLinks[j].weight;
				res[i].effectorLinks[j].effector = EchoIK::FullBodyBipedEffector(mIKInfo.effectorHitPoints[i].effectorLinks[j].effector);
			}
		}
	}

	static void GetBoneHitPoints_Global(IKHitReactionComponentData* pData, std::vector<Echo::EchoIK::HitReaction::HitPointBone>& res)
	{
		// Copy function for vector to curve
		auto CopyCurve = [](AnimationCurve& destCurve, const std::vector<Vector2>& srcData)
			{
				for (const auto& point : srcData)
				{
					destCurve.AddKey(KeyframeTpl<float>(point.x, point.y));
				}
			};

		auto& mIKInfo = pData->mIKInfo;

		res.resize(mIKInfo.boneHitPoints.size());
		for (int i = 0; i < mIKInfo.boneHitPoints.size(); ++i) {
			res[i].name = mIKInfo.boneHitPoints[i].name;
			res[i].colliderName = mIKInfo.boneHitPoints[i].colliderName;
			res[i].crossFadeTime = mIKInfo.boneHitPoints[i].crossFadeTime;
			res[i].multipleSelectionMode = mIKInfo.boneHitPoints[i].multipleSelectionMode;
			res[i].colliderNameVec = mIKInfo.boneHitPoints[i].colliderNameVec;

			// Around center of mass curve.
			auto& aroundInfo = mIKInfo.boneHitPoints[i].aroundCenterOfMass;
			for (int j = 0; j < aroundInfo.size(); ++j)
			{
				res[i].aroundCenterOfMass.AddKey(KeyframeTpl<float>(aroundInfo[j].x, aroundInfo[j].y));
			}

			res[i].hyperbolicMode = mIKInfo.boneHitPoints[i].hyperbolicMode;
			// Around center of mass curve [Lower,Upper]
			CopyCurve(res[i].aroundCenterOfMassLowerLimit, mIKInfo.boneHitPoints[i].aroundCenterOfMassLowerLimit);
			CopyCurve(res[i].aroundCenterOfMassUpperLimit, mIKInfo.boneHitPoints[i].aroundCenterOfMassUpperLimit);

			// Bone Links.
			res[i].boneLinks.resize(mIKInfo.boneHitPoints[i].boneLinks.size());
			for (int j = 0; j < mIKInfo.boneHitPoints[i].boneLinks.size(); ++j) 
			{
				res[i].boneLinks[j].boneName = mIKInfo.boneHitPoints[i].boneLinks[j].boneName;
				res[i].boneLinks[j].weight = mIKInfo.boneHitPoints[i].boneLinks[j].weight;
			}
		}

	}


}

namespace Echo
{

	DECLAREACTORCOMTYPE_CPP(IKHitReactionComponent)
		EDITOR_REFLECT_BEGIN(IKHitReactionComponentInfo)
		EDITOR_REFLECT_MEMBER_UPDATE(m_FullBodyBipedIKName, "FullBodyBipedIKName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_EffectorHitPoints, "effectorHitPoints", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_BoneHitPoints, "boneHitPoints", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_END();
	_DEFINEINFO_VIRFUN(IKHitReactionComponentInfo, IKBaseComponentInfo)
		;
#define IKDATA(ptr) static_cast<IKHitReactionComponentData*>(ptr)
#define IKCOMDATA(ptr) static_cast<EchoIK::HitReaction*>(ptr)


	IKHitReactionComponent::IKHitReactionComponent(Echo::ActorPtr pActor, Echo::ActorCOMMoveableType movableType)
		:IKBaseComponent(pActor, movableType)
	{
		m_pCompData = new IKHitReactionComponentData();
	}

	IKHitReactionComponent::~IKHitReactionComponent(){}

	void IKHitReactionComponent::SetIKInfo(FinalIKInfoBase* pInfo)
	{
		if (pInfo == nullptr) return;
		SetIKInfoData(*(HitReactionInfo*)pInfo);
	}

	bool IKHitReactionComponent::IsCanCreate()
	{
		if (!GetRoleIKComponent()) { return false; }

		auto IsValidColliderName = [](const Name& name)
		{
			return name != Name::g_cEmptyName;
		};

		auto CheckHitPoints = [&IsValidColliderName](const auto& hitPoints)
		{
			for (const auto& hp : hitPoints)
			{
				if (hp.multipleSelectionMode)
				{
					if (hp.colliderNameVec.empty()) { return false; }
					for (const auto& name : hp.colliderNameVec)
					{
						if (!IsValidColliderName(name)) { return false; }
					}
				}
				else
				{
					if (!IsValidColliderName(hp.colliderName)) { return false; }
				}
			}
			return true;
		};


		return	CheckHitPoints(GetEffectorHitPoints()) &&
				CheckHitPoints(GetBoneHitPoints()) &&
				CheckValidFBBIK(IKDATA(m_pCompData)->mIKInfo.ik);
	}


	const std::vector<HitReaHitPointEffectorInfo>& IKHitReactionComponent::GetEffectorHitPoints()
	{
		return IKDATA(m_pCompData)->mIKInfo.effectorHitPoints;
	}

	void IKHitReactionComponent::SetEffectorHitPoints(const std::vector<HitReaHitPointEffectorInfo>& value)
	{
		IKDATA(m_pCompData)->mIKInfo.effectorHitPoints = value;
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->effectorHitPoints.clear();
		GetEffectorHitPoints_Global(IKDATA(m_pCompData), IKCOMDATA(m_pIKComponent)->effectorHitPoints);
		setEffCollider();
	}

	const std::vector<HitReaHitPointBoneInfo>& IKHitReactionComponent::GetBoneHitPoints()
	{
		return IKDATA(m_pCompData)->mIKInfo.boneHitPoints;
	}

	void IKHitReactionComponent::SetBoneHitPoints(const std::vector<HitReaHitPointBoneInfo>& value)
	{
		IKDATA(m_pCompData)->mIKInfo.boneHitPoints = value;

		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->boneHitPoints.clear();
		GetBoneHitPoints_Global(IKDATA(m_pCompData), IKCOMDATA(m_pIKComponent)->boneHitPoints);
		setBoneColliderAndBone();
	}

	void IKHitReactionComponent::SetFullBodyBipedIKName(const Name& ikName) {
		if (IsExistIK()) return;
		IKDATA(m_pCompData)->mIKInfo.ik = ikName;
	}
	const Name& IKHitReactionComponent::GetFullBodyBipedIKName() {
		return IKDATA(m_pCompData)->mIKInfo.ik;
	}

	void IKHitReactionComponent::Hit(const Name& colliderName, const Vector3& force, const Vector3& point)
	{
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->Hit(colliderName, force, point);
	}

	void IKHitReactionComponent::PostCreateIK()
	{
		IKBaseComponent::PostCreateIK();
	}

	void IKHitReactionComponent::setEffCollider()
	{
		IKCOMDATA(m_pIKComponent)->setEffCollider();
	}

	void IKHitReactionComponent::setBoneColliderAndBone()
	{
		IKCOMDATA(m_pIKComponent)->setBoneColliderAndBone();
	}

	EchoIK::FullBodyBipedIK* IKHitReactionComponent::GetFullBodyBipedIK() {
		if (!IsExistIK()) return nullptr;
		return IKCOMDATA(m_pIKComponent)->ik;
	}
	

	IKHitReactionComponentInfo::IKHitReactionComponentInfo() {

	}

	IKHitReactionComponentInfo::IKHitReactionComponentInfo(const Name& comType, ActorCOMMoveableType moveable)
		: IKBaseComponentInfo(comType, moveable) {

	}

	IKHitReactionComponentInfo::~IKHitReactionComponentInfo(){

	}


}

namespace Echo {

	void IKHitReactionComponentInfo::CopyToCompData(IKBaseComponentData* pData)
	{
		if (pData == nullptr) return;
		IKBaseComponentInfo::CopyToCompData(pData);
		IKDATA(pData)->mIKInfo.ik = GetFullBodyBipedIKName();
		IKDATA(pData)->mIKInfo.effectorHitPoints = GetEffectorHitPoints();
		IKDATA(pData)->mIKInfo.boneHitPoints = GetBoneHitPoints();
	}

	void IKHitReactionComponentInfo::CompDataToInfo(IKBaseComponentData* pData, IKBaseComponent* pComp)
	{
		if (pData == nullptr) return;
		IKBaseComponentInfo::CompDataToInfo(pData, pComp);
		IKHitReactionComponent* _pComp = static_cast<IKHitReactionComponent*>(pComp);
		SetFullBodyBipedIKName(IKDATA(pData)->mIKInfo.ik, _pComp);
		SetEffectorHitPoints(IKDATA(pData)->mIKInfo.effectorHitPoints, _pComp);
		SetBoneHitPoints(IKDATA(pData)->mIKInfo.boneHitPoints, _pComp);
	}

	void IKHitReactionComponent::SetIKInfoData(const HitReactionInfo& info)
	{
		SetFullBodyBipedIKName(info.ik);
		SetEffectorHitPoints(info.effectorHitPoints);
		SetBoneHitPoints(info.boneHitPoints);
	}

	const Echo::HitReactionInfo& IKHitReactionComponent::GetIKInfoData()
	{
		return IKDATA(m_pCompData)->mIKInfo;
	}

	void IKHitReactionComponent::UseDefaultInitialize()
	{
		SetFullBodyBipedIKName(Name::g_cEmptyName);
		SetEffectorHitPoints(DefaultHitEffectorPoints());
		SetBoneHitPoints(DefaultHitBonePoints());
	}

}
