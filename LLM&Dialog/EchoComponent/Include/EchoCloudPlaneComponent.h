// --------------------------------------------------------------------------------------
// File:   EchoCloudPlaneComponent.h
// 
// Author: Chen ShiWen
//
// Date:   2022-12-12
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
// --------------------------------------------------------------------------------------
#ifndef __ECHO_CLOUDPLANE_COMPONENT_H__
#define __ECHO_CLOUDPLANE_COMPONENT_H__

#include "EchoPrerequisites.h"
#include "EchoComponentPrerequisites.h"
#include "EchoComDefineMacro.h"

#include "EchoColorValue.h"

namespace Echo {

	class CloudPlane;
	class CloudPlaneComponentInfo;
	class TODComponentManager;

	struct CloudPlaneComponentContext {
		bool                            mCloudPlaneVisible = true;
		std::string						mTextureName = "";     //白天用的贴图
		std::string                     mStarsTextureName = "";  //晚上用的贴图
		ColorValue						mColour		= ColorValue(1.0, 1.0, 1.0);
		float                           mIntensity  = 1.f;
		float                           mHDRIntensity = 1.f;
		ColorValue                      mBackColor  = ColorValue(1.0, 1.0, 1.0);
		float                           mBackColorIntensity = 1.f;
		float							mOpacity	= 1.f;
		float							mHeight		= 10.f;
		float                           mURollSpeed = 0.1f;
		float                           mVRollSpeed = 0.1f;
		float                           mUVTilling = 1.f;
		float                           mEmendation = 1.f;
		int								mPriority	= 0;
		float                           mTransFormTime = 0.f;
	};

	// --------------------------------------------------------------------------------------
	class _EchoComponentExport CloudPlaneComponent : public ActorComponent {
		DECLAREACTORCOMTYPE_H(CloudPlaneComponent)
	public:
		void							init(ActorComponentInfo* info) override;
		virtual void		            unInit() override;
		virtual bool					isCreateFinish() override { return m_bCreateFinish; }
		virtual  void				    resourceArrived(const Name& resName, const Name& resType, bool bFail, const ResourceBasePtr& resourcePtr) override;
	public:
		bool                            getCloudPlaneVisible() const { return mContext.mCloudPlaneVisible; }
		const std::string&				getTextureName() const { return mContext.mTextureName; }
		const std::string&              getStarsTextureName() const { return mContext.mStarsTextureName; }
		const ColorValue&				getColour() const { return mContext.mColour; }
		const float&                    getIntensity() const { return mContext.mIntensity; }
		const float&                    getHDRIntensity() const { return mContext.mHDRIntensity; }
		const ColorValue&               getBackColor() const { return mContext.mBackColor; }
		const float&                    getBackColorIntensity() const { return mContext.mBackColorIntensity; }
		const float&					getOpacity() const { return mContext.mOpacity; }
		const float&					getHeight() const { return mContext.mHeight; }
		const float&                    getURollSpeed() const { return mContext.mURollSpeed; }
		const float&                    getVRollSpeed() const { return mContext.mVRollSpeed; }
		const float&                    getUVTilling() const { return mContext.mUVTilling; }
		const float&                    getEmendation() const { return mContext.mEmendation; }
		const int&						getPriority() const { return mContext.mPriority; }

	public:
		void                            setCloudPlaneVisible(bool visible_);
		void							setTextureName(const std::string& textureName);
		void                            setStarsTextureName(const std::string& texName);

		void							setColour(const ColorValue& colour);
		void                            setIntensity(const float& intensity_);
		void                            setHDRIntensity(const float& hdrIntensity_);
		void                            setBackColor(const ColorValue& backColor_);
		void                            setBackColorIntensity(const float& backIntensity);
		void							setOpacity(const float& opacity);
		void							setHeight(const float& height);
		void                            setURollSpeed(const float& uSpeed);
		void                            setVRollSpeed(const float& vSpeed);
		void                            setUVTilling(const float& uvTilling);
		void                            setEmendation(const float& emendation);
		void							setPriority(int priority);

		void                            setCloudPlaneColor();
		void                            setCloudPlaneBackColor();
		void                            setTransFormTime(float transTime_) { mContext.mTransFormTime = transTime_; }
		float                           getTransFormTime() { return mContext.mTransFormTime; }

		bool                            getIsMoonRise();

		//void                            setCloudPlaneRealVisible();
	public:
		void							use();
		void							unuse();
		void							setTimelineAttribute(std::vector<float>& data, const Name& name) override;
		void                            setIsUpdateTodTimeline(bool isUpdate) { mIsUpdateTodTimeline = isUpdate; }

		virtual   void                  visibleStateChanged(bool isVisible) override;
		TODComponentManager*            getTodComManager() { return m_tMgr; }
	protected:
		bool							_notifyFinish(int type);
		void							_preLoadTexture(const std::string& textureName, int idx);
		void							_loadAndSetTextureName(const std::string& textureName);
		void							_loadAndSetStarsTextureName(const std::string& textureName);
	private:
		CloudPlane*						m_CloudPlane = nullptr;
		bool                            mIsUpdateTodTimeline = true;
		TODComponentManager*            m_tMgr = nullptr;
		bool							m_bCreateFinish = false;
		int								m_bCreateTex = 0;
	public:
		CloudPlaneComponentContext		mContext;
#ifdef ECHO_EDITOR
		bool                            m_bNewCreateCom = true; //判断是否是新创建的组件
#endif
	};

	// --------------------------------------------------------------------------------------
	class _EchoComponentExport CloudPlaneComponentInfo : public ActorComponentInfo {
	public:
		CloudPlaneComponentInfo() = default;
		CloudPlaneComponentInfo(const Name& comType, ActorCOMMoveableType type);

		void							ExportData(cJSON* jsonNode) override;
		void							ImportData(cJSON* jsonNode) override;
		void							CopyTo(ActorComponentInfo* comInfo) override;
		void							OverWrite() override;

#ifdef ECHO_EDITOR
		virtual bool                    getNeedCollectedResource(std::unordered_multimap<ResourceType, std::string>& resMap) override;
#endif

	public:
		EDITOR_REFLECT
		CloudPlaneComponentContext		mContext;
#ifdef ECHO_EDITOR
		bool                            m_bNewCreateCom = true; //判断是否是新创建的组件
#endif
	};

	// --------------------------------------------------------------------------------------
	DEFINECOMFACTORY(CloudPlaneComponent, UNMOVEABLE)

}


#endif // !__ECHO_CLOUDPLANE_COMPONENT_H__


