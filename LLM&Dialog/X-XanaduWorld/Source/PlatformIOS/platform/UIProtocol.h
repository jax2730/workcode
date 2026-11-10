//
//  UIProtocol.h
//  GxUIProject
//
//  Created by mengxin on 16/2/2.
//  Copyright © 2016年 lixin. All rights reserved.
//

#ifndef UIProtocol_h
#define UIProtocol_h

#import <UIKit/UIKit.h>

@protocol UIProtocol<NSObject>

-(void) inputBeginWithText:(NSString *)text x:(unsigned)x y:(unsigned)y;

@end


#endif /* UIProtocol_h */
