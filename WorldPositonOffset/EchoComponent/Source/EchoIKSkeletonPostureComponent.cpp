#include "EchoComponentStableHeaders.h"
#include "EchoIKSkeletonPostureComponent.h"
#include "EchoIKCCDIKComponent.h"
#include "EchoResourceFolderManager.h"

namespace Echo
{
	
	DECLAREACTORCOMTYPE_CPP(IKSkeletonPostureComponent)

		IKSkeletonPostureComponentInfo::IKSkeletonPostureComponentInfo(const Name& mtype, ActorCOMMoveableType movableType)
		: ActorComponentInfo(mtype, movableType)
	{
	}
	IKSkeletonPostureComponentInfo::~IKSkeletonPostureComponentInfo()
	{
	}

	EDITOR_REFLECT_BEGIN(IKSkeletonPostureComponentInfo)
	EDITOR_REFLECT_END()
	_DEFINEINFO_VIRFUN_BASE(IKSkeletonPostureComponentInfo)

	
	IKSkeletonPostureComponent::IKSkeletonPostureComponent(ActorPtr pActor, ActorCOMMoveableType movableType)
		: ProtocolNodeActorComponent(pActor, movableType)
	{
	}

	IKSkeletonPostureComponent::~IKSkeletonPostureComponent(){}

	void IKSkeletonPostureComponent::registerTickTimeBegin() {
		m_priority = 200;
		m_eventSet[TT_PreUpdateScene].insert(EID_EveryFrame);
		registerTickTimeEnd();
	}

	void IKSkeletonPostureComponent::preUpdateScene(EventID, ProtocolHandle) {

		if (m_SkeletonPosture.size() != m_BoneRenderComps.size()) {
			for (int i = 0; i != m_BoneRenderComps.size(); ++i) {
				const BoneRenderComponentPtr& ptr = m_BoneRenderComps[i];
				if (!ptr.expired()) {
					auto iter = m_SkeletonPosture.find(ptr->GetBoneName());
					if (iter == m_SkeletonPosture.end()) {
						m_SkeletonPosture[ptr->GetBoneName()] = ptr->getRotation();
					}
				}
			}
		}

		bool IsChange = false;
		for (int i = 0; i != m_BoneRenderComps.size(); ++i) {
			const BoneRenderComponentPtr& ptr = m_BoneRenderComps[i];
			if (!ptr.expired()) {
				auto iter = m_SkeletonPosture.find(ptr->GetBoneName());
				if (iter != m_SkeletonPosture.end()) {
					Quaternion& quat = iter->second;
					const Quaternion& quat2 = ptr->getRotation();
					if (!quat.isEqual(quat2)) {
						quat = quat2;
						IsChange = true;
					}
				}
			}
		}

		if (IsChange) {
			setTargetNode();
			setSkeletonPosture();
		}

		UpdateBoneRenderTree();
	}

	const Name& IKSkeletonPostureComponent::GetSkeletonPostureName() {
		return m_SkeletonPostureName;
	}

	FBBEffectorType IKSkeletonPostureComponent::GetIKPartType() {
		return m_IKPartType;
	}

	const Name& IKSkeletonPostureComponent::GetSkeletonPostureTemplateName() {
		return m_SkeletonPostureTemplateName;
	}
	const Name& IKSkeletonPostureComponent::GetRootBoneName() {
		return m_RootBoneName;
	}

	const std::map<Name, Quaternion>& IKSkeletonPostureComponent::GetSkeletonPosture() {
		return m_SkeletonPosture;
	}

	void IKSkeletonPostureComponent::UpdateBoneRenderTree() {
		for (auto& pChildComp : getChildActorComponents()) {
			if (pChildComp.second->getType() == BoneRenderComponent::mType) {
				pChildComp.second.getStaticCastWeakPtr<BoneRenderComponent>()->UpdateBoneRenderTree();
			}
		}
	}

