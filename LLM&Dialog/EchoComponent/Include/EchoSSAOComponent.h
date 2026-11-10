#ifndef ECHO_SSAO_COMPONENT_H
#define ECHO_SSAO_COMPONENT_H

#include "EchoComponentPrerequisites.h"
#include "EchoProtocolComponent.h"
#include "Actor/EchoActor.h"
#include "EchoMovableObject.h"

#include "EchoHelperObj.h"
#include "EchoComDefineMacro.h"

namespace Echo
{
	class _EchoComponentExport SSAOComponentInfo : public ActorComponentInfo
	{
	public:
		SSAOComponentInfo()                                    = default;
		SSAOComponentInfo(const SSAOComponentInfo&)            = delete;
		SSAOComponentInfo& operator=(const SSAOComponentInfo&) = delete;
		SSAOComponentInfo(const Name& comType, ActorCOMMoveableType type);
		~SSAOComponentInfo() override = default;
		void ExportData(cJSON* jsonNode) override;
		void ImportData(cJSON* jsonNode) override;
		void CopyTo(ActorComponentInfo* comInfo) override;
		void Overlay(ActorComponentInfo* srcInfo) override;
		void OverWrite() override;

		// TODO: remove pass count & tap count & threshold(maybe) after testing performance
		float m_PassCount             = 4;
		float m_TapCount              = 12;
		float m_BlurPass              = 1;
		float m_EffectRadius          = 1.0f;
		float m_ShadowStrength        = 1.0f;
		float m_ShadowPower           = 1.5f;
		float m_ShadowClamp           = 0.98f;
		float m_HorizonAngleThreshold = 0.12f;
		float m_FadeoutFrom           = 50.0f;
		float m_FadeoutTo             = 300.0f;
		bool m_IsActive               = false;

		float* m_Params = &m_PassCount; // dummy

		// SSSR
		float	m_temporal_stability_factor                 = 0.7f;
		float	m_depth_buffer_thickness                    = 0.015f;
		float	m_roughness_threshold                       = 0.1f;
		float	m_temporal_variance_threshold               = 0.0f;
		int	m_max_traversal_intersections              = 128;
		int	m_min_traversal_occupancy                  = 4;
		int	m_most_detailed_mip                        = 0;
		int	m_samples_per_quad                         = 4;
		int	m_temporal_variance_guided_tracing_enabled = 1;

		EDITOR_REFLECT
	};

	class _EchoComponentExport SSAOComponent : public ActorComponent
	{
		DECLAREACTORCOMTYPE_H(SSAOComponent)

	public:
		void init(ActorComponentInfo* info) override;
		void copyTo(const ActorComponentPtr&) override;
		static void SetParameters(const float* params);
		static void SetActive(bool active);

		static void SetSSSRSettings(const void* params);

	protected:
		SSAOComponentInfo* m_Info = nullptr;
	};

	DEFINECOMFACTORY(SSAOComponent, UNMOVEABLE)
}

#endif
