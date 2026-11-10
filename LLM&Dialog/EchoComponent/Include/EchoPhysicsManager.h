//-----------------------------------------------------------------------------
// File:   EchoPhysicsManager.h
//
// Author: qiao_yuzhi
//
// Date:   2022-7-27
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef __EchoPhysicsManager_h__ 
#define __EchoPhysicsManager_h__

#include "EchoComponentPrerequisites.h"
#include "EchoProtocolComponent.h"
#include "Actor/EchoActor.h"
#include "Actor/EchoActorComponent.h"

#include "EchoHelperObj.h"
#include "EchoHelperObjProFuncImp.h"
#include "Actor/EchoProtocolActorComponent.h"
#include "EchoPhysicsCore.h"
#include "EchoPhysicsPage.h"
#include "EchoDVector3.h"
#include "EchoCoreSystemBase.h"

//#ifndef ECHO_EDITOR
//#define USE_PHYSICS_PAGE
//#endif // !ECHO_EDITOR
//
//#ifdef ECHO_EDITOR
//#define USE_PHYSICS_HELPEROBJ
//#endif // ECHO_EDITOR

#ifndef ECHO_EDITOR
#define PHYSICS_RUNTIME
#endif // !ECHO_EDITOR

#define USE_PHYSICS_PAGE
#define USE_PHYSICS_HELPEROBJ

namespace Echo {

	//EchoPhysics
	class PhysXSceneQuery;
	class PhysXSceneImpl;
	class PhysXStatic;
	class PhysXRigidDynamic;
	struct EchoGeometry;
	class PhysXShape;
	class PhysXJoint;
	class EchoPhysicsSceneCallBack;
	class PhysXControllerCallBack;
	class PhysXMathControllerCallBack;
	class PhysXCharacterController;
	class PhysXMathCharacterController;
	class PhysicsActor;
	class PhysXConvexMesh;
	class PhysXAggregate;
	class PhysXContactSet;

	class PhysXWorldManager;
	class EchoPhysicsManager;
	class MathCharacterController;

	//EchoComponent
	class PhysicsBaseComponent; typedef EchoWeakPtr<PhysicsBaseComponent> PhysicsBaseComponentPtr;
	class PhysicsActorComponent; typedef EchoWeakPtr<PhysicsActorComponent> PhysicsActorComponentPtr;
	class PhysicsLinkComponent; typedef EchoWeakPtr<PhysicsLinkComponent> PhysicsLinkComponentPtr;
	class RigidBodyComponent; typedef EchoWeakPtr<RigidBodyComponent> RigidBodyComponentPtr;
	class ColliderComponent; typedef EchoWeakPtr<ColliderComponent> ColliderComponentPtr;
	class NodeColliderComponent; typedef EchoWeakPtr<NodeColliderComponent> NodeColliderComponentPtr;
	class BoxColliderComponent; typedef EchoWeakPtr<BoxColliderComponent> BoxColliderComponentPtr;
	class CapsuleColliderComponent; typedef EchoWeakPtr<CapsuleColliderComponent> CapsuleColliderComponentPtr;
	class MeshColliderComponent; typedef EchoWeakPtr<MeshColliderComponent> MeshColliderComponentPtr;
	class MeshColliderUNMOVEABLEComponent; typedef EchoWeakPtr<MeshColliderUNMOVEABLEComponent> MeshColliderUNMOVEABLEComponentPtr;
	class SphereColliderComponent; typedef EchoWeakPtr<SphereColliderComponent> SphereColliderComponentPtr;
	class TerrainColliderComponent; typedef EchoWeakPtr<TerrainColliderComponent> TerrainColliderComponentPtr;
	class JointComponent; typedef EchoWeakPtr<JointComponent> JointComponentPtr;
	class FixedJointComponent; typedef EchoWeakPtr<FixedJointComponent> FixedJointComponentPtr;
	class ConfigurableJointComponent; typedef EchoWeakPtr<ConfigurableJointComponent> ConfigurableJointComponentPtr;
	class AggregateComponent; typedef EchoWeakPtr<AggregateComponent> AggregateComponentPtr;
	class ConstantForceComponent; typedef EchoWeakPtr<ConstantForceComponent> ConstantForceComponentPtr;
	class CharatorControllerComponent; typedef EchoWeakPtr<CharatorControllerComponent> CharatorControllerComponentPtr;
	class RagdollComponent; typedef EchoWeakPtr<RagdollComponent> RagdollComponentPtr;

	class GeneratedTerrainPhysics;

	//CallBack
	class EchoPhysXWorldSystemCallBack;
	class EchoColliderCallBack; typedef EchoColliderCallBack* EchoColliderCallBackPtr;
	class EchoRigidBodyCallBack; typedef EchoRigidBodyCallBack* EchoRigidBodyCallBackPtr;
	class EchoControllerCallBack; typedef EchoControllerCallBack* EchoControllerCallBackPtr;
	class EchoJointCallBack; typedef EchoJointCallBack* EchoJointCallBackPtr;
	class EchoPhysicsManagerCallBack; typedef EchoPhysicsManagerCallBack* EchoPhysicsManagerCallBackPtr;

	using EchoSceneQueryFilter = std::function<bool(RigidBodyComponent*, ColliderComponent*)>;
	typedef std::vector<PhysicsLinkComponentPtr> PhysicsLinkComponentPtrList;
	class EchoPhysicsPageGraph;

	enum class EchoPhysicsComponentType : int {
		eCOLLIDER = 0,
		eTERRAIN,
		eRIGIDBODY,
		eCONTROLLER,
		eFORCE,
		eJOINT,
		eAGGREGATE,
		eNUMBER
	};

	class _EchoComponentExport EchoPhysicsManagerCallBack
	{
	public:
		EchoPhysicsManagerCallBack() {}
		virtual ~EchoPhysicsManagerCallBack() {}
		virtual void OnCreate(const RigidBodyComponentPtr& _compPtr) {}
		virtual void OnRelease(const RigidBodyComponentPtr& _compPtr) {}
		virtual void OnWake(const RigidBodyComponentPtr& _compPtr) {}
		virtual void OnSleep(const RigidBodyComponentPtr& _compPtr) {}
	};

	struct EchoContactPointParam
	{
		float separation = 0.0f;	//分离距离，负数表示穿透。
		DVector3 position;			//世界位置
		Vector3 normal;				//法线，指向self
		Vector3 impulse;			//冲量，指向self
	};

	struct EchoTriggerParam {
		RigidBodyComponent* self_rigidbody = nullptr;
		ColliderComponent* self_collider = nullptr;
		RigidBodyComponent* rigidbody = nullptr;
		ColliderComponent* collider = nullptr;
	};

	struct EchoCollisionParam {
		RigidBodyComponent* self_rigidbody = nullptr;
		ColliderComponent* self_collider = nullptr;
		RigidBodyComponent* rigidbody = nullptr;
		ColliderComponent* collider = nullptr;
		std::vector<EchoContactPointParam> points;
	};

	enum class EchoControllerBehaviorFlag
	{
		eCCT_CAN_RIDE_ON_OBJECT = (1 << 0),
		eCCT_SLIDE = (1 << 1),
		eCCT_USER_DEFINED_RIDE = (1 << 2)
	};

	struct EchoControllerHitParam
	{
		bool IsStanding = false;
		float motionLen = 0.f;
		DVector3 pos;
		Vector3 normal;
		Vector3 motionDir;
	};

	struct EchoControllerShapeHitParam : public EchoControllerHitParam
	{
		CharatorControllerComponent* controller = nullptr;
		RigidBodyComponent* actor = nullptr;
		ColliderComponent* shape = nullptr;
	};

	struct EchoMathControllerShapeParam 
	{
		MathCharacterController* controller = nullptr;
		RigidBodyComponent* actor = nullptr;
		ColliderComponent* shape = nullptr;
	};

	struct EchoMathControllerShapeHitParam : public EchoControllerHitParam, public EchoMathControllerShapeParam {};

	struct EchoControllersHitParam : public EchoControllerHitParam {
		CharatorControllerComponent* controller = nullptr;
		CharatorControllerComponent* other = nullptr;
	};

	struct EchoControllerBehaviorParam {
		CharatorControllerComponent* controller = nullptr;
	};

	struct EchoControllerShapeBehaviorParam : public EchoControllerBehaviorParam {
		RigidBodyComponent* actor = nullptr;
		ColliderComponent* shape = nullptr;
	};

