// --------------------------------------------------------------------------------------
// File:   EchoCloudMeshComponent.h
// 
// Author: Chen ShiWen
//
// Date:   2022-12-12
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
// --------------------------------------------------------------------------------------
#ifndef __ECHO_CLOUDMESH_COMPONENT_H__
#define __ECHO_CLOUDMESH_COMPONENT_H__

#include "EchoPrerequisites.h"
#include "EchoComponentPrerequisites.h"
#include "EchoComDefineMacro.h"
#include "EchoTextureManager.h"

namespace Echo {

	class CloudMesh;
	class CloudMeshComponentInfo;

	struct CloudMeshComponentContext {
		std::vector<std::string>   mTextureName;
		std::vector<std::string>   mMeshName;
		std::vector<ColorValue>    mColor;
		std::vector<ColorValue>    mBackColor;
		std::vector<float>         mOpacity;
		std::vector<float>         mEmendation;
		std::vector<float>         mIntensity;
		std::vector<float>         mHDRIntensity;
		std::vector<float>         mBackColorIntensity;
		std::vector<unsigned int>  mIsShowBackColor;

		std::vector<float>         mTopHeight;
		std::vector<float>         mBottomHeight;
		std::vector<float>         mCloudMeshScale;

		std::vector<float>         mURollSpeed;
		std::vector<float>         mVRollSpeed;
		std::vector<float>         mUVTilling;
		std::vector<unsigned int>  mIsUseOriginalUV;
		std::vector<unsigned int>  mIsUseDistort;
		std::vector<float>         mDistortSpeed;
		std::vector<float>         mDistortAmplitude;
		std::vector<float>         mDistortScale;
		std::vector<int32>         mRenderPriority;
		std::vector<bool>		   mCloudRealVisible;	// 实际显隐

		float                      mCloudBorderSoft = 50.f;
		
		bool					   mCloudVisible;
		int						   mPriority = 0;
		float                      mTransFormTime = 0.f;

		/*bool                       mUpMeshTextureOrMeshEmpty = false;
		bool                       mMidMeshTextureOrMeshEmpty = false;
		bool                       mDownMeshTextureOrMeshEmpty = false;*/

		std::vector<float>         m_UVFactory;
		CloudMeshComponentContext() {
			mTextureName.reserve(3);
			mTextureName.resize(3);
			mMeshName.reserve(3);
			mMeshName.resize(3);
			mColor.reserve(3);
			mColor.resize(3,ColorValue(1.f,1.f,1.f,1.f));
			mBackColor.reserve(3);
			mBackColor.resize(3, ColorValue(1.f, 1.f, 1.f, 1.f));
			mOpacity.reserve(3);
			mOpacity.resize(3,1.f);
			mEmendation.reserve(3);
			mEmendation.resize(3,1.f);
			mIntensity.reserve(3);
			mIntensity.resize(3,1.f);
			mHDRIntensity.reserve(3);
			mHDRIntensity.resize(3,0.5f);
			mBackColorIntensity.reserve(3);
			mBackColorIntensity.resize(3,1.f);
			mIsShowBackColor.reserve(3);
			mIsShowBackColor.resize(3, 1);
			mTopHeight.reserve(3);
			mTopHeight.resize(3, 1500.f);
			mBottomHeight.reserve(3);
			mBottomHeight.resize(3, 1000);
			mCloudMeshScale.reserve(3);
			mCloudMeshScale.resize(3,3.f);
		 
			mURollSpeed.reserve(3);
			mURollSpeed.resize(3,0.01f);
			mVRollSpeed.reserve(3);
			mVRollSpeed.resize(3,0.01f);
			mUVTilling.reserve(3);
			mUVTilling.resize(3,1.f);
			mIsUseOriginalUV.reserve(3);
			mIsUseOriginalUV.resize(3, 0);
			mIsUseDistort.reserve(3);
			mIsUseDistort.resize(3, 0);
			mDistortSpeed.reserve(3);
			mDistortSpeed.resize(3,0.f);
			mDistortAmplitude.reserve(3);
			mDistortAmplitude.resize(3, 0.f);
			mDistortScale.reserve(3);
			mDistortScale.resize(3, 0.f);
			mRenderPriority.reserve(3);
			mRenderPriority.resize(3, 1000);
			mCloudRealVisible.resize(3, true);
			m_UVFactory.resize(3, 1.f);
		}
	};

