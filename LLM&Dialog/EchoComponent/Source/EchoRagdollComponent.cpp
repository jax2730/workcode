//-----------------------------------------------------------------------------
// File:  EchoRagdollComponent.cpp
//
// Author: LangGuangyue
//
// Date:   2022-10-20
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include "EchoComponentStableHeaders.h"
#include "EchoRagdollComponent.h"
#include "EchoRigidBodyGroupComponent.h"
#include "EchoRigidBodyComponent.h"
#include "EchoHelperObj.h"
#include "EchoPhysXJoint.h"

#include <EchoEngine.h>
#include "EchoResourceFolderManager.h"
#include "EchoRagdollBehaviorComponentBase.h"
#include "EchoRagdollBehaviorManagerComponent.h"
#include "EchoCharactorControllerComponent.h"
#include "EchoRagdollCreatorComponent.h"
#include "EchoRagdollTools.h"

#define GET_INFO() (static_cast<RagdollComponentInfo*> (this->m_pActComInfo))

#define DBV2FV(dbVec3) ((Vector3)dbVec3)

namespace Echo
{
	namespace
	{
		bool IsBonePartIK(RoleIKComponent* pRoleIK)
		{
			if (pRoleIK == nullptr)
				return false;

			RoleObject* pRoleObj = pRoleIK->GetRoleObject();
			if (pRoleObj == nullptr)
				return false;

			RoleMovable* pRoleMov = pRoleObj->getRoleMovable();
			if (pRoleMov == nullptr)
				return false;

			SkeletonInstance* pSkelInst = pRoleMov->getSkeletonInstance();
			if (pSkelInst == nullptr)
				return false;

			return pSkelInst->getBonePartFlag();
		}
	}

	DECLAREACTORCOMTYPE_CPP(RagdollComponent)
		RagdollComponentInfo::RagdollComponentInfo(const Name& mtype, ActorCOMMoveableType movableType)
		: ActorComponentInfo(mtype, movableType),
		muscleModifier(this)
	{
	}
	RagdollComponentInfo::RagdollComponentInfo()
		:muscleModifier(this)
	{
	}
	RagdollComponentInfo::~RagdollComponentInfo()
	{
	};

	EDITOR_REFLECT_BEGIN(RagdollComponentInfo)
		//EDITOR_REFLECT_MEMBER_UPDATE(isKilling, "isKilling", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(roleName, "roleName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(stateSettings, "StateSettings", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(blendTime, "blendTime", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(fixTargetTransforms, "fixTargetTransforms", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(solverIterationCount, "solverIterationCount", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mappingWeight, "mappingWeight", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(pinWeight, "pinWeight", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(muscleWeight, "muscleWeight", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(muscleSpring, "muscleSpring", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(muscleDamper, "muscleDamper", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(pinPow, "pinPow", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(pinDistanceFalloff, "pinDistanceFalloff", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(maxAngularVelocity, "maxAngularVelocity", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(maxLinearVelocity, "maxLinearVelocity", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(ragdollDensity, "ragdollDensity", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(angularPinning, "angularPinning", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(updateJointAnchors, "updateJointAnchors", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(supportTranslationAnimation, "supportTranslationAnimation", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(angularLimits, "angularLimits", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(internalCollisions, "internalCollisions", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		//EDITOR_REFLECT_MEMBER_UPDATE(rigiBodyGroupName, "rigiBodyGroupName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(bnmInfo, "bnmInfo", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		//EDITOR_REFLECT_MEMBER_UPDATE(muscleData, "MuscleData", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		//EDITOR_REFLECT_MEMBER_UPDATE(muscleArray,"MuscleArray", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_END()
		;

	void WriteJSNode(JSNODE jsonNode, const char* Name, const RagdollStateSettings& settings)
	{
		cJSON* StateSettings = cJSON_CreateObject();
		cJSON_AddItemToObject(jsonNode, "StateSettings", StateSettings);
		{
			WriteJSNode(StateSettings, "killDuration", settings.killDuration);
			WriteJSNode(StateSettings, "deadMuscleWeight", settings.deadMuscleWeight);
			WriteJSNode(StateSettings, "deadMuscleDamper", settings.deadMuscleDamper);
			WriteJSNode(StateSettings, "maxFreezeSqrVelocity", settings.maxFreezeSqrVelocity);
			WriteJSNode(StateSettings, "freezePermanently", settings.freezePermanently);
			WriteJSNode(StateSettings, "enableAngularLimitsOnKill", settings.enableAngularLimitsOnKill);
		}
	}

	bool ReadJSNode(cJSON* jsonNode, const char* Name, RagdollStateSettings& settings)
	{
		cJSON* StateSettings = cJSON_GetObjectItem(jsonNode, "StateSettings");
		if (StateSettings)
		{
			ReadJSNode(StateSettings, "killDuration", settings.killDuration);
			ReadJSNode(StateSettings, "deadMuscleWeight", settings.deadMuscleWeight);
			ReadJSNode(StateSettings, "deadMuscleDamper", settings.deadMuscleDamper);
			ReadJSNode(StateSettings, "maxFreezeSqrVelocity", settings.maxFreezeSqrVelocity);
			ReadJSNode(StateSettings, "freezePermanently", settings.freezePermanently);
			ReadJSNode(StateSettings, "enableAngularLimitsOnKill", settings.enableAngularLimitsOnKill);
			ReadJSNode(StateSettings, "enableInternalCollisionsOnKill", settings.enableInternalCollisionsOnKill);
			return true;
		}
		return false;
	}

	void WriteJSNode(cJSON* jsonNode, const char* Name, const RagdollComponentInfo::BoneNodeAndMucleInfo& data)
	{
		cJSON* dataArray = cJSON_CreateArray();
		cJSON_AddItemToObject(jsonNode, Name, dataArray);
		for (const auto& pa : data)
		{
			cJSON* item = cJSON_CreateObject();
			cJSON_AddItemToArray(dataArray, item);
			{
				WriteJSNode(item, "boneName", pa.first);

				WriteJSNode(item, "jointTargetBoneName", pa.second.jointTargetBoneName);
				WriteJSNode(item, "nodeName", pa.second.nodeName);
				WriteJSNode(item, "rigidbodyName", pa.second.rigidBodyName);
				WriteJSNode(item, "colliderName", pa.second.colliderName);
				WriteJSNode(item, "jointName", pa.second.jointName);

				{
					int temp = pa.second.muclesDesc.group;
					WriteJSNode(item, "mdGroup", temp);
				}
				WriteJSNode(item, "mdMappingWeight", pa.second.muclesDesc.mappingWeight);
				WriteJSNode(item, "mdPinWeight", pa.second.muclesDesc.pinWeight);
				WriteJSNode(item, "mdMuscleWeight", pa.second.muclesDesc.muscleWeight);
				WriteJSNode(item, "mdMuscleDamper", pa.second.muclesDesc.muscleDamper);
				WriteJSNode(item, "mdMapPosition", pa.second.muclesDesc.mapPosition);
			}
		}
	}

	bool ReadJSNode(cJSON* jsonNode, const char* Name, RagdollComponentInfo::BoneNodeAndMucleInfo& data)
	{
		data.clear();
		
		cJSON* dataArray = cJSON_GetObjectItem(jsonNode, Name);
		if (dataArray != nullptr)
		{
			cJSON* item = dataArray->child;
			RagdollComponentInfo::BoneNodeAndMucleInfo::value_type pa;
			while (item != nullptr)
			{
				ReadJSNode(item, "boneName", pa.first);

				ReadJSNode(item, "jointTargetBoneName", pa.second.jointTargetBoneName);
				ReadJSNode(item, "nodeName", pa.second.nodeName);
				ReadJSNode(item, "rigidbodyName", pa.second.rigidBodyName);
				ReadJSNode(item, "colliderName", pa.second.colliderName);
				ReadJSNode(item, "jointName", pa.second.jointName);

				{
					int temp = 0;
					ReadJSNode(item, "mdGroup", temp);
					pa.second.muclesDesc.group = (RagdollMuscle::MuscleGroup)temp;
				}
				ReadJSNode(item, "mdMappingWeight", pa.second.muclesDesc.mappingWeight);
				ReadJSNode(item, "mdPinWeight", pa.second.muclesDesc.pinWeight);
				ReadJSNode(item, "mdMuscleWeight", pa.second.muclesDesc.muscleWeight);
				ReadJSNode(item, "mdMuscleDamper", pa.second.muclesDesc.muscleDamper);
				ReadJSNode(item, "mdMapPosition", pa.second.muclesDesc.mapPosition);

				data.push_back(pa);

				item = item->next;
			}
			return true;
		}
		return false;
	}

	void RagdollComponentInfo::ExportData(cJSON * jsonNode)
	{
		reflect::TypeResolver<RagdollComponentInfo>::get()->ExportJson(this, jsonNode, "");
	}
	void RagdollComponentInfo::ImportData(cJSON * jsonNode)
	{
		reflect::TypeResolver<RagdollComponentInfo>::get()->ImportJson(this, jsonNode, "");
	}
	void RagdollComponentInfo::CopyTo(ActorComponentInfo * comInfo)
	{
		RagdollComponentInfo* minfo = static_cast<RagdollComponentInfo*>(comInfo);
		reflect::TypeResolver<RagdollComponentInfo>::get()->copyTo(this, minfo);
	}
	void RagdollComponentInfo::CopyToData(RagdollComponentData* data)
	{
		if (data == nullptr)
			return;

		data->stateSettings = this->stateSettings;
		data->blendTime = this->blendTime;
		data->fixTargetTransforms = this->fixTargetTransforms;
		data->solverIterationCount = this->solverIterationCount;
		data->mappingWeight = this->mappingWeight;
		data->pinWeight = this->pinWeight;
		data->muscleWeight = this->muscleWeight;
		data->muscleSpring = this->muscleSpring;
		data->muscleDamper = this->muscleDamper;
		data->pinPow = this->pinPow;
		data->pinDistanceFalloff = this->pinDistanceFalloff;
		data->maxAngularVelocity = this->maxAngularVelocity;
		data->maxLinearVelocity = this->maxLinearVelocity;
		data->ragdollDensity = this->ragdollDensity;
		data->angularPinning = this->angularPinning;
		data->updateJointAnchors = this->updateJointAnchors;
		data->supportTranslationAnimation = this->supportTranslationAnimation;
		data->angularLimits = this->angularLimits;
		data->internalCollisions = this->internalCollisions;

		//int end = (int)this->bnmInfo.size();
		//if (end == 0)
		//	return;
		//data->mucleData.resize(end);
		//for (int i = 0; i < end; ++i)
		//{
		//	data->mucleData[i].first = this->bnmInfo[i].first;

		//	data->mucleData[i].second.mappingWeight = this->bnmInfo[i].second.muclesDesc.mappingWeight;
		//	data->mucleData[i].second.pinWeight = this->bnmInfo[i].second.muclesDesc.pinWeight;
		//	data->mucleData[i].second.muscleWeight = this->bnmInfo[i].second.muclesDesc.muscleWeight;
		//	data->mucleData[i].second.muscleDamper = this->bnmInfo[i].second.muclesDesc.muscleDamper;
		//}
	}
	void RagdollComponentInfo::OverWrite()
	{
		if (mOriginComInfo != nullptr)
		{
			RagdollComponentInfo* orilInfo = static_cast<RagdollComponentInfo*> (mOriginComInfo);
			reflect::TypeResolver<RagdollComponentInfo>::get()->overWrite(orilInfo, this);
		}
	}
	RagdollComponent::RagdollComponent(ActorPtr  pActor, ActorCOMMoveableType movableType = ALL_MOVEABLE)
		:ProtocolNodeActorComponent(pActor, movableType)
	{
		m_eventSet[TT_Update].insert(EID_EveryFrame);
		m_eventSet[TT_LateUpdate].insert(EID_EveryFrame);
		m_eventSet[TT_FixedUpdate].insert(EID_EveryFrame);
		m_eventSet[TT_PrePhysicsUpdate].insert(EID_EveryFrame);
		registerTickTimeBegin();
	}
	RagdollComponent::~RagdollComponent()
	{
		DestroyHelper();
	}
	void RagdollComponent::init(ActorComponentInfo * info)
	{
		RagdollComponentInfo* minfo = static_cast<RagdollComponentInfo*>(info);
		if (minfo == NULL)
			return;

		const Name& target = Name(minfo->roleName);

		RagdollComponentData ragdollData; 
		minfo->CopyToData(&ragdollData);
		
		std::vector<struct RagdollBlockDesc> ragdollBlocks;
		for (const auto& [boneName, boneDesc] : minfo->bnmInfo)
		{
			ragdollBlocks.emplace_back();
			auto& block = ragdollBlocks.back();

			block.nodeName		= boneDesc.nodeName;
			block.colliderName  = boneDesc.colliderName;
			block.rigidbodyName = boneDesc.rigidBodyName;
			block.jointName		= boneDesc.jointName;
			block.BoneName		= boneName;
			block.jointTarget	= boneDesc.jointTargetBoneName;

			block.group = boneDesc.muclesDesc.group;
			block.mapping = boneDesc.muclesDesc.mappingWeight;
			block.pin = boneDesc.muclesDesc.pinWeight;
			block.weight = boneDesc.muclesDesc.muscleWeight;
			block.damper = boneDesc.muclesDesc.muscleWeight;
			block.mapPosition = boneDesc.muclesDesc.mapPosition;
		}
		blockDesc = std::move(ragdollBlocks);

		LoadStaticData(target, &ragdollData);
	}
	void RagdollComponent::bindNode()
	{
		CreateHelper();
	}
	void RagdollComponent::replay(ActorComponentInfo * info)
	{
		RagdollComponentInfo* minfo = static_cast<RagdollComponentInfo*>(info);
		if (minfo == NULL)
			return;
	}
	void RagdollComponent::connectCom()
	{
		if (targetName.empty())
			return;

		loadDataFromInfo(&blockDesc);
	}
	
