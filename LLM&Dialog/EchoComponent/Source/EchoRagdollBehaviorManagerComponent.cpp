#include "EchoComponentStableHeaders.h"
#include "EchoRagdollBehaviorManagerComponent.h"
#include "EchoRagdollBehaviorComponentBase.h"
#include "EchoRagdollComponent.h"
namespace Echo
{

	RagdollBehaviorManagerComponentInfo::RagdollBehaviorManagerComponentInfo()
		:ActorComponentInfo()
	{
	}
	RagdollBehaviorManagerComponentInfo::RagdollBehaviorManagerComponentInfo(const Name& mtype, ActorCOMMoveableType movableType)
		:ActorComponentInfo(mtype, movableType)
	{
	}
	RagdollBehaviorManagerComponentInfo::~RagdollBehaviorManagerComponentInfo()
	{
	}
	void RagdollBehaviorManagerComponentInfo::ExportData(cJSON* jsonNode)
	{
		reflect::TypeResolver< RagdollBehaviorManagerComponentInfo>::get()->ExportJson(this,jsonNode,"");
	}
	void RagdollBehaviorManagerComponentInfo::ImportData(cJSON* jsonNode)
	{
		reflect::TypeResolver< RagdollBehaviorManagerComponentInfo>::get()->ImportJson(this, jsonNode, "");
	}
	void RagdollBehaviorManagerComponentInfo::CopyTo(ActorComponentInfo* comInfo)
	{
		reflect::TypeResolver< RagdollBehaviorManagerComponentInfo>::get()->copyTo(this, comInfo);
	}
	void RagdollBehaviorManagerComponentInfo::OverWrite()
	{
		if (mOriginComInfo != nullptr)
		{
			reflect::TypeResolver<RagdollBehaviorManagerComponentInfo>::get()->overWrite(mOriginComInfo, this);
		}
	}

	DECLAREACTORCOMTYPE_CPP(RagdollBehaviorManagerComponent)

	RagdollBehaviorManagerComponent::RagdollBehaviorManagerComponent(ActorPtr  pActor, ActorCOMMoveableType movableType)
		:NodeActorComponent(pActor, movableType)
	{
	}
	RagdollBehaviorManagerComponent::~RagdollBehaviorManagerComponent()
	{}

	void RagdollBehaviorManagerComponentInfo::SetDefaultBehaviorName(const Name& bhvr)
	{
		defaultBehaviorName = bhvr;
	}
	const Name& RagdollBehaviorManagerComponentInfo::GetDefaultBehaviorName()
	{
		return defaultBehaviorName;
	}
	void RagdollBehaviorManagerComponent::GetAllBehavior(std::vector<RagdollBehaviorComponentBase*>& dst)
	{
		const EchoTMap<String, ActorComponentPtr>& allChild = getChildActorComponents();
		
		dst.clear();

		for (auto& acp : allChild)
		{
			if (acp.second.expired())
				continue;
			RagdollBehaviorComponentBase* item = dynamic_cast<RagdollBehaviorComponentBase*>(acp.second.get());
			if (item != nullptr)
				dst.push_back(item);
		}
	}
	void RagdollBehaviorManagerComponent::SetDefaultBehavior(RagdollBehaviorComponentBase* bhvr)
	{
		std::vector<RagdollBehaviorComponentBase*>  dst;
		GetAllBehavior(dst);

		for (auto& acp : dst)
		{
			if (acp == bhvr)
			{
				defaultBehavior = acp->weak_from_this();
				break;
			}
		}
	}
	RagdollBehaviorComponentBase* RagdollBehaviorManagerComponent::GetDefaultBehavior()
	{
		std::vector<RagdollBehaviorComponentBase*>  dst;
		GetAllBehavior(dst);

		RagdollBehaviorComponentBase* fistBehavior = nullptr;
		for (auto& acp : dst)
		{
			if (fistBehavior == nullptr)
				fistBehavior = acp;
			if (acp == defaultBehavior)
				return acp;
		}
		// not found 
		defaultBehavior = fistBehavior == nullptr ? (ActorComponentPtr)nullptr : fistBehavior->weak_from_this();
		return fistBehavior;

	}
	void RagdollBehaviorManagerComponent::BehavioursForeach(RagdollComponent* master,const std::function<void(RagdollBehaviorComponentBase*)>& impl)
	{
		std::vector<RagdollBehaviorComponentBase*>  dst;
		GetAllBehavior(dst);

		for (auto& acp : dst)
		{
			acp->ragdollCom = master->weak_from_this();
			impl(acp);
		}
	}
}
