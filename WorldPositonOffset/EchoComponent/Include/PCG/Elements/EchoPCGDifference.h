#pragma once
#include "EchoComponentPrerequisites.h"

#include "PCG/EchoPCGSettings.h"
#include "PCG/EchoPCGElement.h"
#include "PCG/EchoPCGContext.h"
#include "PCG/Data/EchoPCGDifferenceData.h"

namespace Echo
{
	namespace PCGV2
	{
		namespace PCGDifferenceConstants
		{
			const Name SourceLabel = Name("Source");
			const Name DifferencesLabel = Name("Differences");
		}

		class PCGDifferenceSettings : public PCGV2::PCGSettings
		{
		public:
			PCG_SETTINGS_BODY_H(Difference)

			PCGDifferenceDensityFunction DensityFunction = PCGDifferenceDensityFunction::Minimum;

			PCGDifferenceMode Mode = PCGDifferenceMode::Inferred;

			bool bDiffMetadata = true;

			bool bKeepZeroDensityPoints = false;
		};

		class PCGDifferenceElement : public PCGV2::PCGElement
		{
		public:
			static PCGElement* GetInstance();
		protected:
			virtual bool ExecuteInternal(PCGContext* context) override;
			virtual PCGElementExecutionLoopMode ExecutionLoopMode(const PCGSettings* Settings) const override { return PCGElementExecutionLoopMode::SinglePrimaryPin; }
		};
	}
}

