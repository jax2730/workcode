#include "EchoComponentStableHeaders.h"
#include "EchoColliderComponent.h"
#include "EchoActorProFuncImp.h"
#include "EchoPhysXScene.h"
#include "EchoPhysXAsset.h"
#include "EchoPhysXCommon.h"
#include "Actor/EchoActorHelper.h"
#include "Actor/EchoNodeComponent.h"

namespace Echo {

	ColliderComponentInfo::ColliderComponentInfo(const Name& comType, ActorCOMMoveableType MoveType) : 
		ActorComponentInfo(comType, MoveType), 
		m_FilterGroupType(ePhysXGroup_Mesh) { }
	ColliderComponentInfo::~ColliderComponentInfo() {}

	EDITOR_REFLECT_BEGIN(ColliderComponentInfo)
	EDITOR_REFLECT_MEMBER_UPDATE(m_IsTrigger, "IsTrigger", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_MaterialId, "MaterialId", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_SurfaceType, "SurfaceType", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_FilterGroupType, "FilterGroupType", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_CanInteractFilterGroupType, "CanInteractFilterGroupType", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_END();

	_DEFINEINFO_VIRFUN_BASE(ColliderComponentInfo)

	void ColliderComponentInfo::CopyToData(ColliderComponentData* pData) {
		if (!pData) return;
		pData->isTrigger = GetIsTrigger();
		pData->materialId = GetMaterialId();
		pData->surfaceType = GetSurfaceType();
		pData->filterGroupType = GetFilterGroupType();
		pData->canInteractFilterGroupType = GetCanInteractFilterGroupType();
	}

	ColliderComponent::ColliderComponent(ActorPtr pActor, ActorCOMMoveableType type) : PhysicsActorComponent(pActor, type) {}
	ColliderComponent::~ColliderComponent() {
		SAFE_DELETE(m_pCompData);
	}
	void ColliderComponent::init(ActorComponentInfo* info) {
		PhysicsBaseComponent::init(info);
		if (info) {
			static_cast<ColliderComponentInfo*>(info)->CopyToData(m_pCompData);
			SetCompData(m_pCompData);
		}
		SetNotifiedUpdate(true);
		mPhysicsParentingChanged = std::bind<void>(&ColliderComponent::OnParentingChanged, this, std::placeholders::_1);
	}
	void ColliderComponent::unInit() {
		GetPhysicsManager()->RemoveColliderFromUpdateLocalPoseQueue(this);
		GetPhysicsManager()->RemoveColliderFromUpdateGeometry(this);
		mListenerList.clear();
#ifdef USE_PHYSICS_HELPEROBJ
		ReleaseHelper();
#endif // USE_PHYSICS_HELPEROBJ
		ReleasePhysXObject();
		SetNotifiedUpdate(false);
		mPhysicsParentingChanged = nullptr;
		PhysicsBaseComponent::unInit();
	}
	void ColliderComponent::connectCom() {
		PhysicsBaseComponent::connectCom();
		ChangeAttach();
	}
	void ColliderComponent::disconnectCom() {
		DetachShape();
		RemoveSelfLink();
		PhysicsBaseComponent::disconnectCom();
	}
	void ColliderComponent::replay(ActorComponentInfo* info) {
		if (info) {
			static_cast<ColliderComponentInfo*>(info)->CopyToData(m_pCompData);
			SetCompData(m_pCompData);
		}
	}
	void ColliderComponent::copyTo(const ActorComponentPtr& actCom) {
		if (!actCom.get()) return;
		actCom.getDynamicCastWeakPtr<ColliderComponent>()->SetCompData(m_pCompData);
		actCom.getDynamicCastWeakPtr<ColliderComponent>()->SetHelperVisible(GetHelperVisible());
	}
	void ColliderComponent::OnParentingChanged(const ActorComponentPtr& compPtr) {
		AttachShape(compPtr.getStaticCastWeakPtr<RigidBodyComponent>());
	}
	void ColliderComponent::NotifiChildImp(int changeMask) {
		if ((uint8)changeMask == WorldNode::TSC_NONE) return;
		if (!m_pAttachRigidBody || m_pAttachRigidBody->IsNeedChangeCollider()) {
			if (((uint8)changeMask & WorldNode::TSC_SCALE) == WorldNode::TSC_SCALE) {
				ChangeLocalPose();
				ChangeShape();
			}
			else {
				ChangeLocalPose();
				UpdatePhysXPage();
			}
		}
#ifdef USE_PHYSICS_HELPEROBJ
		if (m_pHelperObj) {
			m_pHelperObj->getSceneNode()->_setDerivedPosition(getWorldPosition());
			m_pHelperObj->getSceneNode()->_setDerivedOrientation(getWorldRotation());
			m_pHelperObj->getSceneNode()->_setDerivedScale(getWorldScale());
		}
#endif // USE_PHYSICS_HELPEROBJ
	}
	void ColliderComponent::showEditorHelper(bool isShow) {
		SetHelperVisible(isShow);
	}
	void ColliderComponent::updateCombinedName() {
#ifdef USE_PHYSICS_HELPEROBJ
		if (m_pHelperObj) {
			Name newName(getCombinedName().append("||").append("HelperObj"));
			m_pHelperObj->setName(newName);
		}
#endif // USE_PHYSICS_HELPEROBJ
	}

	void ColliderComponent::visibleStateChanged(bool activeState) {
		if (!GetPhysicsManager()) return;
		if (activeState) {
			CreatePhysXObject();
		}
		else {
			ReleasePhysXObject();
		}
	}

	void ColliderComponent::CreatePhysics() {
		if (!GetPhysicsManager()->IsPhysicsUpdating()) return;
		if (m_pShape) return;
		CreateShape();
		if (!m_pShape) return;
		m_pShape->SetUserData(this);
		m_pShape->SetUserType((int)PhysXUserTypeEnum::ActorComponent);
		if (m_pAttachRigidBody != nullptr) {
			m_pAttachRigidBody->AttachShapeInternal(m_pShape);
		}
		else {
			CreateDefaultStatic();
		}
		ChangeLocalPoseImpl();
		AttachSelfLink();
		ResetHelperColor();
	}
	void ColliderComponent::ReleasePhysics() {
		if (!m_pShape) return;
		DetachSelfLink();
		if (m_pAttachRigidBody != nullptr) {
			m_pAttachRigidBody->DetachShapeInternal(m_pShape);
		}
		else {
			DeleteDefaultStatic();
		}
		ReleaseShape();
		ResetHelperColor();
	}
	bool ColliderComponent::IsCanPhysics() {
		return true;
	}
	bool ColliderComponent::IsExistPhysics() {
		return m_pShape != nullptr ;
	}
	PhysXShape* ColliderComponent::GetPhysics() {
		return m_pShape;
	}
	bool ColliderComponent::IsCanAttach(RigidBodyComponent* pRB) {
		return true;
	}
	AggregateComponentPtr ColliderComponent::GetAggregate() {
		if (m_pAttachRigidBody == nullptr) {
			return PhysicsActorComponent::GetAggregate();
		}
		else {
			return  m_pAttachRigidBody->GetAggregate();
		}
	}
	void ColliderComponent::CreateShape() {
		AddToEditorUpdateQueue();
		char buffer[ColliderComponent::GEOM_MAX_SIZE];
		EchoGeometry* geom = GenerateGeometry(buffer);
		if (!geom) return;
		m_pShape = GetPhysicsManager()->CreatePhysXShape(geom, m_pCompData->materialId);
		ChangeIsTrigger();
		ChangeFilterGroup();
		ChangeSurfaceType();
	}
	void ColliderComponent::ReleaseShape() {
		GetPhysicsManager()->ReleasePhysXShape(m_pShape);
		m_pShape = nullptr;
	}
	void ColliderComponent::AttachShape(const RigidBodyComponentPtr& pRB) {
		if (pRB != nullptr) {
			if (m_pAttachRigidBody != pRB) {
				if (IsCanAttach(pRB.getPointer())) {
					DetachShape();
					auto thisPtr = getWeakPtr<ColliderComponent>();
					pRB->AddCollider(thisPtr);
					m_pAttachRigidBody = pRB;
					SetIsUsePhysics(m_pAttachRigidBody->GetIsUsePhysics());
					ChangeFilterGroup();
				}
				else {
					DetachShape();
				}
			}
			else {
				if (!IsCanAttach(pRB.getPointer())) {
					DetachShape();
				}
			}
		}
		else {
			DetachShape();
		}
		if (!ExistPhysXObject()) {
			CreatePhysXObject();
		}
	}
	void ColliderComponent::DetachShape() {
		ReleasePhysXObject();
		if (m_pAttachRigidBody != nullptr) {
			auto thisPtr = getWeakPtr<ColliderComponent>();
			m_pAttachRigidBody->RemoveCollider(thisPtr);
			m_pAttachRigidBody = nullptr;
		}
	}
	void ColliderComponent::ChangeShape() {
		AddToEditorUpdateQueue();
		UpdatePhysXPage();
		if (!m_pShape) {
			if (getRealVisibleState() && GetIsUsePhysics() && IsCanPhysics()) {
				AddToPhysicsUpdateQueue();
			}
			return;
		}
		if (!IsCanPhysics()) {
			ReleasePhysics();
		}
		else {
			GetPhysicsManager()->AddColliderToUpdateGeometry(this);
			if (m_pAttachRigidBody) {
				m_pAttachRigidBody->ResetMassAndInertiaTensor();
			}
		}
	}
	void ColliderComponent::ChangeGeometryImpl() {
		if (!m_pShape) return;
		char buffer[ColliderComponent::GEOM_MAX_SIZE];
		EchoGeometry* geom = GenerateGeometry(buffer);
		if (!geom) {
			ReleasePhysics();
		}
		else {
			m_pShape->SetGeometry(geom);
		}
	}
	void ColliderComponent::ChangeLocalPose() {
		GetPhysicsManager()->AddColliderToUpdateLocalPoseQueue(this);
	}
	void ColliderComponent::ChangeLocalPoseImpl() {
		if (!m_pShape) return;
		if (m_pAttachRigidBody != nullptr) {
			DTransform attT = { m_pAttachRigidBody->getWorldPosition(), m_pAttachRigidBody->getWorldRotation() };
			DTransform myT = { getWorldPosition(), getWorldRotation() };
			Transform resT;
			EchoPhysicsManager::convertWorldToLocal(attT, myT, resT);
			if (m_pShape->SetLocalPose(resT)) {
				m_pAttachRigidBody->ResetMassAndInertiaTensor();
			}
		}
		else {
			if (m_pAttachStatic) m_pAttachStatic->SetTransform(getWorldPosition(), getWorldRotation());
		}
	}
	void ColliderComponent::ChangeIsTrigger() {
		if (!m_pShape) return;
		m_pShape->EnableTrigger(m_pCompData->isTrigger);
	}
	void ColliderComponent::ChangeFilterGroup() {
		if (!m_pShape) return;
		EchoSceneFilterShaderFlags flags = (mListenerList.empty() ? EchoSceneFilterShaderFlag::eDEFAULT_NOEVENT : EchoSceneFilterShaderFlag::eDEFAULT);
		if (m_pAttachRigidBody) {
			flags |= m_pAttachRigidBody->GetShapeSceneFilterShaderFlags();
		}
		m_pShape->SetFilterData(m_pCompData->filterGroupType, m_pCompData->canInteractFilterGroupType, flags);
	}
	void ColliderComponent::ChangeMaterialID() {
		if (!m_pShape) return;
		m_pShape->SetPhyMaterial(m_pCompData->materialId);
	}
	void ColliderComponent::ChangeSurfaceType() {
		if (!m_pShape) return;
		m_pShape->SetSurfaceType(m_pCompData->surfaceType);
	}
	void ColliderComponent::ChangeAttach() {
		RigidBodyComponentPtr pRB = EchoPhysicsManager::FindNewRigidBodyComp(getParentNodeCom());
		AttachShape(pRB);
	}
	void ColliderComponent::SetIsTrigger(bool value) {
		m_pCompData->isTrigger = value;
#ifdef ECHO_EDITOR
		if (ActorHelper::getForceSave() && m_pActComInfo)
		{
			ColliderComponentInfo* colliderInfo = static_cast<ColliderComponentInfo*>(m_pActComInfo);
			colliderInfo->SetIsTrigger(value);
		}
#endif
		ChangeIsTrigger();
	}
	void ColliderComponent::SetMaterialId(int value) {
		m_pCompData->materialId = value;
#ifdef ECHO_EDITOR
		if (ActorHelper::getForceSave() && m_pActComInfo)
		{
			ColliderComponentInfo* colliderInfo = static_cast<ColliderComponentInfo*>(m_pActComInfo);
			colliderInfo->SetMaterialId(value);
		}
#endif
		ChangeMaterialID();
	}
	void ColliderComponent::SetSurfaceType(uint32 value) {
		m_pCompData->surfaceType = value;
#ifdef ECHO_EDITOR
		if (ActorHelper::getForceSave() && m_pActComInfo)
		{
			ColliderComponentInfo* colliderInfo = static_cast<ColliderComponentInfo*>(m_pActComInfo);
			colliderInfo->SetSurfaceType(value);
		}
#endif
		ChangeSurfaceType();
	}
	void ColliderComponent::SetFilterGroupType(PhysXGroup value) {
		m_pCompData->filterGroupType = value;
#ifdef ECHO_EDITOR
		if (ActorHelper::getForceSave() && m_pActComInfo)
		{
			ColliderComponentInfo* colliderInfo = static_cast<ColliderComponentInfo*>(m_pActComInfo);
			colliderInfo->SetFilterGroupType(value);
		}
#endif
		ChangeFilterGroup();
	}
	void ColliderComponent::SetCanInteractFilterGroupType(PhysXGroup value) {
		m_pCompData->canInteractFilterGroupType = value;
#ifdef ECHO_EDITOR
		if (ActorHelper::getForceSave() && m_pActComInfo)
		{
			ColliderComponentInfo* colliderInfo = static_cast<ColliderComponentInfo*>(m_pActComInfo);
			colliderInfo->SetCanInteractFilterGroupType(value);
		}
#endif
		ChangeFilterGroup();
	}
	bool ColliderComponent::GetIsTrigger() {
		return m_pCompData->isTrigger;
	}
	int ColliderComponent::GetMaterialId() {
		return m_pCompData->materialId;
	}
	uint32 ColliderComponent::GetSurfaceType(const DBVector3& position, bool bWorld) {
		if (m_pGetSurfaceTypeFun != nullptr) {
			return m_pGetSurfaceTypeFun(position, bWorld);
		}
		else {
			return m_pCompData->surfaceType;
		}
	}
	PhysXGroup ColliderComponent::GetFilterGroupType() {
		return m_pCompData->filterGroupType;
	}
	PhysXGroup ColliderComponent::GetCanInteractFilterGroupType() {
		return m_pCompData->canInteractFilterGroupType;
	}
	void ColliderComponent::SetCompDataBase(ColliderComponentData* pData) {
		if (!pData) return;
		SetIsTrigger(pData->isTrigger);
		SetMaterialId(pData->materialId);
		SetSurfaceType(pData->surfaceType);
		SetFilterGroupType(pData->filterGroupType);
		SetCanInteractFilterGroupType(pData->canInteractFilterGroupType);
	}
	const ColliderComponentData* ColliderComponent::GetCompData() {
		return m_pCompData;
	}

	void ColliderComponent::AddColliderCallBack(const EchoColliderCallBackPtr& pComp) {
		if (!pComp) return;
		mListenerList.insert(pComp);
		if (mListenerList.size() == 1) { ChangeFilterGroup(); }
	}
	void ColliderComponent::RemoveColliderCallBack(const EchoColliderCallBackPtr& pComp) {
		if (!pComp) return;
		mListenerList.erase(pComp);
		if (mListenerList.empty()) { ChangeFilterGroup(); }
	}

	void ColliderComponent::SendCollisionEnter(const EchoCollisionParam& param) {
		if (mListenerList.empty()) return;
		auto iter = mListenerList.begin();
		while (iter != mListenerList.end()) {
			if (*iter == nullptr) {
				iter = mListenerList.erase(iter);
				continue;
			}
			(*iter)->OnCollisionEnter(param);
			++iter;
		}
	}
	void ColliderComponent::SendCollisionExit(const EchoCollisionParam& param) {
		if (mListenerList.empty()) return;
		auto iter = mListenerList.begin();
		while (iter != mListenerList.end()) {
			if (*iter == nullptr) {
				iter = mListenerList.erase(iter);
				continue;
			}
			(*iter)->OnCollisionExit(param);
			++iter;
		}
	}
	void ColliderComponent::SendCollisionStay(const EchoCollisionParam& param) {
		if (mListenerList.empty()) return;
		auto iter = mListenerList.begin();
		while (iter != mListenerList.end()) {
			if (*iter == nullptr) {
				iter = mListenerList.erase(iter);
				continue;
			}
			(*iter)->OnCollisionStay(param);
			++iter;
		}
	}
	void ColliderComponent::SendTriggerEnter(const EchoTriggerParam& param) {
		if (mListenerList.empty()) return;
		auto iter = mListenerList.begin();
		while (iter != mListenerList.end()) {
			if (*iter == nullptr) {
				iter = mListenerList.erase(iter);
				continue;
			}
			(*iter)->OnTriggerEnter(param);
			++iter;
		}
	}
	void ColliderComponent::SendTriggerExit(const EchoTriggerParam& param) {
		if (mListenerList.empty()) return;
		auto iter = mListenerList.begin();
		while (iter != mListenerList.end()) {
			if (*iter == nullptr) {
				iter = mListenerList.erase(iter);
				continue;
			}
			(*iter)->OnTriggerExit(param);
			++iter;
		}
	}
	void ColliderComponent::SendTriggerStay(const EchoTriggerParam& param) {
		if (mListenerList.empty()) return;
		auto iter = mListenerList.begin();
		while (iter != mListenerList.end()) {
			if (*iter == nullptr) {
				iter = mListenerList.erase(iter);
				continue;
			}
			(*iter)->OnTriggerStay(param);
			++iter;
		}
	}
	void ColliderComponent::SendCollisionCCT(const EchoControllerShapeHitParam& param) {
		if (mListenerList.empty()) return;
		auto iter = mListenerList.begin();
		while (iter != mListenerList.end()) {
			if (*iter == nullptr) {
				iter = mListenerList.erase(iter);
				continue;
			}
			(*iter)->OnCollisionCCT(param);
			++iter;
		}
	}

	void ColliderComponent::SendCollisionCCT(const EchoMathControllerShapeHitParam& param) {
		if (mListenerList.empty()) return;
		auto iter = mListenerList.begin();
		while (iter != mListenerList.end()) {
			if (*iter == nullptr) {
				iter = mListenerList.erase(iter);
				continue;
			}
			(*iter)->OnCollisionCCT(param);
			++iter;
		}
	}

	PhysicsActor* ColliderComponent::_getActorPhysX() {
		if (m_pAttachRigidBody != nullptr) {
			return m_pAttachRigidBody->_getActorPhysX();
		}
		else {
			return m_pAttachStatic;
		}
	}
	DVector3 ColliderComponent::getWorldPositionPhysX() {
		if (m_pShape == nullptr) {
			return getWorldPosition();
		}
		if (m_pAttachRigidBody) {
			DTransform trans = m_pAttachRigidBody->getWorldTransformPhysX();
			Transform localPose;
			m_pShape->GetLocalPose(localPose);
			return trans.rotation * localPose.translation + trans.translation;
		}
		else if (m_pAttachStatic) {
			return m_pAttachStatic->GetPosition();
		}
		else {
			return getWorldPosition();
		}
	}
	Quaternion ColliderComponent::getWorldRotationPhysX() {
		if (m_pShape == nullptr) {
			return getWorldRotation();
		}
		if (m_pAttachRigidBody) {
			Quaternion quat = m_pAttachRigidBody->getWorldRotationPhysX();
			Transform localPose;
			m_pShape->GetLocalPose(localPose);
			return quat * localPose.rotation;
		}
		else if (m_pAttachStatic) {
			return m_pAttachStatic->GetRotation();
		}
		else {
			return getWorldRotation();
		}
	}
	DTransform ColliderComponent::getWorldTransformPhysX() {
		if (m_pShape == nullptr) {
			return DTransform(getWorldPosition(), getWorldRotation());
		}
		if (m_pAttachRigidBody) {
			DTransform trans = m_pAttachRigidBody->getWorldTransformPhysX();
			Transform localPose;
			m_pShape->GetLocalPose(localPose);
			DTransform res;
			EchoPhysicsManager::convertLocalToWorld(trans, localPose, res);
			return res;
		}
		else if (m_pAttachStatic) {
			return m_pAttachStatic->GetTransform();
		}
		else {
			return DTransform(getWorldPosition(), getWorldRotation());
		}
	}

#ifdef USE_PHYSICS_PAGE
	EchoPhysXActorObject* ColliderComponent::GetEchoPhysXObject() {
		if (m_pAttachRigidBody) {
			return m_pAttachRigidBody->mPhysXObject;
		}
		else {
			return mPhysXObject;
		}
	}
#endif //USE_PHYSICS_PAGE

	void ColliderComponent::UpdatePhysXPage() {
#ifdef USE_PHYSICS_PAGE
		if (m_pAttachRigidBody) {
			m_pAttachRigidBody->mPhysicsLocalAABBChanged = true;
		}
		EchoPhysXActorObject* sharedPtr = GetEchoPhysXObject();
		if (sharedPtr != nullptr) {
			EchoPhysXGroupObject* pGroup = sharedPtr->GetGroup();
			if (pGroup != nullptr) {
				GetPhysicsManager()->AddPhysXObjectToUpdateQueue(pGroup);
			}
			else {
				GetPhysicsManager()->AddPhysXObjectToUpdateQueue(sharedPtr);
			}
		}
#endif //USE_PHYSICS_PAGE
	}

	void ColliderComponent::SetIsUsePhysics(bool value) {
		if (!value) {
			if (IsExistPhysics()) {
				ReleasePhysics();
			}
			else {
				RemoveFromPhysicsUpdateQueue();
			}
		}
		else {
			if (!IsExistPhysics()) {
				AddToPhysicsUpdateQueue();
			}
			//else {
			//	AttachSelfLink();
			//}
		}
		m_IsUsePhysics = value;
	}

	void ColliderComponent::CreateDefaultStatic() {
		if (m_pAttachStatic != nullptr) {
			DeleteDefaultStatic();
		}
		PhysicsActorInfo info;
		info.shape = nullptr;
		info.userdata = this;
		info.usertype = (int)PhysXUserTypeEnum::Undefined;
		info.transform = Transform{ getWorldPosition(), getWorldRotation() };
		info.surface = 0;
		info.shape = m_pShape;
		if (m_pShape) {
			m_pShape->SetLocalPose(Transform::IDENTITY);
		}
		m_pAttachStatic = GetPhysicsManager()->GetScene()->AddStatic(info);
	}
	void ColliderComponent::DeleteDefaultStatic() {
		if (m_pAttachStatic == nullptr) return;
		GetPhysicsManager()->RemovePhysXActor(m_pAttachStatic);
		m_pAttachStatic = nullptr;
	}

	//bool ColliderComponent::CheckScaleChanged() {
	//	Vector3 newScale = getWorldScale();
	//	Vector3 scaleOffset = newScale - m_lastWorldScale;
	//	if (scaleOffset.AlmostZero()) {
	//		return false;
	//	}
	//	else {
	//		m_lastWorldScale = newScale;
	//		return true;
	//	}
	//}
	bool ColliderComponent::ExistPhysXObject() {
#ifdef USE_PHYSICS_PAGE
		return GetEchoPhysXObject() != nullptr;
#else
		return IsExistPhysics();
#endif // USE_PHYSICS_PAGE

	}

	void ColliderComponent::CreatePhysXObject() {
		if (m_IsUsePhysXPage) {
			GetPhysicsManager()->CreatePhysXObject(this);
		}
		else {
			SetIsUsePhysics(true);
		}
	}
	void ColliderComponent::ReleasePhysXObject() {
		if (m_IsUsePhysXPage) {
			GetPhysicsManager()->ReleasePhysXObject(this);
		}
		else {
			SetIsUsePhysics(false);
		}
	}
	bool ColliderComponent::IsAsync() {
#ifdef USE_PHYSICS_PAGE
		EchoPhysXActorObject* sharedPtr = GetEchoPhysXObject();
		if (sharedPtr != nullptr && sharedPtr->m_pPhysicsPage != nullptr) {
			return sharedPtr->m_pPhysicsPage->isAsync();
		}
		else {
			return m_bAsyncLoad;
		}
#else
		return m_bAsyncLoad;
#endif //USE_PHYSICS_PAGE
	}
	void ColliderComponent::SetIsUsePhysXPage(bool value) {
		if (m_IsUsePhysXPage == value) return;
		if (m_IsUsePhysXPage) {
			GetPhysicsManager()->ReleasePhysXObject(this);
		}
		else {
			GetPhysicsManager()->CreatePhysXObject(this);
		}
		m_IsUsePhysXPage = value;
	}
}

namespace Echo {

#ifdef USE_PHYSICS_HELPEROBJ
	void ColliderComponent::CreateHelper() {
		if (m_pHelperObj) return;
		Name helperName = Name(getCombinedName().append("||").append("HelperObj"));
		m_pHelperObj = new HelperObject(helperName, HelperObjType_Box, false, nullptr, getSceneManager());
		WorldNode* pSN = getSceneNode();
		if (pSN) {
			pSN->addChild(m_pHelperObj->getSceneNode());
		}
		m_pHelperObj->setVisible(m_IsHelperVisible);
		m_pHelperObj->setRenderType(ECHO_DRAW::ECHO_LINE_LIST);
		m_pHelperObj->setOrientation(Quaternion::IDENTITY);
		m_pHelperObj->setScale(Vector3::UNIT_SCALE);
		ResetHelperColor();
}
	void ColliderComponent::ReleaseHelper() {
		if (m_pHelperObj == NULL) return;
		WorldNode* pSN = getSceneNode();
		if (pSN) {
			pSN->removeChild(m_pHelperObj->getSceneNode());
		}
		delete m_pHelperObj;
		m_pHelperObj = NULL;
	}
	void ColliderComponent::UpdateHelper() {
		if (!m_pHelperObj || !m_pHelperObj->getVisible()) {
			return;
		}
		std::vector<Vector3> vbVec;
		std::vector<uint16> ibVec;
		GenerateVBIBCB(vbVec, ibVec);
		if (vbVec.empty() || ibVec.empty()) {
			m_pHelperObj->clearBuffer();
		}
		m_pHelperObj->changeCustomPolygon(vbVec, ibVec);
	}
	void ColliderComponent::UpdateComponentInEditor() {
		if (m_IsHelperVisible) {
			if (!m_pHelperObj && IsCanPhysics()) {
				CreateHelper();
			}
			if (m_pHelperObj) {
				m_pHelperObj->getSceneNode()->_setDerivedPosition(getWorldPosition());
				m_pHelperObj->getSceneNode()->_setDerivedOrientation(getWorldRotation());
				m_pHelperObj->getSceneNode()->_setDerivedScale(getWorldScale());
				UpdateHelper();
			}
		}
		else if (m_pHelperObj) {
			ReleaseHelper();
		}
	}
#endif // USE_PHYSICS_HELPEROBJ

