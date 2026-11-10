//-----------------------------------------------------------------------------
// File:   EchoEntityObjProFunctionImp.cpp
//
// Author: wuyunlong	
//
// Date:   2022-3-29
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#include "EchoComponentStableHeaders.h"
#include "EchoEntityObjProFunctionImp.h"
#include "EchoSceneManager.h"
#include "EchoEntityObject.h"
#include "EchoMeshMgr.h"
#include "EchoMaterialProFunctionImp.h"
#include "EchoResBaseProFunctionImp.h"

//-----------------------------------------------------------------------------
//	CPP File Begin
//-----------------------------------------------------------------------------

namespace Echo
{
	static EntityObjMap					mEntityObjMap;
	//=============================================================================
	void EntityObjProFunctionImp::initProtocolFunction()
	{
		auto p = ProtocolComponentSystem::instance();
		p->registerProtocolFunc(PFID_EntityObj_Create, &EntityObjProFunctionImp::ProCom_CreateEntity);
		p->registerProtocolFunc(PFID_EntityObj_Delete, &EntityObjProFunctionImp::ProCom_DeleteEntity);
		p->registerProtocolFunc(PFID_EntityObj_SetPosition, &EntityObjProFunctionImp::ProCom_SetEntityPos);
		p->registerProtocolFunc(PFID_EntityObj_GetPosition, &EntityObjProFunctionImp::ProCom_GetEntityPos);
		p->registerProtocolFunc(PFID_EntityObj_SetRotation, &EntityObjProFunctionImp::ProCom_SetEntityRot);
		p->registerProtocolFunc(PFID_EntityObj_GetRotation, &EntityObjProFunctionImp::ProCom_GetEntityRot);
		p->registerProtocolFunc(PFID_EntityObj_SetScale, &EntityObjProFunctionImp::ProCom_SetEntityScale);
		p->registerProtocolFunc(PFID_EntityObj_GetScale, &EntityObjProFunctionImp::ProCom_GetEntityScale);
		p->registerProtocolFunc(PFID_EntityObj_ChangeMaterial, &EntityObjProFunctionImp::ProCom_ChangeEntityMat);
		p->registerProtocolExtFunc(Echo::Name("AddEntityTexture"), &EntityObjProFunctionImp::ProCom_AddEntityTex);
	}

	//=============================================================================
	void EntityObjProFunctionImp::uninitProtocolFunction()
	{
		for (auto& iter : mEntityObjMap)
			delete iter.first;
		mEntityObjMap.clear();
		//auto p = ProtocolComponentSystem::instance();
	}

	ProtocolResult EntityObjProFunctionImp::ProCom_CreateEntity(void * handle, ProtocolHandle info)
	{
		CreateEntityInfo* infoPtr = (CreateEntityInfo*)info;
		EntityObject* entityObj = new EntityObject(infoPtr->entityName);
		if(entityObj == NULL)
			return PR_OK;
		entityObj->changeEntityImpl(infoPtr->lodLevel, infoPtr->meshPath,infoPtr->sync);
		mEntityObjMap.emplace(entityObj, infoPtr->entityName);
		ProtocolHandle* handlePtr = (ProtocolHandle*)handle;
		(*handlePtr) = (ProtocolHandle)entityObj;
		entityObj->setPosition(infoPtr->initPos);
		return PR_OK;
	}

	ProtocolResult EntityObjProFunctionImp::ProCom_DeleteEntity(void * data, ProtocolHandle handle)
	{
		if (handle == 0)
			return PR_OK;
		EntityObject* entityObj = (EntityObject*)handle;
		if (entityObj == NULL)
			return PR_OK;
		if (mEntityObjMap.find(entityObj) != mEntityObjMap.end())
		{
			mEntityObjMap.erase(entityObj);
			delete entityObj;
		}
		return PR_OK;
	}

