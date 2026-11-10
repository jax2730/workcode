//-----------------------------------------------------------------------------
// File:   EchoIKFBBIKHeadEffectorComponent.h
//
// Author: qiao_yuzhi
//
// Date:   2022-9-29
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef __EchoIKFBBIKHeadEffectorComponent_h__ 
#define __EchoIKFBBIKHeadEffectorComponent_h__

#include "EchoComponentPrerequisites.h"
#include "EchoProtocolComponent.h"
#include "Actor/EchoActor.h"
#include "EchoComDefineMacro.h"
#include "EchoIKBaseComponent.h"

namespace Echo {

	struct IKFBBIKHeadEffectorComponentData : public IKBaseComponentData {
		FBBIKHeadEffectorInfo mIKInfo;
		String TargetNodeCompName;
		FinalIKInfoBase* GetIKInfo() override { return &mIKInfo; }
	};

	class _EchoComponentExport IKFBBIKHeadEffectorComponent : public IKBaseComponent {
		DECLAREACTORCOMTYPE_H(IKFBBIKHeadEffectorComponent)
			REGISTER_LUA_CLASS_KEY;
	public:
		virtual void SetCompData(IKBaseComponentData* pData) override;
		virtual void SetIKInfo(FinalIKInfoBase* pInfo) override;
		void SetIKInfoData(const FBBIKHeadEffectorInfo& info);
		const FBBIKHeadEffectorInfo& GetIKInfoData();
		virtual void UseDefaultInitialize() override;
		virtual bool IsCanCreate() override;
		inline IKBaseComponentData* CreateCompData() override
		{
			return new IKFBBIKHeadEffectorComponentData();
		}
	public:
		//运行时更新
		void SetPositionWeight(float value); 
		float GetPositionWeight();
		void SetBodyWeight(float value); 
		float GetBodyWeight();
		void SetThighWeight(float value); 
		float GetThighWeight();
		void SetRotationWeight(float value); 
		float GetRotationWeight();
		void SetBodyClampWeight(float value); 
		float GetBodyClampWeight();
		void SetHeadClampWeight(float value); 
		float GetHeadClampWeight();
		void SetBendWeight(float value); 
		float GetBendWeight();
		void SetCCDWeight(float value); 
		float GetCCDWeight();
		void SetRoll(float value); 
		float GetRoll();
		void SetDamper(float value); 
		float GetDamper();
		void SetPostStretchWeight(float value); 
		float GetPostStretchWeight();
		void SetMaxStretch(float value); 
		float GetMaxStretch();
		void SetStretchDamper(float value); 
		float GetStretchDamper();
		void SetChestDirectionWeight(float value); 
		float GetChestDirectionWeight();
		void SetHandsPullBody(bool value); 
		bool GetHandsPullBody();
		void SetFixHead(bool value); 
		bool GetFixHead();
		void SetChestDirection(const Vector3& value); 
		const Vector3& GetChestDirection();
		void SetTargetNodeCompName(const String& value);
		const String& GetTargetNodeCompName();
		//运行时不更新
		void SetFullBodyBipedIKName(const Name& value);
		const Name& GetFullBodyBipedIKName();
		void SetBendBones(const std::vector<NodeWeightInfo>& value); //运行时只更新NodeWeightInfo.weight
		const std::vector<NodeWeightInfo>& GetBendBones();
		void SetCCDBones(const std::vector<Name>& value); 
		const std::vector<Name>& GetCCDBones();
		void SetStretchBones(const std::vector<Name>& value); 
		const std::vector<Name>& GetStretchBones(); 
		void SetChestBones(const std::vector<Name>& value);
		const std::vector<Name>& GetChestBones();
	public:
		void SetWorldTargetNode(const ActorComponentPtr& pComp);
		const ActorComponentPtr& GetWorldTargetNode() { return mTargetPtr; }
	public://运行时接口
		void SetWorldTargetNode(WorldNode* pNode);//不推荐
		EchoIK::FullBodyBipedIK* GetFullBodyBipedIK();
	protected:
		virtual void PostCreateIK() override;
		void ApplyTargetPtr();
		ActorComponentPtr mTargetPtr = nullptr;
	};

