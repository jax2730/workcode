//
//  GLKGameView.h
//  GLESxMetal
//
//  Created by lixin on 15/5/14.
//  Copyright (c) 2015å¹´ pixelsoft. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <GLKit/GLKit.h>

extern const int TOUCH_SUPPORT_COUNT;

@interface GLKGameView : GLKView
{
    
}

@property (nonatomic, strong) NSMutableArray * touchArray;
@property (nonatomic, assign) float            nativeScale;

- (void)initTouchSupport;

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event;
- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event;
- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event;
- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event;

@end
