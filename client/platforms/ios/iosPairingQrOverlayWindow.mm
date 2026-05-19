#include "platforms/ios/iosPairingQrOverlayWindow.h"

#import <UIKit/UIKit.h>
#import <AVFoundation/AVFoundation.h>
#import <QuartzCore/QuartzCore.h>
#import <math.h>

#include <string>

static const CGFloat kAmneziaPairingQrOverlayWindowLevel = (CGFloat)UIWindowLevelAlert + 1000.f;

static AmneziaPairingQrScannedUtf8Handler gOnScanned;
static AmneziaPairingQrOverlayBackHandler gOnBack;
static UIWindow *gPairingQrOverlayWindow = nil;
static bool gTorchRequested = false;
static CFAbsoluteTime gPairingQrOverlayKeySince = -1.0;

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
            return MIN(MAX(reserve, 72.f), 140.f);
        }
    }
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

static UIColor *amneziaPaleGray(void)
{
    return [UIColor colorWithRed:(CGFloat)0xD7 / 255.0 green:(CGFloat)0xD8 / 255.0 blue:(CGFloat)0xDB / 255.0 alpha:1.0];
}

static void amneziaAddCornerMinorArc(UIBezierPath *p, CGPoint C, CGFloat r, CGPoint S, CGPoint E)
{
    const CGFloat as = atan2f((float)(S.y - C.y), (float)(S.x - C.x));
    CGFloat ae = atan2f((float)(E.y - C.y), (float)(E.x - C.x));
    while (ae - as > (CGFloat)M_PI) {
        ae -= (CGFloat)(2.0 * M_PI);
    }
    while (ae - as < (CGFloat)(-M_PI)) {
        ae += (CGFloat)(2.0 * M_PI);
    }
    const CGFloat minor = ae - as;
    const BOOL cw = minor > 0;
    [p addArcWithCenter:C radius:r startAngle:as endAngle:ae clockwise:cw];
}

static UIBezierPath *amneziaScanBracketStrokePath(int corner, CGFloat x0, CGFloat y0, CGFloat s, CGFloat R, CGFloat L, CGFloat t)
{
    const CGFloat r = MAX(1.5, R - t * 0.5);
    UIBezierPath *p = [UIBezierPath bezierPath];
    const CGFloat yy = y0 + t * 0.5f;
    const CGFloat yyb = y0 + s - t * 0.5f;
    const CGFloat xx = x0 + t * 0.5f;
    const CGFloat xxb = x0 + s - t * 0.5f;

    switch (corner) {
        case 0: {
            const CGPoint cTL = CGPointMake(x0 + R, y0 + R);
            const CGPoint sTL = CGPointMake(x0 + R, yy);
            const CGPoint eTL = CGPointMake(xx, y0 + R);
            [p moveToPoint:CGPointMake(x0 + R + L, yy)];
            [p addLineToPoint:sTL];
            amneziaAddCornerMinorArc(p, cTL, r, sTL, eTL);
            const CGFloat yEndTL = MIN(y0 + R + L, y0 + s - R - t * 0.5f);
            [p addLineToPoint:CGPointMake(xx, MAX(yEndTL, y0 + R + 2.f))];
        } break;
        case 1: {
            const CGPoint cTR = CGPointMake(x0 + s - R, y0 + R);
            const CGPoint sTR = CGPointMake(x0 + s - R, yy);
            const CGPoint eTR = CGPointMake(xxb, y0 + R);
            [p moveToPoint:CGPointMake(x0 + s - R - L, yy)];
            [p addLineToPoint:sTR];
            amneziaAddCornerMinorArc(p, cTR, r, sTR, eTR);
            const CGFloat yEndTR = MIN(y0 + R + L, y0 + s - R - t * 0.5f);
            [p addLineToPoint:CGPointMake(xxb, MAX(yEndTR, y0 + R + 2.f))];
        } break;
        case 2: {
            const CGPoint cBL = CGPointMake(x0 + R, y0 + s - R);
            const CGPoint sBL = CGPointMake(x0 + R, yyb);
            const CGPoint eBL = CGPointMake(xx, y0 + s - R);
            [p moveToPoint:CGPointMake(x0 + R + L, yyb)];
            [p addLineToPoint:sBL];
            amneziaAddCornerMinorArc(p, cBL, r, sBL, eBL);
            const CGFloat yEndTopRef = MAX(MIN(y0 + R + L, y0 + s - R - t * 0.5f), y0 + R + 2.f);
            const CGFloat yLegBL = y0 + s + y0 - yEndTopRef;
            [p addLineToPoint:CGPointMake(xx, yLegBL)];
        } break;
        case 3: {
            const CGPoint cBR = CGPointMake(x0 + s - R, y0 + s - R);
            const CGPoint sBR = CGPointMake(x0 + s - R, yyb);
            const CGPoint eBR = CGPointMake(xxb, y0 + s - R);
            [p moveToPoint:CGPointMake(x0 + s - R - L, yyb)];
            [p addLineToPoint:sBR];
            amneziaAddCornerMinorArc(p, cBR, r, sBR, eBR);
            const CGFloat yEndTopRef = MAX(MIN(y0 + R + L, y0 + s - R - t * 0.5f), y0 + R + 2.f);
            const CGFloat yLegBR = y0 + s + y0 - yEndTopRef;
            [p addLineToPoint:CGPointMake(xxb, yLegBR)];
        } break;
        default:
            break;
    }
    return p;
}

