#import "ioscontextmenu.h"

#import <UIKit/UIKit.h>
#import <objc/runtime.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QPointer>
#include <QtGui/QGuiApplication>
#include <QtGui/QInputMethod>
#include <QtQuick/QQuickWindow>

namespace
{
// Keys for attaching the helper objects to the UIView.
const void *kEditMenuInteractionKey = &kEditMenuInteractionKey;
const void *kEditMenuDelegateKey = &kEditMenuDelegateKey;
const void *kEditMenuResponderKey = &kEditMenuResponderKey;

// Menu titles reuse the ContextMenuType translations; the accelerator
// ampersands are meaningless on iOS and get stripped.
NSString *menuTitle(const char *sourceText)
{
    QString title = QCoreApplication::translate("ContextMenuType", sourceText);
    title.remove(QLatin1Char('&'));
    return title.toNSString();
}

// The handler outlives the delegate call, so it must own its own copy of the
// guarded pointer: the local variable here is captured by the block by value
// (copy-constructed when the block is copied to the heap). Capturing a
// C++ lambda's reference capture instead would leave the block with a
// dangling pointer into the delegate method's stack frame.
API_AVAILABLE(ios(16.0))
UIAction *makeEditAction(NSString *title, const QPointer<QQuickItem> &target, const char *slot)
{
    const QPointer<QQuickItem> guardedTarget = target;
    return [UIAction actionWithTitle:title
                               image:nil
                          identifier:nil
                             handler:^(UIAction *) {
                                 if (QQuickItem *item = guardedTarget.data()) {
                                     // Queued: the handler fires mid-dismissal
                                     // of the menu, let UIKit unwind first.
                                     QMetaObject::invokeMethod(item, slot, Qt::QueuedConnection);
                                 }
                             }];
}
}

// UIKit presents an edit menu only when the first responder is inside the
// interaction view's hierarchy. While the virtual keyboard is up that is
// Qt's text input responder, but for read-only fields (or before the
// keyboard appears) nothing suitable is first responder and the present is
// silently ignored ("did not have performable commands and/or actions").
// This zero-sized subview steps in as the first responder for those cases.
@interface AmneziaEditMenuResponderView : UIView
@end

@implementation AmneziaEditMenuResponderView

- (BOOL)canBecomeFirstResponder
{
    return YES;
}

@end

// Builds the edit menu from the state of the focused QML text control and
// invokes its slots directly. The system's suggested actions can't be used:
// they are collected from the first responder, and Qt's text responder is
// first responder only while the virtual keyboard is up (never for read-only
// fields), which would leave the menu empty.
API_AVAILABLE(ios(16.0))
@interface AmneziaEditMenuDelegate : NSObject <UIEditMenuInteractionDelegate>
@property (nonatomic, weak) UIView *responderView;
- (void)setTargetItem:(QQuickItem *)item;
@end

@implementation AmneziaEditMenuDelegate {
    QPointer<QQuickItem> m_target;
}

- (void)setTargetItem:(QQuickItem *)item
{
    m_target = item;
}

- (UIMenu *)editMenuInteraction:(UIEditMenuInteraction *)interaction
           menuForConfiguration:(UIEditMenuConfiguration *)configuration
               suggestedActions:(NSArray<UIMenuElement *> *)suggestedActions
{
    QQuickItem *item = m_target.data();
    if (!item) {
        return nil;
    }

    // The system's suggested actions are UICommands and get re-validated
    // against the first responder right before the menu shows, which makes
    // them hostage to Qt's input-method state. UIActions with handlers skip
    // that validation entirely, so the menu is always built by hand from the
    // QML control's state.
    const bool hasSelection = !item->property("selectedText").toString().isEmpty();
    const bool readOnly = item->property("readOnly").toBool();
    const bool canPaste = item->property("canPaste").toBool();
    const bool hasText = item->property("length").toInt() > 0;

    NSMutableArray<UIMenuElement *> *actions = [NSMutableArray array];
    if (hasSelection && !readOnly) {
        [actions addObject:makeEditAction(menuTitle("C&ut"), m_target, "cut")];
    }
    if (hasSelection) {
        [actions addObject:makeEditAction(menuTitle("&Copy"), m_target, "copy")];
    }
    if (canPaste && !readOnly) {
        [actions addObject:makeEditAction(menuTitle("&Paste"), m_target, "paste")];
    }
    if (hasText) {
        [actions addObject:makeEditAction(menuTitle("&SelectAll"), m_target, "selectAll")];
    }

    if (actions.count == 0) {
        return nil;
    }

    return [UIMenu menuWithTitle:@"" children:actions];
}