	void RagdollComponent::disconnectCom() {
		if (!targetRoot.expired() && mRoleIKCom->GetRoleObject() != nullptr)
			mRoleIKCom->GetRoleObject()->removeRoleLoadListener(this);

		Stop();
		DestroyHelper();
	}

	void RagdollComponent::setActor(const ActorPtr& pActor)
	{
		if (pActor == m_pActor)
			return;
		m_pActor = pActor;
		if (m_pActor == nullptr)
			return;
	}
	void RagdollComponent::copyTo(const ActorComponentPtr& actCom)
	{
		// TO DO
	}
	void RagdollComponent::dealComponentDelete(const ActorComponentPtr& actCom)
	{
		if (actCom->getType() == COMPONENTTYPE(RoleComponent))
		{
			RoleComponent* roleCom = (RoleComponent*)actCom.get();
			if (roleCom->GetRole())
			{
				if (roleCom->GetRole()->getRoleIKComponent() == mRoleIKCom)
				{
					mRoleIKCom = NULL;
					for (auto m : muscles)
						SAFE_DELETE(m);
					muscles.clear();
					mRagdollBlocks.clear();
				}
			}
		}
	}
	void RagdollComponent::visibleStateChanged(bool enable)
	{
		if (enable)
			OnEnable();
		else
			OnDisable();
	}
	void RagdollComponent::registerTickTimeBegin()
	{
		registerTickTimeEnd();
	}
	void RagdollComponent::prePhysicsUpdate(EventID, ProtocolHandle) {
		if (!Root::instance()->m_bUseRagdoll) return;
		if (!initiated) {
			return;
		}
		if(isRoleMove)
			RoleMove();

		//实验逻辑，这部分需要让客户端实现
		if (isMoveCCT && IsActive()) {
			float w = muscles[0]->attribute->pinWeight * muscles[0]->state.pinWeightMlp * pinWeight;
			if (w <= 0.0f) {
				auto pCtrl = EchoPhysicsSystem::instance()->GetCurrentCharactorController();
				if (pCtrl != nullptr) {
					Vector3 pos = muscles[0]->GetTransform()->getWorldPosition();
					Vector3 curPos = pCtrl->GetWorldCenterPosition();
					curPos = pos - curPos;
					float middleHeight = pCtrl->GetRealHeigth();
					curPos.y = curPos.y - Math::Clamp(curPos.y, -0.5f * middleHeight, 0.5f * middleHeight);
					pCtrl->Move(0.0f, curPos);
				}
			}
		}
	}
	void RagdollComponent::fixedUpdate(EventID, ProtocolHandle)
	{
		if (!Root::instance()->m_bUseRagdoll) return;
		FixedUpdate();
		UpdataHelper();
	}
	void RagdollComponent::preUpdate(EventID, ProtocolHandle)
	{
		if (!Root::instance()->m_bUseRagdoll) return;
		Update();
	}
	void RagdollComponent::lateUpdate(EventID, ProtocolHandle)
	{
		if (!Root::instance()->m_bUseRagdoll) return;
		LateUpdate();
	}
	void RagdollComponent::EditorPlayImp()
	{
		// 防止初始化之前刚体位置被改变
		for (auto& m : mRagdollBlocks)
		{
			if (!m.rigidbody.expired())
				((RigidBodyComponent*)(m.rigidbody.get()))->SetIsKinematic(true);
		}
		Awake();
	}
	void RagdollComponent::EditorPauseImp(bool pause)
	{
	}
	void RagdollComponent::EditorStopImp()
	{
		Stop();
	}
	void RagdollComponent::SetRoleIKCom(const std::string & comName)
	{
		if (comName.empty()) return;
		RoleComponent* roleCom = static_cast<RoleComponent*>(m_pActor->getComponent(comName).get());
		if (roleCom == NULL)
			return;
		if (roleCom->GetRole() == nullptr)
			return;
		RoleIKComponent* roleIK = roleCom->GetRole()->getRoleIKComponent();
		if (roleIK == NULL)   
			return;
		mRoleIKCom = roleIK;
		targetRoot = roleCom->weak_from_this();
	}
	void RagdollComponent::SetRoleComponentPtr(const EchoWeakPtr<RoleComponent>& rolePtr) {
		if (rolePtr == nullptr || rolePtr.expired()) return;
		RoleComponent* roleCom = rolePtr.get();
		if (roleCom == NULL)
			return;
		if (roleCom->GetRole() == nullptr)
			return;
		RoleIKComponent* roleIK = roleCom->GetRole()->getRoleIKComponent();
		if (roleIK == NULL)
			return;
		mRoleIKCom = roleIK;
		targetRoot = rolePtr->weak_from_this();
	}

	void RagdollComponent::SetMaterialId(int value) {
		
		for (auto& data : mRagdollBlocks) {
			if (data.collider != nullptr && !data.collider.expired()) {
				((ColliderComponent*)data.collider.getPointer())->SetMaterialId(value);
			}
		}
	}
	void RagdollComponent::SetSurfaceType(uint32 value) {
		for (auto& data : mRagdollBlocks) {
			if (data.collider != nullptr && !data.collider.expired()) {
				((ColliderComponent*)data.collider.getPointer())->SetSurfaceType(value);
			}
		}
	}
	void RagdollComponent::SetFilterGroupType(PhysXGroup value) {
		for (auto& data : mRagdollBlocks) {
			if (data.collider != nullptr && !data.collider.expired()) {
				((ColliderComponent*)data.collider.getPointer())->SetFilterGroupType(value);
			}
		}
	}
	void RagdollComponent::SetCanInteractFilterGroupType(PhysXGroup value) {
		for (auto& data : mRagdollBlocks) {
			if (data.collider != nullptr && !data.collider.expired()) {
				((ColliderComponent*)data.collider.getPointer())->SetCanInteractFilterGroupType(value);
			}
		}
	}

	void RagdollComponent::SetAutoCalculateLocalPose(bool value) {
		for (auto& data : mRagdollBlocks) {
			if (data.joint != nullptr && !data.joint.expired()) {
				((JointComponent*)data.joint.getPointer())->SetAutoCalculateLocalPose(value);
			}
		}
	}

	bool RagdollComponent::loadDataFromInfo(const std::vector<RagdollBlockDesc>* blockData)
	{
		if (isLoadData) return false;

		// muscle block
		if (blockData != nullptr && &blockDesc != blockData)
			blockDesc = *blockData;

		SetRoleIKCom(targetName.toString());
		if (mRoleIKCom == NULL || targetRoot.expired())
			return false;

		mRagdollBlocks.clear();
		for (auto& item : blockDesc)
		{
			mRagdollBlocks.emplace_back();
			auto& block = mRagdollBlocks.back();

			block.node = getActor()->getComponent(item.nodeName.toString());
			block.collider = getActor()->getComponent(item.colliderName.toString());
			block.rigidbody = getActor()->getComponent(item.rigidbodyName.toString());
			block.joint = getActor()->getComponent(item.jointName.toString());

			block.BoneName = item.BoneName;
			block.jointTarget = item.jointTarget;

			block.group = item.group;
			block.mapping = item.mapping;
			block.pin = item.pin;
			block.weight = item.weight;
			block.damper = item.damper;
			block.mapPosition = item.mapPosition;
		}

		isLoadData = true;
		return true;
	}

	void RagdollComponent::LoadStaticData(const Name& _targetName, const RagdollComponentData* ragdollData)
	{
		if (isLoadData) return;

		this->targetName = _targetName;
		// data
		blendTime = ragdollData->blendTime;
		stateSettings = ragdollData->stateSettings;
		fixTargetTransforms = ragdollData->fixTargetTransforms;
		solverIterationCount = ragdollData->solverIterationCount;
		mappingWeight = ragdollData->mappingWeight;
		pinWeight = ragdollData->pinWeight;
		muscleWeight = ragdollData->muscleWeight;
		muscleSpring = ragdollData->muscleSpring;
		muscleDamper = ragdollData->muscleDamper;
		pinPow = ragdollData->pinPow;
		pinDistanceFalloff = ragdollData->pinDistanceFalloff;
		maxAngularVelocity = ragdollData->maxAngularVelocity;
		maxLinearVelocity = ragdollData->maxLinearVelocity;
		ragdollDensity = ragdollData->ragdollDensity;
		angularPinning = ragdollData->angularPinning;
		updateJointAnchors = ragdollData->updateJointAnchors;
		supportTranslationAnimation = ragdollData->supportTranslationAnimation;
		angularLimits = ragdollData->angularLimits;
		internalCollisions = ragdollData->internalCollisions;
	}

	bool RagdollComponent::IsLoad()
	{
		return isLoadData;
	}