@interface AmneziaPairingQrOverlayViewController : UIViewController
@end

@interface AmneziaPairingQrOverlayViewController () <AVCaptureMetadataOutputObjectsDelegate>
@property (nonatomic, strong) AVCaptureSession *captureSession;
@property (nonatomic, strong) AVCaptureMetadataOutput *metadataOutput;
@property (nonatomic, strong) AVCaptureVideoPreviewLayer *previewLayer;
@property (nonatomic, strong) AVCaptureDevice *videoDevice;
@property (nonatomic, strong) dispatch_queue_t sessionQueue;
@property (nonatomic, strong) UIView *cameraContainer;
@property (nonatomic, strong) UIView *headerContainer;
@property (nonatomic, strong) UIButton *backButton;
@property (nonatomic, strong) UILabel *titleLabel;
@property (nonatomic, strong) UILabel *subtitleLabel;
@property (nonatomic, strong) UIButton *torchButton;
@property (nonatomic, strong) NSLayoutConstraint *torchCenterYConstraint;
@property (nonatomic, copy) NSString *chromeTitleText;
@property (nonatomic, copy) NSString *chromeSubtitleText;
@property (nonatomic, strong) UIView *scanDimView;
@property (nonatomic, strong) CAShapeLayer *scanDimMaskLayer;
@property (nonatomic, strong) UIView *scanHoleFillView;
@property (nonatomic, strong) CAShapeLayer *scanHoleHighlightLayer;
@property (nonatomic, strong) UIView *bracketContainer;
@property (nonatomic, strong) NSMutableArray<CAShapeLayer *> *bracketCornerLayers;
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

    UIView *holeFill = [[UIView alloc] init];
    holeFill.translatesAutoresizingMaskIntoConstraints = NO;
    holeFill.backgroundColor = [UIColor clearColor];
    holeFill.opaque = NO;
    holeFill.userInteractionEnabled = NO;
    self.scanHoleFillView = holeFill;
    CAShapeLayer *hi = [CAShapeLayer layer];
    hi.fillColor = [UIColor colorWithWhite:1.0 alpha:0.14].CGColor;
    hi.strokeColor = nil;
    [holeFill.layer addSublayer:hi];
    self.scanHoleHighlightLayer = hi;
    [self.view addSubview:holeFill];

    UIView *dim = [[UIView alloc] init];
    dim.translatesAutoresizingMaskIntoConstraints = NO;
    dim.backgroundColor = [UIColor colorWithWhite:0.02 alpha:0.55];
    dim.userInteractionEnabled = NO;
    dim.opaque = NO;
    self.scanDimView = dim;
    [self.view addSubview:dim];

    CAShapeLayer *dimMask = [CAShapeLayer layer];
    dimMask.fillRule = kCAFillRuleEvenOdd;
    dimMask.fillColor = [UIColor blackColor].CGColor;
    dim.layer.mask = dimMask;
    self.scanDimMaskLayer = dimMask;

    UIView *bracketHost = [[UIView alloc] init];
    bracketHost.translatesAutoresizingMaskIntoConstraints = NO;
    bracketHost.backgroundColor = [UIColor clearColor];
    bracketHost.opaque = NO;
    bracketHost.userInteractionEnabled = NO;
    self.bracketContainer = bracketHost;
    [self.view addSubview:bracketHost];

    self.bracketCornerLayers = [NSMutableArray arrayWithCapacity:4];
    for (NSInteger i = 0; i < 4; i++) {
        CAShapeLayer *sl = [CAShapeLayer layer];
        sl.fillColor = nil;
        sl.strokeColor = [UIColor colorWithWhite:0.94 alpha:1].CGColor;
        sl.lineWidth = 5.0;
        sl.lineCap = kCALineCapRound;
        sl.lineJoin = kCALineJoinRound;
        [bracketHost.layer addSublayer:sl];
        [self.bracketCornerLayers addObject:sl];
    }

    UIView *header = [[UIView alloc] init];
    header.translatesAutoresizingMaskIntoConstraints = NO;
    header.backgroundColor = [UIColor clearColor];
    header.opaque = NO;
    header.userInteractionEnabled = YES;
    self.headerContainer = header;
    [self.view addSubview:header];

    UIButton *back = [UIButton buttonWithType:UIButtonTypeSystem];
    back.translatesAutoresizingMaskIntoConstraints = NO;
    back.tintColor = amneziaPaleGray();
    if (@available(iOS 13.0, *)) {
        const CGFloat kBackArrowPt = 20.0;
        UIImageSymbolConfiguration *sym =
                [UIImageSymbolConfiguration configurationWithPointSize:kBackArrowPt weight:UIImageSymbolWeightMedium
                                                                  scale:UIImageSymbolScaleDefault];
        UIImage *img = [UIImage systemImageNamed:@"arrow.left" withConfiguration:sym];
        [back setImage:[img imageWithRenderingMode:UIImageRenderingModeAlwaysTemplate] forState:UIControlStateNormal];
    } else {
        [back setTitle:@"<" forState:UIControlStateNormal];
    }
    [back addTarget:self action:@selector(backTapped) forControlEvents:UIControlEventTouchUpInside];
    self.backButton = back;
    [header addSubview:back];

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

        [holeFill.topAnchor constraintEqualToAnchor:self.view.topAnchor],
        [holeFill.leadingAnchor constraintEqualToAnchor:self.view.leadingAnchor],
        [holeFill.trailingAnchor constraintEqualToAnchor:self.view.trailingAnchor],
        [holeFill.bottomAnchor constraintEqualToAnchor:self.view.bottomAnchor],

        [dim.topAnchor constraintEqualToAnchor:self.view.topAnchor],
        [dim.leadingAnchor constraintEqualToAnchor:self.view.leadingAnchor],
        [dim.trailingAnchor constraintEqualToAnchor:self.view.trailingAnchor],
        [dim.bottomAnchor constraintEqualToAnchor:self.view.bottomAnchor],

        [bracketHost.topAnchor constraintEqualToAnchor:self.view.topAnchor],
        [bracketHost.leadingAnchor constraintEqualToAnchor:self.view.leadingAnchor],
        [bracketHost.trailingAnchor constraintEqualToAnchor:self.view.trailingAnchor],
        [bracketHost.bottomAnchor constraintEqualToAnchor:self.view.bottomAnchor],

        [header.topAnchor constraintEqualToAnchor:safe.topAnchor],
        [header.leadingAnchor constraintEqualToAnchor:self.view.leadingAnchor],
        [header.trailingAnchor constraintEqualToAnchor:self.view.trailingAnchor],
        [header.heightAnchor constraintGreaterThanOrEqualToConstant:120],

        [back.leadingAnchor constraintEqualToAnchor:header.leadingAnchor constant:8],
        [back.topAnchor constraintEqualToAnchor:header.topAnchor constant:20],
        [back.widthAnchor constraintEqualToConstant:40],
        [back.heightAnchor constraintEqualToConstant:40],

        [title.leadingAnchor constraintEqualToAnchor:header.leadingAnchor constant:16],
        [title.trailingAnchor constraintEqualToAnchor:header.trailingAnchor constant:-16],
        [title.topAnchor constraintEqualToAnchor:back.bottomAnchor],

        [sub.leadingAnchor constraintEqualToAnchor:title.leadingAnchor],
        [sub.trailingAnchor constraintEqualToAnchor:title.trailingAnchor],
        [sub.topAnchor constraintEqualToAnchor:title.bottomAnchor constant:8],
        [sub.bottomAnchor constraintEqualToAnchor:header.bottomAnchor constant:-10],

        [torch.topAnchor constraintGreaterThanOrEqualToAnchor:header.bottomAnchor constant:8],
        [torch.centerXAnchor constraintEqualToAnchor:self.view.centerXAnchor],
        [torch.widthAnchor constraintEqualToConstant:56],
        [torch.heightAnchor constraintEqualToConstant:56],
    ]];
    NSLayoutConstraint *torchCy = [torch.centerYAnchor constraintEqualToAnchor:self.view.topAnchor constant:200.0];
    self.torchCenterYConstraint = torchCy;
    torchCy.active = YES;
    [header setContentHuggingPriority:UILayoutPriorityRequired forAxis:UILayoutConstraintAxisVertical];
    [header setContentCompressionResistancePriority:UILayoutPriorityRequired forAxis:UILayoutConstraintAxisVertical];
}