	struct EchoControllersBehaviorParam : public EchoControllerBehaviorParam {
		CharatorControllerComponent* other = nullptr;
	};

	struct EchoMathControllerShapeBehaviorParam {
		MathCharacterController* controller = nullptr;
		RigidBodyComponent* actor = nullptr;
		ColliderComponent* shape = nullptr;
	};

	/*
		Collider碰撞回调：
		如果两个Collider都是Trigger类型，则不会产生任何回调。
		如果其中一个Collider是Trigger类型，则两个Collider都会调用OnTriggerEnter、OnTriggerExit、OnTriggerStay
		如果两个Collider都不是Trigger类型，则两个Collider都会调用OnCollisionEnter、OnCollisionExit、OnCollisionStay
	*/
	class _EchoComponentExport EchoColliderCallBack
	{
	public:
		EchoColliderCallBack() {}
		virtual ~EchoColliderCallBack() {}
	public:
		virtual void OnCollisionEnter(const EchoCollisionParam& param) {}
		virtual void OnCollisionExit(const EchoCollisionParam& param) {}
		virtual void OnCollisionStay(const EchoCollisionParam& param) {}

		virtual void OnTriggerEnter(const EchoTriggerParam& param) {}
		virtual void OnTriggerExit(const EchoTriggerParam& param) {}
		virtual void OnTriggerStay(const EchoTriggerParam& param) {}//未使用

		virtual void OnCollisionCCT(const EchoControllerShapeHitParam& param) {}
		virtual void OnCollisionCCT(const EchoMathControllerShapeHitParam& param) {}
	};


	class _EchoComponentExport EchoContactSet
	{
	public:
		EchoContactSet() {}
		~EchoContactSet() {}
		DVector3 getPoint(uint32 i) const;
		void setPoint(uint32 i, const DVector3& p) const;
		Vector3 getNormal(uint32 i) const;
		void setNormal(uint32 i, const Vector3& n) const;
		float getSeparation(uint32 i) const;
		void setSeparation(uint32 i, float s) const;
		Vector3 getTargetVelocity(uint32 i) const;
		void setTargetVelocity(uint32 i, const Vector3& v) const;
		float getMaxImpulse(uint32 i) const;
		void setMaxImpulse(uint32 i, float s) const;
		float getRestitution(uint32 i) const;
		void setRestitution(uint32 i, float r) const;
		float getStaticFriction(uint32 i) const;
		void setStaticFriction(uint32 i, float f) const;
		float getDynamicFriction(uint32 i) const;
		void setDynamicFriction(uint32 i, float f) const;
		void ignore(uint32 i) const;
		uint32 size() const;
		bool isInverse() const { return bInver; }
	private:
		mutable PhysXContactSet* m_pContactSet = nullptr;
		bool bInver = false;
		friend class EchoPhysicsSceneCallBack;
	};

	struct EchoContactModifyPair {
		RigidBodyComponent* rigidbody[2] = { nullptr,nullptr };
		ColliderComponent* collider[2] = { nullptr,nullptr };
		DTransform transform[2];
		EchoContactSet contacts;
	};

	/*
		在物理模拟中调用，回调中禁止对rigidbody和collider做任何修改。
	*/
	class _EchoComponentExport EchoRigidBodyCallBack {
	public:
		EchoRigidBodyCallBack() {}
		virtual ~EchoRigidBodyCallBack() {}
	public:
		virtual void OnCreate(RigidBodyComponent*) {}
		virtual void OnRelease(RigidBodyComponent*) {}
		virtual void OnWake(RigidBodyComponent*) {}
		virtual void OnSleep(RigidBodyComponent*) {}
		virtual void OnContactModify(const std::vector<EchoContactModifyPair>& pair) {}
		virtual void OnCCDContactModify(const std::vector<EchoContactModifyPair>& pair) {}
	};

	class _EchoComponentExport EchoControllerCallBack {
	public:
		EchoControllerCallBack() {}
		virtual ~EchoControllerCallBack() {}

		//返回是否缓存是否失效
		virtual bool OnShapeHit(const EchoControllerShapeHitParam& param) { return false; };

		//返回是否缓存是否失效
		virtual bool OnControllerHit(const  EchoControllersHitParam& param) { return false; };

		//返回控制器行为
		virtual  EchoControllerBehaviorFlag GetBehaviorFlag(const EchoControllerShapeBehaviorParam& param) { return EchoControllerBehaviorFlag::eCCT_USER_DEFINED_RIDE; }

		//返回控制器行为
		virtual EchoControllerBehaviorFlag GetBehaviorFlag(const EchoControllersBehaviorParam& param) { return EchoControllerBehaviorFlag::eCCT_USER_DEFINED_RIDE; }
	};

	class _EchoComponentExport EchoMathControllerCallBack {
	public:
		EchoMathControllerCallBack() {}
		virtual ~EchoMathControllerCallBack() {}

		virtual void OnShapeHit(const EchoMathControllerShapeHitParam& param) {  };

		//多线程调用
		virtual bool OnFilterShape(const EchoMathControllerShapeParam& param) { return true; }
		//多线程调用
		virtual  EchoControllerBehaviorFlag GetBehaviorFlag(const EchoMathControllerShapeBehaviorParam& param) { return EchoControllerBehaviorFlag::eCCT_USER_DEFINED_RIDE; }
	};

	class _EchoComponentExport EchoJointCallBack
	{
	public:
		EchoJointCallBack() {}
		virtual ~EchoJointCallBack() {}

		virtual void OnJointBreak() {}
	};

	struct EchoSceneQueryFlag {
		enum Enum : uint32 {
			eNONE = 0,
			ePOSITION = (1 << 0),
			eNORMAL = (1 << 1),
			eASSUME_NO_INITIAL_OVERLAP = (1 << 4),
			eMESH_MULTIPLE = (1 << 5),
			eMESH_ANY = (1 << 6),
			eMESH_BOTH_SIDES = (1 << 7),
			ePRECISE_SWEEP = (1 << 8),

			eSTATIC = (1 << 16),
			eDYNAMIC = (1 << 17),
			eANY_HIT = (1 << 20),

			eSURFACE_TYPE = (1 << 24),

			eALLHITDATA = ePOSITION | eNORMAL | eSURFACE_TYPE,
			eDEFAULT_NONE = eSTATIC | eDYNAMIC,// | eANY_HIT,// | eMESH_ANY,
			eDEFAULT_HIT = eSTATIC | eDYNAMIC | ePOSITION | eNORMAL | eSURFACE_TYPE
		};
	};
	typedef uint32 EchoSceneQueryFlags;

	struct EchoSceneQueryHit {
		EchoSceneQueryHit() {}
		EchoSceneQueryHit(RigidBodyComponent* _rigidbody, ColliderComponent* _collider) : rigidbody(_rigidbody), collider(_collider) {}
		// The Rigidbody of the collider that was hit. If the collider is not attached to a rigidbody then it is null.
		RigidBodyComponent* rigidbody = nullptr;
		//The Collider that was hit.
		ColliderComponent* collider = nullptr;
	};

	struct EchoRaycastHit : public EchoSceneQueryHit
	{
		EchoRaycastHit() : EchoSceneQueryHit() {}
		EchoRaycastHit(const EchoSceneQueryHit& hit) : EchoSceneQueryHit(hit) {}
		EchoRaycastHit(RigidBodyComponent* _rigidbody, ColliderComponent* _collider) : EchoSceneQueryHit(_rigidbody, _collider) {}
		//The distance from the ray's origin to the impact point.
		float distance = 0;
		//The surface type of the impact point of this collider
		uint32 surfaceType = 0;
		//The normal of the surface the ray hit.
		Vector3 normal = Vector3::ZERO;
		//The impact point in world space where the ray hit the collider.
		DVector3 point = DVector3::ZERO;
	};
	
	class _EchoComponentExport EchoPhysicsQuery {
	public:
		//Raycast
		bool Raycast(const DVector3& origin, const Vector3& direction, float maxDistance, PhysXGroup layerMask, const EchoSceneQueryFilter& filter = nullptr, EchoSceneQueryFlags flags = EchoSceneQueryFlag::eDEFAULT_NONE);
		bool Raycast(const DVector3& origin, const Vector3& direction, EchoRaycastHit& hit, float maxDistance, PhysXGroup layerMask, const EchoSceneQueryFilter& filter = nullptr, EchoSceneQueryFlags flags = EchoSceneQueryFlag::eDEFAULT_HIT);
		bool Raycast(const DVector3& origin, const Vector3& direction, std::vector<EchoRaycastHit>& hits, float maxDistance, PhysXGroup layerMask, const EchoSceneQueryFilter& filter = nullptr, EchoSceneQueryFlags flags = EchoSceneQueryFlag::eDEFAULT_HIT);

