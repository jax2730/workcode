//
//  TextInputViewController.h
//  GxUIProject
//
//  Created by mengxin on 16/1/29.
//  Copyright © 2016年 lixin. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

@interface TextInputViewController : UIViewController
{
    
}

@property (nonatomic , weak) IBOutlet UITextField *     textInput;
@property (nonatomic , weak) IBOutlet UIButton*         btok;
@property (nonatomic , weak) UIView *                   parent;

+(TextInputViewController *) newInstance;

-(void)close;
-(void)show:(UIView *)parent withText:(NSString * )text;
-(IBAction) applyText:(id)sender;

@end
