//-----------------------------------------------------------------------------
// File:   EchoRigidBodyComponent.h
//
// Author: qiao_yuzhi
//
// Date:   2022-7-27
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef __EchoRigidBodyComponent_h__ 
#define __EchoRigidBodyComponent_h__

#include "EchoComponentPrerequisites.h"
#include "EchoProtocolComponent.h"
#include "Actor/EchoActor.h"

#include "EchoHelperObj.h"
#include "EchoHelperObjProFuncImp.h"
#include "EchoPhysicsManager.h"
#include "EchoComDefineMacro.h"
#include "EchoJointComponent.h"
#include "EchoComInfoDefineMacro.h"


namespace Echo {

	namespace EchoPhysics {

		enum RigidChange :uint32
		{
			MassChange =			1 << 0,
			PhysicsCreate =			1 << 1,
			CMOffset				= 1 << 2,
			Count 
		};
		enum ForceMode : int
		{
			FORCE = 0,
			IMPULSE,
			VELOCITY_CHANGE,
			ACCELERATION,
			FORCE_MODE_NUMBER,
		};

		enum RigidbodyConstraints : int {
			FreezeNone = 0,
			FreezePositionX = (1 << 0),
			FreezePositionY = (1 << 1),
			FreezePositionZ = (1 << 2),
			FreezeRotationX = (1 << 3),
			FreezeRotationY = (1 << 4),
			FreezeRotationZ = (1 << 5),
			FreezePosition = FreezePositionX | FreezePositionY | FreezePositionZ,
			FreezeRotation = FreezeRotationX | FreezeRotationY | FreezeRotationZ,
			FreezeAll = FreezePosition | FreezeRotation,
		};
		enum CollisionDetectionMode : int {
			Discrete = 0,
			Continuous = 1,
			ContinuousDynamic = 2,
			ContinuousSpeculative = 3
		};

		enum CenterOfMassMode : int {
			AutoMode = 0,
			OffsetMode = 1,
			AutoAndOffsetMode = 2,
			CustomMode = 3
		};
	}

	struct _EchoComponentExport RigidBodyComponentData {

		bool	UseGravity = false;//控制重力是否影响该刚体。若为 false则刚体将呈现失重状态。
		bool	IsKinematic = false;//控制物理是否影响该刚体，若为true则力、碰撞或关节将不再影响刚体，刚体的Transform由用户控制。若为false则正常。
		int		CollisionDetection = EchoPhysics::Discrete;
		int		Constraints = EchoPhysics::FreezeNone;
		float	Mass = 1.0f;
		float	Drag = 0.0f;
		float	AngularDrag = 0.05f;
		int		CenterOfMassMode = EchoPhysics::AutoMode;
		Vector3 CenterOfMassOffset = Vector3::ZERO;

		Vector3 InertiaTensor = Vector3::UNIT_SCALE;
		Vector3 InertiaTensorRotation = Vector3::ZERO;
		Vector3 LocalCenterofMass = Vector3::ZERO;

		RigidBodyComponentData();
		virtual ~RigidBodyComponentData();
		RigidBodyComponentData& operator=(const RigidBodyComponentData& other);
	};

	class RigidBodyComponentInfo;
	class _EchoComponentExport RigidBodyComponent : public PhysicsActorComponent {
		DECLAREACTORCOMTYPE_H(RigidBodyComponent)
		REGISTER_LUA_CLASS_KEY;

	public://继承自ActorComponent
		virtual void init(ActorComponentInfo* info) override;
		virtual void unInit() override;
		virtual void replay(ActorComponentInfo* info) override;
		virtual void copyTo(const ActorComponentPtr& actCom) override;
		virtual void NotifiChildImp(int changeMask) override;
		virtual void setTimelineAttribute(std::vector<float>& data,const Name& name) override;
		virtual void visibleStateChanged(bool activeState) override;
		virtual void connectCom() override;
		virtual void disconnectCom() override;

