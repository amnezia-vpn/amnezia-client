#include "platforms/ios/iosPairingQrOverlayWindow.h"

#import <UIKit/UIKit.h>
#import <AVFoundation/AVFoundation.h>

#include <string>

/** Qt on iOS may use a high window level; stay clearly above the main QQuick window. */
static const CGFloat kAmneziaPairingQrOverlayWindowLevel = (CGFloat)UIWindowLevelAlert + 1000.f;

static AmneziaPairingQrScannedUtf8Handler gOnScanned;
static AmneziaPairingQrOverlayBackHandler gOnBack;
static UIWindow *gPairingQrOverlayWindow = nil;
static bool gTorchRequested = false;
/** Last time overlay became key; used to ignore restartCapture during warm-up (see QML kick timer removal). */
static CFAbsoluteTime gPairingQrOverlayKeySince = -1.0;

static void amneziaPairingQrLogScenes(NSString *tag)
{
    NSUInteger n = 0;
    for (UIScene *scene in UIApplication.sharedApplication.connectedScenes) {
        NSString *cls = NSStringFromClass(scene.class);
        NSString *state = @"?";
        switch (scene.activationState) {
            case UISceneActivationStateUnattached:
                state = @"unattached";
                break;
            case UISceneActivationStateForegroundActive:
                state = @"foregroundActive";
                break;
            case UISceneActivationStateForegroundInactive:
                state = @"foregroundInactive";
                break;
            case UISceneActivationStateBackground:
                state = @"background";
                break;
            default:
                break;
        }
        CGRect bounds = CGRectZero;
        if ([scene isKindOfClass:[UIWindowScene class]]) {
            bounds = ((UIWindowScene *)scene).coordinateSpace.bounds;
        }
        NSLog(@"[PairingQrOverlay] %@ scene[%lu] class=%@ state=%@ bounds=%@", tag, (unsigned long)n, cls, state,
              NSStringFromCGRect(bounds));
        n++;
    }
    if (n == 0) {
        NSLog(@"[PairingQrOverlay] %@ connectedScenes count=0", tag);
    }
}

static void amneziaPairingQrLogWindows(NSString *tag)
{
    NSUInteger i = 0;
    for (UIWindow *cw in UIApplication.sharedApplication.windows) {
        NSLog(@"[PairingQrOverlay] %@ UIWindow[%lu] ptr=%p level=%.1f hidden=%d key=%d bounds=%@ rootVC=%@", tag,
              (unsigned long)i, (void *)cw, cw.windowLevel, (int)cw.hidden, (int)cw.isKeyWindow,
              NSStringFromCGRect(cw.bounds),
              cw.rootViewController ? NSStringFromClass(cw.rootViewController.class) : @"(nil)");
        i++;
    }
    if (i == 0) {
        NSLog(@"[PairingQrOverlay] %@ UIApplication.windows count=0", tag);
    }
}

static UIWindowScene *amneziaForegroundWindowScene(void)
{
    for (UIScene *scene in UIApplication.sharedApplication.connectedScenes) {
        if (scene.activationState == UISceneActivationStateForegroundActive
            && [scene isKindOfClass:[UIWindowScene class]]) {
            return (UIWindowScene *)scene;
        }
    }
    for (UIScene *scene in UIApplication.sharedApplication.connectedScenes) {
        if ([scene isKindOfClass:[UIWindowScene class]]) {
            return (UIWindowScene *)scene;
        }
    }
    return nil;
}

static UIWindow *amneziaPickQtAppWindowToRestore(void)
{
    UIWindow *best = nil;
    for (UIWindow *cw in UIApplication.sharedApplication.windows) {
        if (cw == gPairingQrOverlayWindow || cw.hidden) {
            continue;
        }
        if (cw.windowScene && cw.windowLevel <= UIWindowLevelNormal + 1) {
            if (!best || cw.isKeyWindow) {
                best = cw;
            }
        }
    }
    return best;
}