	bool RagdollComponent::IsShowHelper()
	{
		return m_pHelperObject == nullptr ? false : m_bShowHelper;
	}
	void RagdollComponent::ShowHelper(bool show)
	{
		m_bShowHelper = show;
		if (m_pHelperObject != nullptr)
			m_pHelperObject->setVisible(m_bShowHelper);
	}
	void RagdollComponent::UpdataHelper()
	{
		if (!mRoleIKCom || targetRoot.expired() || !start || !initiated || !autoSimulate() || muscles.size() <= 0)
			return;
		
		BehavioursForeach(
			[](RagdollBehaviorComponentBase* b)
			{
				b->UpdateHelperB();
			});

		if (m_pHelperObject == nullptr || !m_bShowHelper)
			return;

		std::unordered_map<WorldNode*, uint16> wnMap;
		std::vector<Vector3>vb;
		std::vector<uint16> ib;
		std::vector<uint32> cb;
		for (auto& m : muscles)
		{
			Vector3 pos =
				convertWorldToLocalPosition(DBV2FV(m->target->_getDerivedPosition()));
			pos += m_vHelperObjectOffset;
			vb.push_back(pos);
			Vector3 foce, toque;
			if (m->joint != nullptr) {
				m->joint->GetCurrentForceAndTorque(foce, toque);
			}
			if (m_eHelperMode == FoceMode)
				cb.push_back(Vector4(Math::Pow(1.f - 1.f / (foce.length() + 1.f),2), 1.f / (foce.length() + 1.f), 0.f, 0.f).ToRGBA8888());
			else 
				cb.push_back(Vector4(1.f - 1.f / (toque.length() + 1.f ) , 1.f / (toque.length() + 1.f), 0.f, 0.f).ToRGBA8888());
			wnMap.emplace(m->target, (uint16) (vb.size() - 1));
		}
		for (auto& m : muscles)
		{
			if (m->connectedBodyTarget == nullptr)
				continue;
			ib.push_back(wnMap[m->target]);
			ib.push_back(wnMap[m->connectedBodyTarget]);
		}
		m_pHelperObject->changeCustomPolygon(vb, ib,cb);
	}
	void RagdollComponent::SetHelperOffset(const Vector3& offset)
	{
		m_vHelperObjectOffset = offset;
	}
	const Vector3& RagdollComponent::GetHelperOffset()
	{
		return m_vHelperObjectOffset;
	}
	void RagdollComponent::SetHelperMode(HelperMode mode)
	{
		m_eHelperMode = mode;
	}
	RagdollComponent::HelperMode RagdollComponent::GetHelperMode()
	{
		return m_eHelperMode;
	}
	void RagdollComponent::CreateHelper()
	{
		if (Root::instance()->getRuntimeMode() == Root::RuntimeMode::eRM_Client) return;
		if (m_pHelperObject) DestroyHelper();
		Name helperName = Name(getCombinedName().append("||").append("HelperObj"));
		m_pHelperObject = new HelperObject(helperName, HelperObjType_Box, false);
		addChildToNode(m_pHelperObject->getSceneNode());
		UpdateComRootNode(true);
		m_pHelperObject->setVisible(m_bShowHelper);
		m_pHelperObject->setRenderType(ECHO_DRAW::ECHO_LINE_LIST);
		m_pHelperObject->setHelperColorType(HelperObjColorType_Green);
		m_pHelperObject->setOrientation(Quaternion::IDENTITY);
		m_pHelperObject->setScale(Vector3::UNIT_SCALE);
	}
	void RagdollComponent::DestroyHelper()
	{
		if (m_pHelperObject != nullptr)
		{
			RemoveChildFromNode(m_pHelperObject->getSceneNode());
			UpdateComRootNode(true);

			delete m_pHelperObject;
			m_pHelperObject = nullptr;
		}
	}
	void RagdollComponent::ShrinkRagdollBlocks()
	{
		auto iter = mRagdollBlocks.begin();
		while (iter != mRagdollBlocks.end())
			iter = iter->muscle == nullptr ? mRagdollBlocks.erase(iter) : ++iter;
	}
	void RagdollComponent::CopyMuscleToInfo()
	{
		if (GET_INFO() == nullptr)
			return;
		//ShrinkRagdollBlocks();
		GET_INFO()->bnmInfo.clear();
		for (auto& block : blockDesc)
		{
			std::pair<Name, RagdollComponentInfo::BoneNodeDesc> el;
			
			el.first = block.BoneName;
			
			el.second.jointTargetBoneName = block.jointTarget;
			el.second.nodeName		= block.nodeName;
			el.second.colliderName	= block.colliderName;
			el.second.jointName		= block.jointName;
			el.second.rigidBodyName	= block.rigidbodyName;
			
			el.second.muclesDesc.group			= block.group;
			el.second.muclesDesc.mappingWeight	= block.mapping;
			el.second.muclesDesc.pinWeight		= block.pin;
			el.second.muclesDesc.muscleWeight	= block.weight;
			el.second.muclesDesc.muscleDamper	= block.damper;
			el.second.muclesDesc.mapPosition	= block.mapPosition;

			GET_INFO()->bnmInfo.push_back(el);
		}
		GET_INFO()->roleName = targetName.toString();
		//GET_INFO()->rigiBodyGroupName = mRigidBodyGroup->getComName();
	}
	void RagdollComponent::DisableRagdollBlock(bool disablechild)
	{
		mappingBlend = 0.f;

		if (disablechild)
		{
			for (auto* m : muscles)
			{
				if (!m->state.isDisconnected)
				{
					m->rigidbody->getParentNodeCom()->setVisible(false);
				}
			}

		}
		EnableRoleUpdateBoxByAnim(false);

		// 关闭IK骨骼保持更新
		if (mRoleIKCom)
		{
			mRoleIKCom->setKeepUpdate(false);
		}
	}
	void RagdollComponent::ActiveRagdollBlock()
	{
		for (auto& m : muscles)
		{
			if (!m->state.isDisconnected)
			{
				m->Reset();
			}
		}

		for (auto& m : muscles)
		{
			if (!m->state.isDisconnected)
			{
				m->rigidbody->getParentNodeCom()->setVisible(true);
				// m->SetKinematic(true);
			}
		}

		// 这里尚未完善
		//FlagInternalCollisionsForUpdate();

		// 保持IK骨骼更新
		if (mRoleIKCom)
		{
			mRoleIKCom->setKeepUpdate(true);
			// 立刻更新一次
			mRoleIKCom->ManualUpdateTransform();

		}

		for (auto& m : muscles)
		{
			if (!m->state.isDisconnected)
			{
				m->SetMuscleLimit(maxAngularVelocity, maxLinearVelocity, ragdollDensity);
				m->MoveToTarget();
			}
		}

		Read();

		// 角色包围盒随动画更新
		EnableRoleUpdateBoxByAnim(true);
		// 激活时缓慢提供mapping
		if (blendTime > 0.f)
		{
			mappingBlendIncreaseFlag = true;
			return;
		}
		else
		{
			mappingBlend = 1.f;
		}
	}
	void RagdollComponent::ToDeadImpl()
	{
		// 碰撞以及角度限制相关 尚未完成
		if (stateSettings.enableAngularLimitsOnKill) {
			if (!angularLimits) {
				angularLimits = true;
				angularLimitsEnabledOnKill = true;
			}
		}
		if (stateSettings.enableInternalCollisionsOnKill) {
			if (!internalCollisions) {
				internalCollisions = true;
				internalCollisionsEnabledOnKill = true;
			}
		}

		// Set pin weight to 0 to play with joint target rotations only
		for (auto& m : muscles)
		{
			if (!m->state.isDisconnected)
			{
				m->state.pinWeightMlp = 0.0f;
				m->state.muscleDamperAdd = stateSettings.deadMuscleDamper;
			}
		}

		BehavioursForeach(
			[](RagdollBehaviorComponentBase* b)
			{
				b->KillStart();
			});

		// 记录布偶时肌肉强度
		if (muscles.size() != 0)
			weightRange = muscles[0]->state.muscleWeightMlp - stateSettings.deadMuscleWeight;
		else
			weightRange = 0.f;

		if (stateSettings.killDuration > 0.0f && weightRange > 0.0f)
		{
			if (!muscles.empty())
				weightMlp = muscles[0]->state.muscleWeightMlp;
			muscleWeightDecreaseFlag = true;
			return;

		}

		for (auto& m : muscles)
			m->state.muscleWeightMlp = stateSettings.deadMuscleWeight;

		// Disable the Animator
		SetAnimationEnabled(false);

		BehavioursForeach(
			[](RagdollBehaviorComponentBase* b)
			{
				b->KillEnd();
			});
	}
	void RagdollComponent::ToAliveImpl()
	{
		// 恢复死亡的肌肉
		for (auto& m : muscles)
		{
			m->state.pinWeightMlp = 1.0f;
			m->state.muscleWeightMlp = 1.0f;
			m->state.muscleDamperAdd = 0.0f;
		}
		// 清空死亡状态
		muscleWeightDecreaseFlag = false;
		// 碰撞以及角度限制相关 尚未完成
		if (angularLimitsEnabledOnKill)
		{
			angularLimits = false;
			angularLimitsEnabledOnKill = false;
		}
		if (internalCollisionsEnabledOnKill)
		{
			internalCollisions = false;
			internalCollisionsEnabledOnKill = false;
		}
		// Reset all
		BehavioursForeach(
			[](RagdollBehaviorComponentBase* b)
			{
				b->Resurrect();
			});
		SetAnimationEnabled(true);

		if (OnResurrection != nullptr)
			OnResurrection();
	}
	void RagdollComponent::InitRagdoll()
	{
		//mRoleIKCom->OnPreUpdate = std::bind<void>(&RoleIKComponent::UpdateTransform, mRoleIKCom);
	}
#ifdef ECHO_EDITOR
	void RagdollComponent::SaveDataFile(const Name& fileName)
	{
		std::string fileNameStr = fileName.toString();

		RagdollComponentInitData data;
		RagdollTools::ExportRagdollChildrenNodeAndData(this, data);

		data.SyncExport(fileNameStr);
		return;
	}
#endif


	void RagdollComponent::DisableRagdoll()
	{
		bool isDisabled = IsDisabled();
		behaviorMode = RagdollBehaviorMode::Disabled;
		if (!isDisabled)
		{
			DisableRagdollBlock();					
		}
	}
	void RagdollComponent::ToKinematic()
	{
		bool isActive = IsActive();
		bool isKinematic = IsKinematic();
		behaviorMode = RagdollBehaviorMode::Kinematic;

		if (!isActive)
		{
			// 激活物理
			ActiveRagdollBlock();
		}

		// 设置运动学
		if (!isKinematic)
		{
			for (auto& m : muscles)
			{
				if (!m->state.isDisconnected)
				{
					m->SetKinematic(true);
				}
			}
		}
	}
	void RagdollComponent::ToAlive()
	{
		if (!IsActive())
		{
			behaviorMode = RagdollBehaviorMode::_WaitToActive;
			targetBehviorMode = RagdollBehaviorMode::Alive;
			// 提前打开IK更新，收集一下运动速度
			if (mRoleIKCom)
			{
				mRoleIKCom->setKeepUpdate(true);
				mRoleIKCom->ManualUpdateTransform();
			}
			return;
		}

		bool isKinematic = IsKinematic();
		behaviorMode = RagdollBehaviorMode::Alive;
		if (isKinematic)
		{
			// 设置非运动学
			for (auto& m : muscles)
			{
				if (!m->state.isDisconnected)
				{
					m->SetKinematic(false);
				}
			}
		}

		ToAliveImpl();
		if (OnAlive) OnAlive();
	}
	void RagdollComponent::ImmediatelyToAlive()
	{
		bool isActive = IsActive();
		bool isKinematic = IsKinematic();
		behaviorMode = RagdollBehaviorMode::Alive;

		if (!isActive)
		{
			// 激活物理
			ActiveRagdollBlock();
		}

		// 设置非运动学
		if (isKinematic || !isActive)
		{
			for (auto& m : muscles)
			{
				if (!m->state.isDisconnected)
				{
					m->SetKinematic(false);
				}
			}
		}
		ToAliveImpl();
	}
	void RagdollComponent::ToDead()
	{
		if (!IsActive())
		{
			behaviorMode = RagdollBehaviorMode::_WaitToActive;
			targetBehviorMode = RagdollBehaviorMode::Dead;
			// 提前打开IK更新，收集一下运动速度
			if (mRoleIKCom)
			{
				mRoleIKCom->setKeepUpdate(true);
				mRoleIKCom->ManualUpdateTransform();
			}

			return;
		}

		bool isKinematic = IsKinematic();
		behaviorMode = RagdollBehaviorMode::Dead;
		// 设置非运动学
		if (isKinematic)
		{
			for (auto& m : muscles)
			{
				if (!m->state.isDisconnected)
				{
					m->SetKinematic(false);
				}
			}
		}
		ToDeadImpl ();
		if (OnDeathStart) OnDeathStart();
		if (muscleWeightDecreaseFlag == false && OnDeath) OnDeath();
	}
	void RagdollComponent::ImmediatelyToDead()
	{
		bool isActive = IsActive();
		bool isKinematic = IsKinematic();
		behaviorMode = RagdollBehaviorMode::Dead;
		if (!isActive)
		{
			// 激活物理
			ActiveRagdollBlock();
		}

		// 设置非运动学
		if (isKinematic || !isActive)
		{
			for (auto& m : muscles)
			{
				if (!m->state.isDisconnected)
				{
					m->SetKinematic(false);
				}
			}
		}
		ToDeadImpl();
		if (OnDeathStart) OnDeathStart();
		if (muscleWeightDecreaseFlag == false && OnDeath) OnDeath();
	}


	RagdollStateSettings RagdollComponent::GetStateSetting() const
	{
		return this->stateSettings;
	}
	void RagdollComponent::SetStateSetting(const RagdollStateSettings& stateSettings)
	{
		this->stateSettings = stateSettings;
	}

	void RagdollComponent::SlowDeath()
	{
		if (!muscleWeightDecreaseFlag)
			return;

		if (weightMlp > stateSettings.deadMuscleWeight) {
			weightMlp = Math::Max(weightMlp - EchoIK::Time::deltaTime * (weightRange / stateSettings.killDuration), stateSettings.deadMuscleWeight);

			for (auto m : muscles)
				m->state.muscleWeightMlp = weightMlp;

		}
		else
		{
			muscleWeightDecreaseFlag = false;

			for (auto& m : muscles)
				m->state.muscleWeightMlp = stateSettings.deadMuscleWeight;

			// Disable the Animator
			SetAnimationEnabled(false);

			BehavioursForeach(
				[](RagdollBehaviorComponentBase* b)
				{
					b->KillEnd();
				});

			if (OnDeath != nullptr) OnDeath();
		}
	}
	void RagdollComponent::SetAnimationEnabled(bool to)
	{
		animatorDisabled = false;
	}
	void RagdollComponent::ActivateRagdoll(bool kinematic)
	{
		for (auto& m : muscles) 
		{
			m->Reset();
		}

		for (auto& m : muscles) 
		{
			//m->joint->gameObject.setVisible(true);
			if (kinematic)
				m->rigidbody->SetCollisionDetection((uint8)CollisionDetectionMode::Discrete);
			m->SetKinematic(kinematic);
			m->rigidbody->SetVelocity(Vector3::ZERO);
			m->rigidbody->SetAngularVelocity(Vector3::ZERO);
		}

		FlagInternalCollisionsForUpdate();

		Read();

		for (auto& m : muscles) 
		{
			m->MoveToTarget();
		}
	}
	bool RagdollComponent::CanFreeze()
	{
		for (auto& m : muscles) 
		{
			if (m->rigidbody->GetVelocity().LengthSq() > stateSettings.maxFreezeSqrVelocity) 
				return false;
		}
		return true;
	}
	void RagdollComponent::Teleport(Vector3 position, Quaternion rotation, bool moveToTarget)
	{
		teleport = true;
		teleportPosition = position;
		teleportRotation = rotation;
		teleportMoveToTarget = moveToTarget;

		// If disabled, teleport immedietely.
		if (IsDisabled()) 
			Read();
	}
	void RagdollComponent::SetInternalCollisionsManual(bool collide, bool useInternalCollisionIgnores)
	{
		for (int i = 0; i < muscles.size(); i++)
		{
			for (int i2 = i; i2 < muscles.size(); i2++)
			{
				if (i != i2)
				{
					if (collide)
					{
						muscles[i]->ResetInternalCollisions(muscles[i2], useInternalCollisionIgnores);
					}
					else
					{
						muscles[i]->IgnoreInternalCollisions(muscles[i2]);
					}
				}
			}
		}
	}
	void RagdollComponent::SetAngularLimitsManual(bool limited)
	{
		for (int i = 0; i < muscles.size(); i++)
		{
			if (!muscles[i]->state.isDisconnected) muscles[i]->IgnoreAngularLimits(!limited);
		}
	}

