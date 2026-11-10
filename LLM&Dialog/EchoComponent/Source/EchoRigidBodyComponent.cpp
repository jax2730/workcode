#include "EchoComponentStableHeaders.h"
#include "EchoRigidBodyComponent.h"
#include "EchoActorProFuncImp.h"
#include "EchoPhysXScene.h"
#include "EchoPhysXAsset.h"
#include "EchoPhysXCommon.h"
#include "Actor/EchoActorHelper.h"
#include "Actor/EchoNodeComponent.h"
#include "EchoColliderComponent.h"
#include "EchoPhysXManager.h"

namespace Echo{

	RigidBodyComponentData::RigidBodyComponentData() {}
	RigidBodyComponentData::~RigidBodyComponentData() {}
	RigidBodyComponentData& RigidBodyComponentData::operator=(const RigidBodyComponentData& other) {
		if (this == &other) return *this;
		UseGravity = other.UseGravity;
		IsKinematic = other.IsKinematic;
		CollisionDetection = other.CollisionDetection;
		Constraints = other.Constraints;
		Mass = other.Mass;
		Drag = other.Drag;
		AngularDrag = other.AngularDrag;
		CenterOfMassMode = other.CenterOfMassMode;
		CenterOfMassOffset = other.CenterOfMassOffset;
		LocalCenterofMass = other.LocalCenterofMass;
		InertiaTensor = other.InertiaTensor;
		InertiaTensorRotation = other.InertiaTensorRotation;
		return *this;
	}

	DECLAREACTORCOMTYPE_CPP(RigidBodyComponent)

	//-----------------------------------------------------------------------------------------

	RigidBodyComponentInfo::RigidBodyComponentInfo(const Name& comType, ActorCOMMoveableType MoveType) : ActorComponentInfo(comType,MoveType) {}
	RigidBodyComponentInfo::~RigidBodyComponentInfo() {}

