//
//  GDIView.hpp
//  TestMetal
//
//  Created by lixin on 16/1/19.
//  Copyright © 2016年 pixelsoft. All rights reserved.
//

#ifndef __DEMO_VIEW_HPP__
#define __DEMO_VIEW_HPP__

#import "UIProtocol.h"
#include "EchoProtocolComponent.h"
#include "appview.h"


struct DemoView
{
    id<UIProtocol>          m_bridge;
    EAGLContext *context;
    
    
    std::string                  m_assetPath;
    
    int                     m_width;
    int                     m_height;
    
    bool                    m_joystick_mov;
    bool                    m_joystick_dir;
    
    bool Init( const Echo::ECHO_GRAPHIC_PARAM* pGraphicParam, const char * _szPath);
    void Resize( float _width, float _height);
    bool OnRender( unsigned long _tick );
    int  GetFPS();
    int  GetDrawCall();
    int  GetTriangleCnt();
    std::string GetCameraPosition();
    std::string GetResMemUsage();
    void TouchProc(int _type,int _touchId, float _x, float _y );

    //NOTE(yanghang):Dispatch origin touch input.
    void touchDispatch(int _type, int _touchId, float _x, float _y);
    
    void TextProc( const char * Text, int len );
    void Release();
    
    DemoView();
    ~DemoView();
};

extern DemoView * GetAppView();

#endif /* GDIView_hpp */