	void IKSkeletonPostureComponent::importFromFile(const Name& file) {
		IKBaseComponentPtr tempPtr = GetIKBaseComponentPtr();
		SetIKBaseComponentPtr(nullptr);

		EchoIK::IKSkeletonPostureBasePtr skel = EchoIK::IKSkeletonPostureManager::instance()->getPreparedResource(file);

		if (skel.isNull()) return;

		std::vector<std::tuple<Name, Vector3, Quaternion, uint32>> vec;
		skel->getSkeletonPosture(vec);

		m_SkeletonPostureName = file;
		m_IKPartType = skel->getIKPartType();
		m_SkeletonPostureTemplateName = skel->getTemplateName();

		if (skel->getName() != skel->getTemplateName()) {

			setPosition(((EchoIK::IKSkeletonPosture*)skel.get())->getLocalPosition());
			Quaternion quat;
			quat.FromEuler(((EchoIK::IKSkeletonPosture*)skel.get())->getLocalEulerAngle());
			setRotation(quat);

			if (Root::instance()->getRuntimeMode() == Root::eRM_Editor && m_pActComInfo != nullptr) {
				m_pActComInfo->SetPosition(((EchoIK::IKSkeletonPosture*)skel.get())->getLocalPosition());
				m_pActComInfo->SetEulerAngles(((EchoIK::IKSkeletonPosture*)skel.get())->getLocalEulerAngle());
			}
		}

		SetBonePosture(vec);

		SetIKBaseComponentPtr(tempPtr);
	}

	void IKSkeletonPostureComponent::exportToFile(const Name& file) {

		if (m_SkeletonPostureTemplateName.empty()) return;

		EchoIK::IKSkeletonPosture* skel = EchoIK::IKSkeletonPostureManager::instance()->createEmptyIKSkeletonPosture(m_SkeletonPostureTemplateName);

		if (skel == nullptr) return;

		skel->setIKPartType(m_IKPartType);
		std::map<Name, Quaternion> posture;
		GetBonePosture(posture);
		skel->setSkeletonPosture(posture);

		skel->setLocalPosition(getPosition());
		skel->setLocalEulerAngle(getEulerAngles());

		skel->exportDataToFile(file.c_str());
		EchoIK::IKSkeletonPostureManager::instance()->releaseEmptyResource(skel);

	}

	void IKSkeletonPostureComponent::ClearAllChild() {
		auto pActor = getActor();
		EchoTMap<String, ActorComponentPtr> childComps = getChildActorComponents();
		for (auto& pChildComp : childComps) {
			auto comInfo = pChildComp.second->m_pActComInfo;
			pActor->delComponent(pChildComp.second);
			pActor->delComponentInfo(comInfo);
		}
	}

	void IKSkeletonPostureComponent::SetBonePosture(std::vector<std::tuple<Name, Vector3, Quaternion, uint32>>& posture) {
		ClearAllChild();
		auto pActor = getActor();
		std::vector<ActorComponentPtr> tempComps;
		tempComps.push_back(this->weak_from_this());
		for (int i = 0; i != posture.size(); ++i) {
			BoneRenderComponentPtr pComp = pActor->AddComponent<BoneRenderComponent>(std::get<0>(posture[i]).toString(), tempComps[std::get<3>(posture[i])], nullptr, getGroupID(), false);
			if (pComp != nullptr) {
				pComp->SetBoneName(std::get<0>(posture[i]));
				pComp->setPosition(std::get<1>(posture[i]));
				pComp->setRotation(std::get<2>(posture[i]));

				if (Root::instance()->getRuntimeMode() == Root::eRM_Editor && pComp->m_pActComInfo != nullptr) {
					static_cast<BoneRenderComponentInfo*>(pComp->m_pActComInfo)->SetBoneName(std::get<0>(posture[i]));
					static_cast<BoneRenderComponentInfo*>(pComp->m_pActComInfo)->SetPosition(std::get<1>(posture[i]));
					static_cast<BoneRenderComponentInfo*>(pComp->m_pActComInfo)->SetEulerAngles(std::get<2>(posture[i]).ToEuler());
				}
			}
			tempComps.push_back(pComp->weak_from_this());
		}
		m_BoneRenderComps.clear();
		for (int i = 1; i != tempComps.size(); ++i) {
			m_BoneRenderComps.push_back(tempComps[i].getStaticCastWeakPtr<BoneRenderComponent>());
		}
		SetHelperVisible(GetHelperVisible());
	}

	void IKSkeletonPostureComponent::GetBonePosture(std::map<Name, Quaternion>& posture) {
		for (auto& pChildComp : getChildActorComponents()) {
			if (pChildComp.second->getType() == BoneRenderComponent::mType) {
				pChildComp.second.getDynamicCastWeakPtr<BoneRenderComponent>()->GetBonePosture(posture);
			}
		}
	}

