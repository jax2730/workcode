//-----------------------------------------------------------------------------
// File:   ResBaseProFunctionImp.cpp
//
// Author: wuyunlong	
//
// Date:   2022-3-29
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include "EchoComponentStableHeaders.h"
#include "EchoResBaseProFunctionImp.h"
#include "EchoMesh.h"
#include "EchoSubMesh.h"
#include "EchoMeshMgr.h"
#include "EchoTextureManager.h"

//-----------------------------------------------------------------------------
//	CPP File Begin
//-----------------------------------------------------------------------------

namespace Echo
{
	//=============================================================================
	void ResBaseProFunctionImp::initProtocolFunction()
	{
		auto p = ProtocolComponentSystem::instance();
		p->registerProtocolExtFunc(Echo::Name("MaterialLoadJson"), &ResBaseProFunctionImp::ProCom_LoadMaterialJson);
		p->registerProtocolExtFunc(Echo::Name("MeshLoadJson"), &ResBaseProFunctionImp::ProCom_LoadMeshJson);
		p->registerProtocolExtFunc(Echo::Name("LoadMesh"), &ResBaseProFunctionImp::ProCom_LoadMesh);
		p->registerProtocolExtFunc(Echo::Name("LoadTexture"), &ResBaseProFunctionImp::ProCom_LoadTexture);
	}

	//=============================================================================
	void ResBaseProFunctionImp::uninitProtocolFunction()
	{
		//auto p = ProtocolComponentSystem::instance();
	}

	ProtocolResult ResBaseProFunctionImp::ProCom_LoadMeshJson(void* meshInfoPtr, ProtocolHandle pathPtr)
	{
		if (pathPtr == 0)
			return PR_OK;
		Echo::Name path = *(Echo::Name*)pathPtr;
		if (path.empty())
			return PR_OK;
		MeshJsonData* meshJson = (MeshJsonData*)meshInfoPtr;
		Mesh mesh(path);
		mesh.m_bMergeMesh = false; 
		mesh.m_bVirMesh = true;
		try
		{
			mesh.loadInfoFromJson();
		}
		catch (...)
		{
			meshInfoPtr = nullptr;
			return PR_Falied;
		}

		meshJson->meshInfoVersion = "[MESH_INFO_VERSION_GX_v1.0]";
		meshJson->realMeshName = mesh.getRealMeshName();
		meshJson->skeletonName = mesh.m_strSkeletonName;
		for (uint32 i = 0; i < mesh.getNumSubMesh(); i++)
		{ 
			SubMesh* subMesh = mesh.getSubMesh(i);
			meshJson->materialNames.emplace(i,subMesh->getMaterialName());
			meshJson->matInfoNames.emplace(i, subMesh->getMaterialName());;
		}
		return PR_OK;
	}

	ProtocolResult ResBaseProFunctionImp::ProCom_LoadMaterialJson(void* matInfoPtr, ProtocolHandle pathPtr)
	{
		if(pathPtr == 0)
			return PR_OK;
		Echo::Name path = *(Echo::Name*)pathPtr;
		if(path.empty())
			return PR_OK;
		Material material;
		material.m_bMergeInfo = false;
		material.setName(path);
		try
		{
			material.loadFromJson();
		}
		catch(...)
		{
			matInfoPtr = nullptr;
			return PR_Falied;
		}
		MatInfo* matinfo = (MatInfo*)matInfoPtr;
		matinfo->name = material.m_matInfo.name;
		matinfo->shadername = material.m_matInfo.shadername;
		matinfo->genmask = material.m_matInfo.genmask;
		matinfo->textures = material.m_matInfo.textures;
		return PR_OK;
	}

	ProtocolResult ResBaseProFunctionImp::ProCom_LoadMesh(void * handle, ProtocolHandle path)
	{
		Echo::Name* namePtr = (Echo::Name*)path;
		MeshHandle* meshHandle = (MeshHandle*)handle;
		try
		{
			meshHandle->mesh = MeshMgr::instance()->load(*namePtr, false);
		}
		catch (...)
		{
			meshHandle->mesh.setNull();
			return PR_Falied;
		}

		return PR_OK;
	}

	ProtocolResult ResBaseProFunctionImp::ProCom_LoadTexture(void * handle, ProtocolHandle path)
	{
		Echo::Name* namePtr = (Echo::Name*)path;
		TextureHandle* texHandle = (TextureHandle*)handle;
		try
		{
			texHandle->tex = TextureManager::instance()->load(*namePtr, false);
		}
		catch (...)
		{
			texHandle->tex.setNull();
		}

		return PR_OK;
	}
	//=============================================================================
} // namespace Echo

  //-----------------------------------------------------------------------------
  //	CPP File End
  //-----------------------------------------------------------------------------