/** Height left uncovered at the bottom so the Qt tab bar on QUIWindow stays visible. */
static CGFloat amneziaPairingQrBottomTabStripReserve(UIWindowScene *scene)
{
    Class qios = NSClassFromString(@"QIOSViewController");
    if (!qios) {
        return 83.f;
    }
    for (UIWindow *cw in scene.windows) {
        if (!cw.rootViewController) {
            continue;
        }
        if ([cw.rootViewController isKindOfClass:qios]) {
            const CGFloat inset = cw.safeAreaInsets.bottom;
            const CGFloat reserve = inset + 49.f;
            NSLog(@"[PairingQrOverlay] bottomReserve: QUIWindow safeBottom=%.1f -> reserve=%.1f", inset, reserve);
            return MIN(MAX(reserve, 72.f), 140.f);
        }
    }
    NSLog(@"[PairingQrOverlay] bottomReserve: QUIWindow not found, default 83");
    return 83.f;
}

static void amneziaApplyReadableOverCameraShadow(UIView *v)
{
    v.layer.shadowColor = [UIColor blackColor].CGColor;
    v.layer.shadowOffset = CGSizeMake(0, 1);
    v.layer.shadowRadius = 4;
    v.layer.shadowOpacity = 0.9;
    v.layer.masksToBounds = NO;
}

@interface AmneziaPairingQrOverlayViewController : UIViewController
@end

@interface AmneziaPairingQrOverlayViewController () <AVCaptureMetadataOutputObjectsDelegate>
@property (nonatomic, strong) AVCaptureSession *captureSession;
@property (nonatomic, strong) AVCaptureVideoPreviewLayer *previewLayer;
@property (nonatomic, strong) AVCaptureDevice *videoDevice;
@property (nonatomic, strong) dispatch_queue_t sessionQueue;
@property (nonatomic, strong) UIView *cameraContainer;
@property (nonatomic, strong) UIView *headerContainer;
@property (nonatomic, strong) UIButton *backButton;
@property (nonatomic, strong) UILabel *titleLabel;
@property (nonatomic, strong) UILabel *subtitleLabel;
@property (nonatomic, strong) UIButton *torchButton;
@property (nonatomic, copy) NSString *chromeTitleText;
@property (nonatomic, copy) NSString *chromeSubtitleText;
@end

@implementation AmneziaPairingQrOverlayViewController

- (void)viewDidLoad
{
    [super viewDidLoad];
    self.view.backgroundColor = [UIColor clearColor];
    if (!self.sessionQueue) {
        self.sessionQueue = dispatch_queue_create("org.amnezia.pairingqr.overlay", DISPATCH_QUEUE_SERIAL);
    }
    [self buildChromeUi];
}

