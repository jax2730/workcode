//-----------------------------------------------------------------------------
// File:  EchoHelperObjProFuncImp.h
//
// Author: wuyunlong
//
// Date:   2022-4-22
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#include "EchoComponentStableHeaders.h"
#include "EchoHelperObjProFuncImp.h"
//-----------------------------------------------------------------------------
//	CPP File Begin
//-----------------------------------------------------------------------------

namespace Echo
{
	//=============================================================================
	void HelperObjProFuncImp::initProtocolFunction()
	{
		auto p = ProtocolComponentSystem::instance();
		p->registerProtocolFunc(PFID_Helper_Create, &HelperObjProFuncImp::ProCom_CreateHelper);
		p->registerProtocolFunc(PFID_Helper_Delete, &HelperObjProFuncImp::ProCom_DeleteHelper);
		p->registerProtocolFunc(PFID_Helper_SetPosition, &HelperObjProFuncImp::ProCom_SetHelperPos);
		p->registerProtocolFunc(PFID_Helper_GetPosition, &HelperObjProFuncImp::ProCom_GetHelperPos);
		p->registerProtocolFunc(PFID_Helper_SetRotation, &HelperObjProFuncImp::ProCom_SetHelperRot);
		p->registerProtocolFunc(PFID_Helper_GetRotation, &HelperObjProFuncImp::ProCom_GetHelperRot);
		p->registerProtocolFunc(PFID_Helper_SetScale, &HelperObjProFuncImp::ProCom_SetHelperScale);
		p->registerProtocolFunc(PFID_Helper_GetScale, &HelperObjProFuncImp::ProCom_GetHelperScale);
		p->registerProtocolFunc(PFID_Helper_SetRenderType, &HelperObjProFuncImp::ProCom_SetHelperRenderType);
		p->registerProtocolFunc(PFID_Helper_GetRenderType, &HelperObjProFuncImp::ProCom_GetHelperRenderType);
		p->registerProtocolFunc(PFID_Helper_SetCustomMesh, &HelperObjProFuncImp::ProCom_SetHelperCustomMesh);
		p->registerProtocolFunc(PFID_Helper_SetCustomPolygon, &HelperObjProFuncImp::ProCom_SetHelperCustomPolygon);
		p->registerProtocolFunc(PFID_Helper_SetColor, &HelperObjProFuncImp::ProCom_SetHelperColor);
		p->registerProtocolFunc(PFID_Helper_GetColor, &HelperObjProFuncImp::ProCom_GetHelperColor);
		p->registerProtocolFunc(PFID_Helper_IsPosIn, &HelperObjProFuncImp::ProCom_IsPosInHelper);
		p->registerProtocolFunc(PFID_Helper_GetWorldPosList, &HelperObjProFuncImp::ProCom_GetHelperWorldPosList);
		p->registerProtocolFunc(PFID_Helper_GetLocalPosList, &HelperObjProFuncImp::ProCom_GetHelperLocalPosList);
		p->registerProtocolFunc(PFID_Helper_Click, &HelperObjProFuncImp::ProCom_ClickHelper);
	}

	//=============================================================================
	void HelperObjProFuncImp::uninitProtocolFunction()
	{
		//auto p = ProtocolComponentSystem::instance();
	}

	ProtocolResult HelperObjProFuncImp::ProCom_CreateHelper(void * handle, ProtocolHandle info)
	{
		HelperDesc* infoPtr = (HelperDesc*)info;
		HelperObject* helperObject = new HelperObject(infoPtr->helperName,infoPtr->type);
		if (helperObject == NULL)
			return PR_OK;
		*(ProtocolHandle*)handle = (ProtocolHandle)helperObject;
		if (infoPtr->type == HelperObjType_Custom)
			helperObject->changeCustomMesh(infoPtr->customName, infoPtr->customSync);
		else if (infoPtr->type == HelperObjType_CustomPolygon)
			helperObject->changeCustomPolygon(infoPtr->customVBVec, infoPtr->customIBVec);
		helperObject->setPosition(infoPtr->initPos);
		helperObject->setOrientation(infoPtr->initRot);
		helperObject->setScale(infoPtr->initScale);
		helperObject->setRenderType(infoPtr->renderType);
		helperObject->setHelperColorType(infoPtr->color);
		return PR_OK;
	}

