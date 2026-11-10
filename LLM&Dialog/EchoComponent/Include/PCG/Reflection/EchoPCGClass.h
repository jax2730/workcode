#pragma once
//-----------------------------------------------------------------------------
// File:   EchoPCGClass.h
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
		class PCGClass
		{
		public:
			using ClassConstructorType = void (*)(const PCGObjectInitializer&);

			PCGClass(
				std::string_view name,
				ClassConstructorType constructor,
				size_t structureSize
			);

			~PCGClass();

			// Deprecated
			PCGProperty* AddProperty_Deprecated(const Name& name, PCGPropertyType type, size_t offset);

			// Generic property add function, support Bool, Numerical, String, Name, Vector2, Vector3, Vector4
			// Other property type need use their own add function
			_EchoComponentExport PCGProperty* AddProperty(std::string_view name, PCGPropertyType type, size_t offset, PCGPropertyFlags flags = PCGPropertyFlags::Defaults);

			_EchoComponentExport void AddProperty(std::string_view name, PCGProperty* prop);

			_EchoComponentExport PCGNumericProperty* AddNumericalProperty(std::string_view name, PCGPropertyType type, size_t offset, PCGPropertyFlags flags = PCGPropertyFlags::Defaults);

			_EchoComponentExport PCGEnumProperty* AddEnumProperty(std::string_view name, size_t offset, PCGEnum* enumType, PCGPropertyFlags flags = PCGPropertyFlags::Defaults);

			_EchoComponentExport PCGFilePathProperty* AddFilePathProperty(std::string_view name, size_t offset, PCGPropertyFlags flags = PCGPropertyFlags::Defaults);

			_EchoComponentExport PCGStructProperty* AddStructProperty(std::string_view name, size_t offset, PCGClass* structType, PCGPropertyFlags flags = PCGPropertyFlags::Defaults);

			_EchoComponentExport PCGObjectProperty* AddObjectProperty(std::string_view name, size_t offset, PCGPropertyFlags flags = PCGPropertyFlags::Defaults);

			template <typename T>
			PCGSharedPtrProperty* AddSharedPtrProperty(std::string_view name, size_t offset, PCGPropertyFlags flags = PCGPropertyFlags::Defaults)
			{
				PCGGenericPropertyParams params;
				params.name = Name(name.data());
				params.type = PCGPropertyType::SHARED_PTR;
				params.flags = flags;
				params.offset = offset;

				PCGSharedPtrProperty* prop = new PCGProperty_SharedPtr<T>(this, params);
				AddPropertyInternal(params.name, prop);
				return prop;
			}

			template <typename ElementType>
			PCGArrayProperty* AddArrayProperty(std::string_view name, size_t offset, PCGProperty* elementProperty, PCGPropertyFlags flags = PCGPropertyFlags::Defaults)
			{
				PCGArrayPropertyParams params;
				params.name = Name(name.data());
				params.type = PCGPropertyType::ARRAY;
				params.flags = flags;
				params.offset = offset;
				params.elementProperty = elementProperty;

				PCGArrayProperty* prop = new PCGProperty_Array<ElementType>(this, params);
				AddPropertyInternal(params.name, prop);
				return prop;
			}

			const Name& GetName() const { return m_Name; }

			const std::vector<PCGProperty*>& GetProperties() const { return m_Properties; }

			size_t GetStructureSize() const { return m_StructureSize; }

			_EchoComponentExport PCGProperty* FindProperty(std::string_view name) const;

			_EchoComponentExport PCGProperty* FindProperty(const Name& name) const;

			ClassConstructorType GetConstructor() const { return m_Constructor; }

		private:

			void AddPropertyInternal(const Name& name, PCGProperty* prop);

			Name m_Name;

			size_t m_StructureSize = 0;

			ClassConstructorType m_Constructor;

			std::vector<PCGProperty*> m_Properties;

			std::unordered_map<Name, PCGProperty*, NameIDHasher> m_PropertyLookup;
		};

		namespace PCGClassGlobals
		{
			using PCGClassMap = std::unordered_map<Name, PCGClass*, NameIDHasher>;

			_EchoComponentExport PCGClassMap& GetClassMap();

			_EchoComponentExport PCGClass* GetClass(const Name& name);

			_EchoComponentExport void RegisterPCGClass(PCGClass* pcgClass);
		}
	}
}

#define REGISTER_PCG_CLASS(TClass) \
	struct PCGClassAutoRegister_##TClass \
	{ \
		PCGClassAutoRegister_##TClass() \
		{ \
			Echo::PCGV2::PCGClass* pcgClass = TClass::StaticClass(); \
			Echo::PCGV2::PCGClassGlobals::RegisterPCGClass(pcgClass); \
		} \
	}; \
	PCGClassAutoRegister_##TClass PCGClassAutoRegisterInstance_##TClass;

#define BEGIN_PCG_CLASS(TClass) \
	static Echo::PCGV2::PCGClass singleton = []()->Echo::PCGV2::PCGClass{ \
		PCGClass Class{ \
			#TClass, \
			TClass::__DefaultConstructor, \
			sizeof(TClass) \
		};

#define END_PCG_CLASS() \
		return Class; \
	}(); \
	return &singleton;
