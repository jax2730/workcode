//-----------------------------------------------------------------------------
// File:   MaterialProFunctionImp.cpp
//
// Author: wuyunlong	
//
// Date:   2022-3-29
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#include "EchoComponentStableHeaders.h"
#include "EchoMaterialProFunctionImp.h"
#include "EchoMaterial.h"
#include "EchoMaterialManager.h"


//-----------------------------------------------------------------------------
//	CPP File Begin
//-----------------------------------------------------------------------------

namespace Echo
{
	static std::set<MaterialPtr>	mMaterialSet;
	//=============================================================================
	void MaterialProFunctionImp::initProtocolFunction()
	{
		auto p = ProtocolComponentSystem::instance();
		p->registerProtocolFunc(PFID_Material_Create, &MaterialProFunctionImp::ProCom_CreateMaterial);
		p->registerProtocolFunc(PFID_Material_GetName, &MaterialProFunctionImp::ProCom_GetMaterialName);
		p->registerProtocolFunc(PFID_Material_GetShaderName, &MaterialProFunctionImp::ProCom_GetMaterialShaderName);
		p->registerProtocolFunc(PFID_Material_GetGenMask, &MaterialProFunctionImp::ProCom_GetMaterialGenMask);
	}

	//=============================================================================
	void MaterialProFunctionImp::uninitProtocolFunction()
	{
		mMaterialSet.clear();
		//auto p = ProtocolComponentSystem::instance();
	}

	ProtocolResult MaterialProFunctionImp::ProCom_CreateMaterial(void * handle, ProtocolHandle path)
	{
		Echo::Name* namePtr = (Echo::Name*)path;
		MaterialHandle* materialHandle = (MaterialHandle*)handle;
		try
		{
			materialHandle->mat = MaterialManager::instance()->load(*namePtr, false);
		}
		catch (...)
		{
			materialHandle->mat.setNull();
		}
		return PR_OK;
	}

	ProtocolResult MaterialProFunctionImp::ProCom_GetMaterialName(void * name, ProtocolHandle handle)
	{
		if (handle == 0)
			return PR_OK;
		MaterialHandle materialHandle = *(MaterialHandle*)handle;
		*(Echo::Name*)name = materialHandle.mat->getName();
		return PR_OK;
	}

	ProtocolResult MaterialProFunctionImp::ProCom_GetMaterialShaderName(void * shaderName, ProtocolHandle handle)
	{
		if (handle == 0)
			return PR_OK;
		MaterialHandle materialHandle = *(MaterialHandle*)handle;
		*(std::string*)shaderName = materialHandle.mat->m_matInfo.shadername;
		return PR_OK;
	}

	ProtocolResult MaterialProFunctionImp::ProCom_GetMaterialGenMask(void * genMask, ProtocolHandle handle)
	{
		if (handle == 0)
			return PR_OK;
		MaterialHandle materialHandle = *(MaterialHandle*)handle;
		*(std::string*)genMask = materialHandle.mat->m_matInfo.genmask;
		return PR_OK;
	}
	//=============================================================================
} // namespace Echo

  //-----------------------------------------------------------------------------
  //	CPP File End
  //-----------------------------------------------------------------------------
