//-----------------------------------------------------------------------------
// File:  EchoCameraProFunctionImp.h
//
// Author: wuyunlong
//
// Date:   2022-4-12
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#ifndef __EchoCameraProFunctionImp_H__
#define __EchoCameraProFunctionImp_H__

#include "EchoComponentPrerequisites.h"
#include "EchoProtocolComponent.h"
#include "EchoProtocolComponentImp.h"

//-----------------------------------------------------------------------------
//	Head File Begin
//-----------------------------------------------------------------------------

namespace Echo
{
	class _EchoComponentExport CameraProFunctionImp
	{
	public:
		//-----------------------------------------------------------------------------
		static void							initProtocolFunction();
		static void							uninitProtocolFunction();

		//-----------------------------------------------------------------------------
		static ProtocolResult				ProCom_GetMainCamera(void* handle, ProtocolHandle nullHandle);
		static ProtocolResult				ProCom_GetSecondCamera(void* handle, ProtocolHandle nullHandle);

		static ProtocolResult				ProCom_SetCameraPos(void* pos, ProtocolHandle handle);
		static ProtocolResult				ProCom_GetCameraPos(void* pos, ProtocolHandle handle);
		static ProtocolResult				ProCom_SetCameraRot(void* qua, ProtocolHandle handle);
		static ProtocolResult				ProCom_GetCameraRot(void* qua, ProtocolHandle handle);
	};

	class EchoCameraScript
	{
	public:
		REGISTER_LUA_CLASS_KEY;
		static Vector3						Script_GetMainCameraPos();
		static void							Script_SetMainCameraPos(const Vector3& pos);
		static Quaternion					Script_GetMainCameraRot();
		static void							Script_SetMainCameraRot(const Quaternion& qua);
	};

} // namespace Echo

//-----------------------------------------------------------------------------
//	Head File End
//-----------------------------------------------------------------------------
#endif
