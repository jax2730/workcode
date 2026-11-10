//-----------------------------------------------------------------------------
// File:   EditorEntityObjProFunctionImp.cpp
//
// Author: wuyunlong	
//
// Date:   2022-5-20
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include "EchoComponentStableHeaders.h"
#include "EchoActorProFuncImp.h"
//#include "EchoEntityObjProFunctionImp.h"
#include "Actor/EchoNodeComponent.h"
#include "Actor/EchoActorManager.h"
#include "Actor/EchoActorInfoManager.h"
#include "Actor/EchoActor.h"
#include "EchoWorldNode.h"
#include "Actor/EchoActorComFactoryManager.h"

//-----------------------------------------------------------------------------
//	CPP File Begin
//-----------------------------------------------------------------------------

namespace Echo
{
	//=============================================================================
	void EchoActorProFuncImp::initProtocolFunction()
	{
		auto p = Echo::ProtocolComponentSystem::instance();
		
		p->registerProtocolFunc(Echo::PFID_Actor_GetComponent, &EchoActorProFuncImp::ProCom_ActorGetComponent);
		//p->registerProtocolFunc(Echo::PFID_Actor_GetEditorActor, &EchoActorProFuncImp::ProCom_GetEditorActorFromActor);
		Echo::ActorSystem::instance()->componentRegistered();
		
		//=============================================================================
		//p->registerProtocolFunc(Echo::PFID_Component_Create, &EchoActorProFuncImp::ProCom_CreateComponent);
		//p->registerProtocolFunc(Echo::PFID_Component_Delete, &EchoActorProFuncImp::ProCom_DeleteComponent);
		//p->registerProtocolFunc(Echo::PFID_Actor_GetActorID, &EchoActorProFuncImp::ProCom_GetActorID);
		//p->registerProtocolFunc(Echo::PFID_Actor_ChangeID, &EchoActorProFuncImp::ProCom_ChangeActorID);
		//p->registerProtocolFunc(Echo::PFID_Actor_SetVisiable, &EchoActorProFuncImp::ProCom_SetVisiable);
		//p->registerProtocolFunc(Echo::PFID_Actor_GetVisiable, &EchoActorProFuncImp::ProCom_GetVisiable);
		//p->registerProtocolFunc(Echo::PFID_Actor_GetPosition, &EchoActorProFuncImp::ProCom_GetActorPos);
		//p->registerProtocolFunc(Echo::PFID_Actor_GetRotation, &EchoActorProFuncImp::ProCom_GetActorRot);
		//p->registerProtocolFunc(Echo::PFID_Actor_GetEulerAngles, &EchoActorProFuncImp::ProCom_GetActorAngles);
		//p->registerProtocolFunc(Echo::PFID_Actor_GetScale, &EchoActorProFuncImp::ProCom_GetActorScale);
		//p->registerProtocolFunc(Echo::PFID_Actor_GetFilePath, &EchoActorProFuncImp::ProCom_GetActorFilePath);
		//p->registerProtocolFunc(Echo::PFID_Actor_SaveFile, &EchoActorProFuncImp::ProCom_SaveActorFile);
		//p->registerProtocolFunc(Echo::PFID_Actor_SaveAsFile, &EchoActorProFuncImp::ProCom_SaveAsActorFile);
		//p->registerProtocolFunc(Echo::PFID_Actor_AddComponent, &EchoActorProFuncImp::ProCom_ActorAddComponent);
		//p->registerProtocolFunc(Echo::PFID_Actor_RemoveComponent, &EchoActorProFuncImp::ProCom_ActorRemoveComponent);
		//p->registerProtocolFunc(Echo::PFID_Actor_IsEditorChange, &EchoActorProFuncImp::ProCom_IsEditorChange);
		//p->registerProtocolFunc(Echo::PFID_Actor_GetComponents, &EchoActorProFuncImp::ProCom_ActorGetComponents);
	}

