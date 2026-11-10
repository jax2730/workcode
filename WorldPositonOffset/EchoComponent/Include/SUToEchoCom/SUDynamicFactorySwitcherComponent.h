#pragma once
#include "EchoComponentPrerequisites.h"
#include "EchoComDefineMacro.h"
#include "Actor/EchoActorComponent.h"

namespace Echo
{
    class DynamicFactorySwitcherComponentInfo : public Echo::ActorComponentInfo
    {
    public:
        DynamicFactorySwitcherComponentInfo(const Name& mtype, ActorCOMMoveableType movableType = UNMOVEABLE);
        virtual ~DynamicFactorySwitcherComponentInfo() {}
        virtual void ExportData(Echo::cJSON* jsonNode) override;
        virtual void ImportData(Echo::cJSON* jsonNode) override;
        virtual void CopyTo(ActorComponentInfo* comInfo) override;
		virtual void OverWrite() override;

        //void SetExcel(int id);
    };

    class DynamicFactorySwitcherComponent : public Echo::ActorComponent
    {
        DECLAREACTORCOMTYPE_H(DynamicFactorySwitcherComponent)
    public:
        virtual void      init(Echo::ActorComponentInfo* info) override;
        virtual void      unInit() override;
        virtual void      replay(Echo::ActorComponentInfo* info) override {}
        virtual void      connectCom() override;
        virtual void      disconnectCom() override;
        virtual void      visibleStateChanged(bool) override;
        ActorComponentPtr GetActorFactory();


    public:

    };
    CLIENTDEFINECOMFACTORY(DynamicFactorySwitcherComponent, Echo::ActorCOMMoveableType::UNMOVEABLE)
}