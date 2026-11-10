#ifndef __EchoIKAimControllerComponent_H__
#define __EchoIKAimControllerComponent_H__

#include "EchoComponentPrerequisites.h"
#include "EchoProtocolComponent.h"
#include "Actor/EchoActor.h"
#include "EchoComDefineMacro.h"
#include "EchoIKBaseComponent.h"
#include "IK/EchoIKCommon.h"

namespace Echo
{
	class AimControllerComponent;

	struct AimControllerComponentData : public IKBaseComponentData
	{
		AimControllerInfo mIKInfo;
		String mTargetNodeCompName;
		String mAimIKCompName;
		FinalIKInfoBase* GetIKInfo() override { return &mIKInfo; }
	};

	// Dual Observation
	class _EchoComponentExport AimControllerComponent : public IKBaseComponent,public EchoIK::ILifecycleObserver
	{
		DECLAREACTORCOMTYPE_H(AimControllerComponent)
			REGISTER_LUA_CLASS_KEY;
	public:
		RoleObjectListenerExecutionPriority GetExecutionPriority() const override { return RoleObjectListenerExecutionPriority::AIM_IK_CONTROLLER; }
		virtual void SetCompData(IKBaseComponentData* pData) override;
		virtual void SetIKInfo(FinalIKInfoBase* pInfo) override;
		void SetIKInfoData(const AimControllerInfo& info);
		const AimControllerInfo& GetIKInfoData();
		virtual void UseDefaultInitialize() override;
		virtual bool IsCanCreate() override;
		inline IKBaseComponentData* CreateCompData() override
		{
			return new AimControllerComponentData();
		}
	public:
		const String& GetAimIKCompName();
		const String& GetTargetNodeCompName();
		float		GetWeight();
		float		GetTargetSwitchSmoothTime();
		float		GetWeightSmoothTime();
		bool		GetSmoothTurnTowardsTarget();
		float		GetMaxRadiansDelta();
		float		GetMaxMagnitudeDelta();
		float		GetSlerpSpeed();
		float		GetSmoothDampTime();
		Vector3		GetPivotOffsetFromRoot();
		float		GetMinDistance();
		Vector3		GetOffset();
		float		GetMaxRootAngle();
		bool		GetTurnToTarget();
		float		GetTurnToTargetTime();
		bool		GetUseAnimatedAimDirection();
		Vector3		GetAnimatedAimDirection();


		void	SetTargetNodeCompName		(const String& value);
		void	SetAimIKCompName			(const String& value);
		void	SetTargetNodeComponent		(const ActorComponentPtr& pComp);
		void    SetAimIKComponent			(IKAimIKComponent* pComp);
		void	SetWeight					(float		value);
		void	SetTargetSwitchSmoothTime	(float		value);
		void	SetWeightSmoothTime			(float		value);
		void	SetSmoothTurnTowardsTarget	(bool		value);
		void	SetMaxRadiansDelta			(float		value);
		void	SetMaxMagnitudeDelta		(float		value);
		void	SetSlerpSpeed				(float		value);
		void	SetSmoothDampTime			(float		value);
		void	SetPivotOffsetFromRoot		(Vector3	value);
		void	SetMinDistance				(float		value);
		void	SetOffset					(Vector3	value);
		void	SetMaxRootAngle				(float		value);
		void	SetTurnToTarget				(bool		value);
		void	SetTurnToTargetTime			(float		value);
		void	SetUseAnimatedAimDirection	(bool		value);
		void	SetAnimatedAimDirection		(Vector3	value);

		/// Do not manually invoke.
		void	onObjectDestroyed(IKComponent* pIkComponent) override;
	protected:
		void PostCreateIK() override;
		void ApplyTargetPtr();
		void ApplyAimIKPtr();
		ActorComponentPtr  mTargetPtr = nullptr;
		IKAimIKComponent*  mAimIK = nullptr;

	};//Class AimControllerComponent

	class _EchoComponentExport AimControllerComponentInfo : public IKBaseComponentInfo {
	public:
		AimControllerComponentInfo(){}
		AimControllerComponentInfo(const Name& comType, ActorCOMMoveableType moveable) :IKBaseComponentInfo(comType, moveable) {}
		virtual ~AimControllerComponentInfo() {}
		virtual void CopyToCompData(IKBaseComponentData* pData) override;
		virtual void CompDataToInfo(IKBaseComponentData* pData, IKBaseComponent* pComp) override;
		_DECLCOMINFO_VIRFUN
	public:
		_DECLCOMINFO_PARAM_BEGIN(AimControllerComponent)
		_DECLCOMINFO_PARAM_SIM(float, Weight, 1.f, SetWeight);
		_DECLCOMINFO_PARAM_SIM(float, TargetSwitchSmoothTime, 0.3f, SetTargetSwitchSmoothTime);
		_DECLCOMINFO_PARAM_SIM(float, WeightSmoothTime, 0.3f, SetWeightSmoothTime);
		_DECLCOMINFO_PARAM_SIM(bool, SmoothTurnTowardsTarget, true, SetSmoothTurnTowardsTarget);
		_DECLCOMINFO_PARAM_SIM(float, MaxRadiansDelta, 3.f, SetMaxRadiansDelta);
		_DECLCOMINFO_PARAM_SIM(float, MaxMagnitudeDelta, 3.f, SetMaxMagnitudeDelta);
		_DECLCOMINFO_PARAM_SIM(float, SlerpSpeed, 3.f, SetSlerpSpeed);
		_DECLCOMINFO_PARAM_SIM(float, SmoothDampTime, 0.f, SetSmoothDampTime);
		_DECLCOMINFO_PARAM(Vector3, PivotOffsetFromRoot, Vector3::UP, SetPivotOffsetFromRoot);
		_DECLCOMINFO_PARAM_SIM(float, MinDistance, 1.f, SetMinDistance);
		_DECLCOMINFO_PARAM(Vector3, Offset, Vector3(0.f, 0.f, 0.f), SetOffset);
		_DECLCOMINFO_PARAM_SIM(float, MaxRootAngle, 45.f, SetMaxRootAngle);
		_DECLCOMINFO_PARAM_SIM(bool, TurnToTarget, false, SetTurnToTarget);
		_DECLCOMINFO_PARAM_SIM(float, TurnToTargetTime, 0.2f, SetTurnToTargetTime);
		_DECLCOMINFO_PARAM_SIM(bool, UseAnimatedAimDirection, false, SetUseAnimatedAimDirection);
		_DECLCOMINFO_PARAM(Vector3, AnimatedAimDirection, Vector3::FORWARD, SetAnimatedAimDirection);

		_DECLCOMINFO_PARAM(String, TargetNodeCompName, "", SetTargetNodeCompName);
		_DECLCOMINFO_PARAM(String, AimIKCompName, "", SetAimIKCompName);

		_DECLCOMINFO_PARAM_END()

	};//Class IKHitReactionComponentInfo

	DEFINECOMFACTORY(AimControllerComponent, UNMOVEABLE)

}//Namespace Echo




#endif




