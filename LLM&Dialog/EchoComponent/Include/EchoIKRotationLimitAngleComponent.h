#ifndef __ECHOIKROTATIONLIMITANGLECOMPONENT_H__
#define __ECHOIKROTATIONLIMITANGLECOMPONENT_H__

#include "EchoIKBaseComponent.h"
#include "EchoRole.h"
#include "EchoComponentPrerequisites.h"
#include "EchoProtocolComponent.h"
#include "Actor/EchoActor.h"
#include "EchoHelperObj.h"

namespace Echo
{
	
	struct IKRotationLimitAngleComponentData : public IKBaseComponentData {
		RotationLimitAngleInfo mIKInfo;
		FinalIKInfoBase* GetIKInfo() override { return &mIKInfo; };
	};

	class _EchoComponentExport IKRotationLimitAngleComponent : public IKBaseComponent {
		DECLAREACTORCOMTYPE_H(IKRotationLimitAngleComponent)
			REGISTER_LUA_CLASS_KEY;
	public:
		virtual void unInit() override;
		virtual void SetIKInfo(FinalIKInfoBase* pInfo) override;
		virtual void UseDefaultInitialize() override;
		virtual bool IsCanCreate() override;
		void SetIKInfoData(const RotationLimitAngleInfo& info);
		const RotationLimitAngleInfo& GetIKInfoData();
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
			return new IKRotationLimitAngleComponentData();
		}
	public:
		const Name& GetBone();
		void SetBone(const Name& value);
		const Vector3& GetAxisOffset();
		void SetAxisOffset(const Vector3& value);
		float GetLimit();
		void SetLimit(float value);
		float GetTwistLimit();
		void SetTwistLimit(float value);
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

	class _EchoComponentExport IKRotationLimitAngleComponentInfo : public IKBaseComponentInfo {
	public:
		IKRotationLimitAngleComponentInfo() {}
		IKRotationLimitAngleComponentInfo(const Name& comType, ActorCOMMoveableType moveable)
			: IKBaseComponentInfo(comType, moveable) {}
		virtual ~IKRotationLimitAngleComponentInfo() {}
	public:
		virtual void CopyToCompData(IKBaseComponentData* pData) override;
		virtual void CompDataToInfo(IKBaseComponentData* pData, IKBaseComponent* pComp) override;


		_DECLCOMINFO_VIRFUN
	public:
		_DECLCOMINFO_PARAM_BEGIN(IKRotationLimitAngleComponent)
		_DECLCOMINFO_PARAMS(Name, Bone, Name::g_cEmptyName);
		_DECLCOMINFO_PARAMS(Vector3, AxisOffset, Vector3::ZERO);
		_DECLCOMINFO_PARAMS_SIM(float, Limit, 0.0f);
		_DECLCOMINFO_PARAMS_SIM(float, TwistLimit, 0.0f);
		_DECLCOMINFO_PARAMS_SIM(bool, UseDefaultLocalRotation, false);
		_DECLCOMINFO_PARAM_END()
	};

	DEFINECOMFACTORY(IKRotationLimitAngleComponent, UNMOVEABLE)

}

#endif //__ECHOIKROTATIONLIMITANGLECOMPONENT_H__
 



