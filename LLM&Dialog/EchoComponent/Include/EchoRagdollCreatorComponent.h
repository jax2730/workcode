//-----------------------------------------------------------------------------
// File:  EchoRagdollCreatorComponent.h
//
// Author: LangGuangyue
//
// Date:   2022-9-21
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef _EchoRagdollCreatorComponent_H__
#define _EchoRagdollCreatorComponent_H__

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
#include "EchoRagdollComponent.h"
#include "EchoPhysXJoint.h"
#include "EchoComDefineMacro.h"
#include "EchoRagdollData.h"
#include <variant>
namespace Echo
{
#define WorldPos(_node)		_node->_getDerivedPosition()
#define WorldQuat(_node)	_node->_getDerivedOrientation()
	enum COLLIDER_SHAPE
	{
		Box,		//盒子
		Capsule		//胶囊
	};

	enum Creator_Mode
	{
		Automation,	//自动
		Manual,		//手动
		Nomal
	};

	struct RagdollBoneData
	{
		//Name
		std::string					name;	// 模板名
		std::string					nodeName;
		//Bone
		std::string					boneName;
		WorldNode*					mBone;
		//Collider
		COLLIDER_SHAPE				colliderShape;
		std::string					beginBoneName;
		std::string					endBoneName;
		//RigidBody
		bool						IsKinematic = true;
		//Joint
		std::string					jointConnectedName;
		bool						IsSetJointMotion = true;
	};

	// 用于ragdoll初始化
	class _EchoComponentExport RagdollComponentInitData : public ResourceBackgroundQueue::Listener
	{
	public:
		RagdollComponentInitData();
		~RagdollComponentInitData();
		RagdollComponentInitData(const RagdollComponentInitData& _lData);
		RagdollComponentInitData& operator= (const RagdollComponentInitData& _lData);
	public: // 子结构定义
		struct MoveableComponentSimpleData
		{
			Vector3 mPosition	{};
			Vector3 mRotation	{};
			Vector3 mScale		{1.f,1.f,1.f};
		};

		struct RagdollComponentSimpleData 
			: public RagdollComponentData
			, public MoveableComponentSimpleData {};
		
		struct RagdollComponentBlockNodeData
			: public MoveableComponentSimpleData {};

		typedef struct RagdollComponentBlockBoxCollsionSimpleData
		{
			Vector3		mHalfSize {};
		} BoxTag;

		typedef struct RagdollComponentBlockCapsuleCollsionSimpleData
		{
			float		mRadius { 0.f };
			float		mHeight	{ 0.f };
		} CapsuleTag;

		struct RagdollComponentBlockCollsionData
			: public MoveableComponentSimpleData
		{
			std::variant< BoxTag, CapsuleTag>	mCollsionImpl				{};
			bool								mIsTrigger					{ false };
			int									mMaterialId					{ 0 };
			uint32								mFilterGroupType			{ 0xffffffff };
			uint32								mCanInteractFilterGroupType	{ 0xffffffff };
		};
		
		struct RagdollComponentBlockRigdBodyData
			/*: public MoveableComponentSimpleData*/
		{
			bool	mUseGravity			{ false };
			//bool	mIsKinematic		{ false };
			int		mCollisionDetection	{ EchoPhysics::Discrete };
			int		mConstraints		{ EchoPhysics::FreezeNone };
			float	mMass				{ 1.0f };
			float	mDrag				{ 0.0f };
			float	mAngularDrag		{ 0.05f };
		};

