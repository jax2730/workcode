//-----------------------------------------------------------------------------
// File:   EchoColliderComponent.h
//
// Author: qiao_yuzhi
//
// Date:   2022-7-27
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef __EchoColliderComponent_h__ 
#define __EchoColliderComponent_h__

#include "EchoComponentPrerequisites.h"
#include "EchoProtocolComponent.h"
#include "Actor/EchoActor.h"

#include "EchoHelperObj.h"
#include "EchoHelperObjProFuncImp.h"
#include "EchoPhysicsManager.h"
#include "EchoRigidBodyComponent.h"
#include "EchoStringConverter.h"
#include "EchoPhysicsCommon.h"

namespace Echo {

	//SurfaceType定义 移至 EchoPhysicsCommon.h

	struct _EchoComponentExport ColliderComponentData {
		bool isTrigger = false;
		int materialId = -1;
		uint32 surfaceType = 0;
		PhysXGroup filterGroupType = ePhysXGroup_Mesh;
		PhysXGroup canInteractFilterGroupType = ePhysXGroup_All;
		ColliderComponentData() {}
		virtual ~ColliderComponentData() {}
	};

	//碰撞体基类
	class _EchoComponentExport ColliderComponent : public PhysicsActorComponent {
	public:
		REGISTER_LUA_CLASS_KEY;
	public:
		ColliderComponent(ActorPtr pActor,ActorCOMMoveableType type);
		virtual ~ColliderComponent();
		virtual void init(ActorComponentInfo* info) override;
		virtual void unInit() override;
		virtual void replay(ActorComponentInfo* info) override;
		virtual void copyTo(const ActorComponentPtr& actCom) override;
		virtual void NotifiChildImp(int changeMask) override;
		virtual void showEditorHelper(bool isShow) override final;
		virtual void updateCombinedName() override final;
		virtual void visibleStateChanged(bool activeState) override;
		virtual void connectCom() override;
		virtual void disconnectCom() override;

		virtual DVector3 getWorldPositionPhysX() override;
		virtual Quaternion getWorldRotationPhysX() override;
		virtual DTransform getWorldTransformPhysX() override;
	//================================================物理start========================================================
	public:
		virtual void CreatePhysics() override;
		virtual void ReleasePhysics() override;
		virtual bool IsCanPhysics() override;
		virtual bool IsExistPhysics() override;
		PhysXShape* GetPhysics();
		virtual AggregateComponentPtr GetAggregate() override;
		const RigidBodyComponentPtr& GetRigidBody() const { return m_pAttachRigidBody; }
	protected:
	
		void CreateDefaultStatic();
		void DeleteDefaultStatic();
	
		virtual EchoPhysicsComponentType GetPhysicsType() override { return EchoPhysicsComponentType::eCOLLIDER; }
		void OnParentingChanged(const ActorComponentPtr& compPtr);
		virtual bool IsCanAttach(RigidBodyComponent* pRB);
		void CreateShape();
		void ReleaseShape();
		void ChangeShape();
		void AttachShape(const RigidBodyComponentPtr& pRB);
		void DetachShape();
	protected:
		void ChangeMaterialID();
		void ChangeSurfaceType();
		virtual void ChangeIsTrigger();
		void ChangeLocalPose();
		void ChangeLocalPoseImpl();
		void ChangeAttach();
		virtual void ChangeFilterGroup();
		virtual void ResetHelperColor() = 0;
	//================================================物理end========================================================
	//===============================================编辑与渲染start================================================
#ifdef USE_PHYSICS_HELPEROBJ
	protected:
		virtual void UpdateComponentInEditor() override;
		void CreateHelper();
		void ReleaseHelper();
		void UpdateHelper();
		HelperObject* m_pHelperObj = nullptr;
#endif // USE_PHYSICS_HELPEROBJ
	public:
		void SetHelperVisible(bool value);
		bool GetHelperVisible();
		void SetHelperColorType(HelperObjColorType color);
		HelperObjColorType GetHelperColorType();
	//===============================================编辑与渲染end================================================
	public:
		//根据参数填充VBIB（渲染用）,若cbVec为空则仅使用顶点和索引
		virtual void GenerateVBIBCB(std::vector<Vector3>& vbVec, std::vector<uint16>& ibVec) = 0;
		//生成Geometry
		virtual EchoGeometry* GenerateGeometry(void* buffer/*ColliderComponent::GEOM_MAX_SIZE*/) = 0;
		static const size_t GEOM_MAX_SIZE = 64;
	//================================================用户调用start==============================================
	public:
		void SetIsTrigger(bool value);
		bool GetIsTrigger();
		void SetMaterialId(int value);
		int GetMaterialId();
		void SetSurfaceType(uint32 value);
		uint32 GetSurfaceType(const DBVector3& worldPosition = DBVector3::ZERO, bool bWorld = true);
		void SetFilterGroupType(PhysXGroup value);
		PhysXGroup GetFilterGroupType();
		void SetCanInteractFilterGroupType(PhysXGroup value);
		PhysXGroup GetCanInteractFilterGroupType();
		