- (void)applyMetadataRectOfInterestForScanHole:(CGRect)holeInScanDimBounds
{
    if (!self.previewLayer || !self.metadataOutput || !self.scanDimView || !self.cameraContainer) {
        return;
    }
    if (CGRectIsEmpty(holeInScanDimBounds) || holeInScanDimBounds.size.width < 24.0 || holeInScanDimBounds.size.height < 24.0) {
        return;
    }

    CGRect holeInCam = [self.scanDimView convertRect:holeInScanDimBounds toView:self.cameraContainer];
    holeInCam = CGRectIntersection(holeInCam, self.cameraContainer.bounds);
    if (CGRectIsEmpty(holeInCam)) {
        return;
    }

    const CGRect plFrame = self.previewLayer.frame;
    CGRect holeInPreview = CGRectOffset(holeInCam, -plFrame.origin.x, -plFrame.origin.y);
    holeInPreview = CGRectIntersection(holeInPreview, self.previewLayer.bounds);
    if (CGRectIsEmpty(holeInPreview)) {
        return;
    }

    CGRect roi = [self.previewLayer metadataOutputRectOfInterestForRect:holeInPreview];
    roi.origin.x = MAX(0.0, MIN(1.0, roi.origin.x));
    roi.origin.y = MAX(0.0, MIN(1.0, roi.origin.y));
    roi.size.width = MAX(0.02, MIN(1.0 - roi.origin.x, roi.size.width));
    roi.size.height = MAX(0.02, MIN(1.0 - roi.origin.y, roi.size.height));

    AVCaptureMetadataOutput *mo = self.metadataOutput;
    dispatch_queue_t sq = self.sessionQueue;
    if (!mo || !sq) {
        return;
    }
    dispatch_async(sq, ^{
        mo.rectOfInterest = roi;
    });
}

