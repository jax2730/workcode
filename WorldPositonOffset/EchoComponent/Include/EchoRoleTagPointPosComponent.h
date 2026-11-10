//-----------------------------------------------------------------------------
// File:  EchoRoleTagPointPosComponent.h
//
// Author: LangGuangyue
//
// Date:   2022-8-10
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef __EchoRoleTagPointPosComponent_H__
#define __EchoRoleTagPointPosComponent_H__

#include "EchoHelperObj.h"
#include "EchoActorRoleComponent.h"
#include "Actor/EchoNodeComponent.h"
#include "EchoComDefineMacro.h"
#include "EchoComInfoDefineMacro.h"
#include "EchoIKPointComponent.h"
#include "EchoWorldNode.h"
#include "EchoTagPoint.h"


namespace Echo
{
	class TagPointPosComponent; typedef EchoWeakPtr<TagPointPosComponent> TagPointPosComponentPtr;

	class _EchoComponentExport TagPointPosComponent : public IKPointComponent, public RoleObjectListener
	{
		DECLAREACTORCOMTYPE_H(TagPointPosComponent)
	public:
		virtual void						create(ActorComponentInfo* info) override;
		virtual void						init(ActorComponentInfo* info) override;
		virtual void						unInit() override;
		virtual void						bindNode()override;
		virtual void						replay(ActorComponentInfo* info) override;
		virtual void						copyTo(const ActorComponentPtr& actCom) override;
		virtual void						connectCom() override;
		virtual void						disconnectCom() override;
		virtual void						updateCombinedName() override;
		virtual bool						OnRoleCreateFinish() override;
		virtual void						OnRoleDestroy() override;
		virtual void						NotifiChildImp(int changeMask) override;
		virtual void                        selfVisibleStateChanged(Actor_Visible_Type type) override;
		void								OnNodeTransformUpdate(bool bUpdate);
	/************************************************************************************************************/
	public:
		RoleComponent*						GetRoleComponent();
		void								GetRoleTagPointList(std::vector<Name>& tagPoints);
		RoleTagPoint*						GetRoleTagPoint();
	/************************************************************************************************************/
	public:
		const Name&							GetTagPointName();
		void								SetTagPointName(const Name& value);
		const String&						GetOtherTagPointPosCompName();
		void								SetOtherTagPointPosCompName(const String& value);
		const TagPointPosComponentPtr&		GetOtherTagPointPosCompPtr();
		void								SetOtherTagPointPosCompPtr(const TagPointPosComponentPtr& value);

		//已废弃
		const Vector3&						GetLastFrameWorldPosition() { return Vector3::ZERO; }
		//已废弃
		const Quaternion&					GetLastFrameWorldRotation() { return Quaternion::IDENTITY; }
		//已废弃
		void								AddTransformUpdateFunc(void* ptr, const std::function<void()>& func) {}
		//已废弃
		void								RemoveTransformUpdateFunc(void* ptr) {}
	private:
		void								AddToWorldNode();
		void								RemoveFromWorldNode();
		void								CalculateTagPointPos(Transform& trans);
		void								SetOffsetTransform(const Vector3& OffsetPosNeg,const Quaternion& OffsetOrientationNeg, bool bUpdate);
		//void								RecordWorldTransform();
		RoleTagPoint*						_GetRoleTagPoint();
	private:
		Name								m_TagPointName;
		String								m_OtherTagPointPosCompName;
		TagPoint*							GetTagNode();
		TagPoint*							m_pTagNode = nullptr;

		bool								mIsExistOtherTagPoint = false;
		bool								mbInverse = true;
		bool								mIsUpdating = false;
		Vector3								mvOffsetPosNeg = Vector3::ZERO;
		Quaternion							mrOffsetOrientationNeg = Quaternion::IDENTITY;
		TagPointPosComponentPtr				m_OtherTagPointPosCompPtr = nullptr;
		//Transform							m_LastFrameWorldTransform;
		//std::unordered_map<void*, std::function<void()>> m_TransformUpdateFunc;
		friend class EchoIKManager;
	};

	class _EchoComponentExport TagPointPosComponentInfo : public IKPointComponentInfo
	{
	public:
		TagPointPosComponentInfo() {}
		TagPointPosComponentInfo(const Name& mtype, ActorCOMMoveableType movableType = ALL_MOVEABLE);
		virtual ~TagPointPosComponentInfo();
		_DECLCOMINFO_VIRFUN
	public:
		_DECLCOMINFO_PARAM_BEGIN(TagPointPosComponent)
		_DECLCOMINFO_PARAM(Name, TagPointName, Name::g_cEmptyName, SetTagPointName)
		_DECLCOMINFO_PARAM(String, OtherTagPointPosCompName, String(""), SetOtherTagPointPosCompName)
		_DECLCOMINFO_PARAM_END()
	};

	DEFINECOMFACTORY(TagPointPosComponent, ALL_MOVEABLE);
}
#endif
