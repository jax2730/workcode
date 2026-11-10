#ifndef __EchoRagdollBehaviorComponentBase_H__
#define __EchoRagdollBehaviorComponentBase_H__

#include "EchoComponentPrerequisites.h"
#include "Actor/EchoActorComponent.h"

namespace Echo
{
	class	RagdollComponent;
	class   RagdollMuscle;
	class	ColliderComponent;

	struct _EchoComponentExport	MuscleHit
	{
		/// <summary>
		/// The index of the colliding muscle in the PuppetMaster.muscles array.
		/// </summary>
		int muscleIndex;

		/// <summary>
		/// How much should the muscle be unpinned by the hit?
		/// </summary>
		float unPin;

		/// <summary>
		/// The force to add to the muscle's Rigidbody.
		/// </summary>
		Vector3 force;

		/// <summary>
		/// The world space hit point.
		/// </summary>
		Vector3 position;
	};
	struct _EchoComponentExport	MuscleCollision
	{
		/// <summary>
		/// The index of the colliding muscle in the PuppetMaster.muscles array.
		/// </summary>
		int muscleIndex;

		/// <summary>
		/// The collision from OnCollisionEnter/Stay/Exit.
		/// </summary>
		ColliderComponent* collision;

		bool isStay;
	};

	namespace RagdollBehavior
	{
		typedef	std::function<void()>							BehaviourDelegate;
		typedef std::function<void(float)>						BehaviourUpdateDelegate;
		typedef std::function<void(const MuscleHit&)>			HitDelegate;
		typedef std::function<void(const MuscleCollision&)>		CollisionDelegate;

		class _EchoComponentExport AnimationEvent
		{
		public:
			virtual void Activate() = 0;
		};

		struct RagdollEvent
		{
			Name						switchToBehaviour;	// 切换到这个行为,来自RagdollBehaviorComponentBase::GetTypeSpring()
			AnimationEvent*				animationEvent;

			void Trigger(RagdollComponent* ragdollcom);
		};
	}

};// namespace Echo

namespace Echo
{

	// 该类型为所有布偶行为的基类
	// 只需重新该类即可为布偶创建自定义的行为
	// 布偶行为组件不要手动设置visable！！！其visable行为会由自身管理
	class _EchoComponentExport RagdollBehaviorComponentBase : public ActorComponent
	{
	public:
		friend class RagdollBehaviorManger;
		virtual const Name& GetTypeSpring();	// return "BehaviourBase"

	public: // 以下函数调用发生在其控制的RagdollComponent的时间点
		// Called when the RagdollComponent has been Reactivated 
		virtual void OnReactivate() = 0;
		// Called when the RagdollComponent goes from other states to alive state
		virtual void Resurrect() { }
		// called when the RagdollComponent be Frozen (next ragdollComponent lateUpdate)
		virtual void Freeze() { }
		// called when the RagodllComponent is unfrozen (immediately)
		virtual void Unfreeze() { }
		// called when the RagdollComponent goes form alive state to dead or frozen state  
		virtual void KillStart() { }
		virtual void KillEnd() { }
		// called when the RagdollComponent has been teleported
		virtual void OnTeleport(Quaternion deltaRotation, Vector3 deltaPosition, Vector3 pivot, bool moveToTarget) { }
		// called when the RagdollComponent`s muscle has been disconnected /reconnected
		virtual void OnMuscleDisconnected(RagdollMuscle* m) { }
		virtual void OnMuscleReconnected(RagdollMuscle* m) { }
		// called when the RagdollComponent`s muscle has been added or removed 
		// defult will call functor OnHierarchyChanged 
		virtual void OnMuscleAdded(RagdollMuscle* m);
		virtual void OnMuscleRemoved(RagdollMuscle* m);
		RagdollBehavior::BehaviourDelegate OnHierarchyChanged;

	protected: // 以下函数调用发生在布偶行为自己的时间点
		// called when Behavior be activated ( must use member function Activate)
		// Ensure self is initialized
		// Automatically SetVisable TRUE before being called	
		virtual void OnActivate() { }
		RagdollBehavior::BehaviourDelegate			OnPreActivate;
		RagdollBehavior::BehaviourDelegate			OnPostActivate;

		// called when Bahavior be deactivated (visable flag set false)
		// Ensure self is initialized
		virtual void OnDeactivate() { }
		RagdollBehavior::BehaviourDelegate			OnPreDeactivate;
		RagdollBehavior::BehaviourDelegate			OnPostDeactivate;

		// called when Bahaviot be Initialized (init user data in there)
		// change to initialized state after being called
		virtual void OnInitiate() { }
		RagdollBehavior::BehaviourDelegate			OnPreInitiate;
		RagdollBehavior::BehaviourDelegate			OnPostInitiate;

	protected: // 下面函数为对应名称的非虚函数的实现

		// called in member function FixedUpdateB 
		// called in member function UpdateB 
		// called in member function LateUpdateB 
		// Ensure self is initialized and visible
		// Ensure ragdoll and ragdoll has muscle
		virtual void OnFixedUpdate(float deltaTime) { }
		virtual void OnUpdate(float deltaTime) { }
		virtual void OnLateUpdate(float deltaTime) { }
		RagdollBehavior::BehaviourUpdateDelegate	OnPreFixedUpdate;
		RagdollBehavior::BehaviourUpdateDelegate	OnPreUpdate;
		RagdollBehavior::BehaviourUpdateDelegate	OnPreLateUpdate;
		RagdollBehavior::BehaviourUpdateDelegate	OnPostFixedUpdate;
		RagdollBehavior::BehaviourUpdateDelegate	OnPostUpdate;
		RagdollBehavior::BehaviourUpdateDelegate	OnPostLateUpdate;

