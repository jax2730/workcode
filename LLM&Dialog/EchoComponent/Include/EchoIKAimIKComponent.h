//-----------------------------------------------------------------------------
// File:   EchoIKAimIKComponent.h
//
// Author: qiao_yuzhi
//
// Date:   2022-9-29
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef __EchoIKAimIKComponent_h__ 
#define __EchoIKAimIKComponent_h__

#include "EchoComponentPrerequisites.h"
#include "EchoProtocolComponent.h"
#include "Actor/EchoActor.h"
#include "EchoComDefineMacro.h"
#include "EchoIKBaseComponent.h"

namespace Echo {

	struct IKAimIKComponentData : public IKBaseComponentData {
		AimIKInfo mIKInfo;
		String TargetNodeCompName;
		String AimNodeCompName;
		String PoleTargetNodeName;
		FinalIKInfoBase* GetIKInfo() override { return &mIKInfo; }
	};

	class _EchoComponentExport IKAimIKComponent : public IKBaseComponent {
		DECLAREACTORCOMTYPE_H(IKAimIKComponent)
			REGISTER_LUA_CLASS_KEY;
	public:
		RoleObjectListenerExecutionPriority GetExecutionPriority() const override { return RoleObjectListenerExecutionPriority::AIM_IK; }
		virtual void SetCompData(IKBaseComponentData* pData) override;
		virtual void SetIKInfo(FinalIKInfoBase* pInfo) override;
		void SetIKInfoData(const AimIKInfo& info);
		const AimIKInfo& GetIKInfoData();
		virtual void UseDefaultInitialize() override;
		virtual bool IsCanCreate() override;
		inline IKBaseComponentData* CreateCompData() override
		{
			return new IKAimIKComponentData();
		}
	public:
		void SetFixTransforms(bool value); 
		bool GetFixTransforms();
		void SetWeight(float value); 
		float GetWeight();
		void SetPoleWeight(float value); 
		float GetPoleWeight();
		void SetTolerance(float value); 
		float GetTolerance();
		void SetMaxIterations(int value); 
		int GetMaxIterations();
		void SetClampWeight(float value); 
		float GetClampWeight();
		void SetClampSmoothing(int value); 
		int GetClampSmoothing();
		void SetUseRotationLimits(bool value); 
		bool GetUseRotationLimits();
		void SetXY(bool value); 
		bool GetXY();
		void SetAxis(const Vector3& value); 
		const Vector3& GetAxis();
		void SetPoleAxis(const Vector3& value); 
		const Vector3& GetPoleAxis();
		void SetTargetNodeCompName(const String& value);
		const String& GetTargetNodeCompName();
		void SetAimNodeCompName(const String& value);
		const String& GetAimNodeCompName();
		void SetPoleTargetNodeName(const String& value);
		const String& GetPoleTargetNodeName();
		void SetBones(const std::vector<NodeWeightInfo>& value);//运行时只更新NodeWeightInfo.weight
		const std::vector<NodeWeightInfo>& GetBones();
	public:
		void SetTargetNode(const ActorComponentPtr& pComp);
		void SetAimNode(const ActorComponentPtr& pComp);
		void SetPoleTargetNode(const ActorComponentPtr& pComp);
		const ActorComponentPtr& GetTargetNode() { return mTargetPtr; }
		const ActorComponentPtr& GetAimNode() { return mAimPtr; }
		const ActorComponentPtr& GetPoleTargetNode() { return mPoleTargetPtr; }
	public://运行时接口
		void SetTargetNode(WorldNode* pNode);//不推荐
		void SetAimNode(WorldNode* pNode);//不推荐
		void SetPoleTargetNode(WorldNode* pNode);//不推荐
	protected:
		virtual void PostCreateIK() override;
		void ApplyTargetPtr();
		void ApplyAimPtr();
		void ApplyPoleTargetPtr();
		ActorComponentPtr mTargetPtr = nullptr;
		ActorComponentPtr mAimPtr = nullptr;
		ActorComponentPtr mPoleTargetPtr = nullptr;
	};

	class _EchoComponentExport IKAimIKComponentInfo : public IKBaseComponentInfo {
	public:
		IKAimIKComponentInfo() {}
		IKAimIKComponentInfo(const Name& comType, ActorCOMMoveableType moveable) : IKBaseComponentInfo(comType, moveable) {}
		virtual ~IKAimIKComponentInfo() {}
		virtual void CopyToCompData(IKBaseComponentData* pData) override;
		virtual void CompDataToInfo(IKBaseComponentData* pData, IKBaseComponent* pComp) override;
		_DECLCOMINFO_VIRFUN
	public:
		_DECLCOMINFO_PARAM_BEGIN(IKAimIKComponent)
		_DECLCOMINFO_PARAM_SIM(bool, FixTransforms, true, SetFixTransforms)
		_DECLCOMINFO_PARAM_SIM(float, Weight, 0.0f, SetWeight)
		_DECLCOMINFO_PARAM_SIM(float, PoleWeight, 0.0f, SetPoleWeight)
		_DECLCOMINFO_PARAM_SIM(float, Tolerance, 0.1f, SetTolerance)
		_DECLCOMINFO_PARAM_SIM(int, MaxIterations, 0, SetMaxIterations)
		_DECLCOMINFO_PARAM_SIM(float, ClampWeight, 0.0f, SetClampWeight)
		_DECLCOMINFO_PARAM_SIM(int, ClampSmoothing, 0, SetClampSmoothing)
		_DECLCOMINFO_PARAM_SIM(bool, UseRotationLimits, true, SetUseRotationLimits)
		_DECLCOMINFO_PARAM_SIM(bool, XY, false, SetXY)
		_DECLCOMINFO_PARAM(Vector3, Axis, Vector3::ZERO, SetAxis)
		_DECLCOMINFO_PARAM(Vector3, PoleAxis, Vector3::ZERO, SetPoleAxis)
		_DECLCOMINFO_PARAM(std::vector<NodeWeightInfo>, Bones, {}, SetBones)
		_DECLCOMINFO_PARAM(String, TargetNodeCompName, String(""), SetTargetNodeCompName)
		_DECLCOMINFO_PARAM(String, AimNodeCompName, String(""), SetAimNodeCompName)
		_DECLCOMINFO_PARAM(String, PoleTargetNodeName, String(""), SetPoleTargetNodeName)
		_DECLCOMINFO_PARAM_END()
	};

	DEFINECOMFACTORY(IKAimIKComponent, UNMOVEABLE)


}

#endif /*__EchoIKAimIKComponent_h__*/
