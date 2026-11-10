#include "EchoComponentStableHeaders.h"
#include "EchoDebugScriptFunc.h"
#include "EchoLogManager.h"
#include "EchoColorValue.h"
#include "Actor/EchoNodeActorComponent.h"

namespace Echo
{
	void EchoDebugScript::Script_DebugInt(int value)
	{
		char log[512];
		snprintf(log, 512, "Script_Debug (int) : %d", value);
		LogManager::instance()->logMessage(log);
	}

	void EchoDebugScript::Script_DebugFloat(float value)
	{
		char log[512];
        snprintf(log, 512, "Script_Debug (float) : %f", value);
		LogManager::instance()->logMessage(log);
	}

	void EchoDebugScript::Script_DebugString(const std::string & value)
	{
		char log[512];
        snprintf(log, 512, "Script_Debug (String) : %s", value.c_str());
		LogManager::instance()->logMessage(log);
	}

	void EchoDebugScript::Script_DebugVector3(Vector3 value)
	{
		char log[512];
        snprintf(log, 512, "Script_Debug (Vector3) : %f,%f,%f", value.x,value.y,value.z);
		LogManager::instance()->logMessage(log);
	}

	void EchoDebugScript::Script_DebugQuaternion(Quaternion value)
	{
		char log[512];
        snprintf(log, 512, "Script_Debug (Quaternion) : %f,%f,%f,%f", value.x, value.y, value.z, value.w);
		LogManager::instance()->logMessage(log);
	}

	void EchoDebugScript::Script_DebugBool(bool value)
	{
		char log[512];
        snprintf(log, 512, "Script_Debug (bool) : %s", value == true ? "true" : "false");
		LogManager::instance()->logMessage(log);
	}

	void EchoDebugScript::Script_DebugColorValue(ColorValue value)
	{
		char log[512];
        snprintf(log, 512, "Script_Debug (ColorValue) : %f,%f,%f,%f", value.r, value.g, value.b, value.a);
		LogManager::instance()->logMessage(log);
	}
	// 没有 transform 组件， 暂存于此
	void EchoDebugScript::RotateAround(const EchoActorComponentScript& pCom, const Vector3 & axis, float angle)
	{
		ActorComponent* p_com = pCom.getPtr().get();
		if (p_com == nullptr || p_com->IsMoveable() == false) return;

		Echo::Quaternion q1 = p_com->getRotation();
		q1 = q1 * Echo::Quaternion(Echo::Degree(angle), axis);
		if (p_com->getSceneNode() != nullptr)
			dynamic_cast<ComponentNode*>(p_com)->setRotation(q1);
	}

	float EchoDebugScript::CurrentTime()
	{
		float startTime = Echo::Root::instance()->getTimeElapsed();
		return startTime;
	}

	//bool EchoDebugScript::CreateEntity(std::string path, Vector3 pos, int idx)
	//{
	//	Echo::CreateEntityInfo entityInfo;
	//	std::string name = "entityInfo";
	//	entityInfo.meshPath = path;
	//	entityInfo.lodLevel = LOD_LEVEL_0;
	//	entityInfo.sync = true;
	//	entityInfo.entityName = name + "_" + std::to_string(idx);
	//	entityInfo.initPos = pos;
	//	//CreateEntityInfo* infoPtr = (CreateEntityInfo*)info;
	//	EntityObject* entityObj = new EntityObject(entityInfo.entityName);
	//	if (entityObj == NULL)
	//		return PR_OK;
	//	entityObj->changeEntityImpl(entityInfo.lodLevel, entityInfo.meshPath, entityInfo.sync);
	//	mEntityObjMap.emplace(entityObj, entityInfo.entityName);
	//	//ProtocolHandle* handlePtr = (ProtocolHandle*)handle;
	//	//(*handlePtr) = (ProtocolHandle)entityObj;
	//	entityObj->setPosition(entityInfo.initPos);
	//	return PR_OK;
	//}

	//bool EchoDebugScript::DeleteEntity()
	//{
	//	for (auto it = mEntityObjMap.begin(); it != mEntityObjMap.end(); it++)
	//	{
	//		delete it->first;
	//	}		
	//	mEntityObjMap.clear();
	//	return PR_OK;
	//}
	//void EchoDebugScript::ShowTrajectory(Actor* m_pActor, TimelineComponent* TimeLineCom, bool isChange)
	//{
	//	if (m_pActor == NULL)
	//		return;

	//	if (mHelperObj == NULL)
	//	{
	//		mHelperObj = Echo::Root::instance()->getMainSceneManager()->createManualObject(Name("ShowTrajectoryHelper"));
	//		if (m_pActor == NULL)
	//			return;
	//		m_pActor->setHelperObj(mHelperObj);
	//		mHelperObj->setVisible(true);
	//	}

	//	if (isChange)
	//	{
	//		mHelperObj->clear();
	//		mHelperObj->begin("Helper_VS.txt", "Helper_PS.txt", Echo::ECHO_LINE_LIST);

	//		float dettime = 0.00f;
	//		float maxTime = static_cast<float>(TimeLineCom->TimeLineInfo->mMaxWeight);
	//		for (; dettime < maxTime; dettime += 0.01f)
	//		{
	//			Echo::Vector3 lpos, rpos;
	//			float lefttime = dettime;
	//			float righttime = dettime + 0.01f;
	//			if (righttime > maxTime)
	//				righttime = maxTime;
	//			for (int i = 0; i < 3; i++)
	//			{
	//				lpos[i] = TimeLineCom->floatInterpolation(i, lefttime);
	//				rpos[i] = TimeLineCom->floatInterpolation(i, righttime);
	//			}

	//			mHelperObj->position(lpos);
	//			mHelperObj->colour(0, 1, 0);
	//			mHelperObj->position(rpos);
	//			mHelperObj->colour(0, 1, 0);

	//		}

	//		mHelperObj->end();
	//		m_pActor->updateHelperObj();
	//	}

	//	mHelperObj->setVisible(true);
	//	//isShowHelper = isShow;
	//}
}