		struct RagdollComponentBlockJointData
			: public MoveableComponentSimpleData
		{
			Vector3		mAxisOffset			{  };
			float		mBreakForce			{ FLT_MAX };
			float		mBreakTorque		{ FLT_MAX };
			bool		mEnableProjection	{ false };
			bool		mEnableCollision	{ false };
			bool		mEnablePreprocessing{ true };
			float		mMassScaleSelf		{ 1.f };
			float		mMassScaleConnected	{ 1.f };

			float		mLinearLimit		{ 0.f };
			float		mAngularXLowLimit	{ 0.f };
			float		mAngularXHighLimit	{ 0.f };
			float		mAngularYLimit		{ 0.f };
			float		mAngularZLimit		{ 0.f };
			JointLimit	mLinearLimitParm	{  };
			JointLimit	mAngularXLimitParm	{  };
			JointLimit	mAngularYZLimitParm	{  };
			JointDrive	mDriveX				{  };
			JointDrive	mDriveY				{  };
			JointDrive	mDriveZ				{  };
			JointDrive	mAngularDriveX		{  };
			JointDrive	mAngularDriveYZ		{  };
			JointDrive	mSlerpDrive			{  };
			Vector3		mTargetPosition		{  };
			Vector3		mTargetVelocity		{  };
			Vector3		mTargetRotation		{  };
			Vector3		mTargetAngularVelocity {  };
			RDM			mRotationDriveMode	{ RDM::XAndYZ };
			float		mProjectionDistance	{ 0.1f };
			float		mProjectionAngle	{ 180.f };
			bool		mLinearLimitScale	{ false };
			JointMotion	mMotionX			{ JointMotion::FREE };
			JointMotion	mMotionY			{ JointMotion::FREE };
			JointMotion	mMotionZ			{ JointMotion::FREE };
			JointMotion	mMotionAX			{ JointMotion::FREE };
			JointMotion	mMotionAY			{ JointMotion::FREE };
			JointMotion	mMotionAZ			{ JointMotion::FREE };

			String		mConnectedComName	{  };
			String		mSelfComName		{  };
		};

		struct RagdollComponentBlockData
		{
			std::string							mBoneName		{};
			std::string							mTargetBoneName	{};

			RagdollMuscle::MuscleGroup			mGroup			{ RagdollMuscle::MuscleGroup::Hips };
			float								mMapping		{ 1.f };			// [0.f ~ 1.f]
			float								mPin			{ 1.f };			// [0.f ~ 1.f] 
			float								mWeight			{ 1.f };			// [0.f ~ 1.f] 
			float								mDamper			{ 1.f };			// [0.f ~ 1.f]

			RagdollComponentBlockNodeData		mNodeData		{};
			RagdollComponentBlockCollsionData	mCollsionData	{};
			RagdollComponentBlockRigdBodyData	mRigdBodyData	{};
			RagdollComponentBlockJointData		mJointData		{};
		};

	public: // 成员数据
		RagdollComponentSimpleData				mSimpleData{};
		vector<RagdollComponentBlockData>::type	mBlocksData{};
	public: // 导入导出
		typedef std::function<void(bool, RagdollComponentInitData&)> AsynCompletedCallBack;

		// 同步导入路径中的布偶数据，数据无法读取/已经加载或正在加载数据时返回false
		bool SyncImport(const std::string& ragdollDataPath);

		// 异步导入路径中的布偶数据，已经加载过/正在加载不同路径的数据时返回false。
		// 如果加载过相同路径的数据，会立即执行回调。
		// 导入完成后调用回调。第一个参数为导入是否成功，第二个参数为*this
		// 导入确定失败之后才可以再次导入。且IsEmpty会再次返回true
		bool AsynImport(const std::string& ragdollDataPath,const AsynCompletedCallBack &finishCallBack);
		
		// 如果可能，取消异步导入的请求。之后将不在调用此前添加的回调
		// 如果成功取消了一个正在进行的异步导入。返回true。**但是无法保证成员数据没有被修改!**
		// 如果一个未完成的异步导入被取消了。可以再次导入。
		void AbortAsyn();

		// 加载完成时返回true
		bool IsImportCompleted() const;
		// 正在加载时返回true
		bool IsImportDoing() const;
		// 当前无加载
		bool IsEmpty() const;

		// 返回加载完成或正在加载的路径，如果无加载。返回nullptr
		const char* GetImportPath() const;
#ifdef ECHO_EDITOR
		// 同步导出布偶数据
		// 现在将强制更改后缀名为 .ragdolldata
		// 无法变更
		bool SyncExport(const std::string& ragdollDataPath) const;
#endif
	protected:
		void operationCompleted(BackgroundProcessTicket ticket, const ResourceBackgroundQueue::ResourceRequest& request, const ResourceBasePtr& resourcePtr) override;
	protected:
		RagdollDataPtr							mResPtr			{};
		BackgroundProcessTicket					mAynsTicket		{};
		AsynCompletedCallBack					mAynsCallBack	{};
	};

