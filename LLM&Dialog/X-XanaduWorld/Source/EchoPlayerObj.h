#ifndef __EchoPlayerObj_H__
#define __EchoPlayerObj_H__
//-----------------------------------------------------------------------------
//	Head File Begin
//-----------------------------------------------------------------------------
#include "EchoEnginePrerequisites.h"
#include "EchoSceneNode.h"
#include "EchoResourceBackgroundQueue.h"
#include "EchoSceneNode.h"
#include "Effect/EchoEffectHolder.h"
#include "EchoAnimationState.h"
class PlayerObj
{
public:
	PlayerObj(Echo::SceneManager* pSceneManager);
	~PlayerObj();

	void CreateRoleObj(const Echo::Name& roleName, const Echo::Name& roleRes, const Echo::Vector3& position);

	void ChangePlayerEquip(const Echo::Name& euqipName);

	void PlaySkillParticle(const Echo::Name& particleName);
	void StopSkillParticle();

	void PlayAttachRoleObj(const Echo::Name& roleName, const Echo::Name& roleRes);
	void StopDetachRoleObj();

	void PlayPlayerAnimation(Echo::AnimationList ainmationlist);
//	void StopAnimation();

	void SetPlayerPosition();
	void SetPlayerScale();
	void enablePlayerReciveShadow();
	void enablePlayerCastShadow();
	void setPlayerTagPointScale();
private:
	Echo::SceneManager* m_pSceneManager;
	Echo::RoleObject* m_pRoleObject;
	std::vector<Echo::LocatorStruct*> mLocatorStructVector;
	//std::vector<Echo::EffectHolderPtr> mParticles;
	std::vector<Echo::RoleObject*> mRoles;
	Echo::Vector3 mOriginalPos;
	size_t m_LocatorNum;

};

//-----------------------------------------------------------------------------
//	Head File End
//-----------------------------------------------------------------------------
#endif
