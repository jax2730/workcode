#ifndef _EchoRagdollTools_H__
#define _EchoRagdollTools_H__

#include "EchoComponentPrerequisites.h"
#include "EchoPhysXAsset.h"
#include "EchoRagdollMuscle.h"
#include "EchoPhysXAsset.h"
//#include "EchoPhysXJoint.h"
#include "EchoPhysicsManager.h"
#include "EchoRigidBodyComponent.h"
#include "EchoConfigurableJointComponent.h"
#include "EchoBoxColliderComponent.h"
#include "EchoCapsuleColliderComponent.h"
#include "EchoRagdollCreatorComponent.h"
namespace Echo
{
	//template <class Ty>
	//class _Create_Can_Im_Export_Class : public Ty
	//{
	//public:
	//	static int reflectFlag;
	//	void ExportJson(cJSON* jsonNode);
	//	bool ImportJson(cJSON* jsonNode);

	//	operator Ty& () { return *(Ty*)this; }

	//};

	class RagdollTools
	{
	public:
		// Predicts unconstrained Rigidbody* (ignoring collisions and joints) position and rotation after specified PhysX steps.
		static void Predict(RigidBodyComponent* r, int steps, Vector3& position, Quaternion& rotation);

		// Predicts unconstrained Rigidbody* (ignoring collisions and joints) position and rotation after specified PhysX steps using custom gravity, drag and angularDrag settings.
		static void Predict(RigidBodyComponent* r, int steps, Vector3& position, Quaternion& rotation, Vector3 gravity, float drag, float angularDrag);


		// Predicts a single PhysX step of an unconstrained Rigidbody* (ignoring collisions and joints) position and rotation using custom gravity, drag and angularDrag settings.
		static void Predict(Vector3& position, Quaternion& rotation, Vector3& velocity, Vector3& angularVelocity, Vector3 gravity, float drag, float angularDrag);

		// Gets the center of mass of a puppet.
		//static Vector3 GetCenterOfMass(RagdollComponent* puppet);

		// Gets the center of mass of an array of Rigidbodies.
		static Vector3 GetCenterOfMass(std::vector<RigidBodyComponent*> rigidbodies);

		// Gets the velocity of the center of mass of an array of Rigidbodies.
		static Vector3 GetCenterOfMassVelocity(std::vector<RigidBodyComponent*> rigidbodies);

		// Divides an angular acceleration by an inertia tensor.
		static void DivByInertia(Vector3& v, Quaternion rotation, Vector3 inertiaTensor);

		//Scales an angular acceleration by an inertia tensor		
		static void ScaleByInertia(Vector3& v, Quaternion rotation, Vector3 inertiaTensor);
		
		//Returns angular velocity from lastRotation to rotation
		static Vector3 GetAngularVelocity(Quaternion lastRotation, Quaternion rotation, float deltaTime);

		//Returns the angular acceleration from one vector to another.		
		//static Vector3 GetFromToAcceleration(Vector3 fromV, Vector3 toV);
	
		//Returns the angular acceleration from "fromR" to "toR." 
		//Does not guarantee full accuracy with rotations around multiple axes).		
		static Vector3 GetAngularAcceleration(Quaternion fromR, Quaternion toR);
		static Vector3 GetAngularAcceleration(Quaternion fromR, Quaternion toR, float deltaTime);
		
		//Adds torque to the Ridigbody that accelerates it from it's current rotation to another using any force mode.	
		static void AddFromToTorque(RigidBodyComponent* r, Quaternion toR, EchoPhysics::ForceMode forceMode);
		
		//Adds torque to the Ridigbody that accelerates it from one direction to another using any force mode.		
		static void AddFromToTorque(RigidBodyComponent* r, Vector3 fromV, Vector3 toV, EchoPhysics::ForceMode forceMode);
		
		//Adds a force to a Rigidbody* that gets it from one place to another within a single simulation step using any force mode.		
		static void AddFromToForce(RigidBodyComponent* r, Vector3 fromV, Vector3 toV, EchoPhysics::ForceMode forceMode);
		
		//Returns the linear acceleration from one point to another.		
		static Vector3 GetLinearAcceleration(Vector3 fromPoint, Vector3 toPoint);
		
		//The rotation expressed by the joint's axis and secondary axis		
		static Quaternion ToJointSpace(ConfigurableJointComponent* joint);
		
		//Calculates the inertia tensor for a cuboid.		
		static Vector3 CalculateInertiaTensorCuboid(Vector3 size, float mass);
		
		//Divide all the values in v by their respective values in v2.	
		static Vector3 Div(Vector3 v, Vector3 v2);

		
		//Returns true if a ray from 'origin' with 'direction' intersects with a CapsuleCollider. The scale of the CapsuleCollider's Transform is ignored.		
		//static bool RayCapsuleIntersectUnscaled(Vector3 origin, Vector3 direction, CapsuleCollider capsule);

		//Returns true if a ray from 'origin' with 'direction' intersects with a CapsuleCollider.		
		//static bool RayCapsuleIntersect(Vector3 origin, Vector3 direction, CapsuleCollider capsule, float uniformScale);
	
		//Returns true if a ray from 'origin' with 'direction' intersects with the specified CapsuleCollider parameters. The scale of the CapsuleCollider's Transform is ignored.	
		static bool RayCapsuleIntersect(Vector3 origin, Vector3 direction, Vector3 capsuleTransformPos, Quaternion capsuleTransformRot, Vector3 capsuleCenter, float capsuleRadius, float capsuleHeight, int capsuleDir, float scale);
		
		//Returns true if a ray from 'rayOrigin' with direction if 'rayDir' intersects with a capsule from point 'c1' to 'c2' with radius of 'cRadius'.		
		static bool RayCapsuleIntersect(Vector3 rayOrigin, Vector3 rayDir, Vector3 c1, Vector3 c2, float cRadius);

		//Actcomponent 等效 Transform 计算相关
		static Vector3				InverseActorcomponentPoint(ActorComponent* act, const Vector3& inPosition);
		static Vector3				ActComponentComponentPoint(ActorComponent* act, Vector3& inPoint);
		
		// 导出ragdoll的子节点(由ragdollCreatorComponent创建的在ragdoll的block)以及ragdoll自身的信息
		static void ExportRagdollChildrenNodeAndData(RagdollComponent* info, RagdollComponentInitData& data);

		static bool ImportRagdollChildrenNodeAndData(EchoWeakPtr<RoleComponent> role, RagdollComponentPtr target, bool runtime, const RagdollComponentInitData& data);
		static bool ImportRagdollChildrenNode(EchoWeakPtr<RoleComponent> role, RagdollComponentPtr ragdoll, const RagdollComponentInitData& data);
		static bool ImportRagdollData(EchoWeakPtr<RoleComponent> role, RagdollComponentPtr target, bool runtime, const RagdollComponentInitData& data);

		static RoleIKComponent* GetRoleComIK(RoleComponent* pRoleCom);

	};
}

#endif
