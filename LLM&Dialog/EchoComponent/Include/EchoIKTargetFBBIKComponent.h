//-----------------------------------------------------------------------------
// File:   EchoIKTargetFBBIKComponent.h
//
// Author: qiao_yuzhi
//
// Date:   2022-9-29
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef __EchoIKTargetFBBIKComponent_h__ 
#define __EchoIKTargetFBBIKComponent_h__

#include "EchoComDefineMacro.h"
#include "EchoIKBaseComponent.h"
#include "IK/EchoTargetFBBIK.h"

namespace Echo {

	typedef EchoIK::FullBodyBipedEffector FBBEffectorType;
	typedef EchoIK::FullBodyBipedChain FBBChainType;

	struct IKTargetFBBIKComponentData : public IKBaseComponentData {
		TargetFBBIKInfo mIKInfo;
		std::array<String, FBBEffectorType::NumEffectors> TargetPointCompNames;
		std::array<String, FBBChainType::NumChains> BendGoalPointCompNames;
		FinalIKInfoBase* GetIKInfo() override { return &mIKInfo; }
	};

class _EchoComponentExport IKTargetFBBIKComponent : public IKBaseComponent{
	DECLAREACTORCOMTYPE_H(IKTargetFBBIKComponent)
	REGISTER_LUA_CLASS_KEY;
public:
	virtual void SetCompData(IKBaseComponentData* pData) override;
	virtual void SetIKInfo(FinalIKInfoBase* pInfo) override;
	void SetIKInfoData(const TargetFBBIKInfo& info);
	const TargetFBBIKInfo& GetIKInfoData();
	virtual void UseDefaultInitialize() override;
	virtual bool IsCanCreate() override;
	inline IKBaseComponentData* CreateCompData() override
	{
		return new IKTargetFBBIKComponentData();
	}
public:
	//运行时不更新
	void SetFullBodyBipedIKName(const Name& ikName);
	const Name& GetFullBodyBipedIKName();
	//运行时更新
	void SetTargetPointCompName(FBBEffectorType effectorIndex, const String& name);
	const String& GetTargetPointCompName(FBBEffectorType effectorIndex);
	void SetBodyTargetPointCompName(const String& name);
	void SetLeftHandTargetPointCompName(const String& name);
	void SetLeftShoulderTargetPointCompName(const String& name);
	void SetRightHandTargetPointCompName(const String& name);
	void SetRightShoulderTargetPointCompName(const String& name);
	void SetLeftFootTargetPointCompName(const String& name);
	void SetLeftThighTargetPointCompName(const String& name);
	void SetRightFootTargetPointCompName(const String& name);
	void SetRightThighTargetPointCompName(const String& name);
	const String& GetBodyTargetPointCompName();
	const String& GetLeftHandTargetPointCompName();
	const String& GetLeftShoulderTargetPointCompName();
	const String& GetRightHandTargetPointCompName();
	const String& GetRightShoulderTargetPointCompName();
	const String& GetLeftFootTargetPointCompName();
	const String& GetLeftThighTargetPointCompName();
	const String& GetRightFootTargetPointCompName();
	const String& GetRightThighTargetPointCompName();

