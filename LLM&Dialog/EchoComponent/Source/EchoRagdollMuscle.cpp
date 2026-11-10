//-----------------------------------------------------------------------------
// File:  EchoRagdollMuscle.cpp
//
// Author: LangGuangyue
//
// Date:   2022-9-28
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include "EchoComponentStableHeaders.h"
#include "EchoRagdollMuscle.h"
#include "IK/EchoIKCommon.h"
#include "EchoRagdollTools.h"
#include "EchoPhysXScene.h"
#include "EchoPhysXSystem.h"
#include "EchoEngine.h"
#include "EchoRagdollComponent.h"
#include "EchoRagdollBehaviorComponentBase.h"

#define DBV2FV(dbVec3) ((Vector3)dbVec3)

namespace Echo
{


	RagdollMuscle::RagdollMuscle()
	{
		attribute = new MuscleAttribute();
	}
	RagdollMuscle::~RagdollMuscle()
	{
		SAFE_DELETE(attribute);
	}
	//void RagdollMuscle::MuscleWriteJSNode(cJSON * jsonNode)
	//{
	//	WriteJSNode(jsonNode, "attribute.gruop", (int)attribute->group);
	//	WriteJSNode(jsonNode, "attribute.mappingWeight", attribute->mappingWeight);
	//	WriteJSNode(jsonNode, "attribute.pinWeight", attribute->pinWeight);
	//	WriteJSNode(jsonNode, "attribute.muscleWeight", attribute->muscleWeight);
	//	WriteJSNode(jsonNode, "attribute.muscleDamper", attribute->muscleDamper);
	//	WriteJSNode(jsonNode, "attribute.mapPosition", attribute->mapPosition);
	//}
	//void RagdollMuscle::MuscleReadJSNode(cJSON * jsonNode)
	//{
	//	
	//}
	RagdollMuscle::MuscleAttribute::MuscleAttribute()
	{
		mappingWeight = 1.0f;
		pinWeight = 1.0f;
		muscleWeight = 1.0f;
		muscleDamper = 1.0f;
	}
	RagdollMuscle::MuscleAttribute::MuscleAttribute(float _pinWeight, float _muscleWeight, float _mappingWeight, float _muscleDamper, MuscleGroup _group)
	{
		mappingWeight = _mappingWeight;
		pinWeight = _pinWeight;
		muscleWeight = _muscleWeight;
		muscleDamper = _muscleDamper;
		group = _group;
	}
	void RagdollMuscle::MuscleAttribute::Clamp()
	{
		mappingWeight = Math::Clamp(mappingWeight, 0.0f, 1.0f);
		pinWeight = Math::Clamp(pinWeight, 0.0f, 1.0f);
		muscleWeight = Math::Clamp(muscleWeight, 0.0f, 1.0f);
		muscleDamper = Math::Clamp(muscleDamper, 0.0f, 1.0f);
	}
	RagdollMuscle::MuscleState RagdollMuscle::MuscleState::Default()
	{
		MuscleState state;

		state.mappingWeightMlp = 1.0f;
		state.pinWeightMlp = 1.0f;
		state.muscleWeightMlp = 1.0f;
		state.muscleDamperMlp = 1.0f;
		state.muscleDamperAdd = 0.0f;
		state.maxForceMlp = 1.0f;
		state.immunity = 0.0f;
		state.impulseMlp = 1.0f;
		state.isDisconnected = false;

		return state;
	}
	void RagdollMuscle::MuscleState::Clamp()
	{
		mappingWeightMlp = Math::Clamp(mappingWeightMlp, 0.0f, 1.0f);
		pinWeightMlp = Math::Clamp(pinWeightMlp, 0.0f, 1.0f);
		muscleWeightMlp = Math::Clamp(muscleWeightMlp, 0.0f, muscleWeightMlp);
		immunity = Math::Clamp(immunity, 0.0f, 1.0f);
		impulseMlp = Math::Max(impulseMlp, 0.0f);
	}
	RagdollMuscle::MuscleTargetChild::MuscleTargetChild(ActorComponent * t)
	{
		this->t = t;

		defaultLocalPosition = t->getPosition();
		defaultLocalRotation = t->getRotation();
	}
	void RagdollMuscle::MuscleTargetChild::Map()
	{
		ComponentNode* comNode = dynamic_cast<ComponentNode*>(t);
		comNode->setPosition(defaultLocalPosition);
		comNode->setRotation(defaultLocalRotation);
	}
	bool RagdollMuscle::IsValid()
	{
		if (joint == nullptr || target == nullptr)
			return false;
		return true;
	}
	void RagdollMuscle::Rebuild()
	{
		state = MuscleState::Default();
	}
	void RagdollMuscle::Initiate(const std::vector<RagdollMuscle*>& colleagues, const RagdollComponent* pComp)
	{
		initiated = false;
		if (!IsValid())
		{
			assert(false && "muscle is valid !!!");
			return;
		}

		name = joint->getComName();

		state = MuscleState::Default();

		if (!joint->GetConnectedRigidBody().empty())
		{
			for (int i = 0; i < colleagues.size(); i++)
			{
				if (colleagues[i]->joint->GetSelfRigidBody() == joint->GetConnectedRigidBody())
				{
					connectedBodyTarget = colleagues[i]->target;
					break;
				}
			}
		}
		
		
		//Transform Close
		//transform = joint->getParentNodeCom().get();
		//transformComNode = joint->getParentComponentNode();
		ActorComponent* transform = GetTransform();
		String selfrigidbodyName = joint->GetSelfRigidBody();
		RigidBodyComponentPtr selfrigidbody = nullptr;
		if (transform)
		{
		//	selfrigidbody = transform->getActor()->getComponent(selfrigidbodyName);
			selfrigidbody = joint->GetSelfRigidBodyComp();
		}
		if (selfrigidbody != NULL)
			rigidbody = selfrigidbody;

		if (rigidbody == nullptr)
		{
			std::string errorInfo = std::string("-error-\tEcho::RigdollComponent::RagdollMuscle::Initiate [")
							+ "Block("+ transform->getComName() + ") "
							+ "not find RigidBody("+ selfrigidbodyName +")"
							+ "]!";
			LogManager::instance()->logMessage(errorInfo);
			return;
		}

		if (pComp != nullptr)
			SetMuscleLimit(pComp->GetMaxAngularVelocity(), pComp->GetMaxLinearVelocity(), pComp->GetRagdollDensity());

		InitiateAdditionalPin();

		SetKinematic(false);

		UpdateColliders();

		targetParent = static_cast<BoneNode*>(connectedBodyTarget != nullptr ? connectedBodyTarget : target->getParent());

		rebuildConnectedBody = joint->getActor()->getComponent(joint->GetConnectedRigidBody());
		rebuildTargetParent = target->getParent();
		//Transform Close
		rebuildParent = joint->getParentNodeCom()->getParentNodeCom();
		rebuildPosition = transform->getWorldPosition();
		rebuildRotation = transform->getWorldRotation();
		rebuildTargetPosition = DBV2FV(target->getOriginalWorldPosition()/*_getDerivedPosition()*/);
		rebuildTargetRotation = target->getOriginalWorldOrientation()/*_getDerivedOrientation()*/;
		rebuildAngularXMotion = joint->GetMotion(JointAxis::XAngular);
		rebuildAngularYMotion = joint->GetMotion(JointAxis::YAngular);
		rebuildAngularZMotion = joint->GetMotion(JointAxis::ZAngular);

		defaultLocalRotation = localRotation();

		// Joint space
		Vector3 forward = Vector3::VCross(joint->GetAxis(), joint->GetSecondaryAxis()).GetNormalized();
		Vector3 up = Vector3::VCross(forward, joint->GetAxis()).GetNormalized();

		if (forward == up)
		{
			assert(false && "forward equal to up ?");
			//Debug.LogError("Joint " + joint->name + " secondaryAxis is in the exact same direction as it's axis. Please make sure they are not aligned.");
			return;
		}

		rotationRelativeToTarget = Quaternion::QInverse(target->getOriginalWorldOrientation()/*_getDerivedOrientation()*/) * transform->getWorldRotation();

		defaultTargetPosRelToMuscle = RagdollTools::InverseActorcomponentPoint(transform, DBV2FV(target->getOriginalWorldPosition()/*_getDerivedPosition()*/));
		defaultTargetRotRelToMuscle = Quaternion::QInverse(transform->getWorldRotation()) * target->getOriginalWorldOrientation()/*_getDerivedOrientation()*/;
		defaultTargetRotRelToMuscleInverse = Quaternion::QInverse(defaultTargetRotRelToMuscle);
		defaultMuscleRotRelToTarget = Quaternion::QFromToRotation(target->getOriginalWorldOrientation()/*_getDerivedOrientation()*/, transform->getWorldRotation());

		Quaternion toJointSpace = Quaternion::LookRotation(forward, up);
		toJointSpaceInverse = Quaternion::QInverse(toJointSpace);
		toJointSpaceDefault = defaultLocalRotation * toJointSpace;

		toParentSpace = Quaternion::QInverse(targetParentRotation(true)) * parentRotation();

		localRotationConvert = Quaternion::QInverse(targetLocalRotation(true)) * localRotation();

		// Anchoring
		if (rebuildConnectedBody != nullptr)
		{
			//Transform Close
			connectedBodyTransform = rebuildConnectedBody->getParentNodeCom();

			directTargetParent = target->getParent() == connectedBodyTarget;
		}

		// Default angular motions and limits
		angularXMotionDefault = joint->GetMotion(JointAxis::XAngular);
		angularYMotionDefault = joint->GetMotion(JointAxis::YAngular);
		angularZMotionDefault = joint->GetMotion(JointAxis::ZAngular);

		// Mapping
		//if (joint->connectedBody == nullptr) props->mapPosition = true;
		targetRotationRelative = Quaternion::QInverse(transform->getWorldRotation()) *target->getOriginalWorldOrientation()/*_getDerivedOrientation()*/;

		// Resetting
		if (rebuildConnectedBody == nullptr)
		{
			defaultPosition = transform->getPosition();
			defaultRotation = transform->getRotation();
		}
		else
		{
			//Transform Close
			defaultPosition = RagdollTools::InverseActorcomponentPoint(connectedBodyTransform.get(), transform->getWorldPosition());
			defaultRotation = Quaternion::QInverse(rebuildConnectedBody->getParentNodeCom()->getWorldRotation()) * transform->getWorldRotation();
		}

		initPosition = transform->getPosition();
		initRotation = transform->getRotation();

		// Fix target Transforms
		if (targetParent)
		{
			WorldNode targetTransform;
			targetTransform.setPosition(targetParent->getOriginalDerivedPosition());
			targetTransform.setOrientation(targetParent->getOriginalDerivedOrientation());
			targetTransform.setScale(targetParent->_getDerivedScale());

			defaultTargetLocalPosition = targetTransform.convertWorldToLocalPosition(target->getOriginalDerivedPosition());
			defaultTargetLocalRotation = targetTransform.convertWorldToLocalOrientation(target->getOriginalDerivedOrientation());
		}
		else
		{
			defaultTargetLocalPosition = target->getOriginalWorldPosition()/*getPosition()*/;
			defaultTargetLocalRotation = target->getOriginalWorldOrientation()/*getOrientation()*/;
		}

		// Set necessary joint params
		joint->SetRotationDriveMode(RDM::Slerp);

		//joint->configuredInWorldSpace = false;

		//joint->projectionMode = JointProjectionMode::ProjectionNone; //Other projection modes will cause sliding

		//if (joint->GetAnchor() != Vector3::ZERO)
		//{
		//	assert(false && "muscle joint anchor is not zero !!!");
		//	return;
		//}

		targetAnimatedPosition = DBV2FV(target->getOriginalWorldPosition()/*_getDerivedPosition()*/);
		targetAnimatedCenterOfMass = rigidbody->GetWorldCenterofMass();
		targetAnimatedWorldRotation = target->getOriginalWorldOrientation()/*_getDerivedOrientation()*/;
		targetAnimatedRotation = targetLocalRotation(true) * localRotationConvert;

		Read();
		lastReadTime = EchoIK::Time::time; 
		lastWriteTime = EchoIK::Time::time;
		lastMappedPosition = DBV2FV(target->getOriginalWorldPosition()/*_getDerivedPosition()*/);
		lastMappedRotation = target->getOriginalWorldOrientation()/*_getDerivedOrientation()*/;

		//targetChildren = new TargetChild[props->animatedTargetChildren.Length];
		targetChildren.resize(attribute->animatedTargetChildren.size());
		for (int i = 0; i < targetChildren.size(); i++)
		{
			targetChildren[i] = new MuscleTargetChild(attribute->animatedTargetChildren[i]);
		}

		initiated = true;
	}
	void RagdollMuscle::InitiateAdditionalPin()
	{
		if (additionalPin != nullptr)
		{
			additionalRigidbody = additionalPin->getActor()->getComponent(additionalPin->GetConnectedRigidBody());
			additionalPinTargetAnimatedCenterOfMass = additionalRigidbody->GetPhysics()->GetWorldCenterOfMass();
			additionalRigidbody->SetInertiaTensor(RagdollTools::CalculateInertiaTensorCuboid(Vector3::UNIT_SCALE * 0.1f, additionalRigidbody->GetMass()));
		}
	}
	void RagdollMuscle::UpdateColliders()
	{
		AddColliders(joint.get(), true);

		disabledColliders.resize(colliders.size());
		for (uint16 i = 0;i < disabledColliders.size();++i)
		{
			disabledColliders[i] = false;
		}
	}
	void RagdollMuscle::DisableColliders()
	{
	}
	void RagdollMuscle::EnableColliders()
	{
	}
	void RagdollMuscle::AddColliders(ConfigurableJointComponent * joint, bool includeMeshColliders)
	{
		/*ColliderComponent* collider = joint->collider;
		int cCount = 0;
		if (!collider->isTrigger)
		{
			cCount++;
		}

		if (cCount == 0) return;

		colliders.push_back(collider);*/
	}
	void RagdollMuscle::AddCompoundColliders(ConfigurableJointComponent * joint)
	{
	}
	void RagdollMuscle::IgnoreInternalCollisions(RagdollMuscle * m)
	{
		if (m == this) 
			return;
		if (state.isDisconnected || m->state.isDisconnected) 
			return;
	}
	void RagdollMuscle::ResetInternalCollisions(RagdollMuscle * m, bool useInternalCollisionIgnores)
	{
		if (m == this) 
			return;
	}
	bool RagdollMuscle::ForceIgnore(RagdollMuscle * otherMuscle)
	{
		return false;
	}
	void RagdollMuscle::IgnoreAngularLimits(bool ignore)
	{
		if (!initiated) 
			return;
		// Do not use state.isDisconnected here, or severed muscle will not be freed when exploded later

		joint->SetMotion(JointAxis::XAngular, ignore ? JointMotion::FREE : angularXMotionDefault);
		joint->SetMotion(JointAxis::YAngular, ignore ? JointMotion::FREE : angularYMotionDefault);
		joint->SetMotion(JointAxis::ZAngular, ignore ? JointMotion::FREE : angularZMotionDefault);
	}
	void RagdollMuscle::ResetTargetLocalPosition()
	{
		target->setPosition(defaultTargetLocalPosition);
	}
	void RagdollMuscle::FixTargetTransforms()
	{
		if (!initiated) return;

		target->setPosition(defaultTargetLocalPosition);
		target->setOrientation(defaultTargetLocalRotation);
	}
	void RagdollMuscle::Reset()
	{
		if (!initiated) return;
		if (joint == nullptr) return;
		if (state.isDisconnected) return;

		auto* transformComNode = GetTransformNode();
		if (transformComNode)
		{
			if (rebuildConnectedBody == nullptr)
			{
				transformComNode->setPosition(defaultPosition);
				transformComNode->setRotation(defaultRotation);
			}
			else
			{
				if (connectedBodyTransform != nullptr)
				{
					transformComNode->setWorldPosition(RagdollTools::ActComponentComponentPoint(connectedBodyTransform.get(), defaultPosition));
					transformComNode->setWorldRotation(connectedBodyTransform->getWorldRotation() * defaultRotation);
				}
			}
		}
		lastRotationDamper = -1.0f;
	}
	void RagdollMuscle::MoveToTarget()
	{
		if (!initiated || state.isDisconnected || rigidbody == nullptr)
			return;

		auto* transformComNode = GetTransformNode();

		if(transformComNode)
			transformComNode->setWorldTransform({ DBV2FV(target->_getDerivedPosition()) , target->_getDerivedOrientation() * rotationRelativeToTarget });

		positionOffset = Vector3::ZERO;

		if (additionalPin != nullptr)
		{
			additionalPin->getParentComponentNode()->setWorldPosition(additionalPinTarget->getWorldPosition());
		}
	}
	void RagdollMuscle::SetKinematic(bool to)
	{
		if (rigidbody == nullptr)
			return;
		if (to) 
			rigidbody->SetCollisionDetection((uint8)CollisionDetectionMode::Discrete);

		rigidbody->SetIsKinematic(to);
		//rigidbody->GetPhysics()->ToggleKinematic(to);
		if (additionalPin != nullptr)
			additionalRigidbody->SetIsKinematic(to);
	}
	void RagdollMuscle::Read()
	{
		if (state.isDisconnected) 
			return;
		if (rigidbody == nullptr || joint == nullptr)
			return;

		float readDeltaTime = EchoIK::Time::time - lastReadTime;
		lastReadTime = EchoIK::Time::time;

		Vector3 tAM = Vector3::TransformPointUnscaled(DBV2FV(target->_getDerivedPosition()), target->_getDerivedOrientation(), defaultTargetRotRelToMuscleInverse * rigidbody->GetLocalCenterofMass()); // Center of mass is unscaled, so can't use Transform.TransformPoint() here

		if (readDeltaTime > 0.0f && lastReadTime != 0)
			targetVelocity = (tAM - targetAnimatedCenterOfMass) / readDeltaTime;

		targetAnimatedCenterOfMass = tAM;
		targetAnimatedPosition = DBV2FV(target->_getDerivedPosition());
		targetAnimatedWorldRotation = target->_getDerivedOrientation();

		if (additionalPin != nullptr)
		{
			Vector3 aTAM = Vector3::TransformPointUnscaled(additionalPinTarget->getWorldPosition(), additionalPinTarget->getWorldRotation(), additionalRigidbody->GetPhysics()->GetLocalCenterOfMass()); // Center of mass is unscaled, so can't use Transform.TransformPoint() here

			if (readDeltaTime > 0.0f && lastReadTime != 0)
				additionalTargetVelocity = (aTAM - additionalPinTargetAnimatedCenterOfMass) / readDeltaTime;

			additionalPinTargetAnimatedCenterOfMass = aTAM;
		}

		if (joint->GetConnectedRigidBodyComp() != nullptr)
		{
			targetAnimatedRotation = targetLocalRotation() * localRotationConvert;
		}
	}
	void RagdollMuscle::ClearVelocities()
	{
		targetVelocity = Vector3::ZERO;
		//targetAngularVelocity = Vector3.zero;
		mappedVelocity = Vector3::ZERO;
		mappedAngularVelocity = Vector3::ZERO;
		additionalTargetVelocity = Vector3::ZERO;

		targetAnimatedCenterOfMass = Vector3::TransformPointUnscaled(DBV2FV(target->_getDerivedPosition()), target->_getDerivedOrientation(), rigidbody->GetLocalCenterofMass());
		targetAnimatedPosition = DBV2FV(target->_getDerivedPosition());
		targetAnimatedWorldRotation = target->_getDerivedOrientation();
		lastMappedPosition = DBV2FV(target->_getDerivedPosition());
		lastMappedRotation = target->_getDerivedOrientation();
		if (additionalPin != nullptr)
		{
			additionalPinTargetAnimatedCenterOfMass = Vector3::TransformPointUnscaled(additionalPinTarget->getWorldPosition(),additionalPinTarget->getWorldRotation(), additionalRigidbody->GetLocalCenterofMass());
			//additionalTargetAnimatedPosition = additionalPinTarget.position;
		}
	}
	void RagdollMuscle::UpdateAnchor(bool supportTranslationAnimation)
	{
		if (state.isDisconnected) 
			return;
		if (rebuildConnectedBody == nullptr || connectedBodyTarget == nullptr) 
			return;
		if (directTargetParent && !supportTranslationAnimation) 
			return;
		if (joint == nullptr)
			return;

		//if (props->mapPosition) target->getPosition() = targetLocalPosition
		Vector3 anchorUnscaled = InverseTransformPointUnscaled(DBV2FV(connectedBodyTarget->_getDerivedPosition()), connectedBodyTarget->_getDerivedOrientation() * toParentSpace, DBV2FV(target->_getDerivedPosition()));
		//Vector3 anchorUnscaled = joint->GetAnchorConnected();
		float uniformScaleF = 1.0f / connectedBodyTransform->getWorldScale().x; //connectedBodyTransform->lossyScale.x;

// 		joint->SetAnchorConnected(anchorUnscaled * uniformScaleF);

		Transform trans;
		joint->GetLocalPose1(trans);
		trans.translation = anchorUnscaled * uniformScaleF;
		joint->SetLocalPose1(trans);
	}
	void RagdollMuscle::Update(float pinWeightMaster, float muscleWeightMaster, float muscleSpring, float muscleDamper, float pinPow, float pinDistanceFalloff, bool rotationTargetChanged, bool angularPinning, float deltaTime)
	{
		if (!initiated)
			return;

		state.velocity = rigidbody->GetVelocity();
		state.angularVelocity = rigidbody->GetAngularVelocity();

		if (state.isDisconnected)
		{
			state.pinWeightMlp = 0.0f;
			state.muscleWeightMlp = 0.0f;
			state.muscleDamperAdd = 0.0f;
			state.muscleDamperMlp = 0.0f;
			state.mappingWeightMlp = 0.0f;
			state.maxForceMlp = 0.0f;
			state.immunity = 0.0f;
			state.impulseMlp = 1.0f;
		}

		attribute->Clamp();
		state.Clamp();

		Pin(pinWeightMaster, pinPow, pinDistanceFalloff, angularPinning, deltaTime);
		if (rotationTargetChanged) 
			MuscleRotation(muscleWeightMaster, muscleSpring, muscleDamper);
	}

