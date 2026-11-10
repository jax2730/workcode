#include "EchoComponentStableHeaders.h"
#include "EchoIKBodyTiltComponent.h"
#include "IK/EchoBodyTilt.h"

namespace Echo{

	static const std::vector<Echo::BTEffectorLink>& getDefaultPoseLeft()
	{
		static std::vector<BTEffectorLink> poseLeft;
		static bool init = false;
		if (init) return poseLeft;
		{
			BTEffectorLink pLink;
			pLink.effector = EchoIK::FullBodyBipedEffector::LeftShoulder;
			pLink.offset = Vector3(0.2f, -0.f, 0);
			pLink.pin = Vector3(0, 0, 0);
			pLink.pinWeight = Vector3(0, 0, 0);
			poseLeft.push_back(pLink);
		}
		{
			BTEffectorLink pLink;
			pLink.effector = EchoIK::FullBodyBipedEffector::RightShoulder;
			pLink.offset = Vector3(0.3f, -0.1f, 0);
			pLink.pin = Vector3(0, 0, 0);
			pLink.pinWeight = Vector3(0, 0, 0);
			poseLeft.push_back(pLink);
		}
		{
			BTEffectorLink pLink;
			pLink.effector = EchoIK::FullBodyBipedEffector::LeftHand;
			pLink.offset = Vector3(0.2f, 0.f, -0.1f);
			pLink.pin = Vector3(0, 0, 0);
			pLink.pinWeight = Vector3(0, 0, 0);
			poseLeft.push_back(pLink);
		}
		{
			BTEffectorLink pLink;
			pLink.effector = EchoIK::FullBodyBipedEffector::RightHand;
			pLink.offset = Vector3(0.4f, 0.0f, 0.0f);
			pLink.pin = Vector3(0.4f, 0.8f, 0.1f);
			pLink.pinWeight = Vector3(0.3f, 0.3f, 0.3f);
			poseLeft.push_back(pLink);
		}
		{
			BTEffectorLink pLink;
			pLink.effector = EchoIK::FullBodyBipedEffector::Body;
			pLink.offset = Vector3(0.25f, 0, -0.1f);
			pLink.pin = Vector3(0, 0, 0);
			pLink.pinWeight = Vector3(0, 0, 0);
			poseLeft.push_back(pLink);
		}
		{
			BTEffectorLink pLink;
			pLink.effector = EchoIK::FullBodyBipedEffector::LeftThigh;
			pLink.offset = Vector3(-0.1f, -0.f, 0.1f);
			pLink.pin = Vector3(0, 0, 0);
			pLink.pinWeight = Vector3(0, 0, 0);
			poseLeft.push_back(pLink);
		}
		{
			BTEffectorLink pLink;
			pLink.effector = EchoIK::FullBodyBipedEffector::RightThigh;
			pLink.offset = Vector3(-0.1f, -0.1f, 0.1f);
			pLink.pin = Vector3(0, 0, 0);
			pLink.pinWeight = Vector3(0, 0, 0);
			poseLeft.push_back(pLink);
		}
		{
			BTEffectorLink pLink;
			pLink.effector = EchoIK::FullBodyBipedEffector::LeftFoot;
			pLink.offset = Vector3(-0.f, -0.f, 0);
			pLink.pin = Vector3(0.1f, 0, 0);
			pLink.pinWeight = Vector3(0.1f, 0, 0.1f);
			poseLeft.push_back(pLink);
		}

		init = true;
		return poseLeft;
	}

	static const std::vector<Echo::BTEffectorLink>& getDefaultPoseRight()
	{
		static std::vector<BTEffectorLink> poseRight;
		static bool init = false;
		if (init) return poseRight;
		{
			BTEffectorLink pLink;
			pLink.effector = EchoIK::FullBodyBipedEffector::LeftShoulder;
			pLink.offset = Vector3(-0.3f, -0.1f, 0);
			pLink.pin = Vector3(0, 0, 0);
			pLink.pinWeight = Vector3(0, 0, 0);
			poseRight.push_back(pLink);
		}
		{
			BTEffectorLink pLink;
			pLink.effector = EchoIK::FullBodyBipedEffector::RightShoulder;
			pLink.offset = Vector3(-0.2f, 0.0f, 0);
			pLink.pin = Vector3(0, 0, 0);
			pLink.pinWeight = Vector3(0, 0, 0);
			poseRight.push_back(pLink);
		}
		{
			BTEffectorLink pLink;
			pLink.effector = EchoIK::FullBodyBipedEffector::LeftHand;
			pLink.offset = Vector3(-0.4f, 0.f, 0);
			pLink.pin = Vector3(-0.2f, 0.8f, 0.1f);
			pLink.pinWeight = Vector3(0.3f, 0.3f, 0.3f);
			poseRight.push_back(pLink);
		}
		{
			BTEffectorLink pLink;
			pLink.effector = EchoIK::FullBodyBipedEffector::RightHand;
			pLink.offset = Vector3(-0.2f, 0.0f, -0.1f);
			pLink.pin = Vector3(0, 0, 0);
			pLink.pinWeight = Vector3(0, 0, 0);
			poseRight.push_back(pLink);
		}
		{
			BTEffectorLink pLink;
			pLink.effector = EchoIK::FullBodyBipedEffector::Body;
			pLink.offset = Vector3(-0.25f, 0, -0.1f);
			pLink.pin = Vector3(0, 0, 0);
			pLink.pinWeight = Vector3(0, 0, 0);
			poseRight.push_back(pLink);
		}
		{
			BTEffectorLink pLink;
			pLink.effector = EchoIK::FullBodyBipedEffector::LeftThigh;
			pLink.offset = Vector3(0.1f, -0.1f, 0.1f);
			pLink.pin = Vector3(0, 0, 0);
			pLink.pinWeight = Vector3(0, 0, 0);
			poseRight.push_back(pLink);
		}
		{
			BTEffectorLink pLink;
			pLink.effector = EchoIK::FullBodyBipedEffector::RightThigh;
			pLink.offset = Vector3(0.1f, 0.f, 0.1f);
			pLink.pin = Vector3(0, 0, 0);
			pLink.pinWeight = Vector3(0, 0, 0);
			poseRight.push_back(pLink);
		}
		{
			BTEffectorLink pLink;
			pLink.effector = EchoIK::FullBodyBipedEffector::LeftFoot;
			pLink.offset = Vector3(-0.f, -0.f, 0);
			pLink.pin = Vector3(-0.1f, 0, 0);
			pLink.pinWeight = Vector3(0.1f, 0, 0.1f);
			poseRight.push_back(pLink);
		}
		init = true;
		return poseRight;
	}