	class RagdollCreatorCallBack : public RoleLoadListener
	{
	public:
		EchoWeakPtr<RoleComponent>	mRoleCom;
		std::function<bool()>		mFinishCallBack		= nullptr;
		virtual bool OnRoleCreateFinish() override { if (mFinishCallBack) return mFinishCallBack(); else return true; }
	};


	class _EchoComponentExport RagdollCreatorComponentInfo : public ActorComponentInfo
	{
		friend class RagdollCreatorComponent;
	public:
		RagdollCreatorComponentInfo(const Name& mtype, ActorCOMMoveableType movableType = ALL_MOVEABLE);
		virtual ~RagdollCreatorComponentInfo();
		virtual void						ExportData(cJSON* jsonNode)override;
		virtual void						ImportData(cJSON* jsonNode)override;
		virtual void						CopyTo(ActorComponentInfo* comInfo) override;
		virtual void						Overlay(ActorComponentInfo* comInfo) override {};
		virtual void						OverWrite() override;
	};
	//---------------------------------------------------------------------------------
	class _EchoComponentExport RagdollCreatorComponent : public ProtocolNodeActorComponent
	{
		DECLAREACTORCOMTYPE_H(RagdollCreatorComponent)
	public:
		virtual void						init(ActorComponentInfo* info) override;
		virtual void						bindNode() override;
		virtual void						replay(ActorComponentInfo* info) override;
		virtual	void						setActor(const ActorPtr& pActor) override;
		virtual void						copyTo(const ActorComponentPtr& actCom) override;
		virtual void						dealComponentDelete(const ActorComponentPtr& actCom) override;

		virtual void					registerTickTimeBegin() override {};
		virtual void					beginFrame(EventID, ProtocolHandle) override {};
		virtual void					endFrame(EventID, ProtocolHandle) override {}
		virtual void					preUpdateAnimation(EventID, ProtocolHandle) override {}
		virtual void					postUpdateAnimation(EventID, ProtocolHandle) override {}

		virtual void					preUpdateScene(EventID, ProtocolHandle) override {}
		virtual void					postUpdateScene(EventID, ProtocolHandle) override {}
		virtual void					preRenderScene(EventID, ProtocolHandle) override {}
		virtual void					postRenderScene(EventID, ProtocolHandle) override {}

		//Creator
	public:
		//void								GetRoleComList();
		void								GetRoleCom(const String& comName);
		void								SetRoleComponentPtr(const EchoWeakPtr<RoleComponent>& rolePtr);
		RoleComponent* GetMyRole() { return mRoleCom.get(); };
		RagdollComponent* GetRagdoll() { return mRagdollComponent.get(); }
		//void								GetSkeletonList(RoleComponent* roleCom);
		void								GetBonePosition();
		void								CreateRagdoll();
		void								DestroyRagdoll();
		void								ChangeRagdollCollider();
		void								ChangeRagdollCollider(float LengthOverlap);
		void								CreatorEnd();

		bool								IsInitiated();

		bool								AddBoneData(std::string	name);
		bool								BoneHad(std::string boneName);
		void								DelTemplatePreview(std::string templateName);
		void								PreviewRagdoll();
		void								PresetMuscles();
		// 默认使用RoleComponent为父节点
		void								SetRagdollParent(ActorComponentPtr parentPtr) { mRagdollParentPtr = parentPtr; }
		ActorComponentPtr					GetRagdollParent() { return mRagdollParentPtr; };
		// 暂时使用同步的方式 
		// runtime == true  或者指定父节点为runtime模式，则加载的布偶组件为runtime模式
		// runtime == false 并且指定父节点非runtime模式，则加载的布偶组件非runtime模式
		ActorComponentPtr					loadFileData(const Name& fileName, bool runtime = false);
		ActorComponentPtr					loadFileData( RagdollComponentInitData& data, bool runtime = false);
	private:
	//布偶编辑器相关
	public:
		bool								IsDel() { return isDel; };
		bool								IsChange() { return isChange; };
		bool								IsEnd() { return is_end; }
		void								ClearRagdoll(RagdollComponent* ragdoll);
	private:
		void								SortTemplateDataList(); // 需要保证父节点在子节点的前面
		void								InitColliderList();
		//init
		void								InitShape();
		void								InitTorso();
		//Muslce
		void								InitMuscle();
		//Creator
		void								InitCreator(RagdollComponent* ragdoll, BipedIKBoneType type);
		//Joint
		void								InitJoint();
		//RigidBody
		void								InitRigidBody();
		//collider
		void								SetCollider();
		void								CreateCollider(BipedIKBoneType collider, Vector3 startPoint, Vector3 endPoint, COLLIDER_SHAPE colliderType, float lengthOverlap, float width);
		void								CreateCollider(BipedIKBoneType collider, Vector3 startPoint, Vector3 endPoint, COLLIDER_SHAPE colliderType, float lengthOverlap, float width, float proportionAspect, Vector3 widthDirection);
		void								CreateHandCollider(BipedIKBoneType collider, BipedIKBoneType arm, COLLIDER_SHAPE colliderType);
		void								CreateFootCollider(BipedIKBoneType collider, BipedIKBoneType leg, COLLIDER_SHAPE colliderType);
		Vector3								GetQuaternionToDirection(Quaternion r, Vector3 direction);
		Vector3								GetLocalOrthoDirection(WorldNode* transform, Vector3 worldDir);
		