	void RagdollComponent::OnDisable()
	{
		DisableRagdollBlock(false);

		//if (getVisibleState() && initiated)
		//{
		//	for (auto& m : muscles)
		//		m->Reset();
		//}
		hasBeenDisabled = true;
	}

	void RagdollComponent::OnEnable()
	{
		if (initiated && hasBeenDisabled)
		{
			hasBeenDisabled = false;
			if (IsActive())
				ActiveRagdollBlock();	
		}

	}
	void RagdollComponent::Awake()
	{
		if (start)
			return;
		if (mRoleIKCom == nullptr)
			return;
		if (!targetRoot.expired() && mRoleIKCom->isCreateFinish())
		{
			/*if (!initiated)
			{
				Initiate();
			}
			if (initiated)
			{
				OnEnable();
			}
			start = initiated;*/
			start = true;
		}
		else
			mRoleIKCom->GetRoleObject()->addRoleLoadListener(this);

	}

	bool RagdollComponent::OnRoleCreateFinish()
	{
		/*if (!initiated)
			Initiate();
		if (initiated)
		{
			OnEnable();
		}
		start = initiated;*/
		if (!initiated && !start) {
			start = true;
		}
		return true;
	}
	bool RagdollComponent::IsAwaked()
	{
		return this->start;
	}
	void RagdollComponent::Stop()
	{

		if (!start)
		{
			if (!targetRoot.expired() && mRoleIKCom != nullptr && mRoleIKCom->GetRoleObject() != nullptr && !mRoleIKCom->GetRoleObject()->isClean())
				mRoleIKCom->GetRoleObject()->removeRoleLoadListener(this);
			return;
		}

		{
			OnDisable();
			//for (auto& m : muscles)
			//{
			//	m->SetKinematic(true);
			//}
			Destroy();
		}

		start = false;
	}
	WorldNode * RagdollComponent::FindTargetRootRecursive(WorldNode * t)
	{
		if (t->getParent() == nullptr) 
			return nullptr;
		return FindTargetRootRecursive(t->getParent());
	}
	void RagdollComponent::EnableRoleUpdateBoxByAnim(bool Enable)
	{
		if (mRoleIKCom)
		{
			if (!mRagdollBlocks.empty())
			{
				RoleObject* roleObj = mRoleIKCom->GetRoleObject();
				if (roleObj)
				{
					RoleMovable* roleMvb = roleObj->getRoleMovable();
					if (roleMvb)
					{
						if (Enable)
						{
							BoneNode* rootBone = (BoneNode*)mRoleIKCom->getBoneNodeByName(mRagdollBlocks[0].BoneName);
							roleMvb->setUpdateBoxByAnim(true);
							std::optional<uint16> rootID;
							for (const auto& [id, bone] : mRoleIKCom->m_mapTransformsID)
							{
								if (bone == rootBone)
								{
									rootID = id;
									break;
								}
							}

							if (rootID != std::nullopt)
							{
								roleMvb->setUpdateBoxByAnimOnBone(rootID.value_or(0));
							}
						}
						else
						{
							roleMvb->setUpdateBoxByAnim(false);
							roleMvb->setUpdateBoxByAnimeOnDefultBone();
						}
					}
				}
			}
		}


	}
	void RagdollComponent::Initiate()
	{
		if (initiated)
			return;

		if (mRoleIKCom)
		{
			bool lastKeepUpdate = mRoleIKCom->getKeepUpdate();
			mRoleIKCom->setKeepUpdate(true);
			mRoleIKCom->ManualUpdateTransform();
			mRoleIKCom->setKeepUpdate(lastKeepUpdate);
		}

		initiated = false;

		if (this->IsMoveable() && mRoleIKCom->m_pRoleObjNode != nullptr)
		{
			this->setWorldTransform(DTransform(mRoleIKCom->m_pRoleObjNode->_getDerivedPosition(), mRoleIKCom->m_pRoleObjNode->_getDerivedOrientation()));
		}

		for (int i = 0; i < mRagdollBlocks.size(); ++i)
		{
			RagdollMuscle* muscle = new RagdollMuscle();

			muscle->joint = mRagdollBlocks[i].joint;
			muscle->target = (BoneNode*) mRoleIKCom->getBoneNodeByName(mRagdollBlocks[i].BoneName);

			if (muscle->target == nullptr) {
				break;
			}

			muscle->attribute->group = mRagdollBlocks[i].group;
			muscle->attribute->mappingWeight = mRagdollBlocks[i].mapping;
			muscle->attribute->pinWeight = mRagdollBlocks[i].pin;
			muscle->attribute->muscleWeight = mRagdollBlocks[i].weight;
			muscle->attribute->muscleDamper = mRagdollBlocks[i].damper;
			muscle->attribute->mapPosition = mRagdollBlocks[i].mapPosition;

			if (muscle->joint != nullptr)
			{
				mRagdollBlocks[i].muscle = muscle;
				muscles.push_back(muscle);
			}
		}
		if (muscles.size() != mRagdollBlocks.size()) {
			Destroy();
			return;
		}

		// 检查没有连接对象的关节，设置他的线性约束为自由
		if (IsBonePartIK(mRoleIKCom))
		{
			for (int i = 0; i < muscles.size(); i++)
			{
				RagdollMuscle* muscle = muscles[i];
				if (muscle->joint->GetConnectedRigidBodyComp().Expired())
				{
					muscle->joint->SetMotionX(JointMotion::FREE);
					muscle->joint->SetMotionY(JointMotion::FREE);
					muscle->joint->SetMotionZ(JointMotion::FREE);
				}
			}
		}

		for (int i = 0; i < muscles.size(); i++)
		{  
			// Initiating the muscle
			muscles[i]->Initiate(muscles,this);

			// jointListener 
			muscles[i]->jointListener = EchoWrapSharedPtr<_RagdollMuscle_JointComponentListener>(new _RagdollMuscle_JointComponentListener());
			muscles[i]->jointListener->muscleIndex = i;
			muscles[i]->jointListener->pRagdoll = this->weak_from_this();
			muscles[i]->joint->AddJointCallBack(muscles[i]->jointListener.get());
			// colliderBroadcast
			muscles[i]->colliderBroadcaster = EchoWrapSharedPtr<RagdollMuscleColliderBroadcaster>(new RagdollMuscleColliderBroadcaster(this,i));
			ragdollBlockData* block = FindBoneBlock(muscles[i]->target->getName());
			if (block != nullptr && !block->collider.expired())
			{
				((ColliderComponent*)block->collider.get())->AddColliderCallBack(muscles[i]->colliderBroadcaster.get());
			}
		}
		
		if (!IsValid(true))
		{
			LogManager::instance()->logMessage("-error-\tEcho::RagdollComponent::Initiate error!");
			return;
		}

		UpdateHierarchies();

		hierarchyIsFlat = HierarchyIsFlat();

		FlagInternalCollisionsForUpdate();
		FlagAngularLimitsForUpdate();

		initiated = true;

		BehavioursForeach(
			[](RagdollBehaviorComponentBase* b)
			{
				b->Initiate();
			});
		
		RagdollBehaviorManagerComponent* bm = GetBehavioursManager();
		if (bm != nullptr)
		{
			RagdollBehaviorComponentBase* db = bm->GetDefaultBehavior();
			if (db != nullptr)
				db->Activate();
		}

		for (auto& m : muscles)
		{
			m->Read();
		}

		// Mapping
		StoreTargetMappedState();

		defaultMuscles.resize(muscles.size());
		for (uint8 i = 0;i < muscles.size();++i)
		{
			defaultMuscles[i] = muscles[i];
		}

		if (OnPostInitiate != nullptr) OnPostInitiate();

		/*if (!autoSimulate()) 
			mRoleIKCom->enabled = false;*/

		SetAutoCalculateLocalPose(false);
	}
	void RagdollComponent::Destroy()
	{
		initiated = false;

		for (int i = 0; i < mRagdollBlocks.size(); ++i)
			mRagdollBlocks[i].muscle = nullptr;

		for (int i = 0; i < muscles.size(); i++)
		{
			muscles[i]->jointListener = nullptr;
			SAFE_DELETE(muscles[i]);
		}
		muscles.clear();

		disconnectMuscleFlags.clear();
		muscleDisconnectModes.clear();
		disconnectDeactivateFlags.clear();
		reconnectMuscleFlags.clear();
		defaultMuscles.clear();

		BehavioursForeach(
			[](RagdollBehaviorComponentBase* b)
			{
				// TO DO 
			});

		defaultMuscles.clear();

		SetAutoCalculateLocalPose(true);

	}
	void RagdollComponent::OnRebuild()
	{
		rebuildFlag = false;

		if (IsDisabled())
		{
			//Debug.LogError("Can not rebuild a puppet in Disabled mode");
			return;
		}


		rebuildPelvisPos = DBV2FV(defaultMuscles[0]->target->_getDerivedPosition());
		rebuildPelvisRot = defaultMuscles[0]->target->_getDerivedOrientation();

		for (auto m : defaultMuscles)
		{
			m->Rebuild();
		}

		//for (auto m : defaultMuscles)
		//{
		//	if (!ContainsJoint(m->joint))
		//	{
		//		AddMuscle(m->joint, m->target, m->rebuildConnectedBody, m->rebuildTargetParent);
		//	}
		//}

		FlagInternalCollisionsForUpdate();
		FlagAngularLimitsForUpdate();

		BehavioursForeach(
			[](RagdollBehaviorComponentBase* b)
			{
				b->OnReactivate();
			});

		onPostRebuildFlag = true;
	}
	void RagdollComponent::SetSolverIterationCount(int count)
	{
		this->solverIterationCount = count;

	}
	int RagdollComponent::GetSolverIterationCount()const
	{
		return this->solverIterationCount;
	}
	void RagdollComponent::FixedUpdate()
	{
		BehavioursForeach(
			[](RagdollBehaviorComponentBase* b)
			{
				b->FixedUpdateB(EchoIK::Time::deltaTime);
			});

		SetRagdollEnableFlag(IsActive());
		if (!mRoleIKCom || targetRoot.expired() || !initiated || !start || !autoSimulate() || !EchoPhysicsSystem::instance()->GetPhysicsManager(this)->PhysicsIsRunning())
			return;

		if (rebuildFlag) 
			OnRebuild();

		ProcessDisconnects();
		ProcessReconnects();

		if (muscles.size() <= 0)
			return;

		interpolated = IsInterpolated();

		fixedFrame = true;
		if (!IsActive())
		{
			if (teleport)
				Read();
			return;
		}

		pinWeight = Math::Clamp(pinWeight, 0.0f, 1.0f);
		muscleWeight = Math::Clamp(muscleWeight, 0.0f, 1.0f);
		muscleSpring = Math::Clamp(muscleSpring, 0.0f, muscleSpring);
		muscleDamper = Math::Clamp(muscleDamper, 0.0f, muscleDamper);
		pinPow = Math::Clamp(pinPow, 1.0f, 8.0f);
		pinDistanceFalloff = Math::Max(pinDistanceFalloff, 0.0f);

		// If updating the Animator manually here in FixedUpdate
		if (updateMode() == RagdollUpdateMode::Mode_FixedUpdate)
		{
			FixTargetTransforms();

			if (OnRead != nullptr) 
				OnRead();
			BehavioursForeach(
				[](RagdollBehaviorComponentBase* b)
				{
					b->OnRead(EchoIK::Time::deltaTime);
				});
			Read();
			readInFixedUpdate = true;
		}

		//if (!IsFrozen())
		{
			// Update internal collision ignoring
			UpdateInternalCollisions();

			// Update angular limit ignoring
			UpdateAngularLimits();

			// Set solver iteration count
			if (solverIterationCount != lastSolverIterationCount)
			{
				for (int i = 0; i < muscles.size(); i++)
				{
					muscles[i]->rigidbody->SetSolverIterations(solverIterationCount);
				}

				lastSolverIterationCount = solverIterationCount;
			}

			float deltaTime = EchoIK::Time::fixedDeltaTime;

			// Update Muscles
			for (int i = 0; i < muscles.size(); i++)
			{
				muscles[i]->Update(pinWeight, muscleWeight, muscleSpring, muscleDamper, pinPow, pinDistanceFalloff, true, angularPinning, deltaTime);
			}
		}

		// Fix transforms to be sure of not having any drifting when the target bones are not animated
		if (updateMode() == RagdollUpdateMode::Mode_AnimatePhysics) 
			FixTargetTransforms();
	}
	void RagdollComponent::Update()
	{
		BehavioursForeach(
			[](RagdollBehaviorComponentBase* b)
			{
				b->UpdateB(EchoIK::Time::deltaTime);
			});

		if (!mRoleIKCom || targetRoot.expired() || !EchoPhysicsSystem::instance()->GetPhysicsManager(this)->PhysicsIsRunning())
			return;

		if (start && isLoadData && !initiated )
		{
			Initiate();
			if (initiated)
			{
				if (!IsActive())
				{
					behaviorMode = RagdollBehaviorMode::_WaitToActive;
					DisableRagdoll();
				}
				if (IsKinematic())
				{
					behaviorMode = RagdollBehaviorMode::_WaitToActive;
					ToKinematic();
				}
				if (IsAlive())
				{
					behaviorMode = RagdollBehaviorMode::_WaitToActive;
					ImmediatelyToAlive();
				}
				if (IsDead())
				{
					behaviorMode = RagdollBehaviorMode::_WaitToActive;
					ImmediatelyToDead();
				}

				// OnEnable();
			}
			else // 初始化失败
			{
				start = false;
			}
		}

		if (!start || !initiated || !autoSimulate() || muscles.size() <= 0)
			return;
		if (mRoleIKCom && IsActive())
		{
			mRoleIKCom->ManualUpdateTransform();
		}

		if (mappingBlendIncreaseFlag)
			SlowActive();
		if (muscleWeightDecreaseFlag)
			SlowDeath();

		if (updateMode() != RagdollUpdateMode::Mode_Normal) 
			return;

		//FixTargetTransforms();
	}
	void RagdollComponent::LateUpdate()
	{
		BehavioursForeach(
			[](RagdollBehaviorComponentBase* b)
			{
				b->LateUpdateB(EchoIK::Time::deltaTime);
			});

		if (!EchoPhysicsSystem::instance()->GetPhysicsManager(this)->PhysicsIsRunning())
			return;
		if (!mRoleIKCom || targetRoot.expired() || !start || !initiated ||  !autoSimulate() || muscles.size() <= 0)
			return;

		OnLateUpdate();

		if (onPostRebuildFlag)
		{
			defaultMuscles[0]->target->_setDerivedPosition(rebuildPelvisPos);
			defaultMuscles[0]->target->_setDerivedOrientation(rebuildPelvisRot);

			for (auto& m : muscles)
			{
				m->MoveToTarget();
				m->ClearVelocities();
			}

			onPostRebuildFlag = false;
		}

		if (OnPostLateUpdate != nullptr) 
			OnPostLateUpdate();
	}
	void RagdollComponent::OnLateUpdate()
	{
		if (!initiated) 
			return;

		if (animatorDisabled)
			animatorDisabled = false;

		bool animationApplied = updateMode() == RagdollUpdateMode::Mode_Normal || (!readInFixedUpdate && fixedFrame);
		readInFixedUpdate = false;
		bool muscleRead = animationApplied && IsActive(); // If disabled, reading will be done in PuppetMasterModes.cs
	
		if (animationApplied)
		{
			if (OnRead != nullptr) OnRead(); // Update IK
			BehavioursForeach(
				[](RagdollBehaviorComponentBase* b)
				{
					b->OnRead(EchoIK::Time::deltaTime);
				});
		}
		if (muscleRead)
			Read();

		switch (updateMode())
		{
		case RagdollUpdateMode::Mode_FixedUpdate:
			if (!fixedFrame && !interpolated) 
				return;
			break;
		case RagdollUpdateMode::Mode_AnimatePhysics:
			if (!fixedFrame && !interpolated) 
				return;
			break;
		default:
			break;
		}

		// Below is common code for all update modes! For AnimatePhysics modes the following code will run only in fixed frames
		fixedFrame = false;

		// Mapping
		//if (!IsFrozen())
		{
			mappingWeight = Math::Clamp(mappingWeight, 0.0f, 1.0f);
			float mW = mappingWeight * mappingBlend;

			if (IsKinematic())
			{
				// Moving to Target when in Kinematic mode
				MoveToTarget();
			}
			else if (mW > 0.0f)
			{
				if (IsActive() && mRoleIKCom->isEnable())
				{
					for (int i = 0; i < muscles.size(); i++)
					{
						muscles[i]->Map(mW);
					}
				}
			}

			BehavioursForeach(
				[](RagdollBehaviorComponentBase* b)
				{
					b->OnWrite(EchoIK::Time::deltaTime);
				});
			if (OnWrite != nullptr) OnWrite();

			if (!IsDisabled()) // 不是禁用，就要读取速度
			{
				StoreTargetMappedState(); 
				for (auto& m : muscles)
				{
					m->CalculateMappedVelocity();
				}

				if (lastStoreTargetMappedStateIsValid)
				{
					lastCalculateMappedVelocityIsValid = true;
				}
				lastStoreTargetMappedStateIsValid = true;

			}
			else // 失效
			{		
				lastCalculateMappedVelocityIsValid = false;
				lastStoreTargetMappedStateIsValid = false;
			}

			if (behaviorMode == RagdollBehaviorMode::_WaitToActive && lastCalculateMappedVelocityIsValid)
			{
				WaitModeToTagertMode();
			}

		}

		if (mapDisconnectedMuscles)
		{
			for (int i = 0; i < muscles.size(); i++)
			{
				muscles[i]->MapDisconnected();
			}
		}
	}
	void RagdollComponent::WaitModeToTagertMode()
	{
		switch (targetBehviorMode)
		{
		case RagdollBehaviorMode::Alive:
			ImmediatelyToAlive();
			WaitModeCopyVelocity();
			if (OnAlive) OnAlive();
			break;
		case RagdollBehaviorMode::Dead:
			ImmediatelyToDead();
			WaitModeCopyVelocity();
			break;
		default:
			DisableRagdoll();
			return;
		}


	}
	void RagdollComponent::WaitModeCopyVelocity()
	{
		for (auto& m : muscles)
		{
			if (!m->state.isDisconnected)
			{
				m->SetKinematic(false);
				m->rigidbody->WakeUp();
				m->rigidbody->SetVelocity(m->mappedVelocity);
				m->rigidbody->SetAngularVelocity(m->mappedAngularVelocity);
			}
		}
	}
	void RagdollComponent::MoveToTarget()
	{
		for (auto& m : muscles)
		{
			m->MoveToTarget();
		}
	}