	static void transToIKData(IKBodyTiltComponentData* pData, EchoIK::OffsetPose& ikData, const std::vector<BTEffectorLink>& infoData)
	{
		for (int i = 0; i < ikData.effectorLinks.size(); ++i) {
			delete ikData.effectorLinks[i];
			ikData.effectorLinks[i] = nullptr;
		}

		ikData.effectorLinks.resize(infoData.size());

		for (int i = 0; i < infoData.size(); ++i) {
			ikData.effectorLinks[i] = new EchoIK::OffsetPose::EffectorLink();
			ikData.effectorLinks[i]->effector = EchoIK::FullBodyBipedEffector(infoData[i].effector);
			ikData.effectorLinks[i]->offset = infoData[i].offset;
			ikData.effectorLinks[i]->pin = infoData[i].pin;
			ikData.effectorLinks[i]->pinWeight = infoData[i].pinWeight;
		}
	}

}

namespace Echo{

	DECLAREACTORCOMTYPE_CPP(IKBodyTiltComponent)
		EDITOR_REFLECT_BEGIN(IKBodyTiltComponentInfo)
		EDITOR_REFLECT_MEMBER_UPDATE(m_Weight, "weight", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_FullBodyBipedIKName, "FullBodyBipedIKName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_TiltSpeed, "tiltSpeed", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_TiltSensitivity, "tiltSensitivity", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_PoseLeft, "poseLeft", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_PoseRight, "poseRight", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_END();
	_DEFINEINFO_VIRFUN(IKBodyTiltComponentInfo, IKBaseComponentInfo)
		;
#define IKDATA(ptr) static_cast<IKBodyTiltComponentData*>(ptr)
#define IKCOMDATA(ptr) static_cast<EchoIK::BodyTilt*>(ptr)


	IKBodyTiltComponent::IKBodyTiltComponent(Echo::ActorPtr pActor, Echo::ActorCOMMoveableType movableType)
		:IKBaseComponent(pActor, movableType)
	{
		m_pCompData = new IKBodyTiltComponentData();
	}

	IKBodyTiltComponent::~IKBodyTiltComponent(){}

	bool IKBodyTiltComponent::IsCanCreate() {
		auto& poseLeft = GetPoseLeft();
		if (poseLeft.empty()) return false;
		auto& poseRight = GetPoseRight();
		if (poseRight.empty()) return false;
		return CheckValidFBBIK(IKDATA(m_pCompData)->mIKInfo.ikName);
	}

	const Echo::BodyTiltInfo& IKBodyTiltComponent::GetIKInfoData()
	{
		return IKDATA(m_pCompData)->mIKInfo;
	}



	float IKBodyTiltComponent::GetWeight()
	{
		return IKDATA(m_pCompData)->mIKInfo.weight;
	}

	void IKBodyTiltComponent::SetWeight(float value)
	{
		IKDATA(m_pCompData)->mIKInfo.weight = value;if (!IsExistIK()) return;IKCOMDATA(m_pIKComponent)->weight = value;
	}

	void IKBodyTiltComponent::SetFullBodyBipedIKName(const Name& ikName) {
		if (IsExistIK()) return;
		IKDATA(m_pCompData)->mIKInfo.ikName = ikName;
	}
	const Name& IKBodyTiltComponent::GetFullBodyBipedIKName() {
		return IKDATA(m_pCompData)->mIKInfo.ikName;
	}

	float IKBodyTiltComponent::GetTiltSpeed()
	{
		return IKDATA(m_pCompData)->mIKInfo.tiltSpeed;
	}

	void IKBodyTiltComponent::SetTiltSpeed(float value)
	{
		IKDATA(m_pCompData)->mIKInfo.tiltSpeed = value;
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->tiltSpeed = value;
	}

	float IKBodyTiltComponent::GetTiltSensitivity()
	{
		return IKDATA(m_pCompData)->mIKInfo.tiltSensitivity;
	}

	void IKBodyTiltComponent::SetTiltSensitivity(float value)
	{
		IKDATA(m_pCompData)->mIKInfo.tiltSensitivity = value;
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->tiltSensitivity = value;
	}

	const std::vector<Echo::BTEffectorLink>& IKBodyTiltComponent::GetPoseLeft()
	{
		return IKDATA(m_pCompData)->mIKInfo.poseLeft;
	}

	void IKBodyTiltComponent::SetPoseLeft(const std::vector<BTEffectorLink>& value)
	{
		IKDATA(m_pCompData)->mIKInfo.poseLeft = value;
		if (!IsExistIK()) return;
		transToIKData(IKDATA(m_pCompData), (*IKCOMDATA(m_pIKComponent)->poseLeft), IKDATA(m_pCompData)->mIKInfo.poseLeft);

	}

	const std::vector<Echo::BTEffectorLink>& IKBodyTiltComponent::GetPoseRight()
	{
		return IKDATA(m_pCompData)->mIKInfo.poseRight;
	}

	void IKBodyTiltComponent::SetPoseRight(const std::vector<BTEffectorLink>& value)
	{
		IKDATA(m_pCompData)->mIKInfo.poseRight = value;
		if (!IsExistIK()) return;
		transToIKData(IKDATA(m_pCompData), (*IKCOMDATA(m_pIKComponent)->poseRight), IKDATA(m_pCompData)->mIKInfo.poseRight);
	}

	EchoIK::FullBodyBipedIK* IKBodyTiltComponent::GetFullBodyBipedIK() {
		if (!IsExistIK()) return nullptr;
		return IKCOMDATA(m_pIKComponent)->ik;
	}


	IKBodyTiltComponentInfo::IKBodyTiltComponentInfo() {

	}

	IKBodyTiltComponentInfo::IKBodyTiltComponentInfo(const Name& comType, ActorCOMMoveableType moveable)
		: IKBaseComponentInfo(comType, moveable) {

	}

	IKBodyTiltComponentInfo::~IKBodyTiltComponentInfo()
	{

	}

	void IKBodyTiltComponent::SetIKInfo(FinalIKInfoBase* pInfo)
	{
		if (pInfo == nullptr) return;
		SetIKInfoData(*(BodyTiltInfo*)pInfo);
	}

};

namespace Echo {


