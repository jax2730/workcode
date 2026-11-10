#include "EchoComponentStableHeaders.h"
#include "SUToEchoCom/SUDynamicFactorySwitcherComponent.h"
#include "SUToEchoCom/SUActorFactoryComponent.h"
#include "SUToEchoCom/SUDynamicLoadComponent.h"
#include "Actor/EchoActor.h"

namespace Echo
{
    DECLAREACTORCOMTYPE_CPP(DynamicFactorySwitcherComponent)
        DynamicFactorySwitcherComponentInfo::DynamicFactorySwitcherComponentInfo(const Name& mtype, ActorCOMMoveableType movableType) :
        Echo::ActorComponentInfo(mtype, movableType)
    {
    }
    void DynamicFactorySwitcherComponentInfo::ExportData(Echo::cJSON* jsonNode)
    {
        
    }
    void DynamicFactorySwitcherComponentInfo::ImportData(Echo::cJSON* jsonNode)
    {
        
    }

    void DynamicFactorySwitcherComponentInfo::CopyTo(ActorComponentInfo* comInfo)
    {
        
    }
	void DynamicFactorySwitcherComponentInfo::OverWrite()
	{

	}

    DynamicFactorySwitcherComponent::DynamicFactorySwitcherComponent(ActorPtr pActor, ActorCOMMoveableType movableType) :
        ActorComponent(pActor, movableType)
    {
    }

    DynamicFactorySwitcherComponent::~DynamicFactorySwitcherComponent()
    {
    }
    void DynamicFactorySwitcherComponent::init(Echo::ActorComponentInfo* info)
    {
    }
    void DynamicFactorySwitcherComponent::unInit()
    {
    }

    void DynamicFactorySwitcherComponent::connectCom()
    {
    }


    void DynamicFactorySwitcherComponent::disconnectCom()
    {

    }
    ActorComponentPtr DynamicFactorySwitcherComponent::GetActorFactory()
    {
        auto& act = getActor();
        if (act.expired())
            return nullptr;
        auto dynLoad = act->GetFirstComByType<DynamicLoadComponent>();
        if (dynLoad.expired()) return nullptr;
        return dynLoad->GetActorFactory();
    }
    void DynamicFactorySwitcherComponent::visibleStateChanged(bool b)
    {
        auto factory = GetActorFactory();
        if (factory.expired()) return;
        if (factory->getType() == ActorFactoryComponent::mType)
        {
            auto fT = static_cast<ActorFactoryComponent*>(factory.get());
            if (fT->GetCreateType() == eActorTypeBuilding)
            {
                fT->SetBuildingVisble(b);
            }
        }
    }
}