- (void)buildChromeUi
{
    if (self.headerContainer) {
        return;
    }
    UIView *cam = [[UIView alloc] init];
    cam.translatesAutoresizingMaskIntoConstraints = NO;
    cam.backgroundColor = [UIColor clearColor];
    cam.clipsToBounds = YES;
    self.cameraContainer = cam;
    [self.view addSubview:cam];

    UIView *header = [[UIView alloc] init];
    header.translatesAutoresizingMaskIntoConstraints = NO;
    header.backgroundColor = [UIColor clearColor];
    header.opaque = NO;
    header.userInteractionEnabled = YES;
    self.headerContainer = header;
    [self.view addSubview:header];

    UIButton *back = [UIButton buttonWithType:UIButtonTypeSystem];
    back.translatesAutoresizingMaskIntoConstraints = NO;
    back.tintColor = [UIColor whiteColor];
    if (@available(iOS 13.0, *)) {
        UIImage *img = [UIImage systemImageNamed:@"chevron.backward"];
        [back setImage:img forState:UIControlStateNormal];
    } else {
        [back setTitle:@"<" forState:UIControlStateNormal];
    }
    [back addTarget:self action:@selector(backTapped) forControlEvents:UIControlEventTouchUpInside];
    self.backButton = back;
    [header addSubview:back];
    amneziaApplyReadableOverCameraShadow(back);

    UILabel *title = [[UILabel alloc] init];
    title.translatesAutoresizingMaskIntoConstraints = NO;
    title.textColor = [UIColor colorWithWhite:0.96 alpha:1];
    title.font = [UIFont systemFontOfSize:22 weight:UIFontWeightBold];
    title.numberOfLines = 0;
    title.text = self.chromeTitleText.length ? self.chromeTitleText : @"Add device via QR";
    self.titleLabel = title;
    [header addSubview:title];
    amneziaApplyReadableOverCameraShadow(title);

    UILabel *sub = [[UILabel alloc] init];
    sub.translatesAutoresizingMaskIntoConstraints = NO;
    sub.textColor = [UIColor colorWithWhite:0.88 alpha:0.95];
    sub.font = [UIFont systemFontOfSize:14 weight:UIFontWeightRegular];
    sub.numberOfLines = 0;
    sub.text = self.chromeSubtitleText.length
                       ? self.chromeSubtitleText
                       : @"Scan the session QR shown on the device you want to add.";
    self.subtitleLabel = sub;
    [header addSubview:sub];
    amneziaApplyReadableOverCameraShadow(sub);

    UIButton *torch = [UIButton buttonWithType:UIButtonTypeSystem];
    torch.translatesAutoresizingMaskIntoConstraints = NO;
    [torch setTitle:@"🔦" forState:UIControlStateNormal];
    torch.titleLabel.font = [UIFont systemFontOfSize:26];
    torch.backgroundColor = [[UIColor whiteColor] colorWithAlphaComponent:0.22];
    torch.layer.cornerRadius = 28;
    torch.clipsToBounds = YES;
    [torch addTarget:self action:@selector(torchTapped) forControlEvents:UIControlEventTouchUpInside];
    self.torchButton = torch;
    [self.view addSubview:torch];

    UILayoutGuide *safe = self.view.safeAreaLayoutGuide;
    [NSLayoutConstraint activateConstraints:@[
        [cam.topAnchor constraintEqualToAnchor:self.view.topAnchor],
        [cam.leadingAnchor constraintEqualToAnchor:self.view.leadingAnchor],
        [cam.trailingAnchor constraintEqualToAnchor:self.view.trailingAnchor],
        [cam.bottomAnchor constraintEqualToAnchor:self.view.bottomAnchor],

        [header.topAnchor constraintEqualToAnchor:safe.topAnchor],
        [header.leadingAnchor constraintEqualToAnchor:self.view.leadingAnchor],
        [header.trailingAnchor constraintEqualToAnchor:self.view.trailingAnchor],
        [header.heightAnchor constraintGreaterThanOrEqualToConstant:120],

        [back.leadingAnchor constraintEqualToAnchor:header.leadingAnchor constant:4],
        [back.topAnchor constraintEqualToAnchor:header.topAnchor constant:2],
        [back.widthAnchor constraintEqualToConstant:44],
        [back.heightAnchor constraintEqualToConstant:44],

        [title.leadingAnchor constraintEqualToAnchor:header.leadingAnchor constant:16],
        [title.trailingAnchor constraintEqualToAnchor:header.trailingAnchor constant:-16],
        [title.topAnchor constraintEqualToAnchor:back.bottomAnchor constant:2],

        [sub.leadingAnchor constraintEqualToAnchor:title.leadingAnchor],
        [sub.trailingAnchor constraintEqualToAnchor:title.trailingAnchor],
        [sub.topAnchor constraintEqualToAnchor:title.bottomAnchor constant:8],
        [sub.bottomAnchor constraintEqualToAnchor:header.bottomAnchor constant:-10],

        [torch.topAnchor constraintGreaterThanOrEqualToAnchor:header.bottomAnchor constant:8],
        [torch.centerXAnchor constraintEqualToAnchor:self.view.centerXAnchor],
        [torch.widthAnchor constraintEqualToConstant:56],
        [torch.heightAnchor constraintEqualToConstant:56],
        [torch.bottomAnchor constraintEqualToAnchor:safe.bottomAnchor constant:-10],
    ]];
    [header setContentHuggingPriority:UILayoutPriorityRequired forAxis:UILayoutConstraintAxisVertical];
    [header setContentCompressionResistancePriority:UILayoutPriorityRequired forAxis:UILayoutConstraintAxisVertical];
}

- (void)backTapped
{
    NSLog(@"[PairingQrOverlay] native back tapped");
    if (gOnBack) {
        gOnBack();
    }
}

- (void)torchTapped
{
    gTorchRequested = !gTorchRequested;
    NSLog(@"[PairingQrOverlay] native torch toggle -> %d", (int)gTorchRequested);
    [self applyTorchFromGlobalFlag];
    if (gTorchRequested) {
        self.torchButton.backgroundColor = [[UIColor whiteColor] colorWithAlphaComponent:0.42];
        self.torchButton.layer.borderWidth = 2;
        self.torchButton.layer.borderColor = [UIColor colorWithRed:1 green:0.75 blue:0.45 alpha:1].CGColor;
    } else {
        self.torchButton.backgroundColor = [[UIColor whiteColor] colorWithAlphaComponent:0.22];
        self.torchButton.layer.borderWidth = 0;
    }
}