	ProtocolResult EntityObjProFunctionImp::ProCom_SetEntityPos(void * pos, ProtocolHandle handle)
	{
		if (handle == 0)
			return PR_OK;
		EntityObject* entityObj = (EntityObject*)handle;
		if (entityObj == NULL)
			return PR_OK;
		entityObj->setPosition(*(Vector3*)pos);
		return PR_OK;
	}

	ProtocolResult EntityObjProFunctionImp::ProCom_GetEntityPos(void * pos, ProtocolHandle handle)
	{
		if (handle == 0)
			*(Echo::Vector3*) pos = Echo::Vector3::ZERO;
		EntityObject* entityObj = (EntityObject*)handle;
		*(Echo::Vector3*) pos = Echo::Vector3::ZERO;
		if (entityObj != NULL)
			*(Echo::Vector3*) pos = entityObj->getPosition();
		return Echo::PR_OK;
	}

	ProtocolResult EntityObjProFunctionImp::ProCom_SetEntityRot(void * qua, ProtocolHandle handle)
	{
		if (handle == 0)
			return PR_OK;
		EntityObject* entityObj = (EntityObject*)handle;
		if (entityObj == NULL)
			return PR_OK;
		entityObj->setOrientation(*(Quaternion*)qua);
		return PR_OK;
	}

	ProtocolResult EntityObjProFunctionImp::ProCom_GetEntityRot(void * qua, ProtocolHandle handle)
	{
		if (handle == 0)
			*(Echo::Quaternion*) qua = Echo::Quaternion::IDENTITY;
		EntityObject* entityObj = (EntityObject*)handle;
		*(Echo::Quaternion*) qua = Echo::Quaternion::IDENTITY;
		if (entityObj != NULL)
			*(Echo::Quaternion*) qua = entityObj->getOrientation();
		return Echo::PR_OK;
	}

	ProtocolResult EntityObjProFunctionImp::ProCom_SetEntityScale(void * scale, ProtocolHandle handle)
	{
		if (handle == 0)
			return PR_OK;
		EntityObject* entityObj = (EntityObject*)handle;
		if (entityObj == NULL)
			return PR_OK;
		entityObj->setScale(*(Vector3*)scale);
		return PR_OK;
	}

	ProtocolResult EntityObjProFunctionImp::ProCom_GetEntityScale(void * scale, ProtocolHandle handle)
	{
		if (handle == 0)
			*(Echo::Vector3*) scale = Echo::Vector3::UNIT_SCALE;
		EntityObject* entityObj = (EntityObject*)handle;
		*(Echo::Vector3*) scale = Echo::Vector3::UNIT_SCALE;
		if (entityObj != NULL)
			*(Echo::Vector3*) scale = entityObj->getScale();
		return Echo::PR_OK;
	}

	ProtocolResult EntityObjProFunctionImp::ProCom_ChangeEntityMat(void * material, ProtocolHandle handle)
	{
		if (handle == 0)
			return PR_OK;
		EntityObject* entityObj = (EntityObject*)handle;
		if (entityObj == NULL)
			return PR_OK;
		MaterialHandle materialHandle = *(MaterialHandle*)material;
		//entityObj->changeMaterial(LOD_LEVEL_0, materialHandle.mat);
		return PR_OK;
	}

	ProtocolResult EntityObjProFunctionImp::ProCom_AddEntityTex(void* texture, ProtocolHandle handle)
	{
		if (handle == 0)
			return PR_OK;
		EntityObject* entityObj = (EntityObject*)handle;
		if (entityObj == NULL)
			return PR_OK;
		TextureHandle textureHandle = *(TextureHandle*)texture;
		entityObj->addCustomTextureParams(LOD_LEVEL_0, { std::make_pair(MAT_TEXTURE_DIFFUSE, textureHandle.tex) });
		return PR_OK;
	}

	//=============================================================================
	

} // namespace Echo

  //-----------------------------------------------------------------------------
  //	CPP File End
  //-----------------------------------------------------------------------------