		//Sweep
		bool GeometryCast(const EchoGeometry* geometry, const DTransform& pose, const Vector3& direction, float maxDistance, PhysXGroup layerMask, const EchoSceneQueryFilter& filter = nullptr, EchoSceneQueryFlags flags = EchoSceneQueryFlag::eDEFAULT_NONE);
		bool GeometryCast(const EchoGeometry* geometry, const DTransform& pose, const Vector3& direction, EchoRaycastHit& hit, float maxDistance, PhysXGroup layerMask, const EchoSceneQueryFilter& filter = nullptr, EchoSceneQueryFlags flags = EchoSceneQueryFlag::eDEFAULT_HIT);
		bool GeometryCast(const EchoGeometry* geometry, const DTransform& pose, const Vector3& direction, std::vector<EchoRaycastHit>& hits, float maxDistance, PhysXGroup layerMask, const EchoSceneQueryFilter& filter = nullptr, EchoSceneQueryFlags flags = EchoSceneQueryFlag::eDEFAULT_HIT);

		bool SphereCast(const DTransform& pose, const Vector3& direction, float radius, float maxDistance, PhysXGroup layerMask, const EchoSceneQueryFilter& filter = nullptr, EchoSceneQueryFlags flags = EchoSceneQueryFlag::eDEFAULT_NONE);
		bool SphereCast(const DTransform& pose, const Vector3& direction, EchoRaycastHit& hit, float radius, float maxDistance, PhysXGroup layerMask, const EchoSceneQueryFilter& filter = nullptr, EchoSceneQueryFlags flags = EchoSceneQueryFlag::eDEFAULT_HIT);
		bool SphereCast(const DTransform& pose, const Vector3& direction, std::vector<EchoRaycastHit>& hits, float radius, float maxDistance, PhysXGroup layerMask, const EchoSceneQueryFilter& filter = nullptr, EchoSceneQueryFlags flags = EchoSceneQueryFlag::eDEFAULT_HIT);

		bool BoxCast(const DTransform& pose, const Vector3& direction, const Vector3& halfSize, float maxDistance, PhysXGroup layerMask, const EchoSceneQueryFilter& filter = nullptr, EchoSceneQueryFlags flags = EchoSceneQueryFlag::eDEFAULT_NONE);
		bool BoxCast(const DTransform& pose, const Vector3& direction, EchoRaycastHit& hit, const Vector3& halfSize, float maxDistance, PhysXGroup layerMask, const EchoSceneQueryFilter& filter = nullptr, EchoSceneQueryFlags flags = EchoSceneQueryFlag::eDEFAULT_HIT);
		bool BoxCast(const DTransform& pose, const Vector3& direction, std::vector<EchoRaycastHit>& hits, const Vector3& halfSize, float maxDistance, PhysXGroup layerMask, const EchoSceneQueryFilter& filter = nullptr, EchoSceneQueryFlags flags = EchoSceneQueryFlag::eDEFAULT_HIT);

		bool CapsuleCast(const DTransform& pose, const Vector3& direction, float radius, float middleHeight, float maxDistance, PhysXGroup layerMask, const EchoSceneQueryFilter& filter = nullptr, EchoSceneQueryFlags flags = EchoSceneQueryFlag::eDEFAULT_NONE);
		bool CapsuleCast(const DTransform& pose, const Vector3& direction, EchoRaycastHit& hit, float radius, float middleHeight, float maxDistance, PhysXGroup layerMask, const EchoSceneQueryFilter& filter = nullptr, EchoSceneQueryFlags flags = EchoSceneQueryFlag::eDEFAULT_HIT);
		bool CapsuleCast(const DTransform& pose, const Vector3& direction, std::vector<EchoRaycastHit>& hits, float radius, float middleHeight, float maxDistance, PhysXGroup layerMask, const EchoSceneQueryFilter& filter = nullptr, EchoSceneQueryFlags flags = EchoSceneQueryFlag::eDEFAULT_HIT);

		bool ColliderCast(ColliderComponent* collider, const Vector3& direction, float maxDistance, const EchoSceneQueryFilter& filter = nullptr, EchoSceneQueryFlags flags = EchoSceneQueryFlag::eDEFAULT_NONE);
		bool ColliderCast(ColliderComponent* collider, const Vector3& direction, EchoRaycastHit& hit, float maxDistance, const EchoSceneQueryFilter& filter = nullptr, EchoSceneQueryFlags flags = EchoSceneQueryFlag::eDEFAULT_HIT);
		bool ColliderCast(ColliderComponent* collider, const Vector3& direction, std::vector<EchoRaycastHit>& hits, float maxDistance, const EchoSceneQueryFilter& filter = nullptr, EchoSceneQueryFlags flags = EchoSceneQueryFlag::eDEFAULT_HIT);

		//Overlap
		bool OverlapGeometry(const EchoGeometry* geometry, const DTransform& pose, PhysXGroup layerMask, const EchoSceneQueryFilter& filter = nullptr, EchoSceneQueryFlags flags = EchoSceneQueryFlag::eDEFAULT_NONE);
		bool OverlapGeometry(const EchoGeometry* geometry, const DTransform& pose, EchoSceneQueryHit& hit, PhysXGroup layerMask, const EchoSceneQueryFilter& filter = nullptr, EchoSceneQueryFlags flags = EchoSceneQueryFlag::eDEFAULT_HIT);
		bool OverlapGeometry(const EchoGeometry* geometry, const DTransform& pose, std::vector<EchoSceneQueryHit>& hits, PhysXGroup layerMask, const EchoSceneQueryFilter& filter = nullptr, EchoSceneQueryFlags flags = EchoSceneQueryFlag::eDEFAULT_HIT);

		bool OverlapSphere(const DTransform& pose, float radius, PhysXGroup layerMask, const EchoSceneQueryFilter& filter = nullptr, EchoSceneQueryFlags flags = EchoSceneQueryFlag::eDEFAULT_NONE);
		bool OverlapSphere(const DTransform& pose, EchoSceneQueryHit& hit, float radius, PhysXGroup layerMask, const EchoSceneQueryFilter& filter = nullptr, EchoSceneQueryFlags flags = EchoSceneQueryFlag::eDEFAULT_HIT);
		bool OverlapSphere(const DTransform& pose, std::vector<EchoSceneQueryHit>& hits, float radius, PhysXGroup layerMask, const EchoSceneQueryFilter& filter = nullptr, EchoSceneQueryFlags flags = EchoSceneQueryFlag::eDEFAULT_HIT);

		bool OverlapBox(const DTransform& pose, const Vector3& halfSize, PhysXGroup layerMask, const EchoSceneQueryFilter& filter = nullptr, EchoSceneQueryFlags flags = EchoSceneQueryFlag::eDEFAULT_NONE);
		bool OverlapBox(const DTransform& pose, EchoSceneQueryHit& hit, const Vector3& halfSize, PhysXGroup layerMask, const EchoSceneQueryFilter& filter = nullptr, EchoSceneQueryFlags flags = EchoSceneQueryFlag::eDEFAULT_HIT);
		bool OverlapBox(const DTransform& pose, std::vector<EchoSceneQueryHit>& hits, const Vector3& halfSize, PhysXGroup layerMask, const EchoSceneQueryFilter& filter = nullptr, EchoSceneQueryFlags flags = EchoSceneQueryFlag::eDEFAULT_HIT);

		bool OverlapCapsule(const DTransform& pose, float radius, float middleHeight, PhysXGroup layerMask, const EchoSceneQueryFilter& filter = nullptr, EchoSceneQueryFlags flags = EchoSceneQueryFlag::eDEFAULT_NONE);
		bool OverlapCapsule(const DTransform& pose, EchoSceneQueryHit& hit, float radius, float middleHeight, PhysXGroup layerMask, const EchoSceneQueryFilter& filter = nullptr, EchoSceneQueryFlags flags = EchoSceneQueryFlag::eDEFAULT_HIT);
		bool OverlapCapsule(const DTransform& pose, std::vector<EchoSceneQueryHit>& hits, float radius, float middleHeight, PhysXGroup layerMask, const EchoSceneQueryFilter& filter = nullptr, EchoSceneQueryFlags flags = EchoSceneQueryFlag::eDEFAULT_HIT);

