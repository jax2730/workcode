#pragma once
#ifndef _EchoPlayerObjManager_H_
#define _EchoPlayerObjManager_H_

//-----------------------------------------------------------------------------
//	Head File Begin
//-----------------------------------------------------------------------------
#include "EchoEnginePrerequisites.h"
#include "EchoResourceBackgroundQueue.h"
#include "EchoSceneNode.h"
#include "EchoSingleton.h"
#include "EchoRole.h"
#include "EchoRoleObject.h"
class PlayerObj;

//RoleObject客户端调用接口枚举
enum ENUM_ROLE_OBJECT_HOLDER
{
	ROLE_HOLDER_SKILL,					//切换技能
	ROLE_HOLDER_EQUIP,					//切换装备
	ROLE_HOLDER_ROLEOBJ,				//切换武器
	ROLE_HOLDER_POSITION,				//更换位置
	ROlE_HOLDER_SCALE,					//更换缩放值
	ROlE_HOLDER_RECIVESHADOW,			//更换缩放值
	ROlE_HOLDER_CASTSHADOW,				//设置角色是否接受阴影
	ROLE_HOLDER_TAGPOINTSCALE,			//设置角色是否产生阴影
	RolE_HOLSER_ANIMATION,				//切换动画

	ROLE_HOLDER_COUNT
};

class PlayerObjManager:public Echo::Singleton<PlayerObjManager>
{
public:
	PlayerObjManager();
	~PlayerObjManager();
	void SetSceneManager(Echo::SceneManager* pSceneManager);
	void Tick();
	void CreatePlayerObject(Echo::Vector3 inWorldPosi);
	void Destroy();
private:
	void initRoleResources();
	void initParticleResources();
	void initWeapRoleResourecs();
	void ininRoleAnimationResourecs();
	void RandomPlayer(PlayerObj* player);
	void InitPlayerResources();
private:
	std::vector<Echo::Name> m_RolePathVector;
	std::vector<Echo::Name> m_RoleSkinName;
	std::vector<Echo::Name> m_RoleSkillParticleName;
	std::vector<Echo::Name> m_WeapRoleName;
	std::vector<Echo::AnimationDesc> m_UpAnimationNameVector;
	std::vector<Echo::AnimationDesc> m_DownAnimationNameVector;

	std::vector<PlayerObj*> m_Players;
	Echo::SceneManager* m_pSceneManager;
	size_t m_SkillParticleNum;
	size_t m_UpAnimationNum;
	size_t m_DownAnimationNum;
};
#endif // !_EchoPlayerObjManager_H_

//-----------------------------------------------------------------------------
//	Head File Begin
//-----------------------------------------------------------------------------


