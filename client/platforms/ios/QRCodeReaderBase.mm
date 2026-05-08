#if !MACOS_NE
#include "QRCodeReaderBase.h"

#include "platforms/ios/iosPairingCameraAccess.h"

#include <QByteArray>
#include <QDebug>
#include <QThread>

#import <UIKit/UIKit.h>
#import <AVFoundation/AVFoundation.h>

static NSString *amneziaQrThreadTag(void)
{
    if ([NSThread isMainThread]) {
        return @"main";
    }
    return [NSString stringWithFormat:@"bg:%p", (void *)[NSThread currentThread]];
}

static void amneziaQrLogDeviceAuth(void)
{
    AVAuthorizationStatus st = [AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeVideo];
    NSString *stName = @"unknown";
    switch (st) {
        case AVAuthorizationStatusNotDetermined:
            stName = @"notDetermined";
            break;
        case AVAuthorizationStatusRestricted:
            stName = @"restricted";
            break;
        case AVAuthorizationStatusDenied:
            stName = @"denied";
            break;
        case AVAuthorizationStatusAuthorized:
            stName = @"authorized";
            break;
        default:
            break;
    }
    NSLog(@"[QRCodeReader] camera auth status=%@ (%ld)", stName, (long)st);
}

static UIWindow *amneziaKeyWindowForQrCamera(void)
{
    UIApplication *app = [UIApplication sharedApplication];
    NSMutableArray<NSString *> *trace = [NSMutableArray array];

    if (@available(iOS 13.0, *)) {
        NSInteger sceneCount = app.connectedScenes.count;
        [trace addObject:[NSString stringWithFormat:@"connectedScenes=%ld", (long)sceneCount]];
        for (UIScene *scene in app.connectedScenes) {
            if (scene.activationState != UISceneActivationStateForegroundActive) {
                continue;
            }
            if (![scene isKindOfClass:[UIWindowScene class]]) {
                continue;
            }
            UIWindowScene *windowScene = (UIWindowScene *)scene;
            NSInteger winN = windowScene.windows.count;
            [trace addObject:[NSString stringWithFormat:@"foreground UIWindowScene windows=%ld", (long)winN]];
            for (UIWindow *window in windowScene.windows) {
                if (window.isKeyWindow) {
                    NSLog(@"[QRCodeReader] keyWindow pick: scene keyWindow=%@ bounds=%@", window, NSStringFromCGRect(window.bounds));
                    return window;
                }
            }
            for (UIWindow *window in windowScene.windows) {
                if (!window.isHidden) {
                    NSLog(@"[QRCodeReader] keyWindow pick: scene nonHidden=%@ bounds=%@", window, NSStringFromCGRect(window.bounds));
                    return window;
                }
            }
        }
    }

    if (app.keyWindow) {
        [trace addObject:@"app.keyWindow"];
        NSLog(@"[QRCodeReader] keyWindow pick: application.keyWindow=%@ bounds=%@",
              app.keyWindow, NSStringFromCGRect(app.keyWindow.bounds));
        return app.keyWindow;
    }
    for (UIWindow *window in app.windows) {
        if (window.isKeyWindow) {
            NSLog(@"[QRCodeReader] keyWindow pick: windows scan key=%@ bounds=%@", window, NSStringFromCGRect(window.bounds));
            return window;
        }
    }
    UIWindow *first = app.windows.firstObject;
    if (first) {
        NSLog(@"[QRCodeReader] keyWindow pick: firstObject=%@ bounds=%@ trace=[%@]",
              first, NSStringFromCGRect(first.bounds), [trace componentsJoinedByString:@", "]);
        return first;
    }
    NSLog(@"[QRCodeReader] keyWindow pick: NONE trace=[%@]", [trace componentsJoinedByString:@", "]);
    return nil;
}

@interface QRCodeReaderImpl : UIViewController
@end

@interface QRCodeReaderImpl () <AVCaptureMetadataOutputObjectsDelegate>
@property (nonatomic, assign) QRCodeReader *qrCodeReader;
@property (nonatomic, retain) AVCaptureSession *captureSession;
@property (nonatomic, retain) AVCaptureVideoPreviewLayer *videoPreviewPlayer;
@property (nonatomic, retain) AVCaptureDevice *activeCaptureDevice;
@property (nonatomic) dispatch_queue_t sessionQueue;
@end


