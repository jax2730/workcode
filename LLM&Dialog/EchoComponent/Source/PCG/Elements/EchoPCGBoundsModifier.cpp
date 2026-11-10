#include "EchoComponentStableHeaders.h"
#include "PCG/Elements/EchoPCGBoundsModifier.h"
#include "PCG/EchoPCGHelper.h"
#include "PCG/Data/EchoPCGSpatialData.h"
#include "PCG/Data/EchoPCGPointData.h"

namespace Echo
{
	namespace PCGV2
	{
		DEFINE_PCG_ENUM(PCGBoundsModifierMode,
			{ "Set", PCGBoundsModifierMode::Set },
			{ "Intersect", PCGBoundsModifierMode::Intersect },
			{ "Include", PCGBoundsModifierMode::Include },
			{ "Translate", PCGBoundsModifierMode::Translate },
			{ "Scale", PCGBoundsModifierMode::Scale }
		)

		PCG_SETTINGS_BODY_CPP(BoundsModifier)
			PCGClass* PCGBoundsModifierSettings::StaticClass()
		{
			BEGIN_PCG_CLASS(PCGBoundsModifierSettings)
			Class.AddEnumProperty("Mode", STRUCT_OFFSET(PCGBoundsModifierSettings, Mode), StaticEnum<PCGBoundsModifierMode>());
			Class.AddProperty("BoundsMin", PCGPropertyType::VECTOR3, STRUCT_OFFSET(PCGBoundsModifierSettings, BoundsMin));
			Class.AddProperty("BoundsMax", PCGPropertyType::VECTOR3, STRUCT_OFFSET(PCGBoundsModifierSettings, BoundsMax));
			Class.AddProperty("bAffectSteepness", PCGPropertyType::BOOL, STRUCT_OFFSET(PCGBoundsModifierSettings, bAffectSteepness));
			Class.AddProperty("Steepness", PCGPropertyType::FLOAT, STRUCT_OFFSET(PCGBoundsModifierSettings, Steepness));

			PCGSettings::AppendPublicPCGSettingsProperties<PCGBoundsModifierSettings>(Class);
			RegisterPCGSettings("PCGBoundsModifierSettings");
			END_PCG_CLASS()
		}

		std::vector<PCGPinProperties> PCGBoundsModifierSettings::InputPinProperties() const {
			return DefaultPointInputPinProperties();
		}
		std::vector<PCGPinProperties> PCGBoundsModifierSettings::OutputPinProperties() const {
			return DefaultPointOutputPinProperties();
		}

		PCGElement* PCGBoundsModifierElement::GetInstance()
		{
			static PCGBoundsModifierElement* instance = new PCGBoundsModifierElement();
			return instance;
		}

		bool PCGBoundsModifierElement::ExecuteInternal(PCGContext* Context) {

			const PCGBoundsModifierSettings* Settings = Context->GetInputSettings<PCGBoundsModifierSettings>();
			EchoAssert(Settings, "PCGBoundsModifierSettings is nullptr");

			const std::vector<PCGTaggedData> Inputs = Context->inputData.GetInputs();
			std::vector<PCGTaggedData>& Outputs = Context->outputData.taggedDataList;

			for (const PCGTaggedData& Input : Inputs)
			{
				PCGTaggedData& Output = Outputs.emplace_back(Input);

				const PCGSpatialData* SpatialData = Input.data->Cast<PCGSpatialData>();

				if (!SpatialData)
				{
					PCGE_LOG(Error, GraphAndLog, "InputMissingSpatialData", "Unable to get Spatial data from input");
					continue;
				}

				const PCGPointData* PointData = SpatialData->ToPointData(Context);

				if (!PointData)
				{
					PCGE_LOG(Error, GraphAndLog, "InputMissingPointData", "Unable to get Point data from input");
					continue;
				}

				AxisAlignedBox settingBound(Settings->BoundsMin, Settings->BoundsMax);

				const std::vector<PCGPoint>& Points = PointData->GetPoints();

				PCGPointData* OutputData = PCGContext::NewObject<PCGPointData>(Context);
				OutputData->InitializeFromData(PointData);
				std::vector<PCGPoint>& OutputPoints = OutputData->GetMutablePoints();
				Output.data = OutputData;


				int32 PointSize = (int32)Points.size();
				OutputPoints.reserve(PointSize);
				for (int32 Index = 0; Index != PointSize; ++Index)
				{
					const PCGPoint& InPoint = Points[Index];
					PCGPoint& OutPoint = OutputPoints.emplace_back();
					OutPoint = InPoint;
					

					switch (Settings->Mode)
					{
					case PCGBoundsModifierMode::Intersect:
					{
						AxisAlignedBox InBound(InPoint.boundsMin, InPoint.boundsMax);
						InBound = InBound.intersection(settingBound);
						OutPoint.boundsMin = InBound.mMinimum;
						OutPoint.boundsMax = InBound.mMaximum;
						if (Settings->bAffectSteepness)
						{
							OutPoint.steepness = Math::Min(InPoint.steepness, Settings->Steepness);
						}
						break;
					}

					case PCGBoundsModifierMode::Include:
					{
						AxisAlignedBox InBound(InPoint.boundsMin, InPoint.boundsMax);
						InBound.merge(settingBound);
						OutPoint.boundsMin = InBound.mMinimum;
						OutPoint.boundsMax = InBound.mMaximum;
						if (Settings->bAffectSteepness)
						{
							OutPoint.steepness = Math::Max(InPoint.steepness, Settings->Steepness);
						}
						break;
					}

					case PCGBoundsModifierMode::Translate:
					{
						OutPoint.boundsMin += Settings->BoundsMin;
						OutPoint.boundsMax += Settings->BoundsMax;

						if (Settings->bAffectSteepness)
						{
							OutPoint.steepness = Math::Clamp(InPoint.steepness + Settings->Steepness, 0.0f, 1.0f);
						}

						break;
					}
					case PCGBoundsModifierMode::Scale:
					{
						OutPoint.boundsMin *= Settings->BoundsMin;
						OutPoint.boundsMax *= Settings->BoundsMax;

						if (Settings->bAffectSteepness)
						{
							OutPoint.steepness = Math::Clamp(InPoint.steepness * Settings->Steepness, 0.0f, 1.0f);
						}

						break;
					}
					case PCGBoundsModifierMode::Set:
					{
						OutPoint.boundsMin = Settings->BoundsMin;
						OutPoint.boundsMax = Settings->BoundsMax;
						if (Settings->bAffectSteepness)
						{
							OutPoint.steepness = Settings->Steepness;
						}

						break;
					}
					default:
						break;
					}
				}
			}

			return true;
		}
	}
}

