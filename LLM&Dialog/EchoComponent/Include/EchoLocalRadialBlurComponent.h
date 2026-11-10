// --------------------------------------------------------------------------------------
// File:   EchoLocalRadialBlurComponent.h
// 
// Author: Wang Yuxuan
//
// Date:   2025-12-10
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
// --------------------------------------------------------------------------------------

#ifndef __Echo_LOCALRADIALBLUR_COMPONENT_H__
#define __Echo_LOCALRADIALBLUR_COMPONENT_H__

#include "EchoPrerequisites.h"
#include "EchoComponentPrerequisites.h"
#include "EchoComDefineMacro.h"

namespace Echo {
	struct LocalRadialBlurContext
	{
		//Vector3 mSphereCenter{ 90, 20, -111 };       // 模糊区域中心坐标
		float mSphereRadius = 10;                    // 模糊区域球体半径
		float mBlurStrength = 1.0f;                  // 模糊总强度。建议值 1.0，通过动画控制它的淡入淡出。
		float mJitterIntensity = 0.1f;               // 噪点抖动强度，控制光束的“毛刺感”。建议值 0.5 ~ 1.0。值越大，线条越碎。
		float mCenterHotspot = 3.f;                  // 中心聚集度。控制光晕在中心的集中程度。建议值 1.5 ~ 3.0。值越大，边缘衰减越快，中心越亮。
		float mRayLength = 0.05f;                    // 光束长度。决定爆炸射线的最大长度。建议 0.1 ~ 0.2。
		float mAberration = 0.02f;                   // 色散偏移量。专门控制蓝色/紫色通道分离的距离。建议值 0.05 ~ 0.1。
		float mHollowRange = 0.15f;                  // 控制中心清晰区域的大小。范围 0 ~ 1
	};

	class LocalRadialBlur;

	class _EchoComponentExport LocalRadialBlurComponent : public NodeActorComponent
	{
		DECLAREACTORCOMTYPE_H(LocalRadialBlurComponent)
	public:
		void									init(ActorComponentInfo* info) override;
		virtual void							unInit() override;
		virtual void							connectCom() override;
		void									setPosition(const DBVector3& pos, bool updateInstantly = true, bool redirectParent = false);
	public:
		void									use();
		void									unuse();

	public:
		void									setBlurCenter(const Vector3& value);
		void									setBlurRadius(float value);
		void									setBlurStrength(float value);
		void									setJitterIntensity(float value);
		void									setCenterHotspot(float value);
		void									setRayLength(float value);
		void									setAberration(float value);
		void									setHollowRange(float value);
		void									setOpen(bool open);

		bool									isOpen() { return mIsOpen; }

		const LocalRadialBlurContext&			getLocalRadialBlurContext() { return mContext; }
	public:
		virtual void						    setTimelineAttribute(std::vector<float>& data, const Name& name) override;
		virtual void                            visibleStateChanged(bool isVisible) override;

	private:
		LocalRadialBlur*						_getLocalRadiaBlur(class PostProcessChain*& pProcessChain);
		bool									_isValidComponent();
		void									_setLocalRadialBlurContextData();
	private:
		LocalRadialBlurContext					mContext;
		bool									mIsOpen = false;
	public:
		static Name LRB_SPHERECENTER;       // 模糊区域中心坐标
		static Name LRB_SPHERERADIUS;       // 模糊区域球体半径
		static Name LRB_BLURSTRENGTH;       // 模糊总强度
		static Name LRB_JITTERINTENSITY;    // 噪点抖动强度
		static Name LRB_CENTERHOTSPOT;      // 中心聚集度
		static Name LRB_RAYLENGTH;          // 光束长度
		static Name LRB_ABERRATION;         // 色散偏移量
		static Name LRB_HOLLOWRANGE;        // 控制中心清晰区域的大小
		static Name LRB_OPEN;				// 启用
	};

	// ---------------------------------------------------------------------------------------
	class _EchoComponentExport LocalRadialBlurComponentInfo : public ActorComponentInfo
	{
	public:
		LocalRadialBlurComponentInfo() = default;
		LocalRadialBlurComponentInfo(const Name& comType, ActorCOMMoveableType type);
		virtual ~LocalRadialBlurComponentInfo() = default;

		void									ExportData(cJSON* jsonNode) override;
		void									ImportData(cJSON* jsonNode) override;
		void									CopyTo(ActorComponentInfo* comInfo) override;
		void									OverWrite() override;

	public:
		EDITOR_REFLECT

	public:
		//const Vector3& getBlurCenter() const	{ return mContext.mSphereCenter; }
		float getBlurRadius() const				{ return mContext.mSphereRadius; }
		float getBlurStrength() const			{ return mContext.mBlurStrength; }
		float getJitterIntensity() const		{ return mContext.mJitterIntensity; }
		float getCenterHotspot() const			{ return mContext.mCenterHotspot; }
		float getRayLength() const				{ return mContext.mRayLength; }
		float getAberration() const				{ return mContext.mAberration; }
		float getHollowRange() const			{ return mContext.mHollowRange; }


		//void setBlurCenter(const Vector3& value){ mContext.mSphereCenter = value; }
		void setBlurRadius(float value)			{ mContext.mSphereRadius = value; }
		void setBlurStrength(float value)		{ mContext.mBlurStrength = value; }
		void setJitterIntensity(float value)	{ mContext.mJitterIntensity = value; }
		void setCenterHotspot(float value)		{ mContext.mCenterHotspot = value; }
		void setRayLength(float value)			{ mContext.mRayLength = value; }
		void setAberration(float value)			{ mContext.mAberration = value; }
		void setHollowRange(float value)		{ mContext.mHollowRange = value; }

	public:
		LocalRadialBlurContext					mContext;
		bool									mIsOpen = false;
	};

	// ---------------------------------------------------------------------------------------
	DEFINECOMFACTORY(LocalRadialBlurComponent, ALL_MOVEABLE)
};

#endif //__Echo_LOCALRADIALBLUR_COMPONENT_H__
