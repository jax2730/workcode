#ifndef _ECHOIKINERTIACOMPONENT_H_
#define _ECHOIKINERTIACOMPONENT_H_

#include "EchoIKBaseComponent.h"
#include "EchoRole.h"

#include "IK/EchoIKSolverFullBodyBiped.h"
#include "EchoComponentPrerequisites.h"
#include "EchoProtocolComponent.h"
#include "Actor/EchoActor.h"
#include "EchoQuaternion.h"
#include "EchoWeakPtr.h"

namespace Echo
{

	struct IKInertiaComponentData : public IKBaseComponentData {
		IKInertiaInfo mIKInfo;
		FinalIKInfoBase* GetIKInfo() override { return &mIKInfo; }
	};

	class _EchoComponentExport IKInertiaComponent : public IKBaseComponent {
		DECLAREACTORCOMTYPE_H(IKInertiaComponent)
			REGISTER_LUA_CLASS_KEY;
	public:
		virtual void SetIKInfo(FinalIKInfoBase* pInfo) override;
		virtual void UseDefaultInitialize() override;
		virtual bool IsCanCreate() override;
		void SetIKInfoData(const IKInertiaInfo& info);
		const IKInertiaInfo& GetIKInfoData();
		inline IKBaseComponentData* CreateCompData() override
		{
			return new IKInertiaComponentData();
		}
	public:
		void SetFullBodyBipedIKName(const Name& ikName);
		const Name& GetFullBodyBipedIKName();

		float GetWeight();
		void SetWeight(float value);

		const std::vector<InertiaBodyInfo>& GetBodies();
		void SetBodies(const std::vector<InertiaBodyInfo>& value);

		const std::vector<InertiaOffsetLimitsInfo>& GetLimits();
		void SetLimits(const std::vector<InertiaOffsetLimitsInfo>& value);

	public:
		EchoIK::FullBodyBipedIK* GetFullBodyBipedIK();

	};

	class _EchoComponentExport IKInertiaComponentInfo : public IKBaseComponentInfo {
	public:
		IKInertiaComponentInfo() {}
		IKInertiaComponentInfo(const Name& comType, ActorCOMMoveableType moveable)
			: IKBaseComponentInfo(comType, moveable) {}

	public:
		virtual ~IKInertiaComponentInfo() {}
		virtual void CopyToCompData(IKBaseComponentData* pData) override;
		virtual void CompDataToInfo(IKBaseComponentData* pData, IKBaseComponent* pComp) override;


		_DECLCOMINFO_VIRFUN
	public:
		_DECLCOMINFO_PARAM_BEGIN(IKInertiaComponent)
		_DECLCOMINFO_PARAM(Name, FullBodyBipedIKName, Name::g_cEmptyName, SetFullBodyBipedIKName)
		_DECLCOMINFO_PARAM_SIM(float, Weight, 1.f, SetWeight);
		_DECLCOMINFO_PARAM(std::vector<InertiaBodyInfo>, Bodies, {}, SetBodies);
		_DECLCOMINFO_PARAM(std::vector<InertiaOffsetLimitsInfo>, Limits, {}, SetLimits);
		_DECLCOMINFO_PARAM_END()
	};

	DEFINECOMFACTORY(IKInertiaComponent, UNMOVEABLE)
}

#endif // _ECHOIKINERTIACOMPONENT_H_
