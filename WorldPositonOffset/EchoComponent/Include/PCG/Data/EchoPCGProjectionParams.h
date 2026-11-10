#pragma once
#include "EchoComponentPrerequisites.h"
#include "Actor/EchoActor.h"
#include "PCG/Data/EchoPCGDataCommon.h"

namespace Echo
{
	namespace PCGV2
	{

		enum class PCGProjectionColorBlendMode : uint8
		{
			SourceValue,
			TargetValue,
			Add,
			Subtract,
			Multiply
		};
		ENUM_CLASS_FLAGS(PCGProjectionColorBlendMode)
		DECLARE_PCG_ENUM(PCGProjectionColorBlendMode)

		enum class PCGProjectionTagMergeMode : uint8
		{
			Source,
			Target,
			Both
		};
		ENUM_CLASS_FLAGS(PCGProjectionTagMergeMode)
		DECLARE_PCG_ENUM(PCGProjectionTagMergeMode)

		struct _EchoComponentExport PCGProjectionParams
		{
			bool bProjectPositions = true;

			bool bProjectRotations = true;

			bool bProjectScales = false;

			PCGProjectionColorBlendMode ColorBlendMode = PCGProjectionColorBlendMode::SourceValue;

			String AttributeList;

			//PCGMetadataFilterMode AttributeMode = PCGMetadataFilterMode::ExcludeAttributes;

			//PCGMetadataOp AttributeMergeOperation = PCGMetadataOp::TargetValue;

			PCGProjectionTagMergeMode TagMergeOperation = PCGProjectionTagMergeMode::Source;
		};

	}
}
