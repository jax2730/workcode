#include "EchoComponentStableHeaders.h"
#include "EchoCameraBaseComponent.h"
#include "EchoCameraComponentManager.h"
#include "EchoEntity.h"
#include "Actor/EchoActor.h"
#include "EchoSceneManager.h"
#include "EchoStringTable.h"

//#define EditorCameraMesh "Editor/editorResources/scbCamera.mesh"

namespace Echo {


	DECLAREACTORCOMTYPE_CPP(CameraBaseComponent)


		EDITOR_REFLECT_BEGIN(CameraBaseComponentInfo)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mPriority, "CameraPriority", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mContext.mIsUse, "CameraIsUse", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_END()

		String CameraBaseComponent::EDITORCAMERAMESH = "";

	CameraBaseComponentInfo::CameraBaseComponentInfo(const Name& name, ActorCOMMoveableType isMoveable)
		:ActorComponentInfo(name,isMoveable)
	{

	}

	void CameraBaseComponentInfo::ExportData(cJSON* jsonNode)
	{
		reflect::TypeResolver<CameraBaseComponentInfo>::get()->ExportJson(this, jsonNode, "");
	}

	void CameraBaseComponentInfo::ImportData(cJSON* jsonNode)
	{
		if (jsonNode == nullptr)
			return;
		reflect::TypeResolver<CameraBaseComponentInfo>::get()->ImportJson(this, jsonNode, "");
	}

	void CameraBaseComponentInfo::CopyTo(ActorComponentInfo* info)
	{
		if (info == nullptr)
			return;
		auto newObjectInfo = static_cast<CameraBaseComponentInfo*>(info);
		reflect::TypeResolver<CameraBaseComponentInfo>::get()->copyTo(this, newObjectInfo);
	}

	void CameraBaseComponentInfo::OverWrite()
	{
		if (mOriginComInfo != nullptr)
		{
			reflect::TypeResolver<CameraBaseComponentInfo>::get()->overWrite(mOriginComInfo, this);
		}
	}


	void CameraBaseComponent::visibleStateChanged(bool isVisible)
	{
		if (mContext.mIsUse)
		{
			if (mIsActiveCom)
				updateCameraOutImp();
			
			if (isVisible)
				CameraComponentManager::instance()->pushCameraBaseCom(this);
			else
				CameraComponentManager::instance()->popCameraBaseCom(this);

			if (mIsActiveCom)
				updateCameraInImpl();
		}
#ifdef ECHO_EDITOR
		if (mCameraEntity)
			mCameraEntity->setVisible(isVisible);
#endif
	}



	CameraBaseComponent::CameraBaseComponent(ActorPtr pActor, ActorCOMMoveableType isMoveable)
		: NodeActorComponent(pActor, isMoveable)
	{
		if (EDITORCAMERAMESH.empty())
			EDITORCAMERAMESH = StringTable::instance()->getString("Camera", "EditorCameraMesh");
	}

	CameraBaseComponent::~CameraBaseComponent()
	{
	}




	void CameraBaseComponent::setTimelineAttribute(std::vector<float>& data, const Name& attributeName_)
	{
		NodeActorComponent::setTimelineAttribute(data, attributeName_);
		if (attributeName_ == cameraUseStateName)
		{
			if (data[0] == 0)
				setIsUse(false);
			else if (data[0] == 1)
			{
				CameraComponentManager* cMgr = CameraComponentManager::instance();
				CameraBaseComponent* activeCom_ = cMgr->getActiveCameraCom();

				if (cMgr && activeCom_ && activeCom_->getPriority() == mContext.mPriority && activeCom_ != this)
				{
					activeCom_->setIsUse(false);
					cMgr->popCameraBaseCom(activeCom_);
				}
				setIsUse(true);
			}	
		}
		/*else if (attributeName_ == Name("CameraFOV"))
		{
			Degree fov(data[0]);
			getSceneManager()->getActiveCamera()->setFOVy(fov);
		}*/


		if (attributeName_ == positionName_)
		{
			setCameraBasePosition(Vector3(data[0],data[1],data[2]));
		}
		else if (attributeName_ == rotationName_)
		{
			setCameraBaseRotation(Vector3(data[0],data[1],data[2]));
		}
	}

	void CameraBaseComponent::createComRootNodeImpl()
	{
		SceneManager* sMgr = getSceneManager();
		if(sMgr)
		   mComRootNode = sMgr->createSceneNode();
	}

	void CameraBaseComponent::showEditorHelper(bool isShow)
	{
#ifdef ECHO_EDITOR
			if (mCameraEntity)
				mCameraEntity->setVisible(isShow);
#endif
	}

	void CameraBaseComponent::init(ActorComponentInfo* info)
	{
#ifdef ECHO_EDITOR
		createEntity();
		initEditorCameraFbxData();
#endif
		if (info == nullptr)
			return;
		//setIsUse(static_cast<CameraBaseComponentInfo*>(info)->mContext.mIsUse);
		mContext.mPriority = static_cast<CameraBaseComponentInfo*>(info)->mContext.mPriority;
		mContext.mCameraType = static_cast<CameraBaseComponentInfo*>(info)->mContext.mCameraType;
	}

	void CameraBaseComponent::unInit()
	{
		if (mContext.mIsUse)
			setIsUse(false);
#ifdef ECHO_EDITOR
		deleteEntity();
		if (CameraComponentManager::instance()->getEditorCameraNode() == this)
		{
			CameraComponentManager::instance()->setEditorCameraNode(nullptr);
			mIsEditorNode = false;
		}
#endif
	}

