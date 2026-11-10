#ifndef _ECHOIKRECOILCOMPONNET_H_
#define _ECHOIKRECOILCOMPONNET_H_

#include "EchoComponentPrerequisites.h"
#include "EchoProtocolComponent.h"
#include "Actor/EchoActor.h"
#include "EchoIKBaseComponent.h"
#include "EchoRole.h"
#include "EchoQuaternion.h"
#include "EchoIKAimIKComponent.h"
#include "EchoAnimationCurve.h"
#include "EchoWeakPtr.h"

namespace Echo
{

	struct IKRecoilComponentData : public IKBaseComponentData {
		IKRecoilInfo mIKInfo;
		String AimIKName;
		FinalIKInfoBase* GetIKInfo() override { return &mIKInfo; }
	};

	class _EchoComponentExport IKRecoilComponent : public IKBaseComponent {
		DECLAREACTORCOMTYPE_H(IKRecoilComponent)
			REGISTER_LUA_CLASS_KEY;
	public:
		virtual void SetCompData(IKBaseComponentData* pData) override;
		virtual void SetIKInfo(FinalIKInfoBase* pInfo) override;
		virtual void UseDefaultInitialize() override;
		virtual bool IsCanCreate() override;
		void SetIKInfoData(const IKRecoilInfo& info);
		const IKRecoilInfo& GetIKInfoData();
		inline IKBaseComponentData* CreateCompData() override
		{
			return new IKRecoilComponentData();
		}
	public:
		void SetFullBodyBipedIKName(const Name& ikName);
		const Name& GetFullBodyBipedIKName();
		const String& GetAimIKName();
		uint32 GetHandness();
		bool GetTwoHanded();
		float GetMagnitudeRandom();
		const Vector3& GetRotationRandom();
		const Vector3& GetHandRotationOffset();
		float GetBlendTime();
		const std::vector<IKRecoilInfo::Offsets>& GetOffsets();
		const std::vector<Vector2>& GetRecoilWeight();

		void SetAimIKName(const Name& value) {}
		void SetAimIKName(const String& value);
		void SetHandness(uint32 value);
		void SetTwoHanded(bool value);
		void SetMagnitudeRandom(float value);
		void SetRotationRandom(const Vector3& value);
		void SetHandRotationOffset(const Vector3& value);
		void SetBlendTime(float value);
		void SetOffsets(const std::vector<IKRecoilInfo::Offsets>& value);
		void SetRecoilWeight(const std::vector<Vector2>& value);

		void Fire(float magnitude);
		EchoIK::FullBodyBipedIK* GetFullBodyBipedIK();
	public:
		void SetAimIKTarget(const IKAimIKComponentPtr& pComp);
		void ApplyAimIKTargetPtr();
	protected:
		virtual void PostCreateIK() override;
		virtual void PreDestroyIK() override;
		IKAimIKComponentPtr mAimIKPtr = nullptr;
	};

	class _EchoComponentExport IKRecoilComponentInfo : public IKBaseComponentInfo {

	public:
		IKRecoilComponentInfo() {}
		IKRecoilComponentInfo(const Name& comType, ActorCOMMoveableType moveable)
			: IKBaseComponentInfo(comType, moveable) {}

	public:
		virtual ~IKRecoilComponentInfo() {}
		virtual void CopyToCompData(IKBaseComponentData* pData) override;
		virtual void CompDataToInfo(IKBaseComponentData* pData, IKBaseComponent* pComp) override;


		_DECLCOMINFO_VIRFUN
	public:
		_DECLCOMINFO_PARAM_BEGIN(IKRecoilComponent)
		_DECLCOMINFO_PARAM(Name, FullBodyBipedIKName, Name::g_cEmptyName, SetFullBodyBipedIKName)
		_DECLCOMINFO_PARAM(String, AimIKName, "", SetAimIKName)
		_DECLCOMINFO_PARAM_SIM(uint32, Handness, 0, SetHandness)
		_DECLCOMINFO_PARAM_SIM(bool, TwoHanded, true, SetTwoHanded)
		_DECLCOMINFO_PARAM_SIM(float, MagnitudeRandom, 0.1f, SetMagnitudeRandom)
		_DECLCOMINFO_PARAM(Vector3, RotationRandom, Vector3::ZERO, SetRotationRandom)
		_DECLCOMINFO_PARAM(Vector3, HandRotationOffset, Vector3::ZERO, SetHandRotationOffset)
		_DECLCOMINFO_PARAM_SIM(float, BlendTime, 0.1f, SetBlendTime)
		_DECLCOMINFO_PARAM(std::vector<IKRecoilInfo::Offsets>, Offsets, {}, SetOffsets)
		_DECLCOMINFO_PARAM(std::vector<Vector2>, RecoilWeight, {}, SetRecoilWeight)
		_DECLCOMINFO_PARAM_END()
	};

	DEFINECOMFACTORY(IKRecoilComponent, UNMOVEABLE)
}

#endif // _ECHOIKRECOIL_H_