	void RagdollMuscle::UpdateTransfrom()
	{
		//不能这么用
		/*if (rigidbody->GetPhysics())
		{
			transform->setWorldPosition(rigidbody->GetPhysics()->GetPosition());
			transform->setWorldRotation(rigidbody->GetPhysics()->GetRotation());
		}*/
	}

	//void RagdollMuscle::Start()
	//{
	//	targetAnimatedCenterOfMass = rigidbody->GetWorldCenterofMass();
	//	rigidbody->SetIsKinematic(false);
	//}

	//void RagdollMuscle::Stop()
	//{
	//	transform->setPosition(initPosition);
	//	transform->setRotation(initRotation);
	//	rigidbody->SetIsKinematic(true);
	//	Transform trans(transform->getWorldPosition(), transform->getWorldRotation());
	//	//rigidbody->GetPhysics()->SetTransform(trans);
	//	breakFlag = noBreak;
	//	joint->SetIsUsePhysics(true);
	//	
	//}
	void RagdollMuscle::StoreTargetMappedPosition()
	{
		targetMappedPosition = DBV2FV(target->_getDerivedPosition());
	}
	void RagdollMuscle::StoreTargetMappedRotation()
	{
		targetMappedRotation = target->_getDerivedOrientation();
	}
	void RagdollMuscle::Map(float mappingWeightMaster)
	{
		float w = attribute->mappingWeight * mappingWeightMaster * state.mappingWeightMlp;
		if (w <= 0.0f) 
			return;
		auto* transform = GetTransform();
		Vector3 position = Vector3::ZERO;
		Quaternion rotation = Quaternion::IDENTITY;
		if (transform)
		{
			// rigidbody->getWorldPosition() does not work with interpolation
			position = transform->getWorldPosition();
			rotation = transform->getWorldRotation();
		}

		
		
		if (w >= 1.0f)
		{
			target->_setDerivedOrientation(rotation * targetRotationRelative);
		
			if (connectedBodyTransform != nullptr)
			{
				// Mapping in local space of the parent
				Vector3 relativePosition = RagdollTools::InverseActorcomponentPoint(connectedBodyTransform.get(), position);
				target->_setDerivedPosition(connectedBodyTarget->TransformPoint(relativePosition));
			}
			else
			{
				target->_setDerivedPosition(position);
			}
		
		}
		else
		{
			target->_setDerivedOrientation(Quaternion::QLerp(target->_getDerivedOrientation(), rotation * targetRotationRelative, w));

			if (connectedBodyTransform != nullptr)
			{
				// Mapping in local space of the parent
				Vector3 relativePosition = RagdollTools::InverseActorcomponentPoint(connectedBodyTransform.get(), position);
				target->_setDerivedPosition(Vector3::VLerp(DBV2FV(target->_getDerivedPosition()), connectedBodyTarget->TransformPoint(relativePosition), w));
			}
			else
			{
				target->_setDerivedPosition(Vector3::VLerp(DBV2FV(target->_getDerivedPosition()), position, w));
			}
		}
	}
	void RagdollMuscle::CalculateMappedVelocity()
	{
		float writeDeltaTime = EchoIK::Time::time - lastWriteTime;

		if (writeDeltaTime > 0.0f)
		{
			mappedVelocity = (DBV2FV(target->_getDerivedPosition()) - lastMappedPosition) / writeDeltaTime;

			mappedAngularVelocity = RagdollTools::GetAngularVelocity(lastMappedRotation, target->_getDerivedOrientation(), writeDeltaTime);

			lastWriteTime = EchoIK::Time::time;
		}

		lastMappedPosition = DBV2FV(target->_getDerivedPosition());
		lastMappedRotation = target->_getDerivedOrientation();
	}
	void RagdollMuscle::MapDisconnected()
	{
		if (!state.isDisconnected) return;
		if (isPropMuscle) return;
		//if (!transform.gameObject.activeInHierarchy) return;
		auto* transform = GetTransform();
		if (transform)
		{
			target->_setDerivedPosition(transform->convertLocalToWorldPosition(defaultTargetPosRelToMuscle)); // TODO Make private
			target->_setDerivedOrientation(transform->getWorldRotation() * defaultTargetRotRelToMuscle);
		}


		for(auto& c : targetChildren) c->Map();
	}

