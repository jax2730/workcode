//-----------------------------------------------------------------------------
// File:   EchoJointComponent.h
//
// Author: qiao_yuzhi
//
// Date:   2022-8-17
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef EchoJointComponent_h__
#define EchoJointComponent_h__

#include "EchoComponentPrerequisites.h"
#include "EchoHelperObj.h"
#include "Actor/EchoActorComponent.h"
#include "EchoComDefineMacro.h"
#include "EchoPhysicsManager.h"
#include "EchoRigidBodyComponent.h"
#include "EchoComDefineMacro.h"
#include "EchocJsonHelper.h"

namespace Echo
{

	enum class JointCompType {
		INVALID_JOINT,
		CHARARCTER_JOINT,
		CONFIGURABLE_JOINT,
		FIXED_JOINT,
		HINGE_JOINT,
		SPRING_JOINT
	};

	struct JointLimit
	{
		bool	IsSoft = false;			//	区分软极限和硬极限
		float	bounciness = 0;			// 反弹力 [0~1.f]
		float	contanctDistance = 0;	// 限制距离公差
		float	spring = 0;
		float	damper = 0;
		void ToValid() {
			bounciness = Math::Clamp(bounciness, 0.f, 1.0f);
			contanctDistance = Math::Clamp(contanctDistance, 0.f, FLT_MAX);
			spring = Math::Clamp(spring, 0.f, FLT_MAX);
			damper = Math::Clamp(damper, 0.f, FLT_MAX);
		}
	};

	/*
		force = spring * (target position - position) + damper * (target velocity - velocity);
		force = Clamp( force, -maximumForce, maximumForce );
	*/
	struct JointDrive
	{
		float	spring = 0.f;
		float	damper = 0.f;
		float   maximumForce = 0.f;
	};
	struct JointComponentData {

		String			connectedComName = "";				// 依赖的刚体
		String			selfComName = "";					// 自身的刚体
		Vector3			axisOffset = Vector3::ZERO;				// 轴偏移
		Transform		localPose0 = Transform::IDENTITY;		// 关节在刚体1坐标系下的相对姿态
		Transform		localPose1 = Transform::IDENTITY;		// 关节在刚体2坐标系下的相对姿态
		float			breakForce = FLT_MAX;					// 被破环时需要的 力
		float			breakTorque = FLT_MAX;					// 被破环时需要的 力矩
		bool			enableProjection = false;				// 启用投影
		bool			enableCollision = false;				// 启用连接的两个刚体间的碰撞检测
		bool			enablePreprocessing = true;				// 启用预处理
		float			massScaleSelf = 1.f;					// 关节所属的刚体的质量缩放
		float			massScaleConnected = 1.f;				// 被连接的刚体的质量缩放		
		bool			autoCalculateLocalPose = true;			// 是否每帧重置相对姿态

	};

	class JointComponentInfo;

	class _EchoComponentExport JointComponent : public PhysicsLinkComponent, public ComponentNode
	{
	public:
		NODE_ACTOR_COM_H
	public:
		JointComponent(ActorPtr actor, ActorCOMMoveableType type);
		virtual ~JointComponent();
	public:
		virtual void		init(ActorComponentInfo * pInfo)				override final;
		virtual void		unInit()										override final;
		virtual void		replay(ActorComponentInfo * pInfo)				override final;
		virtual void		showEditorHelper(bool isShow)					override final;
		virtual void		copyTo(const ActorComponentPtr& newComp)		override final;
		virtual void		updateCombinedName()							override final;
		virtual void		visibleStateChanged(bool activeState)			override final;
		virtual void		connectCom()									override final;
		virtual void		disconnectCom()									override final;
	public:
		virtual void		CreatePhysics() override final;
		virtual void		ReleasePhysics() override final;
		virtual bool		IsExistPhysics()override final;
		virtual bool		IsCanPhysics()	 override final;
		PhysXJoint*			GetPhysics();
	public:
		std::pair<Vector3, Vector3>		GetForceAndTorque();							// 获得物体时受到的力和力矩		（仅在物理模拟时有效）
		bool							Isbroken();										// 关节是否被破环				（仅在物理模拟时有效）
		inline JointCompType GetJointType() { return m_jointType; }
	protected:
		virtual EchoPhysicsComponentType GetPhysicsType() override { return EchoPhysicsComponentType::eJOINT; }
		virtual void GetJointTransform(DTransform& trans);
		virtual void GetJointLocalPose(Transform & t0, Transform& t1);//transform是关节相对于刚体的位置
		void AttachRigidBodyComp(bool bR0 = true, bool bR1 = true);
		void DetachRigidBodyComp(bool bR0 = true, bool bR1 = true);
		void DetachRigidBody(PhysicsActorComponent* pComp);

