//-----------------------------------------------------------------------------
// File:   EchoConstantForceComponent.h
//
// Author: qiao_yuzhi
//
// Date:   2022-8-25
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef __EchoConstantForceComponent_h__ 
#define __EchoConstantForceComponent_h__

#include "EchoComponentPrerequisites.h"
#include "EchoProtocolComponent.h"
#include "Actor/EchoActor.h"

#include "EchoHelperObj.h"
#include "EchoHelperObjProFuncImp.h"
#include "EchoPhysicsManager.h"
#include "EchoComDefineMacro.h"
#include "EchoRigidBodyComponent.h"


namespace Echo {

	struct ConstantForceComponentData {
		Vector3 Force = { 0,0,0 };
		Vector3 RelativeForce = { 0,0,0 };
		Vector3 Torque = { 0,0,0 };
		Vector3 RelativeTorque = { 0,0,0 };
		int ForceMode = EchoPhysics::FORCE;
	};
	
	class ConstantForceComponentInfo;

	class _EchoComponentExport ConstantForceComponent : public PhysicsBaseComponent {
		DECLAREACTORCOMTYPE_H(ConstantForceComponent)
		REGISTER_LUA_CLASS_KEY;
	public://¼Ì³Ð×ÔActorComponent
		virtual void init(ActorComponentInfo* pInfo) override final;
		virtual void unInit() override final;
		virtual void replay(ActorComponentInfo* info) override;
		virtual void copyTo(const ActorComponentPtr& actCom) override;

		virtual void connectCom() override;
		virtual void disconnectCom() override;
	public:
		virtual void CreatePhysics() override;
		virtual void ReleasePhysics() override;
		virtual bool IsCanPhysics() override;
		virtual bool IsExistPhysics() override;
#ifdef USE_PHYSICS_HELPEROBJ
		virtual void UpdateComponentInEditor() override {}
#endif // USE_PHYSICS_HELPEROBJ
		void ApplyPhysics();
		void ChangeRigidBody();
	public:
		void SetForce(const Vector3& force);
		void SetRelativeForce(const Vector3& force);
		void SetTorque(const Vector3& torque);
		void SetRelativeTorque(const Vector3& torque);
		void SetForceMode(int value);
		const Vector3& GetForce();
		const Vector3& GetRelativeForce();
		const Vector3& GetTorque();
		const Vector3& GetRelativeTorque();
		int GetForceMode();
		void GetForce(Vector3& force);
		void GetRelativeForce(Vector3& force);
		void GetTorque(Vector3& torque);
		void GetRelativeTorque(Vector3& torque);
		void SetCompData(ConstantForceComponentData* pData);
		const ConstantForceComponentData* GetCompData();
	protected:
		virtual EchoPhysicsComponentType GetPhysicsType() override { return EchoPhysicsComponentType::eFORCE; }
	private:
		RigidBodyComponentPtr m_pAttachRigidBodyComp = nullptr;
		ConstantForceComponentData mCompData;
	};

	class _EchoComponentExport ConstantForceComponentInfo : public ActorComponentInfo {
	public:
		ConstantForceComponentInfo() {}
		ConstantForceComponentInfo(const Name& comType, ActorCOMMoveableType MoveType);
		virtual ~ConstantForceComponentInfo();
		virtual void CopyToCompData(ConstantForceComponentData* pData);
		_DECLCOMINFO_VIRFUN
	public:
		_DECLCOMINFO_PARAM_BEGIN(ConstantForceComponent)
		_DECLCOMINFO_PARAM(Vector3, Force, Vector3::ZERO, SetForce);
		_DECLCOMINFO_PARAM(Vector3, RelativeForce, Vector3::ZERO, SetRelativeForce);
		_DECLCOMINFO_PARAM(Vector3, Torque, Vector3::ZERO, SetTorque);
		_DECLCOMINFO_PARAM(Vector3, RelativeTorque, Vector3::ZERO, SetRelativeTorque);
		_DECLCOMINFO_PARAM_SIM(int, ForceMode, EchoPhysics::FORCE, SetForceMode);
		_DECLCOMINFO_PARAM_END()
	};

	DEFINECOMFACTORY(ConstantForceComponent, UNMOVEABLE)

	class _EchoComponentExport EchoConstantForceProFuncImp
	{
	public:
		//-----------------------------------------------------------------------------
		static void								initProtocolFunction();
		static void								uninitProtocolFunction();
	};


}



#endif /*__EchoConstantForceComponent_h__*/
