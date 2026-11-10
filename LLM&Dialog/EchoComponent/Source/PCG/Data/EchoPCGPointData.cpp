#include "EchoComponentStableHeaders.h"

#include "PCG/Data/EchoPCGPointData.h"
#include "PCG/EchoPCGHelper.h"

namespace Echo
{
	namespace PCGV2
	{
		namespace PCGPointHelpers
		{
			bool GetDistanceRatios(const PCGPoint& InPoint, const FVector& InPosition, FVector& OutRatios)
			{
				FVector LocalPosition = InPoint.transform.InverseTransformPosition(InPosition);
				LocalPosition -= (InPoint.boundsMax + InPoint.boundsMin) / 2;
				LocalPosition /= InPoint.GetExtents();

				// ]-2+s, 2-s] is the valid range of values
				const float LowerBound = InPoint.steepness - 2;
				const float HigherBound = 2 - InPoint.steepness;

				if (LocalPosition.x <= LowerBound || LocalPosition.x > HigherBound ||
					LocalPosition.y <= LowerBound || LocalPosition.y > HigherBound ||
					LocalPosition.z <= LowerBound || LocalPosition.z > HigherBound)
				{
					return false;
				}

				// [-s, +s] is the range where the density is 1 on that axis
				const float XDist = Math::Max(0.f, Math::Abs(LocalPosition.x) - InPoint.steepness);
				const float YDist = Math::Max(0.f, Math::Abs(LocalPosition.y) - InPoint.steepness);
				const float ZDist = Math::Max(0.f, Math::Abs(LocalPosition.z) - InPoint.steepness);

				const float DistanceScale = Math::Max(2 - 2 * InPoint.steepness, 1.e-4f);

				OutRatios.x = XDist / DistanceScale;
				OutRatios.y = YDist / DistanceScale;
				OutRatios.z = ZDist / DistanceScale;
				return true;
			}

			float ManhattanDensity(const PCGPoint& InPoint, const FVector& InPosition)
			{
				FVector Ratios;
				if (GetDistanceRatios(InPoint, InPosition, Ratios))
				{
					return InPoint.density * (1.f - Ratios.x) * (1.f - Ratios.y) * (1.f - Ratios.z);
				}
				else
				{
					return 0.f;
				}
			}

			float InverseEuclidianDistance(const PCGPoint& InPoint, const FVector& InPosition)
			{
				FVector Ratios;
				if (GetDistanceRatios(InPoint, InPosition, Ratios))
				{
					return 1.0f - Ratios.Length();
				}
				else
				{
					return 0.f;
				}
			}

			float ComputeOverlapRatio(const FBox& Numerator, const FBox& Denominator)
			{
				const FVector NumeratorExtent = Numerator.GetHalf();
				const FVector DenominatorExtent = Denominator.GetHalf();

				return (float)((DenominatorExtent.x > 0.f ? NumeratorExtent.x / DenominatorExtent.x : 1.0f) *
					(DenominatorExtent.y > 0.f ? NumeratorExtent.y / DenominatorExtent.y : 1.0f) *
					(DenominatorExtent.z > 0.f ? NumeratorExtent.z / DenominatorExtent.z : 1.0f));
			}

			float VolumeOverlap(const PCGPoint& InPoint, const FBox& InBounds, const PCGTransform& InTransform)
			{
				const FBox PointBounds = InPoint.GetLocalDensityBounds();

				PCGTransform PointTransformToInTransform = InTransform.InverseTransform(InPoint.transform);
				const FBox PointBoundsTransformed = PointTransformToInTransform.Transform(PointBounds);

				const FBox FirstOverlap = InBounds.intersection(PointBoundsTransformed);
				if (!FirstOverlap.isFinite())
				{
					return 0;
				}

				const FBox InBoundsTransformed = PointTransformToInTransform.InverseTransform(InBounds);

				const FBox SecondOverlap = InBoundsTransformed.intersection(PointBounds);
				if (!SecondOverlap.isFinite())
				{
					return 0;
				}

				return Math::Min(ComputeOverlapRatio(FirstOverlap, InBounds), ComputeOverlapRatio(SecondOverlap, InBoundsTransformed));
			}

			Quaternion AddQuatWithWeight(const Quaternion& Q, const Quaternion& V, float Weight)
			{
				Quaternion BlendQuat = V * Weight;

				if (Q.Dot(BlendQuat) >= 0.0f)
					return Q + BlendQuat;
				else
					return Q - BlendQuat;
			}

		}


