#include "EchoPlayerObjManager.h"
#include "EchoPlayerObj.h"
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

PlayerObjManager::PlayerObjManager()
	:m_pSceneManager(NULL)
{
	InitPlayerResources();
}


PlayerObjManager::~PlayerObjManager()
{
	Destroy();
}

void PlayerObjManager::SetSceneManager(Echo::SceneManager* pSceneManager)
{
	m_pSceneManager = pSceneManager;
}

void PlayerObjManager::CreatePlayerObject(Echo::Vector3 inWorldPosi)
{
	Destroy();
	for (size_t i = 0; i < m_RolePathVector.size(); i++)
	{
		int randomNo = Echo::Math::RangeRandom(0, (int)m_RolePathVector.size() - 1);
		PlayerObj* pPlayer = new PlayerObj(m_pSceneManager);
		pPlayer->CreateRoleObj(Echo::Name("test"), m_RolePathVector[i], inWorldPosi);
		pPlayer->ChangePlayerEquip(m_RoleSkinName[randomNo]);
		m_Players.push_back(pPlayer);
		inWorldPosi.z += 10.0f;
	}
}

void PlayerObjManager::RandomPlayer(PlayerObj* player)
{
	int randomNo = Echo::Math::RangeRandom(0, (int)ROLE_HOLDER_COUNT);
	switch (ENUM_ROLE_OBJECT_HOLDER(randomNo))
	{
	case ROLE_HOLDER_SKILL:
	{
		int skillParticleNo = Echo::Math::RangeRandom(0, (int)m_SkillParticleNum - 1);
		player->PlaySkillParticle(m_RoleSkillParticleName[skillParticleNo]);
		break;
	}
	case ROLE_HOLDER_EQUIP:
	{
		int randomNo = Echo::Math::RangeRandom(0, (int)m_RolePathVector.size() - 1);
		player->ChangePlayerEquip(m_RoleSkinName[randomNo]);
		break;
	}
	case ROLE_HOLDER_ROLEOBJ:
	{
		int randomNo = Echo::Math::RangeRandom(0, (int)m_WeapRoleName.size() - 1);
		player->PlayAttachRoleObj(Echo::Name("test"), m_WeapRoleName[randomNo]);
		break;

	}
	case ROLE_HOLDER_POSITION:
	{
		player->SetPlayerPosition();
		break;
	}

	case ROlE_HOLDER_SCALE:
	{
		player->SetPlayerScale();
		break;
	}
	case  ROlE_HOLDER_RECIVESHADOW:
	{
		player->enablePlayerReciveShadow();
		break;
	}

	case ROlE_HOLDER_CASTSHADOW:
	{
		player->enablePlayerCastShadow();
		break;
	}
	case ROLE_HOLDER_TAGPOINTSCALE:
	{
		player->setPlayerTagPointScale();
		break;
	}
	case RolE_HOLSER_ANIMATION:
	{
		int upAnimationNo = Echo::Math::RangeRandom(0, (int)m_UpAnimationNum - 1);
		int downAnimationNo = Echo::Math::RangeRandom(0, (int)m_DownAnimationNum - 1);
		Echo::AnimationList temp;
		temp.push_back(m_UpAnimationNameVector[upAnimationNo]);
		temp.push_back(m_DownAnimationNameVector[downAnimationNo]);
		player->PlayPlayerAnimation(temp);
		temp.clear();
	}
	default:
		break;
	}
}

void PlayerObjManager::Tick()
{
	for (size_t j = 0; j < m_Players.size(); j++)
	{
		RandomPlayer(m_Players[j]);
	}
}


void PlayerObjManager::InitPlayerResources()
{
	initRoleResources();
	initParticleResources();
	initWeapRoleResourecs();
	ininRoleAnimationResourecs();
}

