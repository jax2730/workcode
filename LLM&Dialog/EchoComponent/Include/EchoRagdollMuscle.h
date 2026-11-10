//-----------------------------------------------------------------------------
// File:  EchoRagdollMuscle.h
//
// Author: LangGuangyue
//
// Date:   2022-9-28
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef _EchoRagdollMuscle_H__
#define _EchoRagdollMuscle_H__

#include "EchoComponentPrerequisites.h"
//#include "Role/EchoRoleIKComponent.h"
#include "EchoPhysXAsset.h"
//#include "EchoPhysXJoint.h"
#include "EchoPhysicsManager.h"
#include "EchoJointComponent.h"
#include "EchoRigidBodyComponent.h"
#include "EchoConfigurableJointComponent.h"
#include "EchoBoxColliderComponent.h"
#include "EchoCapsuleColliderComponent.h"
#include "EchoBoneNode.h"

namespace Echo
{
	class _RagdollMuscle_JointComponentListener : public EchoJointCallBack
	{
	public: 
		int muscleIndex = 0;
		ActorComponentPtr pRagdoll;
		virtual void OnJointBreak() override;

	};

	class _EchoComponentExport RagdollMuscleColliderBroadcaster : public EchoColliderCallBack
	{
	public:
		RagdollMuscleColliderBroadcaster(RagdollComponent* master , int index);
	public:
		void BroadcasterHit(Vector3 force , Vector3 position , float unPin = 0);
	private:
		virtual void OnCollisionEnter(const EchoCollisionParam& param) override;
		virtual void OnCollisionExit(const EchoCollisionParam& param) override;
		virtual void OnCollisionStay(const EchoCollisionParam& param) override;
	private:
		RagdollComponent* pRagdoll = nullptr;
		int				  muscleIndex = -1;
	};

	class _EchoComponentExport RagdollMuscle
	{
	public:
		RagdollMuscle();
		virtual ~RagdollMuscle();

		enum MuscleGroup
		{
			Hips = 0,
			Spine = 1,
			Head = 2,
			Arm = 3,
			Hand = 4,
			Leg = 5,
			Foot = 6,
			Tail = 7,
			Prop = 8,
			Nomal = 9,

			Custom = 10,
			Custom_00 , Custom_01 , Custom_02 , Custom_03 , Custom_04 , Custom_05 , Custom_06 , Custom_07, 
			Custom_08 , Custom_09 , Custom_10 , Custom_11 , Custom_12 , Custom_13 , Custom_14 , Custom_15,
		};
		enum JointBreakFlag : uint16
		{
			noBreak	   = 0,
			breakItem  = 1 << 0 ,
			breakRoot  = (1 << 1) | breakItem,
			animeBlock = 1 << 2 | breakItem,
		};

		//ºöÂÔÅö×²
		class  InternalCollision
		{
			std::vector<ConfigurableJointComponent*>	muscles;
			std::vector<MuscleGroup>					groups;
		};

		class  MuscleAttribute
		{
		public:
			MuscleAttribute();
			MuscleAttribute(float pinWeight, float muscleWeight, float mappingWeight, float muscleDamper, MuscleGroup group);
			void				Clamp();
		public:
			MuscleGroup			group;
			float				mappingWeight = 1.0f;	//The weight (multiplier) of mapping this muscle's target to the muscle.
			float				pinWeight = 1.0f;		//The weight (multiplier) of pinning this muscle to it's target's position using a simple AddForce command.
			float				muscleWeight = 1.0f;	//The muscle strength (multiplier).
			float				muscleDamper = 1.0f;	//Multiplier of the positionDamper of the ConfigurableJoints' Slerp Drive.
			bool				mapPosition = true;	//If true, will map the target to the world space position of the muscle. Normally this should be true for only the root muscle (the hips).
			std::vector<ActorComponent*>		animatedTargetChildren;
			InternalCollision			internalCollisionMuscles;	// Defines which muscle or muscle groups internal collisions are always ignored with.
			//std::vector<WorldNode*>		animatedTargetChildren;
		};