- (void)viewDidLayoutSubviews
{
    [super viewDidLayoutSubviews];
    if (self.previewLayer && self.cameraContainer) {
        self.previewLayer.frame = self.cameraContainer.bounds;
    }
    if (self.headerContainer) {
        [self.view bringSubviewToFront:self.headerContainer];
    }
    if (self.torchButton) {
        [self.view bringSubviewToFront:self.torchButton];
    }
}

- (void)applyTorchOnMainThread:(BOOL)on
{
    AVCaptureDevice *device = self.videoDevice;
    if (!device || ![device hasTorch]) {
        NSLog(@"[PairingQrOverlay] applyTorch skipped on=%d device=%p hasTorch=%d", (int)on, (void *)device,
              device ? (int)[device hasTorch] : -1);
        if (on && gTorchRequested) {
            __unsafe_unretained AmneziaPairingQrOverlayViewController *unsafeSelf = self;
            dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.12 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
                AmneziaPairingQrOverlayViewController *strongSelf = unsafeSelf;
                if (strongSelf && gTorchRequested) {
                    [strongSelf applyTorchOnMainThread:YES];
                }
            });
        }
        return;
    }
    AVCaptureSession *session = self.captureSession;
    if (on && session && ![session isRunning]) {
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.25 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
            if (gTorchRequested) {
                [self applyTorchOnMainThread:YES];
            }
        });
        return;
    }
    NSError *err = nil;
    if (![device lockForConfiguration:&err]) {
        NSLog(@"[PairingQrOverlay] torch lock failed: %@", err.localizedDescription);
        return;
    }
    if (on) {
        err = nil;
        if (![device setTorchModeOnWithLevel:AVCaptureMaxAvailableTorchLevel error:&err]) {
            if ([device isTorchModeSupported:AVCaptureTorchModeOn]) {
                device.torchMode = AVCaptureTorchModeOn;
            }
        }
    } else {
        device.torchMode = AVCaptureTorchModeOff;
    }
    [device unlockForConfiguration];
}

- (void)applyTorchFromGlobalFlag
{
    [self applyTorchOnMainThread:gTorchRequested ? YES : NO];
}

- (void)stopCapturePipelineOnMainThread
{
    [self applyTorchOnMainThread:NO];
    self.videoDevice = nil;

    AVCaptureSession *session = self.captureSession;
    self.captureSession = nil;

    if (self.previewLayer) {
        [self.previewLayer removeFromSuperlayer];
        self.previewLayer = nil;
    }

    if (session) {
        dispatch_queue_t q = self.sessionQueue;
        if (!q) {
            q = dispatch_queue_create("org.amnezia.pairingqr.overlay", DISPATCH_QUEUE_SERIAL);
            self.sessionQueue = q;
        }
        dispatch_sync(q, ^{
            @try {
                if ([session isRunning]) {
                    [session stopRunning];
                }
            } @catch (NSException *ex) {
                NSLog(@"[PairingQrOverlay] stopRunning exception: %@", ex);
            }
        });
    }
}

