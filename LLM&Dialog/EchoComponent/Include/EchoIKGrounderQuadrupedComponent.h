//-----------------------------------------------------------------------------
// File:   EchoIKGrounderQuadrupedComponent.h
//
// Author: qiao_yuzhi
//
// Date:   2022-9-29
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef __EchoIKGrounderQuadrupedComponent_h__ 
#define __EchoIKGrounderQuadrupedComponent_h__

#include "EchoComponentPrerequisites.h"
#include "EchoProtocolComponent.h"
#include "Actor/EchoActor.h"
#include "EchoComDefineMacro.h"
#include "EchoIKBaseComponent.h"

namespace Echo {

	struct IKGrounderQuadrupedComponentData : public IKBaseComponentData {
		GrounderQuadrupedInfo mIKInfo;
		FinalIKInfoBase* GetIKInfo() override { return &mIKInfo; }
	};

	class _EchoComponentExport IKGrounderQuadrupedComponent : public IKBaseComponent {
		DECLAREACTORCOMTYPE_H(IKGrounderQuadrupedComponent)
			REGISTER_LUA_CLASS_KEY;
	public:
		virtual void SetIKInfo(FinalIKInfoBase* pInfo) override;
		void SetIKInfoData(const GrounderQuadrupedInfo& info);
		const GrounderQuadrupedInfo& GetIKInfoData();
		virtual void UseDefaultInitialize() override;
		virtual bool IsCanCreate() override;

		inline IKBaseComponentData* CreateCompData() override
		{
			return new IKGrounderQuadrupedComponentData();
		}
	public:
		//运行时更新
		void SetWeight(float value); 
		float GetWeight();
		void SetRootRotationWeight(float value); 
		float GetRootRotationWeight();
		void SetMinRootRotation(float value); 
		float GetMinRootRotation();
		void SetMaxRootRotation(float value); 
		float GetMaxRootRotation();
		void SetRootRotationSpeed(float value); 
		float GetRootRotationSpeed();
		void SetMaxLegOffset(float value); 
		float GetMaxLegOffset();
		void SetMaxForeLegOffset(float value); 
		float GetMaxForeLegOffset();
		void SetMaintainHeadRotationWeight(float value); 
		float GetMaintainHeadRotationWeight();
		void SetSolver(const GroundingInfo& value); 
		const GroundingInfo& GetSolver();
		void SetForeLegSolver(const GroundingInfo& value); 
		const GroundingInfo& GetForeLegSolver(); 
		//运行时不更新
		void SetPelvis(const Name& value);
		const Name& GetPelvis();
		void SetLastSpineBone(const Name& value);
		const Name& GetLastSpineBone();
		void SetHead(const Name& value);
		const Name& GetHead();
		void SetLegNames(const std::vector<Name>& value);
		const std::vector<Name>& GetLegNames();
		void SetForeLegNames(const std::vector<Name>& value);
		const std::vector<Name>& GetForeLegNames();
	};

	class _EchoComponentExport IKGrounderQuadrupedComponentInfo : public IKBaseComponentInfo {
	public:
		IKGrounderQuadrupedComponentInfo() {}
		IKGrounderQuadrupedComponentInfo(const Name& comType, ActorCOMMoveableType moveable) : IKBaseComponentInfo(comType, moveable) {}
		virtual ~IKGrounderQuadrupedComponentInfo() {}
		virtual void CopyToCompData(IKBaseComponentData* pData) override;
		virtual void CompDataToInfo(IKBaseComponentData* pData, IKBaseComponent* pComp) override;
		_DECLCOMINFO_VIRFUN
	public:
		_DECLCOMINFO_PARAM_BEGIN(IKGrounderQuadrupedComponent)
		_DECLCOMINFO_PARAM_SIM(float, Weight, 1.0f, SetWeight)
		_DECLCOMINFO_PARAM_SIM(float, RootRotationWeight, 0.5f, SetRootRotationWeight)
		_DECLCOMINFO_PARAM_SIM(float, MinRootRotation, -25.0f,SetMinRootRotation)
		_DECLCOMINFO_PARAM_SIM(float, MaxRootRotation, 45.0f,SetMaxRootRotation)
		_DECLCOMINFO_PARAM_SIM(float, RootRotationSpeed, 5.0f,SetRootRotationSpeed)
		_DECLCOMINFO_PARAM_SIM(float, MaxLegOffset, 0.5f,SetMaxLegOffset)
		_DECLCOMINFO_PARAM_SIM(float, MaxForeLegOffset, 0.5f,SetMaxForeLegOffset)
		_DECLCOMINFO_PARAM_SIM(float, MaintainHeadRotationWeight, 0.5f,SetMaintainHeadRotationWeight)
		_DECLCOMINFO_PARAM(Name, Pelvis, Name::g_cEmptyName,SetPelvis)
		_DECLCOMINFO_PARAM(Name, LastSpineBone, Name::g_cEmptyName,SetLastSpineBone)
		_DECLCOMINFO_PARAM(Name, Head, Name::g_cEmptyName,SetHead)
		_DECLCOMINFO_PARAM(std::vector<Name>, LegNames, {}, SetLegNames)
		_DECLCOMINFO_PARAM(std::vector<Name>, ForeLegNames, {},SetForeLegNames)
		_DECLCOMINFO_PARAM(GroundingInfo, Solver, GroundingInfo(),SetSolver)
		_DECLCOMINFO_PARAM(GroundingInfo, ForeLegSolver, GroundingInfo(), SetForeLegSolver)
		_DECLCOMINFO_PARAM_END()

	};

	DEFINECOMFACTORY(IKGrounderQuadrupedComponent, UNMOVEABLE)


}

#endif /*__EchoIKGrounderQuadrupedComponent_h__*/
