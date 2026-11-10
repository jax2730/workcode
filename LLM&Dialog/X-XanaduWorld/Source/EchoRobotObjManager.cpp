//-----------------------------------------------------------------------------
// File:   EchoRobotObjManager.cpp
//
// Author: xiongbo
//
// Date:  2019-05-18
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#include "EchoStableHeaders.h"
#include "EchoRobotObjManager.h"

//#include "EchoSceneCommonInfoMgr.h"
#include "EchoSingleton.h"
#include "EchoMeshMgr.h"
#include "EchoRoot.h"
#include "Effect/EchoEffectManager.h"
#include "Effect/EchoEffectBase.h"
#include "EchoRoleObject.h"
#include "EchoRoleManager.h"
#include "EchoRobotObj.h"
//-----------------------------------------------------------------------------
//	CPP File Begin
//-----------------------------------------------------------------------------


	RobotObjManager::RobotObjManager()
		: m_pSceneManager(NULL)
		, m_isMove(true)
		, m_isPlayAnimation(true)
		, m_isPlayParticle(true)
	{
		//InitRobotResources();
	}

	RobotObjManager::~RobotObjManager()
	{
		m_RoleResources.clear();
		std::vector<RobotObj*>::iterator it, ite = m_Robots.end();
		for (it = m_Robots.begin(); it != ite; ++it)
		{
			delete (*it);
		}
		m_Robots.clear();
	}

	void RobotObjManager::SetSceneManager(Echo::SceneManager* pSceneManager)
	{
		m_pSceneManager = pSceneManager;
	}

	RobotObj* RobotObjManager::CreateRobotObject(const Echo::Vector3& inWorldPosi)
	{
		if (m_RoleResourceNum == 0)
		{
			return NULL;
		}

		int randomNo = 0;//Echo::Math::RangeRandom(0, (int)m_RoleResourceNum - 1);
		RobotObj* pRobot = new RobotObj(m_pSceneManager);
		pRobot->CreateRoleObj(Echo::Name("test"), m_RolePathVector[randomNo], inWorldPosi);
		//pRobot->ChangeEquip(m_RoleSkinName[randomNo]);
		//pRobot->ChangeRobotEntity(1, Name("role/common/common/mesh/00105_st_002a_h.mesh"))
		m_Robots.push_back(pRobot);
		////ÉíÌå
		//int headNo = Math::RangeRandom(0, m_RoleHeadNum - 1);
		//int hairNo = Math::RangeRandom(0, m_RoleHairNum - 1);
		//int STNo = Math::RangeRandom(0, m_RoleSTNum - 1);
		//int BZNo = Math::RangeRandom(0, m_RoleBZNum - 1);
		//int ShouNo = Math::RangeRandom(0, m_RoleShouNum - 1);

		//pRobot->ChangeRobotEntity(2, m_HeadList[headNo]);
		//pRobot->ChangeRobotEntity(3, m_HairList[hairNo]);
		//pRobot->ChangeRobotEntity(1, m_STList[STNo]);
		//pRobot->ChangeRobotEntity(20, m_BZList[BZNo]);
		//pRobot->ChangeRobotEntity(21, m_ShouList[ShouNo]);

		return pRobot;
	}

	void RobotObjManager::ClearAllRobotObject()
	{
		size_t num = m_Robots.size();
		for (size_t i = 0; i < num; i++)
		{
			delete m_Robots[i];
		}
		m_Robots.clear();
	}

	void RobotObjManager::Tick(const Echo::Vector3& inCameraPosi)
	{
		static unsigned int i = 0;

		for (size_t j = 0; j < m_Robots.size(); j++)
		{
			if (m_isMove)
			{
				m_Robots[j]->Move(inCameraPosi);
			}

			if (i % 100 == 0)
			{
				int upAnimationNo = Echo::Math::RangeRandom(0, (int)m_UpAnimationNum - 1);
				int downAnimationNo = Echo::Math::RangeRandom(0, (int)m_DownAnimationNum - 1);
				Echo::AnimationList temp;
				temp.push_back(m_UpAnimationNameVector[upAnimationNo]);
				temp.push_back(m_DownAnimationNameVector[downAnimationNo]);
				
				if (m_isPlayAnimation)
				{
					m_Robots[j]->PlayAnimation(temp);
				}
				else
				{
					m_Robots[j]->StopAnimation();
				}
				

				if (m_isPlayParticle)
				{
					int skillParticleNo = Echo::Math::RangeRandom(0, (int)m_SkillParticleNum - 1);
					m_Robots[j]->PlaySkillParticle(m_RoleSkillParticleName[skillParticleNo]);
				}
				else
				{
					m_Robots[j]->StopSkillParticle();
				}
				
				temp.clear();
			}
		}

		i = i + 3;
		if (i > 100000000)
		{
			i = 0;
		}
	}

	void RobotObjManager::InitRobotResources()
	{
		initRobotRoleResources();
		initRobotAnimations();
		initRobotParticleResources();
	}

	void RobotObjManager::SetMove(bool inValue)
	{
		m_isMove = inValue;
	}

	void RobotObjManager::SetPlayAnimation(bool inValue)
	{
		m_isPlayAnimation = inValue;
	}

	void RobotObjManager::SetPlayParticle(bool inValue)
	{
		m_isPlayParticle = inValue;
	}

	void RobotObjManager::CreateTestRobotObject(const Echo::Vector3& inWorldPosi, int robotNum)
	{
		int aa = (int)sqrt(robotNum);
		for (int i = 0; i < robotNum; i++)
		{
			int randomx = Echo::Math::RangeRandom(-aa, aa);
			int randomy = Echo::Math::RangeRandom(-aa, aa);
			int randomz = Echo::Math::RangeRandom(-aa, aa);
			Echo::Vector3 robotPosi = inWorldPosi + Echo::Vector3(randomx, randomy, randomz);
			CreateRobotObject(robotPosi);
		}
	}

	void RobotObjManager::initRobotRoleResources()
	{
		m_RolePathVector.clear();
		m_RolePathVector.push_back(Echo::Name("role/zj_001_nan/zj_001_nan_001/zj_001_nan_001.role"));
		//m_RoleSkinName.push_back(Echo::Name("001_h_ceshi"));
		//m_RolePathVector.push_back(Echo::Name("role/zj_001_nan/zj_001_nan_002/zj_001_nan_002.role"));
		//m_RoleSkinName.push_back(Echo::Name("002_h_ceshi"));
		//m_RolePathVector.push_back(Echo::Name("role/zj_001_nan/zj_001_nan_003/zj_001_nan_003.role"));
		//m_RoleSkinName.push_back(Echo::Name("004_h_ceshi"));

		//m_RolePathVector.push_back(Echo::Name("role/zj_002_nv/zj_002_nv_001/zj_002_nv_001.role"));
		//m_RoleSkinName.push_back(Echo::Name("001_h_ceshi"));
		//m_RolePathVector.push_back(Echo::Name("role/zj_002_nv/zj_002_nv_002/zj_002_nv_002.role"));
		//m_RoleSkinName.push_back(Echo::Name("002_h_ceshi"));
		//m_RolePathVector.push_back(Echo::Name("role/zj_002_nv/zj_002_nv_003/zj_002_nv_003.role"));
		//m_RoleSkinName.push_back(Echo::Name("004_h_ceshi"));

		m_RoleResourceNum = m_RolePathVector.size();
		for (size_t i = 0; i < m_RoleResourceNum; i++)
		{
			Echo::RolePtr roleResource = (Echo::RolePtr)(Echo::RoleManager::instance()->load(m_RolePathVector[i]));
			m_RoleResources.push_back(roleResource);
		}
	}

	void RobotObjManager::initRobotAnimations()
	{
		Echo::AnimationDesc temp;
		temp.animationName = Echo::Name("xr_xl_01_ca");
		temp.time = 3.0;
		temp.weight = 1.0;
		m_UpAnimationNameVector.push_back(temp);

		temp.animationName = Echo::Name("zd_gj_dm_jn_01_ca");
		temp.time = 2.0;
		temp.weight = 1.0;
		m_UpAnimationNameVector.push_back(temp);

		temp.animationName = Echo::Name("ck_ty_03_ca");
		temp.time = 3.0;
		temp.weight = 1.0;
		m_UpAnimationNameVector.push_back(temp);

		temp.animationName = Echo::Name("ck_xx_01_ca");
		temp.time = 4.0;
		temp.weight = 1.0;
		m_UpAnimationNameVector.push_back(temp);

		temp.animationName = Echo::Name("ck_xx_02_ca");
		temp.time = 5.0;
		temp.weight = 1.0;
		m_UpAnimationNameVector.push_back(temp);

		temp.animationName = Echo::Name("xr_xl_01_tk");
		temp.time = 3.0;
		temp.weight = 1.0;
		m_DownAnimationNameVector.push_back(temp);

		temp.animationName = Echo::Name("zd_gj_dm_jn_01_tk");
		temp.time = 2.0;
		temp.weight = 1.0;
		m_DownAnimationNameVector.push_back(temp);

		temp.animationName = Echo::Name("ck_ty_03_tk");
		temp.time = 3.0;
		temp.weight = 1.0;
		m_DownAnimationNameVector.push_back(temp);

		temp.animationName = Echo::Name("ck_xx_01_tk");
		temp.time = 4.0;
		temp.weight = 1.0;
		m_DownAnimationNameVector.push_back(temp);

		temp.animationName = Echo::Name("ck_xx_02_tk");
		temp.time = 5.0;
		temp.weight = 1.0;
		m_DownAnimationNameVector.push_back(temp);

		m_UpAnimationNum = m_UpAnimationNameVector.size();
		m_DownAnimationNum = m_DownAnimationNameVector.size();
	}

	void RobotObjManager::initRobotParticleResources()
	{
		m_RoleSkillParticleName.push_back(Echo::Name("gfx/pef/jn_ld_taidao_a_001_1.pef"));
		m_RoleSkillParticleName.push_back(Echo::Name("gfx/pef/jn_ld_taidao_piaodai_001.pef"));
		m_RoleSkillParticleName.push_back(Echo::Name("gfx/pef/jn_ld_taidao_piaodai_001.pef"));
		m_RoleSkillParticleName.push_back(Echo::Name("gfx/pef/jn_ld_taidao_b_001.pef"));
		m_RoleSkillParticleName.push_back(Echo::Name("gfx/pef/jn_ld_taidao_pojia_001.pef"));
		m_RoleSkillParticleName.push_back(Echo::Name("gfx/pef/jn_ld_taidao_e_01.pef"));
		m_RoleSkillParticleName.push_back(Echo::Name("gfx/pef/jn_hz_taidao_001.pef"));
		m_RoleSkillParticleName.push_back(Echo::Name("gfx/pef/jn_hz_taidao_bd_001.pef"));
		m_RoleSkillParticleName.push_back(Echo::Name("gfx/pef/jn_hz_taidao_sd_003.pef"));
		m_RoleSkillParticleName.push_back(Echo::Name("gfx/pef/jn_hz_fuzi_dg_001.pef"));
		m_RoleSkillParticleName.push_back(Echo::Name("gfx/pef/jn_hz_taidao_sd_004.pef"));
		m_RoleSkillParticleName.push_back(Echo::Name("gfx/pef/jn_hz_taidao_badao_001.pef"));
		m_RoleSkillParticleName.push_back(Echo::Name("gfx/pef/jn_hz_taidao_bd_002.pef"));
		m_RoleSkillParticleName.push_back(Echo::Name("gfx/pef/jn_ld_zhonguojian_daopiaodai_01.pef"));
		m_RoleSkillParticleName.push_back(Echo::Name("gfx/pef/jn_ld_taidao_piaodai_001.pef"));
		
		m_SkillParticleNum = m_RoleSkillParticleName.size();
	}

//-----------------------------------------------------------------------------
//	Cpp File End
//-----------------------------------------------------------------------------
