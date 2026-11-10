//-----------------------------------------------------------------------------
// File:  EchoRagdollComponent.h
//
// Author: LangGuangyue
//
// Date:   2022-10-20
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef _EchoRagdollComponent_H__
#define _EchoRagdollComponent_H__

#include "EchoComponentPrerequisites.h"
#include "Actor/EchoActorComponent.h"
#include "Actor/EchoActor.h"
#include "Actor/EchoNodeComponent.h"
#include "EchoColliderComponent.h"
#include "EchoBoxColliderComponent.h"
#include "EchoCapsuleColliderComponent.h"
#include "EchoActorRoleComponent.h"
#include "EchoEnginePrerequisites.h"
#include "EchoSkeletonInstance.h"
#include "Role/EchoRoleCommonComponent.h"
#include "EchoRigidBodyComponent.h"
#include "EchoConfigurableJointComponent.h"
#include "EchoRagdollMuscle.h"
#include "EchocJsonHelper.h"
#include "EchoComDefineMacro.h"
#include "EchoRoleObject.h"

namespace Echo
{
#define COPYINFO(INFO,_val)		INFO->_val = this->_val;
#define COPYINIT(_val)			_val = mInfo->_val;

	class RagdollBehaviorComponentBase;
	class RagdollBehaviorManagerComponent;
	struct RagdollBlockDesc;
	struct ragdollBlockData;


	struct ragdollBlockData {
		ActorComponentPtr	node = nullptr;
		ActorComponentPtr	collider = nullptr;
		ActorComponentPtr	rigidbody = nullptr;
		ActorComponentPtr	joint = nullptr;
		
		Name				BoneName;
		Name				jointTarget;

		RagdollMuscle::MuscleGroup	group = RagdollMuscle::MuscleGroup::Hips;
		float						mapping = 1.f;			// [0.f ~ 1.f]
		float						pin = 1.f;				// [0.f ~ 1.f] 
		float						weight = 1.f;			// [0.f ~ 1.f] 
		float						damper = 1.f;			// [0.f ~ 1.f]
		bool						mapPosition = true;

		RagdollMuscle* muscle = nullptr;
	};

	struct RagdollBlockDesc
	{
		Name				nodeName;
		Name				colliderName;
		Name				rigidbodyName;
		Name				jointName;

		Name				BoneName;
		Name				jointTarget;

		RagdollMuscle::MuscleGroup	group = RagdollMuscle::MuscleGroup::Hips;
		float						mapping = 1.f;			// [0.f ~ 1.f]
		float						pin = 1.f;				// [0.f ~ 1.f] 
		float						weight = 1.f;			// [0.f ~ 1.f] 
		float						damper = 1.f;			// [0.f ~ 1.f]
		bool						mapPosition = true;
	};

	enum RagdollMuscleRemoveMode {
		Sever,			// Severs the body part disconnecting the first joit
		Explode,		// Explodes the body part disconnecting all joints
		Numb,			// Removes the muscle, keeps the joints connected, but disables spring and damper forces
	};

	enum RagdollMuscleDisconnectMode
	{
		Dis_Sever,
		Dis_Explode
	};

	enum RagdollBodyBones
	{
		Hips = 0,
		LeftUpperLeg,
		RightUpperLeg,
		LeftLowerLeg,
		RightLowerLeg,
		LeftFoot,
		RightFoot,
		Spine,
		Chest,
		Neck,
		Head,
		LeftShoulder,
		RightShoulder,
		LeftUpperArm,
		RightUpperArm,
		LeftLowerArm,
		RightLowerArm,
		LeftHand,
		RightHand,
		LeftToes,
		RightToes,
		LeftEye,
		RightEye,
		Jaw,
		LastBone
	};

	enum class RagdollBehaviorMode
	{
		Disabled ,		// 关闭布偶，减少消耗
		_WaitToActive,	// 激活布偶时需要等待一帧收集数据，此时属于该状态。【不能主动设置】
						// 布偶激活后，会在渲染前把骨骼姿态同步到物理块的姿态中
		Kinematic,		// 运动学：	直接将布偶物理块放置到动画骨骼的位置。
		Alive,			// 存活状态：	通过给刚体施加力，和扭矩，以及添加关节的角驱动来尝试将布偶物理推向动画骨骼的位置
		Dead,			// 死亡状态： 不再或少量的将布偶物理推向动画骨骼的位置。
	};

	// 准备弃用，请更换使用RagdollBehaviorMode
	enum RagdollState 
	{
		Alive,
		Dead,
		Frozen
	};
	// 准备弃用，请更换使用RagdollBehaviorMode
	enum RagdollMode
	{
		Active,
		Kinematic,
		Disabled
	};

