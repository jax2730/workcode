//-----------------------------------------------------------------------------
// File:   EchoIKTargetPointComponent.h
//
// Author: qiao_yuzhi
//
// Date:   2022-10-20
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef __EchoIKTargetPointComponent_H__
#define __EchoIKTargetPointComponent_H__

#include "EchoHelperObj.h"
#include "EchoActorRoleComponent.h"
#include "Actor/EchoNodeComponent.h"
#include "EchoBoneRenderComponent.h"
#include "EchoIKTargetFBBIKComponent.h"
#include "EchoIKPointComponent.h"
#include "IK/EchoIKSkeletonPosture.h"

namespace Echo
{
	class IKTargetPointComponent; typedef EchoWeakPtr<IKTargetPointComponent> IKTargetPointComponentPtr;
	using SkpeCallBack = std::function<void(IKTargetPointComponent*)>;

	//==================================================================================
	class _EchoComponentExport IKTargetPointComponent : public IKPointComponent
	{
		DECLAREACTORCOMTYPE_H(IKTargetPointComponent)
	public:
		virtual void	init(ActorComponentInfo* info) override;
		virtual void	unInit() override;
		virtual void	replay(ActorComponentInfo* info) override;
		virtual void	copyTo(const ActorComponentPtr& actCom) override;
		virtual void	connectCom() override;
		virtual void	disconnectCom() override;
	public:
		void SetSkeletonPostureName(const Name& value);
		const Name& GetSkeletonPostureName();
		const std::map<Name, Quaternion>& GetSkeletonPosture();
		FBBEffectorType GetIKPartType();
		void SetIsUseSkpeTransform(bool value);
		bool GetIsUseSkpeTransform();
		void SetSkpeCallBack(SkpeCallBack OnSetSkpe, SkpeCallBack OnResetSkpe);
	protected:
		EchoIK::IKSkeletonPostureBasePtr mSkeletonPosture;
		Name m_SkeletonPostureName = Name::g_cEmptyName;
		bool m_IsUseSkpeTransform = true;
		SkpeCallBack OnResetSkpe = nullptr;
		SkpeCallBack OnSetSkpe = nullptr;
#ifdef USE_IK_HELPEROBJ
		virtual void UpdateHelper() override;
#endif // USE_IK_HELPEROBJ
		static void AsyncLoadSkeletonPostureCallback(IKTargetPointComponentPtr& _thisPtr, EchoIK::IKSkeletonPostureBasePtr& resource);
	};

	class _EchoComponentExport IKTargetPointComponentInfo : public IKPointComponentInfo
	{
	public:
		IKTargetPointComponentInfo() {}
		IKTargetPointComponentInfo(const Name& mtype, ActorCOMMoveableType movableType = ALL_MOVEABLE);
		virtual ~IKTargetPointComponentInfo();
		_DECLCOMINFO_VIRFUN
	public:
		_DECLCOMINFO_PARAM_BEGIN(IKTargetPointComponent)
		_DECLCOMINFO_PARAM(Name, SkeletonPostureName, Name::g_cEmptyName, SetSkeletonPostureName)
		_DECLCOMINFO_PARAM_SIM(bool, IsUseSkpeTransform, true, SetIsUseSkpeTransform)
		_DECLCOMINFO_PARAM_END()
	};

	DEFINECOMFACTORY(IKTargetPointComponent, ALL_MOVEABLE);
}
#endif /*__EchoIKTargetPointComponent_H__*/