	void EchoActorProFuncImp::qtNotInitProtocolFunction()
	{
		auto p = Echo::ProtocolComponentSystem::instance();
		//p->registerProtocolFunc(Echo::PFID_Actor_Create, &EchoActorProFuncImp::ProCom_CreateActor);
		//p->registerProtocolFunc(Echo::PFID_Actor_Delete, &EchoActorProFuncImp::ProCom_DeleteActor);
		p->registerProtocolFunc(Echo::PFID_Actor_SetPosition, &EchoActorProFuncImp::ProCom_SetActorPos);
		p->registerProtocolFunc(Echo::PFID_Actor_SetRotation, &EchoActorProFuncImp::ProCom_SetActorRot);
		p->registerProtocolFunc(Echo::PFID_Actor_SetEulerAngles, &EchoActorProFuncImp::ProCom_SetActorAngles);
		p->registerProtocolFunc(Echo::PFID_Actor_SetScale, &EchoActorProFuncImp::ProCom_SetActorScale);
		p->registerProtocolFunc(Echo::PFID_Actor_LoadFile, &EchoActorProFuncImp::ProCom_LoadActorFile);
		p->registerProtocolFunc(Echo::PFID_SceneNode_Create, &EchoActorProFuncImp::ProCom_CreateSceneNode);
		p->registerProtocolFunc(Echo::PFID_SceneNode_Delete, &EchoActorProFuncImp::ProCom_DeleteSceneNode);

		p->registerProtocolFunc(Echo::PFID_SceneActor_Create, &EchoActorProFuncImp::ProCom_CreateEditorActor);
		p->registerProtocolFunc(Echo::PFID_SceneActor_Delete, &EchoActorProFuncImp::ProCom_DeleteEditorActor);
		p->registerProtocolFunc(Echo::PFID_Actor_GetEditorActor, &EchoActorProFuncImp::ProCom_GetEditorActorFromActor);
		p->registerProtocolFunc(Echo::PFID_EditorActor_GetActor, &EchoActorProFuncImp::ProCom_GetActorFromEditorActor);

		
	}

	//=============================================================================
	void EchoActorProFuncImp::uninitProtocolFunction()
	{
		//auto p = ProtocolComponentManager::instance();
	}

	//Echo::ProtocolResult EchoActorProFuncImp::ProCom_CreateComponent(void * handle, Echo::ProtocolHandle typeHandle)
	//{
	//	if (typeHandle == 0)
	//		return Echo::PR_OK;
	//	Name* comType = (Name*)typeHandle;
	//	ActorComFactory* pFactory = ActorComFactoryManager::getSingleton().GetActorComFactory(*comType);
	//	if (NULL == pFactory)
	//		return Echo::PR_OK;
	//	/*	ActorComponentPtr  pComponent = pFactory->create(NULL);
	//		*(Echo::ProtocolHandle*)handle = (Echo::ProtocolHandle)pComponent;*/
	//	return Echo::PR_OK;
	//}

	//Echo::ProtocolResult EchoActorProFuncImp::ProCom_DeleteComponent(void * isDel, Echo::ProtocolHandle handle)
	//{
	//	if (handle == 0)
	//		return Echo::PR_OK;
	//	ActorComponentPtr  pComponent = (ActorComponentPtr )handle;
	//	if (!pComponent->hasActor())
	//	{
	//		*(bool*)isDel = false;
	//		return Echo::PR_OK;
	//	}
	//	*(bool*)isDel = true;
	//	return Echo::PR_OK;
	//}

	//Echo::ProtocolResult EchoActorProFuncImp::ProCom_CreateActor(void * handle, Echo::ProtocolHandle descPtr)
	//{
	//	if (descPtr == 0)
	//		return Echo::PR_OK;
	//	Echo::ActorDesc* desc = (Echo::ActorDesc*)descPtr;
	//	Echo::ActorPtr  actor = new Echo::Actor();
	//	actor->init(desc->actorInfo);
	//	actor->setPosition(desc->initPos);
	//	*(Echo::ProtocolHandle*)handle = (Echo::ProtocolHandle)actor;
	//	return Echo::PR_OK;
	//}

