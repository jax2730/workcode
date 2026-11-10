//-----------------------------------------------------------------------------
// File:   EchoConfigurableJointComponent.h
//
// Author: qiao_yuzhi
//
// Date:   2022-8-17
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef EchoConfigurableJointComponent_h__
#define EchoConfigurableJointComponent_h__

#include "EchoComponentPrerequisites.h"
#include "EchoJointComponent.h"
#include "EchoQuaternion.h"

namespace Echo
{
	enum class JointAxis : uint32
	{
		XAxis , YAxis , ZAxis ,
		XAngular , YAngular , ZAngular
	};

	enum class JointMotion : int
	{
		FREE, LIMITED, LOCKED
	};

	// RDM : Rotation Drive Mode 
	enum class RDM : bool
	{
		XAndYZ , Slerp
	};

	enum class ConfigurableJointVisibleFlag : uint32 {
		None = 0,
		CenterBox = 1 << 0,
		RigidBody1Axis = 1 << 1,
		RigidBody2Axis = 1 << 2,
		LinearLimit = 1 << 3,
		AngleLimit = 1 << 4,
		ToRigidBodyLine = 1 << 5,
		Axis = RigidBody1Axis | RigidBody2Axis,
		Limit = LinearLimit | AngleLimit,
		All = 0xFFFFFFFF
	};

	struct ConfigurableJointComponentData : public JointComponentData {
		JointMotion		motionX = JointMotion::FREE; 
		JointMotion		motionY = JointMotion::FREE; 
		JointMotion		motionZ = JointMotion::FREE; 
		JointMotion		motionAX = JointMotion::FREE; 
		JointMotion		motionAY = JointMotion::FREE; 
		JointMotion		motionAZ = JointMotion::FREE; 
		//限制参数 start
		float			linearLimit = 0.0f;
		float			angularXLowLimit = 0.0f;
		float			angularXHighLimit = 0.0f;
		float			angularYLimit = 0.0f;
		float			angularZLimit = 0.0f;
		JointLimit		linearLimitParm = {};
		JointLimit		angularXLimitParm = {};
		JointLimit		angularYZLimitParm = {};
		//限制参数 end
		Vector3			targetPosition = Vector3::ZERO;
		Vector3			targetVelocity = Vector3::ZERO;
		JointDrive		driveX = {};
		JointDrive		driveY = {};
		JointDrive		driveZ = {};
		Vector3			targetRotation = Vector3::ZERO;
		Vector3			targetAngularVelocity = Vector3::ZERO;
		RDM				rotationDriveMode = RDM::XAndYZ; ///< enum RDM
		JointDrive		angularDriveX = {};
		JointDrive		angularDriveYZ = {};
		JointDrive		slerpDrive = {};
		float			projectionDistance = 0.1f;
		float			projectionAngle = 180.f;
		bool			configuredInWorldSpace = false;
		bool			linearLimitScale = false;
		bool			useRotationDrive = true;
		bool			useLinearDrive = true;
	};

	_EchoComponentExport void WriteJSNode(JSNODE pRoot, const char * name, const JointMotion & value);
	_EchoComponentExport bool ReadJSNode(JSNODE pRoot, const char * name, JointMotion & value);
	_EchoComponentExport void WriteJSNode(JSNODE pRoot, const char * name, const RDM & value);
	_EchoComponentExport bool ReadJSNode(JSNODE pRoot, const char * name, RDM & value);

	class ConfigurableJointComponentInfo;

	class _EchoComponentExport ConfigurableJointComponent : public JointComponent
	{
	public:
		DECLAREACTORCOMTYPE_H(ConfigurableJointComponent)
	public: 
		void				SetMotionX(JointMotion motion);
		void				SetMotionY(JointMotion motion);
		void				SetMotionZ(JointMotion motion);
		void				SetMotionAX(JointMotion motion);
		void				SetMotionAY(JointMotion motion);
		void				SetMotionAZ(JointMotion motion);
		JointMotion			GetMotionX();
		JointMotion			GetMotionY();
		JointMotion			GetMotionZ();
		JointMotion			GetMotionAX();
		JointMotion			GetMotionAY();
		JointMotion			GetMotionAZ();
		void				SetMotion(JointAxis axis , JointMotion motion);
		JointMotion			GetMotion(JointAxis axis);