	void RagdollMuscle::Pin(float pinWeightMaster, float pinPow, float pinDistanceFalloff, bool angularPinning, float deltaTime)
	{
		positionOffset = targetAnimatedCenterOfMass - rigidbody->GetWorldCenterofMass();

		if (IsNAN(positionOffset.x))
			positionOffset = Vector3::ZERO;

		float w = pinWeightMaster * attribute->pinWeight * state.pinWeightMlp;

		if (w <= 0.0f ||(breakItem == (breakFlag & breakItem)) ) return;

		w = Math::Pow(w, pinPow);

		Pin(rigidbody.get(), positionOffset, targetVelocity, w, pinDistanceFalloff, deltaTime);

		if (angularPinning)
		{
			Vector3 torque = RagdollTools::GetAngularAcceleration(rigidbody->getWorldRotation(), defaultMuscleRotRelToTarget * targetAnimatedWorldRotation);

			torque -= rigidbody->GetAngularVelocity();
			torque *= w;
			rigidbody->AddTorque(torque, PhysXRigidDynamic::ForceMode::eVELOCITY_CHANGE);
		}
	}

	void RagdollMuscle::Pin(RigidBodyComponent * r, Vector3 posOffset, Vector3 targetVel, float w, float pinDistanceFalloff, float deltaTime)
	{
		Vector3 p = posOffset;
		if (deltaTime > 0.0f) p /= deltaTime;

		Vector3 force = -r->GetVelocity() + targetVel + p;
		if (r->GetUseGravity()) 
			force -= r->GetPhysicsManager()->GetGravity() * deltaTime;
		force *= w;
		if (pinDistanceFalloff > 0.0f)
			force /= 1.0f + posOffset.squaredLength() * pinDistanceFalloff;

		r->AddForce(force, PhysXRigidDynamic::ForceMode::eVELOCITY_CHANGE);
	}

