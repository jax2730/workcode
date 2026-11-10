#include "EchoComponentStableHeaders.h"

#include "PCG/Data/EchoPCGProjectionParams.h"

namespace Echo
{
	namespace PCGV2
	{

		DEFINE_PCG_ENUM(PCGProjectionColorBlendMode,
			{ "SourceValue", PCGProjectionColorBlendMode::SourceValue },
			{ "TargetValue", PCGProjectionColorBlendMode::TargetValue },
			{ "Add", PCGProjectionColorBlendMode::Add },
			{ "Subtract", PCGProjectionColorBlendMode::Subtract },
			{ "Multiply", PCGProjectionColorBlendMode::Multiply }
		)

		DEFINE_PCG_ENUM(PCGProjectionTagMergeMode,
			{ "Source", PCGProjectionTagMergeMode::Source },
			{ "Target", PCGProjectionTagMergeMode::Target },
			{ "Both", PCGProjectionTagMergeMode::Both }
		)


	}
}
