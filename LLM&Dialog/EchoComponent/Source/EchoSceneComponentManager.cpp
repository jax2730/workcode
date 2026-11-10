#include "EchoComponentStableHeaders.h"
#include "EchoSceneComponentManager.h"
#include "EchoTODComponentManager.h"
#include "EchoComponentSystem.h"
#include "EchoWorldSystem.h"
#include "EchoWorldManager.h"
#include "EchoEngine.h"
#include "EchoTODComponentSystem.h"

namespace Echo {
	SceneComponentManager::~SceneComponentManager()
	{
	}
	void SceneComponentManager::setChangeSceneState(bool isChangeSceneState_)
	{
		bool isQt = ComponentSystem::instance()->getIsQtInit();
		if (isQt)
			mIsQtChangeScene = isChangeSceneState_;
		else
			mIsChangeScene = isChangeSceneState_;
		if (isChangeSceneState_)
		{
			TODComponentSystem::instance()->getActiveTodComManager()->closeAllTodCom();
		}
	}
	bool  SceneComponentManager::getChangeSceneState()
	{
		bool isQt = ComponentSystem::instance()->getIsQtInit();
		if (isQt)
			return mIsQtChangeScene;
		return mIsChangeScene;
	}

	void SceneComponentManager::setChangeSceneName(const std::string& sceneName_)
	{
		mSceneData.mChangeSceneName = sceneName_;
	}
	void SceneComponentManager::setCameraDirMode(uint16 mode_)
	{
		mSceneData.mCameraDirMode = mode_;
	}
	void SceneComponentManager::setCameraPosition(const Vector3& cameraPosition_)
	{
		mSceneData.mCameraPosition = cameraPosition_;
	}
	void SceneComponentManager::setChangeSceneID(uint32 sceneId_)
	{
		mSceneData.mChangeSceneID = sceneId_;
	}
	void SceneComponentManager::setChangeSceneDataHandle(const SceneDataHandle& dataHandle_)
	{
		mSceneData = dataHandle_;
	}

	void SceneComponentManager::setCameraPosition(float x, float y, float z)
	{
		mSceneData.mCameraPosition = Vector3(x, y, z);
	}


	//脚本
	void EchoSceneComManagerScript::setChangeSceneState(bool isChangeSceneState_)
	{
		SceneComponentManager::instance()->setChangeSceneState(isChangeSceneState_);
	}
	void EchoSceneComManagerScript::setChangeSceneName(const std::string& name)
	{
		SceneComponentManager::instance()->setChangeSceneName(name);
	}
	void EchoSceneComManagerScript::setCameraDirMode(uint16 mode_)
	{
		SceneComponentManager::instance()->setCameraDirMode(mode_);
	}
	void EchoSceneComManagerScript::setChangeSceneId(uint32 sceneId_)
	{
		SceneComponentManager::instance()->setChangeSceneID(sceneId_);
	}
	void EchoSceneComManagerScript::setCameraPosition(float x, float y, float z)
	{
		SceneComponentManager::instance()->setCameraPosition(x, y, z);
	}

	void EchoSceneComManagerProFuncImp::qtNotInitProtocolFunction()
	{
		ProtocolComponentSystem* p = ProtocolComponentSystem::instance();
		p->registerProtocolExtFunc(Name("ChangeScene"), &EchoSceneComManagerProFuncImp::ProCom_ChangeScene);
	}
	void EchoSceneComManagerProFuncImp::qtNotUninitProtocolFunction()
	{
		ProtocolComponentSystem* p = ProtocolComponentSystem::instance();
		p->unregisterProtocolExtFunc(Name("ChangeScene"));
	}

	Echo::ProtocolResult  EchoSceneComManagerProFuncImp::ProCom_ChangeScene(void* data, ProtocolHandle handle)
	{
		//if (ComponentSystem::instance()->getIsQtInit())
		//	return PR_Falied;
		//if (SceneComponentManager::instance()->getChangeSceneState())
		//{
		//	Echo::ProtocolComponentSystem* proComManager = Echo::ProtocolComponentSystem::instance();
		//	proComManager->callProtocolFunc(Echo::PFID_ActorUIDLL_ClearAll, nullptr, 0);

		//	//ActorSystem::instance()->getMainActorManager()->deleteAllActor();
		//	//ActorSystem::instance()->getMainActorManager()->deinit();
		//	std::string sceneName = SceneComponentManager::instance()->getChangeSceneName();
		//	Name worldName = *(Name*)handle;
		//	Echo::WorldManager* worldManager = Echo::WorldSystem::instance()->GetWorldManager(worldName);
		//	GetEchoEngine()->changeLevel(worldName, sceneName);
		//	SceneComponentManager::instance()->setChangeSceneState(false);
		//	worldManager->getSceneManager()->getMainCamera()->setPosition(SceneComponentManager::instance()->getCameraPosition());
		//	worldManager->getSceneManager()->getMainCamera()->setOrientation(Quaternion(0.260934f, 0.458318f, 0.142717f, -0.837553f));
		//}
		return PR_OK;
	}
}