		PCGPoint::PCGPoint(const PCGTransform& inTransform, float inDensity, int32 inSeed)
			: transform(inTransform)
			, density(inDensity)
			, seed(inSeed)
		{
		}

		FBox PCGPoint::GetLocalBounds() const
		{
			return { boundsMin,boundsMax };
		}
		FBox PCGPoint::GetLocalDensityBounds() const
		{
			return { (2.f - steepness) * boundsMin,(2.f - steepness) * boundsMax };
		}
		void PCGPoint::SetLocalBounds(const FBox& InBounds)
		{
			boundsMin = InBounds.getMinimum();
			boundsMax = InBounds.getMaximum();
		}
		BoxSphereBounds PCGPoint::GetDensityBounds() const
		{
			return { transform.Transform(GetLocalDensityBounds()) };
		}

		BoxSphereBounds PCGPoint::GetBounds() const
		{
			return { transform.Transform(GetLocalBounds()) };
		}

		PCGPointRef::PCGPointRef(const PCGPoint& InPoint)
		{
			Point = &InPoint;
			Bounds = InPoint.GetDensityBounds();
		}
		PCGPointRef::PCGPointRef(const PCGPoint& InPoint, const AxisAlignedBox& InBoundsOverride)
		{
			Point = &InPoint;
			Bounds = BoxSphereBounds(InPoint.transform.Transform(InBoundsOverride));
		}

		AxisAlignedBox PCGPointData::GetBounds() const
		{
			if (m_bBoundsAreDirty) {
				RecomputeBounds();
			}
			return m_Bounds;
		}

		const PCGPointData::PointOctree& PCGPointData::GetOctree() const
		{
			if (m_bOctreeIsDirty)
			{
				RebuildOctree();
			}

			return m_Octree;
		}

		bool PCGPointData::SamplePoint(const PCGTransform& InTransform, const FBox& InBounds, PCGPoint& OutPoint, PCGMetadata* OutMetadata) const
		{
			PCGProjectionParams Params{};
			Params.bProjectPositions = Params.bProjectRotations = Params.bProjectScales = false;
			Params.ColorBlendMode = PCGProjectionColorBlendMode::SourceValue;

			return ProjectPoint(InTransform, InBounds, Params, OutPoint, OutMetadata);
		}

		bool PCGPointData::ProjectPoint(const PCGTransform& InTransform, const FBox& InBounds, const PCGProjectionParams& InParams, PCGPoint& OutPoint, PCGMetadata* OutMetadata) const
		{
			return ProjectPoint(InTransform, InBounds, InParams, OutPoint, OutMetadata, true);
		}

