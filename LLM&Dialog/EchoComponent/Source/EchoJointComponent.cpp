#include "EchoComponentStableHeaders.h"
#include "EchoJointComponent.h"

#include "EchoPhysXAsset.h"
#include "EchoPhysXScene.h"
#include "EchoPhysXCommon.h"
#include "EchoPhysXJoint.h"
#include "EchoRigidBodyComponent.h"
#include "EchoCharactorControllerComponent.h"

namespace Echo {

	void WriteJSNode(JSNODE pRoot, const char* name, const JointLimit& limit)
	{
		std::string Name = name;
		// 		WriteJSNode(pRoot, (Name + ".limit").c_str(), limit.limit);
		WriteJSNode(pRoot, (Name + ".isSoft").c_str(), limit.IsSoft);
		WriteJSNode(pRoot, (Name + ".bounciness").c_str(), limit.bounciness);
		WriteJSNode(pRoot, (Name + ".contanctDistance").c_str(), limit.contanctDistance);
		WriteJSNode(pRoot, (Name + ".spring").c_str(), limit.spring);
		WriteJSNode(pRoot, (Name + ".damper").c_str(), limit.damper);
	}
	bool ReadJSNode(JSNODE pRoot, const char* name, JointLimit& limit)
	{
		bool res = true;
		std::string Name = name;
		// 		res = ReadJSNode(pRoot, (Name + ".limit").c_str(), limit.limit) && res;
		res = ReadJSNode(pRoot, (Name + ".isSoft").c_str(), limit.IsSoft) && res;
		res = ReadJSNode(pRoot, (Name + ".bounciness").c_str(), limit.bounciness) && res;
		res = ReadJSNode(pRoot, (Name + ".contanctDistance").c_str(), limit.contanctDistance) && res;
		res = ReadJSNode(pRoot, (Name + ".spring").c_str(), limit.spring) && res;
		res = ReadJSNode(pRoot, (Name + ".damper").c_str(), limit.damper) && res;
		return res;
	}

	void WriteJSNode(JSNODE pRoot, const char* name, const JointDrive& drive)
	{
		std::string Name = name;
		WriteJSNode(pRoot, (Name + ".spring").c_str(), drive.spring);
		WriteJSNode(pRoot, (Name + ".damper").c_str(), drive.damper);
		WriteJSNode(pRoot, (Name + ".maximumForce").c_str(), drive.maximumForce);
	}
	bool ReadJSNode(JSNODE pRoot, const char* name, JointDrive& drive)
	{
		bool res = true;
		std::string Name = name;
		res = ReadJSNode(pRoot, (Name + ".spring").c_str(), drive.spring) && res;
		res = ReadJSNode(pRoot, (Name + ".damper").c_str(), drive.damper) && res;
		res = ReadJSNode(pRoot, (Name + ".maximumForce").c_str(), drive.maximumForce) && res;
		return res;
	}

