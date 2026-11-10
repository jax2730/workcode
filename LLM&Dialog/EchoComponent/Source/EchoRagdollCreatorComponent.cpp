//-----------------------------------------------------------------------------
// File:  EchoRagdollCreatorComponent.cpp
//
// Author: LangGuangyue
//
// Date:   2022-9-21
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include "EchoComponentStableHeaders.h"
#include "EchoRagdollCreatorComponent.h"
#include "EchoRigidBodyGroupComponent.h"
#include "EchoResourceFolderManager.h"
#include "EchoBoneNode.h"
#include "EchoRagdollTools.h"

#define WritePosJSNode(jsonNode,data)										\
			do{																\
				WriteJSNode(jsonNode, "position", data.mPosition);			\
				WriteJSNode(jsonNode, "eulerAngles", data.mRotation);		\
				WriteJSNode(jsonNode, "scale", data.mScale);				\
			}while(false)
#define ReadPosJSNode(jsonNode,data)										\
			do{																\
				ReadJSNode(jsonNode, "position", data.mPosition);			\
				ReadJSNode(jsonNode, "eulerAngles", data.mRotation);		\
				ReadJSNode(jsonNode, "scale", data.mScale);					\
			}while(false)

namespace Echo
{
	static ActorComponentPtr RagdollCreateComponent(const ActorPtr& act, const Name& typeName, const std::string& ComponentName,
													const ActorComponentPtr& parent, bool runtime = false)
	{
		if (act.expired() || act == nullptr)
		{
			assert(false && "invalid actor");
			return nullptr;
		}

		const ActorComponentPtr& parentComponent = parent.expired() ? act->getRootNodeCom() : parent;
		bool childRuntime = parentComponent->IsRuntimeCom() ? true : runtime;

		ActorComponentPtr && child = act->addComponent(typeName, ComponentName, parentComponent, nullptr, 0, childRuntime);
		if (child.expired() || child == nullptr)
		{
			assert(false && "Create child component fail");
			return nullptr;
		}

		return child;
	}

	static void	RagdollDeleteComponent(const ActorPtr& act, const ActorComponentPtr& comp)
	{
		if (act.expired())
		{
			assert(false && "invalid actor!!!");
			return ;
		}
		if (comp.expired())
		{
			// log
			return;
		}
		auto comInfo = comp->m_pActComInfo;
		if (!act->delComponent(comp) && (comp->IsRuntimeCom() ? true : act->delComponentInfo(comInfo)))
		{
			// log
			return;
		}

		return;
	}

	static const std::unordered_map<BipedIKBoneType, Vector3> gJointRotate
	{
		{Biped_Bone_Pelvis, Vector3::ZERO},
		{Biped_Bone_Spine, Vector3::ZERO},
		{Biped_Bone_Spine2, Vector3::ZERO},
		{Biped_Bone_Head, Vector3::ZERO},
		{Biped_Bone_LThigh, Vector3(0,270,270)},
		{Biped_Bone_LCalf, Vector3(0,0,270)},
		{Biped_Bone_LFoot ,Vector3::ZERO},
		{Biped_Bone_RThigh , Vector3(0,270,270)},
		{Biped_Bone_RCalf , Vector3(0,0,270)},
		{Biped_Bone_RFoot ,Vector3::ZERO},
		{Biped_Bone_LUpperArm ,Vector3::ZERO},
		{Biped_Bone_LForearm,Vector3(0,0,90)},
		{Biped_Bone_LHand ,Vector3::ZERO},
		{Biped_Bone_RUpperArm ,Vector3::ZERO},
		{Biped_Bone_RForearm ,Vector3(0,0,90)},
		{Biped_Bone_RHand ,Vector3::ZERO},
	};

	DECLAREACTORCOMTYPE_CPP(RagdollCreatorComponent)
	RagdollCreatorComponentInfo::RagdollCreatorComponentInfo(const Name & mtype, ActorCOMMoveableType movableType)
		: ActorComponentInfo(mtype, movableType)
	{
	}
	RagdollCreatorComponentInfo::~RagdollCreatorComponentInfo()
	{
	}
	void RagdollCreatorComponentInfo::ExportData(cJSON * jsonNode)
	{
	}
	void RagdollCreatorComponentInfo::ImportData(cJSON * jsonNode)
	{
	}
	void RagdollCreatorComponentInfo::CopyTo(ActorComponentInfo * comInfo)
	{
	}
	void RagdollCreatorComponentInfo::OverWrite()
	{
	}
	RagdollCreatorComponent::RagdollCreatorComponent(ActorPtr pActor, ActorCOMMoveableType movableType)
		: ProtocolNodeActorComponent(pActor, movableType)
	{
		mRigidBodyMass[Biped_Bone_Pelvis]= 3.125f;
		mRigidBodyMass[Biped_Bone_Spine] = 3.125f;
		mRigidBodyMass[Biped_Bone_Spine2] = 3.125f;
		mRigidBodyMass[Biped_Bone_Head] = 1.25f;
		mRigidBodyMass[Biped_Bone_LThigh] = 1.875f;
		mRigidBodyMass[Biped_Bone_LCalf] = 1.875f;
		mRigidBodyMass[Biped_Bone_LFoot] = 1.0f;
		mRigidBodyMass[Biped_Bone_RThigh] = 1.875f;
		mRigidBodyMass[Biped_Bone_RCalf] = 1.875f;
		mRigidBodyMass[Biped_Bone_RFoot] = 1.0f;
		mRigidBodyMass[Biped_Bone_LUpperArm] = 1.25f;
		mRigidBodyMass[Biped_Bone_LForearm] = 1.25f;
		mRigidBodyMass[Biped_Bone_LHand] = 1.25f;
		mRigidBodyMass[Biped_Bone_RUpperArm] = 1.25f;
		mRigidBodyMass[Biped_Bone_RForearm] = 1.25f;
		mRigidBodyMass[Biped_Bone_RHand] = 1.25f;

		BoneTypecast[Name(hips_name)] = Biped_Bone_Pelvis;
		BoneTypecast[Name(spine_name)] = Biped_Bone_Spine;
		BoneTypecast[Name(chest_name)] = Biped_Bone_Spine2;
		BoneTypecast[Name(head_name)] = Biped_Bone_Head;
		BoneTypecast[Name(LupLeg_name)] = Biped_Bone_LThigh;
		BoneTypecast[Name(LlowLeg_name)] = Biped_Bone_LCalf;
		BoneTypecast[Name(Lfoot_name)] = Biped_Bone_LFoot;
		BoneTypecast[Name(RupLeg_name)] = Biped_Bone_RThigh;
		BoneTypecast[Name(RlowLeg_name)] = Biped_Bone_RCalf;
		BoneTypecast[Name(Rfoot_name)] = Biped_Bone_RFoot;
		BoneTypecast[Name(LupArm_name)] = Biped_Bone_LUpperArm;
		BoneTypecast[Name(LlowArm_name)] = Biped_Bone_LForearm;
		BoneTypecast[Name(Lhand_name)] = Biped_Bone_LHand;
		BoneTypecast[Name(RupArm_name)] = Biped_Bone_RUpperArm;
		BoneTypecast[Name(RlowArm_name)] = Biped_Bone_RForearm;
		BoneTypecast[Name(Rhand_name)] = Biped_Bone_RHand;

		mJointLimitPairList[Biped_Bone_Pelvis]		= { 0.0f,0.0f,0.0f,0.0f };
		mJointLimitPairList[Biped_Bone_Spine]		= { -30.0f,30.0f,150.0f,150.0f };
		mJointLimitPairList[Biped_Bone_Spine2]		= { -30.0f,30.0f,150.0f,150.0f };
		mJointLimitPairList[Biped_Bone_Head]		= { -30.0f, 30.0f, 85.f, 30.0f };
		mJointLimitPairList[Biped_Bone_LThigh]		= { -35.0f,120.0f,45.f,85.0f };
		mJointLimitPairList[Biped_Bone_LCalf]		= { -150.0f,20.0f,45.f,10.0f };
		mJointLimitPairList[Biped_Bone_LFoot]		= { -50.0f,50.0f,50.0f,25.0f };
		mJointLimitPairList[Biped_Bone_RThigh]		= { -35.0f,120.0f,45.0f,85.0f };
		mJointLimitPairList[Biped_Bone_RCalf]		= { -150.0f,20.0f,45.0f,10.0f };
		mJointLimitPairList[Biped_Bone_RFoot]		= { -50.0f,50.0f,50.0f,25.0f };
		mJointLimitPairList[Biped_Bone_LUpperArm]	= { -35.0f,120.0f,85.0f,45.0f };
		mJointLimitPairList[Biped_Bone_LForearm]	= { 0.0f,170.0f,10.0f,45.0f };
		mJointLimitPairList[Biped_Bone_LHand]		= { -150,150,150,150 };
		mJointLimitPairList[Biped_Bone_RUpperArm]	= { -35.0f,120.0f,85.0f,45.0f };
		mJointLimitPairList[Biped_Bone_RForearm]	= { 0.0f,170.0f,10.0f,45.0f };
		mJointLimitPairList[Biped_Bone_RHand]		= { -50.0f,50.0f,50.0f,25.0f };
	}
	RagdollCreatorComponent::~RagdollCreatorComponent()
	{
		for (auto iter : mTemplateDataList)
		{
			SAFE_DELETE(iter);
		}
		mTemplateDataList.clear();
		isDel = true;
		CallProtocolFunc(Echo::Name("RoleEditorSetRagdoll"), NULL, (ProtocolHandle)this);

		if (!mRagdollCreatorCallBack.mRoleCom.Expired())
		{
			auto* pIK = RagdollTools::GetRoleComIK(mRagdollCreatorCallBack.mRoleCom.Get());
			if (pIK) pIK->GetRoleObject()->removeRoleLoadListener(&mRagdollCreatorCallBack);
		}
	}
	void RagdollCreatorComponent::init(ActorComponentInfo * info)
	{
		RagdollCreatorComponentInfo* minfo = static_cast<RagdollCreatorComponentInfo*>(info);
		if (minfo == NULL)
			return;
		mInfo = minfo;
	}
	void RagdollCreatorComponent::bindNode()
	{
	}
	void RagdollCreatorComponent::replay(ActorComponentInfo * info)
	{
		RagdollCreatorComponentInfo* minfo = static_cast<RagdollCreatorComponentInfo*>(info);
		if (minfo == NULL)
			return;
		mInfo = minfo;
	}
	void RagdollCreatorComponent::setActor(const ActorPtr& pActor)
	{
		if (pActor == m_pActor)
			return;
		m_pActor = pActor;
		if (m_pActor == nullptr)
			return;
	}
	void RagdollCreatorComponent::copyTo(const ActorComponentPtr& actCom)
	{
	}
	void RagdollCreatorComponent::dealComponentDelete(const ActorComponentPtr& actCom)
	{
		RoleComponent* roleCom = (RoleComponent*)actCom.get();
		if (roleCom == mRoleCom.get())
		{
			mRoleCom = NULL;
			for (auto iter : mTemplateDataList)
			{
				SAFE_DELETE(iter);
			}
			mTemplateDataList.clear();
		}
	}
	//void RagdollCreatorComponent::GetRoleComList()
	//{
	//	std::vector<EchoWeakPtr<ActorComponent>> ComList = m_pActor->getComponents();
	//	mRoleList.clear();
	//	for (auto it : ComList)
	//	{
	//		if (it->getType() == COMPONENTTYPE(RoleComponent))
	//			mRoleList.push_back(static_cast<RoleComponent*>(it.get()));
	//	}
	//	mRoleList.shrink_to_fit();
	//}
	void RagdollCreatorComponent::GetRoleCom(const String & comName)
	{
		const EchoWeakPtr<RoleComponent>& roleCom = m_pActor->getComponent(comName).getStaticCastWeakPtr<RoleComponent>();
		if (roleCom == nullptr || roleCom.expired())
			return;
		mRoleCom = roleCom;

		// 启动IK更新
		auto* pObj = roleCom->GetRole();
		if (pObj != nullptr)
		{
			pObj->getRoleIKComponent()->setKeepUpdate(true);
			// 立刻更新一次
			pObj->getRoleIKComponent()->ManualUpdateTransform();
			pObj->getRoleIKComponent()->setKeepUpdate(false);
		}
		//mSkeletonList = mRoleCom->GetRole()->getRoleIKComponent()->m_mapTransforms;
		InitColliderList();
	}
	void RagdollCreatorComponent::SetRoleComponentPtr(const EchoWeakPtr<RoleComponent>& rolePtr) {
		if (rolePtr == nullptr || rolePtr.expired()) return;
		mRoleCom = rolePtr;

		// 启动IK更新
		auto* pObj = rolePtr->GetRole();
		if (pObj != nullptr)
		{
			pObj->getRoleIKComponent()->setKeepUpdate(true);
			// 立刻更新一次
			pObj->getRoleIKComponent()->ManualUpdateTransform();
			pObj->getRoleIKComponent()->setKeepUpdate(false);
		}
		InitColliderList();
	}

	//void RagdollCreatorComponent::GetSkeletonList(RoleComponent * roleCom)
	//{
	//	if (!roleCom) 
	//		return;
	//	RoleObject* pRoleObj = roleCom->GetRole();
	//	if (!pRoleObj) 
	//		return;
	//	RoleMovable* pSkeObj = pRoleObj->getRoleMovable();
	//	SkeletonInstance* pSkeIns = pSkeObj->getSkeletonInstance();
	//	auto pBone = pSkeIns->getRootBone();
	//	AddSkeleton(pBone);
	//	mSkeletonList.shrink_to_fit();
	//}

	void RagdollCreatorComponent::GetBonePosition()
	{
		if (mRoleCom == nullptr || mRoleCom.expired() || mColliderList.empty())
			return;
		for (auto i : mColliderList)
		{
			if (i.second == NULL)
				mColliderPosition[i.first] = Vector3(0, 0, 0);
			else
				mColliderPosition[i.first] = i.second->_getDerivedPosition();
		}
	}

