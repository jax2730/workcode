#pragma once

#include "EchoPrerequisites.h"
#include "EchoComponentPrerequisites.h"
#include "EchoComDefineMacro.h"

/*
此组件不存属性值，仅使用从TODTimelineComponent传递过来的值。
*/

namespace Echo
{
	class TODComponentManager;
	class SphericalOcean;

	struct SphericalOceanContext
	{
		ColorValue	mWaterShallowColor	= ColorValue(42.f / 255.f, 179.f / 255.f, 197.f / 255.f, 1.0f);	//浅海颜色
		ColorValue	mWaterDeepColor		= ColorValue(23.f / 255.f, 105.f / 255.f, 253.f / 255.f, 1.0f);	//深海颜色
		ColorValue	mWaterFoamColor		= ColorValue::White;											//泡沫颜色
		ColorValue	mWaterReflectColor	= ColorValue::Blue;												//水反射颜色
		ColorValue	mUnderWaterColor	= ColorValue(0.514f, 0.749f, 0.839f, 1.f);						//在水下时，水的颜色
		ColorValue	mUnderWaterFogColor = ColorValue(0.031f, 0.467f, 0.529f, 1.f);						//水下雾颜色

		int			mPriority			= 0;	// 优先级
		float       mTransFormTime		= 0.f;	// 过渡时间
	};

	class _EchoComponentExport SphericalOceanComponent : public ActorComponent
	{
		DECLAREACTORCOMTYPE_H(SphericalOceanComponent)
	public:
		void									init(ActorComponentInfo* info) override;
		virtual  void                           unInit() override;
	public:
		void									use();
		void									unuse();

	public:		// get
		const ColorValue&						GetWaterShallowColor()	{ return mContext.mWaterShallowColor; }
		const ColorValue&						GetWaterDeepColor()		{ return mContext.mWaterDeepColor; }
		const ColorValue&						GetWaterFoamColor()		{ return mContext.mWaterFoamColor; }
		const ColorValue&						GetWaterReflectColor()	{ return mContext.mWaterReflectColor; }
		const ColorValue&						GetUnderWaterColor()	{ return mContext.mUnderWaterColor; }
		const ColorValue&						GetUnderWaterFogColor() { return mContext.mUnderWaterFogColor; }

		SphericalOceanContext					GetContext()			{ return mContext; }
		int										getPriority()			{ return mContext.mPriority; }
		TODComponentManager*					GetTodComManager()		{ return mTMgr; }
	public:		// set
		void									SetWaterShallowColor(const ColorValue& value);
		void									SetWaterDeepColor(const ColorValue& value);
		void									SetWaterFoamColor(const ColorValue& value);
		void									SetWaterReflectColor(const ColorValue& value);
		void									SetUnderWaterColor(const ColorValue& value);
		void									SetUnderWaterFogColor(const ColorValue& value);

		void									setPriority(int value);
		void									setTransFormTime(float value);
	public:		// tod
		virtual void						    setTimelineAttribute(std::vector<float>& data, const Name& name) override;
		virtual void                            visibleStateChanged(bool isVisible) override;
	public:
		SphericalOcean*							GetCurrentSphericalOcean();
		bool									isCurrentActiveCom();
	private:
		TODComponentManager*					mTMgr;
		SphericalOceanContext					mContext;

	public:
		static String							WATERSHALLOWCOLOR;			//浅海颜色
		static String							WATERDEEPCOLOR;				//深海颜色
		static String							WATERFOAMCOLOR;				//泡沫颜色
		static String							WATERREFLECTCOLOR;			//水反射颜色
		static String							UNDERWATERCOLOR;			//在水下时，水的颜色
		static String							UNDERWATERFOGCOLOR;			//水下雾颜色
		static String							PRIORITY;					//优先级
		static String							TRANSFORMTIME;				//过渡时间
	};
	
	// ---------------------------------------------------------------------------------------
	class _EchoComponentExport SphericalOceanComponentInfo : public ActorComponentInfo {
	public:
		SphericalOceanComponentInfo() = default;
		SphericalOceanComponentInfo(const Name& comType, ActorCOMMoveableType type);
		virtual ~SphericalOceanComponentInfo() = default;

		void									ExportData(cJSON* jsonNode) override;
		void									ImportData(cJSON* jsonNode) override;
		void									CopyTo(ActorComponentInfo* comInfo) override;
		void									OverWrite() override;

	public:
		EDITOR_REFLECT

	public:
		SphericalOceanContext						mContext;
	};

	// ---------------------------------------------------------------------------------------
	DEFINECOMFACTORY(SphericalOceanComponent, UNMOVEABLE)
}
