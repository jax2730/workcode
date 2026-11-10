#include "EchoComponentStableHeaders.h"
#include "EchoTODShadowComponent.h"
#include "EchoTODComponentManager.h"
#include "EchoTODComponentSystem.h"
#include "EchoTODDataUtil.h"
#include "EchoASSAO.h"
#include "EchoDeferredRenderStrategy.h"

namespace Echo {

	DECLAREACTORCOMTYPE_CPP(TODShadowComponent)


		EDITOR_REFLECT_BEGIN(TODShadowComponentInfo)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mShadowWeight, "ShadowWeight", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mPriority, "ShadowPriority", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)

		EDITOR_REFLECT_MEMBER_UPDATE(mContext.m_AssaoShadowStrength, "AssaoShadowStrength", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.m_AssaoShadowPower, "AssaoShadowPower", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_END()


		TODShadowComponentInfo::TODShadowComponentInfo(const Name& comType, ActorCOMMoveableType type)
		:ActorComponentInfo(comType, type)
	{
	}

	TODShadowComponentInfo::~TODShadowComponentInfo()
	{
	}

	void TODShadowComponentInfo::ExportData(cJSON* jsonNode)
	{
		reflect::TypeResolver<TODShadowComponentInfo>::get()->ExportJson(this, jsonNode, "");
	}

	void TODShadowComponentInfo::ImportData(cJSON* jsonNode)
	{
		reflect::TypeResolver<TODShadowComponentInfo>::get()->ImportJson(this, jsonNode, "");
	}

	void TODShadowComponentInfo::CopyTo(ActorComponentInfo* comInfo)
	{
		if (comInfo == nullptr)
			return;
		TODShadowComponentInfo* todShadowInfo = static_cast<TODShadowComponentInfo*>(comInfo);
		reflect::TypeResolver<TODShadowComponentInfo>::get()->copyTo(this, todShadowInfo);
	}

	void TODShadowComponentInfo::Overlay(ActorComponentInfo* srcInfo)
	{
	}

	void TODShadowComponentInfo::OverWrite()
	{
		if (mOriginComInfo != nullptr)
		{
			reflect::TypeResolver<TODShadowComponentInfo>::get()->overWrite(mOriginComInfo, this);
		}
	}

	TODShadowComponent::TODShadowComponent(Echo::ActorPtr pActor, Echo::ActorCOMMoveableType movableType)
		:ActorComponent(pActor, movableType)
	{
#ifdef ECHO_EDITOR
		TODDataUtil::SetTodTemplate(TodDataID::PARAM_SHADOW_WEIGHT, ShadowWeightName_.toString(), 1);
#endif
	}
	TODShadowComponent::~TODShadowComponent()
	{

	}


	void TODShadowComponent::init(ActorComponentInfo* info)
	{
		if (info)
		{
			TODShadowComponentInfo* p_TODShadowComInfo_ = static_cast<TODShadowComponentInfo*>(info);
			mContext = p_TODShadowComInfo_->mContext;
		}
		TODCOMPONENT_INITMGR(getTODShadowContext, m_tMgr, false);

	}

	void Echo::TODShadowComponent::unInit()
	{
		TODCOMPONENT_UNINITMGR(getTODShadowContext, m_tMgr, T_End);
	}

	void TODShadowComponent::copyTo(const ActorComponentPtr&)
	{
	}

	void TODShadowComponent::setTimelineAttribute(std::vector<float>& data, const Name& name)
	{
		if (!m_tMgr)
			return;
		if (!mIsActiveCom)
			return;
		m_tMgr->setTodComChangeState(T_Shadow);
		if (name == ShadowWeightName_)
			setShadowWeight(data[0]);
	}

	void TODShadowComponent::visibleStateChanged(bool isVisible)
	{
		if (!m_tMgr)
			return;
		if (isVisible)
			m_tMgr->pushComponent(m_tMgr->getTODShadowContext(), this);
		else
			m_tMgr->popComponent(m_tMgr->getTODShadowContext(), this);
	}

	void TODShadowComponent::use()
	{
		if (!m_tMgr)
			return;
		if (m_tMgr->getIsWriteLog())
		{
			std::string logInfo = "";
			logInfo.append("TodShadow : use").append(getCombinedName());
			LogManager::getSingleton().logMessage(Echo::LML_CRITICAL, logInfo.data());
		}
		mIsActiveCom = true;
		setShadowWeight(mContext.mShadowWeight);

		setAssaoShadowPower(mContext.m_AssaoShadowPower);
		setAssaoShadowStrength(mContext.m_AssaoShadowStrength);

	}

	void TODShadowComponent::unuse()
	{
		mIsActiveCom = false;
	}

	void TODShadowComponent::setShadowWeight(float shadowWeight_)
	{
		mContext.mShadowWeight = shadowWeight_;

		if (mIsActiveCom && m_tMgr)
		{
			SceneManager* p_sMgr = m_tMgr->GetSceneManager();
			if (p_sMgr)
				p_sMgr->setShadowWeight(shadowWeight_);
		}
	}

	void TODShadowComponent::setAssaoShadowStrength(float strength_)
	{
		strength_ = std::max(std::min(strength_, 10.f), 0.f);
		mContext.m_AssaoShadowStrength = strength_;
		if (!m_tMgr || !mIsActiveCom)
			return;
		if (m_tMgr->GetSceneManager() != Root::instance()->getActiveSceneManager())
			return;

		RenderSystem* system = Root::instance()->getRenderSystem();
		if (!system) return;

		RenderStrategy* strategy = system->getCurRenderStrategy();
		if (!strategy) return;

		if (system->getCurrentRenderStrategyType() == RS_Deferred)
		{
			auto deferred = dynamic_cast<DeferredRenderStrategy*>(strategy);
			if (!deferred) return;
			deferred->setAOShadowStrength(strength_);
			return;
		}

		PostProcessChain* effects = strategy->getPostProcessChain();
		if (!effects) return;

		ASSAO* ao = dynamic_cast<ASSAO*>(effects->getPostProcess(PostProcess::SSAO));
		if (!ao)
			return;
		ao->setShadowStrength(mContext.m_AssaoShadowStrength);
	}

	void TODShadowComponent::setAssaoShadowPower(float power_)
	{
		power_ = std::max(std::min(power_, 5.f), 0.f);
		mContext.m_AssaoShadowPower = power_;
		
		if (!m_tMgr || !mIsActiveCom)
			return;
		if (m_tMgr->GetSceneManager() != Root::instance()->getActiveSceneManager())
			return;

		RenderSystem* system = Root::instance()->getRenderSystem();
		if (!system) return;

		RenderStrategy* strategy = system->getCurRenderStrategy();
		if (!strategy) return;

		if (system->getCurrentRenderStrategyType() == RS_Deferred)
		{
			auto deferred = dynamic_cast<DeferredRenderStrategy*>(strategy);
			if (!deferred) return;
			deferred->setAOShadowPower(power_);
			return;
		}

		PostProcessChain* effects = strategy->getPostProcessChain();
		if (!effects) return;

		ASSAO* ao = dynamic_cast<ASSAO*>(effects->getPostProcess(PostProcess::SSAO));
		if (!ao)
			return;
		ao->setShadowPower(mContext.m_AssaoShadowPower);
	}

	void TODShadowComponent::setPriority(int priority_)
	{
		mContext.mPriority = priority_;
		if (m_tMgr)
			m_tMgr->adjustComponentPriority(m_tMgr->getTODShadowContext(), this);
	}
}

