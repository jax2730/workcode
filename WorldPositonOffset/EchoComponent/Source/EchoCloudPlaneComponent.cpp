// --------------------------------------------------------------------------------------
// File:   EchoCloudPlaneComponent.cpp
// 
// Author: Chen ShiWen
//
// Date:   2022-12-12
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
// --------------------------------------------------------------------------------------
#include "EchoComponentStableHeaders.h"
#include "EchoCloudPlaneComponent.h"

#include "EchoCloud.h"
#include "EchoTODComponentManager.h"
#include "EchoResourceFolderManager.h"

#include "EchoRenderStrategy.h"
#include "EchoPostProcess.h"
#include "EchoLogManager.h"
#include "EchoTODManager.h"
#include "EchoTODComponentSystem.h"
#include "EchoTODDataUtil.h"

namespace Echo {

	DECLAREACTORCOMTYPE_CPP(CloudPlaneComponent);

	Name planeColorName_ = Name("Colour_Color");
	Name planeIntensityName_ = Name("Intensity");
	Name planeHDRIntensityName_ = Name("HDRIntensity");
	Name planeBackColorName_ = Name("BackColor");
	Name planeBackColorIntensityName_ = Name("BackColorIntensity");
	Name planeOpacityName_ = Name("Opacity");
	Name planeURollSpeedName_ = Name("URollSpeed");
	Name planeVRollSpeedName_ = Name("VRollSpeed");
	Name planeUVTillingName_ = Name("UVTilling");
	Name planeEmendationName_ = Name("Emendation");

