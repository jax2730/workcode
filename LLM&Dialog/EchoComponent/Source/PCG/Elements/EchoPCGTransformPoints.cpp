#include "EchoComponentStableHeaders.h"
#include "PCG/Elements/EchoPCGTransformPoints.h"
#include "PCG/EchoPCGHelper.h"
#include "PCG/Data/EchoPCGSpatialData.h"
#include "PCG/Data/EchoPCGPointData.h"
#include "EchoRandomStream.h"


namespace Echo
{
	namespace PCGV2
	{
		PCG_SETTINGS_BODY_CPP(TransformPoints)
		PCGClass* PCGTransformPointsSettings::StaticClass()
		{
			BEGIN_PCG_CLASS(PCGTransformPointsSettings)
			Class.AddProperty("bAbsoluteOffset", PCGPropertyType::BOOL, STRUCT_OFFSET(PCGTransformPointsSettings, bAbsoluteOffset));
			Class.AddProperty("OffsetMin", PCGPropertyType::VECTOR3, STRUCT_OFFSET(PCGTransformPointsSettings, OffsetMin));
			Class.AddProperty("OffsetMax", PCGPropertyType::VECTOR3, STRUCT_OFFSET(PCGTransformPointsSettings, OffsetMax));
			Class.AddProperty("bAbsoluteRotation", PCGPropertyType::BOOL, STRUCT_OFFSET(PCGTransformPointsSettings, bAbsoluteRotation));
			Class.AddProperty("RotationMin", PCGPropertyType::VECTOR3, STRUCT_OFFSET(PCGTransformPointsSettings, RotationMin));
			Class.AddProperty("RotationMax", PCGPropertyType::VECTOR3, STRUCT_OFFSET(PCGTransformPointsSettings, RotationMax));
			Class.AddProperty("bAbsoluteScale", PCGPropertyType::BOOL, STRUCT_OFFSET(PCGTransformPointsSettings, bAbsoluteScale));
			Class.AddProperty("bUniformScale", PCGPropertyType::BOOL, STRUCT_OFFSET(PCGTransformPointsSettings, bUniformScale));
			Class.AddProperty("ScaleMin", PCGPropertyType::VECTOR3, STRUCT_OFFSET(PCGTransformPointsSettings, ScaleMin));
			Class.AddProperty("ScaleMax", PCGPropertyType::VECTOR3, STRUCT_OFFSET(PCGTransformPointsSettings, ScaleMax));
			Class.AddProperty("bRecomputeSeed", PCGPropertyType::BOOL, STRUCT_OFFSET(PCGTransformPointsSettings, bRecomputeSeed));

			PCGSettings::AppendPublicPCGSettingsProperties<PCGTransformPointsSettings>(Class);
			RegisterPCGSettings("PCGTransformPointsSettings");
			END_PCG_CLASS()
		}

		std::vector<PCGPinProperties> PCGTransformPointsSettings::InputPinProperties() const {
			return DefaultPointInputPinProperties();
		}
		std::vector<PCGPinProperties> PCGTransformPointsSettings::OutputPinProperties() const {
			return DefaultPointOutputPinProperties();
		}

		PCGElement* PCGTransformPointsElement::GetInstance()
		{
			static PCGTransformPointsElement* instance = new PCGTransformPointsElement();
			return instance;
		}

