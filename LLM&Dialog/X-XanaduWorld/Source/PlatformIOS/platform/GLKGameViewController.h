//
//  GLKGameViewController.h
//  GLESxMetal
//
//  Created by lixin on 15/5/14.
//  Copyright (c) 2015å¹´ pixelsoft. All rights reserved.
//

#import <GLKit/GLKit.h>
#import "UIProtocol.h"

@interface GLKGameViewController : GLKViewController<UIProtocol>

-(void)initGL;
-(void)initIGXView;
-(void)tearDownGL;
-(void) inputBeginWithText:(NSString *)text x:(unsigned)x y:(unsigned)y;

@end