- (BOOL)startCapturePipelineOnMainThread
{
    NSLog(@"[PairingQrOverlay] startCapturePipelineOnMainThread entry camera.bounds=%@ thread=%@",
          self.cameraContainer ? NSStringFromCGRect(self.cameraContainer.bounds) : @"(nil)",
          [NSThread isMainThread] ? @"main" : @"bg");

    [self stopCapturePipelineOnMainThread];

    if (!self.cameraContainer) {
        NSLog(@"[PairingQrOverlay] no cameraContainer");
        return NO;
    }

    NSError *error = nil;
    AVCaptureDevice *device = [AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeVideo];
    if (!device) {
        NSLog(@"[PairingQrOverlay] no default video device");
        return NO;
    }
    NSLog(@"[PairingQrOverlay] capture device=%p modelID=%@ localizedName=%@", (void *)device, device.modelID,
          device.localizedName);
    AVCaptureDeviceInput *input = [AVCaptureDeviceInput deviceInputWithDevice:device error:&error];
    if (!input) {
        NSLog(@"[PairingQrOverlay] deviceInput failed: %@", error.localizedDescription);
        return NO;
    }
    self.videoDevice = device;

    AVCaptureSession *session = [[AVCaptureSession alloc] init];
    if ([session canSetSessionPreset:AVCaptureSessionPresetHigh]) {
        session.sessionPreset = AVCaptureSessionPresetHigh;
    }
    NSLog(@"[PairingQrOverlay] session preset=%@", session.sessionPreset);

    [session addInput:input];

    AVCaptureMetadataOutput *meta = [[AVCaptureMetadataOutput alloc] init];
    if (![session canAddOutput:meta]) {
        NSLog(@"[PairingQrOverlay] cannot add metadata output");
        return NO;
    }
    [session addOutput:meta];
    dispatch_queue_t q = self.sessionQueue;
    if (!q) {
        q = dispatch_queue_create("org.amnezia.pairingqr.overlay", DISPATCH_QUEUE_SERIAL);
        self.sessionQueue = q;
    }
    [meta setMetadataObjectsDelegate:self queue:q];
    meta.metadataObjectTypes = @[ AVMetadataObjectTypeQRCode ];

    self.captureSession = session;

    AVCaptureVideoPreviewLayer *preview = [[AVCaptureVideoPreviewLayer alloc] initWithSession:session];
    preview.videoGravity = AVLayerVideoGravityResizeAspectFill;
    self.previewLayer = preview;
    [self.cameraContainer.layer insertSublayer:preview atIndex:0];
    preview.frame = self.cameraContainer.bounds;
    NSLog(@"[PairingQrOverlay] previewLayer on host frame=%@", NSStringFromCGRect(preview.frame));

    AVCaptureSession *runningSession = session;
    __unsafe_unretained AmneziaPairingQrOverlayViewController *weakSelf = self;
    dispatch_async(q, ^{
        NSLog(@"[PairingQrOverlay] session queue: startRunning begin session=%p", (void *)runningSession);
        @try {
            [runningSession startRunning];
        } @catch (NSException *ex) {
            NSLog(@"[PairingQrOverlay] startRunning exception: %@", ex);
        }
        const BOOL running = [runningSession isRunning];
        NSLog(@"[PairingQrOverlay] session queue: startRunning end isRunning=%d", (int)running);
        dispatch_async(dispatch_get_main_queue(), ^{
            AmneziaPairingQrOverlayViewController *strongSelf = weakSelf;
            if (!strongSelf) {
                return;
            }
            AVCaptureVideoPreviewLayer *pl = strongSelf.previewLayer;
            NSLog(@"[PairingQrOverlay] post-start main: sessionRunning=%d previewFrame=%@ hostBounds=%@", (int)runningSession.isRunning,
                  pl ? NSStringFromCGRect(pl.frame) : @"(no preview)", NSStringFromCGRect(strongSelf.cameraContainer.bounds));
            [strongSelf applyTorchFromGlobalFlag];
        });
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.35 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
            AmneziaPairingQrOverlayViewController *strongSelf = weakSelf;
            if (!strongSelf) {
                return;
            }
            AVCaptureVideoPreviewLayer *pl = strongSelf.previewLayer;
            NSLog(@"[PairingQrOverlay] probe+350ms: sessionRunning=%d previewFrame=%@", (int)runningSession.isRunning,
                  pl ? NSStringFromCGRect(pl.frame) : @"(nil)");
            amneziaPairingQrLogWindows(@"probe+350ms");
        });
    });

    return YES;
}

- (void)captureOutput:(AVCaptureOutput *)output
    didOutputMetadataObjects:(NSArray<__kindof AVMetadataMachineReadableCodeObject *> *)metadataObjects
           fromConnection:(AVCaptureConnection *)connection
{
    (void)output;
    (void)connection;
    for (AVMetadataMachineReadableCodeObject *obj in metadataObjects) {
        NSString *value = obj.stringValue;
        if (value.length == 0) {
            continue;
        }
        const char *utf8 = value.UTF8String;
        std::string copy(utf8 ? utf8 : "");
        if (copy.empty()) {
            continue;
        }
        dispatch_async(dispatch_get_main_queue(), ^{
            if (gOnScanned) {
                NSLog(@"[PairingQrOverlay] metadata QR len=%lu", (unsigned long)copy.size());
                gOnScanned(copy.c_str());
            } else {
                NSLog(@"[PairingQrOverlay] metadata QR but gOnScanned is nil");
            }
        });
        break;
    }
}