		//The current state of a muscle. While the similar values in the MuscleAttribute should be defined by the user,
		//this multiplies or adds to them and is hidden and intended for being used by the Puppet Behaviours only.
		struct MuscleState
		{
			float				mappingWeightMlp = 1.0f;//Multiplies the mapping weight of the muscle.
			float				pinWeightMlp = 1.0f;	//Multiplies the pin weight of the muscle.
			float				muscleWeightMlp = 1.0f;	//Multiplies the muscle weight.
			float				maxForceMlp = 1.0f;		//Multiplies slerp drive's max force.
			float				muscleDamperMlp = 1.0f;	//Used by the behaviours to cancel out muscle damper so it could be set to a specific value by muscleDamperAdd.
			float				muscleDamperAdd = 0.0f;	//Adds to the muscle's damper value (can't multiply because it might be set to zero).
			float				immunity = 0.0f;		//Immunity reduces damage from collisions and hits in some Puppet Behaviours (BehaviourPuppet).
			float				impulseMlp = 1.0f;		//Larger impulse multiplier makes the muscle deal more damage to the muscle of other characters in some Puppet Behavours (BehaviourPuppet).
			Vector3				velocity;				//The velocity of the muscle in world space (filled in by the muscle itself).
			Vector3				angularVelocity;		//The angular velocity of the muscle (filled in by the muscle itself).
			bool				isDisconnected = false;	//If the muscle is currently disconnected.
			bool				resetFlag = false;		//If muscle joint orientation needs to be reset when reconnecting.

			static MuscleState Default();

			void Clamp();
		};

		// Used for blocking animation on child bones of disconnected muscle
		class MuscleTargetChild
		{
		public:
			//WorldNode*			t = nullptr;
			ActorComponent*			t = nullptr;
			Vector3				defaultLocalPosition;
			Quaternion			defaultLocalRotation = Quaternion::IDENTITY;

			MuscleTargetChild(ActorComponent* t);

			void		Map();
		};

	public:
		bool				IsValid();
		void				Rebuild();

		// Initiate this Muscle
		virtual void		Initiate(const std::vector<RagdollMuscle*>& colleagues, const RagdollComponent* pComp);

		void				InitiateAdditionalPin();

		// Regather (compound) colliders associated with this muscle.
		void				UpdateColliders();

		// Disables all colliders of this muscle.
		void				DisableColliders();

		// Enables all colliders of this muscle.
		void				EnableColliders();

		// Add all non-trigger colliders on a WorldNode* to an array of colliders
		void				AddColliders(ConfigurableJointComponent* joint, bool includeMeshColliders);

		// Recursively goes through all children of a WorldNode* to find the compound colliders until stopped by a Rigidbody*
		void				AddCompoundColliders(ConfigurableJointComponent* joint);

		// Ignores or collisions with all the colliders of this and another Muscle
		void				IgnoreInternalCollisions(RagdollMuscle* m);

		// Unignores collisions with all the colliders of this and another muscle (unless forced to ignore by props.internalCollisionIgnores)
		void				ResetInternalCollisions(RagdollMuscle* m, bool useInternalCollisionIgnores);

		// Are internal collision between these two muscle forced to ignore?
		bool				ForceIgnore(RagdollMuscle* otherMuscle);

		// Set joint angular motions to either free or to their default values
		void				IgnoreAngularLimits(bool ignore);

		// Moves targetl localPosition back to default (used for reconnecting limb in Dead state when animation is not overriding target localPosition)
		void				ResetTargetLocalPosition();

		// Reset target to its default localPosition and localRotation to protect from unanimated bone drifting
		void				FixTargetTransforms();

		// Reset the WorldNode* to the default state. This is necessary for activating/deactivating the ragdoll without messing it up
		void				Reset();

		// Moves and rotates the muscle to match it's target
		void				MoveToTarget();

		// Toggles muscle Rigidbody*.isKinematic
		void				SetKinematic(bool to);

		// Read the target
		void				Read();

		void				ClearVelocities();

		// Update Joint connected anchor
		void				UpdateAnchor(bool supportTranslationAnimation);

