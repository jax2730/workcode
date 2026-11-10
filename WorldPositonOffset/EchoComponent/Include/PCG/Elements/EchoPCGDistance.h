#pragma once
#include "EchoComponentPrerequisites.h"

#include "PCG/EchoPCGSettings.h"
#include "PCG/EchoPCGElement.h"
#include "PCG/EchoPCGContext.h"

namespace Echo
{
	namespace PCGV2
	{
		enum class PCGDistanceShape : uint8 {
			SphereBounds,
			BoxBounds,
			Center,
		};
		ENUM_CLASS_FLAGS(PCGDistanceShape)
		DECLARE_PCG_ENUM(PCGDistanceShape)

		namespace PCGDistance
		{
			const Name SourceLabel = Name("Source");
			const Name TargetLabel = Name("Target");
		}

		class PCGDistanceSettings : public PCGV2::PCGSettings
		{
		public:
			PCG_SETTINGS_BODY_H(Distance)

			//bool bOutputToAttribute = true;

			//Name OutputAttribute = Name::g_cEmptyName;

			//bool bOutputDistanceVector = false;

			//bool bSetDensity = true;

			float MaximumDistance = 20000.0f;

			PCGDistanceShape SourceShape = PCGDistanceShape::SphereBounds;

			PCGDistanceShape TargetShape = PCGDistanceShape::SphereBounds;

			bool bCheckSourceAgainstRespectiveTarget = false;
		};

		class PCGDistanceElement : public PCGV2::PCGElement
		{
		public:
			static PCGElement* GetInstance();
		protected:
			virtual bool ExecuteInternal(PCGContext* context) override;
			virtual PCGElementExecutionLoopMode ExecutionLoopMode(const PCGSettings* Settings) const override { return PCGElementExecutionLoopMode::SinglePrimaryPin; }
		};
	}
}