	void RagdollCreatorComponent::CreateRagdoll()
	{
		if (mRoleCom == nullptr || mRoleCom.expired() || mColliderList.empty())
			return;
		String ragdollName ="Ragdoll_" + mRoleCom->getComName();
		RagdollComponentPtr RagdollCom = RagdollCreateComponent(mRoleCom->getActor(), COMPONENTTYPE(RagdollComponent), ragdollName, mRoleCom->weak_from_this()).getStaticCastWeakPtr<RagdollComponent>();
		mRagdollComponent = RagdollCom;

		for (auto iter : mColliderList)
		{
			if (iter.second == NULL)
				continue;
			InitCreator(RagdollCom.get(), iter.first);
		}
		//Ragdoll
		mRagdollComponent = RagdollCom;
		//RagdollCom->SetRoleIKCom(mRoleCom->getComName());
		//static_cast<RagdollComponentInfo*>(RagdollCom->m_pActComInfo)->roleName = mRoleCom->getComName();
		roleName = mRoleCom->getComName();

		//Collider
		InitTorso();
		InitShape();
		SetCollider();
		is_create = true;
	}
	void RagdollCreatorComponent::DestroyRagdoll()
	{
		if (mRagdollComponent != nullptr && !mRagdollComponent.expired())
		{
			RagdollDeleteComponent(getActor(), mRagdollComponent->weak_from_this());
			mRagdollComponent = nullptr;
		}
	}
	void RagdollCreatorComponent::ChangeRagdollCollider()
	{
		//Node&&RigidBody
		for (auto iter : mColliderList)
		{
			if (iter.first != Biped_Bone_Root)
				mNodeList[iter.first]->setRotation(Quaternion(0, 0, 0, 1));
			//mNodeList[iter.first]->m_pActComInfo->SetEulerAngles(mNodeList[iter.first]->getEulerAngles());

			ActorComponent* collider = mRagdollComponent->FindBoneBlock(iter.second->getName())->collider.get();
			if (collider != NULL)
			{
				RagdollDeleteComponent(getActor(),collider->weak_from_this());
				mRagdollComponent->FindBoneBlock(iter.second->getName())->collider = nullptr;
			}
		}
		InitTorso();
		InitShape();
		SetCollider();
	}
	void RagdollCreatorComponent::ChangeRagdollCollider(float LengthOverlap)
	{
		for (auto iter : mColliderList)
		{
			//if(iter.first != Biped_Bone_Root)
			//	mNodeList[iter.first]->setRotation(Quaternion(0,0,0,1));
			//mNodeList[iter.first]->m_pActComInfo->SetEulerAngles(mNodeList[iter.first]->getEulerAngles());

			//Name name = Name("Collider_") + iter.second->getName();
			ActorComponent* collider = mRagdollComponent->FindBoneBlock(iter.second->getName())->collider.get();
			if (collider != NULL)
			{
				switch (mColliderShape[iter.first])
				{
				case COLLIDER_SHAPE::Box:
				{
					BoxColliderComponent* box = (BoxColliderComponent*)collider;
					Vector3 height;
					height = box->GetHalfSize();
					height *= (1.0f +LengthOverlap);
					box->SetHalfSize(height);

					if (box->m_pActComInfo != nullptr)
						((BoxColliderComponentInfo*)box->m_pActComInfo)->SetHalfSize(height);
				}
				case COLLIDER_SHAPE::Capsule:
				{
					CapsuleColliderComponent* capsule = (CapsuleColliderComponent*)collider;
					float height;
					height = capsule->GetHeight();
					height *= (1.0f + LengthOverlap);
					capsule->SetHeight(height);
					if (capsule->m_pActComInfo != nullptr)
						((CapsuleColliderComponentInfo*)capsule->m_pActComInfo)->SetHeight(height);
				}
				}
			}
		}
	}
	void RagdollCreatorComponent::CreatorEnd()
	{
		is_end = true;
		for (auto iter : mNodeList)
		{
			iter.second->m_pActComInfo->SetPosition(iter.second->getPosition());
			iter.second->m_pActComInfo->SetEulerAngles(iter.second->getEulerAngles());
		}
		//RigidBody
		InitRigidBody();
		//Joint
		InitJoint();
		//Muscle
		//InitMuscle(); 
		//if (mRagdollComponent == nullptr)
		//	return;
		//for (auto& m : mRagdollComponent->muscles)
		//{
		//	SAFE_DELETE(m);
		//}
		//mRagdollComponent->muscles.clear();

		blockData.clear();
		for (const auto& block : mRagdollComponent->mRagdollBlocks)
		{
			blockData.emplace_back();
			bool blockInvalid = false;

			if (!blockInvalid && block.node != nullptr && !block.node.expired())
				blockData.back().nodeName = (Name)block.node->getComName();
			else
				blockInvalid = true;

			if (!blockInvalid && block.collider != nullptr && !block.collider.expired())
				blockData.back().colliderName = (Name)block.collider->getComName();
			else
				blockInvalid = true;

			if (!blockInvalid && block.rigidbody != nullptr && !block.rigidbody.expired())
				blockData.back().rigidbodyName = (Name)block.rigidbody->getComName();
			else
				blockInvalid = true;

			if (!blockInvalid && block.joint != nullptr && !block.joint.expired())
				blockData.back().jointName = (Name)block.joint->getComName();
			else
				blockInvalid = true;

			if (!blockInvalid && !block.BoneName.empty())
				blockData.back().BoneName = block.BoneName;
			else
				blockInvalid = true;

			blockData.back().jointTarget = block.jointTarget;

			if (blockInvalid)
				blockData.pop_back();
		}
		mRagdollComponent->mRagdollBlocks.clear();

		mRagdollComponent->LoadStaticData(roleName, &ragdollData);
		mRagdollComponent->loadDataFromInfo( &blockData);
		mRagdollComponent->CopyMuscleToInfo();
	}
	bool RagdollCreatorComponent::IsInitiated()
	{
		for (auto iter : mColliderList)
		{
			if (iter.first == Biped_Bone_Spine2)
				continue;
			if (iter.second == NULL)
				return false;
		}
		return true;
	}
	bool RagdollCreatorComponent::AddBoneData(std::string name)
	{
		for (auto iter : mTemplateDataList)
		{
			if (iter->name == name)
				return false;
		}
		RagdollBoneData* data = new RagdollBoneData();
		data->name = name;
		mTemplateDataList.push_back(data);
		return true;
	}
	bool RagdollCreatorComponent::BoneHad(std::string boneName)
	{
		int num = 0;
		for (auto data : mTemplateDataList)
		{
			if (data->boneName == boneName)
				num++;
			if (num == 2)
				return true;
		}
		return false;
	}
	void RagdollCreatorComponent::DelTemplatePreview(std::string templateName)
	{
		std::list<RagdollBoneData*>::iterator deliter = mTemplateDataList.end();
		for (auto data : mTemplateDataList)
		{
			if (data->name == templateName)
			{
				//if (getActor()->getComponent(Name(data->boneName)) != nullptr)
				//{
				//	getActor()->delComponent(getActor()->getComponent(Name(data->boneName)));
					isChange = true;
					m_delPreviewName = templateName;
					CallProtocolFunc(Echo::Name("RoleEditorSetRagdoll"), NULL, (ProtocolHandle)this);
					isChange = false;
				//}
				deliter = std::find(mTemplateDataList.begin(), mTemplateDataList.end(), data);
				SAFE_DELETE(data);
				break;
			}
		}
		if (deliter == mTemplateDataList.end())
			return;
		mTemplateDataList.erase(deliter);
	}
	void RagdollCreatorComponent::PreviewRagdoll()
	{
		if (mRoleCom == nullptr || mRoleCom.expired())
			return;
		RagdollComponentPtr ragdollCom = mRagdollComponent;
		ClearRagdoll(ragdollCom.get());

		if (ragdollCom == NULL)
		{
			String ragdollName = "Ragdoll_" + mRoleCom->getComName();
			ragdollCom = RagdollCreateComponent(mRoleCom->getActor(), COMPONENTTYPE(RagdollComponent), ragdollName, mRoleCom->weak_from_this()).getStaticCastWeakPtr<RagdollComponent>();
			roleName = mRoleCom->getComName();

			mRagdollComponent = ragdollCom;
		}
		if (ragdollCom == NULL)
			return;
		//Component Create&Init			(Transform(Node) : Collider/RigidBody/Joint)
		
		//RigidBodyGroup 
		String rigidBodyGroup_name = "RigidBodyGroup_" + mRagdollComponent->getComName();
		//RigidBodyGroupComponent* rigidbodyGroup = (RigidBodyGroupComponent*)RagdollCreateComponent(getActor(), COMPONENTTYPE(RigidBodyGroupComponent), rigidBodyGroup_name, ragdollCom->weak_from_this()).get();
		//ragdollCom->mRigidBodyGroup = rigidbodyGroup->weak_from_this();
		for (auto data : mTemplateDataList)
		{
			if (data == NULL)
				continue;
			if (data->name.empty() || data->boneName.empty() || data->beginBoneName.empty() || data->endBoneName.empty())
				continue;

			WorldNode* bone = mRoleCom->GetRole()->getRoleIKComponent()->getBoneNodeByName(Name(data->boneName));
			if (!bone)
				continue;
			data->mBone = (BoneNode*)bone;
			//Transform
			String node_name = data->boneName;
			NodeComponent* transform = (NodeComponent*)RagdollCreateComponent(getActor(),COMPONENTTYPE(NodeComponent), node_name, ragdollCom->weak_from_this()).get();
			if (!transform)
				continue;
			ragdollCom->mRagdollBlocks.emplace_back();	// new el 	
			ragdollCom->mRagdollBlocks.back().BoneName = node_name;
			ragdollCom->mRagdollBlocks.back().node = transform->weak_from_this();

			transform->setWorldPosition(WorldPos(bone));
			transform->setWorldRotation(WorldQuat(bone));
			if (transform->m_pActComInfo != nullptr)
			{
				transform->m_pActComInfo->SetPosition(transform->getPosition());
				transform->m_pActComInfo->SetEulerAngles(transform->getEulerAngles());
			}

			//Collider
			WorldNode* beginBone = mRoleCom->GetRole()->getRoleIKComponent()->getBoneNodeByName(Name(data->beginBoneName));
			WorldNode* endBone = mRoleCom->GetRole()->getRoleIKComponent()->getBoneNodeByName(Name(data->endBoneName));

			Vector3 direction = WorldPos(endBone) - WorldPos(beginBone);
			float height = direction.Length();
			float width = height * 0.6f;
			//Vector3 heightAxis = GetQuaternionToDirection(WorldQuat(bone), direction);

			switch (data->colliderShape)
			{
			case COLLIDER_SHAPE::Box:
			{
				if (!beginBone || !endBone)
					continue;
				String box_name = "Collider_" + data->boneName;
				BoxColliderComponent* boxCollider = static_cast<BoxColliderComponent*>(RagdollCreateComponent(getActor(), COMPONENTTYPE(BoxColliderComponent), box_name, transform->weak_from_this()).get());
				BoxColliderComponentInfo* boxInfo = static_cast<BoxColliderComponentInfo*>(boxCollider->m_pActComInfo);
				BoxColliderComponentData ColliderData;
				ColliderData.halfSize = Vector3(height, width, width) * 0.5f;
				//data.halfSize = heightAxis * Vector3(height, width, width) * 0.5f;
				//if (data.halfSize.x == 0.0f) data.halfSize.x = height;
				//if (data.halfSize.y == 0.0f) data.halfSize.y = width;
				//if (data.halfSize.z == 0.0f) data.halfSize.z = width;
				ColliderData.halfSize = Vector3(Math::Abs(ColliderData.halfSize.x), Math::Abs(ColliderData.halfSize.y), Math::Abs(ColliderData.halfSize.z));
				ColliderData.filterGroupType = ePhysXGroup_Mesh;
				ColliderData.canInteractFilterGroupType = ePhysXGroup_All;

				boxCollider->SetCompData(&ColliderData);
				boxCollider->setWorldPosition(Vector3::lerp(WorldPos(beginBone), WorldPos(endBone), 0.5f));
				if (boxInfo)
				{
					boxInfo->SetHalfSize(ColliderData.halfSize);
					boxInfo->SetPosition(boxCollider->getPosition());
					boxInfo->SetFilterGroupType(ColliderData.filterGroupType);
					boxInfo->SetCanInteractFilterGroupType(ColliderData.canInteractFilterGroupType);
				}

				ragdollCom->mRagdollBlocks.back().collider = boxCollider->weak_from_this();

				//ragdollCom->mColliderComList.push_back(boxCollider);
				break;
			}
			case COLLIDER_SHAPE::Capsule:
			{
				if (!beginBone || !endBone)
					continue;
				String capsule_name = "Collider_" + data->boneName;
				CapsuleColliderComponent* capsuleCollider = static_cast<CapsuleColliderComponent*>(RagdollCreateComponent(getActor(), COMPONENTTYPE(CapsuleColliderComponent), capsule_name, transform->weak_from_this()).get());
				CapsuleColliderComponentInfo* capsuleInfo = static_cast<CapsuleColliderComponentInfo*>(capsuleCollider->m_pActComInfo);
				CapsuleColliderComponentData Colliderdata;
				Colliderdata.height = Math::Abs(height);
				Colliderdata.radius = Math::Abs(width * 0.6f);
				Colliderdata.filterGroupType = ePhysXGroup_Mesh;
				Colliderdata.canInteractFilterGroupType = ePhysXGroup_All;

				capsuleCollider->SetCompData(&Colliderdata);
				capsuleCollider->setWorldPosition(Vector3::lerp(WorldPos(beginBone), WorldPos(endBone), 0.5f));

				if (capsuleInfo != nullptr)
				{
					capsuleInfo->SetHeight(Colliderdata.height);
					capsuleInfo->SetRadius(Colliderdata.radius);
					capsuleInfo->SetPosition(capsuleCollider->getPosition());
				}

				ragdollCom->mRagdollBlocks.back().collider = capsuleCollider->weak_from_this();

				//ragdollCom->mColliderComList.push_back(capsuleCollider);
				break;
			}
			}

			//RigidBody
			String rigidbody_name = "RigidBody_" + data->boneName;
			RigidBodyComponent* rigidbody = (RigidBodyComponent*)RagdollCreateComponent(getActor(), COMPONENTTYPE(RigidBodyComponent), rigidbody_name, transform->weak_from_this()).get();
			ragdollCom->mRagdollBlocks.back().rigidbody = rigidbody->weak_from_this();
			//ragdollCom->mRigidBodyList.push_back(rigidbody);
			//ragdollCom->mRigidBodyList.shrink_to_fit();

			rigidbody->SetIsKinematic(data->IsKinematic);
			if (rigidbody->m_pActComInfo != nullptr)
				((RigidBodyComponentInfo*)rigidbody->m_pActComInfo)->SetIsKinematic(data->IsKinematic);

			//rigidbodyGroup->addRigidBody(rigidbody_name);
			//((RigidBodyGroupComponentInfo*)rigidbodyGroup->m_pActComInfo)->rigidBodys.push_back(rigidbody_name);

			//Joint
			String joint_name = "Joint_" + data->boneName;
			ConfigurableJointComponent* jointCom = (ConfigurableJointComponent*)RagdollCreateComponent(getActor(), COMPONENTTYPE(ConfigurableJointComponent), joint_name, transform->weak_from_this()).get();
			ragdollCom->mRagdollBlocks.back().joint = jointCom->weak_from_this();
			//ragdollCom->mJointList.push_back(jointCom);
			//ragdollCom->mJointList.shrink_to_fit();

		}
	
		//Joint Init
		for (auto data : mTemplateDataList)
		{
			String jointName = "Joint_" + data->boneName;
			ragdollBlockData* jointBlock = ragdollCom->FindBoneBlock(Name(data->boneName));
			ConfigurableJointComponent* jointCom = jointBlock ? static_cast<ConfigurableJointComponent*>((jointBlock)->joint.get()) : nullptr;
			if (!jointCom)
				continue;

			//rigidbody<->joint<->rigidbody
			ragdollBlockData* rigidBlock = ragdollCom->FindBoneBlock(Name(data->jointConnectedName));
			RigidBodyComponent* rigidbody = rigidBlock ? static_cast<RigidBodyComponent*>(rigidBlock->rigidbody.get()) : nullptr;
			if (rigidbody != NULL)
			{
				String connectRigidBody = rigidbody->getComName();
				jointCom->SetConnectedRigidBody(connectRigidBody);
				if (jointCom->m_pActComInfo != nullptr)
					((JointComponentInfo*)jointCom->m_pActComInfo)->SetConnectedComName(connectRigidBody);

				ragdollCom->FindBoneBlock(Name(data->boneName))->jointTarget = data->jointConnectedName;
			}
			ragdollBlockData* selfRigidBlock = ragdollCom->FindBoneBlock(Name(data->boneName));
			RigidBodyComponent* SelfRigidBody = selfRigidBlock ? static_cast<RigidBodyComponent*>(selfRigidBlock->rigidbody.get()) : nullptr;
			if (SelfRigidBody != NULL)
			{
				String selfRigidBodyName = SelfRigidBody->getComName();
				jointCom->SetSelfRigidBody(selfRigidBodyName);
				if (jointCom->m_pActComInfo != nullptr)
					((JointComponentInfo*)jointCom->m_pActComInfo)->SetSelfComName(selfRigidBodyName);
			}
			ConfigurableJointComponentInfo* jointInfo = static_cast<ConfigurableJointComponentInfo*>(jointCom->m_pActComInfo);
			JointMotion linearMotion = rigidbody != NULL && data->IsSetJointMotion ? JointMotion::LOCKED : JointMotion::FREE;
			JointMotion angularMotion = rigidbody != NULL && data->IsSetJointMotion ? JointMotion::LIMITED : JointMotion::FREE;

			jointCom->SetMotionX(linearMotion);
			jointCom->SetMotionY(linearMotion);
			jointCom->SetMotionZ(linearMotion);

			jointCom->SetMotionAX(angularMotion);
			jointCom->SetMotionAY(angularMotion);
			jointCom->SetMotionAZ(angularMotion);
			if (jointInfo)
			{
				jointInfo->SetMotionX(linearMotion);
				jointInfo->SetMotionY(linearMotion);
				jointInfo->SetMotionZ(linearMotion);

				jointInfo->SetMotionAX(angularMotion);
				jointInfo->SetMotionAY(angularMotion);
				jointInfo->SetMotionAZ(angularMotion);
			}

			if (rigidbody != NULL) {
				WorldNode* bone = mRoleCom->GetRole()->getRoleIKComponent()->getBoneNodeByName(Name(data->boneName));
				Vector3 axis = GetLocalOrthoDirection(bone, mRoleCom->GetRole()->getRoleIKComponent()->m_pRoleNode->GetRight());
				//Vector3 twistAxis = Vector3::FORWARD;
				Vector3 twistAxis = GetLocalOrthoDirection(bone, WorldPos(bone) - rigidbody->getWorldPosition());
				Vector3 secondaryAxis;
				secondaryAxis.Cross(axis, twistAxis);

				//jointCom->SetAxis(axis);
				//jointCom->SetSecondaryAxis(secondaryAxis);

				//jointInfo->SetAxis(axis);
				//jointInfo->SetSecondaryAxis(secondaryAxis);

				Quaternion quat;
				quat.FromAxes(axis, secondaryAxis, twistAxis);
				jointCom->setRotation(quat);
				if (jointInfo)
				{
					jointInfo->SetEulerAngles(quat.ToEuler());
				}
			}
		}
	}
	void RagdollCreatorComponent::PresetMuscles()
	{
		if (mRoleCom == nullptr || mRoleCom.expired())
			return;
		if (mRagdollComponent == NULL || mRagdollComponent.expired() )
			return;
		//SortTemplateDataList(); // 排序 保证父节点在子节点之前
		//for (auto iter : mTemplateDataList)
		//{
		//	ConfigurableJointComponent* joint = (ConfigurableJointComponent*)mRagdollComponent->FindBoneBlock(Name(iter->boneName))->joint.get() ;
		//	if (joint == NULL)
		//		continue;
		//	ColliderComponent* collider = (ColliderComponent*)mRagdollComponent->FindBoneBlock(Name(iter->boneName))->collider.get();
		//	if (collider == NULL)
		//		continue;

		//	//Init Ragdoll(Muscle)
		//	RagdollMuscle* muscle = new RagdollMuscle();
		//	muscle->joint = joint;
		//	muscle->target = static_cast<BoneNode*>(iter->mBone);
		//	muscle->attribute->group = RagdollMuscle::Custom;
		//	
		//	mRagdollComponent->muscles.push_back(muscle);
		//	mRagdollComponent->FindBoneBlock(Name(iter->boneName))->muscle = muscle;
		//}
		//for (auto& m : mRagdollComponent->muscles)
		//{
		//	SAFE_DELETE(m);
		//}
		blockData.clear();
		for (const auto& block : mRagdollComponent->mRagdollBlocks)
		{
			blockData.emplace_back();
			bool blockInvalid = false;

			if (!blockInvalid && block.node != nullptr && !block.node.expired())
				blockData.back().nodeName = (Name)block.node->getComName();
			else
				blockInvalid = true;

			if (!blockInvalid && block.collider != nullptr && !block.collider.expired())
				blockData.back().colliderName = (Name)block.collider->getComName();
			else
				blockInvalid = true;

			if (!blockInvalid && block.rigidbody != nullptr && !block.rigidbody.expired())
				blockData.back().rigidbodyName = (Name)block.rigidbody->getComName();
			else
				blockInvalid = true;

			if (!blockInvalid && block.joint != nullptr && !block.joint.expired())
				blockData.back().jointName = (Name)block.joint->getComName();
			else
				blockInvalid = true;

			if (!blockInvalid && !block.BoneName.empty())
				blockData.back().BoneName = block.BoneName;
			else
				blockInvalid = true;

			blockData.back().jointTarget = block.jointTarget;

			if (blockInvalid)
				blockData.pop_back();
		}
		mRagdollComponent->mRagdollBlocks.clear();

		mRagdollComponent->LoadStaticData(roleName, &ragdollData);
		mRagdollComponent->loadDataFromInfo(&blockData);
		mRagdollComponent->CopyMuscleToInfo();
		mRagdollComponent->muscles.clear();
		mRagdollComponent->loadDataFromInfo(&blockData);
		mRagdollComponent->CopyMuscleToInfo();
	}
	ActorComponentPtr RagdollCreatorComponent::loadFileData(const Name& fileName,bool runtime)
	{
		// 文件名
		std::string fileNameString = fileName.toString();

		// 创建新的ragdollComponent
		RagdollComponentInitData data;
		if (!data.SyncImport(fileNameString))
			return nullptr;

		 return loadFileData(data, runtime);
	}
	ActorComponentPtr RagdollCreatorComponent::loadFileData( RagdollComponentInitData& data, bool runtime)
	{
		// 暂时使用同步的方式

		if (mRoleCom == nullptr)
			return nullptr;
		if (!data.IsImportCompleted())
		{
			//assert(false && "using unimport data init ragdoll！！！");
			return nullptr;
		}
		// 创建新的ragdollComponent
		RagdollComponentPtr ragdoll;
		String ragdollName(("Ragdoll_" + mRoleCom->getComNameToString()).c_str());
		if (mRagdollParentPtr != nullptr && !mRagdollParentPtr.expired())
		{
			ragdoll = RagdollCreateComponent(mRoleCom->getActor(), COMPONENTTYPE(RagdollComponent), ragdollName, mRagdollParentPtr, runtime).getStaticCastWeakPtr<RagdollComponent>();
			mRagdollComponent = ragdoll;
			// TODO
			//static_cast<RagdollComponentInfo*>(ragdoll->m_pActComInfo)->roleName = "";
			((RagdollComponent*)ragdoll.get())->SetRoleComponentPtr(mRoleCom);
		}
		else if (mRoleCom->getActor() == getActor()) {
			ragdoll = RagdollCreateComponent(mRoleCom->getActor(), COMPONENTTYPE(RagdollComponent), ragdollName, mRoleCom->weak_from_this(), runtime).getStaticCastWeakPtr<RagdollComponent>();
			mRagdollComponent = ragdoll;
			// TODO
			//static_cast<RagdollComponentInfo*>(ragdoll->m_pActComInfo)->roleName = mRoleCom->getComName();
		}
		else {
			ragdoll = RagdollCreateComponent(getActor(), COMPONENTTYPE(RagdollComponent), ragdollName, getParentNodeCom(), runtime).getStaticCastWeakPtr<RagdollComponent>();
			mRagdollComponent = ragdoll;
			// TODO
			//static_cast<RagdollComponentInfo*>(ragdoll->m_pActComInfo)->roleName = "";
			((RagdollComponent*)ragdoll.get())->SetRoleComponentPtr(mRoleCom);
		}
		// 导入json数据文件 需要ik加载完成
		RoleIKComponent* pRoleIK = RagdollTools::GetRoleComIK(mRoleCom.get());
		if (pRoleIK == nullptr)
			return ragdoll;
		if (pRoleIK->isCreateFinish())
		{
			RagdollTools::ImportRagdollChildrenNodeAndData(mRoleCom, ragdoll, runtime, data);
		}
		else
		{
			RagdollTools::ImportRagdollChildrenNode(mRoleCom, ragdoll, data);
			mRagdollCreatorCallBack.mRoleCom = mRoleCom;
			mRagdollCreatorCallBack.mFinishCallBack = std::bind(RagdollTools::ImportRagdollData, mRoleCom, ragdoll, runtime, data);
			pRoleIK->GetRoleObject()->addRoleLoadListener(&mRagdollCreatorCallBack);
		}
		return ragdoll;
	}
	void RagdollCreatorComponent::ClearRagdoll(RagdollComponent* ragdollCom)
	{
		if (ragdollCom == nullptr)
			return;

		for (auto& block : ragdollCom->mRagdollBlocks)
		{
			if (block.joint != nullptr)
				RagdollDeleteComponent(getActor(), block.joint);
			if (block.collider != nullptr)
				RagdollDeleteComponent(getActor(), block.collider);
			if (block.rigidbody != nullptr)
				RagdollDeleteComponent(getActor(), block.rigidbody);
			if (block.node != nullptr)
				RagdollDeleteComponent(getActor(), block.node);
		}
		//if (ragdollCom->mRigidBodyGroup != nullptr)
		//	RagdollDeleteComponent(getActor(), ragdollCom->mRigidBodyGroup);

		ragdollCom->mRagdollBlocks.clear();
		//ragdollCom->mRigidBodyGroup = nullptr;
	}

