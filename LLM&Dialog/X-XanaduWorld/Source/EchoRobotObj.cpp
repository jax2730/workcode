//-----------------------------------------------------------------------------
// File:   EchoRobotObj.cpp
//
// Author: xiongbo
//
// Date:  2019-5-18
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#include "EchoStableHeaders.h"
#include "EchoRobotObj.h"
//#include "EchoSceneCommonInfoMgr.h"
#include "EchoSingleton.h"
#include "EchoMeshMgr.h"
#include "EchoRoot.h"
#include "Effect/EchoEffectManager.h"
#include "Effect/EchoEffectBase.h"
#include "EchoRoleObject.h"
#include "EchoMath.h"
#include "EchoAutoObjectManager.h"
//-----------------------------------------------------------------------------
//	CPP File Begin
//-----------------------------------------------------------------------------

	RobotObj::RobotObj(Echo::SceneManager* pSceneManager)
		: m_pSceneManager(pSceneManager)
		, m_pRoleObject(NULL)
		, xRange(10.0)
		, zRange(-10.0)
		, xMove(0.0)
		, zMove(0.0)
		, xStep(1.0)
		, zStep(1.0)
	{
	}

	RobotObj::~RobotObj()
	{
		StopAnimation();
		StopSkillParticle();
		if (NULL != m_pRoleObject)
		{
			delete m_pRoleObject;
			m_pRoleObject = NULL;
		}
	}


	void RobotObj::CreateRoleObj(const Echo::Name& roleName, const Echo::Name& roleRes, const Echo::Vector3& position)
	{
		//mPosi = position;
		//m_pRoleObject = Echo::AutoObjectManager::instance()->createRoleObj(roleName, roleRes, m_pSceneManager, true, Echo::Vector3::ZERO, Echo::Quaternion::IDENTITY, Echo::Vector3::UNIT_SCALE, true, Echo::Normal_NPC_Logic, false, true, false);//new Echo::RoleObject(roleName, roleRes, m_pSceneManager, true);
		//m_pRoleObject->getAnimationComponent()->setAnimationUpdateManual(true);
		//m_pRoleObject->setPosition(position);
		//m_pRoleObject->setRoleObjectLogicType(Echo::Normal_NPC_Logic);
		//Echo::RolePtr	roleResource = m_pRoleObject->getSkeletonComponent()->getRoleInfo();
		//roleResource->getLocatorList(mLocatorStructVector);
		//m_LocatorNum = mLocatorStructVector.size();
		//std::set<Echo::Name> equipList;
		//roleResource->getEquipInfoName(equipList);
		//int a = Echo::Math::RangeRandom(0, (int)equipList.size() - 1);
		////Echo::Name aa;
		//std::set<Echo::Name>::iterator itb = equipList.begin();
		//for (int i = 0; i < a; i++)
		//{
		//	itb++;
		//}
		//if (itb == equipList.end())
		//{
		//	itb = equipList.begin();
		//}
		//ChangeEquip(*itb);
	}

	void RobotObj::PlayAnimation(const Echo::AnimationList& pList)
	{
		StopAnimation();
		m_pRoleObject->getAnimationComponent()->playAnimation(pList);
	}

	void RobotObj::StopAnimation()
	{
		m_pRoleObject->getAnimationComponent()->clearAnimation();
	}

	void RobotObj::PlaySkillParticle(const Echo::Name& particleName)
	{
		static size_t mmm = 0;
		if (mmm % 3 == 0)
		{
			StopSkillParticle();
		}
		mmm++;
		if (mmm > 9999999)
		{
			mmm = 0;
		}
		
		if (m_LocatorNum == 0)
		{
			return;
		}
		//int a = Echo::Math::RangeRandom(0, (int)m_LocatorNum - 1);
		//Echo::EffectHolderPtr pparticle = Echo::AutoObjectManager::instance()->createParticleHolder(m_pSceneManager->generateMovableName(), particleName, m_pSceneManager);
		//mParticles.push_back(pparticle);
		//m_pRoleObject->getTagPointComponent()->attachParticle(pparticle.get(), mLocatorStructVector[a]);
	}

	void RobotObj::StopSkillParticle()
	{
		//size_t num = mParticles.size();
		//for (size_t i = 0; i < num; i++)
		//{
		//	m_pRoleObject->getTagPointComponent()->detachParticle(mParticles[i].get());
		//}
		//mParticles.clear();
	}

	void RobotObj::Move(const Echo::Vector3& inCameraPosi)
	{
		//static int nn = 0;
		//if (nn % 100 == 0)
		//{
			float x = xStep * Echo::Math::RangeRandom(-1.0f, 1.0f);
			float z = zStep * Echo::Math::RangeRandom(-1.0f, 1.0f);
			xMove += x;
			zMove += z;
			if (xMove < -xRange || xMove > xRange)
			{
				xMove = 0.0;
			}

			if (zMove < -zRange || zMove > zRange)
			{
				zMove = 0.0;
			}
			Echo::Vector3 pp = mPosi + Echo::Vector3(xMove, 0.0, zMove);
			float aaaa = pp.squaredDistance(inCameraPosi);
			m_pRoleObject->setPosition(pp);
			m_pRoleObject->setRoleObjectPriority(90000.0f - aaaa);
			m_pRoleObject->yaw(Echo::Radian(Echo::Math::RangeRandom(-0.5f, 0.5f)));
		//}
		//nn++;
		//if (nn > 1000000)
		//{
		//	nn = 0;
		//}
	}

	void RobotObj::ChangeRobotEntity(Echo::uint8 iPart, const Echo::Name& meshName)
	{
		m_pRoleObject->getEntityComponent()->changeEntity(iPart, meshName);
	}

	void RobotObj::ChangeEquip(const Echo::Name& equipName)
	{
		static int aaaaaa = 0;
		if (aaaaaa % 2 == 0)
		{
			m_pRoleObject->getEntityComponent()->changeEntity(Echo::ROLE_LOD_LEVEL0, 20, Echo::Name("role/zj_001_nan/mesh/00120_bz_001.mesh"));
			m_pRoleObject->getEntityComponent()->changeEntity(Echo::ROLE_LOD_LEVEL0, 21, Echo::Name("role/zj_001_nan/mesh/00121_sz_001.mesh"));
			m_pRoleObject->getEntityComponent()->changeEntity(Echo::ROLE_LOD_LEVEL0, 52, Echo::Name("role/zj_001_nan/mesh/00152_jb_010.mesh"));
			m_pRoleObject->getEntityComponent()->changeEntity(Echo::ROLE_LOD_LEVEL0, 1, Echo::Name("role/zj_001_nan/mesh/00101_st_207.mesh"));
			m_pRoleObject->getEntityComponent()->changeEntity(Echo::ROLE_LOD_LEVEL0, 5, Echo::Name("role/common_zj_equip/mesh/207/20705_yf_a_vir_ct.mesh"));
			m_pRoleObject->getEntityComponent()->changeEntity(Echo::ROLE_LOD_LEVEL0, 6, Echo::Name("role/common_zj_equip/mesh/207/20706_yf_b_vir_ct.mesh"));

			m_pRoleObject->getEntityComponent()->changeEntity(Echo::ROLE_LOD_LEVEL1, 5, Echo::Name("role/common_zj_equip/mesh/207/20705_yf_a_lod1.mesh"));
			m_pRoleObject->getEntityComponent()->changeEntity(Echo::ROLE_LOD_LEVEL1, 6, Echo::Name("role/common_zj_equip/mesh/207/20706_yf_b_lod1.mesh"));


			m_pRoleObject->getEntityComponent()->changeEntity(Echo::ROLE_LOD_LEVEL2, 5, Echo::Name("role/common_zj_equip/mesh/207/20705_yf_a_lod2.mesh"));

			m_pRoleObject->getEntityComponent()->changeEntity(Echo::ROLE_LOD_LEVEL3, 5, Echo::Name("role/common_zj_equip/mesh/207/20705_yf_a_lod3.mesh"));

			m_pRoleObject->getEntityComponent()->changeEntity(Echo::ROLE_LOD_LEVEL4, 5, Echo::Name("role/common_zj_equip/mesh/207/20705_yf_a_lod4.mesh"));
			m_pRoleObject->getEntityComponent()->changeEntity(Echo::ROLE_LOD_LEVEL4, 6, Echo::Name("role/common_zj_equip/mesh/207/20706_yf_b_lod4.mesh"));

			m_pRoleObject->getEntityComponent()->changeEntity(Echo::ROLE_LOD_LEVEL5, 5, Echo::Name("role/zj_001_nan/mesh/00101_st_001_lod3.mesh"));
		}
		else
		{
			m_pRoleObject->getEntityComponent()->changeEntity(Echo::ROLE_LOD_LEVEL0, 20, Echo::Name("role/zj_001_nan/mesh/00120_bz_001.mesh"));
			m_pRoleObject->getEntityComponent()->changeEntity(Echo::ROLE_LOD_LEVEL0, 21, Echo::Name("role/zj_001_nan/mesh/00121_sz_001.mesh"));
			m_pRoleObject->getEntityComponent()->changeEntity(Echo::ROLE_LOD_LEVEL0, 1, Echo::Name("role/zj_001_nan/mesh/00101_st_506.mesh"));
			m_pRoleObject->getEntityComponent()->changeEntity(Echo::ROLE_LOD_LEVEL0, 3, Echo::Name("role/common_zj_equip/mesh/506/50603_fx_a_vir_ct.mesh"));
			m_pRoleObject->getEntityComponent()->changeEntity(Echo::ROLE_LOD_LEVEL0, 4, Echo::Name("role/common_zj_equip/mesh/506/50604_fx_b_vir_ct.mesh"));
			m_pRoleObject->getEntityComponent()->changeEntity(Echo::ROLE_LOD_LEVEL0, 5, Echo::Name("role/common_zj_equip/mesh/506/50605_yf_a_vir_ct.mesh"));
			m_pRoleObject->getEntityComponent()->changeEntity(Echo::ROLE_LOD_LEVEL0, 6, Echo::Name("role/common_zj_equip/mesh/506/50606_yf_b_vir_ct.mesh"));
			m_pRoleObject->getEntityComponent()->changeEntity(Echo::ROLE_LOD_LEVEL0, 7, Echo::Name("role/common_zj_equip/mesh/506/50607_yf_c_vir_ct.mesh"));
			m_pRoleObject->getEntityComponent()->changeEntity(Echo::ROLE_LOD_LEVEL0, 34, Echo::Name("role/common_zj_equip/mesh/506/50634_fs_a_vir_ct.mesh"));

			m_pRoleObject->getEntityComponent()->changeEntity(Echo::ROLE_LOD_LEVEL1, 3, Echo::Name("role/common_zj_equip/mesh/506/50603_fx_a_lod1.mesh"));
			m_pRoleObject->getEntityComponent()->changeEntity(Echo::ROLE_LOD_LEVEL1, 4, Echo::Name("role/common_zj_equip/mesh/506/50604_fx_b_lod1.mesh"));
			m_pRoleObject->getEntityComponent()->changeEntity(Echo::ROLE_LOD_LEVEL1, 5, Echo::Name("role/common_zj_equip/mesh/506/50605_yf_a_lod1.mesh"));
			m_pRoleObject->getEntityComponent()->changeEntity(Echo::ROLE_LOD_LEVEL1, 34, Echo::Name("role/common_zj_equip/mesh/506/50634_fs_a_lod1.mesh"));

			m_pRoleObject->getEntityComponent()->changeEntity(Echo::ROLE_LOD_LEVEL2, 3, Echo::Name("role/common_zj_equip/mesh/506/50603_fx_a_lod2.mesh"));
			m_pRoleObject->getEntityComponent()->changeEntity(Echo::ROLE_LOD_LEVEL2, 5, Echo::Name("role/common_zj_equip/mesh/506/50605_yf_a_lod2.mesh"));

			m_pRoleObject->getEntityComponent()->changeEntity(Echo::ROLE_LOD_LEVEL3, 5, Echo::Name("role/common_zj_equip/mesh/506/50605_yf_a_lod3.mesh"));

			m_pRoleObject->getEntityComponent()->changeEntity(Echo::ROLE_LOD_LEVEL4, 3, Echo::Name("role/common_zj_equip/mesh/506/50603_fx_a_lod1.mesh"));
			m_pRoleObject->getEntityComponent()->changeEntity(Echo::ROLE_LOD_LEVEL4, 4, Echo::Name("role/common_zj_equip/mesh/506/50604_fx_b_lod1.mesh"));
			m_pRoleObject->getEntityComponent()->changeEntity(Echo::ROLE_LOD_LEVEL4, 5, Echo::Name("role/common_zj_equip/mesh/506/50605_yf_a_lod1.mesh"));
			m_pRoleObject->getEntityComponent()->changeEntity(Echo::ROLE_LOD_LEVEL4, 34, Echo::Name("role/common_zj_equip/mesh/506/50634_fs_a_lod1.mesh"));

			m_pRoleObject->getEntityComponent()->changeEntity(Echo::ROLE_LOD_LEVEL5, 5, Echo::Name("role/zj_001_nan/mesh/00101_st_001_lod3.mesh"));
		}
		//else
		//{
			//m_pRoleObject->changeEntity(Echo::ROLE_LOD_LEVEL0, 2, Echo::Name("role/zj_001_nan/mesh_quan/mesh/033/03302_hd_001.mesh"));
			//m_pRoleObject->changeEntity(Echo::ROLE_LOD_LEVEL0, 18, Echo::Name("role/zj_001_nan/mesh_quan/mesh/033/03318_yj_001.mesh"));
			//m_pRoleObject->changeEntity(Echo::ROLE_LOD_LEVEL0, 19, Echo::Name("role/zj_001_nan/mesh_quan/mesh/033/03319_jm_001.mesh"));
			//m_pRoleObject->changeEntity(Echo::ROLE_LOD_LEVEL0, 28, Echo::Name("role/zj_001_nan/mesh_quan/mesh/033/03328_jzt_001.mesh"));
			//m_pRoleObject->changeEntity(Echo::ROLE_LOD_LEVEL0, 1, Echo::Name("role/zj_001_nan/mesh/00101_stmf_000.mesh"));
			//m_pRoleObject->changeEntity(Echo::ROLE_LOD_LEVEL0, 20, Echo::Name("role/zj_001_nan/mesh/00120_bz_001.mesh"));
			//m_pRoleObject->changeEntity(Echo::ROLE_LOD_LEVEL0, 21, Echo::Name("role/zj_001_nan/mesh/00121_sz_001.mesh"));
			//m_pRoleObject->changeEntity(Echo::ROLE_LOD_LEVEL0, 52, Echo::Name("role/zj_001_nan/mesh/00152_jb_001.mesh.mesh"));
			//m_pRoleObject->changeEntity(Echo::ROLE_LOD_LEVEL0, 65, Echo::Name("role/zj_001_nan/mesh/00149_stmf_000.mesh"));

			//m_pRoleObject->changeEntity(Echo::ROLE_LOD_LEVEL1, 1, Echo::Name("role/zj_001_nan/mesh/00101_st_001_lod1.mesh"));
			//m_pRoleObject->changeEntity(Echo::ROLE_LOD_LEVEL1, 65, Echo::Name("role/zj_001_nan/mesh/00149_stmf_000.mesh"));

			//m_pRoleObject->changeEntity(Echo::ROLE_LOD_LEVEL2, 1, Echo::Name("role/zj_001_nan/mesh/00101_st_001_lod1.mesh"));

			//m_pRoleObject->changeEntity(Echo::ROLE_LOD_LEVEL3, 1, Echo::Name("role/zj_001_nan/mesh/00101_st_001_lod1.mesh"));

			//m_pRoleObject->changeEntity(Echo::ROLE_LOD_LEVEL4, 1, Echo::Name("role/zj_001_nan/mesh/00101_st_001_lod4.mesh"));
			//m_pRoleObject->changeEntity(Echo::ROLE_LOD_LEVEL4, 65, Echo::Name("role/zj_001_nan/mesh/00149_stmf_000.mesh"));

			//m_pRoleObject->changeEntity(Echo::ROLE_LOD_LEVEL5, 5, Echo::Name("role/zj_001_nan/mesh/00101_st_001_lod3.mesh"));
		//}
		aaaaaa++;
		if (aaaaaa > 1000)
		{
			aaaaaa = 0;
		}
	}


//-----------------------------------------------------------------------------
//	Cpp File End
//-----------------------------------------------------------------------------
