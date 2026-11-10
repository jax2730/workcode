#include "EchoComponentStableHeaders.h"
#include "PCG/Elements/EchoPCGDistance.h"
#include "PCG/EchoPCGHelper.h"
#include "PCG/Data/EchoPCGSpatialData.h"
#include "PCG/Data/EchoPCGPointData.h"
#include "PCG/Elements/EchoPCGGather.h"

namespace Echo
{
	namespace PCGV2
	{

		namespace PCGDistance
		{
			FVector CalcPosition(PCGDistanceShape Shape, const PCGPoint& SourcePoint, const PCGPoint& TargetPoint, const FVector SourceCenter, const FVector TargetCenter)
			{
				if (Shape == PCGDistanceShape::SphereBounds)
				{
					FVector Dir = TargetCenter - SourceCenter;
					Dir.Normalize();

					return SourceCenter + Dir * SourcePoint.GetScaledExtents().Length();
				}
				else if (Shape == PCGDistanceShape::BoxBounds)
				{
					const FVector LocalTargetCenter = SourcePoint.transform.InverseTransformPosition(TargetCenter);

					const float DistanceSquared = SourcePoint.GetLocalBounds().squaredDistance(LocalTargetCenter);

					FVector Dir = -LocalTargetCenter;
					Dir.Normalize();

					const FVector LocalClosestPoint = LocalTargetCenter + Dir * Math::Sqrt(DistanceSquared);

					return SourcePoint.transform.TransformPosition(LocalClosestPoint);
				}

				// PCGDistanceShape::Center
				return SourceCenter;
			}
		}

		DEFINE_PCG_ENUM(PCGDistanceShape,
			{ "SphereBounds", PCGDistanceShape::SphereBounds },
			{ "BoxBounds", PCGDistanceShape::BoxBounds },
			{ "Center", PCGDistanceShape::Center }
		)

		PCG_SETTINGS_BODY_CPP(Distance)
		PCGClass* PCGDistanceSettings::StaticClass()
		{
			BEGIN_PCG_CLASS(PCGDistanceSettings)
			Class.AddProperty("MaximumDistance", PCGPropertyType::FLOAT, STRUCT_OFFSET(PCGDistanceSettings, MaximumDistance));
			Class.AddEnumProperty("SourceShape", STRUCT_OFFSET(PCGDistanceSettings, SourceShape), StaticEnum<PCGDistanceShape>());
			Class.AddEnumProperty("TargetShape", STRUCT_OFFSET(PCGDistanceSettings, TargetShape), StaticEnum<PCGDistanceShape>());
			Class.AddProperty("bCheckSourceAgainstRespectiveTarget", PCGPropertyType::BOOL, STRUCT_OFFSET(PCGDistanceSettings, bCheckSourceAgainstRespectiveTarget));

			PCGSettings::AppendPublicPCGSettingsProperties<PCGDistanceSettings>(Class);
			RegisterPCGSettings("PCGDistanceSettings");
			END_PCG_CLASS()
		}


		std::vector<PCGPinProperties> PCGDistanceSettings::InputPinProperties() const {
			std::vector<PCGPinProperties> properties;
			properties.emplace_back(PCGDistance::SourceLabel, PCGDataType::Point).SetRequiredPin();
			properties.emplace_back(PCGDistance::TargetLabel, PCGDataType::Point);
#ifdef ECHO_EDITOR
			properties[0].tooltip = "PCGSourcePinTooltip\n"\
				"For each of the source points, a distance attribute will be calculated between it and the nearest target point.";
			properties[1].tooltip = "PCGTargetPinTooltip\n"\
				"The target points to conduct a distance check with each source point.";
#endif // ECHO_EDITOR
			return properties;
		}
		std::vector<PCGPinProperties> PCGDistanceSettings::OutputPinProperties() const {
			std::vector<PCGPinProperties> properties;
			properties.emplace_back(PCGPinConstants::DefaultOutputLabel, PCGDataType::Point);
#ifdef ECHO_EDITOR
			properties[0].tooltip = "PCGOutputPinTooltip\n"\
				"The source points will be output with the newly added 'Distance' attribute as well as have their density set to [0,1] based on the 'Maximum Distance' if 'Set Density' is enabled.";
#endif // ECHO_EDITOR
			return properties;
		}