		bool OverlapCollider(ColliderComponent* collider, const EchoSceneQueryFilter& filter = nullptr, EchoSceneQueryFlags flags = EchoSceneQueryFlag::eDEFAULT_NONE);
		bool OverlapCollider(ColliderComponent* collider, EchoSceneQueryHit& hit, const EchoSceneQueryFilter& filter = nullptr, EchoSceneQueryFlags flags = EchoSceneQueryFlag::eDEFAULT_HIT);
		bool OverlapCollider(ColliderComponent* collider, std::vector<EchoSceneQueryHit>& hits, const EchoSceneQueryFilter& filter = nullptr, EchoSceneQueryFlags flags = EchoSceneQueryFlag::eDEFAULT_HIT);

		virtual PhysXSceneQuery* GetPhysXSceneQuery() const = 0;
	};

	class CommonQuery;
	class _EchoComponentExport EchoCommonQuery {
	public:
		enum Enum {
			eNONE,//不计算结果
			eSINGLE,//单一结果
			eMULTIPLE,//全部结果
		};
	public:
		//Raycast
		EchoCommonQuery(Enum hitType, const DVector3& origin, const Vector3& direction, float maxDistance, PhysXGroup layerMask, const EchoSceneQueryFilter& filter = nullptr, EchoSceneQueryFlags flags = EchoSceneQueryFlag::eDEFAULT_NONE);
		//GeometryCast
		EchoCommonQuery(Enum hitType, const EchoGeometry* geometry, const DTransform& pose, const Vector3& direction, float maxDistance, PhysXGroup layerMask, const EchoSceneQueryFilter& filter = nullptr, EchoSceneQueryFlags flags = EchoSceneQueryFlag::eDEFAULT_NONE);
		//OverlapGeometry
		EchoCommonQuery(Enum hitType, const EchoGeometry* geometry, const DTransform& pose, PhysXGroup layerMask, const EchoSceneQueryFilter& filter = nullptr, EchoSceneQueryFlags flags = EchoSceneQueryFlag::eDEFAULT_NONE);
		virtual ~EchoCommonQuery();

		void Execute(EchoPhysicsQuery* pQuery);
		bool GetResult();
		const std::vector<EchoRaycastHit>& GetResults();
	public:
		CommonQuery* m_private = nullptr;
		friend class EchoQueryTask;
	};

	class PhysXQueryTask;
	class _EchoComponentExport EchoQueryTask {
	public:
		EchoQueryTask(EchoPhysicsQuery* pQuery, const std::vector<EchoCommonQuery*>& commands);
		virtual ~EchoQueryTask();
		void Execute();
		void Wait();
	private:
		PhysXQueryTask* m_private = nullptr;
	};

	class EchoBatchQueryPrivate;
	class _EchoComponentExport EchoBatchQuery {
	public:
		EchoBatchQuery(EchoPhysicsQuery* pQuery);
		~EchoBatchQuery();

		int Raycast(EchoCommonQuery::Enum type, const DVector3& origin, const Vector3& direction, float maxDistance, PhysXGroup layerMask, const EchoSceneQueryFilter& filter = nullptr, EchoSceneQueryFlags flags = EchoSceneQueryFlag::eDEFAULT_NONE);
		int GeometryCast(EchoCommonQuery::Enum type, const EchoGeometry* geometry, const DTransform& pose, const Vector3& direction, float maxDistance, PhysXGroup layerMask, const EchoSceneQueryFilter& filter = nullptr, EchoSceneQueryFlags flags = EchoSceneQueryFlag::eDEFAULT_NONE);
		int OverlapGeometry(EchoCommonQuery::Enum type, const EchoGeometry* geometry, const DTransform& pose, PhysXGroup layerMask, const EchoSceneQueryFilter& filter = nullptr, EchoSceneQueryFlags flags = EchoSceneQueryFlag::eDEFAULT_NONE);

		void Execute();
		void Wait();

		int GetResultNumber();
		bool GetResult(int index);
		const std::vector<EchoRaycastHit>& GetResults(int index);
	private:
		EchoBatchQueryPrivate* m_private = nullptr;
	};

#ifdef USE_PHYSICS_PAGE
	
	namespace EchoPhysics {

		template<typename GraphNodeT>
		class Graph {
		public:
			Graph() {}
			Graph(Graph<GraphNodeT>& graph);
			Graph(std::map<GraphNodeT*, std::list<GraphNodeT*>>& data) { mData.swap(data); }
			Graph(const std::vector<GraphNodeT*>& nodes);
			Graph(const std::vector<GraphNodeT*>& nodes, const std::vector<std::pair<GraphNodeT*, GraphNodeT*>>& edges, bool bTwoWay);
			Graph(const std::set<GraphNodeT*>& nodes, const std::vector<std::pair<GraphNodeT*, GraphNodeT*>>& edges);
			virtual ~Graph() { clear(); }
		public:
			void merge(Graph& graph);
			bool empty() { return mData.empty(); }
			void clear();
			void AddNode(GraphNodeT* pNode);
			bool RemoveNode(GraphNodeT* pNode);
			void AddEdge(GraphNodeT* pNode1, GraphNodeT* pNode2);
			bool RemoveEdge(GraphNodeT* pNode1, GraphNodeT* pNode2);
			bool Reset(std::vector<Graph*>& newGraph, std::vector<GraphNodeT*>& newNode);
		public:
			std::map<GraphNodeT*, std::list<GraphNodeT*>> mData;
		};

	}

	typedef EchoPhysics::Graph<EchoPhysXObject> EchoPhysXObjectGraph;

	class _EchoComponentExport EchoPhysXGroupObject : public EchoPhysXObject {
	public:
		EchoPhysXGroupObject(EchoPhysXObjectGraph* graph);
		virtual ~EchoPhysXGroupObject();
		virtual void PhysxStateChanged(PageLoadStateType type) override;
		virtual AxisAlignedBox getPhysXWorldAABB() override;

		void AddEdge(JointComponent*);
		void AddNode(EchoPhysXObject*);

		bool RemoveEdge(JointComponent*, std::vector<EchoPhysXObjectGraph*>& newGraphs, std::vector<EchoPhysXObject*>& newNodes);
		bool RemoveNode(EchoPhysXObject*, std::vector<EchoPhysXObjectGraph*>& newGraphs, std::vector<EchoPhysXObject*>& newNodes);
		EchoPhysXObjectGraph* m_pGraph = nullptr;
#ifndef PHYSICS_RUNTIME
		static std::unordered_set<EchoPhysXGroupObject*> gAllObject;
#endif // PHYSICS_RUNTIME
	};

	class _EchoComponentExport EchoPhysXActorObject : public EchoPhysXObject {
	public:
		EchoPhysXActorObject(EchoPhysXObjectType _type);
		virtual ~EchoPhysXActorObject() {}
		EchoPhysXGroupObject* GetGroup() { return pGroup; }
		void SetGroup(EchoPhysXGroupObject* _pGroup);
	protected:
		EchoPhysXGroupObject* pGroup = nullptr;
	};

	class _EchoComponentExport EchoPhysXStaticObject : public EchoPhysXActorObject {
	public:
		EchoPhysXStaticObject(ColliderComponent* _pComp);
		virtual ~EchoPhysXStaticObject();
		virtual void PhysxStateChanged(PageLoadStateType type) override;
		virtual AxisAlignedBox getPhysXWorldAABB() override;
		ColliderComponent* m_pComp = nullptr;
#ifndef PHYSICS_RUNTIME
		static std::unordered_set<EchoPhysXStaticObject*> gAllObject;
#endif // PHYSICS_RUNTIME
	};

	class _EchoComponentExport EchoPhysXDynamicObject : public EchoPhysXActorObject {
	public:
		EchoPhysXDynamicObject(RigidBodyComponent* _pComp);
		virtual ~EchoPhysXDynamicObject();
		virtual void PhysxStateChanged(PageLoadStateType type) override;
		virtual AxisAlignedBox getPhysXWorldAABB() override;
		RigidBodyComponent* m_pComp = nullptr;
#ifndef PHYSICS_RUNTIME
		static std::unordered_set<EchoPhysXDynamicObject*> gAllObject;
#endif // PHYSICS_RUNTIME
	};

#endif // USE_PHYSICS_PAGE


