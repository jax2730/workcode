#ifndef __ECHOIKLIMBCOMPONENT_H__
#define __ECHOIKLIMBCOMPONENT_H__

#include "EchoIKBaseComponent.h"
#include "EchoRole.h"
#include "EchoComponentPrerequisites.h"
#include "EchoProtocolComponent.h"
#include "Actor/EchoActor.h"

namespace Echo
{

	struct IKLimbIKComponentData : public IKBaseComponentData {
		LimbIKInfo mIKInfo;
		String TargetName;
		String BendGoalName;
		FinalIKInfoBase* GetIKInfo() override { return &mIKInfo; };
	};

	class _EchoComponentExport IKLimbIKComponent : public IKBaseComponent {
		DECLAREACTORCOMTYPE_H(IKLimbIKComponent)
			REGISTER_LUA_CLASS_KEY;
	public:
		virtual void SetCompData(IKBaseComponentData* pData) override;
		virtual void SetIKInfo(FinalIKInfoBase* pInfo) override;
		virtual void UseDefaultInitialize() override;
		virtual bool IsCanCreate() override;

		void SetIKInfoData(const LimbIKInfo& info);
		const LimbIKInfo& GetIKInfoData();
		inline IKBaseComponentData* CreateCompData() override
		{
			return new IKLimbIKComponentData();
		}
	public:
		bool GetFixTransform();
		float GetIKPositionWeight();
		float GetIKRotationWeight();
		float GetMaintainRotationWeight();
		int GetBendModifier();
		float GetBendModifierWeight();
		const String& GetTargetName();
		const String& GetBendGoalName();
		std::vector<Name>& GetBoneName();

		void SetFixTransform(bool value);
		void SetIKPositionWeight(float value);
		void SetIKRotationWeight(float value);
		void SetMaintainRotationWeight(float value);
		void SetBendModifier(int value);
		void SetBendModifierWeight(float value);
		void SetTargetName(const String& value);
		void SetBendGoalName(const String& value);
		void SetBoneName(const std::vector<Name>& value);
	public:
		void SetLimbTarget(const ActorComponentPtr& pComp);
		void SetLimbBendGoal(const ActorComponentPtr& pComp);
		const ActorComponentPtr& GetLimbTarget() { return mTargetPtr; }
		const ActorComponentPtr& GetLimbBendGoal() { return mBendGoalPtr; }
		void ResetLimbTarget(const ActorComponentPtr& pComp) {  }
	public:
		void ResetLimbTarget();
		void ResetLimbBendGoal();
		void SetSkeletonPosture(const std::map<Name, Quaternion>& boneMap);
		void ResetSkeletonPosture();
	protected:
		virtual void PostCreateIK() override;
		virtual void PreDestroyIK() override;
		void ApplyTargetPtr();
		void ApplyBendGoalPtr();
		ActorComponentPtr mTargetPtr = nullptr;
		ActorComponentPtr mBendGoalPtr = nullptr;
	};

	class _EchoComponentExport IKLimbIKComponentInfo : public IKBaseComponentInfo {
	public:
		IKLimbIKComponentInfo() {}
		IKLimbIKComponentInfo(const Name& comType, ActorCOMMoveableType moveable)
			: IKBaseComponentInfo(comType, moveable) {}

	public:
		virtual ~IKLimbIKComponentInfo() {}
		virtual void CopyToCompData(IKBaseComponentData* pData) override;
		virtual void CompDataToInfo(IKBaseComponentData* pData, IKBaseComponent* pComp) override;


		_DECLCOMINFO_VIRFUN
	public:
		_DECLCOMINFO_PARAM_BEGIN(IKLimbIKComponent)
			_DECLCOMINFO_PARAM_SIM(bool , FixTransform, true, SetFixTransform);
			_DECLCOMINFO_PARAM_SIM(float , IKPositionWeight, 1.f, SetIKPositionWeight);
			_DECLCOMINFO_PARAM_SIM(float, IKRotationWeight, 1.f, SetIKRotationWeight);
			_DECLCOMINFO_PARAM_SIM(float , MaintainRotationWeight, 0.f, SetMaintainRotationWeight);
			_DECLCOMINFO_PARAM_SIM(int , BendModifier, 0, SetBendModifier);
			_DECLCOMINFO_PARAM_SIM(float , BendModifierWeight, 1.f, SetBendModifierWeight);
			_DECLCOMINFO_PARAM(String, TargetName, String(""), SetTargetName);
			_DECLCOMINFO_PARAM(String, BendGoalName, String(""), SetBendGoalName);
			_DECLCOMINFO_PARAM(std::vector<Name>, BoneName, std::vector<Name>(3), SetBoneName);
		_DECLCOMINFO_PARAM_END()
	};

	DEFINECOMFACTORY(IKLimbIKComponent, UNMOVEABLE)

}

#endif //__ECHOIKLIMBCOMPONENT_H__
 



