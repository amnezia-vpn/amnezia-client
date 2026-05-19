#if !MACOS_NE
#include "QRCodeReaderBase.h"

#include <QByteArray>

#import <UIKit/UIKit.h>
#import <AVFoundation/AVFoundation.h>

static UIWindow *amneziaKeyWindowForQrCamera(void)
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
                if (window.isKeyWindow) {
                    return window;
                }
            }
            for (UIWindow *window in windowScene.windows) {
                if (!window.isHidden) {
                    return window;
                }
            }
        }
    }

    if (app.keyWindow) {
        return app.keyWindow;
    }
    for (UIWindow *window in app.windows) {
        if (window.isKeyWindow) {
            return window;
        }
    }
    return app.windows.firstObject;
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
        return;
    }

    AVCaptureSession *session = self.captureSession;
    if (on && session && ![session isRunning]) {
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
        }
    } else {
        device.torchMode = AVCaptureTorchModeOff;
    }
    [device unlockForConfiguration];
}

- (void)applyTorch:(BOOL)on {
    if ([NSThread isMainThread]) {
        [self applyTorchOnMainThread:on];
    } else {
        dispatch_async(dispatch_get_main_queue(), ^{
            [self applyTorchOnMainThread:on];
        });
    }
}

- (BOOL)startReadingOnMainThread {
    [self stopReadingOnMainThread];

    NSError *error = nil;

    AVCaptureDevice *captureDevice = [AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeVideo];
    if (!captureDevice) {
        NSLog(@"[QRCodeReader] defaultDeviceWithMediaType:Video is nil");
        return NO;
    }

    AVCaptureDeviceInput *deviceInput = [AVCaptureDeviceInput deviceInputWithDevice:captureDevice error:&error];

    if (!deviceInput) {
        NSLog(@"[QRCodeReader] deviceInput failed: %@", error.localizedDescription);
        return NO;
    }

    self.activeCaptureDevice = captureDevice;

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

    AVCaptureSession *runningSession = self.captureSession;
    dispatch_async(_sessionQueue, ^{
        [runningSession startRunning];
    });

    return YES;
}

- (BOOL)startReading {
    if ([NSThread isMainThread]) {
        return [self startReadingOnMainThread];
    }
    __block BOOL ok = NO;
    dispatch_sync(dispatch_get_main_queue(), ^{
        ok = [self startReadingOnMainThread];
    });
    return ok;
}

- (void)stopReadingOnMainThread {
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
                    [session stopRunning];
                }
            } @catch (NSException *ex) {
                NSLog(@"[QRCodeReader] session stopRunning exception: %@", ex);
            }
        });
    }

    if (self.videoPreviewPlayer) {
        [self.videoPreviewPlayer removeFromSuperlayer];
        self.videoPreviewPlayer = nil;
    }
}

- (void)stopReading {
    if ([NSThread isMainThread]) {
        [self stopReadingOnMainThread];
    } else {
        dispatch_sync(dispatch_get_main_queue(), ^{
            [self stopReadingOnMainThread];
        });
    }
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
}

void QRCodeReader::startReading() {
    [m_qrCodeReader startReading];
}

void QRCodeReader::stopReading() {
    [m_qrCodeReader stopReading];
}

void QRCodeReader::notifyCodeRead(const QString &code) {
    emit codeReaded(code);
}

void QRCodeReader::setTorchEnabled(bool on) {
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
