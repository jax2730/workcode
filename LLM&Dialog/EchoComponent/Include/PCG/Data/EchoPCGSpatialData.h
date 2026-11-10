#pragma once
#include "EchoComponentPrerequisites.h"
#include "Actor/EchoActor.h"
#include "PCG/Data/EchoPCGDataCommon.h"
#include "PCG/Data/EchoPCGProjectionParams.h"

namespace Echo
{
	namespace PCGV2
	{

		class _EchoComponentExport PCGSpatialData : public PCGData
		{
			PCGDATA_BODY(PCGSpatialData, PCGData)
		public:
			virtual PCGDataType GetDataType() const override { return PCGDataType::Spatial; }

			virtual int GetDimension() const { return 0; }

			virtual AxisAlignedBox GetBounds() const { return AxisAlignedBox::BOX_NULL; }

			virtual bool IsBounded() const { return true; }

			virtual AxisAlignedBox GetStrictBounds() const { return AxisAlignedBox::BOX_NULL; }

			virtual Vector3 GetNormal() const { return Vector3::UNIT_Y; }

			virtual float GetDensityAtPosition(const DVector3& InPosition) const;

			const PCGPointData* ToPointData() const { return ToPointData(nullptr); }

			const PCGPointData* ToPointDataWithContext(PCGContext& Context) const { return ToPointData(&Context); }

			virtual const PCGPointData* ToPointData(PCGContext* Context, const AxisAlignedBox& InBounds = AxisAlignedBox::BOX_INFINITE) const { return nullptr; }

			virtual bool SamplePoint(const PCGTransform& Transform, const FBox& Bounds, PCGPoint& OutPoint, PCGMetadata* OutMetadata) const { return false; }

			virtual void SamplePoints(const std::vector<std::pair<PCGTransform, FBox>>& Samples, std::vector<PCGPoint>& OutPoints, PCGMetadata* OutMetadata) const;

			bool K2_SamplePoint(const PCGTransform& Transform, const FBox& Bounds, PCGPoint& OutPoint, PCGMetadata* OutMetadata) const;

			virtual bool ProjectPoint(const PCGTransform& inTransform, const AxisAlignedBox& inBounds, const PCGProjectionParams& inParams, PCGPoint& outPoint, PCGMetadata* OutMetadata) const;

			virtual void ProjectPoints(const std::vector<std::pair<PCGTransform, FBox>>& Samples, const PCGProjectionParams& InParams, std::vector<PCGPoint>& OutPoints, PCGMetadata* OutMetadata) const;

			bool K2_ProjectPoint(const PCGTransform& InTransform, const FBox& InBounds, const PCGProjectionParams& InParams, PCGPoint& OutPoint, PCGMetadata* OutMetadata) const;

			virtual bool HasNonTrivialTransform() const { return false; }

			void InitializeFromData(const PCGSpatialData* InSource, const PCGMetadata* InMetadataParentOverride = nullptr, bool bInheritMetadata = true, bool bInheritAttributes = true);

			virtual bool RequiresCollapseToSample() const { return false; }

			virtual const PCGSpatialData* FindFirstConcreteShapeFromNetwork() const { return !!(GetDataType() & PCGDataType::Concrete) ? this : nullptr; }
			
			ActorPtr TargetActor = nullptr;

			bool bKeepZeroDensityPoints = false;

			TObjectPtr<PCGMetadata> Metadata = nullptr;
		private:
		};

		class _EchoComponentExport PCGSpatialDataWithPointCache : public PCGSpatialData
		{
			PCGDATA_BODY(PCGSpatialDataWithPointCache, PCGSpatialData)
		public:
			virtual const PCGPointData* ToPointData(PCGContext* Context, const FBox& InBounds = FBox::BOX_NULL) const override;
		protected:
			virtual bool SupportsBoundedPointData() const { return false; }
			virtual const PCGPointData* CreatePointData(PCGContext* Context) const { return nullptr; }
			virtual const PCGPointData* CreatePointData(PCGContext* Context, const FBox& InBounds) const { return CreatePointData(Context); }

		private:
			mutable TObjectPtr<const PCGPointData> CachedPointData;
			mutable std::vector<FBox> CachedBoundedPointDataBoxes;
			mutable std::vector<TObjectPtr<const PCGPointData>> CachedBoundedPointData;
			//mutable FCriticalSection CacheLock;
		};
	}
}