	void IKSkeletonPostureComponent::SetIKBaseComponentPtr(const IKBaseComponentPtr& pComp) {
		resetTargetNode();
		m_CanTargetIKComponentPtr = pComp;
		setTargetNode();
		if (m_CanTargetIKComponentPtr != nullptr) {
			SetRoleComponentPtr(m_CanTargetIKComponentPtr->GetRoleComponent());
		}
		else {
			SetRoleComponentPtr(nullptr);
		}
	}
	const IKBaseComponentPtr& IKSkeletonPostureComponent::GetIKBaseComponentPtr() {
		return m_CanTargetIKComponentPtr;
	}

	void IKSkeletonPostureComponent::SetRoleComponentPtr(const EchoWeakPtr<RoleComponent>& pComp) {
		resetSkeletonPosture();
		m_RoleComponentPtr = pComp;
		setSkeletonPosture();
	}
	const EchoWeakPtr<RoleComponent>& IKSkeletonPostureComponent::GetRoleComponentPtr() {
		return m_RoleComponentPtr;
	}

	void IKSkeletonPostureComponent::setTargetNode() {
		if (m_CanTargetIKComponentPtr != nullptr && m_CanTargetIKComponentPtr->IsExistIK()) {
			Name type = m_CanTargetIKComponentPtr->getType();
			if (type == IKTargetFBBIKComponent::mType) {
				IKTargetFBBIKComponent* ptr = (IKTargetFBBIKComponent*)m_CanTargetIKComponentPtr.get();
				ptr->SetFBBTarget(m_IKPartType, this->weak_from_this());
			}
			else if (type == IKLimbIKComponent::mType) {
				IKLimbIKComponent* ptr = (IKLimbIKComponent*)m_CanTargetIKComponentPtr.get();
				ptr->SetLimbTarget(this->weak_from_this());
			}
			else if (type == IKCCDIKComponent::mType) {
				IKCCDIKComponent* ptr = (IKCCDIKComponent*)m_CanTargetIKComponentPtr.get();
				ptr->SetCCDTarget(this->weak_from_this());
			}
		}
	}
	void IKSkeletonPostureComponent::resetTargetNode() {
		if (m_CanTargetIKComponentPtr != nullptr && m_CanTargetIKComponentPtr->IsExistIK()) {
			Name type = m_CanTargetIKComponentPtr->getType();
			if (type == IKTargetFBBIKComponent::mType) {
				IKTargetFBBIKComponent* ptr = (IKTargetFBBIKComponent*)m_CanTargetIKComponentPtr.get();
				ptr->ResetFBBTarget(m_IKPartType);
			}
			else if (type == IKLimbIKComponent::mType) {
				IKLimbIKComponent* ptr = (IKLimbIKComponent*)m_CanTargetIKComponentPtr.get();
				ptr->ResetLimbTarget();
			}
			else if (type == IKCCDIKComponent::mType) {
				IKCCDIKComponent* ptr = (IKCCDIKComponent*)m_CanTargetIKComponentPtr.get();
				ptr->ResetCCDTarget();
			}
		}
	}

	void IKSkeletonPostureComponent::setSkeletonPosture() {
		if (m_RoleComponentPtr != nullptr && m_RoleComponentPtr->GetRole()) {
			m_RoleComponentPtr->GetRole()->getRoleIKComponent()->SetBoneLocalOrientation(m_SkeletonPosture);
		}
	}

	void IKSkeletonPostureComponent::resetSkeletonPosture() {
		if (m_RoleComponentPtr != nullptr && m_RoleComponentPtr->GetRole()) {
			m_RoleComponentPtr->GetRole()->getRoleIKComponent()->ResetBoneLocalOrientation(m_SkeletonPosture);
		}
	}

	void IKSkeletonPostureComponent::SetHelperVisible(bool value) {
		mIsHelperVisible = value;
		for (int i = 0; i != m_BoneRenderComps.size(); ++i) {
			if (!m_BoneRenderComps[i].expired()) m_BoneRenderComps[i]->SetRenderVisible(mIsHelperVisible);
		}
	}
	bool IKSkeletonPostureComponent::GetHelperVisible() {
		return mIsHelperVisible;
	}

}
