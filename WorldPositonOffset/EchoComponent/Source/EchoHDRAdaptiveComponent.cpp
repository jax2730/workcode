#include "EchoComponentStableHeaders.h"
#include "EchoHDRAdaptiveComponent.h"
#include "EchoHDR.h"
#include "EchoRenderStrategy.h"
#include "EchoTODComponentSystem.h"

namespace Echo
{
	Name HDRAdaptiveComponent::HDREyeAdaptionX = Name("EyeAdaption_X");
	Name HDRAdaptiveComponent::HDREyeAdaptionY = Name("EyeAdaption_Y");
	Name HDRAdaptiveComponent::HDREyeAdaptionZ = Name("EyeAdaption_Z");
	Name HDRAdaptiveComponent::HDREyeAdaptionW = Name("EyeAdaption_W");

	EDITOR_REFLECT_BEGIN(HDRAdaptiveComponentInfo)
	EDITOR_REFLECT_MEMBER_UPDATE(mHDRAdaptiveHandle.mEyeAdaptionX, HDRAdaptiveComponent::HDREyeAdaptionX.toString(), EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mHDRAdaptiveHandle.mEyeAdaptionY, HDRAdaptiveComponent::HDREyeAdaptionY.toString(), EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mHDRAdaptiveHandle.mEyeAdaptionZ, HDRAdaptiveComponent::HDREyeAdaptionZ.toString(), EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mHDRAdaptiveHandle.mEyeAdaptionW, HDRAdaptiveComponent::HDREyeAdaptionW.toString(), EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mHDRAdaptiveHandle.mPriority, "Priority", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_END()

	HDRAdaptiveComponentInfo::HDRAdaptiveComponentInfo(const Name& comType, ActorCOMMoveableType type)
		:ActorComponentInfo(comType, type)
	{
	}
	void HDRAdaptiveComponentInfo::ExportData(cJSON* jsonNode)
	{
		reflect::TypeResolver<HDRAdaptiveComponentInfo>::get()->ExportJson(this, jsonNode, "");
	}
	void HDRAdaptiveComponentInfo::ImportData(cJSON* jsonNode)
	{
		reflect::TypeResolver<HDRAdaptiveComponentInfo>::get()->ImportJson(this, jsonNode, "");
	}
	void HDRAdaptiveComponentInfo::CopyTo(ActorComponentInfo* comInfo)
	{
		if (comInfo == nullptr)
			return;
		reflect::TypeResolver<HDRAdaptiveComponentInfo>::get()->copyTo(this, comInfo);
	}
	void HDRAdaptiveComponentInfo::OverWrite()
	{
		if (mOriginComInfo != nullptr)
		{
			reflect::TypeResolver<HDRAdaptiveComponentInfo>::get()->overWrite(mOriginComInfo, this);
		}
	}

	// ------------------------------------------------------------------------------------------------------------

	DECLAREACTORCOMTYPE_CPP(HDRAdaptiveComponent)
	HDRAdaptiveComponent::HDRAdaptiveComponent(Echo::ActorPtr pActor, Echo::ActorCOMMoveableType movableType)
		:ActorComponent(pActor, movableType)
	{

	}

	HDRAdaptiveComponent::~HDRAdaptiveComponent()
	{

	}

	void HDRAdaptiveComponent::visibleStateChanged(bool isVisible)
	{
		if (!m_tMgr)
			return;
		if (isVisible)
			m_tMgr->pushComponent(m_tMgr->getHDRAdaptiveContext(), this);
		else
			m_tMgr->popComponent(m_tMgr->getHDRAdaptiveContext(), this);
	}
	void HDRAdaptiveComponent::setTimelineAttribute(std::vector<float>& data, const Name& name)
	{
		if(m_tMgr)
			m_tMgr->setTodComChangeState(T_HDRAdaptive);


		if (name == HDRAdaptiveComponent::HDREyeAdaptionX)
			setEyeAdaptionX(data[0]);
		else if (name == HDRAdaptiveComponent::HDREyeAdaptionY)
			setEyeAdaptionY(data[0]);
		else if (name == HDRAdaptiveComponent::HDREyeAdaptionZ)
			setEyeAdaptionZ(data[0]);
		else if (name == HDRAdaptiveComponent::HDREyeAdaptionW)
			setEyeAdaptionW(data[0]);
	}
	void HDRAdaptiveComponent::init(ActorComponentInfo* info)
	{
		HDR* pHDR = getHDR();
		if (pHDR == nullptr) return;

		mDefaultAdaptive = std::make_shared<HDRAdaptiveHandle>();
		Echo::Vector4 param = pHDR->getEyeAdaptationParams();
		mDefaultAdaptive->mEyeAdaptionX = param.x;
		mDefaultAdaptive->mEyeAdaptionY = param.y;
		mDefaultAdaptive->mEyeAdaptionZ = param.z;
		mDefaultAdaptive->mEyeAdaptionW = param.w;

		TODCOMPONENT_INITMGR(getHDRAdaptiveContext, m_tMgr, false);

		HDRAdaptiveComponentInfo* pInfo = static_cast<HDRAdaptiveComponentInfo*>(info);
		if (pInfo == nullptr) return;
		mContext = pInfo->mHDRAdaptiveHandle;
	}
	void HDRAdaptiveComponent::unInit()
	{
		TODCOMPONENT_UNINITMGR(getHDRAdaptiveContext, m_tMgr, T_End);
	}
	void HDRAdaptiveComponent::use()
	{
		if (!m_tMgr || !isOpenHDR()) return;
		if (!m_tMgr->getHDRAdaptiveContext().mIsUseCurrType) return;
		HDR* pHDR = this->getHDR();

		bool isActive = Root::instance()->getActiveSceneManager() == m_tMgr->GetSceneManager();
	

		if (nullptr != pHDR && isActive)
		{
			pHDR->setEyeAdaptationParam0x(mContext.mEyeAdaptionX);
			pHDR->setEyeAdaptationParam0y(mContext.mEyeAdaptionY);
			pHDR->setEyeAdaptationParam0z(mContext.mEyeAdaptionZ);
			pHDR->setEyeAdaptationParam0w(mContext.mEyeAdaptionW);
		}
	}
	void HDRAdaptiveComponent::unuse()
	{
		
	}

	void HDRAdaptiveComponent::setEyeAdaptionX(float value)
	{
		mContext.mEyeAdaptionX = value;
		if (!isCurrentActiveCom())
			return;
		HDR* pHDR = this->getHDR();
		if (nullptr != pHDR)
			pHDR->setEyeAdaptationParam0x(value);
	}

	void HDRAdaptiveComponent::setEyeAdaptionY(float value)
	{
		mContext.mEyeAdaptionY = value;
		if (!isCurrentActiveCom())
			return;
		HDR* pHDR = this->getHDR();
		if (nullptr != pHDR)
			pHDR->setEyeAdaptationParam0y(value);
	}

	void HDRAdaptiveComponent::setEyeAdaptionZ(float value)
	{
		mContext.mEyeAdaptionZ = value;
		if (!isCurrentActiveCom())
			return;
		HDR* pHDR = this->getHDR();
		if (nullptr != pHDR)
			pHDR->setEyeAdaptationParam0z(value);
	}

	void HDRAdaptiveComponent::setEyeAdaptionW(float value)
	{
		mContext.mEyeAdaptionW = value;
		if (!isCurrentActiveCom())
			return;
		HDR* pHDR = this->getHDR();
		if (nullptr != pHDR)
			pHDR->setEyeAdaptationParam0w(value);
	}

	int HDRAdaptiveComponent::getPriority() const
	{
		return mContext.mPriority;
	}
	void HDRAdaptiveComponent::setPriority(const int& val)
	{
		mContext.mPriority = std::max(0, std::min(val, 255));
		if (m_tMgr)
			m_tMgr->adjustComponentPriority(m_tMgr->getHDRAdaptiveContext(), this);
	}
	bool HDRAdaptiveComponent::isCurrentActiveCom()
	{
		if (!m_tMgr) return false;

		if (Root::instance()->getActiveSceneManager() != m_tMgr->GetSceneManager())
			return false;

		const auto& Context = m_tMgr->getHDRAdaptiveContext();
		return Context.mActiveCom != nullptr && Context.mActiveCom == this;
	}
	void HDRAdaptiveComponent::setIsUseCurrent()
	{
		if (!isCurrentActiveCom())
			return;

		HDR* pHDR = this->getHDR();

		if (nullptr != pHDR)
		{
			pHDR->setEyeAdaptationParam0x(mContext.mEyeAdaptionX);
			pHDR->setEyeAdaptationParam0y(mContext.mEyeAdaptionY);
			pHDR->setEyeAdaptationParam0z(mContext.mEyeAdaptionZ);
			pHDR->setEyeAdaptationParam0w(mContext.mEyeAdaptionW);
		}
	}

	bool HDRAdaptiveComponent::isOpenHDR()
	{
		RenderSystem* renderSys = Root::instance()->getRenderSystem();
		RenderStrategy* renderStrategy = nullptr;
		PostProcessChain* processChain = nullptr;
		if (renderSys)
			renderStrategy = renderSys->getCurRenderStrategy();
		if (renderStrategy)
			processChain = renderStrategy->getPostProcessChain();
		if (processChain)
			return processChain->isEnable(PostProcess::Type::HDR);
		return false;
	}

	HDR* HDRAdaptiveComponent::getHDR()
	{
		RenderSystem* renderSys = Root::instance()->getRenderSystem();
		RenderStrategy* renderStrategy = nullptr;
		PostProcessChain* processChain = nullptr;
		HDR* pHDR = nullptr;
		if (renderSys)
			renderStrategy = renderSys->getCurRenderStrategy();
		if (renderStrategy)
			processChain = renderStrategy->getPostProcessChain();
		if(processChain)
			pHDR = static_cast<HDR*>(processChain->getPostProcess(PostProcess::Type::HDR));
		return pHDR;
	}
}
