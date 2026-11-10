
#ifndef __EchoAggregateComponent_H__
#define __EchoAggregateComponent_H__

#include "EchoComponentPrerequisites.h"
#include "EchoProtocolComponent.h"
#include "EchoPhysicsManager.h"
#include "EchoComInfoDefineMacro.h"
#include "EchoComDefineMacro.h"


namespace Echo
{
	struct AggregateComponentData {
		std::vector<String> PhysicsCompNames;
		bool IsCollision = false;
	};

	class AggregateComponentInfo;

	class _EchoComponentExport AggregateComponent: public PhysicsLinkComponent {
	public:
		DECLAREACTORCOMTYPE_H(AggregateComponent)
		REGISTER_LUA_CLASS_KEY;
	public:
		virtual void init(ActorComponentInfo* pInfo) override final;
		virtual void unInit() override final;
		virtual void replay(ActorComponentInfo* pInfo) override final;
		virtual void copyTo(const ActorComponentPtr& newComp) override final;
		virtual void connectCom() override final;
		virtual void disconnectCom() override final;
		virtual void connectActor() override final;
		virtual void disconnectActor() override final;
	public:
		virtual void CreatePhysics() override final;
		virtual void ReleasePhysics() override final;
		virtual bool IsExistPhysics()override final;
		virtual bool IsCanPhysics()	 override final;
#ifdef USE_PHYSICS_HELPEROBJ
		virtual void UpdateComponentInEditor() override final {}
#endif // USE_PHYSICS_HELPEROBJ
		PhysXAggregate* GetPhysics();
		virtual void _actorNotifyPhysX(PhysicsActorComponent* pComp, EchoPhysXNotifyType type) override;
	public:
		void SetIsCollision(bool value);
		bool GetIsCollision();
		void SetPhysicsCompNames(const std::vector<String>& value);
		const std::vector<String>& GetPhysicsCompNames();

		void SetCompData(AggregateComponentData* pData);
		AggregateComponentData* GetCompData();
	public://运行时使用
		void SetUsePointer(bool value);
		inline bool GetUsePointer() { return bUsePointer; }
		void SetPhysicsCompPtrs(const std::vector<PhysicsActorComponentPtr>& value, bool bForce = false);
		const std::vector<PhysicsActorComponentPtr>& GetPhysicsCompPtrs();
	private:
		virtual EchoPhysicsComponentType GetPhysicsType() override { return EchoPhysicsComponentType::eAGGREGATE; }
		void AttachPhysXActorComp(bool bForce = false);
		void DetachPhysXActorComp();
		bool GetSelfPhysXActors(std::set<PhysicsActor*>& actors);
	private:
		bool bUsePointer = false;
		PhysXAggregate* m_pAggregate = nullptr;
		AggregateComponentData mCompData;
		std::vector<PhysicsActorComponentPtr> mPhysicsCompPtrs;
	private:
		friend class EchoPhysicsSystem;
		friend class EchoPhysicsManager;
	};

	class _EchoComponentExport AggregateComponentInfo : public ActorComponentInfo {
	public:
		AggregateComponentInfo() {}
		AggregateComponentInfo(const Name& comType, ActorCOMMoveableType MoveType);
		virtual ~AggregateComponentInfo();
		_DECLCOMINFO_VIRFUN
	public:
		virtual void CopyToCompData(AggregateComponentData* pData);
	public:
		_DECLCOMINFO_PARAM_BEGIN(AggregateComponent)
		_DECLCOMINFO_PARAM_SIM(bool, IsCollision, false, SetIsCollision);
		_DECLCOMINFO_PARAM(std::vector<String>, PhysicsCompNames, std::vector<String>(), SetPhysicsCompNames);
		_DECLCOMINFO_PARAM_END()
	};


	DEFINECOMFACTORY(AggregateComponent, UNMOVEABLE)

}

#endif // !__EchoAggregateComponent_H__
