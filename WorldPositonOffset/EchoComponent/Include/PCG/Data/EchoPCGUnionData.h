#pragma once
#include "EchoComponentPrerequisites.h"

#include "PCG/Data/EchoPCGSpatialData.h"

namespace Echo
{
	namespace PCGV2
	{
		enum class PCGUnionType : uint8
		{
			LeftToRightPriority,
			RightToLeftPriority,
			KeepAll,
		};
		ENUM_CLASS_FLAGS(PCGUnionType)
		DECLARE_PCG_ENUM(PCGUnionType)

		enum class PCGUnionDensityFunction : uint8
		{
			Maximum,
			ClampedAddition,
			Binary
		};
		ENUM_CLASS_FLAGS(PCGUnionDensityFunction)
		DECLARE_PCG_ENUM(PCGUnionDensityFunction)

		class _EchoComponentExport PCGUnionData : public PCGSpatialDataWithPointCache
		{
			PCGDATA_BODY(PCGUnionData, PCGSpatialDataWithPointCache)
		public:
			void Initialize(const PCGSpatialData* InA, const PCGSpatialData* InB);

			void AddData(const PCGSpatialData* InData);

			void SetType(PCGUnionType InUnionType) { UnionType = InUnionType; }
			void SetDensityFunction(PCGUnionDensityFunction InDensityFunction) { DensityFunction = InDensityFunction; }

			virtual PCGDataType GetDataType() const override { return PCGDataType::Spatial; }
		public:
			virtual int GetDimension() const override;
			virtual FBox GetBounds() const override;
			virtual FBox GetStrictBounds() const override;
			virtual bool SamplePoint(const PCGTransform& Transform, const FBox& Bounds, PCGPoint& OutPoint, PCGMetadata* OutMetadata) const override;
			virtual bool HasNonTrivialTransform() const override;
			virtual const PCGSpatialData* FindFirstConcreteShapeFromNetwork() const override;
		public:
			virtual const PCGPointData* CreatePointData(PCGContext* Context) const override;

		protected:

			std::vector<TObjectPtr<const PCGSpatialData>> Data;
			TObjectPtr<const PCGSpatialData> FirstNonTrivialTransformData = nullptr;
			PCGUnionType UnionType = PCGUnionType::LeftToRightPriority;
			PCGUnionDensityFunction DensityFunction = PCGUnionDensityFunction::Maximum;
			FBox CachedBounds;
			FBox CachedStrictBounds;
			int CachedDimension = 0;
		private:
			void CreateSequentialPointData(PCGContext* Context, std::vector<TObjectPtr<const PCGSpatialData>>& DataRawPtr, std::vector<const PCGMetadata*>& InputMetadatas, PCGPointData* PointData, PCGMetadata* OutMetadata, bool bLeftToRight) const;
		};
	}
}