	void RagdollMuscle::MuscleRotation(float muscleWeightMaster, float muscleSpring, float muscleDamper)
	{
		float w = muscleWeightMaster * attribute->muscleWeight * muscleSpring * state.muscleWeightMlp * 10.0f;
		// If no connection point, don't rotate;
		if (joint->GetConnectedRigidBodyComp() == nullptr || (animeBlock == (breakFlag & animeBlock)))
			w = 0.0f;
		else if (w > 0.0f)
		{
			Quaternion quat = LocalToJointSpace(targetAnimatedRotation);
			
			joint->SetTargetRotationQ(quat);
			
		}


		float d = (attribute->muscleDamper * muscleDamper * state.muscleDamperMlp) + state.muscleDamperAdd;

		if (w == lastJointDriveRotationWeight && d == lastRotationDamper) return;
		lastJointDriveRotationWeight = w;

		lastRotationDamper = d;
		slerpDrive.spring = w;
		slerpDrive.maximumForce = Math::Max(w, d) * state.maxForceMlp;
		slerpDrive.damper = d;
		
		joint->SetSlerpDrive(slerpDrive);
	}

	// not use 
	void RagdollMuscle::SetMuscleRotation(float muscleWeightMaster, float muscleSpring, float muscleDamper)
	{
		float w = muscleWeightMaster * attribute->muscleWeight * muscleSpring * 10.0f;

		// If no connection point, don't rotate;
		if (rebuildConnectedBody == nullptr) 
			w = 0.0f;
		else if (w > 0.0f) 
			joint->SetTargetRotationQ(LocalToJointSpace(targetAnimatedRotation));

		float d = (attribute->muscleDamper * muscleDamper);

		slerpDrive.spring = w;
		slerpDrive.maximumForce = Math::Max(w, d);
		slerpDrive.damper = d;

		joint->SetSlerpDrive(slerpDrive);
	}