	enum RagdollUpdateMode
	{
		Mode_Normal,
		Mode_AnimatePhysics,
		Mode_FixedUpdate
	};
	enum RagdollMuscleModfiyFlag : uint16
	{
		MuscleModfiy_None			= 0,
		MuscleModfiy_Joint			= 1 << 0,
		MuscleModfiy_Target			= 1 << 1,
		MuscleModfiy_JointAndTarget = MuscleModfiy_Joint | MuscleModfiy_Target,

		MuscleAttributeModfiy_Group		= 1	<< 2,
		MuscleAttributeModfiy_Mapping	= 1 << 3,
		MuscleAttributeModfiy_Pin		= 1 << 4,
		MuscleAttributeModfiy_Weight	= 1 << 5,
		MuscleAttributeModfiy_Damper	= 1 << 6,
		MuscleAttributeModfiy_AllAttribute = MuscleAttributeModfiy_Group | MuscleAttributeModfiy_Mapping | MuscleAttributeModfiy_Pin | MuscleAttributeModfiy_Weight | MuscleAttributeModfiy_Damper,

		MuscleModfiy_ALL = MuscleModfiy_JointAndTarget | MuscleAttributeModfiy_AllAttribute,
	};
	struct RagdollMuscleModifyValue
	{
		Name						jointComName;		// 无法修改仅仅可以读取
		Name						targetComName;		// 无法修改仅仅可以读取

		RagdollMuscle::MuscleGroup	group		= RagdollMuscle::MuscleGroup::Hips;
		float						mapping		= 0.f;	// [0.f ~ 1.f]
		float						pin			= 0.f;	// [0.f ~ 1.f] 
		float						weight		= 0.f;	// [0.f ~ 1.f] 
		float						damper		= 0.f;	// [0.f ~ 1.f]
	};
	struct RagdollMuscleModify
	{
		Name						muscleName;	// 对应的骨骼的名字

		// RagdollMuscleModfiyFlag combination
		int							modifyFlag = RagdollMuscleModfiyFlag::MuscleModfiy_None;

		RagdollMuscleModifyValue	muscleValue;
	};
	struct RagdollStateSettings 
	{
	public:
		float		killDuration;					//How much does it take to weigh out muscle weight to deadMuscleWeight?
		float		deadMuscleWeight;				//The muscle weight mlp while the puppet is Dead.
		float		deadMuscleDamper;				//The muscle damper add while the puppet is Dead.
		float		maxFreezeSqrVelocity;			//The max square velocity of the ragdoll bones for freezing the puppet.
		bool		freezePermanently;				//If true, RagDoll, all it's behaviours and the ragdoll will be destroyed when the puppet is frozen.
		bool		enableAngularLimitsOnKill;		//If true, will enable angular limits when killing the puppet.
		bool		enableInternalCollisionsOnKill;	//If true, will enable internal collisions when killing the puppet.

		RagdollStateSettings(float killDuration, float deadMuscleWeight = 0.01f, float deadMuscleDamper = 2.0f, float maxFreezeSqrVelocity = 0.02f,
			bool freezePermanently = false, bool enableAngularLimitsOnKill = true, bool enableInternalCollisionsOnKill = true);
		static RagdollStateSettings Default;
	};
	struct _EchoComponentExport RagdollComponentData
	{
		//struct _EchoComponentExport MucleData
		//{
		//	float				mappingWeight = 1.0f;	
		//	float				pinWeight = 1.0f;		
		//	float				muscleWeight = 1.0f;	
		//	float				muscleDamper = 1.0f;
		//};

		RagdollStateSettings	stateSettings				= RagdollStateSettings::Default;
		float					blendTime					= 0.1f;
		bool					fixTargetTransforms			= true;
		int						solverIterationCount		= 10;
		float					mappingWeight				= 1.0f;
		float					pinWeight					= 1.0f;
		float					muscleWeight				= 1.0f;
		float					muscleSpring				= 50.0f;
		float					muscleDamper				= 1.0f;
		float					pinPow						= 4.0f;
		float					pinDistanceFalloff			= 0.0f;
		float					maxAngularVelocity			= 25.f;
		float					maxLinearVelocity			= 60.f;
		float					ragdollDensity				= 1e3f;
		bool					angularPinning				= false;
		bool					updateJointAnchors			= true;
		bool					supportTranslationAnimation = false;
		bool					angularLimits				= true;
		bool					internalCollisions			= true;
		//std::vector<std::pair<Name,MucleData>>	mucleData	;
	};

