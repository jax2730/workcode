//Author: anhaoxiang

#ifndef __EchoCameraComponentManager_h__ 
#define __EchoCameraComponentManager_h__

#include "EchoComponentPrerequisites.h"
#include "EchoProtocolComponent.h"

namespace Echo {

	class CameraBaseComponent;

	class _EchoComponentExport CameraComponentManager : public Singleton<CameraComponentManager>
	{
	public:
		CameraComponentManager();
		~CameraComponentManager();


		void                                pushCameraBaseCom(CameraBaseComponent* cameraCom_);
		void                                popCameraBaseCom(CameraBaseComponent* cameraCom_);
		void                                CameraBaseComPriorityChange(CameraBaseComponent* cameraCom_);

		Camera*                             getActiveCamera();
		void                                setActiveCamera(Camera* camera_);

		CameraBaseComponent*                getActiveCameraCom() { return mActiveCameraCom; }

		bool                                getIsUseCameraComType() { return mIsUseCameraCom; }
		void                                setIsUseCameraComType(bool isUse);

#ifdef ECHO_EDITOR
		void                                setIsUseEditorCamera(bool isUse);
		bool                                getIsUseEditorCamera() { return mIsUseEditorCamera; }
		void                                setEditorCameraNode(CameraBaseComponent* cameraCom_);
		CameraBaseComponent*                getEditorCameraNode() { return mEditorActiveCom; }
		void                                setEditorCameraNineBoxGrid(bool isSet);
		bool                                getIsUseNineGrid() { return mIsUseNineGrid; }
#endif
	private:
		void                                _addCameraBaseCom(CameraBaseComponent* cameraCom_);
		void                                _deleteCameraBaseCom(CameraBaseComponent* cameraCom_);
		void                                _priorityChanged(CameraBaseComponent* cameraCom_);
		void                                _updateCameraComData();
	private:

		std::vector<CameraBaseComponent*>   mCameraDeque;
		CameraBaseComponent*                mActiveCameraCom = nullptr;
		bool                                mIsUseCameraCom = true;

#ifdef ECHO_EDITOR
		bool                                mIsUseEditorCamera = true;
		CameraBaseComponent*                mEditorActiveCom = nullptr;
		bool                                mIsUseNineGrid = false;
#endif 

		Camera*                             mCamera = nullptr;
		Vector3                             mOriPosition;
		Quaternion                          mOriDirection;
	};
}


#endif