	struct CloudMeshHandle 
	{
		CloudMesh*        mCloudUpMesh;
		CloudMesh*        mCloudMidMesh;
		CloudMesh*        mCloudDownMesh;
	};

	class TODComponentManager;
	// --------------------------------------------------------------------------------------
	class _EchoComponentExport CloudMeshComponent : public ActorComponent {
		DECLAREACTORCOMTYPE_H(CloudMeshComponent)
	public:
		static	  bool						isTextureAttribute(const Echo::Name& name);

		void								init(ActorComponentInfo* info) override;
		virtual   void                      unInit() override;
		virtual   void                      visibleStateChanged(bool visible_) override;
		virtual	  bool						isCreateFinish() override { return m_bCreateFinish; }
		virtual   void				        resourceArrived(const Name& resName, const Name& resType, bool bFail, const ResourceBasePtr& resourcePtr) override;
	public:
		void								setTextureName(const std::vector<std::string>& textureName,bool isUpdate = true);

		bool                                loadTextureData(const std::string& textureName, TexturePtr& texPtr_);
		bool								prepareMeshData(const std::string& meshName, MeshPtr& meshPtr);

		void                                setMeshName(const std::vector<std::string>& meshName, bool isUpdate = true);
		void								setColor(const std::vector<ColorValue>& color, bool isUpdate = true);
		void                                setBackColor(const std::vector<ColorValue>& backColor, bool isUpdate = true);
		void                                setCloudColor(bool isUpdate = true);
		void                                setCloudBackColor(bool isUpdate = true);
		void                                setOpacity(const std::vector<float>& opacity, bool isUpdate = true);
		void                                setEmendation(const std::vector<float>& emendation, bool isUpdate = true);
		void                                setIntensity(const std::vector<float>& intensity, bool isUpdate = true);
		void                                setHDRIntensity(const std::vector<float>& HDRIntensity, bool isUpdate = true);
		void                                setBackColorIntensity(const std::vector<float>& backColorIntensity, bool isUpdate = true);
		void                                setIsShowBackColor(const std::vector<unsigned int>& isShowBackColor, bool isUpdate = true);
		void								setPriority(int priority);
		void                                setTopHeight(const std::vector<float>& topHeight, bool isUpdate = true);
		void                                setBottomHeight(const std::vector<float>& bottomHeight, bool isUpdate = true);
		void                                setCloudMeshHeight(bool isUpdate = true);
		void                                setCloudMeshScale(const std::vector<float>& cloudMeshScale, bool isUpdate = true);
		
		void                                setURollSpeed(const std::vector<float>& uRollSpeed, bool isUpdate = true);
		void                                setVRollSpeed(const std::vector<float>& vRollSpeed, bool isUpdate = true);
		void                                setUVTilling(const std::vector<float>& uvTilling, bool isUpdate = true);
		void                                setIsUseOriginalUV(const std::vector<unsigned int>& isUseOriUV);
		void                                setIsUseDistort(const std::vector<unsigned int>& isUseDistort);
		void                                setDistortSpeed(const std::vector<float>& distortSpeed, bool isUpdate = true);
		void                                setDistortAmplitude(const std::vector<float>& distortAmplitude, bool isUpdate = true);
		void                                setDistortScale(const std::vector<float>& distortScale, bool isUpdate = true);
		void                                setRenderPriority(const std::vector<int32>& renderPriority);
		void                                setCloudBorderSoft(const float& cloudBorderSoft, bool isUpdate = true);
		void								setCloudVisible(bool isShow);
		void                                setUVFactory(const std::vector<float>& uvFactory);
		const std::vector<float>&           getUVFactory() { return mContext.m_UVFactory; }

		void                                setTransFormTime(float transTime_); 
		float                               getTransFormTime() { return mContext.mTransFormTime; }

		const std::vector<std::string>&		getTextureName() const { return mContext.mTextureName; }
		const std::vector<std::string>&     getMeshName() const { return mContext.mMeshName; }
		const std::vector<ColorValue>&		getColor() const { return mContext.mColor; }
		const std::vector<ColorValue>&      getBackColor() const { return mContext.mBackColor; }
		const std::vector<float>&			getOpacity() const { return mContext.mOpacity; }
		const std::vector<float>&           getEmendation() const { return mContext.mEmendation; }
		const std::vector<float>&           getIntensity() const { return mContext.mIntensity; }
		const std::vector<float>&           getHDRIntensity() const { return mContext.mHDRIntensity; }
		const std::vector<float>&           getBackColorIntensity() const { return mContext.mBackColorIntensity; }
		const std::vector<unsigned int>&    getIsShowBackColor() const { return mContext.mIsShowBackColor; }
		const int&							getPriority() const { return mContext.mPriority; }
		const std::vector<float>&           getTopHeight() const { return mContext.mTopHeight; }
		const std::vector<float>&           getBottomHeight() const { return mContext.mBottomHeight; }
		const std::vector<float>&           getCloudMeshScale() const { return mContext.mCloudMeshScale; }

