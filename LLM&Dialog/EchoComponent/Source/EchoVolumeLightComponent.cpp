// --------------------------------------------------------------------------------------
// File:   EchoVolumeLightComponent.h
// 
// Author: Wang YuXuan
//
// Date:   2024-1-31
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
// --------------------------------------------------------------------------------------
#include "EchoComponentStableHeaders.h"
#include "EchoVolumeLightComponent.h"
#include "EchoRenderStrategy.h"

#include "EchoTODComponentManager.h"
#include "EchoLogManager.h"
#include "EchoVideoConfig.h"
#include "EchoTODComponentSystem.h"
#include "EchoTODDataUtil.h"

namespace Echo 
{
	DECLAREACTORCOMTYPE_CPP(VolumeLightComponent);

	const Name VolumeLightSpace::VL_RAYAMOUNT			= Name("RaysAmount");			//光线量
	const Name VolumeLightSpace::VL_RAYSATTENUATION		= Name("RaysAttenuation");		//光线衰减
	const Name VolumeLightSpace::VL_RAYSCOLORINTENSITY	= Name("RaysColorIntensity");	//体积光颜色强度
	const Name VolumeLightSpace::VL_RAYSCUSTOMCOLOR		= Name("RaysCustomColor");		//体积光颜色

	VolumeLightComponent::VolumeLightComponent(ActorPtr pActor, ActorCOMMoveableType movableType)
		: ActorComponent(pActor, movableType)
	{
#ifdef ECHO_EDITOR
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_POST_PROCESS_VOLUME_LIGHT_AMOUNT,			VolumeLightSpace::VL_RAYAMOUNT.toString(),			1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_POST_PROCESS_VOLUME_LIGHT__ATTENUATION,	VolumeLightSpace::VL_RAYSATTENUATION.toString(),	1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_POST_PROCESS_VOLUME_LIGHT__COLORINTENSITY,	VolumeLightSpace::VL_RAYSCOLORINTENSITY.toString(),	1);
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_POST_PROCESS_VOLUME_LIGHT_COLOR,			VolumeLightSpace::VL_RAYSCUSTOMCOLOR.toString(),	4);
#endif // ECHO_EDITOR
	}

	VolumeLightComponent::~VolumeLightComponent()
	{
	}

	void VolumeLightComponent::init(ActorComponentInfo* info)
	{
		TODCOMPONENT_INITMGR(getVolumeLightContext, m_tMgr, false);

		if (info == nullptr) return;

		VolumeLightComponentInfo* pInfo = static_cast<VolumeLightComponentInfo*>(info);

		mContext = pInfo->mContext;
	}

	void VolumeLightComponent::unInit()
	{
		TODCOMPONENT_UNINITMGR(getVolumeLightContext, m_tMgr, T_End);
	}

	void VolumeLightComponent::use()
	{
		if (!m_tMgr)
			return;
		VolumeLightComponentInfo* pInfo = nullptr;
		if (m_pActComInfo)
		{
			pInfo = static_cast<VolumeLightComponentInfo*>(m_pActComInfo);
			bool isActive = Root::instance()->getActiveSceneManager() == m_tMgr->GetSceneManager();
			//非激活场景不设置后效
			if (pInfo->mOpenThis && isActive)
			{
				setIsOpenVolumeLight(true);
				setSunShaftsParams(pInfo->mContext);
				mContext = pInfo->mContext;
			}
		}

		if (pInfo && pInfo->mOpenThis && m_tMgr &&  m_tMgr->getIsWriteLog())
		{
			mIsUsed = true;
			std::string logInfo = "";
			logInfo.append("Post Process VolumeLight : use ").append(getCombinedName());
			LogManager::getSingleton().logMessage(Echo::LML_CRITICAL, logInfo.data());
		}
	}

	void VolumeLightComponent::unuse()
	{
		if (mIsUsed && m_tMgr &&  m_tMgr->getIsWriteLog())
		{
			mIsUsed = false;
			std::string logInfo = "";
			logInfo.append("Post Process VolumeLight : unuse").append(getCombinedName());
			LogManager::getSingleton().logMessage(Echo::LML_CRITICAL, logInfo.data());
		}
	}

	void VolumeLightComponent::setPreferences(bool pOpenThis)
	{
		VolumeLightComponentInfo* pInfo = nullptr;
		if (m_pActComInfo)
		{
			pInfo = static_cast<VolumeLightComponentInfo*>(m_pActComInfo);
			pInfo->mOpenThis = pOpenThis;
			if (isValidComponent())
			{
				setIsOpenVolumeLight(pInfo->mOpenThis);
				setSunShaftsParams(mContext);
			}
		}
	}

	void VolumeLightComponent::setRaysAmount(float raysAmount)
	{
		mContext.mRaysAmount = raysAmount;
		if (isValidComponent())
			setSunShaftsParams(mContext);
	}

	void VolumeLightComponent::setRaysAttenuation(float raysAttenuation)
	{
		mContext.mRaysAttenuation = raysAttenuation;
		if (isValidComponent())
			setSunShaftsParams(mContext);
	}

	void VolumeLightComponent::setRaysColorIntensity(float raysColorIntensity)
	{
		mContext.mRaysColorIntensity = raysColorIntensity;
		if (isValidComponent())
			setSunShaftsParams(mContext);
	}

	void VolumeLightComponent::setRaysCustomColor(const ColorValue& color)
	{
		mContext.mRaysCustomColor = color;
		if (isValidComponent())
			setSunShaftsParams(mContext);
	}

	void VolumeLightComponent::setPriority(int priority)
	{
		mContext.mPriority = std::max(0, std::min(priority, 255));
		if(m_tMgr)
		   m_tMgr->adjustComponentPriority(m_tMgr->getVolumeLightContext(), this);
	}

	VolumeLight* VolumeLightComponent::getVolumeLight()
	{
		RenderSystem* pRenderSystem = Root::instance()->getRenderSystem();
		if (!pRenderSystem) return nullptr;

		RenderStrategy* pStrategy = pRenderSystem->getCurRenderStrategy();
		if (!pStrategy) return nullptr;

		PostProcessChain* pProcessChain = pStrategy->getPostProcessChain();
		if (!pProcessChain) return nullptr;

		VolumeLight* pVolumeLight = static_cast<VolumeLight*>(pProcessChain->getPostProcess(PostProcess::Type::VolumeLight));
		return pVolumeLight;
	}

	void VolumeLightComponent::setIsOpenVolumeLight(bool IsOpen)
	{
		RenderSystem* pRenderSystem = Root::instance()->getRenderSystem();
		if (!pRenderSystem) return;

		RenderStrategy* pStrategy = pRenderSystem->getCurRenderStrategy();
		if (!pStrategy) return;

		PostProcessChain* pProcessChain = pStrategy->getPostProcessChain();
		if (!pProcessChain) return;

		pProcessChain->setEnable(PostProcess::Type::VolumeLight, IsOpen);
	}

	bool VolumeLightComponent::getOpenVolumeLightState()
	{
		if (m_pActComInfo)
		{
			VolumeLightComponentInfo* pInfo = static_cast<VolumeLightComponentInfo*>(m_pActComInfo);

			return pInfo->mOpenThis;
		}
		return false;
	}

	bool VolumeLightComponent::isValidComponent()
	{
		if (!m_tMgr) return false;

		if (Root::instance()->getActiveSceneManager() != m_tMgr->GetSceneManager())
			return false;

		TODContext<VolumeLightComponent, VolumeLightComponentContext>& content = m_tMgr->getVolumeLightContext();
		if (content.mActiveCom != nullptr && content.mActiveCom == this)
			return true;
		return false;
	}

	void VolumeLightComponent::setSunShaftsParams(const VolumeLightComponentContext& context)
	{
		VolumeLight* pVolumeLight = getVolumeLight();

		if (nullptr != pVolumeLight)
		{
			pVolumeLight->setSunShaftsParams(
				context.mRaysAmount,
				context.mRaysAttenuation,
				context.mRaysColorIntensity,
				context.mRaysCustomColor.r,
				context.mRaysCustomColor.g,
				context.mRaysCustomColor.b
				);
		}
	}

	void VolumeLightComponent::setTimelineAttribute(std::vector<float>& data, const Name& name_) 
	{
		if (!m_tMgr)
			return;
		TODContext<VolumeLightComponent, VolumeLightComponentContext>& vlContext = m_tMgr ->getVolumeLightContext();

		VolumeLightComponent* activeCom = vlContext.mActiveCom;

		if (activeCom == nullptr || activeCom != this) return;

		if (name_ == VolumeLightSpace::VL_RAYAMOUNT)
			this->setRaysAmount(data[0]);
		else if (name_ == VolumeLightSpace::VL_RAYSATTENUATION)
			this->setRaysAttenuation(data[0]);
		else if (name_ == VolumeLightSpace::VL_RAYSCOLORINTENSITY)
			this->setRaysColorIntensity(data[0]);
		else if (name_ == VolumeLightSpace::VL_RAYSCUSTOMCOLOR)
			this->setRaysCustomColor(ColorValue(data[0], data[1], data[2], data[3]));
	}

	void VolumeLightComponent::visibleStateChanged(bool isVisible)
	{
		if (!m_tMgr)
			return;
		if (isVisible)
			m_tMgr->pushComponent(m_tMgr->getVolumeLightContext(), this);
		else
			m_tMgr->popComponent(m_tMgr->getVolumeLightContext(), this);
	}

	//----------------------------------------------------------------------------------------

	EDITOR_REFLECT_BEGIN(VolumeLightComponentInfo)
	EDITOR_REFLECT_MEMBER_UPDATE(mContext.mRaysAmount, "RaysAmount", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mContext.mRaysAttenuation, "RaysAttenuation", EditorUI_DRAG, nullptr,reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mContext.mRaysColorIntensity, "RaysColorIntensity", EditorUI_DRAG, nullptr,reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mContext.mRaysCustomColor, "RaysCustomColor", EditorUI_DRAG, nullptr,reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mContext.mPriority, "Priority", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(mOpenThis, "State", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_END();

	VolumeLightComponentInfo::VolumeLightComponentInfo(const Name& comType, ActorCOMMoveableType type)
		: ActorComponentInfo(comType, type)
	{

	}

	void VolumeLightComponentInfo::ExportData(cJSON* jsonNode)
	{
		reflect::TypeResolver<VolumeLightComponentInfo>::get()->ExportJson(this, jsonNode, "");
	}

	void VolumeLightComponentInfo::ImportData(cJSON* jsonNode)
	{
		reflect::TypeResolver<VolumeLightComponentInfo>::get()->ImportJson(this, jsonNode, "");
	}

	void VolumeLightComponentInfo::CopyTo(ActorComponentInfo* comInfo)
	{
		if (comInfo == nullptr)
			return;
		VolumeLightComponentInfo* targetInfo = static_cast<VolumeLightComponentInfo*>(comInfo);
		reflect::TypeResolver<VolumeLightComponentInfo>::get()->copyTo(this, targetInfo);
	}

	void VolumeLightComponentInfo::OverWrite() 
	{
		if (mOriginComInfo != nullptr) 
			reflect::TypeResolver<VolumeLightComponentInfo>::get()->overWrite(mOriginComInfo, this);
	}
}