	void _RSrotTemplateDataList(std::list<RagdollBoneData*>::const_iterator last,
								std::list<RagdollBoneData*>::const_iterator checked,
								std::list<RagdollBoneData*> & list )
	{
		if (list.begin() == last)	// 无序的范围已经为0
			return;
		
		auto insetPos = checked;
		insetPos++;

		// find checked`s childern  
		std::vector< std::list<RagdollBoneData*>::const_iterator> children;
		for (auto iter = list.begin(); iter != last; ++iter)
		{
			if ((*iter)->jointConnectedName == (*checked)->boneName)
			{
				children.push_back(iter);
				list.splice(insetPos, list, iter);
			}
		}
		// recurrence childern
		for (auto iter : children)
			_RSrotTemplateDataList(last, iter , list);
		// recurrence remaining
		if (last == list.begin()) // 已经没有剩余的无序范围
			return;
		--last;
		_RSrotTemplateDataList(last,last, list);
	}
	void RagdollCreatorComponent::SortTemplateDataList()
	{
		if (mTemplateDataList.empty())
			return;

		std::list<RagdollBoneData*>::const_iterator last, checkd;
		last = checkd = mTemplateDataList.end();
		--last;
		--checkd;

		_RSrotTemplateDataList(last, checkd, mTemplateDataList);
	}
	void RagdollCreatorComponent::InitColliderList()
	{
		mSkeletonList.clear();
		mSkeletonList.push_back(NULL);
		for (auto iter : mRoleCom->GetRole()->getRoleIKComponent()->m_mapAllBones)
		{
			mSkeletonList.push_back(iter.second);
		}
		mSkeletonList.shrink_to_fit();

		mColliderList[Biped_Bone_Root] = mRoleCom->GetRole()->getRoleIKComponent()->getBoneNodeByName(Name(root_name));
		mColliderList[Biped_Bone_Pelvis] = mRoleCom->GetRole()->getRoleIKComponent()->getBoneNodeByName(Name(hips_name));
		mColliderList[Biped_Bone_Spine] = mRoleCom->GetRole()->getRoleIKComponent()->getBoneNodeByName(Name(spine_name));
		mColliderList[Biped_Bone_Spine2] = mRoleCom->GetRole()->getRoleIKComponent()->getBoneNodeByName(Name(chest_name));
		mColliderList[Biped_Bone_Head] = mRoleCom->GetRole()->getRoleIKComponent()->getBoneNodeByName(Name(head_name));
		mColliderList[Biped_Bone_LThigh] = mRoleCom->GetRole()->getRoleIKComponent()->getBoneNodeByName(Name(LupLeg_name));
		mColliderList[Biped_Bone_LCalf] = mRoleCom->GetRole()->getRoleIKComponent()->getBoneNodeByName(Name(LlowLeg_name));
		mColliderList[Biped_Bone_LFoot] = mRoleCom->GetRole()->getRoleIKComponent()->getBoneNodeByName(Name(Lfoot_name));
		mColliderList[Biped_Bone_RThigh] = mRoleCom->GetRole()->getRoleIKComponent()->getBoneNodeByName(Name(RupLeg_name));
		mColliderList[Biped_Bone_RCalf] = mRoleCom->GetRole()->getRoleIKComponent()->getBoneNodeByName(Name(RlowLeg_name));
		mColliderList[Biped_Bone_RFoot] = mRoleCom->GetRole()->getRoleIKComponent()->getBoneNodeByName(Name(Rfoot_name));
		mColliderList[Biped_Bone_LUpperArm] = mRoleCom->GetRole()->getRoleIKComponent()->getBoneNodeByName(Name(LupArm_name));
		mColliderList[Biped_Bone_LForearm] = mRoleCom->GetRole()->getRoleIKComponent()->getBoneNodeByName(Name(LlowArm_name));
		mColliderList[Biped_Bone_LHand] = mRoleCom->GetRole()->getRoleIKComponent()->getBoneNodeByName(Name(Lhand_name));
		mColliderList[Biped_Bone_RUpperArm] = mRoleCom->GetRole()->getRoleIKComponent()->getBoneNodeByName(Name(RupArm_name));
		mColliderList[Biped_Bone_RForearm] = mRoleCom->GetRole()->getRoleIKComponent()->getBoneNodeByName(Name(RlowArm_name));
		mColliderList[Biped_Bone_RHand] = mRoleCom->GetRole()->getRoleIKComponent()->getBoneNodeByName(Name(Rhand_name));
		
	}
	void RagdollCreatorComponent::InitShape()
	{
		mColliderShape[Biped_Bone_Pelvis] = collider_torso;
		mColliderShape[Biped_Bone_Spine] = collider_torso;
		mColliderShape[Biped_Bone_Spine2] = collider_torso;
		mColliderShape[Biped_Bone_Head] = collider_head;
		mColliderShape[Biped_Bone_LThigh] = collider_leg;
		mColliderShape[Biped_Bone_LCalf] = collider_leg;
		mColliderShape[Biped_Bone_LFoot] = collider_foot;
		mColliderShape[Biped_Bone_RThigh] = collider_leg;
		mColliderShape[Biped_Bone_RCalf] = collider_leg;
		mColliderShape[Biped_Bone_RFoot] = collider_foot;
		mColliderShape[Biped_Bone_LUpperArm] = collider_arm;
		mColliderShape[Biped_Bone_LForearm] = collider_arm;
		mColliderShape[Biped_Bone_LHand] = collider_hand;
		mColliderShape[Biped_Bone_RUpperArm] = collider_arm;
		mColliderShape[Biped_Bone_RForearm] = collider_arm;
		mColliderShape[Biped_Bone_RHand] = collider_hand;

	}
	void RagdollCreatorComponent::InitTorso()
	{
		if (chest && mColliderList[Biped_Bone_Root]->GetUp().Dot(mColliderPosition[Biped_Bone_Spine2] - Vector3::lerp(mColliderPosition[Biped_Bone_LUpperArm], mColliderPosition[Biped_Bone_RUpperArm], 0.5f)) > 0.0f)
		{
			chest = false;
		}
	}
	void RagdollCreatorComponent::InitMuscle()
	{
		RagdollComponent* ragdoll = mRagdollComponent.get() ;
		if (ragdoll == NULL)
			return;
		for (auto& iter : mRagdollComponent->mRagdollBlocks)
		{
			ConfigurableJointComponentPtr joint = iter.joint;
			if (joint == NULL)
				continue;
			ColliderComponent* collider = (ColliderComponent*)iter.collider.get();
			if (collider == NULL)
				continue;

			//Init Ragdoll(Muscle)
			RagdollMuscle* muscle = new RagdollMuscle();
			muscle->joint = joint;
			muscle->target = (BoneNode*)mColliderList[BoneTypecast[iter.BoneName]];

			switch (BoneTypecast[iter.BoneName])
			{
			case Biped_Bone_Root:
				break;
			case Biped_Bone_Pelvis:
				muscle->attribute->group = RagdollMuscle::MuscleGroup::Hips;
				break;
			case Biped_Bone_Spine:
				muscle->attribute->group = RagdollMuscle::MuscleGroup::Spine;
				break;
			case Biped_Bone_LThigh:
				muscle->attribute->group = RagdollMuscle::MuscleGroup::Leg;
				break;
			case Biped_Bone_LCalf:
				muscle->attribute->group = RagdollMuscle::MuscleGroup::Leg;
				break;
			case Biped_Bone_LFoot:
				muscle->attribute->group = RagdollMuscle::MuscleGroup::Foot;
				break;
			case Biped_Bone_RThigh:
				muscle->attribute->group = RagdollMuscle::MuscleGroup::Leg;
				break;
			case Biped_Bone_RCalf:
				muscle->attribute->group = RagdollMuscle::MuscleGroup::Leg;
				break;
			case Biped_Bone_RFoot:
				muscle->attribute->group = RagdollMuscle::MuscleGroup::Foot;
				break;
			case Biped_Bone_Spine1:
				muscle->attribute->group = RagdollMuscle::MuscleGroup::Spine;
				break;
			case Biped_Bone_Spine2:
				muscle->attribute->group = RagdollMuscle::MuscleGroup::Spine;
				break;
			case Biped_Bone_Head:
				muscle->attribute->group = RagdollMuscle::MuscleGroup::Head;
				break;
			case Biped_Bone_LUpperArm:
				muscle->attribute->group = RagdollMuscle::MuscleGroup::Arm;
				break;
			case Biped_Bone_LForearm:
				muscle->attribute->group = RagdollMuscle::MuscleGroup::Arm;
				break;
			case Biped_Bone_LHand:
				muscle->attribute->group = RagdollMuscle::MuscleGroup::Foot;
				break;
			case Biped_Bone_RUpperArm:
				muscle->attribute->group = RagdollMuscle::MuscleGroup::Arm;
				break;
			case Biped_Bone_RForearm:
				muscle->attribute->group = RagdollMuscle::MuscleGroup::Arm;
				break;
			case Biped_Bone_RHand:
				muscle->attribute->group = RagdollMuscle::MuscleGroup::Foot;
				break;
			default:
				break;
			}
			
			mRagdollComponent->muscles.push_back(muscle);
			iter.muscle = muscle;
		}
		
	}
	void RagdollCreatorComponent::InitCreator(RagdollComponent* ragdoll, BipedIKBoneType type)
	{
		if (ragdoll == NULL)
			return;
		//Create Node
		String node_name = mColliderList[type]->getName().toString();
		NodeComponent* ColliderNode = (NodeComponent*)RagdollCreateComponent(getActor(), COMPONENTTYPE(NodeComponent), node_name, ragdoll->weak_from_this()).get();
		ColliderNode->setWorldPosition(mColliderList[type]->_getDerivedPosition());
		ColliderNode->setWorldRotation(mColliderList[type]->_getDerivedOrientation());
		ragdoll->mRagdollBlocks.emplace_back();	// new el 
		ragdoll->mRagdollBlocks.back().BoneName = node_name;
		ragdoll->mRagdollBlocks.back().node = ColliderNode->weak_from_this();
		mNodeList[type] = ColliderNode; 
	}
	void RagdollCreatorComponent::InitJoint()
	{
		if (mRagdollComponent == nullptr || mRagdollComponent.expired())
			return;

		for (auto& iter : mRagdollComponent->mRagdollBlocks)
		{
			ColliderComponent* collider = (ColliderComponent*)iter.collider.get();
			if(collider == NULL)
				continue;
			////Create Joint
			String joint_name = "Joint_" + iter.BoneName.toString();
			ConfigurableJointComponent* jointCom = (ConfigurableJointComponent*)RagdollCreateComponent(getActor(), COMPONENTTYPE(ConfigurableJointComponent), joint_name, iter.node->weak_from_this()).get();
			iter.joint = jointCom->weak_from_this();

			//Name name = Name("Joint_") + iter.second->getComName();
			//ConfigurableJointComponent* jointCom = (ConfigurableJointComponent*)getActor()->getComponent(name);
			//if (jointCom == NULL || iter.first == Biped_Bone_Root || iter.first == Biped_Bone_Pelvis)
			//	continue;
			Name connectRigidBody;
			if (BoneTypecast[iter.BoneName] == Biped_Bone_Spine2)
			{
				connectRigidBody = Name(spine_name);
				if (mRagdollComponent->FindBoneBlock(connectRigidBody)->rigidbody == nullptr)
					connectRigidBody = Name(hips_name);
			}
			else if(BoneTypecast[iter.BoneName] == Biped_Bone_LThigh || BoneTypecast[iter.BoneName] == Biped_Bone_RThigh || BoneTypecast[iter.BoneName] == Biped_Bone_Spine )
				connectRigidBody = Name(hips_name);
			else if (BoneTypecast[iter.BoneName] == Biped_Bone_Head || BoneTypecast[iter.BoneName] == Biped_Bone_LUpperArm || BoneTypecast[iter.BoneName] == Biped_Bone_RUpperArm)
			{
				connectRigidBody = Name(chest_name);
				if (mRagdollComponent->FindBoneBlock(connectRigidBody)->rigidbody == nullptr)
					connectRigidBody = Name(spine_name);
				if (mRagdollComponent->FindBoneBlock(connectRigidBody)->rigidbody == nullptr)
					connectRigidBody = Name(hips_name);
			}
			else if (BoneTypecast[iter.BoneName] == Biped_Bone_LCalf)
				connectRigidBody = Name(LupLeg_name);
			else if (BoneTypecast[iter.BoneName] == Biped_Bone_LFoot)
				connectRigidBody = Name(LlowLeg_name);
			else if (BoneTypecast[iter.BoneName] == Biped_Bone_RCalf)
				connectRigidBody = Name(RupLeg_name);
			else if (BoneTypecast[iter.BoneName] == Biped_Bone_RFoot)
				connectRigidBody = Name(RlowLeg_name);
			else if (BoneTypecast[iter.BoneName] == Biped_Bone_LForearm)
				connectRigidBody = Name(LupArm_name);
			else if (BoneTypecast[iter.BoneName] == Biped_Bone_LHand)
				connectRigidBody = Name(LlowArm_name);
			else if (BoneTypecast[iter.BoneName] == Biped_Bone_RForearm)
				connectRigidBody = Name(RupArm_name);
			else if (BoneTypecast[iter.BoneName] == Biped_Bone_RHand)
				connectRigidBody = Name(RlowArm_name);

			//Name name = Name("RigidBody_") + iter.second->getComName();
			RigidBodyComponent* rigidbody = static_cast<RigidBodyComponent*>(mRagdollComponent->FindBoneBlock(connectRigidBody) == nullptr ? nullptr : mRagdollComponent->FindBoneBlock(connectRigidBody)->rigidbody.get());
			if (rigidbody != nullptr)
			{
				jointCom->SetConnectedRigidBody(rigidbody->getComName());
				((JointComponentInfo*)jointCom->m_pActComInfo)->SetConnectedComName(rigidbody->getComName());
				iter.jointTarget = connectRigidBody;
			}
			RigidBodyComponent* SelfRigidBody =static_cast<RigidBodyComponent*>( iter.rigidbody.get());
			if (SelfRigidBody != NULL)
			{
				jointCom->SetSelfRigidBody(SelfRigidBody->getComName());
				((JointComponentInfo*)jointCom->m_pActComInfo)->SetSelfComName(SelfRigidBody->getComName());
			}
			ConfigurableJointComponentInfo* jointInfo = static_cast<ConfigurableJointComponentInfo*>(jointCom->m_pActComInfo);
			JointMotion linearMotion = rigidbody != NULL ? JointMotion::LOCKED : JointMotion::FREE;
			JointMotion angularMotion = rigidbody != NULL ? JointMotion::LIMITED : JointMotion::FREE;

			jointCom->SetMotionX(linearMotion);
			jointCom->SetMotionY(linearMotion);
			jointCom->SetMotionZ(linearMotion);

			jointCom->SetMotionAX(angularMotion);
			jointCom->SetMotionAY(angularMotion);
			jointCom->SetMotionAZ(angularMotion);

			jointCom->SetEnableCollision(false);
			if (jointInfo)
			{
				jointInfo->SetMotionX(linearMotion);
				jointInfo->SetMotionY(linearMotion);
				jointInfo->SetMotionZ(linearMotion);

				jointInfo->SetMotionAX(angularMotion);
				jointInfo->SetMotionAY(angularMotion);
				jointInfo->SetMotionAZ(angularMotion);
				jointInfo->SetEnableCollision(false);

				jointInfo->SetEnableCollision(false);
			}


			if (rigidbody != NULL) {
				Vector3 axis = GetLocalOrthoDirection(mColliderList[BoneTypecast[iter.BoneName]], mColliderList[Biped_Bone_Root]->GetRight());
				//Vector3 twistAxis = Vector3::FORWARD;
				Vector3 twistAxis = GetLocalOrthoDirection(mColliderList[BoneTypecast[iter.BoneName]], mColliderList[BoneTypecast[iter.BoneName]]->_getDerivedPosition() - rigidbody->getWorldPosition());
				Vector3 secondaryAxis;
				secondaryAxis.Cross(axis, twistAxis);

				//jointCom->SetAxis(axis);
				//jointCom->SetSecondaryAxis(secondaryAxis);

				//jointInfo->SetAxis(axis);
				//jointInfo->SetSecondaryAxis(secondaryAxis);

				Quaternion quat;
				quat.FromAxes(axis, secondaryAxis, twistAxis);
				jointCom->setRotation(quat);
				if (jointInfo)
					jointInfo->SetEulerAngles(quat.ToEuler());

				float Xlow = jointCom->GetAngularXLowLimit();
				float XHight = jointCom->GetAngularXHighLimit();
				float Y = jointCom->GetAngularYLimit();
				float Z = jointCom->GetAngularZLimit();

				Xlow = mJointLimitPairList[BoneTypecast[iter.BoneName]][0] * jointRange;
				XHight = mJointLimitPairList[BoneTypecast[iter.BoneName]][1] * jointRange;
				Y = mJointLimitPairList[BoneTypecast[iter.BoneName]][2] * jointRange;
				Z = mJointLimitPairList[BoneTypecast[iter.BoneName]][3] * jointRange;
				
				jointCom->SetAngularXLowLimit(Xlow);
				jointCom->SetAngularXHighLimit(XHight);
				jointCom->SetAngularYLimit(Y);
				jointCom->SetAngularZLimit(Z);
				jointCom->setEulerAngles(gJointRotate.at(BoneTypecast[iter.BoneName]));
				if (jointInfo)
				{
					jointInfo->SetEulerAngles(gJointRotate.at(BoneTypecast[iter.BoneName]));
					jointInfo->SetAngularXLowLimit(Xlow);
					jointInfo->SetAngularXHighLimit(XHight);
					jointInfo->SetAngularYLimit(Y);
					jointInfo->SetAngularZLimit(Z);
				}
			}
// 			jointCom->SetAnchor(Vector3::ZERO);
// 			jointInfo->SetAnchor(Vector3::ZERO);
		}
	}
	void RagdollCreatorComponent::InitRigidBody()
	{
		if (mRagdollComponent == nullptr || mRagdollComponent.expired())
			return;

		std::vector<String> rigidbodyNames;

		for (auto& iter : mRagdollComponent->mRagdollBlocks)
		{
			////Create Rigidbody
			ColliderComponent* collider = static_cast<ColliderComponent*>(iter.collider.get());

			if (collider == NULL)
				continue;

			String rigidbody_name = "RigidBody_" + iter.BoneName.toString();
			RigidBodyComponent* rigidbody = (RigidBodyComponent*)RagdollCreateComponent(getActor(), COMPONENTTYPE(RigidBodyComponent), rigidbody_name, iter.node->weak_from_this()).get();
			rigidbody->SetIsKinematic(true);
			rigidbody->SetMass(mRigidBodyMass[BoneTypecast[iter.BoneName]]);
			rigidbody->SetCollisionDetection(CollisionDetectionMode::ContinuousSpeculative);
			
			if (rigidbody->m_pActComInfo)
			{
				((RigidBodyComponentInfo*)rigidbody->m_pActComInfo)->SetIsKinematic(true);
				((RigidBodyComponentInfo*)rigidbody->m_pActComInfo)->SetMass(mRigidBodyMass[BoneTypecast[iter.BoneName]]);
				((RigidBodyComponentInfo*)rigidbody->m_pActComInfo)->SetCollisionDetection(CollisionDetectionMode::ContinuousSpeculative);
			}
			rigidbodyNames.push_back(rigidbody_name);
			iter.rigidbody = rigidbody->weak_from_this();
		}

		////Create RigidBodyGroup
		//Name rigidBodyGroup_name = Name("RigidBodyGroup_") + mRagdollComponent->getComName();
		//RigidBodyGroupComponent* rbGroup = (RigidBodyGroupComponent*)RagdollCreateComponent(getActor(), COMPONENTTYPE(RigidBodyGroupComponent), rigidBodyGroup_name, mRagdollComponent->weak_from_this()).get();
		//mRagdollComponent->mRigidBodyGroup = rbGroup->weak_from_this();
		//rbGroup->resetRigidBodys(rigidbodyNames);
		//((RigidBodyGroupComponentInfo*)rbGroup->m_pActComInfo)->rigidBodys = rigidbodyNames;

	}
	void RagdollCreatorComponent::SetCollider()
	{
		//Torso
		Vector3 upperArmToHeadCentroid = Vector3::lerp(Vector3::lerp(mColliderPosition[Biped_Bone_LUpperArm], mColliderPosition[Biped_Bone_RUpperArm], 0.5f), mColliderPosition[Biped_Bone_Head], 0.5);
		Vector3 shoulderDirection = mColliderPosition[Biped_Bone_RUpperArm] - mColliderPosition[Biped_Bone_LUpperArm];
		float torsoWidth = shoulderDirection.Length() * 0.8f;
		float ProportionAspect = 0.4f;

		//Hips
		Vector3 hipsStartPoint = mColliderPosition[Biped_Bone_Pelvis];
		float toHead = Vector3::Distance(mColliderPosition[Biped_Bone_Head], mColliderPosition[Biped_Bone_Root]);
		float toHips = Vector3::Distance(mColliderPosition[Biped_Bone_Pelvis], mColliderPosition[Biped_Bone_Root]);

		if (toHips < toHead * 0.2f)
			hipsStartPoint = Vector3::lerp(mColliderPosition[Biped_Bone_LThigh], mColliderPosition[Biped_Bone_RThigh], 0.5f);

		Vector3 lastEndPoint = spine ? mColliderPosition[Biped_Bone_Spine] : (chest ? mColliderPosition[Biped_Bone_Spine2] : upperArmToHeadCentroid);
		hipsStartPoint += (hipsStartPoint - upperArmToHeadCentroid) * 0.1f;
		float hipsWidth = spine || chest ? torsoWidth * 0.8f : torsoWidth;

		CreateCollider(Biped_Bone_Pelvis, hipsStartPoint, lastEndPoint, mColliderShape[Biped_Bone_Pelvis], colliderLengthOverlap, hipsWidth, ProportionAspect, shoulderDirection);

		//Spine
		if (spine) 
		{
			Vector3 spineStartPoint = lastEndPoint;
			lastEndPoint = chest ? mColliderPosition[Biped_Bone_Spine2] : upperArmToHeadCentroid;

			float spineWidth = chest ? torsoWidth * 0.8f : torsoWidth;

			CreateCollider(Biped_Bone_Spine, spineStartPoint, lastEndPoint, mColliderShape[Biped_Bone_Spine], colliderLengthOverlap, spineWidth, ProportionAspect, shoulderDirection);
		}

		//Chest
		if (chest) 
		{
			Vector3 chestStartPoint = lastEndPoint;
			lastEndPoint = upperArmToHeadCentroid;

			CreateCollider(Biped_Bone_Spine2, chestStartPoint, lastEndPoint, mColliderShape[Biped_Bone_Spine2], colliderLengthOverlap, torsoWidth, ProportionAspect, shoulderDirection);
		}

		//Head
		Vector3 headStartPoint = lastEndPoint;
		Vector3 headEndPoint = headStartPoint + (headStartPoint - hipsStartPoint) * 0.45f;
		Vector3 axis = GetQuaternionToDirection(mNodeList[Biped_Bone_Head]->getWorldRotation(), headEndPoint - headStartPoint);
		Vector3 head_test = Vector3::Project((headEndPoint - headStartPoint), axis);
		head_test.normalise();
		headEndPoint = headStartPoint + head_test * (headEndPoint - headStartPoint).Length();

		CreateCollider(Biped_Bone_Head, headStartPoint, headEndPoint, mColliderShape[Biped_Bone_Head], colliderLengthOverlap, Vector3::Distance(headStartPoint, headEndPoint) * 0.8f);

		//Arms
		float armWidthAspect = 0.4f;

		float leftArmWidth = Vector3::Distance(mColliderPosition[Biped_Bone_LUpperArm], mColliderPosition[Biped_Bone_LForearm]) * armWidthAspect;
		CreateCollider(Biped_Bone_LUpperArm, mColliderPosition[Biped_Bone_LUpperArm], mColliderPosition[Biped_Bone_LForearm], mColliderShape[Biped_Bone_LUpperArm], colliderLengthOverlap, leftArmWidth);
		CreateCollider(Biped_Bone_LForearm, mColliderPosition[Biped_Bone_LForearm], mColliderPosition[Biped_Bone_LHand], mColliderShape[Biped_Bone_LForearm], colliderLengthOverlap, leftArmWidth * 0.9f);

		//float rightArmWidth = Vector3::Distance(mColliderPosition[Biped_Bone_RUpperArm], mColliderPosition[Biped_Bone_RForearm]) * armWidthAspect;
		CreateCollider(Biped_Bone_RUpperArm, mColliderPosition[Biped_Bone_RUpperArm], mColliderPosition[Biped_Bone_RForearm], mColliderShape[Biped_Bone_RUpperArm], colliderLengthOverlap, leftArmWidth);
		CreateCollider(Biped_Bone_RForearm, mColliderPosition[Biped_Bone_RForearm], mColliderPosition[Biped_Bone_RHand], mColliderShape[Biped_Bone_RForearm], colliderLengthOverlap, leftArmWidth * 0.9f);

		//Legs
		float legWidthAspect = 0.3f;

		float leftLegWidth = Vector3::Distance(mColliderPosition[Biped_Bone_LThigh], mColliderPosition[Biped_Bone_LCalf]) * legWidthAspect;
		CreateCollider(Biped_Bone_LThigh, mColliderPosition[Biped_Bone_LThigh], mColliderPosition[Biped_Bone_LCalf], mColliderShape[Biped_Bone_LThigh], colliderLengthOverlap, leftLegWidth);
		CreateCollider(Biped_Bone_LCalf, mColliderPosition[Biped_Bone_LCalf], mColliderPosition[Biped_Bone_LFoot], mColliderShape[Biped_Bone_LCalf], colliderLengthOverlap, leftLegWidth * 0.9f);

		//float rightLegWidth = Vector3::Distance(mColliderPosition[Biped_Bone_RThigh], mColliderPosition[Biped_Bone_RCalf]) * legWidthAspect;
		CreateCollider(Biped_Bone_RThigh, mColliderPosition[Biped_Bone_RThigh], mColliderPosition[Biped_Bone_RCalf], mColliderShape[Biped_Bone_RThigh], colliderLengthOverlap, leftArmWidth);
		CreateCollider(Biped_Bone_RCalf, mColliderPosition[Biped_Bone_RCalf], mColliderPosition[Biped_Bone_RFoot], mColliderShape[Biped_Bone_RCalf], colliderLengthOverlap, leftArmWidth * 0.9f);

		// Hands
		if (hands) 
		{
			CreateHandCollider(Biped_Bone_LHand, Biped_Bone_LForearm, mColliderShape[Biped_Bone_LHand]);
			CreateHandCollider(Biped_Bone_RHand, Biped_Bone_RForearm, mColliderShape[Biped_Bone_RHand]);
		}

		// Feet
		if (feet) 
		{
			CreateFootCollider(Biped_Bone_LFoot, Biped_Bone_LCalf, mColliderShape[Biped_Bone_LFoot]);
			CreateFootCollider(Biped_Bone_RFoot, Biped_Bone_RCalf, mColliderShape[Biped_Bone_RFoot]);
		}
	}
	void RagdollCreatorComponent::CreateCollider(BipedIKBoneType collider, Vector3 startPoint, Vector3 endPoint, COLLIDER_SHAPE colliderType, float LengthOverlap, float width)
	{
		if (mRagdollComponent == nullptr || mRagdollComponent.expired())
			return;

		Vector3 direction = endPoint - startPoint;
		float height = direction.Length() * (1.0f + LengthOverlap);
		Vector3 heightAxis = GetQuaternionToDirection(mColliderList[collider]->_getDerivedOrientation(), direction);
		switch (colliderType)
		{
		case COLLIDER_SHAPE::Box:
		{
			if (mNodeList[collider] == NULL || mColliderList[collider] == NULL)
				return;
			if (mRagdollComponent->FindBoneBlock(mColliderList[collider]->getName())->collider != nullptr)
				return;
			String box_name = "Collider_" + mColliderList[collider]->getName().toString();
			BoxColliderComponent* boxCollider = static_cast<BoxColliderComponent*>(RagdollCreateComponent(getActor(), COMPONENTTYPE(BoxColliderComponent), box_name, mNodeList[collider]->weak_from_this()).get());
			BoxColliderComponentInfo* boxInfo = static_cast<BoxColliderComponentInfo*>(boxCollider->m_pActComInfo);

			BoxColliderComponentData data;
			data.halfSize = heightAxis * Vector3(height, width, width) * 0.5f;
			if (data.halfSize.x == 0.0f) data.halfSize.x = height;
			if (data.halfSize.y == 0.0f) data.halfSize.y = width;
			if (data.halfSize.z == 0.0f) data.halfSize.z = width;
			data.halfSize = Vector3(Math::Abs(data.halfSize.x), Math::Abs(data.halfSize.y), Math::Abs(data.halfSize.z));
			data.filterGroupType = ePhysXGroup_Mesh;
			data.canInteractFilterGroupType = ePhysXGroup_All;

			boxCollider->SetCompData(&data);
			boxCollider->setPosition(mColliderList[collider]->convertWorldToLocalPosition(Vector3::lerp(startPoint, endPoint, 0.5f)));
			if (boxInfo != nullptr)
			{
				boxInfo->SetHalfSize(data.halfSize);
				boxInfo->SetPosition(mColliderList[collider]->convertWorldToLocalPosition(Vector3::lerp(startPoint, endPoint, 0.5f)));
				boxInfo->SetFilterGroupType(data.filterGroupType);
				boxInfo->SetCanInteractFilterGroupType(data.canInteractFilterGroupType);
			}

			//boxCollider->setWorldPosition((Vector3::lerp(startPoint, endPoint, 0.5f)));
			//boxInfo->SetPosition(mColliderList[collider]->convertWorldToLocalPosition(Vector3::lerp(startPoint, endPoint, 0.5f)));
			//mNodeList[collider]->setPosition(Vector3::lerp(startPoint, endPoint, 0.5f));
			mRagdollComponent->FindBoneBlock(mColliderList[collider]->getName())->collider = boxCollider->weak_from_this();
			mNodeList[collider]->setWorldRotation(mColliderList[collider]->getOrientation());
			break;
		}
		case COLLIDER_SHAPE::Capsule:
		{
			if (mNodeList[collider] == NULL || mColliderList[collider] == NULL)
				return;
			if (mRagdollComponent->FindBoneBlock(mColliderList[collider]->getName())->collider != nullptr)
				return;
			String capsule_name = "Collider_" + mColliderList[collider]->getName().toString();
			CapsuleColliderComponent* capsuleCollider = static_cast<CapsuleColliderComponent*>(RagdollCreateComponent(getActor(), COMPONENTTYPE(CapsuleColliderComponent), capsule_name, mNodeList[collider]->weak_from_this()).get());
			CapsuleColliderComponentInfo* capsuleInfo = static_cast<CapsuleColliderComponentInfo*>(capsuleCollider->m_pActComInfo);

			CapsuleColliderComponentData data;
			data.height = Math::Abs(height);
			data.radius = Math::Abs(width * 0.6f);
			data.filterGroupType = ePhysXGroup_Mesh;
			data.canInteractFilterGroupType = ePhysXGroup_All;

			capsuleCollider->SetCompData(&data);
			capsuleCollider->setPosition(mColliderList[collider]->convertWorldToLocalPosition(Vector3::lerp(startPoint, endPoint, 0.5f)));
			if (capsuleInfo != nullptr)
			{
				capsuleInfo->SetHeight(data.height);
				capsuleInfo->SetRadius(data.radius);
				capsuleInfo->SetPosition(mColliderList[collider]->convertWorldToLocalPosition(Vector3::lerp(startPoint, endPoint, 0.5f)));
				capsuleInfo->SetFilterGroupType(data.filterGroupType);
				capsuleInfo->SetCanInteractFilterGroupType(data.canInteractFilterGroupType);
			}
			//capsuleCollider->setWorldPosition(Vector3::lerp(startPoint, endPoint, 0.5f));
			//mNodeList[collider]->setPosition(Vector3::lerp(startPoint, endPoint, 0.5f));
			mRagdollComponent->FindBoneBlock(mColliderList[collider]->getName())->collider = capsuleCollider->weak_from_this();
			mNodeList[collider]->setWorldRotation(mColliderList[collider]->_getDerivedOrientation());
			break;
		}
		}
	}
	void RagdollCreatorComponent::CreateCollider(BipedIKBoneType collider, Vector3 startPoint, Vector3 endPoint, COLLIDER_SHAPE colliderType, float LengthOverlap, float width, float proportionAspect, Vector3 widthDirection)
	{
		if (mRagdollComponent == nullptr)
			return ;

		if (colliderType == COLLIDER_SHAPE::Capsule) {
			CreateCollider(collider, startPoint, endPoint, colliderType, LengthOverlap, width * proportionAspect);
			return;
		}
		if (mNodeList[collider] == NULL || mColliderList[collider] == NULL)
			return;
		String collider_name = "Collider_" + mColliderList[collider]->getName().toString();
		if (mRagdollComponent->FindBoneBlock(mColliderList[collider]->getName())->collider != nullptr)
			return;
		BoxColliderComponent* boxCollider = static_cast<BoxColliderComponent*>(RagdollCreateComponent(getActor(), COMPONENTTYPE(BoxColliderComponent), collider_name, mNodeList[collider]->weak_from_this()).get());
		BoxColliderComponentInfo* boxInfo = static_cast<BoxColliderComponentInfo*>(boxCollider->m_pActComInfo);
		BoxColliderComponentData data;
		
		Vector3 direction = endPoint - startPoint;
		float height = direction.Length() * (1.0f + LengthOverlap);

		Vector3 heightAxis = GetQuaternionToDirection(mColliderList[collider]->_getDerivedOrientation(), direction);
		Vector3 widthAxis = GetQuaternionToDirection(mColliderList[collider]->_getDerivedOrientation(), widthDirection);

		if (widthAxis == heightAxis)
			widthAxis = Vector3(heightAxis.y, heightAxis.z, heightAxis.x);

		Vector3 heightAdd = heightAxis * Vector3(height, height, height) * 0.5f;
		Vector3 widthAdd = widthAxis * Vector3(width, width, width);

		data.halfSize = heightAdd + widthAdd;
		if (data.halfSize.x == 0.0f) data.halfSize.x = width * proportionAspect;
		if (data.halfSize.y == 0.0f) data.halfSize.y = width * proportionAspect;
		if (data.halfSize.z == 0.0f) data.halfSize.z = width * proportionAspect;
		data.halfSize = Vector3(Math::Abs(data.halfSize.x), Math::Abs(data.halfSize.y), Math::Abs(data.halfSize.z));
		data.filterGroupType = ePhysXGroup_Mesh;
		data.canInteractFilterGroupType = ePhysXGroup_All;

		boxCollider->SetCompData(&data);
		boxCollider->setPosition(mColliderList[collider]->convertWorldToLocalPosition(Vector3::lerp(startPoint, endPoint, 0.5f)));
		if (boxInfo)
		{
			boxInfo->SetHalfSize(data.halfSize);
			boxInfo->SetPosition(mColliderList[collider]->convertWorldToLocalPosition(Vector3::lerp(startPoint, endPoint, 0.5f)));
			boxInfo->SetFilterGroupType(data.filterGroupType);
			boxInfo->SetCanInteractFilterGroupType(data.canInteractFilterGroupType);
		}
		//boxCollider->setWorldPosition(Vector3::lerp(startPoint, endPoint, 0.5f));
		//mNodeList[collider]->setPosition(Vector3::lerp(startPoint, endPoint, 0.5f));
		mRagdollComponent->FindBoneBlock(mColliderList[collider]->getName())->collider = boxCollider->weak_from_this();
		mNodeList[collider]->setWorldRotation(mColliderList[collider]->_getDerivedOrientation());
	}
	void RagdollCreatorComponent::CreateHandCollider(BipedIKBoneType collider, BipedIKBoneType arm, COLLIDER_SHAPE colliderType)
	{
		Vector3 direction = mColliderPosition[arm] - mColliderPosition[collider];
		int hand_child = mColliderList[collider]->numChildren();
		if (hand_child > 0)
		{
			Vector3 childPos = Vector3::ZERO;
			for (int i = 0;i < hand_child;i++)
			{
				childPos += (mColliderList[collider]->getChild(i)->getPosition() - mColliderPosition[collider]);
			}
			childPos /= (float)hand_child;

			direction = childPos;
		}
		Vector3 axis = GetQuaternionToDirection(mColliderList[collider]->_getDerivedOrientation(), direction);
		Vector3 endPoint = mColliderPosition[collider] - (mColliderPosition[arm] - mColliderPosition[collider]) * 0.75f;

		Vector3 hand_test = Vector3::Project((endPoint - mColliderPosition[collider]), axis);
		hand_test.normalise();

		endPoint = mColliderPosition[collider] + hand_test * (endPoint - mColliderPosition[collider]).Length();

		CreateCollider(collider, mColliderPosition[collider], endPoint, colliderType, colliderLengthOverlap, Vector3::Distance(endPoint, mColliderPosition[collider]) * 0.5f);

	}
	void RagdollCreatorComponent::CreateFootCollider(BipedIKBoneType collider, BipedIKBoneType leg, COLLIDER_SHAPE colliderType)
	{
		Vector3 root_forward = mColliderList[Biped_Bone_Root]->GetForward();
		Vector3 root_up = mColliderList[Biped_Bone_Root]->GetUp();

		float legHeight = Vector3::Distance(mColliderPosition[leg], mColliderPosition[collider]);
		Vector3 foot_direction = mColliderPosition[collider] + root_forward;
		int hand_child = mColliderList[collider]->numChildren();
		if (hand_child > 0)
		{
			Vector3 childPos = Vector3::ZERO;
			for (int i = 0;i < hand_child;i++)
			{
				childPos += (mColliderList[collider]->getChild(i)->getPosition() - mColliderPosition[collider]);
			}
			childPos /= (float)hand_child;

			foot_direction = childPos;
		}

		Vector3 axis = GetQuaternionToDirection(mColliderList[collider]->_getDerivedOrientation(), foot_direction + root_forward * legHeight * 0.2f);

		Vector3 endPoint = mColliderPosition[collider] + root_forward * legHeight * 0.25f;
		Vector3 foot_test = Vector3::Project(endPoint - mColliderPosition[collider], axis);
		foot_test.normalise();
		endPoint = mColliderPosition[collider] + foot_test * (endPoint - mColliderPosition[collider]).Length();

		float width = Vector3::Distance(endPoint, mColliderPosition[collider]) * 0.5f;
		Vector3 startPoint = mColliderPosition[collider];

		bool footBelowRoot = root_up.Dot(startPoint - mColliderPosition[Biped_Bone_Root]) < 0.0f;
		Vector3 heightOffset = footBelowRoot ? Vector3::ZERO : Vector3::Project((startPoint - root_up * width * 0.5f) - mColliderPosition[Biped_Bone_Root], root_up);

		Vector3 direction = endPoint - startPoint;
		startPoint -= direction * 0.2f;

		CreateCollider(collider, startPoint - heightOffset, endPoint - heightOffset, colliderType, colliderLengthOverlap, width);

	}
	Vector3 RagdollCreatorComponent::GetQuaternionToDirection(Quaternion r, Vector3 direction)
	{
		direction.normalise();
		
		Vector3 axis = Vector3::RIGHT;

		Vector3 right = r * Vector3::RIGHT;
		Vector3 up = r *  Vector3::UP;
		Vector3 forward = r * Vector3::FORWARD;

		right.Normalize();
		up.Normalize();
		forward.Normalize();
		
		float dotX = abs(direction.dotProduct(right));
		float dotY = abs(direction.dotProduct(up));
		if (dotY > dotX) 
			axis = Vector3::UNIT_Y;
		float dotZ = abs(direction.dotProduct(forward));
		if (dotZ > dotX && dotZ > dotY) 
			axis = Vector3::UNIT_Z;

		return axis;
	}
	Vector3 RagdollCreatorComponent::GetLocalOrthoDirection(WorldNode * transform, Vector3 worldDir)
	{
		worldDir.normalise();

		float dotX = worldDir.Dot(transform->GetRight());
		float dotY = worldDir.Dot(transform->GetUp());
		float dotZ = worldDir.Dot(transform->GetForward());

		float absDotX = Math::Abs(dotX);
		float absDotY = Math::Abs(dotY);
		float absDotZ = Math::Abs(dotZ);

		Vector3 orthoDirection = Vector3::RIGHT;
		if (absDotY > absDotX && absDotY > absDotZ) orthoDirection = Vector3::UNIT_Y;
		if (absDotZ > absDotX && absDotZ > absDotY) orthoDirection = Vector3::UNIT_Z;

		if (worldDir.Dot(transform->getOrientation() * orthoDirection) < 0.0f) orthoDirection = -orthoDirection;

		return orthoDirection;
	}
	
