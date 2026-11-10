#include "EchoComponentStableHeaders.h"
#include "EchoRigidBodyGroupComponent.h"
#include "EchoRigidBodyComponent.h"
#include "EchoCharactorControllerComponent.h"
#include "Actor/EchoActorManager.h"

#include "EchoPhysXAggregate.h"
#include "EchoPhysXCharacterController.h"
#include "EchocJsonHelper.h"

//#define GETINFO()			(static_cast<RigidBodyGroupComponentInfo*> (m_pActComInfo))
//#define CASTINFO(pInfo)		(static_cast<RigidBodyGroupComponentInfo*> (pInfo))
//#define CASTCOMP(pComp)		(static_cast<RigidBodyGroupComponent*> (pComp.get()))
//#define IF_RIGIDBODYCANNOTADD(rigidbody)	if (rigidbody == nullptr || rigidbody->getType() != COMPONENTTYPE(RigidBodyComponent) || rigidBodyInGroup(rigidbody))
//#define IF_RIGIDBODYNOTFIND(rigidbody)		if (rigidbody == nullptr || rigidbody->getType() != COMPONENTTYPE(RigidBodyComponent))
//namespace Echo
//{
//
//	EDITOR_REFLECT_BEGIN(RigidBodyGroupComponentInfo)
//		EDITOR_REFLECT_MEMBER_UPDATE(rigidBodys, "rigidBodys", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
//		EDITOR_REFLECT_MEMBER_UPDATE(disableCollision,"enableCollision", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
//	EDITOR_REFLECT_END();
//	_DEFINEINFO_VIRFUN_BASE(RigidBodyGroupComponentInfo)
//	
//	DECLAREACTORCOMTYPE_CPP (RigidBodyGroupComponent)
//	RigidBodyGroupComponent::RigidBodyGroupComponent(ActorPtr pActor, ActorCOMMoveableType type) : PhysicsBaseComponent(pActor, type)
//	{}
//	std::unordered_set<ActorComponent*>	RigidBodyGroupComponent::RigidBodyMap;
//
//	RigidBodyGroupComponent::~RigidBodyGroupComponent()
//	{
//		_delAllRigidBody();
//		ReleasePhysics();
//	}
//	void RigidBodyGroupComponent::init(ActorComponentInfo* pAInfo)
//	{
//		// init on connectCom
//	}
//	void RigidBodyGroupComponent::replay(ActorComponentInfo* pAInfo)
//	{
//		resetRigidBodys(CASTINFO(pAInfo)->rigidBodys);
//		disableCollision(CASTINFO(pAInfo)->disableCollision);
//	}
//	void RigidBodyGroupComponent::connectCom()
//	{
//		PhysicsBaseComponent::connectCom();
//		resetRigidBodys(GETINFO()->rigidBodys);
//		disableCollision(GETINFO()->disableCollision);
//	}
//	void RigidBodyGroupComponent::dealComponentDelete(const ActorComponentPtr& actCom)
//	{
//		if (std::find(mTeamMembers.begin(), mTeamMembers.end(), actCom) != mTeamMembers.end())
//		{
//			_delRigidBody(actCom);
//			if (IsExistPhysics())
//				mRigidBodyAggregate->removeActor(((RigidBodyComponent*)actCom.get())->GetPhysics());
//		}
//	}
//	void RigidBodyGroupComponent::CreatePhysics()
//	{
//		if (mRigidBodyAggregate != nullptr)
//			return;
//		
//		size_t rigidBodyCount = mTeamMembers.size();
//		if (rigidBodyCount == 0)
//			return;
//
//		//SynchronousPhysics();	// 填充数据
//		checkAndAllocAggregateSize((uint32)rigidBodyCount);
//
//		if (!IsExistPhysics()) return;
//		for (auto& el : mTeamMembers)
//		{
//			RigidBodyComponent* pRigidBody = static_cast<RigidBodyComponent*> (el.get());
//			if (pRigidBody) {
//				pRigidBody->AddListener(getWeakPtr<RigidBodyGroupComponent>().getStaticCastWeakPtr<EchoRigidBodyCallBack>());
//			}
//		}
//		SynchronousPhysics();
//	}
//	void RigidBodyGroupComponent::ReleasePhysics()
//	{
//		if (!IsExistPhysics()) return;
//
//		for (auto& el : mTeamMembers)
//		{
//			RigidBodyComponent* pRigidBody = static_cast<RigidBodyComponent*> (el.get());
//			if (pRigidBody)
//				pRigidBody->RemoveListener(getWeakPtr<RigidBodyGroupComponent>().getStaticCastWeakPtr<EchoRigidBodyCallBack>());
//		}
//
//		PhysXSystem::instance()->getScene()->RemoveAggregate(mRigidBodyAggregate);
//		mRigidBodyAggregate = nullptr;
//	}
//	void RigidBodyGroupComponent::SynchronousPhysics()
//	{
//		std::vector<PhysicsActor*> actors;
//		for (auto& el : mTeamMembers)
//		{
//			RigidBodyComponent* pRigidBody = static_cast<RigidBodyComponent*> (el.get());
//			if (!pRigidBody || !pRigidBody->IsExistPhysics()) continue;
//			actors.push_back(pRigidBody->GetPhysics());
//		}
//		if (actors.size() < 2) {
//			return;
//		}
//		if (mRigidBodyAggregate)
//			assert(checkAndAllocAggregateSize((uint32)(actors.size()) + mRigidBodyAggregate->getNbActors()));
//		else
//			assert(checkAndAllocAggregateSize((uint32)(actors.size() * 1.5f )));
//
//		for (PhysicsActor* pPA : actors)
//		{
//			mRigidBodyAggregate->addActor(pPA);
//		}
//	}
//	bool RigidBodyGroupComponent::IsCanPhysics()
//	{
//		return true;
//	}
//	bool RigidBodyGroupComponent::IsExistPhysics()
//	{
//		return mRigidBodyAggregate != nullptr;
//	}
//	void RigidBodyGroupComponent::UpdateComponentInEditor()
//	{
//		RemoveFromEditorUpdateQueue();
//	}
//	std::vector<ActorComponentPtr> RigidBodyGroupComponent::getAllRigidBodyComponent()
//	{
//		return std::vector<ActorComponentPtr> (mTeamMembers.begin(), mTeamMembers.end()) ;
//	}
//	std::vector<Name> RigidBodyGroupComponent::getAllRigidBodyCompName()
//	{
//		std::vector<Name> res ;
//		for (auto& el : mTeamMembers)
//			res.push_back(el->getComName());
//		return res;
//	}
//	size_t RigidBodyGroupComponent::getRigidBodyCount()
//	{
//		return mTeamMembers.size();
//	}
//	bool RigidBodyGroupComponent::isDisableCollision()
//	{
//		return mDisableCollision;
//	}
//
//	bool RigidBodyGroupComponent::addRigidBody(const Name& comName)
//	{
//		ActorComponentPtr rigidbody = FindComponent(comName);
//
//		IF_RIGIDBODYCANNOTADD(rigidbody)
//			return false ;
//		_addRigidBody(rigidbody);
//
//		if (IsExistPhysics())
//		{
//			checkAndAllocAggregateSize(mRigidBodyAggregate->getNbActors() + 1);
//			mRigidBodyAggregate->addActor(((RigidBodyComponent*)rigidbody.get())->GetPhysics());
//		}
//		return true;
//	}
//	uint32 RigidBodyGroupComponent::resetRigidBodys(const std::vector<Name>& comNames)
//	{
//		_delAllRigidBody();
//		
//		for (const Name& name : comNames)
//		{
//			ActorComponentPtr rigidbody = FindComponent(name);
//
//			IF_RIGIDBODYCANNOTADD(rigidbody) 
//				continue;
//			_addRigidBody(rigidbody);
//		}
//		if (IsExistPhysics())
//		{
//			_resetAllRigidBody();
//		}
//		return (uint32)mTeamMembers.size();
//	}
//	void RigidBodyGroupComponent::delRigidBody(const Name& comName)
//	{
//		auto rigidbody = FindComponent(comName);
//		_delRigidBody(rigidbody);
//	}
//	void RigidBodyGroupComponent::disableCollision(bool disable)
//	{
//		mDisableCollision = disable;
//		if (IsExistPhysics())
//			checkAndAllocAggregateSize(0);
//	}
//
//	bool RigidBodyGroupComponent::rigidBodyInGroup(ActorComponentPtr pAC)
//	{
//		return	RigidBodyMap.count(pAC.get()) != 0;
//	}
//
//	void RigidBodyGroupComponent::_addRigidBody(ActorComponentPtr rigidbody)
//	{
//		if (rigidbody.expired())
//			return;
//
//		((RigidBodyComponent*)rigidbody.get())->AddListener(getWeakPtr<RigidBodyGroupComponent>().getStaticCastWeakPtr<EchoRigidBodyCallBack>());
//
//		mTeamMembers.push_back(rigidbody);
//		RigidBodyMap.insert(rigidbody.get());
//	}
//
//	void RigidBodyGroupComponent::_delRigidBody(ActorComponentPtr rigidbody)
//	{
//		if (rigidbody.expired())
//			return;
//
//		((RigidBodyComponent*)rigidbody.get())->RemoveListener(getWeakPtr<RigidBodyGroupComponent>().getStaticCastWeakPtr<EchoRigidBodyCallBack>());
//		RigidBodyMap.erase(rigidbody.get());
//		mTeamMembers.erase( std::find(mTeamMembers.begin(), mTeamMembers.end(),rigidbody));
//	}
//
//	void RigidBodyGroupComponent::_delAllRigidBody()
//	{
//		for (auto& member : mTeamMembers)
//		{
//			RigidBodyMap.erase(member.get());
//			if (!member.expired()) {
//				((RigidBodyComponent*)member.get())->RemoveListener(getWeakPtr<RigidBodyGroupComponent>().getStaticCastWeakPtr<EchoRigidBodyCallBack>());
//			}
//		}
//		mTeamMembers.clear();
//	}
//
//	void RigidBodyGroupComponent::_resetAllRigidBody() {
//		if (!IsExistPhysics()) return;
//		std::vector<PhysicsActor*> actors;
//		for (auto& el : mTeamMembers)
//		{
//			RigidBodyComponent* pRigidBody = static_cast<RigidBodyComponent*> (el.get());
//			if (!pRigidBody || !pRigidBody->IsExistPhysics()) continue;
//			actors.push_back(pRigidBody->GetPhysics());
//		}
//		if (actors.size() < 2) {
//			return;
//		}
//		mRigidBodyAggregate->clearActor();
//		checkAndAllocAggregateSize((uint32)mTeamMembers.size());
//
//		for (PhysicsActor* pPA : actors)
//		{
//			mRigidBodyAggregate->addActor(pPA);
//		}
//	}
//
//	PhysXAggregate* RigidBodyGroupComponent::GetPhysics() {
//		return mRigidBodyAggregate;
//	}
//	
//	bool RigidBodyGroupComponent::checkAndAllocAggregateSize(uint32 quantity)
//	{
//		uint32 newSize = 0;
//		if (mRigidBodyAggregate == nullptr)
//		{
//			//newSize = std::max((uint32)2, (uint32)(quantity * 1.5));
//			newSize = std::max((uint32)2, (uint32)quantity);
//		}
//		else if (mRigidBodyAggregate->getMaxNbActors() < quantity)
//		{
//			//do { newSize =  std::max((uint32)2, (uint32)(quantity * 1.5));} while (newSize < quantity);
//			newSize = quantity;
//		}
//		else if (mRigidBodyAggregate->getSelfCollision() != !mDisableCollision)
//		{
//			newSize = (uint32)mRigidBodyAggregate->getMaxNbActors();
//		}
//		else
//			return true;
//		
//		// 删除旧的aggregate
//		std::vector<PhysicsActor*> selfActors; 
//		if (mRigidBodyAggregate != nullptr)
//		{
//			mRigidBodyAggregate->getActors(selfActors);
//			PhysXSystem::instance()->getScene()->RemoveAggregate(mRigidBodyAggregate);
//		}
//		// 新建新的aggregate
//		PhysXAggregate* newAggregate = PhysXSystem::instance()->getScene()->AddAggregate(newSize, !mDisableCollision);
//		if (newAggregate == nullptr)
//			return false;
//		// 复制数据
//		for (PhysicsActor* actor : selfActors)
//			newAggregate->addActor(actor);
//		mRigidBodyAggregate = newAggregate;
//
//		return true;
//	}
//
//	void RigidBodyGroupComponent::OnRigidBodyStateChanged(uint32 mask) {
//		if ((mask & EchoPhysics::PhysicsCreate) != 0) {
//			_resetAllRigidBody();
//		}
//	}
//
//}
