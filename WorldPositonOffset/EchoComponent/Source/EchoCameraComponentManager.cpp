#include "EchoComponentStableHeaders.h"
#include "EchoCameraComponentManager.h"
#include "EchoCameraBaseComponent.h"
#include "EchoSceneManager.h"
#include "Actor/EchoActor.h"

namespace Echo {

	CameraComponentManager::CameraComponentManager()
	{
	}

	CameraComponentManager::~CameraComponentManager()
	{
	}

	void CameraComponentManager::pushCameraBaseCom(CameraBaseComponent* cameraCom_)
	{
		if (cameraCom_ == nullptr)
			return;
		_addCameraBaseCom(cameraCom_);
		if (mIsUseCameraCom)
		   _updateCameraComData();
	}

	void CameraComponentManager::popCameraBaseCom(CameraBaseComponent* cameraCom_)
	{
		if (cameraCom_ == nullptr)
			return;
		_deleteCameraBaseCom(cameraCom_);
		if (mIsUseCameraCom )
		   _updateCameraComData();
	}

	void CameraComponentManager::CameraBaseComPriorityChange(CameraBaseComponent* cameraCom_)
	{
		_priorityChanged(cameraCom_);
		if (mIsUseCameraCom )
			_updateCameraComData();
	}

	Camera* CameraComponentManager::getActiveCamera()
	{
		if (mIsUseCameraCom)
		{
			if (mCamera == nullptr)
			{
				if (mActiveCameraCom)
				{
					ActorPtr actPtr_ = mActiveCameraCom->getActor();
					if (!actPtr_.expired())
					{
						SceneManager* scMgr = actPtr_->GetSceneManager();
						if (scMgr)
							return scMgr->getActiveCamera();
					}

				}
			}
			else
			   return mCamera;
		}
		return nullptr;
	}

	void CameraComponentManager::setActiveCamera(Camera* camera_)
	{ 
		mCamera = camera_; 
		if (mActiveCameraCom)
		{
			if (camera_)
			{
                  mActiveCameraCom->setCamera(camera_);
			}	
			else
			{
				mActiveCameraCom->setCamera(getActiveCamera());
			}
			mActiveCameraCom->setIsActiveCom(true);
		}
	}

	void CameraComponentManager::setIsUseCameraComType(bool isUse)
	{
		if (isUse)
		{
			mIsUseCameraCom = isUse;
			_updateCameraComData();
			if (mActiveCameraCom)
			{
				Camera* activeCam_ = getActiveCamera();
				if (activeCam_)
				{
					mOriDirection = activeCam_->getOrientation();
					mOriPosition = activeCam_->getPosition();
				}
				mActiveCameraCom->recordCameraState();
				mActiveCameraCom->updateCamera();
			}
				
		}
		else
		{
			if (mActiveCameraCom)
			{
				mActiveCameraCom->restoreCameraState(mOriPosition,mOriDirection);
				mActiveCameraCom->updateCameraOutImp();
                mActiveCameraCom->setCamera(nullptr);
				mActiveCameraCom->setIsActiveCom(false);
			}
			mActiveCameraCom = nullptr;
			mIsUseCameraCom = isUse;
		}
	}

#ifdef ECHO_EDITOR

	void CameraComponentManager::setIsUseEditorCamera(bool isUse)
	{
		mIsUseEditorCamera = isUse;
		if (mEditorActiveCom)
		{
			if(isUse)
				mEditorActiveCom->setIsEditorNode(true);
			else
				mEditorActiveCom->setIsEditorNode(false);
		}
	}

	void CameraComponentManager::setEditorCameraNode(CameraBaseComponent* cameraCom_)
	{
		if (mEditorActiveCom)
			mEditorActiveCom->setIsEditorNode(false);
		mEditorActiveCom = cameraCom_;
		if (mEditorActiveCom && mIsUseEditorCamera)
			mEditorActiveCom->setIsEditorNode(true);
	}