- (void)layoutScanOverlayGeometry
{
    if (!self.scanDimView || !self.scanDimMaskLayer || !self.scanHoleHighlightLayer || self.bracketCornerLayers.count != 4) {
        return;
    }
    const CGRect vb = self.scanDimView.bounds;
    if (vb.size.width < 32 || vb.size.height < 32) {
        return;
    }

    CGFloat sqSz = floor(MIN(vb.size.width, vb.size.height) * 0.72);
    CGFloat sqX = (vb.size.width - sqSz) / 2.0;
    CGFloat sqY = (vb.size.height - sqSz) / 2.0;

    CGFloat headerBottom = CGRectGetMaxY(self.headerContainer.frame);
    if (headerBottom < 8.0) {
        headerBottom = 132.0 + self.view.safeAreaInsets.top;
    }
    sqY = MAX(sqY, headerBottom + 8.0);

    const CGFloat kBottomBandForTorch = 80.0;
    const CGFloat maxHoleBottom = vb.size.height - kBottomBandForTorch;
    if (sqY + sqSz > maxHoleBottom) {
        sqY = maxHoleBottom - sqSz;
        sqY = MAX(sqY, headerBottom + 8.0);
    }

    sqX = MAX(8.0, MIN(sqX, vb.size.width - sqSz - 8.0));
    sqY = MAX(headerBottom + 4.0, MIN(sqY, vb.size.height - sqSz - 8.0));

    const CGRect hole = CGRectMake(sqX, sqY, sqSz, sqSz);
    CGFloat holeR = MIN(28.0, MAX(10.0, sqSz * 0.056));
    {
        const CGFloat half = 0.5 * MIN(hole.size.width, hole.size.height);
        holeR = MIN(holeR, MAX(6.0, half - 2.0));
    }
    UIBezierPath *holeRoundPath = [UIBezierPath bezierPathWithRoundedRect:hole cornerRadius:holeR];

    UIBezierPath *path = [UIBezierPath bezierPathWithRect:vb];
    [path appendPath:holeRoundPath];
    self.scanDimMaskLayer.frame = vb;
    self.scanDimMaskLayer.path = path.CGPath;

    self.scanHoleHighlightLayer.frame = CGRectMake(0, 0, vb.size.width, vb.size.height);
    self.scanHoleHighlightLayer.path = holeRoundPath.CGPath;

    const CGFloat bracketThick = 5.0;
    const CGFloat bracketLen = (CGFloat)MAX(28, (NSInteger)floor(sqSz * 0.13));
    const CGFloat x0 = hole.origin.x;
    const CGFloat y0 = hole.origin.y;
    const CGFloat s = hole.size.width;

    const CGFloat t = bracketThick;
    const CGFloat L = bracketLen;

    for (NSUInteger i = 0; i < 4; i++) {
        CAShapeLayer *layer = self.bracketCornerLayers[i];
        layer.lineWidth = t;
        layer.path = amneziaScanBracketStrokePath((int)i, x0, y0, s, holeR, L, t).CGPath;
    }

    if (self.torchCenterYConstraint && self.torchButton) {
        const CGFloat holeBottom = CGRectGetMaxY(hole);
        const CGFloat bandBottom = vb.size.height;
        const CGFloat torchH = 56.0;
        CGFloat torchCenterY = (holeBottom + bandBottom) * 0.5;
        const CGFloat minC = holeBottom + torchH * 0.5 + 6.0;
        const CGFloat maxC = bandBottom - torchH * 0.5 - MAX(6.0, self.view.safeAreaInsets.bottom);
        torchCenterY = MAX(minC, MIN(maxC, torchCenterY));
        if (minC > maxC) {
            torchCenterY = (minC + maxC) * 0.5;
        }
        const CGFloat hdr = headerBottom + torchH * 0.5 + 10.0;
        torchCenterY = MAX(torchCenterY, hdr);
        self.torchCenterYConstraint.constant = torchCenterY;
    }

    [self applyMetadataRectOfInterestForScanHole:hole];
}