		virtual void _actorNotifyPhysX(PhysicsActorComponent* pComp, EchoPhysXNotifyType type) override;
	public:
		void					SetConnectedRigidBody(const String& ComName);
		const String&			GetConnectedRigidBody();
		void					SetSelfRigidBody(const String& ComName);
		const String&			GetSelfRigidBody();
		void					SetAxisOffset(const Vector3& axis);//物理运行时无法修改
		const Vector3 &			GetAxisOffset();
		Vector3					GetSecondaryAxis();
		Vector3					GetAxis();		
		void					SetPhysicsAxis(const Vector3& axis1, const Vector3& axis2);

		const Vector3&			GetAnchor();
		const Vector3&			GetAnchorConnected();
		const Quaternion&		GetAnchorRotation();
		const Quaternion&		GetAnchorConnectedRotation();
		void					SetAutoCalculateLocalPose(bool value);
		bool					GetAutoCalculateLocalPose();

		void					SetBreakForce(float breakForce);
		float					GetBreakForce();
		void					SetBreakTorque(float breakTorque);
		float					GetBreakTorque();
		
		void					SetEnableProjection(bool enable);
		bool					GetEnableProjection();

		void					SetEnableCollision(bool enable);
		bool					GetEnableCollision();

		void					SetEnablePreprocessing(bool enable);
		bool					GetEnablePreprocessing();

		void					SetMassScaleSelf(float massScale);
		float					GetMassScaleSelf();
		void					SetMassScaleConnected(float massScale);
		float					GetMassScaleConnected();

		Vector3					GetCurrentForce();
		Vector3					GetCurrentTorque();
		void					GetCurrentForceAndTorque(Vector3& force,Vector3& torque);

		inline bool				GetIsJointBreak() { return mIsBreak; }


		const PhysicsActorComponentPtr& GetSelfRigidBodyComp();
		const PhysicsActorComponentPtr& GetConnectedRigidBodyComp();
		virtual void			SetCompData(JointComponentData* pData);
		JointComponentData*		GetCompData();
		void					ResetLocalPose(bool IsResetPhysics = false);

		void					SetHelperVisible(bool value);
		bool					GetHelperVisible();

