#include "platforms/ios/iosPairingCameraAccess.h"

#import <AVFoundation/AVFoundation.h>
#import <UIKit/UIKit.h>

static UIViewController *amneziaKeyWindowViewController(void)
{
    UIApplication *app = [UIApplication sharedApplication];
    if (@available(iOS 13.0, *)) {
        for (UIScene *scene in app.connectedScenes) {
            if (scene.activationState != UISceneActivationStateForegroundActive) {
                continue;
            }
            if (![scene isKindOfClass:[UIWindowScene class]]) {
                continue;
            }
            UIWindowScene *windowScene = (UIWindowScene *)scene;
            for (UIWindow *window in windowScene.windows) {
                if (window.isKeyWindow && window.rootViewController) {
                    return window.rootViewController;
                }
            }
            for (UIWindow *window in windowScene.windows) {
                if (!window.isHidden && window.rootViewController) {
                    return window.rootViewController;
                }
            }
        }
    }
    for (UIWindow *window in app.windows) {
        if (window.isKeyWindow && window.rootViewController) {
            return window.rootViewController;
        }
    }
    for (UIWindow *window in app.windows) {
        if (window.rootViewController) {
            return window.rootViewController;
        }
    }
    return nil;
}

/** QML window is shorter than UIKit UIWindow (e.g. 759 vs 852); camera preview covers full window — dim strips in safe areas. */
static UIView *s_pairingSafeTopDim = nil;
static UIView *s_pairingSafeBottomDim = nil;
static id s_pairingSafeDimOrientationToken = nil;
/** Qt root for layout: bottom strip fills from maxY to host bottom when Qt view is shorter than host. */
static __unsafe_unretained UIView *s_pairingDimQtRoot = nil;
/** QML-driven tab band (e.g. scanDimBleedBottom) added to safe bottom when Qt root is full height. */
static int s_pairingNativeBottomExtraPt = 0;
/** Opaque strip on UIWindow.layer above AVCaptureVideoPreviewLayer — preview can composite above UIDropShadowView subviews. */
static CALayer *s_pairingWindowBottomMaskLayer = nil;

static UIColor *amneziaPairingBottomChromeOpaqueColor(void)
{
    return [UIColor colorWithRed:(CGFloat)(28.0 / 255.0) green:(CGFloat)(29.0 / 255.0) blue:(CGFloat)(33.0 / 255.0) alpha:1.0f];
}

static CALayer *amneziaFindVideoPreviewLayerInWindow(UIWindow *window)
{
    if (!window) {
        return nil;
    }
    for (CALayer *ly in window.layer.sublayers) {
        if ([ly isKindOfClass:[AVCaptureVideoPreviewLayer class]]) {
            return ly;
        }
    }
    return nil;
}

static void amneziaRemovePairingWindowBottomMaskLayer(void)
{
    [s_pairingWindowBottomMaskLayer removeFromSuperlayer];
    s_pairingWindowBottomMaskLayer = nil;
}

/** Maps bottom strip from host (UIDropShadowView) into window coords and stacks above camera preview on window.layer. */
static void amneziaSyncPairingWindowBottomMaskLayer(UIWindow *window, UIView *host, CGFloat bottomY, CGFloat bottomH, CGFloat width)
{
    if (!window || !host || bottomH < 0.5f) {
        amneziaRemovePairingWindowBottomMaskLayer();
        return;
    }
    CGRect hostRect = CGRectMake(0, bottomY, width, bottomH);
    CGRect winRect = [window convertRect:hostRect fromView:host];
    if (!s_pairingWindowBottomMaskLayer) {
        s_pairingWindowBottomMaskLayer = [[CALayer alloc] init];
        s_pairingWindowBottomMaskLayer.backgroundColor = amneziaPairingBottomChromeOpaqueColor().CGColor;
    }
    s_pairingWindowBottomMaskLayer.frame = winRect;
    s_pairingWindowBottomMaskLayer.zPosition = -500.f;

    CALayer *preview = amneziaFindVideoPreviewLayerInWindow(window);
    if (preview) {
        [window.layer insertSublayer:s_pairingWindowBottomMaskLayer above:preview];
    } else if (s_pairingWindowBottomMaskLayer.superlayer) {
        [s_pairingWindowBottomMaskLayer removeFromSuperlayer];
    }
}

static void amneziaRemovePairingSafeAreaDimStrips(void)
{
    if (s_pairingSafeDimOrientationToken) {
        [[NSNotificationCenter defaultCenter] removeObserver:s_pairingSafeDimOrientationToken];
        s_pairingSafeDimOrientationToken = nil;
    }
    [s_pairingSafeTopDim removeFromSuperview];
    [s_pairingSafeBottomDim removeFromSuperview];
    s_pairingSafeTopDim = nil;
    s_pairingSafeBottomDim = nil;
    s_pairingDimQtRoot = nil;
    amneziaRemovePairingWindowBottomMaskLayer();
}