@implementation QRCodeReaderImpl

- (void)viewDidLoad {
    [super viewDidLoad];

    self.captureSession = nil;
    if (!_sessionQueue) {
        _sessionQueue = dispatch_queue_create("org.amnezia.qr.session", DISPATCH_QUEUE_SERIAL);
    }
}

- (void)setQrCodeReader:(QRCodeReader *)value {
    _qrCodeReader = value;
}

- (AVCaptureDevice *)resolvedCaptureDevice {
    if (self.activeCaptureDevice) {
        return self.activeCaptureDevice;
    }
    AVCaptureSession *session = self.captureSession;
    if (!session) {
        return nil;
    }
    for (AVCaptureInput *input in session.inputs) {
        if ([input isKindOfClass:[AVCaptureDeviceInput class]]) {
            AVCaptureDevice *d = ((AVCaptureDeviceInput *)input).device;
            if (d) {
                NSLog(@"[QRCodeReader] resolvedCaptureDevice from session input device=%p", d);
                return d;
            }
        }
    }
    return nil;
}

- (void)applyTorchOnMainThread:(BOOL)on {
    AVCaptureDevice *device = [self resolvedCaptureDevice];
    if (!device) {
        if (on) {
            NSLog(@"[QRCodeReader] torch ON failed: no device (active=%p session=%p inputs=%lu)",
                  self.activeCaptureDevice,
                  self.captureSession,
                  (unsigned long)(self.captureSession ? self.captureSession.inputs.count : 0));
        }
        return;
    }
    if (![device hasTorch]) {
        NSLog(@"[QRCodeReader] torch: device %p has no torch", device);
        return;
    }

    AVCaptureSession *session = self.captureSession;
    if (on && session && ![session isRunning]) {
        NSLog(@"[QRCodeReader] torch: session not running yet; retry in 0.25s (session=%p)", session);
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.25 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
            if (on) {
                [self applyTorchOnMainThread:YES];
            }
        });
        return;
    }

    NSError *err = nil;
    if (![device lockForConfiguration:&err]) {
        NSLog(@"[QRCodeReader] torch lock failed: %@", err.localizedDescription);
        return;
    }

    if (on) {
        err = nil;
        if (![device setTorchModeOnWithLevel:AVCaptureMaxAvailableTorchLevel error:&err]) {
            NSLog(@"[QRCodeReader] setTorchModeOnWithLevel failed: %@ — trying torchMode", err.localizedDescription);
            if ([device isTorchModeSupported:AVCaptureTorchModeOn]) {
                device.torchMode = AVCaptureTorchModeOn;
            }
        } else {
            NSLog(@"[QRCodeReader] torch ON ok level=maxAvailable");
        }
    } else {
        device.torchMode = AVCaptureTorchModeOff;
    }
    [device unlockForConfiguration];
}

- (void)applyTorch:(BOOL)on {
    NSLog(@"[QRCodeReader] applyTorch requested on=%d thread=%@", (int)on, amneziaQrThreadTag());
    if ([NSThread isMainThread]) {
        [self applyTorchOnMainThread:on];
    } else {
        dispatch_async(dispatch_get_main_queue(), ^{
            [self applyTorchOnMainThread:on];
        });
    }
}