	void DataToJson(const RagdollComponentInitData& info, cJSON* jsonNode)
	{
		if (jsonNode == nullptr)
			return;

		// ragdollData
		cJSON* jsonRagdoll = cJSON_CreateObject();
		cJSON_AddItemToObject(jsonNode, "RagdollData", jsonRagdoll);
		{
			RagdollStateSettings		setting = info.mSimpleData.stateSettings;
			float						blendTime = info.mSimpleData.blendTime;
			bool						fixTargetTransforms = info.mSimpleData.fixTargetTransforms;
			int							solverIterationCount = info.mSimpleData.solverIterationCount;
			float						mappingWeight = info.mSimpleData.mappingWeight;
			float						pinWeight = info.mSimpleData.pinWeight;
			float						muscleWeight = info.mSimpleData.muscleWeight;
			float						muscleSpring = info.mSimpleData.muscleSpring;
			float						muscleDamper = info.mSimpleData.muscleDamper;
			float						pinPow = info.mSimpleData.pinPow;
			float						pinDistanceFalloff = info.mSimpleData.pinDistanceFalloff;
			float						maxAngularVelocity = info.mSimpleData.maxAngularVelocity;
			float						maxLinearVelocity = info.mSimpleData.maxLinearVelocity;
			float						ragdollDensity = info.mSimpleData.ragdollDensity;

			bool						angularPinning = info.mSimpleData.angularPinning;
			bool						updateJointAnchors = info.mSimpleData.updateJointAnchors;
			bool						supportTranslationAnimation = info.mSimpleData.supportTranslationAnimation;
			bool						angularLimits = info.mSimpleData.angularLimits;
			bool					 	internalCollisions = info.mSimpleData.internalCollisions;

			//WriteJSNode(jsonRagdoll, "roleName", ragdollInfo->roleName);					// 使用新的角色
			WriteJSNode(jsonRagdoll, "stateSettings", setting);
			WriteJSNode(jsonRagdoll, "blendTime", blendTime);
			WriteJSNode(jsonRagdoll, "fixTargetTransforms", fixTargetTransforms);
			WriteJSNode(jsonRagdoll, "solverIterationCount", solverIterationCount);
			WriteJSNode(jsonRagdoll, "mappingWeight", mappingWeight);
			WriteJSNode(jsonRagdoll, "pinWeight", pinWeight);
			WriteJSNode(jsonRagdoll, "muscleWeight", muscleWeight);
			WriteJSNode(jsonRagdoll, "muscleSpring", muscleSpring);
			WriteJSNode(jsonRagdoll, "muscleDamper", muscleDamper);
			WriteJSNode(jsonRagdoll, "pinPow", pinPow);
			WriteJSNode(jsonRagdoll, "pinDistanceFalloff", pinDistanceFalloff);
			WriteJSNode(jsonRagdoll, "maxAngularVelocity", maxAngularVelocity);
			WriteJSNode(jsonRagdoll, "maxLinearVelocity", maxLinearVelocity);
			WriteJSNode(jsonRagdoll, "ragdollDensity", ragdollDensity);
			WriteJSNode(jsonRagdoll, "angularPinning", angularPinning);
			WriteJSNode(jsonRagdoll, "updateJointAnchors", updateJointAnchors);
			WriteJSNode(jsonRagdoll, "supportTranslationAnimation", supportTranslationAnimation);
			WriteJSNode(jsonRagdoll, "angularLimits", angularLimits);
			WriteJSNode(jsonRagdoll, "internalCollisions", internalCollisions);
			//WriteJSNode(jsonRagdoll, "bnmInfo", ragdollInfo->bnmInfo);						// 不直接存储block的引用，通过信息创建新的block，并引用它
			//WriteJSNode(jsonRagdoll, "rigiBodyGroupName", ragdollInfo->rigiBodyGroupName);	// 附加组件重新创建

			// pos 
			WritePosJSNode(jsonRagdoll, info.mSimpleData);
		}
		// ragdollblocks
		cJSON* jsonRagdollBlocks = cJSON_CreateArray();
		cJSON_AddItemToObject(jsonNode, "RagdollBlocks", jsonRagdollBlocks);
		{
			// item 
			for (const auto& block : info.mBlocksData)
			{

				cJSON* jsonBlock = cJSON_CreateObject();
				cJSON_AddItemToArray(jsonRagdollBlocks, jsonBlock);

				// boneName jointTargetBoneName
				WriteJSNode(jsonBlock, "boneName", block.mBoneName);
				WriteJSNode(jsonBlock, "jointTargetBoneName", block.mTargetBoneName);

				// nodeComponent
				cJSON* jsonBlockNodeComponent = cJSON_CreateObject();
				cJSON_AddItemToObject(jsonBlock, "Node", jsonBlockNodeComponent);
				WritePosJSNode(jsonBlockNodeComponent, block.mNodeData);

				// colliderComponent
				cJSON* jsonBlockColliderComponent = cJSON_CreateObject();
				cJSON_AddItemToObject(jsonBlock, "Collider", jsonBlockColliderComponent);
				WritePosJSNode(jsonBlockColliderComponent, block.mCollsionData);
				WriteJSNode(jsonBlockColliderComponent, "isTrigger", block.mCollsionData.mIsTrigger);
				WriteJSNode(jsonBlockColliderComponent, "materialId", block.mCollsionData.mMaterialId);
				WriteJSNode(jsonBlockColliderComponent, "filterGroupType", block.mCollsionData.mFilterGroupType);
				WriteJSNode(jsonBlockColliderComponent, "canInteractFilterGroupType", block.mCollsionData.mCanInteractFilterGroupType);
				const RagdollComponentInitData::BoxTag* box = nullptr;
				const RagdollComponentInitData::CapsuleTag* capsule = nullptr;
				if ((box = std::get_if<RagdollComponentInitData::BoxTag>(&block.mCollsionData.mCollsionImpl)) != nullptr)
				{
					WriteJSNode(jsonBlockColliderComponent, "colliderType",COMPONENTTYPE(BoxColliderComponent));
					WriteJSNode(jsonBlockColliderComponent, "halfSize", box->mHalfSize);
				}
				else if ((capsule = std::get_if<RagdollComponentInitData::CapsuleTag>(&block.mCollsionData.mCollsionImpl)) != nullptr)
				{
					WriteJSNode(jsonBlockColliderComponent, "colliderType", COMPONENTTYPE(CapsuleColliderComponent));
					WriteJSNode(jsonBlockColliderComponent, "radius", capsule->mRadius);
					WriteJSNode(jsonBlockColliderComponent, "height", capsule->mHeight);
				}

				// rigidBodyComponent 
				cJSON* jsonBlockRigidBodyComponent = cJSON_CreateObject();
				cJSON_AddItemToObject(jsonBlock, "RigidBody", jsonBlockRigidBodyComponent);

				WriteJSNode(jsonBlockRigidBodyComponent, "useGravity",block.mRigdBodyData.mUseGravity);
				WriteJSNode(jsonBlockRigidBodyComponent, "collisionDetection", block.mRigdBodyData.mCollisionDetection);
				WriteJSNode(jsonBlockRigidBodyComponent, "constraints", block.mRigdBodyData.mConstraints);
				WriteJSNode(jsonBlockRigidBodyComponent, "mass", block.mRigdBodyData.mMass);
				WriteJSNode(jsonBlockRigidBodyComponent, "drag", block.mRigdBodyData.mDrag);
				WriteJSNode(jsonBlockRigidBodyComponent, "angularDrag", block.mRigdBodyData.mAngularDrag);

				// jointComponent
				cJSON* jsonBlockJointComponent = cJSON_CreateObject();
				cJSON_AddItemToObject(jsonBlock, "Joint", jsonBlockJointComponent);
				WritePosJSNode(jsonBlockJointComponent, block.mJointData);

				WriteJSNode(jsonBlockJointComponent, "axisOffset", block.mJointData.mAxisOffset);
				if (block.mJointData.mBreakForce == FLT_MAX)
					WriteJSNode(jsonBlockJointComponent, "breakForce", -1.f);
				else
					WriteJSNode(jsonBlockJointComponent, "breakForce", block.mJointData.mBreakForce);
				if (block.mJointData.mBreakTorque == FLT_MAX)
					WriteJSNode(jsonBlockJointComponent, "breakTorque", -1.f);
				else
					WriteJSNode(jsonBlockJointComponent, "breakTorque", block.mJointData.mBreakTorque);
				WriteJSNode(jsonBlockJointComponent, "enableProjection", block.mJointData.mEnableProjection);
				WriteJSNode(jsonBlockJointComponent, "enableCollision", block.mJointData.mEnableCollision);
				WriteJSNode(jsonBlockJointComponent, "enablePreprocessing", block.mJointData.mEnablePreprocessing);
				WriteJSNode(jsonBlockJointComponent, "massScaleSelf", block.mJointData.mMassScaleSelf);
				WriteJSNode(jsonBlockJointComponent, "massScaleConnected", block.mJointData.mMassScaleConnected);
				WriteJSNode(jsonBlockJointComponent, "ConnectedComName", block.mJointData.mConnectedComName);
				WriteJSNode(jsonBlockJointComponent, "SelfComName", block.mJointData.mSelfComName);

				WriteJSNode(jsonBlockJointComponent, "linearLimit", block.mJointData.mLinearLimit);
				WriteJSNode(jsonBlockJointComponent, "angularXLowLimit", block.mJointData.mAngularXLowLimit);
				WriteJSNode(jsonBlockJointComponent, "angularXHighLimit", block.mJointData.mAngularXHighLimit);
				WriteJSNode(jsonBlockJointComponent, "angularYLimit", block.mJointData.mAngularYLimit);
				WriteJSNode(jsonBlockJointComponent, "angularZLimit", block.mJointData.mAngularZLimit);
				WriteJSNode(jsonBlockJointComponent, "linearLimitParm", block.mJointData.mLinearLimitParm);
				WriteJSNode(jsonBlockJointComponent, "angularXLimitParm", block.mJointData.mAngularXLimitParm);
				WriteJSNode(jsonBlockJointComponent, "angularYZLimitParm", block.mJointData.mAngularYZLimitParm);
				WriteJSNode(jsonBlockJointComponent, "driveX", block.mJointData.mDriveX);
				WriteJSNode(jsonBlockJointComponent, "driveY", block.mJointData.mDriveY);
				WriteJSNode(jsonBlockJointComponent, "driveZ", block.mJointData.mDriveZ);
				WriteJSNode(jsonBlockJointComponent, "angularDriveX", block.mJointData.mAngularDriveX);
				WriteJSNode(jsonBlockJointComponent, "angularDriveYZ", block.mJointData.mAngularDriveYZ);
				WriteJSNode(jsonBlockJointComponent, "slerpDrive", block.mJointData.mSlerpDrive);
				WriteJSNode(jsonBlockJointComponent, "targetPosition", block.mJointData.mTargetPosition);
				WriteJSNode(jsonBlockJointComponent, "targetVelocity", block.mJointData.mTargetVelocity);
				WriteJSNode(jsonBlockJointComponent, "targetRotation", block.mJointData.mTargetRotation);
				WriteJSNode(jsonBlockJointComponent, "targetAngularVelocity", block.mJointData.mTargetAngularVelocity);
				WriteJSNode(jsonBlockJointComponent, "rotationDriveMode", block.mJointData.mRotationDriveMode);
				WriteJSNode(jsonBlockJointComponent, "projectionDistance", block.mJointData.mProjectionDistance);
				WriteJSNode(jsonBlockJointComponent, "projectionAngle", block.mJointData.mProjectionAngle);
				WriteJSNode(jsonBlockJointComponent, "linearLimitScale", block.mJointData.mLinearLimitScale);
				WriteJSNode(jsonBlockJointComponent, "motionX", block.mJointData.mMotionX);
				WriteJSNode(jsonBlockJointComponent, "motionY", block.mJointData.mMotionY);
				WriteJSNode(jsonBlockJointComponent, "motionZ", block.mJointData.mMotionZ);
				WriteJSNode(jsonBlockJointComponent, "motionAX", block.mJointData.mMotionAX);
				WriteJSNode(jsonBlockJointComponent, "motionAY", block.mJointData.mMotionAY);
				WriteJSNode(jsonBlockJointComponent, "motionAZ", block.mJointData.mMotionAZ);
				// muscles desc 
				cJSON* jsonBlockMuscleDesc = cJSON_CreateObject();
				cJSON_AddItemToObject(jsonBlock, "MusclesDesc", jsonBlockMuscleDesc);

				WriteJSNode(jsonBlockMuscleDesc, "group", (int)block.mGroup);
				WriteJSNode(jsonBlockMuscleDesc, "mappingWeight", block.mMapping);
				WriteJSNode(jsonBlockMuscleDesc, "pinWeight", block.mPin);
				WriteJSNode(jsonBlockMuscleDesc, "muscleWeight", block.mWeight);
				WriteJSNode(jsonBlockMuscleDesc, "muscleDamper", block.mDamper);
			}
		}
	}