		// Update this Muscle
		virtual void		Update(float pinWeightMaster, float muscleWeightMaster, float muscleSpring, float muscleDamper, float pinPow, float pinDistanceFalloff, bool rotationTargetChanged, bool angularPinning, float deltaTime);


		void				UpdateTransfrom();

		void				StoreTargetMappedPosition();

		void				StoreTargetMappedRotation();

		// Map the target bone to this Rigidbody*
		void				Map(float mappingWeightMaster);

		// How fast the mapped target is moving? Will be used to set rigidbody velocities when puppet is killed. 
		// Rigidbody* velocities otherwise might be close to 0 when FixedUpdate called more than once per frame or velocity wrongully changing when mapping weights not 1.
		void				CalculateMappedVelocity();

		// Move the target to the muscle if the muscle is disconnected
		void				MapDisconnected();
		// Add force to the rigidbody to make it match the target position
		void				Pin(float pinWeightMaster, float pinPow, float pinDistanceFalloff, bool angularPinning, float deltaTime);

		void				Pin(RigidBodyComponent* r, Vector3 posOffset, Vector3 targetVel, float w, float pinDistanceFalloff, float deltaTime);

		// Apply Joint targetRotation to match the target rotation
		void				MuscleRotation(float muscleWeightMaster, float muscleSpring, float muscleDamper);

		// Apply Joint targetRotation to match the target rotation
		void				SetMuscleRotation(float muscleWeightMaster, float muscleSpring, float muscleDamper);

		void				SetMuscleLimit(float maxAngular, float maxLinear, float density);

		Quaternion			localRotation();
		Quaternion			parentRotation();
		Quaternion			targetParentRotation(bool useOriginalWorldTransform = false);
		// Get the rotation of the target
		Quaternion			targetLocalRotation(bool useOriginalWorldTransform = false);
		// Convert a local rotation to local joint space rotation
		Quaternion			LocalToJointSpace(Quaternion localRotation);

		void				RoleMove(const Vector3& lastRolePosition, const Quaternion& lastRoleRotation, const Vector3& rolePosition, const Quaternion& roleRotation, bool angularPinning);

		ActorComponent*		GetTransform();
		ComponentNode*		GetTransformNode();

		// Inversetransforms a point by the specified position and rotation
		static Vector3		InverseTransformPointUnscaled(const Vector3& position, const Quaternion& rotation, const Vector3& point);

		
	public:
		Name							name;
		ConfigurableJointComponentPtr	joint = nullptr;
		BoneNode*						target = nullptr;
		//ActorComponent*					target = nullptr;
		MuscleAttribute*				attribute = nullptr; 
		MuscleState						state = MuscleState::Default();

		std::vector<int>				parentIndexes;		//The indexes of all the parent muscle of this muscle.
		std::vector<int>				childIndexes;		//The indexes  of all the child muscle of this muscle.
		std::vector<bool>				childFlags;			//Flags for all the bones of the PuppetMaster indicating whether they are children of this muscle or not.
		std::vector<int>				kinshipDegrees;		//How many muscle are between this muscle and all the other muscle of the PuppetMaster.

		//MuscleCollisionBroadcaster* broadcaster = nullptr;		//The muscle collision event broadcaster component on the Rigidbody*.
		EchoWrapSharedPtr<_RagdollMuscle_JointComponentListener> jointListener = nullptr;	//Broadcasts OnJointBreak events to PuppetMaster.
		EchoWrapSharedPtr<RagdollMuscleColliderBroadcaster> colliderBroadcaster = nullptr;
		uint16							breakFlag = JointBreakFlag::noBreak;

		Vector3							positionOffset;						//Gets the offset of this muscle from it's target.
		//WorldNode*						transform = nullptr;				//Gets the WorldNode* of this muscle.
		//ActorComponent*					transform = nullptr;
		//ComponentNode*					transformComNode = nullptr;
		RigidBodyComponentPtr			rigidbody = nullptr;				//Gets the Rigidbody* of this muscle.
		WorldNode*						connectedBodyTarget = nullptr;		//Gets the target of this muscle joint's connectedBody if it has any.
		//ActorComponent*					connectedBodyTarget = nullptr;
		Vector3							targetAnimatedPosition;				//Gets the last read world space position of the target.
		Quaternion						targetAnimatedWorldRotation;		//Gets the last read world space rotation of the target.
		std::vector<ColliderComponentPtr> colliders;							//All the colliders of this muscle (including possible compound colliders).
		Vector3							targetVelocity;						//Gets the velocity of the target Transform.

