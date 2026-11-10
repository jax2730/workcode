#include "EchoComponentStableHeaders.h"
#include "EchoIKEffectorOffsetComponent.h"
#include "IK/EchoEffectorOffest.h"

namespace Echo
{

	DECLAREACTORCOMTYPE_CPP(IKEffectorOffsetComponent)

#define  IKDATA(ptr) static_cast<IKEffectorOffsetComponentData*>(ptr)
#define IKCOMDATA(ptr) static_cast<EchoIK::EffectorOffest*>(ptr)

		EDITOR_REFLECT_BEGIN(IKEffectorOffsetComponentInfo)
		EDITOR_REFLECT_MEMBER_UPDATE(m_FullBodyBipedIKName, "FullBodyBipedIKName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_BodyOffset,"BodyOffset",EditorUI_DRAG, nullptr , reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_HMRPWeight,"HMRPWeight",EditorUI_DRAG, nullptr , reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_LeftFootOffset,"LeftFootOffset",EditorUI_DRAG, nullptr , reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_RightFootOffset,"RightFootOffset",EditorUI_DRAG, nullptr , reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_LeftHandOffset,"LeftHandOffset",EditorUI_DRAG, nullptr , reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_RightHandOffset,"RightHandOffset",EditorUI_DRAG, nullptr , reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_LeftShoulderOffset,"LeftShoulderOffset",EditorUI_DRAG, nullptr , reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_RightShoulderOffset,"RightShoulderOffset",EditorUI_DRAG, nullptr , reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_LeftThighOffset,"LeftThighOffset",EditorUI_DRAG, nullptr , reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_RightThighOffset,"RightThighOffset",EditorUI_DRAG, nullptr , reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_END();
	_DEFINEINFO_VIRFUN(IKEffectorOffsetComponentInfo, IKBaseComponentInfo)


	IKEffectorOffsetComponent::IKEffectorOffsetComponent(Echo::ActorPtr pActor, Echo::ActorCOMMoveableType movableType)
		: IKBaseComponent(pActor, movableType)
	{
		m_pCompData = new IKEffectorOffsetComponentData();
	}

	IKEffectorOffsetComponent::~IKEffectorOffsetComponent(){}

	//void Echo::IKEffectorOffsetComponent::SetCompData(IKBaseComponentData* pData)
	//{
	//	if (!pData) return;
	//
	//	SetRoleCompName(IKDATA(pData)->roleCompName);
	//	SetIKInfoData(IKDATA(pData)->mIKInfo);
	//}
	
	void Echo::IKEffectorOffsetComponent::SetIKInfo(FinalIKInfoBase* pInfo)
	{
		if (!pInfo) return;
		SetIKInfoData(*(IKEffectorOffsetInfo*)pInfo);
	}
	
	const Echo::IKEffectorOffsetInfo& Echo::IKEffectorOffsetComponent::GetIKInfoData()
	{
		return IKDATA(m_pCompData)->mIKInfo;
	}
	

	bool IKEffectorOffsetComponent::IsCanCreate() {
		return CheckValidFBBIK(IKDATA(m_pCompData)->mIKInfo.ik);
	}
	
	void IKEffectorOffsetComponent::SetFullBodyBipedIKName(const Name& ikName) {
		if (IsExistIK()) return;
		IKDATA(m_pCompData)->mIKInfo.ik = ikName;
	}
	const Name& IKEffectorOffsetComponent::GetFullBodyBipedIKName() {
		return IKDATA(m_pCompData)->mIKInfo.ik;
	}

	float Echo::IKEffectorOffsetComponent::GetWeight()
	{
		return IKDATA(m_pCompData)->mIKInfo.weight;
	}
	
	void Echo::IKEffectorOffsetComponent::SetWeight(float value)
	{
		IKDATA(m_pCompData)->mIKInfo.weight = value;
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->weight = value;
	}
	
	float Echo::IKEffectorOffsetComponent::GetHMRPWeigth()
	{
		return IKDATA(m_pCompData)->mIKInfo.HMRPWeight;
	}
	
	void Echo::IKEffectorOffsetComponent::SetHMRPWeight(float value)
	{
		IKDATA(m_pCompData)->mIKInfo.HMRPWeight = value;
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->handsMaintainRelativePositionWeight = value;
	}
	
	const Vector3& IKEffectorOffsetComponent::GetBodyOffset()
	{
		return IKDATA(m_pCompData)->mIKInfo.bodyOffset;
	}
	
	void IKEffectorOffsetComponent::SetBodyOffset(const Vector3& value)
	{
		IKDATA(m_pCompData)->mIKInfo.bodyOffset = value;
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->bodyOffset = value;
	}
	
	const Vector3& IKEffectorOffsetComponent::GetLeftShoulderOffset()
	{
		return IKDATA(m_pCompData)->mIKInfo.leftShoulderOffset;
	}
	
	void IKEffectorOffsetComponent::SetLeftShoulderOffset(const Vector3& value)
	{
		IKDATA(m_pCompData)->mIKInfo.leftShoulderOffset = value;
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->leftShoulderOffset = value;
	}
	
	const Vector3& IKEffectorOffsetComponent::GetRightShoulderOffset()
	{
		return IKDATA(m_pCompData)->mIKInfo.rightShoulderOffset;
	}
	
	void IKEffectorOffsetComponent::SetRightShoulderOffset(const Vector3& value)
	{
		IKDATA(m_pCompData)->mIKInfo.rightShoulderOffset = value;
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->rightShoulderOffset = value;
	}
	
	const Vector3& IKEffectorOffsetComponent::GetLeftThighOffset()
	{
		return IKDATA(m_pCompData)->mIKInfo.leftThighOffset;
	}
	
	void IKEffectorOffsetComponent::SetLeftThighOffset(const Vector3& value)
	{
		IKDATA(m_pCompData)->mIKInfo.leftThighOffset = value;
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->leftThighOffset = value;
	}
	
	const Vector3& IKEffectorOffsetComponent::GetRightThighOffset()
	{
		return IKDATA(m_pCompData)->mIKInfo.rightThighOffset;
	}
	
	void IKEffectorOffsetComponent::SetRightThighOffset(const Vector3& value)
	{
		IKDATA(m_pCompData)->mIKInfo.rightThighOffset = value;
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->rightThighOffset = value;
	}
	
	const Vector3& IKEffectorOffsetComponent::GetLeftHandOffset()
	{
		return IKDATA(m_pCompData)->mIKInfo.leftHandOffset;
	}
	
	void IKEffectorOffsetComponent::SetLeftHandOffset(const Vector3& value)
	{
		IKDATA(m_pCompData)->mIKInfo.leftHandOffset = value;
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->leftHandOffset = value;
	}
	
	const Vector3& Echo::IKEffectorOffsetComponent::GetRightHandOffset()
	{
		return IKDATA(m_pCompData)->mIKInfo.rightHandOffset;
	}
	
	void IKEffectorOffsetComponent::SetRightHandOffset(const Vector3& value)
	{
		IKDATA(m_pCompData)->mIKInfo.rightHandOffset = value;
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->rightHandOffset = value;
	}
	
	const Vector3& Echo::IKEffectorOffsetComponent::GetLeftFootOffset()
	{
		return IKDATA(m_pCompData)->mIKInfo.leftFootOffset;
	}
	
	void IKEffectorOffsetComponent::SetLeftFootOffset(const Vector3& value)
	{
		IKDATA(m_pCompData)->mIKInfo.leftFootOffset = value;
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->leftFootOffset = value;
	}
	
	const Vector3& IKEffectorOffsetComponent::GetRightFootOffset()
	{
		return IKDATA(m_pCompData)->mIKInfo.rightFootOffset;
	}
	
	void Echo::IKEffectorOffsetComponent::SetRightFootOffset(const Vector3& value)
	{
		IKDATA(m_pCompData)->mIKInfo.rightFootOffset = value;
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->rightFootOffset = value;
	}

	EchoIK::FullBodyBipedIK* IKEffectorOffsetComponent::GetFullBodyBipedIK() {
		if (!IsExistIK()) return nullptr;
		return IKCOMDATA(m_pIKComponent)->ik;
	}

}

namespace Echo {


