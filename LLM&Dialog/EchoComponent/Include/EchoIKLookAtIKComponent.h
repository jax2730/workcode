//-----------------------------------------------------------------------------
// File:   EchoIKLookAtIKComponent.h
//
// Author: qiao_yuzhi
//
// Date:   2022-9-29
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef __EchoIKLookAtIKComponent_h__ 
#define __EchoIKLookAtIKComponent_h__

#include "EchoComponentPrerequisites.h"
#include "EchoProtocolComponent.h"
#include "Actor/EchoActor.h"
#include "EchoComDefineMacro.h"
#include "EchoIKBaseComponent.h"

namespace Echo {

	struct IKLookAtIKComponentData : public IKBaseComponentData {
		LookAtIKInfo mIKInfo;
		String TargetNodeCompName;
		FinalIKInfoBase* GetIKInfo() override { return &mIKInfo; }
	};

	class _EchoComponentExport IKLookAtIKComponent : public IKBaseComponent {
		DECLAREACTORCOMTYPE_H(IKLookAtIKComponent)
			REGISTER_LUA_CLASS_KEY;
	public:
		virtual void SetCompData(IKBaseComponentData* pData) override;
		virtual void SetIKInfo(FinalIKInfoBase* pInfo) override;
		void SetIKInfoData(const LookAtIKInfo& info);
		const LookAtIKInfo& GetIKInfoData();
		virtual void UseDefaultInitialize() override;
		virtual bool IsCanCreate() override;
		inline IKBaseComponentData* CreateCompData() override
		{
			return new IKLookAtIKComponentData();
		}
	public:
		//运行时更新
		void SetTargetNodeCompName(const String& value);
		const String& GetTargetNodeCompName();
		void SetFixTransforms(bool value); 
		bool GetFixTransforms();
		void SetWeight(float value); 
		float GetWeight();
		void SetBodyWeight(float value); 
		float GetBodyWeight();
		void SetHeadWeight(float value); 
		float GetHeadWeight();
		void SetEyesWeight(float value); 
		float GetEyesWeight();
		void SetClampWeight(float value); 
		float GetClampWeight();
		void SetClampWeightHead(float value); 
		float GetClampWeightHead();
		void SetClampWeightEyes(float value); 
		float GetClampWeightEyes();
		void SetClampSmoothing(int value); 
		int GetClampSmoothing();
		//运行时不更新
		void SetHead(const Name& value); const Name& GetHead();
		void SetSpine(const std::vector<Name>& value); const std::vector<Name>& GetSpine();
		void SetEyes(const std::vector<Name>& value); const std::vector<Name>& GetEyes();
	public:
		void SetTargetNode(const ActorComponentPtr& pComp);
		const ActorComponentPtr& GetTargetNode() { return mTargetPtr; }
	public://运行时接口
		void SetTargetNode(WorldNode* pNode);//不推荐
	protected:
		virtual void PostCreateIK() override;
		void ApplyTargetPtr();
		ActorComponentPtr mTargetPtr = nullptr;
	};

	class _EchoComponentExport IKLookAtIKComponentInfo : public IKBaseComponentInfo {
	public:
		IKLookAtIKComponentInfo() {}
		IKLookAtIKComponentInfo(const Name& comType, ActorCOMMoveableType moveable) : IKBaseComponentInfo(comType, moveable) {}
		virtual ~IKLookAtIKComponentInfo() {}
		virtual void CopyToCompData(IKBaseComponentData* pData) override;
		virtual void CompDataToInfo(IKBaseComponentData* pData, IKBaseComponent* pComp) override;
		_DECLCOMINFO_VIRFUN
	public:
		_DECLCOMINFO_PARAM_BEGIN(IKLookAtIKComponent)
		_DECLCOMINFO_PARAM_SIM(bool, FixTransforms, true, SetFixTransforms)               
		_DECLCOMINFO_PARAM_SIM(float, Weight, 1.0f, SetWeight)				
		_DECLCOMINFO_PARAM_SIM(float, BodyWeight, 0.216f, SetBodyWeight)
		_DECLCOMINFO_PARAM_SIM(float, HeadWeight, 1.0f, SetHeadWeight)					
		_DECLCOMINFO_PARAM_SIM(float, EyesWeight, 1.0f, SetEyesWeight)						
		_DECLCOMINFO_PARAM_SIM(float, ClampWeight, 0.5f, SetClampWeight)					
		_DECLCOMINFO_PARAM_SIM(float, ClampWeightHead, 0.5f, SetClampWeightHead)
		_DECLCOMINFO_PARAM_SIM(float, ClampWeightEyes, 0.5f, SetClampWeightEyes)          
		_DECLCOMINFO_PARAM_SIM(int, ClampSmoothing, 2, SetClampSmoothing)           
		_DECLCOMINFO_PARAM(Name, Head, Name::g_cEmptyName, SetHead)					
		_DECLCOMINFO_PARAM(std::vector<Name>, Spine, {}, SetSpine)
		_DECLCOMINFO_PARAM(std::vector<Name>, Eyes, {}, SetEyes)
		_DECLCOMINFO_PARAM(String, TargetNodeCompName, "", SetTargetNodeCompName)
		_DECLCOMINFO_PARAM_END()
	};

	DEFINECOMFACTORY(IKLookAtIKComponent, UNMOVEABLE)


}

#endif /*__EchoIKLookAtIKComponent_h__*/
