#ifndef _EchoDirecLightComponent_h__
#define _EchoDirecLightComponent_h__

#include "EchoCommonStableHeaders.h"
#include "EchoComponentPrerequisites.h"
#include "EchoProtocolComponent.h"

#include "Actor/EchoActor.h"
#include "EchoComInfoDefineMacro.h"
#include "EchoSceneManager.h"
#include "EchoComDefineMacro.h"


namespace Echo
{
	class DirecLightComponentInfo;
	class TODComponentManager;

	struct DirecLightInfoData {
		Vector4   mPBRMainLightColor = Vector4(1.f, 1.f, 1.f, 1.f);
		Vector4   mPBRSkyLightColor = Vector4(1.f, 1.f, 1.f, 1.f);
		Vector4   mPBRGroundLightColor = Vector4(1.f, 1.f, 1.f, 1.f);
		float	  mPBRAmbientBlendThreshold = 1.f;
		Vector4   mRoleMainLightColor = Vector4(1.f, 1.f, 1.f, 1.f);
		Vector4	  mRoleSkyLightColor = Vector4(1.f, 1.f, 1.f, 1.f);
		Vector4   mRoleGroundLightColor = Vector4(1.f, 1.f, 1.f, 1.f);
		float	  mRoleAmbientBlendThreshold = 1.f;

		int	      mPriority = 0;
		float     mPBRMainLightPower = 1.f;
		float	  mPBRSkyLightPower = 1.f;
		float	  mPBRGroundLightPower = 1.f;
		float	  mRoleMainLightPower = 1.f;
		float	  mRoleSkyLightPower = 1.f;
		float	  mRoleGroundLightPower = 1.f;
		float	  mMainLightYawAngle = 0.f;
		float	  mMainLightPitchAngle = 0.f;

		ColorValue mMainLightColor = ColorValue::White;
		float      mMainLightColorPower = 5.f;
		ColorValue mMainSkyLightColor = ColorValue::White;
		float      mMainSkyLightPower = 1.f;
		ColorValue mMainGroundLightColor = ColorValue::White;
		float      mMainGroundLightPower = 1.f;

		float     mMainLightHDRPower = 1.f;
		float     mMainSkyLightHDRPower = 1.f;
		float     mMainGroundLightHDRPower = 1.f;
		float     mPBRMainLightHDRPower = 1.f;
		float     mPBRSkyLightHDRPower = 1.f;
		float     mPBRGroundLightHDRPower = 1.f;
		float     mRoleMainLightHDRPower = 1.f;
		float     mRoleSkyLightHDRPower = 1.f;
		float     mRoleGroundLightHDRPower = 1.f;

		float     m_ViewLightPercent = 0.05f;
		float     m_PBRViewLightPercent = 0.05f;
		float     m_RoleViewLightPercent = 0.05f;
		float     mTransFormTime = 0.f;
	};

	//=============================================================================
	class _EchoComponentExport DirecLightComponent : public ActorComponent
	{
		DECLAREACTORCOMTYPE_H(DirecLightComponent)
	public:
		REGISTER_LUA_CLASS_KEY;

		virtual void							init(ActorComponentInfo* info) override;
		virtual void                            unInit() override;
		virtual void							copyTo(const ActorComponentPtr&) override;
		virtual void                            setTimelineAttribute(std::vector<float>&, const Name&) override;
		virtual void                            visibleStateChanged(bool isVisible) override;

		void                                    setMainLightYawAngle(float yawAngle_);
		void                                    setMainLightPitchAngle(float pitchAngle_);

		void 								    setInitPBRMainLight(const Vector4& color_);
		void								    setInitPBRSkyLight(const Vector4& color_);
		void								    setInitPBRGroundLight(const Vector4& color_);
		void								    setInitRoleMainLight(const Vector4& color_);
		void								    setInitRoleSkyLight(const Vector4& color_);
		void								    setInitRoleGroundLight(const Vector4& color_);
		void                                    setInitMainLightColor(const ColorValue& color_);
		void                                    setInitMainSkyLightColor(const ColorValue& color_);
		void                                    setInitMainGroundLightColor(const ColorValue& color_);
		

		void 									setPBRMainLightPower(float power_);
		void 									setPBRSkyLightPower(float power_);
		void 									setPBRGroundLightPower(float power_);
		void									setPBRAmbientBlendThreshold(float);
		void 									setRoleMainLightPower(float power_);
		void 									setRoleSkyLightPower(float power_);
		void 									setRoleGroundLightPower(float power_);
		void									setRoleAmbientBlendThreshold(float);
		void                                    setMainSkyLightPower(float power_);
        void                                    setMainLightPower(float power_);
		void                                    setMainGroundLightPower(float power_);

