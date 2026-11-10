#include "EchoComponentStableHeaders.h"
#include "PCG/Elements/EchoPCGDensityFilter.h"
#include "PCG/EchoPCGHelper.h"
#include "PCG/Data/EchoPCGSpatialData.h"
#include "PCG/Data/EchoPCGPointData.h"


namespace Echo
{
	namespace PCGV2
	{
		PCG_SETTINGS_BODY_CPP(DensityFilter)
		PCGClass* PCGDensityFilterSettings::StaticClass()
		{
			BEGIN_PCG_CLASS(PCGDensityFilterSettings)
				Class.AddProperty("LowerBound", PCGPropertyType::FLOAT, STRUCT_OFFSET(PCGDensityFilterSettings, LowerBound));
			Class.AddProperty("UpperBound", PCGPropertyType::FLOAT, STRUCT_OFFSET(PCGDensityFilterSettings, UpperBound));
			Class.AddProperty("bInvertFilter", PCGPropertyType::BOOL, STRUCT_OFFSET(PCGDensityFilterSettings, bInvertFilter));
			Class.AddProperty("bKeepZeroDensityPoints", PCGPropertyType::BOOL, STRUCT_OFFSET(PCGDensityFilterSettings, bKeepZeroDensityPoints));

			PCGSettings::AppendPublicPCGSettingsProperties<PCGDensityFilterSettings>(Class);
			RegisterPCGSettings("PCGDensityFilterSettings");
			END_PCG_CLASS()
		}

		std::vector<PCGPinProperties> PCGDensityFilterSettings::InputPinProperties() const {
			return DefaultPointInputPinProperties();
		}
		std::vector<PCGPinProperties> PCGDensityFilterSettings::OutputPinProperties() const {
			return DefaultPointOutputPinProperties();
		}

		PCGElement* PCGDensityFilterElement::GetInstance()
		{
			static PCGDensityFilterElement* instance = new PCGDensityFilterElement();
			return instance;
		}

		bool PCGDensityFilterElement::ExecuteInternal(PCGContext* Context) {
			
			const PCGDensityFilterSettings* Settings = Context->GetInputSettings<PCGDensityFilterSettings>();
			EchoAssert(Settings, "PCGDensityFilterSettings is nullptr");

			const std::vector<PCGTaggedData> Inputs = Context->inputData.GetInputs();
			std::vector<PCGTaggedData>& Outputs = Context->outputData.taggedDataList;

			const bool bInvertFilter = Settings->bInvertFilter;
			const float LowerBound = Settings->LowerBound;
			const float UpperBound = Settings->UpperBound;
			const bool bKeepZeroDensityPoints = Settings->bKeepZeroDensityPoints;

			const float MinBound = Math::Min(LowerBound, UpperBound);
			const float MaxBound = Math::Max(LowerBound, UpperBound);

			const bool bNoResults = (MaxBound <= 0.0f && !bInvertFilter) || (MinBound == 0.0f && MaxBound >= 1.0f && bInvertFilter);
			const bool bTrivialFilter = (MinBound <= 0.0f && MaxBound >= 1.0f && !bInvertFilter) || (MinBound == 0.0f && MaxBound == 0.0f && bInvertFilter);

			if (bNoResults && !bKeepZeroDensityPoints)
			{
				PCGE_LOG(Verbose, LogOnly, "AllInputsRejected", "Skipped - all inputs rejected");
				return true;
			}

			// TODO: embarassingly parallel loop
			for (const PCGTaggedData& Input : Inputs)
			{
				PCGTaggedData& Output = Outputs.emplace_back(Input);

				if (!Input.data)
				{
					PCGE_LOG(Error, GraphAndLog, "InvalidInputData", "Invalid input data");
					continue;
				}

				// Skip processing if the transformation is trivial
				if (bTrivialFilter)
				{
					PCGE_LOG(Verbose, LogOnly, "TrivialFilter", "Skipped - trivial filter");
					continue;
				}

				const PCGPointData* OriginalData = Input.data->Cast<PCGSpatialData>()->ToPointData(Context);

				if (!OriginalData)
				{
					PCGE_LOG(Error, GraphAndLog, "NoPointDataInInput", "Unable to get point data from input");
					continue;
				}

				const std::vector<PCGPoint>& Points = OriginalData->GetPoints();

				PCGPointData* FilteredData = PCGContext::NewObject<PCGPointData>(Context);
				FilteredData->InitializeFromData(OriginalData);
				std::vector<PCGPoint>& FilteredPoints = FilteredData->GetMutablePoints();

				Output.data = FilteredData;

				int32 PointSize = (int32)Points.size();
				FilteredPoints.reserve(PointSize);
				for (int32 Index = 0; Index != PointSize; ++Index) {
					const PCGPoint& Point = Points[Index];

					bool bInRange = (Point.density >= MinBound && Point.density <= MaxBound);
					if (bInRange != bInvertFilter)
					{
						FilteredPoints.emplace_back(Point);
					}
#ifdef ECHO_EDITOR
					else if (bKeepZeroDensityPoints)
					{
						FilteredPoints.emplace_back(Point).density = 0.f;
					}
#endif // ECHO_EDITOR

				}
				PCGE_LOG(Verbose, LogOnly, "GenerationInfo", ("Generated " + std::to_string(FilteredPoints.size()) + " points out of " + std::to_string(Points.size()) + " source points").c_str());
			}
			
			return true;

		}

	}
}