	void RagdollComponent::RoleMove() {
		if (!IsAlive()) return;
		Vector3 curPos = targetRoot->getWorldPosition();
		Quaternion curRot = targetRoot->getWorldRotation();
		if ((curPos - lastTargetRootPosition).isAlmostZero() && curRot.isEqual(lastTargetRootRotation, 0.00001f)) {
			return;
		}
		Quaternion lastRotInv = lastTargetRootRotation.GetInverse();
		for (int i = 0; i < muscles.size(); i++)
		{
			muscles[i]->RoleMove(lastTargetRootPosition, lastRotInv, curPos, curRot, angularPinning);
		}


	}

	void RagdollComponent::SetRagdollEnableFlag(bool pFlag)
	{
		if (mRoleIKCom != nullptr)
		{
			mRoleIKCom->mRagdollEnable = pFlag;
			if (pFlag)
			{
				mRoleIKCom->ImmediatelyUpdateIK();
			}
		}
	}


	void RagdollComponent::SetBlendTime(float time)
	{
		blendTime = std::max(0.f,time);
	}
	float RagdollComponent::GetBlendTime()const
	{
		return blendTime;
	}
	void RagdollComponent::SetFixTargetTransforms(bool fixTargetTransforms)
	{
		this->fixTargetTransforms = fixTargetTransforms;
		//FixTargetTransforms();
	}
	bool RagdollComponent::IsFixTargetTransforms()const
	{
		return this->fixTargetTransforms;
	}
	void RagdollComponent::Read()
	{
		lastTargetRootPosition = targetRoot->getWorldPosition();
		lastTargetRootRotation = targetRoot->getWorldRotation();
		if (teleport)
		{
			ActorPtr act = getActor();
			ActorComponentPtr t = act->addComponent(COMPONENTTYPE(NodeComponent), "Temp", act->getRootNodeCom(), nullptr, 0, true);

			t.get<NodeComponent>()->setWorldPosition(getParentNodeCom() == nullptr ? getParentNodeCom()->getWorldPosition() : DBVector3::ZERO);
			t.get<NodeComponent>()->setWorldRotation(getParentNodeCom() == nullptr ? getParentNodeCom()->getWorldRotation() : Quaternion::IDENTITY);
			
			ActorComponentPtr parent = getParentNodeCom();
			ActorComponentPtr tragetParent = targetRoot->getParentNodeCom();
			t->addSubComponent(weak_from_this());
			t->addSubComponent(targetRoot);

			Vector3 p = getParentNodeCom()->getWorldPosition();

			ComponentNode* parentComNode = getParentComponentNode();
			Quaternion targetDeltaRotation = Quaternion::QFromToRotation(targetRoot->getWorldRotation(), teleportRotation);
			parentComNode->setWorldRotation(targetDeltaRotation * getParentNodeCom()->getWorldRotation());

			Vector3 targetDeltaPosition = teleportPosition - targetRoot->getWorldPosition();
			parentComNode->setWorldPosition(getParentNodeCom()->getWorldPosition() + targetDeltaPosition);

			parent->addSubComponent(weak_from_this());
			tragetParent->addSubComponent(targetRoot);
			act->delComponent(t);

			muscles[0]->targetMappedPosition = p + targetDeltaRotation * (muscles[0]->targetMappedPosition - p) + targetDeltaPosition;
			muscles[0]->targetSampledPosition = p + targetDeltaRotation * (muscles[0]->targetSampledPosition - p) + targetDeltaPosition;

			muscles[0]->targetMappedRotation = targetDeltaRotation * muscles[0]->targetMappedRotation;
			muscles[0]->targetSampledRotation = targetDeltaRotation * muscles[0]->targetSampledRotation;

			if (teleportMoveToTarget)
			{
				for(auto& m : muscles)
				{
					m->MoveToTarget();
				}
			}

			for (auto& m : muscles)
			{
				m->ClearVelocities();
			}

			BehavioursForeach(
				[&](RagdollBehaviorComponentBase* b)
				{
					b->OnTeleport(targetDeltaRotation, targetDeltaPosition, p, teleportMoveToTarget);
				});

			teleport = false;

		}

		if (!IsAlive())
			return;
		for (auto& m : muscles)
		{
			m->Read();
		}

		if (IsAlive() && updateJointAnchors)
		{
			for (int i = 0; i < muscles.size(); i++)
			{
				muscles[i]->UpdateAnchor(supportTranslationAnimation);
			}
		}
	}
	void RagdollComponent::FixTargetTransforms()
	{
		if (!IsAlive()) 
			return;
		if (OnFixTransforms != nullptr) 
			OnFixTransforms();

		BehavioursForeach(
			[](RagdollBehaviorComponentBase* b)
			{
				b->OnFixTransforms();
			});

		if (!fixTargetTransforms && !hasProp)
			return;
		if (!IsActive())
			return;

		mappingWeight = Math::Clamp(mappingWeight, 0.0f, 1.0f);

		float mW = mappingWeight * mappingBlend;
		if (mW <= 0.0f) 
			return;

		for (int i = 0; i < muscles.size(); i++)
		{
			if (fixTargetTransforms || muscles[i]->attribute->group == RagdollMuscle::MuscleGroup::Prop)
			{
				muscles[i]->FixTargetTransforms();
			}
		}
	}
	void RagdollComponent::FlagInternalCollisionsForUpdate()
	{
		if (manualInternalCollisionControl) 
			return;
		internalCollisionsEnabled = !internalCollisions;
	}
	void RagdollComponent::UpdateInternalCollisions()
	{
		if (manualInternalCollisionControl) 
			return;
		if (internalCollisionsEnabled == internalCollisions) 
			return;

		if (internalCollisions) 
			ResetInternalCollisions();
		else 
			IgnoreInternalCollisions();
	}
	void RagdollComponent::UpdateInternalCollisions(RagdollMuscle * m)
	{
		if (manualInternalCollisionControl) 
			return;

		for (auto otherMuscle : muscles)
		{
			if (otherMuscle != m)
			{
				if (internalCollisions)
				{
					m->ResetInternalCollisions(otherMuscle, true);
				}
				else
				{
					m->IgnoreInternalCollisions(otherMuscle);
				}
			}
		}
	}
	void RagdollComponent::IgnoreInternalCollisions()
	{
		if (manualInternalCollisionControl)
			return;

		for (int i = 0; i < muscles.size(); i++)
		{
			for (int i2 = i; i2 < muscles.size(); i2++)
			{
				if (i != i2)
				{
					muscles[i]->IgnoreInternalCollisions(muscles[i2]);
				}
			}
		}

		internalCollisions = false;
		internalCollisionsEnabled = false;
	}
	void RagdollComponent::IgnoreInternalCollisions(RagdollMuscle * m)
	{
		if (manualInternalCollisionControl)
			return;

		for (auto otherMuscle : muscles)
		{
			if (otherMuscle != m)
			{
				m->IgnoreInternalCollisions(otherMuscle);
			}
		}
	}
	void RagdollComponent::ResetInternalCollisions()
	{
		if (manualInternalCollisionControl) 
			return;

		for (int i = 0; i < muscles.size(); i++)
		{
			for (int i2 = i; i2 < muscles.size(); i2++)
			{
				if (i != i2)
				{
					muscles[i]->ResetInternalCollisions(muscles[i2], true);
				}
			}
		}

		internalCollisions = true;
		internalCollisionsEnabled = true;
	}
	void RagdollComponent::ResetInternalCollisions(RagdollMuscle * m, bool useInternalCollisionIgnores)
	{
		if (manualInternalCollisionControl) 
			return;

		for (auto otherMuscle : muscles)
		{
			if (otherMuscle != m)
			{
				m->ResetInternalCollisions(otherMuscle, useInternalCollisionIgnores);
			}
		}
	}
	void RagdollComponent::FlagAngularLimitsForUpdate()
	{
		if (manualAngularLimitControl) 
			return;
		angularLimitsEnabled = !angularLimits;
	}
	void RagdollComponent::UpdateAngularLimits()
	{
		if (manualAngularLimitControl) 
			return;
		if (angularLimitsEnabled == angularLimits) 
			return;

		for (int i = 0; i < muscles.size(); i++)
		{
			if (!muscles[i]->state.isDisconnected) muscles[i]->IgnoreAngularLimits(!angularLimits);
		}

		angularLimitsEnabled = angularLimits;
	}
	void RagdollComponent::DisconnectMuscleRecursive(int index, RagdollMuscleDisconnectMode disconnectMode, bool deactivate)
	{
		if (index < 0 || index >= muscles.size())
			return;

		disconnectMuscleFlags[index] = true;
		muscleDisconnectModes[index] = disconnectMode;
		disconnectDeactivateFlags[index] = deactivate;
	}
	void RagdollComponent::ReconnectMuscleRecursive(int index)
	{
		if (index < 0 || index >= muscles.size())
			return;

		if (index > 0) 
			index = GetHighestDisconnectedParentIndex(index);

		reconnectMuscleFlags[index] = true;
	}
	void RagdollComponent::RemoveMuscleRecursive(ConfigurableJointComponent* joint, bool attachTarget, bool blockTargetAnimation , RagdollMuscleRemoveMode removeMode )
	{
		if (!CheckIfInitiated()) return;

		if (joint == nullptr || !ContainsJoint(joint))
			return;
		
		int index = GetMuscleIndex(joint);
		std::vector<RagdollMuscle*> newMuscles;
		
		for (int i = 0; i < muscles.size(); i++)
		{
			if (i != index && !muscles[index]->childFlags[i])
				newMuscles.push_back(muscles[i] );

			else
			{
				// TO DO
				//if (muscles[i].broadcaster != null)
				//{
				//	muscles[i].broadcaster.enabled = false;
				//	Destroy(muscles[i].broadcaster);
				//}
				//if (muscles[i].jointBreakBroadcaster != null)
				//{
				//	muscles[i].jointBreakBroadcaster.enabled = false;
				//	Destroy(muscles[i].jointBreakBroadcaster);
				//}

			}
		}
		
		RagdollMuscle::JointBreakFlag animeFlag = blockTargetAnimation ? RagdollMuscle::animeBlock:RagdollMuscle::noBreak;
		switch (removeMode)
		{
		case RagdollMuscleRemoveMode::Sever:
			joint->setVisible(false);

			muscles[index]->breakFlag = RagdollMuscle::breakRoot | animeFlag;
			for (int i = 0; i < muscles[index]->childIndexes.size(); i++)
			{
				KillJoint(muscles[muscles[index]->childIndexes[i]]->joint.get());
				muscles[muscles[index]->childIndexes[i]]->breakFlag = RagdollMuscle::breakItem | animeFlag;
			}
			break;
		case RagdollMuscleRemoveMode::Explode:
			joint->setVisible(false);
			muscles[index]->breakFlag = RagdollMuscle::breakRoot | animeFlag;

			for (int i = 0; i < muscles[index]->childIndexes.size(); i++)
			{
				muscles[muscles[index]->childIndexes[i]]->joint->setVisible(false);
				muscles[muscles[index]->childIndexes[i]]->breakFlag = RagdollMuscle::breakRoot | animeFlag;
			}
			break;
		case RagdollMuscleRemoveMode::Numb:
			KillJoint(joint);
			muscles[index]->breakFlag = RagdollMuscle::breakItem;

			for (int i = 0; i < muscles[index]->childIndexes.size(); i++)
			{
				KillJoint(muscles[muscles[index]->childIndexes[i]]->joint.get());
				muscles[muscles[index]->childIndexes[i]]->breakFlag = RagdollMuscle::breakItem | animeFlag;
			}
			break;
		}

		if (attachTarget)
		{
			auto* transform = muscles[index]->GetTransform();
			if (transform)
			{
				transform->getActor()->getRootNodeCom()->addSubComponent(transform->weak_from_this());

				for (int i = 0; i < muscles[index]->childIndexes.size(); i++)
				{
					auto* childTransform = muscles[muscles[index]->childIndexes[i]]->GetTransform();
					if (removeMode == RagdollMuscleRemoveMode::Explode || childTransform->getParentNodeCom() == this)
					{
						childTransform->getActor()->getRootNodeCom()->addSubComponent(childTransform->weak_from_this());
					}
				}
			}
		}
		
		if (OnMuscleRemoved != nullptr) OnMuscleRemoved(muscles[index]);
		for (int i = 0; i < muscles[index]->childIndexes.size(); i++)
		{
			auto c = muscles[muscles[index]->childIndexes[i]];
			if (OnMuscleRemoved != nullptr) OnMuscleRemoved(c);
		}

		// Enable collisions between the new muscles and the removed colliders
		if (!internalCollisionsEnabled)
		{
			for (RagdollMuscle* newMuscle : newMuscles)
			{
				newMuscle->ResetInternalCollisions(muscles[index], false);

				for (int childMuscleIndex = 0; childMuscleIndex < muscles[index]->childIndexes.size(); childMuscleIndex++)
				{
					newMuscle->ResetInternalCollisions(muscles[childMuscleIndex], false);
				}
			}
		}

		UpdateHierarchies();
	}
	void RagdollComponent::FixMusclePositions()
	{
		for (auto& m : muscles) {
			if (m->GetTransformNode() != nullptr && m->target != nullptr) {
				m->GetTransformNode()->setWorldPosition(DBV2FV(m->target->_getDerivedPosition()));
			}
		}
	}
	void RagdollComponent::FixMusclePositionsAndRotations()
	{
		for (auto& m : muscles)
		{
			if (m->GetTransformNode() != nullptr && m->target != nullptr)
			{
				m->GetTransformNode()->setWorldPosition(DBV2FV(m->target->_getDerivedPosition()));
				m->GetTransformNode()->setWorldRotation(m->target->_getDerivedOrientation());
			}
		}
	}
	bool RagdollComponent::HierarchyIsFlat()
	{
		for (auto& m : muscles)
		{
			if (m->GetTransform()->getParentNodeCom() != this)
				return false;
		}
		return true;
	}
	int RagdollComponent::GetHighestDisconnectedParentIndex(int index)
	{
		for (int i = (int)muscles[index]->parentIndexes.size() - 1; i > -1; i--)
		{
			int parentIndex = muscles[index]->parentIndexes[i];
			if (muscles[parentIndex]->state.isDisconnected)
				return parentIndex;
		}

		return index;
	}
	void RagdollComponent::ProcessDisconnects()
	{
		for (int i = 0; i < disconnectMuscleFlags.size(); i++)
		{
			if (disconnectMuscleFlags[i]) 
				OnDisconnectMuscleRecursive(i, muscleDisconnectModes[i], disconnectDeactivateFlags[i]);
		}

		for (int i = 0; i < disconnectMuscleFlags.size(); i++)
		{
			disconnectMuscleFlags[i] = false;
			disconnectDeactivateFlags[i] = false;
		}
	}
	void RagdollComponent::ProcessReconnects()
	{
		for (int i = 0; i < reconnectMuscleFlags.size(); i++)
		{
			if (reconnectMuscleFlags[i]) 
				OnReconnectMuscleRecursive(i);
		}

		for (int i = 0; i < reconnectMuscleFlags.size(); i++)
		{
			reconnectMuscleFlags[i] = false;
		}
	}
	void RagdollComponent::OnReconnectMuscleRecursive(int index)
	{
		if (index == 0)
		{
			//state = RagdollState::Alive;

			for (auto& m : muscles)
			{
				if (m->isPropMuscle) continue;
				m->state.isDisconnected = false;
				m->FixTargetTransforms();
			}

			for (auto& m : muscles)
			{
				if (m->isPropMuscle) continue;

				m->Reset();
				m->Read();
				m->ClearVelocities();
			}
		}

		ReconnectMuscle(muscles[index]);

		for (int i = 0; i < muscles[index]->childIndexes.size(); i++)
		{
			int childIndex = muscles[index]->childIndexes[i];

			if (muscles[childIndex]->isPropMuscle) continue;

			ReconnectMuscle(muscles[childIndex]);
		}
	}
	void RagdollComponent::KillJoint(ConfigurableJointComponent * joint)
	{
		if (joint == nullptr)
			return;
		joint->SetTargetRotation(Vector3::ZERO);
		JointDrive j;
		j.spring = 0.0f;
		j.damper = 0.0f;

		joint->SetSlerpDrive(j);
	}
	void RagdollComponent::SlowActive()
	{
		if (mappingBlend < 1.f)
		{
			mappingBlend = Math::Clamp(mappingBlend + EchoIK::Time::deltaTime / blendTime, 0.f, 1.f);
		}
		else
		{
			mappingBlend = 1.f;

			mappingBlendIncreaseFlag = false;
		}
	}
	void RagdollComponent::SetGlobalMappingWeight(float weight)
	{
		this->mappingWeight = weight;
	}
	void RagdollComponent::SetGlobalPinWeight(float weight)
	{
		this->pinWeight = weight;
	}
	void RagdollComponent::SetGlobalMuscleWeight(float weight)
	{
		this->muscleWeight = weight;
	}
	float RagdollComponent::GetGlobalMappingWeight()const
	{
		return this->mappingWeight;
	}
	float RagdollComponent::GetGlobalPinWeight()const
	{
		return this->pinWeight;
	}
	float RagdollComponent::GetGlobalMuscleWeight()const
	{
		return this->muscleWeight;
	}
	void RagdollComponent::SetGlobalMuscleSpring(float spring)
	{
		this->muscleSpring = spring;
	}
	void RagdollComponent::SetGlobalMuscleDamper(float damper)
	{
		this->muscleDamper = damper;
	}
	float RagdollComponent::GetGlobalMuscleSpring()const
	{
		return this->muscleSpring;
	}
	float RagdollComponent::GetGlobalMuscleDamper()const
	{
		return this->muscleDamper;
	}
	void RagdollComponent::SetPinPow(float pow)
	{
		this->pinPow = pow;
	}
	void RagdollComponent::SetPinDistanceFalloff(float val)
	{
		this->pinDistanceFalloff = val;
	}
	void RagdollComponent::SetAngularPinning(bool val)
	{
		this->angularPinning = val;
	}
	void RagdollComponent::SetUpdateJointAnchors(bool val)
	{
		this->updateJointAnchors = val;
	}
	void RagdollComponent::SetSupportTranslationAnimation(bool val)
	{
		this->supportTranslationAnimation = val;
	}
	void RagdollComponent::SetMaxAngularVelocity(float val)
	{
		this->maxAngularVelocity = val;
	}
	void RagdollComponent::SetMaxLinearVelocity(float val)
	{
		this->maxLinearVelocity = val;
	}
	void RagdollComponent::SetRagdollDensity(float val)
	{
		this->ragdollDensity = val;
	}
	void RagdollComponent::SetAngularLimits(bool val)
	{
		this->angularLimits = val;
	}
	void RagdollComponent::SetInternalCollisions(bool val)
	{
		this->internalCollisions = val;
	}
	float RagdollComponent::GetPinPow()const
	{
		return this->pinPow;
	}
	float RagdollComponent::GetPinDistanceFalloff()const
	{
		return this->pinDistanceFalloff;
	}
	float RagdollComponent::GetMaxAngularVelocity()const
	{
		return this->maxAngularVelocity;
	}
	float RagdollComponent::GetMaxLinearVelocity()const
	{
		return this->maxLinearVelocity;
	}
	float RagdollComponent::GetRagdollDensity()const
	{
		return this->ragdollDensity;
	}
	bool RagdollComponent::GetAngularPinning()const
	{
		return this->angularPinning;
	}
	bool RagdollComponent::GetUpdateJointAnchors()const
	{
		return this->updateJointAnchors;
	}
	bool RagdollComponent::GetSupportTranslationAnimation()const
	{
		return this->supportTranslationAnimation;
	}
	bool RagdollComponent::GetAngularLimits()const
	{
		return this->angularLimits;
	}
	bool RagdollComponent::GetInternalCollisions()const
	{
		return this->internalCollisions;
	}
	uint16 RagdollComponent::SetMuscleAttribute(const RagdollMuscleModify& modfiy)
	{
		ragdollBlockData* block = FindBoneBlock(modfiy.muscleName);
		RagdollMuscle*	  muscle = GetMuscle(modfiy.muscleName);
		if (block == nullptr || (initiated && muscle == nullptr))
			return MuscleModfiy_None;
		
		uint16	resFlag = MuscleModfiy_None;

		uint16	flag = modfiy.modifyFlag;
		const RagdollMuscleModifyValue&	newValue = modfiy.muscleValue;

		if (flag & MuscleAttributeModfiy_Group)
		{
			block->group = newValue.group;
			if (muscle) muscle->attribute->group = newValue.group;
			resFlag |= MuscleAttributeModfiy_Group;
		}
		if (flag & MuscleAttributeModfiy_Mapping)
		{
			if (newValue.mapping >= 0.f && newValue.mapping <= 1.f)
			{
				block->mapping = newValue.mapping;
				if (muscle) muscle->attribute->mappingWeight = newValue.mapping;
				resFlag |= MuscleAttributeModfiy_Mapping;
			}
		}
		if (flag & MuscleAttributeModfiy_Pin)
		{
			if (newValue.pin >= 0.f && newValue.pin <= 1.f)
			{
				block->pin = newValue.pin;
				if (muscle) muscle->attribute->pinWeight = newValue.pin;
				resFlag |= MuscleAttributeModfiy_Pin;
			}
		}
		if (flag & MuscleAttributeModfiy_Weight)
		{
			if (newValue.weight >= 0.f && newValue.weight <= 1.f)
			{
				block->weight = newValue.weight;
				if (muscle) muscle->attribute->muscleWeight = newValue.weight;
				resFlag |= MuscleAttributeModfiy_Weight;
			}
		}
		if (flag & MuscleAttributeModfiy_Damper)
		{
			if (newValue.damper >= 0.f && newValue.damper <= 1.f)
			{
				block->damper = newValue.damper;
				if (muscle)	muscle->attribute->muscleDamper = newValue.damper;
				resFlag |= MuscleAttributeModfiy_Damper;
			}
		}
		return resFlag;
	}
	void RagdollComponent::GetMuscleAttribute(RagdollMuscleModify& data)
	{
		ragdollBlockData*	block = FindBoneBlock(data.muscleName);
		RagdollMuscle*		muscle = GetMuscle(data.muscleName);
		if (block == nullptr || (initiated && muscle == nullptr))
		{
			data.modifyFlag = MuscleModfiy_None;
			return;
		}
		uint16 flag = data.modifyFlag;
			
		if (flag & MuscleModfiy_Joint)
		{
			data.muscleValue.jointComName = block->joint->getComName();
		}
		if (flag & MuscleModfiy_Target)
		{
			data.muscleValue.targetComName = block->BoneName;
		}
		if (flag & MuscleAttributeModfiy_Group)
		{
			if (muscle) block->group = muscle->attribute->group;
			data.muscleValue.group = block->group;
		}
		if (flag & MuscleAttributeModfiy_Mapping)
		{
			if (muscle) block->mapping = muscle->attribute->mappingWeight;
			data.muscleValue.mapping = block->mapping;
		}
		if (flag & MuscleAttributeModfiy_Pin)
		{
			if (muscle) block->pin = muscle->attribute->pinWeight;
			data.muscleValue.pin = block->pin;
		}
		if (flag & MuscleAttributeModfiy_Weight)
		{
			if (muscle) block->weight = muscle->attribute->muscleWeight;
			data.muscleValue.weight = block->weight;
		}
		if (flag & MuscleAttributeModfiy_Damper)
		{
			if (muscle) block->damper = muscle->attribute->muscleDamper;
			data.muscleValue.damper = block->damper;
		}
	}
	std::vector<RagdollMuscleModify> RagdollComponent::getAllMuscleAttribue()
	{
		std::vector<RagdollMuscleModify> res;
		
		for (auto& bIter : mRagdollBlocks)
		{
			if (initiated && bIter.muscle == nullptr)
				continue;

			res.push_back(RagdollMuscleModify());
			res.back().modifyFlag = MuscleModfiy_ALL;
			res.back().muscleName = bIter.BoneName;

			GetMuscleAttribute(res.back());

		}
		return res;
	}
	void RagdollComponent::SetComData(const RagdollComponentData* data)
	{
		this->blendTime = data->blendTime;
		this->stateSettings = data->stateSettings;
		this->fixTargetTransforms = data->fixTargetTransforms;
		this->solverIterationCount = data->solverIterationCount;
		this->mappingWeight = data->mappingWeight;
		this->pinWeight = data->pinWeight;
		this->muscleWeight = data->muscleWeight;
		this->muscleSpring = data->muscleSpring;
		this->muscleDamper = data->muscleDamper;
		this->pinPow = data->pinPow;
		this->pinDistanceFalloff = data->pinDistanceFalloff;
		this->angularPinning = data->angularPinning;
		this->updateJointAnchors = data->updateJointAnchors;
		this->supportTranslationAnimation = data->supportTranslationAnimation;
		this->angularLimits = data->angularLimits;
		this->internalCollisions = data->internalCollisions;
		
		//int end = (int)std::min(data->mucleData.size(), this->muscles.size());
		//if (end == 0)
		//	return;
		//for (int i = 0; i < end; ++i)
		//{
		//	for (auto& m : this->muscles) // 肌肉数量一般10几个， 暂时不需要优化 
		//	{
		//		if (m->target != nullptr && m->target->getName() == data->mucleData[i].first && m->attribute != nullptr)
		//		{
		//			m->attribute->mappingWeight = data->mucleData[i].second.mappingWeight;
		//			m->attribute->pinWeight = data->mucleData[i].second.pinWeight;
		//			m->attribute->muscleWeight = data->mucleData[i].second.muscleWeight;
		//			m->attribute->muscleDamper = data->mucleData[i].second.muscleDamper;
		//		
		//			break;
		//		}
		//	}
		//}
	}
	void RagdollComponent::BehavioursForeach(const std::function<void(RagdollBehaviorComponentBase*)>& impl)
	{
#if	USE_RAGDOLL_HEHAVIOURS
		RagdollBehaviorManagerComponent* behavioursManager = GetBehavioursManager();

		if (behavioursManager != nullptr)
			behavioursManager->BehavioursForeach(this,impl);
#else
		(void)(impl);
#endif
	}
	RagdollBehaviorManagerComponent* RagdollComponent::GetBehavioursManager()
	{
		ActorComponentPtr parent = getParentNodeCom();

		if (parent.expired())
			return nullptr;
		for (auto& acp : getParentNodeCom()->getChildActorComponents())
		{
			if (acp.second.expired())
				continue;
			if (acp.second->getType() == COMPONENTTYPE(RagdollBehaviorManagerComponent))
			{
				return (RagdollBehaviorManagerComponent*)acp.second.get();
			}
		}
		return nullptr;
	}
	ragdollBlockData* RagdollComponent::FindBoneBlock(const Name& boneName)
	{
		for (auto& item : mRagdollBlocks)
			if (item.BoneName == boneName)
				return &item;
		return nullptr;
	}
	RagdollMuscle* RagdollComponent::GetMuscle(const Name& boneName)
	{
		for (const auto& item : mRagdollBlocks)
			if (item.BoneName == boneName)
				return item.muscle;
		return nullptr;
	}
	//void RagdollComponent::SetMuscleWeights(RagdollMuscle::MuscleGroup group, float muscleWeight, float pinWeight, float mappingWeight, float muscleDamper)
	//{
	//	if (!CheckIfInitiated()) 
	//		return;

