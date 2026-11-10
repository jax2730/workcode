//-----------------------------------------------------------------------------
// File:   EchoCharactorControllerComponent.h
//
// Author: qiao_yuzhi
//
// Date:   2022-8-4
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef EchoCharactorControllerComponent_h__
#define EchoCharactorControllerComponent_h__

#include "EchoComponentPrerequisites.h"
#include "EchoProtocolComponent.h"
#include "Actor/EchoActor.h"
#include "EchoComDefineMacro.h"
#include "EchoActorProFuncImp.h"
#include "EchoHelperObj.h"
#include "EchoPhysicsManager.h"
#include "EchoColliderComponent.h"

namespace Echo
{ 
	class RoleComponent;

	enum CCTCenterModel {
		CCT_Top = 0,
		CCT_Middle = 1,
		CCT_Bottom = 2,
		CCT_Number
	};

	struct _EchoComponentExport CharatorControllerComponentData : public ColliderComponentData {
	public:
		float		slopeLimit = 45.f;//0.0f~90.0f
		float		stepOffset = 0.3f;//0.0f~ (middleHeight + radius * 2)
		float		skinWidth = 0.01f;//0.0f~FLT_MAX
		float		minMoveDistance = 0.f;//0.0f~FLT_MAX
		//胶囊体整体高为 2*radius + middleHeight
		float		radius = 0.3f;//胶囊体半径 0.00001f~FLT_MAX
		float		middleHeight = 1.4f;//胶囊体中部高 0.00001f~FLT_MAX
		Vector3		center = { 0.f,1.f,0.f };
		uint32		centerModel = CCT_Middle;
	public:
		CharatorControllerComponentData() {}
		virtual ~CharatorControllerComponentData() {}
	};

	class CharatorControllerComponentInfo;

	class _EchoComponentExport CharatorControllerComponent : public ColliderComponent
	{
	public:
		REGISTER_LUA_CLASS_KEY;
		DECLAREACTORCOMTYPE_H(CharatorControllerComponent)

		virtual void								init(ActorComponentInfo* info)	override;
		virtual void								unInit() override;
		virtual void								replay(ActorComponentInfo* info)override;
		virtual void								NotifiChildImp(int changeMask)  override;
		virtual void								visibleStateChanged(bool activeState) override;

		virtual void								connectCom() override;
		virtual void								disconnectCom() override;
		virtual void								connectActor() override;
		virtual void								disconnectActor() override;

		virtual DVector3							getWorldPositionPhysX() override;
		virtual Quaternion							getWorldRotationPhysX() override;
		virtual DTransform							getWorldTransformPhysX() override;
	public: 
		void										SetRealSize(float radius, float middleHeight);
		void										GetRealSize(float& radius, float& middleHeight);
		void										SetIsCurrentController(bool value);
		bool										GetIsCurrentController();
		void										SyncPhysXGlobalPose();

