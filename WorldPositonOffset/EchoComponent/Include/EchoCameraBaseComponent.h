//Author: anhaoxiang

#ifndef __EchoCameraBaseComponent_h__ 
#define __EchoCameraBaseComponent_h__

#include "EchoComponentPrerequisites.h"
#include "EchoComDefineMacro.h"


namespace Echo {

	enum CameraComType
	{
		Echo_Camera,
		Client_Camera,
		End_Camera
	};

	struct CameraComBaseData {
		int              mPriority = 0;
		CameraComType    mCameraType = Echo_Camera;

		bool             mIsUse    = false;
	};

#ifdef ECHO_EDITOR
	struct _EchoComponentExport EditorCameraData {
		Vector3      mPosition;
		Quaternion   mRotation;
		float        mFov;
	};

	struct _EchoComponentExport EditorCameraFbxData
	{
		std::string   mFovName;
		std::string   mFovButtomName;
		std::vector<std::string>  mFovItemName;
		bool          isExportFov;
	};
#endif

	class _EchoComponentExport CameraBaseComponentInfo : public ActorComponentInfo
	{
	public:
		CameraBaseComponentInfo() {}
		CameraBaseComponentInfo(const Name& name, ActorCOMMoveableType isMoveable);
		virtual ~CameraBaseComponentInfo() {}

		virtual void                  ExportData(cJSON* jsonNode) override;
		virtual void                  ImportData(cJSON* jsonNode) override;
		virtual void                  CopyTo(ActorComponentInfo* info) override;
		virtual void                  OverWrite() override;
	public:
		CameraComBaseData             mContext;
		EDITOR_REFLECT

	};

	static Name cameraUseStateName = Name("cameraComUseState");

	class _EchoComponentExport CameraBaseComponent : public NodeActorComponent
	{
		friend class CameraComponentManager;
		DECLAREACTORCOMTYPE_H(CameraBaseComponent)
	public:
		//重写组件的visible状态切换函数
		virtual void                  visibleStateChanged(bool isVisible) override;
		virtual void                  setTimelineAttribute(std::vector<float>&, const Name&) override;
		virtual void                  createComRootNodeImpl() override;
		virtual void                  showEditorHelper(bool isShow = true) override;

		void						  init(ActorComponentInfo* info) override;
		void                          unInit() override;
        
		//标识是否启用当前
		void                          setIsUse(bool isUse_);
		bool                          getIsUse() { return mContext.mIsUse; }
		
		void                          updateCamera();
		
		void                          setCameraBasePosition(const Vector3& position_);
		void                          setCameraBaseRotation(const Vector3& rotation);

		//返回当前组件是否是正在生效的相机标签
		bool                          getIsActiveCom() { return mIsActiveCom; }


		//当该组件是生效组件时，设置位置和旋转时会调用下面两个函数
		virtual   void                setCameraBasePositionImpl(const Vector3& position_);
		virtual   void                setCameraBaseRotationImpl(const Vector3& rotation);
		
		//启用状态为true时会调用该函数
		virtual   void                updateCameraInImpl();
		//启用状态为false时会调用该函数
		virtual   void                updateCameraOutImp();
		
		//当不使用相机节点组件时，会传递一个位移和旋转信息去还原相机的位置以及旋转信息,不需要可以重写为空
		virtual   void			      restoreCameraState(const Vector3& pos_,const Quaternion& ros);
		//当使用相机节点组件时，给一个记录现在相机属性的时机（此时相机属性还未被组件修改）
		virtual   void			      recordCameraState();

		void                          setFov(float fov);
#ifdef ECHO_EDITOR
		virtual   void                initEditorCameraFbxData();
		const EditorCameraFbxData&    getEditorCameraFbxData();

		void                          createEntity();
		void                          deleteEntity();

		virtual void                  setEditorCameraData();
		void                          setIsEditorNode(bool isEditorNode_) { mIsEditorNode = isEditorNode_; }
		bool                          getIsEditorNode() { return mIsEditorNode; }
		void                          setEditorCameraNode();
#endif
		int                           getPriority() { return mContext.mPriority; }
		void                          setPriority(int priority);

	private:
		void                          setIsActiveCom(bool isActive_) { mIsActiveCom = isActive_; }
		void                          setCamera(Camera* pCamera_) { mCamera = pCamera_; }
	protected:
		CameraComBaseData             mContext;
		bool                          mIsActiveCom = false;

		float                         mFov;
#ifdef ECHO_EDITOR
		EditorCameraFbxData           mEditorFbxData;
#endif
	private:
		Camera*                       mCamera = nullptr;
#ifdef ECHO_EDITOR
		Entity*                       mCameraEntity = nullptr;
		EditorCameraData              mEditorCameraData;
		bool                          mIsEditorNode = false;
#endif
		static String				  EDITORCAMERAMESH;
	};

	DEFINECOMFACTORY(CameraBaseComponent, ActorCOMMoveableType::ALL_MOVEABLE)
}


#endif
