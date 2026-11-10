//-----------------------------------------------------------------------------
// File:   EchoUI3DComponentManager.h
//
// Author: yanghang
//
// Date:  2022-11-07
//
// Copyright (c) PixelSoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#if !defined(ECHO3DUICOMPONENTMANAGER_H)

#include "Echo3DUIComponent.h"

namespace echofgui {
    class GRoot;
}

namespace Echo
{
    struct HitTest3DUIComponentSystemInfo
    {
        Echo::Vector2 screenPos;
        echofgui::GRoot* root;
    };
    
    //IMPORTANT: Manage scene 3D UI ray-cast order sort and rendering order sort on the whole interface unit.
    // Need to hold all reference of scene 3D UI component.
    class _EchoComponentExport UI3DComponentManager : public Singleton<UI3DComponentManager>
    {
    public:
        UI3DComponentManager();
        ~UI3DComponentManager();
        
        //NOTE: call when 3D UI component create or destroy.
        void pushComponent(Echo3DUIComponent* com);
        void popComponent(Echo3DUIComponent* com);

        //NOTE: call when 3D UI component after initialize it's data and change it's usage type on editting.
        void markAsSceneComponent(Echo3DUIComponent* com);
        void removeMarkAsSceneComponent(Echo3DUIComponent* com);

        void need3DUIComponentUpdate(Echo3DUIComponent* com);
        void updateWait3DUIComponent();

        //IMPORTANT(yanghang):tick this each frame that before push render to RenderQueue. 
        void updateScene3DUIRenderPriority();

        private:
        std::vector<Echo3DUIComponent*> m_all3DUIComponents;
        std::vector<Echo3DUIComponent*> m_scene3DUIComponents;
        std::set<Echo3DUIComponent*>    m_waitUpdateComponents;
        
        //NOTE: default cull distance is 5 Kilometers.
        float m_cullDistance;
        float m_cullDistanceSquare;
        
        bool m_enableCull;
#if defined( _DEBUG)|| defined(DEBUG)
        bool m_debugMode;
#endif
    };

}
#define ECHO3DUICOMPONENTMANAGER_H
#endif
