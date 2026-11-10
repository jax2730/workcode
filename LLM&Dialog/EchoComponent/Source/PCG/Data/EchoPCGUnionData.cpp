#include "EchoComponentStableHeaders.h"

#include "PCG/Data/EchoPCGUnionData.h"
#include "PCG/Data/EchoPCGPointData.h"
#include "PCG/EchoPCGContext.h"

namespace Echo
{
	namespace PCGV2
	{
		namespace PCGUnionDataMaths
		{
			float ComputeDensity(float InDensityToUpdate, float InOtherDensity, PCGUnionDensityFunction DensityFunction)
			{
				if (DensityFunction == PCGUnionDensityFunction::ClampedAddition)
				{
					return Math::Min(InDensityToUpdate + InOtherDensity, 1.0f);
				}
				else if (DensityFunction == PCGUnionDensityFunction::Binary)
				{
					return (InOtherDensity > 0.f) ? 1.0f : InDensityToUpdate;
				}
				else // Maximum
				{
					return Math::Max(InDensityToUpdate, InOtherDensity);
				}
			}

			float UpdateDensity(float& InDensityToUpdate, float InOtherDensity, PCGUnionDensityFunction DensityFunction)
			{
				InDensityToUpdate = ComputeDensity(InDensityToUpdate, InOtherDensity, DensityFunction);
				return InDensityToUpdate;
			}
		}


		DEFINE_PCG_ENUM(PCGUnionType,
			{ "LeftToRightPriority", PCGUnionType::LeftToRightPriority },
			{ "RightToLeftPriority", PCGUnionType::RightToLeftPriority },
			{ "KeepAll", PCGUnionType::KeepAll }
		)


		DEFINE_PCG_ENUM(PCGUnionDensityFunction,
			{ "Maximum", PCGUnionDensityFunction::Maximum },
			{ "ClampedAddition", PCGUnionDensityFunction::ClampedAddition },
			{ "Binary", PCGUnionDensityFunction::Binary }
		)


		void PCGUnionData::Initialize(const PCGSpatialData* InA, const PCGSpatialData* InB)
		{
			if (InA == nullptr || InB == nullptr) return;
			AddData(InA);
			AddData(InB);
		}

		int PCGUnionData::GetDimension() const
		{
			return CachedDimension;
		}
		FBox PCGUnionData::GetBounds() const
		{
			return CachedBounds;
		}
		FBox PCGUnionData::GetStrictBounds() const
		{
			return CachedStrictBounds;
		}
		bool PCGUnionData::SamplePoint(const PCGTransform& InTransform, const FBox& InBounds, PCGPoint& OutPoint, PCGMetadata* OutMetadata) const
		{
			PCGTransform PointTransform = InTransform;
			bool bHasSetPoint = false;

			if (FirstNonTrivialTransformData)
			{
				if (FirstNonTrivialTransformData->SamplePoint(InTransform, InBounds, OutPoint, OutMetadata))
				{
					PointTransform = OutPoint.transform;
					bHasSetPoint = true;

					if (DensityFunction == PCGUnionDensityFunction::Binary && OutPoint.density > 0.0f)
					{
						OutPoint.density = 1.0f;
					}
				}
			}

			std::vector<TObjectPtr<const PCGSpatialData>> DataRawPtr = Data;

			const bool bSkipLoop = (bHasSetPoint && !OutMetadata && OutPoint.density >= 1.0f);
			const int32 DataCount = (int32)DataRawPtr.size();
			for (int32 DataIndex = 0; DataIndex < DataCount && !bSkipLoop; ++DataIndex)
			{
				if (DataRawPtr[DataIndex] == FirstNonTrivialTransformData)
				{
					continue;
				}

				PCGPoint PointInData;
				if (DataRawPtr[DataIndex]->SamplePoint(PointTransform, InBounds, PointInData, OutMetadata))
				{
					if (!bHasSetPoint)
					{
						OutPoint = PointInData;
						bHasSetPoint = true;
					}
					else
					{
						// Update density
						PCGUnionDataMaths::UpdateDensity(OutPoint.density, PointInData.density, DensityFunction);

						//OutPoint.color = Vector4(
						//	FMath::Max(OutPoint.Color.X, PointInData.Color.X),
						//	FMath::Max(OutPoint.Color.Y, PointInData.Color.Y),
						//	FMath::Max(OutPoint.Color.Z, PointInData.Color.Z),
						//	FMath::Max(OutPoint.Color.W, PointInData.Color.W));

						// Merge properties into OutPoint
						//if (OutMetadata)
						//{
						//	if (OutPoint.MetadataEntry != PCGInvalidEntryKey && PointInData.MetadataEntry != PCGInvalidEntryKey)
						//	{
						//		OutMetadata->MergePointAttributesSubset(OutPoint, OutMetadata, OutMetadata, PointInData, OutMetadata, DataRawPtr[DataIndex]->Metadata, OutPoint, EPCGMetadataOp::Max);
						//	}
						//	else if (PointInData.MetadataEntry != PCGInvalidEntryKey)
						//	{
						//		OutPoint.MetadataEntry = PointInData.MetadataEntry;
						//	}
						//}
					}

					if (bHasSetPoint && !OutMetadata && OutPoint.density >= 1.0f)
					{
						break;
					}
				}
			}

			return (bHasSetPoint && OutPoint.density > 0);
		}

