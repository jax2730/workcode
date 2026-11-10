//-----------------------------------------------------------------------------
// File:  EchoHelperObjProFuncImp.h
//
// Author: wuyunlong
//
// Date:   2022-4-22
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#ifndef __EchoHelperObjProFunctionImp_H__
#define __EchoHelperObjProFunctionImp_H__

#include "EchoComponentPrerequisites.h"
#include "EchoProtocolComponent.h"
#include "EchoProtocolComponentImp.h"
#include "EchoRenderOperation.h"
#include "EchoEntityObject.h"
#include "EchoHelperObj.h"
#include "EchoRay.h"

//-----------------------------------------------------------------------------
//	Head File Begin
//-----------------------------------------------------------------------------

namespace Echo
{
	struct HelperDesc
	{
		Echo::Name					helperName;
		HelperObjType				type;
		HelperObjColorType			color;
		Vector3						initPos;
		Quaternion					initRot;
		Vector3						initScale;
		ECHO_DRAW					renderType;
		Name						customName;
		bool						customSync;
		std::vector<Vector3>		customVBVec;
		std::vector<uint16>			customIBVec;
		HelperDesc()
		{
			initPos = Vector3::ZERO;
			initRot = Quaternion::IDENTITY;
			initScale = Vector3::UNIT_SCALE;
			type = HelperObjType_Const;
			color = HelperObjColorType_Green;
			renderType = ECHO_DRAW_MODE_COUNT;
			customSync = false;
		}
	};

	struct HelperCustomMeshInfo
	{
		Name meshName;
		bool sync;
		HelperCustomMeshInfo()
		{
			sync = false;
		}
	};

	struct HelperCustomPolygonInfo
	{
		std::vector<Vector3> vbVec;
		std::vector<uint16> ibVec;
	};
	
	struct PosInHelper
	{
		Vector3				inPos;
		ProtocolHandle		handle;
		PosInHelper()
		{
			handle = 0;
		}
	};

	struct ClickHelper
	{
		Echo::Ray			ray;
		ProtocolHandle		handle;
		ClickHelper()
		{
			handle = 0;
		}
	};

	class _EchoComponentExport HelperObjProFuncImp
	{
	public:
		//-----------------------------------------------------------------------------
		static void							initProtocolFunction();
		static void							uninitProtocolFunction();

		//-----------------------------------------------------------------------------
		static ProtocolResult				ProCom_CreateHelper(void* handle, ProtocolHandle info);
		static ProtocolResult				ProCom_DeleteHelper(void* data, ProtocolHandle handle);
		static ProtocolResult				ProCom_SetHelperPos(void* pos, ProtocolHandle handle);
		static ProtocolResult				ProCom_GetHelperPos(void* pos, ProtocolHandle handle);
		static ProtocolResult				ProCom_SetHelperRot(void* qua, ProtocolHandle handle);
		static ProtocolResult				ProCom_GetHelperRot(void* qua, ProtocolHandle handle);
		static ProtocolResult				ProCom_SetHelperScale(void* scale, ProtocolHandle handle);
		static ProtocolResult				ProCom_GetHelperScale(void* scale, ProtocolHandle handle);
		static ProtocolResult				ProCom_SetHelperRenderType(void* type, ProtocolHandle handle);
		static ProtocolResult				ProCom_GetHelperRenderType(void* type, ProtocolHandle handle);
		static ProtocolResult				ProCom_SetHelperCustomMesh(void* info, Echo::ProtocolHandle handle);
		static ProtocolResult				ProCom_SetHelperCustomPolygon(void* info, Echo::ProtocolHandle handle);
		static ProtocolResult				ProCom_SetHelperColor(void* color, ProtocolHandle handle);
		static ProtocolResult				ProCom_GetHelperColor(void* color, ProtocolHandle handle);
		static ProtocolResult				ProCom_IsPosInHelper(void* isIn, ProtocolHandle handle);
		static ProtocolResult				ProCom_GetHelperWorldPosList(void* outlist, ProtocolHandle handle);
		static ProtocolResult				ProCom_GetHelperLocalPosList(void* outlist, ProtocolHandle handle);
		static ProtocolResult				ProCom_ClickHelper(void* clickDis, ProtocolHandle handle);
	private:

	};

} // namespace Echo

  //-----------------------------------------------------------------------------
  //	Head File End
  //-----------------------------------------------------------------------------
#endif