static void amneziaLayoutPairingSafeAreaDimStrips(void)
{
    if (!s_pairingSafeTopDim || !s_pairingSafeTopDim.superview) {
        return;
    }
    UIWindow *w = s_pairingSafeTopDim.window;
    UIView *host = s_pairingSafeTopDim.superview;
    if (!w || !host) {
        return;
    }
    UIEdgeInsets insets = w.safeAreaInsets;
    CGRect hb = host.bounds;
    s_pairingSafeTopDim.frame = CGRectMake(0, 0, hb.size.width, insets.top);

    CGFloat bottomY = hb.size.height - insets.bottom;
    CGFloat bottomH = insets.bottom;
    UIView *qt = s_pairingDimQtRoot;
    if (qt && qt.superview) {
        CGRect qInHost = [host convertRect:qt.bounds fromView:qt];
        const CGFloat qMaxY = CGRectGetMaxY(qInHost);
        if (qMaxY < hb.size.height - 0.5f) {
            bottomY = qMaxY;
            bottomH = hb.size.height - qMaxY;
        } else if (s_pairingNativeBottomExtraPt > 0) {
            bottomH = insets.bottom + (CGFloat)s_pairingNativeBottomExtraPt;
            bottomY = hb.size.height - bottomH;
        }
    }

    if (bottomH < 0.5f) {
        bottomH = 0.f;
        bottomY = hb.size.height;
    } else {
        /** Pull strip slightly past layout bounds — QML vs UIKit gap can leave a hairline of preview at the physical bottom. */
        const CGFloat kBottomOverscanPt = 12.f;
        bottomH += kBottomOverscanPt;
        bottomY = hb.size.height - bottomH;
        if (bottomY < insets.top + 2.f) {
            bottomY = insets.top + 2.f;
            bottomH = hb.size.height - bottomY;
        }
        /** Stack↔tab hairline: preview on UIWindow.layer can leak slightly above computed strip top vs Qt chrome. */
        const CGFloat kHairlineCoverUpPt = 4.f;
        bottomY -= kHairlineCoverUpPt;
        bottomH += kHairlineCoverUpPt;
        if (bottomY < insets.top + 2.f) {
            bottomH -= (insets.top + 2.f) - bottomY;
            bottomY = insets.top + 2.f;
        }
    }
    s_pairingSafeBottomDim.frame = CGRectMake(0, bottomY, hb.size.width, bottomH);

    amneziaSyncPairingWindowBottomMaskLayer(w, host, bottomY, bottomH, hb.size.width);
}

static void amneziaInstallPairingSafeAreaDimStrips(UIWindow *window, UIView *qtRootView)
{
    amneziaRemovePairingSafeAreaDimStrips();
    if (!window || !qtRootView) {
        return;
    }
    UIView *host = qtRootView.superview ?: window;
    if (!host) {
        return;
    }

    UIEdgeInsets insets = window.safeAreaInsets;
    CGRect hb = host.bounds;
    UIColor *dim = [UIColor colorWithWhite:0 alpha:0.55f];
    UIColor *bottomChromeOpaque = amneziaPairingBottomChromeOpaqueColor();

    s_pairingSafeTopDim = [[UIView alloc] initWithFrame:CGRectMake(0, 0, hb.size.width, insets.top)];
    s_pairingSafeTopDim.backgroundColor = dim;
    s_pairingSafeTopDim.opaque = NO;
    s_pairingSafeTopDim.layer.opaque = NO;
    s_pairingSafeTopDim.userInteractionEnabled = NO;
    s_pairingSafeTopDim.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleBottomMargin;

    s_pairingSafeBottomDim = [[UIView alloc] initWithFrame:CGRectMake(0, hb.size.height - insets.bottom, hb.size.width, insets.bottom)];
    s_pairingSafeBottomDim.backgroundColor = bottomChromeOpaque;
    s_pairingSafeBottomDim.opaque = YES;
    s_pairingSafeBottomDim.layer.opaque = YES;
    s_pairingSafeBottomDim.userInteractionEnabled = NO;
    s_pairingSafeBottomDim.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleTopMargin;

    s_pairingDimQtRoot = qtRootView;

    [host insertSubview:s_pairingSafeTopDim belowSubview:qtRootView];
    [host insertSubview:s_pairingSafeBottomDim belowSubview:qtRootView];

    s_pairingSafeDimOrientationToken = [[NSNotificationCenter defaultCenter]
        addObserverForName:UIDeviceOrientationDidChangeNotification
                      object:nil
                       queue:[NSOperationQueue mainQueue]
                  usingBlock:^(__unused NSNotification *note) {
                      dispatch_async(dispatch_get_main_queue(), ^{
                          amneziaLayoutPairingSafeAreaDimStrips();
                      });
                  }];
}