- (BOOL)startReadingOnMainThread {
    NSLog(@"[QRCodeReader] startReadingOnMainThread begin thread=%@", amneziaQrThreadTag());
    amneziaQrLogDeviceAuth();

    [self stopReadingOnMainThread];

    NSError *error = nil;

    AVCaptureDevice *captureDevice = [AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeVideo];
    if (!captureDevice) {
        NSLog(@"[QRCodeReader] defaultDeviceWithMediaType:Video is nil");
        return NO;
    }
    NSLog(@"[QRCodeReader] capture device=%p localizedName=%@", captureDevice, captureDevice.localizedName);

    AVCaptureDeviceInput *deviceInput = [AVCaptureDeviceInput deviceInputWithDevice:captureDevice error:&error];

    if (!deviceInput) {
        NSLog(@"[QRCodeReader] deviceInput failed: %@", error.localizedDescription);
        return NO;
    }

    self.activeCaptureDevice = captureDevice;
    NSLog(@"[QRCodeReader] activeCaptureDevice set to %p", self.activeCaptureDevice);

    AVCaptureSession *session = [[AVCaptureSession alloc] init];
    [session addInput:deviceInput];

    AVCaptureMetadataOutput *capturedMetadataOutput = [[AVCaptureMetadataOutput alloc] init];
    [session addOutput:capturedMetadataOutput];

    if (!_sessionQueue) {
        _sessionQueue = dispatch_queue_create("org.amnezia.qr.session", DISPATCH_QUEUE_SERIAL);
    }
    [capturedMetadataOutput setMetadataObjectsDelegate:self queue:_sessionQueue];
    [capturedMetadataOutput setMetadataObjectTypes:[NSArray arrayWithObject:AVMetadataObjectTypeQRCode]];

    self.captureSession = session;
    [session release];

    AVCaptureVideoPreviewLayer *preview = [[AVCaptureVideoPreviewLayer alloc] initWithSession:self.captureSession];
    [preview setVideoGravity:AVLayerVideoGravityResizeAspectFill];
    self.videoPreviewPlayer = preview;
    [preview release];

    UIWindow *keyWindow = amneziaKeyWindowForQrCamera();
    if (!keyWindow) {
        NSLog(@"[QRCodeReader] startReading: no keyWindow (UIKit must run on main)");
        [self stopReadingOnMainThread];
        return NO;
    }

    CGRect bounds = keyWindow.bounds;
    [self.videoPreviewPlayer setFrame:bounds];
    self.videoPreviewPlayer.zPosition = -1000.f;
    [keyWindow.layer insertSublayer:self.videoPreviewPlayer atIndex:0];
    amneziaIosPairingRelayoutChromeIfNeeded();
    NSLog(@"[QRCodeReader] previewLayer inserted window=%@ layer.sublayers.count=%lu bounds=%@",
          keyWindow, (unsigned long)keyWindow.layer.sublayers.count, NSStringFromCGRect(bounds));

    AVCaptureSession *runningSession = self.captureSession;
    dispatch_async(_sessionQueue, ^{
        NSLog(@"[QRCodeReader] session startRunning on session queue session=%p", runningSession);
        [runningSession startRunning];
        dispatch_async(dispatch_get_main_queue(), ^{
            NSLog(@"[QRCodeReader] after startRunning isRunning=%d", (int)runningSession.isRunning);
        });
    });

    NSLog(@"[QRCodeReader] startReading OK activeDevice=%p window bounds=%@",
          self.activeCaptureDevice, NSStringFromCGRect(bounds));

    return YES;
}

- (BOOL)startReading {
    NSLog(@"[QRCodeReader] startReading entry thread=%@ qt=%p", amneziaQrThreadTag(), (void *)QThread::currentThread());
    if ([NSThread isMainThread]) {
        return [self startReadingOnMainThread];
    }
    __block BOOL ok = NO;
    dispatch_sync(dispatch_get_main_queue(), ^{
        ok = [self startReadingOnMainThread];
    });
    NSLog(@"[QRCodeReader] startReading exit ok=%d (dispatched to main)", (int)ok);
    return ok;
}

