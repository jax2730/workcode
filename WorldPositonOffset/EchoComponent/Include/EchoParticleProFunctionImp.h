//-----------------------------------------------------------------------------
// File:  EchoParticleProFunctionImp.h
//
// Author: wuyunlong
//
// Date:   2022-4-14
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#ifndef __EchoParticleProFunctionImp_H__
#define __EchoParticleProFunctionImp_H__

#include "EchoProtocolComponent.h"
#include "EchoProtocolComponentImp.h"

//-----------------------------------------------------------------------------
//	Head File Begin
//-----------------------------------------------------------------------------

namespace Echo
{

	class _EchoComponentExport ParticleProFunctionImp
	{
	public:
		//-----------------------------------------------------------------------------
		static void							initProtocolFunction();
		static void							uninitProtocolFunction();

		//-----------------------------------------------------------------------------
		static ProtocolResult				ProCom_CreateParticle(void* handle, ProtocolHandle descPtr);
		static ProtocolResult				ProCom_DeleteParticle(void* data, ProtocolHandle handle);
		static ProtocolResult				ProCom_SetParticlePos(void* pos, ProtocolHandle handle);
		static ProtocolResult				ProCom_GetParticlePos(void* pos, ProtocolHandle handle);
		static ProtocolResult				ProCom_SetParticleRot(void* qua, ProtocolHandle handle);
		static ProtocolResult				ProCom_GetParticleRot(void* qua, ProtocolHandle handle);
		static ProtocolResult				ProCom_SetParticleScale(void* scale, ProtocolHandle handle);
		static ProtocolResult				ProCom_GetParticleScale(void* scale, ProtocolHandle handle);
	};

} // namespace Echo

  //-----------------------------------------------------------------------------
  //	Head File End
  //-----------------------------------------------------------------------------
#endif
