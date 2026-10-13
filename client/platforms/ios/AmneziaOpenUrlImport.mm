#import "AmneziaOpenUrlImport.h"

#include "ios_controller.h"

#include <QFile>
#include <QString>

#include <dispatch/dispatch.h>

void AmneziaHandleOpenUrl(NSURL *url)
{
    if (!url) {
        return;
    }

    NSString *scheme = url.scheme ? [url.scheme lowercaseString] : @"";
    if ([scheme isEqualToString:@"vpn"]) {
        NSString *absolute = url.absoluteString;
        if (absolute.length == 0) {
            return;
        }
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(1 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
            IosController::Instance()->importConfigFromOutside(QString::fromUtf8([absolute UTF8String]));
        });
        return;
    }

    if (!url.isFileURL) {
        return;
    }

    QString filePath = QString::fromUtf8([url.path UTF8String]);
    if (filePath.isEmpty()) {
        return;
    }

    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(1 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
        if (filePath.contains(QLatin1String("backup"))) {
            IosController::Instance()->importBackupFromOutside(filePath);
            return;
        }

        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly)) {
            return;
        }

        const QByteArray data = file.readAll();
        IosController::Instance()->importConfigFromOutside(QString::fromUtf8(data));
    });
}