	//Echo::ProtocolResult EchoActorProFuncImp::ProCom_DeleteActor(void * data, Echo::ProtocolHandle handle)
	//{
	//	if (handle == 0)
	//		return Echo::PR_OK;
	//	Echo::ActorPtr  actor = (Echo::ActorPtr )handle;
	//	if (actor == NULL)
	//		return Echo::PR_OK;
	//	Echo::ActorPtr  parentActor = actor->getParentActor();
	//	if (parentActor == NULL)
	//	{
	//		SAFE_DELETE(actor);
	//	}
	//	else
	//	{
	//		parentActor->delActor(actor);
	//	}
	//	return Echo::PR_OK;
	//}

	//Echo::ProtocolResult EchoActorProFuncImp::ProCom_GetActorID(void * id, Echo::ProtocolHandle handle)
	//{
	//	if (handle == 0)
	//		return Echo::PR_OK;
	//	Echo::ActorPtr  actor = (Echo::ActorPtr )handle;
	//	if (actor == NULL)
	//		return Echo::PR_OK;
	//	*(Name*)id = actor->getActorID();
	//	return Echo::PR_OK;
	//}

	//Echo::ProtocolResult EchoActorProFuncImp::ProCom_ChangeActorID(void * id, Echo::ProtocolHandle handle)
	//{
	//	if (handle == 0)
	//		return Echo::PR_OK;
	//	Echo::ActorPtr  actor = (Echo::ActorPtr )handle;
	//	if(actor->getActorInfo())
	//		actor->getActorInfo()->sName = (*(Name*)id).c_str();
	//	if (actor == NULL)
	//		return Echo::PR_OK;
	//	actor->setActorID(*(Name*)id);
	//	return Echo::PR_OK;
	//}

	//Echo::ProtocolResult EchoActorProFuncImp::ProCom_GetVisiable(void * visiable, Echo::ProtocolHandle handle)
	//{
	//	if (handle == 0)
	//		return Echo::PR_OK;
	//	Echo::ActorPtr  actor = (Echo::ActorPtr )handle;
	//	if (actor == NULL)
	//		return Echo::PR_OK;
	//	*(bool*)visiable = actor->getVisiable();
	//	return Echo::PR_OK;
	//}

	//Echo::ProtocolResult EchoActorProFuncImp::ProCom_SetVisiable(void * visiable, Echo::ProtocolHandle handle)
	//{
	//	if (handle == 0)
	//		return Echo::PR_OK;
	//	Echo::ActorPtr  actor = (Echo::ActorPtr )handle;
	//	if (actor == NULL)
	//		return Echo::PR_OK;
	//	bool* visiablePtr = (bool*)visiable;
	//	actor->setVisiable(*visiablePtr);
	//	return Echo::PR_OK;
	//}

	Echo::ProtocolResult EchoActorProFuncImp::ProCom_LoadActorFile(void * isFinish, Echo::ProtocolHandle handle)
	{
		if (handle == 0)
			return Echo::PR_OK;
		Echo::LoadResDesc* loadDesc = (Echo::LoadResDesc*)handle;
		if(loadDesc->handle == 0)
			return Echo::PR_OK;
		Echo::ActorPtr  actor = ((Actor*)loadDesc->handle)->weak_from_this();
		if (actor == nullptr)
			return Echo::PR_OK;
		Echo::Vector3 pos = actor->getPosition();
		*(bool*)isFinish = false;
		ActorInfoPtr actInfo = Echo::ActorInfoManager::instance()->loadInfo(Echo::Name(loadDesc->loadPath));
		if (!actInfo.isNull())
		{
			actor->reInit(actInfo);
			actor->setPosition(pos);
			*(bool*)isFinish = true;
		}
		return Echo::PR_OK;
	}

