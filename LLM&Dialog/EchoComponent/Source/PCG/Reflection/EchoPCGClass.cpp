//-----------------------------------------------------------------------------
// File:   EchoPCGClass.cpp
//
// Author: zhouguangyuan
//
// Date:   2025-10-22
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include "EchoComponentStableHeaders.h"
#include "PCG/Reflection/EchoPCGClass.h"

#include "PCG/Reflection/EchoPCGEnum.h"

namespace Echo
{
	namespace PCGV2
	{
		PCGClass::PCGClass(
			std::string_view name,
			ClassConstructorType constructor,
			size_t structureSize)
			: m_Name(Name(name.data()))
			, m_StructureSize(structureSize)
			, m_Constructor(constructor)
		{

		}

		PCGClass::~PCGClass()
		{
			for (PCGProperty* prop : m_Properties)
			{
				delete prop;
			}
			m_Properties.clear();
		}

		PCGProperty* PCGClass::AddProperty_Deprecated(const Name& name, PCGPropertyType type, size_t offset)
		{
			PCGGenericPropertyParams params;
			params.name = name;
			params.type = type;
			params.offset = offset;

			PCGProperty* prop = new PCGProperty(this, params);
			AddPropertyInternal(name, prop);
			return prop;
		}

		PCGProperty* PCGClass::FindProperty(std::string_view name) const
		{
			return FindProperty(Name(name.data()));
		}

		PCGProperty* PCGClass::FindProperty(const Name& name) const
		{
			auto iter = m_PropertyLookup.find(name);
			if (iter != m_PropertyLookup.end())
			{
				return iter->second;
			}
			return nullptr;
		}

		PCGProperty* PCGClass::AddProperty(std::string_view name, PCGPropertyType type, size_t offset, PCGPropertyFlags flags/* = PCGPropertyFlags::Defaults*/)
		{
			PCGProperty* Result = nullptr;

			switch (type)
			{
			case PCGPropertyType::INT8:
			case PCGPropertyType::INT16:
			case PCGPropertyType::INT:
			case PCGPropertyType::INT64:
			case PCGPropertyType::UINT8:
			case PCGPropertyType::UINT16:
			case PCGPropertyType::UINT64:
			case PCGPropertyType::FLOAT:
			case PCGPropertyType::DOUBLE:
				Result = AddNumericalProperty(name, type, offset, flags);
				break;
			case PCGPropertyType::BOOL:
			case PCGPropertyType::STRING:
			case PCGPropertyType::NAME:
			case PCGPropertyType::VECTOR2:
			case PCGPropertyType::VECTOR3:
			case PCGPropertyType::VECTOR4:
				Result = AddProperty_Deprecated(Name(name.data()), type, offset);
				break;
			default:
				assert(false && "Unsupported property type! Please use specific AddProperty function instead, like AddEnumProperty, AddFilePathProperty, etc.");
				break;
			}

			return Result;
		}

		void PCGClass::AddProperty(std::string_view name, PCGProperty* prop)
		{
			AddPropertyInternal(Name(name.data()), prop);
		}

		_EchoComponentExport PCGNumericProperty* PCGClass::AddNumericalProperty(std::string_view name, PCGPropertyType type, size_t offset, PCGPropertyFlags flags /*= PCGPropertyFlags::Defaults*/)
		{
			PCGGenericPropertyParams params;
			params.name = Name(name.data());
			params.type = type;
			params.flags = flags;
			params.offset = offset;

			PCGNumericProperty* prop = nullptr;
			switch (type)
			{
			case Echo::PCGV2::PCGPropertyType::INT8:
				prop = new PCGInt8Property(this, params);
				break;
			case Echo::PCGV2::PCGPropertyType::INT16:
				prop = new PCGInt16Property(this, params);
				break;
			case Echo::PCGV2::PCGPropertyType::INT:
				prop = new PCGInt32Property(this, params);
				break;
			case Echo::PCGV2::PCGPropertyType::INT64:
				prop = new PCGInt64Property(this, params);
				break;
			case Echo::PCGV2::PCGPropertyType::UINT8:
				prop = new PCGByteProperty(this, params);
				break;
			case Echo::PCGV2::PCGPropertyType::UINT16:
				prop = new PCGUInt16Property(this, params);
				break;
			case Echo::PCGV2::PCGPropertyType::UINT32:
				prop = new PCGUInt32Property(this, params);
				break;
			case Echo::PCGV2::PCGPropertyType::UINT64:
				prop = new PCGUInt64Property(this, params);
				break;
			case Echo::PCGV2::PCGPropertyType::FLOAT:
				prop = new PCGFloatProperty(this, params);
				break;
			case Echo::PCGV2::PCGPropertyType::DOUBLE:
				prop = new PCGDoubleProperty(this, params);
				break;
			default:
				assert(false && "Unsupported numerical property type!");
				break;
			}
			if (prop)
			{
				AddPropertyInternal(params.name, prop);
			}
			return prop;
		}

