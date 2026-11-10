#include "EchoComponentStableHeaders.h"

#include "PCG/Data/EchoPCGSpatialData.h"

#include "PCG/Data/EchoPCGPointData.h"

namespace Echo
{
	namespace PCGV2
	{
		float PCGSpatialData::GetDensityAtPosition(const DVector3& InPosition) const
		{
			PCGPoint TemporaryPoint;
			if (SamplePoint(PCGTransform(InPosition), AxisAlignedBox(Vector3::ZERO, Vector3::ZERO), TemporaryPoint, nullptr))
			{
				return TemporaryPoint.density;
			}
			else
			{
				return 0.f;
			}
		}

		bool PCGSpatialData::ProjectPoint(const PCGTransform& InTransform, const AxisAlignedBox& InBounds, const PCGProjectionParams& InParams, PCGPoint& OutPoint, PCGMetadata* OutMetadata) const
		{
			
			const bool bResult = SamplePoint(InTransform, InBounds, OutPoint, OutMetadata);

			if (!InParams.bProjectPositions)
			{
				OutPoint.transform.SetLocation(InTransform.GetLocation());
			}

			if (!InParams.bProjectRotations)
			{
				OutPoint.transform.SetRotation(InTransform.GetRotation());
			}

			if (!InParams.bProjectScales)
			{
				OutPoint.transform.SetScale(InTransform.GetScale());
			}

			return bResult;
		}

		void PCGSpatialData::ProjectPoints(const std::vector<std::pair<PCGTransform, FBox>>& InSamples, const PCGProjectionParams& InParams, std::vector<PCGPoint>& OutPoints, PCGMetadata* OutMetadata) const
		{
			if (InSamples.size() != OutPoints.size()) return;

			for (int Index = 0; Index < (int)InSamples.size(); ++Index)
			{
				const std::pair<PCGTransform, FBox>& Sample = InSamples[Index];
				PCGPoint& OutPoint = OutPoints[Index];

				if (!ProjectPoint(Sample.first, Sample.second, InParams, OutPoint, OutMetadata))
				{
					OutPoint.density = 0.f;
				}
			}
		}

		bool PCGSpatialData::K2_ProjectPoint(const PCGTransform& InTransform, const FBox& InBounds, const PCGProjectionParams& InParams, PCGPoint& OutPoint, PCGMetadata* OutMetadata) const
		{
			return ProjectPoint(InTransform, InBounds, InParams, OutPoint, OutMetadata);
		}

		void PCGSpatialData::SamplePoints(const std::vector<std::pair<PCGTransform, FBox>>& InSamples, std::vector<PCGPoint>& OutPoints, PCGMetadata* OutMetadata) const
		{
			if (InSamples.size() != OutPoints.size()) return;
			for (int Index = 0; Index < (int)InSamples.size(); ++Index)
			{
				const std::pair<PCGTransform, FBox>& Sample = InSamples[Index];
				PCGPoint& OutPoint = OutPoints[Index];

				if (!SamplePoint(Sample.first, Sample.second, OutPoint, OutMetadata))
				{
					OutPoint.density = 0.f;
				}
			}
		}

		bool PCGSpatialData::K2_SamplePoint(const PCGTransform& InTransform, const FBox& InBounds, PCGPoint& OutPoint, PCGMetadata* OutMetadata) const
		{
			return SamplePoint(InTransform, InBounds, OutPoint, OutMetadata);
		}

		void PCGSpatialData::InitializeFromData(const PCGSpatialData* InSource, const PCGMetadata* InMetadataParentOverride, bool bInheritMetadata, bool bInheritAttributes)
		{
			if (InSource && TargetActor == nullptr)
			{
				TargetActor = InSource->TargetActor;
			}

			//if (!Metadata)
			//{
			//	Metadata = std::make_shared<PCGMetadata>(this);
			//}
			//
			//if (!bInheritMetadata || InMetadataParentOverride || InSource)
			//{
			//	const PCGMetadata* ParentMetadata = bInheritMetadata ? (InMetadataParentOverride ? InMetadataParentOverride : (InSource ? InSource->Metadata : nullptr)) : nullptr;
			//	Metadata->Initialize(ParentMetadata, bInheritAttributes);
			//}
			//else
			//{
			//	UE_LOG(LogPCG, Warning, TEXT("InitializeFromData has both no source and no metadata override"));
			//}
		}

		const PCGPointData* PCGSpatialDataWithPointCache::ToPointData(PCGContext* Context, const FBox& InBounds) const
		{

			if (InBounds.isFinite() && SupportsBoundedPointData())
			{
				TObjectPtr<const PCGPointData> BoundedPointData = nullptr;
				for (int CachedDataIndex = 0; CachedDataIndex < CachedBoundedPointDataBoxes.size(); ++CachedDataIndex)
				{
					if (InBounds == CachedBoundedPointDataBoxes[CachedDataIndex])
					{
						BoundedPointData = CachedBoundedPointData[CachedDataIndex];
						break;
					}
				}

				if (!BoundedPointData)
				{
					BoundedPointData = CreatePointData(Context, InBounds);
					CachedBoundedPointDataBoxes.emplace_back(InBounds);
					CachedBoundedPointData.emplace_back(BoundedPointData);
				}

				return BoundedPointData;
			}
			else
			{
				//if (!CachedPointData)
				{
					
					if (!CachedPointData)
					{
						CachedPointData = CreatePointData(Context, InBounds);
					}

				}

				return CachedPointData;
			}

		}

	}
}