	//---------------------------------------------------------------------------------
	class RagdollComponentInfo;
	
	//---------------------------------------------------------------------------------
	class _EchoComponentExport RagdollComponent : public ProtocolNodeActorComponent, public RoleLoadListener
	{
		DECLAREACTORCOMTYPE_H(RagdollComponent)
	public:
		virtual void						init(ActorComponentInfo* info) override;
		virtual void						bindNode() override;
		virtual void						replay(ActorComponentInfo* info) override;
		virtual void						connectCom() override;
		virtual void						disconnectCom() override;
		virtual	void						setActor(const ActorPtr& pActor) override;
		virtual void						copyTo(const ActorComponentPtr& actCom) override;
		virtual void						dealComponentDelete(const ActorComponentPtr& actCom) override;
		virtual void						visibleStateChanged(bool enable) override;

		virtual void						registerTickTimeBegin() override;
		virtual void						prePhysicsUpdate(EventID, ProtocolHandle) override;
		virtual void						fixedUpdate(EventID, ProtocolHandle)override;
		virtual void						preUpdate(EventID, ProtocolHandle)override;
		virtual void						lateUpdate(EventID, ProtocolHandle)override;

		virtual void						EditorPlayImp()				override;
		virtual void						EditorPauseImp(bool pause)	override;
		virtual void						EditorStopImp()				override;

		virtual void						ResetRegisterFuns() {};
	//------------------------------------------------------------Void------------------------------------------------------
	public: //  布偶构建相关
		// 加载静态数据和肌肉数据， 
		void					LoadStaticData(const Name& targetName, const RagdollComponentData* ragdollData);
		// 准备充分后从静态数据获取真正的资源
		// 调用成功之后无法再次调用，LoadStaticDataAndBlockData也会失效
		bool					loadDataFromInfo(const std::vector<RagdollBlockDesc>* blockData);
		bool					IsLoad();
	public:
		// 启动相关 

		void					Awake();
		bool					IsAwaked();
		void					Stop();

		bool					OnRoleCreateFinish() override;		// late loadDataFromInfo

		void					ShrinkRagdollBlocks();	// 删除mRagdollBlocks中的空元素
		void					CopyMuscleToInfo();		// 将创建的肌肉信息复制到info中(尝试持久保存数据)
#ifdef ECHO_EDITOR
		// 同步导出布偶数据
		// 现在将强制更改后缀名为 .ragdolldata
		// 无法变更
		void					SaveDataFile(const Name& fileName);	// 同步导出，一般仅在编辑器中使用
#endif
		// 布偶行为切换
		RagdollBehaviorMode		GetRagdollBehaviorMode(){ return behaviorMode; };
		bool					IsDisabled()			{ return behaviorMode == RagdollBehaviorMode::Disabled; }
		bool					IsActive()				{ return behaviorMode > RagdollBehaviorMode::_WaitToActive; }
		bool					IsKinematic()			{ return behaviorMode == RagdollBehaviorMode::Kinematic; }
		bool					IsAlive()				{ return behaviorMode == RagdollBehaviorMode::Alive; }
		bool					IsDead()				{ return behaviorMode == RagdollBehaviorMode::Dead; }

		void					DisableRagdoll();		// 禁用布偶
		void					ToKinematic();			// 运动学
		void					ToAlive();				// 存活		如果是从禁用模式切换过来的话，则会先进入到WaitToActive模式
		void					ImmediatelyToAlive();	// 立刻存活	不经历WaitToActive模式。这会导致布偶死亡后无法继承布偶禁用时的速度。
		void					ToDead();				// 死亡		如果是从禁用模式切换过来的话，则会先进入到WaitToActive模式，死亡后肌肉强度会逐渐减低至死亡强度。
		void					ImmediatelyToDead();	// 立刻死亡	不经历WaitToActive模式。这会导致布偶死亡后无法继承布偶禁用时的速度，肌肉强度也会立刻变为死亡强度。
		// 准备弃用，请更换使用RagdollBehaviorMode
		void SetMode(RagdollMode mode)
		{
			switch (mode)
			{
			case RagdollMode::Active:
				ToDead();
				break;
			case RagdollMode::Kinematic:
				ToKinematic();
				break;
			case RagdollMode::Disabled:
				DisableRagdoll();
				break;
			default:
				break;
			}
		}
		void SetState(RagdollState state)
		{
			switch (state)
			{
			case RagdollState::Alive:
				ImmediatelyToAlive();
				break;
			case RagdollState::Dead:
				ToDead();
				break;
			case RagdollState::Frozen:
				break;
			default:
				break;
			}
		}
		RagdollMode GetMode()
		{
			switch (behaviorMode)
			{
			case Echo::RagdollBehaviorMode::Disabled:
				return RagdollMode::Disabled;
				break;
			case Echo::RagdollBehaviorMode::_WaitToActive:
				return RagdollMode::Disabled;
				break;
			case Echo::RagdollBehaviorMode::Kinematic:
				return RagdollMode::Kinematic;
				break;
			case Echo::RagdollBehaviorMode::Alive:
				return RagdollMode::Active;
				break;
			case Echo::RagdollBehaviorMode::Dead:
				return RagdollMode::Active;
				break;
			default:
				return RagdollMode::Disabled;
				break;
			}
		}
		bool IsFrozen() { return false; }
		// 参数相关
		const Name&				GetTargetName()				{ return targetName; }