	EDITOR_REFLECT_BEGIN(RigidBodyComponentInfo)
		EDITOR_REFLECT_MEMBER_UPDATE(m_UseGravity, "UseGravity", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_IsKinematic, "IsKinematic", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_CollisionDetection, "CollisionDetection", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_Constraints, "Constraints", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_Mass, "Mass", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_Drag, "Drag", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_AngularDrag, "AngularDrag", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_CenterOfMassMode, "CenterOfMassMode", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_CenterOfMassOffset, "CenterOfMassOffset", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_LocalCenterofMass, "LocalCenterofMass", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_InertiaTensor, "InertiaTensor", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_InertiaTensorRotation, "InertiaTensorRotation", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_END();

	_DEFINEINFO_VIRFUN_BASE(RigidBodyComponentInfo)

	void RigidBodyComponentInfo::CopyToCompData(RigidBodyComponentData* pData) {
		if (!pData) return;
		pData->UseGravity	= m_UseGravity;
		pData->IsKinematic	= m_IsKinematic;
		pData->CollisionDetection = m_CollisionDetection;
		pData->Constraints = m_Constraints;
		pData->Mass = m_Mass;
		pData->Drag = m_Drag;
		pData->AngularDrag = m_AngularDrag;
		pData->CenterOfMassMode = m_CenterOfMassMode;
		pData->CenterOfMassOffset = m_CenterOfMassOffset;

		pData->LocalCenterofMass = m_LocalCenterofMass;
		pData->InertiaTensor = m_InertiaTensor;
		pData->InertiaTensorRotation = m_InertiaTensorRotation;
	}

	//-----------------------------------------------------------------------------------------

	RigidBodyComponent::RigidBodyComponent(ActorPtr pActor, ActorCOMMoveableType type) : PhysicsActorComponent(pActor,type){}
	RigidBodyComponent::~RigidBodyComponent() {
		SAFE_DELETE(m_pForceAndTorque);
	}

	void RigidBodyComponent::init(ActorComponentInfo* info) {
		PhysicsBaseComponent::init(info);
		RigidBodyComponentInfo* pInfo = (RigidBodyComponentInfo*)info;
		if (pInfo) {
			pInfo->CopyToCompData(&mCompData);
		}
		SetNotifiedUpdate(true);
		mPhysicsParentingChanged = std::bind<void>(&RigidBodyComponent::OnParentingChanged, this, std::placeholders::_1);
	}
	void RigidBodyComponent::unInit() {
		GetPhysicsManager()->RemoveRigidBodyFromUpdateMAITQueue(this);
		GetPhysicsManager()->RemoveRigidBodyFromUpdatePhysXTransformQueue(this);
		ReleasePhysXObject();
		SetNotifiedUpdate(false);
		mPhysicsParentingChanged = nullptr;
		PhysicsBaseComponent::unInit();
	}
	void RigidBodyComponent::connectCom() {
		PhysicsBaseComponent::connectCom();
		CreatePhysXObject();
		auto& parent = getParentNodeCom();
		if (parent != nullptr && getRealVisibleState()) {
			parent->NotifiAllChildParentingChanged(weak_from_this(), false);
		}
		mParentRigidBodyPtr = EchoPhysicsManager::FindNewRigidBodyComp(getParentNodeCom(), this);
	}
	void RigidBodyComponent::disconnectCom() {
		RemoveSelfCollider();
		RemoveSelfLink();
		ReleasePhysXObject();

		mParentRigidBodyPtr = nullptr;
		auto& parent = getParentNodeCom();
		if (parent != nullptr) {
			RigidBodyComponentPtr pRB = EchoPhysicsManager::FindNewRigidBodyComp(parent->getParentNodeCom(), this);
			parent->NotifiAllChildParentingChanged(pRB.getStaticCastWeakPtr<ActorComponent>(), false);
		}
		PhysicsBaseComponent::disconnectCom();
	}
	void RigidBodyComponent::OnParentingChanged(const ActorComponentPtr& compPtr) {
		if (compPtr == this) {
			mParentRigidBodyPtr = EchoPhysicsManager::FindNewRigidBodyComp(getParentNodeCom(), this);
		}
		else {
			if (compPtr == nullptr) {
				mParentRigidBodyPtr = nullptr;
			}
			else {
				mParentRigidBodyPtr = compPtr.getStaticCastWeakPtr<RigidBodyComponent>();
			}
		}
	}
	uint32 RigidBodyComponent::GetRigidBodyTreeLevel() const {
		if (mParentRigidBodyPtr) {
			return mParentRigidBodyPtr->GetRigidBodyTreeLevel() + 1;
		}
		else {
			return 0;
		}
	}
	void RigidBodyComponent::replay(ActorComponentInfo* info) {
		RigidBodyComponentInfo* pInfo = (RigidBodyComponentInfo*)info;
		if (pInfo) {
			pInfo->CopyToCompData(&mCompData);
		}
	}
	void RigidBodyComponent::copyTo(const ActorComponentPtr& actCom) {
		if (!actCom) return;
		actCom.getDynamicCastWeakPtr<RigidBodyComponent>()->SetCompData(&mCompData);
	}
	void RigidBodyComponent::RemoveSelfCollider() {
		if (!m_ColliderList.empty()) {
			std::vector<ColliderComponentPtr> tempVec(m_ColliderList.begin(), m_ColliderList.end());
			for (auto& pCollider : tempVec) {
				if(pCollider != nullptr) pCollider->DetachShape();
			}
			m_ColliderList.clear();
			tempVec.clear();
		}
	}
	void RigidBodyComponent::DetachSelfCollider() {
		if (!m_ColliderList.empty()) {
			for (auto& pCollider : m_ColliderList) {
				if (pCollider != nullptr) {
					if (pCollider->IsExistPhysics()) {
						m_pRigidBody->DetachShape(*pCollider->m_pShape);
					}
				}
			}
		}
		ResetMassAndInertiaTensor();
	}
	void RigidBodyComponent::AttachSelfCollider() {
		if (!m_ColliderList.empty()) {
			for (auto& pCollider : m_ColliderList) {
				if (pCollider != nullptr) {
					if (pCollider->IsExistPhysics()) {
						m_pRigidBody->AttachShape(*pCollider->m_pShape);
						pCollider->AttachSelfLink();
					}
				}
			}
		}
		ResetMassAndInertiaTensor();
	}
	void RigidBodyComponent::ResetAttachSelfCollider() {
		const auto& parent = getParentNodeCom();
		if (parent != nullptr && getRealVisibleState()) {
			parent->NotifiAllChildParentingChanged(weak_from_this(), false);
		}
	}
	bool RigidBodyComponent::CheckSelfCollider() {
		if (m_pRigidBody == nullptr || m_ColliderList.empty() || GetIsKinematic()) return true;
		std::vector<PhysXShape*> shapes;
		m_pRigidBody->GetShapes(shapes);
		for (PhysXShape* shape : shapes) {
			if (shape == nullptr || shape->GetGeometryType() == EchoGeometryType::eTRIANGLEMESH) {
				return false;
			}
		}
		return true;
	}
	void RigidBodyComponent::AddCollider(ColliderComponentPtr& pComp) {
		if (!pComp) return;
		m_ColliderList.insert(pComp);
		if (!m_pRigidBody || !pComp->IsExistPhysics()) return;
		EchoAssert(false, "RigidBodyComponent::AddCollider is ErrorState.");
		AttachShapeInternal(pComp->m_pShape);
	}
	void RigidBodyComponent::RemoveCollider(ColliderComponentPtr& pComp) {
		if (!pComp) return;
		m_ColliderList.erase(pComp);
		if (!m_pRigidBody || !pComp->IsExistPhysics()) return;
		EchoAssert(false, "RigidBodyComponent::RemoveCollider is ErrorState.");
		DetachShapeInternal(pComp->m_pShape);
	}
	void RigidBodyComponent::AttachShapeInternal(PhysXShape* pShape) {
		if (m_pRigidBody == nullptr) return;
		m_pRigidBody->AttachShape(*pShape);
		ResetMassAndInertiaTensor();
	}
	void RigidBodyComponent::DetachShapeInternal(PhysXShape* pShape) {
		if (m_pRigidBody == nullptr) return;
		//m_pRigidBody->DetachShape(*pShape);
		ResetMassAndInertiaTensor();
	}
	const std::unordered_set<ColliderComponentPtr>& RigidBodyComponent::GetColliderCompList() {
		return m_ColliderList;
	}
	AxisAlignedBox RigidBodyComponent::GetPhysicsWorldAABB() {
		AxisAlignedBox box = GetPhysicsLocalAABB();
		EchoPhysicsManager::AxisAlignedBoxLocalToWorld(box, getWorldPosition(), getWorldRotation());
		return box;
	}
	const AxisAlignedBox& RigidBodyComponent::GetPhysicsLocalAABB() {
		if (mPhysicsLocalAABBChanged) {
			UpdatePhysicsLocalAABB();
		}
		return mPhysicsLocalAABB;
	}
	void RigidBodyComponent::UpdatePhysicsLocalAABB() {
		mPhysicsLocalAABBChanged = false;
		if (m_ColliderList.empty()) {
			mPhysicsLocalAABB = AxisAlignedBox(-0.5f, -0.5f, -0.5f, 0.5f, 0.5f, 0.5f);
		}
		else {
			mPhysicsLocalAABB.Empty();
			DTransform attT = { getWorldPosition(), getWorldRotation() };
			for (auto& iter : m_ColliderList) {
				if (iter != nullptr) {
					AxisAlignedBox box;
					EchoPhysicsManager::computePhysicsLocalAABB(iter.getPointer(), box);
					DTransform myT = { iter->getWorldPosition(), iter->getWorldRotation() };
					Transform resT;
					EchoPhysicsManager::convertWorldToLocal(attT, myT, resT);
					EchoPhysicsManager::AxisAlignedBoxLocalToWorld(box, resT.translation, resT.rotation);
					mPhysicsLocalAABB.Add(box);
				}
			}
		}
	}
	bool RigidBodyComponent::IsNeedChangeCollider() {
		return m_IsUpdatePhysicsTransform;
	}
	void RigidBodyComponent::CreatePhysics() {
		if (!GetPhysicsManager()->IsPhysicsUpdating()) return;
		if (m_pRigidBody != nullptr) return;
		//获取actor在scene中的位置和姿态
		Vector3 position = getParentNodeCom()->getWorldPosition();
		Quaternion rotation = getParentNodeCom()->getWorldRotation();

		PhysXRigidDynamicInfo info;
		info.shape = nullptr;
		info.userdata = this;
		info.usertype = (int)PhysXUserTypeEnum::ActorComponent;
		info.transform = Transform{ position, rotation };
		info.surface = 0;
		info.mass = mCompData.Mass;
		info.drag = mCompData.Drag;
		info.angularDrag = mCompData.AngularDrag;
		info.useGravity = mCompData.UseGravity;
		info.isKinematic = mCompData.IsKinematic;
		info.lockFlage = (RigidbodyConstraints)mCompData.Constraints;
		info.collisionMode = (CollisionDetectionMode)mCompData.CollisionDetection;

		m_pRigidBody = GetPhysicsManager()->GetScene()->AddRigidDynamic(info);
		
		if (m_pRigidBody == nullptr) return;

		m_pRigidBody->SetSolverIterationCounts(mSolverPositionIterations, mSolverVelocityIterations);
		m_pRigidBody->SetMaxDepenetrationVelocity(mMaxDepenetrationVelocity);
		m_pRigidBody->SetMaxAngularVelocity(mMaxAngularVelocity);
		m_pRigidBody->SetMaxLinearVelocity(mMaxLinearVelocity);
		m_pRigidBody->SetMaxContactImpulse(mMaxContactImpulse);
		m_pRigidBody->SetSleepThreshold(mSleepThreshold);
		m_pRigidBody->SetLinearVelocity(mVelocity);
		m_pRigidBody->SetAngularVelocity(mAngularVelocity);

		ApplyForceAndTorque();
		SAFE_DELETE(m_pForceAndTorque);

		AttachSelfCollider();
		AttachSelfLink();
		SendCreateEvent();
	}
	void RigidBodyComponent::ReleasePhysics() {
		if (!m_pRigidBody) return;
		DetachSelfLink();

		mVelocity = Vector3::ZERO;
		mAngularVelocity = Vector3::ZERO;

		GetPhysicsManager()->RemovePhysXActor(m_pRigidBody);
		m_pRigidBody = nullptr;

		SendReleaseEvent();
	}
	void RigidBodyComponent::NotifiChildImp(int changeMask) {
		if ((uint8)changeMask == WorldNode::TSC_NONE) return;
		if (!m_pRigidBody) {
			UpdatePhysXPage();
		}
		else if (m_IsUpdatePhysicsTransform) {
			if (GetIsKinematic() && !GetPhysicsManager()->IsRigidBodyMoveing() && m_IsAutoUpdateKinematicTarget)
			{
				DTransform trans(getParentNodeCom()->getWorldPosition(), getParentNodeCom()->getWorldRotation());
				SetIsKinematicImpl(trans, false);
			}
			else
			{
				GetPhysicsManager()->AddRigidBodyToUpdatePhysXTransformQueue(this);
			}
			UpdatePhysXPage();
		}
	}

	void RigidBodyComponent::visibleStateChanged(bool activeState) {
		if (!GetPhysicsManager()) return;
		auto& parent = getParentNodeCom();
		if (parent == nullptr) return;
		if (activeState) {
			parent->NotifiAllChildParentingChanged(weak_from_this(), false);
			CreatePhysXObject();
		}
		else {
			RigidBodyComponentPtr pRB = EchoPhysicsManager::FindNewRigidBodyComp(parent->getParentNodeCom(), this);
			parent->NotifiAllChildParentingChanged(pRB.getStaticCastWeakPtr<ActorComponent>(), false);
			ReleasePhysXObject();
		}
	}

	Name physicsWorldPositionName_ = Name("PhysicsWorldPosition");
	Name physicsWorldRotationName_ = Name("PhysicsWorldRotation");
	Name physicsLocalPositionName_ = Name("PhysicsLocalPosition");
	Name physicsLocalRotationName_ = Name("PhysicsLocalRotation");

	void RigidBodyComponent::setTimelineAttribute(std::vector<float>& data,const Name& name) {
	
		if (name == physicsWorldPositionName_) {
			MoveWorldPosition(Vector3(data[0], data[1], data[2]));
		} else if (name == physicsWorldRotationName_) {
			Vector3	eulerAngles = Vector3(data[0], data[1], data[2]);
			Quaternion rotation;
			rotation.FromEuler(eulerAngles);
			MoveWorldRotation(rotation);
		}
		else if (name == physicsLocalPositionName_) {
			MovePosition(Vector3(data[0], data[1], data[2]));
		}
		else if (name == physicsLocalRotationName_) {
			Vector3	eulerAngles = Vector3(data[0], data[1], data[2]);
			Quaternion rotation;
			rotation.FromEuler(eulerAngles);
			MoveRotation(rotation);
		}
	}

	bool RigidBodyComponent::MoveWorldPosition(const DVector3& position) {
		if (!m_pRigidBody || !GetIsKinematic()) return false;
		DTransform trans;
		if (!m_pRigidBody->GetKinematicTarget(trans)) {
			trans.rotation = getParentNodeCom()->getWorldRotation();
		}
		trans.translation = position;
		SetIsKinematicImpl(trans, true);
		return true;
	}
	bool RigidBodyComponent::MoveWorldRotation(const Quaternion& rotation) {
		if (!m_pRigidBody || !GetIsKinematic()) return false;
		DTransform trans;
		if (!m_pRigidBody->GetKinematicTarget(trans)) {
			trans.translation = getParentNodeCom()->getWorldPosition();
		}
		trans.rotation = rotation;
		SetIsKinematicImpl(trans, true);
		return true;
	}
	bool RigidBodyComponent::MoveWorldTransform(const DTransform& trans) {
		if (!m_pRigidBody || !GetIsKinematic()) return false;
		SetIsKinematicImpl(trans, true);
		return true;
	}



	bool RigidBodyComponent::MovePosition(const DVector3& position) {
		if (!m_pRigidBody || !GetIsKinematic()) return false;
		
		DTransform trans;
		if (!m_pRigidBody->GetKinematicTarget(trans)) {
			trans.rotation = getParentNodeCom()->getWorldRotation();
		}
		trans.translation = position;

		const ActorComponentPtr& pp = getParentNodeCom()->getParentNodeCom();
		if (pp) {
			trans.translation = pp->convertLocalToWorldPosition(position);
		}
		else {
			trans.translation = position;
		}
		SetIsKinematicImpl(trans, true);
		return true;
	}
	bool RigidBodyComponent::MoveRotation(const Quaternion& rotation) {
		if (!m_pRigidBody || !GetIsKinematic()) return false;

		DTransform trans;
		if (!m_pRigidBody->GetKinematicTarget(trans)) {
			trans.translation = getParentNodeCom()->getWorldPosition();
		}
		trans.rotation = rotation;

		const ActorComponentPtr& pp = getParentNodeCom()->getParentNodeCom();
		if (pp) {
			trans.rotation = pp->convertLocalToWorldRotation(rotation);
		}
		else {
			trans.rotation = rotation;
		}
		SetIsKinematicImpl(trans, true);
		return true;
	}
	bool RigidBodyComponent::MoveTransform(const DTransform& transform) {
		if (!m_pRigidBody || !GetIsKinematic()) return false;
		const ActorComponentPtr& pp = getParentNodeCom()->getParentNodeCom();
		DTransform trans = transform;
		if (pp) {
			trans = DTransform(pp->convertLocalToWorldPosition(transform.translation), pp->convertLocalToWorldRotation(transform.rotation));
		}
		SetIsKinematicImpl(trans, true);
		return true;
	}

	void RigidBodyComponent::_SetSolverIterationCounts() {
		m_pRigidBody->SetSolverIterationCounts(mSolverPositionIterations, mSolverVelocityIterations);
	}
	void RigidBodyComponent::_GetSolverIterationCounts() {
		m_pRigidBody->GetSolverIterationCounts(mSolverPositionIterations, mSolverVelocityIterations);
	}

	void RigidBodyComponent::SetSolverIterations(uint32 value) {
		mSolverPositionIterations = value;
		if (!m_pRigidBody) return;
		_SetSolverIterationCounts();
	}
	uint32 RigidBodyComponent::GetSolverIterations() {
		return mSolverPositionIterations;
	}

	void RigidBodyComponent::SetSolverVelocityIterations(uint32 value) {
		mSolverVelocityIterations = value;
		if (!m_pRigidBody) return;
		_SetSolverIterationCounts();
	}
	uint32 RigidBodyComponent::GetSolverVelocityIterations() {
		return mSolverVelocityIterations;
	}

	void RigidBodyComponent::SetMaxDepenetrationVelocity(float value) {
		mMaxDepenetrationVelocity = value;
		if (!m_pRigidBody) return;
		m_pRigidBody->SetMaxDepenetrationVelocity(value);
	}
	float RigidBodyComponent::GetMaxDepenetrationVelocity() {
		return mMaxDepenetrationVelocity;
	}

	void RigidBodyComponent::SetMaxAngularVelocity(float value) {
		mMaxAngularVelocity = value;
		if (!m_pRigidBody) return;
		m_pRigidBody->SetMaxAngularVelocity(value);
	}
	float RigidBodyComponent::GetMaxAngularVelocity() {
		return mMaxAngularVelocity;
	}

	void RigidBodyComponent::SetMaxLinearVelocity(float value) {
		mMaxLinearVelocity = value;
		if (!m_pRigidBody) return;
		m_pRigidBody->SetMaxLinearVelocity(value);
	}
	float RigidBodyComponent::GetMaxLinearVelocity() {
		return mMaxLinearVelocity;
	}

	void RigidBodyComponent::SetMaxContactImpulse(float value) {
		mMaxContactImpulse = value;
		if (!m_pRigidBody) return;
		m_pRigidBody->SetMaxContactImpulse(value);
	}
	float RigidBodyComponent::GetMaxContactImpulse() {
		return mMaxContactImpulse;
	}

	void RigidBodyComponent::SetSleepThreshold(float value) {
		mSleepThreshold = value;
		if (!m_pRigidBody) return;
		m_pRigidBody->SetSleepThreshold(value);
	}
	float RigidBodyComponent::GetSleepThreshold() {
		return mSleepThreshold;
	}

	bool RigidBodyComponent::SetDensity(float density) {
		if (!m_pRigidBody) return false;
		if (!m_pRigidBody->SetDensity(density)) {
			return false;
		}
		mCompData.Mass = m_pRigidBody->GetMass();
		return true;
	}

	void PhysXSweepHitToEchoRaycastHit(PhysXRigidDynamic::SweepHit& physXhit, EchoRaycastHit& hit) {
		EchoPhysicsManager::PhysXPtrToEchoPtr(physXhit.actor, physXhit.shape, hit.rigidbody, hit.collider);
		hit.distance = physXhit.distance;
		hit.normal = physXhit.normal;
		hit.point = physXhit.position;
		if (hit.collider != nullptr) {
			hit.surfaceType = hit.collider->GetSurfaceType(hit.point);
		}
		else {
			hit.surfaceType = 0;
		}
	}

	bool RigidBodyComponent::SweepTest(const Vector3& direction, EchoRaycastHit& hitInfo, float maxDistance) {
		if (!m_pRigidBody) return false;

		Vector3 dirNor = direction.GetNormalized();
		if (dirNor.isAlmostZero()) {
			return false;
		}
		maxDistance = Math::Clamp(std::abs(maxDistance), 0.00001f, FLT_MAX);
		
		PhysXRigidDynamic::SweepHit hit;
		if (!m_pRigidBody->Sweep(dirNor, maxDistance, hit)) {
			return false;
		}
		PhysXSweepHitToEchoRaycastHit(hit, hitInfo);
		return true;
	}

	bool RigidBodyComponent::SweepTest(const Vector3& direction, std::vector<EchoRaycastHit>& hitInfos, float maxDistance) {
		if (!m_pRigidBody) return false;

		Vector3 dirNor = direction.GetNormalized();
		if (dirNor.isAlmostZero()) {
			return false;
		}
		maxDistance = Math::Clamp(std::abs(maxDistance), 0.00001f, FLT_MAX);

		std::vector<PhysXRigidDynamic::SweepHit> hits;
		if (!m_pRigidBody->Sweep(dirNor, maxDistance, hits)) {
			return false;
		}
		for (int i = 0; i != hits.size(); ++i) {
			PhysXSweepHitToEchoRaycastHit(hits[i], hitInfos.emplace_back());
		}
		return true;
	}

	bool RigidBodyComponent::IsCanPhysics() {
		return true;
	}
	bool RigidBodyComponent::IsExistPhysics() {
		return m_pRigidBody != nullptr;
	}

	PhysXRigidDynamic* RigidBodyComponent::GetPhysics() {
		return m_pRigidBody;
	}

	void RigidBodyComponent::SetIsKinematicImpl(const DTransform& trans, bool bMove) {
		m_pRigidBody->SetKinematicTarget(trans);
		m_IsKinematicTarget = bMove;
	}

	void RigidBodyComponent::SetWorldTransformPhysX(const DTransform& trans) {
		m_IsUpdatePhysicsTransform = false;
		getParentComponentNode()->setWorldTransform(trans, 1e-6f);
		if (GetIsKinematic()) {
			SetIsKinematicImpl(trans, false);
		}
		else {
			GetPhysicsManager()->AddRigidBodyToUpdatePhysXTransformQueue(this);
		}
		UpdatePhysXPage();
		m_IsUpdatePhysicsTransform = true;
	}

	void RigidBodyComponent::SetWorldTransformFromPhysX(const DTransform& trans) {
		m_IsUpdatePhysicsTransform = false;
		bool bUpdateComp = true;
		if (GetIsKinematic()) {
			if (m_IsKinematicTarget) {
				m_IsKinematicTarget = false;	
			}
			else {
				bUpdateComp = false;
			}
		}
		if (bUpdateComp) {
			getParentComponentNode()->setWorldTransform(trans, 1e-6f);
			UpdatePhysXPage();
		}
		m_IsUpdatePhysicsTransform = true;
	}
	void RigidBodyComponent::SetLocalTransformFromPhysX(const DTransform& trans) {
		m_IsUpdatePhysicsTransform = false;
		bool bUpdateComp = true;
		if (GetIsKinematic()) {
			if (m_IsKinematicTarget) {
				m_IsKinematicTarget = false;
			}
			else {
				bUpdateComp = false;
			}
		}
		if (bUpdateComp) {
			getParentComponentNode()->setTransform(trans, 1e-6f);
			UpdatePhysXPage();
		}
		m_IsUpdatePhysicsTransform = true;
	}

	void RigidBodyComponent::SyncWorldTransformToPhysX() {
		if (!m_pRigidBody) return;
		DTransform trans(getParentNodeCom()->getWorldPosition(), getParentNodeCom()->getWorldRotation());
		m_pRigidBody->SetTransform(trans);
	}
	void RigidBodyComponent::SetDrag(float drag) {
		mCompData.Drag = drag;
#ifdef ECHO_EDITOR
		if (ActorHelper::getForceSave() && m_pActComInfo)
		{
			RigidBodyComponentInfo* colliderInfo = static_cast<RigidBodyComponentInfo*>(m_pActComInfo);
			colliderInfo->SetDrag(drag);
		}
#endif
		if (!m_pRigidBody) return;
		m_pRigidBody->SetLinearDamping(drag);
	}
	void RigidBodyComponent::SetAngularDrag(float angularDrag) {
		mCompData.AngularDrag = angularDrag;
#ifdef ECHO_EDITOR
		if (ActorHelper::getForceSave() && m_pActComInfo)
		{
			RigidBodyComponentInfo* colliderInfo = static_cast<RigidBodyComponentInfo*>(m_pActComInfo);
			colliderInfo->SetAngularDrag(angularDrag);
		}
#endif
		if (!m_pRigidBody) return;
		m_pRigidBody->SetAngularDamping(angularDrag);
	}
	void RigidBodyComponent::SetUseGravity(bool useGravity) {
		mCompData.UseGravity = useGravity;
#ifdef ECHO_EDITOR
		if (ActorHelper::getForceSave() && m_pActComInfo)
		{
			RigidBodyComponentInfo* colliderInfo = static_cast<RigidBodyComponentInfo*>(m_pActComInfo);
			colliderInfo->SetUseGravity(useGravity);
		}
#endif
		if (!m_pRigidBody) return;
		m_pRigidBody->ToggleGravity(useGravity);
		WakeUp();
	}
	void RigidBodyComponent::SetIsKinematic(bool isKinematic) {
		bool change = mCompData.IsKinematic != isKinematic;
		mCompData.IsKinematic = isKinematic;
#ifdef ECHO_EDITOR
		if (ActorHelper::getForceSave() && m_pActComInfo)
		{
			RigidBodyComponentInfo* colliderInfo = static_cast<RigidBodyComponentInfo*>(m_pActComInfo);
			colliderInfo->SetIsKinematic(isKinematic);
		}
#endif
		if (!change) return;
		if (!CheckSelfCollider()) {
			mCompData.IsKinematic = !isKinematic;
			return;
		}
		ResetAttachSelfCollider();
		if (!m_pRigidBody) return;
		m_pRigidBody->ToggleKinematic(isKinematic);
		mCompData.CollisionDetection = m_pRigidBody->GetCollisionDetectionMode();
	}
	void RigidBodyComponent::SetCollisionDetection(int collisionDetection) {
		mCompData.CollisionDetection = collisionDetection;
#ifdef ECHO_EDITOR
		if (ActorHelper::getForceSave() && m_pActComInfo)
		{
			RigidBodyComponentInfo* colliderInfo = static_cast<RigidBodyComponentInfo*>(m_pActComInfo);
			colliderInfo->SetCollisionDetection(collisionDetection);
		}
#endif
		if (!m_pRigidBody) return;
		m_pRigidBody->SetCollisionDetectionMode((CollisionDetectionMode)collisionDetection);
		mCompData.CollisionDetection = m_pRigidBody->GetCollisionDetectionMode();

	}
	void RigidBodyComponent::SetConstraints(int constraints) {
		mCompData.Constraints = constraints;
#ifdef ECHO_EDITOR
		if (ActorHelper::getForceSave() && m_pActComInfo)
		{
			RigidBodyComponentInfo* colliderInfo = static_cast<RigidBodyComponentInfo*>(m_pActComInfo);
			colliderInfo->SetConstraints(constraints);
		}
#endif
		if (!m_pRigidBody) return;
		m_pRigidBody->SetLockFlage((RigidbodyConstraints)constraints);
		WakeUp();
	}

	float RigidBodyComponent::GetDrag(){
		return mCompData.Drag;
	}
	float RigidBodyComponent::GetAngularDrag(){
		return mCompData.AngularDrag;
	}
	bool RigidBodyComponent::GetUseGravity(){
		return mCompData.UseGravity;
	}
	bool RigidBodyComponent::GetIsKinematic(){
		return mCompData.IsKinematic;
	}
	int RigidBodyComponent::GetCollisionDetection(){
		return mCompData.CollisionDetection;
	}
	int RigidBodyComponent::GetConstraints(){
		return mCompData.Constraints;
	}

	void RigidBodyComponent::AddForceAtPosition(const Vector3& force, const DVector3& position, int mode) {
		if (force.IsNan() || force.isAlmostZero()) return;
		if (!m_pRigidBody) {
			Vector3 torce = Vector3::VCross((position - getParentNodeCom()->getWorldPosition()), force);
			GetModeForceOrTorque(mode, true) += force;
			GetModeForceOrTorque(mode, false) += torce;
		}
		else {
			m_pRigidBody->AddForceAtPos(force, position, (PhysXRigidDynamic::ForceMode)mode);
		}
	}
	void RigidBodyComponent::AddForce(const Vector3& force, int mode) {
		if (force.IsNan() || force.isAlmostZero()) return;
		if (!m_pRigidBody) {
			GetModeForceOrTorque(mode, true) += force;
		}
		else {
			m_pRigidBody->AddForce(force, (PhysXRigidDynamic::ForceMode)mode);
		}
	}
	void RigidBodyComponent::AddRelativeForce(const Vector3& force, int mode) {
		if (force.IsNan() || force.isAlmostZero()) return;
		Vector3 worldForce = getParentNodeCom()->getWorldRotation() * force;
		if (!m_pRigidBody) {
			GetModeForceOrTorque(mode, true) += worldForce;
		}
		else {	
			m_pRigidBody->AddForce(worldForce, (PhysXRigidDynamic::ForceMode)mode);
		}
	}
	void RigidBodyComponent::AddTorque(const Vector3& torque, int mode) {
		if (torque.IsNan() || torque.isAlmostZero()) return;
		if (!m_pRigidBody) {
			GetModeForceOrTorque(mode, false) += torque;
		}
		else {
			m_pRigidBody->AddTorque(torque, (PhysXRigidDynamic::ForceMode)mode);
		}
	}
	void RigidBodyComponent::AddRelativeTorque(const Vector3& torque, int mode) {
		if (torque.IsNan() || torque.isAlmostZero()) return;
		Vector3 worldTorque = getParentNodeCom()->getWorldRotation() * torque;
		if (!m_pRigidBody) {
			GetModeForceOrTorque(mode, false) += worldTorque;
		}
		else {
			m_pRigidBody->AddTorque(worldTorque, (PhysXRigidDynamic::ForceMode)mode);
		}
	}
	void RigidBodyComponent::ClearForce(int mode) {
		if (!m_pRigidBody) {
			GetModeForceOrTorque(mode, true) = Vector3::ZERO;
		}
		else {
			m_pRigidBody->ClearForce((PhysXRigidDynamic::ForceMode)mode);
		}
	}
	void RigidBodyComponent::ClearTorque(int mode) {
		if (!m_pRigidBody) {
			GetModeForceOrTorque(mode, false) = Vector3::ZERO;
		}
		else {
			m_pRigidBody->ClearTorque((PhysXRigidDynamic::ForceMode)mode);
		}
	}
	Vector3& RigidBodyComponent::GetModeForceOrTorque(int mode, bool bLinearOrAngular) {
		if (m_pForceAndTorque == nullptr) {
			m_pForceAndTorque = new ForceAndTorqueData();
		}
		if (bLinearOrAngular) {
			switch (mode) {
			case EchoPhysics::FORCE: {
				return m_pForceAndTorque->mSpatialLinearForce;
			}
			case EchoPhysics::ACCELERATION: {
				return m_pForceAndTorque->mSpatialLinearAcceleration;
			}
			case EchoPhysics::IMPULSE: {
				return m_pForceAndTorque->mSpatialLinearImpulse;
			}
			case EchoPhysics::VELOCITY_CHANGE: {
				return m_pForceAndTorque->mSpatialLinearVelocity;
			}
			default: {
				return m_pForceAndTorque->mSpatialLinearForce;
			}
			}
		}
		else {
			switch (mode) {
			case EchoPhysics::FORCE: {
				return m_pForceAndTorque->mSpatialAngularForce;
			}
			case EchoPhysics::ACCELERATION: {
				return m_pForceAndTorque->mSpatialAngularAcceleration;
			}
			case EchoPhysics::IMPULSE: {
				return m_pForceAndTorque->mSpatialAngularImpulse;
			}
			case EchoPhysics::VELOCITY_CHANGE: {
				return m_pForceAndTorque->mSpatialAngularVelocity;
			}
			default: {
				return m_pForceAndTorque->mSpatialAngularForce;
			}
			}
		}
	}

	void RigidBodyComponent::ApplyForceAndTorque() {
		if (m_pForceAndTorque == nullptr || m_pRigidBody == nullptr) return;
		for (int i = 0; i != EchoPhysics::ForceMode::FORCE_MODE_NUMBER; ++i) {
			AddForce(GetModeForceOrTorque(i, true), i);
			AddTorque(GetModeForceOrTorque(i, false), i);
		}
	}
	RigidBodyComponentData& RigidBodyComponent::GetCompData() {
		return mCompData;
	}
	void RigidBodyComponent::SetCompData(RigidBodyComponentData* pData) {
		if (!pData) return;
		SetMass(pData->Mass);
		SetDrag(pData->Drag);
		SetAngularDrag(pData->AngularDrag);
		SetUseGravity(pData->UseGravity);
		SetIsKinematic(pData->IsKinematic);
		SetCollisionDetection(pData->CollisionDetection);
		SetConstraints(pData->Constraints);
		SetCenterOfMassMode(pData->CenterOfMassMode);
		SetCenterOfMassOffset(pData->CenterOfMassOffset);

		SetLocalCenterofMass(pData->LocalCenterofMass);
		SetInertiaTensor(pData->InertiaTensor);
		SetInertiaTensorRotation(pData->InertiaTensorRotation);
	}

	PhysicsActor* RigidBodyComponent::_getActorPhysX() {
		return m_pRigidBody;
	}
	DVector3 RigidBodyComponent::getWorldPositionPhysX() {
		if (m_pRigidBody == nullptr) {
			return getWorldPosition();
		}
		return m_pRigidBody->GetPosition();
	}
	Quaternion RigidBodyComponent::getWorldRotationPhysX() {
		if (m_pRigidBody == nullptr) {
			return getWorldRotation();
		}
		return m_pRigidBody->GetRotation();
	}
	DTransform RigidBodyComponent::getWorldTransformPhysX() {
		if (m_pRigidBody == nullptr) {
			return DTransform(getWorldPosition(), getWorldRotation());
		}
		return m_pRigidBody->GetTransform();
	}
	void RigidBodyComponent::UpdatePhysXPage() {
#ifdef USE_PHYSICS_PAGE
		if (mPhysXObject) {
			EchoPhysXGroupObject* pGroup = mPhysXObject->GetGroup();
			if (pGroup != nullptr) {
				GetPhysicsManager()->AddPhysXObjectToUpdateQueue(pGroup);
			}
			else {
				GetPhysicsManager()->AddPhysXObjectToUpdateQueue(mPhysXObject);
			}
		}
#endif //USE_PHYSICS_PAGE
	}

	void RigidBodyComponent::SetIsUsePhysics(bool value) {
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

	//=====================================================================================================================================


	void RigidBodyComponent::ResetMassAndInertiaTensor(bool bSync) {
		if (PhysXWorldSystem::instance()->IsSynchroJob() && bSync)
		{
			GetPhysicsManager()->RemoveRigidBodyFromUpdateMAITQueue(this);
			ResetMassAndInertiaTensorImpl();
		}
		else
		{
			GetPhysicsManager()->AddRigidBodyToUpdateMAITQueue(this);
		}
	}
	void RigidBodyComponent::ResetMassAndInertiaTensorImpl() {
		if (!m_pRigidBody) return;
		switch (GetCenterOfMassMode()) {
		case EchoPhysics::AutoMode: {
			m_pRigidBody->ResetLocalCenterOfMassAndInertiaTensor(nullptr);
			break;
		}
		case EchoPhysics::OffsetMode: {
			m_pRigidBody->ResetLocalCenterOfMassAndInertiaTensor(&GetCenterOfMassOffset());
			break;
		}
		case EchoPhysics::AutoAndOffsetMode: {
			m_pRigidBody->ResetLocalCenterOfMassAndInertiaTensor(nullptr);
			Vector3 center = m_pRigidBody->GetLocalCenterOfMass();
			center += GetCenterOfMassOffset();
			m_pRigidBody->SetLocalCenterOfMass(center);
			break;
		}
		case EchoPhysics::CustomMode: {
			Quaternion quat;
			quat.FromEuler(mCompData.InertiaTensorRotation);
			Transform massTrans(mCompData.LocalCenterofMass, quat);
			m_pRigidBody->SetMassCenterLocalPose(massTrans);
			m_pRigidBody->SetInertiaTensor(mCompData.InertiaTensor);
			break;
		}
		default: {
			m_pRigidBody->ResetLocalCenterOfMassAndInertiaTensor(nullptr);
			break;
		}
		}
		if (GetCenterOfMassMode() != EchoPhysics::CustomMode) {
			Transform massTrans;
			m_pRigidBody->GetMassCenterLocalPose(massTrans);
			mCompData.LocalCenterofMass = massTrans.translation;
			mCompData.InertiaTensorRotation = massTrans.rotation.ToEulerPositive();
			m_pRigidBody->GetInertiaTensor(mCompData.InertiaTensor);
		}
		DTransform trans(getParentNodeCom()->getWorldPosition(), getParentNodeCom()->getWorldRotation());
		m_pRigidBody->SetTransform(trans);
	}

	void RigidBodyComponent::SetMass(float mass) {
		mCompData.Mass = mass;
#ifdef ECHO_EDITOR
		if (ActorHelper::getForceSave() && m_pActComInfo)
		{
			RigidBodyComponentInfo* colliderInfo = static_cast<RigidBodyComponentInfo*>(m_pActComInfo);
			colliderInfo->SetMass(mass);
		}
#endif
		if (!m_pRigidBody) return;
		m_pRigidBody->SetMass(mass);
		ResetMassAndInertiaTensor();
	}

	float RigidBodyComponent::GetMass() {
		return mCompData.Mass;
	}

	void RigidBodyComponent::SetSpeed(float value) {
		Vector3 velocity = GetVelocity();
		if (velocity.Length() == 0) return;
		velocity = velocity * (value / velocity.Length());
		SetVelocity(velocity);
	}

	void RigidBodyComponent::SetCenterOfMassMode(int value) {
		mCompData.CenterOfMassMode = value;
#ifdef ECHO_EDITOR
		if (ActorHelper::getForceSave() && m_pActComInfo)
		{
			RigidBodyComponentInfo* colliderInfo = static_cast<RigidBodyComponentInfo*>(m_pActComInfo);
			colliderInfo->SetCenterOfMassMode(value);
		}
#endif
		ResetMassAndInertiaTensor();
	}
	int RigidBodyComponent::GetCenterOfMassMode() {
		return mCompData.CenterOfMassMode;
	}

	void RigidBodyComponent::SetCenterOfMassOffset(const Vector3& value) {
		mCompData.CenterOfMassOffset = value;
#ifdef ECHO_EDITOR
		if (ActorHelper::getForceSave() && m_pActComInfo)
		{
			RigidBodyComponentInfo* colliderInfo = static_cast<RigidBodyComponentInfo*>(m_pActComInfo);
			colliderInfo->SetCenterOfMassOffset(value);
		}
#endif
		ResetMassAndInertiaTensor();
	}
	const Vector3& RigidBodyComponent::GetCenterOfMassOffset() {
		return mCompData.CenterOfMassOffset;
	}

	float RigidBodyComponent::GetSpeed() {
		return GetVelocity().Length();
	}


	void RigidBodyComponent::SetVelocity(const Vector3& value) {
		//mPhysicsData.Velocity = value;
		//if (!IsExistPhysics()) return;
		//m_pRigidBody->SetLinearVelocity(value);
		//if (!value.isAlmostZero()) {
		//	WakeUp();
		//}
		mVelocity = value;
		if (!IsExistPhysics()) return;
		m_pRigidBody->SetLinearVelocity(value);
	}
	void RigidBodyComponent::SetAngularVelocity(const Vector3& value) {
		//mPhysicsData.AngularVelocity = value;
		//if (!IsExistPhysics()) return;
		//m_pRigidBody->SetAngularVelocity(value);
		//if (!value.isAlmostZero()) {
		//	WakeUp();
		//}
		mAngularVelocity = value;
		if (!IsExistPhysics()) return;
		m_pRigidBody->SetAngularVelocity(value);
	}
	void RigidBodyComponent::SetInertiaTensor(const Vector3& value) {
		//mPhysicsData.InertiaTensor = value;
		//if (!IsExistPhysics()) return;
		//m_pRigidBody->SetInertiaTensor(value);
		//if (!value.isAlmostZero()) {
		//	WakeUp();
		//}
		mCompData.InertiaTensor = value;
#ifdef ECHO_EDITOR
		if (ActorHelper::getForceSave() && m_pActComInfo)
		{
			RigidBodyComponentInfo* colliderInfo = static_cast<RigidBodyComponentInfo*>(m_pActComInfo);
			colliderInfo->SetInertiaTensor(value);
		}
#endif
		ResetMassAndInertiaTensor();
	}
	void RigidBodyComponent::SetInertiaTensorRotation(const Vector3& value) {
		//mPhysicsData.InertiaTensorRotation = value;
		//if (!IsExistPhysics()) return;
		//Quaternion quat;
		//quat.FromEuler(value);
		//m_pRigidBody->SetInertiaTensorRotation(quat);
		//if (!value.isAlmostZero()) {
		//	WakeUp();
		//}
		mCompData.InertiaTensorRotation = value;
#ifdef ECHO_EDITOR
		if (ActorHelper::getForceSave() && m_pActComInfo)
		{
			RigidBodyComponentInfo* colliderInfo = static_cast<RigidBodyComponentInfo*>(m_pActComInfo);
			colliderInfo->SetInertiaTensorRotation(value);
		}
#endif
		ResetMassAndInertiaTensor();
	}
	void RigidBodyComponent::SetLocalCenterofMass(const Vector3& value) {
		//mPhysicsData.LocalCenterofMass = value;
		//if (!IsExistPhysics()) return;
		//m_pRigidBody->SetLocalCenterOfMass(value);
		//if (!value.isAlmostZero()) {
		//	WakeUp();
		//}
		mCompData.LocalCenterofMass = value;
#ifdef ECHO_EDITOR
		if (ActorHelper::getForceSave() && m_pActComInfo)
		{
			RigidBodyComponentInfo* colliderInfo = static_cast<RigidBodyComponentInfo*>(m_pActComInfo);
			colliderInfo->SetLocalCenterofMass(value);
		}
#endif
		ResetMassAndInertiaTensor();
	}
	void RigidBodyComponent::SetWorldCenterofMass(const Vector3& value) {
		//mPhysicsData.WorldCenterofMass = value;
		//if (!IsExistPhysics()) return;
		//m_pRigidBody->SetWorldCenterOfMass(value);
		//if (!value.isAlmostZero()) {
		//	WakeUp();
		//}
		Vector3 localCenterOfMass = getWorldRotation().Inverse() * (value - getWorldPosition());
		SetLocalCenterofMass(localCenterOfMass);
	}

	void RigidBodyComponent::SetIsSleep(bool value) {
		//mPhysicsData.IsSleep = value;
		if (!IsExistPhysics()) return;
		if (m_pRigidBody->IsSleeping() == value) return;
		if (value) {
			m_pRigidBody->PutToSleep();
		}
		else {
			m_pRigidBody->WakeUp();
		}
	}

	const Vector3& RigidBodyComponent::GetVelocity() {
		if (IsExistPhysics()) _GetVelocity(mVelocity);
		return mVelocity;
	}
	const Vector3& RigidBodyComponent::GetAngularVelocity() {
		if (IsExistPhysics()) _GetAngularVelocity(mAngularVelocity);
		return mAngularVelocity;
	}
	const Vector3& RigidBodyComponent::GetInertiaTensor() {
		return mCompData.InertiaTensor;
	}
	const Vector3& RigidBodyComponent::GetInertiaTensorRotation() {
		return mCompData.InertiaTensorRotation;
	}
	const Vector3& RigidBodyComponent::GetLocalCenterofMass() {
		return mCompData.LocalCenterofMass;
	}
	Vector3 RigidBodyComponent::GetWorldCenterofMass() {
		return (getWorldRotation() * mCompData.LocalCenterofMass) + getWorldPosition();
	}
	bool RigidBodyComponent::GetIsSleep() {
		if (IsExistPhysics()) {
			return m_pRigidBody->IsSleeping();
		}
		else {
			return false;
		}
	}
	void RigidBodyComponent::_GetVelocity(Vector3& value) {
		value = m_pRigidBody->GetLinearVelocity();
	}
	void RigidBodyComponent::_GetAngularVelocity(Vector3& value) {
		value = m_pRigidBody->GetAngularVelocity();
	}
	//void RigidBodyComponent::_GetInertiaTensor(Vector3& value) {
	//	if (!m_pRigidBody) {
	//		value = 1;
	//		return;
	//	}
	//	m_pRigidBody->GetInertiaTensor(value);
	//}
	//void RigidBodyComponent::_GetInertiaTensorRotation(Vector3& value) {
	//	if (!m_pRigidBody) {
	//		value = 0;
	//		return;
	//	}
	//	Quaternion quat;
	//	m_pRigidBody->GetInertiaTensorRotation(quat);
	//	value = quat.ToEulerPositive();
	//}
	//void RigidBodyComponent::_GetLocalCenterofMass(Vector3& value) {
	//	if (!m_pRigidBody) {
	//		value = 0;
	//		return;
	//	}
	//	m_pRigidBody->GetLocalCenterOfMass(value);
	//}
	//void RigidBodyComponent::_GetWorldCenterofMass(Vector3& value) {
	//	if (!m_pRigidBody) {
	//		value = getParentNodeCom()->getWorldPosition();
	//		return;
	//	}
	//	m_pRigidBody->GetWorldCenterOfMass(value);
	//}
	//void RigidBodyComponent::_GetIsSleep(bool& value) {
	//	if (!m_pRigidBody) {
	//		value = true;
	//		return;
	//	}
	//	value = m_pRigidBody->IsSleeping();
	//}


	void RigidBodyComponent::CreatePhysXObject() {
		if (m_IsUsePhysXPage) {
			GetPhysicsManager()->CreatePhysXObject(this);
		}
		else {
			SetIsUsePhysics(true);
		}
	}
	void RigidBodyComponent::ReleasePhysXObject() {
		if (m_IsUsePhysXPage) {
			GetPhysicsManager()->ReleasePhysXObject(this);
		}
		else {
			SetIsUsePhysics(false);
		}
	}
	void RigidBodyComponent::SetIsUsePhysXPage(bool value) {
		if (m_IsUsePhysXPage == value) return;
		if (m_IsUsePhysXPage) {
			GetPhysicsManager()->ReleasePhysXObject(this);
		}
		else {
			GetPhysicsManager()->CreatePhysXObject(this);
		}
		m_IsUsePhysXPage = value;
	}

	void RigidBodyComponent::AddRigidBodyCallBack(const EchoRigidBodyCallBackPtr& pComp, int priority) {
		if (!pComp) return;
		bool bEmpty = mListenerList.empty();
		for (auto iter = mListenerList.begin(); iter != mListenerList.end(); ++iter) {
			if (iter->second == pComp) {
				mListenerList.erase(iter);
				break;
			}
		}
		auto addIter = mListenerList.begin();
		for (auto iter = mListenerList.begin(); iter != mListenerList.end(); ++iter) {
			if (iter->first < priority) {
				break;
			}
			else {
				addIter = iter;
			}
		}
		mListenerList.insert(addIter, std::make_pair(priority, pComp));
		if (bEmpty != mListenerList.empty()) { ChangeFilterGroup(); }
	}
	void RigidBodyComponent::RemoveRigidBodyCallBack(const EchoRigidBodyCallBackPtr& pComp) {
		if (!pComp) return;
		bool bEmpty = mListenerList.empty();
		for (auto iter = mListenerList.begin(); iter != mListenerList.end(); ++iter) {
			if (iter->second == pComp) {
				mListenerList.erase(iter);
				break;
			}
		}
		if (bEmpty != mListenerList.empty()) { ChangeFilterGroup(); }
	}

	void RigidBodyComponent::SendContactModify(const std::vector<EchoContactModifyPair>& pair) {
		if (mListenerList.empty()) return;
		auto iter = mListenerList.begin();
		while (iter != mListenerList.end()) {
			if (iter->second == nullptr) {
				iter = mListenerList.erase(iter);
				continue;
			}
			iter->second->OnContactModify(pair);
			++iter;
		}
	}

	void RigidBodyComponent::SendCCDContactModify(const std::vector<EchoContactModifyPair>& pair) {
		if (mListenerList.empty()) return;
		auto iter = mListenerList.begin();
		while (iter != mListenerList.end()) {
			if (iter->second == nullptr) {
				iter = mListenerList.erase(iter);
				continue;
			}
			iter->second->OnCCDContactModify(pair);
			++iter;
		}
	}

	void RigidBodyComponent::SendWakeEvent() {
		GetPhysicsManager()->SendRigidBodyWake(this->getWeakPtr<RigidBodyComponent>());
		if (mListenerList.empty()) return;
		auto iter = mListenerList.begin();
		while (iter != mListenerList.end()) {
			if (iter->second == nullptr) {
				iter = mListenerList.erase(iter);
				continue;
			}
			iter->second->OnWake(this);
			++iter;
		}
	}

	void RigidBodyComponent::SendSleepEvent() {
		GetPhysicsManager()->SendRigidBodySleep(this->getWeakPtr<RigidBodyComponent>());
		if (mListenerList.empty()) return;
		auto iter = mListenerList.begin();
		while (iter != mListenerList.end()) {
			if (iter->second == nullptr) {
				iter = mListenerList.erase(iter);
				continue;
			}
			iter->second->OnSleep(this);
			++iter;
		}
	}

	void RigidBodyComponent::SendCreateEvent() {
		GetPhysicsManager()->SendRigidBodyCreate(this->getWeakPtr<RigidBodyComponent>());
		if (mListenerList.empty()) return;
		auto iter = mListenerList.begin();
		while (iter != mListenerList.end()) {
			if (iter->second == nullptr) {
				iter = mListenerList.erase(iter);
				continue;
			}
			iter->second->OnCreate(this);
			++iter;
		}
	}

	void RigidBodyComponent::SendReleaseEvent() {
		GetPhysicsManager()->SendRigidBodyRelease(this->getWeakPtr<RigidBodyComponent>());
		if (mListenerList.empty()) return;
		auto iter = mListenerList.begin();
		while (iter != mListenerList.end()) {
			if (iter->second == nullptr) {
				iter = mListenerList.erase(iter);
				continue;
			}
			iter->second->OnRelease(this);
			++iter;
		}
	}

	void RigidBodyComponent::ChangeFilterGroup() {
		if (!m_ColliderList.empty()) {
			for (auto& pCollider : m_ColliderList) {
				if (pCollider != nullptr) {
					pCollider->ChangeFilterGroup();
				}
			}
		}
	}
	uint32 RigidBodyComponent::GetShapeSceneFilterShaderFlags() {
		EchoSceneFilterShaderFlags flags = 0;
		if (!mListenerList.empty()) {
			flags |= EchoSceneFilterShaderFlag::eMODIFY_CONTACTS;
		}
		return flags;
	}
}

namespace Echo {

	RigidBodyComponent* EchoRigidBodyScript::Script_GetRigidBodyComponent(Actor* pActor, const std::string& name) {

		ProtocolComponentSystem* pMgr = ProtocolComponentSystem::instance();
		ProtocolHandle comHandle = 0;
		ActorComTypeDesc desc;
		desc.actHandle = (Echo::ProtocolHandle)pActor;
		desc.comType = RigidBodyComponent::mType;
		desc.comName = name;
		ProtocolHandle descHandle = (ProtocolHandle)&desc;
		ProtocolResult result = pMgr->callProtocolFunc(PFID_Actor_GetComponent, &comHandle, descHandle);

		if (result != PR_OK)
			return nullptr;
		if (comHandle == 0)
			return nullptr;

		return  (RigidBodyComponent*)comHandle;

	}

}
