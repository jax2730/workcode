#pragma once
#include "EchoComponentPrerequisites.h"

#include "PCG/EchoPCGSettings.h"
#include "PCG/EchoPCGElement.h"
#include "PCG/EchoPCGContext.h"

namespace Echo
{
	namespace PCGV2
	{
		class PCGInputOutputSettings : public PCGSettings
		{
		public:
			PCGInputOutputSettings();

			PCG_OBJECT_BODY(PCGInputOutputSettings)

			virtual PCGElement* GetElement() const override;

			virtual Name GetDefaultNodeTitle() const override { return m_bIsInput ? Name("Input Node") : Name("Output Node"); }

			virtual PCGSettings* Clone() const override;

			void UseAsInput() { SetInput(true); }

			void UseAsOutput() { SetInput(false); }

		protected:

			void SetInput(bool bIsInput);

			virtual std::vector<PCGPinProperties> InputPinProperties() const override;

			virtual std::vector<PCGPinProperties> OutputPinProperties() const override;

			bool m_bIsInput = false;

			std::vector<PCGPinProperties> m_Pins;

			bool m_bHasAddedDefaultPin = false;
		};

		class PCGInputOutputElement : public PCGElement
		{
		public:
			static PCGElement* GetInstance();

		protected:
			virtual PCGContext* CreateContext() override;

			bool ExecuteInternal(PCGContext* inContext) override;
		};

		struct PCGInputOutputContext : public PCGContext
		{

		};
	}
}