		RagdollStateSettings	GetStateSetting() const;
		void					SetStateSetting(const RagdollStateSettings& stateSettings);

		void					SetBlendTime(float time);
		float					GetBlendTime() const;

		void					SetFixTargetTransforms(bool fixTargetTransforms);
		bool					IsFixTargetTransforms() const;

		void					SetSolverIterationCount(int count);
		int						GetSolverIterationCount() const;

		void					SetGlobalMappingWeight(float weight);	// [0.f ~ 1.f]
		void					SetGlobalPinWeight(float weight);		// [0.f ~ 1.f]
		void					SetGlobalMuscleWeight(float weight);	// [0.f ~ 1.f]
		float					GetGlobalMappingWeight() const;
		float					GetGlobalPinWeight() const;
		float					GetGlobalMuscleWeight() const;

		void					SetGlobalMuscleSpring(float spring);	// [0.f ~ +INF]
		void					SetGlobalMuscleDamper(float damper);	// [0.f ~ +INF]
		float					GetGlobalMuscleSpring() const;
		float					GetGlobalMuscleDamper() const;

		void					SetPinPow(float pow);					// [0.f ~ 8.f]
		void					SetPinDistanceFalloff(float val);		// [0.f ~ +INF]
		void					SetAngularPinning(bool  val);
		void					SetUpdateJointAnchors(bool  val);
		void					SetSupportTranslationAnimation(bool val);
		void					SetMaxAngularVelocity(float val);
		void					SetMaxLinearVelocity(float val);
		void					SetRagdollDensity(float val);
		void					SetAngularLimits(bool val);
		void					SetInternalCollisions(bool val);		 
		float					GetPinPow() const;
		float					GetPinDistanceFalloff()const;
		float					GetMaxAngularVelocity()const;
		float					GetMaxLinearVelocity()const;
		float					GetRagdollDensity()const;
		bool					GetAngularPinning()const;
		bool					GetUpdateJointAnchors()const;
		bool					GetSupportTranslationAnimation()const;
		bool					GetAngularLimits()const;
		bool					GetInternalCollisions()const;

		// muscleName字段为希望更改/获取的肌肉的骨骼名字，modfiyFlag为希望更改/获取哪些属性
		// muscleValue字段用于传递/接收属性值
		uint16					SetMuscleAttribute(const RagdollMuscleModify& modfiy);	
		void					GetMuscleAttribute(RagdollMuscleModify& data);
		std::vector<RagdollMuscleModify>
								getAllMuscleAttribue();

		void					SetComData(const RagdollComponentData* data);

		// 操作相关
		ActorComponentPtr		GetTargetRoot() { return targetRoot; }
		void					Teleport(Vector3 position, Quaternion rotation, bool moveToTarget);

		RagdollMuscle*			GetMuscle(WorldNode* target);
		RagdollMuscle*			GetMuscle(RigidBodyComponent* rigidbody);
		RagdollMuscle*			GetMuscle(ConfigurableJointComponent* joint);
		RagdollMuscle*			GetMuscle(const Name& boneName);

		int						GetMuscleIndex(WorldNode* target);
		int						GetMuscleIndex(RigidBodyComponent* rigidbody);
		int						GetMuscleIndex(ConfigurableJointComponent* joint);
		int						GetMuscleIndex(const Name& boneName);

		ragdollBlockData*		FindBoneBlock(const Name& boneName);

		bool					ContainsJoint(ConfigurableJointComponent* joint);