		bool PCGPointData::ProjectPoint(const PCGTransform& InTransform, const FBox& InBounds, const PCGProjectionParams& InParams, PCGPoint& OutPoint, PCGMetadata* OutMetadata, bool bUseBounds) const
		{
			if (m_bOctreeIsDirty)
			{
				RebuildOctree();
			}

			std::vector<std::pair<const PCGPoint*, float>> Contributions;
			const bool bSampleInVolume = !InBounds.isNull();

			if (!bSampleInVolume)
			{
				const Vector3 InPosition = InTransform.GetLocation();
				m_Octree.FindFirstElementWithBoundsTest(PCG::BoxCenterAndExtent(InPosition, Vector3::ZERO), [&InPosition, &Contributions](const PCGPointRef& InPointRef) {
					Contributions.emplace_back(std::make_pair(InPointRef.Point, PCGPointHelpers::InverseEuclidianDistance(*InPointRef.Point, InPosition)));
					return true;
				});
			}
			else
			{
				FBox TransformedBounds = InTransform.Transform(InBounds);
				
				m_Octree.FindFirstElementWithBoundsTest(PCG::BoxCenterAndExtent(TransformedBounds), [bUseBounds, &InBounds, &InTransform, &Contributions](const PCGPointRef& InPointRef) {
					const float Contribution = bUseBounds ? PCGPointHelpers::VolumeOverlap(*InPointRef.Point, InBounds, InTransform) : 1.0f;
					if (Contribution > 0.0f) {
						Contributions.emplace_back(std::make_pair(InPointRef.Point, Contribution));
					}
					return true;
				});
			}

			float SumContributions = 0.f;
			float MaxContribution = 0.f;
			const PCGPoint* MaxContributor = nullptr;

			for (const std::pair<const PCGPoint*, float>& Contribution : Contributions)
			{
				SumContributions += Contribution.second;
				if (Contribution.second > MaxContribution) {
					MaxContribution = Contribution.second;
					MaxContributor = Contribution.first;
				}
			}

			if (SumContributions <= 0.0f) {
				return false;
			}

			const float DensityNormalizationFactor = ((SumContributions > 1.0f) ? (1.0f / SumContributions) : 1.0f);

			std::vector<std::pair<const PCGPoint*, float>> ContributionsForMetadata;

			DVector3 WeightedPosition = DVector3::ZERO;
			Quaternion WeightedQuat = Quaternion(0.0f, 0.0f, 0.0f, 0.0f);
			Vector3 WeightedScale = Vector3::ZERO;
			float WeightedDensity = 0.f;
			Vector3 WeightedBoundsMin = Vector3::ZERO;
			Vector3 WeightedBoundsMax = Vector3::ZERO;
			//Vector4 WeightedColor = Vector4::ZERO;
			float WeightedSteepness = 0.f;

			for (const std::pair<const PCGPoint*, float>& Contribution : Contributions)
			{
				const PCGPoint& SourcePoint = *Contribution.first;
				const float Weight = Contribution.second / SumContributions;

				WeightedPosition += SourcePoint.transform.GetLocation() * Weight;
				WeightedQuat = PCGPointHelpers::AddQuatWithWeight(WeightedQuat, SourcePoint.transform.GetRotation(), Weight);
				WeightedScale += SourcePoint.transform.GetScale() * Weight;

				if (!bSampleInVolume)
				{
					WeightedDensity += PCGPointHelpers::ManhattanDensity(SourcePoint, InTransform.GetLocation());
				}
				else
				{
					WeightedDensity += SourcePoint.density * (bUseBounds ? (Contribution.second * DensityNormalizationFactor) : Weight);
				}

				WeightedBoundsMin += SourcePoint.boundsMin * Weight;
				WeightedBoundsMax += SourcePoint.boundsMax * Weight;
				//WeightedColor += SourcePoint.color * Weight;
				WeightedSteepness += SourcePoint.steepness * Weight;

				ContributionsForMetadata.emplace_back(std::make_pair(Contribution.first, static_cast<float>(Weight)));
			}

			if (InParams.bProjectPositions)
			{
				OutPoint.transform.SetLocation(bSampleInVolume ? WeightedPosition : InTransform.GetLocation());
			}
			else
			{
				OutPoint.transform.SetLocation(InTransform.GetLocation());
			}

			if (InParams.bProjectRotations)
			{
				WeightedQuat.Normalize();
				OutPoint.transform.SetRotation(WeightedQuat);
			}
			else
			{
				OutPoint.transform.SetRotation(InTransform.GetRotation());
			}

			if (InParams.bProjectScales)
			{
				OutPoint.transform.SetScale(WeightedScale);
			}
			else
			{
				OutPoint.transform.SetScale(InTransform.GetScale());
			}

			OutPoint.density = static_cast<float>(WeightedDensity);
			OutPoint.boundsMin = WeightedBoundsMin;
			OutPoint.boundsMax = WeightedBoundsMax;
			//OutPoint.Color = WeightedColor;
			OutPoint.steepness = WeightedSteepness;

			//if (OutMetadata)
			//{
			//	//TRACE_CPUPROFILER_EVENT_SCOPE(UPCGPointData::SamplePoint::SetupMetadata);
			//	// Initialise metadata entry for this temporary point
			//	PCGMetadataAccessorHelpers::InitializeMetadataWithParent(OutPoint, OutMetadata, *MaxContributor, Metadata);
			//
			//	if (ContributionsForMetadata.Num() > 1)
			//	{
			//		OutMetadata->ComputePointWeightedAttribute(OutPoint, MakeArrayView(ContributionsForMetadata), Metadata);
			//	}
			//}

			return true;

		}

		void PCGPointData::InitializeFromActor(const ActorPtr& InActor, bool* bOutOptionalSanitizedTagAttributeName)
		{
			if (Metadata == nullptr) return;
			//assert(Metadata && Metadata->GetAttributeCount() == 0);

			AddSinglePointFromActor(InActor, bOutOptionalSanitizedTagAttributeName);
		}

