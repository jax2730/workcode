#include "EchoComponentStableHeaders.h"
#include "PCG/EchoPCGSettings.h"

#include "PCG/EchoPCGElement.h"
#include "PCG/EchoPCGHelper.h"
#include "PCG/EchoPCGGraph.h"

namespace Echo
{
	namespace PCGV2
	{
		PCGSettings::PCGSettings()
			: PCGObject()
		{
			m_Seed = static_cast<int32>(std::hash<std::string_view>()(GetSettingsClassName().c_str()));
		}

		std::unique_ptr<PCGNode> PCGSettings::CreateNode()
		{
			return std::make_unique<PCGNode>(nullptr);
		}

		PCGSettings::~PCGSettings()
		{

		}

		std::vector<PCGPinProperties> PCGSettings::AllInputPinProperties()
		{
			return InputPinProperties();
		}

		std::vector<PCGPinProperties> PCGSettings::AllOutputPinProperties()
		{
			return OutputPinProperties();
		}

		int PCGSettings::GetSeed(const PCGComponent* inSourceComponent) const
		{
			if (!m_bUseSeed)
			{
				return 42;
			}
			// TODO: use inSourceComponent seed
			// return inSourceComponent ? PCGHelpers::ComputeSeed(m_Seed, static_cast<int>(inSourceComponent->GetSeed())) : m_Seed;
			return m_Seed;
		}

		Name PCGSettings::GetSettingsClassName() const
		{
			return GetClass()->GetName();
		}

		Name PCGSettings::GetDefaultNodeTitle() const
		{
			return GetSettingsClassName();
		}

		PCGClass* PCGSettings::StaticClass()
		{
			BEGIN_PCG_CLASS(PCGSettings)

			PCGSettings::AppendPublicPCGSettingsProperties<PCGSettings>(Class);

			END_PCG_CLASS()
		}

		std::vector<PCGPinProperties> PCGSettings::InputPinProperties() const
		{
			std::vector<PCGPinProperties> properties;
			return properties;
		}

		std::vector<PCGPinProperties> PCGSettings::OutputPinProperties() const
		{
			std::vector<PCGPinProperties> properties;
			return properties;
		}

		std::vector<PCGPinProperties> PCGSettings::DefaultPointInputPinProperties() const
		{
			std::vector<PCGPinProperties> properties;
			properties.emplace_back(PCGPinConstants::DefaultInputLabel, PCGDataType::Point).SetRequiredPin();
			return properties;
		}
		std::vector<PCGPinProperties> PCGSettings::DefaultPointOutputPinProperties() const
		{
			std::vector<PCGPinProperties> properties;
			properties.emplace_back(PCGPinConstants::DefaultOutputLabel, PCGDataType::Point);
			return properties;
		}

		PCGSettingsRegistry& PCGSettingsRegistry::Get()
		{
			static PCGSettingsRegistry instance;
			return instance;
		}

		void PCGSettingsRegistry::AddPCGSettings(const Name& className, const Name& displayName, const Name& category)
		{
			PCGSettingsItem item;
			item.className = className;
			item.displayName = displayName.empty() ? className : displayName;
			item.category = category;
			m_settingsItems.push_back(std::move(item));
		}

		void RegisterPCGSettings(std::string_view className, std::string_view displayName /*= ""*/, std::string_view category /*= ""*/)
		{
			PCGSettingsRegistry::Get().AddPCGSettings(Name(className.data()), Name(displayName.data()), Name(category.data()));
		}

		std::unique_ptr<PCGSettings> CreatePCGSettings(const Name& className)
		{
			PCGClass* settingsClass = PCGClassGlobals::GetClass(className);
			PCGSettings* createdSettings = NewPCGObject<PCGSettings>(settingsClass);
			return std::unique_ptr<PCGSettings>(createdSettings);
		}

	}
}