		bool PCGUnionData::HasNonTrivialTransform() const {
			return (FirstNonTrivialTransformData != nullptr || PCGSpatialDataWithPointCache::HasNonTrivialTransform());
		}

		const PCGSpatialData* PCGUnionData::FindFirstConcreteShapeFromNetwork() const
		{
			for (const PCGSpatialData* Datum : Data)
			{
				const PCGSpatialData* Candidate = Datum ? Datum->FindFirstConcreteShapeFromNetwork() : nullptr;
				if (Candidate)
				{
					return Candidate;
				}
			}

			return nullptr;
		}
		const PCGPointData* PCGUnionData::CreatePointData(PCGContext* Context) const
		{
			const bool bBinaryDensity = (DensityFunction == PCGUnionDensityFunction::Binary);
			
			// Trivial results
			if (Data.size() == 0)
			{
				//UE_LOG(LogPCG, Error, TEXT("Invalid union"));
				return nullptr;
			}
			else if (Data.size() == 1 && !bBinaryDensity)
			{
				//UE_LOG(LogPCG, Verbose, TEXT("Union is trivial"));
				return Data[0]->ToPointData(Context);
			}
			
			// Cache raw pointers for metadata as these are much faster to use
			std::vector<TObjectPtr<const PCGSpatialData>> DataRawPtr = Data;
			std::vector<const PCGMetadata*> InputMetadatas;
			InputMetadatas.resize(Data.size());
			for (int32 i = 0; i < (int32)DataRawPtr.size(); i++)
			{
				InputMetadatas[i] = DataRawPtr[i]->ToPointData(Context)->Metadata;
			}

			PCGPointData* PointData = PCGContext::NewObject<PCGPointData>(Context);
			PointData->InitializeFromData(this, InputMetadatas[0]);
			PointData->InitializeFromData(this, nullptr);

			PCGMetadata* OutMetadata = PointData->Metadata;
			//check(OutMetadata);

			// Initialize metadata
			for (const PCGMetadata* InputMetadata : InputMetadatas)
			{
				OutMetadata->AddAttributes(InputMetadata);
			}

			switch (UnionType)
			{
			case PCGUnionType::LeftToRightPriority:
			default:
				CreateSequentialPointData(Context, DataRawPtr, InputMetadatas, PointData, OutMetadata, /*bLeftToRight=*/true);
				break;
			
			case PCGUnionType::RightToLeftPriority:
				CreateSequentialPointData(Context, DataRawPtr, InputMetadatas, PointData, OutMetadata, /*bLeftToRight=*/false);
				break;
			
			case PCGUnionType::KeepAll:
			{
				std::vector<PCGPoint>& TargetPoints = PointData->GetMutablePoints();
				for (int32 DataIndex = 0; DataIndex < (int32)DataRawPtr.size(); ++DataIndex)
				{
					const PCGSpatialData* Datum = DataRawPtr[DataIndex];
					const PCGPointData* DatumPointData = Datum->ToPointData(Context);
					//const PCGMetadata* DatumPointMetadata = DatumPointData->Metadata;
			
					//int32 TargetPointOffset = (int32)TargetPoints.size();
					TargetPoints.insert(TargetPoints.end(), DatumPointData->GetPoints().begin(), DatumPointData->GetPoints().end());
			
					//if (DataIndex > 0 && DatumPointData->GetPoints().size() > 0)
					//{
					//	// TODO: could optimize case where there is a common parent between Data 0 and current data, for points that still point to common parent metadata.
					//	//std::vector<PCGPoint> TargetPointsSubset = MakeArrayView(&TargetPoints[TargetPointOffset], DatumPointData->GetPoints().Num());
					//	//for (FPCGPoint& Point : TargetPointsSubset)
					//	//{
					//	//	Point.MetadataEntry = PCGInvalidEntryKey;
					//	//}
					//
					//	//if (OutMetadata && DatumPointMetadata && DatumPointMetadata->GetAttributeCount() > 0)
					//	//{
					//	//	OutMetadata->SetPointAttributes(MakeArrayView(DatumPointData->GetPoints()), DatumPointMetadata, TargetPointsSubset, Context);
					//	//}
					//}
				}
			
				// Correct density for binary-style union
				if (bBinaryDensity)
				{
					for (PCGPoint& TargetPoint : TargetPoints)
					{
						TargetPoint.density = ((TargetPoint.density > 0.f) ? 1.0f : 0.f);
					}
				}
				break;
			}
			}
			
			//UE_LOG(LogPCG, Verbose, TEXT("Union generated %d points out of %d data sources"), PointData->GetPoints().Num(), Data.Num());
			
			return PointData;
		}