		// called in member function UpdateHelperB
		// Ensure self is initialized and visible
		virtual void OnUpdateHelperBehaviour() { }

		// reserve. It will be used when ControllerComponent iterate to a version that can switch animation update mode
		// not use now
		virtual void OnFixTransformsBehaviour() { }
		RagdollBehavior::BehaviourDelegate			OnPreFixTransforms;
		RagdollBehavior::BehaviourDelegate			OnPostFixTransforms;

		// called in member function OnRead
		// Ensure self is initialized and visible
		virtual void OnReadBehaviour(float deltaTime) { }
		RagdollBehavior::BehaviourUpdateDelegate	OnPreRead;
		RagdollBehavior::BehaviourUpdateDelegate	OnPostRead;

		// called in nember function OnWrite
		// Ensure self is initialized and visible
		virtual void OnWriteBehaviour(float deltaTime) { }
		RagdollBehavior::BehaviourUpdateDelegate	OnPreWrite;
		RagdollBehavior::BehaviourUpdateDelegate	OnPostWrite;

		// called in nember function OnMuscleHit 
		// called in nember function OnMuscleCollision
		// called in nember function OnMuscleCollisionExit
		// Ensure self is initialized
		virtual void OnMuscleHitBehaviour(const MuscleHit& hit) { }
		virtual void OnMuscleCollisionBehaviour(const MuscleCollision& collision) { }
		virtual void OnMuscleCollisionExitBehaviour(const MuscleCollision& collision) { }
		RagdollBehavior::HitDelegate				OnPreMuscleHit;
		RagdollBehavior::CollisionDelegate			OnPreMuscleCollision;
		RagdollBehavior::CollisionDelegate			OnPreMuscleCollisionExit;
		RagdollBehavior::HitDelegate				OnPostMuscleHit;
		RagdollBehavior::CollisionDelegate			OnPostMuscleCollision;
		RagdollBehavior::CollisionDelegate			OnPostMuscleCollisionExit;

	public: //以下函数无需重写但可以重新其实现或修改函数对象 与碰撞相关
		// TODO:
		// called from RagdollComponent::muscle.el.colldierBroadcaster.hit
		void OnMuscleHit(const MuscleHit& hit);
		// called every frame when touches another Collider or Rigidbody.
		void OnMuscleCollision(const MuscleCollision& collision);
		// called when exits Collider or Rigidboy
		void OnMuscleCollisionExit(const MuscleCollision& collision);

	public:// 以下函数无需重写但可以重新其实现或修改函数对象 用于提供给布偶组件调用

		// called when RagdollComponent Initiate 
		void Initiate();
		// reserve. It will be used when ControllerComponent iterate to a version that can switch animation update mode
		void OnFixTransforms();
		// called when Ragdollcomponent read traget animaion pos
		void OnRead(float deltaTime);
		// called when Ragdollcomponent has completed 'map' or 'move to traget aniamion'
		void OnWrite(float deltaTime);
		// called when Ragdollcomponent need the behaviour is activeted
		void Activate();
		// called when Ragdollcomponent ticking
		void FixedUpdateB(float deltaTime);
		void UpdateB(float deltaTime);
		void LateUpdateB(float deltaTime);
		// called when Ragdollcomponent update helper 
		void UpdateHelperB();

	protected:	// 工具函数 以后可能转移到专门的工具类
		// 将角色节点旋转到骨骼朝向
		void RotateTargetToRootMuscle();
		// 将角色盆骨骨骼移动到物理组件位置 (可在三个方向分别插值 0.f为100%的映射，1.f不做移动，大于1.f将向反方向移动)
		void TranslateTargetToRootMuscle(float maintainX = 0.f , float maintainY = 0.f , float maintainZ = 0.f);
		// 将角色节点向下移动到第一个光线接触到的物体 layers 为光线投射的掩码
		void GroundTarget(int layers);
		// TODO：布偶道具完成后添加
		void RemovePropMuscles() {}							
		bool ContainsRemovablePropMuscle() { return false; }
	public:
		virtual void visibleStateChanged(bool visibleState) override final;
	private:
		void OnEnable();
		void OnDisable();
	public:
		bool				IsDeactived() { return deactivated; }
		void				Deactived(bool isDeactived) { deactivated = isDeactived; }
		bool				IsForceActive() { return forceActive; }
	protected:
		void				ForceActive(bool isForceActive) { forceActive = isForceActive ;}
		ActorComponentPtr&	GetRagdollCom() { return ragdollCom; }
	
	private:
		friend class RagdollBehaviorManagerComponent;
		ActorComponentPtr	ragdollCom;	// 保存使用该行为的主人
	private:
		bool deactivated	= true;
		bool forceActive	= false;
		bool initiated		= false;
	};
}; // namespace Echo



#endif // !__EchoRagdollBehaviorComponentBase_H__
