//-----------------------------------------------------------------------------
// File:  ResBaseProFunctionImp.h
//
// Author: wuyunlong
//
// Date:   2022-3-29
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#ifndef __ResBaseProFunctionImp_H__
#define __ResBaseProFunctionImp_H__

#include "EchoComponentPrerequisites.h"
#include "EchoProtocolComponent.h"
#include "EchoProtocolComponentImp.h"
#include "EchoMesh.h"
#include "EchoTexture.h"
#include "EchoName.h"

//-----------------------------------------------------------------------------
//	Head File Begin
//-----------------------------------------------------------------------------

namespace Echo
{
	//这部分为了保证各个插件能通用，只准增加不许删除！
	struct MeshJsonData
	{
		Echo::Name					meshInfoVersion;
		Echo::Name					realMeshName;
		Echo::Name					skeletonName;
		std::map<int, Echo::Name>	materialNames;
		std::map<int, Echo::Name>	matInfoNames;
	}; 

	struct MeshHandle
	{
		Echo::MeshPtr mesh;
	};

	struct TextureHandle
	{
		Echo::TexturePtr tex;
	};

	class _EchoComponentExport ResBaseProFunctionImp
	{
	public:
		//-----------------------------------------------------------------------------
		static void							initProtocolFunction();
		static void							uninitProtocolFunction();

		//-----------------------------------------------------------------------------
		static ProtocolResult				ProCom_LoadMeshJson(void* meshInfoPtr, ProtocolHandle pathPtr);
		static ProtocolResult				ProCom_LoadMaterialJson(void* matInfoPtr, ProtocolHandle pathPtr);
		static ProtocolResult				ProCom_LoadMesh(void * handle, ProtocolHandle pathPtr);
		static ProtocolResult				ProCom_LoadTexture(void * handle, ProtocolHandle pathPtr);
	};

} // namespace Echo

  //-----------------------------------------------------------------------------
  //	Head File End
  //-----------------------------------------------------------------------------
#endif
