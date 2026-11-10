#include "EchoComponentStableHeaders.h"
#include "PCG/Elements/EchoPCGInputOutputSettings.h"

namespace Echo
{
	namespace PCGV2
	{
		PCGInputOutputSettings::PCGInputOutputSettings()
			: PCGSettings()
		{

		}

		PCGElement* PCGInputOutputSettings::GetElement() const
		{
			return PCGInputOutputElement::GetInstance();
		}


		PCGSettings* PCGInputOutputSettings::Clone() const
		{
			return CloneT<PCGInputOutputSettings>();
		}

		REGISTER_PCG_CLASS(PCGInputOutputSettings)
		PCGClass* PCGInputOutputSettings::StaticClass()
		{
			BEGIN_PCG_CLASS(PCGInputOutputSettings)
				Class.AddProperty("bIsInput", PCGPropertyType::BOOL, STRUCT_OFFSET(PCGInputOutputSettings, m_bIsInput));

				PCGProperty* pinElementProp = Class.AddStructProperty("Pins_Ele", 0, PCGPinProperties::StaticClass(), PCGPropertyFlags::Transient);
				pinElementProp->SetDisplayName("Pin");

				Class.AddArrayProperty<PCGPinProperties>("Pins", STRUCT_OFFSET(PCGInputOutputSettings, m_Pins), pinElementProp);

				RegisterPCGSettings("PCGInputOutputSettings");
			END_PCG_CLASS()
		}

		void PCGInputOutputSettings::SetInput(bool bIsInput)
		{
			m_bIsInput = bIsInput;

			if (m_bHasAddedDefaultPin) return;

			m_bHasAddedDefaultPin = true;

			if (m_Pins.empty())
			{
				m_Pins.emplace_back(m_bIsInput ? PCGPinConstants::DefaultOutputLabel : PCGPinConstants::DefaultInputLabel, PCGDataType::Any);
			}
		}

		std::vector<PCGPinProperties> PCGInputOutputSettings::InputPinProperties() const
		{
			return m_Pins;
		}

		std::vector<PCGPinProperties> PCGInputOutputSettings::OutputPinProperties() const
		{
			return m_Pins;
		}

		PCGElement* PCGInputOutputElement::GetInstance()
		{
			static PCGInputOutputElement* instance = new PCGInputOutputElement();
			return instance;
		}

		PCGContext* PCGInputOutputElement::CreateContext()
		{
			return new PCGInputOutputContext();
		}

		bool PCGInputOutputElement::ExecuteInternal(PCGContext* inContext)
		{
			return true;
		}

	}
}
