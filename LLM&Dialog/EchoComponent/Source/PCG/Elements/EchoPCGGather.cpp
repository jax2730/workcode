#include "EchoComponentStableHeaders.h"
#include "PCG/Elements/EchoPCGGather.h"
#include "PCG/EchoPCGHelper.h"
#include "PCG/Data/EchoPCGSpatialData.h"
#include "PCG/Data/EchoPCGPointData.h"

namespace Echo
{
	namespace PCGV2
	{

		namespace PCGGather
		{
			PCGDataCollection GatherDataForPin(const PCGDataCollection& InputData, const Name& InputLabel, const Name& OutputLabel)
			{
				std::vector<PCGTaggedData> GatheredData = InputData.GetInputsByPin(InputLabel);
				PCGDataCollection Output;

				if (GatheredData.empty())
				{
					return Output;
				}

				if (GatheredData.size() == InputData.taggedDataList.size())
				{
					Output = InputData;
				}
				else
				{
					Output.taggedDataList = std::move(GatheredData);
				}

				for (PCGTaggedData& TaggedData : Output.taggedDataList)
				{
					TaggedData.pinLabel = OutputLabel;
				}

				return Output;
			}
		}

	}
}

