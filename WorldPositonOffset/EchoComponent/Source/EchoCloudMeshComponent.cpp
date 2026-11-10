// --------------------------------------------------------------------------------------
// File:   EchoCloudMeshComponent.cpp
// 
// Author: Chen ShiWen
//
// Date:   2022-12-12
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
// --------------------------------------------------------------------------------------
#include "EchoComponentStableHeaders.h"
#include "EchoCloudMeshComponent.h"

#include "EchoCloud.h"
#include "EchoTODComponentManager.h"
#include "EchoRenderStrategy.h"
#include "EchoPostProcess.h"
#include "EchoTODManager.h"
#include "EchoLogManager.h"
#include "EchoTODComponentSystem.h"
#include "EchoSceneManager.h"
#include "EchoTODDataUtil.h"
#include "EchoMeshMgr.h"

namespace Echo {


	Name cloudMeshTextureUpName_ = Name("CloudMeshTextureName0");
	Name cloudMeshTextureMidName_ = Name("CloudMeshTextureName1");
	Name cloudMeshTextureDownName_ = Name("CloudMeshTextureName2");
	
	Name cloudMeshUpColorName_ = Name("CloudMeshColor0");
	Name cloudMeshMidColorName_ = Name("CloudMeshColor1");
	Name cloudMeshDownColorName_ = Name("CloudMeshColor2");

	Name cloudMeshUpBackColorName_ = Name("BackColor0");
	Name cloudMeshMidBackColorName_ = Name("BackColor1");
	Name cloudMeshDownBackColorName_ = Name("BackColor2");

	Name cloudMeshUpOpacityName_ = Name("Opacity0");
	Name cloudMeshMidOpacityName_ = Name("Opacity1");
	Name cloudMeshDownOpacityName_ = Name("Opacity2");

	Name cloudMeshUpEmendationName_ = Name("Emendation0");
	Name cloudMeshMidEmendationName_ = Name("Emendation1");
	Name cloudMeshDownEmendationName_ = Name("Emendation2");

	Name cloudMeshUpIntensityName_ = Name("Intensity0");
	Name cloudMeshMidIntensityName_ = Name("Intensity1");
	Name cloudMeshDownIntensityName_ = Name("Intensity2");

	Name cloudMeshUpHDRIntensityName_ = Name("HDRIntensity0");
	Name cloudMeshMidHDRIntensityName_ = Name("HDRIntensity1");
	Name cloudMeshDownHDRIntensityName_ = Name("HDRIntensity2");

	Name cloudMeshUpBackColorIntensityName_ = Name("BackColorIntensity0");
	Name cloudMeshMidBackColorIntensityName_ = Name("BackColorIntensity1");
	Name cloudMeshDownBackColorIntensityName_ = Name("BackColorIntensity2");

	const String RESTEXTURE = "Texture";
	const String RESMESH = "Mesh";

	DECLAREACTORCOMTYPE_CPP(CloudMeshComponent);

	CloudMeshComponent::CloudMeshComponent(ActorPtr pActor, ActorCOMMoveableType movableType) :
		ActorComponent(pActor, movableType)
	{
#ifdef ECHO_EDITOR
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_CLOUD_MESH_UP_OPACITY,				cloudMeshUpOpacityName_.toString(),				1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_CLOUD_MESH_UP_EMENDATION,			cloudMeshUpEmendationName_.toString(),			1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_CLOUD_MESH_UP_COLOR,				cloudMeshUpColorName_.toString(),				4);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_CLOUD_MESH_UP_COLOR_POWER,			cloudMeshUpIntensityName_.toString(),			1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_CLOUD_MESH_UP_HDR_POWER,			cloudMeshUpHDRIntensityName_.toString(),		1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_CLOUD_MESH_UP_SUNBACK_COLOR,		cloudMeshUpBackColorName_.toString(),			4);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_CLOUD_MESH_UP_SUNBACK_COLOR_POWER, cloudMeshUpBackColorIntensityName_.toString(),	1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_CLOUD_MESH_UP_TEXTURE,				cloudMeshTextureUpName_.toString(),				1);

		TODDataUtil::SetTodTemplate(TodDataID::PARAM_CLOUD_MESH_MID_OPACITY,				cloudMeshMidOpacityName_.toString(),			1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_CLOUD_MESH_MID_EMENDATION,				cloudMeshMidEmendationName_.toString(),			1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_CLOUD_MESH_MID_COLOR,					cloudMeshMidColorName_.toString(),				4);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_CLOUD_MESH_MID_COLOR_POWER,			cloudMeshMidIntensityName_.toString(),			1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_CLOUD_MESH_MID_HDR_POWER,				cloudMeshMidHDRIntensityName_.toString(),		1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_CLOUD_MESH_MID_SUNBACK_COLOR,			cloudMeshMidBackColorName_.toString(),			4);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_CLOUD_MESH_MID_SUNBACK_COLOR_POWER,	cloudMeshMidBackColorIntensityName_.toString(),	1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_CLOUD_MESH_MID_TEXTURE,				cloudMeshTextureMidName_.toString(),			1);

		TODDataUtil::SetTodTemplate(TodDataID::PARAM_CLOUD_MESH_DOWN_OPACITY,				cloudMeshDownOpacityName_.toString(),				1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_CLOUD_MESH_DOWN_EMENDATION,			cloudMeshDownEmendationName_.toString(),			1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_CLOUD_MESH_DOWN_COLOR,					cloudMeshDownColorName_.toString(),					4);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_CLOUD_MESH_DOWN_COLOR_POWER,			cloudMeshDownIntensityName_.toString(),				1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_CLOUD_MESH_DOWN_HDR_POWER,				cloudMeshDownHDRIntensityName_.toString(),			1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_CLOUD_MESH_DOWN_SUNBACK_COLOR,			cloudMeshDownBackColorName_.toString(),				4);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_CLOUD_MESH_DOWN_SUNBACK_COLOR_POWER,	cloudMeshDownBackColorIntensityName_.toString(),	1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_CLOUD_MESH_DOWN_TEXTURE,				cloudMeshTextureDownName_.toString(),				1);