- (void)backTapped
{
    if (gOnBack) {
        gOnBack();
    }
}

- (void)torchTapped
{
    gTorchRequested = !gTorchRequested;
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
    [self layoutScanOverlayGeometry];
    if (self.scanHoleFillView) {
        [self.view bringSubviewToFront:self.scanHoleFillView];
    }
    if (self.scanDimView) {
        [self.view bringSubviewToFront:self.scanDimView];
    }
    if (self.bracketContainer) {
        [self.view bringSubviewToFront:self.bracketContainer];
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
    self.metadataOutput = nil;

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
    self.metadataOutput = meta;

    AVCaptureVideoPreviewLayer *preview = [[AVCaptureVideoPreviewLayer alloc] initWithSession:session];
    preview.videoGravity = AVLayerVideoGravityResizeAspectFill;
    self.previewLayer = preview;
    [self.cameraContainer.layer insertSublayer:preview atIndex:0];
    preview.frame = self.cameraContainer.bounds;

    [self.view layoutIfNeeded];
    [self layoutScanOverlayGeometry];

    AVCaptureSession *runningSession = session;
    __unsafe_unretained AmneziaPairingQrOverlayViewController *weakSelf = self;
    dispatch_async(q, ^{
        @try {
            [runningSession startRunning];
        } @catch (NSException *ex) {
            NSLog(@"[PairingQrOverlay] startRunning exception: %@", ex);
        }
        const BOOL running = [runningSession isRunning];
        dispatch_async(dispatch_get_main_queue(), ^{
            AmneziaPairingQrOverlayViewController *strongSelf = weakSelf;
            if (!strongSelf) {
                return;
            }
            AVCaptureVideoPreviewLayer *pl = strongSelf.previewLayer;
            [strongSelf applyTorchFromGlobalFlag];
        });
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.35 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
            AmneziaPairingQrOverlayViewController *strongSelf = weakSelf;
            if (!strongSelf) {
                return;
            }
            AVCaptureVideoPreviewLayer *pl = strongSelf.previewLayer;
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
                gOnScanned(copy.c_str());
            } else {
            }
        });
        break;
    }
}

