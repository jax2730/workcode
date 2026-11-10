//-----------------------------------------------------------------------------
// File:   EchoRobotObjManager.h
//
// Author: xiongbo
//
// Date:  2019-05-17
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#ifndef __EchoRobotObjManager_H__
#define __EchoRobotObjManager_H__
//-----------------------------------------------------------------------------
//	Head File Begin
//-----------------------------------------------------------------------------
#include "EchoEnginePrerequisites.h"
#include "EchoWorldManager.h"
#include "EchoSceneNode.h"
#include "EchoResourceBackgroundQueue.h"
#include "EchoRoleObject.h"
#include "EchoSingleton.h"
#include "EchoRole.h"
class RobotObj;

	class RobotObjManager :public Echo::Singleton<RobotObjManager>
	{
	public:
		RobotObjManager();
		~RobotObjManager();

		void SetSceneManager(Echo::SceneManager* pSceneManager);

		RobotObj* CreateRobotObject(const Echo::Vector3& inWorldPosi);
		
		void ClearAllRobotObject();
		void Tick(const Echo::Vector3& inCameraPosi);

		void InitRobotResources();

		void SetMove(bool inValue);
		void SetPlayAnimation(bool inValue);
		void SetPlayParticle(bool inValue);

		//Test
		void CreateTestRobotObject(const Echo::Vector3& inWorldPosi, int robotNum);
	private:

		void initRobotRoleResources();
		void initRobotAnimations();
		void initRobotParticleResources();

	private:
		//std::vector<Name> m_AnimationNameVector;
		std::vector<Echo::AnimationDesc> m_UpAnimationNameVector;
		std::vector<Echo::AnimationDesc> m_DownAnimationNameVector;
		std::vector<Echo::Name> m_RolePathVector;
		std::vector<Echo::Name> m_RoleSkinName;
		std::vector<Echo::Name> m_RoleSkillParticleName;
		//std::vector<Name> m_HeadList;			//头	2
		//std::vector<Name> m_HairList;			//头发	3
		//std::vector<Name> m_STList;				//身体	1
		//std::vector<Name> m_BZList;				//脖子	20
		//std::vector<Name> m_ShouList;			//手	21
		
		size_t m_RoleResourceNum;
		size_t m_UpAnimationNum;
		size_t m_DownAnimationNum;
		size_t m_SkillParticleNum;
		//size_t m_RoleHeadNum;
		//size_t m_RoleHairNum;
		//size_t m_RoleSTNum;
		//size_t m_RoleBZNum;
		//size_t m_RoleShouNum;

		std::vector<Echo::RolePtr> m_RoleResources;
		std::vector<RobotObj*> m_Robots;
		Echo::SceneManager* m_pSceneManager;
		bool m_isMove;
		bool m_isPlayAnimation;
		bool m_isPlayParticle;
	};

//-----------------------------------------------------------------------------
//	Head File End
//-----------------------------------------------------------------------------
#endif