	void ColliderComponent::SetHelperVisible(bool value) {
		m_IsHelperVisible = value;
		AddToEditorUpdateQueue();
	}
	bool ColliderComponent::GetHelperVisible() {
		return m_IsHelperVisible;
	}
	void ColliderComponent::SetHelperColorType(HelperObjColorType color) {
#ifdef USE_PHYSICS_HELPEROBJ
		if (m_pHelperObj) m_pHelperObj->setHelperColorType(color);
#endif // USE_PHYSICS_HELPEROBJ
	}
	HelperObjColorType ColliderComponent::GetHelperColorType() {
#ifdef USE_PHYSICS_HELPEROBJ
		if (m_pHelperObj) {
			return m_pHelperObj->getHelperColorType();
	}
		return (HelperObjColorType)PhysXColor::gNotExistPhysics;
#else
		return (HelperObjColorType)PhysXColor::gNotExistPhysics;
#endif // USE_PHYSICS_HELPEROBJ
	}

}

namespace Echo {

	NODE_ACTOR_COM_CPP(NodeColliderComponent)
	
	NodeColliderComponent::NodeColliderComponent(ActorPtr pActor, ActorCOMMoveableType type) : ColliderComponent(pActor, type) {}
	NodeColliderComponent::~NodeColliderComponent() { }
	void NodeColliderComponent::NotifiChildImp(int changeMask) {
		if ((uint8)changeMask == WorldNode::TSC_NONE) return;
		if (!m_pAttachRigidBody || m_pAttachRigidBody->IsNeedChangeCollider()) {
			if (((uint8)changeMask & WorldNode::TSC_SCALE) == WorldNode::TSC_SCALE) {
				ChangeLocalPose();
				ChangeShape();
			}
			else {
				ChangeLocalPose();
				UpdatePhysXPage();
			}
		}
	}
#ifdef USE_PHYSICS_HELPEROBJ
	void NodeColliderComponent::UpdateComponentInEditor() {
		if (m_IsHelperVisible) {
			if (!m_pHelperObj && IsCanPhysics()) {
				CreateHelper();
			}
			if (m_pHelperObj) {
				UpdateHelper();
			}
		}
		else if (m_pHelperObj) {
			ReleaseHelper();
		}
	}
#endif // USE_PHYSICS_HELPEROBJ

}
