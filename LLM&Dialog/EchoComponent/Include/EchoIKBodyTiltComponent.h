#ifndef __ECHOIKBODYTITLCOMPONENT_H__
#define __ECHOIKBODYTITLCOMPONENT_H__

#include "EchoIKBaseComponent.h"
#include "EchoRole.h"
#include "EchoComponentPrerequisites.h"
#include "EchoProtocolComponent.h"
#include "Actor/EchoActor.h"

namespace Echo
{
	
	struct IKBodyTiltComponentData : public IKBaseComponentData {
		BodyTiltInfo mIKInfo;
		FinalIKInfoBase* GetIKInfo() override { return &mIKInfo; }
	};

	class _EchoComponentExport IKBodyTiltComponent : public IKBaseComponent {
		DECLAREACTORCOMTYPE_H(IKBodyTiltComponent)
			REGISTER_LUA_CLASS_KEY;
	public:
		//virtual void SetCompData(IKBaseComponentData* pData) override;
		virtual void SetIKInfo(FinalIKInfoBase* pInfo) override;
		virtual void UseDefaultInitialize() override;
		virtual bool IsCanCreate() override;
		void SetIKInfoData(const BodyTiltInfo& info);
		const BodyTiltInfo& GetIKInfoData();
		inline IKBaseComponentData* CreateCompData() override
		{
			return new IKBodyTiltComponentData();
		}
	public:
		float GetWeight();
		void SetWeight(float value);

		void SetFullBodyBipedIKName(const Name& value);
		const Name& GetFullBodyBipedIKName();

		float GetTiltSpeed();
		void SetTiltSpeed(float value);

		float GetTiltSensitivity();
		void SetTiltSensitivity(float value);

		const std::vector<BTEffectorLink>& GetPoseLeft();
		void SetPoseLeft(const std::vector<BTEffectorLink>& value);

		const std::vector<BTEffectorLink>& GetPoseRight();
		void SetPoseRight(const std::vector<BTEffectorLink>& value);

		EchoIK::FullBodyBipedIK* GetFullBodyBipedIK();

	};

	class _EchoComponentExport IKBodyTiltComponentInfo : public IKBaseComponentInfo {
	public:
		IKBodyTiltComponentInfo();
		IKBodyTiltComponentInfo(const Name& comType, ActorCOMMoveableType moveable);

	public:
		virtual ~IKBodyTiltComponentInfo();
		virtual void CopyToCompData(IKBaseComponentData* pData) override;
		virtual void CompDataToInfo(IKBaseComponentData* pData, IKBaseComponent* pComp) override;

		_DECLCOMINFO_VIRFUN
	public:
		_DECLCOMINFO_PARAM_BEGIN(IKBodyTiltComponent)
			_DECLCOMINFO_PARAM_SIM(float, Weight, 1.f, SetWeight);
			_DECLCOMINFO_PARAM(Name, FullBodyBipedIKName, Name::g_cEmptyName, SetFullBodyBipedIKName);
			_DECLCOMINFO_PARAM_SIM(float, TiltSpeed, 5.f, SetTiltSpeed);
			_DECLCOMINFO_PARAM_SIM(float, TiltSensitivity, 5.f, SetTiltSensitivity);
			_DECLCOMINFO_PARAM(std::vector<BTEffectorLink>, PoseLeft, {}, SetPoseLeft);
			_DECLCOMINFO_PARAM(std::vector<BTEffectorLink>, PoseRight, {}, SetPoseRight);
		_DECLCOMINFO_PARAM_END()

	};//Class IKHitReactionComponentInfo

	DEFINECOMFACTORY(IKBodyTiltComponent, UNMOVEABLE)

}

#endif