	CloudPlaneComponent::CloudPlaneComponent(ActorPtr pActor, ActorCOMMoveableType movableType) :
		ActorComponent(pActor, movableType)
	{
#ifdef ECHO_EDITOR
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_CLOUD_PLANE_U_ROLLSPEED,			planeURollSpeedName_.toString(),			1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_CLOUD_PLANE_V_ROLLSPEED,			planeVRollSpeedName_.toString(),			1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_CLOUD_PLANE_UV_TILLING,			planeUVTillingName_.toString(),				1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_CLOUD_PLANE_OPACITY,				planeOpacityName_.toString(),				1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_CLOUD_PLANE_EMENDATION,			planeEmendationName_.toString(),			1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_CLOUD_PLANE_COLOR,					planeColorName_.toString(),					4);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_CLOUD_PLANE_COLOR_POWER,			planeIntensityName_.toString(),				1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_CLOUD_PLANE_HDR_POWER,				planeHDRIntensityName_.toString(),			1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_CLOUD_PLANE_SUNBACK_COLOR,			planeBackColorName_.toString(),				4);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_CLOUD_PLANE_SUNBACK_COLOR_POWER,	planeBackColorIntensityName_.toString(),	1);
#endif // ECHO_EDITOR
	}

	CloudPlaneComponent::~CloudPlaneComponent()
	{
	}

	void CloudPlaneComponent::init(ActorComponentInfo* info)
	{
		if (info != nullptr)
		{
			CloudPlaneComponentInfo* comInfo = static_cast<CloudPlaneComponentInfo*>(info);
			mContext = comInfo->mContext;

#ifdef ECHO_EDITOR
			m_bNewCreateCom = comInfo->m_bNewCreateCom;
#endif
		}

		TODCOMPONENT_INITMGR(getCloudPlaneContext, m_tMgr, true);
		
		if (!m_tMgr)
		{
			m_bCreateFinish = true;
			LogManager::instance()->logMessage("CloudPlaneComponent::init TODComponentManager object does not exist!");
			return;
		}

		_preLoadTexture(mContext.mTextureName, 0);
		_preLoadTexture(mContext.mStarsTextureName, 1);
	}

	void CloudPlaneComponent::unInit()
	{
		if(m_tMgr)
			m_CloudPlane = nullptr;
		TODCOMPONENT_UNINITMGR(getCloudPlaneContext, m_tMgr, T_CloudPlane);
	}

	void CloudPlaneComponent::resourceArrived(const Name& resName, const Name& resType, bool bFail, const ResourceBasePtr& resourcePtr)
	{
		if (resourcePtr.isNull())
		{
			std::string logInfo = "";
			logInfo.append("Texture :").append(resName.c_str()).append(" load false");
			LogManager::getSingleton().logMessage(Echo::LML_CRITICAL, logInfo.data());
			return;
		}
	
		if(m_tMgr)
			m_tMgr->addTodTexturePtr(resName.c_str(), static_cast<TexturePtr>(resourcePtr));

		if (resName.c_str() == mContext.mTextureName)
			_notifyFinish(0);
		else if (resName.c_str() == mContext.mStarsTextureName)
			_notifyFinish(1);
	}

	void CloudPlaneComponent::setCloudPlaneVisible(bool visible_)
	{
		mContext.mCloudPlaneVisible = visible_;
		if (m_CloudPlane == nullptr )
			return;
		m_CloudPlane->setVisible(visible_);
	}

	void CloudPlaneComponent::setTextureName(const std::string & textureName)
	{
		_loadAndSetTextureName(textureName);
		
	}

	void CloudPlaneComponent::setStarsTextureName(const std::string& texName)
	{
		_loadAndSetStarsTextureName(texName);
	}

	void CloudPlaneComponent::setColour(const ColorValue & colour)
	{
		mContext.mColour = colour;
		setCloudPlaneColor();
	}

	void CloudPlaneComponent::setIntensity(const float& intensity_)
	{
		mContext.mIntensity = intensity_;
		setCloudPlaneColor();
	}

	void CloudPlaneComponent::setHDRIntensity(const float& hdrIntensity_)
	{
		mContext.mHDRIntensity = hdrIntensity_;
		setCloudPlaneColor();
		setCloudPlaneBackColor();
	}

	void CloudPlaneComponent::setBackColor(const ColorValue& backColor_)
	{
		mContext.mBackColor = backColor_;
		setCloudPlaneBackColor();
	}

	void CloudPlaneComponent::setBackColorIntensity(const float& backIntensity)
	{
		mContext.mBackColorIntensity = backIntensity;
		setCloudPlaneBackColor();
	}

	void CloudPlaneComponent::setOpacity(const float& opacity)
	{
		float p = std::max(0.0f, opacity);
		if (m_CloudPlane) {
			m_CloudPlane->setCloudOpacity(p);
		}
		mContext.mOpacity = p;
	}

	void CloudPlaneComponent::setHeight(const float& height)
	{
		if (m_CloudPlane && m_CloudPlane->getCloudHeight() != Vector2(height)) {
			m_CloudPlane->setCloudHeight(Vector2(height));
		}
		mContext.mHeight = height;
	}

	void CloudPlaneComponent::setURollSpeed(const float& uSpeed)
	{
		mContext.mURollSpeed = uSpeed;
		if (m_CloudPlane == nullptr)
			return;
		m_CloudPlane->setCloudUSpeed(uSpeed);
	}

	void CloudPlaneComponent::setVRollSpeed(const float& vSpeed)
	{
		mContext.mVRollSpeed = vSpeed;
		if (m_CloudPlane == nullptr)
			return;
		m_CloudPlane->setCloudVSpeed(vSpeed);
	}

	void CloudPlaneComponent::setUVTilling(const float& uvTilling)
	{
		mContext.mUVTilling = uvTilling;
		if (m_CloudPlane == nullptr)
			return;
		m_CloudPlane->setCloudUVTilling(uvTilling);
	}

	void CloudPlaneComponent::setEmendation(const float& emendation)
	{
		mContext.mEmendation = emendation;
		if (m_CloudPlane == nullptr)
			return;
		m_CloudPlane->setCloudEmendation(emendation);
	}

	bool CloudPlaneComponent::getIsMoonRise()
	{
		if(!m_CloudPlane)
		   return false;
		return m_CloudPlane->getMoonRise();
	}

	//void CloudPlaneComponent::setCloudPlaneRealVisible()
	//{
	//	if (!m_CloudPlane)
	//		return;

	//	if (getIsMoonRise())
	//	{
	//		if (mContext.mStarsTextureName.empty())
	//			m_CloudPlane->setVisible(false);
	//		else if (mContext.mCloudPlaneVisible)
	//			m_CloudPlane->setVisible(true);
	//			
	//	}
	//	else
	//	{
	//		if (mContext.mTextureName.empty())
	//			m_CloudPlane->setVisible(false);
	//		else if (mContext.mCloudPlaneVisible)
	//			m_CloudPlane->setVisible(true);
	//	}

	//}

	void CloudPlaneComponent::use()
	{
		if (!m_tMgr)
			return;

		SceneManager* sMgr = m_tMgr->GetSceneManager();
		if (sMgr == nullptr)
			return;
		Cloud* cloud_ = sMgr->getCloud();
		m_CloudPlane = m_tMgr->getComCloudPlane();

		if (cloud_ == nullptr || m_CloudPlane == nullptr)
			return;

#ifdef ECHO_EDITOR
		if (m_bNewCreateCom)
		{
			std::string texName_;
			if (cloud_->getCloudPlane())
				texName_ = cloud_->getCloudPlane()->getCloudTextureName();

			if (mContext.mTextureName.empty())
				mContext.mTextureName = texName_;

			m_bNewCreateCom = false;
		}
#endif

		if (m_tMgr->getIsWriteLog())
		{
			std::string logInfo = "";
			logInfo.append("CloudPlane : use").append(getCombinedName());
			LogManager::getSingleton().logMessage(Echo::LML_CRITICAL, logInfo.data());
		}

		setCloudPlaneVisible(mContext.mCloudPlaneVisible);
		setTextureName(mContext.mTextureName);
		if(!mContext.mStarsTextureName.empty())
		   setStarsTextureName(mContext.mStarsTextureName);
	    setOpacity(mContext.mOpacity);
	
		setURollSpeed(mContext.mURollSpeed);
		setVRollSpeed(mContext.mVRollSpeed);
		setUVTilling(mContext.mUVTilling);
		setEmendation(mContext.mEmendation);
        setPriority(mContext.mPriority);
	    setCloudPlaneColor();
		setCloudPlaneBackColor();
		setHeight(mContext.mHeight);
	}

	void CloudPlaneComponent::unuse()
	{
		m_CloudPlane = nullptr;
		if (m_tMgr && m_tMgr->getIsWriteLog())
		{
			std::string logInfo = "";
			logInfo.append("CloudPlane : unuse").append(getCombinedName());
			LogManager::getSingleton().logMessage(Echo::LML_CRITICAL, logInfo.data());
		}
	}

	void CloudPlaneComponent::setPriority(int priority)
	{
		mContext.mPriority = priority;
		if(m_tMgr)
			m_tMgr->adjustComponentPriority(m_tMgr->getCloudPlaneContext(), this);
	}

	void CloudPlaneComponent::setCloudPlaneColor()
	{
		if (m_CloudPlane == nullptr)
			return;
		float tempPower = mContext.mIntensity;
		bool flag = Root::instance()->getRenderSystem()->getCurRenderStrategy()->getPostProcessChain()->isEnable(PostProcess::HDR);
		if (flag)
			tempPower *= mContext.mHDRIntensity;
		m_CloudPlane->setCloudColour(tempPower * mContext.mColour);
	}

	void CloudPlaneComponent::setCloudPlaneBackColor()
	{
		if (m_CloudPlane == nullptr)
			return;
		float tempPower = mContext.mBackColorIntensity;
		bool flag = Root::instance()->getRenderSystem()->getCurRenderStrategy()->getPostProcessChain()->isEnable(PostProcess::HDR);
		if (flag)
			tempPower *= mContext.mHDRIntensity;
		m_CloudPlane->setCloudSunBackColour(tempPower * mContext.mBackColor);
	}


	void CloudPlaneComponent::setTimelineAttribute(std::vector<float>& data,const Name& name) 
	{
		if (!m_tMgr)
			return;
		CloudPlaneComponent* activeCom = m_tMgr->getCloudPlaneContext().mActiveCom;
		if (activeCom == nullptr || activeCom != this )
			return;
		m_tMgr->setTodComChangeState(T_CloudPlane);
		//setCloudPlaneVisible(mContext.mCloudPlaneVisible);
		//setStarsTextureName(mContext.mStarsTextureName);
		//setTextureName(mContext.mTextureName);
		//setHeight(mContext.mHeight);

		if (name == planeColorName_)
			setColour(ColorValue(data[0], data[1], data[2], data[3]));
		else if (name == planeIntensityName_)
			setIntensity(data[0]);
		else if (name == planeHDRIntensityName_)
			setHDRIntensity(data[0]);
		else if (name == planeBackColorName_)
			setBackColor(ColorValue(data[0], data[1], data[2], data[3]));
		else if (name == planeBackColorIntensityName_)
			setBackColorIntensity(data[0]);
		else if (name == planeOpacityName_)
			setOpacity(data[0]);
		else if (name == planeURollSpeedName_)
			setURollSpeed(data[0]);
		else if (name == planeVRollSpeedName_)
			setVRollSpeed(data[0]);
		else if (name == planeUVTillingName_)
			setUVTilling(data[0]);
		else if (name == planeEmendationName_)
			setEmendation(data[0]);
	}

	void CloudPlaneComponent::visibleStateChanged(bool isVisible)
	{
		if (!m_tMgr)
			return;
		if (isVisible)
			m_tMgr->pushComponent(m_tMgr->getCloudPlaneContext(), this);
		else
			m_tMgr->popComponent(m_tMgr->getCloudPlaneContext(), this);
	}

	bool CloudPlaneComponent::_notifyFinish(int type)
	{
		if (!m_bCreateFinish)
			m_bCreateTex |= (1 << type);
		
		if (m_bCreateTex == 0b11 && !m_bCreateFinish)
		{
			m_bCreateFinish = true;
			m_pActor->notifyComponentFinish(weak_from_this());
		}

		CloudPlaneComponent* activeCom = m_tMgr->getCloudPlaneContext().mActiveCom;
		if (activeCom == nullptr || activeCom != this)
			return false;	// 不为当前生效组件
		return true;
	}

	void CloudPlaneComponent::_preLoadTexture(const std::string& textureName, int idx)
	{
		if (textureName.empty())
			_notifyFinish(idx);
		else
		{
			TexturePtr texPtr;
			m_tMgr->getTodTexturePtrByName(textureName, texPtr);
			if (!texPtr.isNull())
				_notifyFinish(idx);
			else
			{
				if (Root::instance()->getRuntimeMode() != Root::eRM_Editor)
				{
					ResourceLoadPriority priority = WorkQueue::MakePriority(100, WorkQueue::RESOURCE_TYPE_SKY);
					requestResouce(Name(textureName), TextureManager::instance()->getResourceType(), priority);
					return;
				}

				texPtr = TextureManager::instance()->load(Name(textureName));
				m_tMgr->addTodTexturePtr(textureName, texPtr);
				_notifyFinish(idx);
			}
		}
	}

	void CloudPlaneComponent::_loadAndSetTextureName(const std::string& textureName)
	{
		mContext.mTextureName = textureName;

		if (m_CloudPlane == nullptr)
			return;

		if (textureName.empty())
			return;

		/*if (m_CloudPlane->getVisible() == false)
			return;*/

		if (!m_tMgr)
			return;
		TexturePtr texPtr;
		m_tMgr->getTodTexturePtrByName(textureName, texPtr);
		if (!texPtr.isNull())
			m_CloudPlane->setCloudTextureName(textureName);
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
						m_CloudPlane->setCloudTextureName(textureName);
						return;
					}
					else if (pResource->isPrepared())
					{
						pResource->load();
						m_tMgr->addTodTexturePtr(textureName, static_cast<TexturePtr>(pResource));
						m_CloudPlane->setCloudTextureName(textureName);
						return;
					}
					else
					{
						/*ResourceLoadPriority priority = WorkQueue::MakePriority(100, WorkQueue::RESOURCE_TYPE_SKY);
						requestResouce(Name(textureName), TextureManager::instance()->getResourceType(), priority);*/

						if (TODSystem::instance()->showLog(LogType::LT_CloudPlane))
							LogManager::instance()->logMessage(" [CloudPlane] setTexture Error : " + textureName + " -- ra");
					}
				}
				else
				{
					/*ResourceLoadPriority priority = WorkQueue::MakePriority(100, WorkQueue::RESOURCE_TYPE_SKY);
					requestResouce(Name(textureName), TextureManager::instance()->getResourceType(), priority);*/

					if (TODSystem::instance()->showLog(LogType::LT_CloudPlane))
						LogManager::instance()->logMessage(" [CloudPlane] setTexture Error : " + textureName + " -- a");
				}
			}
			else
			{
				texPtr = TextureManager::instance()->load(Name(textureName));
				m_tMgr->addTodTexturePtr(textureName, texPtr);
				m_CloudPlane->setCloudTextureName(textureName);
			}
		}
	}

	void CloudPlaneComponent::_loadAndSetStarsTextureName(const std::string& texName)
	{
		mContext.mStarsTextureName = texName;
		if (m_CloudPlane == nullptr)
			return;
		if (texName.empty())
			return;
		if (!m_tMgr)
			return;
		TexturePtr texPtr;
		m_tMgr->getTodTexturePtrByName(texName, texPtr);
		if (!texPtr.isNull())
			m_CloudPlane->setStarsTextureName(texName);
		else
		{
			if (Root::instance()->getRuntimeMode() != Root::eRM_Editor)
			{
				Name starTexName = Name(texName);

				if (/*!pResource.isNull()*/TextureManager::instance()->resourceExists(starTexName))
				{
					ResourceBasePtr pResource = TextureManager::instance()->getByName(starTexName);
					if (pResource->isLoaded())
					{
						m_tMgr->addTodTexturePtr(texName, static_cast<TexturePtr>(pResource));
						m_CloudPlane->setStarsTextureName(texName);
						return;
					}
					else if (pResource->isPrepared())
					{
						pResource->load();
						m_tMgr->addTodTexturePtr(texName, static_cast<TexturePtr>(pResource));
						m_CloudPlane->setStarsTextureName(texName);
						return;
					}
				}

				//assert(!"CloudPlaneComponent::_loadAndSetStarsTextureName Failed to submit Texture creation request!");
				if (TODSystem::instance()->showLog(LogType::LT_CloudPlane))
					LogManager::instance()->logMessage("[CloudPlane] CloudPlaneComponent::_loadAndSetStarsTextureName Failed to submit Texture creation request!");
			}
			else
			{
				texPtr = TextureManager::instance()->load(Name(texName));
				m_tMgr->addTodTexturePtr(texName, texPtr);
				m_CloudPlane->setStarsTextureName(texName);
			}
		}
	}

	// ---------------------------------------------------------------------------------------
	EDITOR_REFLECT_BEGIN(CloudPlaneComponentInfo)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mCloudPlaneVisible, "CloudPlaneVisible", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mTextureName, "TextureName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mColour, "Colour", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mIntensity, "Intensity", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mHDRIntensity, "HDRIntensity", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mBackColor, "BackColor", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mBackColorIntensity, "BackColorIntensity", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mOpacity, "Opacity", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mHeight, "Height", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mURollSpeed, "URollSpeed", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mVRollSpeed, "VRollSpeed", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mUVTilling, "UVTilling", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mEmendation, "Emendation", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mPriority, "Priority", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mTransFormTime, "TransTime", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		//EDITOR_REFLECT_MEMBER_UPDATE(mContext.mMeshOrTextureNameEmpty, "MeshOrTextureNameEmpty", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)

		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mStarsTextureName, "StarsTextureName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)

	EDITOR_REFLECT_END()
	
	CloudPlaneComponentInfo::CloudPlaneComponentInfo(const Name & comType, ActorCOMMoveableType type) :
		ActorComponentInfo(comType, type)
	{
	}

	void CloudPlaneComponentInfo::ExportData(cJSON * jsonNode)
	{
		reflect::TypeResolver<CloudPlaneComponentInfo>::get()->ExportJson(this, jsonNode, "");
	}

	void CloudPlaneComponentInfo::ImportData(cJSON * jsonNode)
	{
		reflect::TypeResolver<CloudPlaneComponentInfo>::get()->ImportJson(this, jsonNode, "");

#ifdef ECHO_EDITOR
		m_bNewCreateCom = false;
#endif
	}

	void CloudPlaneComponentInfo::CopyTo(ActorComponentInfo * comInfo)
	{
		if (comInfo == nullptr)
			return;
		CloudPlaneComponentInfo* targetInfo = static_cast<CloudPlaneComponentInfo*>(comInfo);
		reflect::TypeResolver<CloudPlaneComponentInfo>::get()->copyTo(this, targetInfo);

#ifdef ECHO_EDITOR
		targetInfo->m_bNewCreateCom = false;
#endif
	}

	void CloudPlaneComponentInfo::OverWrite() {
		if (mOriginComInfo != nullptr) {
			reflect::TypeResolver<CloudPlaneComponentInfo>::get()->overWrite(mOriginComInfo, this);
		}
	}
#ifdef ECHO_EDITOR
	bool CloudPlaneComponentInfo::getNeedCollectedResource(std::unordered_multimap<ResourceType, std::string>& resMap)
	{
		bool isCollectTex = false;
		if (!mContext.mTextureName.empty())
		{
	        resMap.insert(std::make_pair(ResourceType::NeedCopyRes,mContext.mTextureName));
			isCollectTex = true;
		}
		if (!mContext.mStarsTextureName.empty())
		{
			resMap.insert(std::make_pair(ResourceType::NeedCopyRes, mContext.mStarsTextureName));
			isCollectTex = true;
		}
		return isCollectTex;
	}
#endif

	
}
