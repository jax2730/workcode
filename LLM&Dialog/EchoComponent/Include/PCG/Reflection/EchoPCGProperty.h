#pragma once
//-----------------------------------------------------------------------------
// File:   EchoPCGProperty.h
//
// Author: zhouguangyuan
//
// Date:   2025-8-6
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#include "EchoComponentPrerequisites.h"

#include "PCG/Reflection/EchoPCGReflectionMacros.h"

namespace Echo
{
	namespace PCGV2
	{
		enum class PCGPropertyFlags : uint64
		{
			None = 0,
			Edit = 1 << 0,  // Property is editable
			Transient = 1 << 1,  // Property is transient, won't be saved and not visible in editor
			ReadOnly = 1 << 2,  // Property is read-only, visible in editor but not editable

			Defaults = Edit
		};
		ENUM_CLASS_FLAGS(PCGPropertyFlags);

		enum class PCGPropertyType
		{
			ENUM,
			BOOL,
			INT8,
			INT16,
			INT,
			INT64,
			UINT8,
			UINT16,
			UINT32,
			UINT64,
			FLOAT,
			DOUBLE,
			STRING,
			NAME,
			FILEPATH,
			VECTOR2,
			VECTOR3,
			VECTOR4,
			STRUCT,
			// Pointers, only PCGObject derived class pointer
			OBJECT_PTR,  
			UNIQUE_PTR, // Not implemented yet
			SHARED_PTR,
			ARRAY
		};
		DECLARE_PCG_ENUM(PCGPropertyType)

		struct PCGGenericPropertyParams
		{
			Name name;
			PCGPropertyType type;
			PCGPropertyFlags flags = PCGPropertyFlags::Defaults;
			size_t offset;
		};

		class PCGProperty
		{
		public:
			PCGProperty(PCGClass* owner, const PCGGenericPropertyParams& params)
				: m_Owner(owner)
				, m_Name(params.name)
				, m_Type(params.type)
				, m_AddressOffset(params.offset)
				, m_PropertySize(0)
				, m_Flags(params.flags)
			{
			}

			virtual ~PCGProperty() = default;

			void* GetPropertyAddress(void* instance) const
			{
				return reinterpret_cast<void*>(reinterpret_cast<char*>(instance) + m_AddressOffset);
			}

			const Name& GetName() const { return m_Name; }

			size_t GetSize() const { return m_PropertySize; }

			bool IsVisibleInEditor() const { return !IsTransient() && (m_Flags & PCGPropertyFlags::Edit) == PCGPropertyFlags::Edit; }

			bool IsReadOnly() const { return (m_Flags & PCGPropertyFlags::ReadOnly) == PCGPropertyFlags::ReadOnly; }

			bool IsTransient() const { return (m_Flags & PCGPropertyFlags::Transient) == PCGPropertyFlags::Transient; }

			PCGPropertyType GetType() const { return m_Type; }

			const std::string& GetCategory() const { return m_EditorData.at(Name("Category")); }

			PCGProperty* SetCategory(std::string_view category) { m_EditorData[Name("Category")] = category; return this; }

			PCGClass* GetOwner() const { return m_Owner; }

			PCGProperty* SetDisplayName(std::string_view displayName) { m_EditorData[Name("DisplayName")] = displayName; return this; }

			_EchoComponentExport const char* GetDisplayName() const;

		protected:
			PCGClass* m_Owner;

			Name m_Name;

			PCGPropertyType m_Type;

			size_t m_AddressOffset;

			size_t m_PropertySize;

			PCGPropertyFlags m_Flags = PCGPropertyFlags::Defaults;

			std::unordered_map<Name, std::string, NameIDHasher> m_EditorData;
		};

		class PCGNumericProperty : public PCGProperty
		{
		public:
			PCGNumericProperty(PCGClass* owner, const PCGGenericPropertyParams& params)
				: PCGProperty(owner, params)
			{
			}

			// For enum underlying property
			virtual int64 ToInt64(const void* address) = 0;

			virtual void SetFromInt64(void* address, int64 intValue) = 0;
		};

		template<typename CppNumericType>
		class PCGProperty_Numeric : public PCGNumericProperty
		{
		public:
			PCGProperty_Numeric(PCGClass* owner, const PCGGenericPropertyParams& params)
				: PCGNumericProperty(owner, params)
			{
				m_PropertySize = sizeof(CppNumericType);
			}

			virtual int64 ToInt64(const void* address) override
			{
				return static_cast<int64>(*reinterpret_cast<const CppNumericType*>(address));
			}

			virtual void SetFromInt64(void* address, int64 intValue) override
			{
				*reinterpret_cast<CppNumericType*>(address) = static_cast<CppNumericType>(intValue);
			}
		};

		class PCGByteProperty : public PCGProperty_Numeric<uint8>
		{
		public:
			PCGByteProperty(PCGClass* owner, const PCGGenericPropertyParams& params)
				: PCGProperty_Numeric<uint8>(owner, params)
			{
				m_Type = PCGPropertyType::UINT8;
			}
		};

