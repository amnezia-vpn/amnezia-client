#import <UIKit/UIKit.h>
#import "QtAppDelegate-C-Interface.h"

#include "ui/pages_logic/StartPageLogic.h"

@interface QtAppDelegate : UIResponder <UIApplicationDelegate>
+(QtAppDelegate *)sharedQtAppDelegate;
@property (nonatomic) StartPageLogic* startPageLogic;
@end
