//-----------------------------------------------------------------------------
// File:   EchoIKGrounderFBBIKComponent.h
//
// Author: qiao_yuzhi
//
// Date:   2022-9-29
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef __EchoIKGrounderFBBIKComponent_h__ 
#define __EchoIKGrounderFBBIKComponent_h__

#include "EchoComponentPrerequisites.h"
#include "EchoProtocolComponent.h"
#include "Actor/EchoActor.h"
#include "EchoComDefineMacro.h"
#include "EchoIKBaseComponent.h"

namespace Echo {

	struct IKGrounderFBBIKComponentData : public IKBaseComponentData {
		GrounderFBBIKInfo mIKInfo;
		FinalIKInfoBase* GetIKInfo() override { return &mIKInfo; }
	};

class _EchoComponentExport IKGrounderFBBIKComponent : public IKBaseComponent{
	DECLAREACTORCOMTYPE_H(IKGrounderFBBIKComponent)
	REGISTER_LUA_CLASS_KEY;
public:
	virtual void SetIKInfo(FinalIKInfoBase* pInfo) override;
	void SetIKInfoData(const GrounderFBBIKInfo& info);
	const GrounderFBBIKInfo& GetIKInfoData();
	virtual void UseDefaultInitialize() override;
	virtual bool IsCanCreate() override;
	inline IKBaseComponentData* CreateCompData() override
	{
		return new IKGrounderFBBIKComponentData();
	}
public:
	//运行时更新
	void SetWeight(float value);
	float GetWeight();
	void SetSpineBend(float value);
	float GetSpineBend();
	void SetSpineSpeed(float value);
	float GetSpineSpeed();
	void SetSolver(const GroundingInfo& value);
	const GroundingInfo& GetSolver();
	//运行时不更新
	void SetFullBodyBipedIKName(const Name& ikName);
	const Name& GetFullBodyBipedIKName();
	void SetSpine(const std::vector<SpineEffectorInfo>& value);
	const std::vector<SpineEffectorInfo>& GetSpine();
	EchoIK::FullBodyBipedIK* GetFullBodyBipedIK();
};

class _EchoComponentExport IKGrounderFBBIKComponentInfo : public IKBaseComponentInfo {
public:
	IKGrounderFBBIKComponentInfo() {}
	IKGrounderFBBIKComponentInfo(const Name& comType, ActorCOMMoveableType moveable) : IKBaseComponentInfo(comType, moveable) {}
	virtual ~IKGrounderFBBIKComponentInfo() {}
	virtual void CopyToCompData(IKBaseComponentData* pData) override;
	virtual void CompDataToInfo(IKBaseComponentData* pData, IKBaseComponent* pComp) override;
	_DECLCOMINFO_VIRFUN
public:
	_DECLCOMINFO_PARAM_BEGIN(IKGrounderFBBIKComponent)
	_DECLCOMINFO_PARAM_SIM(float, Weight, 0.0f, SetWeight)
	_DECLCOMINFO_PARAM_SIM(float, SpineBend, 2.0f, SetSpineBend)
	_DECLCOMINFO_PARAM_SIM(float, SpineSpeed, 3.0f, SetSpineSpeed)
	_DECLCOMINFO_PARAM(Name, FullBodyBipedIKName, Name::g_cEmptyName, SetFullBodyBipedIKName)
	_DECLCOMINFO_PARAM(GroundingInfo, Solver, GroundingInfo(), SetSolver)
	_DECLCOMINFO_PARAM(std::vector<SpineEffectorInfo>, Spine, {}, SetSpine)
	_DECLCOMINFO_PARAM_END()

};

DEFINECOMFACTORY(IKGrounderFBBIKComponent, UNMOVEABLE)


}

#endif /*__EchoIKGrounderFBBIKComponent_h__*/
