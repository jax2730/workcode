#include "EchoComponentStableHeaders.h"
#include "SUToEchoCom/SUDynamicLoadComponent.h"
#include "SUToEchoCom/SUDynamicFactorySwitcherComponent.h"
#include "SUToEchoCom/SUActorFactoryComponent.h"
#include "Actor/EchoActor.h"
#include "EchoHLODComponent.h"
#include "EchoImposterComponent.h"

namespace Echo
{
    DECLAREACTORCOMTYPE_CPP(DynamicLoadComponent)
    DynamicLoadComponentInfo::DynamicLoadComponentInfo(const Name& mtype, ActorCOMMoveableType movableType) :
        Echo::ActorComponentInfo(mtype, movableType)
    {
    }
    void DynamicLoadComponentInfo::ExportData(Echo::cJSON* jsonNode)
    {
        
    }
    void DynamicLoadComponentInfo::ImportData(Echo::cJSON* jsonNode)
    {
        
    }

    void DynamicLoadComponentInfo::CopyTo(ActorComponentInfo* comInfo)
    {
        
    }
	void DynamicLoadComponentInfo::OverWrite()
	{

	}

    DynamicLoadComponent::DynamicLoadComponent(ActorPtr pActor, ActorCOMMoveableType movableType) :
        ActorComponent(pActor, movableType)
    {
    }

    DynamicLoadComponent::~DynamicLoadComponent()
    {

    }
    void DynamicLoadComponent::init(Echo::ActorComponentInfo* info)
    {

    }
    void DynamicLoadComponent::unInit()
    {

    }

    void DynamicLoadComponent::connectCom()
    {
        switch (m_CurrentLevel)
        {
        case Echo::RenderSwitchLevel::RenderSwitchError:
            break;
        case Echo::RenderSwitchLevel::InposterRender:
            LoadInposterAccordingActorFactoryData();
            break;
        case Echo::RenderSwitchLevel::HlodRender:
            LoadHlodAccordingToActorFactoryData();
            break;
        case Echo::RenderSwitchLevel::NormalRender:
            LoadNormalAccordingActoryFactory();
            break;
        default:
            break;
        }
    }

    void DynamicLoadComponent::disconnectCom()
    {
        switch (m_CurrentLevel)
        {
        case Echo::RenderSwitchLevel::RenderSwitchError:
            break;
        case Echo::RenderSwitchLevel::InposterRender:
            ReleaseInposterAcorrdingTpActorFactoryData();
            break;
        case Echo::RenderSwitchLevel::HlodRender:
            ReleaseHlodAccordingToActorFactoryData();
            break;
        case Echo::RenderSwitchLevel::NormalRender:
            RelaseNormalAccordingActorFactory();
            break;
        default:
            break;
        }
    }

    void DynamicLoadComponent::LoadHlodAccordingToActorFactoryData()
    {
        auto Hlod = getActor()->AddComponent<Echo::HLODComponent>("HLODComponent", getActor()->getRootNodeCom());
        auto m_ActorFac = GetActorFactory();
        if (m_ActorFac.expired() || Hlod.expired())
        {
            return;
        }
        auto FacPtr = static_cast<ActorFactoryComponent*>(m_ActorFac.get());
        auto blockID = FacPtr->GetExcelID();
        auto building = GetExcelLineData(building_list, blockID);
        if (!building) return;
        std::string hlodpath = "";
        if (std::string() == "")
        {
            hlodpath = "extra_render_resource/hlod/building/" + std::string("building_") + std::to_string(blockID) + ".mesh";
        }
        else
        {
            hlodpath = building->hlod_path().c_str();
        }
        Hlod->loadEntity(Echo::Name(hlodpath));
        if (CustomerData.size() >= 3)
        {
            if (CustomerData[0] != 0)
                Hlod->setHLODFactor(CustomerData[0]);
            if (CustomerData[1] != 0)
                Hlod->setDissolveFactor(CustomerData[1]);
            if (CustomerData[2] != 0)
                Hlod->setCullFactor(CustomerData[2]);
        }
        else
        {
            Hlod->enableHLOD(false);
        }
        m_RelatedComponents.emplace_back(Hlod->weak_from_this());
    }
    void DynamicLoadComponent::LoadInposterAccordingActorFactoryData()
    {
        auto m_ActorFac = GetActorFactory();
        if (m_ActorFac.expired()) return;
        auto FacPtr = static_cast<ActorFactoryComponent*>(m_ActorFac.get());
        auto blcokID = FacPtr->GetExcelID();
        auto building = GetExcelLineData(building_list, blcokID);
        if (building)
        {
            std::string imposter_path = "";
            if (std::string(building->imposter_path().c_str()) == "")
            {
                imposter_path = std::string("extra_render_resource/imposter/building/") + "building_" + std::to_string(blcokID) + ".dds";
            }
            else
            {
                imposter_path = building->imposter_path().c_str();
            }
            auto imposter = getActor()->AddComponent<ImposterComponent>("ImpostorComponent", getActor()->getRootNodeCom());
            if (!imposter.expired())
            {
                imposter->createImposter(Echo::Name(imposter_path));
                if (CustomerData.size() > 0)
                {
                    if (CustomerData[0] != 0)
                    {
                        auto nn = (m_Max - m_Min).length() * CustomerData[0];
                        imposter->SetEnableDistance(nn);
                        imposter->enableImposter(true); //开启关闭在这里
                        imposter->SetImposterScale(m_Min * 2, m_Max * 2);
                        auto center = m_Min + (m_Max - m_Min) / 2;
                        imposter->setPosition(center);
                        imposter->SetEnableAABBData(m_Min, m_Max);
                        imposter->SetImposterVisible(false);
                    }
                    else
                    {
                        imposter->enableImposter(false);
                    }
                }
                else
                {
                    imposter->enableImposter(false);
                }
                //imposter->set
                m_RelatedComponents.emplace_back(imposter->weak_from_this());
            }
        }
        //SetParentRender(false);
        //SetRenderVis(true);
    }
    void DynamicLoadComponent::LoadNormalAccordingActoryFactory()
    {
        auto m_ActorFac = GetActorFactory();
        if (m_ActorFac.expired())
        {
            return;
        }
        static_cast<ActorFactoryComponent*>(m_ActorFac.get())->CreateBaseBuilding(); //创建盒子
        static_cast<ActorFactoryComponent*>(m_ActorFac.get())->SetBuildingVisble(getRealVisibleState());

        auto comptr = this->getActor()->AddComponent<DynamicFactorySwitcherComponent>("DynamicFactorySwitcherComponent", this->getActor()->getRootNodeCom());
        if (!comptr.expired())
        {
            m_RelatedComponents.emplace_back(comptr->weak_from_this());
        }
        //SetParentRender(false);
        //SetRenderVis(true);
    }