	public:
		Creator_Mode											mode = Nomal;
		RagdollCreatorComponentInfo*							mInfo;
		//std::vector<RoleComponent*>								mRoleList;
		std::vector<Bone*>										mSkeletonList;

		//双足人形角色模板
		std::map<BipedIKBoneType, WorldNode*>					mColliderList;
		std::unordered_map<BipedIKBoneType, NodeComponent*>		mNodeList;
		std::unordered_map<BipedIKBoneType, Vector3>			mColliderPosition;
		std::unordered_map<BipedIKBoneType, COLLIDER_SHAPE>		mColliderShape;
		std::unordered_map<BipedIKBoneType, float>							mRigidBodyMass;
		std::unordered_map<BipedIKBoneType, std::vector<float>>			mJointLimitPairList;
		unordered_map<Name, BipedIKBoneType>::type				BoneTypecast;
		bool													is_create = false;
		bool													is_end = false;
		//float													weight = 75.00f;
		bool													spine = false;
		bool													chest = true;
		bool													hands = true;
		bool													feet = false;
		float													jointRange = 1.00f;
		float													colliderLengthOverlap = 0.10f;

		std::string												root_name = "Bip001";
		std::string												hips_name = "Bip001 Pelvis";
		std::string												spine_name = "Bip001 Spine";
		std::string												chest_name = "Bip001 Spine2";
		std::string												head_name = "Bip001 Head";
		std::string												LupLeg_name = "Bip001 L Thigh";
		std::string												LlowLeg_name = "Bip001 L Calf";
		std::string												RupLeg_name = "Bip001 R Thigh";
		std::string												RlowLeg_name = "Bip001 R Calf";
		std::string												Lhand_name = "Bip001 L Hand";
		std::string												Rhand_name = "Bip001 R Hand";
		std::string												Lfoot_name = "Bip001 L Foot";
		std::string												Rfoot_name = "Bip001 R Foot";
		std::string												LupArm_name = "Bip001 L UpperArm";
		std::string												LlowArm_name = "Bip001 L Forearm";
		std::string												RupArm_name = "Bip001 R UpperArm";
		std::string												RlowArm_name = "Bip001 R Forearm";

		COLLIDER_SHAPE											collider_torso = Box;
		COLLIDER_SHAPE											collider_head = Capsule;
		COLLIDER_SHAPE											collider_hand = Capsule;
		COLLIDER_SHAPE											collider_arm = Capsule;
		COLLIDER_SHAPE											collider_leg = Capsule;
		COLLIDER_SHAPE											collider_foot = Box;

		//自定义模板
		std::list<RagdollBoneData*>								mTemplateDataList;
		std::string												m_delPreviewName;
		std::string												m_rootBone;
	private:
		EchoWeakPtr<RoleComponent>								mRoleCom;
		RagdollComponentPtr										mRagdollComponent = nullptr;
		bool													isDel = false;
		bool													isChange = false;
		ActorComponentPtr										mRagdollParentPtr = nullptr;
		RagdollCreatorCallBack									mRagdollCreatorCallBack;
	private:
		// ragdoll static data and block data
		Name							roleName;
		RagdollComponentData			ragdollData;
		std::vector<RagdollBlockDesc>	blockData;

	};
	DEFINECOMFACTORY(RagdollCreatorComponent, ALL_MOVEABLE)
}

#endif // _EchoRagdollCreatorComponent_H__
