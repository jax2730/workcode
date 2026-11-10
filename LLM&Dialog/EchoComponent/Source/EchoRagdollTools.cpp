#include "EchoComponentStableHeaders.h"
#include "EchoRagdollTools.h"
#include "IK/EchoIKCommon.h"
#include "EchoRagdollComponent.h"
#include "EchoRigidBodyGroupComponent.h"

#define WritePosJSNode(jsonNode,comp)												\
			do{																		\
				if (comp->IsMoveable())												\
				{																	\
					WriteJSNode(jsonNode, "position", (Vector3)comp->getPosition());\
					WriteJSNode(jsonNode, "eulerAngles", comp->getEulerAngles());	\
					WriteJSNode(jsonNode, "scale", comp->getScale());				\
				}																	\
				else																\
				{																	\
					WriteJSNode(jsonNode, "position", Vector3(0.f,0.f,0.f));		\
					WriteJSNode(jsonNode, "eulerAngles", Vector3(0.f, 0.f, 0.f));	\
					WriteJSNode(jsonNode, "scale", Vector3(1.f, 1.f, 1.f));			\
				}																	\
			}while(false)

#define ReadPosJSNode(jsonNode,comp)												\
			do {																	\
				if (comp->IsMoveable())												\
				{																	\
					Vector3 temp;													\
					ReadJSNode(jsonNode, "position", temp);							\
					comp->setPosition(temp);										\
					if (comp->m_pActComInfo != nullptr)								\
						comp->m_pActComInfo->SetPosition(temp);						\
					ReadJSNode(jsonNode, "eulerAngles", temp);						\
					comp->setEulerAngles(temp);										\
					if (comp->m_pActComInfo != nullptr)								\
						comp->m_pActComInfo->SetEulerAngles(temp);					\
					ReadJSNode(jsonNode, "scale", temp);							\
					comp->setScale(temp);											\
					if (comp->m_pActComInfo != nullptr)								\
						comp->m_pActComInfo->SetScale(temp);						\
				}																	\
			} while (false)

#define WritePosData(data,comp)														\
			do{																		\
				if (comp->IsMoveable())												\
				{																	\
					data.mPosition	= (Vector3)comp->getPosition();					\
					data.mRotation	= comp->getEulerAngles();						\
					data.mScale		= comp->getScale();								\
				}																	\
				else																\
				{																	\
					data.mPosition	= Vector3(0.f,0.f,0.f);							\
					data.mRotation	= Vector3(0.f, 0.f, 0.f);						\
					data.mScale		= Vector3(1.f, 1.f, 1.f);						\
				}																	\
			}while(false)

#define ReadPosData(data,comp)														\
			do {																	\
				if (comp->IsMoveable())												\
				{																	\
					comp->setPosition(data.mPosition);								\
					if (comp->m_pActComInfo != nullptr)								\
						comp->m_pActComInfo->SetPosition(data.mPosition);			\
					comp->setEulerAngles(data.mRotation);							\
					if (comp->m_pActComInfo != nullptr)								\
						comp->m_pActComInfo->SetEulerAngles(data.mRotation);		\
					comp->setScale(data.mScale);									\
					if (comp->m_pActComInfo != nullptr)								\
						comp->m_pActComInfo->SetScale(data.mScale);					\
				}																	\
			} while (false)

