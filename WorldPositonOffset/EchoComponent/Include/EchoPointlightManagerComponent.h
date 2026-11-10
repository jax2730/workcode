// --------------------------------------------------------------------------------------
// File:   EchoPointlightManagerComponent.h
// 
// Author: An HaoXiang
//
// Date:   2022-12-19
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
// --------------------------------------------------------------------------------------

#ifndef _EchoPointlightManagerComponent_h__
#define _EchoPointlightManagerComponent_h__
#include "EchoComponentPrerequisites.h"
#include "EchoMovableObject.h"
#include "EchoComDefineMacro.h"

namespace Echo {

	struct pLightManagerHandle {
		float			mLightPower = 1.f;	//全局点光源强度系数
		float           mSelfEmissiveLightPower = 1.f; //全局自发光材质强度系数
		ColorValue		mRolePointLightColor = ColorValue::White;//主角身上点光源颜色
		ColorValue		mRolePointLightColorLinear = ColorValue::White;//主角身上点光源颜色
		float			mRolePointLightPower = 5.f;//主角身上点光源强度
		bool            mIsUseRolePointLight = false;

		float			mTransFormTime = 0.f;
		int             mPriority = 0;
	};

	class _EchoComponentExport  PointlightManagerComponentInfo : public ActorComponentInfo 
	{
	public:
		PointlightManagerComponentInfo() {}
		PointlightManagerComponentInfo(const Name& comType, ActorCOMMoveableType type);
		virtual ~PointlightManagerComponentInfo();
		virtual void									ExportData(cJSON* jsonNode) override;
		virtual void									ImportData(cJSON* jsonNode) override;
		virtual void									CopyTo(ActorComponentInfo* comInfo) override;
		virtual void                                    OverWrite() override;
	public:
		pLightManagerHandle                             mContext;
		EDITOR_REFLECT
	};


	class TODComponentManager;
	class SceneManager;

	class _EchoComponentExport PointlightManagerComponent :public ActorComponent
	{
		DECLAREACTORCOMTYPE_H(PointlightManagerComponent)
	public:
		virtual void                                     init(ActorComponentInfo* info) override;
		virtual void                                     unInit() override;
		virtual void                                     copyTo(const ActorComponentPtr&) override;
		virtual void                                     setTimelineAttribute(std::vector<float>& data,const Name& name) override;
		virtual void                                     visibleStateChanged(bool isVisible) override;

		void                                             use();
		void                                             unuse();
		
		void                                             setGlobalPointLightPower(float value_);
		float                                            getGlobalPointLightPower() { return mContext.mLightPower; }
	
		void                                             setSelfEmissiveLightPower(float power_);
		float                                            getSelfEmissiveLightPower() { return mContext.mSelfEmissiveLightPower; }
		void                                             setIsUseRolePointLight(bool isUse);
		bool                                             getIsUseRolePointLight() { return mContext.mIsUseRolePointLight; }
		void                                             setRolePointLightColor(const ColorValue& color_);
		const ColorValue&                                getRolePointLightColor() { return mContext.mRolePointLightColorLinear; }
		void                                             setRolePointLightPower(float power_);
		float                                            getRolePointLightPower() { return mContext.mRolePointLightPower; }

		void                                             setPriority(int priority_);
		int                                              getPriority() { return mContext.mPriority; }
		void											 setTransFormTime(float time) { mContext.mTransFormTime = time; }
		float											 getTransFormTime() const { return mContext.mTransFormTime; }

		TODComponentManager*                             getTodComManager() { return mTodComMgr; }
	public:
		pLightManagerHandle                              mContext;
	private:
		TODComponentManager*                             mTodComMgr = nullptr;
		SceneManager*                                    mSceneMgr = nullptr;
	};

	DEFINECOMFACTORY(PointlightManagerComponent, UNMOVEABLE)
}

#endif