#endif // ECHO_EDITOR
	}

	CloudMeshComponent::~CloudMeshComponent()
	{
	}

	bool CloudMeshComponent::isTextureAttribute(const Name& name)
	{
		return name == cloudMeshTextureUpName_ ||
			name == cloudMeshTextureMidName_ ||
			name == cloudMeshTextureDownName_;
	}


	void CloudMeshComponent::init(ActorComponentInfo* info)
	{
		if (info != nullptr)
		{
			CloudMeshComponentInfo* mCloudComInfo = static_cast<CloudMeshComponentInfo*>(info);
			mContext = mCloudComInfo->mContext;
			mContext.mCloudRealVisible[0] = mContext.mCloudVisible;
			mContext.mCloudRealVisible[1] = mContext.mCloudVisible;
			mContext.mCloudRealVisible[2] = mContext.mCloudVisible;

#ifdef ECHO_EDITOR
			m_bNewCreateCom = mCloudComInfo->m_bNewCreateCom;
#endif
		}

		TODCOMPONENT_INITMGR(getCloudMeshContext, m_tMgr, true);

		if (m_tMgr == nullptr)
		{
			m_bCreateFinish = true;
			LogManager::instance()->logMessage("CloudMeshComponent::init TODComponentManager object does not exist!");
			return;
		}

		if (Root::instance()->getRuntimeMode() == Root::eRM_Editor)
			m_bCreateFinish = true;

		if (TODSystem::instance()->showLog(LogType::LT_CloudMesh))
		{
			stringstream ss;
			ss << mContext.mMeshName[0] << " | " << mContext.mMeshName[1] << " | " << mContext.mMeshName[2];
			LogManager::instance()->logMessage("[CloudMesh Init] Prepare Mesh : " + ss.str());
			ss.str("");
			ss << mContext.mTextureName[0] << " | " << mContext.mTextureName[1] << " | " << mContext.mTextureName[2];
			LogManager::instance()->logMessage("[CloudMesh Init] Prepare Mesh : " + ss.str());
		}

		_preLoadTextureIdx(eCloudType_Mesh_Up);
		_preLoadTextureIdx(eCloudType_Mesh_Mid);
		_preLoadTextureIdx(eCloudType_Mesh_Down);

		_prePerpareMeshIdx(eCloudType_Mesh_Up);
		_prePerpareMeshIdx(eCloudType_Mesh_Mid);
		_prePerpareMeshIdx(eCloudType_Mesh_Down);
	}

	void CloudMeshComponent::unInit()
	{
		TODCOMPONENT_UNINITMGR(getCloudMeshContext, m_tMgr, T_CloudMesh);
	}

	void CloudMeshComponent::visibleStateChanged(bool isVisible)
	{
		if (!m_tMgr)
			return;
		if (isVisible)
			m_tMgr->pushComponent(m_tMgr->getCloudMeshContext(), this);
		else
			m_tMgr->popComponent(m_tMgr->getCloudMeshContext(), this);
	}

	void CloudMeshComponent::resourceArrived(const Name& resName, const Name& resType, bool bFail, const ResourceBasePtr& resourcePtr)
	{
		if (resourcePtr.isNull())
		{
			std::string logInfo = "";
			logInfo.append("Texture :").append(resName.c_str()).append(" load false");
			LogManager::getSingleton().logMessage(Echo::LML_CRITICAL, logInfo.data());
			return;
		}

		if (TODSystem::instance()->showLog(LogType::LT_CloudMesh))
			LogManager::instance()->logMessage("[CloudMesh resourceArrived] Resource : " + resName.toString());

		if (m_tMgr == nullptr)
			return;

		if (resType == TextureManager::instance()->getResourceType())
		{
			const std::string& texName_ = resName.c_str();

			if (!resourcePtr.isNull() && resourcePtr->isPrepared())
				resourcePtr->load();

			mTextureRes.insert(std::make_pair(texName_, resourcePtr));
			if (texName_ == mContext.mTextureName[0])
				_notifyFinish(0, RESTEXTURE);
			if (texName_ == mContext.mTextureName[1])
				_notifyFinish(1, RESTEXTURE);
			if (texName_ == mContext.mTextureName[2])
				_notifyFinish(2, RESTEXTURE);
		}
		else if (resType == MeshMgr::instance()->getResourceType())
		{
			const std::string& meshName = resName.c_str();

			mMeshRes.insert(std::make_pair(meshName, resourcePtr));
			if (meshName == mContext.mMeshName[0])
				_notifyFinish(0, RESMESH);
			if (meshName == mContext.mMeshName[1])
				_notifyFinish(1, RESMESH);
			if (meshName == mContext.mMeshName[2])
				_notifyFinish(2, RESMESH);
		}
	}

	void CloudMeshComponent::setTextureName(const std::vector<std::string>& texName, bool isUpdate)
	{
		mContext.mTextureName = texName;
		if (m_tMgr == nullptr)
			return;
		_loadAndSetTextureIdx(eCloudType_Mesh_Up);
		_loadAndSetTextureIdx(eCloudType_Mesh_Mid);
		_loadAndSetTextureIdx(eCloudType_Mesh_Down);
	}

	bool CloudMeshComponent::loadTextureData(const std::string& textureName)
	{
		if (!m_tMgr) return false;

		if (mTextureRes.find(textureName) != mTextureRes.end()) return true;

		if(TextureManager::instance()->resourceExists(Name(textureName)))
		{
			mTextureRes.insert(std::make_pair(textureName, TextureManager::instance()->getByName(Name(textureName))));
			return true;
		}

		if (Root::instance()->getRuntimeMode() != Root::eRM_Editor)
		{
			ResourceLoadPriority priority = WorkQueue::MakePriority(100, WorkQueue::RESOURCE_TYPE_SKY);
			requestResouce(Name(textureName), TextureManager::instance()->getResourceType(), priority);
			return false;
		}

		mTextureRes.insert(std::make_pair(textureName, TextureManager::instance()->load(Name(textureName))));
		return true;
	}

	bool CloudMeshComponent::prepareMeshData(const std::string& meshName)
	{
		if (!m_tMgr) return false;

		if (mMeshRes.find(meshName) != mMeshRes.end()) return true;

		if (MeshMgr::instance()->resourceExists(Name(meshName)))
		{
			mMeshRes.insert(std::make_pair(meshName, MeshMgr::instance()->getByName(Name(meshName))));
			return true;
		}

		if (Root::instance()->getRuntimeMode() != Root::eRM_Editor)
		{
			ResourceLoadPriority priority = WorkQueue::MakePriority(100, WorkQueue::RESOURCE_TYPE_SKY);
			requestResouce(Name(meshName), MeshMgr::instance()->getResourceType(), priority);
			SetLoadResourceState(false);
			return false;
		}

		mMeshRes.insert(std::make_pair(meshName, MeshMgr::instance()->prepare(Name(meshName))));
		return true;
	}

	void CloudMeshComponent::setMeshName(const std::vector<std::string>& meshName,bool isUpdateMaterial)
	{
		mContext.mMeshName = meshName;

		bool needUpdateTexture = false;
		needUpdateTexture |= _setMeshName(mCloudUpMesh, eCloudType_Mesh_Up, isUpdateMaterial);
		needUpdateTexture |= _setMeshName(mCloudMidMesh, eCloudType_Mesh_Mid, isUpdateMaterial);
		needUpdateTexture |= _setMeshName(mCloudDownMesh, eCloudType_Mesh_Down, isUpdateMaterial);

#ifdef ECHO_EDITOR
		if (needUpdateTexture)
			setTextureName(mContext.mTextureName);
#endif // ECHO_EDITOR

	}

	void CloudMeshComponent::setColor(const std::vector<ColorValue>& color, bool isUpdateMaterial)
	{
		mContext.mColor = color;
		setCloudColor(isUpdateMaterial);
	}


	void CloudMeshComponent::setBackColor(const std::vector<ColorValue>& backColor, bool isUpdateMaterial)
	{
		mContext.mBackColor = backColor;
		setCloudBackColor(isUpdateMaterial);
	}


	void CloudMeshComponent::setOpacity(const std::vector<float>& opacity, bool isUpdateMaterial)
	{
		
		bool isUpdate = false;
		if (opacity == mContext.mOpacity)
			isUpdate = true;
		if (mCloudUpMesh && (isUpdate || mContext.mOpacity[0] != opacity[0]))
		{
			mCloudUpMesh->setCloudOpacity(opacity[0]);
			updateCloudMeshMaterial(mCloudUpMesh, 0 , isUpdateMaterial);
		}
		if (mCloudMidMesh && (isUpdate || mContext.mOpacity[1] != opacity[1]))
		{
            mCloudMidMesh->setCloudOpacity(opacity[1]);
			updateCloudMeshMaterial(mCloudMidMesh,1 , isUpdateMaterial);
		}		
		if (mCloudDownMesh && (isUpdate || mContext.mOpacity[2] != opacity[2]))
		{
			mCloudDownMesh->setCloudOpacity(opacity[2]);
			updateCloudMeshMaterial(mCloudDownMesh, 2 , isUpdateMaterial);
		}
		mContext.mOpacity = opacity;
	}

	void CloudMeshComponent::setEmendation(const std::vector<float>& emendation, bool isUpdateMaterial)
	{
		bool isUpdate = false;
		if (emendation == mContext.mEmendation)
			isUpdate = true;
		if (mCloudUpMesh && (isUpdate || mContext.mEmendation[0] != emendation[0]))
		{
			mCloudUpMesh->setCloudEmendation(emendation[0]);
			updateCloudMeshMaterial(mCloudUpMesh, 0 ,isUpdateMaterial);
		}
		if (mCloudMidMesh && (isUpdate || mContext.mEmendation[1] != emendation[1]))
		{
			mCloudMidMesh->setCloudEmendation(emendation[1]);
			updateCloudMeshMaterial(mCloudMidMesh, 1, isUpdateMaterial);
		}
		if (mCloudDownMesh && (isUpdate || mContext.mEmendation[2] != emendation[2]))
		{
			mCloudDownMesh->setCloudEmendation(emendation[2]);
			updateCloudMeshMaterial(mCloudDownMesh, 2, isUpdateMaterial);
		}
		 mContext.mEmendation = emendation;
	}

	void CloudMeshComponent::setCloudColor(bool isUpdateMaterial)
	{
		bool flag = Root::instance()->getRenderSystem()->getCurRenderStrategy()->getPostProcessChain()->isEnable(PostProcess::HDR);
		if (mCloudUpMesh)
		{
			float tempMultiplier = mContext.mIntensity[0];
			if (flag)
				tempMultiplier *= mContext.mHDRIntensity[0];
			mCloudUpMesh->setCloudColour(mContext.mColor[0] * tempMultiplier);
			updateCloudMeshMaterial(mCloudUpMesh, 0 , isUpdateMaterial);
		}
		if (mCloudMidMesh)
		{
			float tempMultiplier = mContext.mIntensity[1];
			if (flag)
				tempMultiplier *= mContext.mHDRIntensity[1];
			mCloudMidMesh->setCloudColour(mContext.mColor[1] * tempMultiplier);
			updateCloudMeshMaterial(mCloudMidMesh, 1 ,isUpdateMaterial);
		}
		if (mCloudDownMesh)
		{
			float tempMultiplier = mContext.mIntensity[2];
			if (flag)
				tempMultiplier *= mContext.mHDRIntensity[2];
			mCloudDownMesh->setCloudColour(mContext.mColor[2] * tempMultiplier);
			updateCloudMeshMaterial(mCloudDownMesh, 2, isUpdateMaterial);
		}
	}

	void CloudMeshComponent::setCloudBackColor(bool isUpdateMaterial)
	{
		bool flag = Root::instance()->getRenderSystem()->getCurRenderStrategy()->getPostProcessChain()->isEnable(PostProcess::HDR);
		if (mCloudUpMesh)
		{
			float tempMultiplier = mContext.mBackColorIntensity[0];
			if (flag)
				tempMultiplier *= mContext.mHDRIntensity[0];
			mCloudUpMesh->setCloudSunBackColour(mContext.mBackColor[0] * tempMultiplier);
			updateCloudMeshMaterial(mCloudUpMesh, 0 , isUpdateMaterial);
		}
		if (mCloudMidMesh)
		{
			float tempMultiplier = mContext.mBackColorIntensity[1];
			if (flag)
				tempMultiplier *= mContext.mHDRIntensity[1];
			mCloudMidMesh->setCloudSunBackColour(mContext.mBackColor[1] * tempMultiplier);
			updateCloudMeshMaterial(mCloudMidMesh, 1 , isUpdateMaterial);
		}
		if (mCloudDownMesh)
		{
			float tempMultiplier = mContext.mBackColorIntensity[2];
			if (flag)
				tempMultiplier *= mContext.mHDRIntensity[2];
			mCloudDownMesh->setCloudSunBackColour(mContext.mBackColor[2] * tempMultiplier);
			updateCloudMeshMaterial(mCloudDownMesh, 2 , isUpdateMaterial);
		}
	}

	void CloudMeshComponent::setIntensity(const std::vector<float>& intensity,bool isUpdateMaterial)
	{
		mContext.mIntensity = intensity;
		setCloudColor(isUpdateMaterial);
	}

	void CloudMeshComponent::setHDRIntensity(const std::vector<float>& HDRIntensity, bool isUpdateMaterial)
	{
		mContext.mHDRIntensity = HDRIntensity;
		setCloudColor(isUpdateMaterial);
		setCloudBackColor(isUpdateMaterial);
	}

	void CloudMeshComponent::setBackColorIntensity(const std::vector<float>& backColorIntensity,bool isUpdateMaterial)
	{
		mContext.mBackColorIntensity = backColorIntensity;
		setCloudBackColor(isUpdateMaterial);
	}

	void CloudMeshComponent::setIsShowBackColor(const std::vector<unsigned int>& isShowBackColor,bool isUpdateMaterial)
	{
		bool isUpdate = false;
		if (isShowBackColor == mContext.mIsShowBackColor)
			isUpdate = true;
	
		if (mCloudUpMesh && (isUpdate || mContext.mIsShowBackColor[0] != isShowBackColor[0]))
		{
			bool flag = isShowBackColor[0] == 1 ? true : false;
			mCloudUpMesh->setUseSunBackColor(flag);
			updateCloudMeshMaterial(mCloudUpMesh, 0, isUpdateMaterial);
		}
		if (mCloudMidMesh && (isUpdate || mContext.mIsShowBackColor[1] != isShowBackColor[1]))
		{
			bool flag = isShowBackColor[1] == 1 ? true : false;
			mCloudMidMesh->setUseSunBackColor(flag);
			updateCloudMeshMaterial(mCloudMidMesh, 1, isUpdateMaterial);
		}
		if (mCloudDownMesh && (isUpdate || mContext.mIsShowBackColor[2] != isShowBackColor[2]))
		{
			bool flag = isShowBackColor[2] == 1 ? true : false;
			mCloudDownMesh->setUseSunBackColor(flag);
			updateCloudMeshMaterial(mCloudDownMesh, 2, isUpdateMaterial);
		}
		mContext.mIsShowBackColor = isShowBackColor;
	}
	
	void CloudMeshComponent::setTopHeight(const std::vector<float>& topHeight,bool isUpdateMaterial)
	{
		mContext.mTopHeight = topHeight;
		setCloudMeshHeight(isUpdateMaterial);
	}

	void CloudMeshComponent::setBottomHeight(const std::vector<float>& bottomHeight,bool isUpdateMaterial)
	{
		mContext.mBottomHeight = bottomHeight;
		setCloudMeshHeight(isUpdateMaterial);
	}

	void CloudMeshComponent::setCloudMeshHeight(bool isUpdateMaterial)
	{
		if (mCloudUpMesh)
		{
			mCloudUpMesh->setCloudHeight(Vector2(mContext.mBottomHeight[0], mContext.mTopHeight[0]));
			updateCloudMeshMaterial(mCloudUpMesh, 0, isUpdateMaterial);
		}
		if (mCloudMidMesh)
		{
			mCloudMidMesh->setCloudHeight(Vector2(mContext.mBottomHeight[1], mContext.mTopHeight[1]));
			updateCloudMeshMaterial(mCloudMidMesh, 1, isUpdateMaterial);
		}
		if (mCloudDownMesh)
		{
			mCloudDownMesh->setCloudHeight(Vector2(mContext.mBottomHeight[2], mContext.mTopHeight[2]));
			updateCloudMeshMaterial(mCloudDownMesh, 2, isUpdateMaterial);
		}
	}

	void CloudMeshComponent::setCloudMeshScale(const std::vector<float>& cloudMeshScale,bool isUpdateMaterial)
	{
		bool isUpdate = false;
		if (cloudMeshScale == mContext.mCloudMeshScale)
			isUpdate = true;
		
		if (mCloudUpMesh && (isUpdate || mContext.mCloudMeshScale[0] != cloudMeshScale[0]))
		{
			mCloudUpMesh->setScale(cloudMeshScale[0]);
			updateCloudMeshMaterial(mCloudUpMesh, 0, isUpdateMaterial);
		}
		if (mCloudMidMesh && (isUpdate || mContext.mCloudMeshScale[1] != cloudMeshScale[1]))
		{
			mCloudMidMesh->setScale(cloudMeshScale[1]);
			updateCloudMeshMaterial(mCloudMidMesh, 1, isUpdateMaterial);
		}
		if (mCloudDownMesh && (isUpdate || mContext.mCloudMeshScale[2] != cloudMeshScale[2]))
		{
			mCloudDownMesh->setScale(cloudMeshScale[2]);
			updateCloudMeshMaterial(mCloudDownMesh, 2, isUpdateMaterial);
		}
		mContext.mCloudMeshScale = cloudMeshScale;
	}
	void CloudMeshComponent::setURollSpeed(const std::vector<float>& uRollSpeed,bool isUpdateMaterial)
	{
		bool isUpdate = false;
		if (uRollSpeed == mContext.mURollSpeed)
			isUpdate = true;
	
		if (mCloudUpMesh && (isUpdate || mContext.mURollSpeed[0] != uRollSpeed[0]))
		{
			mCloudUpMesh->setCloudUSpeed(uRollSpeed[0]);
			updateCloudMeshMaterial(mCloudUpMesh, 0,isUpdateMaterial);
		}
		if (mCloudMidMesh && (isUpdate || mContext.mURollSpeed[1] != uRollSpeed[1]))
		{
			mCloudMidMesh->setCloudUSpeed(uRollSpeed[1]);
			updateCloudMeshMaterial(mCloudMidMesh, 1, isUpdateMaterial);
		}
		if (mCloudDownMesh && (isUpdate || mContext.mURollSpeed[2] != uRollSpeed[2]))
		{
			mCloudDownMesh->setCloudUSpeed(uRollSpeed[2]);
			updateCloudMeshMaterial(mCloudDownMesh, 2, isUpdateMaterial);
		}
		mContext.mURollSpeed = uRollSpeed;
	}

	void CloudMeshComponent::setVRollSpeed(const std::vector<float>& vRollSpeed,bool isUpdateMaterial)
	{
		bool isUpdate = false;
		if (vRollSpeed == mContext.mVRollSpeed)
			isUpdate = true;
		
		if (mCloudUpMesh && (isUpdate || mContext.mVRollSpeed[0] != vRollSpeed[0]))
		{
			mCloudUpMesh->setCloudVSpeed(vRollSpeed[0]);
			updateCloudMeshMaterial(mCloudUpMesh, 0, isUpdateMaterial);
		}
		if (mCloudMidMesh && (isUpdate || mContext.mVRollSpeed[1] != vRollSpeed[1]))
		{
			mCloudMidMesh->setCloudVSpeed(vRollSpeed[1]);
			updateCloudMeshMaterial(mCloudMidMesh, 1, isUpdateMaterial);
		}
		if (mCloudDownMesh && (isUpdate || mContext.mVRollSpeed[2] != vRollSpeed[2]))
		{
			mCloudDownMesh->setCloudVSpeed(vRollSpeed[2]);
			updateCloudMeshMaterial(mCloudDownMesh, 2, isUpdateMaterial);
		}
        mContext.mVRollSpeed = vRollSpeed;
	}
	void CloudMeshComponent::setUVTilling(const std::vector<float>& uvTilling,bool isUpdateMaterial)
	{
		bool isUpdate = false;
		if (uvTilling == mContext.mUVTilling)
			isUpdate = true;
		
		if (mCloudUpMesh && (isUpdate || mContext.mUVTilling[0] != uvTilling[0]))
		{
			mCloudUpMesh->setCloudUVTilling(uvTilling[0]);
			updateCloudMeshMaterial(mCloudUpMesh, 0, isUpdateMaterial);
		}
		if (mCloudMidMesh && (isUpdate || mContext.mUVTilling[1] != uvTilling[1]))
		{
			mCloudMidMesh->setCloudUVTilling(uvTilling[1]);
			updateCloudMeshMaterial(mCloudMidMesh, 1 , isUpdateMaterial);
		}
		if (mCloudDownMesh && (isUpdate || mContext.mUVTilling[2] != uvTilling[2]))
		{
			mCloudDownMesh->setCloudUVTilling(uvTilling[2]);
			updateCloudMeshMaterial(mCloudDownMesh, 2, isUpdateMaterial);
		}
		mContext.mUVTilling = uvTilling;
	}
	void CloudMeshComponent::setIsUseOriginalUV(const std::vector<unsigned int>& isUseOriUV)
	{
		bool isUpdate = false;
		if (isUseOriUV == mContext.mIsUseOriginalUV)
			isUpdate = true;
		
		if (mCloudUpMesh && (isUpdate || mContext.mIsUseOriginalUV[0] != isUseOriUV[0]))
		{
			bool flag = isUseOriUV[0] == 1 ? true : false;
			mCloudUpMesh->setUseOriginalUV(flag);
		}
		if (mCloudMidMesh && (isUpdate || mContext.mIsUseOriginalUV[1] != isUseOriUV[1]))
		{
			bool flag = isUseOriUV[1] == 1 ? true : false;
			mCloudMidMesh->setUseOriginalUV(flag);
		}
		if (mCloudDownMesh && (isUpdate || mContext.mIsUseOriginalUV[2] != isUseOriUV[2]))
		{
			bool flag = isUseOriUV[2] == 1 ? true : false;
			mCloudDownMesh->setUseOriginalUV(flag);
		}
       mContext.mIsUseOriginalUV = isUseOriUV;
	}
	CloudMesh* CloudMeshComponent::getCloudMeshByIdx(int idx)
	{
		Echo::CloudMesh* pCloudMesh = nullptr;

		if (idx == eCloudType_Mesh_Up)
		{
			pCloudMesh = mCloudUpMesh;
		}
		else if (idx == eCloudType_Mesh_Mid)
		{
			pCloudMesh = mCloudMidMesh;
		}
		else if (idx == eCloudType_Mesh_Down)
		{
			pCloudMesh = mCloudDownMesh;
		}

		return pCloudMesh;
	}

	void CloudMeshComponent::setMeshOrTextureNameState(int idx)
	{
		int id = idx - eCloudType_Mesh_Up;
		Echo::CloudMesh* pCloudMesh = getCloudMeshByIdx(idx);

		if (!pCloudMesh)
			return;
		if (!mContext.mTextureName[id].empty() && !mContext.mMeshName[id].empty())
		{
			if (mContext.mCloudVisible)
			{
				pCloudMesh->setVisible(true);
				mContext.mCloudRealVisible[id] = true;
			}

		}
		else
		{
			pCloudMesh->setVisible(false);
			mContext.mCloudRealVisible[id] = false;
		}

	}

	void CloudMeshComponent::clearResource()
	{
		mTextureRes.clear();
		mMeshRes.clear();
	}

	void CloudMeshComponent::updateCloudMeshMaterial(CloudMesh* cloudMesh_, const int& idx,bool isUpdateMaterial)
	{
		if (!isUpdateMaterial || !mIsUpdateTodTimeline)
			return;
		cloudMesh_->updateMaterial();
	}

	void CloudMeshComponent::_loadAndSetTextureIdx(int idx)
	{
		if (m_tMgr == nullptr || !m_bCreateFinish)return;

		Echo::CloudMesh* pCloudMesh = getCloudMeshByIdx(idx);
		if (!pCloudMesh) return;

		int id = idx - eCloudType_Mesh_Up;
		std::vector<String>& texName = mContext.mTextureName;
		String pNewTexture = texName[id];

		setMeshOrTextureNameState(idx);

		if (pNewTexture.empty() || !pCloudMesh->getVisible()) return;

		std::string pOldTexture;
		pCloudMesh->getCloudEffectTextureName(pOldTexture);
		if (pNewTexture == pOldTexture) return;

		TexturePtr texPtr_;
		if (mTextureRes.find(pNewTexture) != mTextureRes.end())
			texPtr_ = mTextureRes[pNewTexture];

		if (texPtr_.isNull())
		{	// 预防动态修改组件纹理操作
			if (Root::instance()->getRuntimeMode() == Root::eRM_Editor)
				texPtr_ = TextureManager::instance()->load(Name(pNewTexture));
			else if (TextureManager::instance()->resourceExists(Name(pNewTexture)))
			{
				ResourceBasePtr res = TextureManager::instance()->getByName(Name(pNewTexture));
				if (res->isPrepared())
					static_cast<TexturePtr>(res)->load();
				if (res->isLoaded())
					texPtr_ = static_cast<TexturePtr>(res);
			}
		}

		if (!texPtr_.isNull())
		{
			mTextureRes.insert(std::make_pair(pNewTexture, texPtr_));
			TODManager* todMgr = m_tMgr->GetTODManager();
			if (todMgr)
				todMgr->setCurrentCloudDiffuseNameByCom(idx, pNewTexture);
			pCloudMesh->setCloudTextureFromTOD(texPtr_);
			updateCloudMeshMaterial(pCloudMesh, id);
		}
		else if (TODSystem::instance()->showLog(LogType::LT_CloudMesh))
			LogManager::instance()->logMessage(" [CloudMesh] setTexture Error : " + pNewTexture);
	}

	void CloudMeshComponent::_preLoadTextureIdx(int idx)
	{
		int id = idx - eCloudType_Mesh_Up;
		std::vector<String>& texName = mContext.mTextureName;

		if (texName[id].empty() || loadTextureData(texName[id]))
			_notifyFinish(id, RESTEXTURE);
	}

	void CloudMeshComponent::_prePerpareMeshIdx(int idx)
	{
		int id = idx - eCloudType_Mesh_Up;
		std::vector<String>& meshName = mContext.mMeshName;

		if(meshName[id].empty() || prepareMeshData(meshName[id]))
			_notifyFinish(id, RESMESH);
	}

	bool CloudMeshComponent::_notifyFinish(int type, const String& Data)
	{
		if (!m_bCreateFinish && Data == RESTEXTURE)
			m_bCreateKey |= (1 << type);
		if (!m_bCreateFinish && Data == RESMESH)
			m_bCreateKey |= (1 << (type + 3));

		if (m_bCreateKey == 0b111111 && !m_bCreateFinish)
		{
			m_bCreateFinish = true;
			m_pActor->notifyComponentFinish(weak_from_this());
		}

		CloudMeshComponent* activeCom = m_tMgr->getCloudMeshContext().mActiveCom;
		if (activeCom == nullptr || activeCom != this)
			return false;	// 不为当前生效组件
		return true;
	}

	bool CloudMeshComponent::_setMeshName(CloudMesh* pCloudMesh, int idx, bool isUpdateMaterial)
	{
		if (!m_bCreateFinish || !pCloudMesh) return false;

		int id = idx - eCloudType_Mesh_Up;

		if (mContext.mMeshName[id].empty())
		{
			pCloudMesh->setVisible(false);
			mContext.mCloudRealVisible[id] = false;
			return false;
		}

		if (pCloudMesh->getMeshFile() != mContext.mMeshName[id])
		{
			String meshName;
			if (mMeshRes.find(mContext.mMeshName[id]) != mMeshRes.end() ||
				Root::instance()->getRuntimeMode() == Root::eRM_Editor  ||				//预防动态修改与编辑器修改操作
				MeshMgr::instance()->resourceExists(Name(mContext.mMeshName[id])))		//
			{
				meshName = mContext.mMeshName[id];
				if (Root::instance()->getRuntimeMode() == Root::eRM_Editor)
					mMeshRes.insert(std::make_pair(meshName, MeshMgr::instance()->prepare(Name(meshName))));
				else if(MeshMgr::instance()->resourceExists(Name(mContext.mMeshName[id])))
					mMeshRes.insert(std::make_pair(meshName, MeshMgr::instance()->getByName(Name(meshName))));
			}

			if (!meshName.empty())
			{
				pCloudMesh->setMeshFile(mContext.mMeshName[id]);
				setMeshOrTextureNameState(idx);
				updateCloudMeshMaterial(pCloudMesh, id, isUpdateMaterial);
				return true;
			}
		}
		return false;
	}

