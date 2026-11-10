//
//  GameUtils.h
//  PadBO2
//
//  Created by chenanzhi on 2018/9/18.
//  Copyright © 2018年 EchoEngine. All rights reserved.
//

#ifndef GameUtils_h
#define GameUtils_h

#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>

@interface GameUtils : NSObject  ;

+(NSString *) getSysInfoByName: (const char *)typeSpecifier;

+(NSString *) getPlatform;

+(unsigned long long) getSysInfo: (uint) typeSpecifier;

//unit is KB
+(unsigned long long) getTotalMemory;

//unit is KB
+(unsigned long long) getUserMemory;

//unit is KB
+(unsigned long long) getAvailableMemory;

+(CGFloat) getScreenWidth;

+(CGFloat) getScreenHeight;

+(CGFloat) getScreenPPI;

@end

#endif /* GameUtils_h */