@end

static void amneziaPairingQrOverlayTeardownOnMain(void)
{
    NSLog(@"[PairingQrOverlay] teardownOnMain overlayPtr=%p", (void *)gPairingQrOverlayWindow);
    UIWindow *w = gPairingQrOverlayWindow;
    gPairingQrOverlayWindow = nil;
    gOnScanned = nullptr;
    gOnBack = nullptr;
    gTorchRequested = false;
    gPairingQrOverlayKeySince = -1.0;

    if (w) {
        UIViewController *root = w.rootViewController;
        w.rootViewController = nil;
        w.hidden = YES;
        if ([root isKindOfClass:[AmneziaPairingQrOverlayViewController class]]) {
            [(AmneziaPairingQrOverlayViewController *)root stopCapturePipelineOnMainThread];
        }
    }

    UIWindow *restore = amneziaPickQtAppWindowToRestore();
    if (restore) {
        NSLog(@"[PairingQrOverlay] teardown: restore keyWindow to %@", restore);
        [restore makeKeyWindow];
    } else {
        NSLog(@"[PairingQrOverlay] teardown: no window to restore as key");
    }
    amneziaPairingQrLogWindows(@"afterTeardown");
}

void amneziaIosPairingQrOverlayPresent(AmneziaPairingQrScannedUtf8Handler onScanned, AmneziaPairingQrOverlayBackHandler onBack,
                                       const std::string &titleUtf8, const std::string &subtitleUtf8)
{
    const bool hasScan = static_cast<bool>(onScanned);
    const bool hasBack = static_cast<bool>(onBack);
    NSLog(@"[PairingQrOverlay] C++ present requested hasScan=%d hasBack=%d callerThread=%p", (int)hasScan, (int)hasBack,
          (void *)NSThread.currentThread);
    AmneziaPairingQrScannedUtf8Handler scanH = std::move(onScanned);
    AmneziaPairingQrOverlayBackHandler backH = std::move(onBack);
    const std::string titleCopy = titleUtf8;
    const std::string subCopy = subtitleUtf8;

    dispatch_async(dispatch_get_main_queue(), ^{
        NSLog(@"[PairingQrOverlay] present block on main");
        amneziaPairingQrLogScenes(@"beforeTeardown");
        amneziaPairingQrLogWindows(@"beforeTeardown");
        amneziaPairingQrOverlayTeardownOnMain();
        gOnScanned = std::move(scanH);
        gOnBack = std::move(backH);
        NSLog(@"[PairingQrOverlay] callbacks stored scan=%d back=%d", (int)(bool)gOnScanned, (int)(bool)gOnBack);

        UIWindowScene *scene = amneziaForegroundWindowScene();
        if (!scene) {
            NSLog(@"[PairingQrOverlay] present: no UIWindowScene");
            amneziaPairingQrLogScenes(@"sceneNil");
            gOnScanned = nullptr;
            gOnBack = nullptr;
            return;
        }

        const CGFloat bottomReserve = amneziaPairingQrBottomTabStripReserve(scene);
        const CGRect sceneBounds = scene.coordinateSpace.bounds;
        const CGRect overlayFrame = CGRectMake(0, 0, sceneBounds.size.width, sceneBounds.size.height - bottomReserve);

        AmneziaPairingQrOverlayViewController *vc = [[AmneziaPairingQrOverlayViewController alloc] init];
        NSString *nsTitle = titleCopy.empty() ? nil : [NSString stringWithUTF8String:titleCopy.c_str()];
        NSString *nsSub = subCopy.empty() ? nil : [NSString stringWithUTF8String:subCopy.c_str()];
        vc.chromeTitleText = nsTitle;
        vc.chromeSubtitleText = nsSub;

        UIWindow *w = [[UIWindow alloc] initWithWindowScene:scene];
        w.frame = overlayFrame;
        w.windowLevel = kAmneziaPairingQrOverlayWindowLevel;
        w.backgroundColor = [UIColor blackColor];
        w.rootViewController = vc;
        gPairingQrOverlayWindow = w;
        NSLog(@"[PairingQrOverlay] created UIWindow ptr=%p frame=%@ (sceneH=%.0f bottomReserve=%.0f) level=%.1f", (void *)w,
              NSStringFromCGRect(w.frame), sceneBounds.size.height, bottomReserve, w.windowLevel);

        [w makeKeyAndVisible];
        [w layoutIfNeeded];
        [vc.view setNeedsLayout];
        [vc.view layoutIfNeeded];
        NSLog(@"[PairingQrOverlay] after layout vc.view.bounds=%@ window.key=%d", NSStringFromCGRect(vc.view.bounds),
              (int)w.isKeyWindow);

        gPairingQrOverlayKeySince = CFAbsoluteTimeGetCurrent();

        if (![vc startCapturePipelineOnMainThread]) {
            NSLog(@"[PairingQrOverlay] startCapture failed");
        }
        amneziaPairingQrLogWindows(@"afterPresent");
    });
}

