#import <UIKit/UIKit.h>
#import "QtAppDelegate-C-Interface.h"

#include "ui/controllers/importController.h"

@interface QtAppDelegate : UIResponder <UIApplicationDelegate>
+(QtAppDelegate *)sharedQtAppDelegate;
@property (nonatomic) ImportController* ImportController;
@end