@end

static void amneziaPairingQrOverlayTeardownOnMain(void)
{
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
        [restore makeKeyWindow];
    } else {
    }
}

void amneziaIosPairingQrOverlayPresent(AmneziaPairingQrScannedUtf8Handler onScanned, AmneziaPairingQrOverlayBackHandler onBack,
                                       const std::string &titleUtf8, const std::string &subtitleUtf8)
{
    const bool hasScan = static_cast<bool>(onScanned);
    const bool hasBack = static_cast<bool>(onBack);
    AmneziaPairingQrScannedUtf8Handler scanH = std::move(onScanned);
    AmneziaPairingQrOverlayBackHandler backH = std::move(onBack);
    const std::string titleCopy = titleUtf8;
    const std::string subCopy = subtitleUtf8;

    dispatch_async(dispatch_get_main_queue(), ^{

        amneziaPairingQrOverlayTeardownOnMain();
        gOnScanned = std::move(scanH);
        gOnBack = std::move(backH);

        UIWindowScene *scene = amneziaForegroundWindowScene();
        if (!scene) {
            NSLog(@"[PairingQrOverlay] present: no UIWindowScene");
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

        [w makeKeyAndVisible];
        [w layoutIfNeeded];
        [vc.view setNeedsLayout];
        [vc.view layoutIfNeeded];

        gPairingQrOverlayKeySince = CFAbsoluteTimeGetCurrent();

        if (![vc startCapturePipelineOnMainThread]) {
            NSLog(@"[PairingQrOverlay] startCapture failed");
        }
    });
}

void amneziaIosPairingQrOverlayDismiss()
{
    dispatch_async(dispatch_get_main_queue(), ^{
        amneziaPairingQrOverlayTeardownOnMain();
    });
}

void amneziaIosPairingQrOverlaySetTorchEnabled(bool on)
{
    gTorchRequested = on;
    dispatch_async(dispatch_get_main_queue(), ^{
        UIWindow *win = gPairingQrOverlayWindow;
        if (!win) {
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
    dispatch_async(dispatch_get_main_queue(), ^{
        const CFAbsoluteTime now = CFAbsoluteTimeGetCurrent();
        if (gPairingQrOverlayKeySince > 0 && (now - gPairingQrOverlayKeySince) < 1.0) {
            return;
        }
        UIWindow *w = gPairingQrOverlayWindow;
        if (!w) {
            return;
        }
        UIViewController *root = w.rootViewController;
        if (![root isKindOfClass:[AmneziaPairingQrOverlayViewController class]]) {
            return;
        }
        AmneziaPairingQrOverlayViewController *vc = (AmneziaPairingQrOverlayViewController *)root;
        [vc stopCapturePipelineOnMainThread];
        if (![vc startCapturePipelineOnMainThread]) {
            NSLog(@"[PairingQrOverlay] restart startCapture failed");
        }
    });
}
