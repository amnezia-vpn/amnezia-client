#ifndef QRCODEUTILS_H
#define QRCODEUTILS_H

#include <QString>

#include "qrcodegen.hpp"

namespace qrCodeUtils
{
    constexpr const qint16 qrMagicCode = 1984;

    QList<QString> generateQrCodeImageSeries(const QByteArray &data);
    QList<QString> generateQrCodeImageSeriesPlainText(const QByteArray &utf8Text);
    qrcodegen::QrCode generateQrCode(const QByteArray &data);
    QString svgToBase64(const QString &image);
};

#endif // QRCODEUTILS_H
