#include "EchoComponentStableHeaders.h"
#include "EchoAggregateComponent.h"
#include "EchoRigidBodyComponent.h"
#include "Actor/EchoActorManager.h"
#include "EchoPhysXAggregate.h"
#include "EchocJsonHelper.h"
#include "EchoPhysXManager.h"

namespace Echo {

	AggregateComponent::AggregateComponent(ActorPtr actor, ActorCOMMoveableType type) : PhysicsLinkComponent(actor, type) {}

	AggregateComponent::~AggregateComponent(){}

	void AggregateComponent::init(ActorComponentInfo* pInfo) {
		PhysicsBaseComponent::init(pInfo);
		if (pInfo) {
			AggregateComponentData tempData;
			static_cast<AggregateComponentInfo*>(pInfo)->CopyToCompData(&tempData);
			SetCompData(&tempData);
		}
	}
	void AggregateComponent::unInit() {
		ReleasePhysics();
		mPhysicsCompPtrs.clear();
		PhysicsBaseComponent::unInit();
	}
	void AggregateComponent::connectCom() {
		PhysicsBaseComponent::connectCom();
		DetachPhysXActorComp();
		AttachPhysXActorComp();
	}
	void AggregateComponent::disconnectCom() {
		DetachPhysXActorComp();
		PhysicsBaseComponent::disconnectCom();
	}
	void AggregateComponent::replay(ActorComponentInfo* pInfo) {
		if (!pInfo) return;
		AggregateComponentData tempData;
		static_cast<AggregateComponentInfo*>(pInfo)->CopyToCompData(&tempData);
		SetCompData(&tempData);
	}
	void AggregateComponent::copyTo(const ActorComponentPtr& newComp) {
		if (newComp == nullptr) return;
		AggregateComponent* pComp = (AggregateComponent*)newComp.getPointer();
		pComp->SetCompData(&mCompData);
	}
	void AggregateComponent::connectActor() {
		PhysicsBaseComponent::connectActor();
		DetachPhysXActorComp();
		AttachPhysXActorComp();
	}
	void AggregateComponent::disconnectActor() {
		PhysicsBaseComponent::disconnectActor();
		DetachPhysXActorComp();
	}

	void AggregateComponent::CreatePhysics() {
		if (m_pAggregate != nullptr) return;
		int size = (int)mPhysicsCompPtrs.size();
		m_pAggregate = GetPhysicsManager()->GetScene()->AddAggregate(size, mCompData.IsCollision);
		if (m_pAggregate == nullptr) return;
		std::set<PhysicsActor*> actors;
		GetSelfPhysXActors(actors);
		for (PhysicsActor* pActor : actors) {
			m_pAggregate->addActor(pActor);
		}
	}
	void AggregateComponent::ReleasePhysics() {
		if (m_pAggregate == nullptr) return;
		GetPhysicsManager()->RemovePhysXAggregate(m_pAggregate);
		m_pAggregate = nullptr;
	}

	bool AggregateComponent::GetSelfPhysXActors(std::set<PhysicsActor*>& actors) {
		actors.clear();
		int size = (int)mPhysicsCompPtrs.size();
		if (size == 0) return false;
		for (int i = 0; i != size; ++i) {
			if (mPhysicsCompPtrs[i] != nullptr && mPhysicsCompPtrs[i]->GetPhysicsManager() == GetPhysicsManager()) {
				PhysicsActor* pActor = mPhysicsCompPtrs[i]->_getActorPhysX();
				if (pActor) {
					actors.insert(pActor);
				}
			}
		}
		return true;
	}

	bool AggregateComponent::IsExistPhysics() {
		return m_pAggregate != nullptr;
	}
	bool AggregateComponent::IsCanPhysics() {
		std::set<PhysicsActor*> actors;
		GetSelfPhysXActors(actors);
		return actors.size() > 1;
	}
	PhysXAggregate* AggregateComponent::GetPhysics() {
		return m_pAggregate;
	}
	void AggregateComponent::_actorNotifyPhysX(PhysicsActorComponent* pComp, EchoPhysXNotifyType type) {
		if (pComp == nullptr || pComp->GetPhysicsManager() != GetPhysicsManager()) return;
		if (type == PhysXNotify_Create || type == PhysXNotify_Destroy) {
			ReleasePhysics();
			AddToPhysicsUpdateQueue();
		}
		else if (type == PhysXNotify_Break) {
			ReleasePhysics();
			if (!GetUsePointer()) {
				DetachPhysXActorComp();
				AttachPhysXActorComp();
			}
			AddToPhysicsUpdateQueue();
		}
	}

	void AggregateComponent::SetCompData(AggregateComponentData* pData) {
		if (pData == nullptr) return;
		SetIsCollision(pData->IsCollision);
		SetPhysicsCompNames(pData->PhysicsCompNames);
	}
	AggregateComponentData* AggregateComponent::GetCompData() {
		return &mCompData;
	}
	void AggregateComponent::SetUsePointer(bool value) { bUsePointer = value; }

	void AggregateComponent::SetPhysicsCompPtrs(const std::vector<PhysicsActorComponentPtr>& value, bool bForce) {
		if (!GetUsePointer()) return;
		ReleasePhysics();
		DetachPhysXActorComp();
		mPhysicsCompPtrs.clear();
		mPhysicsCompPtrs = value;
		AttachPhysXActorComp(bForce);
		AddToPhysicsUpdateQueue();
	}
	const std::vector<PhysicsActorComponentPtr>& AggregateComponent::GetPhysicsCompPtrs() {
		return mPhysicsCompPtrs;
	}

