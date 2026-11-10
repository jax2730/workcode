//-----------------------------------------------------------------------------
// File:  EchoActorRoleComponent.cpp
//
// Author: LangGuangyue
//
// Date:   2022-7-21
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include "EchoComponentStableHeaders.h"
#include "EchoCommonStableHeaders.h"
#include "EchoActorRoleComponent.h"
#include "EchoActorProFuncImp.h"
#include "EchoSkeletonInstance.h"
#include "EchoBoneRenderComponent.h"
#include "EchoResourceFolderManager.h"
#include "Actor/EchoActorHelper.h"
namespace Echo
{
	DECLAREACTORCOMTYPE_CPP(RoleComponent)

	EDITOR_REFLECT_BEGIN(RoleComponentInfo)
	EDITOR_REFLECT_MEMBER_UPDATE(m_RolePath, "Role", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_RoleEquipName, "EquipName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_DissolveStrength, "DissolveStrength", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_RoleAnimationName_1, "AnimationName_1", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_RoleAnimationName_2, "AnimationName_2", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_SelectedBonePartId, "SelectedBonePartId", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_UseBonePart, "UseBonePart", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)

	EDITOR_REFLECT_END();

	RoleComponentInfo::RoleComponentInfo(const Name& mtype,ActorCOMMoveableType movableType)
		:ActorComponentInfo(mtype, movableType)
		, m_RoleAnimationName_1("")
		, m_RoleAnimationName_2("")

	{
	}

	RoleComponentInfo::~RoleComponentInfo()
	{
	}

	void RoleComponentInfo::ExportData(cJSON * jsonNode)
	{
		reflect::TypeResolver<RoleComponentInfo>::get()->ExportJson(this, jsonNode, "");
	}

	void RoleComponentInfo::ImportData(cJSON * jsonNode)
	{
		reflect::TypeResolver<RoleComponentInfo>::get()->ImportJson(this, jsonNode, "");
	}

	void RoleComponentInfo::CopyTo(ActorComponentInfo * comInfo)
	{
		RoleComponentInfo* roleInfo = static_cast<RoleComponentInfo*>(comInfo);
		reflect::TypeResolver<RoleComponentInfo>::get()->copyTo(this, roleInfo);
	}

	void RoleComponentInfo::Overlay(ActorComponentInfo * comInfo)
	{
	}

	void RoleComponentInfo::OverWrite()
	{
		if (mOriginComInfo != nullptr)
		{
			RoleComponentInfo* oriInfo = static_cast<RoleComponentInfo*> (mOriginComInfo);
			reflect::TypeResolver<RoleComponentInfo>::get()->overWrite(oriInfo, this);
		}
	}
#ifdef ECHO_EDITOR
	bool RoleComponentInfo::getNeedCollectedResource(std::unordered_multimap<ResourceType, std::string>& resMap)
	{
		if (m_RolePath.empty()) {
			return false;
		}
		resMap.insert(std::make_pair(ResourceType::Role, m_RolePath));
		return true;
	}
#endif

	void RoleComponentInfo::ReleaseInfo()
	{
		m_RolePath = "";
		m_DissolveStrength = 0.0f;
		m_RoleEquipName = "";
		m_RoleAnimationName_1 = "";
		m_RoleAnimationName_2 = "";
		m_SelectedBonePartId = 0;
		m_UseBonePart = false;
	}

	RoleComponent::RoleComponent(ActorPtr  pActor, ActorCOMMoveableType movableType)
		: ProtocolNodeActorComponent(pActor, movableType)
	{

		RoleObjectDesc desc;
		desc.clientLogMessage = mClientLogMessage;
		desc.pWorldMgr = getWorldManager();
		m_pRole = new RoleObject(desc);
	}

	RoleComponent::~RoleComponent()
	{

	}

	void RoleComponent::init(ActorComponentInfo * info)
	{
		ProtocolNodeActorComponent::init(info);
		m_pActComInfo = info;
		RoleComponentInfo* mRoleInfo = static_cast<RoleComponentInfo*>(m_pActComInfo);
		if (mRoleInfo != NULL)
		{
			setRole(mRoleInfo);
		}
		if (!mRolePath.empty())
		{
			createRole(mRolePath.toString(), getActor()->isSyncLoadActor(),mUseBonePart, mSelectedBonePartId);
			GetRoleEquipName();
			GetRoleAnimationName();
			showRole();
			SetRoleAnimation(mRoleAnimationName_1,true,0);
			SetRoleAnimation(mRoleAnimationName_2, true, 0);
		}
	}

	void RoleComponent::unInit()
	{
		ProtocolNodeActorComponent::unInit();
		isDeling = true;
		removeRole();
	}

	void RoleComponent::bindNode()
	{
		if (m_pParentNodeCom == nullptr)
			return;
		if (m_pRole != NULL && m_pRole->getSceneNode()->getParent() == nullptr)
		{
			//addChildToNode(m_pRole->getSceneNode());
			UpdateComRootNode();
		}
	}

	void RoleComponent::replay(ActorComponentInfo * info)
	{
		m_pActComInfo = info;
		RoleComponentInfo* mRoleInfo = static_cast<RoleComponentInfo*>(m_pActComInfo);
		if (mRoleInfo != NULL)
		{
			setRole(mRoleInfo);
		}
		if (!mRolePath.empty())
		{
			createRole(mRolePath.toString(), getActor()->isSyncLoadActor());
			GetRoleEquipName();
			GetRoleAnimationName();
			showRole();
		}
	}

	void RoleComponent::setActor(const ActorPtr&  pActor)
	{
		if (pActor == m_pActor)
			return;
		m_pActor = pActor;
		if (m_pActor == nullptr)
			return;
	}

	void RoleComponent::copyTo(const ActorComponentPtr&  actCom)
	{
	}

	void RoleComponent::updateActiveTypeState(ActorComActiveFlags inVal, ActorComActiveFlags inChangeMask)
	{
		if (m_pRole)
		{
			if (inChangeMask & ActorComActiveBits::CASTSHADOW_BIT)
				m_pRole->enableCastShadow(!(inVal & ActorComActiveBits::CASTSHADOW_BIT));
			if (inChangeMask & ActorComActiveBits::SHOWENTITY_BIT)
				m_pRole->setShowEntity(!(inVal & ActorComActiveBits::SHOWENTITY_BIT));
		}
	}

	void RoleComponent::updatePortalCullType(PortalCullType cullType)
	{
		if (m_pRole)
		{
			m_pRole->setPortalCullType(cullType);
		}
	}

	void RoleComponent::updateCombinedName()
	{
		if (m_pRole == NULL)
			return;
		Name roleName = Name(getCombinedName().append("||")) + mRolePath;
		m_pRole->setName(roleName);
	}

	void RoleComponent::visibleStateChanged(bool visibleState)
	{
		if (m_pRole)
		{
			if (ActorSystem::instance()->isHideRenderObject())
			{
				m_pRole->setParentVisible(false);
				return;
			}

			m_pRole->setParentVisible(visibleState);
		}
	}

	void RoleComponent::createComRootNodeImpl()
	{
		mComRootNode = m_pRole->getSceneNode();
	}

	void RoleComponent::DeleteComRootNodeImpl()
	{
		SAFE_DELETE(m_pRole);
		mComRootNode = nullptr;
	}

	void RoleComponent::setRole(RoleComponentInfo* roleInfo)
	{
		if (roleInfo == nullptr)
			return;
		mRolePath = roleInfo->m_RolePath;
		mRoleEquipName = roleInfo->m_RoleEquipName;
		mRoleAnimationName_1 = roleInfo->m_RoleAnimationName_1;
		mRoleAnimationName_2 = roleInfo->m_RoleAnimationName_2;
		mDissolveStrength = roleInfo->m_DissolveStrength;
		mSelectedBonePartId = roleInfo->m_SelectedBonePartId;
		mUseBonePart = roleInfo->m_UseBonePart;
	}

	void RoleComponent::showRole()
	{
		if(!mRoleEquipName.empty())
			SetRoleEquip(mRoleEquipName.toString());

		SetRoleDissolveStrength(mDissolveStrength);
	}

	void RoleComponent::setRolePath(const std::string & path)
	{
		mRolePath = path;
	}

	void RoleComponent::ChangeRole(const std::string & path, bool synchronous/* = false*/, bool useBonePart /*= false*/, uint16 bonePartId/* = 0*/)
	{
		if (useBonePart)
		{
			// 客户端直接调用该函数时,需要初始化变量
			mSelectedBonePartId = bonePartId;
			mUseBonePart = useBonePart;
		}
		mRootBoneMotionType = 0;
		removeRole();
		createRole(path, synchronous, mUseBonePart, mSelectedBonePartId);
	}

	void RoleComponent::SetRootBoneMotion(uint32 pMotionType)
	{
		RoleRootMotionComponent* lRoleRootMotion = m_pRole == nullptr? nullptr: m_pRole->getRootMotionComponent();
		if (lRoleRootMotion == nullptr)  return;
		mRootBoneMotionType = pMotionType;
		lRoleRootMotion->setRootMotionType(mRootBoneMotionType); 
	}

	uint32 RoleComponent::GetRootBoneMotion()
	{
		return mRootBoneMotionType;
	}

	void RoleComponent::ChangeRole(RoleObjectDesc& roleDesc)
	{
		removeRole();
		createRole(roleDesc);
	}

	Vector3 RoleComponent::GetRootMotionPositionOffset()
	{
		if (m_pRole == nullptr)
			return Vector3::ZERO;
		return m_pRole->getRootMotionComponent()->getPosOffset();
	}

	Quaternion RoleComponent::GetRootMotionRotationOffset()
	{
		if (m_pRole == nullptr)
			return Quaternion::IDENTITY;
		return m_pRole->getRootMotionComponent()->getOrientationOffset();
	}

	Vector3 RoleComponent::GetRootMotionScaleOffset()
	{
		if (m_pRole == nullptr)
			return Vector3::UNIT_SCALE;
		return m_pRole->getRootMotionComponent()->getScaleOffset();
	}

	void RoleComponent::GetRootMotionOffset(Vector3& pos, Quaternion& rot, Vector3& scale)
	{
		if (m_pRole == nullptr)
		{
			pos = Vector3::ZERO;
			rot = Quaternion::IDENTITY;
			scale = Vector3::UNIT_SCALE;
		}
		m_pRole->getRootMotionComponent()->getRootOffset(rot, pos, scale);
	}

	void RoleComponent::GetRoleEquipName()
	{
		Role* role = GetRoleInfo();
		if (role == NULL)
			return;
		mRoleEquipNameList.clear();
		role->getEquipInfoName(mRoleEquipNameList);
	}

	void RoleComponent::GetRoleAnimationName()
	{
		Role* role = GetRoleInfo();
		if (role == NULL)
			return;
		mRoleAnimationList.clear();
		role->getAllAnimation(mRoleAnimationList);
	}

	void RoleComponent::SetRoleEquip(const std::string & name)
	{
		if (m_pRole == NULL)
			return;
		m_pRole->getSkeletonComponent()->changeEquip(Name(name), true);
		mRoleEquipName = Name(name);
#ifdef ECHO_EDITOR
		if (ActorHelper::getForceSave() && m_pActComInfo)
		{
			RoleComponentInfo* mRoleInfo = static_cast<RoleComponentInfo*>(m_pActComInfo);
			mRoleInfo->m_RoleEquipName = mRoleEquipName.toString();
		}
#endif
	}

	void RoleComponent::SetRoleAnimation(const Name& name, bool isLoop, uint8 animationIndex, AnimationStateListener* listener,
		float startTime, float endTime, bool synchronous, bool keep, float weight, bool bFileName, uint16 blendmaskID,
		int channel, AnimationStateType type, uint8 additiveIndex, uint8 additiveBlendIndex)
	{
		if (m_pRole == NULL || name.empty() || name == "")
			return;
		if (Root::instance()->getRuntimeMode() == Root::RuntimeMode::eRM_Editor
			&& (!ActorSystem::instance()->getMainActorManager()->IsEditorPlay()
				|| (ActorSystem::instance()->getMainActorManager()->IsEditorPlay() && ActorSystem::instance()->getMainActorManager()->IsEditorPause())))
			return;
		m_pRole->getAnimationComponent()->setAnimationUpdateManual(false);
		m_pRole->getAnimationComponent()->playAnimation(name, isLoop, animationIndex, listener,
			startTime, endTime, synchronous, keep, weight, bFileName, blendmaskID,
			channel, type, additiveIndex, additiveBlendIndex);
	}

	void RoleComponent::ClearAnimation()
	{
		if (m_pRole == NULL)
			return;
		m_pRole->getAnimationComponent()->clearAnimation();
	}

	void RoleComponent::StopAnimation(const std::string & aniName)
	{
		if (m_pRole == NULL)
			return;
		m_pRole->getAnimationComponent()->stopAnimation(Name(aniName));
	}

	void RoleComponent::SetAnimationPause(const Name& aniName, bool isPause)
	{
		if (m_pRole == nullptr)
			return;
		Name animationName = m_pRole->getSkeletonComponent()->getRoleInfo()->getAnimationFileName(aniName);
		AnimationState* as = m_pRole->getAnimationComponent()->getAnimationState(animationName);
		if (as != nullptr)
			as->setPause(isPause);
	}

	void RoleComponent::SetAnimationAllPause(bool isPause)
	{
		if (m_pRole == nullptr)
			return;
		AnimationStateSet* roleAnimSet = m_pRole->getAnimationComponent()->getAnimationStateSet();
		if (!roleAnimSet)
			return;
		if (isPause)
			roleAnimSet->pauseAllAnimation();
		else
			roleAnimSet->resumeAllAnimation();
	}

	void RoleComponent::SetPlayAnimation(const Name& aniName, float time)
	{
		if (m_pRole == nullptr)
			return;
		Name animationName = m_pRole->getSkeletonComponent()->getRoleInfo()->getAnimationFileName(aniName);
		AnimationState* as = m_pRole->getAnimationComponent()->getAnimationState(animationName);
		if (as != nullptr)
		{
			as->addTime(time);
		}
	}

	void RoleComponent::SetRoleAnimation_1(const std::string & name)
	{
		mRoleAnimationName_1 = Name(name);
		if (ActorSystem::instance()->getMainActorManager()->IsEditorPlay())
		{
			SetRoleAnimation(Name(name), true, 0);
			SetAnimationPause(Name(name), ActorSystem::instance()->getMainActorManager()->IsEditorPause());
		}
#ifdef ECHO_EDITOR
		if (ActorHelper::getForceSave() && m_pActComInfo)
		{
			RoleComponentInfo* mRoleInfo = static_cast<RoleComponentInfo*>(m_pActComInfo);
			mRoleInfo->m_RoleAnimationName_1 = name;
		}
#endif
	}

	void RoleComponent::SetRoleAnimation_2(const std::string & name)
	{
		mRoleAnimationName_2 = Name(name);
		if (ActorSystem::instance()->getMainActorManager()->IsEditorPlay())
		{
			SetRoleAnimation(Name(name), true, 1);
			SetAnimationPause(Name(name), ActorSystem::instance()->getMainActorManager()->IsEditorPause());
		}
#ifdef ECHO_EDITOR
		if (ActorHelper::getForceSave() && m_pActComInfo)
		{
			RoleComponentInfo* mRoleInfo = static_cast<RoleComponentInfo*>(m_pActComInfo);
			mRoleInfo->m_RoleAnimationName_2 = name;
		}
#endif
	}

	void RoleComponent::SetAnimationWeight(const Name & name, float weight)
	{
		if (m_pRole == NULL)
			return;
		Name animationName = m_pRole->getSkeletonComponent()->getRoleInfo()->getAnimationFileName(Name(name));
		AnimationState* as = m_pRole->getAnimationComponent()->getAnimationState(animationName);
		if (as == NULL)
			return;
		else
			as->setWeight(weight);
	}

	void RoleComponent::SetRoleDissolveStrength(float val)
	{
		if (m_pRole == NULL)
			return;
		m_pRole->getMakeupCompoent()->setDissolveStrength(val);
		mDissolveStrength = val;
	}

	float RoleComponent::GetRoleDissolveStrength()
	{
		if (m_pRole == NULL)
			return 0.0f;
		return m_pRole->getMakeupCompoent()->getDissolveStrength();
	}

	void RoleComponent::SetRoleDissolveEdgeColor(Vector3 _val)
	{
		if (m_pRole == NULL)
			return;
		m_pRole->getMakeupCompoent()->setDissolveEdgeColor(_val);
	}

	void RoleComponent::SetRoleDissolveColorStrength(float _val)
	{
		if (m_pRole == NULL)
			return;
		m_pRole->getMakeupCompoent()->setDissolveColorStrength(_val);
	}

	void RoleComponent::SetRoleDissolveTillingU(float _val)
	{
		if (m_pRole == NULL)
			return;
		m_pRole->getMakeupCompoent()->setDissolveTillingU(_val);
	}

	void RoleComponent::SetRoleDissolveTillingV(float _val)
	{
		if (m_pRole == NULL)
			return;
		m_pRole->getMakeupCompoent()->setDissolveTillingV(_val);
	}

	void RoleComponent::SetRoleDissolveEdgeWidth(float _val)
	{
		if (m_pRole == NULL)
			return;
		m_pRole->getMakeupCompoent()->setDissolveEdgeWidth(_val);
	}

	void RoleComponent::SetRoleDissolveSoftValue(float _val)
	{
		if (m_pRole == NULL)
			return;
		m_pRole->getMakeupCompoent()->setDissolveSoftValue(_val);
	}

	void RoleComponent::SetRoleDissolveNoiseTexture(Echo::String _texture)
	{
		if (m_pRole == NULL)
			return;
		m_pRole->getMakeupCompoent()->setDissolveNoiseTexture(_texture);
	}

	bool RoleComponent::IsAnimationStop(const Name& animation)
	{
		if (m_pRole == NULL)
			return false;
		Echo::Name animationName = m_pRole->getSkeletonComponent()->getRoleInfo()->getAnimationFileName(animation);
		AnimationStateSet* roleAnimationSet = m_pRole->getAnimationComponent()->getAnimationStateSet();
		if (!roleAnimationSet)
			return false;
		AnimationState* as = roleAnimationSet->getAnimationState(animationName);

		if (as == NULL)
			return true;
		else
			return as->getEnabled();
	}

	void RoleComponent::BonePreview()
	{
		CallProtocolFunc(Echo::Name("RoleEditorSetRoleObj"), NULL, (ProtocolHandle)this);
	}

	bool RoleComponent::MatchAnimation()
	{
		if (mRoleAnimationList.empty())
			return false;
		int index = (int)mRoleAnimationName_1.toString().find_last_of("_");
		if (!index || mRoleAnimationName_1.empty() || mRoleAnimationName_1.size() <= 3)
			return false;
		std::string name = mRoleAnimationName_1.toString();
		name = name.replace(index, 3, "_ca");
		if(name == mRoleAnimationName_1.toString())
			name = name.replace(index, 3, "_tk");
		for (auto animation : mRoleAnimationList)
		{
			if (animation.c_str() == name)
			{
				mRoleAnimationName_2 = name;
				return true;
			}
		}
		return false;
	}

	void RoleComponent::MatchAnimation(Name & animation_1, Name & animation_2)
	{
		if (animation_1.empty())
			return;
		if (mRoleAnimationList.empty())
			return;
		std::string name = animation_1.toString();
		int index = (int)name.find_last_of("_");
		if (!index || animation_1.empty() || animation_1.size() <= 3)
			return;
		name = name.replace(index, 3, "_ca");
		if (name == animation_1.toString())
			name = name.replace(index, 3, "_tk");
		for (auto animation : mRoleAnimationList)
		{
			if (animation.c_str() == name)
			{
				animation_2 = name;
				return;
			}
		}
		return;
	}

	Echo::EntityMap RoleComponent::getLowestEntityMap()
	{
		EntityMap entityMap;
		if (GetRole() == nullptr)
			return entityMap;


		for (int i = ROLE_LOD_LEVEL_CNT - 1; i >= 0; --i)
		{
			auto& lowestMap = GetRole()->getEntityComponent()->getEntityMap(i);
			if (lowestMap.size() == 0)
				continue;
			entityMap = lowestMap;
		}

		return entityMap;
	}

	RoleObject * RoleComponent::GetRole()
	{
		return m_pRole;
	}

	Role * RoleComponent::GetRoleInfo()
	{
		if (m_pRole == NULL)
			return NULL;
		Role* role = m_pRole->getSkeletonComponent()->getRoleInfo().get();
		if (role == NULL)
			return NULL;
		return role;
	}

	void RoleComponent::SetCustomRoleAABB(const Vector3& min, const Vector3& max)
	{
		mbIsCustomRoleAABB = true;
		if (min.x > max.x ||
			min.y > max.y ||
			min.z > max.z)
		{
			mCustomRoleAABB[0] = { -1.f,-1.f,-1.f };
			mCustomRoleAABB[1] = { 1.f,1.f,1.f };
		}
		else
		{
			mCustomRoleAABB[0] = min;
			mCustomRoleAABB[1] = max;
		}

		_SetCustomRoleAABBImpl();
	}
	bool RoleComponent::GetCustomRoleAABB(Vector3& min, Vector3& max)
	{
		if (mbIsCustomRoleAABB)
		{
			min = mCustomRoleAABB[0];
			max = mCustomRoleAABB[1];
			return true;
		}
		return false;
	}
	bool RoleComponent::GetCustomRoleAABB()
	{
		return mbIsCustomRoleAABB;
	}
#ifdef ECHO_EDITOR
	void RoleComponent::getErrorMessageImpl(String& message)
	{
		if (!hasErrorMessage())
			return;
		if (m_pRole == nullptr)
			return;
		m_pRole->getErrorMessage(message);
	}
	bool RoleComponent::hasErrorMessageImpl()
	{
		bool ret = false;
		if (m_pRole == nullptr)
			return ret;
		ret = m_pRole->hasErrorMessage();
		if (ret)
		{
			REPORT_ERROR_MESSAGE_INSTANCE(ActorComponent, this);
		}
		return ret;
	}
#endif

	// 骨骼组功能begin
	SkeletonInstance* RoleComponent::GetSkeletonInstance()
	{
		SkeletonInstance* lSkeletonIns = nullptr;
		if (m_pRole->getRoleMovable() != nullptr)
		{
			lSkeletonIns = m_pRole->getRoleMovable()->getSkeletonInstance();
		}
		return lSkeletonIns;
	}
#ifdef ECHO_EDITOR
	void RoleComponent::BonePartSwitch(bool pFlag)
	{
		// 获取骨骼实例
		SkeletonInstance* lSkeletonIns = GetSkeletonInstance();
		if (lSkeletonIns == nullptr)  return;
		mUseBonePart = pFlag;
		ChangeRole(mRolePath.toString(), true, mUseBonePart, mSelectedBonePartId);
	}
	void RoleComponent::BonePartToggle(uint16 pBonePartId)
	{
		// 获取骨骼实例
		SkeletonInstance* lSkeletonIns = GetSkeletonInstance();
		if (lSkeletonIns == nullptr || GetRoleInfo() == nullptr)  return;
		if (GetRoleInfo()->mBonePartMap.find(pBonePartId) != GetRoleInfo()->mBonePartMap.end())
		{
			// 判断是否越界
			mSelectedBonePartId = pBonePartId;
		}
		ChangeRole(mRolePath.toString(), true, mUseBonePart, mSelectedBonePartId);
	}
#endif // ECHO_EDITOR
	// 骨骼组功能end


	void RoleComponent::createRole(const std::string & path, bool synchronous /*= false*/, bool useBonePart/* = false*/, uint16 bonePartId/* = 0*/)
	{
		//if (m_pRole != NULL)
		//	return;
		mRolePath = Name(path);
		Name roleName = mRolePath;
#ifdef ECHO_EDITOR
		if (ActorHelper::getForceSave() && m_pActComInfo)
		{
			RoleComponentInfo* mRoleInfo = static_cast<RoleComponentInfo*>(m_pActComInfo);
			mRoleInfo->m_RolePath = path;
		}		
#endif
		if (Root::instance()->getRuntimeMode() == Root::eRM_Editor)
			roleName = Name(getCombinedName().append("||")) + mRolePath;

		RoleObjectDesc desc;
		desc.name = roleName;
		desc.resName = mRolePath;
		desc.synchronous = synchronous;
		desc.clientLogMessage = mClientLogMessage;
		desc.pWorldMgr = getWorldManager();
		desc.useBonePart = useBonePart;
		desc.bonePartId = bonePartId;
		//m_pRole = new RoleObject(desc);
		m_pRole->createObj(desc);
		if (ActorSystem::instance()->isHideRenderObject())
		{
			if (m_pRole)
				m_pRole->setParentVisible(false);
		}
		////false--异步操作
		//if (Root::instance()->getRuntimeMode() == Root::RuntimeMode::eRM_Client)
		//	m_pRole = new RoleObject(roleName, mRolePath, NULL, false);

		////true--同步操作
		//if (Root::instance()->getRuntimeMode() == Root::RuntimeMode::eRM_Editor)
		//	m_pRole = new RoleObject(roleName, mRolePath, NULL, true);

		//m_pRole->setQueryFlags(m_QtFlags);
		if (m_pParentNodeCom == nullptr)
			return;
		if (m_pRole != NULL)
		{
			//addChildToNode(m_pRole->getSceneNode());
			UpdateComRootNode(true, true);
		}

		if (GetCustomRoleAABB())
			_SetCustomRoleAABBImpl();
	}

	void RoleComponent::createRole(RoleObjectDesc& roleDesc)
	{
		//if (m_pRole != NULL)
		//	return;
		mRolePath = roleDesc.resName;
		roleDesc.name = mRolePath;
#ifdef ECHO_EDITOR
		if (ActorHelper::getForceSave() && m_pActComInfo)
		{
			RoleComponentInfo* mRoleInfo = static_cast<RoleComponentInfo*>(m_pActComInfo);
			mRoleInfo->m_RolePath = mRolePath.toString();
		}
#endif
		if (roleDesc.clientLogMessage.empty())
			roleDesc.clientLogMessage = mClientLogMessage;
		if (Root::instance()->getRuntimeMode() == Root::eRM_Editor)
			roleDesc.name = Name(getCombinedName().append("||")) + mRolePath;
		if (roleDesc.pWorldMgr == nullptr)
			roleDesc.pWorldMgr = getWorldManager();

		//m_pRole = new RoleObject(roleDesc);

		m_pRole->createObj(roleDesc);
		if (m_pParentNodeCom == nullptr)
			return;
		if (m_pRole != NULL)
		{
			//addChildToNode(m_pRole->getSceneNode());
			UpdateComRootNode(true, true);
		}

		if (GetCustomRoleAABB())
			_SetCustomRoleAABBImpl();
	}

	void RoleComponent::removeRole()
	{
		CallProtocolFunc(Name("RoleEditorNotifyDestroy"), NULL, (ProtocolHandle)this);

		if (m_pRole == NULL)
			return;
		//delete m_pRole;
		//m_pRole = NULL;
		m_pRole->destroyObj();

		mRolePath = "";
		mRoleEquipName = "";
		mRoleAnimationName_1 = "";
		mRoleAnimationName_2 = "";
		mDissolveStrength = 0.0f;
	}

	void RoleComponent::CreateBoneComponents() {
		RoleObject* pRoleObj = GetRole();
		if (!pRoleObj) return;
		RoleMovable* pSkeObj = pRoleObj->getRoleMovable();
		if (!pSkeObj) return;
		SkeletonInstance* pSkeIns = pSkeObj->getSkeletonInstance();
		if (!pSkeIns) return;
		auto pBone = pSkeIns->getRootBone();
		CreateBoneComponent(weak_from_this(), pBone);
	}

	void RoleComponent::CreateBoneComponent(ActorComponentPtr  pParentComp, Bone* pSelfBone) {

		if (!pSelfBone) return;
		static Name newType = COMPONENTTYPE(BoneRenderComponent);
		Name newCompName = pSelfBone->getName();
		BoneRenderComponent* pChildActComp = (BoneRenderComponent*)getActor()->addComponent(newType, pSelfBone->getName().toString(), pParentComp, nullptr, getGroupID(), true).get();
		if (!pChildActComp) return;
		pChildActComp->SetBone(pSelfBone);
		int num = pSelfBone->numChildren();
		if (num != 0) {
			for (int i = 0; i != num; ++i) {
				CreateBoneComponent(pChildActComp->weak_from_this(), (Echo::Bone*)pSelfBone->getChild(i));
			}
		}
		pChildActComp->UpdateRender();
	}

	void RoleComponent::EditorPlayImp()
	{
		SetRoleAnimation(mRoleAnimationName_1, true, 0);
		SetRoleAnimation(mRoleAnimationName_2, true, 1);
	}

	void RoleComponent::EditorPauseImp(bool pause)
	{
		SetAnimationAllPause(pause);
	}

	void RoleComponent::EditorStopImp()
	{
		ClearAnimation();
	}

	void RoleComponent::_SetCustomRoleAABBImpl()
	{
		RoleMovable* prm = nullptr;
		SceneNode* ppn = nullptr;
		if (m_pRole == nullptr)
			return;
		if (m_pRole->isCreateFinish())
		{
			prm = m_pRole->getRoleMovable();
			if (prm == nullptr)
				return;

			prm->setAABB(mCustomRoleAABB[0], mCustomRoleAABB[1]);
			ppn = static_cast<SceneNode*>(prm->getParentNode());
			if (ppn == nullptr)
				return;
			ppn->needUpdateBounds();
			//ppn->_updateAxisBox();
			ppn->_updateBounds();
		}
		else
		{
			m_pRole->addRoleLoadListener(&mCustomRoleAABBCallBack);
		}
	}

	EchoRoleComScript EchoRoleComScript::GetRoleComponent(const EchoActorScript& pActor, const std::string & name)
	{
		return EchoRoleComScript(pActor, name);
	}

	bool RoleComponent::SetCustomRoleAABBListener::OnRoleCreateFinish()
	{
		RoleMovable* prm = m_pRoleObj->getRoleMovable();
		if (prm == nullptr)
			return false;

		prm->setAABB(pComp->mCustomRoleAABB[0], pComp->mCustomRoleAABB[1]);
		SceneNode* ppn = static_cast<SceneNode*>(prm->getParentNode());
		if (ppn == nullptr)
			return false;
		ppn->needUpdateBounds();
		//ppn->_updateAxisBox();
		ppn->_updateBounds();
		pComp->UpdateBounds();
		return true;
	}

}
