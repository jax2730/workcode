#pragma once
#include "EchoComponentPrerequisites.h"
#include "Actor/EchoActorComponent.h"
#include "EchoComDefineMacro.h"

namespace Echo
{
    enum class RenderSwitchLevel
    {
        RenderSwitchError = 0,
        InposterRender = 1,
        HlodRender = 2,
        NormalRender = 3,
        RenderSwitchCount
    };

    class DynamicLoadComponentInfo : public Echo::ActorComponentInfo
    {
    public:
        DynamicLoadComponentInfo(const Name& mtype, ActorCOMMoveableType movableType = UNMOVEABLE);
        virtual ~DynamicLoadComponentInfo() {}
        virtual void ExportData(Echo::cJSON* jsonNode) override;
        virtual void ImportData(Echo::cJSON* jsonNode) override;
        virtual void CopyTo(ActorComponentInfo* comInfo) override;
		virtual void OverWrite() override;

        //void SetExcel(int id);
    };

    class DynamicLoadComponent : public Echo::ActorComponent
    {
        DECLAREACTORCOMTYPE_H(DynamicLoadComponent)
    public:
        virtual void            init(Echo::ActorComponentInfo* info) override;
        virtual void            unInit() override;
        virtual void            replay(Echo::ActorComponentInfo* info) override {}
        void                    SetLevel(RenderSwitchLevel level);
        virtual void            connectCom() override;
        virtual void            disconnectCom() override;
        ActorComponentPtr       GetActorFactory();
        void                    visibleStateChanged(bool visibleState) override;


    private:
        void                    LoadHlodAccordingToActorFactoryData();
        void                    LoadInposterAccordingActorFactoryData();
        void                    ReleaseHlodAccordingToActorFactoryData();
        void                    ReleaseInposterAcorrdingTpActorFactoryData();

    private: //这些资产由ActorFactory缓存
        void                    LoadNormalAccordingActoryFactory();
        void                    RelaseNormalAccordingActorFactory();
        Echo::Vector3                        m_Min = Echo::Vector3::ZERO;
        Echo::Vector3                        m_Max = Echo::Vector3::ZERO;
        std::vector<Echo::ActorComponentPtr> m_RelatedComponents;

        RenderSwitchLevel m_CurrentLevel = RenderSwitchLevel::RenderSwitchError;

    public:
        void                    SetMinMax(const Echo::Vector3& min, const Echo::Vector3& max);
        void                    SetRenderVis(bool);
        DynamicLoadComponent*   GetParentDynamicComponent();
        void                    SetParentRender(bool);

        std::vector<float> CustomerData;


    };
    CLIENTDEFINECOMFACTORY(DynamicLoadComponent, Echo::ActorCOMMoveableType::UNMOVEABLE)
}