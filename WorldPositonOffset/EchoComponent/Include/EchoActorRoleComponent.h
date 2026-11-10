//-----------------------------------------------------------------------------
// File:  EchoActorRoleComponent.h
//
// Author: LangGuangyue
//
// Date:   2022-7-21
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef __EchoActorRoleComponent_H__
#define __EchoActorRoleComponent_H__

#include "EchoComponentPrerequisites.h"
#include "EchoProtocolComponent.h"
#include "Actor/EchoActor.h"
#include "EchoRoleObject.h"
#include "EchoAnimationState.h"
#include "EchoAnimationManager.h"
#include "EchoComDefineMacro.h"

namespace Echo
{
	class RoleComponentInfo;
	//=============================================================================
	class _EchoComponentExport RoleComponent : public ProtocolNodeActorComponent
	{
		DECLAREACTORCOMTYPE_H(RoleComponent)
	public:
		REGISTER_LUA_CLASS_KEY;

		virtual void					init(ActorComponentInfo* info) override;
		virtual void					unInit() override;
		virtual void					bindNode() override;
		virtual void					replay(ActorComponentInfo* info) override;
		virtual	void					setActor(const ActorPtr&  pActor) override;
		virtual void					copyTo(const ActorComponentPtr&  actCom) override;
		virtual void					updateCombinedName() override;

		virtual void					registerTickTimeBegin() override {};
		virtual void					beginFrame(EventID, ProtocolHandle) override {};
		virtual void					endFrame(EventID, ProtocolHandle) override {}
		virtual void					preUpdateAnimation(EventID, ProtocolHandle) override {}
		virtual void					postUpdateAnimation(EventID, ProtocolHandle) override {}
		virtual void					updateActiveTypeState(ActorComActiveFlags inVal, ActorComActiveFlags inChangeMask) override;
		virtual void					updatePortalCullType(PortalCullType cullType) override;


		virtual void					preUpdateScene(EventID, ProtocolHandle) override {}
		virtual void					postUpdateScene(EventID, ProtocolHandle) override {}
		virtual void					preRenderScene(EventID, ProtocolHandle) override {}
		virtual void					postRenderScene(EventID, ProtocolHandle) override {}

		virtual void							visibleStateChanged(bool visibleState)override;
		virtual void							createComRootNodeImpl() override;
		virtual void							DeleteComRootNodeImpl() override;

		//加载时初始化角色相关内容
		void							setRole(RoleComponentInfo* roleInfo);
		void							showRole();
		void							setRolePath(const std::string& path);

		void							ChangeRole(const std::string& path, bool synchronous = false,bool useBonePart = false, uint16 bonePartId = 0);
		void							ChangeRole(RoleObjectDesc& roleDesc);
		
		// 设置RoleObj根骨骼的RootMotion模式
		void							SetRootBoneMotion(uint32 pMotionType); // enum RootMotionType [head file is <EchoBone.h>]
		uint32							GetRootBoneMotion();

		Vector3							GetRootMotionPositionOffset();
		Quaternion						GetRootMotionRotationOffset();
		Vector3							GetRootMotionScaleOffset();
		void							GetRootMotionOffset(Vector3 &pos, Quaternion &rot, Vector3& scale);
		//获取角色皮肤资源
		void							GetRoleEquipName();
		//获取角色动画资源
		void							GetRoleAnimationName();

		//更换皮肤
		void							SetRoleEquip(const std::string& name);
		//播放动画
		void							SetRoleAnimation(const Name& name, bool isLoop, uint8 animationIndex = 0, AnimationStateListener* listener = 0,
			float startTime = 0.0f, float endTime = 0.0f, bool synchronous = false, bool keep = false, float weight = 1.0f, bool bFileName = false, uint16 blendmaskID = 0,
			int channel = 0, AnimationStateType type = AnimationStateType::BaseAnimation, uint8 additiveIndex = 0, uint8 additiveBlendIndex = 0);
		
		//清除动画
		void							ClearAnimation();
		//停止播放动画
		void							StopAnimation(const std::string& aniName);
		//暂停动画
		void							SetAnimationPause(const Name& aniName, bool isPause);
		void							SetAnimationAllPause(bool isPause);
		//动画步进播放（逐帧）
		void							SetPlayAnimation(const Name& aniName, float time = 1.0f);

		//设置动画
		void							SetRoleAnimation_1(const std::string& name);
		void							SetRoleAnimation_2(const std::string& name);
		
		//设置动画权重
		void							SetAnimationWeight(const Name& name, float weight);
		//消融接口
		void							SetRoleDissolveStrength(float val);
		float							GetRoleDissolveStrength();
		//设置溶解描边属性
		void							SetRoleDissolveEdgeColor(Vector3 _val);
		void							SetRoleDissolveColorStrength(float _val);
		void							SetRoleDissolveTillingU(float _val);
		void							SetRoleDissolveTillingV(float _val);
		void							SetRoleDissolveEdgeWidth(float _val);
		void							SetRoleDissolveSoftValue(float _val);
		void							SetRoleDissolveNoiseTexture(Echo::String _texture);

		bool							IsAnimationStop(const Name& animation);

		void							BonePreview();
		bool							MatchAnimation();
		void							MatchAnimation(Name& animation_1, Name& animation_2);

		Echo::EntityMap					getLowestEntityMap();