		void					SetIsUsePhysXPage(bool value);
		void					SetIsUsePhysics(bool value);
	public:
		void					SetLocalPose0(const Transform& trans);
		void					SetLocalPose1(const Transform& trans);
		void					GetLocalPose0(Transform& trans);
		void					GetLocalPose1(Transform& trans);
		void					GetRelativeTransform(Transform& trans);
		void					GetRelativeLinearVelocity(Vector3& value);
		void					GetRelativeAngularVelocity(Vector3& value);
	protected:
		void					CreatePhysXObject();
		void					ReleasePhysXObject();
	public:
		void					AddJointCallBack(const EchoJointCallBackPtr& pComp);
		void					RemoveJointCallBack(const EchoJointCallBackPtr& pComp);
	private:
		void					SendJointBreak();
		std::set<EchoJointCallBackPtr>	mListenerList;
#ifdef USE_PHYSICS_HELPEROBJ
	protected:
		virtual void			UpdateComponentInEditor() override final;
		virtual void			CreateHelper();
		virtual void			ReleaseHelper();
		virtual void			UpdateHelper();
		HelperObject* m_pHelperObj = nullptr;
		HelperObject* m_pHelperObj1 = nullptr;
		HelperObject* m_pHelperObj2 = nullptr;
	public:
		static float			gDrawScale;
		static void				SetGlobalDrawScale(float value);
#endif // USE_PHYSICS_HELPEROBJ
	protected:
		JointCompType			m_jointType			= JointCompType::INVALID_JOINT;	// 子类在构造函数中设置此值
		PhysXJoint*				m_pJoint			= nullptr;
		JointComponentData*		m_pCompData			= nullptr;
		bool					mIsBreak			= false;
		PhysicsActorComponentPtr m_pRigidBodyComp0 = nullptr;
		PhysicsActorComponentPtr m_pRigidBodyComp1 = nullptr;

#ifdef USE_PHYSICS_PAGE
	public:
		EchoPhysXActorObject* GetEchoPhysXObject();
#endif //USE_PHYSICS_PAGE
	private:
		friend class EchoPhysicsSystem;
		friend class EchoPhysicsManager;
		friend class RigidBodyComponent;
		friend class EchoPhysicsSceneCallBack;
		friend class ColliderComponent;
	};

	//////////////////////////////////////////////////////////////////////////
	class _EchoComponentExport JointComponentInfo : public ActorComponentInfo
	{
	public:
		JointComponentInfo() {}
		JointComponentInfo(const Name& comType, ActorCOMMoveableType MoveType);
		virtual ~JointComponentInfo();
		virtual void	CopyToCompData(JointComponentData* pData);

		_DECLCOMINFO_VIRFUN
	public:
		_DECLCOMINFO_PARAM_BEGIN(JointComponent)
		_DECLCOMINFO_PARAM(Vector3, AxisOffset, Vector3::ZERO, SetAxisOffset);
		_DECLCOMINFO_PARAM_SIM(float, BreakForce, FLT_MAX, SetBreakForce);
		_DECLCOMINFO_PARAM_SIM(float, BreakTorque, FLT_MAX, SetBreakTorque);
		_DECLCOMINFO_PARAM_SIM(bool, EnableProjection, false, SetEnableProjection);
		_DECLCOMINFO_PARAM_SIM(bool, EnableCollision, false, SetEnableCollision);
		_DECLCOMINFO_PARAM_SIM(bool, EnablePreprocessing, true, SetEnablePreprocessing);
		_DECLCOMINFO_PARAM_SIM(bool, AutoCalculateLocalPose, true, SetAutoCalculateLocalPose);
		_DECLCOMINFO_PARAM_SIM(float, MassScaleSelf, 1.f, SetMassScaleSelf);
		_DECLCOMINFO_PARAM_SIM(float, MassScaleConnected, 1.f, SetMassScaleConnected);
		_DECLCOMINFO_PARAM(String, ConnectedComName, "", SetConnectedRigidBody);
		_DECLCOMINFO_PARAM(String, SelfComName, "", SetSelfRigidBody);
		_DECLCOMINFO_PARAM(Transform, LocalPose0, Transform::IDENTITY, SetLocalPose0);
		_DECLCOMINFO_PARAM(Transform, LocalPose1, Transform::IDENTITY, SetLocalPose1);
		_DECLCOMINFO_PARAM_END()
	};

	_EchoComponentExport void WriteJSNode(JSNODE pRoot, const char* name, const JointLimit& limit);
	_EchoComponentExport bool ReadJSNode(JSNODE pRoot, const char* name, JointLimit& limit);

	_EchoComponentExport void WriteJSNode(JSNODE pRoot, const char* name, const JointDrive& drive);
	_EchoComponentExport bool ReadJSNode(JSNODE pRoot, const char* name, JointDrive& drive);

}
#endif // EchoJointComponent_h__