static void amneziaApplyUnderlayTransparencyToView(UIView *view, BOOL transparent, NSUInteger depth, NSUInteger maxDepth)
{
    if (!view || depth > maxDepth) {
        return;
    }
    if (transparent) {
        view.opaque = NO;
        view.backgroundColor = [UIColor clearColor];
        view.layer.opaque = NO;
    } else {
        view.opaque = YES;
        view.backgroundColor = [UIColor blackColor];
        view.layer.opaque = YES;
    }
    if (depth < maxDepth) {
        for (UIView *child in view.subviews) {
            amneziaApplyUnderlayTransparencyToView(child, transparent, depth + 1, maxDepth);
        }
    }
}

/** Qt's QUIMetalView often sits deeper than a shallow walk; render thread can reset layer flags after resize. */
static void amneziaForceMetalViewsTransparent(UIView *view, NSUInteger depth, NSUInteger maxDepth)
{
    if (!view || depth > maxDepth) {
        return;
    }
    NSString *cn = NSStringFromClass([view class]);
    if ([cn rangeOfString:@"Metal"].location != NSNotFound) {
        view.opaque = NO;
        view.backgroundColor = [UIColor clearColor];
        view.layer.opaque = NO;
        if (view.layer) {
            view.layer.backgroundColor = [UIColor clearColor].CGColor;
        }
    }
    if (depth < maxDepth) {
        for (UIView *child in view.subviews) {
            amneziaForceMetalViewsTransparent(child, depth + 1, maxDepth);
        }
    }
}

static void amneziaApplyPairingUnderlayWalk(UIView *root, BOOL enable)
{
    const NSUInteger kMaxDepth = 24;
    amneziaApplyUnderlayTransparencyToView(root, enable ? YES : NO, 0, kMaxDepth);
    if (enable) {
        amneziaForceMetalViewsTransparent(root, 0, kMaxDepth);
    }
}

bool amneziaIosPairingCameraAccessGranted()
{
    const AVAuthorizationStatus status = [AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeVideo];
    return status == AVAuthorizationStatusAuthorized;
}

void amneziaIosRequestPairingCameraAccess(const std::function<void(bool)> &onDone)
{
    const AVAuthorizationStatus status = [AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeVideo];
    if (status == AVAuthorizationStatusAuthorized) {
        onDone(true);
        return;
    }
    if (status == AVAuthorizationStatusDenied || status == AVAuthorizationStatusRestricted) {
        onDone(false);
        return;
    }
    [AVCaptureDevice requestAccessForMediaType:AVMediaTypeVideo
                             completionHandler:^(BOOL granted) {
                                 dispatch_async(dispatch_get_main_queue(), ^{
                                     onDone(static_cast<bool>(granted));
                                 });
                             }];
}

void amneziaIosOpenApplicationSettings()
{
    NSURL *url = [NSURL URLWithString:UIApplicationOpenSettingsURLString];
    if (url != nil) {
        [[UIApplication sharedApplication] openURL:url options:@{} completionHandler:nil];
    }
}

void amneziaIosPairingRelayoutChromeIfNeeded(void)
{
    if (!s_pairingSafeBottomDim || !s_pairingSafeBottomDim.superview) {
        return;
    }
    amneziaLayoutPairingSafeAreaDimStrips();
}

void amneziaIosSetPairingEmbeddedCameraNativeBottomExtraPt(int extraPt)
{
    const int v = extraPt < 0 ? 0 : extraPt;
    if (s_pairingNativeBottomExtraPt == v) {
        return;
    }
    s_pairingNativeBottomExtraPt = v;
    void (^relayout)(void) = ^{
        amneziaLayoutPairingSafeAreaDimStrips();
    };
    if ([NSThread isMainThread]) {
        relayout();
    } else {
        dispatch_async(dispatch_get_main_queue(), relayout);
    }
}

void amneziaIosApplyEmbeddedCameraUnderlayToQtView(bool enable)
{
    void (^work)(void) = ^{
        UIViewController *vc = amneziaKeyWindowViewController();
        if (!vc || !vc.view) {
            return;
        }
        UIView *root = vc.view;
        UIWindow *win = root.window;
        if (!enable) {
            s_pairingNativeBottomExtraPt = 0;
            amneziaRemovePairingSafeAreaDimStrips();
        }
        if (enable) {
            vc.edgesForExtendedLayout = UIRectEdgeAll;
            if (@available(iOS 11.0, *)) {
                vc.extendedLayoutIncludesOpaqueBars = YES;
            }
        }
        amneziaApplyPairingUnderlayWalk(root, enable);
        if (enable && win) {
            amneziaInstallPairingSafeAreaDimStrips(win, root);
            amneziaLayoutPairingSafeAreaDimStrips();
        }
        if (enable) {
            dispatch_async(dispatch_get_main_queue(), ^{
                UIViewController *vc2 = amneziaKeyWindowViewController();
                if (!vc2 || !vc2.view) {
                    return;
                }
                amneziaApplyPairingUnderlayWalk(vc2.view, YES);
            });
        }
    };
    if ([NSThread isMainThread]) {
        work();
    } else {
        dispatch_sync(dispatch_get_main_queue(), work);
    }
}
