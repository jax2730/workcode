#pragma once
#include "EchoComponentPrerequisites.h"

#include "PCG/EchoPCGSettings.h"
#include "PCG/EchoPCGElement.h"
#include "PCG/EchoPCGContext.h"

namespace Echo
{
	namespace PCGV2
	{
		class PCGGetLandscapeDataSettings : public PCGSettings
		{
		public:
			PCGGetLandscapeDataSettings();

			PCG_OBJECT_BODY(PCGGetLandscapeDataSettings)

			virtual PCGElement* GetElement() const override;

			virtual Name GetDefaultNodeTitle() const override { return Name("Get Landscape"); }

			virtual PCGSettings* Clone() const override;

		protected:

			virtual std::vector<PCGPinProperties> OutputPinProperties() const override;
		};

		class PCGGetLandscapeDataElement : public PCGElement
		{
		public:
			static PCGElement* GetInstance();

		protected:
			virtual PCGContext* CreateContext() override;

			bool ExecuteInternal(PCGContext* inContext) override;
		};

		struct PCGGetLandscapeDataContext : public PCGContext
		{

		};
	}
}
