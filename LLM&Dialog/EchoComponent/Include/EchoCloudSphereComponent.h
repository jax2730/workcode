// --------------------------------------------------------------------------------------
// File:   EchoCloudSphereComponent.h
// 
// Author: Chen ShiWen
//
// Date:   2022-12-12
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
// --------------------------------------------------------------------------------------
#ifndef __ECHO_CLOUDSPHERE_COMPONENT_H__
#define __ECHO_CLOUDSPHERE_COMPONENT_H__

#include "EchoPrerequisites.h"
#include "EchoComponentPrerequisites.h"
#include "EchoComDefineMacro.h"

#include "EchoColorValue.h"
#include "EchoTextureManager.h"

namespace Echo {

	class CloudSphere;
	class CloudSphereComponentInfo;
	class TODComponentManager;

	struct CloudSphereComponentContext {
		float                           mSpeed = 0.1f;
		float							mOpacity = 1.0f;
		float                           mEmendation = 1.f;
		ColorValue						mColour = ColorValue::White;
		float                           mIntensity = 1.f;
		float                           mHDRIntensity = 0.5f;
		ColorValue                      mBackColour =ColorValue::White;
		float                           mBackColorIntensity = 1.f;
		std::string						mTextureName = "";
		bool                            mCloudSphereVisible = true;
		std::string                     mMeshName = "";
		int								mPriority = 0;
		float                           mTransFormTime = 0.f;

	};

	// --------------------------------------------------------------------------------------
	class _EchoComponentExport CloudSphereComponent : public ActorComponent {
		DECLAREACTORCOMTYPE_H(CloudSphereComponent)
	public:		
		void							init(ActorComponentInfo* info) override;
		REGISTER_LUA_CLASS_KEY;
	public:
		static bool						isTextureAttribute(const Echo::Name& name);

		virtual   void                  unInit() override;
		virtual	  bool					isCreateFinish() override;
		void                            setSpeed(const float& speed_);
		void							setOpacity(const float& opacity);
		void                            setEmendation(const float& emendation_);
		void							setColour(const ColorValue& colour_);
		void                            setIntensity(const float& intensity_);
		void                            setHDRIntensity(const float& HDRIntensity_);
		void                            setBackColour(const ColorValue& backColour_);
		void                            setBackColorIntensity(const float& backColorIntensity_);
		void							setTextureName(const std::string& textureName);
		void                            setCloudSphereVisible(bool visible_);
		void                            setMeshName(const std::string& meshName_);
		void							setPriority(const int& priority);
		
		void                            setCloudSphereColour();
		void                            setCloudSphereBackColour();

		const                           float& getSpeed() const { return mContext.mSpeed; }
		const                           float& getOpacity() const { return mContext.mOpacity; }
		const                           float& getEmendation() const { return mContext.mEmendation; }
		const							ColorValue& getColour() const { return mContext.mColour; }
		const                           float& getIntensity() const { return mContext.mIntensity; }
		const                           float& getHDRIntensity() const { return mContext.mHDRIntensity; }
		const                           ColorValue& getBackColour() const { return mContext.mBackColour; }
		const                           float& getBackColorIntensity() const { return mContext.mBackColorIntensity; }
        const							std::string& getTextureName() const { return mContext.mTextureName; }
		bool                            getCloudSphereVisible() const { return mContext.mCloudSphereVisible; }
		const                           std::string& getMeshName() const { return mContext.mMeshName;  }
		int								getPriority() const { return mContext.mPriority; }

		void                            setCloudSphereRealVisible();
		void                            setTransFormTime(float transTime_) { mContext.mTransFormTime = transTime_; }
		float                           getTransFormTime() { return mContext.mTransFormTime; }
	public:
		void							use();
		void							unuse();
		void                            setIsUpdateTodTimeline(bool isUpdate) { mIsUpdateTodTimeline = isUpdate; }
		void							setTimelineAttribute(std::vector<float>& data, const Name& name) override;
		void                            setTimelineStringAttribute(const std::string& value, const Name& attrName) override;
		TODComponentManager*            getTodComManager() { return m_tMgr; }
		virtual  void                   visibleStateChanged(bool isVisible) override;
		virtual  void				    resourceArrived(const Name& resName, const Name& resType, bool bFail, const ResourceBasePtr& resourcePtr) override;
	protected:
		bool							_notifyFinish(const std::string& type);
		void							_preLoadTextureName(const std::string& textureName);
		void							_loadAndSetTextureName(const std::string& textureName);
		void							_prePrepareMeshName(const std::string& meshName);
	private:
		CloudSphere*					m_CloudSphere = nullptr;
		bool                            mIsUpdateTodTimeline = true;
		TODComponentManager*            m_tMgr = nullptr;

		int								m_bCreateKey = 0;
		bool							m_bCreateFinish = false;
#ifdef ECHO_EDITOR
		bool                            m_bNewCreateCom = true; //判断是否是新创建的组件
#endif
	public:
		CloudSphereComponentContext		mContext;
        
	};

	// --------------------------------------------------------------------------------------
	class _EchoComponentExport CloudSphereComponentInfo : public ActorComponentInfo {
	public:
		CloudSphereComponentInfo() = default;
		CloudSphereComponentInfo(const Name& comType, ActorCOMMoveableType type);
	
		void							ExportData(cJSON* jsonNode) override;
		void							ImportData(cJSON* jsonNode) override;
		void							CopyTo(ActorComponentInfo* comInfo) override;
		void							OverWrite() override;

#ifdef ECHO_EDITOR
		virtual bool                    getNeedCollectedResource(std::unordered_multimap<ResourceType, std::string>& resMap) override;
#endif

	public:
		EDITOR_REFLECT

		CloudSphereComponentContext		mContext;

#ifdef ECHO_EDITOR
		bool                            m_bNewCreateCom = true;
#endif
	};

	// --------------------------------------------------------------------------------------
	DEFINECOMFACTORY(CloudSphereComponent, UNMOVEABLE);
}


#endif // !__ECHO_CLOUDSPHERE_COMPONENT_H__