	void JsonToData(RagdollComponentInitData& info, cJSON* jsonNode)
	{
		if (jsonNode == nullptr)
			return ;

		cJSON* jsonRagdoll = cJSON_GetObjectItem(jsonNode, "RagdollData");
		cJSON* jsonRagdollBlocks = cJSON_GetObjectItem(jsonNode, "RagdollBlocks");

		// ragdollData
		if (nullptr != jsonRagdoll)
		{
			RagdollStateSettings	stateSettings = RagdollStateSettings::Default;
			float					blendTime = 0.1f;
			bool					fixTargetTransforms = true;
			int						solverIterationCount = 10;
			float					mappingWeight = 1.0f;
			float					pinWeight = 1.0f;
			float					muscleWeight = 1.0f;
			float					muscleSpring = 50.0f;
			float					muscleDamper = 1.0f;
			float					pinPow = 4.0f;
			float					pinDistanceFalloff = 0.0f;
			float					maxAngularVelocity = 25.f;
			float					maxLinearVelocity = 60.f;
			float					ragdollDensity = 1e3f;
			bool					angularPinning = false;
			bool					updateJointAnchors = true;
			bool					supportTranslationAnimation = false;
			bool					angularLimits = true;
			bool					internalCollisions = true;

			//ReadJSNode(jsonRagdoll, "roleName", roleName);					// 使用新的角色
			ReadJSNode(jsonRagdoll, "stateSettings", stateSettings);
			ReadJSNode(jsonRagdoll, "blendTime", blendTime);
			ReadJSNode(jsonRagdoll, "fixTargetTransforms", fixTargetTransforms);
			ReadJSNode(jsonRagdoll, "solverIterationCount", solverIterationCount);
			ReadJSNode(jsonRagdoll, "mappingWeight", mappingWeight);
			ReadJSNode(jsonRagdoll, "pinWeight", pinWeight);
			ReadJSNode(jsonRagdoll, "muscleWeight", muscleWeight);
			ReadJSNode(jsonRagdoll, "muscleSpring", muscleSpring);
			ReadJSNode(jsonRagdoll, "muscleDamper", muscleDamper);
			ReadJSNode(jsonRagdoll, "pinPow", pinPow);
			ReadJSNode(jsonRagdoll, "pinDistanceFalloff", pinDistanceFalloff);
			ReadJSNode(jsonRagdoll, "maxAngularVelocity", maxAngularVelocity);
			ReadJSNode(jsonRagdoll, "maxLinearVelocity", maxLinearVelocity);
			ReadJSNode(jsonRagdoll, "ragdollDensity", ragdollDensity);
			ReadJSNode(jsonRagdoll, "angularPinning", angularPinning);
			ReadJSNode(jsonRagdoll, "updateJointAnchors", updateJointAnchors);
			ReadJSNode(jsonRagdoll, "supportTranslationAnimation", supportTranslationAnimation);
			ReadJSNode(jsonRagdoll, "angularLimits", angularLimits);
			ReadJSNode(jsonRagdoll, "internalCollisions", internalCollisions);
			//ReadJSNode(jsonRagdoll, "bnmInfo", bnmInfo);						// 不直接读取block的引用，通过信息创建新的block，并引用它
			//ReadJSNode(jsonRagdoll, "rigiBodyGroupName", rigiBodyGroupName);	// 附加组件重新创建

			info.mSimpleData.stateSettings = stateSettings;
			info.mSimpleData.blendTime = blendTime;
			info.mSimpleData.fixTargetTransforms = fixTargetTransforms;
			info.mSimpleData.solverIterationCount = solverIterationCount;
			info.mSimpleData.mappingWeight = mappingWeight;
			info.mSimpleData.pinWeight = pinWeight;
			info.mSimpleData.muscleWeight = muscleWeight;
			info.mSimpleData.muscleSpring = muscleSpring;
			info.mSimpleData.muscleDamper = muscleDamper;
			info.mSimpleData.pinPow = pinPow;
			info.mSimpleData.pinDistanceFalloff = pinDistanceFalloff;
			info.mSimpleData.maxAngularVelocity = maxAngularVelocity;
			info.mSimpleData.maxLinearVelocity = maxLinearVelocity;
			info.mSimpleData.ragdollDensity = ragdollDensity;
			info.mSimpleData.angularPinning = angularPinning;
			info.mSimpleData.updateJointAnchors = updateJointAnchors;
			info.mSimpleData.supportTranslationAnimation = supportTranslationAnimation;
			info.mSimpleData.angularLimits = angularLimits;
			info.mSimpleData.internalCollisions = internalCollisions;

			// pos 
			ReadPosJSNode(jsonRagdoll, info.mSimpleData);
		}

		// Create and init ragdollblocks 
		if (nullptr != jsonRagdollBlocks && jsonRagdollBlocks->child != nullptr)
		{
			info.mBlocksData.clear();
			cJSON* jsonBlock = jsonRagdollBlocks->child;
			while (jsonBlock)
			{
				info.mBlocksData.emplace_back();

				decltype(auto) block = info.mBlocksData.back();
				//auto& muscles = ragdoll->mRagdollBlocks.back();

				// boneName jointTargetBoneName
				ReadJSNode(jsonBlock, "boneName", block.mBoneName);
				ReadJSNode(jsonBlock, "jointTargetBoneName", block.mTargetBoneName);

				// nodeComponent
				cJSON* jsonBlockNodeComponent = cJSON_GetObjectItem(jsonBlock, "Node");
				if (nullptr != jsonBlockNodeComponent)
				{
					ReadPosJSNode(jsonBlockNodeComponent, block.mNodeData);
				}

				// colliderComponent
				cJSON* jsonBlockColliderComponent = cJSON_GetObjectItem(jsonBlock, "Collider");
				if (jsonBlockColliderComponent != nullptr)
				{
					Name ColliderType;
					ReadJSNode(jsonBlockColliderComponent, "colliderType", ColliderType);
					if (ColliderType == COMPONENTTYPE(BoxColliderComponent))
					{
						Vector3 halfSize;
						ReadJSNode(jsonBlockColliderComponent, "halfSize", halfSize);
						block.mCollsionData.mCollsionImpl = RagdollComponentInitData::BoxTag
						{
							.mHalfSize = halfSize
						};
					}
					else if (ColliderType == COMPONENTTYPE(CapsuleColliderComponent))
					{
						float radius = 0.f, height = 0.f;
						ReadJSNode(jsonBlockColliderComponent, "radius", radius);
						ReadJSNode(jsonBlockColliderComponent, "height", height);

						block.mCollsionData.mCollsionImpl = RagdollComponentInitData::CapsuleTag
						{
							.mRadius = radius,
							.mHeight = height
						};
					}
					bool   isTrigger = false;
					int    materialId = 0;
					uint32 filterGroupType = 0xffffffff;
					uint32 canInteractFilterGroupType = 0xffffffff;
					ReadJSNode(jsonBlockColliderComponent, "isTrigger", isTrigger);
					ReadJSNode(jsonBlockColliderComponent, "materialId", materialId);
					ReadJSNode(jsonBlockColliderComponent, "filterGroupType", filterGroupType);
					ReadJSNode(jsonBlockColliderComponent, "canInteractFilterGroupType", canInteractFilterGroupType);
					block.mCollsionData.mIsTrigger = isTrigger;
					block.mCollsionData.mMaterialId = materialId;
					block.mCollsionData.mFilterGroupType = filterGroupType;
					block.mCollsionData.mCanInteractFilterGroupType = canInteractFilterGroupType;
					ReadPosJSNode(jsonBlockColliderComponent, block.mCollsionData);
				}
				// rigidbodyComponent
				cJSON* jsonBlockRigidBodyComponent = cJSON_GetObjectItem(jsonBlock, "RigidBody");
				if (nullptr != jsonBlockRigidBodyComponent)
				{
					bool	useGracity = false;
					bool	isKinematic = false;
					int		collisionDetection = EchoPhysics::Discrete;
					int		constraints = EchoPhysics::FreezeNone;
					float	mass = 1.0f;
					float	drag = 0.0f;
					float	angularDrag = 0.05f;

					ReadJSNode(jsonBlockRigidBodyComponent, "useGravity", useGracity);
					//ReadJSNode(jsonBlockRigidBodyComponent, "isKinematic", isKinematic);
					isKinematic = true;
					ReadJSNode(jsonBlockRigidBodyComponent, "collisionDetection", collisionDetection);
					ReadJSNode(jsonBlockRigidBodyComponent, "constraints", constraints);
					ReadJSNode(jsonBlockRigidBodyComponent, "mass", mass);
					ReadJSNode(jsonBlockRigidBodyComponent, "drag", drag);
					ReadJSNode(jsonBlockRigidBodyComponent, "angularDrag", angularDrag);

					block.mRigdBodyData.mUseGravity  = useGracity;
					block.mRigdBodyData.mCollisionDetection = collisionDetection;
					block.mRigdBodyData.mConstraints = constraints;
					block.mRigdBodyData.mMass = mass;
					block.mRigdBodyData.mDrag = drag;
					block.mRigdBodyData.mAngularDrag = angularDrag;
				}
				// jointComponent
				cJSON* jsonBlockJointComponent = cJSON_GetObjectItem(jsonBlock, "Joint");
				if (nullptr != jsonBlockJointComponent)
				{
					Vector3		AxisOffset = Vector3::ZERO;
					float		BreakForce = FLT_MAX;
					float		BreakTorque = FLT_MAX;
					bool		EnableProjection = false;
					bool		EnableCollision = false;
					bool		EnablePreprocessing = true;
					float		MassScaleSelf = 1.f;
					float		MassScaleConnected = 1.f;

					float		LinearLimit = 0.f;
					float		AngularXLowLimit = 0.f;
					float		AngularXHighLimit = 0.f;
					float		AngularYLimit = 0.f;
					float		AngularZLimit = 0.f;
					JointLimit	LinearLimitParm;
					JointLimit	AngularXLimitParm;
					JointLimit	AngularYZLimitParm;
					JointDrive	DriveX;
					JointDrive	DriveY;
					JointDrive	DriveZ;
					JointDrive	AngularDriveX;
					JointDrive	AngularDriveYZ;
					JointDrive	SlerpDrive;
					Vector3		TargetPosition = Vector3::ZERO;
					Vector3		TargetVelocity = Vector3::ZERO;
					Vector3	TargetRotation = Vector3::ZERO;
					Vector3		TargetAngularVelocity = Vector3::ZERO;
					RDM			RotationDriveMode = RDM::XAndYZ;
					float		ProjectionDistance = 0.1f;
					float		ProjectionAngle = 180.f;
					bool		LinearLimitScale = false;
					JointMotion	MotionX = JointMotion::FREE;
					JointMotion	MotionY = JointMotion::FREE;
					JointMotion	MotionZ = JointMotion::FREE;
					JointMotion	MotionAX = JointMotion::FREE;
					JointMotion	MotionAY = JointMotion::FREE;
					JointMotion	MotionAZ = JointMotion::FREE;

					String ConnectedComName;
					String SelfComName;

					ReadJSNode(jsonBlockJointComponent, "axisOffset", AxisOffset);
					ReadJSNode(jsonBlockJointComponent, "breakForce", BreakForce);
					if (BreakForce < 0.f)
						BreakForce = FLT_MAX;
					ReadJSNode(jsonBlockJointComponent, "breakTorque", BreakTorque);
					if (BreakTorque < 0.f)
						BreakTorque = FLT_MAX;
					ReadJSNode(jsonBlockJointComponent, "enableProjection", EnableProjection);
					ReadJSNode(jsonBlockJointComponent, "enableCollision", EnableCollision);
					ReadJSNode(jsonBlockJointComponent, "enablePreprocessing", EnablePreprocessing);
					ReadJSNode(jsonBlockJointComponent, "massScaleSelf", MassScaleSelf);
					ReadJSNode(jsonBlockJointComponent, "massScaleConnected", MassScaleConnected);
					ReadJSNode(jsonBlockJointComponent, "ConnectedComName", ConnectedComName);
					ReadJSNode(jsonBlockJointComponent, "SelfComName", SelfComName);

					ReadJSNode(jsonBlockJointComponent, "linearLimit", LinearLimit);
					ReadJSNode(jsonBlockJointComponent, "angularXLowLimit", AngularXLowLimit);
					ReadJSNode(jsonBlockJointComponent, "angularXHighLimit", AngularXHighLimit);
					ReadJSNode(jsonBlockJointComponent, "angularYLimit", AngularYLimit);
					ReadJSNode(jsonBlockJointComponent, "angularZLimit", AngularZLimit);
					ReadJSNode(jsonBlockJointComponent, "linearLimitParm", LinearLimitParm);
					ReadJSNode(jsonBlockJointComponent, "angularXLimitParm", AngularXLimitParm);
					ReadJSNode(jsonBlockJointComponent, "angularYZLimitParm", AngularYZLimitParm);
					ReadJSNode(jsonBlockJointComponent, "driveX", DriveX);
					ReadJSNode(jsonBlockJointComponent, "driveY", DriveY);
					ReadJSNode(jsonBlockJointComponent, "driveZ", DriveZ);
					ReadJSNode(jsonBlockJointComponent, "angularDriveX", AngularDriveX);
					ReadJSNode(jsonBlockJointComponent, "angularDriveYZ", AngularDriveYZ);
					ReadJSNode(jsonBlockJointComponent, "slerpDrive", SlerpDrive);
					ReadJSNode(jsonBlockJointComponent, "targetPosition", TargetPosition);
					ReadJSNode(jsonBlockJointComponent, "targetVelocity", TargetVelocity);
					ReadJSNode(jsonBlockJointComponent, "targetRotation", TargetRotation);
					ReadJSNode(jsonBlockJointComponent, "targetAngularVelocity", TargetAngularVelocity);
					ReadJSNode(jsonBlockJointComponent, "rotationDriveMode", RotationDriveMode);
					ReadJSNode(jsonBlockJointComponent, "projectionDistance", ProjectionDistance);
					ReadJSNode(jsonBlockJointComponent, "projectionAngle", ProjectionAngle);
					ReadJSNode(jsonBlockJointComponent, "linearLimitScale", LinearLimitScale);
					ReadJSNode(jsonBlockJointComponent, "motionX", MotionX);
					ReadJSNode(jsonBlockJointComponent, "motionY", MotionY);
					ReadJSNode(jsonBlockJointComponent, "motionZ", MotionZ);
					ReadJSNode(jsonBlockJointComponent, "motionAX", MotionAX);
					ReadJSNode(jsonBlockJointComponent, "motionAY", MotionAY);
					ReadJSNode(jsonBlockJointComponent, "motionAZ", MotionAZ);

					block.mJointData.mAxisOffset = AxisOffset;
					block.mJointData.mBreakForce = BreakForce;
					block.mJointData.mBreakTorque = BreakTorque;
					block.mJointData.mEnableProjection = EnableProjection;
					block.mJointData.mEnableCollision = EnableCollision;
					block.mJointData.mEnablePreprocessing = EnablePreprocessing;
					block.mJointData.mMassScaleSelf = MassScaleSelf;
					block.mJointData.mMassScaleConnected = MassScaleConnected;
					block.mJointData.mSelfComName = SelfComName;
					block.mJointData.mConnectedComName = ConnectedComName;

					block.mJointData.mLinearLimit = LinearLimit;
					block.mJointData.mAngularXLowLimit = AngularXLowLimit;
					block.mJointData.mAngularXHighLimit = AngularXHighLimit;
					block.mJointData.mAngularYLimit = AngularYLimit;
					block.mJointData.mAngularZLimit = AngularZLimit;
					block.mJointData.mLinearLimitParm = LinearLimitParm;
					block.mJointData.mAngularXLimitParm = AngularXLimitParm;
					block.mJointData.mAngularYZLimitParm = AngularYZLimitParm;
					block.mJointData.mDriveX = DriveX;
					block.mJointData.mDriveY = DriveY;
					block.mJointData.mDriveZ = DriveZ;
					block.mJointData.mAngularDriveX = AngularDriveX;
					block.mJointData.mAngularDriveYZ = AngularDriveYZ;
					block.mJointData.mSlerpDrive = SlerpDrive;
					block.mJointData.mTargetPosition = TargetPosition;
					block.mJointData.mTargetVelocity = TargetVelocity;
					block.mJointData.mTargetRotation = TargetRotation;
					block.mJointData.mTargetAngularVelocity = TargetAngularVelocity;
					block.mJointData.mRotationDriveMode = RotationDriveMode;
					block.mJointData.mProjectionDistance = ProjectionDistance;
					block.mJointData.mProjectionAngle = ProjectionAngle;
					block.mJointData.mLinearLimitScale = LinearLimitScale;
					block.mJointData.mMotionX = MotionX;
					block.mJointData.mMotionY = MotionY;
					block.mJointData.mMotionZ = MotionZ;
					block.mJointData.mMotionAX = MotionAX;
					block.mJointData.mMotionAY = MotionAY;
					block.mJointData.mMotionAZ = MotionAZ;
					ReadPosJSNode(jsonBlockJointComponent, block.mJointData);
				}
				// muscles desc 
				cJSON* jsonBlockMusclesDesc = cJSON_GetObjectItem(jsonBlock, "MusclesDesc");
				int 			group;
				float			mappingWeight;
				float			pinWeight;
				float			muscleWeight;
				float			muscleDamper;

				ReadJSNode(jsonBlockMusclesDesc, "group", group);
				ReadJSNode(jsonBlockMusclesDesc, "mappingWeight", mappingWeight);
				ReadJSNode(jsonBlockMusclesDesc, "pinWeight", pinWeight);
				ReadJSNode(jsonBlockMusclesDesc, "muscleWeight", muscleWeight);
				ReadJSNode(jsonBlockMusclesDesc, "muscleDamper", muscleDamper);

				block.mGroup = (RagdollMuscle::MuscleGroup)group;
				block.mMapping = mappingWeight;
				block.mPin = pinWeight;
				block.mWeight = muscleWeight;
				block.mDamper = muscleDamper;
				// end
				jsonBlock = jsonBlock->next;
			}
		}
	}