		PCGEnumProperty* PCGClass::AddEnumProperty(std::string_view name, size_t offset, PCGEnum* enumType, PCGPropertyFlags flags /*= PCGPropertyFlags::Defaults*/)
		{
			PCGEnumPropertyParams params;
			params.name = Name(name.data());
			params.type = PCGPropertyType::ENUM;
			params.flags = flags;
			params.offset = offset;
			params.enumType = enumType;

			PCGEnumProperty* prop = new PCGEnumProperty(this, params);

			// Add underlying numerical property
			PCGPropertyType underlyingType = enumType->GetUnderlyingPropertyType();
			PCGNumericProperty* underlyingProperty = AddNumericalProperty("", underlyingType, offset, PCGPropertyFlags::Transient);

			prop->SetUnderlyingProperty(underlyingProperty);

			AddPropertyInternal(params.name, prop);
			return prop;
		}

		PCGFilePathProperty* PCGClass::AddFilePathProperty(std::string_view name, size_t offset, PCGPropertyFlags flags /*= PCGPropertyFlags::Defaults*/)
		{
			PCGGenericPropertyParams params;
			params.name = Name(name.data());
			params.flags = flags;
			params.offset = offset;

			PCGFilePathProperty* prop = new PCGFilePathProperty(this, params);
			AddPropertyInternal(params.name, prop);
			return prop;
		}

		PCGStructProperty* PCGClass::AddStructProperty(std::string_view name, size_t offset, PCGClass* structType, PCGPropertyFlags flags /*= PCGPropertyFlags::Defaults*/)
		{
			PCGStructPropertyParams params;
			params.name = Name(name.data());
			params.type = PCGPropertyType::STRUCT;
			params.flags = flags;
			params.offset = offset;
			params.structType = structType;

			PCGStructProperty* prop = new PCGStructProperty(this, params);
			AddPropertyInternal(params.name, prop);
			return prop;
		}

		PCGObjectProperty* PCGClass::AddObjectProperty(std::string_view name, size_t offset, PCGPropertyFlags flags /*= PCGPropertyFlags::Defaults*/)
		{
			PCGGenericPropertyParams params;
			params.name = Name(name.data());
			params.type = PCGPropertyType::OBJECT_PTR;
			params.flags = flags;
			params.offset = offset;

			PCGObjectProperty* prop = new PCGObjectProperty(this, params);
			AddPropertyInternal(params.name, prop);
			return prop;
		}

		void PCGClass::AddPropertyInternal(const Name& name, PCGProperty* prop)
		{
			m_Properties.push_back(prop);
			m_PropertyLookup.emplace(name, prop);
		}

		PCGClassGlobals::PCGClassMap& PCGClassGlobals::GetClassMap()
		{
			static PCGClassMap classMap;
			return classMap;
		}

		PCGClass* PCGClassGlobals::GetClass(const Name& name)
		{
			PCGClassMap& classMap = GetClassMap();
			auto it = classMap.find(name);
			if (it != classMap.end())
				return it->second;
			return nullptr;
		}

		void PCGClassGlobals::RegisterPCGClass(PCGClass* pcgClass)
		{
			PCGClassMap& classMap = GetClassMap();
			auto it = classMap.find(pcgClass->GetName());
			assert(it == classMap.end() && "PCGClass already registered! Please check the name of PCGClass is not duplicated and the REGISTER_PCG_CLASS macro is used in the .cpp file");
			if (it == classMap.end())
			{
				classMap[pcgClass->GetName()] = pcgClass;
			}
		}


	}
}