		/// <summary>
	   /// Removes the muscle with the specified joint and all muscles connected to it from PuppetMaster management. This will not destroy the body part/prop, but just release it from following the target.
	   /// If you call RemoveMuscleRecursive on an upper arm muscle, the entire arm will be disconnected from the rest of the body.
	   /// </summary>
	   /// <param name="joint">The joint of the muscle (and the muscles connected to it) to remove.</param>
	   /// <param name="attachTarget">If set to <c>true</c> , the target Transform of the first muscle will be parented to the disconnected limb.</param>
	   /// <param name="blockTargetAnimation">If set to <c>true</c>, will add AnimationBlocker.cs to the removed target bones. That will override animation that would otherwise still be writing on those bones.</param>
	   /// <param name="removeMode">Remove mode. Sever cuts the body part by disconnecting the first joint. Explode explodes the body part disconnecting all joints. Numb removes the muscles from PuppetMaster management, keeps the joints connected and disables spring and damper forces.</param>
		
		void					RemoveMuscleRecursive(ConfigurableJointComponent* joint, bool attachTarget, bool blockTargetAnimation = false, RagdollMuscleRemoveMode removeMode = RagdollMuscleRemoveMode::Sever);
		// TODO
		void					ReplaceMuscle(ConfigurableJointComponent* oldJoint, ConfigurableJointComponent* newJoint) { if (!CheckIfInitiated()) return; };	
		// TODO
		void					SetMuscles(std::vector<RagdollMuscle*> newMuscles) { if (!CheckIfInitiated()) return; };								
		// TODO
		void					DisableMuscleRecursive(ConfigurableJointComponent* joint) { if (!CheckIfInitiated()) return; };
		// TODO
		void					EnableMuscleRecursive(ConfigurableJointComponent* joint) { if (!CheckIfInitiated()) return; };
		// TODO
		//bool					AddPropMuscle(ConfigurableJointComponent* addPropMuscleTo, Vector3 position, Quaternion rotation, Vector3 additionalPinOffset, ActorComponent* targetParent = nullptr, RagdollProp* initiateWithProp = nullptr);
		// TODO
		bool					IsDisconnecting(int muscleIndex) { return disconnectMuscleFlags[muscleIndex]; };
		// TODO
		bool					IsReconnecting(int muscleIndex) { return reconnectMuscleFlags[muscleIndex]; };
		// TODO
		void					DisconnectMuscleRecursive(int index, RagdollMuscleDisconnectMode disconnectMode = RagdollMuscleDisconnectMode::Dis_Sever, bool deactivate = false);
		// TODO
		void					ReconnectMuscleRecursive(int index);
		// TODO
		void					AddMuscle(ConfigurableJointComponent* joint, WorldNode* target, RigidBodyComponent* connectTo, WorldNode* targetParent, RagdollMuscle::MuscleAttribute* muscleAttribute = nullptr, bool forceTreeHierarchy = false, bool forceLayers = true) {};

		void					BehavioursForeach(const std::function<void(RagdollBehaviorComponentBase*)>& impl);
		RagdollBehaviorManagerComponent*		
								GetBehavioursManager();

		void					SetRoleComponentPtr(const EchoWeakPtr<RoleComponent>& rolePtr);

		void					SetMaterialId(int value);
		void					SetSurfaceType(uint32 value);
		void					SetFilterGroupType(PhysXGroup value);
		void					SetCanInteractFilterGroupType(PhysXGroup value);
		void					SetAutoCalculateLocalPose(bool value);

	public: // 一些调试用的接口
		bool					IsValid_LastCalculateMappedVelocity() { return lastCalculateMappedVelocityIsValid; }

	public:
		// helperObject 相关
		enum HelperMode : uint16
		{
			FoceMode		= 0,
			TuqueMode 
		};
		bool					IsShowHelper();
		void					ShowHelper(bool show);
		void					UpdataHelper();
		void					SetHelperOffset(const Vector3& offset);
		const Vector3 &			GetHelperOffset();
		void					SetHelperMode(HelperMode mode);
		HelperMode				GetHelperMode();
	private:
		void					CreateHelper();
		void					DestroyHelper();
		HelperObject*			m_pHelperObject = nullptr;
		bool					m_bShowHelper = false;
		Vector3					m_vHelperObjectOffset = Vector3::ZERO;
		HelperMode				m_eHelperMode = FoceMode;

	private:
		// 私有函数
		//void					InitRagdollMuscle();
		//void					InitRagdollData();

		void					DisableRagdollBlock(bool disablechild = true);
		void					ActiveRagdollBlock();
		void					ToDeadImpl();
		void					ToAliveImpl();

		void					InitRagdoll();
		void					SetRoleIKCom(const std::string& comName);

						
		void					SlowDeath();
	