#ifdef ECHO_EDITOR
	void CloudMeshComponent::_initFirstCreateMeshAndTexData()
	{
		if (!m_bNewCreateCom)
			return;

		m_bNewCreateCom = false;

		if (!m_tMgr)
			return;
		SceneManager* sMgr = m_tMgr->GetSceneManager();
		if (sMgr == nullptr)
			return;
		Cloud* cloud_ = sMgr->getCloud();
		if (cloud_ == nullptr)
			return;

		std::string upMeshName;
		std::string upTexName;
		std::string midMeshName;
		std::string midTexName;
		std::string downMeshName;
		std::string downTexName;
		if (cloud_->getCloudMeshUp())
		{
			upMeshName = cloud_->getCloudMeshUp()->getMeshFile();
			cloud_->getCloudMeshUp()->getCloudEffectTextureName(upTexName);
		}
		if (cloud_->getCloudMeshMid())
		{
			midMeshName = cloud_->getCloudMeshMid()->getMeshFile();
			cloud_->getCloudMeshMid()->getCloudEffectTextureName(midTexName);
		}

		if (cloud_->getCloudMeshDown())
		{
			downMeshName = cloud_->getCloudMeshDown()->getMeshFile();
			cloud_->getCloudMeshDown()->getCloudEffectTextureName(downTexName);
		}

		if (mContext.mMeshName[0].empty())
			mContext.mMeshName[0] = upMeshName;

		if (mContext.mMeshName[1].empty())
			mContext.mMeshName[1] = midMeshName;

		if (mContext.mMeshName[2].empty())
			mContext.mMeshName[2] = downMeshName;

		if (mContext.mTextureName[0].empty())
			mContext.mTextureName[0] = upTexName;

		if (mContext.mTextureName[1].empty())
			mContext.mTextureName[1] = midTexName;

		if (mContext.mTextureName[2].empty())
			mContext.mTextureName[2] = downTexName;
	}
