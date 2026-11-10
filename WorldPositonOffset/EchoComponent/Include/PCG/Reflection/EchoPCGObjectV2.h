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

#include "EchoName.h"
#include "PCG/Reflection/EchoPCGClass.h"

namespace Echo
{
	namespace PCGV2
	{
		class PCGClass;
		class PCGObject;

		class PCGObjectInitializer
		{
		public:
			PCGObjectInitializer() {}

			PCGObjectInitializer(PCGObject* obj)
				: m_Obj(obj)
			{
			}

			ECHO_FORCE_INLINE auto GetObj() const -> PCGObject* { return m_Obj; }

			static _EchoComponentExport auto Get() -> const PCGObjectInitializer&;

			PCGObject* m_Obj = nullptr;
		};

		class _EchoComponentExport PCGObject
		{
		public:
			PCGObject();

			PCGObject(const Name& name)
				: m_NamePrivate(name)
			{
			}

			virtual ~PCGObject() = default;

			virtual PCGClass* GetClass() const;

			static PCGClass* StaticClass();

			Name GetObjectName() const { return m_NamePrivate; }

			void SetObjectName(const Name& name) { m_NamePrivate = name; }

			static void __DefaultConstructor(const PCGObjectInitializer& X) { assert(false && "Cannot instantiate PCGObject directly"); }

			virtual void PostEditChangeProperty(const PCGProperty* property) {}

		protected:
			Name m_NamePrivate;
		};

		template<class T>
		void InternalConstructor(const PCGObjectInitializer& X)
		{
			T::__DefaultConstructor(X);
		}

		struct PCGStaticConstructObjectParameters
		{
			PCGClass* classType = nullptr;

			Name name;

			size_t size = 0;
		};

		_EchoComponentExport PCGObject* StaticAllocateObject(const PCGClass* pcgClass, const Name& name);

		_EchoComponentExport PCGObject* StaticConstructObject(const PCGStaticConstructObjectParameters& params);

		template<typename T>
		T* NewPCGObject(Name name)
		{
			static_assert(std::is_base_of<PCGObject, T>::value, "T must be derived from DObject");

			PCGStaticConstructObjectParameters params;
			params.classType = T::StaticClass();
			params.name = name;
			params.size = sizeof(T);

			PCGObject* obj = StaticConstructObject(params);

			return static_cast<T*>(obj);
		}

		template<typename T>
		T* NewPCGObject(PCGClass* inClass, Name name = Name::g_cEmptyName)
		{
			static_assert(std::is_base_of<PCGObject, T>::value, "T must be derived from DObject");

			PCGStaticConstructObjectParameters params;
			params.classType = inClass;
			params.name = name.empty() ? inClass->GetName() : name;
			params.size = inClass->GetStructureSize();

			PCGObject* obj = StaticConstructObject(params);

			return static_cast<T*>(obj);
		}
	}
}

#define PCG_OBJECT_DEFAULT_CONSTRUCTOR(TClass) \
	static void __DefaultConstructor(const PCGObjectInitializer& X) { new (X.GetObj()) TClass(); }

#define PCG_STRUCT_DEFAULT_CONSTRUCTOR() \
	static void __DefaultConstructor(const PCGObjectInitializer& X) { assert(false && "Cannot instantiate struct with initializer"); } \

#define PCG_ABSTRACT_OBJECT_DEFAULT_CONSTRUCTOR(TClass) \
	static void __DefaultConstructor(const PCGObjectInitializer& X) { assert(false && "Cannot instantiate abstract class"); } \

#define PCG_OBJECT_CORE_FUNC_DECL \
	virtual PCGClass* GetClass() const override \
	{ \
		return StaticClass(); \
	} \
	\
	static PCGClass* StaticClass();

#define PCG_STRUCT_CORE_FUNC_DECL \
	PCGClass* GetClass() const \
	{ \
		return StaticClass(); \
	} \
	\
	static PCGClass* StaticClass();

// Macro to declare PCG struct/class body, apply in header file with public access
// Need implement StaticClass() in cpp file

// Body for struct, have the same functions as PCGObject but these functions are not virtual
#define PCG_STRUCT_BODY() \
	PCG_STRUCT_DEFAULT_CONSTRUCTOR() \
	PCG_STRUCT_CORE_FUNC_DECL

// Body for class derived from PCGObject that can be instantiated
#define PCG_OBJECT_BODY(TClass) \
	PCG_OBJECT_DEFAULT_CONSTRUCTOR(TClass) \
	PCG_OBJECT_CORE_FUNC_DECL

// Body for an abstract class derived from PCGObject that should not call NewPCGObject to instantiate
#define PCG_ABSTRACT_OBJECT_BODY(TClass) \
	PCG_ABSTRACT_OBJECT_DEFAULT_CONSTRUCTOR(TClass) \
	PCG_OBJECT_CORE_FUNC_DECL