	bool RagdollComponentInitData::SyncImport(const std::string& ragdollDataPath)
	{
		if (!mResPtr.isNull())
			return false;
		Name resName = Name(ragdollDataPath);

		try
		{
			mResPtr = RagdollDataManager::instance()->load(resName);
		}
		catch (...)
		{
			LogManager::getSingleton().logMessage(Echo::LML_CRITICAL, " RagdollData : File not exit [ " + ragdollDataPath + " ]");
			mResPtr.setNull();
			return false;
		}

		//把json写入数据
		JsonToData(*this, mResPtr->mpData);
		return true;
	}

	bool _PathComp(const std::string& left, const std::string& right)
	{
		std::string comp[2]{ left ,right };
		for (std::string& path : comp)
		{
			std::replace(path.begin(), path.end(), '\\', '/');

			size_t pos ;
			while ((pos = path.find("//")) != path.npos)
			{
				path.erase(path.begin() + pos);
			}
		}

		return comp[0] == comp[1];
	}

	bool RagdollComponentInitData::AsynImport(const std::string& ragdollDataPath,const AsynCompletedCallBack& finishCallBack)
	{
		if (!mResPtr.isNull() && !_PathComp(mResPtr->getName().c_str(), ragdollDataPath))
			return false;

		Name resName = Name(ragdollDataPath);
		if (mResPtr.isNull())
		{
			ResourceBasePtr findRes = RagdollDataManager::instance()->getByName(resName);
			if (!findRes.isNull())
			{
				mResPtr = findRes;
			}
		}
		
		if (!mResPtr.isNull())
		{
			if (mResPtr->isLoaded())
			{
				JsonToData(*this, mResPtr->mpData);
				finishCallBack(true,*this);
			}
			else if (mResPtr->isPrepared())
			{
				mResPtr->load();
				JsonToData(*this, mResPtr->mpData);
				finishCallBack(true, *this);
			}
			else
			{
				ResourceLoadPriority priority = WorkQueue::MakePriority(100, WorkQueue::RESOURCE_TYPE_ENTITY);
				mAynsTicket = ResourceBackgroundQueue::instance()->prepare(RagdollDataManager::instance()->getResourceType(), resName, priority, false, nullptr, nullptr, this);
				mAynsCallBack = finishCallBack;
			}
		}
		else
		{
			mResPtr = RagdollDataManager::instance()->createResource(resName, false);
			ResourceLoadPriority priority = WorkQueue::MakePriority(100, WorkQueue::RESOURCE_TYPE_ENTITY);
			mAynsTicket = ResourceBackgroundQueue::instance()->prepare(RagdollDataManager::instance()->getResourceType(), resName, priority, false, nullptr, nullptr, this);
			mAynsCallBack = finishCallBack;
		}
		return true;
	}

