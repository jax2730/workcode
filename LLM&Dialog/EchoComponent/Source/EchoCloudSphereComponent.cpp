// --------------------------------------------------------------------------------------
// File:   EchoCloudSphereComponent.cpp
// 
// Author: Chen ShiWen
//
// Date:   2022-12-12
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
// --------------------------------------------------------------------------------------
#include "EchoComponentStableHeaders.h"
#include "EchoCloudSphereComponent.h"

#include "EchoCloud.h"
#include "EchoRenderStrategy.h"
#include "EchoPostProcess.h"
#include "EchoTODComponentManager.h"
#include "EchoResourceFolderManager.h"
#include "EchoTODManager.h"
#include "EchoLogManager.h"
#include "EchoTODComponentSystem.h"
#include "EchoTODDataUtil.h"
#include "EchoMeshMgr.h"

namespace Echo {

	DECLAREACTORCOMTYPE_CPP(CloudSphereComponent);

	Name sphereColorName_ = Name("Colour_Color");
	Name sphereOpacityName_ = Name("Opacity");
	Name sphereSpeedName_ = Name("Speed");
	Name sphereEmendationName_ = Name("Emendation");
	Name sphereIntensityName_ = Name("Intensity");
	Name sphereHDRIntensityName_ = Name("HDRIntensity");
	Name sphereBackColorName_ = Name("CloudSphereBackColor");
	Name sphereBackIntensityName_ = Name("BackIntensity");
	Name sphereTextureName_ = Name("CloudSphereTextureName");

	const String RESTEXTURE = "Texture";
	const String RESMESH = "Mesh";

