#ifndef __ECHO_SCENECOMPONENT_MANAGER_H__
#define __ECHO_SCENECOMPONENT_MANAGER_H__

#include "EchoComponentPrerequisites.h"
#include "EchoSingleton.h"
#include "EchoProtocolComponent.h"
#include "Actor/EchoActorComponent.h"

namespace Echo {

	struct SceneDataHandle {
		 SceneDataHandle& operator=(const SceneDataHandle& rhs)
		{
			 if (&rhs != this)
			 {
				 mChangeSceneName = rhs.mChangeSceneName;
				 mCameraDirMode = rhs.mCameraDirMode;
				 mCameraPosition = rhs.mCameraPosition;
				 mChangeSceneID = rhs.mChangeSceneID;
			 }
			 return *this;
		}

		std::string  mChangeSceneName;
		uint16       mCameraDirMode;  //相机朝向模式 0：lookat		1:adjust	2:free
		Vector3      mCameraPosition;
		uint32	     mChangeSceneID;
	};
	class _EchoComponentExport SceneComponentManager : public Singleton<SceneComponentManager>
	{
	public:
		virtual ~SceneComponentManager();

		void                         setChangeSceneState(bool isChangeSceneState_);
		bool                         getChangeSceneState();
		void                         setChangeSceneName(const std::string& sceneName_);
		const std::string&           getChangeSceneName() { return mSceneData.mChangeSceneName; }
		void                         setCameraDirMode(uint16 mode_);
		uint16                       getCameraDirMode() { return mSceneData.mCameraDirMode; }
		void                         setCameraPosition(const Vector3& cameraPosition_);
		void                         setCameraPosition(float x, float y, float z);
		const Vector3&               getCameraPosition() { return mSceneData.mCameraPosition; }

		void                         setChangeSceneID(uint32 sceneId_);
		uint32                       getChangeSceneID() { return mSceneData.mChangeSceneID; }
		void                         setChangeSceneDataHandle(const SceneDataHandle& dataHandle_);
		const SceneDataHandle&       getChangeSceneDataHandle() { return mSceneData; }

	private:
		SceneDataHandle  mSceneData;
		bool             mIsQtChangeScene = false;
		bool             mIsChangeScene = false;
	};

	class EchoSceneComManagerScript : public EchoActorComponentScript
	{
	public:
		REGISTER_LUA_CLASS_KEY;
		EchoSceneComManagerScript() = default;
	public:
		static void                     setChangeSceneState(bool isChangeSceneState_);
		static void                     setChangeSceneName(const std::string& name);
		static void                     setCameraDirMode(uint16 mode_);
		static void                     setCameraPosition(float x, float y, float z);
		static void                     setChangeSceneId(uint32 sceneId_);
	};


	class _EchoComponentExport EchoSceneComManagerProFuncImp
	{
		public:
			static void qtNotInitProtocolFunction();
			static void qtNotUninitProtocolFunction();

			static Echo::ProtocolResult  ProCom_ChangeScene(void* data, ProtocolHandle handle);
	};
}


#endif
