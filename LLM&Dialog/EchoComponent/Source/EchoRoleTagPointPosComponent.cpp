//-----------------------------------------------------------------------------
// File:  EchoRoleTagPointPosComponent.cpp
//
// Author: LangGuangyue
//
// Date:   2022-8-10
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include "EchoComponentStableHeaders.h"
#include "EchoRoleTagPointPosComponent.h"
#include "EchoBoxComponent.h"
#include "EchoSkeletonInstance.h"
#include "EchoTagPoint.h"
#include "EchoIKBaseComponent.h"
#include "EchoWorldNode.h"

namespace Echo
{

	DECLAREACTORCOMTYPE_CPP(TagPointPosComponent)
		TagPointPosComponentInfo::TagPointPosComponentInfo(const Name& mtype, ActorCOMMoveableType movableType)
		: IKPointComponentInfo(mtype, movableType)
	{
	}
	TagPointPosComponentInfo::~TagPointPosComponentInfo()
	{
	}

	EDITOR_REFLECT_BEGIN(TagPointPosComponentInfo)
	EDITOR_REFLECT_MEMBER_UPDATE(m_TagPointName, "TagPointName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_OtherTagPointPosCompName, "OtherTagPointPosCompName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_END();
	_DEFINEINFO_VIRFUN(TagPointPosComponentInfo, IKPointComponentInfo)


	TagPointPosComponent::TagPointPosComponent(ActorPtr pActor, ActorCOMMoveableType movableType)
		: IKPointComponent(pActor, movableType)
	{

	}

	TagPointPosComponent::~TagPointPosComponent()
	{
	}

	void TagPointPosComponent::create(ActorComponentInfo* info)
	{
		ActorComponent::create(info);
		if (!info) { return; }
		TagPointPosComponentInfo* pInfo = dynamic_cast<TagPointPosComponentInfo*>(info);
		SetTagPointName(pInfo->GetTagPointName());
		SetOtherTagPointPosCompName(pInfo->GetOtherTagPointPosCompName());
	}
	void TagPointPosComponent::init(ActorComponentInfo * info)
	{
		mIsUpdating = false;
		IKPointComponent::init(info);
		//SetNotifiedUpdate(true);
		//if (!info) return;
		//TagPointPosComponentInfo* pInfo = dynamic_cast<TagPointPosComponentInfo*>(info);
		//SetTagPointName(pInfo->GetTagPointName());
		//SetOtherTagPointPosCompName(pInfo->GetOtherTagPointPosCompName());
	}
	void TagPointPosComponent::unInit() {
		mIsUpdating = false;
		SetNotifiedUpdate(false);
	}
	void TagPointPosComponent::NotifiChildImp(int changeMask) {
		if (((uint8)changeMask & WorldNode::TSC_SCALE) != WorldNode::TSC_SCALE) return;
		if (m_OtherTagPointPosCompPtr != nullptr) {
			OnNodeTransformUpdate(true);
		}
	}

	void TagPointPosComponent::selfVisibleStateChanged(Actor_Visible_Type type)
	{
		if (m_pTagNode && type == Visible_Type_Original)
		{
			if (getVisibleStateByType(type))
			{
				auto it = std::bind<void>(&TagPointPosComponent::OnNodeTransformUpdate, this, std::placeholders::_1);
				m_pTagNode->addTransformUpdateFunc(this, it);
				OnNodeTransformUpdate(false);
				
			}
			else
			{
				m_pTagNode->removeTransformUpdateFunc(this);
			}
		}
		ActorComponent::selfVisibleStateChanged(type);

	}

	void TagPointPosComponent::bindNode()
	{
		IKPointComponent::bindNode();
	}
	void TagPointPosComponent::connectCom() {
		IKPointComponent::connectCom();
		AddToWorldNode();
		OnNodeTransformUpdate(true);
	}

	void TagPointPosComponent::disconnectCom() {
		IKPointComponent::disconnectCom();
		RemoveFromWorldNode();
		m_pTagNode = nullptr;
		OnNodeTransformUpdate(false);

	}
	void TagPointPosComponent::replay(ActorComponentInfo * info)
	{
		IKPointComponent::replay(info);
		if (!info) return;
		TagPointPosComponentInfo* pInfo = dynamic_cast<TagPointPosComponentInfo*>(info);
		SetTagPointName(pInfo->GetTagPointName());
		SetOtherTagPointPosCompName(pInfo->GetOtherTagPointPosCompName());
	}
	void TagPointPosComponent::copyTo(const ActorComponentPtr& actCom)
	{
		if (!actCom) return;
		IKPointComponent::copyTo(actCom);
		((TagPointPosComponent*)actCom.getPointer())->SetTagPointName(GetTagPointName());
		((TagPointPosComponent*)actCom.getPointer())->SetOtherTagPointPosCompName(GetOtherTagPointPosCompName());
	}
	void TagPointPosComponent::updateCombinedName() {
#ifdef USE_IK_HELPEROBJ
		if (m_pHelperObj) {
			Name newName(getCombinedName().append("||").append("TagPoint"));
			m_pHelperObj->setName(newName);
		}
#endif // USE_IK_HELPEROBJ
	}

	RoleComponent * TagPointPosComponent::GetRoleComponent()
	{
		ActorComponent* pComp = getParentNodeCom().get();
		if (!pComp || pComp->getType() != RoleComponent::mType) return nullptr;
		return (RoleComponent*)pComp;
	}

	void TagPointPosComponent::GetRoleTagPointList(std::vector<Name>& tagPoints)
	{
		RoleComponent* roleComp = GetRoleComponent();
		if (!roleComp || !roleComp->GetRoleInfo()) return;
		LocatorStructMap mp;
		roleComp->GetRoleInfo()->getAllLocatorInfo(mp);
		for (auto iter : mp) {
			tagPoints.push_back(iter.first);
		}
	}
	RoleTagPoint* TagPointPosComponent::GetRoleTagPoint() {
		RoleComponent* roleCom = GetRoleComponent();
		if (!roleCom) return nullptr;
		RoleObject* roleObject = roleCom->GetRole();
		if (!roleObject) return nullptr;
		if (roleObject->isCreateFinish()) {
			return roleObject->getTagPointComponent()->getRoleTagPoint(m_TagPointName, true);
		}
		else {
			return nullptr;
		}
	}

	const Name&	TagPointPosComponent::GetTagPointName() {
		return m_TagPointName;
	}

	void TagPointPosComponent::SetTagPointName(const Name& value) {
		if (m_TagPointName == value && m_pTagNode != nullptr) return;
		RemoveFromWorldNode();
		m_TagPointName = value;
		m_pTagNode = nullptr;
		AddToWorldNode();
		OnNodeTransformUpdate(true);
	}

	const String& TagPointPosComponent::GetOtherTagPointPosCompName() {
		return m_OtherTagPointPosCompName;
	}
	void TagPointPosComponent::SetOtherTagPointPosCompName(const String& value) {
		if (m_OtherTagPointPosCompName == value && m_OtherTagPointPosCompPtr != nullptr) return;
		RemoveFromWorldNode();
		m_OtherTagPointPosCompName = value;
		m_OtherTagPointPosCompPtr = FindComponent<TagPointPosComponent>(GetOtherTagPointPosCompName());
		AddToWorldNode();
		OnNodeTransformUpdate(true);
	}

	const TagPointPosComponentPtr& TagPointPosComponent::GetOtherTagPointPosCompPtr() {
		return m_OtherTagPointPosCompPtr;
	}
	void TagPointPosComponent::SetOtherTagPointPosCompPtr(const TagPointPosComponentPtr& value) {
		if (m_OtherTagPointPosCompPtr == value) return;
		RemoveFromWorldNode();
		m_OtherTagPointPosCompName = (value == nullptr) ? String("") : value->getCombinedName();
		m_OtherTagPointPosCompPtr = value;
		AddToWorldNode();
		OnNodeTransformUpdate(true);
	}
	
	TagPoint* TagPointPosComponent::GetTagNode() {
		if (m_pTagNode) return m_pTagNode;
		RoleTagPoint* pRoleTagPoint = _GetRoleTagPoint();
		if (!pRoleTagPoint) return nullptr;
		m_pTagNode = pRoleTagPoint->getTagPoint();
		return m_pTagNode;
	}
	void TagPointPosComponent::AddToWorldNode() {
		TagPoint* pTagNode = GetTagNode();
		if (pTagNode)
		{
			if (getVisibleStateByType(Visible_Type_Original))
			{
				auto it = std::bind<void>(&TagPointPosComponent::OnNodeTransformUpdate, this, std::placeholders::_1);
				pTagNode->addTransformUpdateFunc(this, it);
			}

			bool bManualScale = false;
			LocatorStruct* pInfo = GetRoleComponent()->GetRoleInfo()->getLocatorInfo(GetTagPointName());
			if (pInfo) {
				bManualScale = pInfo->bManualScale;
			}
			WorldNode* selfNode = GetWorldNode();
			if (selfNode) {
				selfNode->setInheritScale(!bManualScale);
			}
			if (m_OtherTagPointPosCompPtr) {
				m_OtherTagPointPosCompPtr->mIsExistOtherTagPoint = true;
				if (pInfo) {
					m_OtherTagPointPosCompPtr->mbInverse = pInfo->bInverse;
					m_OtherTagPointPosCompPtr->SetOffsetTransform(-(pInfo->vOffsetPos * pInfo->scale), pInfo->rOffsetOrientation.Inverse(),true);
				}
				else {
					m_OtherTagPointPosCompPtr->mbInverse = true;
					m_OtherTagPointPosCompPtr->SetOffsetTransform(Vector3::ZERO, Quaternion::IDENTITY,true);
					LogManager::instance()->logMessage("TagPointPosComponent::AddToWorldNode: getLocatorInfo is nullptr");
				}
				SetNotifiedUpdate(true);
			}
		}
	}
	void TagPointPosComponent::RemoveFromWorldNode() {
		WorldNode* selfNode = GetWorldNode();
		if (selfNode) {
			selfNode->setInheritScale(true);
		}
		if (m_pTagNode)
		{
			m_pTagNode->removeTransformUpdateFunc(this);
		}
		if (m_OtherTagPointPosCompPtr) {
			m_OtherTagPointPosCompPtr->mIsExistOtherTagPoint = false;
			SetNotifiedUpdate(false);
		}
	}

	void TagPointPosComponent::SetOffsetTransform(const Vector3& OffsetPosNeg, const Quaternion& OffsetOrientationNeg, bool bUpdate) {
		mvOffsetPosNeg = OffsetPosNeg;
		mrOffsetOrientationNeg = OffsetOrientationNeg;
		OnNodeTransformUpdate(bUpdate);
	}

	RoleTagPoint* TagPointPosComponent::_GetRoleTagPoint() {
		RoleComponent* roleCom = GetRoleComponent();
		if (!roleCom) return nullptr;
		RoleObject* roleObject = roleCom->GetRole();
		if (!roleObject) return nullptr;
		if (m_pRoleObj) {
			m_pRoleObj->removeRoleObjectListener(this);
		}
		roleObject->addRoleObjectListener(this);
		if (roleObject->isCreateFinish()) {
			return roleObject->getTagPointComponent()->getRoleTagPoint(m_TagPointName, true);
		}
		else {
			return nullptr;
		}
	}

	void TagPointPosComponent::CalculateTagPointPos(Transform& trans) {
		Transform oldTrans = trans;
		trans.rotation = oldTrans.rotation * mrOffsetOrientationNeg;
		trans.translation = (mbInverse ? trans.rotation : oldTrans.rotation) * mvOffsetPosNeg + oldTrans.translation;
	}

	void TagPointPosComponent::OnNodeTransformUpdate(bool bUpdate) {
		if (mIsUpdating) return;
		mIsUpdating = true;
		TagPoint* pTagNode = m_pTagNode;
		Transform trans = Transform::IDENTITY;
		Vector3 scale = Vector3::UNIT_SCALE;
		if (pTagNode) {
			trans.translation = pTagNode->_getLastDerivedPosition();
			trans.rotation = pTagNode->_getLastDerivedOrientation();
			scale = pTagNode->_getLastDerivedScale();
			if (mIsExistOtherTagPoint) {
				CalculateTagPointPos(trans);
			}
		}
		setTransform(trans.translation,trans.rotation, scale, bUpdate,true,false);
		if (m_OtherTagPointPosCompPtr) {
			Vector3 tagPointScale = getWorldScale();
			auto parTagPointParent = m_OtherTagPointPosCompPtr->getParentNodeCom();
			if (parTagPointParent != nullptr) {
				Vector3 parTagPointScale = parTagPointParent->getWorldScale();
				if (!EchoAlmostZero(parTagPointScale.x) && !EchoAlmostZero(parTagPointScale.y) && !EchoAlmostZero(parTagPointScale.z)) {
					tagPointScale = tagPointScale / parTagPointScale;
				}
			}
			m_OtherTagPointPosCompPtr->SetOffsetTransform(-(trans.translation * tagPointScale), trans.rotation.Inverse(),bUpdate);
		}
		mIsUpdating = false;
	}
	bool TagPointPosComponent::OnRoleCreateFinish() {
		if (m_pTagNode) return false;
		AddToWorldNode();
		OnNodeTransformUpdate(true);
		return true;
	}

	void TagPointPosComponent::OnRoleDestroy() {
		RemoveFromWorldNode();
		m_pTagNode = nullptr;
	}
}
