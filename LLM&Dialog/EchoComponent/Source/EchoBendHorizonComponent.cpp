#include "EchoComponentStableHeaders.h"
#include "EchoBendHorizonComponent.h"
#include "EchoPrerequisites.h"
#include "EchoBendHorizon.h"
#include "EchoRenderStrategy.h"
#include "EchoRenderTarget.h"

namespace Echo
{
	DECLAREACTORCOMTYPE_CPP(BendHorizonComponent)
		EDITOR_REFLECT_BEGIN(BendHorizonComponentInfo)
		EDITOR_REFLECT_MEMBER_UPDATE(mBendCurveFactorA, "Bend Curve Factor A", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mBendCurveFactorC, "Bend Curve Factor C", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mStartSmoothBendHorizonHeigh, "Start Smooth Bend Horizon Heigh", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mEndSmoothBendHorizonHeigh, "End Smooth Bend Horizon Heigh", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(misOpenBendHorizon, "Open Bend Horizon", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_END()

	BendHorizonComponentInfo::BendHorizonComponentInfo() {}

	BendHorizonComponentInfo::BendHorizonComponentInfo(const Name& comType, ActorCOMMoveableType type)
		: ActorComponentInfo(comType, type)
	{
	}

	BendHorizonComponentInfo::~BendHorizonComponentInfo()
	{
	}

	void BendHorizonComponentInfo::ExportData(cJSON* jsonNode)
	{
		reflect::TypeResolver<BendHorizonComponentInfo>::get()->ExportJson(this, jsonNode, "");
	}

	void BendHorizonComponentInfo::ImportData(cJSON* jsonNode)
	{
		reflect::TypeResolver<BendHorizonComponentInfo>::get()->ImportJson(this, jsonNode, "");
	}

	void BendHorizonComponentInfo::CopyTo(ActorComponentInfo* comInfo)
	{
		if (comInfo == nullptr)
			return;
		BendHorizonComponentInfo* info = static_cast<BendHorizonComponentInfo*>(comInfo);
		reflect::TypeResolver<BendHorizonComponentInfo>::get()->copyTo(this, info);
	}

	void BendHorizonComponentInfo::Overlay(ActorComponentInfo* srcInfo)
	{
	}

	BendHorizonComponent::BendHorizonComponent(ActorPtr  pActor, ActorCOMMoveableType movableType)
		:ActorComponent(pActor, movableType)
	{
	}

	BendHorizonComponent::~BendHorizonComponent()
	{

	}

	void BendHorizonComponentInfo::OverWrite()
	{
		if (mOriginComInfo != nullptr)
		{
			reflect::TypeResolver<BendHorizonComponentInfo>::get()->overWrite(mOriginComInfo, this);
		}
	}

	void BendHorizonComponent::init(ActorComponentInfo * info)
	{
		BendHorizonComponentInfo* mBendHorizonInfo = static_cast<BendHorizonComponentInfo*>(info);
		if (mBendHorizonInfo == nullptr)
			return;
		switchBendHorizon(mBendHorizonInfo->misOpenBendHorizon);
		setStartSmoothBendHeigh(mBendHorizonInfo->mStartSmoothBendHorizonHeigh);
		setEndSmoothBendHeigh(mBendHorizonInfo->mEndSmoothBendHorizonHeigh);
		setBendCurveFactorA(mBendHorizonInfo->mBendCurveFactorA);
		setBendCurveFactorC(mBendHorizonInfo->mBendCurveFactorC);
	}

	void BendHorizonComponent::copyTo(const ActorComponentPtr& pcomp)
	{
	}

	void BendHorizonComponent::setStartSmoothBendHeigh(float& value)
	{
		auto renderSystem = Root::instance()->getRenderSystem();
		if (renderSystem)
		{
			RenderStrategy* strategy = renderSystem->getCurRenderStrategy();
			if (strategy)
			{
				PostProcessChain* chain = strategy->getPostProcessChain();
				BendHorizon* bendHorizonProcess = (BendHorizon*)(chain->getPostProcess(PostProcess::BendHorizon));
				if (bendHorizonProcess)
				{
					bendHorizonProcess->m_startSmoothBendHeight = value;
				}
			}
		}
	}
	void BendHorizonComponent::setEndSmoothBendHeigh(float& value)
	{
		auto renderSystem = Root::instance()->getRenderSystem();
		if (renderSystem)
		{
			RenderStrategy* strategy = renderSystem->getCurRenderStrategy();
			if (strategy)
			{
				PostProcessChain* chain = strategy->getPostProcessChain();
				BendHorizon* bendHorizonProcess = (BendHorizon*)(chain->getPostProcess(PostProcess::BendHorizon));
				if (bendHorizonProcess)
				{
					bendHorizonProcess->m_endSmoothBendHeight = value;
				}
			}
		}
	}

	void BendHorizonComponent::setBendCurveFactorA(float& value)
	{
		auto renderSystem = Root::instance()->getRenderSystem();
		if (renderSystem)
		{
			RenderStrategy* strategy = renderSystem->getCurRenderStrategy();
			if (strategy)
			{
				PostProcessChain* chain = strategy->getPostProcessChain();
				BendHorizon* bendHorizonProcess = (BendHorizon*)(chain->getPostProcess(PostProcess::BendHorizon));
				if (bendHorizonProcess)
				{
					bendHorizonProcess->m_curve_a = value;
				}
			}
		}
	}

	void BendHorizonComponent::setBendCurveFactorC(float& value)
	{
		auto renderSystem = Root::instance()->getRenderSystem();
		if (renderSystem)
		{
			RenderStrategy* strategy = renderSystem->getCurRenderStrategy();
			if (strategy)
			{
				PostProcessChain* chain = strategy->getPostProcessChain();
				BendHorizon* bendHorizonProcess = (BendHorizon*)(chain->getPostProcess(PostProcess::BendHorizon));
				if (bendHorizonProcess)
				{
					bendHorizonProcess->m_curve_c = value;
				}
			}
		}
	}

	void BendHorizonComponent::switchBendHorizon(bool& isOpen)
	{
		RenderSystem* pRenderSys = Root::instance()->getRenderSystem();
		if (pRenderSys)
		{
			for (uint32 i = 0; i < RENDER_STRATEGY_CNT; i++)
			{
				RenderStrategy* pRenderStrategy = pRenderSys->getRenderStrategy(i);
				if (pRenderStrategy)
				{
					PostProcessChain* pPostProcessChain = pRenderStrategy->getPostProcessChain();
					if (pPostProcessChain)
					{
						pPostProcessChain->setEnable(PostProcess::Type::BendHorizon, isOpen);
					}
				}
			}
		}
	}

}
