//-----------------------------------------------------------------------------
// File:   EchoIKSkeletonPostureComponent.h
//
// Author: qiao_yuzhi
//
// Date:   2023-2-23
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef __EchoIKSkeletonPostureComponent_h__ 
#define __EchoIKSkeletonPostureComponent_h__

#include "EchoComponentPrerequisites.h"
#include "EchoProtocolComponent.h"
#include "Actor/EchoActor.h"

#include "EchoHelperObj.h"
#include "EchoHelperObjProFuncImp.h"
#include "IK/EchoIKSkeletonPosture.h"
#include "EchoIKTargetFBBIKComponent.h"
#include "EchoBoneRenderComponent.h"
#include "EchoIKTargetPointComponent.h"
#include "EchoIKLimbIKComponnet.h"


namespace Echo {

	class IKSkeletonPostureComponent; typedef EchoWeakPtr<IKSkeletonPostureComponent> IKSkeletonPostureComponentPtr;

	class _EchoComponentExport IKSkeletonPostureComponent : public ProtocolNodeActorComponent {
		DECLAREACTORCOMTYPE_H(IKSkeletonPostureComponent)
	public:
		virtual void registerTickTimeBegin() override;
		virtual void preUpdateScene(EventID, ProtocolHandle) override;
	public:
		const Name& GetSkeletonPostureName();
		const Name& GetRootBoneName();
		FBBEffectorType GetIKPartType();
		const std::map<Name, Quaternion>& GetSkeletonPosture();
		const Name& GetSkeletonPostureTemplateName();
		void UpdateBoneRenderTree();
		void importFromFile(const Name& file);
		void exportToFile(const Name& file);
		void ClearAllChild();
		void SetBonePosture(std::vector<std::tuple<Name, Vector3, Quaternion, uint32>>& posture);
		void GetBonePosture(std::map<Name, Quaternion>& posture);
		void SetIKBaseComponentPtr(const IKBaseComponentPtr& pComp);
		const IKBaseComponentPtr& GetIKBaseComponentPtr();
		void SetRoleComponentPtr(const EchoWeakPtr<RoleComponent>& pComp);
		const EchoWeakPtr<RoleComponent>& GetRoleComponentPtr();
		void SetHelperVisible(bool value);
		bool GetHelperVisible();
	private:
		void setTargetNode();
		void resetTargetNode();
		void setSkeletonPosture();
		void resetSkeletonPosture();
	private:
		FBBEffectorType m_IKPartType = FBBEffectorType::NumEffectors;
		Name m_RootBoneName = Name::g_cEmptyName;
		Name m_SkeletonPostureName = Name::g_cEmptyName;
		Name m_SkeletonPostureTemplateName = Name::g_cEmptyName;
		std::vector<BoneRenderComponentPtr> m_BoneRenderComps;
		std::map<Name, Quaternion> m_SkeletonPosture;
		IKBaseComponentPtr m_CanTargetIKComponentPtr;
		EchoWeakPtr<RoleComponent> m_RoleComponentPtr;
		bool mIsHelperVisible = false;
	};

	class _EchoComponentExport IKSkeletonPostureComponentInfo : public ActorComponentInfo {
	public:
		IKSkeletonPostureComponentInfo() {}
		IKSkeletonPostureComponentInfo(const Name& comType, ActorCOMMoveableType MoveType);
		virtual ~IKSkeletonPostureComponentInfo();
		_DECLCOMINFO_VIRFUN
	public:
		_DECLCOMINFO_PARAM_BEGIN(IKSkeletonPostureComponent)
		_DECLCOMINFO_PARAM_END()
	};

	DEFINECOMFACTORY(IKSkeletonPostureComponent, ALL_MOVEABLE);
}



#endif /*__EchoBoneRenderComponent_h__*/