		bool										PhysicsUpdateComponent();	//物理更新父组件
		bool										ComponentUpdatePhysics();	//父组件更新物理
		virtual void								CreatePhysics() override;
		virtual void								ReleasePhysics() override;
		virtual bool								IsCanPhysics() override;
		virtual bool								IsExistPhysics() override;
	//=================================================用户调用start===========================================================
	public:
		uint32										Move(float dtime, const Vector3& dist, bool bMove = true/*是否移动组件位置*/);	// 当前的行为会带着父节点一起移动
		DVector3									GetWorldCenterPosition();
		DVector3									GetWorldFootPosition(bool bSkin = true);
		DVector3									GetWorldHeadPosition(bool bSkin = true);
		void										SetWorldCenterPosition(const DVector3& position, bool bMove = true);
		void										SetWorldFootPosition(const DVector3& position, bool bMove = true, bool bSkin = true);
		void										SetWorldHeadPosition(const DVector3& position, bool bMove = true, bool bSkin = true);
		void										SetUpDirection(const Vector3& up);
		const Vector3&								GetUpDirection();
		void										SetSlopeLimit(float slopLimit);
		float										GetSlopeLimit();
		void										SetStepOffset(float steoOffset);
		float										GetStepOffset();
		void										SetSkinWidth(float skinWidth);
		float										GetSkinWidth();
		void										SetMinMoveDistance(float minMoveDistance);
		float										GetMinMoveDistance();
		void										SetNonWalkableMode(bool value);
		bool										GetNonWalkableMode();
		void										SetCenter(const Vector3 & center);
		const Vector3&								GetCenter();
		void										SetCenterModel(uint32 model);//enum CCTCenterModel
		uint32										GetCenterModel();
		void										SetRadius(float radius);
		float										GetRadius();
		void										SetMiddleHeight(float middleHeight);
		float										GetMiddleHeight();
		void										SetRealHeight(float height); //设置胶囊体真实的整体高度（计算缩放，此值与物理世界值相同）
		float										GetRealHeigth(); //获取胶囊体真实的整体高度（计算缩放，此值与物理世界值相同）
		void										SetControllerCallBack(const EchoControllerCallBackPtr& pCallBack);
		const EchoControllerCallBackPtr&			GetControllerCallBack();
		void										InvalidateCache();
		virtual void								SetCompData(ColliderComponentData* pData) override;
		PhysXCharacterController*					GetPhysXCharacterController();
	public://CCT附近缓存空间查询
		//Raycast
		bool Raycast(const DVector3& origin, const Vector3& direction, float maxDistance, PhysXGroup layerMask, const EchoSceneQueryFilter& filter = nullptr);
		bool Raycast(const DVector3& origin, const Vector3& direction, EchoRaycastHit& hit, float maxDistance, PhysXGroup layerMask, const EchoSceneQueryFilter& filter = nullptr);
		bool Raycast(const DVector3& origin, const Vector3& direction, std::vector<EchoRaycastHit>& hits, float maxDistance, PhysXGroup layerMask, const EchoSceneQueryFilter& filter = nullptr);
		//Sweep
		bool GeometryCast(const EchoGeometry* geometry, const DTransform& pose, const Vector3& direction, float maxDistance, PhysXGroup layerMask, const EchoSceneQueryFilter& filter = nullptr);
		bool GeometryCast(const EchoGeometry* geometry, const DTransform& pose, const Vector3& direction, EchoRaycastHit& hit, float maxDistance, PhysXGroup layerMask, const EchoSceneQueryFilter& filter = nullptr);
		bool GeometryCast(const EchoGeometry* geometry, const DTransform& pose, const Vector3& direction, std::vector<EchoRaycastHit>& hits, float maxDistance, PhysXGroup layerMask, const EchoSceneQueryFilter& filter = nullptr);
		//Overlap
		bool OverlapGeometry(const EchoGeometry* geometry, const DTransform& pose, PhysXGroup layerMask, const EchoSceneQueryFilter& filter = nullptr);
		bool OverlapGeometry(const EchoGeometry* geometry, const DTransform& pose, EchoSceneQueryHit& hit, PhysXGroup layerMask, const EchoSceneQueryFilter& filter = nullptr);
		bool OverlapGeometry(const EchoGeometry* geometry, const DTransform& pose, std::vector<EchoSceneQueryHit>& hits, PhysXGroup layerMask, const EchoSceneQueryFilter& filter = nullptr);
	//=================================================用户调用end===========================================================
	private:
		virtual EchoPhysicsComponentType GetPhysicsType() override { return EchoPhysicsComponentType::eCONTROLLER; }
		virtual void ResetHelperColor() override;
		void GenerateVBIBCB(std::vector<Vector3>& vbVec, std::vector<uint16>& ibVec) override;
		EchoGeometry* GenerateGeometry(void* buffer) override;
		void										ChangeGeometry();
		virtual void								ChangeGeometryImpl() override;
		void										SetCCTWorldPosition(const DVector3& position);
		DVector3									GetCCTWorldPosition();
		void										UseCenterModel(DVector3& position, bool toPhy);
		const Vector3&								CalCenterOffset(bool bSkin);

		void										SetPositionCCT(const DVector3& position);
		const DVector3 &							GetPositionCCT();

		void										ResetCCTWorldPosition();

		virtual PhysicsActor*						_getActorPhysX() override;

		virtual void								ChangeIsTrigger() override;
		virtual void								ChangeFilterGroup() override;
		Quaternion									GetPhysicsWorldRotation();

		void										UpdateHelperPosition();
		void										UpdateHelperRotation();
	private:
		PhysXCharacterController*					m_pChactrl = nullptr;
		EchoControllerCallBackPtr					m_pCallBack = nullptr;
		Vector3										mUpDirection = Vector3::UP;
		DVector3									mCctPosition = DVector3::ZERO;
#ifdef USE_PHYSICS_HELPEROBJ
	private:
		virtual void								UpdateComponentInEditor() override;
#endif // USE_PHYSICS_HELPEROBJ
	private:
		friend class EchoPhysicsSystem;
		friend class EchoPhysicsSceneCallBack;
		friend class EchoPhysicsManager;
	};

	class _EchoComponentExport CharatorControllerComponentInfo : public ColliderComponentInfo
	{
		friend class CharatorControllerComponent;
	public:
		CharatorControllerComponentInfo() {}
		CharatorControllerComponentInfo(const Name& comType, ActorCOMMoveableType moveType) :ColliderComponentInfo(comType, moveType) {}
		virtual ~CharatorControllerComponentInfo() {}
		_DECLCOMINFO_VIRFUN
	public:
		virtual void CopyToData(ColliderComponentData* pData) override;