		const ColliderComponentData* GetCompData();
		virtual void SetCompData(ColliderComponentData* pData) = 0;

		void AddColliderCallBack(const EchoColliderCallBackPtr& pComp);
		void RemoveColliderCallBack(const EchoColliderCallBackPtr& pComp);

		virtual void SetIsUsePhysics(bool value);
		void SetIsUsePhysXPage(bool value);
	//===============================================用户调用end================================================
	private:
		bool ExistColliderCallBack() { return !mListenerList.empty(); }
		void SendCollisionEnter(const EchoCollisionParam& param);
		void SendCollisionExit(const EchoCollisionParam& param);
		void SendCollisionStay(const EchoCollisionParam& param);
		void SendTriggerEnter(const EchoTriggerParam& param);
		void SendTriggerExit(const EchoTriggerParam& param);
		void SendTriggerStay(const EchoTriggerParam& param);
		void SendCollisionCCT(const EchoControllerShapeHitParam& param);
		void SendCollisionCCT(const EchoMathControllerShapeHitParam& param);
	protected:
		void SetCompDataBase(ColliderComponentData* pData);
		void UpdatePhysXPage();
		//bool CheckScaleChanged();
		bool ExistPhysXObject();

		void CreatePhysXObject();
		void ReleasePhysXObject();
		bool IsAsync();

		virtual void ChangeGeometryImpl();
		virtual PhysicsActor* _getActorPhysX() override;
#ifdef USE_PHYSICS_PAGE
	public:
		EchoPhysXActorObject* GetEchoPhysXObject();
#endif //USE_PHYSICS_PAGE
	protected:
		PhysXShape* m_pShape = nullptr;
		PhysXStatic* m_pAttachStatic = nullptr;
		RigidBodyComponentPtr m_pAttachRigidBody = nullptr;
		ColliderComponentData* m_pCompData = nullptr;
		std::function<uint32(const DBVector3& position, bool bWorld)> m_pGetSurfaceTypeFun = nullptr;
		std::set<EchoColliderCallBackPtr> mListenerList;
		//Vector3 m_lastWorldScale = Vector3::UNIT_SCALE;
		bool m_bAsyncLoad : 1 = false;
		bool m_IsChangeSurfaceType : 1 = true;
		bool m_IsCurrentController : 1 = false;
		bool mNonWalkableMode : 1 = false;
		bool m_bUpdateGeometry : 1 = false;
	private:
		friend class EchoPhysicsSystem;
		friend class EchoPhysicsManager;
		friend class PhysicsPage;
		friend class RigidBodyComponent;
		friend class EchoPhysicsSceneCallBack;
		friend class EchoPhysicsControllerCallBack;
		friend class EchoMathPhysicsControllerCallBack;
		friend class EchoPhysXDynamicObject;
		friend class EchoPhysXStaticObject;
		friend class EditorColliderComponentUI;
		friend class SphericalTerrainComponent;
	};

	class _EchoComponentExport ColliderComponentInfo : public ActorComponentInfo {
	public:
		ColliderComponentInfo() {}
		ColliderComponentInfo(const Name& comType, ActorCOMMoveableType MoveType);
		virtual ~ColliderComponentInfo();
		virtual void CopyToData(ColliderComponentData* pData);
		_DECLCOMINFO_VIRFUN
	public:
		_DECLCOMINFO_PARAM_BEGIN(ColliderComponent)
		_DECLCOMINFO_PARAM_SIM(bool, IsTrigger, false, SetIsTrigger)
		_DECLCOMINFO_PARAM_SIM(int, MaterialId, -1, SetMaterialId)
		_DECLCOMINFO_PARAM_SIM(uint32, SurfaceType, 0, SetSurfaceType)
		_DECLCOMINFO_PARAM_SIM(PhysXGroup, FilterGroupType, ePhysXGroup_Mesh, SetFilterGroupType)
		_DECLCOMINFO_PARAM_SIM(PhysXGroup, CanInteractFilterGroupType, ePhysXGroup_All, SetCanInteractFilterGroupType)
		_DECLCOMINFO_PARAM_END()
	private:
		friend class EchoPhysicsSystem;
		friend class EchoPhysicsManager;
	};


}

namespace Echo {

	class _EchoComponentExport NodeColliderComponent : public ColliderComponent, public ComponentNode {
	public:
		NODE_ACTOR_COM_H
		REGISTER_LUA_CLASS_KEY;
	public:
		NodeColliderComponent(ActorPtr pActor, ActorCOMMoveableType type);
		virtual ~NodeColliderComponent();
		virtual void NotifiChildImp(int changeMask) override;
#ifdef USE_PHYSICS_HELPEROBJ
	protected:
		virtual void UpdateComponentInEditor() override;
#endif // USE_PHYSICS_HELPEROBJ
	};

}


#endif /*__EchoColliderComponent_h__*/