	RagdollComponentInitData::RagdollComponentInitData()
	{
	}

	RagdollComponentInitData::~RagdollComponentInitData()
	{
		AbortAsyn();
	}

	RagdollComponentInitData::RagdollComponentInitData(const RagdollComponentInitData& _lData)
	{
		*this = _lData;
	}

	RagdollComponentInitData& RagdollComponentInitData::operator = (const RagdollComponentInitData& _lData)
	{
		if (_lData.IsImportCompleted())
		{
			mSimpleData = _lData.mSimpleData;
			mBlocksData = _lData.mBlocksData;
			mResPtr = _lData.mResPtr;
		}
		return *this;
	}

	bool RagdollComponentInitData::IsImportCompleted() const
	{
		if (mResPtr.isNull())
			return false;

		if (mResPtr->isLoaded())
		{
			return true;
		}
		else if (mResPtr->isPrepared())
		{
			mResPtr->load();
			return true;
		}

		return false;
	}

	bool RagdollComponentInitData::IsImportDoing() const
	{
		if (mResPtr.isNull())
			return false;

		if (mResPtr->isLoaded())
		{
			return false;
		}
		else if (mResPtr->isPrepared())
		{
			mResPtr->load();
			return false;
		}

		return true; 
	}

	bool RagdollComponentInitData::IsEmpty() const
	{
		return mResPtr.isNull();
	}