namespace Echo
{
	void RagdollTools::Predict(RigidBodyComponent * r, int steps, Vector3 & position, Quaternion & rotation)
	{
	}
	void RagdollTools::Predict(RigidBodyComponent * r, int steps, Vector3 & position, Quaternion & rotation, Vector3 gravity, float drag, float angularDrag)
	{
	}
	void RagdollTools::Predict(Vector3 & position, Quaternion & rotation, Vector3 & velocity, Vector3 & angularVelocity, Vector3 gravity, float drag, float angularDrag)
	{
	}
	Vector3 RagdollTools::GetCenterOfMass(std::vector<RigidBodyComponent*> rigidbodies)
	{
		return Vector3::ZERO;
	}
	Vector3 RagdollTools::GetCenterOfMassVelocity(std::vector<RigidBodyComponent*> rigidbodies)
	{
		return Vector3::ZERO;
	}
	void RagdollTools::DivByInertia(Vector3 & v, Quaternion rotation, Vector3 inertiaTensor)
	{
		v = rotation * Div(Quaternion::QInverse(rotation) * v, inertiaTensor);
	}
	void RagdollTools::ScaleByInertia(Vector3 & v, Quaternion rotation, Vector3 inertiaTensor)
	{
	}
	Vector3 RagdollTools::GetAngularVelocity(Quaternion lastRotation, Quaternion rotation, float deltaTime)
	{
		return Vector3::ZERO;
	}
	Vector3 RagdollTools::GetAngularAcceleration(Quaternion fromR, Quaternion toR)
	{
		return Vector3::ZERO;
	}
	Vector3 RagdollTools::GetAngularAcceleration(Quaternion fromR, Quaternion toR, float deltaTime)
	{
		/*Vector3 axis = Vector3::Cross(fromR * Vector3.forward, toR * Vector3.forward);
		Vector3 axis2 = Vector3.Cross(fromR * Vector3.up, toR * Vector3.up);
		float angle = Quaternion.Angle(fromR, toR);
		Vector3 acc = Vector3.Normalize(axis + axis2) * angle * Mathf.Deg2Rad;*/

		Vector3 axis = Vector3::VCross(fromR * Vector3::FORWARD, toR * Vector3::FORWARD);
		Vector3 axis2 = Vector3::VCross(fromR * Vector3::UP, toR * Vector3::UP);
		float angle = Quaternion::Angle(fromR, toR);
		Vector3 acc = (axis + axis2).GetNormalized() * angle * Math::fDeg2Rad;
		return acc / deltaTime;
	}
	void RagdollTools::AddFromToTorque(RigidBodyComponent * r, Quaternion toR, EchoPhysics::ForceMode forceMode)
	{
	}
	void RagdollTools::AddFromToTorque(RigidBodyComponent * r, Vector3 fromV, Vector3 toV, EchoPhysics::ForceMode forceMode)
	{
	}
	void RagdollTools::AddFromToForce(RigidBodyComponent * r, Vector3 fromV, Vector3 toV, EchoPhysics::ForceMode forceMode)
	{
	}
	Vector3 RagdollTools::GetLinearAcceleration(Vector3 fromPoint, Vector3 toPoint)
	{
		return (toPoint - fromPoint) / EchoIK::Time::fixedDeltaTime;
	}
	Quaternion RagdollTools::ToJointSpace(ConfigurableJointComponent * joint)
	{
		Vector3 forward = Vector3::VCross(joint->GetAxis(), joint->GetSecondaryAxis());
		Vector3 up = Vector3::VCross(forward, joint->GetAxis());
		return Quaternion::LookRotation(forward, up);
	}
	Vector3 RagdollTools::CalculateInertiaTensorCuboid(Vector3 size, float mass)
	{
		float x2 = size.x * size.x;
		float y2 = size.y * size.y;
		float z2 = size.z * size.z;

		float mlp = 1.f / 12.f * mass;

		return Vector3(
			mlp * (y2 + z2),
			mlp * (x2 + z2),
			mlp * (x2 + y2));
	}
	Vector3 RagdollTools::Div(Vector3 v, Vector3 v2)
	{
		return Vector3(v.x / v2.x, v.y / v2.y, v.z / v2.z);
	}
	bool RagdollTools::RayCapsuleIntersect(Vector3 origin, Vector3 direction, Vector3 capsuleTransformPos, Quaternion capsuleTransformRot, Vector3 capsuleCenter, float capsuleRadius, float capsuleHeight, int capsuleDir, float scale)
	{
		float r = capsuleRadius;
		float h = Math::Max(r, capsuleHeight);
		r *= scale;
		h *= scale;

		Vector3 dir = capsuleDir == 0 ? Vector3::RIGHT : capsuleDir == 1 ? Vector3::UP : Vector3::FORWARD;
		dir = capsuleTransformRot * dir;
		float o = (h * 0.5f - r);
		Vector3 dirO = dir * o;

		Vector3 cCenterWorld = capsuleTransformPos + capsuleTransformRot * capsuleCenter * scale;

		Vector3 c1 = cCenterWorld - dirO;
		Vector3 c2 = cCenterWorld + dirO;

		return RayCapsuleIntersect(origin, direction, c1, c2, r);
	}
	bool RagdollTools::RayCapsuleIntersect(Vector3 rayOrigin, Vector3 rayDir, Vector3 c1, Vector3 c2, float cRadius)
	{
		Vector3 cDir = c2 - c1;
		Vector3 c1R = rayOrigin - c1;

		float cDirDot = Vector3::VDot(cDir, cDir);
		float cDirToRayDir = Vector3::VDot(cDir, rayDir);
		float cDirToc1R = Vector3::VDot(cDir, c1R);
		float rayDirToc1R = Vector3::VDot(rayDir, c1R);
		float c1RDot = Vector3::VDot(c1R, c1R);

		float a = cDirDot - cDirToRayDir * cDirToRayDir;
		float b = cDirDot * rayDirToc1R - cDirToc1R * cDirToRayDir;
		float c = cDirDot * c1RDot - cDirToc1R * cDirToc1R - cRadius * cRadius * cDirDot;
		float h = b * b - a * c;
		if (h >= 0.0)
		{
			float t = (-b - Math::Sqrt(h)) / a;

			float y = cDirToc1R + t * cDirToRayDir;

			if (y > 0.0f && y < cDirDot) return t > 0.0f;

			Vector3 oc = (y <= 0.0f) ? c1R : rayOrigin - c2;
			b = Vector3::VDot(rayDir, oc);
			c = Vector3::VDot(oc, oc) - cRadius * cRadius;
			h = b * b - c;
			if (h > 0.0f)
			{
				return (-b - Math::Sqrt(h)) > 0.0f;
			}
		}
		return false;
	}

