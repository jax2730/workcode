#pragma once
//-----------------------------------------------------------------------------
// File:   EchoPCGEnum.h
//
// Author: zhouguangyuan
//
// Date:   2025-10-22
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include "EchoComponentPrerequisites.h"

#include "PCG/Reflection/EchoPCGProperty.h"

namespace Echo
{
	namespace PCGV2
	{
		class PCGEnum
		{
		public:
			PCGEnum(const Name& name) : m_Name(name) {}
			~PCGEnum() = default;

			void AddValue(const Name& name, int64 value, const Name& displayName = Name())
			{
				m_Items.push_back(std::make_pair(name, value));
				if (!displayName.empty())
				{
					m_DisplayNames[value] = displayName;
				}
			}

			int64 GetValue(const Name& name) const
			{
				for (const auto& pair : m_Items)
				{
					if (pair.first == name)
						return pair.second;
				}
				throw std::runtime_error("Enum value not found");
			}

			const Name& GetName() const { return m_Name; }

			Name GetName(int64 value) const
			{
				for (const auto& pair : m_Items)
				{
					if (pair.second == value)
						return pair.first;
				}
				throw std::runtime_error("Enum value not found");
			}

			void SetUnderlyingType(const Name& cppType) { m_UnderlyingTypeName = cppType; }

			Name GetUnderlyingType() const { return m_UnderlyingTypeName; }

			_EchoComponentExport PCGPropertyType GetUnderlyingPropertyType() const;

			const std::vector<std::pair<Name, int64>>& GetAllItems() const { return m_Items; }

			Name GetDisplayName(int64 value) const
			{
				auto it = m_DisplayNames.find(value);
				if (it != m_DisplayNames.end())
					return it->second;
				return GetName(value);
			}

		private:
			Name m_Name;
			Name m_UnderlyingTypeName;
			std::vector<std::pair<Name, int64>> m_Items;
			std::unordered_map<int64, Name> m_DisplayNames;
		};

		namespace PCGEnumGlobals
		{
			using PCGEnumMap = std::unordered_map<Name, PCGEnum*, NameIDHasher>;

			_EchoComponentExport PCGEnumMap& GetAllEnums();

			_EchoComponentExport void RegisterEnum(PCGEnum* enumType);

			_EchoComponentExport PCGEnum* GetEnum(const Name& name);

			_EchoComponentExport int64 GetEnumValue(const Name& enumTypeName, const Name& valueName);

			_EchoComponentExport Name GetEnumName(const Name& enumTypeName, int value);
		}

		template<typename EnumType>
		struct PCGEnumItemParams
		{
			Name itemName;
			int64 intValue;
			Name displayName;

			PCGEnumItemParams(std::string_view name, EnumType enumValue, std::string_view displayName = "")
				: itemName(name.data()), intValue(static_cast<int64>(enumValue)), displayName(displayName.data())
			{
			}
		};
	}
}

#define REGISTER_PCG_ENUM_INTERNAL(EnumType) \
	struct PCGEnumAutoRegister_##EnumType \
	{ \
		PCGEnumAutoRegister_##EnumType() \
		{ \
			Echo::PCGV2::PCGEnum* enumType = Z_Construct_PCGEnum_##EnumType(); \
			Echo::PCGV2::PCGEnumGlobals::RegisterEnum(enumType); \
		} \
	}; \
	PCGEnumAutoRegister_##EnumType PCGEnumAutoRegisterInstance_##EnumType; \

#define DEFINE_PCG_STATIC_ENUM_INTERNAL(EnumType) \
	template<> \
	Echo::PCGV2::PCGEnum* StaticEnum<EnumType>() \
	{ \
		return Z_Construct_PCGEnum_##EnumType(); \
	}

// Use DECLARE_PCG_ENUM in header file and DEFINE_PCG_ENUM in source file
#define DEFINE_PCG_ENUM(EnumType, ...) \
	Echo::PCGV2::PCGEnum* Z_Construct_PCGEnum_##EnumType() \
	{ \
		static PCGEnum* singleton = NULL; \
		if (!singleton) \
		{ \
			Echo::PCGV2::PCGEnum* enumType = new Echo::PCGV2::PCGEnum(Name(#EnumType)); \
			enumType->SetUnderlyingType(ENUM_UNDERLYING_TYPE_NAME(EnumType)); \
			Echo::PCGV2::PCGEnumItemParams<EnumType> elements[] = {__VA_ARGS__}; \
			for (const auto& element : elements) \
			{ \
				enumType->AddValue(element.itemName, element.intValue, element.displayName); \
			} \
			singleton = enumType; \
		} \
		return singleton; \
	} \
	REGISTER_PCG_ENUM_INTERNAL(EnumType) \
	DEFINE_PCG_STATIC_ENUM_INTERNAL(EnumType)