void PlayerObjManager::initRoleResources()
{
	m_RolePathVector.clear();
	m_RoleSkinName.clear();

	m_RolePathVector.push_back(Echo::Name("role/zj_002_nv/zj_002_nv_001/zj_002_nv_001.role"));
	m_RoleSkinName.push_back(Echo::Name("000_bayifu"));

	m_RolePathVector.push_back(Echo::Name("role/zj_002_nv/zj_002_nv_001/zj_002_nv_001.role"));
	m_RoleSkinName.push_back(Echo::Name("001_nv_h_ceshi"));

	m_RolePathVector.push_back(Echo::Name("role/zj_002_nv/zj_002_nv_001/zj_002_nv_001.role"));
	m_RoleSkinName.push_back(Echo::Name("001+206"));

	m_RolePathVector.push_back(Echo::Name("role/zj_002_nv/zj_002_nv_001/zj_002_nv_001.role"));
	m_RoleSkinName.push_back(Echo::Name("001+463_JLcaifeng"));

	m_RolePathVector.push_back(Echo::Name("role/zj_002_nv/zj_002_nv_001/zj_002_nv_001.role"));
	m_RoleSkinName.push_back(Echo::Name("001+480_hushinvjiao"));

	m_RolePathVector.push_back(Echo::Name("role/zj_002_nv/zj_002_nv_001/zj_002_nv_001.role"));
	m_RoleSkinName.push_back(Echo::Name("002_nv_h_ceshi"));

	m_RolePathVector.push_back(Echo::Name("role/zj_002_nv/zj_002_nv_001/zj_002_nv_001.role"));
	m_RoleSkinName.push_back(Echo::Name("005_nv_h_ceshi"));

}

void PlayerObjManager::initParticleResources()
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

void PlayerObjManager::initWeapRoleResourecs()
{
	m_WeapRoleName.push_back(Echo::Name("role/wq_015_gong/wq_015_gong.role"));
	m_WeapRoleName.push_back(Echo::Name("role/wq_004_ruanqiang/wq_004_ruanqiang.role"));
	m_WeapRoleName.push_back(Echo::Name("role/wq_043_yujian/wq_043_yujian.role"));
	m_WeapRoleName.push_back(Echo::Name("role/wq_038_liangfaxiandanqiang/wq_038_liangfaxiandanqiang.role.role"));

}

void PlayerObjManager::ininRoleAnimationResourecs()
{
	Echo::AnimationDesc temp;
	temp.animationName = Echo::Name("zc_dm_xr_xx_11_ca");
	temp.time = 3.0;
	temp.weight = 1.0;
	m_UpAnimationNameVector.push_back(temp);

	temp.animationName = Echo::Name("zc_dm_zl_4_z_01_ca");
	temp.time = 2.0;
	temp.weight = 1.0;
	m_UpAnimationNameVector.push_back(temp);

	temp.animationName = Echo::Name("zc_dp_dm_pb_q_02_ca");
	temp.time = 5.0;
	temp.weight = 1.0;
	m_UpAnimationNameVector.push_back(temp);


	temp.animationName = Echo::Name("zc_dm_xr_xx_11_tk");
	temp.time = 3.0;
	temp.weight = 1.0;
	m_DownAnimationNameVector.push_back(temp);

	temp.animationName = Echo::Name("zc_dm_zl_4_z_01_tk");
	temp.time = 2.0;
	temp.weight = 1.0;
	m_DownAnimationNameVector.push_back(temp);

	temp.animationName = Echo::Name("zc_dp_dm_pb_q_02_tk");
	temp.time = 5.0;
	temp.weight = 1.0;
	m_DownAnimationNameVector.push_back(temp);

	m_UpAnimationNum = m_UpAnimationNameVector.size();
	m_DownAnimationNum = m_DownAnimationNameVector.size();
}
void PlayerObjManager::Destroy()
{
	std::vector<PlayerObj*>::iterator it, ite = m_Players.end();
	for (it = m_Players.begin(); it != ite; ++it)
	{
		delete (*it);
	}
	m_Players.clear();
}
//-----------------------------------------------------------------------------
//	Cpp File End
//-----------------------------------------------------------------------------
