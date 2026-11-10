#include "EchoComponentStableHeaders.h"

#include "PCG/Data/EchoPCGSurfaceData.h"

namespace Echo
{
	namespace PCGV2
	{
		void PCGSurfaceData::CopyBaseSurfaceData(PCGSurfaceData* NewSurfaceData) const
		{
			NewSurfaceData->transform = transform;
			NewSurfaceData->bKeepZeroDensityPoints = bKeepZeroDensityPoints;
		}
	}
}
