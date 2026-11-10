//-----------------------------------------------------------------------------
// File:  MaterialProFunctionImp.h
//
// Author: wuyunlong
//
// Date:   2022-3-29
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#ifndef __MaterialProFunctionImp_H__
#define __MaterialProFunctionImp_H__

#include "EchoComponentPrerequisites.h"
#include "EchoProtocolComponent.h"
#include "EchoProtocolComponentImp.h"
#include "EchoMaterial.h"

//-----------------------------------------------------------------------------
//	Head File Begin
//-----------------------------------------------------------------------------

namespace Echo
{
	struct MaterialHandle
	{
		Echo::MaterialPtr mat;
	};
	class _EchoComponentExport MaterialProFunctionImp
	{
	public:
		//-----------------------------------------------------------------------------
		static void							initProtocolFunction();
		static void							uninitProtocolFunction();

		//-----------------------------------------------------------------------------
		static ProtocolResult				ProCom_CreateMaterial(void* handle, ProtocolHandle path);
		static ProtocolResult				ProCom_GetMaterialName(void* name, ProtocolHandle handle);
		static ProtocolResult				ProCom_GetMaterialShaderName(void* shaderName, ProtocolHandle handle);
		static ProtocolResult				ProCom_GetMaterialGenMask(void* genMask, ProtocolHandle handle);
	};

} // namespace Echo

  //-----------------------------------------------------------------------------
  //	Head File End
  //-----------------------------------------------------------------------------
#endif