		std::set<Name>					EquipList() { return mRoleEquipNameList; };
		std::set<Name>					AnimationList() { return mRoleAnimationList; };
		AnimationItemArray				AnimationArray() { return mAnimationItemArray; };

		bool							IsDel() { return isDeling; };

		RoleObject*						GetRole();
		Role*							GetRoleInfo();
		const Name&						GetRolePath() { return mRolePath; };
		const Name&						GetRoleEquipname() { return mRoleEquipName; };
		const Name&						GetRoleAnimationName_1() { return mRoleAnimationName_1; };
		const Name&						GetRoleAnimationName_2() { return mRoleAnimationName_2; };

		void							CreateBoneComponents();

		void							SetClientLogMessage(const Name& str) { mClientLogMessage = str; };

		// 重载角色包围盒 开启后无法关闭(运行时数据，不做保存)
		void							SetCustomRoleAABB(const Vector3& min , const Vector3& max);
		bool							GetCustomRoleAABB(Vector3& min, Vector3& max);
		bool							GetCustomRoleAABB();

#ifdef ECHO_EDITOR
		virtual void							getErrorMessageImpl(String& message) override;
		virtual bool							hasErrorMessageImpl() override;
#endif
		// 骨骼组功能begin
		uint16							BonePartGetId() { return mSelectedBonePartId; };
		bool							BonePartSwitch() { return mUseBonePart; };
		SkeletonInstance*				GetSkeletonInstance();

#ifdef ECHO_EDITOR
		// 骨骼组编辑操作
		void							BonePartSwitch(bool pFlag);
		void							BonePartToggle(uint16 pBonePartId);
#endif 
		// 骨骼组功能end
	public:
		//RoleComponentInfo*				mRoleInfo = nullptr;

		float							mDissolveStrength = 0.0f;
	private:
		void							createRole(const std::string& path, bool synchronous = false, bool useBonePart = false, uint16 bonePartId = 0);
		void							createRole(RoleObjectDesc& roleDesc);
		void							removeRole();
		void							CreateBoneComponent(ActorComponentPtr  pParentComp, Bone* pSelfBone);
	protected:
		virtual void					EditorPlayImp() override;
		virtual void					EditorPauseImp(bool pause) override;
		virtual void					EditorStopImp() override;
	private:
		class SetCustomRoleAABBListener : public RoleLoadListener
		{
		public:
			virtual bool OnRoleCreateFinish();
			SetCustomRoleAABBListener(RoleComponent* comp) :pComp(comp) {};
		private:
			RoleComponent* pComp ;
		};
		void							_SetCustomRoleAABBImpl();
	private:
		SetCustomRoleAABBListener		mCustomRoleAABBCallBack {this};
		bool							mbIsCustomRoleAABB = false;
		Vector3							mCustomRoleAABB[2];
	private:
		RoleObject*						m_pRole = nullptr;

		bool							isDeling = false;
	protected:
		Name							mRolePath = Name("");
		Name							mRoleEquipName = Name("");
		Name							mRoleAnimationName_1 = Name("");
		Name							mRoleAnimationName_2 = Name("");

		std::set<Name>					mRoleEquipNameList;				
		std::set<Name>					mRoleAnimationList;
		AnimationItemArray				mAnimationItemArray;

		Name							mClientLogMessage = Name::g_cEmptyName;

		uint32							mRootBoneMotionType = 0;
		uint16							mSelectedBonePartId = 0;
		bool							mUseBonePart		= false;
	};

	class _EchoComponentExport RoleComponentInfo : public ActorComponentInfo
	{
		friend class RoleComponent;
	public:
		RoleComponentInfo() {};
		RoleComponentInfo(const Name& mtype, ActorCOMMoveableType movableType = ALL_MOVEABLE);
		virtual ~RoleComponentInfo();
		virtual void					ExportData(cJSON* jsonNode)override;
		virtual void					ImportData(cJSON* jsonNode)override;
		virtual void					CopyTo(ActorComponentInfo* comInfo) override;
		virtual void					Overlay(ActorComponentInfo* comInfo) override;
		virtual void					OverWrite() override;

#ifdef ECHO_EDITOR
		virtual bool					getNeedCollectedResource(std::unordered_multimap<ResourceType, std::string>& resMap) override;
#endif

		void							ReleaseInfo();

	public:
		std::string m_RolePath = "";
		std::string m_RoleEquipName = "";

		float m_DissolveStrength = 0.0f;

		std::string m_RoleAnimationName_1 = "";
		std::string m_RoleAnimationName_2 = "";

		// 角色组件骨骼分块功能
		uint16 m_SelectedBonePartId = 0;
		bool   m_UseBonePart = false;			

		EDITOR_REFLECT
	};

	class EchoRoleComScript : public EchoActorComponentScript
	{
	public:
		REGISTER_LUA_CLASS_KEY;
	public:
		EchoRoleComScript() = default;
		EchoRoleComScript(const EchoActorScript& pActor, const std::string& name) :
			EchoActorComponentScript(pActor, name) {}

		static EchoRoleComScript GetRoleComponent(const EchoActorScript& pActor, const std::string& name);
	};

	DEFINECOMFACTORY(RoleComponent,ALL_MOVEABLE);
}

#endif // !__EchoRoleComponent_H__