	struct _EchoComponentExport PhysXPageLoadPointData {
		PhysXPageLoadPointData() {}
		PhysXPageLoadPointData(const ActorComponentPtr& _compPtr,
			const Vector3& _position, 
			bool _forceAsync) 
			: m_pCompPtr(_compPtr), 
			m_vLastPosition(_position), 
			m_bForceAsync(_forceAsync) {}
		ActorComponentPtr m_pCompPtr = nullptr;
		Vector3 m_vLastPosition = Vector3::ZERO;
		bool m_bForceAsync = false;
	};

	class _EchoComponentExport PhysicsPagedLayerManager {
	public:
		PhysicsPagedLayerManager();
		virtual ~PhysicsPagedLayerManager();
		void setCellSize(float cellSize);
		float getCellSize();
		void setAsyncLoadWidth(float width);
		float getAsyncLoadWidth();
		int getPageSize() { return mPageSize; }
		const std::map<uint16, PhysicsPagedLayer*>& getMapPagedLayers();

		void setNeedUpdate(bool value) { m_bNeedUpdate = value; }
		void SetPageSize(float cellSize, unsigned int _FrameAsyncLoadNumber, float _AsyncLoadWidth, int _PageSize);
		void addTarget(const Vector3& position, bool bForceAsync);
		void setTarget(int index, const Vector3& position, bool bForceAsync);
		void removeTarget(int index);
		void clear();
		void update(const std::vector<PhysXPageLoadPointData>& targets);

		void updateEchoPhysXObject(EchoPhysXObject* pEchoPhysXObject);
		void addEchoPhysXObject(EchoPhysXObject* pEchoPhysXObject);
		void removeEchoPhysXObject(EchoPhysXObject* pEchoPhysXObject);

		void addEchoPhysXObject(uint16 depth, const EchoIndex3& index, EchoPhysXObject* pEchoPhysXObject);
		void deletePagedLayer(PhysicsPagedLayer* pLayer);
		PhysicsPagedLayer* createPagedLayer(uint16 depth);
		PhysicsPagedLayer* getPagedLayer(uint16 depth);
		EchoIndex3 worldAABBToIndex(const AxisAlignedBox& box, int& depth);

		int updateAsyncQueue(int maxLoadNumber);
		void addAsyncLoadPhysXObject(EchoPhysXObject*);
		void removeAsyncLoadPhysXObject(EchoPhysXObject*);
		void clearAsyncLoadPhysXObject();

		void SetPhysicsManager(EchoPhysicsManager* pMgr) { m_pManager = pMgr; }
		EchoPhysicsManager* GetPhysicsManager() { return m_pManager; }
	private:
		EchoPhysXObjectType mType = ePhysXStaticObjectType;
		EchoPhysicsManager* m_pManager = nullptr;

		std::vector<std::pair<Vector3,bool>> mLoadTargets;

		float m_cellSize = 256.0f;
		float m_fMinMoveDisSqure = 100.f;
		bool m_bNeedUpdate = false;
		float mAsyncLoadWidth = 0.2f;
		int mPageSize = 1;

		//PageSizeData mPageSizeData;
		std::map<uint16, PhysicsPagedLayer*> m_mapPagedLayers;

		int mFrameAsyncLoadNumber = 100;
		std::list<EchoPhysXObject*> mAsyncLoadPhysicsObjectQueue;
		friend class EchoPhysicsSystem;
		friend class EchoPhysicsManager;
		friend class PhysicsManagerUI;
	};


	class _EchoComponentExport PhysicsBaseComponent : public ActorComponent {
	public:
		PhysicsBaseComponent(ActorPtr pActor, ActorCOMMoveableType type);
		virtual ~PhysicsBaseComponent();
		virtual void connectCom() override;
		virtual void disconnectCom() override;
		virtual void init(ActorComponentInfo* info) override;
		virtual void unInit() override;
		virtual void visibleStateChanged(bool activeState) override;
	public:
		//创建物理
		virtual void CreatePhysics() = 0;
		//释放物理
		virtual void ReleasePhysics() = 0;
		//组件是否准备好创建物理
		virtual bool IsCanPhysics() = 0;
		//组件中是否存在物理
		virtual bool IsExistPhysics() = 0;
#ifdef USE_PHYSICS_HELPEROBJ
		//编辑器模式下更新组件
		virtual void UpdateComponentInEditor() = 0;
#endif // USE_PHYSICS_HELPEROBJ
	public:
		//引擎不会调用
		inline void SetUserData(void* pUserData) { m_pUserData = pUserData; }
		inline void* GetUserData() { return m_pUserData; }
		inline void SetUserType(int userType) { m_UserType = userType; }
		inline int GetUserType() { return m_UserType; }
	private:
		int m_UserType = 0;
		void* m_pUserData = nullptr;
	public:
		EchoPhysicsManager* GetPhysicsManager();
		inline bool GetIsPhysicsUpdate() const { return m_bUpdatePhysics; }
		inline bool GetIsEditorUpdate() const { return m_bUpdateEditor; }
		inline bool GetIsUsePhysics() const { return m_IsUsePhysics; }
		inline bool GetIsUsePhysXPage() const { return m_IsUsePhysXPage; }
	protected:
		virtual EchoPhysicsComponentType GetPhysicsType() { return EchoPhysicsComponentType::eNUMBER; }
		void AddToPhysicsUpdateQueue();
		void RemoveFromPhysicsUpdateQueue();
		void AddToEditorUpdateQueue();
		void RemoveFromEditorUpdateQueue();
	protected:
		bool m_bUpdatePhysics : 1 = false;
		bool m_bUpdateEditor : 1 = false;
		bool m_IsUsePhysics : 1 = false;
		bool m_IsUsePhysXPage : 1 = true;
		bool m_bPhysXTransformChanged : 1 = false;
		bool m_IsHelperVisible : 1 = false;
		bool m_IsUpdatePhysicsTransform : 1 = true;
		bool m_bWaitCreateGroup : 1 = false;
	public:
		ActorComponentPtr FindComponent(const String& compName);
		template<typename T>
		EchoWeakPtr<T> FindComponent(const String& compName) {
			return FindComponent(compName).getDynamicCastWeakPtr<T>();
		}
	private:
		EchoPhysicsManager* m_pPhysicsManager = nullptr;
		friend class EchoPhysicsSystem;
		friend class EchoPhysicsManager;
		friend class EchoPhysXStaticObject;
		friend class EchoPhysXDynamicObject;
		friend class EchoPhysicsPageGraph;
	};

	enum EchoPhysXNotifyType {
		PhysXNotify_Create,
		PhysXNotify_Destroy,
		PhysXNotify_Break
	};

	class _EchoComponentExport PhysicsLinkComponent : public PhysicsBaseComponent {
	public:
		PhysicsLinkComponent(ActorPtr pActor, ActorCOMMoveableType type) : PhysicsBaseComponent(pActor, type) {}
		virtual ~PhysicsLinkComponent() {}
	private:
		virtual void _actorNotifyPhysX(PhysicsActorComponent* pComp, EchoPhysXNotifyType type) = 0;
	private:
		friend class PhysicsActorComponent;
	};

	class _EchoComponentExport PhysicsActorComponent : public PhysicsBaseComponent {
	public:
		PhysicsActorComponent(ActorPtr pActor, ActorCOMMoveableType type) : PhysicsBaseComponent(pActor, type) {}
		virtual ~PhysicsActorComponent();
	public:
		void GetJointCompList(std::vector<JointComponent*>& joints);
		virtual AggregateComponentPtr GetAggregate();
		virtual DVector3 getWorldPositionPhysX() = 0;
		virtual Quaternion getWorldRotationPhysX() = 0;
		virtual DTransform getWorldTransformPhysX() = 0;
	private:
		virtual PhysicsActor* _getActorPhysX() = 0;
		virtual void _actorNotifyPhysX(PhysicsActorComponent* pComp, EchoPhysXNotifyType type) {}
		void _removePhysX(PhysicsLinkComponentPtr& pComp);
		void _addPhysX(PhysicsLinkComponentPtr& pComp);
	protected:
		void AttachSelfLink();
		void DetachSelfLink();
		void RemoveSelfLink();
#ifdef USE_PHYSICS_PAGE
	public:
		EchoPhysXActorObject* GetPhysXObject() { return mPhysXObject; }
	protected:
		EchoPhysXActorObject* mPhysXObject = nullptr;
#endif //USE_PHYSICS_PAGE
		AggregateComponentPtr mAggregatePtr = nullptr;
	private:
		friend class EchoPhysicsSystem;
		friend class EchoPhysicsManager;
		friend class JointComponent;
		friend class AggregateComponent;
		friend class ConfigurableJointComponent;
		friend class EchoPhysicsPageGraph;
	};