		void					SetAnimationEnabled(bool to);
		void					ActivateRagdoll(bool kinematic = false);
		bool					CanFreeze();

		void					ResetStateSettings() { stateSettings = RagdollStateSettings::Default; };

		//bool					isActive() { return initiated && (activeMode == RagdollMode::Active || isBlending()); };

		RagdollUpdateMode		updateMode() { return m_UpdateMode; };
		//bool					controlsAnimator() { return isActive() && initiated && updateMode() == RagdollUpdateMode::Mode_FixedUpdate; };
		void					SetInternalCollisionsManual(bool collide, bool useInternalCollisionIgnores);
		void					SetAngularLimitsManual(bool limited);

		bool					autoSimulate() { return true; };

		void					OnDisable() ;
		void					OnEnable();

		WorldNode*				FindTargetRootRecursive(WorldNode* t);
		void					EnableRoleUpdateBoxByAnim(bool Enable);

		void					Initiate();
		void					Destroy();

		void					OnDestroy() {};
		bool					IsInterpolated() { return false; };
		void					OnRebuild();

		void					FixedUpdate();
		void					Update();
		void					LateUpdate();
		void					OnLateUpdate();

		void					WaitModeToTagertMode();
		void					WaitModeCopyVelocity();

		void					Read();
		void					FixTargetTransforms();
		void					VisualizeTargetPose() {};
		void					VisualizeHierarchy(WorldNode* t, ColorValue color) {};

		void					MoveToTarget();

		void					FlagInternalCollisionsForUpdate();
		void					UpdateInternalCollisions();
		void					UpdateInternalCollisions(RagdollMuscle* m);
		void					IgnoreInternalCollisions();
		void					IgnoreInternalCollisions(RagdollMuscle* m);
		void					ResetInternalCollisions();
		void					ResetInternalCollisions(RagdollMuscle* m, bool useInternalCollisionIgnores);
		void					FlagAngularLimitsForUpdate();
		void					UpdateAngularLimits();

		void					Rebuild() { rebuildFlag = true; };

		void					FlattenHierarchy() { hierarchyIsFlat = true; };
		void					TreeHierarchy() { hierarchyIsFlat = false; };

		void					FixMusclePositions();
		void					FixMusclePositionsAndRotations();

		bool					HierarchyIsFlat();

		int						GetHighestDisconnectedParentIndex(int index);
		void					ProcessDisconnects();
		void					ProcessReconnects();
		void					OnDisconnectMuscleRecursive(int index, RagdollMuscleDisconnectMode disconnectMode = RagdollMuscleDisconnectMode::Dis_Sever, bool deactivate = false) {};
		void					DisconnectMuscle(RagdollMuscle* m, bool sever, bool deactivate) {};
		void					OnReconnectMuscleRecursive(int index);
		void					ReconnectMuscle(RagdollMuscle* m) {};
		void					AddIndexesRecursive(int index, std::vector<int>& indexes) {};

		void					DisconnectJoint(ConfigurableJointComponent* joint) {};
		void					KillJoint(ConfigurableJointComponent* joint);

		void					DisableImmediately() {};
		void					SlowActive();

		static RagdollComponent* SetUp(WorldNode* target, WorldNode* ragdoll, int characterControllerLayer, int ragdollLayer) { return nullptr; };
		static RagdollComponent* SetUp(WorldNode* target, int characterControllerLayer, int ragdollLayer) { return nullptr; };
		void					SetUpTo(WorldNode* setUpTo, int characterControllerLayer, int ragdollLayer) {};
		static void				RemoveRagdollComponents(WorldNode* target, int characterControllerLayer) {};
		void					SetUpMuscles(WorldNode* setUpTo){};
		void					RemoveUnnecessaryBones() {};

		void					SampleTargetMappedState();
		void					FixTargetToSampledState(float weight);
		void					StoreTargetMappedState();

		void					UpdateHierarchies();
		bool					HasProp();
		void					UpdateBroadcasterMuscleIndexes();
		void					AssignParentAndChildIndexes();
		ConfigurableJointComponent*	
								FindRigdiBodyToJoint(RigidBodyComponent* rigidBody);
		void					AddToParentsRecursive(ConfigurableJointComponent* joint, std::vector<int>& indexes);
		void					AddToChildrenRecursive(ConfigurableJointComponent* joint, std::vector<int>& indexes, std::vector<bool>& childFlags);
		void					AssignKinshipDegrees();
		void					AssignKinshipsDownRecursive(std::vector<int>& kinshipDegrees, int degree, int index);
		void					AssignKinshipsUpRecursive(std::vector<int>& kinshipDegrees, int degree, int index);
		int						GetMuscleIndexLowLevel(ConfigurableJointComponent* joint);