	//	for (auto m : muscle) {
	//		if (m->attribute->group == group) {
	//			m->attribute->muscleWeight = muscleWeight;
	//			m->attribute->pinWeight = pinWeight;
	//			m->attribute->mappingWeight = mappingWeight;
	//			m->attribute->muscleDamper = muscleDamper;
	//		}
	//	}
	//}
	//void RagdollComponent::SetMuscleWeights(WorldNode * target, float muscleWeight, float pinWeight, float mappingWeight, float muscleDamper)
	//{
	//	if (!CheckIfInitiated()) 
	//		return;

	//	int index = GetMuscleIndex(target);
	//	if (index == -1) 
	//		return;
	//	SetMuscleWeights(index, muscleWeight, pinWeight, mappingWeight, muscleDamper);
	//}
	//void RagdollComponent::SetMuscleWeights(RagdollBodyBones humanBodyBone, float muscleWeight, float pinWeight, float mappingWeight, float muscleDamper)
	//{
	//	if (!CheckIfInitiated()) 
	//		return;

	//	int index = GetMuscleIndex(humanBodyBone);
	//	if (index == -1) 
	//		return;
	//	SetMuscleWeights(index, muscleWeight, pinWeight, mappingWeight, muscleDamper);
	//}
	//void RagdollComponent::SetMuscleWeightsRecursive(WorldNode * target, float muscleWeight, float pinWeight, float mappingWeight, float muscleDamper)
	//{
	//	if (!CheckIfInitiated()) 
	//		return;

