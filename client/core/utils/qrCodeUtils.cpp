#include "qrCodeUtils.h"

#include <QIODevice>
#include <QList>

QList<QString> qrCodeUtils::generateQrCodeImageSeriesPlainText(const QByteArray &utf8Text)
{
    const QString text = QString::fromUtf8(utf8Text);
    qrcodegen::QrCode qr = qrcodegen::QrCode::encodeText(text.toUtf8().constData(), qrcodegen::QrCode::Ecc::LOW);
    const QString svg = QString::fromStdString(toSvgString(qr, 1));
    return { svgToBase64(svg) };
}

QList<QString> qrCodeUtils::generateQrCodeImageSeries(const QByteArray &data)
{
    double k = 850;

    quint8 chunksCount = std::ceil(data.size() / k);
    QList<QString> chunks;
    for (int i = 0; i < data.size(); i = i + k) {
        QByteArray chunk;
        QDataStream s(&chunk, QIODevice::WriteOnly);
        s << qrCodeUtils::qrMagicCode << chunksCount << (quint8)std::round(i / k) << data.mid(i, k);

        QByteArray ba = chunk.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);

        qrcodegen::QrCode qr = qrcodegen::QrCode::encodeText(ba, qrcodegen::QrCode::Ecc::LOW);
        QString svg = QString::fromStdString(toSvgString(qr, 1));
        chunks.append(svgToBase64(svg));
    }

    return chunks;
}

QString qrCodeUtils::generatePlainQrCodeImage(const QByteArray &data)
{
    qrcodegen::QrCode qr = qrcodegen::QrCode::encodeText(data, qrcodegen::QrCode::Ecc::LOW);
    QString svg = QString::fromStdString(toSvgString(qr, 1));
    return svgToBase64(svg);
}

QString qrCodeUtils::svgToBase64(const QString &image)
{
    return "data:image/svg;base64," + QString::fromLatin1(image.toUtf8().toBase64().data());
}

qrcodegen::QrCode qrCodeUtils::generateQrCode(const QByteArray &data)
{
    return qrcodegen::QrCode::encodeText(data, qrcodegen::QrCode::Ecc::LOW);
}