		void				SetLinearLimit(float value);
		void				SetAngularXLowLimit(float value);
		void				SetAngularXHighLimit(float value);
		void				SetAngularYLimit(float value);
		void				SetAngularZLimit(float value);
		void				SetLinearLimitParm(const JointLimit& value);
		void				SetAngularXLimitParm(const JointLimit& value);
		void				SetAngularYZLimitParm(const JointLimit& value); 

		float				GetLinearLimit();
		float				GetAngularXLowLimit();
		float				GetAngularXHighLimit();
		float				GetAngularYLimit();
		float				GetAngularZLimit();
		const JointLimit&	GetLinearLimitParm();
		const JointLimit&	GetAngularXLimitParm();
		const JointLimit&	GetAngularYZLimitParm();

		void				SetTargetPosition(const Vector3 & pos);
		const Vector3 &		GetTargetPosition();
		void				SetTargetVelocity(const Vector3 & vel);
		const Vector3 &		GetTargetVelocity();

		void				SetDriveX(const JointDrive & drive);
		const JointDrive &	GetDriveX();
		void				SetDriveY(const JointDrive & drive);
		const JointDrive &	GetDriveY();
		void				SetDriveZ(const JointDrive & drive);
		const JointDrive &	GetDriveZ();

		void				SetTargetRotationQ(const Quaternion& rotation);
		Quaternion			GetTargetRotationQ();
		void				SetTargetRotation(const Vector3& rotation);
		const Vector3&		GetTargetRotation();
		void				SetTargetAngularVelocity(const Vector3 & vel);
		const Vector3 &		GetTargetAngularVelocity();

		void				SetUseLinearDrive(bool bUse);
		bool				GetUseLinearDrive();
		void				SetUseRotationDrive(bool bUse);
		bool				GetUseRotationDrive();

		void				SetRotationDriveMode(RDM mode);
		RDM					GetRotationDriveMode();

		void				SetAngularDriveX(const JointDrive & drive);
		const JointDrive &	GetAngularDriveX();
		void				SetAngularDriveYZ(const JointDrive & drive);
		const JointDrive &	GetAngularDriveYZ();
		void				SetSlerpDrive(const JointDrive & drive);
		const JointDrive &	GetSlerpDrive();

		void				SetProjectionDistance(float dis);
		float				GetProjectionDistance();
		void				SetProjectionAngle(float angle);
		float				GetProjectionAngle();

		void				SetLinearLimitScale(bool enable);
		bool				GetLinearLimitScale();

		void				SetDriveMode(bool value);
		bool				GetDriveMode();
		virtual void		SetCompData(JointComponentData* pData) override;
		void				SetVisibleFlags(uint32 value);
		uint32				GetVisibleFlags();


		float				GetTwistAngle();
		float				GetSwingYAngle();
		float				GetSwingZAngle();
	private:
		virtual void GetJointTransform(DTransform& trans) override;
		void ChangeLinearLimit();
		void ChangeAngularXLimit();
		void ChangeAngularYZLimit();
		float GetScaleLinearLimit();
		void ChangeRotationDrive(uint32 mask);
		void ChangeLinearDrive(uint32 mask);
#ifdef USE_PHYSICS_HELPEROBJ
	private:
		virtual void CreateHelper() override;
		virtual void ReleaseHelper() override;
		virtual void UpdateHelper() override;
	protected:
		uint32 mVisibleFlags = (uint32)ConfigurableJointVisibleFlag::All;
#endif // USE_PHYSICS_HELPEROBJ
	};

	class _EchoComponentExport ConfigurableJointComponentInfo : public JointComponentInfo
	{
	public:
		ConfigurableJointComponentInfo() {}
		ConfigurableJointComponentInfo(const Name& comType, ActorCOMMoveableType MoveType);
		virtual ~ConfigurableJointComponentInfo();
	public:
		virtual void	CopyToCompData(JointComponentData* pData) override;
		_DECLCOMINFO_VIRFUN
	public:

