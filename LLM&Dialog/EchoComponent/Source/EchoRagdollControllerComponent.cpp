#include "EchoComponentStableHeaders.h"
#include "EchoRagdollControllerComponent.h"

namespace Echo
{
	DECLAREACTORCOMTYPE_CPP(RagdollControllerComponent)

	RagdollControllerComponentInfo::RagdollControllerComponentInfo(const Name & mtype, ActorCOMMoveableType movableType)
		: ActorComponentInfo(mtype, movableType)
	{
	}
	RagdollControllerComponentInfo::RagdollControllerComponentInfo()
	{}
	RagdollControllerComponentInfo::~RagdollControllerComponentInfo()
	{
	}
	EDITOR_REFLECT_BEGIN(RagdollControllerComponentInfo)
		EDITOR_REFLECT_MEMBER_UPDATE(mRoleName, "mRoleName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mRagdollName, "mRagdollName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
		EDITOR_REFLECT_MEMBER_UPDATE(mAnimationName,"mAnimationName", EditorUI_DRAG, nullptr, reflect::Info_SHOW_ALL)
	EDITOR_REFLECT_END()
	void RagdollControllerComponentInfo::ExportData(cJSON * jsonNode)
	{
		reflect::TypeResolver<RagdollControllerComponentInfo>::get()->ExportJson(this, jsonNode, "");
	}

	void RagdollControllerComponentInfo::ImportData(cJSON * jsonNode)
	{
		reflect::TypeResolver<RagdollControllerComponentInfo>::get()->ImportJson(this, jsonNode, "");
	}

	void RagdollControllerComponentInfo::CopyTo(ActorComponentInfo * comInfo)
	{
		RagdollControllerComponentInfo* info = static_cast<RagdollControllerComponentInfo*>(comInfo);
		reflect::TypeResolver<RagdollControllerComponentInfo>::get()->copyTo(this,info);
	}
	void RagdollControllerComponentInfo::OverWrite()
	{
		if (mOriginComInfo != nullptr)
		{
			RagdollControllerComponentInfo* oriInfo = static_cast<RagdollControllerComponentInfo*> (mOriginComInfo);
			reflect::TypeResolver<RagdollControllerComponentInfo>::get()->overWrite(oriInfo, this);
		}
	}
	RagdollControllerComponent::RagdollControllerComponent(ActorPtr pActor, ActorCOMMoveableType movableType = ALL_MOVEABLE)
		:ProtocolNodeActorComponent(pActor, movableType)
	{
		registerTickTimeBegin();
	}
	RagdollControllerComponent::~RagdollControllerComponent()
	{
	}
	void RagdollControllerComponent::init(ActorComponentInfo* info)
	{
		RagdollControllerComponentInfo* minfo = static_cast<RagdollControllerComponentInfo*>(info);
		if (!minfo)
			return;
		mRoleName = minfo->mRoleName;
		mRagdollName = minfo->mRagdollName;
		mAnimationName = minfo->mAnimationName;
	}
	void RagdollControllerComponent::bindNode()
	{
	}
	void RagdollControllerComponent::replay(ActorComponentInfo * info)
	{
		RagdollControllerComponentInfo* minfo = static_cast<RagdollControllerComponentInfo*>(info);
		if (!minfo)
			return;
		mRoleName = minfo->mRoleName;
		mRagdollName = minfo->mRagdollName;
		mAnimationName = minfo->mAnimationName;
	}
	void RagdollControllerComponent::connectCom()
	{
		GetRoleList();
		GetRagdollList();
	}
	void RagdollControllerComponent::setActor(const ActorPtr& pActor)
	{
		if (pActor == m_pActor)
			return;
		m_pActor = pActor;
		if (m_pActor == nullptr)
			return;
	}
	void RagdollControllerComponent::copyTo(const ActorComponentPtr& actCom)
	{
	}
	void RagdollControllerComponent::dealComponentDelete(const ActorComponentPtr& actCom)
	{
	}
	void RagdollControllerComponent::registerTickTimeBegin()
	{
		m_eventSet[TT_BeginFrame].insert(EID_EveryFrame);
		registerTickTimeEnd();
	}
	void RagdollControllerComponent::beginFrame(EventID mEventID, ProtocolHandle handle)
	{
		if (!BeginNext())
			return;
		switch (mEventID)
		{
		case EID_EveryFrame:
		{
			if (mIsDeath && !mRoleName.empty() && !mRagdollName.empty())
			{
				//if (GetRagdollCom()->GetGlobalPinWeight() > 0.f)
				//{
				//	GetRagdollCom()->SetGlobalPinWeight(std::max(0.f, GetRagdollCom()->GetGlobalPinWeight() - EchoIK::Time::deltaTime * GetRagdollCom()->fadeOutPinWeightSpeed));
				//}
				//if (GetRagdollCom()->GetGlobalMuscleWeight() > 0.f)
				//{
				//	GetRagdollCom()->SetGlobalMuscleWeight(std::max(GetRagdollCom()->deadMuscleWeight, GetRagdollCom()->GetGlobalMuscleWeight() - EchoIK::Time::deltaTime * GetRagdollCom()->fadeOutMuscleWeightSpeed));
				//}
			}
		}
		default:
			break;
		}
	}
	void RagdollControllerComponent::GetRoleList()
	{
		EchoTMap<String, ActorComponentPtr> ComList = m_pActor->getComponents();
		mRoleList.clear();
		for (auto it : ComList)
		{
			if (it.second->getType() == COMPONENTTYPE(RoleComponent))
				mRoleList.push_back(it.second->getComName());
		}
		mRoleList.shrink_to_fit();
	}
	void RagdollControllerComponent::GetRagdollList()
	{
		EchoTMap<String, ActorComponentPtr> ComList = m_pActor->getComponents();
		mRagdollList.clear();
		for (auto it : ComList)
		{
			if (it.second->getType() == COMPONENTTYPE(RagdollComponent))
				mRagdollList.push_back(it.second->getComName());
		}
		mRagdollList.shrink_to_fit();
	}
	RoleComponent * RagdollControllerComponent::GetRoleCom()
	{
		return static_cast<RoleComponent*>(getActor()->getComponent(mRoleName).get());
	}
	RagdollComponent * RagdollControllerComponent::GetRagdollCom()
	{
		RagdollComponent* ragdollCom = static_cast<RagdollComponent*>(getActor()->getComponent(mRagdollName).get());
		if (ragdollCom == NULL)
			return NULL;
		if (GetRoleCom())
		{
			if (GetRoleCom()->getComName() != static_cast<RagdollComponentInfo*>(ragdollCom->m_pActComInfo)->roleName)
				return NULL;
		}
		return ragdollCom;
	}
}
