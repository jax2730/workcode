#include "EchoComponentStableHeaders.h"
#include "PCG/EchoPCGContext.h"
#include "PCG/EchoPCGComponent.h"
#include "PCG/EchoPCGSettings.h"
namespace Echo
{
	namespace PCGV2
	{
		int PCGContext::GetSeed() const
		{
			if (const PCGSettings* Settings = GetInputSettings<PCGSettings>())
			{
				return Settings->GetSeed();
			}
			//else if (!sourceComponent.expired())
			//{
			//	return sourceComponent->Seed;
			//}

			return 42;
		}

		void PCGContext::InitializeSettings()
		{
			if (node)
			{
				m_SettingsWithOverride = std::unique_ptr<PCGSettings>(node->GetSettings()->Clone());
				//TODO: parameter override
			}
			else
			{
				m_SettingsWithOverride = nullptr;
			}
		}

		String PCGContext::GetTaskName() const {
			return sourceComponent ? sourceComponent->getCombinedName() : "Non-PCG Component";
		}
		String PCGContext::GetComponentName() const {
			return sourceComponent ? sourceComponent->getCombinedName() : "Non-PCG Component";
		}

	}
}