		virtual DVector3 getWorldPositionPhysX() override;
		virtual Quaternion getWorldRotationPhysX() override;
		virtual DTransform getWorldTransformPhysX() override;
	//==============================物理部分start=====================================
	public:
		virtual void CreatePhysics() override;
		virtual void ReleasePhysics() override;
		virtual bool IsCanPhysics() override;
		virtual bool IsExistPhysics() override;
#ifdef USE_PHYSICS_HELPEROBJ
		virtual void UpdateComponentInEditor() override {}
#endif // USE_PHYSICS_HELPEROBJ
		PhysXRigidDynamic* GetPhysics();
	protected:
		virtual EchoPhysicsComponentType GetPhysicsType() override { return EchoPhysicsComponentType::eRIGIDBODY; }
		void SetWorldTransformFromPhysX(const DTransform& trans);
		void SetLocalTransformFromPhysX(const DTransform& trans);
		void SyncWorldTransformToPhysX();
		bool IsNeedChangeCollider();
		void DetachSelfCollider();
		void AttachSelfCollider();
		void ResetAttachSelfCollider();
		bool CheckSelfCollider();
		void AddCollider(ColliderComponentPtr& pComp);
		void RemoveCollider(ColliderComponentPtr& pComp);
		void RemoveSelfCollider();
		void OnParentingChanged(const ActorComponentPtr& compPtr);
	public:
		AxisAlignedBox GetPhysicsWorldAABB();
		const AxisAlignedBox& GetPhysicsLocalAABB();
		const std::unordered_set<ColliderComponentPtr>& GetColliderCompList();
		void SetIsUsePhysXPage(bool value);
		void SetIsUsePhysics(bool value);
		const RigidBodyComponentPtr& GetParentRigidBodyPtr() const { return mParentRigidBodyPtr; }
		uint32 GetRigidBodyTreeLevel() const;
	public:
		//=================================================用户调用start===========================================================
		//质量
		void SetMass(float mass);
		float GetMass();

		//线性阻尼
		void SetDrag(float drag);
		float GetDrag();

		//角阻尼
		void SetAngularDrag(float angularDrag);
		float GetAngularDrag();

		//使用重力
		void SetUseGravity(bool useGravity);
		bool GetUseGravity();

		//运动学
		void SetIsKinematic(bool isKinematic);
		bool GetIsKinematic();

		//碰撞检测模式
		void SetCollisionDetection(int collisionDetection);//enum EchoPhysics::CollisionDetectionMode
		int GetCollisionDetection();

		//刚体限制
		void SetConstraints(int constraints);//enum EchoPhysics::RigidbodyConstraints
		int GetConstraints();

		//线性速度值
		void SetSpeed(float value);
		float GetSpeed();

		//线性速度
		void SetVelocity(const Vector3& value);
		const Vector3& GetVelocity();

		//角速度
		void SetAngularVelocity(const Vector3& value);
		const Vector3& GetAngularVelocity();

		//线性惯性张量
		void SetInertiaTensor(const Vector3& value);
		const Vector3& GetInertiaTensor();

		//角惯性张量
		void SetInertiaTensorRotation(const Vector3& value);
		const Vector3& GetInertiaTensorRotation();

		//质心相对于刚体的位置
		void SetLocalCenterofMass(const Vector3& value);
		const Vector3& GetLocalCenterofMass();

		//弃用
		void SetLocalCenterofMass(const Vector3& value, bool IsLock) {  }
		void UnlockLocalCenterofMass() {}

		//设置质心计算模式
		void SetCenterOfMassMode(int value);//enum EchoPhysics::CenterOfMassMode
		int GetCenterOfMassMode();

		//设置质心计算偏移
		void SetCenterOfMassOffset(const Vector3& value);
		const Vector3& GetCenterOfMassOffset();

		//质心的世界位置
		void SetWorldCenterofMass(const Vector3& value);
		Vector3 GetWorldCenterofMass();

		//重置质心和惯性张量
		void ResetMassAndInertiaTensor(bool bSync = false);
		bool IsWaitResetMassAndInertiaTensor() const { return m_IsUpdateMAIT; }

		//睡眠状态
		void SetIsSleep(bool value);
		bool GetIsSleep();
		
		//唤醒刚体
		void WakeUp() { SetIsSleep(false); }
		//强制刚体睡眠
		void PutToSleep() { SetIsSleep(true); }

		//施加力和扭矩
		void AddForceAtPosition(const Vector3& force, const DVector3& position, int mode = EchoPhysics::FORCE);
		void AddForce(const Vector3& force, int mode = EchoPhysics::FORCE);
		void AddRelativeForce(const Vector3& force, int mode = EchoPhysics::FORCE);
		void AddTorque(const Vector3& torque, int mode = EchoPhysics::FORCE);
		void AddRelativeTorque(const Vector3& torque, int mode = EchoPhysics::FORCE);
		void ClearForce(int mode = EchoPhysics::FORCE);
		void ClearTorque(int mode = EchoPhysics::FORCE);

