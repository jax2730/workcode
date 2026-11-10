#pragma once
#include "EchoComponentPrerequisites.h"

#include "PCG/EchoPCGSettings.h"
#include "PCG/EchoPCGElement.h"
#include "PCG/EchoPCGContext.h"

namespace Echo
{
	namespace PCGV2
	{
		
		namespace PCGGather
		{
			/** Gathers the input data into a single data collection and updates the tags */
			PCGDataCollection GatherDataForPin(const PCGDataCollection& InputData, const Name& InputLabel = PCGPinConstants::DefaultInputLabel, const Name& OutputLabel = PCGPinConstants::DefaultOutputLabel);
		}
	}
}