	/*Echo::ProtocolResult EchoActorProFuncImp::ProCom_SaveActorFile(void * isFinish, Echo::ProtocolHandle handle)
	{
		if (handle == 0)
			return Echo::PR_OK;
		Echo::ActorPtr  actor = (Echo::ActorPtr )handle;
		if (actor == NULL)
			return Echo::PR_OK;
		*(bool*)isFinish = actor->save();
		return Echo::PR_OK;
	}*/

	/*Echo::ProtocolResult EchoActorProFuncImp::ProCom_SaveAsActorFile(void * isFinish, Echo::ProtocolHandle handle)
	{
		if (handle == 0)
			return Echo::PR_OK;
		Echo::SaveAsDesc* saveAs = (Echo::SaveAsDesc*)handle;
		if (saveAs == NULL)
			return Echo::PR_OK;
		Echo::ActorPtr  actor = (Echo::ActorPtr )saveAs->handle;
		if (actor == NULL)
			return Echo::PR_OK;
		*(bool*)isFinish = actor->SaveAs(saveAs->SaveAsPath.c_str(), saveAs->updateHandlePath);
		return Echo::PR_OK;
	}*/

	//Echo::ProtocolResult EchoActorProFuncImp::ProCom_GetActorFilePath(void * path, Echo::ProtocolHandle handle)
	//{
	//	if (handle == 0)
	//		return Echo::PR_OK;
	//	Echo::ActorPtr  actor = (Echo::ActorPtr )handle;
	//	if (actor == NULL)
	//		return Echo::PR_OK;
	//	*(std::string*)path = actor->getFilePath();
	//	return Echo::PR_OK;
	//}

	/*Echo::ProtocolResult EchoActorProFuncImp::ProCom_ActorAddComponent(void * comHandle, Echo::ProtocolHandle handle)
	{
		if (handle == 0)
			return Echo::PR_OK;
		Echo::ActorComTypeDesc* addComTypeDesc = (Echo::ActorComTypeDesc*)handle;
		if (addComTypeDesc->actHandle == 0)
			return Echo::PR_OK;
		Echo::ActorPtr  actor = (Echo::ActorPtr )addComTypeDesc->actHandle;
		if (actor == NULL)
			return Echo::PR_OK;
		Echo::ActorComponentPtr  component = (Echo::ActorComponentPtr )(addComTypeDesc->comHandle);
		*(Echo::ProtocolHandle*)comHandle = (Echo::ProtocolHandle)actor->addComponent(addComTypeDesc->comType,addComTypeDesc->comName, component);
		
		return Echo::PR_OK;
	}*/

	/*Echo::ProtocolResult EchoActorProFuncImp::ProCom_ActorRemoveComponent(void * isRemove, Echo::ProtocolHandle handle)
	{
		if (handle == 0)
			return Echo::PR_OK;
		Echo::RemoveActorComHandleDesc* delComDesc = (Echo::RemoveActorComHandleDesc*)handle;
		if (delComDesc->actHandle == 0 || delComDesc->comHandle == 0)
			return Echo::PR_OK;
		Echo::ActorPtr  actor = (Echo::ActorPtr )delComDesc->actHandle;
		if (actor == NULL)
			return Echo::PR_OK;
		Echo::ActorComponentPtr  component = (Echo::ActorComponentPtr )delComDesc->comHandle;

		if (component == NULL)
			return Echo::PR_OK;
		*(bool*)isRemove = (Echo::ProtocolHandle)actor->delComponent(component);
		return Echo::PR_OK;
	}*/

	Echo::ProtocolResult EchoActorProFuncImp::ProCom_ActorGetComponent(void * comHandle, Echo::ProtocolHandle handle)
	{
		if (handle == 0)
			return Echo::PR_OK;
		Echo::ActorComTypeDesc* comTypeDesc = (Echo::ActorComTypeDesc*)handle;
		if (comTypeDesc->actHandle == 0)
			return Echo::PR_OK;
		Echo::ActorPtr  actor = ((Actor*)comTypeDesc->actHandle)->weak_from_this();
		if (actor == nullptr)
			return Echo::PR_OK;
		Echo::ProtocolHandle component = (Echo::ProtocolHandle)(actor->getComponent(comTypeDesc->comName).get());
		*(Echo::ProtocolHandle*)comHandle = component;
		return Echo::PR_OK;
	}

