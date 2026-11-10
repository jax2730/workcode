//-----------------------------------------------------------------------------
// File:   EchoRoleObjProFunctionImp.cpp
//
// Author: wuyunlong	
//
// Date:   2022-4-12
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#include "EchoComponentStableHeaders.h"
#include "EchoRoleObjProFunctionImp.h"
#include "EchoSceneManager.h"
#include "EchoAutoObjectManager.h"

#include "EchoActorRoleComponent.h"
//-----------------------------------------------------------------------------
//	CPP File Begin
//-----------------------------------------------------------------------------

namespace Echo
{
	//=============================================================================
	void RoleObjProFunctionImp::initProtocolFunction()
	{
		auto p = ProtocolComponentSystem::instance();
		p->registerProtocolFunc(Echo::PFID_RoleObj_Create, &RoleObjProFunctionImp::ProCom_CreateRoleObj);
		p->registerProtocolFunc(Echo::PFID_RoleObj_Delete, &RoleObjProFunctionImp::ProCom_DeleteRoleObj);
		p->registerProtocolFunc(Echo::PFID_RoleObj_SetPosition, &RoleObjProFunctionImp::ProCom_SetRoleObjPos);
		p->registerProtocolFunc(Echo::PFID_RoleObj_SetRotation, &RoleObjProFunctionImp::ProCom_SetRoleObjRot);
		p->registerProtocolFunc(Echo::PFID_RoleObj_SetScale, &RoleObjProFunctionImp::ProCom_SetRoleObjScale);

		p->registerProtocolExtFunc(Name("logDCRole"), &RoleObjProFunctionImp::ProCom_LogCustomRoleAABBInfo);
	}

	//=============================================================================
	void RoleObjProFunctionImp::uninitProtocolFunction()
	{
		auto p = ProtocolComponentSystem::instance();
		p->unregisterProtocolExtFunc(Name("logDCRole"));
	}

	ProtocolResult RoleObjProFunctionImp::ProCom_CreateRoleObj(void * handle, ProtocolHandle descPtr)
	{
		RoleObjectDesc desc = *(RoleObjectDesc*)descPtr;
		RoleObject* roleObj = new RoleObject(desc);
		*(ProtocolHandle*)handle = (ProtocolHandle)roleObj;
		return PR_OK;
	}

	ProtocolResult RoleObjProFunctionImp::ProCom_DeleteRoleObj(void * data, ProtocolHandle handle)
	{
		if (handle == 0)
			return PR_OK;
		RoleObject* roleObj = (RoleObject*)handle;
		if (roleObj == NULL)
			return PR_OK;
		delete roleObj;
		//AutoObjectManager::instance()->destroyRoleObj(roleObj);
		return PR_OK;
	}

	ProtocolResult RoleObjProFunctionImp::ProCom_SetRoleObjPos(void * pos, ProtocolHandle handle)
	{
		if (handle == 0)
			return PR_OK;
		RoleObject* roleObj = (RoleObject*)handle;
		if (roleObj == NULL)
			return PR_OK;
		roleObj->setPosition(*(Vector3*)pos);
		return PR_OK;
	}

	ProtocolResult RoleObjProFunctionImp::ProCom_GetRoleObjPos(void * pos, ProtocolHandle handle)
	{
		if (handle == 0)
			*(Echo::Vector3*) pos = Echo::Vector3::ZERO;
		RoleObject* roleObj = (RoleObject*)handle;
		*(Echo::Vector3*) pos = Echo::Vector3::ZERO;
		if (roleObj != NULL)
			*(Echo::Vector3*) pos = roleObj->getPosition();
		return Echo::PR_OK;
	}

	ProtocolResult RoleObjProFunctionImp::ProCom_SetRoleObjRot(void * qua, ProtocolHandle handle)
	{
		if (handle == 0)
			return PR_OK;
		RoleObject* roleObj = (RoleObject*)handle;
		if (roleObj == NULL)
			return PR_OK;
		roleObj->setOrientation(*(Quaternion*)qua);
		return PR_OK;
	}

	ProtocolResult RoleObjProFunctionImp::ProCom_GetRoleObjRot(void * qua, ProtocolHandle handle)
	{
		if (handle == 0)
			*(Echo::Quaternion*) qua = Echo::Quaternion::IDENTITY;
		RoleObject* roleObj = (RoleObject*)handle;
		*(Echo::Quaternion*) qua = Echo::Quaternion::IDENTITY;
		if (roleObj != NULL)
			*(Echo::Quaternion*) qua = roleObj->getOrientation();
		return Echo::PR_OK;
	}

	ProtocolResult RoleObjProFunctionImp::ProCom_SetRoleObjScale(void * scale, ProtocolHandle handle)
	{
		if (handle == 0)
			return PR_OK;
		RoleObject* roleObj = (RoleObject*)handle;
		if (roleObj == NULL)
			return PR_OK;
		roleObj->setScale(*(Vector3*)scale);
		return PR_OK;
	}

	ProtocolResult RoleObjProFunctionImp::ProCom_GetRoleObjScale(void * scale, ProtocolHandle handle)
	{
		if (handle == 0)
			*(Echo::Vector3*) scale = Echo::Vector3::ZERO;
		RoleObject* roleObj = (RoleObject*)handle;
		*(Echo::Vector3*) scale = Echo::Vector3::ZERO;
		if (roleObj != NULL)
			*(Echo::Vector3*) scale = roleObj->getScale();
		return Echo::PR_OK;
	}

	ProtocolResult RoleObjProFunctionImp::ProCom_LogCustomRoleAABBInfo(void*, ProtocolHandle)
	{
		std::vector<std::tuple<std::string, Vector3, Vector3>> infos;

		decltype(auto) mgrMap = ActorSystem::instance()->getAllActorManager();
		for (const auto& [nm, pMgr] : mgrMap)
		{
			if (pMgr == nullptr)
				continue;

			decltype(auto) actList = pMgr->mRootActorSet;
			for (const auto& actptr : actList)
			{
				if (actptr.expired() || actptr == nullptr)
					continue;
				std::vector<ActorComponentPtr> rolecomps;
				actptr->getComponentsByType(RoleComponent::mType, rolecomps, true);

				for (ActorComponentPtr comp : rolecomps)
				{
					if (comp.expired() || comp == nullptr)
						continue;
					RoleComponent* pRoleComp = (RoleComponent*)comp.getPointer();

					if (pRoleComp->GetCustomRoleAABB())
					{
						Vector3 min, max;
						pRoleComp->GetCustomRoleAABB(min, max);
						infos.emplace_back(pRoleComp->getCombinedName(), min, max);
					}
				}
			}
		}

		LogManager::instance()->logMessage("CustomAABBRoleComponent:: " + std::to_string(infos.size()));
		for (const auto& [name, min, max] : infos)
		{
			std::string info;
			info += "min[" + std::to_string(min.x) + "," + std::to_string(min.y) + "," + std::to_string(min.z) + "] ";
			info += "max[" + std::to_string(max.x) + "," + std::to_string(max.y) + "," + std::to_string(max.z) + "] ";
			info += name;
			LogManager::instance()->logMessage(info);
		}

		return PR_OK;
	}


	//=============================================================================
} // namespace Echo

  //-----------------------------------------------------------------------------
  //	CPP File End
  //-----------------------------------------------------------------------------