		class PCGUInt16Property : public PCGProperty_Numeric<uint16>
		{
		public:
			PCGUInt16Property(PCGClass* owner, const PCGGenericPropertyParams& params)
				: PCGProperty_Numeric<uint16>(owner, params)
			{
				m_Type = PCGPropertyType::UINT16;
			}
		};

		class PCGUInt32Property : public PCGProperty_Numeric<uint32>
		{
		public:
			PCGUInt32Property(PCGClass* owner, const PCGGenericPropertyParams& params)
				: PCGProperty_Numeric<uint32>(owner, params)
			{
				m_Type = PCGPropertyType::UINT32;
			}
		};

		class PCGUInt64Property : public PCGProperty_Numeric<uint64>
		{
		public:
			PCGUInt64Property(PCGClass* owner, const PCGGenericPropertyParams& params)
				: PCGProperty_Numeric<uint64>(owner, params)
			{
				m_Type = PCGPropertyType::UINT64;
			}
		};

		class PCGInt8Property : public PCGProperty_Numeric<int8>
		{
		public:
			PCGInt8Property(PCGClass* owner, const PCGGenericPropertyParams& params)
				: PCGProperty_Numeric<int8>(owner, params)
			{
				m_Type = PCGPropertyType::INT8;
			}
		};

		class PCGInt16Property : public PCGProperty_Numeric<int16>
		{
		public:
			PCGInt16Property(PCGClass* owner, const PCGGenericPropertyParams& params)
				: PCGProperty_Numeric<int16>(owner, params)
			{
				m_Type = PCGPropertyType::INT16;
			}
		};

		class PCGInt32Property : public PCGProperty_Numeric<int32>
		{
		public:
			PCGInt32Property(PCGClass* owner, const PCGGenericPropertyParams& params)
				: PCGProperty_Numeric<int32>(owner, params)
			{
				m_Type = PCGPropertyType::INT;
			}
		};

		class PCGInt64Property : public PCGProperty_Numeric<int64>
		{
		public:
			PCGInt64Property(PCGClass* owner, const PCGGenericPropertyParams& params)
				: PCGProperty_Numeric<int64>(owner, params)
			{
				m_Type = PCGPropertyType::INT64;
			}
		};

		class PCGFloatProperty : public PCGProperty_Numeric<float>
		{
		public:
			PCGFloatProperty(PCGClass* owner, const PCGGenericPropertyParams& params)
				: PCGProperty_Numeric<float>(owner, params)
			{
				m_Type = PCGPropertyType::FLOAT;
			}
		};

		class PCGDoubleProperty : public PCGProperty_Numeric<double>
		{
		public:
			PCGDoubleProperty(PCGClass* owner, const PCGGenericPropertyParams& params)
				: PCGProperty_Numeric<double>(owner, params)
			{
				m_Type = PCGPropertyType::DOUBLE;
			}
		};

		class PCGStringProperty : public PCGProperty
		{
		public:
			PCGStringProperty(PCGClass* owner, const PCGGenericPropertyParams& params)
				: PCGProperty(owner, params)
			{
				m_Type = PCGPropertyType::STRING;
				m_PropertySize = sizeof(std::string);
			}
		};

		class PCGFilePathProperty : public PCGStringProperty
		{
		public:
			PCGFilePathProperty(PCGClass* owner, const PCGGenericPropertyParams& params)
				: PCGStringProperty(owner, params)
			{
				m_Type = PCGPropertyType::FILEPATH;
				m_PropertySize = sizeof(std::string);
			}

			constexpr static const char* filterKey = "FilePathFilter";
			constexpr static const char* initialDirKey = "FilePathInitialDir";

			PCGFilePathProperty* SetFilter(const std::string& filter) { m_EditorData[Name(filterKey)] = filter; return this; }

			const std::string& GetFilter() const { return m_EditorData.at(Name(filterKey)); }

			PCGFilePathProperty* SetInitialDirectory(std::string_view dir) { m_EditorData[Name(initialDirKey)] = dir;  return this; }

			const std::string& GetInitialDirectory() const { return m_EditorData.at(Name(initialDirKey)); }
		};

		_EchoComponentExport std::string MakeFileFilterString(std::string_view description, std::string_view pattern);

		struct PCGEnumPropertyParams : public PCGGenericPropertyParams
		{
			PCGEnum* enumType;
		};

		class PCGEnumProperty : public PCGProperty
		{
		public:
			PCGEnumProperty(PCGClass* owner, const PCGEnumPropertyParams& params)
				: PCGProperty(owner, params)
				, m_Enum(params.enumType)
			{
				m_Type = PCGPropertyType::ENUM;
			}

			virtual ~PCGEnumProperty()
			{
			}

			_EchoComponentExport void SetUnderlyingProperty(PCGNumericProperty* prop);

			void SetIntValue(void* address, int64 intValue) const { m_UnderlyingProperty->SetFromInt64(address, intValue); }

			_EchoComponentExport void SetNameValue(void* address, const Name& enumNameValue) const;

			_EchoComponentExport Name GetNameValue(const void* address) const;

			int64 GetIntValue(const void* address) const { return m_UnderlyingProperty->ToInt64(address); }