		RigidBodyComponentPtr			additionalRigidbody = nullptr;		//The Rigidbody* of additional pin.
		ConfigurableJointComponentPtr	additionalPin = nullptr;			//The additional pin joint (optional);
		//WorldNode*						additionalPinTarget = nullptr;		//Target WorldNode* of the additional pin.
		ActorComponentPtr				additionalPinTarget = nullptr;
		float							additionalPinWeight = 0.f;			//Weight of the additional pin (multiplies this muscle's pin weight).

		Vector3							mappedVelocity;					
		Vector3							mappedAngularVelocity;			
		bool							isPropMuscle = false;			
		//int								index = -1;			

		//Gets the default sampled rotation offset of the Muscle* from it's target. If the muscle's rotation matches with it's target's in the Editor (while not playing) this will return Quaternion::IDENTITY.	
		Quaternion						targetRotationRelative;			

		RigidBodyComponentPtr			rebuildConnectedBody = nullptr;
		WorldNode*						rebuildTargetParent = nullptr;
		//ActorComponent*					rebuildTargetParent = nullptr;
		Vector3							defaultTargetPosRelToMuscle;
		Quaternion						defaultTargetRotRelToMuscle;
		Quaternion						defaultMuscleRotRelToTarget;
		//WorldNode*						rebuildParent = nullptr;
		ActorComponentPtr				rebuildParent = nullptr;

		Vector3							rebuildPosition;
		Quaternion						rebuildRotation = Quaternion::IDENTITY;
		Vector3							rebuildTargetPosition;
		Quaternion						rebuildTargetRotation = Quaternion::IDENTITY;
		JointMotion						rebuildAngularXMotion;
		JointMotion						rebuildAngularYMotion;
		JointMotion						rebuildAngularZMotion;

		Vector3							targetMappedPosition;
		Quaternion						targetMappedRotation = Quaternion::IDENTITY;
		Vector3							targetSampledPosition;
		Quaternion						targetSampledRotation = Quaternion::IDENTITY;

		JointDrive						slerpDrive;// = new JointDrive();
		float							lastJointDriveRotationWeight = -1.f, lastRotationDamper = -1.f;

		Vector3							defaultPosition;
		Vector3							defaultTargetLocalPosition;
		Vector3							lastMappedPosition;
		Vector3							initPosition;

		Quaternion						defaultLocalRotation;
		Quaternion						localRotationConvert;
		Quaternion						toParentSpace;
		Quaternion						toJointSpaceInverse;
		Quaternion						toJointSpaceDefault;
		Quaternion						targetAnimatedRotation;
		Quaternion						defaultRotation;
		Quaternion						rotationRelativeToTarget;
		Quaternion						defaultTargetLocalRotation;
		Quaternion						lastMappedRotation;
		Quaternion						initRotation;

		BoneNode*						targetParent = nullptr;
		//WorldNode*						connectedBodyTransform = nullptr;
		//ActorComponent*					targetParent = nullptr;
		ActorComponentPtr				connectedBodyTransform = nullptr;

		JointMotion						angularXMotionDefault;
		JointMotion						angularYMotionDefault;
		JointMotion						angularZMotionDefault;

		bool							directTargetParent = false;
		bool							initiated = false;

		std::vector<PhysXShape*>		_colliders;

		float							lastReadTime = 0.f;
		float							lastWriteTime = 0.f;
		float							lastPinWeight = 0.0f;

		std::vector<bool>				disabledColliders;
		std::vector<MuscleTargetChild*> targetChildren;

		Vector3							additionalTargetVelocity;
		Vector3							targetAnimatedCenterOfMass;
		Vector3							additionalPinTargetAnimatedCenterOfMass;
		Quaternion						defaultTargetRotRelToMuscleInverse = Quaternion::IDENTITY;

	};
}
#endif // _EchoRagdollMuscle_H__
