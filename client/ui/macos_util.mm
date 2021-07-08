#include <QMainWindow>
#include <Cocoa/Cocoa.h>
#include "macos_util.h"


#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>

void setDockIconVisible(bool visible)
{
    if (!visible) {
        [NSApp setActivationPolicy: NSApplicationActivationPolicyAccessory];
    } else {
        [NSApp setActivationPolicy: NSApplicationActivationPolicyRegular];
    }
}

//this Objective-c class is used to override the action of system close button and zoom button
//https://stackoverflow.com/questions/27643659/setting-c-function-as-selector-for-nsbutton-produces-no-results
@interface ButtonPasser : NSObject{
}
@property(readwrite) QMainWindow* window;
+ (void)closeButtonAction:(id)sender;
- (void)zoomButtonAction:(id)sender;
@end

@implementation ButtonPasser{
}
+ (void)closeButtonAction:(id)sender
{
    Q_UNUSED(sender);
    ProcessSerialNumber pn;
    GetFrontProcess (&pn);
    ShowHideProcess(&pn,false);
}
- (void)zoomButtonAction:(id)sender
{
    Q_UNUSED(sender);
    if (0 == self.window) return;
    if (self.window->isMaximized()) self.window->showNormal();
    else self.window->showMaximized();
}
@end

void fixWidget(QWidget *widget)
{
    NSView *view = (NSView *)widget->winId();
    if (0 == view) return;
    NSWindow *window = view.window;
    if (0 == window) return;

    //override the action of close button
    //https://stackoverflow.com/questions/27643659/setting-c-function-as-selector-for-nsbutton-produces-no-results
    //https://developer.apple.com/library/content/documentation/General/Conceptual/CocoaEncyclopedia/Target-Action/Target-Action.html
//    NSButton *closeButton = [window standardWindowButton:NSWindowCloseButton];
//    [closeButton setTarget:[ButtonPasser class]];
//    [closeButton setAction:@selector(closeButtonAction:)];

    [[window standardWindowButton:NSWindowZoomButton] setHidden:YES];
    [[window standardWindowButton:NSWindowMiniaturizeButton] setHidden:YES];
}