	void RagdollMuscle::SetMuscleLimit(float maxAngular, float maxLinear, float density)
	{
		if (rigidbody.expired())
			return;

		rigidbody->SetDensity(density);

		rigidbody->SetMaxAngularVelocity(maxAngular);
		rigidbody->SetMaxLinearVelocity(maxLinear);
		rigidbody->SetMaxContactImpulse(maxLinear * rigidbody->GetMass());
	}

	Quaternion RagdollMuscle::localRotation()
	{
		return Quaternion::QInverse(parentRotation()) * GetTransform()->getWorldRotation();
	}

	Quaternion RagdollMuscle::parentRotation()
	{
		if (rebuildConnectedBody != nullptr)
			return rebuildConnectedBody->getWorldRotation();
		auto* transform = GetTransform();

		if (!transform || transform->getParentNodeCom() == nullptr)
			return Quaternion::IDENTITY;
		return transform->getParentNodeCom()->getWorldRotation();
	}

	Quaternion RagdollMuscle::targetParentRotation(bool useOriginalWorldTransform /*= false*/)
	{
		if (targetParent == nullptr) 
			return Quaternion::IDENTITY;
		return useOriginalWorldTransform ? targetParent->getOriginalWorldOrientation() : targetParent->_getDerivedOrientation();
	}