		bool PCGTransformPointsElement::ExecuteInternal(PCGContext* Context) {

			const PCGTransformPointsSettings* Settings = Context->GetInputSettings<PCGTransformPointsSettings>();
			EchoAssert(Settings, "PCGTransformPointsSettings is nullptr");

			const std::vector<PCGTaggedData> Inputs = Context->inputData.GetInputs();
			std::vector<PCGTaggedData>& Outputs = Context->outputData.taggedDataList;

			//const bool bApplyToAttribute = Settings->bApplyToAttribute;
			//const Name AttributeName = Settings->AttributeName;
			const Vector3& OffsetMin = Settings->OffsetMin;
			const Vector3& OffsetMax = Settings->OffsetMax;
			const bool bAbsoluteOffset = Settings->bAbsoluteOffset;
			const Vector3& RotationMin = Settings->RotationMin;
			const Vector3& RotationMax = Settings->RotationMax;
			const bool bAbsoluteRotation = Settings->bAbsoluteRotation;
			const Vector3& ScaleMin = Settings->ScaleMin;
			const Vector3& ScaleMax = Settings->ScaleMax;
			const bool bAbsoluteScale = Settings->bAbsoluteScale;
			const bool bUniformScale = Settings->bUniformScale;
			const bool bRecomputeSeed = Settings->bRecomputeSeed;

			const int Seed = Context->GetSeed();

			// Use implicit capture, since we capture a lot
			//ProcessPoints(Context, Inputs, Outputs, [&](const FPCGPoint& InPoint, FPCGPoint& OutPoint)
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

				//Name LocalAttributeName = AttributeName;
				//const PCGMetaDataAttribute<Transform>* SourceAttribute = nullptr;
				//
				//if (bApplyToAttribute)
				//{
				//	const PCGMetaData* PointMetadata = PointData->ConstMetadata();
				//	check(PointMetadata);
				//
				//	if (LocalAttributeName == NAME_None)
				//	{
				//		LocalAttributeName = PointMetadata->GetLatestAttributeNameOrNone();
				//	}
				//
				//	// Validate that the attribute has the proper type
				//	const FPCGMetadataAttributeBase* FoundAttribute = PointMetadata->GetConstAttribute(LocalAttributeName);
				//
				//	if (!FoundAttribute || FoundAttribute->GetTypeId() != PCG::Private::MetadataTypes<FTransform>::Id)
				//	{
				//		PCGE_LOG(Error, GraphAndLog, FText::Format(LOCTEXT("AttributeMissing", "Attribute '{0}' does not exist or is not a transform"), FText::FromName(LocalAttributeName)));
				//		continue;
				//	}
				//
				//	SourceAttribute = static_cast<const FPCGMetadataAttribute<FTransform>*>(FoundAttribute);
				//}

				const std::vector<PCGPoint>& Points = PointData->GetPoints();

				PCGPointData* OutputData = PCGContext::NewObject<PCGPointData>(Context);
				OutputData->InitializeFromData(PointData);
				std::vector<PCGPoint>& OutputPoints = OutputData->GetMutablePoints();
				Output.data = OutputData;

				//FPCGMetadataAttribute<FTransform>* TargetAttribute = nullptr;
				//TArray<TTuple<int64, int64>> AllMetadataEntries;
				//
				//if (bApplyToAttribute)
				//{
				//	check(SourceAttribute && OutputData && OutputData->Metadata);
				//	TargetAttribute = OutputData->Metadata->GetMutableTypedAttribute<FTransform>(LocalAttributeName);
				//	AllMetadataEntries.SetNum(Points.Num());
				//}

				int32 PointSize = (int32)Points.size();
				OutputPoints.reserve(PointSize);
				for (int32 Index = 0; Index != PointSize; ++Index)
				{
					const PCGPoint& InPoint = Points[Index];
					PCGPoint& OutPoint = OutputPoints.emplace_back();
					OutPoint = InPoint;

					RandomStream RandomSource(PCGHelpers::ComputeSeed(Seed, InPoint.seed));

					const Vector3 RandomOffset = RandomSource.VRandRange(OffsetMin, OffsetMax);

					const Vector3 RotationXYZ = RandomSource.VRandRange(RotationMin, RotationMax);
					
					const Quaternion RandomRotation = Quaternion::QFromEuler(RotationXYZ);

					Vector3 RandomScale;
					if (bUniformScale)
					{
						RandomScale = Vector3(RandomSource.FRandRange(ScaleMin.x, ScaleMax.x));
					}
					else
					{
						RandomScale.x = RandomSource.FRandRange(ScaleMin.x, ScaleMax.x);
						RandomScale.y = RandomSource.FRandRange(ScaleMin.y, ScaleMax.y);
						RandomScale.z = RandomSource.FRandRange(ScaleMin.z, ScaleMax.z);
					}

					PCGTransform SourceTransform;

					SourceTransform = InPoint.transform;
					//if (!bApplyToAttribute)
					//{
					//	SourceTransform = InPoint.transform;
					//}
					//else
					//{
					//	//SourceTransform = SourceAttribute->GetValueFromItemKey(InPoint.MetadataEntry);
					//}

					PCGTransform FinalTransform = SourceTransform;

					if (bAbsoluteOffset)
					{
						FinalTransform.SetLocation(SourceTransform.GetLocation() + RandomOffset);
					}
					else
					{
						FinalTransform.SetLocation(SourceTransform.GetRotation()* RandomOffset + SourceTransform.GetLocation());
					}

					if (bAbsoluteRotation)
					{
						FinalTransform.SetRotation(RandomRotation);
					}
					else
					{
						FinalTransform.SetRotation(SourceTransform.GetRotation() * RandomRotation);
					}

					if (bAbsoluteScale)
					{
						FinalTransform.SetScale(RandomScale);
					}
					else
					{
						FinalTransform.SetScale(SourceTransform.GetScale() * RandomScale);
					}

					OutPoint.transform = FinalTransform;

					if (bRecomputeSeed)
					{
						const DVector3& Position = FinalTransform.GetLocation();
						OutPoint.seed = PCGHelpers::ComputeSeed((int)Position.x, (int)Position.y, (int)Position.z);
					}

					//if (!bApplyToAttribute)
					//{
					//	OutPoint.transform = FinalTransform;
					//
					//	if (bRecomputeSeed)
					//	{
					//		const DVector3& Position = FinalTransform.GetLocation();
					//		OutPoint.seed = PCGHelpers::ComputeSeed((int)Position.x, (int)Position.y, (int)Position.z);
					//	}
					//}
					//else
					//{
					//	OutPoint.MetadataEntry = OutputData->Metadata->AddEntryPlaceholder();
					//	AllMetadataEntries[Index] = MakeTuple(OutPoint.MetadataEntry, InPoint.MetadataEntry);
					//	TargetAttribute->SetValue(OutPoint.MetadataEntry, FinalTransform);
					//}
				}

				//if (TargetAttribute)
				//{
				//	OutputData->Metadata->AddDelayedEntries(AllMetadataEntries);
				//}
			}

			return true;

		}
	}
}