		_DECLCOMINFO_PARAM_BEGIN(CharatorControllerComponent)
		_DECLCOMINFO_PARAM_SIM(uint32, CenterModel, CCT_Middle, SetCenterModel);
		_DECLCOMINFO_PARAM_SIM(float, SlopeLimit, 45.f, SetSlopeLimit);
		_DECLCOMINFO_PARAM_SIM(float, StepOffset, 0.3f, SetStepOffset);
		_DECLCOMINFO_PARAM_SIM(float, SkinWidth, 0.01f, SetSkinWidth);
		_DECLCOMINFO_PARAM_SIM(float, MinMoveDistance, 0.0f, SetMinMoveDistance);
		_DECLCOMINFO_PARAM_SIM(float, Radius, 0.3f, SetRadius);
		_DECLCOMINFO_PARAM_SIM(float, MiddleHeight, 1.4f, SetMiddleHeight);
		_DECLCOMINFO_PARAM(Vector3, Center, Vector3(0.f, 1.f, 0.f), SetCenter);
		_DECLCOMINFO_PARAM_SIM(bool, IsCurrentController, false, SetIsCurrentController);
		_DECLCOMINFO_PARAM_END()
	};


	DEFINECOMFACTORY(CharatorControllerComponent, UNMOVEABLE)

	class EchoCharatorControllerScript
	{
	public:
		REGISTER_LUA_CLASS_KEY;
		static CharatorControllerComponent* Script_GetCharatorControllerComponent(Actor* pActor, const std::string& name);

	};

	class PhysXMathCctMoveTask;
	class _EchoComponentExport EchoMathCctMoveTask {
	public:
		EchoMathCctMoveTask(PhysXMathCharacterController* pCharl);
		virtual ~EchoMathCctMoveTask();
		void Move(const Vector3& dist, float elapsedTime, float minDist, PhysXGroupEnum canInteractGroup = ePhysXGroup_All);
		void Wait();
		bool IsMoving() const;
		uint32 GetResult();
		using FinishCallback = std::function<void()>;
		void SetFinishCallback(const FinishCallback& callback);
		PhysXMathCharacterController* GetController() { return m_pCharl; }
	private:
		PhysXMathCharacterController* m_pCharl = nullptr;
		PhysXMathCctMoveTask* m_pPrivate = nullptr;
	};

	class MathCharacterControllerTask;
	class _EchoComponentExport MathCharacterController {
	public:
		MathCharacterController(EchoPhysicsManager* pMgr);
		virtual ~MathCharacterController();
	public:
		bool										IsPrepareMove();
		void										PrepareMove(float dtime, const Vector3& dist);
		uint32										SynchroMove();
		void										SetFinishCallback(const std::function<void(MathCharacterController*)>& callback);

		uint32										Move(float dtime, const Vector3& dist);
		DVector3									GetWorldCenterPosition();
		DVector3									GetWorldFootPosition(bool bSkin = true);
		DVector3									GetWorldHeadPosition(bool bSkin = true);
		void										SetWorldCenterPosition(const DVector3& position);
		void										SetWorldFootPosition(const DVector3& position, bool bSkin = true);
		void										SetWorldHeadPosition(const DVector3& position, bool bSkin = true);
		void										SetUpDirection(const Vector3& up);
		const Vector3&								GetUpDirection();
		void										SetSlopeLimit(float slopLimit);
		float										GetSlopeLimit();
		void										SetStepOffset(float steoOffset);
		float										GetStepOffset();
		void										SetSkinWidth(float skinWidth);
		float										GetSkinWidth();
		void										SetMinMoveDistance(float minMoveDistance);
		float										GetMinMoveDistance();
		void										SetNonWalkableMode(bool value);
		bool										GetNonWalkableMode();
		void										SetCenter(const Vector3& center);
		const Vector3&								GetCenter();
		void										SetCenterModel(uint32 model);//enum CCTCenterModel
		uint32										GetCenterModel();
		void										SetRadius(float radius);
		float										GetRadius();
		void										SetMiddleHeight(float middleHeight);
		float										GetMiddleHeight();
		void										SetCanInteractFilterGroupType(PhysXGroup value);
		PhysXGroup									GetCanInteractFilterGroupType();
		void										SetRealHeight(float height);
		float										GetRealHeigth();
		void										SetControllerCallBack(EchoMathControllerCallBack* pCallBack);
		EchoMathControllerCallBack*					GetControllerCallBack() const;
		void										InvalidateCache();
		PhysXMathCharacterController*				GetPhysXMathCharacterController();
		EchoPhysicsManager*							GetPhysicsManager() const { return m_pManager; }
		void										SetRealSize(float radius, float middleHeight);
		void										GetRealSize(float& radius, float& middleHeight);
		void										setWorldPosition(const DVector3& position) { SetCCTWorldPosition(position); }
		DVector3									getWorldPosition() { return GetCCTWorldPosition(); }
		const Quaternion&							getWorldRotation() { return mWorldRotation; }
		const Vector3&								getWorldScale() { return Vector3::UNIT_SCALE; }
		void										SetHelperVisible(bool value);
		bool										GetHelperVisible();
		void										ResetHelperColor();
		void										SetCompData(CharatorControllerComponentData* pData);

