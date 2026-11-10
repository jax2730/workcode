//-----------------------------------------------------------------------------
// File:   EchoIKGrounderIKComponent.h
//
// Author: qiao_yuzhi
//
// Date:   2022-9-29
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef __EchoIKGrounderIKComponent_h__ 
#define __EchoIKGrounderIKComponent_h__

#include "EchoComponentPrerequisites.h"
#include "EchoProtocolComponent.h"
#include "Actor/EchoActor.h"
#include "EchoComDefineMacro.h"
#include "EchoIKBaseComponent.h"

namespace Echo {


	struct IKGrounderIKComponentData : public IKBaseComponentData {
		GrounderIKInfo mIKInfo;
		FinalIKInfoBase* GetIKInfo() override { return &mIKInfo; }
	};

	class _EchoComponentExport IKGrounderIKComponent : public IKBaseComponent {
		DECLAREACTORCOMTYPE_H(IKGrounderIKComponent)
			REGISTER_LUA_CLASS_KEY;
	public:
		virtual void SetIKInfo(FinalIKInfoBase* pInfo) override;
		void SetIKInfoData(const GrounderIKInfo& info);
		const GrounderIKInfo& GetIKInfoData();
		virtual void UseDefaultInitialize() override;
		virtual bool IsCanCreate() override;
		inline IKBaseComponentData* CreateCompData() override
		{
			return new IKGrounderIKComponentData();
		}
	public:
		//运行时更新
		void SetWeight(float value); 
		float GetWeight();
		void SetRootRotationWeight(float value); 
		float GetRootRotationWeight();
		void SetRootRotationSpeed(float value); 
		float GetRootRotationSpeed();
		void SetMaxRootRotationAngle(float value); 
		float GetMaxRootRotationAngle();
		void SetSolver(const GroundingInfo& value); 
		const GroundingInfo& GetSolver(); 
		//运行时不更新
		void SetPelvis(const Name& value);
		const Name& GetPelvis();
		void SetLegNames(const std::vector<Name>& value);
		const std::vector<Name>& GetLegNames();
	};

	class _EchoComponentExport IKGrounderIKComponentInfo : public IKBaseComponentInfo {
	public:
		IKGrounderIKComponentInfo() {}
		IKGrounderIKComponentInfo(const Name& comType, ActorCOMMoveableType moveable) : IKBaseComponentInfo(comType, moveable) {}
		virtual ~IKGrounderIKComponentInfo() {}
		virtual void CopyToCompData(IKBaseComponentData* pData) override;
		virtual void CompDataToInfo(IKBaseComponentData* pData, IKBaseComponent* pComp) override;
		_DECLCOMINFO_VIRFUN
	public:
		_DECLCOMINFO_PARAM_BEGIN(IKGrounderIKComponent)
		_DECLCOMINFO_PARAM_SIM(float, Weight, 1.0f, SetWeight)
		_DECLCOMINFO_PARAM_SIM(float, RootRotationWeight, 1.0f, SetRootRotationWeight)
		_DECLCOMINFO_PARAM_SIM(float, RootRotationSpeed, 5.0f, SetRootRotationSpeed)
		_DECLCOMINFO_PARAM_SIM(float, MaxRootRotationAngle, 45.0f, SetMaxRootRotationAngle)
		_DECLCOMINFO_PARAM(Name, Pelvis, Name::g_cEmptyName, SetPelvis)
		_DECLCOMINFO_PARAM(std::vector<Name>, LegNames, {}, SetLegNames)
		_DECLCOMINFO_PARAM(GroundingInfo, Solver, GroundingInfo(), SetSolver)
		_DECLCOMINFO_PARAM_END()
	};

	DEFINECOMFACTORY(IKGrounderIKComponent, UNMOVEABLE)


}

#endif /*__EchoIKGrounderIKComponent_h__*/
