// --------------------------------------------------------------------------------------
// File:   EchoSkyComponent.h
// 
// Author: Chen ShiWen
//
// Date:   2022-12-7
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
// --------------------------------------------------------------------------------------

#ifndef __ECHO_SKY_COMPONENT_H__
#define __ECHO_SKY_COMPONENT_H__

#include "EchoPrerequisites.h"
#include "EchoComponentPrerequisites.h"
#include "EchoComDefineMacro.h"

#include "EchoColorValue.h"

namespace Echo {

	class SkyComponentInfo;
	class TODComponentManager;
	struct SkyComponentContext {
	public:
		ColorValue								getBottomColor() const { return mBottomColor; }
		ColorValue								getTopColor() const { return mTopColor; }
		float									getColourRatio() const { return mColourRatio; }
		float									getFogHeight() const { return mFogHeight; }
		int										getPriority() const { return mPriority; }
		bool									getPGSkyEnable()const { return mEnablePGSky; }
		int										getPGSkyIndex()const { return mPGSkyIndex; }
		float									getAnimeSpeed() const { return mAnimeSpeed; }
		ColorValue								getLayer1Color() const { return mLayer1Color; }
		ColorValue								getLayer2Color() const { return mLayer2Color; }

		void									setBottomColor(const ColorValue& val) { mBottomColor = val; }
		void									setTopColor(const ColorValue& val) { mTopColor = val; }
		void									setColourRatio(float val) { mColourRatio = val; }
		void									setFogHeight(float val) { mFogHeight = val; }
		void									setPriority(int val) { mPriority = val; }
		void									setPGSkyIndex(int index) { mPGSkyIndex = index; }
		void									setPGSkyEnable(bool enable) { mEnablePGSky = enable; }
		void									setAnimeSpeed(float speed) { mAnimeSpeed = speed; }
		void									setLayer1Color(const ColorValue& color) { mLayer1Color = color; }
		void									setLayer2Color(const ColorValue& color) { mLayer2Color = color; }

		ColorValue								mBottomColor	= ColorValue::Red;
		ColorValue								mTopColor		= ColorValue::Red;
		float									mColourRatio	= 0.5f;
		float									mFogHeight		= 0.5f;
		int										mPriority		= 0;
		float                                   mTransFormTime  = 0.f;
		bool									mEnablePGSky    = false;
		int										mPGSkyIndex		= 0;
		float									mAnimeSpeed		= 1.0f;
		ColorValue								mLayer1Color	= ColorValue{ 1.0f, 0.5f, 0.0f, 1.0f };
		ColorValue								mLayer2Color	= ColorValue{ 0.0f, 0.5f, 1.0f, 1.0f };
	};

	class _EchoComponentExport SkyComponent : public ActorComponent {
		DECLAREACTORCOMTYPE_H(SkyComponent)
	public:
		void									init(ActorComponentInfo* info) override;
		virtual  void                           unInit() override;
	public:
		void									use();
		void									unuse();

	public:
		void									setBottomColor(const ColorValue& bottomColor);
		void									setTopColor(const ColorValue& topColor);
		void									setColourRatio(float colourRatio);
		void									setFogHeight(float fogHeight);
		void									setPriority(int priority);
		void									setPGSkyEnable(bool enable);
		void									setPGSkyIndex(int index);
		void									setAnimeSpeed(float speed);
		void									setLayer1Color(const ColorValue& color);
		void									setLayer2Color(const ColorValue& color);

	public:
		ColorValue								getBottomColor() const { return mContext.getBottomColor(); }
		ColorValue								getTopColor() const { return mContext.getTopColor(); }
		float									getColourRatio() const { return mContext.getColourRatio(); }
		float									getFogHeight() const { return mContext.getFogHeight(); }
		int										getPriority() const { return mContext.getPriority(); }
		bool									getPGSkyEnable()const { return mContext.getPGSkyEnable(); }
		int										getPGSkyIndex()const { return mContext.getPGSkyIndex(); }
		float									getAnimeSpeed() const { return mContext.getAnimeSpeed(); }
		ColorValue								getLayer1Color() const { return mContext.getLayer1Color(); }
		ColorValue								getLayer2Color() const { return mContext.getLayer2Color(); }
		
		void                                    setTransFormTime(float transTime_) { mContext.mTransFormTime = transTime_; }
		float                                   getTransFormTime() { return mContext.mTransFormTime; }
		TODComponentManager*                    getTodComManager() { return m_tMgr; }

		const SkyComponentContext&				getContext() { return mContext; }
	public:
		virtual void						    setTimelineAttribute(std::vector<float>& data, const Name& name) override;
		virtual void                            visibleStateChanged(bool isVisible) override;

	private:
		Sky*									m_Sky = nullptr;
		SkyComponentContext						mContext;
		TODComponentManager*                    m_tMgr = nullptr;
	};

	// ---------------------------------------------------------------------------------------
	class _EchoComponentExport SkyComponentInfo : public ActorComponentInfo {
	public:
		SkyComponentInfo() = default;
		SkyComponentInfo(const Name& comType, ActorCOMMoveableType type);
		virtual ~SkyComponentInfo() = default;

		void									ExportData(cJSON* jsonNode) override;
		void									ImportData(cJSON* jsonNode) override;
		void									CopyTo(ActorComponentInfo* comInfo) override;
		void									OverWrite() override;

	public:
		EDITOR_REFLECT

	public:
		ColorValue								getBottomColor() const { return mContext.mBottomColor; }
		ColorValue								getTopColor() const { return mContext.mTopColor; }
		float									getColourRatio() const { return mContext.mColourRatio; }
		float									getFogHeight() const { return mContext.mFogHeight; }
		int										getPriority() const { return mContext.mPriority; }
		bool									getPGSkyEnable()const { return mContext.mEnablePGSky; }
		int										getPGSkyIndex()const { return mContext.mPGSkyIndex; }

		void									setBottomColor(const ColorValue& val) { mContext.mBottomColor = val; }
		void									setTopColor(const ColorValue& val) { mContext.mTopColor = val; }
		void									setColourRatio(float val) { mContext.mColourRatio = val; }
		void									setFogHeight(float val) { mContext.mFogHeight = val; }
		void									setPriority(int val) { mContext.mPriority = val; }
		void									setPGSkyEnable(bool val) { mContext.mEnablePGSky = val; }
		void									setPGSkyIndex(int val) { mContext.mPGSkyIndex = val; }
	public:
		SkyComponentContext						mContext;
	};

	// ---------------------------------------------------------------------------------------
	DEFINECOMFACTORY(SkyComponent, UNMOVEABLE)

};

#endif //__ECHO_SKY_COMPONENT_H__