#endif

	void CloudMeshComponent::setTimelineStringAttribute(const std::string& value,const Name& attrName)
	{
		if (!m_tMgr)
			return;
		CloudMeshComponent* activeCom = m_tMgr->getCloudMeshContext().mActiveCom;
		if (activeCom == nullptr || activeCom != this )
			return;
		m_tMgr->setTodComChangeState(T_CloudMesh);
		setCloudVisible(mContext.mCloudRealVisible);
		setMeshName(mContext.mMeshName);
	
		if (cloudMeshTextureUpName_ == attrName)
		{
			mContext.mTextureName[0] = value;
			setTextureName(mContext.mTextureName);
		}
		else if (cloudMeshTextureMidName_ == attrName)
		{
			mContext.mTextureName[1] = value;
			setTextureName(mContext.mTextureName);
		}
		else if (cloudMeshTextureDownName_ == attrName)
		{
			mContext.mTextureName[2] = value;
			setTextureName(mContext.mTextureName);
		}
	}
	void CloudMeshComponent::setIsUseDistort(const std::vector<unsigned int>& isUseDistort)
	{
		bool isUpdate = false;
		if (isUseDistort == mContext.mIsUseDistort)
			isUpdate = true;
	
		if (mCloudUpMesh && (isUpdate || mContext.mIsUseDistort[0] != isUseDistort[0]))
		{
			bool flag = isUseDistort[0] == 1 ? true : false;
			mCloudUpMesh->setUseDistort(flag);
		}
		if (mCloudMidMesh && (isUpdate || mContext.mIsUseDistort[1] != isUseDistort[1]))
		{
			bool flag = isUseDistort[1] == 1 ? true : false;
			mCloudMidMesh->setUseDistort(flag);
		}
		if (mCloudDownMesh && (isUpdate || mContext.mIsUseDistort[2] != isUseDistort[2]))
		{
			bool flag = isUseDistort[2] == 1 ? true : false;
			mCloudDownMesh->setUseDistort(flag);
		}
		mContext.mIsUseDistort = isUseDistort;
	}
	void CloudMeshComponent::setDistortSpeed(const std::vector<float>& distortSpeed,bool isUpdateMaterial)
	{
		bool isUpdate = false;
		if (distortSpeed == mContext.mDistortSpeed)
			isUpdate = true;

		if (mCloudUpMesh && (isUpdate || mContext.mDistortSpeed[0] != distortSpeed[0]))
		{
			mCloudUpMesh->setDistortSpeed(distortSpeed[0]);
			updateCloudMeshMaterial(mCloudUpMesh, 0, isUpdateMaterial);
		}
		if (mCloudMidMesh && (isUpdate || mContext.mDistortSpeed[1] != distortSpeed[1]))
		{
			mCloudMidMesh->setDistortSpeed(distortSpeed[1]);
			updateCloudMeshMaterial(mCloudMidMesh, 1, isUpdateMaterial);
		}
		if (mCloudDownMesh && (isUpdate || mContext.mDistortSpeed[2] != distortSpeed[2]))
		{
			mCloudDownMesh->setDistortSpeed(distortSpeed[2]);
			updateCloudMeshMaterial(mCloudDownMesh, 2, isUpdateMaterial);
		}

		mContext.mDistortSpeed = distortSpeed;
	}
	void CloudMeshComponent::setDistortAmplitude(const std::vector<float>& distortAmplitude,bool isUpdateMaterial)
	{
		bool isUpdate = false;
		if (distortAmplitude == mContext.mDistortAmplitude)
			isUpdate = true;
		if (mCloudUpMesh && (isUpdate || mContext.mDistortAmplitude[0] != distortAmplitude[0]))
		{
			mCloudUpMesh->setDistortAmplitude(distortAmplitude[0]);
			updateCloudMeshMaterial(mCloudUpMesh, 0, isUpdateMaterial);
		}
		if (mCloudMidMesh && (isUpdate || mContext.mDistortAmplitude[1] != distortAmplitude[1]))
		{
			mCloudMidMesh->setDistortAmplitude(distortAmplitude[1]);
			updateCloudMeshMaterial(mCloudMidMesh, 1, isUpdateMaterial);
		}
		if (mCloudDownMesh && (isUpdate || mContext.mDistortAmplitude[2] != distortAmplitude[2]))
		{
			mCloudDownMesh->setDistortAmplitude(distortAmplitude[2]);
			updateCloudMeshMaterial(mCloudDownMesh, 2, isUpdateMaterial);
		}
		mContext.mDistortAmplitude = distortAmplitude;
	}
	void CloudMeshComponent::setDistortScale(const std::vector<float>& distortScale,bool isUpdateMaterial)
	{
		bool isUpdate = false;
		if (distortScale == mContext.mDistortScale)
			isUpdate = true;
		if (mCloudUpMesh && (isUpdate || mContext.mDistortScale[0] != distortScale[0]))
		{
			mCloudUpMesh->setDistortScale(distortScale[0]);
			updateCloudMeshMaterial(mCloudUpMesh, 0, isUpdateMaterial);
		}
		if (mCloudMidMesh && (isUpdate || mContext.mDistortScale[1] != distortScale[1]))
		{
			mCloudMidMesh->setDistortScale(distortScale[1]);
			updateCloudMeshMaterial(mCloudMidMesh, 1, isUpdateMaterial);
		}
		if (mCloudDownMesh && (isUpdate || mContext.mDistortScale[2] != distortScale[2]))
		{
			mCloudDownMesh->setDistortScale(distortScale[2]);
			updateCloudMeshMaterial(mCloudDownMesh, 2, isUpdateMaterial);
		}
		mContext.mDistortScale = distortScale;
	}
	void CloudMeshComponent::setRenderPriority(const std::vector<int32>& renderPriority)
	{
		bool isUpdate = false;
		if (renderPriority == mContext.mRenderPriority)
			isUpdate = true;
		if (mCloudUpMesh && (isUpdate || mContext.mRenderPriority[0] != renderPriority[0]))
		{
			mCloudUpMesh->setRenderPriority(renderPriority[0]);
		}
		if (mCloudMidMesh && (isUpdate || mContext.mRenderPriority[1] != renderPriority[1]))
		{
			mCloudMidMesh->setRenderPriority(renderPriority[1]);
		}
		if (mCloudDownMesh && (isUpdate || mContext.mRenderPriority[2] != renderPriority[2]))
		{
			mCloudDownMesh->setRenderPriority(renderPriority[2]);
		}
		mContext.mRenderPriority = renderPriority;
	}
	void CloudMeshComponent::setCloudBorderSoft(const float& cloudBorderSoft,bool isUpdateMaterial)
	{
		if (mCloudUpMesh && mCloudUpMesh->getCloudBorderSoft() != cloudBorderSoft)
		{
			mCloudUpMesh->setCloudBorderSoft(cloudBorderSoft);
			updateCloudMeshMaterial(mCloudUpMesh, 0, isUpdateMaterial);
		}
		if (mCloudMidMesh && mCloudMidMesh->getCloudBorderSoft() != cloudBorderSoft)
		{
			mCloudMidMesh->setCloudBorderSoft(cloudBorderSoft);
			updateCloudMeshMaterial(mCloudMidMesh, 1, isUpdateMaterial);
		}
		if (mCloudDownMesh && mCloudDownMesh->getCloudBorderSoft() != cloudBorderSoft)
		{
			mCloudDownMesh->setCloudBorderSoft(cloudBorderSoft);
			updateCloudMeshMaterial(mCloudDownMesh, 2, isUpdateMaterial);
		}
		mContext.mCloudBorderSoft = cloudBorderSoft;

	}
	void CloudMeshComponent::setTransFormTime(float transTime)
	{
		mContext.mTransFormTime = transTime;
	}


	void CloudMeshComponent::setCloudVisible(bool isShow)
	{
		bool isChange = false;

#define SETVISIBLE(Name)			\
		if(mContext.Name != isShow)	\
			isChange = true;		\
		mContext.Name = isShow;

		SETVISIBLE(mCloudVisible);
		SETVISIBLE(mCloudRealVisible[0]);
		SETVISIBLE(mCloudRealVisible[1]);
		SETVISIBLE(mCloudRealVisible[2]);
#undef SETVISIBLE

		if (mCloudUpMesh)
			mCloudUpMesh->setVisible(isShow);
		if (mCloudMidMesh)
			mCloudMidMesh->setVisible(isShow);
		if(mCloudDownMesh)
			mCloudDownMesh->setVisible(isShow);

		if (isChange)
		{	// 减少性能开销
			_loadAndSetTextureIdx(eCloudType_Mesh_Up);
			_loadAndSetTextureIdx(eCloudType_Mesh_Mid);
			_loadAndSetTextureIdx(eCloudType_Mesh_Down);
		}

		setMeshOrTextureNameState(eCloudType_Mesh_Up);
		setMeshOrTextureNameState(eCloudType_Mesh_Mid);
		setMeshOrTextureNameState(eCloudType_Mesh_Down);
	}

	void CloudMeshComponent::setCloudVisible(const std::vector<bool>& isShow)
	{
		if (isShow.size() < 3) return;

		bool isChange = false;

#define SETVISIBLE(id)										\
		if(mContext.mCloudRealVisible[id] != isShow[id])	\
			isChange = true;								\
		mContext.mCloudRealVisible[id] = isShow[id];

		SETVISIBLE(0);
		SETVISIBLE(1);
		SETVISIBLE(2);
#undef SETVISIBLE

		bool v = isShow[0] || isShow[1] || isShow[2];
		if (mContext.mCloudVisible != v)
			isChange = true;
		mContext.mCloudVisible = v;

		if (mCloudUpMesh)
			mCloudUpMesh->setVisible(isShow[0]);
		if (mCloudMidMesh)
			mCloudMidMesh->setVisible(isShow[1]);
		if (mCloudDownMesh)
			mCloudDownMesh->setVisible(isShow[2]);

		if (isChange)
		{	// 减少性能开销
			_loadAndSetTextureIdx(eCloudType_Mesh_Up);
			_loadAndSetTextureIdx(eCloudType_Mesh_Mid);
			_loadAndSetTextureIdx(eCloudType_Mesh_Down);
		}

		setMeshOrTextureNameState(eCloudType_Mesh_Up);
		setMeshOrTextureNameState(eCloudType_Mesh_Mid);
		setMeshOrTextureNameState(eCloudType_Mesh_Down);
	}

	void CloudMeshComponent::setUVFactory(const std::vector<float>& uvFactory)
	{
		mContext.m_UVFactory = uvFactory;
		if (mCloudUpMesh)
		{
			mCloudUpMesh->setUVFactory(uvFactory[0]);
			
		}
		if (mCloudMidMesh )
		{
			mCloudMidMesh->setUVFactory(uvFactory[1]);
			
		}
		if (mCloudDownMesh )
		{
			mCloudDownMesh->setUVFactory(uvFactory[2]);
			
		}

	}

	void CloudMeshComponent::use()
	{
		if (!m_tMgr)
			return;

		SceneManager* sMgr = m_tMgr->GetSceneManager();
		if (sMgr == nullptr)
			return;
		Cloud* cloud_ = sMgr->getCloud();
		if (cloud_ == nullptr)
			return;
		if (m_tMgr->getIsWriteLog())
		{
			std::string logInfo = "";
			logInfo.append("CloudMesh : use").append(getCombinedName());
			LogManager::getSingleton().logMessage(Echo::LML_CRITICAL, logInfo.data());
		}

		mCloudUpMesh = m_tMgr->getCloudUpMesh();
		mCloudMidMesh = m_tMgr->getCloudMidMesh();
		mCloudDownMesh = m_tMgr->getCloudDownMesh();

		if (mCloudUpMesh == nullptr || mCloudMidMesh == nullptr || mCloudDownMesh == nullptr)
			return;

		mCloudUpMesh->setCloudFadingOut(false);
		mCloudMidMesh->setCloudFadingOut(false);
		mCloudDownMesh->setCloudFadingOut(false);

#ifdef ECHO_EDITOR
		_initFirstCreateMeshAndTexData();
#endif

		setCloudColor(false);
		setCloudBackColor(false);
		setOpacity(mContext.mOpacity,false);
		setEmendation(mContext.mEmendation,false);
		setPriority(mContext.mPriority);

		mCloudUpMesh->setVisible(mContext.mCloudRealVisible[0]);
		mCloudMidMesh->setVisible(mContext.mCloudRealVisible[1]);
		mCloudDownMesh->setVisible(mContext.mCloudRealVisible[2]);

		setMeshOrTextureNameState(eCloudType_Mesh_Up);
		setMeshOrTextureNameState(eCloudType_Mesh_Mid);
		setMeshOrTextureNameState(eCloudType_Mesh_Down);

		setCloudMeshAtribute();
	}

	void CloudMeshComponent::unuse()
	{
		if (m_tMgr == nullptr)
			return;

		mCloudUpMesh = nullptr;
		mCloudMidMesh = nullptr;
		mCloudDownMesh = nullptr;
		if (m_tMgr->getIsWriteLog())
		{
			std::string logInfo = "";
			logInfo.append("CloudMesh : unuse").append(getCombinedName());
			LogManager::getSingleton().logMessage(Echo::LML_CRITICAL, logInfo.data());
		}
	}

	void CloudMeshComponent::setCloudMeshAtribute()
	{
		setIsUseOriginalUV(mContext.mIsUseOriginalUV);
		setIsShowBackColor(mContext.mIsShowBackColor,false);
		setIsUseDistort(mContext.mIsUseDistort);
		setMeshName(mContext.mMeshName,false);
		setTextureName(mContext.mTextureName,false);
		setCloudMeshHeight(false);
		setCloudMeshScale(mContext.mCloudMeshScale, false);
		setURollSpeed(mContext.mURollSpeed, false);
		setVRollSpeed(mContext.mVRollSpeed, false);
		setUVTilling(mContext.mUVTilling, false);
		setDistortSpeed(mContext.mDistortSpeed, false);
		setDistortAmplitude(mContext.mDistortAmplitude, false);
		setDistortScale(mContext.mDistortScale, false);
		setRenderPriority(mContext.mRenderPriority);
		mCloudUpMesh->setCloudTextureName(mContext.mTextureName[0]);
		mCloudMidMesh->setCloudTextureName(mContext.mTextureName[1]);
		mCloudDownMesh->setCloudTextureName(mContext.mTextureName[2]);

		setUVFactory(mContext.m_UVFactory);
		mCloudUpMesh->updateMaterial();
		mCloudMidMesh->updateMaterial();
		mCloudDownMesh->updateMaterial();
	}

	void CloudMeshComponent::setPriority(int priority)
	{
		mContext.mPriority = priority;
		if(m_tMgr)
		   m_tMgr->adjustComponentPriority(m_tMgr->getCloudMeshContext(), this);
	}


	void CloudMeshComponent::setTimelineAttribute(std::vector<float>& data, const Name& name) 
	{	
		if (m_tMgr == nullptr)
			return;
		CloudMeshComponent* activeCom = m_tMgr->getCloudMeshContext().mActiveCom;
		if (activeCom == nullptr || activeCom != this )
			return;

		m_tMgr->setTodComChangeState(T_CloudMesh);

		setCloudVisible(mContext.mCloudRealVisible);
		setCloudBorderSoft(mContext.mCloudBorderSoft);
		setMeshName(mContext.mMeshName);

		if (name == cloudMeshUpColorName_) {
			mContext.mColor[0] = ColorValue(data[0], data[1], data[2], data[3]);
			setCloudColor();
		}
		else if (name == cloudMeshMidColorName_) {
			mContext.mColor[1] = ColorValue(data[0], data[1], data[2], data[3]);
			setCloudColor();
		}
		else if (name == cloudMeshDownColorName_) {
			mContext.mColor[2] = ColorValue(data[0], data[1], data[2], data[3]);
			setCloudColor();
		}

		else if (name == cloudMeshUpBackColorName_) {
			mContext.mBackColor[0] = ColorValue(data[0], data[1], data[2], data[3]);
			setCloudBackColor();
		}
		else if (name == cloudMeshMidBackColorName_) {
			mContext.mBackColor[1] = ColorValue(data[0], data[1], data[2], data[3]);
			setCloudBackColor();
		}
		else if (name == cloudMeshDownBackColorName_) {
			mContext.mBackColor[2] = ColorValue(data[0], data[1], data[2], data[3]);
			setCloudBackColor();
		}

		else if (name == cloudMeshUpOpacityName_) {
			mContext.mOpacity[0] = data[0];
			setOpacity(mContext.mOpacity);
		}
		else if (name == cloudMeshMidOpacityName_) {
			mContext.mOpacity[1] = data[0];
			setOpacity(mContext.mOpacity);
		}
		else if (name == cloudMeshDownOpacityName_) {
			mContext.mOpacity[2] = data[0];
			setOpacity(mContext.mOpacity);
		}

		else if (name == cloudMeshUpEmendationName_) {
			mContext.mEmendation[0] = data[0];
			setEmendation(mContext.mEmendation);
		}
		else if (name == cloudMeshMidEmendationName_) {
			mContext.mEmendation[1] = data[0];
			setEmendation(mContext.mEmendation);
		}
		else if (name == cloudMeshDownEmendationName_) {
			mContext.mEmendation[2] = data[0];
			setEmendation(mContext.mEmendation);
		}

	
		else if (name == cloudMeshUpIntensityName_) {
			mContext.mIntensity[0] = data[0];
			setCloudColor();
		}
		else if (name == cloudMeshMidIntensityName_) {
			mContext.mIntensity[1] = data[0];
			setCloudColor();
		}
		else if (name == cloudMeshDownIntensityName_) {
			mContext.mIntensity[2] = data[0];
			setCloudColor();
		}

		else if (name == cloudMeshUpHDRIntensityName_) {
			mContext.mHDRIntensity[0] = data[0];
			setCloudColor();
			setCloudBackColor();
		}
		else if (name == cloudMeshMidHDRIntensityName_) {
			mContext.mHDRIntensity[1] = data[0];
			setCloudColor();
			setCloudBackColor();
		}
		else if (name == cloudMeshDownHDRIntensityName_) {
			mContext.mHDRIntensity[2] = data[0];
			setCloudColor();
			setCloudBackColor();
		}
		else if (name == cloudMeshUpBackColorIntensityName_) {
			mContext.mBackColorIntensity[0] = data[0];
			setCloudBackColor();
		}
		else if (name == cloudMeshMidBackColorIntensityName_) {
			mContext.mBackColorIntensity[1] = data[0];
			setCloudBackColor();
		}
		else if (name == cloudMeshDownBackColorIntensityName_) {
			mContext.mBackColorIntensity[2] = data[0];
			setCloudBackColor();
		}

	}

	// ---------------------------------------------------------------------------------------
	EDITOR_REFLECT_BEGIN(CloudMeshComponentInfo)
	EDITOR_REFLECT_MEMBER_UPDATE(mContext.mPriority, "Priority", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mContext.mTextureName, "TextureName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mContext.mMeshName, "MeshName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mContext.mColor, "CloudMeshColor", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mContext.mBackColor, "BackColor", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mContext.mOpacity, "Opacity", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mContext.mEmendation, "Emendation", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mContext.mIntensity, "Intensity", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mContext.mHDRIntensity, "HDRIntensity", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mContext.mBackColorIntensity, "BackColorIntensity", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mContext.mIsShowBackColor, "IsShowBackColor", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mContext.mTopHeight, "TopHeight", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mContext.mBottomHeight, "BottomHeight", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mContext.mCloudMeshScale, "CloudMeshScale", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mContext.mURollSpeed, "URollSpeed", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mContext.mVRollSpeed, "VRollSpeed", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)EDITOR_REFLECT_MEMBER_UPDATE(mContext.mIsShowBackColor, "IsShowBackColor", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mContext.mUVTilling, "UVTilling", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mContext.mIsUseOriginalUV, "IsUseOriginalUV", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mContext.mIsUseDistort, "IsUseDistort", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mContext.mDistortSpeed, "DistortSpeed", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mContext.mDistortAmplitude, "DistortAmplitude", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mContext.mDistortScale, "DistortScale", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mContext.mRenderPriority, "RenderDistance", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mContext.mCloudBorderSoft, "CloudBorderSoft", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mContext.mCloudVisible, "CloudVisible", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
    EDITOR_REFLECT_MEMBER_UPDATE(mContext.mTransFormTime, "TransTime", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	/*EDITOR_REFLECT_MEMBER_UPDATE(mContext.mUpMeshTextureOrMeshEmpty, "CloudUpMeshTextureOrMeshEmpty", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mContext.mMidMeshTextureOrMeshEmpty, "CloudMidMeshTextureOrMeshEmpty", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mContext.mDownMeshTextureOrMeshEmpty, "CloudDownMeshTextureOrMeshEmpty", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)*/
	EDITOR_REFLECT_MEMBER_UPDATE(mContext.m_UVFactory, "UVFactory", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_END();

	CloudMeshComponentInfo::CloudMeshComponentInfo(const Name & comType, ActorCOMMoveableType type) :
		ActorComponentInfo(comType, type)
	{
	}

	void CloudMeshComponentInfo::ExportData(cJSON * jsonNode)
	{
		reflect::TypeResolver<CloudMeshComponentInfo>::get()->ExportJson(this, jsonNode, "");
	}

	void CloudMeshComponentInfo::ImportData(cJSON * jsonNode)
	{
#ifdef ECHO_EDITOR
		m_bNewCreateCom = false;
#endif
		reflect::TypeResolver<CloudMeshComponentInfo>::get()->ImportJson(this, jsonNode, "");
	}

	void CloudMeshComponentInfo::CopyTo(ActorComponentInfo * comInfo)
	{
		if(comInfo == nullptr)
			return;
		CloudMeshComponentInfo* targetInfo = static_cast<CloudMeshComponentInfo*>(comInfo);
		reflect::TypeResolver<CloudMeshComponentInfo>::get()->copyTo(this, targetInfo);

#ifdef ECHO_EDITOR
		targetInfo->m_bNewCreateCom = false;
#endif
	}

	void CloudMeshComponentInfo::OverWrite() {
		if (mOriginComInfo != nullptr) {
			reflect::TypeResolver<CloudMeshComponentInfo>::get()->overWrite(mOriginComInfo, this);
		}
	}

#ifdef ECHO_EDITOR
	bool CloudMeshComponentInfo::getNeedCollectedResource(std::unordered_multimap<ResourceType, std::string>& resMap)
	{
		bool isNeedCopy = false;
		for (const std::string& meshPath : mContext.mMeshName)
		{
			if (!meshPath.empty())
			{
				resMap.insert(std::make_pair(ResourceType::Mesh, meshPath));
				isNeedCopy = true;
			}
		}

		for (const std::string& texName : mContext.mTextureName)
		{
			if (!texName.empty())
			{
				resMap.insert(std::make_pair(ResourceType::NeedCopyRes,texName));
				isNeedCopy = true;
			}
		}
		return isNeedCopy;
	}
#endif
	
}