	const char* RagdollComponentInitData::GetImportPath() const
	{
		if (mResPtr.isNull())
			return nullptr;
		return mResPtr->getName().c_str();
	}
#ifdef ECHO_EDITOR
	bool RagdollComponentInitData::SyncExport(const std::string& ragdollDataPath)const
	{
		// 文件名
		std::string fileNameString = ragdollDataPath;
		std::replace(fileNameString.begin(), fileNameString.end(), '/', '\\');
		size_t dpos = fileNameString.find_last_of('.');
		if (dpos != std::string::npos)
		{
			fileNameString = fileNameString.substr(0,dpos);
		}
		fileNameString.append(".ragdolldata");

		// 数据流
		DataStreamPtr pDataStream(Root::instance()->CreateDataStream(fileNameString.c_str()));
		if (pDataStream.isNull())
			return false;

		// 将数据写入json
		cJSON* jsonRoot = cJSON_CreateObject();
		DataToJson(*this, jsonRoot);

		// json数据写入文件
		char* data = cJSON_Print(jsonRoot);
		if (data == nullptr)
			return false;
		size_t t = pDataStream->writeData(data, strlen(data));
		assert(t != 0 && "write data size is zreo !!!");
		// 释放资源
		pDataStream->close();
		free(data);

		return true;
	}
#endif
	void RagdollComponentInitData::operationCompleted(BackgroundProcessTicket ticket, const ResourceBackgroundQueue::ResourceRequest& request, const ResourceBasePtr& resourcePtr)
	{
		if (!request.result.error && IsImportCompleted())
		{
			JsonToData(*this, mResPtr->mpData);
			if (mAynsCallBack)
				mAynsCallBack(true, *this);
		}
		else
		{
			LogManager::getSingleton().logMessage(Echo::LML_CRITICAL, " RagdollData : File load faild [ " + request.name.toString() + " ]");
			mResPtr.setNull();
			if (mAynsCallBack)
				mAynsCallBack(false, *this);
		}

		mAynsCallBack = nullptr;
		mAynsTicket = 0;
	}
	void RagdollComponentInitData::AbortAsyn()
	{
		if (!mResPtr.isNull() && mAynsTicket != 0)
		{
			if (ResourceBackgroundQueue::Alive())
				ResourceBackgroundQueue::instance()->abortRequest(mAynsTicket);
			mAynsTicket = 0;
			mAynsCallBack = nullptr;
			mResPtr.setNull();
		}
	}
}