	void SetBendGoalPointCompName(FBBChainType chainIndex, const String& name);
	const String& GetBendGoalPointCompName(FBBChainType chainIndex);
	void SetLeftArmPointCompName(const String& name);
	void SetLeftLegPointCompName(const String& name);
	void SetRightArmPointCompName(const String& name);
	void SetRightLegPointCompName(const String& name);
	const String& GetLeftArmPointCompName();
	const String& GetLeftLegPointCompName();
	const String& GetRightArmPointCompName();
	const String& GetRightLegPointCompName();
public://运行时接口
	void SetFBBEffectorWeight(FBBEffectorType effectIndex, float positionWeight, float rotationWeight);
	void SetFBBChainWeight(FBBChainType chainIndex, float weight);
	void SetSkeletonPosture(const std::map<Name, Quaternion>& boneMap, bool IsSet = true);
	void ResetSkeletonPosture(const std::map<Name, Quaternion>& boneMap);
public:
	void SetFBBTarget(FBBEffectorType effectorIndex, const ActorComponentPtr& pComp);
	const ActorComponentPtr& GetFBBTarget(FBBEffectorType effectorIndex) { return mTargetPtrs[effectorIndex]; }
	void SetFBBBendGoal(FBBChainType chainIndex, const ActorComponentPtr& pComp);
	const ActorComponentPtr& GetFBBBendGoal(FBBChainType chainIndex) { return mBendGoalPtrs[chainIndex]; }
	EchoIK::FullBodyBipedIK* GetFullBodyBipedIK();
	void ResetFBBTarget(FBBEffectorType effectorIndex, const ActorComponentPtr& pComp) {}
	void ResetFBBBendGoal(FBBChainType chainIndex, const ActorComponentPtr& pComp) {}
public:
	void ResetFBBTarget(FBBEffectorType effectorIndex);
	void ResetFBBBendGoal(FBBChainType chainIndex);
protected:
	virtual void PostCreateIK() override;
	virtual void PreDestroyIK() override;
	void ApplyTargetPtr(FBBEffectorType type);
	void ApplyBendGoalPtr(FBBChainType type);
	void ApplyTargetPtrs();
	void ApplyBendGoalPtrs();
	void ResetFBBTargets();
	void ResetFBBBendGoals();
	std::array<ActorComponentPtr, FBBEffectorType::NumEffectors> mTargetPtrs;
	std::array<ActorComponentPtr, FBBChainType::NumChains> mBendGoalPtrs;
};

class _EchoComponentExport IKTargetFBBIKComponentInfo : public IKBaseComponentInfo {
public:
	IKTargetFBBIKComponentInfo() {}
	IKTargetFBBIKComponentInfo(const Name& comType, ActorCOMMoveableType moveable) : IKBaseComponentInfo(comType, moveable) {}
	virtual ~IKTargetFBBIKComponentInfo() {}
	virtual void CopyToCompData(IKBaseComponentData* pData) override;
	virtual void CompDataToInfo(IKBaseComponentData* pData, IKBaseComponent* pComp) override;
	_DECLCOMINFO_VIRFUN
public:
	void SetTargetPointCompName(FBBEffectorType effectorIndex, const String& name, IKTargetFBBIKComponent* pComp);
	const String& GetTargetPointCompName(FBBEffectorType effectorIndex);
	void SetBendGoalPointCompName(FBBChainType chainIndex, const String& name, IKTargetFBBIKComponent* pComp);
	const String& GetBendGoalPointCompName(FBBChainType chainIndex);
public:
	_DECLCOMINFO_PARAM_BEGIN(IKTargetFBBIKComponent)
	_DECLCOMINFO_PARAM(Name, FullBodyBipedIKName, Name::g_cEmptyName, SetFullBodyBipedIKName)
	_DECLCOMINFO_PARAM(String, BodyTargetPointCompName, String(""), SetBodyTargetPointCompName)
	_DECLCOMINFO_PARAM(String, LeftHandTargetPointCompName, String(""), SetLeftHandTargetPointCompName)
	_DECLCOMINFO_PARAM(String, LeftShoulderTargetPointCompName, String(""), SetLeftShoulderTargetPointCompName)
	_DECLCOMINFO_PARAM(String, RightHandTargetPointCompName, String(""), SetRightHandTargetPointCompName)
	_DECLCOMINFO_PARAM(String, RightShoulderTargetPointCompName, String(""), SetRightShoulderTargetPointCompName)
	_DECLCOMINFO_PARAM(String, LeftFootTargetPointCompName, String(""), SetLeftFootTargetPointCompName)
	_DECLCOMINFO_PARAM(String, LeftThighTargetPointCompName, String(""), SetLeftThighTargetPointCompName)
	_DECLCOMINFO_PARAM(String, RightFootTargetPointCompName, String(""), SetRightFootTargetPointCompName)
	_DECLCOMINFO_PARAM(String, RightThighTargetPointCompName, String(""), SetRightThighTargetPointCompName)
	_DECLCOMINFO_PARAM(String, LeftArmPointCompName, String(""), SetLeftArmPointCompName)
	_DECLCOMINFO_PARAM(String, LeftLegPointCompName, String(""), SetLeftLegPointCompName)
	_DECLCOMINFO_PARAM(String, RightArmPointCompName, String(""), SetRightArmPointCompName)
	_DECLCOMINFO_PARAM(String, RightLegPointCompName, String(""), SetRightLegPointCompName)
	_DECLCOMINFO_PARAM_END()
};

DEFINECOMFACTORY(IKTargetFBBIKComponent, UNMOVEABLE)


}

#endif /*__EchoIKTargetFBBIKComponent_h__*/