	Vector3 RagdollTools::InverseActorcomponentPoint(ActorComponent * act, const Vector3 & inPosition) 
	{
		Vector3 newPosition, localPosition;
		ActorComponent* father = nullptr;
		if (act->getNodeBindState())
			father = act->getParentNodeCom().get();
		if (father)
			localPosition = InverseActorcomponentPoint(father, inPosition);
		else
			localPosition = inPosition;

		localPosition -= act->getPosition();
		newPosition = Vector3::RotateVectorByQuat(act->getRotation().GetInverse(), localPosition);

		newPosition /= act->getScale();

		return newPosition;
	}

	Vector3 RagdollTools::ActComponentComponentPoint(ActorComponent * act, Vector3 & inPoint)
	{
		Vector3 worldPos = inPoint;

		ActorComponent* cur = act;
		while (cur)
		{
			worldPos *= (cur->getScale());
			worldPos = Vector3::RotateVectorByQuat(cur->getRotation(), worldPos);
			worldPos += cur->getPosition();

			cur = cur->getParentNodeCom().get();
		}
		return worldPos;
	}

	void RagdollTools::ExportRagdollChildrenNodeAndData(RagdollComponent* info, RagdollComponentInitData& data)
	{
		if (info == nullptr )
			return;

		// ragdollData
		{
			RagdollStateSettings		setting = info->GetStateSetting();
			float						blendTime = info->GetBlendTime();
			bool						fixTargetTransforms = info->IsFixTargetTransforms();
			int							solverIterationCount = info->GetSolverIterationCount();
			float						mappingWeight = info->GetGlobalMappingWeight();
			float						pinWeight = info->GetGlobalPinWeight();
			float						muscleWeight = info->GetGlobalMuscleWeight();
			float						muscleSpring = info->GetGlobalMuscleSpring();
			float						muscleDamper = info->GetGlobalMuscleDamper();
			float						pinPow = info->GetPinPow();
			float						pinDistanceFalloff = info->GetPinDistanceFalloff();
			float						maxAngularVelocity = info->GetMaxAngularVelocity();
			float						maxLinearVelocity = info->GetMaxLinearVelocity();
			float						ragdollDensity = info->GetRagdollDensity();
			bool						angularPinning = info->GetAngularPinning();
			bool						updateJointAnchors = info->GetUpdateJointAnchors();
			bool						supportTranslationAnimation = info->GetSupportTranslationAnimation();
			bool						angularLimits = info->GetAngularLimits();
			bool					 	internalCollisions = info->GetInternalCollisions();

			data.mSimpleData.stateSettings = setting;
			data.mSimpleData.blendTime = blendTime;
			data.mSimpleData.fixTargetTransforms = fixTargetTransforms;
			data.mSimpleData.solverIterationCount = solverIterationCount;
			data.mSimpleData.mappingWeight = mappingWeight;
			data.mSimpleData.pinWeight = pinWeight;
			data.mSimpleData.muscleWeight = muscleWeight;
			data.mSimpleData.muscleSpring = muscleSpring;
			data.mSimpleData.muscleDamper = muscleDamper;
			data.mSimpleData.pinPow = pinPow;
			data.mSimpleData.pinDistanceFalloff = pinDistanceFalloff;
			data.mSimpleData.maxAngularVelocity = maxAngularVelocity;
			data.mSimpleData.maxLinearVelocity = maxLinearVelocity;
			data.mSimpleData.ragdollDensity = ragdollDensity;
			data.mSimpleData.angularPinning = angularPinning;
			data.mSimpleData.updateJointAnchors = updateJointAnchors;
			data.mSimpleData.supportTranslationAnimation = supportTranslationAnimation;
			data.mSimpleData.angularLimits = angularLimits;
			data.mSimpleData.internalCollisions = internalCollisions;

			// pos 
			WritePosData(data.mSimpleData, info);

		}
		// ragdollblocks
		{
			// item 
			data.mBlocksData.clear();
			for (const auto& block : info->mRagdollBlocks)
			{
				// block 是否有效
				if (block.BoneName.empty() ||
					block.node.expired() || block.node == nullptr ||
					block.collider.expired() || block.collider == nullptr ||
					block.rigidbody.expired() || block.rigidbody == nullptr ||
					block.joint.expired() || block.joint == nullptr)
				{
					continue;
				}

				decltype(auto) blockData = data.mBlocksData.emplace_back();

				// boneName jointTargetBoneName
				blockData.mBoneName = block.BoneName.toString();
				blockData.mTargetBoneName = block.jointTarget.toString();

				// nodeComponent
				NodeComponent* node = static_cast<NodeComponent*>(block.node.getPointer());
				WritePosData(blockData.mNodeData, node);

				// colliderComponent
				ColliderComponent* collider = static_cast<ColliderComponent*>(block.collider.getPointer());
				WritePosData(blockData.mCollsionData, collider);

				blockData.mCollsionData.mIsTrigger			= collider->GetIsTrigger();
				blockData.mCollsionData.mMaterialId			= collider->GetMaterialId();
				blockData.mCollsionData.mFilterGroupType	= collider->GetFilterGroupType();
				blockData.mCollsionData. mCanInteractFilterGroupType = collider->GetCanInteractFilterGroupType();
				if (collider->getType() == COMPONENTTYPE(BoxColliderComponent))
				{
					BoxColliderComponent* boxCollider = static_cast<BoxColliderComponent*>(collider);
					blockData.mCollsionData.mCollsionImpl = RagdollComponentInitData::BoxTag
					{ 
						.mHalfSize = boxCollider->GetHalfSize() 
					};
				}
				else if (collider->getType() == COMPONENTTYPE(CapsuleColliderComponent))
				{
					CapsuleColliderComponent* capsuleCollider = static_cast<CapsuleColliderComponent*>(collider);
					blockData.mCollsionData.mCollsionImpl = RagdollComponentInitData::CapsuleTag
					{
						.mRadius = capsuleCollider->GetRadius(),
						.mHeight = capsuleCollider->GetHeight()
					};
				}

				// rigidBodyComponent 
				RigidBodyComponent* rigidBody = static_cast<RigidBodyComponent*>(block.rigidbody.getPointer());

				blockData.mRigdBodyData.mUseGravity			= rigidBody->GetUseGravity();
				blockData.mRigdBodyData.mCollisionDetection	= rigidBody->GetCollisionDetection();
				blockData.mRigdBodyData.mConstraints		= rigidBody->GetConstraints();
				blockData.mRigdBodyData.mMass				= rigidBody->GetMass();
				blockData.mRigdBodyData.mDrag				= rigidBody->GetDrag();
				blockData.mRigdBodyData.mAngularDrag		= rigidBody->GetAngularDrag();

				// jointComponent
				ConfigurableJointComponent* joint = static_cast<ConfigurableJointComponent*>(block.joint.getPointer());

				WritePosData(blockData.mJointData, joint);

				blockData.mJointData.mAxisOffset = joint->GetAxisOffset();
				if (joint->GetBreakForce() == FLT_MAX)
					blockData.mJointData.mBreakForce = -1.f;
				else
					blockData.mJointData.mBreakForce = joint->GetBreakForce();
				if (joint->GetBreakTorque() == FLT_MAX)
					blockData.mJointData.mBreakTorque = -1.f;
				else
					blockData.mJointData.mBreakTorque = joint->GetBreakTorque();
				blockData.mJointData.mEnableProjection = joint->GetEnableProjection();
				blockData.mJointData.mEnableCollision = joint->GetEnableCollision();
				blockData.mJointData.mEnablePreprocessing = joint->GetEnablePreprocessing();
				blockData.mJointData.mMassScaleSelf = joint->GetMassScaleSelf();
				blockData.mJointData.mMassScaleConnected = joint->GetMassScaleConnected();
				blockData.mJointData.mConnectedComName = joint->GetConnectedRigidBody();
				blockData.mJointData.mSelfComName = joint->GetSelfRigidBody();

				blockData.mJointData.mLinearLimit = joint->GetLinearLimit();
				blockData.mJointData.mAngularXLowLimit = joint->GetAngularXLowLimit();
				blockData.mJointData.mAngularXHighLimit = joint->GetAngularXHighLimit();
				blockData.mJointData.mAngularYLimit = joint->GetAngularYLimit();
				blockData.mJointData.mAngularZLimit = joint->GetAngularZLimit();
				blockData.mJointData.mLinearLimitParm = joint->GetLinearLimitParm();
				blockData.mJointData.mAngularXLimitParm = joint->GetAngularXLimitParm();
				blockData.mJointData.mAngularYZLimitParm = joint->GetAngularYZLimitParm();
				blockData.mJointData.mDriveX = joint->GetDriveX();
				blockData.mJointData.mDriveY = joint->GetDriveY();
				blockData.mJointData.mDriveZ = joint->GetDriveZ();
				blockData.mJointData.mAngularDriveX = joint->GetAngularDriveX();
				blockData.mJointData.mAngularDriveYZ = joint->GetAngularDriveYZ();
				blockData.mJointData.mSlerpDrive = joint->GetSlerpDrive();
				blockData.mJointData.mTargetPosition = joint->GetTargetPosition();
				blockData.mJointData.mTargetVelocity = joint->GetTargetVelocity();
				blockData.mJointData.mTargetRotation = joint->GetTargetRotation();
				blockData.mJointData.mTargetAngularVelocity = joint->GetTargetAngularVelocity();
				blockData.mJointData.mRotationDriveMode = joint->GetRotationDriveMode();
				blockData.mJointData.mProjectionDistance = joint->GetProjectionDistance();
				blockData.mJointData.mProjectionAngle = joint->GetProjectionAngle();
				blockData.mJointData.mLinearLimitScale = joint->GetLinearLimitScale();
				blockData.mJointData.mMotionX = joint->GetMotionX();
				blockData.mJointData.mMotionY = joint->GetMotionY();
				blockData.mJointData.mMotionZ = joint->GetMotionZ();
				blockData.mJointData.mMotionAX = joint->GetMotionAX();
				blockData.mJointData.mMotionAY = joint->GetMotionAY();
				blockData.mJointData.mMotionAZ = joint->GetMotionAZ();
				// muscles desc 
				blockData.mGroup	= block.group;
				blockData.mMapping	= block.mapping;
				blockData.mPin		= block.pin;
				blockData.mWeight	= block.weight;
				blockData.mDamper	= block.damper;
			}
		}
	}

