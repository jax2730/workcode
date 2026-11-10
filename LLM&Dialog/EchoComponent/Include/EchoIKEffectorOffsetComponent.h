#ifndef _EchoIKEffectorOffsetComponent_h__ 
#define _EchoIKEffectorOffsetComponent_h__ 

#include "EchoComponentPrerequisites.h"
#include "EchoProtocolComponent.h"
#include "Actor/EchoActor.h"
#include "EchoIKBaseComponent.h"
#include "EchoRole.h"

namespace Echo {

	struct IKEffectorOffsetComponentData : public IKBaseComponentData {
		IKEffectorOffsetInfo mIKInfo;
		FinalIKInfoBase* GetIKInfo() override {  return &mIKInfo; }
	};

	class _EchoComponentExport IKEffectorOffsetComponent : public IKBaseComponent {
		DECLAREACTORCOMTYPE_H(IKEffectorOffsetComponent)
			REGISTER_LUA_CLASS_KEY;
	public:
		virtual void SetIKInfo(FinalIKInfoBase* pInfo) override;
		void SetIKInfoData(const IKEffectorOffsetInfo& info);
		const IKEffectorOffsetInfo& GetIKInfoData();
		virtual void UseDefaultInitialize() override;
		virtual bool IsCanCreate() override;
		inline IKBaseComponentData* CreateCompData() override
		{
			return new IKEffectorOffsetComponentData();
		}
	public:
		void SetFullBodyBipedIKName(const Name& ikName);
		const Name& GetFullBodyBipedIKName();
		float GetWeight();
		void SetWeight(float value);
		float GetHMRPWeigth();
		void SetHMRPWeight(float value);

		const Vector3& GetBodyOffset();
		void SetBodyOffset(const Vector3& value);


		const Vector3& GetLeftShoulderOffset();
		void SetLeftShoulderOffset(const Vector3& value);
		const Vector3& GetRightShoulderOffset();
		void SetRightShoulderOffset(const Vector3& value);

		const Vector3& GetLeftThighOffset();
		void SetLeftThighOffset(const Vector3& value);
		const Vector3& GetRightThighOffset();
		void SetRightThighOffset(const Vector3& value);

		const Vector3& GetLeftHandOffset();
		void SetLeftHandOffset(const Vector3& value);
		const Vector3& GetRightHandOffset();
		void SetRightHandOffset(const Vector3& value);

		const Vector3& GetLeftFootOffset();
		void SetLeftFootOffset(const Vector3& value);
		const Vector3& GetRightFootOffset();
		void SetRightFootOffset(const Vector3& value);
	public:
		EchoIK::FullBodyBipedIK* GetFullBodyBipedIK();
	};

	class _EchoComponentExport IKEffectorOffsetComponentInfo : public IKBaseComponentInfo {
	public:
		IKEffectorOffsetComponentInfo() { }
		IKEffectorOffsetComponentInfo(const Name& comType, ActorCOMMoveableType moveable) : IKBaseComponentInfo(comType, moveable) { ; }
		virtual ~IKEffectorOffsetComponentInfo() { ; }

		virtual void CopyToCompData(IKBaseComponentData* pData) override;
		virtual void CompDataToInfo(IKBaseComponentData* pData, IKBaseComponent* pComp) override;

		_DECLCOMINFO_VIRFUN
	public:
		_DECLCOMINFO_PARAM_BEGIN(IKEffectorOffsetComponent)
		_DECLCOMINFO_PARAM(Name, FullBodyBipedIKName, Name::g_cEmptyName, SetFullBodyBipedIKName)
		_DECLCOMINFO_PARAM_SIM(float, Weight, 1.f, SetWeight)
		_DECLCOMINFO_PARAM_SIM(float, HMRPWeight, 0.f, SetHMRPWeight)
		_DECLCOMINFO_PARAM(Vector3, BodyOffset, Vector3::ZERO, SetBodyOffset)
		_DECLCOMINFO_PARAM(Vector3, RightShoulderOffset, Vector3::ZERO, SetRightShoulderOffset)
		_DECLCOMINFO_PARAM(Vector3, LeftShoulderOffset, Vector3::ZERO, SetLeftShoulderOffset)
		_DECLCOMINFO_PARAM(Vector3, RightThighOffset, Vector3::ZERO, SetRightThighOffset)
		_DECLCOMINFO_PARAM(Vector3, LeftThighOffset, Vector3::ZERO, SetLeftThighOffset)
		_DECLCOMINFO_PARAM(Vector3, RightHandOffset, Vector3::ZERO, SetRightHandOffset)
		_DECLCOMINFO_PARAM(Vector3, LeftHandOffset, Vector3::ZERO, SetLeftHandOffset)
		_DECLCOMINFO_PARAM(Vector3, RightFootOffset, Vector3::ZERO, SetRightFootOffset)
		_DECLCOMINFO_PARAM(Vector3, LeftFootOffset, Vector3::ZERO, SetLeftFootOffset)
		_DECLCOMINFO_PARAM_END()
	};

	DEFINECOMFACTORY(IKEffectorOffsetComponent, UNMOVEABLE)
}



#endif

