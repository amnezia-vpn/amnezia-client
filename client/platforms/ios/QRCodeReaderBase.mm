#if !MACOS_NE
#include "QRCodeReaderBase.h"

#import <UIKit/UIKit.h>
#import <AVFoundation/AVFoundation.h>

@interface QRCodeReaderImpl : UIViewController
@end

@interface QRCodeReaderImpl () <AVCaptureMetadataOutputObjectsDelegate>
@property (nonatomic) QRCodeReader* qrCodeReader;
@property (nonatomic, strong) AVCaptureSession *captureSession;
@property (nonatomic, strong) AVCaptureVideoPreviewLayer *videoPreviewPlayer;
@end


@implementation QRCodeReaderImpl

- (void)viewDidLoad {
    [super viewDidLoad];

    _captureSession = nil;
}

- (void)setQrCodeReader: (QRCodeReader*)value {
    _qrCodeReader = value;
}

- (BOOL)startReading {
    NSError *error;

    AVCaptureDevice *captureDevice = [AVCaptureDevice defaultDeviceWithMediaType: AVMediaTypeVideo];
    AVCaptureDeviceInput *deviceInput = [AVCaptureDeviceInput deviceInputWithDevice: captureDevice error: &error];

    if(!deviceInput) {
        NSLog(@"Error %@", error.localizedDescription);
        return NO;
    }

    _captureSession = [[AVCaptureSession alloc]init];
    [_captureSession addInput:deviceInput];

    AVCaptureMetadataOutput *capturedMetadataOutput = [[AVCaptureMetadataOutput alloc] init];
    [_captureSession addOutput:capturedMetadataOutput];

    dispatch_queue_t dispatchQueue;
    dispatchQueue = dispatch_queue_create("myQueue", NULL);
    [capturedMetadataOutput setMetadataObjectsDelegate: self queue: dispatchQueue];
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

    [_captureSession startRunning];

    return YES;
}

- (void)stopReading {
    [_captureSession stopRunning];
    _captureSession = nil;

    [_videoPreviewPlayer removeFromSuperlayer];
}

- (void)captureOutput:(AVCaptureOutput *)output didOutputMetadataObjects:(NSArray<__kindof AVMetadataObject *> *)metadataObjects fromConnection:(AVCaptureConnection *)connection {

    if (metadataObjects != nil && metadataObjects.count > 0) {
        AVMetadataMachineReadableCodeObject *metadataObject = [metadataObjects objectAtIndex:0];

        if ([[metadataObject type] isEqualToString: AVMetadataObjectTypeQRCode]) {
            _qrCodeReader->emit codeReaded([metadataObject stringValue].UTF8String);
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
#endif
