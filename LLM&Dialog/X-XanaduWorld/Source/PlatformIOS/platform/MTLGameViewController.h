//
//  MTLGameViewController.h
//  GLESxMetal//
//  Created by lixin on 15/5/14.
//  Copyright (c) 2015å¹´ pixelsoft. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <Metal/Metal.h>
#import <QuartzCore/QuartzCore.h>
#import <simd/simd.h>
#import "UIProtocol.h"
#include "TextInputViewController.h"
#include <MetalKit/MetalKit.h>

@interface MTLGameViewController : UIViewController<UIProtocol, MTKViewDelegate>


//-(void) showInputView;

-(void) inputBeginWithText:(NSString *)text x:(unsigned)x y:(unsigned)y;

@end
