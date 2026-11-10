//-----------------------------------------------------------------------------
// File:   EchoPCGObject.cpp
//
// Author: zhouguangyuan
//
// Date:   2025-10-22
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include "EchoComponentStableHeaders.h"
#include "PCG/Reflection/EchoPCGObjectV2.h"

#include "PCG/Reflection/EchoPCGClass.h"

namespace Echo
{
	namespace PCGV2
	{
		auto PCGObjectInitializer::Get() -> const PCGObjectInitializer&
		{
			static thread_local PCGObjectInitializer instance;
			return instance;
		}

		PCGObject::PCGObject()
		{
			m_NamePrivate = GetClass()->GetName();
		}

		PCGClass* PCGObject::GetClass() const
		{
			return StaticClass();
		}

		REGISTER_PCG_CLASS(PCGObject)
		PCGClass* PCGObject::StaticClass()
		{
			BEGIN_PCG_CLASS(PCGObject)
			END_PCG_CLASS()
		}


		PCGObject* StaticAllocateObject(const PCGClass* pcgClass, const Name& name)
		{
			if (!pcgClass) return nullptr;

			size_t size = pcgClass->GetStructureSize();
			assert(size > sizeof(PCGObject) && "PCGObject size is larger than allocated size!");

			void* memory = malloc(size);
			if (!memory)
			{
				assert(false && "Allocation failed!");
				return nullptr;
			}
			memset(memory, 0, size);
			PCGObject* obj = static_cast<PCGObject*>(memory);
			new (obj) PCGObject(name);

			return obj;
		}

		PCGObject* StaticConstructObject(const PCGStaticConstructObjectParameters& params)
		{
			PCGObject* obj = StaticAllocateObject(params.classType, params.name);
			PCGClass* inClass = params.classType;
			assert(obj && inClass && "Failed to allocate PCGObject!");

			PCGObjectInitializer initializer{ obj };
			inClass->GetConstructor()(initializer);

			return obj;
		}
	}
}