		PCGElement* PCGDistanceElement::GetInstance()
		{
			static PCGDistanceElement* instance = new PCGDistanceElement();
			return instance;
		}

		bool PCGDistanceElement::ExecuteInternal(PCGContext* Context) {

			if (Context->node && !Context->node->IsInputPinConnected(PCGDistance::TargetLabel))
			{
				// If Target pin is unconnected then we no-op and pass through all data from Source pin.
				Context->outputData = PCGGather::GatherDataForPin(Context->inputData, PCGDistance::SourceLabel);
				return true;
			}

			const PCGDistanceSettings* Settings = Context->GetInputSettings<PCGDistanceSettings>();
			EchoAssert(Settings, "PCGDistanceSettings is nullptr");

			const bool bSetDensity = true;
			const PCGDistanceShape SourceShape = (PCGDistanceShape)Settings->SourceShape;
			const PCGDistanceShape TargetShape = (PCGDistanceShape)Settings->TargetShape;

			const float MaximumDistance = Math::Max(0.0f, Settings->MaximumDistance);
			const float MaximumDistanceRecip = MaximumDistance > 1.e-4f ? 1.0f / MaximumDistance : 0.0f;

			std::vector<PCGTaggedData> Sources = Context->inputData.GetInputsByPin(PCGDistance::SourceLabel);
			std::vector<PCGTaggedData> Targets = Context->inputData.GetInputsByPin(PCGDistance::TargetLabel);
			std::vector<PCGTaggedData>& Outputs = Context->outputData.taggedDataList;

			std::vector<const PCGPointData*> TargetPointDatas;
			TargetPointDatas.reserve(Targets.size());

			for (const PCGTaggedData& Target : Targets)
			{
				const PCGSpatialData* TargetData = Target.data->Cast<PCGSpatialData>();

				if (!TargetData)
				{
					PCGE_LOG(Error, GraphAndLog, "TargetMustBeSpatial", "Target must be Spatial data, found ");
					continue;
				}

				const PCGPointData* TargetPointData = TargetData->ToPointData(Context);
				if (!TargetPointData)
				{
					PCGE_LOG(Error, GraphAndLog, "CannotConvertToPoint", "Cannot convert target '{0}' into Point data");
					continue;
				}

				TargetPointDatas.emplace_back(TargetPointData);
			}

			if (TargetPointDatas.empty())
			{
				// If Target pin has no valid data then we no-op and pass through all data from Source pin.
				Context->outputData = PCGGather::GatherDataForPin(Context->inputData, PCGDistance::SourceLabel);
				return true;
			}

			if (Settings->bCheckSourceAgainstRespectiveTarget && Sources.size() > 1 && TargetPointDatas.size() > 1 && Sources.size() != TargetPointDatas.size())
			{
				//PCGLog::InputOutput::LogInvalidCardinalityError(PCGDistance::SourceLabel, PCGDistance::TargetLabel, Context);
				return true;
			}

			// First find the total Input bounds which will determine the size of each cell
			for (int32 SourceIndex = 0; SourceIndex < Sources.size(); ++SourceIndex)
			{
				const PCGTaggedData& Source = Sources[SourceIndex];
				// Add the point bounds to the input cell

				const PCGSpatialData* SourceData = Source.data->Cast<PCGSpatialData>();

				if (!SourceData)
				{
					PCGE_LOG(Error, GraphAndLog, "InvalidInputData", "Invalid input data");
					continue;
				}

				const PCGPointData* SourcePointData = SourceData->ToPointData(Context);
				if (!SourcePointData)
				{
					PCGE_LOG(Error, GraphAndLog, "CannotConvertToPointData", "Cannot convert input Spatial data to Point data");
					continue;
				}

				PCGPointData* OutputData = PCGContext::NewObject<PCGPointData>(Context);
				OutputData->InitializeFromData(SourcePointData);
				Outputs.emplace_back(Source).data = OutputData;

				//if (Settings->bOutputToAttribute && !Settings->OutputAttribute.empty())
				//{
				//	if (bOutputDistanceVector)
				//	{
				//		OutputData->Metadata->FindOrCreateAttribute<FVector>(Settings->OutputAttribute.GetAttributeName());
				//	}
				//	else
				//	{
				//		OutputData->Metadata->FindOrCreateAttribute<double>(Settings->OutputAttribute.GetAttributeName());
				//	}
				//}

				//TUniquePtr<PCGAttributeAccessor> Accessor;
				//TUniquePtr<PCGAttributeAccessorKeys> Keys;
				//
				//if (Settings->bOutputToAttribute)
				//{
				//	Accessor = PCGAttributeAccessorHelpers::CreateAccessor(OutputData, Settings->OutputAttribute);
				//}
				//
				//// If the selected attribute is a property or extra property and not the correct type, invalidate the accessor
				//if (Accessor.IsValid())
				//{
				//	using PCG::Private::MetadataTypes;
				//
				//	if ((bOutputDistanceVector && !IsBroadcastableOrConstructible(Accessor->GetUnderlyingType(), MetadataTypes<FVector>::Id)) ||
				//		(!bOutputDistanceVector && !IsBroadcastableOrConstructible(Accessor->GetUnderlyingType(), MetadataTypes<double>::Id)))
				//	{
				//		PCGE_LOG(Warning, GraphAndLog, LOCTEXT("InvalidAccessorType", "Selected type for Output Attribute is incompatible with distance as output."));
				//		Accessor = nullptr;
				//	}
				//}

				std::vector<PCGPoint>& OutPoints = OutputData->GetMutablePoints();
				const std::vector<PCGPoint>& SourcePoints = SourcePointData->GetPoints();
				OutPoints.resize(SourcePoints.size());


				//if (Accessor.IsValid())
				//{
				//	Keys = PCGAttributeAccessorHelpers::CreateKeys(OutputData, Settings->OutputAttribute);
				//
				//	if (!Keys.IsValid())
				//	{
				//		PCGE_LOG(Warning, GraphAndLog, LOCTEXT("CannotCreateAccessorKeys", "Cannot create accessor keys on output points."));
				//		Accessor = nullptr;
				//	}
				//}

				//struct TemporaryResultCache
				//{
				//	std::vector<double> Distances;
				//	std::vector<FVector> DistanceVectors;
				//} ResultCache;
				//
				//// Set up a cache so we can set all the attributes in a single range set
				////if (Accessor.IsValid())
				//{
				//	if (bOutputDistanceVector)
				//	{
				//		//ResultCache.DistanceVectors.SetNumUninitialized(SourcePoints.Num());
				//		ResultCache.DistanceVectors.resize(SourcePoints.size());
				//	}
				//	else
				//	{
				//		//ResultCache.Distances.SetNumUninitialized(SourcePoints.Num());
				//		ResultCache.Distances.resize(SourcePoints.size());
				//	}
				//}

				auto ProcessDistance = [&](int32 ReadIndex, int32 WriteIndex)
					{
						PCGPoint& OutPoint = OutPoints[WriteIndex];
						const PCGPoint& SourcePoint = SourcePoints[ReadIndex];

						OutPoint = SourcePoint;
						
						const BoxSphereBounds SourceQueryBounds = BoxSphereBounds(SourcePoint.transform.Transform(AxisAlignedBox(SourcePoint.boundsMin - FVector(MaximumDistance), SourcePoint.boundsMax + FVector(MaximumDistance))));

						const FVector SourceCenter = SourcePoint.transform.TransformPosition(SourcePoint.GetLocalCenter());

						float MinDistanceSquared = MaximumDistance * MaximumDistance;
						FVector MinDistanceVector = FVector::ZERO;

						// Signed distance field for calculating the closest point of source and target
						auto CalculateSDF = [&MinDistanceSquared, &MinDistanceVector, &SourcePoint, SourceCenter, SourceShape, TargetShape](const PCGPointRef& TargetPointRef) -> bool
							{
								// If the source pointer and target pointer are the same, ignore distance to the exact same point
								if (&SourcePoint == TargetPointRef.Point)
								{
									return true;
								}

								const PCGPoint& TargetPoint = *TargetPointRef.Point;
								const FVector& TargetCenter = TargetPointRef.Bounds.getCenter();

								const FVector SourceShapePos = PCGDistance::CalcPosition(SourceShape, SourcePoint, TargetPoint, SourceCenter, TargetCenter);
								const FVector TargetShapePos = PCGDistance::CalcPosition(TargetShape, TargetPoint, SourcePoint, TargetCenter, SourceCenter);

								const FVector ToTargetShapeDir = TargetShapePos - SourceShapePos;
								const FVector ToTargetCenterDir = TargetCenter - SourceCenter;

								const float Sign = Math::Sign(ToTargetShapeDir.Dot(ToTargetCenterDir));
								const float ThisDistanceSquared = ToTargetShapeDir.LengthSq() * Sign;

								if (ThisDistanceSquared < MinDistanceSquared)
								{
									MinDistanceSquared = ThisDistanceSquared;
									MinDistanceVector = ToTargetShapeDir;
								}
								return true;
							};

						auto CheckAgainstTargetPointData = [&SourceQueryBounds, &CalculateSDF](const PCGPointData* TargetPointData)
							{
								EchoAssert(TargetPointData, "TargetPointData is nullptr");

								const PCGPointData::PointOctree& Octree = TargetPointData->GetOctree();

								Octree.FindFirstElementWithBoundsTest(SourceQueryBounds, CalculateSDF);
							};

						if (Settings->bCheckSourceAgainstRespectiveTarget)
						{
							CheckAgainstTargetPointData(TargetPointDatas[SourceIndex % TargetPointDatas.size()]);
						}
						else
						{
							for (const PCGPointData* TargetPointData : TargetPointDatas)
							{
								CheckAgainstTargetPointData(TargetPointData);
							}
						}

						const float Distance = Math::Sign(MinDistanceSquared) * Math::Sqrt(Math::Abs(MinDistanceSquared));

						//if (bWriteToAttribute)
						//{
						//	if (bOutputDistanceVector)
						//	{
						//		ResultCache.DistanceVectors[WriteIndex] = MinDistanceVector;
						//	}
						//	else
						//	{
						//		ResultCache.Distances[WriteIndex] = Distance;
						//	}
						//}

						if (bSetDensity)
						{
							OutPoint.density = MaximumDistance > 1.e-4f ? (float)(Math::Clamp(Distance, -MaximumDistance, MaximumDistance) * MaximumDistanceRecip) : 1.0f;
						}

						return true;
					};

				int SourcePointSize = (int)SourcePoints.size();
				for (int sourcePointIndex = 0; sourcePointIndex != SourcePointSize; ++sourcePointIndex) {
					ProcessDistance(sourcePointIndex, sourcePointIndex);
				}
				//if (bOutputDistanceVector)
				//{
				//	for (int sourcePointIndex = 0; sourcePointIndex != SourcePointSize; ++sourcePointIndex) {
				//		ResultCache.DistanceVectors[sourcePointIndex] = OutPoints[sourcePointIndex].density;
				//	}
				//}
				//else
				//{
				//	for (int sourcePointIndex = 0; sourcePointIndex != SourcePointSize; ++sourcePointIndex) {
				//		ResultCache.DistanceVectors[sourcePointIndex] = OutPoints[sourcePointIndex].density;
				//	}
				//}

				//if (FPCGAsync::AsyncProcessingOneToOneEx(&Context->AsyncState, SourcePoints.Num(), /*InitializeFunc=*/[] {}, ProcessDistance, /*bEnableTimeSlicing=*/false))
				//{
				//	if (Accessor.IsValid())
				//	{
				//		// Set all the attributes at once
				//		if (bOutputDistanceVector)
				//		{
				//			Accessor->SetRange<FVector>(ResultCache.DistanceVectors, 0, *Keys);
				//		}
				//		else
				//		{
				//			Accessor->SetRange<double>(ResultCache.Distances, 0, *Keys);
				//		}
				//	}
				//}
			}

			return true;

		}
	}
}