	bool RagdollTools::ImportRagdollChildrenNodeAndData(EchoWeakPtr<RoleComponent> role,  RagdollComponentPtr ragdoll, bool runtime, const RagdollComponentInitData& data)
	{
		if (ImportRagdollChildrenNode(role,ragdoll,data))
			return	ImportRagdollData(role, ragdoll,runtime,data);
		return false;
	}

	bool RagdollTools::ImportRagdollChildrenNode(EchoWeakPtr<RoleComponent> role, RagdollComponentPtr ragdoll, const RagdollComponentInitData& data)
	{
		if (ragdoll.expired() || role.expired())
			return false;

		RagdollComponentData ragdollData;
		// ragdollData
		{
			ragdollData.stateSettings = data.mSimpleData.stateSettings;
			ragdollData.blendTime = data.mSimpleData.blendTime;
			ragdollData.fixTargetTransforms = data.mSimpleData.fixTargetTransforms;
			ragdollData.solverIterationCount = data.mSimpleData.solverIterationCount;
			ragdollData.mappingWeight = data.mSimpleData.mappingWeight;
			ragdollData.pinWeight = data.mSimpleData.pinWeight;
			ragdollData.muscleWeight = data.mSimpleData.muscleWeight;
			ragdollData.muscleSpring = data.mSimpleData.muscleSpring;
			ragdollData.muscleDamper = data.mSimpleData.muscleDamper;
			ragdollData.pinPow = data.mSimpleData.pinPow;
			ragdollData.pinDistanceFalloff = data.mSimpleData.pinDistanceFalloff;
			ragdollData.maxAngularVelocity = data.mSimpleData.maxAngularVelocity;
			ragdollData.maxLinearVelocity = data.mSimpleData.maxLinearVelocity;
			ragdollData.ragdollDensity = data.mSimpleData.ragdollDensity;
			ragdollData.angularPinning = data.mSimpleData.angularPinning;
			ragdollData.updateJointAnchors = data.mSimpleData.updateJointAnchors;
			ragdollData.supportTranslationAnimation = data.mSimpleData.supportTranslationAnimation;
			ragdollData.angularLimits = data.mSimpleData.angularLimits;
			ragdollData.internalCollisions = data.mSimpleData.internalCollisions;
			// pos 
			ReadPosData(data.mSimpleData, ragdoll);
		}
		ragdoll->LoadStaticData((Name)role->getComName(), &ragdollData);
		return true;
	}