		//移动Kinematic刚体，如果IsKinematic为false则不会有效果
		bool MoveWorldPosition(const DVector3& position);
		bool MoveWorldRotation(const Quaternion& rotation);
		bool MoveWorldTransform(const DTransform& transform);
		bool MovePosition(const DVector3& position);
		bool MoveRotation(const Quaternion& rotation);
		bool MoveTransform(const DTransform& transform);

		//由物理组件发起更新，可以减少更新消耗
		void SetWorldTransformPhysX(const DTransform& trans);

		//位置迭代次数
		void SetSolverIterations(uint32 value);
		uint32 GetSolverIterations();

		//速度迭代次数
		void SetSolverVelocityIterations(uint32 value);
		uint32 GetSolverVelocityIterations();

		//刚体从重叠状态恢复的最大速度
		void SetMaxDepenetrationVelocity(float value);
		float GetMaxDepenetrationVelocity();

		//刚体角速度最大值
		void SetMaxAngularVelocity(float value);
		float GetMaxAngularVelocity();

		//刚体线性速度最大值
		void SetMaxLinearVelocity(float value);
		float GetMaxLinearVelocity();

		//刚体所受冲量最大值
		void SetMaxContactImpulse(float value);
		float GetMaxContactImpulse();

		//刚体进入睡眠状态的动能阈值
		void SetSleepThreshold(float value);
		float GetSleepThreshold();

		//设置密度，根据Collider体积计算质量并SetMass
		bool SetDensity(float density);

		//已废弃
		void SetIsCheckCollider(bool value) {}
		bool GetIsCheckCollider() { return true; }

		//扫描测试
		bool SweepTest(const Vector3& direction, EchoRaycastHit& hitInfo, float maxDistance);
		bool SweepTest(const Vector3& direction, std::vector<EchoRaycastHit>& hitInfo, float maxDistance);

		RigidBodyComponentData& GetCompData();
		void SetCompData(RigidBodyComponentData* pData);

		void AddRigidBodyCallBack(const EchoRigidBodyCallBackPtr& pComp, int priority = 0);
		void RemoveRigidBodyCallBack(const EchoRigidBodyCallBackPtr& pComp);

		//自动更新运动学目标
		bool GetIsAutoUpdateKinematicTarget() const {return m_IsAutoUpdateKinematicTarget; }
		void SetIsAutoUpdateKinematicTarget(bool v) { m_IsAutoUpdateKinematicTarget = v; }
	//=================================================用户调用end===========================================================
	private:
		void _GetVelocity(Vector3& value);
		void _GetAngularVelocity(Vector3& value);
		void _SetSolverIterationCounts();
		void _GetSolverIterationCounts();
		virtual PhysicsActor* _getActorPhysX() override;
		void UpdatePhysXPage();

		void AttachShapeInternal(PhysXShape* pShape);
		void DetachShapeInternal(PhysXShape* pShape);
		void UpdatePhysicsLocalAABB();
		void ResetMassAndInertiaTensorImpl();

		void CreatePhysXObject();
		void ReleasePhysXObject();

