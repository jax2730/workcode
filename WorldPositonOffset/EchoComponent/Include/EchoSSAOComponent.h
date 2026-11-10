#ifndef ECHO_SSAO_COMPONENT_H
#define ECHO_SSAO_COMPONENT_H

#include "EchoComponentPrerequisites.h"

#include "EchoComDefineMacro.h"

namespace Echo
{
	struct ASSAOSettings;
	struct SSSRSettings;
	struct SSSSettings;
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

		EDITOR_REFLECT
	};

	class _EchoComponentExport SSAOComponent : public ActorComponent
	{
		DECLAREACTORCOMTYPE_H(SSAOComponent)

	public:
		void init(ActorComponentInfo* info) override;
		void copyTo(const ActorComponentPtr&) override;
		
		static void SetSSAOParameters(const ASSAOSettings& params);
		static std::optional<ASSAOSettings> GetSSAOParameters();

		static void SetSSSRSettings(const SSSRSettings& params);
		static std::optional<SSSRSettings> GetSSSRSettings();

		static void SetContactShadowSamples(uint32 samples);
		static uint32 GetContactShadowSamples();

		static void SetContactShadowSettings(const SSSSettings& params);
		static std::optional<SSSSettings> GetContactShadowSettings();
	protected:
		SSAOComponentInfo* m_Info = nullptr;
	};

	DEFINECOMFACTORY(SSAOComponent, UNMOVEABLE)
}

#endif