		bool					IsValid(bool log);
		bool					CheckMassVariation(float threshold, bool log) { return true; };
		bool					CheckIfInitiated() { return initiated; };
		void					RoleMove();
		void					SetRagdollEnableFlag(bool pFlag);

	//-----------------------------------------------------------Data----------------------------------------
	public:
		typedef std::function<void()> UpdateDelegate,StateDelegate;
		typedef std::function<void(RagdollMuscle*)> MuscleDelegate;

		StateDelegate	OnFreeze				= nullptr;
		StateDelegate	OnUnfreeze				= nullptr;
		StateDelegate	OnAlive					= nullptr;
		StateDelegate	OnDeath					= nullptr;
		StateDelegate   OnDeathStart			= nullptr;
		StateDelegate	OnResurrection			= nullptr;


		UpdateDelegate	OnPostInitiate			= nullptr;
		UpdateDelegate	OnRead					= nullptr;
		UpdateDelegate	OnWrite					= nullptr;
		UpdateDelegate	OnPostLateUpdate		= nullptr;
		UpdateDelegate	OnFixTransforms			= nullptr;
		UpdateDelegate	OnHierarchyChanged		= nullptr;

		MuscleDelegate	OnMuscleRemoved			= nullptr;
		MuscleDelegate	OnMuscleDisconnected	= nullptr;
		MuscleDelegate	OnMuscleReconnected		= nullptr;
	public:
		// 肌肉数据
		std::vector<ragdollBlockData>	mRagdollBlocks;
		//ActorComponentPtr				mRigidBodyGroup;
		std::vector<RagdollMuscle*>		muscles;		// 肌肉的顺序是重要的！！！被关节连接的肌肉节点必须在该关节节点前
	protected:
		// 静态数据
		Name					targetName;
		
		RagdollBehaviorMode		behaviorMode = RagdollBehaviorMode::Disabled;
		RagdollBehaviorMode		targetBehviorMode = RagdollBehaviorMode::Disabled;
		RagdollStateSettings	stateSettings = RagdollStateSettings::Default;
		float					blendTime = 0.f;
		bool					fixTargetTransforms = true;
		int						solverIterationCount = 10;
		float					mappingWeight = 1.0f;
		float					pinWeight = 1.0f;
		float					muscleWeight = 1.0f;
		float					muscleSpring = 50.0f;
		float					muscleDamper = 1.0f;
		float					pinPow = 4.0f;
		float					pinDistanceFalloff = 0.0f;
		float					maxAngularVelocity = 25.f;	// 限制角速度 25r/s 
		float					maxLinearVelocity = 60.f;	// 限制线速度 60m/s
		float					ragdollDensity = 1e3f;		// 布偶密度为 1000kg/m^3
		bool					angularPinning = false;
		bool					updateJointAnchors = true;
		bool					supportTranslationAnimation = false;
		bool					angularLimits = true;
		bool					internalCollisions = true;


		std::vector<RagdollBlockDesc>	blockDesc;

		
	private:
		// 动态数据
		bool					initiated = false;	// 初始化完成为true
		bool					start = false;	// awake后为true，准备初始化

		bool					angularLimitsEnabledOnKill = false;
		bool					internalCollisionsEnabledOnKill = false;

		bool					manualInternalCollisionControl = false;
		bool					manualAngularLimitControl = false;
		bool					mapDisconnectedMuscles = false;

		bool					internalCollisionsEnabled = true;
		bool					angularLimitsEnabled = true;
		bool					fixedFrame = true;
		int						lastSolverIterationCount = 1;
		bool					animatorDisabled = false;
		bool					interpolated = false;
		bool					hasBeenDisabled = false;
		bool					hierarchyIsFlat = true;

		bool					teleport = false;
		Vector3					teleportPosition;
		Quaternion				teleportRotation = Quaternion::IDENTITY;
		bool					teleportMoveToTarget = false;
		bool					rebuildFlag = false;
		bool					onPostRebuildFlag = false;
		std::vector<bool>		disconnectMuscleFlags;
		std::vector<RagdollMuscleDisconnectMode> muscleDisconnectModes;
		std::vector<bool>		disconnectDeactivateFlags;
		std::vector<bool>		reconnectMuscleFlags;

		std::vector<RagdollMuscle*> defaultMuscles;
		Vector3					rebuildPelvisPos;
		Quaternion				rebuildPelvisRot = Quaternion::IDENTITY;
		float					simulationDeltaTime = 0.f;