	void CameraBaseComponent::setIsUse(bool isUse_)
	{
		bool isChangeState = (isUse_ != mContext.mIsUse);
		CameraComponentManager* cMgr = CameraComponentManager::instance();
		if (cMgr && isChangeState)
		{
			if (isUse_)
			{
				cMgr->pushCameraBaseCom(this);
				updateCamera();
#ifdef ECHO_EDITOR
				if (mCameraEntity)
					mCameraEntity->setDiffuseColor(ColorValue::Red * 5);	   
				setEditorCameraNode();
#endif
			}
			else
			{
#ifdef ECHO_EDITOR
				if (mCameraEntity)
					mCameraEntity->resetDiffuseColor();
#endif
				if(mIsActiveCom)
					updateCameraOutImp();
				cMgr->popCameraBaseCom(this);
			}
				
		}
		mContext.mIsUse = isUse_;
	}

	void CameraBaseComponent::updateCamera()
	{
#ifdef ECHO_EDITOR
		mEditorCameraData.mPosition = getWorldPosition();
		mEditorCameraData.mRotation = getWorldRotation();
#endif
		if (mIsActiveCom)
		    updateCameraInImpl();
	}

	void CameraBaseComponent::setCameraBasePosition(const Vector3& position_)
	{
		if (mIsActiveCom)
			setCameraBasePositionImpl(position_);
#ifdef ECHO_EDITOR
		mEditorCameraData.mPosition = getWorldPosition();
		setEditorCameraData();
#endif
	}

	void CameraBaseComponent::setCameraBaseRotation(const Vector3& rotation)
	{
		if (mIsActiveCom)
			setCameraBaseRotationImpl(rotation);
#ifdef ECHO_EDITOR
		mEditorCameraData.mRotation = getWorldRotation();
		setEditorCameraData();
#endif
	}

	void CameraBaseComponent::setCameraBasePositionImpl(const Vector3& position_)
	{
		if (mCamera)
			mCamera->setPosition(getWorldPosition());
	}

	void CameraBaseComponent::setCameraBaseRotationImpl(const Vector3& rotation)
	{
		if (mCamera)
			mCamera->setOrientation(getWorldRotation());
	}

	void CameraBaseComponent::updateCameraInImpl()
	{
		if (mCamera)
		{
			Vector3 pos_ = getWorldPosition();
			Quaternion rot_ = getWorldRotation();
			mCamera->setPosition(pos_);
			mCamera->setOrientation(rot_);
		}
	}
	void CameraBaseComponent::updateCameraOutImp()
	{
	}
	void CameraBaseComponent::restoreCameraState(const Vector3& pos_, const Quaternion& ros)
	{
		if (mCamera)
		{
			mCamera->setOrientation(ros);
			mCamera->setPosition(pos_);
		}

	}
	void CameraBaseComponent::recordCameraState()
	{
	}

	void CameraBaseComponent::setFov(float fov)
	{
		mFov = fov;
#ifdef ECHO_EDITOR
		mEditorCameraData.mFov = fov;
		setEditorCameraData();
#endif
	}

#ifdef ECHO_EDITOR
	void CameraBaseComponent::initEditorCameraFbxData()
	{
		mEditorFbxData.isExportFov = false;
		//测试代码
		/*mEditorFbxData.mFovButtomName = "Fov";
		mEditorFbxData.mFovItemName.resize(1);
		mEditorFbxData.mFovItemName [0] = "fov";
		mEditorFbxData.mFovName = "CameraFOV";*/

	}
	const EditorCameraFbxData& CameraBaseComponent::getEditorCameraFbxData()
	{
		return mEditorFbxData;
	}
	void CameraBaseComponent::createEntity()
	{
		if (mCameraEntity != nullptr)
			return;

		if (m_pActor == nullptr || m_pActor.expired())
			return;
		SceneManager* scMgr = m_pActor->GetSceneManager();
		if (scMgr == nullptr)
			scMgr = Root::instance()->getActiveSceneManager();
		if (scMgr == nullptr)
			return;

		const std::string& entityName_ = getCombinedName();
		mCameraEntity = scMgr->createEntity(Name(entityName_ + "||" + EDITORCAMERAMESH), Name(EDITORCAMERAMESH));

		if (mCameraEntity)
		{
			mCameraEntity->setCameraCullMode(DistanceCull);
			mCameraEntity->setRenderingDistance(10000.f);
			attachToNode(mCameraEntity);
			UpdateComRootNode();
		}
	}

	void CameraBaseComponent::deleteEntity()
	{
		if (mCameraEntity == nullptr)
			return;

		if (m_pActor == nullptr || m_pActor.expired())
			return;
		SceneManager* scMgr = m_pActor->GetSceneManager();
		if (scMgr == nullptr)
			return;
		scMgr->destroyEntity(mCameraEntity);
		mCameraEntity = nullptr;
	}
	void CameraBaseComponent::setEditorCameraData()
	{
	   if(mIsEditorNode)
		   ProtocolComponentSystem::instance()->callProtocolFunc(Echo::PFID_SetEdotirCamera_Data, &mEditorCameraData, 0);
	}
	void CameraBaseComponent::setEditorCameraNode()
	{
		CameraComponentManager::instance()->setEditorCameraNode(this);
		mEditorCameraData.mPosition = getWorldPosition();
		mEditorCameraData.mRotation = getWorldRotation();
		mEditorCameraData.mFov = mFov;
		setEditorCameraData();
	}
#endif

	void CameraBaseComponent::setPriority(int priority)
	{
		mContext.mPriority = priority;
		CameraComponentManager::instance()->CameraBaseComPriorityChange(this);
	}
}