			PCGNumericProperty* GetUnderlyingProperty() const { return m_UnderlyingProperty; }

			PCGEnum* GetEnum() const { return m_Enum; }

		private:
			PCGNumericProperty* m_UnderlyingProperty = nullptr;

			PCGEnum* m_Enum = nullptr;
		};

		// Struct Property
		struct PCGStructPropertyParams : public PCGGenericPropertyParams
		{
			PCGClass* structType;
		};

		class PCGStructProperty : public PCGProperty
		{
		public:
			PCGStructProperty(PCGClass* owner, const PCGStructPropertyParams& params);

			PCGClass* GetStructType() const { return m_StructType; }

		private:
			PCGClass* m_StructType = nullptr;
		};

		class PCGObjectProperty : public PCGProperty
		{
		public:
			PCGObjectProperty(PCGClass* owner, const PCGGenericPropertyParams& params)
				: PCGProperty(owner, params)
			{
				m_Type = PCGPropertyType::OBJECT_PTR;
				m_PropertySize = sizeof(PCGObject*);
			}
		};

		class PCGSharedPtrProperty : public PCGProperty
		{
		public:
			PCGSharedPtrProperty(PCGClass* owner, const PCGGenericPropertyParams& params)
				: PCGProperty(owner, params)
			{
				m_Type = PCGPropertyType::SHARED_PTR;
			}

			virtual std::shared_ptr<PCGObject> ToSharedPtr(void* address) const = 0;

			virtual std::shared_ptr<const PCGObject> ToSharedPtr(const void* address) const
			{
				return ToSharedPtr(const_cast<void*>(address));
			}
		};

		template<typename T>
		class PCGProperty_SharedPtr : public PCGSharedPtrProperty
		{
		public:
			using Type = std::shared_ptr<T>;

			PCGProperty_SharedPtr(PCGClass* owner, const PCGGenericPropertyParams& params)
				: PCGSharedPtrProperty(owner, params)
			{
				m_PropertySize = sizeof(Type);
			}

			virtual std::shared_ptr<PCGObject> ToSharedPtr(void* address) const override
			{
				return *reinterpret_cast<Type*>(address);
			}
		};

		// Array Property
		struct PCGArrayPropertyParams : public PCGGenericPropertyParams
		{
			PCGProperty* elementProperty;
		};

		class PCGArrayProperty : public PCGProperty
		{
		public:
			PCGArrayProperty(PCGClass* owner, const PCGArrayPropertyParams& params)
				: PCGProperty(owner, params)
				, m_ElementProperty(params.elementProperty)
			{
				m_Type = PCGPropertyType::ARRAY;
			}

			PCGProperty* GetElementProperty() const { return m_ElementProperty; }

			virtual void AddNewElement(void* arrayAddress) const = 0;

			virtual bool RemoveElement(void* arrayAddress, size_t index) const = 0;

			virtual void* GetElementAddress(void* arrayAddress, size_t index) const = 0;

			virtual const void* GetElementAddress(const void* arrayAddress, size_t index) const = 0;

			virtual size_t GetArraySize(const void* arrayAddress) const = 0;

			virtual void ClearAllElements(void* arrayAddress) const = 0;

			virtual void ResizeArray(void* arrayAddress, size_t newSize) const = 0;

		private:
			PCGProperty* m_ElementProperty = nullptr;
		};

		template<typename ElementType>
		class PCGProperty_Array : public PCGArrayProperty
		{
		public:
			using Type = std::vector<ElementType>;

			PCGProperty_Array(PCGClass* owner, const PCGArrayPropertyParams& params)
				: PCGArrayProperty(owner, params)
			{
				m_PropertySize = sizeof(Type);
			}

			virtual void AddNewElement(void* arrayAddress) const override
			{
				Type* vec = CastArray(arrayAddress);
				vec->emplace_back();
			}

			virtual bool RemoveElement(void* arrayAddress, size_t index) const override
			{
				Type* vec = CastArray(arrayAddress);
				if (index < vec->size())
				{
					vec->erase(vec->begin() + index);
					return true;
				}
				return false;
			}

			virtual void* GetElementAddress(void* arrayAddress, size_t index) const override
			{
				Type* vec = CastArray(arrayAddress);
				if (index < vec->size())
				{
					return &(vec->at(index));
				}
				return nullptr;
			}

			virtual const void* GetElementAddress(const void* arrayAddress, size_t index) const override
			{
				return GetElementAddress(const_cast<void*>(arrayAddress), index);
			}

			virtual size_t GetArraySize(const void* arrayAddress) const override
			{
				return CastArray(arrayAddress)->size();
			}

			virtual void ClearAllElements(void* arrayAddress) const override
			{
				CastArray(arrayAddress)->clear();
			}

			virtual void ResizeArray(void* arrayAddress, size_t newSize) const override
			{
				CastArray(arrayAddress)->resize(newSize);
			}

		private:

			inline Type* CastArray(void* arrayAddress) const
			{
				return static_cast<Type*>(arrayAddress);
			}

			inline const Type* CastArray(const void* arrayAddress) const
			{
				return static_cast<Type*>(const_cast<void*>(arrayAddress));
			}

		};
	}
}