	bool RagdollTools::ImportRagdollData(EchoWeakPtr<RoleComponent> role, RagdollComponentPtr ragdoll, bool runtime, const RagdollComponentInitData& data)
	{
		if (ragdoll.expired() || role.expired())
			return false;

		RoleIKComponent* pRoleIK = GetRoleComIK(role.get());
		if (pRoleIK == nullptr)
			return false;
		std::vector<RagdollBlockDesc>	 blockData;
		{
			for (decltype(auto) blockInfo : data.mBlocksData)
			{
				if (pRoleIK->getBoneNodeByName(Name(blockInfo.mBoneName)) == nullptr)
					continue;

				blockData.emplace_back();
				auto& block = blockData.back();
				//auto& muscles = ragdoll->mRagdollBlocks.back();

				// boneName jointTargetBoneName
				block.BoneName = blockInfo.mBoneName;
				block.jointTarget = blockInfo.mTargetBoneName;

				// nodeComponent
				bool NodeRuntime = ragdoll->IsRuntimeCom() ? true : runtime;
				NodeComponent* node = static_cast<NodeComponent*>(ragdoll->getActor()->addComponent(	// new node
					COMPONENTTYPE(NodeComponent),
					block.BoneName.toString(),
					ragdoll->weak_from_this(),
					nullptr, 0, NodeRuntime).get()
					);

				if (nullptr != node)
				{
					ReadPosData(blockInfo.mNodeData, node);
					block.nodeName = node->getComName();

					// colliderComponent
					NodeColliderComponent* collider = nullptr;
					ColliderComponentInfo* colliderInfo = nullptr;
					{
						Name ColliderType;
						bool colliderRuntime = node->IsRuntimeCom() ? true : runtime;
						const RagdollComponentInitData::BoxTag* box = nullptr;
						const RagdollComponentInitData::CapsuleTag* capsule = nullptr;
						if (nullptr != (box = std::get_if<RagdollComponentInitData::BoxTag>(&blockInfo.mCollsionData.mCollsionImpl)))
						{
							collider = static_cast<NodeColliderComponent*>(ragdoll->getActor()->addComponent(	// new node
								COMPONENTTYPE(BoxColliderComponent),
								"Collider_" + block.BoneName.toString(),
								node->weak_from_this(),
								nullptr, 0, colliderRuntime).get()
								);
							if (collider != nullptr)
							{
								Vector3 halfSize = box->mHalfSize;
								colliderInfo = static_cast<BoxColliderComponentInfo*>(collider->m_pActComInfo);

								if (colliderInfo != 0)
									static_cast<BoxColliderComponentInfo*>(colliderInfo)->SetHalfSize(halfSize, static_cast<BoxColliderComponent*>(collider));
								else
									static_cast<BoxColliderComponent*>(collider)->SetHalfSize(halfSize);
							}
						}
						else if (nullptr != (capsule = std::get_if<RagdollComponentInitData::CapsuleTag>(&blockInfo.mCollsionData.mCollsionImpl)))
						{
							collider = static_cast<NodeColliderComponent*>(ragdoll->getActor()->addComponent(	// new node
								COMPONENTTYPE(CapsuleColliderComponent),
								"Collider_" + block.BoneName.toString(),
								node->weak_from_this(),
								nullptr, 0, colliderRuntime).get()
								);
							if (collider != nullptr)
							{
								float radius = capsule->mRadius, height = capsule->mHeight;
								colliderInfo = static_cast<CapsuleColliderComponentInfo*>(collider->m_pActComInfo);
								if (colliderInfo != 0)
								{
									static_cast<CapsuleColliderComponentInfo*>(colliderInfo)->SetRadius(radius, (CapsuleColliderComponent*)(collider));
									static_cast<CapsuleColliderComponentInfo*>(colliderInfo)->SetHeight(height, (CapsuleColliderComponent*)(collider));
								}
								else
								{
									static_cast<CapsuleColliderComponent*>(collider)->SetRadius(radius);
									static_cast<CapsuleColliderComponent*>(collider)->SetHeight(height);
								}
							}
						}
						if (collider != nullptr)
						{
							if (colliderInfo != nullptr)
							{
								colliderInfo->SetIsTrigger(blockInfo.mCollsionData.mIsTrigger, collider);
								colliderInfo->SetMaterialId(blockInfo.mCollsionData.mMaterialId, collider);
								colliderInfo->SetFilterGroupType(blockInfo.mCollsionData.mFilterGroupType, collider);
								colliderInfo->SetCanInteractFilterGroupType(blockInfo.mCollsionData.mCanInteractFilterGroupType, collider);
							}
							else
							{
								collider->SetIsTrigger(blockInfo.mCollsionData.mIsTrigger);
								collider->SetMaterialId(blockInfo.mCollsionData.mMaterialId);
								collider->SetFilterGroupType(blockInfo.mCollsionData.mFilterGroupType);
								collider->SetCanInteractFilterGroupType(blockInfo.mCollsionData.mCanInteractFilterGroupType);
							}

							ReadPosData(blockInfo.mCollsionData, collider);
							block.colliderName = collider->getComName();
						}
					}
					// rigidbodyComponent
					bool rigidBodyRuntime = node->IsRuntimeCom() ? true : runtime;
					RigidBodyComponent* rigidBody = static_cast<RigidBodyComponent*>(ragdoll->getActor()->addComponent(
						COMPONENTTYPE(RigidBodyComponent),
						"RigidBody_" + block.BoneName.toString(),
						node->weak_from_this(),
						nullptr, 0, rigidBodyRuntime
					).get());
					if (nullptr != rigidBody)
					{
						RigidBodyComponentInfo* rigidBodyInfo = static_cast<RigidBodyComponentInfo*>(rigidBody->m_pActComInfo);

						if (rigidBodyInfo != nullptr)
						{
							rigidBodyInfo->SetUseGravity(blockInfo.mRigdBodyData.mUseGravity, rigidBody);
							rigidBodyInfo->SetCollisionDetection(blockInfo.mRigdBodyData.mCollisionDetection, rigidBody);
							rigidBodyInfo->SetIsKinematic(true, rigidBody);
							rigidBodyInfo->SetConstraints(blockInfo.mRigdBodyData.mConstraints, rigidBody);
							rigidBodyInfo->SetMass(blockInfo.mRigdBodyData.mMass, rigidBody);
							rigidBodyInfo->SetDrag(blockInfo.mRigdBodyData.mDrag, rigidBody);
							rigidBodyInfo->SetAngularDrag(blockInfo.mRigdBodyData.mAngularDrag, rigidBody);
						}
						else
						{
							rigidBody->SetUseGravity(blockInfo.mRigdBodyData.mUseGravity);
							rigidBody->SetCollisionDetection(blockInfo.mRigdBodyData.mCollisionDetection);
							rigidBody->SetIsKinematic(true);
							rigidBody->SetConstraints(blockInfo.mRigdBodyData.mConstraints);
							rigidBody->SetMass(blockInfo.mRigdBodyData.mMass);
							rigidBody->SetDrag(blockInfo.mRigdBodyData.mDrag);
							rigidBody->SetAngularDrag(blockInfo.mRigdBodyData.mAngularDrag);
						}
						//ReadPosJSNode(jsonBlockRigidBodyComponent, rigidBody);
						block.rigidbodyName = rigidBody->getComName();
					}
					// jointComponent
					bool jointRuntime = node->IsRuntimeCom() ? true : runtime;
					ConfigurableJointComponent* joint = static_cast<ConfigurableJointComponent*>(ragdoll->getActor()->addComponent(
						COMPONENTTYPE(ConfigurableJointComponent),
						"Joint_" + block.BoneName.toString(),
						node->weak_from_this(),
						nullptr, 0, jointRuntime).get());
					if (nullptr != joint)
					{
						ConfigurableJointComponentInfo* jointInfo = static_cast<ConfigurableJointComponentInfo*>(joint->m_pActComInfo);

						Vector3		AxisOffset = blockInfo.mJointData.mAxisOffset;
						float		BreakForce = blockInfo.mJointData.mBreakForce;
						float		BreakTorque = blockInfo.mJointData.mBreakTorque;
						bool		EnableProjection = blockInfo.mJointData.mEnableProjection;
						bool		EnableCollision = blockInfo.mJointData.mEnableCollision;
						bool		EnablePreprocessing = blockInfo.mJointData.mEnablePreprocessing;
						float		MassScaleSelf = blockInfo.mJointData.mMassScaleSelf;
						float		MassScaleConnected = blockInfo.mJointData.mMassScaleConnected;

						float		LinearLimit = blockInfo.mJointData.mLinearLimit;
						float		AngularXLowLimit = blockInfo.mJointData.mAngularXLowLimit;
						float		AngularXHighLimit = blockInfo.mJointData.mAngularXHighLimit;
						float		AngularYLimit = blockInfo.mJointData.mAngularYLimit;
						float		AngularZLimit = blockInfo.mJointData.mAngularZLimit;
						JointLimit	LinearLimitParm = blockInfo.mJointData.mLinearLimitParm;
						JointLimit	AngularXLimitParm = blockInfo.mJointData.mAngularXLimitParm;
						JointLimit	AngularYZLimitParm = blockInfo.mJointData.mAngularYZLimitParm;
						JointDrive	DriveX = blockInfo.mJointData.mDriveX;
						JointDrive	DriveY = blockInfo.mJointData.mDriveY;
						JointDrive	DriveZ = blockInfo.mJointData.mDriveZ;
						JointDrive	AngularDriveX = blockInfo.mJointData.mAngularDriveX;
						JointDrive	AngularDriveYZ = blockInfo.mJointData.mAngularDriveYZ;
						JointDrive	SlerpDrive = blockInfo.mJointData.mSlerpDrive;
						Vector3		TargetPosition = blockInfo.mJointData.mTargetPosition;
						Vector3		TargetVelocity = blockInfo.mJointData.mTargetVelocity;
						Vector3		TargetRotation = blockInfo.mJointData.mTargetRotation;
						Vector3		TargetAngularVelocity = blockInfo.mJointData.mTargetAngularVelocity;
						RDM			RotationDriveMode = blockInfo.mJointData.mRotationDriveMode;
						float		ProjectionDistance = blockInfo.mJointData.mProjectionDistance;
						float		ProjectionAngle = blockInfo.mJointData.mProjectionAngle;
						bool		LinearLimitScale = blockInfo.mJointData.mLinearLimitScale;
						JointMotion	MotionX = blockInfo.mJointData.mMotionX;
						JointMotion	MotionY = blockInfo.mJointData.mMotionY;
						JointMotion	MotionZ = blockInfo.mJointData.mMotionZ;
						JointMotion	MotionAX = blockInfo.mJointData.mMotionAX;
						JointMotion	MotionAY = blockInfo.mJointData.mMotionAY;
						JointMotion	MotionAZ = blockInfo.mJointData.mMotionAZ;

						String		ConnectedComName = blockInfo.mJointData.mConnectedComName;
						//String		SelfComName = blockInfo.mJointData.mSelfComName;

						if (jointInfo != nullptr)
						{
							jointInfo->SetAxisOffset(AxisOffset, joint);
							jointInfo->SetBreakForce(BreakForce, joint);
							jointInfo->SetBreakTorque(BreakTorque, joint);
							jointInfo->SetEnableProjection(EnableProjection, joint);
							jointInfo->SetEnableCollision(EnableCollision, joint);
							jointInfo->SetEnablePreprocessing(EnablePreprocessing, joint);
							jointInfo->SetMassScaleSelf(MassScaleSelf, joint);
							jointInfo->SetMassScaleConnected(MassScaleConnected, joint);
							if (!block.jointTarget.empty()) {
								String connectedRigidBodyName;
								for (const auto& targetBlock : blockData)
								{
									if (targetBlock.BoneName == block.jointTarget)
									{
										connectedRigidBodyName = targetBlock.rigidbodyName.toString();
										break;
									}
								}
								jointInfo->SetSelfComName(block.rigidbodyName.toString(), joint);
								jointInfo->SetConnectedComName(connectedRigidBodyName, joint);
							}
							else {
								jointInfo->SetSelfComName(block.rigidbodyName.toString(), joint);
								jointInfo->SetConnectedComName(ConnectedComName, joint);
							}

							jointInfo->SetLinearLimit(LinearLimit, joint);
							jointInfo->SetAngularXLowLimit(AngularXLowLimit, joint);
							jointInfo->SetAngularXHighLimit(AngularXHighLimit, joint);
							jointInfo->SetAngularYLimit(AngularYLimit, joint);
							jointInfo->SetAngularZLimit(AngularZLimit, joint);
							jointInfo->SetLinearLimitParm(LinearLimitParm, joint);
							jointInfo->SetAngularXLimitParm(AngularXLimitParm, joint);
							jointInfo->SetAngularYZLimitParm(AngularYZLimitParm, joint);
							jointInfo->SetDriveX(DriveX, joint);
							jointInfo->SetDriveY(DriveY, joint);
							jointInfo->SetDriveZ(DriveZ, joint);
							jointInfo->SetAngularDriveX(AngularDriveX, joint);
							jointInfo->SetAngularDriveYZ(AngularDriveYZ, joint);
							jointInfo->SetSlerpDrive(SlerpDrive, joint);
							jointInfo->SetTargetPosition(TargetPosition, joint);
							jointInfo->SetTargetVelocity(TargetVelocity, joint);
							jointInfo->SetTargetRotation(TargetRotation, joint);
							jointInfo->SetTargetAngularVelocity(TargetAngularVelocity, joint);
							jointInfo->SetRotationDriveMode(RotationDriveMode, joint);
							jointInfo->SetProjectionDistance(ProjectionDistance, joint);
							jointInfo->SetProjectionAngle(ProjectionAngle, joint);
							jointInfo->SetLinearLimitScale(LinearLimitScale, joint);
							jointInfo->SetMotionX(MotionX, joint);
							jointInfo->SetMotionY(MotionY, joint);
							jointInfo->SetMotionZ(MotionZ, joint);
							jointInfo->SetMotionAX(MotionAX, joint);
							jointInfo->SetMotionAY(MotionAY, joint);
							jointInfo->SetMotionAZ(MotionAZ, joint);
						}
						else
						{
							joint->SetAxisOffset(AxisOffset);
							joint->SetBreakForce(BreakForce);
							joint->SetBreakTorque(BreakTorque);
							joint->SetEnableProjection(EnableProjection);
							joint->SetEnableCollision(EnableCollision);
							joint->SetEnablePreprocessing(EnablePreprocessing);
							joint->SetMassScaleSelf(MassScaleSelf);
							joint->SetMassScaleConnected(MassScaleConnected);
							if (!block.jointTarget.empty()) {
								String connectedRigidBodyName;
								for (const auto& targetBlock : blockData)
								{
									if (targetBlock.BoneName == block.jointTarget)
									{
										connectedRigidBodyName = targetBlock.rigidbodyName.toString();
										break;
									}
								}
								joint->SetSelfRigidBody(block.rigidbodyName.toString());
								joint->SetConnectedRigidBody(connectedRigidBodyName);
							}
							else {
								joint->SetSelfRigidBody(block.rigidbodyName.toString());
								joint->SetConnectedRigidBody(ConnectedComName);
							}

							joint->SetLinearLimit(LinearLimit);
							joint->SetAngularXLowLimit(AngularXLowLimit);
							joint->SetAngularXHighLimit(AngularXHighLimit);
							joint->SetAngularYLimit(AngularYLimit);
							joint->SetAngularZLimit(AngularZLimit);
							joint->SetLinearLimitParm(LinearLimitParm);
							joint->SetAngularXLimitParm(AngularXLimitParm);
							joint->SetAngularYZLimitParm(AngularYZLimitParm);
							joint->SetDriveX(DriveX);
							joint->SetDriveY(DriveY);
							joint->SetDriveZ(DriveZ);
							joint->SetAngularDriveX(AngularDriveX);
							joint->SetAngularDriveYZ(AngularDriveYZ);
							joint->SetSlerpDrive(SlerpDrive);
							joint->SetTargetPosition(TargetPosition);
							joint->SetTargetVelocity(TargetVelocity);
							joint->SetTargetRotation(TargetRotation);
							joint->SetTargetAngularVelocity(TargetAngularVelocity);
							joint->SetRotationDriveMode(RotationDriveMode);
							joint->SetProjectionDistance(ProjectionDistance);
							joint->SetProjectionAngle(ProjectionAngle);
							joint->SetLinearLimitScale(LinearLimitScale);
							joint->SetMotionX(MotionX);
							joint->SetMotionY(MotionY);
							joint->SetMotionZ(MotionZ);
							joint->SetMotionAX(MotionAX);
							joint->SetMotionAY(MotionAY);
							joint->SetMotionAZ(MotionAZ);
						}
						ReadPosData(blockInfo.mJointData, joint);
						block.jointName = joint->getComName();
					}
				}
				// muscles desc	
				block.group = blockInfo.mGroup;
				block.mapping = blockInfo.mMapping;
				block.pin = blockInfo.mPin;
				block.weight = blockInfo.mWeight;
				block.damper = blockInfo.mDamper;
			}
		}

		// init ragdoll
		ragdoll->loadDataFromInfo(&blockData);
		if (!runtime)
			ragdoll->CopyMuscleToInfo();
		return true;
	}

	RoleIKComponent* RagdollTools::GetRoleComIK(RoleComponent* pRoleCom)
	{
		if (pRoleCom == nullptr)
			return nullptr;
		RoleObject* pRoleObj = pRoleCom->GetRole();
		if (pRoleObj == nullptr)
			return nullptr;
		return pRoleObj->getRoleIKComponent();
	}
	
}
