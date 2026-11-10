#include "EchoComponentStableHeaders.h"

#include "PCG/Data/EchoPCGDifferenceData.h"
#include "PCG/Data/EchoPCGUnionData.h"
#include "PCG/Data/EchoPCGPointData.h"
#include "PCG/EchoPCGContext.h"

namespace Echo
{
	namespace PCGV2
	{
		DEFINE_PCG_ENUM(PCGDifferenceDensityFunction,
			{ "Minimum", PCGDifferenceDensityFunction::Minimum },
			{ "ClampedSubstraction", PCGDifferenceDensityFunction::ClampedSubstraction },
			{ "Binary", PCGDifferenceDensityFunction::Binary }
		)


		DEFINE_PCG_ENUM(PCGDifferenceMode,
			{ "Inferred", PCGDifferenceMode::Inferred },
			{ "Continuous", PCGDifferenceMode::Continuous },
			{ "Discrete", PCGDifferenceMode::Discrete }
		)

		PCGUnionDensityFunction ToUnionDensityFunction(PCGDifferenceDensityFunction InDensityFunction)
		{
			if (InDensityFunction == PCGDifferenceDensityFunction::ClampedSubstraction)
			{
				return PCGUnionDensityFunction::ClampedAddition;
			}
			else if (InDensityFunction == PCGDifferenceDensityFunction::Binary)
			{
				return PCGUnionDensityFunction::Binary;
			}
			else
			{
				return PCGUnionDensityFunction::Maximum;
			}
		}

		void PCGDifferenceData::Initialize(const PCGSpatialData* InData)
		{
			if (InData == nullptr) return;
			Source = InData;
			TargetActor = InData->TargetActor;
		}

		void PCGDifferenceData::K2_AddDifference(const PCGSpatialData* InDifference)
		{
			return AddDifference(nullptr, InDifference);
		}

		void PCGDifferenceData::AddDifference(PCGContext* InContext, const PCGSpatialData* InDifference)
		{
			if (InDifference == nullptr) return;

			if (!GetBounds().Intersect(InDifference->GetBounds())) {
				return;
			}
			if (!Difference) {
				Difference = InDifference;
			}
			else {
				if (!DifferencesUnion) {
					DifferencesUnion = PCGContext::NewObject<PCGUnionData>(InContext);
					DifferencesUnion->AddData(Difference);
					DifferencesUnion->SetDensityFunction(ToUnionDensityFunction(DensityFunction));
					Difference = DifferencesUnion;
				}

				DifferencesUnion->AddData(InDifference);
			}
		}

		void PCGDifferenceData::SetDensityFunction(PCGDifferenceDensityFunction InDensityFunction)
		{
			DensityFunction = InDensityFunction;

			if (GetDifferencesUnion())
			{
				GetDifferencesUnion()->SetDensityFunction(ToUnionDensityFunction(DensityFunction));
			}
		}

		int PCGDifferenceData::GetDimension() const
		{
			return GetSource()->GetDimension();
		}
		FBox PCGDifferenceData::GetBounds() const
		{
			return GetSource()->GetBounds();
		}
		FBox PCGDifferenceData::GetStrictBounds() const
		{
			return GetSource()->GetStrictBounds();
		}
		bool PCGDifferenceData::SamplePoint(const PCGTransform& InTransform, const FBox& InBounds, PCGPoint& OutPoint, PCGMetadata* OutMetadata) const
		{
			PCGPoint PointFromSource;
			if (!GetSource()->SamplePoint(InTransform, InBounds, PointFromSource, OutMetadata))
			{
				return false;
			}
			OutPoint = PointFromSource;

			PCGPoint PointFromDiff;
			if (GetDifference() && GetDifference()->SamplePoint(InTransform, InBounds, PointFromDiff, OutMetadata))
			{
				const bool bBinaryDensity = (DensityFunction == PCGDifferenceDensityFunction::Binary);

				OutPoint.density = bBinaryDensity ? 0.0f : Math::Max(0.0f, PointFromSource.density - PointFromDiff.density);

				return OutPoint.density > 0.0f;
			}
			else
			{
				return true;
			}
		}
		bool PCGDifferenceData::HasNonTrivialTransform() const
		{
			return GetSource()->HasNonTrivialTransform();
		}

		const PCGPointData* PCGDifferenceData::CreatePointData(PCGContext* Context) const
		{
			const PCGPointData* SourcePointData = GetSource()->ToPointData(Context);

			if (!SourcePointData)
			{
				PCGE_LOG_C(LogPCG, Error, Context, "PCGDifferenceData", "Difference unable to get source points");
				return nullptr;
			}

			if (!GetDifference())
			{
				PCGE_LOG_C(LogPCG, Verbose, Context, "PCGDifferenceData", "Difference is trivial");
				return nullptr;
			}

			const PCGMetadata* SourceMetadata = SourcePointData->Metadata;
			const std::vector<PCGPoint>& SourcePoints = SourcePointData->GetPoints();

			PCGPointData* Data = PCGContext::NewObject<PCGPointData>(Context);
			Data->InitializeFromData(this, SourceMetadata);

			PCGMetadata* OutMetadata = Data->Metadata;
			std::vector<PCGPoint>& TargetPoints = Data->GetMutablePoints();

			const PCGMetadata* DifferenceMetadata = GetDifference()->Metadata;
			TObjectPtr<PCGMetadata> TempDiffMetadata = nullptr;
			if (bDiffMetadata && OutMetadata && DifferenceMetadata)
			{
				//TempDiffMetadata = PCGContext::NewObject<PCGMetadata>(Context);
				//TempDiffMetadata->Initialize(DifferenceMetadata);
			}

			{
				std::vector<std::pair<PCGTransform, FBox>> Samples(SourcePoints.size());
				const int NumPoints = (int)Samples.size();

				for (int pointIndex = 0; pointIndex < NumPoints; ++pointIndex) {
					Samples[pointIndex].first = SourcePoints[pointIndex].transform;
					Samples[pointIndex].second = SourcePoints[pointIndex].GetLocalBounds();
				}

				std::vector<PCGPoint> PointsFromDiff;
				PointsFromDiff.resize(NumPoints);

				GetDifference()->SamplePoints(Samples, PointsFromDiff, TempDiffMetadata);

				const bool bBinaryDensity = (DensityFunction == PCGDifferenceDensityFunction::Binary);

				for (int PointIndex = 0; PointIndex < NumPoints; ++PointIndex)
				{
					const PCGPoint& Point = SourcePoints[PointIndex];
					PCGPoint& PointFromDiff = PointsFromDiff[PointIndex];

					const float Density = (bBinaryDensity && PointFromDiff.density > 0) ? 0.0f : Point.density - PointFromDiff.density;

					if (Density > 0)
					{
						PCGPoint& OutPoint = TargetPoints.emplace_back(Point);
						OutPoint.density = Density;

						// TODO: create an array-based MergePointsAttributeSubset
						//if (TempDiffMetadata && PointFromDiff.MetadataEntry != PCGInvalidEntryKey)
						//{
						//	OutMetadata->MergePointAttributesSubset(Point, SourceMetadata, SourceMetadata, PointFromDiff, TempDiffMetadata, TempDiffMetadata, OutPoint, EPCGMetadataOp::Sub);
						//}
					}
					else if (bKeepZeroDensityPoints)
					{
						PCGPoint& RejectedPoint = TargetPoints.emplace_back(Point);
						RejectedPoint.density = 0;
					}
				}
			}

			return Data;
		}
	}
}