	Quaternion RagdollMuscle::targetLocalRotation(bool useOriginalWorldTransform /*= false*/)
	{
		return useOriginalWorldTransform ? 
			Quaternion::QInverse(targetParentRotation(true) * toParentSpace) * target->getOriginalWorldOrientation() 
			:
			Quaternion::QInverse(targetParentRotation() * toParentSpace) * target->_getDerivedOrientation();
	}

	Quaternion RagdollMuscle::LocalToJointSpace(Quaternion localRotation)
	{
		return toJointSpaceInverse * Quaternion::QInverse(localRotation) * toJointSpaceDefault;
	}

	Vector3 RagdollMuscle::InverseTransformPointUnscaled(const Vector3& position, const Quaternion& rotation, const Vector3& point)
	{
		return Quaternion::QInverse(rotation) * (point - position);
	}

	void RagdollMuscle::RoleMove(const Vector3& lastRolePosition, const Quaternion& lastRoleRotationInv, const Vector3& rolePosition, const Quaternion& roleRotation, bool angularPinning) {
		targetAnimatedCenterOfMass = roleRotation * (lastRoleRotationInv * (targetAnimatedCenterOfMass - lastRolePosition)) + rolePosition;
		if (angularPinning) {
			targetAnimatedWorldRotation = roleRotation * (lastRoleRotationInv * targetAnimatedWorldRotation);
		}
	}

