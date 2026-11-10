#include "EchoComponentStableHeaders.h"
#include "EchoSSAOComponent.h"

#include <EchoASSAO.h>
#include <EchoDeferredRenderStrategy.h>
#include <EchoRenderStrategy.h>

#include "EchoPostProcess.h"

namespace Echo
{
	DECLAREACTORCOMTYPE_CPP(SSAOComponent)

	EDITOR_REFLECT_BEGIN(SSAOComponentInfo)
		EDITOR_REFLECT_MEMBER_UPDATE(m_PassCount, "深度纹理数", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_TapCount, "采样对数量", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_BlurPass, "模糊", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_EffectRadius, "遮蔽范围", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_ShadowStrength, "阴影强度", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_ShadowPower, "阴影指数", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_ShadowClamp, "阴影上限", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_HorizonAngleThreshold, "角度阈值", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_FadeoutFrom, "衰减起点", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_FadeoutTo, "衰减终点", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)

		EDITOR_REFLECT_MEMBER_UPDATE(m_temporal_stability_factor, "Temporal Stability", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_depth_buffer_thickness, "Depth Buffer Thickness", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_roughness_threshold, "Roughness Threshold", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_temporal_variance_threshold, "Temporal Variance Threshold", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_max_traversal_intersections, "Max Traversal Iterations", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_min_traversal_occupancy, "Min Traversal Occupancy", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_most_detailed_mip, "Most Detailed Level", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_samples_per_quad, "Samples Per Quad", EditorUI_NORAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(m_temporal_variance_guided_tracing_enabled, "Enable Variance Guided Tracing", EditorUI_NORAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_END()

	SSAOComponentInfo::SSAOComponentInfo(const Name& comType, const ActorCOMMoveableType type) :
		ActorComponentInfo(comType, type) {}

	void SSAOComponentInfo::ExportData(cJSON* jsonNode)
	{
		reflect::TypeResolver<SSAOComponentInfo>::get()->ExportJson(this, jsonNode, "");
	}

	void SSAOComponentInfo::ImportData(cJSON* jsonNode)
	{
		reflect::TypeResolver<SSAOComponentInfo>::get()->ImportJson(this, jsonNode, "");
	}

	void SSAOComponentInfo::CopyTo(ActorComponentInfo* comInfo)
	{
		if (comInfo == nullptr)return;

		const auto info = dynamic_cast<SSAOComponentInfo*>(comInfo);
		reflect::TypeResolver<SSAOComponentInfo>::get()->copyTo(this, info);
	}

	SSAOComponent::SSAOComponent(ActorPtr pActor, const ActorCOMMoveableType movableType) :
		ActorComponent(std::move(pActor), movableType) { }

	SSAOComponent::~SSAOComponent()
	{
		SetActive(false);
	}

	void SSAOComponentInfo::Overlay(ActorComponentInfo* srcInfo) {}

	void SSAOComponentInfo::OverWrite()
	{
		if (mOriginComInfo == nullptr) return;

		reflect::TypeResolver<SSAOComponentInfo>::get()->overWrite(mOriginComInfo, this);
	}

	void SSAOComponent::init(ActorComponentInfo* info)
	{
		if (info == nullptr) return;
		m_Info = dynamic_cast<SSAOComponentInfo*>(info);
		if (!m_Info) return;
		SetActive(m_Info->m_IsActive);
		SetParameters(&m_Info->m_PassCount);
	}

	void SSAOComponent::copyTo(const ActorComponentPtr& actorComponentPtr)
	{
	}

	void SSAOComponent::SetParameters(const float* params)
	{
#if ECHO_EDITOR
		const auto system = Root::instance()->getRenderSystem();
		if (!system) return;

		const auto strategy = system->getCurRenderStrategy();
		if (!strategy) return;

		if (system->getCurrentRenderStrategyType() == RS_Deferred)
		{
			auto deferred = dynamic_cast<DeferredRenderStrategy*>(strategy);
			if (!deferred) return;
			deferred->setAOParameters(*reinterpret_cast<const ASSAO::ASSAOSettings*>(params));
			return;
		}

		const auto effects = strategy->getPostProcessChain();
		if (!effects) return;

		const auto ao = dynamic_cast<ASSAO*>(effects->getPostProcess(PostProcess::SSAO));
		if (!ao) return;
		ao->setParameters(reinterpret_cast<const ASSAO::ASSAOSettings*>(params));
		effects->setEnable(PostProcess::SSAO, effects->isEnable(PostProcess::SSAO) && params[0] >= 1);
#endif
	}

	void SSAOComponent::SetActive(const bool active)
	{
#if ECHO_EDITOR
		const auto system = Root::instance()->getRenderSystem();
		if (!system) return;

		const auto strategy = system->getCurRenderStrategy();
		if (!strategy) return;

		const auto effects = strategy->getPostProcessChain();
		if (!effects) return;

		effects->setEnable(PostProcess::SSAO, active);
#endif
	}

	void SSAOComponent::SetSSSRSettings(const void* params)
	{
#if ECHO_EDITOR
		const auto system = Root::instance()->getRenderSystem();
		if (!system) return;

		const auto strategy = dynamic_cast<DeferredRenderStrategy*>(system->getCurRenderStrategy());
		if (!strategy || system->getCurrentRenderStrategyType() != RS_Deferred) return;

		strategy->setSSSRParameters(params);

#endif
	}
}