	void CameraComponentManager::setEditorCameraNineBoxGrid(bool isSet)
	{
		mIsUseNineGrid = isSet;
		ProtocolComponentSystem::instance()->callProtocolFunc(Echo::PFID_SetEditorCameraNineBoxGrid, (void*)&isSet, 0);
	}
#endif

	void CameraComponentManager::_addCameraBaseCom(CameraBaseComponent* cameraCom_)
	{
		if (cameraCom_ == nullptr)
			return;
		size_t sz = mCameraDeque.size();
		if(sz == 0)
		{
			mCameraDeque.emplace_back(cameraCom_);
			return;
		}

		for (size_t i = 0; i < sz; ++i)
		{
			if (mCameraDeque[i] == cameraCom_)
				return;
		}

		mCameraDeque.resize(sz + 1);

		int currPriority = cameraCom_->getPriority();
		int resSize = static_cast<int>(sz) + 1;

		for (int i = resSize - 2; i >= 0; --i)
		{
			int tempPriority = mCameraDeque[i]->getPriority();
			if (tempPriority <= currPriority)
			{
				mCameraDeque[i + 1] = mCameraDeque[i];
			}
			else
			{
				mCameraDeque[i + 1] = cameraCom_;
				return;
			}
		}
		mCameraDeque[0] = cameraCom_;
	}

	void CameraComponentManager::_deleteCameraBaseCom(CameraBaseComponent* cameraCom_)
	{
#ifdef ECHO_EDITOR
		if (cameraCom_ == mEditorActiveCom)
			mEditorActiveCom = nullptr;
#endif  

		if (cameraCom_ == nullptr)
			return;

		auto targetIter = std::find(mCameraDeque.begin(), mCameraDeque.end(), cameraCom_);
		if(targetIter != mCameraDeque.end())
		   mCameraDeque.erase(targetIter);
	}
	void CameraComponentManager::_priorityChanged(CameraBaseComponent* cameraCom_)
	{
		if (cameraCom_ == nullptr)
			return;

		auto iter = std::find(mCameraDeque.begin(), mCameraDeque.end(), cameraCom_);
		if (iter == mCameraDeque.end())
			return;

		size_t sz = mCameraDeque.size();
		if (sz == 1)
			return;

		size_t idx = static_cast<int>(iter - mCameraDeque.begin());

		//向前遍历
		for (size_t i = idx; i >= 1; --i)
		{
			int currPriority = mCameraDeque[i]->getPriority();
			int prePriority = mCameraDeque[i - 1]->getPriority();
			if (currPriority >= prePriority)
			{
				CameraBaseComponent* temp = mCameraDeque[i];
				mCameraDeque[i] = mCameraDeque[i - 1];
				mCameraDeque[i - 1] = temp;
				temp = nullptr;
			}
			else
			{
				if (i != idx)
					return;
			}
		}
		//向后遍历
		for (size_t i = idx; i < sz - 1; i++)
		{
			int currPriority = mCameraDeque[i]->getPriority();
			int lastPriority = mCameraDeque[i + 1]->getPriority();
			if (currPriority < lastPriority)
			{
				CameraBaseComponent* temp = mCameraDeque[i];
				mCameraDeque[i] = mCameraDeque[i + 1];
				mCameraDeque[i + 1] = temp;
				temp = nullptr;
			}
			else
				return;
		}
	}
	void CameraComponentManager::_updateCameraComData()
	{
		if (mActiveCameraCom)
		{
			mActiveCameraCom->setCamera(nullptr);
			mActiveCameraCom->setIsActiveCom(false);
			mActiveCameraCom = nullptr;
		}

		if (!mCameraDeque.empty())
		{
			mActiveCameraCom = mCameraDeque[0];
			mActiveCameraCom->setCamera(getActiveCamera());
			mActiveCameraCom->setIsActiveCom(true);
		}
	}
}
