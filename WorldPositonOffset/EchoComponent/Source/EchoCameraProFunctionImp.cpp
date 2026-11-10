//-----------------------------------------------------------------------------
// File:   EchoCameraProFunctionImp.cpp
//
// Author: wuyunlong	
//
// Date:   2022-4-12
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#include "EchoComponentStableHeaders.h"
#include "EchoCameraProFunctionImp.h"
#include "EchoSceneManager.h"
#include "EchoEntityObject.h"
#include "EchoMeshMgr.h"
#include "EchoMaterialProFunctionImp.h"

//-----------------------------------------------------------------------------
//	CPP File Begin
//-----------------------------------------------------------------------------

namespace Echo
{
	//=============================================================================
	void CameraProFunctionImp::initProtocolFunction()
	{
		auto p = ProtocolComponentSystem::instance();
		p->registerProtocolFunc(PFID_MainCamera_GetHandle, &CameraProFunctionImp::ProCom_GetMainCamera);
		p->registerProtocolFunc(PFID_SecondCamera_GetHandle, &CameraProFunctionImp::ProCom_GetSecondCamera);
		p->registerProtocolFunc(PFID_Camera_SetPosition, &CameraProFunctionImp::ProCom_SetCameraPos);
		p->registerProtocolFunc(PFID_Camera_GetPosition, &CameraProFunctionImp::ProCom_GetCameraPos);
		p->registerProtocolFunc(PFID_Camera_SetOrientation, &CameraProFunctionImp::ProCom_SetCameraRot);
		p->registerProtocolFunc(PFID_Camera_GetOrientation, &CameraProFunctionImp::ProCom_GetCameraRot);
	}

	//=============================================================================
	void CameraProFunctionImp::uninitProtocolFunction()
	{
		//auto p = ProtocolComponentSystem::instance();
	}

	ProtocolResult CameraProFunctionImp::ProCom_GetMainCamera(void * handle, ProtocolHandle nullHandle)
	{
		*(ProtocolHandle*)handle = (ProtocolHandle)Root::instance()->getMainSceneManager()->getMainCamera();
		return PR_OK;
	}

	ProtocolResult CameraProFunctionImp::ProCom_GetSecondCamera(void * handle, ProtocolHandle nullHandle)
	{
		*(ProtocolHandle*)handle = (ProtocolHandle)Root::instance()->getMainSceneManager()->getSecondMainCamera();
		return PR_OK;
	}

	ProtocolResult CameraProFunctionImp::ProCom_SetCameraPos(void * pos, ProtocolHandle handle)
	{
		if (handle == 0)
			return PR_OK;
		Echo::Camera* camera = (Echo::Camera*)handle;
		if (camera != NULL)
			camera->setPosition(*(Vector3*)pos);
		return PR_OK;
	}
	ProtocolResult CameraProFunctionImp::ProCom_GetCameraPos(void * pos, ProtocolHandle handle)
	{
		if (handle == 0)
			return PR_OK;
		Echo::Camera* camera = (Echo::Camera*)handle;
		if (camera != NULL)
			*(Vector3*)pos = camera->getPosition();
		return PR_OK;
	}
	ProtocolResult CameraProFunctionImp::ProCom_SetCameraRot(void * qua, ProtocolHandle handle)
	{
		if (handle == 0)
			return PR_OK;
		Echo::Camera* camera = (Echo::Camera*)handle;
		if (camera != NULL)
			camera->setOrientation(*(Quaternion*)qua);
		return PR_OK;
	}
	ProtocolResult CameraProFunctionImp::ProCom_GetCameraRot(void * qua, ProtocolHandle handle)
	{
		if (handle == 0)
			return PR_OK;
		Echo::Camera* camera = (Echo::Camera*)handle;
		if (camera != NULL)
			*(Quaternion*)qua = camera->getOrientation();
		return PR_OK;
	}

	Vector3 EchoCameraScript::Script_GetMainCameraPos()
	{
		ProtocolComponentSystem* proComponentMgr = ProtocolComponentSystem::instance();
		Vector3 pos = Vector3::ZERO;
		ProtocolHandle cameraHandle = 0;
		Echo::ProtocolResult result = proComponentMgr->callProtocolFunc(PFID_MainCamera_GetHandle, &cameraHandle, PH_NullHandle);
		if (result != PR_OK)
			return pos;
		proComponentMgr->callProtocolFunc(PFID_Camera_GetPosition, &pos, cameraHandle);
		return pos;
	}

	void EchoCameraScript::Script_SetMainCameraPos(const Vector3 & pos)
	{
		ProtocolComponentSystem* proComponentMgr = ProtocolComponentSystem::instance();
		ProtocolHandle cameraHandle = 0;
		Echo::ProtocolResult result = proComponentMgr->callProtocolFunc(PFID_MainCamera_GetHandle, &cameraHandle, PH_NullHandle);
		if (result != PR_OK)
			return;
		proComponentMgr->callProtocolFunc(PFID_Camera_SetPosition, (void*)&pos, cameraHandle);
	}

	Quaternion EchoCameraScript::Script_GetMainCameraRot()
	{
		ProtocolComponentSystem* proComponentMgr = ProtocolComponentSystem::instance();
		Quaternion qua = Quaternion::IDENTITY;
		ProtocolHandle cameraHandle = 0;
		Echo::ProtocolResult result = proComponentMgr->callProtocolFunc(PFID_MainCamera_GetHandle, &cameraHandle, PH_NullHandle);
		if (result != PR_OK)
			return qua;
		proComponentMgr->callProtocolFunc(PFID_Camera_GetOrientation, &qua, cameraHandle);
		return qua;
	}

	void EchoCameraScript::Script_SetMainCameraRot(const Quaternion & qua)
	{
		ProtocolComponentSystem* proComponentMgr = ProtocolComponentSystem::instance();
		ProtocolHandle cameraHandle = 0;
		Echo::ProtocolResult result = proComponentMgr->callProtocolFunc(PFID_MainCamera_GetHandle, &cameraHandle, PH_NullHandle);
		if (result != PR_OK)
			return;
		proComponentMgr->callProtocolFunc(PFID_Camera_SetOrientation, (void*)&qua, cameraHandle);
	}
	//=============================================================================
} // namespace Echo

//-----------------------------------------------------------------------------
//	CPP File End
//-----------------------------------------------------------------------------
