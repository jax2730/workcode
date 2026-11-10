#include "EchoPlayerObj.h"
#include "EchoStableHeaders.h"

//-----------------------------------------------------------------------------
//	CPP File Begin
//-----------------------------------------------------------------------------
#include "EchoSingleton.h"
#include "EchoMeshMgr.h"
#include "EchoRoot.h"
#include "Effect/EchoEffectManager.h"
#include "Effect/EchoEffectBase.h"
#include "EchoMath.h"
#include "EchoAutoObjectManager.h"

PlayerObj::PlayerObj(Echo::SceneManager* pSceneManager)
:m_pSceneManager(pSceneManager)
, m_pRoleObject(NULL)
{
}


PlayerObj::~PlayerObj()
{
	StopSkillParticle();
	StopDetachRoleObj();
	if (NULL != m_pRoleObject)
	{
		delete m_pRoleObject;
		m_pRoleObject = NULL;
	}
}


void PlayerObj::CreateRoleObj(const Echo::Name& roleName, const Echo::Name& roleRes, const Echo::Vector3& position)
{
	m_pRoleObject = new Echo::RoleObject(roleName,roleRes,m_pSceneManager,true);
	m_pRoleObject->getAnimationComponent()->setAnimationUpdateManual(true);
	m_pRoleObject->setPosition(position);
	mOriginalPos = m_pRoleObject->getPosition();
	Echo::RolePtr	roleResource = m_pRoleObject->getSkeletonComponent()->getRoleInfo();
	roleResource->getLocatorList(mLocatorStructVector);
	m_LocatorNum = mLocatorStructVector.size();
}

void PlayerObj::ChangePlayerEquip(const Echo::Name& equipName)
{
	m_pRoleObject->getSkeletonComponent()->changeEquip(equipName);
}

void PlayerObj::PlaySkillParticle(const Echo::Name& particleName)
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

	//int mRandom = Echo::Math::RangeRandom(0, (int)m_LocatorNum - 1);
	//Echo::EffectHolderPtr pparticle = Echo::AutoObjectManager::instance()->createParticleHolder(m_pSceneManager->generateMovableName(), particleName, m_pSceneManager);
	//mParticles.push_back(pparticle);
	//m_pRoleObject->getTagPointComponent()->attachParticle(pparticle.get(), mLocatorStructVector[mRandom]);

}

void PlayerObj::StopSkillParticle()
{
	//size_t num = mParticles.size();
	//for (size_t i = 0; i < num; i++)
	//{
	//	m_pRoleObject->getTagPointComponent()->detachParticle(mParticles[i].get());
	//}
	//mParticles.clear();
}

void PlayerObj::PlayAttachRoleObj(const Echo::Name& roleName, const Echo::Name& roleRes)
{
	static size_t mmm = 0;
	if (mmm % 6 == 0)
	{
		StopDetachRoleObj();
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
	Echo::RoleObject*   m_pRoleObj = new Echo::RoleObject(roleName, roleRes, m_pSceneManager, true);
	m_pRoleObj->getAnimationComponent()->setAnimationUpdateManual(true);
	m_pRoleObj->getSkeletonComponent()->changeEquip(Echo::Name("skin"));
	mRoles.push_back(m_pRoleObj);

	int mRandom = Echo::Math::RangeRandom(0, (int)m_LocatorNum - 1);
	m_pRoleObject->getTagPointComponent()->attachRoleObj(m_pRoleObj, mLocatorStructVector[mRandom]->sLocatorName);
}

void PlayerObj::StopDetachRoleObj()
{
	size_t num = mRoles.size();
	
	for (size_t i = 0; i < num; i++)
	{
		m_pRoleObject->getTagPointComponent()->detachRoleObj(mRoles[i]);
		if (NULL != mRoles[i])
		{
			delete(mRoles[i]);
			mRoles[i] = NULL;
		}
	}
	mRoles.clear();
}

void PlayerObj::PlayPlayerAnimation(Echo::AnimationList ainmationlist)
{
	static int n = 0;
	if (n % 100 == 0)
	{
		m_pRoleObject->getAnimationComponent()->clearAnimation();
		m_pRoleObject->getAnimationComponent()->playAnimation(ainmationlist);
	}	
	n++;
	if (n > 100000)
	{
		n = 0;
	}
}
void PlayerObj::SetPlayerPosition()
{
	static size_t n = 0;
	Echo::Vector3 mCuePos = mOriginalPos + 5.0f;
	if (n % 5)
		mCuePos = mOriginalPos;
	n++;
	if (n > 9999999)
	{
		n = 0;
	}
	m_pRoleObject->setPosition(mCuePos);
}

void PlayerObj::SetPlayerScale()
{
	static float scaleVal = 1.0f;
	if (scaleVal == 5)
		scaleVal = 1;
	m_pRoleObject->setScale(Echo::Vector3(scaleVal));
	scaleVal++;
}

void PlayerObj::enablePlayerReciveShadow()
{
	bool isEnable = m_pRoleObject->isResiveShadow() ? false : true;
	m_pRoleObject->enableReciveShadow(isEnable);
}

void PlayerObj::enablePlayerCastShadow()
{
	bool isEnable = m_pRoleObject->isCastShadow() ? false : true;
	m_pRoleObject->enableCastShadow(isEnable);
}

void PlayerObj::setPlayerTagPointScale()
{
	static float scaleVal = 1.0f;
	if (scaleVal == 5)
		scaleVal = 1.0f;
	m_pRoleObject->getSkeletonComponent()->setTagPointScale(scaleVal);
	scaleVal++;
}
//-----------------------------------------------------------------------------
//	Cpp File End
//-----------------------------------------------------------------------------
