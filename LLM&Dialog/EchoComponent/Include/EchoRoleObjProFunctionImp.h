//-----------------------------------------------------------------------------
// File:  EchoRoleObjProFunctionImp.h
//
// Author: wuyunlong
//
// Date:   2022-4-12
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#ifndef __EchoRoleObjProFunctionImp_H__
#define __EchoRoleObjProFunctionImp_H__

#include "EchoComponentPrerequisites.h"
#include "EchoProtocolComponent.h"
#include "EchoProtocolComponentImp.h"

//-----------------------------------------------------------------------------
//	Head File Begin
//-----------------------------------------------------------------------------

namespace Echo
{
	struct RoleTagPointHandle
	{
		ProtocolHandle roleHandle;
		ProtocolHandle tagPointName;
		bool isShow;
		RoleTagPointHandle()
		{
			roleHandle = 0;
			isShow = true;
		}
	};

	class _EchoComponentExport RoleObjProFunctionImp
	{
	public:
		//-----------------------------------------------------------------------------
		static void							initProtocolFunction();
		static void							uninitProtocolFunction();

		//-----------------------------------------------------------------------------
		static ProtocolResult				ProCom_CreateRoleObj(void* handle, ProtocolHandle descPtr);
		static ProtocolResult				ProCom_DeleteRoleObj(void* data, ProtocolHandle handle);
		static ProtocolResult				ProCom_SetRoleObjPos(void* pos, ProtocolHandle handle);
		static ProtocolResult				ProCom_GetRoleObjPos(void* pos, ProtocolHandle handle);
		static ProtocolResult				ProCom_SetRoleObjRot(void* qua, ProtocolHandle handle);
		static ProtocolResult				ProCom_GetRoleObjRot(void* qua, ProtocolHandle handle);
		static ProtocolResult				ProCom_SetRoleObjScale(void* scale, ProtocolHandle handle);
		static ProtocolResult				ProCom_GetRoleObjScale(void* scale, ProtocolHandle handle);

		static ProtocolResult				ProCom_LogCustomRoleAABBInfo(void*, ProtocolHandle);
	};

} // namespace Echo

  //-----------------------------------------------------------------------------
  //	Head File End
  //-----------------------------------------------------------------------------
#endif
