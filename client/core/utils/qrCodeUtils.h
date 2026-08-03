#ifndef QRCODEUTILS_H
#define QRCODEUTILS_H

#include <QString>

#include "qrcodegen.hpp"

namespace qrCodeUtils
{
    constexpr const qint16 qrMagicCode = 1984;

    QList<QString> generateQrCodeImageSeries(const QByteArray &data);
    // Single plain QR of the raw bytes — no Amnezia chunk/magic envelope. Use for external links
    // (e.g. tg://proxy) that must be scannable by third-party apps / the camera, not the Amnezia app.
    QString generatePlainQrCodeImage(const QByteArray &data);
    qrcodegen::QrCode generateQrCode(const QByteArray &data);
    QString svgToBase64(const QString &image);
};

#endif // QRCODEUTILS_H