	class _EchoComponentExport EchoPhysicsManager : public ProtocolComponent, public EchoPhysicsQuery {
	public:
		virtual ~EchoPhysicsManager();

		void EditorPlay();
		void EditorPause(bool pause);
		void EditorStop();

		void FlushPhysicsScene();

		virtual void registerTickTimeBegin() override;
		virtual void beginFrame(EventID, ProtocolHandle) override;
		virtual void endFrame(EventID, ProtocolHandle) override;

		virtual void getProtocolType(Name& type) override;
		virtual void getProtocolTypeToString(std::string& typeStr) override;
	private:
		void FlushPhysicsSceneImpl();
		void _fixedUpdate();
		void _latePhysicsUpdate();
		void _prePhysicsUpdate();
		void _postPhysicsUpdate();
	private:
		Name mName;
		bool mIsRunPhysics = false;
		EchoPhysicsSceneCallBack* mPhysXSceneCallBack = nullptr;
		PhysXControllerCallBack* mPhysXCCTCallBack = nullptr;
		PhysXMathControllerCallBack* mPhysXMathCCTCallBack = nullptr;
		//bool mIsResetCCT = false;
		int mCurMoveNumber = 0;
		int mCurCollisionNumber = 0;
		int mCurTriggerNumber = 0;
		bool mIsRigidBodyMoveing = false;
		bool mIsPhysicsUpdating = false;
		bool mIsShowCollider = false;
		bool mIsShowJoint = false;
		bool mIsShowCCT = false;
		float mCCTShapeScale = 1.0f;
		PhysXWorldManager* m_pPhysXWorldManager = nullptr;
		ActorManager* m_pActorManager = nullptr;
	private:
		void GetSelfLinkCompList(PhysicsActorComponent* pComp, PhysicsLinkComponentPtrList*& pJoints);
		PhysicsLinkComponentPtrList& GetSelfLinkCompList(PhysicsActorComponent* pComp);
		void ClearSelfLinkCompList(PhysicsActorComponent* pComp);
		std::unordered_map<PhysicsActorComponent*, PhysicsLinkComponentPtrList> mAllActorLinkCompListMap;
	public://物理运行时
		bool PhysicsIsRunning() const;
		bool PhysicsIsUpdating() const;
		void SetRunPhysics(bool IsRun);
		void SetUpdatePhysics(bool IsUpdate);
		void StartPhysics();
		void StopPhysics();
		//每帧调用
		void UpdateComponentInEditor();
		void UpdatePhysics();
		void AsyncUpdatePhysics();
		void ApplyPhysics();


		void UpdateRigidBodyMove();
		void UpdateSceneEvent();

		void ResetAllColliderColor();
		inline bool IsRigidBodyMoveing() { return mIsRigidBodyMoveing; }
		inline bool IsPhysicsUpdating() { return mIsPhysicsUpdating; }
		inline void SetIsPhysicsUpdating(bool value) { mIsPhysicsUpdating = value; }
		void SetColliderHelperVisible(bool value);
		void SetJointHelperVisible(bool value);
		void SetCCTHelperVisible(bool value);
		bool GetColliderHelperVisible() { return mIsShowCollider; }
		bool GetJointHelperVisible() { return mIsShowJoint; }
		bool GetCCTHelperVisible() { return mIsShowCCT; }

		const std::unordered_set<RigidBodyComponent*>& GetAllRigidBody() { return m_RigidBodyList; }
		const std::unordered_set<ColliderComponent*>& GetAllCollider() { return m_ColliderList; }
		const std::unordered_set<TerrainColliderComponent*>& GetAllTerrain() { return m_TerrainColliderList; }
		const std::unordered_set<JointComponent*>& GetAllJoint() { return m_JointList; }
		const std::unordered_set<ConstantForceComponent*>& GetAllForce() { return m_ForceList; }
		const std::unordered_set<CharatorControllerComponent*>& GetAllController() { return m_ControllerList; }
		const std::unordered_set<AggregateComponent*>& GetAllAggregate() { return m_AggregateList; }
		const std::unordered_set<MathCharacterController*>& GetAllMathController() { return m_MathControllerList; }

		int GetRigidBody_Exist();
		int GetCollider_Exist();
		int GetTerrain_Exist();
		int GetJoint_Exist();
		int GetForce_Exist();
		int GetController_Exist();
		int GetAggregate_Exist();
		int GetMathController_Exist();
	private:
		//运行时数据
		std::array<std::vector<PhysicsBaseComponent*>, (size_t)EchoPhysicsComponentType::eNUMBER> mPhysicsUpdateQueue;
#ifdef USE_PHYSICS_HELPEROBJ
		std::vector<PhysicsBaseComponent*> mEditorUpdateQueue;
#endif // USE_PHYSICS_HELPEROBJ
		//物理组件
		std::unordered_set<RigidBodyComponent*> m_RigidBodyList;
		std::unordered_set<ColliderComponent*> m_ColliderList;
		std::unordered_set<TerrainColliderComponent*> m_TerrainColliderList;//地形块
		std::unordered_set<JointComponent*> m_JointList;
		std::unordered_set<ConstantForceComponent*> m_ForceList;
		std::unordered_set<CharatorControllerComponent*> m_ControllerList;
		std::unordered_set<AggregateComponent*> m_AggregateList;
		//物理相关组件
		std::unordered_map<Index, TerrainColliderComponentPtr> m_TerrainColliderPtrMap;
		std::vector<EchoPhysicsManagerCallBackPtr> m_CallBackList;
		std::unordered_set<GeneratedTerrainPhysics*> m_SphericalTerrainList;
		std::unordered_set<MathCharacterController*> m_MathControllerList;
		std::unordered_map<uint32, ActorComponentPtr> m_SurfaceTypeColliderPtrMap;
	private:
		void AddTerrainColliderComponent(Index index); 
		void RemoveTerrainColliderComponent(Index index);
		void CreateTerrainColliderComponent();
		void DestroyTerrainColliderComponent();
		void CreateSurfaceTypeCollider();

#ifdef USE_PHYSICS_HELPEROBJ
		//运行时从更新队列中添加移除物理组件
		bool AddToEditorUpdateQueue(PhysicsBaseComponent* pComp);
		bool RemoveFromEditorUpdateQueue(PhysicsBaseComponent* pComp);
#endif // USE_PHYSICS_HELPEROBJ
		//运行时从更新队列中添加移除物理组件
		bool AddToPhysicsUpdateQueue(PhysicsBaseComponent* pComp);
		bool RemoveFromPhysicsUpdateQueue(PhysicsBaseComponent* pComp);
		//添加、移除物理组件
		bool AddPhysicsComponent(PhysicsBaseComponent* pComponent);
		bool RemovePhysicsComponent(PhysicsBaseComponent* pComponent);
		//通知全部关节更新
		void SetAllJointIsUpdateHelper(bool value);

		bool AddSphericalTerrain(GeneratedTerrainPhysics*);
		bool RemoveSphericalTerrain(GeneratedTerrainPhysics*);

