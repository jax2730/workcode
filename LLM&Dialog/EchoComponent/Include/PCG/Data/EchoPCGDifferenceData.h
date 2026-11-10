#pragma once
#include "EchoComponentPrerequisites.h"

#include "PCG/Data/EchoPCGSpatialData.h"

namespace Echo
{
	namespace PCGV2
	{
		class PCGUnionData;

		enum class PCGDifferenceDensityFunction : uint8
		{
			Minimum,
			ClampedSubstraction,
			Binary
		};
		ENUM_CLASS_FLAGS(PCGDifferenceDensityFunction)
		DECLARE_PCG_ENUM(PCGDifferenceDensityFunction)

		enum class PCGDifferenceMode : uint8 {
			Inferred,
			Continuous,
			Discrete
		};
		ENUM_CLASS_FLAGS(PCGDifferenceMode)
		DECLARE_PCG_ENUM(PCGDifferenceMode)

		class _EchoComponentExport PCGDifferenceData : public PCGSpatialDataWithPointCache
		{
			PCGDATA_BODY(PCGDifferenceData, PCGSpatialDataWithPointCache)
		public:
			void Initialize(const PCGSpatialData* InData);

			void K2_AddDifference(const PCGSpatialData* InDifference);

			void AddDifference(const PCGSpatialData* InDifference) { AddDifference(nullptr, InDifference); }

			void AddDifference(PCGContext* InContext, const PCGSpatialData* InDifference);

			void SetDensityFunction(PCGDifferenceDensityFunction InDensityFunction);

			bool bDiffMetadata = true;

			virtual PCGDataType GetDataType() const override { return PCGDataType::Spatial; }

		public:
			virtual int GetDimension() const override;
			virtual FBox GetBounds() const override;
			virtual FBox GetStrictBounds() const override;
			virtual bool SamplePoint(const PCGTransform& Transform, const FBox& Bounds, PCGPoint& OutPoint, PCGMetadata* OutMetadata) const override;
			virtual bool HasNonTrivialTransform() const override;
			virtual const PCGSpatialData* FindFirstConcreteShapeFromNetwork() const override { return GetSource() ? GetSource()->FindFirstConcreteShapeFromNetwork() : nullptr; }
		public:
			virtual const PCGPointData* CreatePointData(PCGContext* Context) const override;
		protected:

			TObjectPtr<const PCGSpatialData> Source = nullptr;
			TObjectPtr<const PCGSpatialData> Difference = nullptr;
			TObjectPtr<PCGUnionData> DifferencesUnion = nullptr;
			PCGDifferenceDensityFunction DensityFunction = PCGDifferenceDensityFunction::Minimum;

		public:
			inline const PCGSpatialData* GetSource() const { return Source; }
			inline const PCGSpatialData* GetDifference() const { return Difference; }
			inline PCGUnionData* GetDifferencesUnion() const { return DifferencesUnion; }
		};
	}
}
