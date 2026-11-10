//
//  AppDelegate.h
//  GLESxMetal
//
//  Created by mengxin on 15/5/14.
//  Copyright (c) 2015å¹´ pixelsoft. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <GLKit/GLKit.h>
#import <Metal/Metal.h>

#import "DemoView.hpp"

enum startup_type
{
    eSTART_GLES20,
    eSTART_MTL
};

@interface AppDelegate : UIResponder <UIApplicationDelegate>

@property (strong, nonatomic) UIWindow *        window;

@end