	Echo::ProtocolResult EchoActorProFuncImp::ProCom_SetActorComponentPos(void * pos, Echo::ProtocolHandle handle)
	{
		if (handle == 0)
			return Echo::PR_Falied;
		Echo::ActorComponentPtr  com = ((ActorComponent*)handle)->weak_from_this();
		if (com == nullptr)
			return Echo::PR_Falied;
		if (com->m_pActComInfo != nullptr)
		com->m_pActComInfo->SetPosition(*(Vector3*)pos);
		return Echo::PR_OK;
	}

	Echo::ProtocolResult EchoActorProFuncImp::ProCom_SetActorComponentRot(void * qua, Echo::ProtocolHandle handle)
	{
		if (handle == 0)
			return Echo::PR_Falied;
		Echo::ActorComponentPtr  com = ((ActorComponent*)handle)->weak_from_this();
		if (com == nullptr)
			return Echo::PR_Falied;
		if (com->getSceneNode() != nullptr)
			com.getDynamicCastWeakPtr<ComponentNode>()->setRotation(*(Quaternion*)qua);
		if (com->m_pActComInfo != nullptr)
		com->m_pActComInfo->SetEulerAngles(com->getEulerAngles());
		return Echo::PR_OK;
	}

	Echo::ProtocolResult EchoActorProFuncImp::ProCom_SetActorComponentAngles(void * angle, Echo::ProtocolHandle handle)
	{
		if (handle == 0)
			return Echo::PR_Falied;
		Echo::ActorComponentPtr  com = ((ActorComponent*)handle)->weak_from_this();
		if (com == nullptr)
			return Echo::PR_Falied;
		if (com->m_pActComInfo != nullptr)
		com->m_pActComInfo->SetEulerAngles(*(Vector3*)angle);
		return Echo::PR_OK;
	}

	Echo::ProtocolResult EchoActorProFuncImp::ProCom_SetActorComponentScale(void * scale, Echo::ProtocolHandle handle)
	{
		if (handle == 0)
			return Echo::PR_Falied;
		Echo::ActorComponentPtr  com = ((ActorComponent*)handle)->weak_from_this();
		if (com == nullptr)
			return Echo::PR_Falied;
		if (com->m_pActComInfo != nullptr)
		com->m_pActComInfo->SetScale(*(Vector3*)scale);
		return Echo::PR_OK;
	}

	Echo::ProtocolResult EchoActorProFuncImp::ProCom_SetActorComponentWorldPos(void * pos, Echo::ProtocolHandle handle)
	{
		if (handle == 0)
			return Echo::PR_Falied;
		Echo::ActorComponentPtr  com = ((ActorComponent*)handle)->weak_from_this();
		if (com == nullptr)
			return Echo::PR_Falied;
		Vector3 worldPos = *(Vector3*)pos;
		Vector3 localPos = com->getParentNodeCom()->convertWorldToLocalPosition(worldPos);
		if (com->m_pActComInfo != nullptr)
		com->m_pActComInfo->SetPosition(localPos);
		return Echo::PR_OK;
	}

	Echo::ProtocolResult EchoActorProFuncImp::ProCom_SetActorComponentWorldRot(void * qua, Echo::ProtocolHandle handle)
	{
		if (handle == 0)
			return Echo::PR_Falied;
		Echo::ActorComponentPtr  com = ((ActorComponent*)handle)->weak_from_this();
		if (com == nullptr)
			return Echo::PR_Falied;
		Quaternion localRot = com->getParentNodeCom()->convertWorldToLocalRotation(*(Quaternion*)qua);
		if(com->getSceneNode() != nullptr)
			com.getDynamicCastWeakPtr<ComponentNode>()->setRotation(localRot);
		if (com->m_pActComInfo != nullptr)
		com->m_pActComInfo->SetEulerAngles(com->getEulerAngles());
		return Echo::PR_OK;
	}