	ProtocolResult HelperObjProFuncImp::ProCom_DeleteHelper(void * data, ProtocolHandle handle)
	{
		if (handle == 0)
			return PR_OK;
		HelperObject* helperObject = (HelperObject*)handle;
		if (helperObject == NULL)
			return PR_OK;
		SAFE_DELETE(helperObject);
		return PR_OK;
	}

	ProtocolResult HelperObjProFuncImp::ProCom_SetHelperPos(void * pos, ProtocolHandle handle)
	{
		if (handle == 0)
			return PR_OK;
		HelperObject* helperObject = (HelperObject*)handle;
		if (helperObject == NULL)
			return PR_OK;
		helperObject->setPosition(*(Vector3*)pos);
		return PR_OK;
	}

	ProtocolResult HelperObjProFuncImp::ProCom_GetHelperPos(void * pos, ProtocolHandle handle)
	{
		*(Echo::Vector3*) pos = Echo::Vector3::ZERO;
		if (handle == 0)
			return Echo::PR_OK;
		HelperObject* helperObject = (HelperObject*)handle;
		if (helperObject != NULL)
			*(Echo::Vector3*) pos = helperObject->getPosition();
		return Echo::PR_OK;
	}

	ProtocolResult HelperObjProFuncImp::ProCom_SetHelperRot(void * qua, ProtocolHandle handle)
	{
		if (handle == 0)
			return PR_OK;
		HelperObject* helperObject = (HelperObject*)handle;
		if (helperObject == NULL)
			return PR_OK;
		helperObject->setOrientation(*(Quaternion*)qua);
		return PR_OK;
	}

	ProtocolResult HelperObjProFuncImp::ProCom_GetHelperRot(void * qua, ProtocolHandle handle)
	{
		*(Echo::Quaternion*) qua = Echo::Quaternion::IDENTITY;
		if (handle == 0)
			return Echo::PR_OK;
		HelperObject* helperObject = (HelperObject*)handle;
		if (helperObject != NULL)
			*(Echo::Quaternion*) qua = helperObject->getOrientation();
		return Echo::PR_OK;
	}

	ProtocolResult HelperObjProFuncImp::ProCom_SetHelperScale(void * scale, ProtocolHandle handle)
	{
		if (handle == 0)
			return PR_OK;
		HelperObject* helperObject = (HelperObject*)handle;
		if (helperObject == NULL)
			return PR_OK;
		helperObject->setScale(*(Vector3*)scale);
		return PR_OK;
	}

	ProtocolResult HelperObjProFuncImp::ProCom_GetHelperScale(void * scale, ProtocolHandle handle)
	{
		*(Echo::Vector3*) scale = Echo::Vector3::UNIT_SCALE;
		if (handle == 0)
			return Echo::PR_OK;
		HelperObject* helperObject = (HelperObject*)handle;
		if (helperObject != NULL)
			*(Echo::Vector3*) scale = helperObject->getScale();
		return Echo::PR_OK;
	}

	ProtocolResult HelperObjProFuncImp::ProCom_SetHelperRenderType(void * type, ProtocolHandle handle)
	{
		if (handle == 0)
			return PR_OK;
		HelperObject* helperObject = (HelperObject*)handle;
		if (helperObject == NULL)
			return PR_OK;
		helperObject->setRenderType(*(ECHO_DRAW*)type);
		return PR_OK;
	}

	ProtocolResult HelperObjProFuncImp::ProCom_GetHelperRenderType(void * type, ProtocolHandle handle)
	{
		*(Echo::ECHO_DRAW*) type = ECHO_DRAW_MODE_COUNT;
		if (handle == 0)
			return Echo::PR_OK;
		HelperObject* helperObject = (HelperObject*)handle;
		if (helperObject != NULL)
			*(Echo::ECHO_DRAW*) type = helperObject->getRenderType();
		return Echo::PR_OK;
	}

