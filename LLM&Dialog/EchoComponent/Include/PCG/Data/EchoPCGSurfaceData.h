#pragma once
#include "EchoComponentPrerequisites.h"

#include "PCG/Data/EchoPCGSpatialData.h"

namespace Echo
{
	namespace PCGV2
	{
		class _EchoComponentExport PCGSurfaceData : public PCGSpatialDataWithPointCache
		{
			PCGDATA_BODY(PCGSurfaceData, PCGSpatialDataWithPointCache)
		public:
			virtual PCGDataType GetDataType() const override { return PCGDataType::Surface; }

			virtual int GetDimension() const override { return 2; }
			virtual bool HasNonTrivialTransform() const override { return true; }

			const PCGTransform& GetTransform() const { return transform; }
			const FBox& GetLocalBounds() const { return LocalBounds; }

		protected:
			void CopyBaseSurfaceData(PCGSurfaceData* NewSurfaceData) const;

			PCGTransform transform;

			FBox LocalBounds = FBox::BOX_NULL;
		};
	}
}
