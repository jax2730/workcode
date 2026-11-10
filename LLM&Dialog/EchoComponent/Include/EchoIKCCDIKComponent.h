#ifndef __ECHOIKCCDCOMPONENT_H__
#define __ECHOIKCCDCOMPONENT_H__
#include "EchoIKBaseComponent.h"
#include "EchoRole.h"
#include "EchoComponentPrerequisites.h"
#include "EchoProtocolComponent.h"
#include "Actor/EchoActor.h"

namespace Echo
{

	struct IKCCDIKComponentData : public IKBaseComponentData {
		IKCCDIKInfo mIKInfo;
		String TargetName;
		FinalIKInfoBase* GetIKInfo() override { return &mIKInfo; }
	};

	class _EchoComponentExport IKCCDIKComponent : public IKBaseComponent {
		DECLAREACTORCOMTYPE_H(IKCCDIKComponent)
			REGISTER_LUA_CLASS_KEY;
	public:
		virtual void SetCompData(IKBaseComponentData* pData) override;
		virtual void SetIKInfo(FinalIKInfoBase* pInfo) override;
		virtual void UseDefaultInitialize() override;
		virtual bool IsCanCreate() override;
		void SetIKInfoData(const IKCCDIKInfo& info);
		const IKCCDIKInfo& GetIKInfoData();
		inline IKBaseComponentData* CreateCompData() override
		{
			return new IKCCDIKComponentData();
		}
	public:
		bool GetFixTransform();
		const String& GetTargetName();
		float GetPositionWeight();
		float GetRotationWeight();
		float GetTolerance();
		int GetMaxIteration();
		bool GetUseRotationLimits();
		bool GetXY();
		std::vector<NodeWeightInfo>& GetBones();

		void SetFixTransform(bool value);
		void SetTargetName(const String& value);
		void SetPositionWeight(float value);
		void SetRotationWeight(float value);
		void SetTolerance(float value);
		void SetMaxIteration(int value);
		void SetUseRotationLimits(bool value);
		void SetXY(bool value);
		void SetBones(const std::vector<NodeWeightInfo>& value);
	public:
		void SetCCDTarget(const ActorComponentPtr& pComp);
		const ActorComponentPtr& GetCCDTarget() { return mTargetPtr; }
		void ResetCCDTarget(const ActorComponentPtr& pComp) {}
	public:
		void ResetCCDTarget();
		void SetSkeletonPosture(const std::map<Name, Quaternion>& boneMap);
		void ResetSkeletonPosture();
	protected:
		virtual void PostCreateIK() override;
		virtual void PreDestroyIK() override;
		void ApplyTargetPtr();
		ActorComponentPtr mTargetPtr = nullptr;
	};

	class _EchoComponentExport IKCCDIKComponentInfo : public IKBaseComponentInfo {
	public:
		IKCCDIKComponentInfo();
		IKCCDIKComponentInfo(const Name& comType, ActorCOMMoveableType moveable);

	public:
		virtual ~IKCCDIKComponentInfo();
		virtual void CopyToCompData(IKBaseComponentData* pData) override;
		virtual void CompDataToInfo(IKBaseComponentData* pData, IKBaseComponent* pComp) override;


		_DECLCOMINFO_VIRFUN
	public:
		_DECLCOMINFO_PARAM_BEGIN(IKCCDIKComponent)
			_DECLCOMINFO_PARAM_SIM(bool, FixTransform, false, SetFixTransform);
			_DECLCOMINFO_PARAM(String, TargetName, String(""), SetTargetName);
			_DECLCOMINFO_PARAM_SIM(float, PositionWeight, 1.f, SetPositionWeight);
			_DECLCOMINFO_PARAM_SIM(float, RotationWeight, 1.f, SetRotationWeight);
			_DECLCOMINFO_PARAM_SIM(float, Tolerance, 0.f, SetTolerance);
			_DECLCOMINFO_PARAM_SIM(int, MaxIteration, 4, SetMaxIteration);
			_DECLCOMINFO_PARAM_SIM(bool, UseRotationLimits, true, SetUseRotationLimits);
			_DECLCOMINFO_PARAM_SIM(bool, XY, false, SetXY);
			_DECLCOMINFO_PARAM(std::vector<NodeWeightInfo>, Bones, {}, SetBones);
		_DECLCOMINFO_PARAM_END()
	};

	DEFINECOMFACTORY(IKCCDIKComponent, UNMOVEABLE)
}

#endif