	ProtocolResult HelperObjProFuncImp::ProCom_SetHelperCustomMesh(void * info, Echo::ProtocolHandle handle)
	{
		if (handle == 0 || info == NULL)
			return PR_OK;
		HelperObject* helperObject = (HelperObject*)handle;
		if (helperObject == NULL)
			return PR_OK;
		HelperCustomMeshInfo* meshInfo = static_cast<HelperCustomMeshInfo*>(info);
		helperObject->changeCustomMesh(meshInfo->meshName,meshInfo->sync);
		return PR_OK;
	}

	ProtocolResult HelperObjProFuncImp::ProCom_SetHelperCustomPolygon(void * info, Echo::ProtocolHandle handle)
	{
		if (handle == 0 || info == NULL)
			return PR_OK;
		HelperObject* helperObject = (HelperObject*)handle;
		if (helperObject == NULL)
			return PR_OK;
		HelperCustomPolygonInfo* polygonInfo = static_cast<HelperCustomPolygonInfo*>(info);
		helperObject->changeCustomPolygon(polygonInfo->vbVec, polygonInfo->ibVec);
		return PR_OK;
	}

	ProtocolResult HelperObjProFuncImp::ProCom_SetHelperColor(void * color, ProtocolHandle handle)
	{
		if (handle == 0)
			return PR_OK;
		HelperObject* helperObject = (HelperObject*)handle;
		if (helperObject == NULL)
			return PR_OK;
		helperObject->setHelperColorType(*(HelperObjColorType*)(color));
		return PR_OK;
	}

	ProtocolResult HelperObjProFuncImp::ProCom_GetHelperColor(void * color, ProtocolHandle handle)
	{
		*(Echo::Vector4*) color = Vector4(0.f,1.f,0.f,1.f);
		if (handle == 0)
			return Echo::PR_OK;
		HelperObject* helperObject = (HelperObject*)handle;
		if (helperObject != NULL)
			*(Echo::Vector4*) color = helperObject->getHelperColor();
		return Echo::PR_OK;
	}

	ProtocolResult HelperObjProFuncImp::ProCom_IsPosInHelper(void * isIn, ProtocolHandle handle)
	{
		if (handle == 0)
			return Echo::PR_OK;
		PosInHelper* posInHelper = (PosInHelper*)handle;
		if(posInHelper->handle == 0)
			return Echo::PR_OK;
		HelperObject* helperObject = (HelperObject*)posInHelper->handle;
		*(bool*)isIn = helperObject->isPosIn(posInHelper->inPos);
		return Echo::PR_OK;
	}

	ProtocolResult HelperObjProFuncImp::ProCom_GetHelperWorldPosList(void * outlist, ProtocolHandle handle)
	{
		if (handle == 0)
			return Echo::PR_OK;
		HelperObject* helperObject = (HelperObject*)handle;
		if (helperObject == NULL)
			return Echo::PR_OK;
		std::vector<Vector3>* list = (std::vector<Vector3>*)outlist;
		helperObject->getPointsWorldPos(*list);
		return Echo::PR_OK;
	}

	ProtocolResult HelperObjProFuncImp::ProCom_GetHelperLocalPosList(void * outlist, ProtocolHandle handle)
	{
		if (handle == 0)
			return Echo::PR_OK;
		HelperObject* helperObject = (HelperObject*)handle;
		if (helperObject == NULL)
			return Echo::PR_OK;
		std::vector<Vector3>* list = (std::vector<Vector3>*)outlist;
		helperObject->getPointsLocalPos(*list);
		return Echo::PR_OK;
	}

	ProtocolResult HelperObjProFuncImp::ProCom_ClickHelper(void* clickDis, ProtocolHandle handle)
	{
		if (handle == 0)
			return Echo::PR_OK;
		ClickHelper* clickPtr = (ClickHelper*)handle;
		if (clickPtr->handle == 0)
			return Echo::PR_OK;
		HelperObject* helperObject = (HelperObject*)clickPtr->handle;
		if (helperObject == NULL)
			return Echo::PR_OK;
		*(float*)clickDis = helperObject->click(clickPtr->ray);
		return Echo::PR_OK;
	}
	//=============================================================================
} // namespace Echo

  //-----------------------------------------------------------------------------
  //	CPP File End
  //-----------------------------------------------------------------------------