	//	for (int i = 0; i < muscle.size(); i++) 
	//	{
	//		if (muscle[i]->target == target) 
	//		{
	//			SetMuscleWeightsRecursive(i, muscleWeight, pinWeight, mappingWeight, muscleDamper);
	//			return;
	//		}
	//	}
	//}
	//void RagdollComponent::SetMuscleWeightsRecursive(int muscleIndex, float muscleWeight, float pinWeight, float mappingWeight, float muscleDamper)
	//{
	//	if (!CheckIfInitiated()) 
	//		return;

	//	SetMuscleWeights(muscleIndex, muscleWeight, pinWeight, mappingWeight, muscleDamper);

	//	for (int i = 0; i < muscle[muscleIndex]->childIndexes.size(); i++) 
	//	{
	//		int childIndex = muscle[muscleIndex]->childIndexes[i];
	//		SetMuscleWeights(childIndex, muscleWeight, pinWeight, mappingWeight, muscleDamper);
	//	}
	//}
	//void RagdollComponent::SetMuscleWeightsRecursive(RagdollBodyBones humanBodyBone, float muscleWeight, float pinWeight, float mappingWeight, float muscleDamper)
	//{
	//	if (!CheckIfInitiated()) 
	//		return;

	//	int index = GetMuscleIndex(humanBodyBone);
	//	if (index == -1) 
	//		return;
	//	SetMuscleWeightsRecursive(index, muscleWeight, pinWeight, mappingWeight, muscleDamper);
	//}
	//void RagdollComponent::SetMuscleWeights(int muscleIndex, float muscleWeight, float pinWeight, float mappingWeight, float muscleDamper)
	//{
	//	if (!CheckIfInitiated()) 
	//		return;

	//	if (muscleIndex < 0.0f || muscleIndex >= muscle.size()) 
	//		return;

