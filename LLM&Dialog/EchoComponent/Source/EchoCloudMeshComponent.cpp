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

		if (m_tMgr == nullptr)
			return;

		if (resType == TextureManager::instance()->getResourceType())
		{
			const std::string& texName_ = resName.c_str();

			m_tMgr->addTodTexturePtr(resName.c_str(), static_cast<TexturePtr>(resourcePtr));
			TODManager* todMgr = m_tMgr->GetTODManager();

			if (!resourcePtr.isNull() && resourcePtr->isPrepared())
				resourcePtr->load();

			if (texName_ == mContext.mTextureName[0])
			{
				if (todMgr)
					todMgr->setCurrentCloudDiffuseNameByCom(eCloudType_Mesh_Up, texName_);
				_notifyFinish(0, RESTEXTURE);
			}
			if (texName_ == mContext.mTextureName[1])
			{
				if (todMgr)
					todMgr->setCurrentCloudDiffuseNameByCom(eCloudType_Mesh_Mid, texName_);
				_notifyFinish(1, RESTEXTURE);
			}
			if (texName_ == mContext.mTextureName[2])
			{
				if (todMgr)
					todMgr->setCurrentCloudDiffuseNameByCom(eCloudType_Mesh_Down, texName_);
				_notifyFinish(2, RESTEXTURE);
			}
		}
		else if (resType == MeshMgr::instance()->getResourceType())
		{
			const std::string& meshName = resName.c_str();

			m_tMgr->addCloudMeshPtr(resName.c_str(), static_cast<MeshPtr>(resourcePtr));
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

	bool CloudMeshComponent::loadTextureData(const std::string& textureName,TexturePtr& texPtr_)
	{
		//TexturePtr texPtr;
		if (!m_tMgr)
			return false;
		m_tMgr->getTodTexturePtrByName(textureName, texPtr_);
		if (texPtr_.isNull())
		{
			if (/*texPtr_.isNull()*/!TextureManager::instance()->resourceExists(Name(textureName)))
			{
				if (Root::instance()->getRuntimeMode() != Root::eRM_Editor)
				{
					ResourceLoadPriority priority = WorkQueue::MakePriority(100, WorkQueue::RESOURCE_TYPE_SKY);
					requestResouce(Name(textureName), TextureManager::instance()->getResourceType(), priority);
					return false;
				}
				else
				{
					texPtr_ = TextureManager::instance()->load(Name(textureName));
					m_tMgr->addTodTexturePtr(textureName, texPtr_);
				}
			}
			else
			{
				texPtr_ = TextureManager::instance()->getByName(Name(textureName));
				m_tMgr->addTodTexturePtr(textureName, texPtr_);
			}
		}
		return true;
	}

	bool CloudMeshComponent::prepareMeshData(const std::string& meshName, MeshPtr& meshPtr)
	{
		if (!m_tMgr)
			return false;
		m_tMgr->getCloudMeshPtrPtrByName(meshName, meshPtr);
		if (meshPtr.isNull())
		{
			if (/*meshPtr.isNull()*/!MeshMgr::instance()->resourceExists(Name(meshName)))
			{
				if (Root::instance()->getRuntimeMode() != Root::eRM_Editor)
				{
					ResourceLoadPriority priority = WorkQueue::MakePriority(100, WorkQueue::RESOURCE_TYPE_SKY);
					requestResouce(Name(meshName), MeshMgr::instance()->getResourceType(), priority);
					SetLoadResourceState(false);
					return false;
				}
			}
			else if (Root::instance()->getRuntimeMode() != Root::eRM_Editor)
			{
				meshPtr = MeshMgr::instance()->getByName(Name(meshName));
				m_tMgr->addCloudMeshPtr(meshName, meshPtr);
			}
		}
		return true;
	}

	void CloudMeshComponent::setMeshName(const std::vector<std::string>& meshName,bool isUpdateMaterial)
	{
		mContext.mMeshName = meshName;

		auto SetMeshFunc = [&](CloudMesh* pCloudMesh, int idx)
		{
			if (pCloudMesh)
			{
				if (!meshName[idx].empty())
				{
					pCloudMesh->setMeshFile(meshName[idx]);
					setUpMeshOrTextureNameState();
					updateCloudMeshMaterial(pCloudMesh, idx, isUpdateMaterial);
				}
				else
				{
					pCloudMesh->setVisible(false);
					mContext.mCloudRealVisible[idx] = false;
				}
			}
		};

		SetMeshFunc(mCloudUpMesh, 0);
		SetMeshFunc(mCloudMidMesh, 1);
		SetMeshFunc(mCloudDownMesh, 2);

		/*if (mCloudUpMesh)
		{
			if (!meshName[0].empty())
			{
				mCloudUpMesh->setMeshFile(meshName[0]);
				setUpMeshOrTextureNameState();
				updateCloudMeshMaterial(mCloudUpMesh, 0, isUpdateMaterial);
			}
			else
			{
				mCloudUpMesh->setVisible(false);
				mContext.mCloudRealVisible[0] = false;
			}
		} 

		if (mCloudMidMesh)
		{
			if(!meshName[1].empty())
			{
				mCloudMidMesh->setMeshFile(meshName[1]);
				setMidMeshOrTextureNameState();
				updateCloudMeshMaterial(mCloudMidMesh, 1, isUpdateMaterial);
			}
			else
			{
				mCloudMidMesh->setVisible(false);
				mContext.mCloudRealVisible[1] = false;
			}
		} 

		if (mCloudDownMesh)
		{
			if (!meshName[2].empty())
			{
				mCloudDownMesh->setMeshFile(meshName[2]);
				setDownMeshOrTextureNameState();
				updateCloudMeshMaterial(mCloudDownMesh, 2, isUpdateMaterial);
			}
			else
			{
				mCloudDownMesh->setVisible(false);
				mContext.mCloudRealVisible[2] = false;
			}
		}*/
#ifdef ECHO_EDITOR
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
	void CloudMeshComponent::setUpMeshOrTextureNameState()
	{
		if (!mCloudUpMesh)
			return;
		if (!mContext.mTextureName[0].empty() && !mContext.mMeshName[0].empty())
		{
			if (mContext.mCloudVisible)
			{
				mCloudUpMesh->setVisible(true);
				mContext.mCloudRealVisible[0] = true;
			}
		}
		else
		{
			mCloudUpMesh->setVisible(false);
			mContext.mCloudRealVisible[0] = false;
		}
	}
	void CloudMeshComponent::setMidMeshOrTextureNameState()
	{
		if (!mCloudMidMesh)
			return;
		if (!mContext.mTextureName[1].empty() && !mContext.mMeshName[1].empty())
		{
			if (mContext.mCloudVisible)
			{
                mCloudMidMesh->setVisible(true);
				mContext.mCloudRealVisible[1] = true;
			}
				
		}
		else
		{
			mCloudMidMesh->setVisible(false);
			mContext.mCloudRealVisible[1] = false;
		}
	}
	void CloudMeshComponent::setDownMeshOrTextureNameState()
	{
		if (!mCloudDownMesh)
			return;
		if (!mContext.mTextureName[2].empty() && !mContext.mMeshName[2].empty())
		{
			if (mContext.mCloudVisible)
			{
                mCloudDownMesh->setVisible(true);
				mContext.mCloudRealVisible[2] = true;
			}
				
		}
		else
		{
			mCloudDownMesh->setVisible(false);
			mContext.mCloudRealVisible[1] = false;
		}
	}



	void CloudMeshComponent::updateCloudMeshMaterial(CloudMesh* cloudMesh_, const int& idx,bool isUpdateMaterial)
	{
		if (!isUpdateMaterial || !mIsUpdateTodTimeline)
			return;
		cloudMesh_->updateMaterial();
	}

	void CloudMeshComponent::_loadAndSetTextureIdx(int idx)
	{
		if (m_tMgr == nullptr)
			return;
		TODManager* todMgr = m_tMgr->GetTODManager();

		std::function<void()> pSetMeshOrTextureNameState = nullptr;
		Echo::CloudMesh* pCloudMesh = nullptr;
		int id = idx - eCloudType_Mesh_Up;
		std::vector<String>& texName = mContext.mTextureName;
		String pNewTexture = texName[id];

		if (idx == eCloudType_Mesh_Up)
		{
			pSetMeshOrTextureNameState = std::bind<void>(&CloudMeshComponent::setUpMeshOrTextureNameState, this);
			pCloudMesh = mCloudUpMesh;
		}
		else if (idx == eCloudType_Mesh_Mid)
		{
			pSetMeshOrTextureNameState = std::bind<void>(&CloudMeshComponent::setMidMeshOrTextureNameState, this);
			pCloudMesh = mCloudMidMesh;
		}
		else if (idx == eCloudType_Mesh_Down)
		{
			pSetMeshOrTextureNameState = std::bind<void>(&CloudMeshComponent::setDownMeshOrTextureNameState, this);
			pCloudMesh = mCloudDownMesh;
		}

		if (pCloudMesh)
		{
			if (pNewTexture.empty())
				pSetMeshOrTextureNameState();
			else
			{
				pSetMeshOrTextureNameState();
				if (pCloudMesh->getVisible() == true)
				{
					std::string pOldTexture;
					pCloudMesh->getCloudEffectTextureName(pOldTexture);
					if (pNewTexture != pOldTexture)
					{
						TexturePtr texPtr_;
						m_tMgr->getTodTexturePtrByName(pNewTexture, texPtr_);
						if (texPtr_.isNull())
						{
							if (Root::instance()->getRuntimeMode() != Root::eRM_Editor)
							{

								if (/*!texPtr_.isNull()*/TextureManager::instance()->resourceExists(Name(pNewTexture)))
								{
									texPtr_ = static_cast<TexturePtr>(TextureManager::instance()->getByName(Name(pNewTexture)));
									if (texPtr_->isLoaded())
									{
										m_tMgr->addTodTexturePtr(pNewTexture.c_str(), texPtr_);
										if (todMgr)
											todMgr->setCurrentCloudDiffuseNameByCom(idx, pNewTexture);
										pCloudMesh->setCloudTextureFromTOD(texPtr_);
										updateCloudMeshMaterial(pCloudMesh, id);
										return;
									}
									else if (texPtr_->isPrepared())
									{
										texPtr_->load();
										m_tMgr->addTodTexturePtr(pNewTexture.c_str(), texPtr_);
										if (todMgr)
											todMgr->setCurrentCloudDiffuseNameByCom(idx, pNewTexture);
										pCloudMesh->setCloudTextureFromTOD(texPtr_);
										updateCloudMeshMaterial(pCloudMesh, id);
										return;
									}
									else
									{
										ResourceLoadPriority priority = WorkQueue::MakePriority(100, WorkQueue::RESOURCE_TYPE_SKY);
										requestResouce(Name(pNewTexture), TextureManager::instance()->getResourceType(), priority);

										if (TODSystem::instance()->showLog(LogType::LT_CloudMesh))
											LogManager::instance()->logMessage(" [CloudMesh] setTexture Error : " + pNewTexture + " -- ra");
									}
								}
								else
								{
									ResourceLoadPriority priority = WorkQueue::MakePriority(100, WorkQueue::RESOURCE_TYPE_SKY);
									requestResouce(Name(pNewTexture), TextureManager::instance()->getResourceType(), priority);
									if (TODSystem::instance()->showLog(LogType::LT_CloudMesh))
										LogManager::instance()->logMessage(" [CloudMesh] setTexture Error : " + pNewTexture + " -- a");
								}
							}
							else
							{
								texPtr_ = TextureManager::instance()->load(Name(pNewTexture));
								if (todMgr)
									todMgr->setCurrentCloudDiffuseNameByCom(idx, pNewTexture);
								pCloudMesh->setCloudTextureFromTOD(texPtr_);
								updateCloudMeshMaterial(pCloudMesh, id);
							}
						}
						else
						{
							if (todMgr)
								todMgr->setCurrentCloudDiffuseNameByCom(idx, pNewTexture);
							pCloudMesh->setCloudTextureFromTOD(texPtr_);
							updateCloudMeshMaterial(pCloudMesh, id);
						}
					}
				}
			}
		}
	}

	void CloudMeshComponent::_preLoadTextureIdx(int idx)
	{
		int id = idx - eCloudType_Mesh_Up;
		std::vector<String>& texName = mContext.mTextureName;

		if (texName[id].empty())
			_notifyFinish(id, RESTEXTURE);
		else
		{
			TODManager* todMgr = m_tMgr->GetTODManager();
			TexturePtr texPtr_;
			bool isLoaded = loadTextureData(texName[id], texPtr_);
			if (isLoaded)
			{
				if (todMgr)
					todMgr->setCurrentCloudDiffuseNameByCom(idx, texName[id]);
				_notifyFinish(id, RESTEXTURE);
			}
		}
	}

	void CloudMeshComponent::_prePerpareMeshIdx(int idx)
	{
		int id = idx - eCloudType_Mesh_Up;
		std::vector<String>& meshName = mContext.mMeshName;
		if (meshName[id].empty())
			_notifyFinish(id, RESMESH);
		else
		{
			MeshPtr meshPtr;
			bool isLoaded = prepareMeshData(meshName[id], meshPtr);
			if (isLoaded)
				_notifyFinish(id, RESMESH);
		}
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
		setCloudVisible(mContext.mCloudVisible);
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
		mContext.mCloudVisible = isShow;

		mContext.mCloudRealVisible[0] = isShow;
		mContext.mCloudRealVisible[1] = isShow;
		mContext.mCloudRealVisible[2] = isShow;

		if (mCloudUpMesh)
			mCloudUpMesh->setVisible(isShow);
		if (mCloudMidMesh)
			mCloudMidMesh->setVisible(isShow);
		if(mCloudDownMesh)
			mCloudDownMesh->setVisible(isShow);

		_loadAndSetTextureIdx(eCloudType_Mesh_Up);
		_loadAndSetTextureIdx(eCloudType_Mesh_Mid);
		_loadAndSetTextureIdx(eCloudType_Mesh_Down);

		setUpMeshOrTextureNameState();
		setMidMeshOrTextureNameState();
		setDownMeshOrTextureNameState();
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

		setUpMeshOrTextureNameState();
		setMidMeshOrTextureNameState();
		setDownMeshOrTextureNameState();

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

		setCloudVisible(mContext.mCloudVisible);
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