	void Echo::IKEffectorOffsetComponent::SetIKInfoData(const IKEffectorOffsetInfo& info)
	{
		SetFullBodyBipedIKName(info.ik);
		SetWeight(info.weight);
		SetHMRPWeight(info.HMRPWeight);
		SetBodyOffset(info.bodyOffset);
		SetLeftFootOffset(info.leftFootOffset);
		SetLeftHandOffset(info.leftHandOffset);
		SetLeftShoulderOffset(info.leftShoulderOffset);
		SetLeftThighOffset(info.leftThighOffset);
		SetRightFootOffset(info.rightFootOffset);
		SetRightHandOffset(info.rightHandOffset);
		SetRightShoulderOffset(info.rightShoulderOffset);
		SetRightThighOffset(info.rightThighOffset);
	}

	void Echo::IKEffectorOffsetComponent::UseDefaultInitialize()
	{
		SetFullBodyBipedIKName(Name::g_cEmptyName);
		SetWeight(1);
		SetHMRPWeight(0);
		SetBodyOffset(Vector3::ZERO);
		SetLeftFootOffset(Vector3::ZERO);
		SetRightFootOffset(Vector3::ZERO);
		SetLeftHandOffset(Vector3::ZERO);
		SetRightHandOffset(Vector3::ZERO);
		SetLeftShoulderOffset(Vector3::ZERO);
		SetRightShoulderOffset(Vector3::ZERO);
		SetLeftThighOffset(Vector3::ZERO);
		SetRightThighOffset(Vector3::ZERO);
	}

