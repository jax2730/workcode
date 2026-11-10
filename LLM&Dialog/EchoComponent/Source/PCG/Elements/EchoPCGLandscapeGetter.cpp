#include "EchoComponentStableHeaders.h"

#include "PCG/Elements/EchoPCGLandscapeGetter.h"
#include "PCG/Data/EchoPCGLandscapeData.h"

namespace Echo
{
	namespace PCGV2
	{
		PCGGetLandscapeDataSettings::PCGGetLandscapeDataSettings()
			: PCGSettings()
		{
		}

		PCGElement* PCGGetLandscapeDataSettings::GetElement() const
		{
			return PCGGetLandscapeDataElement::GetInstance();
		}

		PCGSettings* PCGGetLandscapeDataSettings::Clone() const
		{
			return CloneT<PCGGetLandscapeDataSettings>();
		}

		REGISTER_PCG_CLASS(PCGGetLandscapeDataSettings)
		PCGClass* PCGGetLandscapeDataSettings::StaticClass()
		{
			BEGIN_PCG_CLASS(PCGGetLandscapeDataSettings)
				RegisterPCGSettings("PCGGetLandscapeDataSettings");
			END_PCG_CLASS()
		}

		std::vector<PCGPinProperties> PCGGetLandscapeDataSettings::OutputPinProperties() const
		{
			std::vector<PCGPinProperties> properties;
			properties.emplace_back(PCGPinConstants::DefaultOutputLabel, PCGDataType::Landscape);
			return properties;
		}

		PCGElement* PCGGetLandscapeDataElement::GetInstance()
		{
			static PCGGetLandscapeDataElement* instance = new PCGGetLandscapeDataElement();
			return instance;
		}

		PCGContext* PCGGetLandscapeDataElement::CreateContext()
		{
			return new PCGGetLandscapeDataContext();
		}

		bool PCGGetLandscapeDataElement::ExecuteInternal(PCGContext* inContext)
		{
			assert(inContext);
			PCGGetLandscapeDataContext* context = static_cast<PCGGetLandscapeDataContext*>(inContext);
			const PCGGetLandscapeDataSettings* settings = context->GetInputSettings<PCGGetLandscapeDataSettings>();
			assert(settings);

			PCGLandscapeData* landscapeData = PCGContext::NewObject<PCGLandscapeData>(inContext);
			landscapeData->Initialize();

			PCGTaggedData& taggedData = context->outputData.taggedDataList.emplace_back();
			taggedData.data = landscapeData;
			taggedData.pinLabel = PCGPinConstants::DefaultOutputLabel;
			return true;
		}
	}
}
