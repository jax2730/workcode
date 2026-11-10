#include "EchoComponentStableHeaders.h"
#include "PCG/Elements/EchoPCGDifference.h"
#include "PCG/EchoPCGHelper.h"
#include "PCG/Data/EchoPCGSpatialData.h"
#include "PCG/Data/EchoPCGPointData.h"

namespace Echo
{
	namespace PCGV2
	{

		PCG_SETTINGS_BODY_CPP(Difference)
		PCGClass* PCGDifferenceSettings::StaticClass()
		{
			BEGIN_PCG_CLASS(PCGDifferenceSettings)

			Class.AddEnumProperty("DensityFunction", STRUCT_OFFSET(PCGDifferenceSettings, DensityFunction), StaticEnum<PCGDifferenceDensityFunction>());
			Class.AddEnumProperty("Mode", STRUCT_OFFSET(PCGDifferenceSettings, Mode), StaticEnum<PCGDifferenceMode>());
			Class.AddProperty("bDiffMetadata", PCGPropertyType::BOOL, STRUCT_OFFSET(PCGDifferenceSettings, bDiffMetadata));
			PCGSettings::AppendPublicPCGSettingsProperties<PCGDifferenceSettings>(Class);			
			RegisterPCGSettings("PCGDifferenceSettings");
			END_PCG_CLASS()
		}


		std::vector<PCGPinProperties> PCGDifferenceSettings::InputPinProperties() const {
			std::vector<PCGPinProperties> properties;
			properties.emplace_back(PCGDifferenceConstants::SourceLabel, PCGDataType::Spatial).SetRequiredPin();
			properties.emplace_back(PCGDifferenceConstants::DifferencesLabel, PCGDataType::Spatial);
#ifdef ECHO_EDITOR
			properties[0].tooltip = "SourcePropertiesTooltip\n"\
				"The source input of the operation, from which the difference will be subtracted. Multiple pins or data on this pin will be iterated upon, resulting in an output of the difference operation for every input.";
			properties[1].tooltip = "DifferencesPropertiesTooltip\n"\
				"The difference input of the operation. Inputs on this pin will be implicitly unioned together to be used in the difference operation.";
#endif // ECHO_EDITOR
			return properties;
		}
		std::vector<PCGPinProperties> PCGDifferenceSettings::OutputPinProperties() const {
			std::vector<PCGPinProperties> properties;
			properties.emplace_back(PCGPinConstants::DefaultOutputLabel, PCGDataType::Point);
#ifdef ECHO_EDITOR
			properties[0].tooltip = "OutputPropertiesTooltip\n"\
				"The data after being processed through the difference operation.";
#endif // ECHO_EDITOR
			return properties;
		}

		PCGElement* PCGDifferenceElement::GetInstance()
		{
			static PCGDifferenceElement* instance = new PCGDifferenceElement();
			return instance;
		}
		
		bool PCGDifferenceElement::ExecuteInternal(PCGContext* Context) {

			if (Context->node && !Context->node->IsInputPinConnected(PCGDifferenceConstants::SourceLabel) && !Context->node->IsInputPinConnected(PCGDifferenceConstants::DifferencesLabel))
			{
				return true;
			}

			const PCGDifferenceSettings* Settings = Context->GetInputSettings<PCGDifferenceSettings>();
			EchoAssert(Settings, "PCGDifferenceSettings is nullptr");

			std::vector<PCGTaggedData> Sources = Context->inputData.GetInputsByPin(PCGDifferenceConstants::SourceLabel);
			std::vector<PCGTaggedData> Differences = Context->inputData.GetInputsByPin(PCGDifferenceConstants::DifferencesLabel);
			std::vector<PCGTaggedData>& Outputs = Context->outputData.taggedDataList;

			bool bHasPointsInSource = false;
			bool bHasPointsInDifferences = false;

			// Validate that there's usable data on the differences pin
			bool bDifferenceDataHasValidData = false;
			for (const PCGTaggedData& Difference : Differences)
			{
				if (Difference.data && Difference.data->IsA<PCGSpatialData>())
				{
					bDifferenceDataHasValidData = true;
					bHasPointsInDifferences |= Difference.data->IsA<PCGPointData>();
					break;
				}
			}

			// If no valid difference data, send sources directly to output and return
			if (!bDifferenceDataHasValidData)
			{
				Outputs = Sources;
				return true;
			}

			// Iterate over the sources and subtract the differences
			for (PCGTaggedData& Source : Sources)
			{
				const PCGSpatialData* SourceSpatialData = Source.data->Cast<PCGSpatialData>();

				// Propagate non-spatial data
				if (!SourceSpatialData)
				{
					Outputs.emplace_back(Source);
					continue;
				}

				bHasPointsInSource |= SourceSpatialData->IsA<PCGPointData>();

				PCGDifferenceData* DifferenceData = PCGContext::NewObject<PCGDifferenceData>(Context);
				DifferenceData->Initialize(SourceSpatialData);

				for (PCGTaggedData& Difference : Differences)
				{
					if (const PCGSpatialData* DifferenceSpatialData = Difference.data->Cast<PCGSpatialData>())
					{
						DifferenceData->AddDifference(Context, DifferenceSpatialData);
					}
				}

				DifferenceData->SetDensityFunction(Settings->DensityFunction);
				DifferenceData->bDiffMetadata = Settings->bDiffMetadata;
				DifferenceData->bKeepZeroDensityPoints = Settings->bKeepZeroDensityPoints;

				PCGTaggedData& Output = Outputs.emplace_back(Source);
				Output.data = DifferenceData;

				// Finally, apply any discretization based on the mode
				if ((Settings->Mode == PCGDifferenceMode::Discrete ||
					(Settings->Mode == PCGDifferenceMode::Inferred && bHasPointsInSource && bHasPointsInDifferences)))
				{
					Output.data = DifferenceData->ToPointData(Context);
				}
			}

			return true;

		}
	}
}

