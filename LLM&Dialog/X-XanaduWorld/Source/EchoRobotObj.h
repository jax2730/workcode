//-----------------------------------------------------------------------------
// File:   EchoRobotObj.h
//
// Author: xiongbo
//
// Date:  2019-05-17
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#ifndef __EchoRobotObj_H__
#define __EchoRobotObj_H__
//-----------------------------------------------------------------------------
//	Head File Begin
//-----------------------------------------------------------------------------
#include "EchoEnginePrerequisites.h"
#include "EchoSceneNode.h"
#include "EchoResourceBackgroundQueue.h"
#include "EchoRoleObject.h"


	class RobotObj
	{
	public:
		RobotObj(Echo::SceneManager* pSceneManager);
		~RobotObj();

		void CreateRoleObj(const Echo::Name& roleName, const Echo::Name& roleRes, const Echo::Vector3& position);

		void PlayAnimation(const Echo::AnimationList& pList);
		void StopAnimation();

		void PlaySkillParticle(const Echo::Name& particleName);
		void StopSkillParticle();
		
		void Move(const Echo::Vector3& inCameraPosi);		//行走移动旋转

		void ChangeRobotEntity(Echo::uint8 iPart, const Echo::Name& meshName);
		//切换装备
		void ChangeEquip(const Echo::Name& equipName);
	private:
		Echo::SceneManager* m_pSceneManager;
		Echo::RoleObject* m_pRoleObject;
		std::vector<Echo::LocatorStruct*> mLocatorStructVector;
		//std::vector<Echo::EffectHolderPtr> mParticles;
		size_t m_LocatorNum;
		float xRange;
		float zRange;
		float xMove;
		float zMove;
		float xStep;
		float zStep;
		Echo::Vector3 mPosi;		
	};

//-----------------------------------------------------------------------------
//	Head File End
//-----------------------------------------------------------------------------
#endif