		inline void									SetUserData(void* pUserData) { m_pUserData = pUserData; }
		inline void*								GetUserData() const { return m_pUserData; }
		inline void									SetUserType(int userType) { m_UserType = userType; }
		inline int									GetUserType() const { return m_UserType; }
	public://CCT附近缓存空间查询
		//Raycast
		bool Raycast(const DVector3& origin, const Vector3& direction, float maxDistance, PhysXGroup layerMask, const EchoSceneQueryFilter& filter = nullptr);
		bool Raycast(const DVector3& origin, const Vector3& direction, EchoRaycastHit& hit, float maxDistance, PhysXGroup layerMask, const EchoSceneQueryFilter& filter = nullptr);
		bool Raycast(const DVector3& origin, const Vector3& direction, std::vector<EchoRaycastHit>& hits, float maxDistance, PhysXGroup layerMask, const EchoSceneQueryFilter& filter = nullptr);
		//Sweep
		bool GeometryCast(const EchoGeometry* geometry, const DTransform& pose, const Vector3& direction, float maxDistance, PhysXGroup layerMask, const EchoSceneQueryFilter& filter = nullptr);
		bool GeometryCast(const EchoGeometry* geometry, const DTransform& pose, const Vector3& direction, EchoRaycastHit& hit, float maxDistance, PhysXGroup layerMask, const EchoSceneQueryFilter& filter = nullptr);
		bool GeometryCast(const EchoGeometry* geometry, const DTransform& pose, const Vector3& direction, std::vector<EchoRaycastHit>& hits, float maxDistance, PhysXGroup layerMask, const EchoSceneQueryFilter& filter = nullptr);
		//Overlap
		bool OverlapGeometry(const EchoGeometry* geometry, const DTransform& pose, PhysXGroup layerMask, const EchoSceneQueryFilter& filter = nullptr);
		bool OverlapGeometry(const EchoGeometry* geometry, const DTransform& pose, EchoSceneQueryHit& hit, PhysXGroup layerMask, const EchoSceneQueryFilter& filter = nullptr);
		bool OverlapGeometry(const EchoGeometry* geometry, const DTransform& pose, std::vector<EchoSceneQueryHit>& hits, PhysXGroup layerMask, const EchoSceneQueryFilter& filter = nullptr);
	private:
		void CreatePhysics();
		void ReleasePhysics();
		void CreateHelper();
		void ReleaseHelper();
		void UpdateHelper();
		Quaternion GetPhysicsWorldRotation();
		void GenerateVBIBCB(std::vector<Vector3>& vbVec, std::vector<uint16>& ibVec);
		void ChangeGeometry();
		void SetCCTWorldPosition(const DVector3& position);
		DVector3 GetCCTWorldPosition();
		void UseCenterModel(DVector3& position, bool toPhy);
		const Vector3& CalCenterOffset(bool bSkin);
		void Move_Async(float dtime, const Vector3& dist);
		void Move_Sync();
	private:
		bool		mNonWalkableMode = false;
		float		mSlopeLimit = 45.f;
		float		mStepOffset = 0.3f;
		float		mSkinWidth = 0.01f;
		float		mMinMoveDistance = 0.f;
		float		mRadius = 0.3f;
		float		mMiddleHeight = 1.4f;
		Vector3		mCenter = { 0.f,1.f,0.f };
		uint32		mCenterModel = CCT_Middle;
		PhysXGroup mCanInteractFilterGroupType = ePhysXGroup_All;
		uint32		mResult = 0;
		bool m_IsHelperVisible = false;
		int m_UserType = 0;
		void* m_pUserData = nullptr;

		EchoPhysicsManager* m_pManager = nullptr;
		PhysXMathCharacterController* m_pChactrl = nullptr;
		EchoMathControllerCallBack* m_pCallBack = nullptr;
		MathCharacterControllerTask* m_pTask = nullptr;
		Vector3 mUpDirection = Vector3::UP;
		HelperObject* m_pHelperObj = nullptr;
		Quaternion mWorldRotation = Quaternion::IDENTITY;
		Vector3 mWorldScale = Vector3::UNIT_SCALE;
	private:
		friend class EchoPhysicsSystem;
		friend class EchoPhysicsSceneCallBack;
		friend class EchoPhysicsManager;
		friend class MathCharacterControllerTask;
	};

}; // namespace Echo
#endif // EchoCharactorControllerComponent_h__
