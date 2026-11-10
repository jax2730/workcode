//-----------------------------------------------------------------------------
// File:  EchoEmitterComponent.h
//
// Author: LangGuangyue
//
// Date:   2024-01-08
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#ifndef _EchoEmitterComponent_H__
#define _EchoEmitterComponent_H__

#include "EchoComponentPrerequisites.h"
#include "Actor/EchoActor.h"
#include "EchoComDefineMacro.h"
#include "Effect/EchoEffectEmitterObject.h"
#include "Actor/EchoProtocolActorComponent.h"
#include "Effect/EchoEffectObjectData.h"

namespace Echo
{
	class EmitterComponent;
	class EmitterComponentInfo;

	class ActorEmitterVisualData : public EffectVisualData, public ActorLoadListener
	{
	public:
		ActorEmitterVisualData(EmitterComponent* com);
		~ActorEmitterVisualData();

		virtual void	reset() override;
		virtual bool	OnActorCreateFinish() override;

		void		init(const Echo::Name& actfile);
	public:
		ActorPtr		m_actor = nullptr;
		Echo::Name		m_actorPath;

		ActorComponentPtr  m_EmitterCom = nullptr;
	};

	class _EchoComponentExport EmitterComponent : public ProtocolNodeActorComponent, public EmitterComponentBase
	{
		DECLAREACTORCOMTYPE_H(EmitterComponent)
			REGISTER_LUA_CLASS_KEY;
	public:
		//ActorComponent
		virtual void					init(ActorComponentInfo* info) override;
		virtual void					unInit() override;
		virtual void					bindNode() override;
		virtual void					replay(ActorComponentInfo* info) override;
		virtual void					NotifiChildImp(int changeMask) override;

		//ProtocolComponent
		virtual void					registerTickTimeBegin() override;
		virtual void					beginFrame(EventID, ProtocolHandle) override;

		//EmitterComponentBase
		virtual const DVector3&			getWorldPosition() override;
		virtual const DVector3&			getLastWorldPosition() override;
		virtual const Vector3&			getScale() override;
		virtual void					setScale(const Vector3& scale) override;
		virtual const Matrix4&			getLocalMatrix() override;
		virtual const DMatrix4&			getWorldMatrix() override;

		virtual void					reset() override {};
		virtual void					freeze() override {};
		virtual void					active() override {};

	protected:
		virtual void					EditorPlayImp() override;
		virtual void					EditorPauseImp(bool pause) override;
		virtual void					EditorStopImp() override;

	public:
		void					tick();

		//发射物数量
		uint16					getEmittedActorNum();
		void					setEmittedActorNum(uint16 num);

		//发射器类型
		EffectEmitterType				getEmitterType();
		void					setEmitterType(EffectEmitterType type);

		//延迟时间
		float					getEmitterDelayTime();
		void					setEmitterDelayTime(float delayTime);

		//发射器生命周期
		float					getEmitterLifeTime();
		void					setEmitterLifeTime(float lifeTime);


		//持续发射时间
		float					getEmitterEmitTime();
		void					setEmitterEmitTime(float emitTime);


		//发射速率
		float					getEmitterPreSecond();
		void					setEmitterPreSecond(float speed);
		
		//是否按世界空间发射
		bool					getEmitterIsWorldSpace();
		void					setEmitterIsWorldSpace(bool isWorldSpace);

		//预热
		bool					getEmitterPreHeat();
		void					setEmitterPreHeat(bool isPreHeat);
		float					getEmitterPreHeatTime();
		void					setEmitterPreHeatTime(float preheatTime);

		void					setLiveForeverOrLoop(const int& val);
		int						getLiveForeverOrLoop();

		//随机发射方向
		bool					getIsActorRandomDirection();
		void					setIsActorRandomDirection(bool isRandom);

		/*发射actor的速度、声明周期、旋转、缩放
		在非固定的情况下，是处于两个数min和max之间的随机值
		在固定的情况下，取的min的值
		*/
		//声明周期
		bool					getIsFixedLifeTime();
		void					setIsFixedLifeTime(bool isFixed);
		float					getActorLifeTimeMin();
		void					setActorLifeTimeMin(float lifeTime);
		float					getActorLifeTimeMax();
		void					setActorLifeTimeMax(float lifeTime);

		//速度
		bool					getIsFixedSpeed();
		void					setIsFixedSpeed(bool isFixed);
		float					getActorSpeedMin();
		void					setActorSpeedMin(float speed);
		float					getActorSpeedMax();
		void					setActorSpeedMax(float speed);

		//旋转（欧拉角）
		bool					getIsFixedQuat();
		void					setIsFixedQuat(bool isFixed);
		Vector3					getActorQuatMin();
		void					setActorQuatMin(Vector3 Quat);
		Vector3					getActorQuatMax();
		void					setActorQuatMax(Vector3 Quat);

		//缩放
		bool					getIsFixedScale();
		void					setIsFixedScale(bool isFixed);
		Vector3					getActorScaleMin();
		void					setActorScaleMin(Vector3 Scale);
		Vector3					getActorScaleMax();
		void					setActorScaleMax(Vector3 Scale);

		void					UpdateActorEmitterFile(const Echo::Name& actFile, bool isSync = false);

	public:
		EffectEmitterData		m_EmitterData;
		Echo::Name			m_actorFile;
	protected:
		EffectEmitterObject*		m_pEmitterObj = nullptr;

		Vector3				scale = Vector3::UNIT_SCALE;

		DVector3				oldpos = DVector3::ZERO;
		DVector3				newpos = DVector3::ZERO;

		Matrix4				localMatrix;
		DMatrix4				worldMatrix;
	};

	class _EchoComponentExport EmitterComponentInfo : public ActorComponentInfo
	{
		friend class EmitterComponent;
	public:
		EmitterComponentInfo() {}
		EmitterComponentInfo(const Echo::Name& comType, ActorCOMMoveableType moveType);
		virtual ~EmitterComponentInfo();

#ifdef ECHO_EDITOR
		virtual bool					getNeedCollectedResource(std::unordered_multimap<ResourceType, std::string>& resMap) override;
#endif
		_DECLCOMINFO_VIRFUN
		EDITOR_REFLECT
	public:
		EffectEmitterData		emitterData;
		Echo::Name			m_actorFile;
	};

	void _EchoComponentExport WriteJSNode(cJSON* jsonNode, const char* Name, const EffectEmitterData& data);
	bool _EchoComponentExport ReadJSNode(cJSON* jsonNode, const char* Name, EffectEmitterData& data);

	DEFINECOMFACTORY(EmitterComponent, ALL_MOVEABLE)
}
#endif