		bool AddMathCharacterController(MathCharacterController*);
		bool RemoveMathCharacterController(MathCharacterController*);
	public:
		void AddCallBack(const EchoPhysicsManagerCallBackPtr& callback);
		void RemoveCallBack(const EchoPhysicsManagerCallBackPtr& callback);
		void SendRigidBodyCreate(const RigidBodyComponentPtr& _compPtr);
		void SendRigidBodyRelease(const RigidBodyComponentPtr& _compPtr);
		void SendRigidBodyWake(const RigidBodyComponentPtr& _compPtr);
		void SendRigidBodySleep(const RigidBodyComponentPtr& _compPtr);
		void SetPhysXPageLoadMainActor(const ActorComponentPtr& _compPtr, bool bForceAsync = false);
		const ActorComponentPtr& GetPhysXPageLoadMainActor();
		void AddPhysXPageLoadMainActor(const ActorComponentPtr& _compPtr, bool bForceAsync = false);
		void RemovePhysXPageLoadMainActor(const ActorComponentPtr& _compPtr);
		void RemovePhysXPageLoadMainActor(int index);
		int GetPhysXPageLoadMainActorNumber();
		PhysXPageLoadPointData GetPhysXPageLoadMainActor(int index);
		void SetPhysXPageLoadMainActor(int index, const ActorComponentPtr& _compPtr, bool bForceAsync = false);
		bool ExportStaticPhysics(cJSON* root);
	public://Scene
		const Name& GetName();
		Camera* GetCamera();
		SceneManager* GetSceneManager();
		WorldManager* GetWorldManager();
		ActorManager* GetActorManager();
		PhysXWorldManager* GetPhysXWorldManager();
		bool IsExistScene() const;
		PhysXSceneImpl* GetScene() const;
		virtual PhysXSceneQuery* GetPhysXSceneQuery() const override;

		Vector3 GetGravity();
		void SetGravity(const Vector3& vec);

		float GetSimulationTime();

		uint32 GetMultiQueryBufferSize() { return 100; }
		void SetMultiQueryBufferSize(uint32 size) {}

		int GetCurrentMoveNumber();
		int GetCurrentCollisionNumber();
		int GetCurrentTriggerNumber();
		void SetAsyncLoadNumber(int num);
		int GetAsyncLoadNumber();

		void SetDefaultControllerBehavior(uint32 flag);
		uint32 GetDefaultControllerBehavior();

		void setCellSize(float cellSize, EchoPhysXObjectType type = ePhysXStaticObjectType);
		float getCellSize(EchoPhysXObjectType type = ePhysXStaticObjectType);

		PhysicsPagedLayerManager* getMapPagedLayers(EchoPhysXObjectType type = ePhysXStaticObjectType);
		void calculatePagedLayersData(std::map<float, uint32>& data, EchoPhysXObjectType type = ePhysXStaticObjectType);

		void resetCellSize(float cellSize);
		void setUsePhysXPage(bool value);
		bool getUsePhysXPage() { return mIsUsePhysXPage; }
	public://Controller
		void SetControllerOverlapRecoveryModule(bool value);
		bool GetControllerOverlapRecoveryModule();

		void SetControllerTessellation(bool flag, float maxEdgeLength = 1.0f);
		std::pair<bool, float> GetControllerTessellation();

		void SetControllerPreciseSweeps(bool flag);
		bool GetControllerPreciseSweeps();

		void SetControllerPreventVerticalSlidingAgainstCeiling(bool flag);
		bool GetControllerPreventVerticalSlidingAgainstCeiling();

		void SetMeshDoubleSided(bool flag);
		bool GetMeshDoubleSided();

		void SetControllerIsSyncGlobalPose(bool value);
		bool GetControllerIsSyncGlobalPose();
		void SetControllerShapeScale(float value);
		float GetControllerShapeScale();
	public:
		static void AxisAlignedBoxLocalToWorld(AxisAlignedBox& box, const Vector3& worldPosition, const Quaternion& worldRotation);
		static void computePhysicsWorldAABB(const std::vector<ColliderComponent*>& colliders, AxisAlignedBox& box);
		static void computePhysicsWorldAABB(const std::vector<ColliderComponentPtr>& colliders, AxisAlignedBox& box);
		static void computePhysicsWorldAABB(ColliderComponent* collider, AxisAlignedBox& box);
		static void computePhysicsLocalAABB(ColliderComponent* collider, AxisAlignedBox& box);
		static void computePhysicsLocalAABB(const std::vector<ColliderComponent*>& colliders, const std::vector<Transform>& transforms, AxisAlignedBox& box);
		static void computePhysicsLocalAABB(const std::vector<ColliderComponentPtr>& colliders, const std::vector<Transform>& transforms, AxisAlignedBox& box);
		static void computePhysicsLocalAABB(const std::vector<ColliderComponent*>& colliders, const DTransform& transform, AxisAlignedBox& box);
		static void computePhysicsLocalAABB(const std::vector<ColliderComponentPtr>& colliders, const DTransform& transform, AxisAlignedBox& box);
		static bool ExportStaticPhysicsJson(const ActorPtr& actor, cJSON* root);
		static bool ExportStaticPhysicsJson(const ActorInfoPtr& actorInfo, cJSON* root);
		static bool ExportStaticPhysicsJson(const ActorInfoPtr& actorInfo, const Name& actorFilePath);
		static bool CheckStaticPhysics(const ActorInfoPtr& actorInfo);
		static bool PhysXPtrToEchoPtr(PhysicsActor* actor, PhysXShape* shape, RigidBodyComponent*& rigidbody, ColliderComponent*& collider);
		static void convertWorldToLocal(const DTransform& t1, const DTransform& t2, Transform& res);
		static void convertLocalToWorld(const DTransform& t1, const Transform& t2, DTransform& res);
		static void FindColliderComponent(const ActorPtr& actor, std::vector<ColliderComponent*>& colliders, bool IsSearchChildActor = false);
		static void FindColliderComponentPtr(const ActorPtr& actor, std::vector<ColliderComponentPtr>& colliders, bool IsSearchChildActor = false);
		static RigidBodyComponentPtr FindNewRigidBodyComp(const ActorComponentPtr& pNodeComp);
		static RigidBodyComponentPtr FindNewRigidBodyComp(const ActorComponentPtr& pNodeComp, RigidBodyComponent* exclude);
		//全部Collider类型名
		static const std::set<Name> gColliderTypes;
		static const std::vector<Name> gColliderTypesVec;

		static EchoPhysicsManager* instance();
	private:
		void UpdatePhysicsImpl(EchoPhysicsComponentType type);

		void updateEchoPhysXObject(EchoPhysXObject* pEchoPhysXObject);
		void addEchoPhysXObject(EchoPhysXObject* pEchoPhysXObject);
		void removeEchoPhysXObject(EchoPhysXObject* pEchoPhysXObject);
		void UpdatePhysXPage();
		void UpdatePhysXWorldOrigin();
		void UpdateSphericalTerrain();
		void ClearSphericalTerrainPhysics();

		void CreatePhysXObject(RigidBodyComponent* pComp);
		void ReleasePhysXObject(RigidBodyComponent* pComp);
		void CreatePhysXObject(JointComponent* pComp);
		void ReleasePhysXObject(JointComponent* pComp);
		void CreatePhysXObject(ColliderComponent* pComp);
		void ReleasePhysXObject(ColliderComponent* pComp);
	private:
		bool mIsUsePhysXPage = true;
		int mAsyncUpdateNumber = 1000;
		std::vector<PhysXPageLoadPointData> mPhysXPageLoadMainActors;
		std::map<EchoPhysXObjectType, PhysicsPagedLayerManager> mPagedLayers;
#ifdef USE_PHYSICS_PAGE
	private:
		void AddPhysXObjectToUpdateQueue(EchoPhysXObject* ptr);
		void RemovePhysXObjectFromUpdateQueue(EchoPhysXObject* ptr);
		void UpdatePhysXObjectBoundQueue();
		void ClearPhysXObjectBoundQueue();
		std::unordered_set<EchoPhysXObject*> mUpdateBoundQueue;
		EchoPhysicsPageGraph* m_pGraph = nullptr;
#endif // USE_PHYSICS_PAGE
	private:
		void ClearAllUpdateCacheQueue();

		void FlushSceneSimulationCache();
		uint32 mFlushSimulationCycleCount = 0;

		void AddRigidBodyToUpdateMAITQueue(RigidBodyComponent* pComp);
		void RemoveRigidBodyFromUpdateMAITQueue(RigidBodyComponent* pComp);
		void UpdateRigidBodyMAITQueue();
		void ClearRigidBodyMAITQueue();
		uint32 mUpdateMAITCycleCount = 0;
		std::unordered_set<RigidBodyComponent*> mUpdateMAITQueue;

		void AddColliderToUpdateLocalPoseQueue(ColliderComponent* pComp);
		void RemoveColliderFromUpdateLocalPoseQueue(ColliderComponent* pComp);
		void UpdateColliderLocalPoseQueue();
		void ClearColliderLocalPoseQueue();
		std::unordered_set<ColliderComponent*> mUpdateLocalPoseQueue;

