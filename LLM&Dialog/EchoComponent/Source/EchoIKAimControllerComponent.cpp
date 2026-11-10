#include "EchoComponentStableHeaders.h"
#include "EchoIKAimControllerComponent.h"
#include "IK/EchoAimController.h"
#include "EchoIKAimIKComponent.h"
#include "IK/EchoAimIK.h"

namespace Echo
{
	
	DECLAREACTORCOMTYPE_CPP(AimControllerComponent)

#define IKDATA(ptr) static_cast<AimControllerComponentData*>(ptr)
#define IKCOMDATA(ptr) static_cast<EchoIK::AimController*>(ptr)

		EDITOR_REFLECT_BEGIN(AimControllerComponentInfo)
		EDITOR_REFLECT_MEMBER_UPDATE(m_Weight, "Weight", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_TargetSwitchSmoothTime, "TargetSwitchSmoothTime", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_WeightSmoothTime, "WeightSmoothTime", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_SmoothTurnTowardsTarget, "SmoothTurnTowardsTarget", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_MaxRadiansDelta, "MaxRadiansDelta", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_MaxMagnitudeDelta, "MaxMagnitudeDelta", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_SlerpSpeed, "SlerpSpeed", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_SmoothDampTime, "SmoothDampTime", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_PivotOffsetFromRoot, "PivotOffsetFromRoot", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_MinDistance, "MinDistance", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_Offset, "Offset", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_MaxRootAngle, "MaxRootAngle", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_TurnToTarget, "TurnToTarget", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_TurnToTargetTime, "TurnToTargetTime", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_UseAnimatedAimDirection, "UseAnimatedAimDirection", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_AnimatedAimDirection, "AnimatedAimDirection", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_AimIKCompName, "AimIKCompName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_TargetNodeCompName, "TargetNodeCompName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_END();
	_DEFINEINFO_VIRFUN(AimControllerComponentInfo, IKBaseComponentInfo)


	AimControllerComponent::AimControllerComponent(Echo::ActorPtr pActor, Echo::ActorCOMMoveableType movableType)
		:IKBaseComponent(pActor, movableType)
	{
		m_pCompData = new AimControllerComponentData();
	}

	AimControllerComponent::~AimControllerComponent(){}

	void Echo::AimControllerComponent::SetIKInfo(FinalIKInfoBase* pInfo)
	{
		if (pInfo == nullptr) return;
		SetIKInfoData(*(AimControllerInfo*)pInfo);
	}

	bool AimControllerComponent::IsCanCreate()
	{
		if(mAimIK == nullptr ||
		  mAimIK->GetIKComponent() == nullptr)
		{ return false; }

		RoleIKComponent* pRoleIK = GetRoleIKComponent();
		if (pRoleIK == nullptr) { return false; }

		return true;
	}

	void AimControllerComponent::PostCreateIK()
	{
		if (m_pIKComponent == nullptr) { return; }
		ApplyTargetPtr();
		ApplyAimIKPtr();
	}

	void AimControllerComponent::ApplyTargetPtr()
	{
		if (mTargetPtr == nullptr) { return; }
		IKCOMDATA(m_pIKComponent)->SetWorldTargetNode(mTargetPtr, getTagPointNameByTargetComponent(mTargetPtr));
	}

	void AimControllerComponent::ApplyAimIKPtr()
	{
		if (mAimIK == nullptr) { return; }
		IKCOMDATA(m_pIKComponent)->SetAimIK(mAimIK->GetIKComponent());
		IKCOMDATA(m_pIKComponent)->AddObserver(this);
		
	}

	void Echo::AimControllerComponent::UseDefaultInitialize()
	{
		SetAimIKCompName("");
		SetWeight(1.f);
		SetTargetNodeCompName("");
		SetTargetSwitchSmoothTime(0.3f);
		SetWeightSmoothTime(0.3f);
		SetSmoothTurnTowardsTarget(true);
		SetMaxRadiansDelta(3.f);
		SetMaxMagnitudeDelta(3.f);
		SetSlerpSpeed(3.f);
		SetSmoothDampTime(0.f);
		SetPivotOffsetFromRoot(Vector3::UP);
		SetMinDistance(1.f);
		SetOffset({0.f,0.f,0.f});
		SetMaxRootAngle(1.f);
		SetTurnToTarget(false);
		SetTurnToTargetTime(0.2f);
		SetUseAnimatedAimDirection(true);
		SetAnimatedAimDirection(Vector3::FORWARD);
	}
	
	void Echo::AimControllerComponent::SetIKInfoData(const AimControllerInfo& info)
	{
		SetWeight(info.weight);
		SetTargetSwitchSmoothTime(info.targetSwitchSmoothTime);
		SetWeightSmoothTime(info.weightSmoothTime);
		SetSmoothTurnTowardsTarget(info.smoothTurnTowardsTarget);
		SetMaxRadiansDelta(info.maxRadiansDelta);
		SetMaxMagnitudeDelta(info.maxMagnitudeDelta);
		SetSlerpSpeed(info.slerpSpeed);
		SetSmoothDampTime(info.smoothDampTime);
		SetPivotOffsetFromRoot(info.pivotOffsetFromRoot);
		SetMinDistance(info.minDistance);
		SetOffset(info.offset);
		SetMaxRootAngle(info.maxRootAngle);
		SetTurnToTarget(info.turnToTarget);
		SetTurnToTargetTime(info.turnToTargetTime);
		SetUseAnimatedAimDirection(info.useAnimatedAimDirection);
		SetAnimatedAimDirection(info.animatedAimDirection);
	}
	
	const Echo::AimControllerInfo& Echo::AimControllerComponent::GetIKInfoData()
	{
		return IKDATA(m_pCompData)->mIKInfo;
	}
	
	const String& Echo::AimControllerComponent::GetAimIKCompName()
	{
		return IKDATA(m_pCompData)->mAimIKCompName;
	}

	const String& Echo::AimControllerComponent::GetTargetNodeCompName()
	{
		return IKDATA(m_pCompData)->mTargetNodeCompName;
	}

	float Echo::AimControllerComponent::GetWeight()
	{
		return IKDATA(m_pCompData)->mIKInfo.weight;
	}

	
	float Echo::AimControllerComponent::GetTargetSwitchSmoothTime()
	{
		return IKDATA(m_pCompData)->mIKInfo.targetSwitchSmoothTime;
	}
	
	float Echo::AimControllerComponent::GetWeightSmoothTime()
	{
		return IKDATA(m_pCompData)->mIKInfo.weightSmoothTime;
	}
	
	bool Echo::AimControllerComponent::GetSmoothTurnTowardsTarget()
	{
		return IKDATA(m_pCompData)->mIKInfo.smoothTurnTowardsTarget;
	}
	
	float Echo::AimControllerComponent::GetMaxRadiansDelta()
	{
		return IKDATA(m_pCompData)->mIKInfo.maxRadiansDelta;
	}
	
	float Echo::AimControllerComponent::GetMaxMagnitudeDelta()
	{
		return IKDATA(m_pCompData)->mIKInfo.maxMagnitudeDelta;
	}
	
	float Echo::AimControllerComponent::GetSlerpSpeed()
	{
		return IKDATA(m_pCompData)->mIKInfo.slerpSpeed;
	}
	
	float Echo::AimControllerComponent::GetSmoothDampTime()
	{
		return IKDATA(m_pCompData)->mIKInfo.smoothDampTime;
	}
	
	Echo::Vector3 Echo::AimControllerComponent::GetPivotOffsetFromRoot()
	{
		return IKDATA(m_pCompData)->mIKInfo.pivotOffsetFromRoot;
	}
	
	float Echo::AimControllerComponent::GetMinDistance()
	{
		return IKDATA(m_pCompData)->mIKInfo.minDistance;
	}
	
	Echo::Vector3 Echo::AimControllerComponent::GetOffset()
	{
		return IKDATA(m_pCompData)->mIKInfo.offset;
	}
	
	float Echo::AimControllerComponent::GetMaxRootAngle()
	{
		return IKDATA(m_pCompData)->mIKInfo.maxRootAngle;
	}
	
	bool Echo::AimControllerComponent::GetTurnToTarget()
	{
		return IKDATA(m_pCompData)->mIKInfo.turnToTarget;
	}
	
	float Echo::AimControllerComponent::GetTurnToTargetTime()
	{
		return IKDATA(m_pCompData)->mIKInfo.turnToTargetTime;
	}
	
	bool Echo::AimControllerComponent::GetUseAnimatedAimDirection()
	{
		return IKDATA(m_pCompData)->mIKInfo.useAnimatedAimDirection;
	}
	
	Echo::Vector3 Echo::AimControllerComponent::GetAnimatedAimDirection()
	{
		return IKDATA(m_pCompData)->mIKInfo.animatedAimDirection;
	}

	void Echo::AimControllerComponent::SetTargetNodeCompName(const String& value)
	{
		IKDATA(m_pCompData)->mTargetNodeCompName = value;
		ActorComponentPtr compPtr = FindComponent(value);
		SetTargetNodeComponent(compPtr);
	}

	void Echo::AimControllerComponent::SetAimIKCompName(const String& value)
	{
		IKDATA(m_pCompData)->mAimIKCompName = value;
		ActorComponentPtr compPtr = FindComponent(value);
		IKAimIKComponent* aimIKComp = static_cast<IKAimIKComponent*>(compPtr.get());
		SetAimIKComponent(aimIKComp);
	}

	void Echo::AimControllerComponent::SetTargetNodeComponent(const ActorComponentPtr& pComp)
	{
		mTargetPtr = pComp;
		if (IsExistIK())
		{
			ApplyTargetPtr();
		}
	}

	void Echo::AimControllerComponent::SetAimIKComponent(IKAimIKComponent* pComp)
	{
		mAimIK = pComp;
		if (IsExistIK())
		{
			ApplyAimIKPtr();
		}
	}

	void Echo::AimControllerComponent::SetWeight(float value)
	{
		if (EchoIsNanOrInf(value)) { return; }
		IKDATA(m_pCompData)->mIKInfo.weight = value; 
		if (!IsExistIK()) return; 
		IKCOMDATA(m_pIKComponent)->weight = value;
	}

	void Echo::AimControllerComponent::SetTargetSwitchSmoothTime(float value)
	{
		if (EchoIsNanOrInf(value)) { return; }
		IKDATA(m_pCompData)->mIKInfo.targetSwitchSmoothTime = value; 
		if (!IsExistIK()) return; 
		IKCOMDATA(m_pIKComponent)->targetSwitchSmoothTime = value;
	}
	
	void Echo::AimControllerComponent::SetWeightSmoothTime(float value)
	{
		if (EchoIsNanOrInf(value)) { return; }
		IKDATA(m_pCompData)->mIKInfo.weightSmoothTime = value; 
		if (!IsExistIK()) return; 
		IKCOMDATA(m_pIKComponent)->weightSmoothTime = value;
	}
	
	void Echo::AimControllerComponent::SetSmoothTurnTowardsTarget(bool value)
	{
		IKDATA(m_pCompData)->mIKInfo.smoothTurnTowardsTarget = value; 
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->smoothTurnTowardsTarget = value;
	}
	
	void Echo::AimControllerComponent::SetMaxRadiansDelta(float value)
	{
		if (EchoIsNanOrInf(value)) { return; }
		IKDATA(m_pCompData)->mIKInfo.maxRadiansDelta = value; 
		if (!IsExistIK()) return;
		IKCOMDATA(m_pIKComponent)->maxRadiansDelta = value;
	}
	
	void Echo::AimControllerComponent::SetMaxMagnitudeDelta(float value)
	{
		if (EchoIsNanOrInf(value)) { return; }
		IKDATA(m_pCompData)->mIKInfo.maxMagnitudeDelta = value; 
		if (!IsExistIK()) return; 
		IKCOMDATA(m_pIKComponent)->maxMagnitudeDelta = value;
	}
	
	void Echo::AimControllerComponent::SetSlerpSpeed(float value)
	{
		if (EchoIsNanOrInf(value)) { return; }
		IKDATA(m_pCompData)->mIKInfo.slerpSpeed = value; 
		if (!IsExistIK()) return; 
		IKCOMDATA(m_pIKComponent)->slerpSpeed = value;
	}
	
	void Echo::AimControllerComponent::SetSmoothDampTime(float value)
	{
		if (EchoIsNanOrInf(value)) { return; }
		IKDATA(m_pCompData)->mIKInfo.smoothDampTime = value; 
		if (!IsExistIK()) return; 
		IKCOMDATA(m_pIKComponent)->smoothDampTime = value;
	}
	
	void Echo::AimControllerComponent::SetPivotOffsetFromRoot(Vector3 value)
	{
		IKDATA(m_pCompData)->mIKInfo.pivotOffsetFromRoot = value; 
		if (!IsExistIK()) return; 
		IKCOMDATA(m_pIKComponent)->pivotOffsetFromRoot = value;
	}
	
	void Echo::AimControllerComponent::SetMinDistance(float value)
	{
		if (EchoIsNanOrInf(value)) { return; }
		IKDATA(m_pCompData)->mIKInfo.minDistance = value; 
		if (!IsExistIK()) return; 
		IKCOMDATA(m_pIKComponent)->minDistance = value;
	}
	
	void Echo::AimControllerComponent::SetOffset(Vector3 value)
	{
		IKDATA(m_pCompData)->mIKInfo.offset = value; 
		if (!IsExistIK()) return; 
		IKCOMDATA(m_pIKComponent)->offset = value;
	}
	
	void Echo::AimControllerComponent::SetMaxRootAngle(float value)
	{
		if (EchoIsNanOrInf(value)) { return; }
		IKDATA(m_pCompData)->mIKInfo.maxRootAngle = value; 
		if (!IsExistIK()) return; 
		IKCOMDATA(m_pIKComponent)->maxRootAngle = value;
	}
	
	void Echo::AimControllerComponent::SetTurnToTarget(bool value)
	{
		IKDATA(m_pCompData)->mIKInfo.turnToTarget = value; 
		if (!IsExistIK()) return; 
		IKCOMDATA(m_pIKComponent)->turnToTarget = value;
	}
	
	void Echo::AimControllerComponent::SetTurnToTargetTime(float value)
	{
		if (EchoIsNanOrInf(value)) { return; }
		IKDATA(m_pCompData)->mIKInfo.turnToTargetTime = value; 
		if (!IsExistIK()) return; 
		IKCOMDATA(m_pIKComponent)->turnToTargetTime = value;
	}
	
	void Echo::AimControllerComponent::SetUseAnimatedAimDirection(bool value)
	{
		IKDATA(m_pCompData)->mIKInfo.useAnimatedAimDirection = value; 
		if (!IsExistIK()) return; 
		IKCOMDATA(m_pIKComponent)->useAnimatedAimDirection = value;
	}
	
	void Echo::AimControllerComponent::SetAnimatedAimDirection(Vector3 value)
	{
		IKDATA(m_pCompData)->mIKInfo.animatedAimDirection = value; 
		if (!IsExistIK()) return; 
		IKCOMDATA(m_pIKComponent)->animatedAimDirection = value;
	}

	void Echo::AimControllerComponent::SetCompData(IKBaseComponentData* pData)
	{
		if (pData == nullptr) return;
		IKBaseComponent::SetCompData(pData);
		SetTargetNodeCompName(IKDATA(pData)->mTargetNodeCompName);
		SetAimIKCompName(IKDATA(pData)->mAimIKCompName);
	}

	void Echo::AimControllerComponent::onObjectDestroyed(IKComponent* pIkComponent)
	{
		if (m_pIKComponent == pIkComponent)
		{
			m_pIKComponent = nullptr;
			mIsCreateIK = true;
		}
	}

	void Echo::AimControllerComponentInfo::CopyToCompData(IKBaseComponentData* pData)
	{
		if (pData == nullptr) return;
		IKBaseComponentInfo::CopyToCompData(pData);

		IKDATA(pData)->mIKInfo.weight = GetWeight();
		IKDATA(pData)->mIKInfo.targetSwitchSmoothTime = GetTargetSwitchSmoothTime();
		IKDATA(pData)->mIKInfo.weightSmoothTime = GetWeightSmoothTime();
		IKDATA(pData)->mIKInfo.smoothTurnTowardsTarget = GetSmoothTurnTowardsTarget();
		IKDATA(pData)->mIKInfo.maxRadiansDelta = GetMaxRadiansDelta();
		IKDATA(pData)->mIKInfo.maxMagnitudeDelta = GetMaxMagnitudeDelta();
		IKDATA(pData)->mIKInfo.slerpSpeed = GetSlerpSpeed();
		IKDATA(pData)->mIKInfo.smoothDampTime = GetSmoothDampTime();
		IKDATA(pData)->mIKInfo.pivotOffsetFromRoot = GetPivotOffsetFromRoot();
		IKDATA(pData)->mIKInfo.minDistance = GetMinDistance();
		IKDATA(pData)->mIKInfo.offset = GetOffset();
		IKDATA(pData)->mIKInfo.maxRootAngle = GetMaxRootAngle();
		IKDATA(pData)->mIKInfo.turnToTarget = GetTurnToTarget();
		IKDATA(pData)->mIKInfo.turnToTargetTime = GetTurnToTargetTime();
		IKDATA(pData)->mIKInfo.useAnimatedAimDirection = GetUseAnimatedAimDirection();
		IKDATA(pData)->mIKInfo.animatedAimDirection = GetAnimatedAimDirection();

		IKDATA(pData)->mTargetNodeCompName = GetTargetNodeCompName();
		IKDATA(pData)->mAimIKCompName = GetAimIKCompName();
	}
	
	void Echo::AimControllerComponentInfo::CompDataToInfo(IKBaseComponentData* pData, IKBaseComponent* pComp)
	{
		if (pData == nullptr) return;
		IKBaseComponentInfo::CompDataToInfo(pData, pComp);
		AimControllerComponent* _pComp = static_cast<AimControllerComponent*>(pComp);

		SetWeight(IKDATA(pData)->mIKInfo.weight, _pComp);
		SetTargetSwitchSmoothTime(IKDATA(pData)->mIKInfo.targetSwitchSmoothTime, _pComp);
		SetWeightSmoothTime(IKDATA(pData)->mIKInfo.weightSmoothTime, _pComp);
		SetSmoothTurnTowardsTarget(IKDATA(pData)->mIKInfo.smoothTurnTowardsTarget, _pComp);
		SetMaxRadiansDelta(IKDATA(pData)->mIKInfo.maxRadiansDelta, _pComp);
		SetMaxMagnitudeDelta(IKDATA(pData)->mIKInfo.maxMagnitudeDelta, _pComp);
		SetSlerpSpeed(IKDATA(pData)->mIKInfo.slerpSpeed, _pComp);
		SetSmoothDampTime(IKDATA(pData)->mIKInfo.smoothDampTime, _pComp);
		SetPivotOffsetFromRoot(IKDATA(pData)->mIKInfo.pivotOffsetFromRoot, _pComp);
		SetMinDistance(IKDATA(pData)->mIKInfo.minDistance, _pComp);
		SetOffset(IKDATA(pData)->mIKInfo.offset, _pComp);
		SetMaxRootAngle(IKDATA(pData)->mIKInfo.maxRootAngle, _pComp);
		SetTurnToTarget(IKDATA(pData)->mIKInfo.turnToTarget, _pComp);
		SetTurnToTargetTime(IKDATA(pData)->mIKInfo.turnToTargetTime, _pComp);
		SetUseAnimatedAimDirection(IKDATA(pData)->mIKInfo.useAnimatedAimDirection, _pComp);
		SetAnimatedAimDirection(IKDATA(pData)->mIKInfo.animatedAimDirection, _pComp);

		SetTargetNodeCompName(IKDATA(pData)->mTargetNodeCompName, _pComp);
		SetAimIKCompName(IKDATA(pData)->mAimIKCompName, _pComp);
	}
}