	class _EchoComponentExport IKFBBIKHeadEffectorComponentInfo : public IKBaseComponentInfo {
	public:
		IKFBBIKHeadEffectorComponentInfo() {}
		IKFBBIKHeadEffectorComponentInfo(const Name& comType, ActorCOMMoveableType moveable) : IKBaseComponentInfo(comType, moveable) {}
		virtual ~IKFBBIKHeadEffectorComponentInfo() {}
		virtual void CopyToCompData(IKBaseComponentData* pData) override;
		virtual void CompDataToInfo(IKBaseComponentData* pData, IKBaseComponent* pComp) override;

		_DECLCOMINFO_VIRFUN
	public:
		_DECLCOMINFO_PARAM_BEGIN(IKFBBIKHeadEffectorComponent)
		_DECLCOMINFO_PARAM_SIM(float, PositionWeight, 1.0f, SetPositionWeight)
		_DECLCOMINFO_PARAM_SIM(float, BodyWeight,  0.8f, SetBodyWeight)
		_DECLCOMINFO_PARAM_SIM(float, ThighWeight,  0.8f, SetThighWeight)
		_DECLCOMINFO_PARAM_SIM(float, RotationWeight,  0.3f, SetRotationWeight)
		_DECLCOMINFO_PARAM_SIM(float, BodyClampWeight,  0.5f, SetBodyClampWeight)
		_DECLCOMINFO_PARAM_SIM(float, HeadClampWeight,  0.5f, SetHeadClampWeight)
		_DECLCOMINFO_PARAM_SIM(float, BendWeight,  0.6f, SetBendWeight)
		_DECLCOMINFO_PARAM_SIM(float, CCDWeight,  0.0f, SetCCDWeight)
		_DECLCOMINFO_PARAM_SIM(float, Roll,  0.0f, SetRoll)
		_DECLCOMINFO_PARAM_SIM(float, Damper,  300.0f, SetDamper)
		_DECLCOMINFO_PARAM_SIM(float, PostStretchWeight, 1.0f, SetPostStretchWeight)
		_DECLCOMINFO_PARAM_SIM(float, MaxStretch,  1.0f, SetMaxStretch)
		_DECLCOMINFO_PARAM_SIM(float, StretchDamper,  0.0f, SetStretchDamper)
		_DECLCOMINFO_PARAM_SIM(float, ChestDirectionWeight,  1.0f, SetChestDirectionWeight)
		_DECLCOMINFO_PARAM_SIM(bool, HandsPullBody,  true, SetHandsPullBody)
		_DECLCOMINFO_PARAM_SIM(bool, FixHead,  false, SetFixHead)
		_DECLCOMINFO_PARAM(Name, FullBodyBipedIKName,  Name::g_cEmptyName, SetFullBodyBipedIKName)
		_DECLCOMINFO_PARAM(std::vector<NodeWeightInfo>, BendBones, {}, SetBendBones)
		_DECLCOMINFO_PARAM(std::vector<Name>, CCDBones, {}, SetCCDBones)
		_DECLCOMINFO_PARAM(std::vector<Name>, StretchBones, {}, SetStretchBones)
		_DECLCOMINFO_PARAM(Vector3, ChestDirection, Vector3::ZERO, SetChestDirection)
		_DECLCOMINFO_PARAM(std::vector<Name>, ChestBones, {}, SetChestBones)
		_DECLCOMINFO_PARAM(String, TargetNodeCompName, String(""), SetTargetNodeCompName)
		_DECLCOMINFO_PARAM_END()
	};

	DEFINECOMFACTORY(IKFBBIKHeadEffectorComponent, UNMOVEABLE)


}

#endif /*__EchoIKFBBIKHeadEffectorComponent_h__*/
