//-----------------------------------------------------------------------------
// File:  EchoActorProFuncImp.h
//
// Author: wuyunlong
//
// Date:   2022-5-20
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#ifndef __EchoActorProFuncImp_H__
#define __EchoActorProFuncImp_H__

#include "EchoComponentPrerequisites.h"
#include "EchoProtocolComponent.h"
#include "EchoProtocolComponentImp.h"
#include "Actor/EchoActor.h"

//-----------------------------------------------------------------------------
//	Head File Begin
//-----------------------------------------------------------------------------

namespace Echo
{
	struct ActorComTypeDesc
	{
		ProtocolHandle	actHandle;
		ProtocolHandle	comHandle;
		Name			comType;
		std::string		comName;
		ActorComTypeDesc()
		{
			actHandle = 0;
			comHandle = 0;
		}
	};

	struct RemoveActorComHandleDesc
	{
		ProtocolHandle	actHandle;
		ProtocolHandle	comHandle;
		RemoveActorComHandleDesc()
		{
			actHandle = 0;
			comHandle = 0;
		}
	};

	struct ActorComHandleDesc
	{
		ProtocolHandle	comHandle;
		Name			comType;
		std::string		comName;
		std::string		comCommentary;
		ActorComHandleDesc()
		{
			comHandle = 0;
		}
	};

	typedef std::vector<ActorComHandleDesc> ActorComList;
	typedef std::unordered_map<ProtocolHandle, Name> ChildActorMap;
	//留下的接口基本都是Qt和运行时有差异的接口。修改谨慎修改。
	class _EchoComponentExport EchoActorProFuncImp
	{
	public:
		REGISTER_LUA_CLASS_KEY;
		//-----------------------------------------------------------------------------
		static void									initProtocolFunction();
		static void									qtNotInitProtocolFunction();
		static void									uninitProtocolFunction();

		//static Echo::ProtocolResult				ProCom_CreateComponent(void* handle, Echo::ProtocolHandle typeHandle);
		//static Echo::ProtocolResult				ProCom_DeleteComponent(void* isDel, Echo::ProtocolHandle handle);

		//-----------------------------------------------------------------------------
		//static Echo::ProtocolResult				ProCom_CreateActor(void* handle, Echo::ProtocolHandle descPtr);
		//static Echo::ProtocolResult				ProCom_DeleteActor(void* data, Echo::ProtocolHandle handle);
		//static Echo::ProtocolResult				ProCom_GetActorID(void* id, Echo::ProtocolHandle handle);
		//static Echo::ProtocolResult				ProCom_ChangeActorID(void* id, Echo::ProtocolHandle handle);
		//static Echo::ProtocolResult				ProCom_GetVisiable(void* visiable, Echo::ProtocolHandle handle);
		//static Echo::ProtocolResult				ProCom_SetVisiable(void* visiable, Echo::ProtocolHandle handle);

		static Echo::ProtocolResult					ProCom_LoadActorFile(void* isFinish, Echo::ProtocolHandle handle);
		//static Echo::ProtocolResult				ProCom_SaveActorFile(void* isFinish, Echo::ProtocolHandle handle);
		//static Echo::ProtocolResult				ProCom_SaveAsActorFile(void* isFinish, Echo::ProtocolHandle handle);
		//static Echo::ProtocolResult				ProCom_GetActorFilePath(void* path, Echo::ProtocolHandle handle);

		//static Echo::ProtocolResult				ProCom_ActorAddComponent(void* comHandle, Echo::ProtocolHandle handle);
		//static Echo::ProtocolResult				ProCom_ActorRemoveComponent(void* isRemove, Echo::ProtocolHandle handle);
		static Echo::ProtocolResult					ProCom_ActorGetComponent(void* comHandle, Echo::ProtocolHandle handle);
		//static Echo::ProtocolResult				ProCom_ActorGetComponents(void* actorComList, Echo::ProtocolHandle handle);
		static Echo::ProtocolResult					ProCom_SetActorComponentPos(void* pos, Echo::ProtocolHandle handle);
		static Echo::ProtocolResult					ProCom_SetActorComponentRot(void* qua, Echo::ProtocolHandle handle);
		static Echo::ProtocolResult					ProCom_SetActorComponentAngles(void* angle, Echo::ProtocolHandle handle);
		static Echo::ProtocolResult					ProCom_SetActorComponentScale(void* scale, Echo::ProtocolHandle handle);
		static Echo::ProtocolResult					ProCom_SetActorComponentWorldPos(void* pos, Echo::ProtocolHandle handle);
		static Echo::ProtocolResult					ProCom_SetActorComponentWorldRot(void* qua, Echo::ProtocolHandle handle);
		static Echo::ProtocolResult					ProCom_SetActorComponentWorldScale(void* scale, Echo::ProtocolHandle handle);

		//static Echo::ProtocolResult				ProCom_ActorAddChild(void* childHandle, Echo::ProtocolHandle handle);
		//static Echo::ProtocolResult				ProCom_ActorRemoveChild(void* isRemove, Echo::ProtocolHandle handle);
		//static Echo::ProtocolResult				ProCom_ActorGetChilds(void* actorsMap, Echo::ProtocolHandle handle);

		static Echo::ProtocolResult					ProCom_SetActorPos(void* pos, Echo::ProtocolHandle handle);
		//static Echo::ProtocolResult				ProCom_GetActorPos(void* pos, Echo::ProtocolHandle handle);
		static Echo::ProtocolResult					ProCom_SetActorRot(void* qua, Echo::ProtocolHandle handle);
		//static Echo::ProtocolResult				ProCom_GetActorRot(void* qua, Echo::ProtocolHandle handle);
		static Echo::ProtocolResult					ProCom_SetActorAngles(void* angle, Echo::ProtocolHandle handle);
		//static Echo::ProtocolResult				ProCom_GetActorAngles(void* angle, Echo::ProtocolHandle handle);
		static Echo::ProtocolResult					ProCom_SetActorScale(void* scale, Echo::ProtocolHandle handle);
		//static Echo::ProtocolResult				ProCom_GetActorScale(void* scale, Echo::ProtocolHandle handle);

		//static Echo::ProtocolResult				ProCom_IsEditorChange(void* change, Echo::ProtocolHandle handle);
		static Echo::ProtocolResult					ProCom_CreateSceneNode(void* editor, Echo::ProtocolHandle handle);
		static Echo::ProtocolResult					ProCom_DeleteSceneNode(void* editor, Echo::ProtocolHandle handle);

		static Echo::ProtocolResult					ProCom_CreateEditorActor(void* handle, Echo::ProtocolHandle descPtr);
		static Echo::ProtocolResult					ProCom_DeleteEditorActor(void* nullHandle, Echo::ProtocolHandle handle);
		static Echo::ProtocolResult					ProCom_GetEditorActorFromActor(void* editorActHandle, Echo::ProtocolHandle handle);
		static Echo::ProtocolResult					ProCom_GetActorFromEditorActor(void* actHandle, Echo::ProtocolHandle handle);
	};

} // namespace Echo

  //-----------------------------------------------------------------------------
  //	Head File End
  //-----------------------------------------------------------------------------
#endif