- (void)editMenuInteraction:(UIEditMenuInteraction *)interaction
   willDismissMenuForConfiguration:(UIEditMenuConfiguration *)configuration
                          animator:(id<UIEditMenuInteractionAnimating>)animator
{
    // Give the borrowed first-responder status back once the menu goes away.
    if (self.responderView.isFirstResponder) {
        [self.responderView resignFirstResponder];
    }
}

@end

bool IosContextMenu::isAvailable() const
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
    // Since Qt 6.10 the ContextMenu attached type is backed by a native menu
    // on iOS, so the helper must stay out of the way.
    return false;
#else
    // UIEditMenuInteraction needs iOS 16, which is the deployment target.
    return true;
#endif
}

void IosContextMenu::present(QQuickItem *target, qreal x, qreal y)
{
    if (!target || !target->window()) {
        return;
    }

    // On iOS QWindow::winId() is the backing UIView.
    UIView *view = (__bridge UIView *)reinterpret_cast<void *>(target->window()->winId());
    if (!view) {
        return;
    }

    // Scene coordinates match the backing view's coordinate space.
    const QPointF scenePos = target->mapToScene(QPointF(x, y));

    AmneziaEditMenuDelegate *delegate = objc_getAssociatedObject(view, kEditMenuDelegateKey);
    UIEditMenuInteraction *interaction = objc_getAssociatedObject(view, kEditMenuInteractionKey);
    AmneziaEditMenuResponderView *responderView = objc_getAssociatedObject(view, kEditMenuResponderKey);
    if (!interaction) {
        responderView = [[AmneziaEditMenuResponderView alloc] initWithFrame:CGRectZero];
        [view addSubview:responderView];

        delegate = [[AmneziaEditMenuDelegate alloc] init];
        delegate.responderView = responderView;

        interaction = [[UIEditMenuInteraction alloc] initWithDelegate:delegate];
        [view addInteraction:interaction];

        objc_setAssociatedObject(view, kEditMenuResponderKey, responderView, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        objc_setAssociatedObject(view, kEditMenuDelegateKey, delegate, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        objc_setAssociatedObject(view, kEditMenuInteractionKey, interaction, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }

    [delegate setTargetItem:target];

    // While the keyboard is up for the target field, Qt's text input
    // responder is the first responder and lives in this view's responder
    // chain, which satisfies UIKit. Otherwise (read-only fields never raise
    // the keyboard) borrow first-responder status. Qt's idea of the keyboard
    // state is not trustworthy here, so key off the field being editable.
    const bool readOnly = target->property("readOnly").toBool();
    if (readOnly || !QGuiApplication::inputMethod()->isVisible()) {
        [responderView becomeFirstResponder];
    }

    // Defer the actual present to the next main-loop iteration: the request
    // arrives while the long-press touch is still active, and presenting
    // mid-gesture makes UIKit discard the menu. Requests are also coalesced —
    // a double tap asks for the menu twice (the TapHandler and the ContextMenu
    // attached type both fire), and re-presenting makes the menu flicker.
    static BOOL presentPending = NO;
    if (presentPending) {
        return;
    }
    presentPending = YES;

    UIEditMenuConfiguration *configuration =
        [UIEditMenuConfiguration configurationWithIdentifier:nil
                                                 sourcePoint:CGPointMake(scenePos.x(), scenePos.y())];
    dispatch_async(dispatch_get_main_queue(), ^{
        presentPending = NO;
        [interaction presentEditMenuWithConfiguration:configuration];
    });
}
