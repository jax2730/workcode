//-----------------------------------------------------------------------------
// File:   EchoIKPointComponent.h
//
// Author: qiao_yuzhi
//
// Date:   2022-10-20
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef __EchoIKPointComponent_H__
#define __EchoIKPointComponent_H__

#include "EchoHelperObj.h"
#include "EchoComDefineMacro.h"
#include "EchoIKBaseComponent.h"


namespace Echo
{
	class IKPointComponent; typedef EchoWeakPtr<IKPointComponent> IKPointComponentPtr;

	//==================================================================================
	class _EchoComponentExport IKPointComponent : public NodeActorComponent
	{
		DECLAREACTORCOMTYPE_H(IKPointComponent)
	public:
		virtual void						init(ActorComponentInfo* info) override;
		virtual void						unInit() override;
		virtual void						replay(ActorComponentInfo* info) override;
		virtual void						copyTo(const ActorComponentPtr& actCom) override;
		virtual void						connectCom() override;
		virtual void						disconnectCom() override;
		virtual void						showEditorHelper(bool isShow) override;
		virtual void						updateCombinedName() override;
	public:
		ActorComponentPtr FindComponent(const String& compName);
		template<typename T>
		EchoWeakPtr<T> FindComponent(const String& compName) {
			return FindComponent(compName).getDynamicCastWeakPtr<T>();
		}
	public:
		void								SetHelperVisible(bool value);
		bool								GetHelperVisible();
		WorldNode*							GetWorldNode();
#ifdef USE_IK_HELPEROBJ
	protected:
		bool								IsVisible = true;
		HelperObject* m_pHelperObj = nullptr;
		void								CreateHelper();
		void								ReleaseHelper();
		virtual void						UpdateHelper();
#endif // USE_IK_HELPEROBJ
	};

	class _EchoComponentExport IKPointComponentInfo : public ActorComponentInfo
	{
	public:
		IKPointComponentInfo() {}
		IKPointComponentInfo(const Name& mtype, ActorCOMMoveableType movableType = ALL_MOVEABLE);
		virtual ~IKPointComponentInfo();

		_DECLCOMINFO_VIRFUN
	};

	DEFINECOMFACTORY(IKPointComponent, ALL_MOVEABLE);
}
#endif /*__EchoIKPointComponent_H__*/
