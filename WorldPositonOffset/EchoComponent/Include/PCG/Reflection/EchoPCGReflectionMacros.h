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

namespace Echo
{
	namespace PCGV2
	{
		class PCGEnum;
		class PCGObject;
		class PCGClass;
		class PCGObjectInitializer;

		template<typename EnumType> PCGEnum* StaticEnum()
		{
			assert(false && "Enum not declared. Please use DECLARE_PCG_ENUM macro to declare the enum.");
			return nullptr;
		}
	}
}

#define TYPE_NAME(type) Name(typeid(type).name())

#define ENUM_UNDERLYING_TYPE_NAME(EnumType) TYPE_NAME(std::underlying_type<EnumType>::type)

#define STRUCT_OFFSET(StructType, Member) ((::size_t)&reinterpret_cast<char const volatile&>((((StructType*)0)->Member)))

#define DECLARE_PCG_ENUM(EnumType) \
	template<> \
	Echo::PCGV2::PCGEnum* StaticEnum<EnumType>();


