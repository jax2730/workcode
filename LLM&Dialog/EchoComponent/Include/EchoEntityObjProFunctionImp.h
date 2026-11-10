//-----------------------------------------------------------------------------
// File:  EchoEntityObjProFunctionImp.h
//
// Author: wuyunlong
//
// Date:   2022-3-29
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#ifndef __EchoEntityObjProFunctionImp_H__
#define __EchoEntityObjProFunctionImp_H__

#include "EchoComponentPrerequisites.h"
#include "EchoProtocolComponent.h"
#include "EchoProtocolComponentImp.h"
#include "EchoRenderOperation.h"
#include "EchoEntityObject.h"

//-----------------------------------------------------------------------------
//	Head File Begin
//-----------------------------------------------------------------------------

namespace Echo
{
	struct CreateEntityInfo
	{
		Echo::Name		entityName;
		Echo::Name		meshPath;
		EM_LOD_LEVEL	lodLevel;
		bool			sync;
		Vector3			initPos;
		CreateEntityInfo()
		{
			lodLevel = LOD_LEVEL_MAX;
			sync = false;
			initPos = Vector3::ZERO;
		}
	};
	typedef std::map<Echo::EntityObject*, Echo::Name> EntityObjMap;
	class _EchoComponentExport EntityObjProFunctionImp
	{
	public:
		//-----------------------------------------------------------------------------
		static void							initProtocolFunction();
		static void							uninitProtocolFunction();

		//-----------------------------------------------------------------------------
		static ProtocolResult				ProCom_CreateEntity(void* handle, ProtocolHandle info);
		static ProtocolResult				ProCom_DeleteEntity(void* data, ProtocolHandle handle);
		static ProtocolResult				ProCom_SetEntityPos(void* pos, ProtocolHandle handle);
		static ProtocolResult				ProCom_GetEntityPos(void* pos, ProtocolHandle handle);
		static ProtocolResult				ProCom_SetEntityRot(void* qua, ProtocolHandle handle);
		static ProtocolResult				ProCom_GetEntityRot(void* qua, ProtocolHandle handle);
		static ProtocolResult				ProCom_SetEntityScale(void* scale, ProtocolHandle handle);
		static ProtocolResult				ProCom_GetEntityScale(void* scale, ProtocolHandle handle);
		static ProtocolResult				ProCom_ChangeEntityMat(void* material, ProtocolHandle handle);
		static ProtocolResult				ProCom_AddEntityTex(void* texture, ProtocolHandle handle);
	private:
		
	};

} // namespace Echo

  //-----------------------------------------------------------------------------
  //	Head File End
  //-----------------------------------------------------------------------------
#endif
