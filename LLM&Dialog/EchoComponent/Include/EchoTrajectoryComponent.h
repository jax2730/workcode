//-----------------------------------------------------------------------------
// File:   EchoTrajectoryComponent.h
//
// Author: wuyunlong
//
// Date:   2022-6-5
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#ifndef _EchoTrajectoryComponent_h__
#define _EchoTrajectoryComponent_h__

#include "EchoComponentPrerequisites.h"
#include "EchoProtocolComponent.h"

#include "EchoHermiteCurve.h"
#include "EchoHermite.h"
#include "Actor/EchoActor.h"
#include "Actor/EchoActorComponent.h"

#include "EchoMovableObject.h"
#include "EchoManualObject.h"
#include "EchoComDefineMacro.h"

namespace Echo
{
	//=============================================================================
	class _EchoComponentExport TrajectoryComponentInfo : public ActorComponentInfo
	{
	public:
		TrajectoryComponentInfo(const Name& comType, ActorCOMMoveableType MoveType = ALL_MOVEABLE);
		virtual ~TrajectoryComponentInfo();

		virtual void								ExportData(cJSON* jsonNode)override;
		virtual void								ImportData(cJSON* jsonNode)override;
		virtual void								CopyTo(ActorComponentInfo* comInfo)override;
		virtual void								OverWrite() override;
	public:
		EchoFloatHermite							mPosHermite[3];
		int											mMaxWeight;
	};
	//=============================================================================
	class _EchoComponentExport TrajectoryComponent : public NodeActorComponent
	{
		DECLAREACTORCOMTYPE_H(TrajectoryComponent)
	public:
		REGISTER_LUA_CLASS_KEY;
		
		void									CreatePoint(int weight,const Vector3& pos);
		void									RemovePoint(int weight);
		void									RemoveKey(int axis, int weight);

		Vector3									InterpolationPos(float weight);
		Vector3									InterpolationWorldPos(float weight);

		void									ShowTrajectoryHelper(bool isShow);
		bool									IsShowHelper() { return isShowHelper; }
		void									UpdateTrajectoryHelper();
		
		float									GetDistance();
		void									GetPosVec(float step, float wide);

		Vector3									GetPos(int i);

		virtual	void							setActor(const ActorPtr& pActor) override;
		virtual void                            createComRootNodeImpl() override;
	private:
		void									CreateHelper();
		void									ReleaseHelper();
		void									UpdateVertex();
	public:
		std::vector<Vector3>					posVec;
		float									distance;
	private:
		bool									isShowHelper;
		Echo::ManualObject*						mHelperObj;
	};
	DEFINECOMFACTORY(TrajectoryComponent, ALL_MOVEABLE)

	class _EchoComponentExport EchoTrajectoryProFuncImp
	{
	public:
		//-----------------------------------------------------------------------------
		static void								initProtocolFunction();
		static void								uninitProtocolFunction();

		static Echo::ProtocolResult				ProCom_ShowTrajectory(void* isShow, Echo::ProtocolHandle handle);
	};

	class EchoTrajectoryScript
	{
	public:
		REGISTER_LUA_CLASS_KEY;
		static bool								Script_ShowTrajectory(Echo::ProtocolHandle actHandle,const std::string& comID, bool isShow);
		static TrajectoryComponent *			Script_GetTrajectoryComponent(Actor* pActor, const std::string& name);
	};
}
// namespace Echo
#endif 