	void AggregateComponent::AttachPhysXActorComp(bool bForce) {
		PhysicsLinkComponentPtr thisPtr = getWeakPtr<PhysicsLinkComponent>();
		auto AddPhysXFun = [&](int curIndex) {
			AggregateComponentPtr aggPtr = mPhysicsCompPtrs[curIndex]->GetAggregate();
			if (aggPtr == nullptr || aggPtr == this) {
				mPhysicsCompPtrs[curIndex]->_addPhysX(thisPtr);
			}
			else {
				mPhysicsCompPtrs[curIndex] = nullptr;
			}
		};
		if (!GetUsePointer()) {
			for (int i = 0; i != mCompData.PhysicsCompNames.size(); ++i) {
				if (!mCompData.PhysicsCompNames[i].empty()) {
					ActorComponentPtr compPtr = FindComponent(mCompData.PhysicsCompNames[i]);
					if (compPtr != nullptr) {
						mPhysicsCompPtrs[i] = compPtr.getStaticCastWeakPtr<PhysicsActorComponent>();
					}
					else {
						mPhysicsCompPtrs[i] = nullptr;
					}
				}
			}
		}
		if (bForce) {
			std::set<AggregateComponentPtr> oldAggPtrs;
			for (int i = 0; i != mPhysicsCompPtrs.size(); ++i) {
				if (mPhysicsCompPtrs[i] != nullptr) {
					AggregateComponentPtr aggPtr = mPhysicsCompPtrs[i]->GetAggregate();
					if (aggPtr) {
						oldAggPtrs.insert(aggPtr);
					}
					aggPtr = mPhysicsCompPtrs[i]->PhysicsActorComponent::GetAggregate();
					if (aggPtr) {
						oldAggPtrs.insert(aggPtr);
					}
				}
			}
			for (auto& aggPtr : oldAggPtrs) {
				aggPtr->SetPhysicsCompPtrs({});
			}
		}
		for (int i = 0; i != mPhysicsCompPtrs.size(); ++i) {
			if (mPhysicsCompPtrs[i] != nullptr) {
				AddPhysXFun(i);
			}
		}
	}
	void AggregateComponent::DetachPhysXActorComp() {
		PhysicsLinkComponentPtr thisPtr = getWeakPtr<PhysicsLinkComponent>();
		auto RemovePhysXFun = [&](int curIndex) {
			if (mPhysicsCompPtrs[curIndex] != nullptr) {
				mPhysicsCompPtrs[curIndex]->_removePhysX(thisPtr);
			}
			mPhysicsCompPtrs[curIndex] = nullptr;
		};
		if (!GetUsePointer()) {
			PhysicsLinkComponentPtr thisPtr = getWeakPtr<PhysicsLinkComponent>();
			for (int i = 0; i != mPhysicsCompPtrs.size(); ++i) {
				RemovePhysXFun(i);
			}
		}
		else {
			PhysicsLinkComponentPtr thisPtr = getWeakPtr<PhysicsLinkComponent>();
			for (int i = 0; i != mPhysicsCompPtrs.size(); ++i) {
				if (mPhysicsCompPtrs[i] != nullptr) {
					mPhysicsCompPtrs[i]->_removePhysX(thisPtr);
				}
			}
		}
	}

	void AggregateComponent::SetIsCollision(bool value) {
		if (mCompData.IsCollision == value) return;
		mCompData.IsCollision = value;
		ReleasePhysics();
		AddToPhysicsUpdateQueue();
	}
	bool AggregateComponent::GetIsCollision() {
		return mCompData.IsCollision;
	}

	void AggregateComponent::SetPhysicsCompNames(const std::vector<String>& value) {
		if (GetUsePointer()) return;
		DetachPhysXActorComp();
		mCompData.PhysicsCompNames = value;
		mPhysicsCompPtrs.clear();
		mPhysicsCompPtrs.resize(value.size());
		AttachPhysXActorComp();
		ReleasePhysics();
		AddToPhysicsUpdateQueue();
	}
	const std::vector<String>& AggregateComponent::GetPhysicsCompNames() {
		return mCompData.PhysicsCompNames;
	}

}

namespace Echo {

	DECLAREACTORCOMTYPE_CPP(AggregateComponent)

	EDITOR_REFLECT_BEGIN(AggregateComponentInfo)
	EDITOR_REFLECT_MEMBER_UPDATE(m_IsCollision, "IsCollision", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_MEMBER_UPDATE(m_PhysicsCompNames, "PhysicsCompNames", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_END();

	_DEFINEINFO_VIRFUN_BASE(AggregateComponentInfo)

	AggregateComponentInfo::AggregateComponentInfo(const Name& comType, ActorCOMMoveableType MoveType) : ActorComponentInfo(comType, MoveType) {}
	AggregateComponentInfo::~AggregateComponentInfo() {}

	void AggregateComponentInfo::CopyToCompData(AggregateComponentData* pData) {
		if (!pData) return;
		pData->IsCollision = m_IsCollision;
		pData->PhysicsCompNames = m_PhysicsCompNames;
	}

}
