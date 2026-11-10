#pragma once
#include "EchoComponentPrerequisites.h"

#include "PCG/EchoPCGSettings.h"
#include "PCG/EchoPCGElement.h"
#include "PCG/EchoPCGContext.h"

namespace Echo
{
	namespace PCGV2
	{
		class PCGDensityFilterSettings : public PCGV2::PCGSettings
		{
		public:
			PCG_SETTINGS_BODY_H(DensityFilter)

			float LowerBound = 0.5f;
			float UpperBound = 1.0f;
			bool bInvertFilter = false;
			bool bKeepZeroDensityPoints = false;
		};

		class PCGDensityFilterElement : public PCGV2::PCGElement
		{
		public:
			static PCGElement* GetInstance();
		protected:
			virtual bool ExecuteInternal(PCGContext* context) override;

			virtual PCGElementExecutionLoopMode ExecutionLoopMode(const PCGSettings* Settings) const override { return PCGElementExecutionLoopMode::SinglePrimaryPin; }
		};

	}
}

