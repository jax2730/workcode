#pragma once
#include "EchoComponentPrerequisites.h"

#include "EchoVector3.h"

#include "PCG/Data/EchoPCGSpatialData.h"
#include "EchoPCGOctree.h"

namespace Echo
{
	namespace PCGV2
	{
		typedef AxisAlignedBox BoxSphereBounds;

		struct _EchoComponentExport PCGPoint
		{
			PCGPoint() {}

			PCGPoint(const PCGTransform& inTransform, float inDensity, int32 inSeed);

			FBox GetLocalBounds() const;
			FBox GetLocalDensityBounds() const;
			void SetLocalBounds(const FBox& InBounds);
			BoxSphereBounds GetDensityBounds() const;
			BoxSphereBounds GetBounds() const;

			FVector GetExtents() const { return (boundsMax - boundsMin) / 2.0; }
			void SetExtents(const FVector& InExtents)
			{
				const FVector Center = GetLocalCenter();
				boundsMin = Center - InExtents;
				boundsMax = Center + InExtents;
			}

			FVector GetScaledExtents() const { return GetExtents() * transform.GetScale(); }

			DVector3 GetLocalCenter() const { return (boundsMax + boundsMin) / 2.0; }
			void SetLocalCenter(const DVector3& InCenter)
			{
				const DVector3 Delta = InCenter - GetLocalCenter();
				boundsMin += Delta;
				boundsMax += Delta;
			}

			FVector GetLocalSize() const { return boundsMax - boundsMin; }
			FVector GetScaledLocalSize() const { return GetLocalSize() * transform.GetScale(); }

			PCGTransform transform;

			DVector3 boundsMin = -DVector3::UNIT_SCALE;

			DVector3 boundsMax = -DVector3::UNIT_SCALE;

			float density = 1.0f;

			float steepness = 0.5f;

			int32 seed = 0;
		};

		struct _EchoComponentExport PCGPointRef
		{
			PCGPointRef(const PCGPoint& InPoint);
			PCGPointRef(const PCGPoint& InPoint, const AxisAlignedBox& InBoundsOverride);

			const PCGPoint* Point;
			BoxSphereBounds Bounds;
		};

		struct _EchoComponentExport PCGPointRefSemantics
		{
			enum { MaxElementsPerLeaf = 16 };
			enum { MinInclusiveElementsPerNode = 7 };
			enum { MaxNodeDepth = 12 };

			//typedef TInlineAllocator<MaxElementsPerLeaf> ElementAllocator;

			static const BoxSphereBounds& GetBoundingBox(const PCGPointRef& InPoint)
			{
				return InPoint.Bounds;
			}

			static const bool AreElementsEqual(const PCGPointRef& A, const PCGPointRef& B)
			{
				// TODO: verify if that's sufficient
				return A.Point == B.Point;
			}

			static void ApplyOffset(PCGPointRef& InPoint)
			{
				//ensureMsgf(false, TEXT("Not implemented"));
			}

			static void SetElementId(const PCGPointRef& Element, PCG::OctreeElementId OctreeElementID)
			{

			}
		};

		class _EchoComponentExport PCGPointData : public PCGSpatialData
		{
			PCGDATA_BODY(PCGPointData, PCGSpatialData)
		public:
			typedef PCG::Octree<PCGPointRef, PCGPointRefSemantics> PointOctree;

			virtual PCGDataType GetDataType() const override { return PCGDataType::Point; };

			virtual int GetDimension() const override { return 0; }
			virtual AxisAlignedBox GetBounds() const override;
			virtual const PCGPointData* ToPointData(PCGContext* Context, const AxisAlignedBox& InBounds = AxisAlignedBox::BOX_INFINITE) const override { return this; }
			virtual bool SamplePoint(const PCGTransform& Transform, const FBox& Bounds, PCGPoint& OutPoint, PCGMetadata* OutMetadata) const override;
			virtual bool ProjectPoint(const PCGTransform& InTransform, const FBox& InBounds, const PCGProjectionParams& InParams, PCGPoint& OutPoint, PCGMetadata* OutMetadata) const override;

		public:
			void InitializeFromActor(const ActorPtr& InActor, bool* bOutOptionalSanitizedTagAttributeName = nullptr);

			void AddSinglePointFromActor(const ActorPtr& InActor, bool* bOutOptionalSanitizedTagAttributeName = nullptr);

			const std::vector<PCGPoint>& GetPoints() const { return m_Points; }

			std::vector<PCGPoint> GetPointsCopy() const { return m_Points; }

			int GetNumPoints() const { return (int)m_Points.size(); }

			bool IsEmpty() const { return m_Points.empty(); }

			PCGPoint GetPoint(int Index) const;

			void SetPoints(const std::vector<PCGPoint>& InPoints);

			void CopyPointsFrom(const PCGPointData* InData, const std::vector<int>& InDataIndices);

			std::vector<PCGPoint>& GetMutablePoints();

			const PointOctree& GetOctree() const;

			bool ProjectPoint(const PCGTransform& InTransform, const FBox& InBounds, const PCGProjectionParams& InParams, PCGPoint& OutPoint, PCGMetadata* OutMetadata, bool bUseBounds) const;

			bool IsOctreeDirty() const { return m_bOctreeIsDirty; }
		protected:
			void RebuildOctree() const;
			void RecomputeBounds() const;
		private:
			std::vector<PCGPoint> m_Points;

			mutable PointOctree m_Octree;
			mutable AxisAlignedBox m_Bounds;
			mutable bool m_bBoundsAreDirty = true;
			mutable bool m_bOctreeIsDirty = true;
		};
	}
}

