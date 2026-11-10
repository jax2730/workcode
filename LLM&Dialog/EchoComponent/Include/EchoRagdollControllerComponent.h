//-----------------------------------------------------------------------------
// File:  EchoRagdollComponent.h
//
// Author: LangGuangyue
//
// Date:   2022-10-21
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef _EchoRagdollControllerComponent_H__
#define _EchoRagdollControllerComponent_H__

#include "EchoComponentPrerequisites.h"
#include "Actor/EchoActorComponent.h"
#include "Actor/EchoActor.h"
#include "EchoActorRoleComponent.h"
#include "EchoEnginePrerequisites.h"
#include "EchoRagdollComponent.h"
#include "EchoRagdollMuscle.h"
#include "EchoRagdollTools.h"
#include "EchocJsonHelper.h"
#include "EchoComDefineMacro.h"

namespace Echo
{
	class RagdollControllerComponentInfo;
	//---------------------------------------------------------------------------------
	class _EchoComponentExport RagdollControllerComponent : public ProtocolNodeActorComponent
	{
		DECLAREACTORCOMTYPE_H(RagdollControllerComponent)
	public:
		virtual void						init(ActorComponentInfo* info) override;
		virtual void						bindNode() override;
		virtual void						replay(ActorComponentInfo* info) override;
		virtual void						connectCom() override;
		virtual	void						setActor(const ActorPtr& pActor) override;
		virtual void						copyTo(const ActorComponentPtr& actCom) override;
		virtual void						dealComponentDelete(const ActorComponentPtr& actCom) override;

		virtual void						registerTickTimeBegin() override;
		virtual void						beginFrame(EventID, ProtocolHandle handle) override;
		virtual void						endFrame(EventID, ProtocolHandle handle) override {};
		virtual void						preUpdateAnimation(EventID, ProtocolHandle handle) override {};
		virtual void						postUpdateAnimation(EventID, ProtocolHandle handle) override {}
		virtual void						preUpdateScene(EventID, ProtocolHandle handle) override {};
		virtual void						postUpdateScene(EventID, ProtocolHandle handle) override {}
		virtual void						preRenderScene(EventID, ProtocolHandle handle) override {};
		virtual void						postRenderScene(EventID, ProtocolHandle handle) override {};
		virtual void						ResetRegisterFuns() {};
		//------------------------------------------------------------Void------------------------------------------------------
	public:
		void								GetRoleList();
		void								GetRagdollList();

		bool								BeginNext() { return GetRagdollCom() && GetRoleCom(); };

		RoleComponent*						GetRoleCom();
		RagdollComponent*					GetRagdollCom();

		//-----------------------------------------------------------Data-------------------------------------------------------
	public:
		RagdollControllerComponentInfo*		mInfo;

		std::vector<std::string>			mRoleList;
		std::vector<std::string>			mRagdollList;
		std::string							mRoleName = "";
		std::string							mRagdollName = "";

		const int							mDeathKey = 549;	//Keyboard:"D"
		const int							mResetKey = 563;	//Keyboard:"R"
		bool								mIsDeath = false;
		bool								mIsReset = false;

		std::string							mAnimationName = "";

	};
	//---------------------------------------------------------------------------------
	class _EchoComponentExport RagdollControllerComponentInfo : public ActorComponentInfo
	{
		friend class RagdollControllerComponent;
	public:
		RagdollControllerComponentInfo(const Name& mtype, ActorCOMMoveableType movableType = ALL_MOVEABLE);
		RagdollControllerComponentInfo();
		virtual ~RagdollControllerComponentInfo();
		virtual void						ExportData(cJSON* jsonNode)override;
		virtual void						ImportData(cJSON* jsonNode)override;
		virtual void						CopyTo(ActorComponentInfo* comInfo) override;
		virtual void						Overlay(ActorComponentInfo* comInfo) override {};
		virtual void						OverWrite() override;

	public:
		std::string		mRoleName		= "";
		std::string		mRagdollName	= "";
		std::string		mAnimationName	= "";
		EDITOR_REFLECT

	};
	DEFINECOMFACTORY(RagdollControllerComponent, ALL_MOVEABLE)
}

#endif // _EchoRagdollControllerComponent_H__