	Echo::ProtocolResult EchoActorProFuncImp::ProCom_SetActorComponentWorldScale(void * scale, Echo::ProtocolHandle handle)
	{
		if (handle == 0)
			return Echo::PR_Falied;
		Echo::ActorComponentPtr  com = ((ActorComponent*)handle)->weak_from_this();
		if (com == nullptr)
			return Echo::PR_Falied;
		Vector3 localScale = com->getParentNodeCom()->convertWorldToLocalScale(*(Vector3*)scale);
		if (com->m_pActComInfo != nullptr)
		com->m_pActComInfo->SetScale(localScale);
		return Echo::PR_OK;
	}

	/*Echo::ProtocolResult EchoActorProFuncImp::ProCom_ActorGetComponents(void * actorComList, Echo::ProtocolHandle handle)
	{
		if (handle == 0)
			return Echo::PR_OK;
		Echo::ActorPtr  actor = (Echo::ActorPtr )handle;
		if (actor == NULL)
			return Echo::PR_OK;
		ActorComList list;
		for (auto& iter : actor->getComponents())
		{
			ActorComHandleDesc desc;
			desc.comHandle = (ProtocolHandle)iter;
			desc.comType = iter->getType();
			desc.comName = iter->m_pActComInfo->sComName;
			desc.comCommentary = iter->m_pActComInfo->sCommentary;
			list.push_back(desc);
		}
		*(ActorComList*)actorComList = list;
		return Echo::PR_OK;
	}*/

	Echo::ProtocolResult EchoActorProFuncImp::ProCom_SetActorPos(void * pos, Echo::ProtocolHandle handle)
	{
		if (handle == 0)
			return Echo::PR_OK;
		Echo::ActorPtr  actor = ((Actor* )handle)->weak_from_this();
		if (actor == nullptr)
			return Echo::PR_OK;
		Vector3* posPtr = (Vector3*)pos;
		actor->setPosition(*posPtr);
		return Echo::PR_OK;
	}

	Echo::ProtocolResult EchoActorProFuncImp::ProCom_SetActorRot(void * qua, Echo::ProtocolHandle handle)
	{
		if (handle == 0)
			return Echo::PR_OK;
		Echo::ActorPtr  actor = ((Actor*)handle)->weak_from_this();
		if (actor == nullptr)
			return Echo::PR_OK;
		Quaternion* quaPtr = (Quaternion*)qua;
		actor->setRotation(*quaPtr);
		return Echo::PR_OK;
	}

	Echo::ProtocolResult EchoActorProFuncImp::ProCom_SetActorAngles(void * angle, Echo::ProtocolHandle handle)
	{
		if (handle == 0)
			return Echo::PR_OK;
		Echo::ActorPtr  actor = ((Actor*)handle)->weak_from_this();
		if (actor == nullptr)
			return Echo::PR_OK;
		Vector3* anglePtr = (Vector3*)angle;
		actor->setEulerAngles(*anglePtr);
		return Echo::PR_OK;
	}

	Echo::ProtocolResult EchoActorProFuncImp::ProCom_SetActorScale(void * scale, Echo::ProtocolHandle handle)
	{
		if (handle == 0)
			return Echo::PR_OK;
		Echo::ActorPtr  actor = ((Actor*)handle)->weak_from_this();
		if (actor == nullptr)
			return Echo::PR_OK;
		Vector3* scalePtr = (Vector3*)scale;
		actor->setScale(*scalePtr);
		return Echo::PR_OK;
	}

	/*Echo::ProtocolResult EchoActorProFuncImp::ProCom_IsEditorChange(void * change, Echo::ProtocolHandle handle)
	{
		if (handle == 0)
			return Echo::PR_OK;
		Echo::ActorPtr  actor = (Echo::ActorPtr )handle;
		if (actor == NULL)
			return Echo::PR_OK;
		*(bool*)change = actor->IsEditorChange();
		return Echo::PR_OK;
	}*/