		void									setviewLightPercent();
		void                                    setViewLightPercent(float percent_, bool isOld = false);
		float                                   getViewLightPercent() { return mContext.m_ViewLightPercent; }
		void									setPBRViewLightPercent(float percent_);
		float									getPBRViewLightPercent() { return mContext.m_PBRViewLightPercent; }
		void									setRoleViewLightPercent(float percent_);
		float									getRoleViewLightPercent() { return mContext.m_RoleViewLightPercent; }

		Vector4								    getInitPBRMainLight();
		Vector4								    getInitPBRSkyLight();
		Vector4								    getInitPBRGroundLight();
		Vector4								    getInitRoleMainLight();
		Vector4								    getInitRoleSkyLight();
		Vector4								    getInitRoleGroundLight();
		const ColorValue&                       getInitMainLightColor();
		float                                   getMainLightPower();
		const ColorValue&                       getInitMainSkyLightColor();
		float                                   getMainSkyLightPower();
		const ColorValue&                       getInitMainGroundLightColor();
		float                                   getMainGroundLightPower();

		//HDR
		void                                    setMainLightHDRPower(float power_);
		float                                   getMainLightHDRPower();
		void                                    setMainSkyLightHDRPower(float power_);
		float                                   getMainSkyLightHDRPower();
		void                                    setMainGroundLightHDRPower(float power_);
		float                                   getMainGroundLightHDRPower();
		void                                    setPBRMainLightHDRPower(float power_);
		float                                   getPBRMainLightHDRPower();
		void                                    setPBRSkyLightHDRPower(float power_);
		float                                   getPBRSkyLightHDRPower();
		void                                    setPBRGroundLightHDRPower(float power_);
		float                                   getPBRGroundLightHDRPower();
		void                                    setRoleMainLightHDRPower(float power_);
		float                                   getRoleMainLightHDRPower();
		void                                    setRoleSkyLightHDRPower(float power_);
		float                                   getRoleSkyLightHDRPower();
        void                                    setRoleGroundLightHDRPower(float power_);
		float                                   getRoleGroundLightHDRPower();

		//设置场景
		void                                    setMainLightColor();
		void                                    setMainSkyLightColor();
		void                                    setMainGroundLightColor();
		void									setPBRMainLightColor();
		void									setPBRSkyLightColor();
		void									setPBRGroundLightColor();
		void									setRoleMainLightColor();
		void									setRoleSkyLightColor();
		void 									setRoleGroundLightColor();

		// use for temporary script
		void									setPriority(int priori);
		int 									getPriority();

		float 									getPBRMainLightPower();
		float 									getPBRSkyLightPower();
		float 									getPBRGroundLightPower();
		float									getPBRAmbientBlendThreshold();
		float 									getRoleMainLightPower();
		float 									getRoleSkyLightPower();
		float 									getRoleGroundLightPower();
		float									getRoleAmbientBlendThreshold();
		void                                    setMainLightDirection();

		void                                    setRealTime(bool flag);
		bool                                    getRealTime();

		void                                    setScreenAttribute();
		void                                    updateTodItemState(bool flag_);

		void                                    setTransFormTime(float transTime_) { mContext.mTransFormTime = transTime_; }
		float                                   getTransFormTime() { return mContext.mTransFormTime; }
		TODComponentManager*                    getTodComManager() { return m_tMgr; }
	public:
		DirecLightInfoData                      mContext;
		TODComponentManager*                    m_tMgr = nullptr;
	};

	class _EchoComponentExport DirecLightComponentInfo : public ActorComponentInfo
	{
	public:
		DirecLightComponentInfo(){}
		DirecLightComponentInfo(const Name& comType, ActorCOMMoveableType type);
		virtual ~DirecLightComponentInfo();
		virtual void									ExportData(cJSON* jsonNode) override;
		virtual void									ImportData(cJSON* jsonNode) override;
		virtual void									CopyTo(ActorComponentInfo* comInfo) override;
		virtual void                                    Overlay(ActorComponentInfo* srcInfo) override;
		virtual void                                    OverWrite() override;
	private:
		int												mVersion = 1;
	public:
		DirecLightInfoData                              mContext;
		EDITOR_REFLECT
	};
   

	DEFINECOMFACTORY(DirecLightComponent, UNMOVEABLE)

}
// namespace Echo
#endif 
