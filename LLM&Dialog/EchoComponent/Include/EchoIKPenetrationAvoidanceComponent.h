#ifndef __EchoIKPenetrationAvoidanceComponent_H__
#define __EchoIKPenetrationAvoidanceComponent_H__

#include "EchoIKBaseComponent.h"
#include "EchoRole.h"
#include "EchoComponentPrerequisites.h"
#include "EchoProtocolComponent.h"
#include "Actor/EchoActor.h"

namespace Echo
{

	struct IKPenetrationAvoidanceComponentData : public IKBaseComponentData {
		PenetrationAvoidanceInfo mIKInfo;
		FinalIKInfoBase* GetIKInfo() override { return &mIKInfo; }
	};

	class _EchoComponentExport IKPenetrationAvoidanceComponent : public IKBaseComponent {
		DECLAREACTORCOMTYPE_H(IKPenetrationAvoidanceComponent)
			REGISTER_LUA_CLASS_KEY;
	public:
		virtual void SetIKInfo(FinalIKInfoBase* pInfo) override;
		virtual void UseDefaultInitialize() override;
		virtual bool IsCanCreate() override;
		void SetIKInfoData(const PenetrationAvoidanceInfo& info);
		const PenetrationAvoidanceInfo& GetIKInfoData();
		inline IKBaseComponentData* CreateCompData() override
		{
			return new IKPenetrationAvoidanceComponentData();
		}
	public:
		float GetWeight();
		void SetWeight(float value);

		void SetFullBodyBipedIKName(const Name& value);
		const Name& GetFullBodyBipedIKName();

		const std::vector<Avoider>& GetAvoider();
		void SetAvoider(const std::vector<Avoider>& value);

		EchoIK::FullBodyBipedIK* GetFullBodyBipedIK();
	};


	class _EchoComponentExport IKPenetrationAvoidanceComponentInfo : public IKBaseComponentInfo {
	public:
		IKPenetrationAvoidanceComponentInfo();
		IKPenetrationAvoidanceComponentInfo(const Name& comType, ActorCOMMoveableType moveable);

	public:
		virtual ~IKPenetrationAvoidanceComponentInfo();
		virtual void CopyToCompData(IKBaseComponentData* pData) override;
		virtual void CompDataToInfo(IKBaseComponentData* pData, IKBaseComponent* pComp) override;

		_DECLCOMINFO_VIRFUN
	public:
		_DECLCOMINFO_PARAM_BEGIN(IKPenetrationAvoidanceComponent)
		_DECLCOMINFO_PARAM(Name, FullBodyBipedIKName, Name::g_cEmptyName, SetFullBodyBipedIKName);
		_DECLCOMINFO_PARAM_SIM(float, Weight, 1.f, SetWeight);
		_DECLCOMINFO_PARAM(std::vector<Avoider>, Avoider, {}, SetAvoider);
		_DECLCOMINFO_PARAM_END()

	};

	DEFINECOMFACTORY(IKPenetrationAvoidanceComponent, UNMOVEABLE)

}

#endif



