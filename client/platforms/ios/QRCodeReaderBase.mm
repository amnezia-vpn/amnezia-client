#if !MACOS_NE
#include "QRCodeReaderBase.h"

#include <QByteArray>
#include <QDebug>

#import <UIKit/UIKit.h>
#import <AVFoundation/AVFoundation.h>

@interface QRCodeReaderImpl : UIViewController
@end

@interface QRCodeReaderImpl () <AVCaptureMetadataOutputObjectsDelegate>
@property (nonatomic) QRCodeReader* qrCodeReader;
@property (nonatomic, strong) AVCaptureSession *captureSession;
@property (nonatomic, strong) AVCaptureVideoPreviewLayer *videoPreviewPlayer;
@property (nonatomic) dispatch_queue_t sessionQueue;
@end


@implementation QRCodeReaderImpl

- (void)viewDidLoad {
    [super viewDidLoad];

    _captureSession = nil;
    if (!_sessionQueue) {
        _sessionQueue = dispatch_queue_create("org.amnezia.qr.session", DISPATCH_QUEUE_SERIAL);
    }
}

- (void)setQrCodeReader: (QRCodeReader*)value {
    _qrCodeReader = value;
}

- (BOOL)startReading {
    [self stopReading];

    NSError *error = nil;

    AVCaptureDevice *captureDevice = [AVCaptureDevice defaultDeviceWithMediaType: AVMediaTypeVideo];
    AVCaptureDeviceInput *deviceInput = [AVCaptureDeviceInput deviceInputWithDevice: captureDevice error: &error];

    if (!deviceInput) {
        NSLog(@"[QRCodeReader] deviceInput failed: %@", error.localizedDescription);
        return NO;
    }

    _captureSession = [[AVCaptureSession alloc]init];
    [_captureSession addInput:deviceInput];

    AVCaptureMetadataOutput *capturedMetadataOutput = [[AVCaptureMetadataOutput alloc] init];
    [_captureSession addOutput:capturedMetadataOutput];

    if (!_sessionQueue) {
        _sessionQueue = dispatch_queue_create("org.amnezia.qr.session", DISPATCH_QUEUE_SERIAL);
    }
    [capturedMetadataOutput setMetadataObjectsDelegate: self queue: _sessionQueue];
    [capturedMetadataOutput setMetadataObjectTypes: [NSArray arrayWithObject:AVMetadataObjectTypeQRCode]];

    _videoPreviewPlayer = [[AVCaptureVideoPreviewLayer alloc] initWithSession: _captureSession];
    
    CGFloat statusBarHeight = [UIApplication sharedApplication].statusBarFrame.size.height;

    QRect cameraRect = _qrCodeReader->cameraSize();
    CGRect cameraCGRect = CGRectMake(cameraRect.x(),
                                     cameraRect.y() + statusBarHeight,
                                     cameraRect.width(),
                                     cameraRect.height());

    [_videoPreviewPlayer setVideoGravity: AVLayerVideoGravityResizeAspectFill];
    [_videoPreviewPlayer setFrame: cameraCGRect];

    CALayer* layer = [UIApplication sharedApplication].keyWindow.layer;
    [layer addSublayer: _videoPreviewPlayer];

    AVCaptureSession *session = _captureSession;
    dispatch_async(_sessionQueue, ^{
        [session startRunning];
    });

    NSLog(@"[QRCodeReader] startReading OK frame=(%.1f,%.1f,%.1f,%.1f) statusBar=%.1f",
          cameraCGRect.origin.x, cameraCGRect.origin.y, cameraCGRect.size.width, cameraCGRect.size.height, statusBarHeight);

    return YES;
}

- (void)stopReading {
    if (_captureSession) {
        AVCaptureSession *session = _captureSession;
        dispatch_async(_sessionQueue, ^{
            [session stopRunning];
        });
        _captureSession = nil;
    }
    if (_videoPreviewPlayer) {
        [_videoPreviewPlayer removeFromSuperlayer];
        _videoPreviewPlayer = nil;
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
    [m_qrCodeReader setQrCodeReader: this];
}

QRect QRCodeReader::cameraSize() {
    return m_cameraSize;
}

void QRCodeReader::setCameraSize(QRect value) {
    m_cameraSize = value;
    qInfo() << "[QRCodeReader] setCameraSize" << value;
}

void QRCodeReader::startReading() {
    const BOOL ok = [m_qrCodeReader startReading];
    if (!ok) {
        qWarning() << "[QRCodeReader] startReading failed (see NSLogs)";
    }
}

void QRCodeReader::stopReading() {
    [m_qrCodeReader stopReading];
    qInfo() << "[QRCodeReader] stopReading";
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