		bool					readInFixedUpdate = false;

		bool					mappingBlendIncreaseFlag = false;
		float					mappingBlend = 1.0f;

		bool					muscleWeightDecreaseFlag = false;
		float					weightRange = 0.f;
		float					weightMlp = 0.f;
		bool					muscleToFreeze = true;

		bool					storeTargetMappedState = true;
		bool					lastStoreTargetMappedStateIsValid = false;
		bool					lastCalculateMappedVelocityIsValid = false;

		bool					targetMappedStateStored = false;
		bool					targetMappedStateSampled = false;
		bool					sampleTargetMappedState = false;
		bool					hasProp = false;

		bool					isLoadData = false;	// 加载完成了data数据 可能会等待roleobj加载完成才为true
		RagdollUpdateMode		m_UpdateMode = Mode_Normal;
		Vector3					lastTargetRootPosition;
		Quaternion				lastTargetRootRotation;
	public:
		bool					isRoleMove = true;
		bool					isMoveCCT = false;
	private:
		RoleIKComponent*		mRoleIKCom = nullptr;
		ActorComponentPtr		targetRoot = nullptr;
	};

	class _EchoComponentExport RagdollComponentInfo : public ActorComponentInfo
	{
		friend class RagdollComponent;
	public:
		RagdollComponentInfo(const Name& mtype, ActorCOMMoveableType movableType = ALL_MOVEABLE);
		RagdollComponentInfo();
		virtual ~RagdollComponentInfo();
		virtual void						ExportData(cJSON* jsonNode)override;
		virtual void						ImportData(cJSON* jsonNode)override;
		virtual void						CopyTo(ActorComponentInfo* comInfo) override;
		virtual void						CopyToData(RagdollComponentData* data);
		virtual void						OverWrite()	override;
		virtual void						Overlay(ActorComponentInfo* comInfo) override {};
	private:
		class _EchoComponentExport _InfoModifyMuscle
		{
		private:
			RagdollComponentInfo* pInfo;
		public:
			_InfoModifyMuscle(RagdollComponentInfo* pInfo) :pInfo(pInfo){}
			RagdollMuscleModify& operator= (RagdollMuscleModify& modfiy);
		};
	public:
		struct BoneNodeDesc 
		{
			Name							jointTargetBoneName;
			Name							nodeName;
			Name							rigidBodyName;
			Name							colliderName;
			Name							jointName;
			RagdollMuscle::MuscleAttribute	muclesDesc;
		};
		typedef std::vector <std::pair<Name, BoneNodeDesc>> BoneNodeAndMucleInfo;

		std::string				roleName					= "";

		RagdollStateSettings	stateSettings				= RagdollStateSettings::Default;
		float					blendTime					= 0.1f;
		bool					fixTargetTransforms			= true;
		int						solverIterationCount		= 10;
		float					mappingWeight				= 1.0f;
		float					pinWeight					= 1.0f;
		float					muscleWeight				= 1.0f;
		float					muscleSpring				= 50.0f;
		float					muscleDamper				= 1.0f;
		float					pinPow						= 4.0f;
		float					pinDistanceFalloff			= 0.0f;
		float					maxAngularVelocity			= 25.f;	
		float					maxLinearVelocity			= 60.f;	
		float					ragdollDensity				= 1e3f;	
		bool					angularPinning				= false;
		bool					updateJointAnchors			= true;
		bool					supportTranslationAnimation = false;
		bool					angularLimits				= true;
		bool					internalCollisions			= true;
		BoneNodeAndMucleInfo	bnmInfo						;

		//Name					rigiBodyGroupName			;
		_InfoModifyMuscle		muscleModifier				;	// 不需要导入导出 用于自动更改bnmInfo的值(为了符合UI的机制)
		EDITOR_REFLECT
	};

	void _EchoComponentExport WriteJSNode(JSNODE jsonNode, const char* Name, const RagdollStateSettings& settings);
	bool _EchoComponentExport ReadJSNode(cJSON* jsonNode, const char* Name, RagdollStateSettings& settings);
	void _EchoComponentExport WriteJSNode(cJSON* jsonNode, const char* Name, const RagdollComponentInfo::BoneNodeAndMucleInfo& data);
	bool _EchoComponentExport ReadJSNode(cJSON* jsonNode, const char* Name, RagdollComponentInfo::BoneNodeAndMucleInfo& data);

	DEFINECOMFACTORY(RagdollComponent, ALL_MOVEABLE)
}

#endif // _EchoRagdollComponent_H__