	//	muscle[muscleIndex]->attribute->muscleWeight = muscleWeight;
	//	muscle[muscleIndex]->attribute->pinWeight = pinWeight;
	//	muscle[muscleIndex]->attribute->mappingWeight = mappingWeight;
	//	muscle[muscleIndex]->attribute->muscleDamper = muscleDamper;
	//}
	RagdollMuscle * RagdollComponent::GetMuscle(WorldNode * target)
	{
		int index = GetMuscleIndex(target);
		if (index == -1) 
			return nullptr;
		return muscles[index];
	}
	RagdollMuscle * RagdollComponent::GetMuscle(RigidBodyComponent * rigidbody)
	{
		int index = GetMuscleIndex(rigidbody);
		if (index == -1)
			return nullptr;
		return muscles[index];
	}
	RagdollMuscle * RagdollComponent::GetMuscle(ConfigurableJointComponent * joint)
	{
		int index = GetMuscleIndex(joint);
		if (index == -1)
			return nullptr;
		return muscles[index];
	}
	bool RagdollComponent::ContainsJoint(ConfigurableJointComponent * joint)
	{
		if (!CheckIfInitiated())
			return false;

		for (auto m : muscles)
		{
			if (m->joint == joint)
				return true;
		}
		return false;
	}
	int RagdollComponent::GetMuscleIndex(WorldNode * target)
	{
		if (!CheckIfInitiated()) 
			return -1;

		if (target == nullptr)
			return -1;

		for (int i = 0; i < muscles.size(); i++)
		{
			if (muscles[i]->target == target)
				return i;
		}

		return -1;
	}
	int RagdollComponent::GetMuscleIndex(RigidBodyComponent * rigidbody)
	{
		if (!CheckIfInitiated())
			return -1;

		if (rigidbody == nullptr)
			return -1;

		for (int i = 0; i < muscles.size(); i++)
		{
			if (muscles[i]->rigidbody == rigidbody)
				return i;
		}

		return -1;
	}
	int RagdollComponent::GetMuscleIndex(ConfigurableJointComponent * joint)
	{
		if (!CheckIfInitiated())
			return -1;

		if (joint == nullptr)
			return -1;

		for (int i = 0; i < muscles.size(); i++)
		{
			if (muscles[i]->joint == joint)
				return i;
		}

		return -1;
	}
	void RagdollComponent::SampleTargetMappedState()
	{
		if (!CheckIfInitiated()) 
			return;

		sampleTargetMappedState = true;

		if (!targetMappedStateStored)
		{
			sampleTargetMappedState = true;
			return;
		}

		for (int i = 0; i < muscles.size(); i++)
		{
			if (i == 0) 
				muscles[i]->targetSampledPosition = muscles[i]->targetMappedPosition;

			muscles[i]->targetSampledRotation = muscles[i]->targetMappedRotation;
		}

		targetMappedStateSampled = true;
	}
	void RagdollComponent::FixTargetToSampledState(float weight)
	{
		if (!CheckIfInitiated()) 
			return;

		if (weight <= 0.0f) 
			return;

		if (!targetMappedStateSampled)
			return;

		for (int i = 0; i < muscles.size(); i++)
		{
			if (i == 0) 
				muscles[i]->target->_setDerivedPosition(Vector3::VLerp(DBV2FV(muscles[i]->target->_getDerivedPosition()), muscles[i]->targetSampledPosition, weight));

			muscles[i]->target->_setDerivedOrientation(Quaternion::QLerp(muscles[i]->target->_getDerivedOrientation(), muscles[i]->targetSampledRotation, weight));
		}

		for (auto m : muscles)
		{
			//m->positionOffset = DBV2FV(m->target->_getDerivedPosition()) - m->rigidbody->GetPhysics()->GetPosition();
			m->positionOffset = DBV2FV(m->target->_getDerivedPosition()) - m->rigidbody->getWorldPosition();
		}
	}
	void RagdollComponent::StoreTargetMappedState()
	{
		if (!CheckIfInitiated()) 
			return;

		if (!storeTargetMappedState) 
			return;

		for (int i = 0; i < muscles.size(); i++)
		{
			if (i == 0) 
				muscles[i]->StoreTargetMappedPosition();

			muscles[i]->StoreTargetMappedRotation();
		}

		targetMappedStateStored = true;

		if (sampleTargetMappedState) 
			SampleTargetMappedState();
		sampleTargetMappedState = false;
	}
	void RagdollComponent::UpdateHierarchies()
	{
		//for (int i = 0; i < muscle.size(); i++) {
		//	muscle[i]->index = i;
		//	if (muscle[i]->broadcaster != nullptr) muscle[i]->broadcaster->muscleIndex = i;
		//	if (muscle[i]->jointBreakBroadcaster != nullptr) muscle[i]->jointBreakBroadcaster->muscleIndex = i;
		//}

		targetMappedStateStored = false;
		targetMappedStateSampled = false;

		AssignParentAndChildIndexes();
		AssignKinshipDegrees();
		UpdateBroadcasterMuscleIndexes();

		if (disconnectMuscleFlags.size() != muscles.size())
		{
			disconnectMuscleFlags.resize(muscles.size());
			muscleDisconnectModes.resize(muscles.size());
			disconnectDeactivateFlags.resize(muscles.size());
			reconnectMuscleFlags.resize(muscles.size());
		}

		hasProp = HasProp();

		if (OnHierarchyChanged != nullptr)
			OnHierarchyChanged();
	}
	bool RagdollComponent::HasProp()
	{
		for (auto m : muscles)
		{
			if (m->attribute->group == RagdollMuscle::MuscleGroup::Prop) 
				return true;
		}
		return false;
	}
	void RagdollComponent::UpdateBroadcasterMuscleIndexes()
	{
		/*for (int i = 0; i < muscle.size(); i++) {
			if (muscle[i]->broadcaster != nullptr)
				muscle[i]->broadcaster->muscleIndex = i;
			if (muscle[i]->jointBreakBroadcaster != nullptr)
				muscle[i]->jointBreakBroadcaster->muscleIndex = i;
		}*/
	}
	void RagdollComponent::AssignParentAndChildIndexes()
	{
		for (int i = 0; i < muscles.size(); i++) {
			// Parents
			//muscle[i]->parentIndexes = new int[0];

			if (muscles[i]->rebuildConnectedBody != nullptr)
				AddToParentsRecursive(FindRigdiBodyToJoint(muscles[i]->rebuildConnectedBody.get()), muscles[i]->parentIndexes);

			// Children
			muscles[i]->childFlags.resize(muscles.size());

			for (int n = 0; n < muscles.size(); n++)
			{
				if (i != n && muscles[n]->rebuildConnectedBody == muscles[i]->rigidbody)
				{
					AddToChildrenRecursive(muscles[n]->joint.get(), muscles[i]->childIndexes, muscles[i]->childFlags);
				}
			}
		}
	}
	ConfigurableJointComponent * RagdollComponent::FindRigdiBodyToJoint(RigidBodyComponent * rigidBody)
	{
		if (rigidBody == NULL)
			return nullptr;
		EchoTMap<String, ActorComponentPtr> joint_test = rigidBody->getParentNodeCom()->getChildActorComponents();
		for (auto joint : joint_test)
		{
			if (joint.second->getType() == COMPONENTTYPE(ConfigurableJointComponent))
				return static_cast<ConfigurableJointComponent*>(joint.second.get());
		}
		return nullptr;
	}
	void RagdollComponent::AddToParentsRecursive(ConfigurableJointComponent * joint, std::vector<int>& indexes)
	{
		if (joint == nullptr) 
			return;

		int muscleIndex = GetMuscleIndexLowLevel(joint);
		if (muscleIndex == -1)
			return;


		indexes.push_back(muscleIndex);
		RigidBodyComponent* rigidbody = (RigidBodyComponent*)joint->getActor()->getComponent(joint->GetConnectedRigidBody()).get();
		if (rigidbody == nullptr) 
			return;
		AddToParentsRecursive(FindRigdiBodyToJoint(rigidbody), indexes);
	}
	void RagdollComponent::AddToChildrenRecursive(ConfigurableJointComponent * joint, std::vector<int>& indexes, std::vector<bool>& childFlags)
	{
		if (joint == nullptr) 
			return;

		int muscleIndex = GetMuscleIndexLowLevel(joint);
		if (muscleIndex == -1) 
			return;

		std::string rigidbodyname = joint->GetSelfRigidBody();
		RigidBodyComponent* rigidbody = (RigidBodyComponent*)joint->getActor()->getComponent(rigidbodyname).get();
		if (rigidbody == nullptr)
			return;
		indexes.push_back(muscleIndex);

		childFlags[muscleIndex] = true;

		for (int i = 0; i < muscles.size(); i++)
		{
			if (i != muscleIndex && muscles[i]->rebuildConnectedBody == rigidbody)
			{
				AddToChildrenRecursive(muscles[i]->joint.get(), indexes, childFlags);
			}
		}
	}
	void RagdollComponent::AssignKinshipDegrees()
	{
		for (int i = 0; i < muscles.size(); i++)
		{
			muscles[i]->kinshipDegrees.resize(muscles.size());
			AssignKinshipsDownRecursive(muscles[i]->kinshipDegrees, 1, i);
			AssignKinshipsUpRecursive(muscles[i]->kinshipDegrees, 1, i);
		}
	}
	void RagdollComponent::AssignKinshipsDownRecursive(std::vector<int>& kinshipDegrees, int degree, int index)
	{
		for (int i = 0; i < muscles.size(); i++) {
			if (i != index) {
				if (muscles[i]->rebuildConnectedBody == muscles[index]->rigidbody) {
					kinshipDegrees[i] = degree;

					AssignKinshipsDownRecursive(kinshipDegrees, degree + 1, i);
				}
			}
		}
	}
	void RagdollComponent::AssignKinshipsUpRecursive(std::vector<int>& kinshipDegrees, int degree, int index)
	{
		for (int i = 0; i < muscles.size(); i++) {
			if (i != index) {
				if (muscles[i]->rigidbody == muscles[index]->rebuildConnectedBody) {
					kinshipDegrees[i] = degree;

					AssignKinshipsUpRecursive(kinshipDegrees, degree + 1, i);

					for (int c = 0; c < muscles.size(); c++) {
						if (c != i && c != index) {
							if (muscles[c]->rebuildConnectedBody == muscles[i]->rigidbody) {
								kinshipDegrees[c] = degree + 1;

								AssignKinshipsDownRecursive(kinshipDegrees, degree + 2, c);
							}
						}
					}
				}
			}
		}
	}
	int RagdollComponent::GetMuscleIndexLowLevel(ConfigurableJointComponent * joint)
	{
		for (int i = 0; i < muscles.size(); i++)
		{
			if (muscles[i]->joint == joint)
				return i;
		}
		return -1;
	}
	bool RagdollComponent::IsValid(bool log)
	{
		if (muscles.empty() || muscles.size() == 0)
			return false;

		for (int i = 0; i < muscles.size(); i++)
		{
			if (muscles[i] == nullptr)
				return false;

			if (!muscles[i]->IsValid() || muscles[i]->rigidbody == nullptr)
				return false;
		}

		if (targetRoot == nullptr)
			return false;
				
		for (int i = 0; i < muscles.size(); i++) {
			for (int c = 0; c < muscles.size(); c++) {
				if (i != c)
				{
					if (muscles[i] == muscles[c] || muscles[i]->joint == muscles[c]->joint)
						return false;
				}
			}
		}

		if (muscles[0]->rebuildConnectedBody != nullptr && muscles.size() > 1)
		{
			for (int i = 1; i < muscles.size(); i++)
			{
				if (muscles[i]->rigidbody == muscles[0]->rebuildConnectedBody)
					return false;
				
			}
		}

		//for (int i = 0; i < muscle.size(); i++) {
		//	if ((muscle[i]->joint->getWorldPosition() - muscle[i]->target->_getDerivedPosition()).LengthSq() > 0.001f) 
		//		return false;
		//}

		CheckMassVariation(100.0f, true);

		return true;
	}
	RagdollStateSettings::RagdollStateSettings(float killDuration, float deadMuscleWeight, float deadMuscleDamper, float maxFreezeSqrVelocity, bool freezePermanently, bool enableAngularLimitsOnKill, bool enableInternalCollisionsOnKill)
	{
		this->killDuration = killDuration;
		this->deadMuscleWeight = deadMuscleWeight;
		this->deadMuscleDamper = deadMuscleDamper;
		this->maxFreezeSqrVelocity = maxFreezeSqrVelocity;
		this->freezePermanently = freezePermanently;
		this->enableAngularLimitsOnKill = enableAngularLimitsOnKill;
		this->enableInternalCollisionsOnKill = enableInternalCollisionsOnKill;
	}

	RagdollStateSettings  RagdollStateSettings::Default(1.0f, 0.01f, 2.0f, 0.02f, false, true, true);

	RagdollMuscleModify& RagdollComponentInfo::_InfoModifyMuscle::operator=(RagdollMuscleModify& modfiy)
	{
		if (pInfo == nullptr)
		{
			modfiy.modifyFlag = 0;
			return modfiy;
		}

		for (auto& pa : pInfo->bnmInfo)
		{
			if (pa.first == modfiy.muscleName)
			{
				uint16 flag = modfiy.modifyFlag;
				auto& newValue = modfiy.muscleValue;
				if (flag & MuscleAttributeModfiy_Group)
				{
					pa.second.muclesDesc.group = newValue.group;
				}
				if (flag & MuscleAttributeModfiy_Mapping)
				{
					if (newValue.mapping >= 0.f && newValue.mapping <= 1.f)
					{
						pa.second.muclesDesc.mappingWeight =  newValue.mapping;
					}
				}
				if (flag & MuscleAttributeModfiy_Pin)
				{
					if (newValue.pin >= 0.f && newValue.pin <= 1.f)
					{
						pa.second.muclesDesc.pinWeight = newValue.pin;
					}
				}
				if (flag & MuscleAttributeModfiy_Weight)
				{
					if (newValue.weight >= 0.f && newValue.weight <= 1.f)
					{
						pa.second.muclesDesc.muscleWeight = newValue.weight;
					}
				}
				if (flag & MuscleAttributeModfiy_Damper)
				{
					if (newValue.damper >= 0.f && newValue.damper <= 1.f)
					{
						pa.second.muclesDesc.muscleDamper = newValue.damper;
					}
				}
				return modfiy;
			}
		}
		modfiy.modifyFlag = 0;
		return modfiy;
	}




	/*const ActorPtr& RagdollComponent::CreateAndMoveToNewActor(const Name& newActorName) {
		const ActorPtr& curActorPtr = getActor();
		if (curActorPtr == nullptr || curActorPtr.expired()) {
			return curActorPtr;
		}
		const ActorPtr& curParActorPtr = curActorPtr->getParentActor();
		if (curParActorPtr == nullptr || curParActorPtr.expired()) {
			return curActorPtr;
		}
		const ActorPtr& newActorPtr = curParActorPtr->createChildActor(newActorName);
		if (newActorPtr == nullptr || newActorPtr.expired()) {
			return curActorPtr;
		}
		const ActorComponentPtr& newThis = newActorPtr->addCloneComponent(newActorPtr->getRootNodeCom(), this->weak_from_this(), true);
		
		return newActorPtr;
	}*/
}