	void IKBodyTiltComponent::SetIKInfoData(const BodyTiltInfo& info)
	{
		SetWeight(info.weight);
		SetFullBodyBipedIKName(info.ikName);
		SetTiltSpeed(info.tiltSpeed);
		SetTiltSensitivity(info.tiltSensitivity);
		SetPoseLeft(info.poseLeft);
		SetPoseRight(info.poseRight);
	}
	void IKBodyTiltComponent::UseDefaultInitialize()
	{
		SetFullBodyBipedIKName(Name::g_cEmptyName);
		SetWeight(1.f);
		SetTiltSpeed(5.f);
		SetTiltSensitivity(5.f);
		SetPoseLeft(getDefaultPoseLeft());
		SetPoseRight(getDefaultPoseRight());
	}

	void IKBodyTiltComponentInfo::CopyToCompData(IKBaseComponentData* pData)
	{
		if (pData == nullptr) return;
		IKBaseComponentInfo::CopyToCompData(pData);

		IKDATA(pData)->mIKInfo.weight = GetWeight();
		IKDATA(pData)->mIKInfo.ikName = GetFullBodyBipedIKName();
		IKDATA(pData)->mIKInfo.tiltSpeed = GetTiltSpeed();
		IKDATA(pData)->mIKInfo.tiltSensitivity = GetTiltSensitivity();
		IKDATA(pData)->mIKInfo.poseLeft = GetPoseLeft();
		IKDATA(pData)->mIKInfo.poseRight = GetPoseRight();
	}

	void IKBodyTiltComponentInfo::CompDataToInfo(IKBaseComponentData* pData, IKBaseComponent* pComp)
	{
		if (pData == nullptr) return;
		IKBaseComponentInfo::CompDataToInfo(pData, pComp);
		IKBodyTiltComponent* _pComp = static_cast<IKBodyTiltComponent*>(pComp);

		SetWeight(IKDATA(pData)->mIKInfo.weight, _pComp);
		SetFullBodyBipedIKName(IKDATA(pData)->mIKInfo.ikName, _pComp);
		SetTiltSpeed(IKDATA(pData)->mIKInfo.tiltSpeed, _pComp);
		SetTiltSensitivity(IKDATA(pData)->mIKInfo.tiltSensitivity, _pComp);
		SetPoseLeft(IKDATA(pData)->mIKInfo.poseLeft, _pComp);
		SetPoseRight(IKDATA(pData)->mIKInfo.poseRight, _pComp);
	}
}