		void PCGPointData::AddSinglePointFromActor(const ActorPtr& InActor, bool* bOutOptionalSanitizedTagAttributeName)
		{
			if (InActor == nullptr) return;

			PCGPoint& Point = GetMutablePoints().emplace_back();
			Point.steepness = 1.0f;
			Point.transform.translation = InActor->getWorldPosition();
			Point.transform.rotation = InActor->getWorldRotation();
			Point.transform.scale = InActor->getWorldScale();

			const DVector3& Position = Point.transform.GetLocation();
			Point.seed = PCGHelpers::ComputeSeed((int)Position.x, (int)Position.y, (int)Position.z);

			const FBox LocalBounds = PCGHelpers::GetActorLocalBounds(InActor);
			Point.boundsMin = LocalBounds.getMinimum();
			Point.boundsMax = LocalBounds.getMaximum();

			//Point.MetadataEntry = Metadata->AddEntry();
			//
			//FPCGMetadataAttribute<FSoftObjectPath>* ActorReferenceAttribute = Metadata->FindOrCreateAttribute(PCGPointDataConstants::ActorReferenceAttribute, FSoftObjectPath(), /*bAllowsInterpolation=*/false, /*bOverrideParent=*/false, /*bOverwriteIfTypeMismatch=*/false);
			//if (ActorReferenceAttribute)
			//{
			//	ActorReferenceAttribute->SetValue(Point.MetadataEntry, FSoftObjectPath(InActor));
			//}

			bool bSanitizedAttributeNames = false;

			//// Parse tags as well
			//for (FName Tag : InActor->tags)
			//{
			//	PCG::Private::FParseTagResult TagData(Tag);
			//	if (PCG::Private::SetAttributeFromTag(TagData, Metadata, Point.MetadataEntry, /*bCanCreateAttribute=*/true))
			//	{
			//		bSanitizedAttributeNames |= TagData.HasBeenSanitized();
			//	}
			//}

			if (bOutOptionalSanitizedTagAttributeName)
			{
				*bOutOptionalSanitizedTagAttributeName = bSanitizedAttributeNames;
			}
		}


		void PCGPointData::RebuildOctree() const
		{
			//FScopeLock Lock(&CachedDataLock);

			if (!m_bOctreeIsDirty)
			{
				return;
			}


			FBox PointBounds = GetBounds();
			FVector center = PointBounds.getCenter();
			FVector extent = PointBounds.getHalfSize();
			float octreeExtent = extent[extent.MainAxis() - 1];
			PointOctree NewOctree(center, octreeExtent);

			for (const PCGPoint& Point : m_Points)
			{
				NewOctree.AddElement(PCGPointRef(Point));
			}

			m_Octree = std::move(NewOctree);
			m_bOctreeIsDirty = false;
		}
		void PCGPointData::RecomputeBounds() const
		{
			//FScopeLock Lock(&CachedDataLock);

			if (!m_bBoundsAreDirty)
			{
				return;
			}

			FBox NewBounds = FBox::BOX_NULL;
			for (const PCGPoint& Point : m_Points)
			{
				BoxSphereBounds PointBounds = Point.GetDensityBounds();
				NewBounds.merge(PointBounds);
			}

			m_Bounds = NewBounds;
			m_bBoundsAreDirty = false;
		}

		PCGPoint PCGPointData::GetPoint(int index) const
		{
			if (index >= 0 && index < m_Points.size())
			{
				return m_Points[index];
			}
			else
			{
				return PCGPoint{};
			}
		}

		void PCGPointData::SetPoints(const std::vector<PCGPoint>& InPoints)
		{
			GetMutablePoints() = InPoints;
		}

		void PCGPointData::CopyPointsFrom(const PCGPointData* InData, const std::vector<int>& InDataIndices)
		{
			if (InData == nullptr) return;
			
			m_Points.resize(InDataIndices.size());

			// TODO: parallel-for this?
			for (int PointIndex = 0; PointIndex < (int)InDataIndices.size(); ++PointIndex)
			{
				m_Points[PointIndex] = InData->m_Points[InDataIndices[PointIndex]];
			}

			m_bBoundsAreDirty = true;
			m_bOctreeIsDirty = true;
		}

		std::vector<PCGPoint>& PCGPointData::GetMutablePoints()
		{
			m_bOctreeIsDirty = true;
			m_bBoundsAreDirty = true;
			return m_Points;
		}
	}
}
