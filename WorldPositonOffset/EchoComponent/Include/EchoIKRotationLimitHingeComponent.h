#ifndef __ECHOIKROTATIONLIMITHINGECOMPONENT_H__
#define __ECHOIKROTATIONLIMITHINGECOMPONENT_H__

#include "EchoIKBaseComponent.h"
#include "EchoRole.h"
#include "EchoComponentPrerequisites.h"
#include "EchoProtocolComponent.h"
#include "Actor/EchoActor.h"
#include "EchoHelperObj.h"

namespace Echo
{

	struct IKRotationLimitHingeComponentData : public IKBaseComponentData {
		RotationLimitHingeInfo mIKInfo;
		FinalIKInfoBase* GetIKInfo() override { return &mIKInfo; };
	};

	class _EchoComponentExport IKRotationLimitHingeComponent : public IKBaseComponent {
		DECLAREACTORCOMTYPE_H(IKRotationLimitHingeComponent)
			REGISTER_LUA_CLASS_KEY;
	public:
		virtual void unInit() override;
		virtual void SetIKInfo(FinalIKInfoBase* pInfo) override;
		virtual void UseDefaultInitialize() override;
		virtual bool IsCanCreate() override;
		void SetIKInfoData(const RotationLimitHingeInfo& info);
		const RotationLimitHingeInfo& GetIKInfoData();
		void SetHelperVisible(bool value);
		bool GetHelperVisible();
		void SetHelperScale(float value);
		float GetHelperScale();
		void SetHelperOffset(const Vector3& offset);
		const Vector3& GetHelperOffset();
		virtual void showEditorHelper(bool isShow) override;
		virtual void UpdateComponentInEditor() override;
		inline IKBaseComponentData* CreateCompData() override
		{
			return new IKRotationLimitHingeComponentData();
		}
	public:
		const Name& GetBone();
		void SetBone(const Name& value);
		const Vector3& GetAxisOffset();
		void SetAxisOffset(const Vector3& value);
		float GetMin();
		void SetMin(float value);
		float GetMax();
		void SetMax(float value);
		bool GetUseLimits();
		void SetUseLimits(bool value);
		bool GetUseDefaultLocalRotation();
		void SetUseDefaultLocalRotation(bool value);
#ifdef USE_IK_HELPEROBJ
	protected:
		void CreateHelper();
		void ReleaseHelper();
		void UpdateHelper();
	private:
		bool mIsUpdateEditor = true;
		bool mIsHelperVisible = true;
		HelperObject* m_pHelperObj = nullptr;
		HelperObject* m_pHelperObj2 = nullptr;
		float m_fHelperScale = 1.0f;
		Vector3 m_HelperOffset = Vector3::ZERO;
		SceneNode* pHelperRootNode = nullptr;
#endif // USE_IK_HELPEROBJ
	};

	class _EchoComponentExport IKRotationLimitHingeComponentInfo : public IKBaseComponentInfo {
	public:
		IKRotationLimitHingeComponentInfo() {}
		IKRotationLimitHingeComponentInfo(const Name& comType, ActorCOMMoveableType moveable)
			: IKBaseComponentInfo(comType, moveable) {}
		virtual ~IKRotationLimitHingeComponentInfo() {}
	public:
		virtual void CopyToCompData(IKBaseComponentData* pData) override;
		virtual void CompDataToInfo(IKBaseComponentData* pData, IKBaseComponent* pComp) override;


		_DECLCOMINFO_VIRFUN
	public:
		_DECLCOMINFO_PARAM_BEGIN(IKRotationLimitHingeComponent)
		_DECLCOMINFO_PARAMS(Name, Bone, Name::g_cEmptyName);
		_DECLCOMINFO_PARAMS(Vector3, AxisOffset, Vector3::ZERO);
		_DECLCOMINFO_PARAMS_SIM(float, Min, 0.0f);
		_DECLCOMINFO_PARAMS_SIM(float, Max, 0.0f);
		_DECLCOMINFO_PARAMS_SIM(bool, UseLimits, true);
		_DECLCOMINFO_PARAMS_SIM(bool, UseDefaultLocalRotation, false);
		_DECLCOMINFO_PARAM_END()
	};

	DEFINECOMFACTORY(IKRotationLimitHingeComponent, UNMOVEABLE)

}

#endif //__ECHOIKROTATIONLIMITHINGECOMPONENT_H__
 



