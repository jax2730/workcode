//-----------------------------------------------------------------------------
// File:   EchoPCGProperty.cpp
//
// Author: zhouguangyuan
//
// Date:   2025-8-6
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include "EchoComponentStableHeaders.h"
#include "PCG/Reflection/EchoPCGProperty.h"

#include "PCG/Reflection/EchoPCGEnum.h"
#include "PCG/Reflection/EchoPCGClass.h"
namespace Echo
{
	namespace PCGV2
	{
		DEFINE_PCG_ENUM(PCGPropertyType,
			{ "Enum", PCGPropertyType::ENUM },
			{ "Bool", PCGPropertyType::BOOL },
			{ "Int8", PCGPropertyType::INT8 },
			{ "Int16", PCGPropertyType::INT16 },
			{ "Int", PCGPropertyType::INT },
			{ "Int64", PCGPropertyType::INT64 },
			{ "UInt8", PCGPropertyType::UINT8 },
			{ "UInt16", PCGPropertyType::UINT16 },
			{ "UInt32", PCGPropertyType::UINT32 },
			{ "UInt64", PCGPropertyType::UINT64 },
			{ "Float", PCGPropertyType::FLOAT },
			{ "Double", PCGPropertyType::DOUBLE },
			{ "String", PCGPropertyType::STRING },
			{ "Name", PCGPropertyType::NAME },
			{ "FilePath", PCGPropertyType::FILEPATH },
			{ "Vector2", PCGPropertyType::VECTOR2 },
			{ "Vector3", PCGPropertyType::VECTOR3 },
			{ "Vector4", PCGPropertyType::VECTOR4 },
			{ "Struct", PCGPropertyType::STRUCT },
			{ "ObjectPtr", PCGPropertyType::OBJECT_PTR },
			{ "UniquePtr", PCGPropertyType::UNIQUE_PTR },
			{ "SharedPtr", PCGPropertyType::SHARED_PTR },
			{ "Array", PCGPropertyType::ARRAY }
		)

		const char* PCGProperty::GetDisplayName() const
		{
			auto it = m_EditorData.find(Name("DisplayName"));
			if (it != m_EditorData.end())
				return it->second.c_str();
			else
				return m_Name.c_str();
		}

		void PCGEnumProperty::SetUnderlyingProperty(PCGNumericProperty* prop)
		{
			assert(prop && "Underlying property cannot be null");
			m_UnderlyingProperty = prop;
			m_PropertySize = m_UnderlyingProperty->GetSize();
		}

		void PCGEnumProperty::SetNameValue(void* address, const Name& enumNameValue) const
		{
			try
			{
				int64 intValue = m_Enum->GetValue(enumNameValue);
				m_UnderlyingProperty->SetFromInt64(address, intValue);
			}
			catch (...)
			{
				// Enum value not found, do nothing
				return;
			}
		}

		Name PCGEnumProperty::GetNameValue(const void* address) const
		{
			int64 intValue = m_UnderlyingProperty->ToInt64(address);
			return m_Enum->GetName(intValue);
		}

		std::string MakeFileFilterString(std::string_view description, std::string_view pattern)
		{
			std::string result;
			result += description;
			result.push_back('\0');
			result += pattern;
			result.push_back('\0');
			return result;
		}

		PCGStructProperty::PCGStructProperty(PCGClass* owner, const PCGStructPropertyParams& params)
			: PCGProperty(owner, params)
			, m_StructType(params.structType)
		{
			m_Type = PCGPropertyType::STRUCT;
			m_PropertySize = m_StructType->GetStructureSize();
		}

	}
}