		_DECLCOMINFO_PARAM_BEGIN(ConfigurableJointComponent)
		_DECLCOMINFO_PARAM_SIM(float, LinearLimit, 0.0f, SetLinearLimit);
		_DECLCOMINFO_PARAM_SIM(float, AngularXLowLimit, 0.0f, SetAngularXLowLimit);
		_DECLCOMINFO_PARAM_SIM(float, AngularXHighLimit, 0.0f, SetAngularXHighLimit);
		_DECLCOMINFO_PARAM_SIM(float, AngularYLimit, 0.0f, SetAngularYLimit);
		_DECLCOMINFO_PARAM_SIM(float, AngularZLimit, 0.0f, SetAngularZLimit);
		_DECLCOMINFO_PARAM(JointLimit, LinearLimitParm, JointLimit(), SetLinearLimitParm);
		_DECLCOMINFO_PARAM(JointLimit, AngularXLimitParm, JointLimit(), SetAngularXLimitParm);
		_DECLCOMINFO_PARAM(JointLimit, AngularYZLimitParm, JointLimit(), SetAngularYZLimitParm);
		_DECLCOMINFO_PARAM(JointDrive, DriveX, JointDrive(), SetDriveX);
		_DECLCOMINFO_PARAM(JointDrive, DriveY, JointDrive(), SetDriveY);
		_DECLCOMINFO_PARAM(JointDrive, DriveZ, JointDrive(), SetDriveZ);
		_DECLCOMINFO_PARAM(JointDrive, AngularDriveX, JointDrive(), SetAngularDriveX);
		_DECLCOMINFO_PARAM(JointDrive, AngularDriveYZ, JointDrive(), SetAngularDriveYZ);
		_DECLCOMINFO_PARAM(JointDrive, SlerpDrive, JointDrive(), SetSlerpDrive);
		_DECLCOMINFO_PARAM(Vector3, TargetPosition, Vector3::ZERO, SetTargetPosition);
		_DECLCOMINFO_PARAM(Vector3, TargetVelocity, Vector3::ZERO, SetTargetVelocity);
		_DECLCOMINFO_PARAM(Vector3, TargetRotation, Vector3::ZERO, SetTargetRotation);
		_DECLCOMINFO_PARAM(Vector3, TargetAngularVelocity, Vector3::ZERO, SetTargetAngularVelocity);
		_DECLCOMINFO_PARAM_SIM(RDM, RotationDriveMode, RDM::XAndYZ, SetRotationDriveMode);
		_DECLCOMINFO_PARAM_SIM(float, ProjectionDistance, 0.1f, SetProjectionDistance);
		_DECLCOMINFO_PARAM_SIM(float, ProjectionAngle, 180.f, SetProjectionAngle);
		_DECLCOMINFO_PARAM_SIM(bool, LinearLimitScale, false, SetLinearLimitScale);
		_DECLCOMINFO_PARAM_SIM(JointMotion, MotionX, JointMotion::FREE, SetMotionX);
		_DECLCOMINFO_PARAM_SIM(JointMotion, MotionY, JointMotion::FREE, SetMotionY);
		_DECLCOMINFO_PARAM_SIM(JointMotion, MotionZ, JointMotion::FREE, SetMotionZ);
		_DECLCOMINFO_PARAM_SIM(JointMotion, MotionAX, JointMotion::FREE, SetMotionAX);
		_DECLCOMINFO_PARAM_SIM(JointMotion, MotionAY, JointMotion::FREE, SetMotionAY);
		_DECLCOMINFO_PARAM_SIM(JointMotion, MotionAZ, JointMotion::FREE, SetMotionAZ);
		_DECLCOMINFO_PARAMS_SIM(bool, UseLinearDrive, true);
		_DECLCOMINFO_PARAMS_SIM(bool, UseRotationDrive, true);
		_DECLCOMINFO_PARAM_END()
	};

	DEFINECOMFACTORY(ConfigurableJointComponent,ALL_MOVEABLE)
}
#endif // EchoConfigurableJointComponent_h__
