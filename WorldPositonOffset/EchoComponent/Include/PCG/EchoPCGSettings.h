#pragma once

#include "EchoComponentPrerequisites.h"

#include "PCG/EchoPCGCommon.h"
#include "PCG/EchoPCGPin.h"

namespace Echo
{
	struct cJSON;

	namespace PCGV2
	{
		class PCGElement;
		enum class EPCGSettingsType : uint8
		{
			InputOutput,
			Spatial,
			Density,
			Blueprint,
			Metadata,
			Filter,
			Sampler,
			Spawner,
			Subgraph,
			Debug,
			Generic,
			Param,
			HierarchicalGeneration,
			ControlFlow,
			PointOps,
			GraphParameters,
			Reroute,
			GPU,
			DynamicMesh,
		};

		class _EchoComponentExport PCGSettings : public PCGObject
		{
		public:

			PCGSettings();

			PCG_ABSTRACT_OBJECT_BODY(PCGSettings)

			virtual ~PCGSettings();

			virtual std::unique_ptr<PCGNode> CreateNode();

			virtual PCGElement* GetElement() const = 0;

			std::vector<PCGPinProperties> AllInputPinProperties();

			std::vector<PCGPinProperties> AllOutputPinProperties();

			int GetSeed(const PCGComponent* inSourceComponent = nullptr) const;

			Name GetSettingsClassName() const;

			virtual Name GetDefaultNodeTitle() const;

			virtual PCGSettings* Clone() const = 0;

			virtual EPCGSettingsType GetType() const { return EPCGSettingsType::Generic; }
		protected:
			template<typename T>
			T* CloneT() const { return new T(*(T*)this); }

			virtual std::vector<PCGPinProperties> InputPinProperties() const;

			virtual std::vector<PCGPinProperties> OutputPinProperties() const;

			std::vector<PCGPinProperties> DefaultPointInputPinProperties() const;

			std::vector<PCGPinProperties> DefaultPointOutputPinProperties() const;

			template<typename SettingsType>
			static void AppendPublicPCGSettingsProperties(PCGClass& Class)
			{
				Class.AddProperty("bUseSeed", PCGPropertyType::BOOL, STRUCT_OFFSET(SettingsType, m_bUseSeed))->SetCategory("Seeding");
				Class.AddProperty("Seed", PCGPropertyType::INT, STRUCT_OFFSET(SettingsType, m_Seed))->SetCategory("Seeding");
			}

			bool m_bUseSeed = false;

			int32 m_Seed = 0xC35A9631;
		};

		struct PCGSettingsItem
		{
			Name className;
			Name displayName;
			Name category;
		};

		class _EchoComponentExport PCGSettingsRegistry
		{
		public:
			static PCGSettingsRegistry& Get();

			void AddPCGSettings(const Name& className, const Name& displayName, const Name& category);

			std::vector<PCGSettingsItem>& GetAllSettings() { return m_settingsItems; }

		private:
			PCGSettingsRegistry() = default;

			std::vector<PCGSettingsItem> m_settingsItems;
		};

		_EchoComponentExport void RegisterPCGSettings(std::string_view className, std::string_view displayName = "", std::string_view category = "");

		_EchoComponentExport std::unique_ptr<PCGSettings> CreatePCGSettings(const Name& className);

	}
}


#define PCG_SETTINGS_BODY_H(CLASS)\
PCG##CLASS##Settings();\
virtual PCGElement* GetElement() const override;\
virtual PCGSettings* Clone() const override;\
virtual Name GetDefaultNodeTitle() const override;\
virtual std::vector<PCGPinProperties> InputPinProperties() const override;\
virtual std::vector<PCGPinProperties> OutputPinProperties() const override;\
PCG_OBJECT_BODY(PCG##CLASS##Settings)


#define CLASS_TO_STRING(CLASS) #CLASS

#define PCG_SETTINGS_BODY_CPP(CLASS)\
REGISTER_PCG_CLASS(PCG##CLASS##Settings)\
PCG##CLASS##Settings::PCG##CLASS##Settings() : PCGSettings() {}\
PCGElement* PCG##CLASS##Settings::GetElement() const { return PCG##CLASS##Element::GetInstance(); }\
PCGSettings* PCG##CLASS##Settings::Clone() const { return CloneT<PCG##CLASS##Settings>(); }\
Name PCG##CLASS##Settings::GetDefaultNodeTitle() const { return Name(#CLASS); }\