	Echo::ProtocolResult EchoActorProFuncImp::ProCom_CreateSceneNode(void * editor, Echo::ProtocolHandle handle)
	{
		SceneManager * pMainSceneMgr = Root::instance()->getMainSceneManager();
		if (pMainSceneMgr)
		{
			Echo::SceneNode* node = pMainSceneMgr->createSceneNode();
			*(Echo::ProtocolHandle*)editor = (Echo::ProtocolHandle)(node);
			return Echo::PR_OK;
		}

		return Echo::PR_Falied;
	}

	Echo::ProtocolResult EchoActorProFuncImp::ProCom_DeleteSceneNode(void * editor, Echo::ProtocolHandle handle)
	{
		SceneManager * pMainSceneMgr = Root::instance()->getMainSceneManager();
		if (pMainSceneMgr)
		{
			pMainSceneMgr->destroySceneNode((Echo::SceneNode*)editor);
			return Echo::PR_OK;
		}

		return Echo::PR_Falied;
	}

	Echo::ProtocolResult EchoActorProFuncImp::ProCom_CreateEditorActor(void * handle, Echo::ProtocolHandle descPtr)
	{
		Echo::ActorDesc* desc = (Echo::ActorDesc*)descPtr;
		Name tempName = Echo::Root::instance()->getActiveSceneManager()->getName();
		Echo::ActorPtr  actor = Echo::ActorSystem::instance()->getActorManager(tempName)->createActor(Name(desc->actorFile), desc->actorName, true);
		if (actor == nullptr)
			return Echo::PR_Falied;
		actor->setPosition(desc->initPos);
		actor->getActorInfo()->getRootComInfo()->SetPosition(desc->initPos);
		*(Echo::ProtocolHandle*)handle = (Echo::ProtocolHandle)(actor.get());
		return Echo::PR_OK;
	}

	Echo::ProtocolResult EchoActorProFuncImp::ProCom_DeleteEditorActor(void * nullHandle, Echo::ProtocolHandle handle)
	{
		if (handle == 0)
			return Echo::PR_Falied;
		Echo::ActorPtr  actor = ((Actor*)handle)->weak_from_this();
		if (actor == nullptr)
			return Echo::PR_Falied;
		//std::string sceneActorFile = Echo::Root::instance()->getRootResourcePath() + actor->getFilePath();
		//remove(sceneActorFile.c_str());
		//Echo::ActorSystem::instance()->getMainActorManager()->deleteActor(actor);
		actor->getActorManager()->deleteActor(actor);
		return Echo::PR_OK;
	}

	Echo::ProtocolResult EchoActorProFuncImp::ProCom_GetEditorActorFromActor(void * editorActHandle, Echo::ProtocolHandle handle)
	{
		if (handle == 0)
		{
			*(Echo::ProtocolHandle*)editorActHandle = 0;
			return Echo::PR_OK;
		}	
		Echo::ActorPtr  actor = ((Actor*)handle)->weak_from_this();
		if (actor == nullptr)
		{
			*(Echo::ProtocolHandle*)editorActHandle = 0;
			return Echo::PR_OK;
		}
		*(Echo::ProtocolHandle*)editorActHandle = (Echo::ProtocolHandle)(actor.get());
		return Echo::PR_OK;
	}

	Echo::ProtocolResult EchoActorProFuncImp::ProCom_GetActorFromEditorActor(void * actHandle, Echo::ProtocolHandle handle)
	{
		*(Echo::ProtocolHandle*)actHandle = 0;
		if (handle == 0)
			return Echo::PR_OK;
		ActorPtr  actor = ((Actor*)handle)->weak_from_this();
		if (actor == nullptr)
			return Echo::PR_OK;
		*(Echo::ProtocolHandle*)actHandle = (Echo::ProtocolHandle)(actor.get());
		return Echo::PR_OK;
	}
	//=============================================================================
} // namespace Echo

  //-----------------------------------------------------------------------------
  //	CPP File End
  //-----------------------------------------------------------------------------