	void IKEffectorOffsetComponentInfo::CopyToCompData(IKBaseComponentData* pData)
	{
		if (!pData) return;
		//Copy data from  a data instance and  make sure the [ik base component] data copyed this data.
		IKBaseComponentInfo::CopyToCompData(pData);
		IKDATA(pData)->mIKInfo.ik = GetFullBodyBipedIKName();
		IKDATA(pData)->mIKInfo.weight = GetWeight();
		IKDATA(pData)->mIKInfo.HMRPWeight = GetHMRPWeight();
		IKDATA(pData)->mIKInfo.bodyOffset = GetBodyOffset();

		IKDATA(pData)->mIKInfo.leftFootOffset = GetLeftFootOffset();
		IKDATA(pData)->mIKInfo.rightFootOffset = GetRightFootOffset();

		IKDATA(pData)->mIKInfo.leftHandOffset = GetLeftHandOffset();
		IKDATA(pData)->mIKInfo.rightHandOffset = GetRightHandOffset();

		IKDATA(pData)->mIKInfo.leftShoulderOffset = GetLeftShoulderOffset();
		IKDATA(pData)->mIKInfo.rightShoulderOffset = GetRightShoulderOffset();

		IKDATA(pData)->mIKInfo.leftThighOffset = GetLeftThighOffset();
		IKDATA(pData)->mIKInfo.rightThighOffset = GetRightThighOffset();
	}

	void IKEffectorOffsetComponentInfo::CompDataToInfo(IKBaseComponentData* pData, IKBaseComponent* pComp)
	{
		if (!pData) return;

		IKBaseComponentInfo::CompDataToInfo(pData, pComp);

		IKEffectorOffsetComponent* comp = (IKEffectorOffsetComponent*)pComp;
		SetFullBodyBipedIKName(IKDATA(pData)->mIKInfo.ik, comp);
		SetWeight(IKDATA(pData)->mIKInfo.weight, comp);
		SetHMRPWeight(IKDATA(pData)->mIKInfo.HMRPWeight, comp);
		SetBodyOffset(IKDATA(pData)->mIKInfo.bodyOffset, comp);

		SetLeftFootOffset(IKDATA(pData)->mIKInfo.leftFootOffset, comp);
		SetLeftHandOffset(IKDATA(pData)->mIKInfo.leftHandOffset, comp);
		SetLeftShoulderOffset(IKDATA(pData)->mIKInfo.leftShoulderOffset, comp);
		SetLeftThighOffset(IKDATA(pData)->mIKInfo.leftThighOffset, comp);

		SetRightFootOffset(IKDATA(pData)->mIKInfo.rightFootOffset, comp);
		SetRightHandOffset(IKDATA(pData)->mIKInfo.rightHandOffset, comp);
		SetRightShoulderOffset(IKDATA(pData)->mIKInfo.rightShoulderOffset, comp);
		SetRightThighOffset(IKDATA(pData)->mIKInfo.rightThighOffset, comp);
	}

}