		const std::vector<float>&           getURollSpeed() const  { return mContext.mURollSpeed; }
		const std::vector<float>&           getVRollSpeed() const  { return mContext.mVRollSpeed; }
		const std::vector<float>&           getUVTilling() const { return mContext.mUVTilling;  }
		const std::vector<unsigned int>&    getIsUseOriginalUV() const { return mContext.mIsUseOriginalUV; }
		const std::vector<unsigned int>&    getIsUseDistort() const { return mContext.mIsUseDistort; }
		const std::vector<float>&           getDistortSpeed() const { return mContext.mDistortSpeed; }
		const std::vector<float>&           getDistortAmplitude() const { return mContext.mDistortAmplitude; }
		const std::vector<float>&           getDistortScale() const { return mContext.mDistortScale; }
		const std::vector<int32>&           getRenderPriority() const { return mContext.mRenderPriority; }
		const float&                        getCloudBorderSoft() const { return mContext.mCloudBorderSoft; }
		const std::vector<bool>&			getCloudVisible() const { return mContext.mCloudRealVisible; }
		bool								getCloudGlobalVisible() const { return mContext.mCloudVisible; }


		void                                setUpMeshOrTextureNameState();
		void                                setMidMeshOrTextureNameState();
		void                                setDownMeshOrTextureNameState();
	private:
		void                                updateCloudMeshMaterial(CloudMesh* cloudMesh_ ,const int& idx,bool isUpdate = true);

		
		void								_loadAndSetTextureIdx(int idx);
		void								_preLoadTextureIdx(int idx);
		void								_prePerpareMeshIdx(int idx);
		bool								_notifyFinish(int type, const String& Data);
#ifdef ECHO_EDITOR
		void                                _initFirstCreateMeshAndTexData();
#endif
	public:
		void								use();
		void								unuse();
		void                                setCloudMeshAtribute();
		void                                setIsUpdateTodTimeline(bool isUpdate) { mIsUpdateTodTimeline = isUpdate; }
		void								setTimelineAttribute(std::vector<float>& data, const Name& name) override;
		void                                setTimelineStringAttribute(const std::string& value, const Name& attrName) override;
		TODComponentManager*                getTodComManager() { return m_tMgr; }
	public:
		CloudMeshComponentContext			mContext;
	private:
		CloudMesh*							mCloudUpMesh = nullptr;
		CloudMesh*                          mCloudMidMesh = nullptr;
		CloudMesh*                          mCloudDownMesh = nullptr;
		bool                                mIsUpdateTodTimeline = true;
		TODComponentManager*                m_tMgr = nullptr;
		bool								m_bCreateFinish = false;
		int									m_bCreateKey = 0;
#ifdef ECHO_EDITOR
		bool                                m_bNewCreateCom = true;
#endif
	};

	// --------------------------------------------------------------------------------------
	class _EchoComponentExport CloudMeshComponentInfo : public ActorComponentInfo {
	public:
		CloudMeshComponentInfo() = default;
		CloudMeshComponentInfo(const Name& comType, ActorCOMMoveableType type);

		void								ExportData(cJSON* jsonNode) override;
		void								ImportData(cJSON* jsonNode) override;
		void								CopyTo(ActorComponentInfo* comInfo) override;
		void								OverWrite() override;

#ifdef ECHO_EDITOR
		virtual bool                        getNeedCollectedResource(std::unordered_multimap<ResourceType, std::string>& resMap) override;
#endif

	public:
		EDITOR_REFLECT

	public:
		CloudMeshComponentContext			mContext;
#ifdef ECHO_EDITOR
		bool                                m_bNewCreateCom = true;
#endif
	};
	// --------------------------------------------------------------------------------------
	DEFINECOMFACTORY(CloudMeshComponent, UNMOVEABLE)
}

#endif // !__ECHO_CLOUDMESH_COMPONENT_H__