    void DynamicLoadComponent::ReleaseHlodAccordingToActorFactoryData()
    {
        for (auto& p : m_RelatedComponents)
        {
            p->getActor()->delComponent(p);
        }
        m_RelatedComponents.clear();
    }
    void DynamicLoadComponent::ReleaseInposterAcorrdingTpActorFactoryData()
    {
        for (auto& p : m_RelatedComponents)
        {
            p->getActor()->delComponent(p);
        }
        m_RelatedComponents.clear();
    }
    void DynamicLoadComponent::RelaseNormalAccordingActorFactory()
    {
        auto m_ActorFac = GetActorFactory();
        if (m_ActorFac.expired())
        {
            return;
        }
        static_cast<ActorFactoryComponent*>(m_ActorFac.get())->ClearResource();

        for (auto& p : m_RelatedComponents)
        {
            p->getActor()->delComponent(p);
        }
        m_RelatedComponents.clear();
    }

    void DynamicLoadComponent::SetLevel(RenderSwitchLevel level)
    {
        m_CurrentLevel = level;
    }
    ActorComponentPtr DynamicLoadComponent::GetActorFactory()
    {
        int cLevel = (int)m_CurrentLevel;
        Echo::ActorPtr currentNode = getActor();
        while (cLevel > 0)
        {
            currentNode = currentNode->getParentActor();
            if (currentNode.expired()) return nullptr;
            cLevel--;
        }
        return currentNode->GetFirstComByType<ActorFactoryComponent>();
    }

    void DynamicLoadComponent::visibleStateChanged(bool visibleState)
    {
        if (m_CurrentLevel == Echo::RenderSwitchLevel::NormalRender)
        {
            auto m_ActorFac = GetActorFactory();
            if (m_ActorFac.expired())
            {
                return;
            }
            static_cast<ActorFactoryComponent*>(m_ActorFac.get())->SetBuildingVisble(getRealVisibleState());
        }
    }

    void DynamicLoadComponent::SetRenderVis(bool b)
    {
        for (auto& p : m_RelatedComponents)
        {
            if (p.expired()) continue;
            p->setVisible(b);
        }
    }
    void DynamicLoadComponent::SetMinMax(const Echo::Vector3& min, const Echo::Vector3& max)
    {
        m_Min = min;
        m_Max = max;
    }

    DynamicLoadComponent* DynamicLoadComponent::GetParentDynamicComponent()
    {
        auto act = this->getActor()->getParentActor();
        if (act.expired()) return nullptr;
        auto dync = act->GetFirstComByType<DynamicLoadComponent>();
        if (dync.expired()) return nullptr;
        return dync.get();
    }

    void DynamicLoadComponent::SetParentRender(bool b)
    {
        int                   times = (int)m_CurrentLevel - 2;
        DynamicLoadComponent* compoent = this;
        for (int i = 0; i < times; ++i)
        {
            if (compoent)
            {
                compoent = compoent->GetParentDynamicComponent();
                if (compoent) compoent->SetRenderVis(b);
            }
        }
    }
}
