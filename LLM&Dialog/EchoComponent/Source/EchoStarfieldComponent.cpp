#include "EchoComponentStableHeaders.h"
#include "EchoStarfieldComponent.h"
#include "EchoPrerequisites.h"
#include "EchoStarfield.h"
#include "EchoRenderStrategy.h"
#include "EchoRenderTarget.h"

namespace Echo {
	DECLAREACTORCOMTYPE_CPP(StarfieldComponent)

		EDITOR_REFLECT_BEGIN(StarfieldComponentInfo)
		EDITOR_REFLECT_MEMBER_UPDATE(mCenter, "Centrol Point", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mSpeed, "Speed", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mEffectLength, "Effect Length", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mEffectWidth, "Effect Width", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mLoopCount,"Loop Count",EditorUI_DRAG,nullptr,reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_END()

	StarfieldComponentInfo::StarfieldComponentInfo():
	mCenter(0.5f,0.5f),
	mSpeed(0.2f),
	mEffectLength(1.0f),
	mEffectWidth(1.0f),
	mLoopCount(1.0f),
	mIsOpenStarfield(false)
	{
	
	}

	StarfieldComponentInfo::StarfieldComponentInfo(const Name& comType, ActorCOMMoveableType type)
		:ActorComponentInfo(comType, type),
		mCenter(0.5f, 0.5f),
		mSpeed(0.2f),
		mEffectLength(1.0f),
		mEffectWidth(1.0f),
		mLoopCount(1.0f),
		mIsOpenStarfield(false)
	{
	}

	StarfieldComponentInfo::~StarfieldComponentInfo()
	{

	}

	void StarfieldComponentInfo::ExportData(cJSON* jsonNode)
	{
		reflect::TypeResolver<StarfieldComponentInfo>::get()->ExportJson(this, jsonNode, "");
	}

	void StarfieldComponentInfo::ImportData(cJSON* jsonNode)
	{
		reflect::TypeResolver<StarfieldComponentInfo>::get()->ImportJson(this, jsonNode, "");
	}

	void StarfieldComponentInfo::CopyTo(ActorComponentInfo* comInfo)
	{
		if (comInfo == nullptr)return;

		StarfieldComponentInfo* info = static_cast<StarfieldComponentInfo*>(comInfo);
		reflect::TypeResolver<StarfieldComponentInfo>::get()->copyTo(this, info);
	}

	void StarfieldComponentInfo::Overlay(ActorComponentInfo* srcInfo)
	{

	}

	void StarfieldComponentInfo::OverWrite()
	{
		if (mOriginComInfo != nullptr)
		{
			reflect::TypeResolver<StarfieldComponentInfo>::get()->overWrite(mOriginComInfo, this);
		}
	}

	StarfieldComponent::StarfieldComponent(ActorPtr pActor, ActorCOMMoveableType movableType)
		:ActorComponent(pActor, movableType)
	{

	}

	//组件析构的时候关闭效果
	StarfieldComponent::~StarfieldComponent()
	{
		switchStarfield(false);
	}

	void StarfieldComponent::init(ActorComponentInfo* info)
	{
		if (info == nullptr)
			return;
		auto mStarfieldComponentInfo = static_cast<StarfieldComponentInfo*>(info);
		if (mStarfieldComponentInfo == nullptr)
			return;
		setCenter(mStarfieldComponentInfo->mCenter);
		setSpeed(mStarfieldComponentInfo->mSpeed);
		setEffectLength(mStarfieldComponentInfo->mEffectLength);
		setEffectWidth(mStarfieldComponentInfo->mEffectWidth);
		switchStarfield(mStarfieldComponentInfo->mIsOpenStarfield);
	}

	void StarfieldComponent::copyTo(const ActorComponentPtr& pcomp)
	{
	}

	void StarfieldComponent::setCenter(const Vector2& value)
	{
		float x = std::clamp(value.x,0.0f, 1.0f);
		float y = std::clamp(value.y, 0.0f, 1.0f);
		
		auto renderSystem = Root::instance()->getRenderSystem();
		if (renderSystem)
		{
			RenderStrategy* strategy = renderSystem->getCurRenderStrategy();
			if (strategy)
			{
				PostProcessChain* chain = strategy->getPostProcessChain();
				Starfield* starfieldProcess = static_cast<Starfield*>(chain->getPostProcess(PostProcess::StarField));
				if (starfieldProcess)
				{
					starfieldProcess->mCenter = Vector2(x,y);
				}
			}
		}
	}

	void StarfieldComponent::setSpeed(float value)
	{
		value = std::clamp(value, -2.0f, 2.0f);
		auto renderSystem = Root::instance()->getRenderSystem();
		if (renderSystem)
		{
			RenderStrategy* strategy = renderSystem->getCurRenderStrategy();
			if (strategy)
			{
				PostProcessChain* chain = strategy->getPostProcessChain();
				Starfield* starfieldProcess = static_cast<Starfield*>(chain->getPostProcess(PostProcess::StarField));
				if (starfieldProcess)
				{
					starfieldProcess->mSpeed = value;
				}
			}
		}
	}

	void StarfieldComponent::setEffectLength(float value)
	{
		value = std::clamp(value,0.f, 10.f);
		auto renderSystem = Root::instance()->getRenderSystem();
		if (renderSystem)
		{
			RenderStrategy* strategy = renderSystem->getCurRenderStrategy();
			if (strategy)
			{
				PostProcessChain* chain = strategy->getPostProcessChain();
				Starfield* starfieldProcess = static_cast<Starfield*>(chain->getPostProcess(PostProcess::StarField));
				if (starfieldProcess)
				{
					starfieldProcess->mEffectLength = value;
				}
			}
		}
	}

	void StarfieldComponent::setEffectWidth(float value)
	{
		value = std::clamp(value, 0.f, 10.f);
		auto renderSystem = Root::instance()->getRenderSystem();
		if (renderSystem)
		{
			RenderStrategy* strategy = renderSystem->getCurRenderStrategy();
			if (strategy)
			{
				PostProcessChain* chain = strategy->getPostProcessChain();
				Starfield* starfieldProcess = static_cast<Starfield*>(chain->getPostProcess(PostProcess::StarField));
				if (starfieldProcess)
				{
					starfieldProcess->mEffectWidth = value;
				}
			}
		}
	}

	void StarfieldComponent::setLoopCount(float value)
	{
		value = std::clamp(value, 0.0f, 7.0f);
		auto renderSystem = Root::instance()->getRenderSystem();
		if (renderSystem)
		{
			RenderStrategy* strategy = renderSystem->getCurRenderStrategy();
			if (strategy)
			{
				PostProcessChain* chain = strategy->getPostProcessChain();
				Starfield* starfieldProcess = static_cast<Starfield*>(chain->getPostProcess(PostProcess::StarField));
				if (starfieldProcess)
				{
					starfieldProcess->mLoopCount = value;
				}
			}
		}
	}

	void StarfieldComponent::switchStarfield(bool isOpen)
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
						Starfield* starfieldProcess = static_cast<Starfield*>(pPostProcessChain->getPostProcess(PostProcess::StarField));
						if (isOpen)
						{
							starfieldProcess->mStartTime = std::chrono::system_clock::now();
							starfieldProcess->mZPos = 0.f;
						}
						pPostProcessChain->setEnable(PostProcess::Type::StarField, isOpen);
						
					}
				}
			}
		}
	}

}