void amneziaIosPairingQrOverlayDismiss()
{
    NSLog(@"[PairingQrOverlay] C++ dismiss requested");
    dispatch_async(dispatch_get_main_queue(), ^{
        amneziaPairingQrOverlayTeardownOnMain();
    });
}

bool amneziaIosPairingQrOverlayIsPresented()
{
    if ([NSThread isMainThread]) {
        return gPairingQrOverlayWindow != nil && !gPairingQrOverlayWindow.hidden;
    }
    __block bool out = false;
    dispatch_sync(dispatch_get_main_queue(), ^{
        out = (gPairingQrOverlayWindow != nil && !gPairingQrOverlayWindow.hidden);
    });
    return out;
}

void amneziaIosPairingQrOverlaySetTorchEnabled(bool on)
{
    NSLog(@"[PairingQrOverlay] C++ setTorch=%d", (int)on);
    gTorchRequested = on;
    dispatch_async(dispatch_get_main_queue(), ^{
        UIWindow *win = gPairingQrOverlayWindow;
        if (!win) {
            NSLog(@"[PairingQrOverlay] setTorch: no overlay window");
            return;
        }
        UIViewController *root = win.rootViewController;
        if ([root isKindOfClass:[AmneziaPairingQrOverlayViewController class]]) {
            AmneziaPairingQrOverlayViewController *vc = (AmneziaPairingQrOverlayViewController *)root;
            [vc applyTorchFromGlobalFlag];
            if (vc.torchButton) {
                if (on) {
                    vc.torchButton.backgroundColor = [[UIColor whiteColor] colorWithAlphaComponent:0.42];
                    vc.torchButton.layer.borderWidth = 2;
                    vc.torchButton.layer.borderColor = [UIColor colorWithRed:1 green:0.75 blue:0.45 alpha:1].CGColor;
                } else {
                    vc.torchButton.backgroundColor = [[UIColor whiteColor] colorWithAlphaComponent:0.18];
                    vc.torchButton.layer.borderWidth = 0;
                }
            }
        }
    });
}

void amneziaIosPairingQrOverlayRestartCapture()
{
    NSLog(@"[PairingQrOverlay] C++ restartCapture requested");
    dispatch_async(dispatch_get_main_queue(), ^{
        const CFAbsoluteTime now = CFAbsoluteTimeGetCurrent();
        if (gPairingQrOverlayKeySince > 0 && (now - gPairingQrOverlayKeySince) < 1.0) {
            NSLog(@"[PairingQrOverlay] restartCapture: skipped warm-up (%.3fs since overlay key)", now - gPairingQrOverlayKeySince);
            return;
        }
        UIWindow *w = gPairingQrOverlayWindow;
        if (!w) {
            NSLog(@"[PairingQrOverlay] restartCapture: no overlay window");
            return;
        }
        UIViewController *root = w.rootViewController;
        if (![root isKindOfClass:[AmneziaPairingQrOverlayViewController class]]) {
            NSLog(@"[PairingQrOverlay] restartCapture: rootVC class=%@", root ? NSStringFromClass(root.class) : @"(nil)");
            return;
        }
        AmneziaPairingQrOverlayViewController *vc = (AmneziaPairingQrOverlayViewController *)root;
        NSLog(@"[PairingQrOverlay] restartCapture: stop+start");
        [vc stopCapturePipelineOnMainThread];
        if (![vc startCapturePipelineOnMainThread]) {
            NSLog(@"[PairingQrOverlay] restart startCapture failed");
        }
    });
}
