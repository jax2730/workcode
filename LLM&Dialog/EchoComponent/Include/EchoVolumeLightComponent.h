// --------------------------------------------------------------------------------------
// File:   EchoVolumeLightComponent.h
// 
// Author: Wang YuXuan
//
// Date:   2024-1-31
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
// --------------------------------------------------------------------------------------

#ifndef __ECHO_VOLUMELIGHT_COMPONENT_H__
#define __ECHO_VOLUMELIGHT_COMPONENT_H__

#include "EchoPrerequisites.h"
#include "EchoComponentPrerequisites.h"
#include "EchoComDefineMacro.h"
#include "EchoColorValue.h"
#include "EchoRenderTarget.h"
#include "EchoVolumeLight.h"

namespace Echo {

	class VolumeLightComponentInfo;

	struct VolumeLightComponentContext 
	{
		float									mRaysAmount = 0.f;						//光线量
		float									mRaysAttenuation = 0.f;					//光线衰减
		float									mRaysColorIntensity = 0.f;				//体积光颜色强度
		ColorValue								mRaysCustomColor = ColorValue::White;	//体积光颜色
		int										mPriority = 0;

		bool                                    mIsUseVolumeLight = false;
	};

	class _EchoComponentExport VolumeLightSpace
	{
	public:
		const static Name VL_RAYAMOUNT;				//光线量
		const static Name VL_RAYSATTENUATION;		//光线衰减
		const static Name VL_RAYSCOLORINTENSITY;	//体积光颜色强度
		const static Name VL_RAYSCUSTOMCOLOR;		//体积光颜色
	};

	class TODComponentManager;
	class _EchoComponentExport VolumeLightComponent : public ActorComponent 
	{
		DECLAREACTORCOMTYPE_H(VolumeLightComponent)
	public:
		virtual void							init(ActorComponentInfo* info) override;
		virtual void							unInit() override;
	public:
		void									use();
		void									unuse();

		void									setPreferences(bool pOpenThis);
	public:
		void									setRaysAmount(float raysAmount);
		void									setRaysAttenuation(float raysAttenuation);
		void									setRaysColorIntensity(float raysColorIntensity);
		void									setRaysCustomColor(const ColorValue& color);
		void									setPriority(int priority);

		float									getRayAmount() { return mContext.mRaysAmount; }
		float									getRaysAttenuation() { return mContext.mRaysAttenuation; }
		float									getRaysColorIntensity() { return mContext.mRaysColorIntensity; }
		ColorValue								getRaysCustomColor() { return mContext.mRaysCustomColor; }
		int										getPriority() { return mContext.mPriority; }

		static VolumeLight*						getVolumeLight();
		static void								setIsOpenVolumeLight(bool IsOpen);

		bool									getOpenVolumeLightState();
	public:
		bool									isValidComponent();
		void									setSunShaftsParams(const VolumeLightComponentContext& context);

		virtual void						    setTimelineAttribute(std::vector<float>& data, const Name& name) override;
		virtual void                            visibleStateChanged(bool isVisible) override;
		TODComponentManager*                    getTodComManager() { return m_tMgr; }
	private:
		VolumeLightComponentContext				mContext;
		TODComponentManager*                    m_tMgr = nullptr;

		bool									mIsUsed = false;
	};

	using VolumeLightComponentPtr = EchoWeakPtr<VolumeLightComponent>;

	// ---------------------------------------------------------------------------------------
	class _EchoComponentExport VolumeLightComponentInfo : public ActorComponentInfo {
	public:
		VolumeLightComponentInfo() = default;
		VolumeLightComponentInfo(const Name& comType, ActorCOMMoveableType type);
		virtual ~VolumeLightComponentInfo() = default;

		void									ExportData(cJSON* jsonNode) override;
		void									ImportData(cJSON* jsonNode) override;
		void									CopyTo(ActorComponentInfo* comInfo) override;
		void									OverWrite() override;

	public:
		EDITOR_REFLECT

#define DEFAULT_MEMBER_OPERATION(name, type, data)	\
		void set##name(type value) { data.m##name = value; }	\
		type get##name() { return data.m##name; }

		DEFAULT_MEMBER_OPERATION(RaysAmount, float, mContext);
		DEFAULT_MEMBER_OPERATION(RaysAttenuation, float, mContext);
		DEFAULT_MEMBER_OPERATION(RaysColorIntensity, float, mContext);
		DEFAULT_MEMBER_OPERATION(RaysCustomColor, ColorValue, mContext);
		DEFAULT_MEMBER_OPERATION(Priority, int, mContext);
#undef DEFAULT_MEMBER_OPERATION

	public:
		VolumeLightComponentContext				mContext;
		bool									mOpenThis = true;
	};

	// ---------------------------------------------------------------------------------------
	DEFINECOMFACTORY(VolumeLightComponent, UNMOVEABLE)

};

#endif //__ECHO_VOLUMELIGHT_COMPONENT_H__