		bool ExistRigidBodyCallBack() { return !mListenerList.empty(); }
		void SendContactModify(const std::vector<EchoContactModifyPair>& pair);
		void SendCCDContactModify(const std::vector<EchoContactModifyPair>& pair);
		void SendWakeEvent();
		void SendSleepEvent();
		void SendCreateEvent();
		void SendReleaseEvent();
		void ChangeFilterGroup();
		uint32 GetShapeSceneFilterShaderFlags();
		Vector3& GetModeForceOrTorque(int mode, bool bForce);
		void ApplyForceAndTorque();
		void SetIsKinematicImpl(const DTransform& trans, bool bMove);
	private:
		PhysXRigidDynamic* m_pRigidBody = nullptr;
		std::unordered_set<ColliderComponentPtr> m_ColliderList;
		RigidBodyComponentData mCompData;
		bool mPhysicsLocalAABBChanged : 1 = true;
		bool m_IsUpdateMAIT : 1 = false;
		bool m_IsKinematicTarget : 1 = false;
		bool m_IsAutoUpdateKinematicTarget : 1 = false;
		//位置迭代次数
		uint32 mSolverPositionIterations = 4;
		//速度迭代次数
		uint32 mSolverVelocityIterations = 1;
		//刚体从重叠状态恢复的最大速度
		float mMaxDepenetrationVelocity = 1e+32f;
		//刚体角速度最大值
		float mMaxAngularVelocity = 100.0f;
		//刚体线性速度最大值
		float mMaxLinearVelocity = 1e+16f;
		//刚体所受冲量最大值
		float mMaxContactImpulse = 1e+32f;
		//刚体进入睡眠状态的动能阈值
		float mSleepThreshold = 0.005f;
		//线性速度
		Vector3 mVelocity = Vector3::ZERO;
		//角速度
		Vector3 mAngularVelocity = Vector3::ZERO;
		//力
		struct ForceAndTorqueData {
			Vector3 mSpatialLinearForce = Vector3::ZERO;
			Vector3 mSpatialAngularForce = Vector3::ZERO;
			Vector3 mSpatialLinearImpulse = Vector3::ZERO;
			Vector3 mSpatialAngularImpulse = Vector3::ZERO;
			Vector3 mSpatialLinearAcceleration = Vector3::ZERO;
			Vector3 mSpatialAngularAcceleration = Vector3::ZERO;
			Vector3 mSpatialLinearVelocity = Vector3::ZERO;
			Vector3 mSpatialAngularVelocity = Vector3::ZERO;
		};
		ForceAndTorqueData* m_pForceAndTorque = nullptr;
		AxisAlignedBox mPhysicsLocalAABB = AxisAlignedBox(-0.5f, -0.5f, -0.5f, 0.5f, 0.5f, 0.5f);
		std::vector<std::pair<int,EchoRigidBodyCallBackPtr>> mListenerList;
		RigidBodyComponentPtr mParentRigidBodyPtr = nullptr;
	private:
		friend class EchoPhysicsSystem;
		friend class EchoPhysicsManager;
		friend class ColliderComponent;
		friend class NodeColliderComponent;
		friend class WheelColliderComponent;
		friend class JointComponent;
		friend class EchoPhysXDynamicObject;
		friend class RigidBodyGroupComponent;
		friend class EchoPhysicsSceneCallBack;
	};

	class _EchoComponentExport RigidBodyComponentInfo : public ActorComponentInfo {
	public:
		RigidBodyComponentInfo() {}
		RigidBodyComponentInfo(const Name& comType, ActorCOMMoveableType MoveType);
		virtual ~RigidBodyComponentInfo();
		virtual void CopyToCompData(RigidBodyComponentData* pData);
		_DECLCOMINFO_VIRFUN
	public:
		_DECLCOMINFO_PARAM_BEGIN(RigidBodyComponent)
		_DECLCOMINFO_PARAM_SIM(bool, UseGravity, false, SetUseGravity);
		_DECLCOMINFO_PARAM_SIM(bool, IsKinematic, false, SetIsKinematic);
		_DECLCOMINFO_PARAM_SIM(int, CollisionDetection, EchoPhysics::Discrete, SetCollisionDetection);
		_DECLCOMINFO_PARAM_SIM(int, Constraints, EchoPhysics::FreezeNone, SetConstraints);
		_DECLCOMINFO_PARAM_SIM(float, Mass, 1.0f, SetMass);
		_DECLCOMINFO_PARAM_SIM(float, Drag, 0.0f, SetDrag);
		_DECLCOMINFO_PARAM_SIM(float, AngularDrag, 0.05f, SetAngularDrag);
		_DECLCOMINFO_PARAM_SIM(int, CenterOfMassMode, EchoPhysics::AutoMode, SetCenterOfMassMode);
		_DECLCOMINFO_PARAM(Vector3, CenterOfMassOffset, Vector3::ZERO, SetCenterOfMassOffset)
		_DECLCOMINFO_PARAM(Vector3, LocalCenterofMass, Vector3::ZERO, SetLocalCenterofMass)
		_DECLCOMINFO_PARAM(Vector3, InertiaTensor, Vector3::UNIT_SCALE, SetInertiaTensor)
		_DECLCOMINFO_PARAM(Vector3, InertiaTensorRotation, Vector3::ZERO, SetInertiaTensorRotation)
		_DECLCOMINFO_PARAM_END()
	};

	DEFINECOMFACTORY(RigidBodyComponent, UNMOVEABLE)


	class EchoRigidBodyScript {

	public:
		REGISTER_LUA_CLASS_KEY;
		static RigidBodyComponent* Script_GetRigidBodyComponent(Actor* pActor, const std::string& name);

	};

}



#endif /*__EchoRigidBodyComponent_h__*/