	ActorComponent* RagdollMuscle::GetTransform()
	{
		if (joint)
			return joint->getParentNodeCom().get();
		return nullptr;
	}

	ComponentNode* RagdollMuscle::GetTransformNode()
	{
		if (joint)
			return joint->getParentComponentNode();
		return nullptr;
	}

	void _RagdollMuscle_JointComponentListener::OnJointBreak()
	{
		{
			if (pRagdoll != nullptr)
				((RagdollComponent*)pRagdoll.get())->RemoveMuscleRecursive(((RagdollComponent*)pRagdoll.get())->muscles[muscleIndex]->joint.get(), true, true);
		}
	}
	RagdollMuscleColliderBroadcaster::RagdollMuscleColliderBroadcaster(RagdollComponent* master,int index)
	{
		pRagdoll = master;
		muscleIndex = index;
	}

	void RagdollMuscleColliderBroadcaster::BroadcasterHit(Vector3 force, Vector3 position, float unPin)
	{
		MuscleHit hit{ muscleIndex , unPin , force , position };
		pRagdoll->BehavioursForeach([&hit](RagdollBehaviorComponentBase* b)
			{
				b->OnMuscleHit(hit);
			});
	}
	void RagdollMuscleColliderBroadcaster::OnCollisionEnter(const EchoCollisionParam& param)
	{
		if (!pRagdoll || pRagdoll->muscles.size() <= muscleIndex || pRagdoll->muscles[muscleIndex]->state.isDisconnected)
			return;
		MuscleCollision collision
		{
			muscleIndex, 
			param.collider,
			false
		};
		pRagdoll->BehavioursForeach([&collision](RagdollBehaviorComponentBase* b)
			{
				b->OnMuscleCollision(collision);
			});
	}
	void RagdollMuscleColliderBroadcaster::OnCollisionExit(const EchoCollisionParam& param)
	{
		if (!pRagdoll || pRagdoll->muscles.size() <= muscleIndex || pRagdoll->muscles[muscleIndex]->state.isDisconnected)
			return;
		MuscleCollision collision
		{
			muscleIndex,
			param.collider,
			false
		};
		pRagdoll->BehavioursForeach([&collision](RagdollBehaviorComponentBase* b)
			{
				b->OnMuscleCollisionExit(collision);
			});
	}
	void RagdollMuscleColliderBroadcaster::OnCollisionStay(const EchoCollisionParam& param)
	{
		if (!pRagdoll || pRagdoll->muscles.size() <= muscleIndex || pRagdoll->muscles[muscleIndex]->state.isDisconnected)
			return;
		MuscleCollision collision
		{
			muscleIndex,
			param.collider,
			true
		};
		pRagdoll->BehavioursForeach([&collision](RagdollBehaviorComponentBase* b)
			{
				b->OnMuscleCollision(collision);
			});
	}
}
