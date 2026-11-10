//-----------------------------------------------------------------------------
// File:   EchoUI3DComponentManager.cpp
//
// Author: yanghang
//
// Date:  2022-11-07
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#include "EchoComponentStableHeaders.h"
#include "3DUI/Echo3DUIComponentManager.h"
#include "Actor/EchoProtocolActorComponent.h"
#include "EchoRoot.h"
#include "EchoUIRenderManager.h"
#include "UI/GRoot.h"

using namespace echofgui;
namespace Echo
{
    void UI3DComponentManager::pushComponent(Echo3DUIComponent* com)
    {
        if(com)
        {
            bool isDuplicatePush = false;
            for(auto beginIte = m_all3DUIComponents.begin();
                beginIte != m_all3DUIComponents.end();
                beginIte++)
            {
                if(*beginIte == com)
                {
                    isDuplicatePush = true;
                    break;
                }
            }

            if(!isDuplicatePush)
            {
                m_all3DUIComponents.push_back(com);
            }
            else
            {
                assert(false && "The call the try to push duplicate 3D UI component, check the code!");
            }
        }
    }
    void UI3DComponentManager::popComponent(Echo3DUIComponent* com)
    {
        if(com)
        {
            for(auto beginIte = m_all3DUIComponents.begin();
                beginIte != m_all3DUIComponents.end();
                beginIte++)
            {
                if(*beginIte == com)
                {
                    m_all3DUIComponents.erase(beginIte);
                    break;
                }
            }

            for(auto beginIte = m_scene3DUIComponents.begin();
                beginIte != m_scene3DUIComponents.end();
                beginIte++)
            {
                if(*beginIte == com)
                {
                    m_scene3DUIComponents.erase(beginIte);
                    break;
                }
            }

            m_waitUpdateComponents.erase(com);
        }
    }

    //NOTE: call when 3D UI component change it's usage type on editting.
    void UI3DComponentManager::removeMarkAsSceneComponent(Echo3DUIComponent* com)
    {
        if(com)
        {
            for(auto beginIte = m_scene3DUIComponents.begin();
                beginIte != m_scene3DUIComponents.end();
                beginIte++)
            {
                if(*beginIte == com)
                {
                    m_scene3DUIComponents.erase(beginIte);
                    break;
                }
            }
        }
    }
    void UI3DComponentManager::need3DUIComponentUpdate(Echo3DUIComponent* com)
    {
        if (com)
        {
            m_waitUpdateComponents.emplace(com);
        }
    }
    void UI3DComponentManager::updateWait3DUIComponent()
    {
        for (Echo3DUIComponent* com : m_waitUpdateComponents)
            com->UpdateComRootNode(true);
        m_waitUpdateComponents.clear();
    }
    void UI3DComponentManager::markAsSceneComponent(Echo3DUIComponent* com)
    {
        if(com)
        {
            bool isDuplicateMark = false;
            for(auto beginIte = m_scene3DUIComponents.begin();
                beginIte != m_scene3DUIComponents.end();
                beginIte++)
            {
                if(*beginIte == com)
                {
                    isDuplicateMark = true;
                    break;
                }
            }

            if(!isDuplicateMark)
            {
                m_scene3DUIComponents.push_back(com);
            }
        }
    }

    void UI3DComponentManager::updateScene3DUIRenderPriority()
    {

        auto mainSceneManager = Root::instance()->getMainSceneManager();
        auto mainCamera = mainSceneManager->getMainCamera();
        DMatrix4 toCameraViewSpace = mainCamera->getViewMatrix();
        Vector3 cameraPos = mainCamera->getDerivedPosition();

        if(false)
        {
            //IMPORTANT(yanghang):Top level 3DUI frustum cull.
#if defined( _DEBUG)|| defined(DEBUG)
            //int32 culled_count = 0;
            //int32 visible_count = 0;
#endif
            for(auto& com : m_scene3DUIComponents)
            {
                GObject* fguiTopGObject = com->getGObject();
                if(fguiTopGObject)
                {
                    bool visible = true;
                    if(m_enableCull)
                    {
                        const AxisAlignedBox& aabb = com->getWorldAABB();
                        ////IMPORTANT:Camera frustum cull
                        if(mainCamera->isVisible(aabb))
                        {
                            //IMPORTANT:Distance cull.
                            Vector3 boxCenter = aabb.GetCenter();
                            float boundSphereRadiusSqr = aabb.GetHalf().squaredLength();

                            Vector3 cameraToAABB = cameraPos - boxCenter;
                            float cameraToAABBDistance = cameraToAABB.squaredLength();
                            visible &= (cameraToAABBDistance < (m_cullDistanceSquare + boundSphereRadiusSqr));
                        }
                        else
                        {
                            visible &= false;
                        }
                    }
                }
            }
        }

        struct UI3DComponentInfo
        {
            Echo3DUIComponent* com;
            float comViewSpaceZ;
        };

        struct UI3DComponentCameraSpaceSort
        {
            bool operator()(const UI3DComponentInfo& a, const UI3DComponentInfo& b) const
            {
                return(a.comViewSpaceZ < b.comViewSpaceZ);
            }
        };

        std::vector<UI3DComponentInfo> componentInfoArray;

        UI3DComponentInfo tempInfo;
        for(auto com : m_scene3DUIComponents)
        {
            Vector4 temp_v(com->getWorldPosition());
            temp_v = toCameraViewSpace * temp_v;
            tempInfo.comViewSpaceZ =  temp_v.z;
            tempInfo.com = com;
            componentInfoArray.push_back(tempInfo);
        }

        std::stable_sort(componentInfoArray.begin(),
                         componentInfoArray.end(),
                         UI3DComponentCameraSpaceSort());

        //IMPORTANT(yanghang):Use  UI3DComponent pointer as the [RenderPriorityLookupIndex].
        // the sorting order as [RenderPriorityOffset].
        //RenderPriorityOffsetLookupTable& globalLookupTable = UIRenderManager::instance()->getRenderPriorityOffsetLookupTable();
        //globalLookupTable.clear();
        //int priorityOffset = 0;
        //for(auto info : componentInfoArray)
        //{
        //    globalLookupTable[(RenderPriorityLookupIndex)info.com] = priorityOffset;
        //    priorityOffset++;
        //}
    }

    static ProtocolResult
    updateScene3DUIRenderPriority_ProtocolFuncion(void* result, ProtocolHandle handle)
    {
        UI3DComponentManager::instance()->updateScene3DUIRenderPriority();
        return PR_OK;
    }

    	static ProtocolResult
    update3DUIComponent_ProtocolFunction(void* result, ProtocolHandle handle)
    {
        UI3DComponentManager::instance()->updateWait3DUIComponent();
        return PR_OK;
    }

    UI3DComponentManager::UI3DComponentManager()
    {
        m_cullDistance = 5000.0f;
        m_cullDistanceSquare = m_cullDistance * m_cullDistance;
        m_enableCull = true;
#if defined( _DEBUG)|| defined(DEBUG)
        m_debugMode = false;
#endif

        auto protocolMgr = ProtocolComponentSystem::instance();
        protocolMgr->registerProtocolExtFunc(Name("updateScene3DUIRenderPriority_ProtocolFuncion"),
                                             updateScene3DUIRenderPriority_ProtocolFuncion);
    }

    UI3DComponentManager::~UI3DComponentManager()
    {
    }

} // end namespace echo