	CloudSphereComponent::CloudSphereComponent(ActorPtr pActor, ActorCOMMoveableType movableType) :
		ActorComponent(pActor, movableType) 
	{
#ifdef ECHO_EDITOR
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_CLOUD_SPEED,				sphereSpeedName_.toString(),			1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_CLOUD_OPACITY,				sphereOpacityName_.toString(),			1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_CLOUD_EMENDATION,			sphereEmendationName_.toString(),		1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_CLOUD_COLOR,				sphereColorName_.toString(),			4);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_CLOUD_COLOR_POWER,			sphereIntensityName_.toString(),		1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_CLOUD_HDR_POWER,			sphereHDRIntensityName_.toString(),		1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_CLOUD_SUNBACK_COLOR,		sphereBackColorName_.toString(),		4);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_CLOUD_SUNBACK_COLOR_POWER, sphereBackIntensityName_.toString(),	1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_CLOUD_TEXTURE,				sphereTextureName_.toString(),			1);
#endif // ECHO_EDITOR
	}

	CloudSphereComponent::~CloudSphereComponent()
	{
	}

	void CloudSphereComponent::init(ActorComponentInfo* info)
	{
		if (info != nullptr)
		{
			CloudSphereComponentInfo* comInfo = static_cast<CloudSphereComponentInfo*>(info);
			mContext = comInfo->mContext;
#ifdef ECHO_EDITOR
			m_bNewCreateCom = comInfo->m_bNewCreateCom;
#endif
		}

		TODCOMPONENT_INITMGR(getCloudSphereContext, m_tMgr, true);

		if (!m_tMgr)
		{
			m_bCreateFinish = true;
			LogManager::instance()->logMessage("CloudSphereComponent::init TODComponentManager object does not exist!");
			return;
		}

		_preLoadTextureName(mContext.mTextureName);
		_prePrepareMeshName(mContext.mMeshName);
	}

	bool CloudSphereComponent::isTextureAttribute(const Echo::Name& name)
	{
		return name == sphereTextureName_;
	}

	void CloudSphereComponent::setTextureName(const std::string & textureName)
	{
		mContext.mTextureName = textureName;
		_loadAndSetTextureName(textureName);
	}

	void CloudSphereComponent::setCloudSphereVisible(bool visible_)
	{	
		mContext.mCloudSphereVisible = visible_;
		if (m_CloudSphere)
			m_CloudSphere->setVisible(visible_);
		//因目前实现在云不可见时不实际更新所用贴图，所以需要额外调用一次相关逻辑
		if(visible_)
		   _loadAndSetTextureName(mContext.mTextureName);
	}

	void CloudSphereComponent::setMeshName(const std::string& meshName_)
	{
		mContext.mMeshName = meshName_;
		if (m_CloudSphere)
		{
			m_CloudSphere->setMeshFile(meshName_);
#ifdef ECHO_EDITOR
			setTextureName(mContext.mTextureName);
#endif // ECHO_EDITOR

		}
	}
	
	void CloudSphereComponent::setColour(const ColorValue & colour)
	{
		mContext.mColour = colour;
		setCloudSphereColour();
	}

	void CloudSphereComponent::setIntensity(const float& intensity_)
	{
		mContext.mIntensity = intensity_;
		setCloudSphereColour();
	}

	void CloudSphereComponent::setHDRIntensity(const float& HDRIntensity_)
	{
		mContext.mHDRIntensity = HDRIntensity_;
		bool flag = Root::instance()->getRenderSystem()->getCurRenderStrategy()->getPostProcessChain()->isEnable(PostProcess::HDR);
		if (!flag)
			return;
		setCloudSphereColour();
		setCloudSphereBackColour();
	}

	void CloudSphereComponent::setBackColour(const ColorValue& backColour_)
	{
		mContext.mBackColour = backColour_;
		setCloudSphereBackColour();
	}

	void CloudSphereComponent::setBackColorIntensity(const float& backColorIntensity_)
	{
		mContext.mBackColorIntensity = backColorIntensity_;
		setCloudSphereBackColour();
	}

	void CloudSphereComponent::unInit()
	{
		TODCOMPONENT_UNINITMGR(getCloudSphereContext, m_tMgr, T_CloudSphere);
		m_CloudSphere = nullptr;
	}

	bool CloudSphereComponent::isCreateFinish()
	{
		return m_bCreateFinish;
	}

	void CloudSphereComponent::setSpeed(const float& speed_)
	{
		if (m_CloudSphere)
			m_CloudSphere->setCloudSpeed(speed_);
		mContext.mSpeed = speed_;
	}

	void CloudSphereComponent::setOpacity(const float& opacity)
	{
		if (m_CloudSphere)
			m_CloudSphere->setCloudOpacity(opacity);
		mContext.mOpacity = opacity;
	}

	void CloudSphereComponent::setEmendation(const float& emendation_)
	{
		if (m_CloudSphere)
			m_CloudSphere->setCloudEmendation(emendation_);
		mContext.mEmendation = emendation_;
	}

	void CloudSphereComponent::setPriority(const int& priority)
	{
		mContext.mPriority = priority;
		if(m_tMgr)
		   m_tMgr->adjustComponentPriority(m_tMgr->getCloudSphereContext(), this);
	}

	void CloudSphereComponent::setCloudSphereColour()
	{
		if (m_CloudSphere == nullptr)
			return;
		float tempPower = mContext.mIntensity;
		bool flag = Root::instance()->getRenderSystem()->getCurRenderStrategy()->getPostProcessChain()->isEnable(PostProcess::HDR);
		if (flag)
			tempPower *= mContext.mHDRIntensity;
		m_CloudSphere->setCloudColour(tempPower * mContext.mColour);
	}

	void CloudSphereComponent::setCloudSphereBackColour()
	{
		if (m_CloudSphere == nullptr)
			return;
		float tempPower = mContext.mBackColorIntensity;
		bool flag = Root::instance()->getRenderSystem()->getCurRenderStrategy()->getPostProcessChain()->isEnable(PostProcess::HDR);
		if (flag)
			tempPower *= mContext.mHDRIntensity;
		m_CloudSphere->setCloudSunBackColour(tempPower * mContext.mBackColour);
	}

	void CloudSphereComponent::setCloudSphereRealVisible()
	{
		if (!m_CloudSphere)
			return;

		if (!mContext.mTextureName.empty() && !mContext.mMeshName.empty())
		{
			if (mContext.mCloudSphereVisible)
				m_CloudSphere->setVisible(true);
		}
		else
		{
			if (m_CloudSphere->getVisible() != false)
			{
				std::stringstream ss;
				ss << "CloudSphereComponent : " << getComNameToString() << "Mesh Or Texture Is Empty";
				ss << " Mesh Name : " << mContext.mMeshName.c_str() << "  TextureName : " << mContext.mTextureName.c_str() << "\n";
				LogManager::instance()->logMessage(Echo::LML_CRITICAL, ss.str());
			}
			m_CloudSphere->setVisible(false);
		}
	}

	void CloudSphereComponent::use()
	{
		if (!m_tMgr)
			return;

		SceneManager* sMgr = m_tMgr->GetSceneManager();
		if (sMgr == nullptr)
			return;

		Cloud* cloud_ = sMgr->getCloud();
		m_CloudSphere = m_tMgr->getComCloudSphere();
		if (cloud_ == nullptr || m_CloudSphere == nullptr)
			return;

		if (m_tMgr->getIsWriteLog())
		{
			std::string logInfo = "";
			logInfo.append("CloudSphere : use").append(getCombinedName());
			LogManager::getSingleton().logMessage(Echo::LML_CRITICAL, logInfo.data());
		}


#ifdef ECHO_EDITOR
		if (m_bNewCreateCom)
		{
			if (mContext.mMeshName.empty())
				mContext.mMeshName = m_CloudSphere->getMeshFile();

			if (mContext.mTextureName.empty())
			{
				m_CloudSphere->getCloudEffectTextureName(mContext.mTextureName);
			}
			m_bNewCreateCom = false;
		}
#endif // EchoDE

	
		setMeshName(mContext.mMeshName);
		setTextureName(mContext.mTextureName);
		setSpeed(mContext.mSpeed);
		setOpacity(mContext.mOpacity);
		setEmendation(mContext.mEmendation);
		setCloudSphereVisible(mContext.mCloudSphereVisible);
        setPriority(mContext.mPriority);
		setCloudSphereColour();
		setCloudSphereBackColour();
		setCloudSphereRealVisible();
	}

	void CloudSphereComponent::unuse()
	{
		m_CloudSphere = nullptr;
		if (m_tMgr &&  m_tMgr->getIsWriteLog())
		{
			std::string logInfo = "";
			logInfo.append("CloudSphere : unuse").append(getCombinedName());
			LogManager::getSingleton().logMessage(Echo::LML_CRITICAL, logInfo.data());
		}
	}

	void CloudSphereComponent::setTimelineAttribute(std::vector<float>& data,const Name& name) 
	{
		if (!m_tMgr)
			return;
		CloudSphereComponent* activeCom = m_tMgr->getCloudSphereContext().mActiveCom;
		if (activeCom == nullptr || activeCom != this )
			return;
		m_tMgr->setTodComChangeState(T_CloudSphere);
		setCloudSphereVisible(mContext.mCloudSphereVisible);
		setMeshName(mContext.mMeshName);
		if (name == sphereColorName_)
			setColour(ColorValue(data[0],data[1],data[2],data[3]));
		else if (name == sphereOpacityName_)
			setOpacity(data[0]);
		else if (name == sphereSpeedName_)
			setSpeed(data[0]);
		else if (name == sphereEmendationName_)
			setEmendation(data[0]);
		else if (name == sphereIntensityName_)
			setIntensity(data[0]);
		else if (name == sphereHDRIntensityName_)
			setHDRIntensity(data[0]);
		else if (name == sphereBackColorName_)
			setBackColour(ColorValue(data[0],data[1],data[2],data[3]));
		else if (name == sphereBackIntensityName_)
			setBackColorIntensity(data[0]);
	}

	void CloudSphereComponent::setTimelineStringAttribute(const std::string& value,const Name& attrName)
	{
		if (!m_tMgr)
			return;
		CloudSphereComponent* activeCom = m_tMgr->getCloudSphereContext().mActiveCom;
		if (activeCom == nullptr || activeCom != this )
			return;
		m_tMgr->setTodComChangeState(T_CloudSphere);
		setCloudSphereVisible(mContext.mCloudSphereVisible);
		setMeshName(mContext.mMeshName);
		if(attrName == sphereTextureName_)
		     setTextureName(value);
	}

	void CloudSphereComponent::visibleStateChanged(bool isVisible)
	{
		if (!m_tMgr)
			return;
		if (isVisible)
			m_tMgr->pushComponent(m_tMgr->getCloudSphereContext(), this);
		else
			m_tMgr->popComponent(m_tMgr->getCloudSphereContext(), this);
		
	}

	void CloudSphereComponent::resourceArrived(const Name& resName, const Name& resType, bool bFail, const ResourceBasePtr& resourcePtr)
	{
		if (resourcePtr.isNull())
		{
			std::string logInfo = "";
			logInfo.append("Texture :").append(resName.c_str()).append(" load false");
			LogManager::getSingleton().logMessage(Echo::LML_CRITICAL, logInfo.data());
			return;
		}

		if (resType == TextureManager::instance()->getResourceType())
		{
			if (!resourcePtr.isNull() && resourcePtr->isPrepared())
				resourcePtr->load();

			if (m_tMgr)
			{
				m_tMgr->addTodTexturePtr(resName.c_str(), static_cast<TexturePtr>(resourcePtr));
				TODManager* todMgr = m_tMgr->GetTODManager();
				if (todMgr)
					todMgr->setCurrentCloudDiffuseNameByCom(eCloudType_Sphere, resName.c_str());
			}
			_notifyFinish(RESTEXTURE);
		}
		else if (resType == MeshMgr::instance()->getResourceType())
		{
			if (m_tMgr)
				m_tMgr->addCloudMeshPtr(resName.c_str(), static_cast<MeshPtr>(resourcePtr));
			_notifyFinish(RESMESH);
		}
	}

	bool CloudSphereComponent::_notifyFinish(const std::string& type)
	{
		if (!m_bCreateFinish && type == RESTEXTURE)
			m_bCreateKey |= 1;
		if (!m_bCreateFinish && type == RESMESH)
			m_bCreateKey |= (1 << 1);

		if (!m_bCreateFinish && m_bCreateKey == 0b11)
		{
			m_bCreateFinish = true;
			m_pActor->notifyComponentFinish(weak_from_this());
		}

		CloudSphereComponent* activeCom = m_tMgr->getCloudSphereContext().mActiveCom;
		if (activeCom == nullptr || activeCom != this)
			return false;	// 不为当前生效组件
		return true;
	}

	void CloudSphereComponent::_preLoadTextureName(const std::string& textureName)
	{
		if (textureName.empty())
			_notifyFinish(RESTEXTURE);
		else
		{
			TexturePtr texPtr;
			m_tMgr->getTodTexturePtrByName(textureName, texPtr);

			if(!texPtr.isNull())
				_notifyFinish(RESTEXTURE);
			else
			{
				if (/*texPtr.isNull()*/!TextureManager::instance()->resourceExists(Name(textureName)))
				{
					if (Root::instance()->getRuntimeMode() != Root::eRM_Editor)
					{
						ResourceLoadPriority priority = WorkQueue::MakePriority(100, WorkQueue::RESOURCE_TYPE_SKY);
						requestResouce(Name(textureName), TextureManager::instance()->getResourceType(), priority);
					}
					else
					{
						texPtr = TextureManager::instance()->load(Name(textureName));
						m_tMgr->addTodTexturePtr(textureName, texPtr);
						_notifyFinish(RESTEXTURE);
					}
				}
				else
				{
					texPtr = TextureManager::instance()->getByName(Name(textureName));
					m_tMgr->addTodTexturePtr(textureName, texPtr);
					_notifyFinish(RESTEXTURE);
				}
			}
		}
	}

	void CloudSphereComponent::_loadAndSetTextureName(const std::string& textureName)
	{
		if (m_CloudSphere == nullptr)
			return;
		
		if (textureName.empty())
		{
			setCloudSphereRealVisible();
			return;
		}
		setCloudSphereRealVisible();

		if (!m_CloudSphere->getVisible() || !m_tMgr)
			return;
		std::string tempName;
		m_CloudSphere->getCloudEffectTextureName(tempName);
		if (textureName != tempName)
		{
			TexturePtr texPtr;
			TODManager* todMgr = m_tMgr->GetTODManager();

			m_tMgr->getTodTexturePtrByName(textureName, texPtr);
			if (!texPtr.isNull())
			{
				if (todMgr)
					todMgr->setCurrentCloudDiffuseNameByCom(eCloudType_Sphere, textureName);
				m_CloudSphere->setCloudTextureFromTOD(texPtr);
			}
			else
			{
				if (Root::instance()->getRuntimeMode() != Root::eRM_Editor)
				{
					Name starTexName = Name(textureName);
					if (/*!pResource.isNull()*/TextureManager::instance()->resourceExists(starTexName))
					{
						ResourceBasePtr pResource = TextureManager::instance()->getByName(starTexName);

						if (pResource->isLoaded())
						{
							m_tMgr->addTodTexturePtr(textureName, static_cast<TexturePtr>(pResource));
							if (todMgr)
								todMgr->setCurrentCloudDiffuseNameByCom(eCloudType_Sphere, textureName.c_str());
							m_CloudSphere->setCloudTextureFromTOD(static_cast<TexturePtr>(pResource));
							return;
						}
						else if (pResource->isPrepared())
						{
							pResource->load();
							m_tMgr->addTodTexturePtr(textureName, static_cast<TexturePtr>(pResource));
							if (todMgr)
								todMgr->setCurrentCloudDiffuseNameByCom(eCloudType_Sphere, textureName.c_str());
							m_CloudSphere->setCloudTextureFromTOD(static_cast<TexturePtr>(pResource));
							return;
						}
						else
						{
							ResourceLoadPriority priority = WorkQueue::MakePriority(100, WorkQueue::RESOURCE_TYPE_SKY);
							requestResouce(Name(textureName), TextureManager::instance()->getResourceType(), priority);

							if (TODSystem::instance()->showLog(LogType::LT_CloudSphere))
								LogManager::instance()->logMessage(" [CloudSphere] setTexture Error : " + textureName + " -- ra");
						}
					}
					else
					{
						ResourceLoadPriority priority = WorkQueue::MakePriority(100, WorkQueue::RESOURCE_TYPE_SKY);
						requestResouce(Name(textureName), TextureManager::instance()->getResourceType(), priority);

						if (TODSystem::instance()->showLog(LogType::LT_CloudSphere))
							LogManager::instance()->logMessage(" [CloudSphere] setTexture Error : " + textureName + " -- a");
					}

				}
				else
				{
					texPtr = TextureManager::instance()->load(Name(textureName));
					m_tMgr->addTodTexturePtr(textureName, texPtr);
					if (todMgr)
						todMgr->setCurrentCloudDiffuseNameByCom(eCloudType_Sphere, textureName.c_str());
					m_CloudSphere->setCloudTextureFromTOD(texPtr);
				}
			}
		}
	}

	void CloudSphereComponent::_prePrepareMeshName(const std::string& meshName)
	{
		if (meshName.empty())
			_notifyFinish(RESMESH);
		else
		{
			MeshPtr meshPtr;
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
						return;
					}
				}
			}
			_notifyFinish(RESMESH);
		}
	}

	// ---------------------------------------------------------------------------------------
	EDITOR_REFLECT_BEGIN(CloudSphereComponentInfo)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mSpeed, "Speed", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mOpacity, "Emendation", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mEmendation, "Opacity", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mColour, "Colour", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mIntensity, "Intensity", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mHDRIntensity, "HDRIntensity", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mBackColour, "CloudSphereBackColor", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mBackColorIntensity, "BackIntensity", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mTextureName, "TextureName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mCloudSphereVisible, "CloudSphereVisible", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mMeshName, "MeshName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mPriority, "Priority", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mTransFormTime, "TransTime", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		//EDITOR_REFLECT_MEMBER_UPDATE(mContext.mMeshOrTextureEmpty, "MeshOrTextureEmpty", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_END();

	CloudSphereComponentInfo::CloudSphereComponentInfo(const Name & comType, ActorCOMMoveableType type) :
		ActorComponentInfo(comType, type)
	{
	}

	void CloudSphereComponentInfo::ExportData(cJSON * jsonNode)
	{
		reflect::TypeResolver<CloudSphereComponentInfo>::get()->ExportJson(this, jsonNode, "");
	}

	void CloudSphereComponentInfo::ImportData(cJSON * jsonNode)
	{
		reflect::TypeResolver<CloudSphereComponentInfo>::get()->ImportJson(this, jsonNode, "");
#ifdef ECHO_EDITOR
		m_bNewCreateCom = false;
#endif
	}

	void CloudSphereComponentInfo::CopyTo(ActorComponentInfo * comInfo)
	{
		if (comInfo == nullptr)
			return;
		CloudSphereComponentInfo* targetInfo = static_cast<CloudSphereComponentInfo*>(comInfo);
		reflect::TypeResolver<CloudSphereComponentInfo>::get()->copyTo(this, targetInfo);

#ifdef ECHO_EDITOR
		targetInfo->m_bNewCreateCom = false;
#endif
	} 

	void CloudSphereComponentInfo::OverWrite() {
		if (mOriginComInfo != nullptr) {
			reflect::TypeResolver<CloudSphereComponentInfo>::get()->overWrite(mOriginComInfo, this);
		}
	}

#ifdef ECHO_EDITOR
	bool CloudSphereComponentInfo::getNeedCollectedResource(std::unordered_multimap<ResourceType, std::string>& resMap)
	{
		if(mContext.mTextureName.empty() && mContext.mMeshName.empty())
		   return false;
		if (!mContext.mTextureName.empty())
			resMap.insert(std::make_pair(ResourceType::NeedCopyRes,mContext.mTextureName));
		if (!mContext.mMeshName.empty())
			resMap.insert(std::make_pair(ResourceType::Mesh,mContext.mMeshName));
		return true;
	}
#endif
}