		void PCGUnionData::AddData(const PCGSpatialData* InData)
		{
			if (InData == nullptr) return;

			Data.emplace_back(InData);

			if (Data.size() == 1)
			{
				TargetActor = InData->TargetActor;
				CachedBounds = InData->GetBounds();
				CachedStrictBounds = InData->GetStrictBounds();
				CachedDimension = InData->GetDimension();
				Metadata->Initialize(InData->Metadata);
			}
			else
			{
				CachedBounds.Add(InData->GetBounds());
				CachedStrictBounds = CachedStrictBounds.intersection(InData->GetStrictBounds());
				CachedDimension = Math::Max(CachedDimension, InData->GetDimension());
				Metadata->AddAttributes(InData->Metadata);
			}

			if (!FirstNonTrivialTransformData && InData->HasNonTrivialTransform())
			{
				FirstNonTrivialTransformData = InData;
			}
		}

		void PCGUnionData::CreateSequentialPointData(PCGContext* Context, std::vector<TObjectPtr<const PCGSpatialData>>& InputDatas, std::vector<const PCGMetadata*>& InputMetadatas, PCGPointData* PointData, PCGMetadata* OutMetadata, bool bLeftToRight) const
		{
			std::vector<PCGPoint>& TargetPoints = PointData->GetMutablePoints();
			
			int32 FirstDataIndex = (bLeftToRight ? 0 : (int32)InputDatas.size() - 1);
			int32 LastDataIndex = (bLeftToRight ? (int32)InputDatas.size() : -1);
			int32 DataIndexIncrement = (bLeftToRight ? 1 : -1);

			// Note: this is a O(N^2) implementation. 
			// TODO: It is easy to implement a kind of divide & conquer algorithm here, but it will require some temporary storage.
			for (int32 DataIndex = FirstDataIndex; DataIndex != LastDataIndex; DataIndex += DataIndexIncrement)
			{
				// For each point, if it is not already "processed" by previous data,
				// add it & compute its final density
				const PCGPointData* CurrentPointData = InputDatas[DataIndex]->ToPointData(Context);
				const std::vector<PCGPoint>& Points = CurrentPointData->GetPoints();

				
				for (int index = 0; index < (int)Points.size(); ++index) {
					const PCGPoint& Point = Points[index];

					// Discard point if it is already covered by a previous data
					bool bPointToExclude = false;
					for (int32 PreviousDataIndex = FirstDataIndex; PreviousDataIndex != DataIndex; PreviousDataIndex += DataIndexIncrement)
					{
						if (InputDatas[PreviousDataIndex]->GetDensityAtPosition(Point.transform.GetLocation()) != 0)
						{
							bPointToExclude = true;
							break;
						}
					}

					if (bPointToExclude)
					{
						continue;
					}
					PCGPoint& OutPoint = TargetPoints.emplace_back();
					OutPoint = Point;
					//if (OutMetadata->GetParent() != InputMetadatas[DataIndex])
					//{
					//	UPCGMetadataAccessorHelpers::InitializeMetadata(OutPoint, OutMetadata);
					//	// Since we can't inherit from the parent point, we'll set the values directly here
					//	OutMetadata->SetPointAttributes(Point, InputMetadatas[DataIndex], OutPoint);
					//}

					if (DensityFunction == PCGUnionDensityFunction::Binary && OutPoint.density > 0.f)
					{
						OutPoint.density = 1.0f;
					}

					// Update density & metadata based on current & following data
					for (int32 FollowingDataIndex = DataIndex + DataIndexIncrement; FollowingDataIndex != LastDataIndex; FollowingDataIndex += DataIndexIncrement)
					{
						//const PCGMetadata* FollowingMetadata = InputMetadatas[FollowingDataIndex];

						// If density is saturated and there are no metadata attributes then we can skip this data as it will not contribute.
						//if (OutPoint.density >= 1.0f && (!FollowingMetadata || FollowingMetadata->GetAttributeCount() == 0))
						if (OutPoint.density >= 1.0f)
						{
							continue;
						}

						PCGPoint PointInData;
						if (InputDatas[FollowingDataIndex]->SamplePoint(OutPoint.transform, OutPoint.GetLocalBounds(), PointInData, OutMetadata))
						{
							// Update density
							PCGUnionDataMaths::UpdateDensity(OutPoint.density, PointInData.density, DensityFunction);

							// Update color
							//OutPoint.Color = FVector4(
							//	FMath::Max(OutPoint.Color.X, PointInData.Color.X),
							//	FMath::Max(OutPoint.Color.Y, PointInData.Color.Y),
							//	FMath::Max(OutPoint.Color.Z, PointInData.Color.Z),
							//	FMath::Max(OutPoint.Color.W, PointInData.Color.W));

							//if (OutPoint.MetadataEntry != PCGInvalidEntryKey && PointInData.MetadataEntry != PCGInvalidEntryKey)
							//{
							//	OutMetadata->MergePointAttributesSubset(OutPoint, OutMetadata, OutMetadata, PointInData, OutMetadata, FollowingMetadata, OutPoint, EPCGMetadataOp::Max);
							//}
							//else if (PointInData.MetadataEntry != PCGInvalidEntryKey)
							//{
							//	OutPoint.MetadataEntry = PointInData.MetadataEntry;
							//}
						}
					}
				}

			}
		}
	}
}