		void AddRigidBodyToUpdatePhysXTransformQueue(RigidBodyComponent* pComp);
		void RemoveRigidBodyFromUpdatePhysXTransformQueue(RigidBodyComponent* pComp);
		void UpdateRigidBodyPhysXTransformQueue();
		void ClearRigidBodyPhysXTransformQueue();
		std::unordered_set<RigidBodyComponent*> mUpdatePhysXTransformQueue;

		void AddColliderToUpdateGeometry(ColliderComponent* pComp);
		void RemoveColliderFromUpdateGeometry(ColliderComponent* pComp);
		void UpdateColliderGeometryQueue();
		void ClearColliderGeometryQueue();
		std::unordered_set<ColliderComponent*> mUpdateColliderGeometryQueue;

		PhysXShape* CreatePhysXShape(EchoGeometry* geom, int matID = -1);
		void ReleasePhysXShape(PhysXShape* pShape);
		void RemovePhysXActor(PhysXRigidDynamic* pRigid);
		void RemovePhysXActor(PhysXStatic* pRigid);
		void RemovePhysXActor(PhysXCharacterController* pRigid);
		void RemovePhysXAggregate(PhysXAggregate* pAggre);
		void UpdatePhysXWaitDeleteQueue();
		std::vector<PhysicsActor*> mPhysXActorWaitDeleteQueue;
		std::vector<PhysXShape*> mPhysXShapeWaitDeleteQueue;
		std::vector<PhysXAggregate*> mPhysXAggregateWaitDeleteQueue;

		void AppearUncommittedChanges();
		bool mHasUncommittedChanges = false;

		void SyncAllCctPhysXGlobalPose();
	private:
		void UpdatePredictVelocity(const Vector3& position);
		Vector3 m_PredictLastPosition = Vector3::MIN;
		Vector3 m_PredictVelocity = Vector3::ZERO;
		float m_PredictSpeed = 0.0f;
		std::array<Vector3, 30> m_OffsetCaches;
		uint32 m_OffsetCacheIndex = 0;
		Vector3 m_SumOffset = Vector3::ZERO;
	private:
		void TestGeometryQueryTime();
		void TestSceneQueryTime();
		void TestMeshGeometryQueryTime();
		void TestSceneQueryTime(const AxisAlignedBox& bound);
	private:
		EchoPhysicsManager(const Name& name);
		friend class CoreSystemBase<EchoPhysicsManager>;
		friend class EchoPhysicsSystemProFuncImp;
		friend class EchoPhysicsSceneCallBack;
		friend class PhysicsManagerUI;
		friend class PhysicsBaseComponent;
		friend class PhysicsActorComponent;
		friend class PhysicsLinkComponent;
		friend class EchoPhysXActorObject;
		friend class EchoPhysXStaticObject;
		friend class EchoPhysXDynamicObject;
		friend class EchoPhysXGroupObject;
		friend class CharatorControllerComponent;
		friend class ColliderComponent;
		friend class RigidBodyComponent;
		friend class JointComponent;
		friend class EchoPhysXObject;
		friend class EchoPhysXWorldSystemCallBack_EchoPhysicsSystem;
		friend class EchoPhysicsSystem;
		friend class GeneratedTerrainPhysics;
		friend class EchoPhysicsPageGraph;
		friend class MathCharacterController;
		friend class AggregateComponent;
	};

	class _EchoComponentExport EchoPhysicsSystem : public Singleton<EchoPhysicsSystem>, public CoreSystemBase<EchoPhysicsManager>
	{
	public:
		static	Echo::Name			mType;
		const Echo::Name&	getType() { return mType; }
	public:
		REGISTER_LUA_CLASS_KEY
		EchoPhysicsSystem();
		virtual ~EchoPhysicsSystem();

		void EditorPlay();
		void EditorPause(bool pause);
		void EditorStop();

		EchoPhysicsManager* GetPhysicsManager(ActorManager* pActorMgr);
		EchoPhysicsManager* GetPhysicsManager(ActorComponent* pComp);
		EchoPhysicsManager* GetPhysicsManager(WorldManager* pWorldMgr);
		EchoPhysicsManager* GetPhysicsManager(SceneManager* pSceneMgr);
		EchoPhysicsManager* GetMainPhysicsManager() const { return GetMainManager(); }
		EchoPhysicsManager* GetActivePhysicsManager();
	private:
		CharatorControllerComponent* m_pCurrentCharatorControllerComponent = nullptr;
		EchoPhysXWorldSystemCallBack* mPhysXWorldCallBack = nullptr;
	public:
		bool PhysicsIsRunning() const;
		bool PhysicsIsUpdating() const;
		void SetRunPhysics(bool IsRun);
		void SetUpdatePhysics(bool IsUpdate);
		void UpdateComponentInEditor();
		void ResetAllColliderColor();
		void SetColliderHelperVisible(bool value);
		void SetJointHelperVisible(bool value);
		void SetCCTHelperVisible(bool value);
		void SetAllJointIsUpdateHelper(bool value);
		void SetCurrentCharactorController(CharatorControllerComponent* pComp);
		CharatorControllerComponent* GetCurrentCharactorController();
	private:
		void SetUsePhysXPageS(const vector<String>::type& value);
		void ResetCellSizeS(const vector<String>::type& value);
		void SetPhysXPageLoadMainActorS(const vector<String>::type& value);
		void AddPhysXPageLoadMainActorS(const vector<String>::type& value);
		void SetPhysXDebugLinesS(const vector<String>::type& value);
		void ShowPhysicsData();
		void ShowDebugLinesFlagsData();
		void ShowHelp();
		void TestSceneQueryTime(const vector<String>::type& value);
	private://Test ConsoleCommand
		void PrintMemoryS(const vector<String>::type& value);
		void TestS(const vector<String>::type& value);
		//Real bullet
		void ShootDynamicSphereColliderS(const vector<String>::type& value);
		void ShootDynamicSphereCollider(int number);
		void ClearShootCollider();
		void SetSpeedS(const vector<String>::type& value);
		void SetCollisionDetectionModeS(const vector<String>::type& value);
		void SetRandomModeS(const vector<String>::type& value);
		float mSpeed = 100.f;
		int mCollisionDetectionMode = 2;
		int mRandomMode = 0;
		//Static Scene
		void SetCreateStaticParentActorS(const vector<String>::type& value);
		void CreateStaticTestPhysicsS(const vector<String>::type& value);
		void CreateStaticTestPhysics(int number);
		void CreateStaticTestPhysicsBox(int number);
		void CreateStaticTestPhysicsSphere(int number);
		void CreateStaticTestPhysicsMesh(const String& meshName, int number);
		void ClearStaticTestPhysics();
		void SetCreateStaticPositionMinS(const vector<String>::type& value);
		void SetCreateStaticPositionMaxS(const vector<String>::type& value);
		void SetCreateStaticOriginS(const vector<String>::type& value);
		void SetCreateStaticRandomScaleS(const vector<String>::type& value);
		void SetCreateStaticRadiusS(const vector<String>::type& value);
		void SetCreateStaticModeS(const vector<String>::type& value);
		Vector3 mCreateStaticPositionMin = Vector3(-250.f, -10.f, -250.f);
		Vector3 mCreateStaticPositionMax = Vector3(250.f, 10.f, 250.f);
		Vector3 mCreateStaticOrigin = Vector3::ZERO;
		float mCreateStaticRadiusMin = 100.0f;
		float mCreateStaticRadiusMax = 110.0f;
		float mCreateStaticRandomScaleMin = 1.0f;
		float mCreateStaticRandomScaleMax = 6.0f;
		int mCreateStaticMode = 0;
		ActorPtr mCreateStaticParentActor = nullptr;
	private:
		friend class EchoPhysicsSystemProFuncImp;
		friend class EchoPhysicsSceneCallBack;
		friend class PhysicsManagerUI;
		friend class PhysicsBaseComponent;
		friend class PhysicsActorComponent;
		friend class PhysicsLinkComponent;
		friend class EchoPhysXActorObject;
		friend class EchoPhysXStaticObject;
		friend class EchoPhysXDynamicObject;
		friend class EchoPhysXGroupObject;
		friend class EchoPhysicsManager;
		friend class CharatorControllerComponent;
		friend class ColliderComponent;
		friend class RigidBodyComponent;
		friend class JointComponent;
		friend class EchoPhysXWorldSystemCallBack_EchoPhysicsSystem;
		friend class ComponentSystem;

	};

}



#endif /*__EchoPhysicsSystem_h__*/
