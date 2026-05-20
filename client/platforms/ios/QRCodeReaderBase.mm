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

- (BOOL)startReadingOnMainThread {
    [self stopReadingOnMainThread];

    NSError *error = nil;

    AVCaptureDevice *captureDevice = [AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeVideo];
    if (!captureDevice) {
        return NO;
    }

    AVCaptureDeviceInput *deviceInput = [AVCaptureDeviceInput deviceInputWithDevice:captureDevice error:&error];

    if (!deviceInput) {
        return NO;
    }

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
    AVCaptureSession *session = self.captureSession;
    self.captureSession = nil;

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
                NSLog(@"Session stopRunning exception: %@", ex);
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

- (void)captureOutput:(AVCaptureOutput *)output didOutputMetadataObjects:(NSArray<__kindof AVMetadataObject *> *)metadataObjects fromConnection:(AVCaptureConnection *)connection {

    if (metadataObjects != nil && metadataObjects.count > 0) {
        AVMetadataMachineReadableCodeObject *metadataObject = [metadataObjects objectAtIndex:0];

        if ([[metadataObject type] isEqualToString: AVMetadataObjectTypeQRCode]) {
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
    [m_qrCodeReader setQrCodeReader: this];
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
void QRCodeReader::notifyCodeRead(const QString &) {}
#endif
