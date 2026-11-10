// --------------------------------------------------------------------------------------
// File:   EchoFogComponent.h
// 
// Author: An HaoXiang
//
// Date:   2022-12-19
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
// --------------------------------------------------------------------------------------

#ifndef _EchoFogComponent_h__
#define _EchoFogComponent_h__
#include "EchoComponentPrerequisites.h"
#include "EchoProtocolComponent.h"
#include "Actor/EchoActor.h"
#include "EchoMovableObject.h"

#include "EchoComInfoDefineMacro.h"
#include "EchoHelperObj.h"
#include "EchoHelperObjProFuncImp.h"
#include "EchoComDefineMacro.h"

namespace Echo {

	class TODComponentManager;
	class SceneManager;

	struct FogComDataHandle {
		ColorValue  mFirstNearFogColor = ColorValue(1.f, 1.f, 1.f, 1.f);
		ColorValue  mFirstFarFogColor = ColorValue(1.f, 1.f, 1.f, 1.f);
		ColorValue  mSecondNearFogColor = ColorValue(1.f, 1.f, 1.f, 1.f);
		ColorValue  mSecondFarFogColor = ColorValue(1.f, 1.f, 1.f, 1.f);
		ColorValue	mThirdNearFogColor = ColorValue(1.f, 1.f, 1.f, 1.f);
		ColorValue	mThirdFarFogColor = ColorValue(1.f, 1.f, 1.f, 1.f);
		ColorValue	mFourthNearFogColor = ColorValue(1.f, 1.f, 1.f, 1.f);
		ColorValue	mFourthFarFogColor = ColorValue(1.f, 1.f, 1.f, 1.f);
		Vector4		mNearFogColorIntensity = Vector4(1.f, 1.f, 1.f, 1.f);
		Vector4     mFarFogColorIntensity = Vector4(1.f, 1.f, 1.f, 1.f);
		Vector4     mNearDensity = Vector4(0.1f, 0.1f, 0.1f, 0.1f);
		Vector4	    mFarDensity = Vector4(0.1f, 0.1f, 0.1f, 0.1f);
		Vector4     mNearTopHeight = Vector4(10.f, 10.f, 10.f, 10.f);
		Vector4	    mFarTopHeight = Vector4(10.f, 10.f, 10.f, 10.f);
		Vector4	    mNearRampStart = Vector4(200.f, 200.f, 200.f, 200.f);
		Vector4	    mFarRampStart = Vector4(1000.f, 1000.f, 1000.f, 1000.f);
		Vector4	    mNearRampEnd = Vector4(1000.f, 1000.f, 1000.f, 1000.f);
		Vector4	    mFarRampEnd = Vector4(2000.f, 3000.f, 4000.f, 4000.f);

		float       mTransFormTime = 0.f;
		int	        mPriority = 0;
    };

	class _EchoComponentExport FogComponentInfo:public ActorComponentInfo {
	public:
		FogComponentInfo(){}
		FogComponentInfo(const Name& comType, ActorCOMMoveableType type);
		virtual ~FogComponentInfo();
		virtual void									ExportData(cJSON* jsonNode) override;
		virtual void									ImportData(cJSON* jsonNode) override;
		virtual void									CopyTo(ActorComponentInfo* comInfo) override;
		virtual void                                    Overlay(ActorComponentInfo* srcInfo) override;
		virtual void                                    OverWrite() override;
	public:
		FogComDataHandle                                mContext;
		EDITOR_REFLECT
	};
	class _EchoComponentExport FogComponent :public ActorComponent {
		DECLAREACTORCOMTYPE_H(FogComponent)
	public:
		REGISTER_LUA_CLASS_KEY;
		void						            init(ActorComponentInfo* info) override;
		virtual void                            unInit() override;

		virtual void							copyTo(const ActorComponentPtr&) override;
		virtual void                            setTimelineAttribute(std::vector<float>& data,const Name& name) override;
		virtual void                            visibleStateChanged(bool isVisible) override;


		void                                    setInitFirstNearFogColor(const ColorValue& color_);
		void                                    setInitFirstNearFogPower(float power_);
		void                                    setInitFirstFarFogColor(const ColorValue& color_);
		void                                    setInitFirstFarFogPower(float power_);
		void                                    setInitSecondNearFogColor(const ColorValue& color_);
		void                                    setInitSecondNearFogPower(float power_);
		void                                    setInitSecondFarFogColor(const ColorValue& color_);
		void                                    setInitSecondFarFogPower(float power_);
		void                                    setInitThirdNearFogColor(const ColorValue& color_);
		void                                    setInitThirdNearFogPower(float power_);
		void                                    setInitThirdFarFogColor(const ColorValue& color_);
		void                                    setInitThirdFarFogPower(float power_);
		void                                    setInitFourthNearFogColor(const ColorValue& color_);
		void                                    setInitFourthNearFogPower(float power_);
		void                                    setInitFourthFarFogColor(const ColorValue& color_);
		void                                    setInitFourthFarFogPower(float power_);
		
		void                                    setFirstNearFogColor();
		void                                    setFirstFarFogColor();
		void                                    setSecondNearFogColor();
		void                                    setSecondFarFogColor();
		void                                    setThirdNearFogColor();
		void                                    setThirdFarFogColor();
		void                                    setFourthNearFogColor();
		void                                    setFourthFarFogColor();

		void                                    setNearDensity(const Vector4& nearDensity_);
		void                                    setFarDensity(const Vector4& farDensity_);
		void                                    setNearTopHeight(const Vector4& nearTopHeight_);
		void                                    setFarTopHeight(const Vector4& farTopHeight_);
		void                                    setNearRampStart(const Vector4& nearRampStart_);
		void                                    setFarRampStart(const Vector4& farRampStart_);
		void                                    setNearRampEnd(const Vector4& nearRampEnd_);
		void                                    setFarRampEnd(const Vector4& farRampEnd_);

		void                                    setMainSceneFog();
		void                                    setRealTime(bool isUse_);
		bool                                    getRealTime();
		int                                     getPriority();
		void                                    setPriority(int priority_);

		float                                   getTransFormTime() { return mContext.mTransFormTime; }
		void                                    setTransFormTime(float transTime_) { mContext.mTransFormTime = transTime_; }
		void                                    updateTodItemState(bool flag_);
		TODComponentManager*                    getTodComManager() { return m_tMgr; }
	public:
		FogComDataHandle                        mContext;
	private:
		ColorValue                              mFogColor[8];
		float                                   mDensity[8];
		float                                   mRampStart[8];
		float                                   mRampEnd[8];
		float                                   mTopHeight[8];

		TODComponentManager*                    m_tMgr = nullptr;
		SceneManager*                           m_sMgr = nullptr;
	};

	DEFINECOMFACTORY(FogComponent, UNMOVEABLE)
		
};

#endif
