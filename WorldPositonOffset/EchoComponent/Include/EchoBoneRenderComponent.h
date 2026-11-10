//-----------------------------------------------------------------------------
// File:   EchoBoneRenderComponent.h
//
// Author: qiao_yuzhi
//
// Date:   2022-9-7
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef __EchoBoneRenderComponent_h__ 
#define __EchoBoneRenderComponent_h__

#include "EchoComponentPrerequisites.h"
#include "EchoProtocolComponent.h"
#include "Actor/EchoActor.h"

#include "EchoHelperObj.h"
#include "EchoHelperObjProFuncImp.h"
#include "EchoPhysicsManager.h"
#include "EchoRigidBodyComponent.h"
#include "EchoStringConverter.h"
#include "IK/EchoIKSkeletonPosture.h"
#include "EchoIKBaseComponent.h"


namespace Echo {

	class BoneRenderComponent; typedef EchoWeakPtr<BoneRenderComponent> BoneRenderComponentPtr;

	class _EchoComponentExport BoneRenderComponent : public NodeActorComponent {
		DECLAREACTORCOMTYPE_H(BoneRenderComponent)
		REGISTER_LUA_CLASS_KEY;
	public:
		virtual void init(ActorComponentInfo* info)				override;
		virtual void unInit()									override;
		virtual void replay(ActorComponentInfo* info)			override;
		virtual void copyTo(const ActorComponentPtr& actCom) override;
		virtual void connectCom()								override;
		virtual void disconnectCom()							override;
		virtual void showEditorHelper(bool isShow)				override;
		virtual void updateCombinedName()						override;
	public:
		void UpdateBoneRenderTree();
	public:
		void UpdateRender();
		void CreateRender();
		void ReleaseHelper();
		void ChangeVBIB();
		bool IsRootBoneRender();
	public:
		void SetRenderVisible(bool value);
		bool GetRenderVisible();

		void SetBone(Bone* pBone);
		Bone* GetBone();
		const Name& GetBoneName();
		void SetBoneName(const Name& name);
	public:
		void GetSkeletonPosturesData(std::map<Name, Quaternion>& posture, Vector3& selfWorldPos, Quaternion& selfWorldRot);
		//获取自身及子BoneRender的旋转
		void GetBonePosture(std::map<Name, Quaternion>& boneMap);

		void exportToFile(const Name& file, EchoIK::FullBodyBipedEffector type);
		EchoIK::FullBodyBipedEffector importFromFile(const Name& file);
	private:
		void ClearAllChild();
		void GetBonePosture(std::vector<std::tuple<Name, Vector3, Quaternion, uint32>>& posture, uint32 index);
		void SetBonePosture(std::vector<std::tuple<Name, Vector3, Quaternion, uint32>>& posture, uint32 index);
	private:
		HelperObject* m_pHelperObj = nullptr;
		Bone* m_pBone = nullptr;
		Name mBoneName;
		bool mIsVisible = true;

		static float gBoneWidth;
		static std::vector<Vector3> gVB;
		static std::vector<uint16> gIB;
		static std::vector<uint32> gColors;
		static void GenerateRenderBuffer();
		friend class IKTargetPointComponent;
	};

	class _EchoComponentExport BoneRenderComponentInfo : public ActorComponentInfo {
	public:
		BoneRenderComponentInfo() {}
		BoneRenderComponentInfo(const Name& comType, ActorCOMMoveableType MoveType);
		virtual ~BoneRenderComponentInfo();
		_DECLCOMINFO_VIRFUN
	public:
		_DECLCOMINFO_PARAM_BEGIN(BoneRenderComponent)
		_DECLCOMINFO_PARAM(Name, BoneName, Name(""), SetBoneName)
		_DECLCOMINFO_PARAM_END()
	};

	DEFINECOMFACTORY(BoneRenderComponent, ALL_MOVEABLE);
}



#endif /*__EchoBoneRenderComponent_h__*/