- (void)stopReadingOnMainThread {
    NSLog(@"[QRCodeReader] stopReadingOnMainThread thread=%@", amneziaQrThreadTag());
    [self applyTorchOnMainThread:NO];
    self.activeCaptureDevice = nil;

    AVCaptureSession *session = self.captureSession;
    self.captureSession = nil;

    /**
     * Must run stopRunning on the same serial queue as startRunning, synchronously before tearing down.
     * Async stop + immediate start (e.g. foreground resume calling restartPairingIosCamera) left stopRunning
     * racing startRunning's internal beginConfiguration/commitConfiguration → NSGenericException crash.
     */
    if (session) {
        if (!_sessionQueue) {
            _sessionQueue = dispatch_queue_create("org.amnezia.qr.session", DISPATCH_QUEUE_SERIAL);
        }
        dispatch_sync(_sessionQueue, ^{
            @try {
                if ([session isRunning]) {
                    NSLog(@"[QRCodeReader] session stopRunning (sync) session=%p", session);
                    [session stopRunning];
                }
            } @catch (NSException *ex) {
                NSLog(@"[QRCodeReader] session stopRunning exception: %@", ex);
            }
        });
    }

    if (self.videoPreviewPlayer) {
        NSLog(@"[QRCodeReader] remove preview from superlayer");
        [self.videoPreviewPlayer removeFromSuperlayer];
        self.videoPreviewPlayer = nil;
    }
}

- (void)stopReading {
    NSLog(@"[QRCodeReader] stopReading entry thread=%@ qt=%p", amneziaQrThreadTag(), (void *)QThread::currentThread());
    if ([NSThread isMainThread]) {
        [self stopReadingOnMainThread];
    } else {
        dispatch_sync(dispatch_get_main_queue(), ^{
            [self stopReadingOnMainThread];
        });
    }
    NSLog(@"[QRCodeReader] stopReading exit");
}

- (void)captureOutput:(AVCaptureOutput *)output
    didOutputMetadataObjects:(NSArray<__kindof AVMetadataObject *> *)metadataObjects
               fromConnection:(AVCaptureConnection *)connection {

    if (metadataObjects != nil && metadataObjects.count > 0) {
        AVMetadataMachineReadableCodeObject *metadataObject = [metadataObjects objectAtIndex:0];

        if ([[metadataObject type] isEqualToString:AVMetadataObjectTypeQRCode]) {
            NSString *value = [metadataObject stringValue];
            if (value.length == 0) {
                return;
            }
            NSLog(@"[QRCodeReader] metadata QR len=%lu", static_cast<unsigned long>(value.length));
            QRCodeReader *cpp = _qrCodeReader;
            const QByteArray utf8([value UTF8String]);
            dispatch_async(dispatch_get_main_queue(), ^{
                cpp->notifyCodeRead(QString::fromUtf8(utf8));
            });
        }
    }
}

@end

QRCodeReader::QRCodeReader() {
    m_qrCodeReader = [[QRCodeReaderImpl alloc] init];
    [m_qrCodeReader setQrCodeReader:this];
}

QRect QRCodeReader::cameraSize() {
    return m_cameraSize;
}

void QRCodeReader::setCameraSize(QRect value) {
    m_cameraSize = value;
    qInfo() << "[QRCodeReader] setCameraSize" << value;
}

void QRCodeReader::startReading() {
    qInfo() << "[QRCodeReader] C++ startReading thread" << QThread::currentThread();
    const BOOL ok = [m_qrCodeReader startReading];
    if (!ok) {
        qWarning() << "[QRCodeReader] C++ startReading failed (see NSLogs)";
    } else {
        qInfo() << "[QRCodeReader] C++ startReading ok";
    }
}

void QRCodeReader::stopReading() {
    qInfo() << "[QRCodeReader] C++ stopReading thread" << QThread::currentThread();
    [m_qrCodeReader stopReading];
}

void QRCodeReader::notifyCodeRead(const QString &code) {
    emit codeReaded(code);
}

void QRCodeReader::setTorchEnabled(bool on) {
    qInfo() << "[QRCodeReader] C++ setTorchEnabled" << on << "thread" << QThread::currentThread();
    [(QRCodeReaderImpl *)m_qrCodeReader applyTorch:on ? YES : NO];
}
#else
#include "QRCodeReaderBase.h"

QRCodeReader::QRCodeReader()
{

}

QRect QRCodeReader::cameraSize() {
    return QRect();
}

void QRCodeReader::startReading() {}
void QRCodeReader::stopReading() {}
void QRCodeReader::setCameraSize(QRect) {}
void QRCodeReader::setTorchEnabled(bool) {}
void QRCodeReader::notifyCodeRead(const QString &) {}
#endif
