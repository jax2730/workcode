//-----------------------------------------------------------------------------
// File:   EchoPCGEnum.cpp
//
// Author: zhouguangyuan
//
// Date:   2025-10-22
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include "EchoComponentStableHeaders.h"
#include "PCG/Reflection/EchoPCGEnum.h"

namespace Echo
{
	namespace PCGV2
	{
		PCGEnumGlobals::PCGEnumMap& PCGEnumGlobals::GetAllEnums()
		{
			static PCGEnumMap globalEnumMap;
			return globalEnumMap;
		}

		void PCGEnumGlobals::RegisterEnum(PCGEnum* enumType)
		{
			Name name = enumType->GetName();
			PCGEnumMap& enums = GetAllEnums();
			auto it = enums.find(name);
			assert(it == enums.end() && "Enum already registered! Please check the name of enums is not duplicated and the PCG_ENUM macro is used in the .cpp file");
			if (it != enums.end())
			{
				enums[name] = enumType;
			}
		}

		PCGEnum* PCGEnumGlobals::GetEnum(const Name& name)
		{
			PCGEnumMap& enums = GetAllEnums();
			auto it = enums.find(name);
			if (it != enums.end())
				return it->second;
			return nullptr;
		}

		int64 PCGEnumGlobals::GetEnumValue(const Name& enumTypeName, const Name& valueName)
		{
			PCGEnum* enumType = GetEnum(enumTypeName);
			if (enumType)
			{
				return enumType->GetValue(valueName);
			}
			throw std::invalid_argument(std::string("Enum type not found: ") + enumTypeName.c_str());
		}

		Name PCGEnumGlobals::GetEnumName(const Name& enumTypeName, int value)
		{
			PCGEnum* enumType = GetEnum(enumTypeName);
			if (enumType)
			{
				return enumType->GetName(value);
			}
			return Name("Invalid enum value");
		}

		PCGPropertyType PCGEnum::GetUnderlyingPropertyType() const
		{
			if (m_UnderlyingTypeName == TYPE_NAME(int8_t))
				return PCGPropertyType::INT8;
			else if (m_UnderlyingTypeName == TYPE_NAME(int16_t))
				return PCGPropertyType::INT16;
			else if (m_UnderlyingTypeName == TYPE_NAME(int32_t))
				return PCGPropertyType::INT;
			else if (m_UnderlyingTypeName == TYPE_NAME(int64_t))
				return PCGPropertyType::INT64;
			else if (m_UnderlyingTypeName == TYPE_NAME(uint8_t))
				return PCGPropertyType::UINT8;
			else if (m_UnderlyingTypeName == TYPE_NAME(uint16_t))
				return PCGPropertyType::UINT16;
			else if (m_UnderlyingTypeName == TYPE_NAME(uint32_t))
				return PCGPropertyType::UINT32;
			else if (m_UnderlyingTypeName == TYPE_NAME(uint64_t))
				return PCGPropertyType::UINT64;

			assert(false && "Unsupported enum underlying type! Currently only int8, int16, int32, int64, uint8, uint16, uint32, uint64 are supported.");
			return PCGPropertyType::INT;
		}
	}
}