	EDITOR_REFLECT_BEGIN(JointComponentInfo)
		EDITOR_REFLECT_MEMBER_UPDATE(m_AxisOffset, "AxisOffset", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)	// 轴偏移
		EDITOR_REFLECT_MEMBER_UPDATE(m_BreakForce, "BreakForce", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)	// 被破环时需要的 力
		EDITOR_REFLECT_MEMBER_UPDATE(m_BreakTorque, "BreakTorque", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)	// 被破环时需要的 力矩
		EDITOR_REFLECT_MEMBER_UPDATE(m_EnableProjection, "EnableProjection", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)	// 启用投影
		EDITOR_REFLECT_MEMBER_UPDATE(m_EnableCollision, "EnableCollision", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)	// 启用连接的两个刚体间的碰撞检测
		EDITOR_REFLECT_MEMBER_UPDATE(m_EnablePreprocessing, "EnablePreprocessing", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)		// 启用预处理
		EDITOR_REFLECT_MEMBER_UPDATE(m_AutoCalculateLocalPose, "AutoCalculateLocalPose", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_MassScaleSelf, "MassScaleSelf", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)		// 关节所属的刚体的质量缩放
		EDITOR_REFLECT_MEMBER_UPDATE(m_MassScaleConnected, "MassScaleConnected", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)		// 被连接的刚体的质量缩放
		EDITOR_REFLECT_MEMBER_UPDATE(m_ConnectedComName, "ConnectedComName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)	// 依赖的刚体
		EDITOR_REFLECT_MEMBER_UPDATE(m_SelfComName, "SelfComName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)	// 自身的刚体
		EDITOR_REFLECT_MEMBER_UPDATE(m_LocalPose0, "LocalPose0", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_LocalPose1, "LocalPose1", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_END();


	NODE_ACTOR_COM_CPP(JointComponent)

	JointComponentInfo::JointComponentInfo(const Name& comType, ActorCOMMoveableType MoveType) : ActorComponentInfo(comType, MoveType) {}
	JointComponentInfo::~JointComponentInfo() {}

	_DEFINEINFO_VIRFUN_BASE(JointComponentInfo)

	void JointComponentInfo::CopyToCompData(JointComponentData* pData) {
		if (!pData) return;
		pData->connectedComName = m_ConnectedComName;
		pData->selfComName = m_SelfComName;
		pData->axisOffset = m_AxisOffset;
		pData->breakForce = m_BreakForce;
		pData->breakTorque = m_BreakTorque;
		pData->enableProjection = m_EnableProjection;
		pData->enableCollision = m_EnableCollision;
		pData->enablePreprocessing = m_EnablePreprocessing;
		pData->massScaleSelf = m_MassScaleSelf;
		pData->massScaleConnected = m_MassScaleConnected;
		pData->autoCalculateLocalPose = m_AutoCalculateLocalPose;
		pData->localPose0 = m_LocalPose0;
		pData->localPose1 = m_LocalPose1;
	}
	//////////////////////////////////////////////////////////////////////////

	void JointComponent::updateCombinedName() {
#ifdef USE_PHYSICS_HELPEROBJ
		if (m_pHelperObj) {
			Name newName(getCombinedName().append("||").append("HelperObj"));
			m_pHelperObj->setName(newName);
		}
		if (m_pHelperObj1) {
			Name newName(getCombinedName().append("||").append("HelperObj1"));
			m_pHelperObj1->setName(newName);
		}
		if (m_pHelperObj2) {
			Name newName(getCombinedName().append("||").append("HelperObj2"));
			m_pHelperObj2->setName(newName);
		}
#endif // USE_PHYSICS_HELPEROBJ
	}


	JointComponent::JointComponent(ActorPtr actor, ActorCOMMoveableType type) : PhysicsLinkComponent(actor, type) {}

	JointComponent::~JointComponent()
	{
		SAFE_DELETE(m_pCompData);
	};
	void JointComponent::init(ActorComponentInfo* pInfo)
	{
		PhysicsBaseComponent::init(pInfo);
		if (pInfo) {
			static_cast<JointComponentInfo*>(pInfo)->CopyToCompData(m_pCompData);
		}
	}
	void JointComponent::unInit() {
		ReleasePhysXObject();
#ifdef USE_PHYSICS_HELPEROBJ
		ReleaseHelper();
#endif // USE_PHYSICS_HELPEROBJ
		PhysicsBaseComponent::unInit();
	}
	void JointComponent::connectCom() {
		PhysicsBaseComponent::connectCom();
		AttachRigidBodyComp();
	}
	void JointComponent::disconnectCom() {
		DetachRigidBodyComp();
		PhysicsBaseComponent::disconnectCom();
	}
	void JointComponent::replay(ActorComponentInfo* pInfo)
	{
		if (!pInfo) return;
		static_cast<JointComponentInfo*>(pInfo)->CopyToCompData(m_pCompData);
	}

	void JointComponent::visibleStateChanged(bool activeState) {
		if (!GetPhysicsManager()) return;
		if (activeState) {
			CreatePhysXObject();
		}
		else {
			ReleasePhysXObject();
		}
	}
	
	void JointComponent::showEditorHelper(bool isShow) {
		SetHelperVisible(isShow);
	}

	void JointComponent::copyTo(const ActorComponentPtr& newComp)
	{
		if (!newComp) return;
		auto newCompPtr = newComp.getStaticCastWeakPtr<JointComponent>();
		newCompPtr->SetCompData(m_pCompData);
		newCompPtr->SetHelperVisible(GetHelperVisible());
	}

	void JointComponent::CreatePhysics()
	{
		if (m_jointType == JointCompType::INVALID_JOINT || m_pJoint != nullptr)
			return;
		mIsBreak = false;
		const PhysicsActorComponentPtr& body0 = GetSelfRigidBodyComp();
		const PhysicsActorComponentPtr& body1 = GetConnectedRigidBodyComp();
		PhysicsActor* actor0 = body0 == nullptr ? nullptr : body0->_getActorPhysX();
		PhysicsActor* actor1 = body1 == nullptr ? nullptr : body1->_getActorPhysX();

		if ((!actor0 && !actor1) || (actor0 == actor1)) return;

		PhysXJointDesc desc;
		{
			desc.actor0 = actor0;
			desc.actor1 = actor1;
//#ifndef USE_PHYSICS_PAGE
			ResetLocalPose(false);
//#endif //USE_PHYSICS_PAGE
			GetLocalPose0(desc.localPose0);
			GetLocalPose1(desc.localPose1);
		}

		switch (m_jointType)
		{
		case JointCompType::CHARARCTER_JOINT:
			desc.jointType = PhysXJointType::D6Joint;
			break;
		case JointCompType::CONFIGURABLE_JOINT:
			desc.jointType = PhysXJointType::D6Joint;
			break;
		case JointCompType::FIXED_JOINT:
			desc.jointType = PhysXJointType::FixedJoint;
			break;
		case JointCompType::HINGE_JOINT:
			desc.jointType = PhysXJointType::RevoluteJoint;
			break;
		case JointCompType::SPRING_JOINT:
			desc.jointType = PhysXJointType::DistanceJoint;
			break;
		default:
			assert(false && "Unkonw Joint comp type");
			break;
		}
		desc.userData = this;
		desc.userType = (int)PhysXUserTypeEnum::ActorComponent;
		m_pJoint = GetPhysicsManager()->GetScene()->AddJoint(desc);
		if (!m_pJoint) return;
		SetCompData(nullptr);
	}
	void JointComponent::ReleasePhysics()
	{
		if (m_pJoint == nullptr) return;
		GetPhysicsManager()->GetScene()->RemoveJoint(m_pJoint);
		m_pJoint = nullptr;
		mIsBreak = false;
		AddToEditorUpdateQueue();
		
	}

	bool JointComponent::IsExistPhysics()
	{
		return m_pJoint != nullptr;
	}

	bool JointComponent::IsCanPhysics()
	{
		if (!getRealVisibleState()) return false;
		const PhysicsActorComponentPtr& body0 = GetSelfRigidBodyComp();
		const PhysicsActorComponentPtr& body1 = GetConnectedRigidBodyComp();
		if (body0 == nullptr && body1 == nullptr) {
			return false;
		}
		PhysicsActor* pAct0 = nullptr;
		PhysicsActor* pAct1 = nullptr;
		if (body0 != nullptr) {
			if (body0->GetPhysicsManager() != GetPhysicsManager()) {
				return false;
			}
			pAct0 = body0->_getActorPhysX();
			if (pAct0 == nullptr) {
				return false;
			}
		}
		if (body1 != nullptr) {
			if (body1->GetPhysicsManager() != GetPhysicsManager()) {
				return false;
			}
			pAct1 = body1->_getActorPhysX();
			if (pAct1 == nullptr) {
				return false;
			}
		}
		return pAct0 != pAct1;
	}
	PhysXJoint* JointComponent::GetPhysics() {
		return m_pJoint;
	}

	void JointComponent::DetachRigidBody(PhysicsActorComponent* pComp) {
		if (pComp == GetSelfRigidBodyComp()) {
			DetachRigidBodyComp(true, false);
		} else if (pComp == GetConnectedRigidBodyComp()) {
			DetachRigidBodyComp(false, true);
		}
		//ChangeRigidBody();
		AddToPhysicsUpdateQueue();
	}
	std::pair<Vector3, Vector3> JointComponent::GetForceAndTorque()
	{
		if (!IsExistPhysics())
			return std::pair<Vector3, Vector3>();
		Vector3 f, t;
		m_pJoint->getConstraint().getForce(f, t);
		return std::make_pair(f, t);
	}

	bool JointComponent::Isbroken()
	{
		if (!IsExistPhysics())
			return false;
		return m_pJoint->getConstraintFlags() & (uint32)PhysXConstraintFlag::eBROKEN;
	}
	void JointComponent::GetJointTransform(DTransform& trans) {
		trans.translation = getWorldPosition();
		trans.rotation = getWorldRotation();
	}
	void JointComponent::GetJointLocalPose(Transform& t0, Transform& t1) {

		const PhysicsActorComponentPtr& body1 = GetSelfRigidBodyComp();
		const PhysicsActorComponentPtr body2 = GetConnectedRigidBodyComp();
		
		DTransform jointTrans, body1Trans, body2Trans;
		DTransform selfTrans = { getWorldPosition(),getWorldRotation() };
		GetJointTransform(jointTrans);
		if (!body1) {
			t0 = selfTrans;
		}
		else {
			body1Trans = body1->getWorldTransformPhysX();
			EchoPhysicsManager::convertWorldToLocal(body1Trans, selfTrans, t0);
		}
		if (!body2) {
			t1 = jointTrans;
		}
		else {
			body2Trans = body2->getWorldTransformPhysX();
			EchoPhysicsManager::convertWorldToLocal(body2Trans, jointTrans, t1);
		}
	}
	const PhysicsActorComponentPtr& JointComponent::GetSelfRigidBodyComp() {
		return m_pRigidBodyComp0;
	}
	const PhysicsActorComponentPtr& JointComponent::GetConnectedRigidBodyComp() {
		return m_pRigidBodyComp1;
	}
	void JointComponent::SetLocalPose0(const Transform& trans) {
		m_pCompData->localPose0 = trans;
		if (!IsExistPhysics()) {
			return;
		}
		m_pJoint->setLocalPose(PhysXJointActorIndex::JointActor0, trans);
	}
	void JointComponent::SetLocalPose1(const Transform& trans) {
		m_pCompData->localPose1 = trans;
		if (!IsExistPhysics()) {
			return;
		}
		m_pJoint->setLocalPose(PhysXJointActorIndex::JointActor1, trans);
	}
	void JointComponent::GetLocalPose0(Transform& trans) {
		if (!IsExistPhysics()) {
			trans = m_pCompData->localPose0;
			return;
		}
		trans = m_pJoint->getLocalPose(PhysXJointActorIndex::JointActor0);
	}
	void JointComponent::GetLocalPose1(Transform& trans) {
		if (!IsExistPhysics()) {
			trans = m_pCompData->localPose1;
			return;
		}
		trans = m_pJoint->getLocalPose(PhysXJointActorIndex::JointActor1);
	}

	void JointComponent::GetRelativeTransform(Transform& trans) {
		if (!IsExistPhysics()) return;
		trans = m_pJoint->getRelativeTransform();
	}

	void JointComponent::GetRelativeLinearVelocity(Vector3& value) {
		if (!IsExistPhysics()) return;
		value = m_pJoint->getRelativeLinearVelocity();
	}
	void JointComponent::GetRelativeAngularVelocity(Vector3& value) {
		if (!IsExistPhysics()) return;
		value = m_pJoint->getRelativeAngularVelocity();
	}

	void JointComponent::AddJointCallBack(const EchoJointCallBackPtr& pComp) {
		if (!pComp) return;
		mListenerList.insert(pComp);
	}
	void JointComponent::RemoveJointCallBack(const EchoJointCallBackPtr& pComp) {
		if (!pComp) return;
		mListenerList.erase(pComp);
	}
	void JointComponent::SendJointBreak() {
		mIsBreak = true;
		AddToEditorUpdateQueue();
		if (mListenerList.empty()) return;
		auto iter = mListenerList.begin();
		while (iter != mListenerList.end()) {
			if (*iter == nullptr) {
				iter = mListenerList.erase(iter);
				continue;
			}
			(*iter)->OnJointBreak();
			++iter;
		}
	}

	void JointComponent::AttachRigidBodyComp(bool bR0, bool bR1) {
		PhysicsLinkComponentPtr thisPtr = getWeakPtr<PhysicsLinkComponent>();
		DetachRigidBodyComp(bR0, bR1);
		if (bR0) {
			if (!GetSelfRigidBody().empty()) {
				ActorComponentPtr compPtr = FindComponent(GetSelfRigidBody());
				if (compPtr != nullptr) {
					m_pRigidBodyComp0 = compPtr.getStaticCastWeakPtr<PhysicsActorComponent>();
					m_pRigidBodyComp0->_addPhysX(thisPtr);
				}
			}

		}
		if (bR1) {
			if (!GetConnectedRigidBody().empty()) {
				ActorComponentPtr compPtr = FindComponent(GetConnectedRigidBody());
				if (compPtr != nullptr) {
					m_pRigidBodyComp1 = compPtr.getStaticCastWeakPtr<PhysicsActorComponent>();
					m_pRigidBodyComp1->_addPhysX(thisPtr);
				}
			}

		}
		ResetLocalPose(false);
		if ((bR0 && m_pRigidBodyComp0 != nullptr) || (bR1 && m_pRigidBodyComp1 != nullptr)) {
			CreatePhysXObject();
			AddToEditorUpdateQueue();
		}

	}
	void JointComponent::DetachRigidBodyComp(bool bR0, bool bR1) {
		PhysicsLinkComponentPtr thisPtr = getWeakPtr<PhysicsLinkComponent>();
		if ((bR0 && m_pRigidBodyComp0 != nullptr) || (bR1 && m_pRigidBodyComp1 != nullptr)) {
			ReleasePhysXObject();
		}
		if (bR0) {
			if (m_pRigidBodyComp0 != nullptr) {
				m_pRigidBodyComp0->_removePhysX(thisPtr);
			}
			m_pRigidBodyComp0 = nullptr;
		}
		if (bR1) {
			if (m_pRigidBodyComp1 != nullptr) {
				m_pRigidBodyComp1->_removePhysX(thisPtr);
			}
			m_pRigidBodyComp1 = nullptr;
		}
		if (bR0 || bR1) {
			AddToEditorUpdateQueue();
		}
	}

	void JointComponent::SetConnectedRigidBody(const String& ComName)
	{
		if (ComName == GetConnectedRigidBody() || (!ComName.empty() && ComName == GetSelfRigidBody())) return;
		m_pCompData->connectedComName = ComName;
		AttachRigidBodyComp(false, true);
		//ChangeRigidBody();
		ReleasePhysics();
		AddToPhysicsUpdateQueue();
	}
	void JointComponent::SetSelfRigidBody(const String& ComName)
	{
		if (ComName == GetSelfRigidBody() || (!ComName.empty() && ComName == GetConnectedRigidBody())) return;
		m_pCompData->selfComName = ComName;
		AttachRigidBodyComp(true, false);
		//ChangeRigidBody();
		ReleasePhysics();
		AddToPhysicsUpdateQueue();
	}
	
	const String& JointComponent::GetConnectedRigidBody()
	{
		return m_pCompData->connectedComName;
	}

	const String& JointComponent::GetSelfRigidBody()
	{
		return m_pCompData->selfComName;
	}

	void JointComponent::SetAxisOffset(const Vector3& axis) {
		if (IsExistPhysics()) return;
		if (m_pCompData->axisOffset == axis) return;
		m_pCompData->axisOffset = axis;
		AddToEditorUpdateQueue();
	}
	const Vector3& JointComponent::GetAxisOffset() {
		return m_pCompData->axisOffset;
	}


	void JointComponent::SetPhysicsAxis(const Vector3& axis1, const Vector3& axis2) {
		if (axis1.isZeroLength() || axis2.isZeroLength()) {
			setRotation(Quaternion::IDENTITY);
		}
		else {
			Quaternion quat;
			quat.FromAxes(axis1, axis2, Vector3::VCross(axis1, axis2));
			setRotation(quat);
		}

		if (IsExistPhysics()) {
			ResetLocalPose(true);
		}
	}

	Vector3 JointComponent::GetSecondaryAxis()
	{
		return getRotation() * Vector3::UNIT_Y;
	}

	Vector3 JointComponent::GetAxis()
	{
		return getRotation() * Vector3::UNIT_X;
	}

	/*void JointComponent::SetAnchor(const Vector3& anchor)
	{
		if (!anchor.isEqual(m_pCompData->anchor)) {
			AddToEditorUpdateQueue();
		}
		m_pCompData->anchor = anchor;
	}*/

	const Vector3& JointComponent::GetAnchor()
	{
		return m_pCompData->localPose0.translation;
	}

	/*void JointComponent::SetAnchorConnected(const Vector3& AnchorConnected)
	{
		if (!AnchorConnected.isEqual(m_pCompData->anchorConnected)) {
			AddToEditorUpdateQueue();
		}
		m_pCompData->anchorConnected = AnchorConnected;
	}*/

	const Vector3& JointComponent::GetAnchorConnected()
	{

		return m_pCompData->localPose1.translation;
	}

	const Quaternion& JointComponent::GetAnchorRotation() {
		return m_pCompData->localPose0.rotation;
	}
	const Quaternion& JointComponent::GetAnchorConnectedRotation() {
		return m_pCompData->localPose1.rotation;
	}

	void JointComponent::SetAutoCalculateLocalPose(bool value) {
		m_pCompData->autoCalculateLocalPose = value;
	}
	bool JointComponent::GetAutoCalculateLocalPose() {
		return m_pCompData->autoCalculateLocalPose;
	}

	void JointComponent::SetBreakForce(float breakForce)
	{
		if (breakForce < 0.f)
			return;

		m_pCompData->breakForce = breakForce;

		if (IsExistPhysics())
			m_pJoint->setBreakForce(breakForce, m_pCompData->breakTorque);
	}

	float JointComponent::GetBreakForce()
	{

		return m_pCompData->breakForce;
	}

	void JointComponent::SetBreakTorque(float breakTorque)
	{
		if (breakTorque < 0.f)
			return;

		m_pCompData->breakTorque = breakTorque;

		if (IsExistPhysics())
			m_pJoint->setBreakForce(m_pCompData->breakForce, breakTorque);
	}

	float JointComponent::GetBreakTorque()
	{

		return m_pCompData->breakTorque;
	}

	void JointComponent::SetEnableProjection(bool enable)
	{

		m_pCompData->enableProjection = enable;

		if (IsExistPhysics())
			m_pJoint->setConstraintFlag(PhysXConstraintFlag::ePROJECTION, enable);
	}

	bool JointComponent::GetEnableProjection()
	{

		return m_pCompData->enableProjection;
	}


	void JointComponent::SetEnableCollision(bool enable)
	{

		m_pCompData->enableCollision = enable;

		if (IsExistPhysics())
			m_pJoint->setConstraintFlag(PhysXConstraintFlag::eCOLLISION_ENABLED, enable);
	}

	bool JointComponent::GetEnableCollision()
	{

		return m_pCompData->enableCollision;
	}

	void JointComponent::SetEnablePreprocessing(bool enable)
	{

		m_pCompData->enablePreprocessing = enable;

		if (IsExistPhysics())
			m_pJoint->setConstraintFlag(PhysXConstraintFlag::eDISABLE_PREPROCESSING, !enable);
	}

	bool JointComponent::GetEnablePreprocessing()
	{

		return m_pCompData->enablePreprocessing;
	}

	void JointComponent::SetMassScaleSelf(float massScale)
	{

		m_pCompData->massScaleSelf = massScale;

		if (IsExistPhysics())
			m_pJoint->setInvMassScale0(massScale);
	}

	float JointComponent::GetMassScaleSelf()
	{

		return m_pCompData->massScaleSelf;
	}

	void JointComponent::SetMassScaleConnected(float massScale)
	{

		m_pCompData->massScaleConnected = massScale;

		if (IsExistPhysics())
			m_pJoint->setInvMassScale1(massScale);
	}

	float JointComponent::GetMassScaleConnected()
	{

		return m_pCompData->massScaleConnected;
	}

	Vector3 JointComponent::GetCurrentForce() {
		if (!IsExistPhysics()) {
			return Vector3::ZERO;
		}
		Vector3 force, torque;
		GetCurrentForceAndTorque(force, torque);
		return force;
	}
	Vector3 JointComponent::GetCurrentTorque() {
		if (!IsExistPhysics()) {
			return Vector3::ZERO;
		}
		Vector3 force, torque;
		GetCurrentForceAndTorque(force, torque);
		return torque;
	}
	void JointComponent::GetCurrentForceAndTorque(Vector3& force, Vector3& torque) {
		if (!IsExistPhysics()) {
			force = Vector3::ZERO;
			torque = Vector3::ZERO;
			return;
		}
		m_pJoint->getConstraint().getForce(force, torque);
	}


	void JointComponent::SetCompData(JointComponentData* pData) {
		JointComponentData* pCompData = (JointComponentData*)(pData ? pData : m_pCompData);
		if (pData) SetSelfRigidBody(pCompData->selfComName);
		if (pData) SetConnectedRigidBody(pCompData->connectedComName);
		if (pData) SetAxisOffset(pCompData->axisOffset);
		SetBreakForce(pCompData->breakForce);
		SetBreakTorque(pCompData->breakTorque);
		SetEnableProjection(pCompData->enableProjection);
		SetEnableCollision(pCompData->enableCollision);
		SetEnablePreprocessing(pCompData->enablePreprocessing);
		SetMassScaleSelf(pCompData->massScaleSelf);
		SetMassScaleConnected(pCompData->massScaleConnected);
		if (pData) SetAutoCalculateLocalPose(pCompData->autoCalculateLocalPose);
		if (pData) SetLocalPose0(pCompData->localPose0);
		if (pData) SetLocalPose1(pCompData->localPose1);
	}
	JointComponentData* JointComponent::GetCompData() {
		return m_pCompData;
	}
	/*void JointComponent::ResetAnchor() {
		RigidBodyComponent* body1 = GetSelfRigidBodyComp().get();
		RigidBodyComponent* body2 = GetConnectedRigidBodyComp().get();
		Transform selfTrans = GetRealWorldTransform();
		Transform jointTrans;
		GetJointTransform(jointTrans);
		if (!body1) {
			SetAnchor(selfTrans.translation);
		}
		else {
			SetAnchor(body1->getWorldRotation().Inverse() * (selfTrans.translation - body1->getWorldPosition()));
		}
		if (!body2) {
			SetAnchorConnected(jointTrans.translation);
		}
		else {
			SetAnchorConnected(body2->getWorldRotation().Inverse() * (jointTrans.translation - body2->getWorldPosition()));
		}
	}*/
	void JointComponent::ResetLocalPose(bool IsResetPhysics) {
		if (GetAutoCalculateLocalPose()) GetJointLocalPose(m_pCompData->localPose0, m_pCompData->localPose1);
		if (IsResetPhysics && IsExistPhysics()) {
			SetLocalPose0(m_pCompData->localPose0);
			SetLocalPose1(m_pCompData->localPose1);
		}
	}

	void JointComponent::_actorNotifyPhysX(PhysicsActorComponent* pComp, EchoPhysXNotifyType type) {
		switch (type) {
			case PhysXNotify_Break: {
				DetachRigidBody(pComp);
				break;
			}
			case PhysXNotify_Create: {
				AddToPhysicsUpdateQueue();
				break;
			}
			case PhysXNotify_Destroy: {
				if (IsExistPhysics()) {
					ReleasePhysics();
				}
				break;
			}
			default: {
				break;
			}
		}
	}

	void JointComponent::SetIsUsePhysics(bool value) {
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
		}
		m_IsUsePhysics = value;
	}

	void JointComponent::CreatePhysXObject() {
		if (m_IsUsePhysXPage) {
			GetPhysicsManager()->CreatePhysXObject(this);
		}
		else {
			SetIsUsePhysics(true);
		}
	}
	void JointComponent::ReleasePhysXObject() {
		if (m_IsUsePhysXPage) {
			GetPhysicsManager()->ReleasePhysXObject(this);
		}
		else {
			SetIsUsePhysics(false);
		}
	}
	void JointComponent::SetIsUsePhysXPage(bool value) {
		if (m_IsUsePhysXPage == value) return;
		if (m_IsUsePhysXPage) {
			GetPhysicsManager()->ReleasePhysXObject(this);
		}
		else {
			GetPhysicsManager()->CreatePhysXObject(this);
		}
		m_IsUsePhysXPage = value;
	}

#ifdef USE_PHYSICS_PAGE
	EchoPhysXActorObject* JointComponent::GetEchoPhysXObject() {
		if (m_pRigidBodyComp0 != nullptr) {
			return m_pRigidBodyComp0->mPhysXObject;
		}
		else if(m_pRigidBodyComp1 != nullptr) {
			return m_pRigidBodyComp1->mPhysXObject;
		} 
		else {
			return nullptr;
		}
	}
#endif //USE_PHYSICS_PAGE

}; // end Echo 


namespace Echo {

#ifdef USE_PHYSICS_HELPEROBJ
	float JointComponent::gDrawScale = 0.1f;

	void JointComponent::UpdateComponentInEditor() {
		if (GetHelperVisible()) {
			UpdateHelper();
			AddToEditorUpdateQueue();
		}
	}

	void JointComponent::CreateHelper() {
		if (m_pHelperObj) return;
		Name helperName = Name(getCombinedName().append("||").append("HelperObj"));
		m_pHelperObj = new HelperObject(helperName, HelperObjType_Box, false, nullptr, getSceneManager(), true);
		addChildToNode(m_pHelperObj->getSceneNode());
		m_pHelperObj->setVisible(m_IsHelperVisible);
		m_pHelperObj->setRenderType(ECHO_DRAW::ECHO_TRIANGLE_LIST);
		m_pHelperObj->setHelperColorType(HelperObjColorType_Green);
		m_pHelperObj->setOrientation(Quaternion::IDENTITY);
		m_pHelperObj->setScale(Vector3::UNIT_SCALE);
	}
	void JointComponent::ReleaseHelper() {
		if (m_pHelperObj) {
			RemoveChildFromNode(m_pHelperObj->getSceneNode());
			SAFE_DELETE(m_pHelperObj);
		}
	}
	void JointComponent::UpdateHelper() {
		if (!m_pHelperObj || !m_pHelperObj->getVisible()) {
			return;
		}
		if (mIsBreak) {
			m_pHelperObj->clearBuffer();
			return;
		}
		float drawScale = gDrawScale;

		std::vector<Vector3> vbVec;
		std::vector<uint16> ibVec;
		std::vector<uint32> colors;

		auto AddIndexBuffer = [](std::vector<uint16>& _ibVec, const std::vector<uint16>& _newIBVec, uint16 _startIB) {
			if (_newIBVec.empty()) return;
			size_t curSize = _ibVec.size();
			int newIBSize = (int)_newIBVec.size();
			_ibVec.resize(curSize + newIBSize);
			for (int i = 0; i != newIBSize; ++i) {
				_ibVec[curSize + i] = _startIB + _newIBVec[i];
			}
		};

		//绘制方线
		auto DrawBox = [&](const Vector3& AxisFront, const Vector3& AxisUp, const Vector3& AxisRight, uint32 Color, float LengthP = 1.0f, float LengthN = 1.0f, float Width = 0.01f, bool IsDrawArrow = false) {

			if (AxisFront.isZeroLength() || AxisFront.IsNan() || AxisUp.isZeroLength() || AxisUp.IsNan() || AxisRight.isZeroLength() || AxisRight.IsNan() || Width < 1e-06) {
				return;
			}

			uint16 startIB = (uint16)vbVec.size();


			std::vector<Vector3> tVB(8);
			tVB[0] = Vector3(-LengthN, -Width, -Width);	//0
			tVB[1] = Vector3(-LengthN, -Width, Width);	//1
			tVB[2] = Vector3(-LengthN, Width, Width);	//2
			tVB[3] = Vector3(-LengthN, Width, -Width);	//3
			tVB[4] = Vector3(LengthP, -Width, -Width);	//4
			tVB[5] = Vector3(LengthP, -Width, Width);	//5
			tVB[6] = Vector3(LengthP, Width, Width);	//6
			tVB[7] = Vector3(LengthP, Width, -Width);	//7

			static std::vector<uint16> indexBuffer1 = {
				0,1,2,
				0,2,3,
				4,6,5,
				4,7,6,
				2,6,7,
				2,7,3,
				0,4,1,
				4,5,1,
				1,5,6,
				1,6,2,
				0,3,7,
				0,7,4
			};

			AddIndexBuffer(ibVec, indexBuffer1, startIB);

			if (IsDrawArrow) {
				startIB += 8;
				tVB.push_back(Vector3(LengthP + 3.f * Width, 0.f, 0.f));
				tVB.push_back(Vector3(LengthP - 3.f * Width, -3.f * Width, -3.f * Width));
				tVB.push_back(Vector3(LengthP - 3.f * Width, -3.f * Width, 3.f * Width));
				tVB.push_back(Vector3(LengthP - 3.f * Width, 3.f * Width, 3.f * Width));
				tVB.push_back(Vector3(LengthP - 3.f * Width, 3.f * Width, -3.f * Width));

				static std::vector<uint16> indexBuffer2 = {
					0,1,2,
					0,2,3,
					0,3,4,
					0,4,1,
					0,4,1,
					1,2,3,
					2,3,4
				};

				AddIndexBuffer(ibVec, indexBuffer2, startIB);
			}

			Matrix3 mat;
			mat.FromAxes(AxisFront, AxisUp, AxisRight);
			for (int i = 0; i != tVB.size(); ++i) {
				tVB[i] = mat * tVB[i];
			}

			vbVec.insert(vbVec.end(), tVB.begin(), tVB.end());
			colors.insert(colors.end(), tVB.size(), Color);


		};


		//自身
		{
			vbVec.clear();
			ibVec.clear();
			colors.clear();

			Vector4 colorV4;
			colorV4 = { 1.0f, 1.0f, 1.0f, 1.0f };
			uint32 colorSelf = colorV4.ToRGBA8888();

			DrawBox(Vector3::UNIT_X, Vector3::UNIT_Y, Vector3::UNIT_Z, colorSelf, drawScale * 0.05f, drawScale * 0.05f, drawScale * 0.05f);

			if (vbVec.empty() || ibVec.empty()) {
				m_pHelperObj->clearBuffer();
			}
			else {
				m_pHelperObj->changeCustomPolygon(vbVec, ibVec, colors);
			}
		}

	}

	void JointComponent::SetGlobalDrawScale(float value) {
		gDrawScale = value;
		EchoPhysicsSystem::instance()->SetAllJointIsUpdateHelper(true);
		//GetPhysicsManager()->SetAllJointIsUpdateHelper(true);
	}

#endif // USE_PHYSICS_HELPEROBJ

	void JointComponent::SetHelperVisible(bool value) {
		m_IsHelperVisible = value;
#ifdef USE_PHYSICS_HELPEROBJ
		if (m_IsHelperVisible) CreateHelper();
		if (m_pHelperObj) {
			m_pHelperObj->setVisible(value);
		}
		if (m_pHelperObj1) {
			m_pHelperObj1->setVisible(value);
		}
		if (m_pHelperObj2) {
			m_pHelperObj2->setVisible(value);
		}
		AddToEditorUpdateQueue();
#endif // USE_PHYSICS_HELPEROBJ
	}
	bool JointComponent::GetHelperVisible() {
#ifdef USE_PHYSICS_HELPEROBJ
		return m_IsHelperVisible;
#else
		return false;
#endif // USE_PHYSICS_HELPEROBJ
	}

	
}
