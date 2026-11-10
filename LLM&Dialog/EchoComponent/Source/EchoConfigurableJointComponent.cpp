#include "EchoComponentStableHeaders.h"
#include "EchoConfigurableJointComponent.h"
#include "EchoVector3.h"
#include "EchoPhysXJoint.h"

namespace Echo {

	void WriteJSNode(JSNODE pRoot, const char * name, const JointMotion & value)
	{
		int v = (int)value;
		WriteJSNode(pRoot, name, v);
	}
	bool ReadJSNode(JSNODE pRoot, const char * name, JointMotion & value)
	{
		int v;
		bool res = ReadJSNode(pRoot, name, v);
		if (res) {
			value = (JointMotion)v;
		}
		return res;
	}

	void WriteJSNode(JSNODE pRoot, const char * name, const RDM & value)
	{
		bool v = value == RDM::Slerp;
		WriteJSNode(pRoot, name, v);
	}
	bool ReadJSNode(JSNODE pRoot, const char * name, RDM & value)
	{
		bool v;
		bool res = ReadJSNode(pRoot, name, v);
		if (res) {
			value = (RDM)v;
		}
		return res;
	}

EDITOR_REFLECT_BEGIN(ConfigurableJointComponentInfo)
EDITOR_REFLECT_MEMBER_UPDATE(m_LinearLimit, "LinearLimit", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
EDITOR_REFLECT_MEMBER_UPDATE(m_AngularXLowLimit, "AngularXLowLimit", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
EDITOR_REFLECT_MEMBER_UPDATE(m_AngularXHighLimit, "AngularXHighLimit", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
EDITOR_REFLECT_MEMBER_UPDATE(m_AngularYLimit, "AngularYLimit", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
EDITOR_REFLECT_MEMBER_UPDATE(m_AngularZLimit, "AngularZLimit", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
EDITOR_REFLECT_MEMBER_UPDATE(m_TargetPosition, "TargetPosition", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
EDITOR_REFLECT_MEMBER_UPDATE(m_TargetVelocity, "TargetVelocity", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
EDITOR_REFLECT_MEMBER_UPDATE(m_TargetRotation, "TargetRotation", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
EDITOR_REFLECT_MEMBER_UPDATE(m_TargetAngularVelocity, "TargetAngularVelocity", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
EDITOR_REFLECT_MEMBER_UPDATE(m_ProjectionDistance, "ProjectionDistance", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
EDITOR_REFLECT_MEMBER_UPDATE(m_ProjectionAngle, "ProjectionAngle", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
EDITOR_REFLECT_MEMBER_UPDATE(m_LinearLimitScale, "LinearLimitScale", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
EDITOR_REFLECT_MEMBER_UPDATE(m_LinearLimitParm, "LinearLimitParm", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
EDITOR_REFLECT_MEMBER_UPDATE(m_AngularXLimitParm, "AngularXLimitParm", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
EDITOR_REFLECT_MEMBER_UPDATE(m_AngularYZLimitParm, "AngularYZLimitParm", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
EDITOR_REFLECT_MEMBER_UPDATE(m_DriveX, "DriveX", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
EDITOR_REFLECT_MEMBER_UPDATE(m_DriveY, "DriveY", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
EDITOR_REFLECT_MEMBER_UPDATE(m_DriveZ, "DriveZ", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
EDITOR_REFLECT_MEMBER_UPDATE(m_AngularDriveX, "AngularDriveX", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
EDITOR_REFLECT_MEMBER_UPDATE(m_AngularDriveYZ, "AngularDriveYZ", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
EDITOR_REFLECT_MEMBER_UPDATE(m_SlerpDrive, "SlerpDrive", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
EDITOR_REFLECT_MEMBER_UPDATE(m_RotationDriveMode, "RotationDriveMode", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
EDITOR_REFLECT_MEMBER_UPDATE(m_MotionX, "MotionX", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
EDITOR_REFLECT_MEMBER_UPDATE(m_MotionY, "MotionY", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
EDITOR_REFLECT_MEMBER_UPDATE(m_MotionZ, "MotionZ", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
EDITOR_REFLECT_MEMBER_UPDATE(m_MotionAX, "MotionAX", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
EDITOR_REFLECT_MEMBER_UPDATE(m_MotionAY, "MotionAY", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
EDITOR_REFLECT_MEMBER_UPDATE(m_MotionAZ, "MotionAZ", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
EDITOR_REFLECT_MEMBER_UPDATE(m_UseLinearDrive, "UseLinearDrive", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
EDITOR_REFLECT_MEMBER_UPDATE(m_UseRotationDrive, "UseRotationDrive", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
EDITOR_REFLECT_END();

	ConfigurableJointComponentInfo::ConfigurableJointComponentInfo(const Name& comType, ActorCOMMoveableType MoveType) : JointComponentInfo(comType, MoveType) {}
	ConfigurableJointComponentInfo::~ConfigurableJointComponentInfo() {}

	_DEFINEINFO_VIRFUN(ConfigurableJointComponentInfo, JointComponentInfo)

	
	void ConfigurableJointComponentInfo::CopyToCompData(JointComponentData* pData) {
		if (!pData) return;
		JointComponentInfo::CopyToCompData(pData);
		ConfigurableJointComponentData* pCompData = (ConfigurableJointComponentData*)pData;

		pCompData->linearLimit = m_LinearLimit;
		pCompData->angularXLowLimit = m_AngularXLowLimit;
		pCompData->angularXHighLimit = m_AngularXHighLimit;
		pCompData->angularYLimit = m_AngularYLimit;
		pCompData->angularZLimit = m_AngularZLimit;
		pCompData->linearLimitParm = m_LinearLimitParm;
		pCompData->angularXLimitParm = m_AngularXLimitParm;
		pCompData->angularYZLimitParm = m_AngularYZLimitParm;
		pCompData->driveX = m_DriveX;
		pCompData->driveY = m_DriveY;
		pCompData->driveZ = m_DriveZ;
		pCompData->angularDriveX = m_AngularDriveX;
		pCompData->angularDriveYZ = m_AngularDriveYZ;
		pCompData->slerpDrive = m_SlerpDrive;
		pCompData->targetPosition = m_TargetPosition;
		pCompData->targetVelocity = m_TargetVelocity;
		pCompData->targetRotation = m_TargetRotation;
		pCompData->targetAngularVelocity = m_TargetAngularVelocity;
		pCompData->rotationDriveMode = m_RotationDriveMode;
		pCompData->projectionDistance = m_ProjectionDistance;
		pCompData->projectionAngle = m_ProjectionAngle;
		pCompData->linearLimitScale = m_LinearLimitScale;
		pCompData->motionX = m_MotionX;
		pCompData->motionY = m_MotionY;
		pCompData->motionZ = m_MotionZ;
		pCompData->motionAX = m_MotionAX;
		pCompData->motionAY = m_MotionAY;
		pCompData->motionAZ = m_MotionAZ;
		pCompData->useLinearDrive = m_UseLinearDrive;
		pCompData->useRotationDrive = m_UseRotationDrive;

	}
//////////////////////////////////////////////////////////////////////////

static const PhysXD6Axis D6AXIS[6]
{
	PhysXD6Axis::eX ,
	PhysXD6Axis::eY ,
	PhysXD6Axis::eZ ,
	PhysXD6Axis::eTWIST,
	PhysXD6Axis::eSWING1,
	PhysXD6Axis::eSWING2
};
static const PhysXD6Motion D6MOTION[3] 
{
	PhysXD6Motion::eFREE,
	PhysXD6Motion::eLIMITED,
	PhysXD6Motion::eLOCKED,
};

#define GET_JOINT (static_cast<PhysXD6Joint*> (this->m_pJoint))
#define GET_COMPDATA (static_cast<ConfigurableJointComponentData*>(m_pCompData))
#define TOPHYSXD6AXIS(Axis)		D6AXIS[(int)Axis] 
#define TOPHYSXD6MOTION(Motion) D6MOTION[(int)Motion]


DECLAREACTORCOMTYPE_CPP(ConfigurableJointComponent)

	ConfigurableJointComponent::ConfigurableJointComponent(ActorPtr actor, ActorCOMMoveableType type) : JointComponent(actor, type) {
		m_jointType = JointCompType::CONFIGURABLE_JOINT;
		m_pCompData = new ConfigurableJointComponentData();
	} 
	ConfigurableJointComponent:: ~ConfigurableJointComponent() {
#ifdef USE_PHYSICS_HELPEROBJ
		ReleaseHelper();
#endif // USE_PHYSICS_HELPEROBJ
	}

	void ConfigurableJointComponent::SetMotionX(JointMotion motion){
		SetMotion(JointAxis::XAxis, motion);
	}
	void ConfigurableJointComponent::SetMotionY(JointMotion motion) {
		SetMotion(JointAxis::YAxis, motion);
	}
	void ConfigurableJointComponent::SetMotionZ(JointMotion motion) {
		SetMotion(JointAxis::ZAxis, motion);
	}
	void ConfigurableJointComponent::SetMotionAX(JointMotion motion) {
		SetMotion(JointAxis::XAngular, motion);
	}
	void ConfigurableJointComponent::SetMotionAY(JointMotion motion) {
		SetMotion(JointAxis::YAngular, motion);
	}
	void ConfigurableJointComponent::SetMotionAZ(JointMotion motion) {
		SetMotion(JointAxis::ZAngular, motion);
	}

	JointMotion ConfigurableJointComponent::GetMotionX() {
		return GetMotion(JointAxis::XAxis);
	}
	JointMotion ConfigurableJointComponent::GetMotionY() {
		return GetMotion(JointAxis::YAxis);
	}
	JointMotion ConfigurableJointComponent::GetMotionZ() {
		return GetMotion(JointAxis::ZAxis);
	}
	JointMotion ConfigurableJointComponent::GetMotionAX() {
		return GetMotion(JointAxis::XAngular);
	}
	JointMotion ConfigurableJointComponent::GetMotionAY() {
		return GetMotion(JointAxis::YAngular);
	}
	JointMotion ConfigurableJointComponent::GetMotionAZ() {
		return GetMotion(JointAxis::ZAngular);
	}

	void ConfigurableJointComponent::SetMotion(JointAxis axis, JointMotion motion)
	{
		switch (axis)
		{
		case Echo::JointAxis::XAxis:
			GET_COMPDATA->motionX = motion;
			break;
		case Echo::JointAxis::YAxis:
			GET_COMPDATA->motionY = motion;
			break;
		case Echo::JointAxis::ZAxis:
			GET_COMPDATA->motionZ = motion;
			break;
		case Echo::JointAxis::XAngular:
			GET_COMPDATA->motionAX = motion;
			break;
		case Echo::JointAxis::YAngular:
			GET_COMPDATA->motionAY = motion;
			break;
		case Echo::JointAxis::ZAngular:
			GET_COMPDATA->motionAZ = motion;
			break;
		}

		if (IsExistPhysics())
			GET_JOINT->setMotion(TOPHYSXD6AXIS(axis), TOPHYSXD6MOTION(motion));

		AddToEditorUpdateQueue();
	}

	JointMotion ConfigurableJointComponent::GetMotion(JointAxis axis)
	{
		JointMotion res;
		switch (axis)
		{
		case Echo::JointAxis::XAxis:
			res = (JointMotion)GET_COMPDATA->motionX;
			break;
		case Echo::JointAxis::YAxis:
			res = (JointMotion)GET_COMPDATA->motionY;
			break;
		case Echo::JointAxis::ZAxis:
			res = (JointMotion)GET_COMPDATA->motionZ;
			break;
		case Echo::JointAxis::XAngular:
			res = (JointMotion)GET_COMPDATA->motionAX;
			break;
		case Echo::JointAxis::YAngular:
			res = (JointMotion)GET_COMPDATA->motionAY;
			break;
		case Echo::JointAxis::ZAngular:
			res = (JointMotion)GET_COMPDATA->motionAZ;
			break;
		}
		return res;
	}


	void ConfigurableJointComponent::SetLinearLimit(float value) { 
		value = Math::Clamp(value, 0.0f, FLT_MAX);
		GET_COMPDATA->linearLimit = value;
		ChangeLinearLimit();
	}
	void ConfigurableJointComponent::SetAngularXLowLimit(float value) { 
		value = Math::Clamp(value, -180.0f, GetAngularXHighLimit());
		GET_COMPDATA->angularXLowLimit = value;
		ChangeAngularXLimit();
	}
	void ConfigurableJointComponent::SetAngularXHighLimit(float value) { 
		value = Math::Clamp(value, GetAngularXLowLimit(), 180.0f);
		GET_COMPDATA->angularXHighLimit = value;
		ChangeAngularXLimit();
	}
	void ConfigurableJointComponent::SetAngularYLimit(float value) { 
		value = Math::Clamp(value, 0.0f, 180.0f);
		GET_COMPDATA->angularYLimit = value;
		ChangeAngularYZLimit();
	}
	void ConfigurableJointComponent::SetAngularZLimit(float value) { 
		value = Math::Clamp(value, 0.0f, 180.0f);
		GET_COMPDATA->angularZLimit = value;
		ChangeAngularYZLimit();
	}
	void ConfigurableJointComponent::SetLinearLimitParm(const JointLimit& value) { 
		GET_COMPDATA->linearLimitParm = value;
		GET_COMPDATA->linearLimitParm.ToValid();
		ChangeLinearLimit();
	}
	void ConfigurableJointComponent::SetAngularXLimitParm(const JointLimit& value) { 
		GET_COMPDATA->angularXLimitParm = value;
		GET_COMPDATA->angularXLimitParm.ToValid();
		GET_COMPDATA->angularXLimitParm.contanctDistance = Math::Clamp(GET_COMPDATA->angularXLimitParm.contanctDistance, 0.0f, GetAngularXHighLimit() - GetAngularXLowLimit());
		ChangeAngularXLimit();
	}
	void ConfigurableJointComponent::SetAngularYZLimitParm(const JointLimit& value) { 
		GET_COMPDATA->angularYZLimitParm = value;
		GET_COMPDATA->angularYZLimitParm.ToValid();
		ChangeAngularYZLimit();
	}
	float ConfigurableJointComponent::GetLinearLimit() { return GET_COMPDATA->linearLimit; }
	float ConfigurableJointComponent::GetAngularXLowLimit() { return GET_COMPDATA->angularXLowLimit; }
	float ConfigurableJointComponent::GetAngularXHighLimit() { return GET_COMPDATA->angularXHighLimit; }
	float ConfigurableJointComponent::GetAngularYLimit() { return GET_COMPDATA->angularYLimit; }
	float ConfigurableJointComponent::GetAngularZLimit() { return GET_COMPDATA->angularZLimit; }
	const JointLimit& ConfigurableJointComponent::GetLinearLimitParm() { return GET_COMPDATA->linearLimitParm; }
	const JointLimit& ConfigurableJointComponent::GetAngularXLimitParm() { return GET_COMPDATA->angularXLimitParm; }
	const JointLimit& ConfigurableJointComponent::GetAngularYZLimitParm() { return GET_COMPDATA->angularYZLimitParm; }
	void ConfigurableJointComponent::ChangeLinearLimit() {
		if (IsExistPhysics())
		{
			PhysXJointLinearLimit limit(
				GET_COMPDATA->linearLimitParm.IsSoft ? GET_COMPDATA->linearLimitParm.spring : 0.0f,
				GET_COMPDATA->linearLimitParm.IsSoft ? GET_COMPDATA->linearLimitParm.damper : 0.0f,
				GetScaleLinearLimit(),
				GET_COMPDATA->linearLimitParm.IsSoft ? 0.0f : GET_COMPDATA->linearLimitParm.bounciness,
				GET_COMPDATA->linearLimitParm.contanctDistance);
			limit.ToValid();
			GET_JOINT->setLinearLimit(limit);
		}
		AddToEditorUpdateQueue();
	}
	void ConfigurableJointComponent::ChangeAngularXLimit() {
		if (IsExistPhysics())
		{
			PhysXJointAngularLimitPair limit(
				GET_COMPDATA->angularXLimitParm.IsSoft ? GET_COMPDATA->angularXLimitParm.spring : 0.0f,
				GET_COMPDATA->angularXLimitParm.IsSoft ? GET_COMPDATA->angularXLimitParm.damper : 0.0f,
				Deg2Rad(GET_COMPDATA->angularXLowLimit),
				Deg2Rad(GET_COMPDATA->angularXHighLimit),
				GET_COMPDATA->angularXLimitParm.IsSoft ? 0.0f : GET_COMPDATA->angularXLimitParm.bounciness,
				GET_COMPDATA->angularXLimitParm.contanctDistance);
			limit.ToValid();
			GET_JOINT->setTwistLimit(limit);
		}
		AddToEditorUpdateQueue();
	}
	void ConfigurableJointComponent::ChangeAngularYZLimit() {
		if (IsExistPhysics())
		{
			PhysXJointLimitCone limit(
				GET_COMPDATA->angularYZLimitParm.IsSoft ? GET_COMPDATA->angularYZLimitParm.spring : 0.0f,
				GET_COMPDATA->angularYZLimitParm.IsSoft ? GET_COMPDATA->angularYZLimitParm.damper : 0.0f,
				Deg2Rad(GET_COMPDATA->angularYLimit),
				Deg2Rad(GET_COMPDATA->angularZLimit),
				GET_COMPDATA->angularYZLimitParm.IsSoft ? 0.0f : GET_COMPDATA->angularYZLimitParm.bounciness,
				GET_COMPDATA->angularYZLimitParm.contanctDistance);
			limit.ToValid();
			GET_JOINT->setSwingLimit(limit);
		}
		AddToEditorUpdateQueue();
	}
	float ConfigurableJointComponent::GetScaleLinearLimit() {
		float limit = GetLinearLimit();
		if (!GetLinearLimitScale()) return limit;
		Vector3 scale = getWorldScale();
		return limit * std::min(std::min(std::abs(scale.x), std::abs(scale.y)), std::abs(scale.z));
	}

	void ConfigurableJointComponent::SetTargetPosition(const Vector3 & pos)
	{
		GET_COMPDATA->targetPosition = pos;

		if (IsExistPhysics()) {
			Quaternion quat;
			quat.FromEuler(GET_COMPDATA->targetRotation);
			GET_JOINT->setDrivePosition(Transform(GET_COMPDATA->targetPosition, quat));
		}
	}

	const Vector3 & ConfigurableJointComponent::GetTargetPosition()
	{
		return 	GET_COMPDATA->targetPosition;
	}

	void ConfigurableJointComponent::SetTargetVelocity(const Vector3 & vel)
	{
		GET_COMPDATA->targetVelocity = vel;

		if (IsExistPhysics())
			GET_JOINT->setDriveVelocity(GET_COMPDATA->targetVelocity, GET_COMPDATA->targetAngularVelocity);
	}

	const Vector3 & ConfigurableJointComponent::GetTargetVelocity()
	{
		return GET_COMPDATA->targetVelocity;
	}

	void ConfigurableJointComponent::SetDriveX(const JointDrive & drive)
	{
		GET_COMPDATA->driveX = drive;

		ChangeLinearDrive(1);
	}

	const JointDrive & ConfigurableJointComponent::GetDriveX()
	{
		return GET_COMPDATA->driveX;
	}

	void ConfigurableJointComponent::SetDriveY(const JointDrive & drive)
	{
		GET_COMPDATA->driveY = drive;

		ChangeLinearDrive(2);
			
	}

	const JointDrive & ConfigurableJointComponent::GetDriveY()
	{
		return GET_COMPDATA->driveY;
	}

	void ConfigurableJointComponent::SetDriveZ(const JointDrive & drive)
	{
		GET_COMPDATA->driveZ = drive;

		ChangeLinearDrive(4);
	}

	const JointDrive & ConfigurableJointComponent::GetDriveZ()
	{
		return GET_COMPDATA->driveZ;
	}

	void ConfigurableJointComponent::SetTargetRotationQ(const Quaternion& rotation) {
		Quaternion quat = rotation;
		GET_COMPDATA->targetRotation = quat.ToEulerPositive();

		if (IsExistPhysics()) {
			GET_JOINT->setDrivePosition(Transform(GET_COMPDATA->targetPosition, quat));
		}
	}
	Quaternion ConfigurableJointComponent::GetTargetRotationQ() {
		Quaternion targetRotationQ;
		targetRotationQ.FromEuler(GET_COMPDATA->targetRotation);
		return targetRotationQ;
	}

	void ConfigurableJointComponent::SetTargetRotation(const Vector3& rotation)
	{
		GET_COMPDATA->targetRotation = rotation;

		if (IsExistPhysics()) {
			Quaternion quat;
			quat.FromEuler(GET_COMPDATA->targetRotation);
			GET_JOINT->setDrivePosition(Transform(GET_COMPDATA->targetPosition, quat));
		}
	}

	const Vector3& ConfigurableJointComponent::GetTargetRotation()
	{
		return GET_COMPDATA->targetRotation;
	}

	void ConfigurableJointComponent::SetTargetAngularVelocity(const Vector3 & vel)
	{
		GET_COMPDATA->targetAngularVelocity = vel;

		if (IsExistPhysics())
			GET_JOINT->setDriveVelocity(GET_COMPDATA->targetVelocity, GET_COMPDATA->targetAngularVelocity);
	}

	const Vector3 & ConfigurableJointComponent::GetTargetAngularVelocity()
	{
		return GET_COMPDATA->targetAngularVelocity;
	}

	void ConfigurableJointComponent::SetUseLinearDrive(bool bUse) {
		GET_COMPDATA->useLinearDrive = bUse;
		ChangeLinearDrive(15);
	}
	bool ConfigurableJointComponent::GetUseLinearDrive() {
		return GET_COMPDATA->useLinearDrive;
	}

	void ConfigurableJointComponent::SetUseRotationDrive(bool bUse) {
		GET_COMPDATA->useRotationDrive = bUse;
		ChangeRotationDrive(15);
	}
	bool ConfigurableJointComponent::GetUseRotationDrive() {
		return GET_COMPDATA->useRotationDrive;
	}

	void ConfigurableJointComponent::ChangeRotationDrive(uint32 mask) {
		if (!IsExistPhysics()) return;
		if (GetUseRotationDrive()) {
			if (mask & 1) {
				PhysXD6JointDrive pxDrive(GET_COMPDATA->angularDriveX.spring, GET_COMPDATA->angularDriveX.damper, GET_COMPDATA->angularDriveX.maximumForce, false);
				pxDrive.ToValid();
				GET_JOINT->setDrive(PhysXD6Drive::eTWIST, pxDrive);
			}
			if (mask & 2) {
				PhysXD6JointDrive pxDrive(GET_COMPDATA->angularDriveYZ.spring, GET_COMPDATA->angularDriveYZ.damper, GET_COMPDATA->angularDriveYZ.maximumForce, false);
				pxDrive.ToValid();
				GET_JOINT->setDrive(PhysXD6Drive::eSWING, pxDrive);
			}
			if (mask & 4) {
				PhysXD6JointDrive pxDrive(GET_COMPDATA->slerpDrive.spring, GET_COMPDATA->slerpDrive.damper, GET_COMPDATA->slerpDrive.maximumForce, false);
				pxDrive.ToValid();
				GET_JOINT->setDrive(PhysXD6Drive::eSLERP, pxDrive);
			}
		}
		else if(mask & 8) {
			PhysXD6JointDrive pxDrive(0.0f, 0.0f, 0.0f, false);
			GET_JOINT->setDrive(PhysXD6Drive::eTWIST, pxDrive);
			GET_JOINT->setDrive(PhysXD6Drive::eSWING, pxDrive);
			GET_JOINT->setDrive(PhysXD6Drive::eSLERP, pxDrive);
		}
	}

	void ConfigurableJointComponent::ChangeLinearDrive(uint32 mask) {
		if (!IsExistPhysics()) return;
		if (GetUseLinearDrive()) {
			if (mask & 1) {
				PhysXD6JointDrive pxDrive(GET_COMPDATA->driveX.spring, GET_COMPDATA->driveX.damper, GET_COMPDATA->driveX.maximumForce, false);
				pxDrive.ToValid();
				GET_JOINT->setDrive(PhysXD6Drive::eX, pxDrive);
			}
			if (mask & 2) {
				PhysXD6JointDrive pxDrive(GET_COMPDATA->driveY.spring, GET_COMPDATA->driveY.damper, GET_COMPDATA->driveY.maximumForce, false);
				pxDrive.ToValid();
				GET_JOINT->setDrive(PhysXD6Drive::eY, pxDrive);
			}
			if (mask & 4) {
				PhysXD6JointDrive pxDrive(GET_COMPDATA->driveZ.spring, GET_COMPDATA->driveZ.damper, GET_COMPDATA->driveZ.maximumForce, false);
				pxDrive.ToValid();
				GET_JOINT->setDrive(PhysXD6Drive::eZ, pxDrive);
			}
		}
		else if (mask & 8) {
			PhysXD6JointDrive pxDrive(0.0f, 0.0f, 0.0f, false);
			GET_JOINT->setDrive(PhysXD6Drive::eX, pxDrive);
			GET_JOINT->setDrive(PhysXD6Drive::eY, pxDrive);
			GET_JOINT->setDrive(PhysXD6Drive::eZ, pxDrive);
		}
	}

	void ConfigurableJointComponent::SetRotationDriveMode(RDM mode)
	{
		GET_COMPDATA->rotationDriveMode = mode;

		if (IsExistPhysics())
			GET_JOINT->setConstraintFlag(PhysXConstraintFlag::eIMPROVED_SLERP, mode == RDM::Slerp);
	}

	RDM ConfigurableJointComponent::GetRotationDriveMode()
	{
		return GET_COMPDATA->rotationDriveMode;
	}

	void ConfigurableJointComponent::SetAngularDriveX(const JointDrive & drive)
	{
		GET_COMPDATA->angularDriveX = drive;

		ChangeRotationDrive(1);

	}

	const JointDrive & ConfigurableJointComponent::GetAngularDriveX()
	{
		return GET_COMPDATA->angularDriveX;
	}

	void ConfigurableJointComponent::SetAngularDriveYZ(const JointDrive & drive)
	{
		GET_COMPDATA->angularDriveYZ = drive;

		ChangeRotationDrive(2);
	}

	const JointDrive & ConfigurableJointComponent::GetAngularDriveYZ()
	{
		return GET_COMPDATA->angularDriveYZ;
	}

	void ConfigurableJointComponent::SetSlerpDrive(const JointDrive & drive)
	{
		GET_COMPDATA->slerpDrive = drive;

		ChangeRotationDrive(4);
	}

	const JointDrive & ConfigurableJointComponent::GetSlerpDrive()
	{
		return GET_COMPDATA->slerpDrive;
	}

	void ConfigurableJointComponent::SetProjectionDistance(float dis)
	{
		GET_COMPDATA->projectionDistance = dis;

		if (IsExistPhysics())
			GET_JOINT->setProjectionLinearTolerance(dis);
	}

	float ConfigurableJointComponent::GetProjectionDistance()
	{
		return GET_COMPDATA->projectionDistance;
	}

	void ConfigurableJointComponent::SetProjectionAngle(float angle)
	{
		GET_COMPDATA->projectionAngle = angle;

		if (IsExistPhysics())
			GET_JOINT->setProjectionAngularTolerance(angle);
	}

	float ConfigurableJointComponent::GetProjectionAngle()
	{
		return GET_COMPDATA->projectionAngle;
	}

	void ConfigurableJointComponent::SetLinearLimitScale(bool enable)
	{
		GET_COMPDATA->linearLimitScale = enable;

		if (IsExistPhysics())
			ChangeLinearLimit();
	}

	bool ConfigurableJointComponent::GetLinearLimitScale()
	{
		return GET_COMPDATA->linearLimitScale;
	}
	void ConfigurableJointComponent::SetDriveMode(bool value) {
		if (!IsExistPhysics()) return;
		GET_JOINT->setDriveModel(value);
	}
	bool ConfigurableJointComponent::GetDriveMode() {
		if (!IsExistPhysics()) return false;
		return GET_JOINT->getDriveModel();
	}

	float ConfigurableJointComponent::GetTwistAngle() {
		if (!IsExistPhysics()) return 0.0f;
		return Rad2Deg(GET_JOINT->getTwist());
	}
	float ConfigurableJointComponent::GetSwingYAngle() {
		if (!IsExistPhysics()) return 0.0f;
		return Rad2Deg(GET_JOINT->getSwingYAngle());
	}
	float ConfigurableJointComponent::GetSwingZAngle() {
		if (!IsExistPhysics()) return 0.0f;
		return Rad2Deg(GET_JOINT->getSwingZAngle());
	}

	void ConfigurableJointComponent::SetCompData(JointComponentData* pData) {
		JointComponent::SetCompData(pData);
		ConfigurableJointComponentData* pCompData = (ConfigurableJointComponentData*)(pData ? pData : m_pCompData);

		SetLinearLimit(pCompData->linearLimit);
		SetAngularXLowLimit(pCompData->angularXLowLimit);
		SetAngularXHighLimit(pCompData->angularXHighLimit);
		SetAngularYLimit(pCompData->angularYLimit);
		SetAngularZLimit(pCompData->angularZLimit);
		SetLinearLimitParm(pCompData->linearLimitParm);
		SetAngularXLimitParm(pCompData->angularXLimitParm);
		SetAngularYZLimitParm(pCompData->angularYZLimitParm);

		// SetSecondaryAxis(pCompData->secondaryAxis);
		// SetAxisOffset(pCompData->axisOffset);
		SetMotion(JointAxis::XAxis, (JointMotion)pCompData->motionX);
		SetMotion(JointAxis::YAxis, (JointMotion)pCompData->motionY);
		SetMotion(JointAxis::ZAxis, (JointMotion)pCompData->motionZ);
		SetMotion(JointAxis::XAngular, (JointMotion)pCompData->motionAX);
		SetMotion(JointAxis::YAngular, (JointMotion)pCompData->motionAY);
		SetMotion(JointAxis::ZAngular, (JointMotion)pCompData->motionAZ);

// 		SetLinearLimitSpring(pCompData->linearLimitSpring);
// 		SetLinearLimit(pCompData->linearLimit);
// 
// 		SetAngularXLimitSpring(pCompData->angularXLimitSpring);
// 		SetAngularXLowLimit(pCompData->angularXLowLimit);
// 		SetAngularXHighLimit(pCompData->angularXHighLimit);
// 
// 		SetAngularYZLimitSpring(pCompData->angularYZLimitSpring);
// 		SetAngularYLimit(pCompData->angularYLimit);
// 		SetAngularZLimit(pCompData->angularZLimit);

		SetTargetPosition(pCompData->targetPosition);
		SetTargetVelocity(pCompData->targetAngularVelocity);

		SetDriveX(pCompData->driveX);
		SetDriveY(pCompData->driveY);
		SetDriveZ(pCompData->driveZ);

		SetTargetRotation(pCompData->targetRotation);
		SetTargetAngularVelocity(pCompData->targetAngularVelocity);

		SetRotationDriveMode(pCompData->rotationDriveMode);

		SetAngularDriveX(pCompData->angularDriveX);
		SetAngularDriveYZ(pCompData->angularDriveYZ);
		SetSlerpDrive(pCompData->slerpDrive);

		SetProjectionDistance(pCompData->projectionDistance);
		SetProjectionAngle(pCompData->projectionAngle);

		//SetConfiguredInWorldSpace(pCompData->configuredInWorldSpace);
		SetLinearLimitScale(pCompData->linearLimitScale);

		SetUseLinearDrive(pCompData->useLinearDrive);
		SetUseRotationDrive(pCompData->useRotationDrive);
	}

	void ConfigurableJointComponent::GetJointTransform(DTransform& trans) {
		JointComponent::GetJointTransform(trans);
		Vector3 rotation = GetAxisOffset();
		Quaternion quat;
		quat.FromEuler(rotation);
		trans.rotation = trans.rotation * quat;
	}

};


namespace Echo {

#ifdef USE_PHYSICS_HELPEROBJ

	void ConfigurableJointComponent::CreateHelper() {
		if (m_pHelperObj) ReleaseHelper();
		Name helperName = Name(getCombinedName().append("||").append("HelperObj"));
		m_pHelperObj = new HelperObject(helperName, HelperObjType_Box, false, nullptr, getSceneManager(), true);
		addChildToNode(m_pHelperObj->getSceneNode());
		m_pHelperObj->setVisible(m_IsHelperVisible);
		m_pHelperObj->setRenderType(ECHO_DRAW::ECHO_TRIANGLE_LIST);
		m_pHelperObj->setHelperColorType(HelperObjColorType_Green);
		m_pHelperObj->setOrientation(Quaternion::IDENTITY);
		m_pHelperObj->setScale(Vector3::UNIT_SCALE);

		helperName = Name(getCombinedName().append("||").append("HelperObj1"));
		m_pHelperObj1 = new HelperObject(helperName, HelperObjType_Box, false, nullptr, getSceneManager(), true);
		m_pHelperObj1->setVisible(m_IsHelperVisible);
		m_pHelperObj1->setRenderType(ECHO_DRAW::ECHO_TRIANGLE_LIST);
		m_pHelperObj1->setHelperColorType(HelperObjColorType_Green);
		m_pHelperObj1->setOrientation(Quaternion::IDENTITY);
		m_pHelperObj1->setScale(Vector3::UNIT_SCALE);

		helperName = Name(getCombinedName().append("||").append("HelperObj2"));
		m_pHelperObj2 = new HelperObject(helperName, HelperObjType_Box, false, nullptr, getSceneManager(), true);
		m_pHelperObj2->setVisible(m_IsHelperVisible);
		m_pHelperObj2->setRenderType(ECHO_DRAW::ECHO_TRIANGLE_LIST);
		m_pHelperObj2->setHelperColorType(HelperObjColorType_Green);
		m_pHelperObj2->setOrientation(Quaternion::IDENTITY);
		m_pHelperObj2->setScale(Vector3::UNIT_SCALE);

	}
	void ConfigurableJointComponent::ReleaseHelper() {
		if (m_pHelperObj) {
			RemoveChildFromNode(m_pHelperObj->getSceneNode());
			SAFE_DELETE(m_pHelperObj);
		}
		if (m_pHelperObj1) {
			SAFE_DELETE(m_pHelperObj1);
		}
		if (m_pHelperObj2) {
			SAFE_DELETE(m_pHelperObj2);
		}
	}

	void ConfigurableJointComponent::UpdateHelper() {
		if (!m_pHelperObj || !m_pHelperObj->getVisible()) {
			return;
		}

		float drawScale = gDrawScale;

		const PhysicsActorComponentPtr& body0 = GetSelfRigidBodyComp();
		const PhysicsActorComponentPtr& body1 = GetConnectedRigidBodyComp();
		bool bRenderBody0 = false, bRenderBody1 = false;
		if (!body0) {
			m_pHelperObj1->setPosition(Vector3::ZERO);
			m_pHelperObj1->setOrientation(Quaternion::IDENTITY);
			bRenderBody0 = true;
		}
		else {
			m_pHelperObj1->setPosition(body0->getWorldPositionPhysX());
			m_pHelperObj1->setOrientation(body0->getWorldRotationPhysX());
			if (GetPhysicsManager()->PhysicsIsRunning()) {
				bRenderBody0 = body0->_getActorPhysX() != nullptr;
			}
			else {
				bRenderBody0 = true;
			}
		}

		if (!body1) {
			m_pHelperObj2->setPosition(Vector3::ZERO);
			m_pHelperObj2->setOrientation(Quaternion::IDENTITY);
			bRenderBody1 = true;
		}
		else {
			m_pHelperObj2->setPosition(body1->getWorldPositionPhysX());
			m_pHelperObj2->setOrientation(body1->getWorldRotationPhysX());
			if (GetPhysicsManager()->PhysicsIsRunning()) {
				bRenderBody1 = body1->_getActorPhysX() != nullptr;
			}
			else {
				bRenderBody1 = true;
			}
		}

		uint32 flags = GetVisibleFlags();

		std::vector<Vector3> vbVec;
		std::vector<uint16> ibVec;
		std::vector<uint32> colors;


		float AngleXLow = GetAngularXLowLimit();
		float AngleXHigh = GetAngularXHighLimit();
		float AngleY = GetAngularYLimit();
		float AngleZ = GetAngularZLimit();
		float LinearLimit = GetScaleLinearLimit();

		AngleXHigh = Math::Clamp(AngleXHigh, -179.5f, 179.5f);
		AngleXLow = Math::Clamp(AngleXLow, -179.5f, AngleXHigh);
		if (AngleXLow == AngleXHigh) {
			AngleXLow = AngleXLow - 0.25f;
			AngleXHigh = AngleXHigh + 0.25f;
		}

		AngleY = Math::Clamp(AngleY, 0.5f, 179.5f);
		AngleZ = Math::Clamp(AngleZ, 0.5f, 179.5f);
		LinearLimit = Math::Clamp(LinearLimit, 0.f, FLT_MAX);

		bool rSelfBox, rRigidBody1Axis, rRigidBody2Axis, rLinearLimit, rAngularLimit, rToRigidBodyLine1, rToRigidBodyLine2;
		rSelfBox = (flags & (uint32)(ConfigurableJointVisibleFlag::CenterBox)) != 0;
		rRigidBody1Axis = (flags & (uint32)(ConfigurableJointVisibleFlag::RigidBody1Axis)) != 0;
		rRigidBody2Axis = (flags & (uint32)(ConfigurableJointVisibleFlag::RigidBody2Axis)) != 0;
		rLinearLimit = (flags & (uint32)(ConfigurableJointVisibleFlag::LinearLimit)) != 0;
		rAngularLimit = (flags & (uint32)(ConfigurableJointVisibleFlag::AngleLimit)) != 0;
		rToRigidBodyLine1 = ((flags & (uint32)(ConfigurableJointVisibleFlag::ToRigidBodyLine)) != 0) && body0;
		rToRigidBodyLine2 = ((flags & (uint32)(ConfigurableJointVisibleFlag::ToRigidBodyLine)) != 0) && body1;


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

				// 				ibVec.push_back(startIB + 0); ibVec.push_back(startIB + 1); ibVec.push_back(startIB + 2);
				// 				ibVec.push_back(startIB + 0); ibVec.push_back(startIB + 2); ibVec.push_back(startIB + 3);
				// 				ibVec.push_back(startIB + 0); ibVec.push_back(startIB + 3); ibVec.push_back(startIB + 4);
				// 				ibVec.push_back(startIB + 0); ibVec.push_back(startIB + 4); ibVec.push_back(startIB + 1);
				// 				ibVec.push_back(startIB + 0); ibVec.push_back(startIB + 4); ibVec.push_back(startIB + 1);
				// 				ibVec.push_back(startIB + 1); ibVec.push_back(startIB + 2); ibVec.push_back(startIB + 3);
				// 				ibVec.push_back(startIB + 2); ibVec.push_back(startIB + 3); ibVec.push_back(startIB + 4);

			}

			Matrix3 mat;
			mat.FromAxes(AxisFront, AxisUp, AxisRight);
			for (int i = 0; i != tVB.size(); ++i) {
				tVB[i] = mat * tVB[i];
			}

			vbVec.insert(vbVec.end(), tVB.begin(), tVB.end());
			colors.insert(colors.end(), tVB.size(), Color);

			// 			for (int i = 0; i != tVB.size(); ++i) {
			// 				tVB[i] = mat * tVB[i];
			// 				vbVec.push_back(tVB[i]);
			// 				colors.push_back(Color);
			// 			}


		};
		//绘制锥面
		auto DrawCone = [&](float Angle1/*0~180*/, float Angle2/*0~-180*/, const Quaternion& WorldRotation, uint32 Color, float Length = 1.0f, uint32 numSides = 40) {
			Angle1 = Deg2Rad(Angle1) * 0.5f;
			Angle2 = Deg2Rad(Angle2) * 0.5f;
			float a = Math::Sin(Angle1);
			float b = Math::Sin(Angle2);
			uint16 startIB = (uint16)vbVec.size();

			std::vector<Vector3> tVB(numSides);

			vbVec.push_back(Vector3::ZERO);
			colors.push_back(Color);

			for (int i = 0; i != numSides; ++i) {
				float curAngle = 2 * ECHO_PI * i / numSides;
				float sTheta = Math::Sin(curAngle);
				float cTheta = Math::Cos(curAngle);

				auto phi = Math::ATan2(a * sTheta, b * cTheta);
				float sqr = a * a * sTheta * sTheta + b * b * cTheta * cTheta;
				float rsq = Math::Sqrt(1 - sqr);
				float r = Math::Sqrt(sqr);
				float x = 1 - 2 * sqr;
				float y = 2 * r * rsq * Math::Cos(phi);
				float z = 2 * r * rsq * Math::Sin(phi);
				tVB[i] = WorldRotation * (Vector3(x, y, z) * Length);
			}
			for (int i = 0; i != tVB.size() - 1; ++i) {
				vbVec.push_back(tVB[i]);
				colors.push_back(Color);
				ibVec.push_back(startIB + i + 1);
				ibVec.push_back(startIB);
				ibVec.push_back(startIB + i + 2);

				ibVec.push_back(startIB + i + 2);
				ibVec.push_back(startIB);
				ibVec.push_back(startIB + i + 1);
			}
			vbVec.push_back(tVB[tVB.size() - 1]);
			colors.push_back(Color);
			ibVec.push_back(startIB + (uint16)tVB.size());
			ibVec.push_back(startIB);
			ibVec.push_back(startIB + 1);

			ibVec.push_back(startIB + 1);
			ibVec.push_back(startIB);
			ibVec.push_back(startIB + (uint16)tVB.size());
		};

		//绘制扇面
		auto DrawSector = [&](float Angle1, float Angle2, const Vector3& AxisRight, const Vector3& AxisFront, uint32 Color, float Length, uint32 numSides = 40) {

			if (AxisFront.isZeroLength() || AxisFront.IsNan() || AxisRight.isZeroLength() || AxisRight.IsNan() || Length < 1e-06 || numSides < 2) {
				return;
			}

			uint16 startIB = (uint16)vbVec.size();


			std::vector<Vector3> tVB(numSides);

			Angle1 = Deg2Rad(Angle1);
			Angle2 = Deg2Rad(Angle2);

			float spaceAngle = (Angle2 - Angle1) / (float)(numSides - 1);

			Matrix3 mat;
			Radian rad;

			vbVec.push_back(Vector3::ZERO);
			colors.push_back(Color);

			Vector3 curAxis = AxisFront;
			curAxis.Normalize();
			curAxis = curAxis * Length;
			rad = Angle1;
			mat.FromAngleAxis(AxisRight, rad);
			curAxis = mat * curAxis;
			rad = spaceAngle;
			mat.FromAngleAxis(AxisRight, rad);
			for (int i = 0; i != numSides; ++i) {
				tVB[i] = curAxis;
				curAxis = mat * curAxis;
			}

			for (int i = 0; i != tVB.size() - 1; ++i) {
				vbVec.push_back(tVB[i]);
				colors.push_back(Color);
				ibVec.push_back(startIB + i + 1);
				ibVec.push_back(startIB);
				ibVec.push_back(startIB + i + 2);

				ibVec.push_back(startIB + i + 2);
				ibVec.push_back(startIB);
				ibVec.push_back(startIB + i + 1);
			}
			vbVec.push_back(tVB[tVB.size() - 1]);
			colors.push_back(Color);


		};

		auto DrawLine = [&](const Vector3& pos1, const Vector3& pos2, uint32 color, float Width = 0.01f) {


			uint16 startIB = (uint16)vbVec.size();

			Vector3 target = pos2 - pos1;
			float Length = target.length();
			target.Normalize();

			std::vector<Vector3> tVB(8);
			tVB[0] = Vector3(0, -Width, -Width);	//0
			tVB[1] = Vector3(0, -Width, Width);	//1
			tVB[2] = Vector3(0, Width, Width);	//2
			tVB[3] = Vector3(0, Width, -Width);	//3
			tVB[4] = Vector3(Length, -Width, -Width);	//4
			tVB[5] = Vector3(Length, -Width, Width);	//5
			tVB[6] = Vector3(Length, Width, Width);	//6
			tVB[7] = Vector3(Length, Width, -Width);	//7

			ibVec.push_back(startIB + 0);
			ibVec.push_back(startIB + 1);
			ibVec.push_back(startIB + 2);
			ibVec.push_back(startIB + 0);
			ibVec.push_back(startIB + 2);
			ibVec.push_back(startIB + 3);
			ibVec.push_back(startIB + 4);
			ibVec.push_back(startIB + 6);
			ibVec.push_back(startIB + 5);
			ibVec.push_back(startIB + 4);
			ibVec.push_back(startIB + 7);
			ibVec.push_back(startIB + 6);
			ibVec.push_back(startIB + 2);
			ibVec.push_back(startIB + 6);
			ibVec.push_back(startIB + 7);
			ibVec.push_back(startIB + 2);
			ibVec.push_back(startIB + 7);
			ibVec.push_back(startIB + 3);
			ibVec.push_back(startIB + 0);
			ibVec.push_back(startIB + 4);
			ibVec.push_back(startIB + 1);
			ibVec.push_back(startIB + 4);
			ibVec.push_back(startIB + 5);
			ibVec.push_back(startIB + 1);
			ibVec.push_back(startIB + 1);
			ibVec.push_back(startIB + 5);
			ibVec.push_back(startIB + 6);
			ibVec.push_back(startIB + 1);
			ibVec.push_back(startIB + 6);
			ibVec.push_back(startIB + 2);
			ibVec.push_back(startIB + 0);
			ibVec.push_back(startIB + 3);
			ibVec.push_back(startIB + 7);
			ibVec.push_back(startIB + 0);
			ibVec.push_back(startIB + 7);
			ibVec.push_back(startIB + 4);

			Quaternion quat;
			quat.RotationAxisToAxis(Vector3::UNIT_X, target);

			for (int i = 0; i != tVB.size(); ++i) {
				tVB[i] = quat * tVB[i];
				vbVec.push_back(tVB[i]);
				colors.push_back(color);
			}
		};


		Quaternion rigid1WorldRotation = Quaternion::IDENTITY;

		//刚体1

		if (bRenderBody0) {
			vbVec.clear();
			ibVec.clear();
			colors.clear();

			Vector4 colorV4;
			colorV4 = { 1.0f, 0.0f, 0.0f, 0.6f };
			uint32 colorR = colorV4.ToRGBA8888();
			colorV4 = { 0.0f, 1.0f, 0.0f, 0.6f };
			uint32 colorG = colorV4.ToRGBA8888();
			colorV4 = { 0.0f, 0.0f, 1.0f, 0.6f };
			uint32 colorB = colorV4.ToRGBA8888();
			colorV4 = { 0.0f, 1.0f, 0.0f, 1.0f };
			uint32 colorL = colorV4.ToRGBA8888();
			colorV4 = { 1.0f, 0.0f, 0.0f, 0.6f };
			uint32 colorRigid = colorV4.ToRGBA8888();

			Transform localPose;
			GetLocalPose0(localPose);
			if (!IsExistPhysics()) {
				Quaternion RotationOffset;
				RotationOffset.FromEuler(GetAxisOffset());
				localPose.rotation = localPose.rotation * RotationOffset;
			}

			Vector3 CompAxisX = Vector3::UNIT_X * localPose.rotation;
			Vector3 CompAxisY = Vector3::UNIT_Y * localPose.rotation;
			Vector3 CompAxisZ = Vector3::UNIT_Z * localPose.rotation;

			rigid1WorldRotation = m_pHelperObj1->getSceneNode()->convertLocalToWorldOrientation(localPose.rotation);

			if (rRigidBody1Axis) {
				DrawBox(CompAxisX, CompAxisY, CompAxisZ, colorR, drawScale * 1.1f, 0.0f, drawScale * 0.01f, true);
				DrawBox(CompAxisY, CompAxisZ, CompAxisX, colorG, drawScale * 1.1f, 0.0f, drawScale * 0.01f, true);
				DrawBox(CompAxisZ, CompAxisX, CompAxisY, colorB, drawScale * 1.1f, 0.0f, drawScale * 0.01f, true);
			}

			if (rLinearLimit) {
				if (GetMotionX() == JointMotion::LIMITED) DrawBox(CompAxisX, CompAxisY, CompAxisZ, colorL, LinearLimit, LinearLimit, drawScale * 0.005f);
				if (GetMotionY() == JointMotion::LIMITED) DrawBox(CompAxisY, CompAxisX, CompAxisZ, colorL, LinearLimit, LinearLimit, drawScale * 0.005f);
				if (GetMotionZ() == JointMotion::LIMITED) DrawBox(CompAxisZ, CompAxisX, CompAxisY, colorL, LinearLimit, LinearLimit, drawScale * 0.005f);
			}

			for (int i = 0; i != vbVec.size(); ++i) {
				vbVec[i] += localPose.translation;
			}

			if (rToRigidBodyLine1) DrawLine(Vector3::ZERO, localPose.translation, colorRigid, drawScale * 0.005f);

			if (vbVec.empty() || ibVec.empty()) {
				m_pHelperObj1->clearBuffer();
			}
			else {
				m_pHelperObj1->changeCustomPolygon(vbVec, ibVec, colors);
			}

		}

		//刚体2
		if (bRenderBody1) {

			vbVec.clear();
			ibVec.clear();
			colors.clear();

			Vector4 colorV4;

			colorV4 = { 1.0f, 0.0f, 0.0f, 0.6f };
			uint32 colorR = colorV4.ToRGBA8888();
			colorV4 = { 0.0f, 1.0f, 0.0f, 0.6f };
			uint32 colorG = colorV4.ToRGBA8888();
			colorV4 = { 0.0f, 0.0f, 1.0f, 0.6f };
			uint32 colorB = colorV4.ToRGBA8888();

			colorV4 = { 0.0f, 0.0f, 0.8f, 0.3f };
			uint32 colorAX = colorV4.ToRGBA8888();
			colorV4 = { 0.8f, 0.0f, 0.0f, 0.3f };
			uint32 colorAYZ = colorV4.ToRGBA8888();
			colorV4 = { 0.0f, 0.0f, 1.0f, 0.6f };
			uint32 colorRigid = colorV4.ToRGBA8888();

			Transform localPose;
			GetLocalPose1(localPose);
			Vector3 CompAxisX = Vector3::UNIT_X * localPose.rotation;
			Vector3 CompAxisY = Vector3::UNIT_Y * localPose.rotation;
			Vector3 CompAxisZ = Vector3::UNIT_Z * localPose.rotation;

			Quaternion quat = m_pHelperObj2->getSceneNode()->convertWorldToLocalOrientation(rigid1WorldRotation);
			Vector3 axisX = Vector3::UNIT_X * quat;
			Vector3 axisY = Vector3::UNIT_Y * quat;
			Vector3 axisZ = Vector3::UNIT_Z * quat;

			if (rRigidBody2Axis) {

				DrawBox(CompAxisX, CompAxisY, CompAxisZ, colorR, drawScale * 1.1f, 0.0f, drawScale * 0.01f);
				DrawBox(CompAxisY, CompAxisZ, CompAxisX, colorG, drawScale * 1.1f, 0.0f, drawScale * 0.01f);
				DrawBox(CompAxisZ, CompAxisX, CompAxisY, colorB, drawScale * 1.1f, 0.0f, drawScale * 0.01f);

				DrawBox(axisX, axisY, axisZ, colorR, drawScale * 1.1f, 0.0f, drawScale * 0.01f, true);
				DrawBox(axisY, axisZ, axisX, colorG, drawScale * 1.1f, 0.0f, drawScale * 0.01f, true);
				DrawBox(axisZ, axisX, axisY, colorB, drawScale * 1.1f, 0.0f, drawScale * 0.01f, true);
			}

			if (rAngularLimit) {
				JointMotion axm = GetMotionAX();
				JointMotion aym = GetMotionAY();
				JointMotion azm = GetMotionAZ();

				if (axm == JointMotion::LIMITED) {
					DrawSector(AngleXLow, AngleXHigh, axisX, axisZ, colorAX, drawScale * 1.0f);//X
				}
				else if (axm == JointMotion::FREE) {
					DrawSector(-180.0f, 180.0f, axisX, axisZ, colorAX, drawScale * 1.0f);//X
				}
				if ((aym == JointMotion::FREE) && (azm == JointMotion::FREE)) {
					DrawSector(-180.0f, 180.0f, CompAxisY, CompAxisX, colorAYZ, drawScale * 1.0f);
					DrawSector(-180.0f, 180.0f, CompAxisZ, CompAxisX, colorAYZ, drawScale * 1.0f);
				}
				else if ((aym != JointMotion::LOCKED) || (azm != JointMotion::LOCKED)) {
					if (aym == JointMotion::FREE) {
						AngleY = 179.5f;
					}
					else if (aym == JointMotion::LOCKED) {
						AngleY = 0.5f;
					}
					if (azm == JointMotion::FREE) {
						AngleZ = 179.5f;
					}
					else if (azm == JointMotion::LOCKED) {
						AngleZ = 0.5f;
					}
					DrawCone(AngleY, AngleZ, localPose.rotation, colorAYZ, drawScale * 1.0f);//YZ
				}
			}

			for (int i = 0; i != vbVec.size(); ++i) {
				vbVec[i] += localPose.translation;
			}

			if (rToRigidBodyLine2) DrawLine(Vector3::ZERO, localPose.translation, colorRigid, drawScale * 0.005f);

			if (vbVec.empty() || ibVec.empty()) {
				m_pHelperObj2->clearBuffer();
			}
			else {
				m_pHelperObj2->changeCustomPolygon(vbVec, ibVec, colors);
			}
		}


		//自身
		{
			vbVec.clear();
			ibVec.clear();
			colors.clear();

			Vector4 colorV4;
			colorV4 = { 1.0f, 1.0f, 1.0f, 1.0f };
			uint32 colorSelf = colorV4.ToRGBA8888();

			if (rSelfBox) {
				DrawBox(Vector3::UNIT_X, Vector3::UNIT_Y, Vector3::UNIT_Z, colorSelf, drawScale * 0.05f, drawScale * 0.05f, drawScale * 0.05f);
			}
			if (vbVec.empty() || ibVec.empty()) {
				m_pHelperObj->clearBuffer();
			}
			else {
				m_pHelperObj->changeCustomPolygon(vbVec, ibVec, colors);
			}
		}
	}
#endif // USE_PHYSICS_HELPEROBJ
	void ConfigurableJointComponent::SetVisibleFlags(uint32 value) {
#ifdef USE_PHYSICS_HELPEROBJ
		mVisibleFlags = value;
		AddToEditorUpdateQueue();
#endif // USE_PHYSICS_HELPEROBJ
	}
	uint32 ConfigurableJointComponent::GetVisibleFlags() {
#ifdef USE_PHYSICS_HELPEROBJ
		return mVisibleFlags;
#else
		return (uint32)ConfigurableJointVisibleFlag::All;
#endif // USE_PHYSICS_HELPEROBJ
	}

}
