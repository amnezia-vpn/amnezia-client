#include "trayIconCommon.h"

#include <QBuffer>
#include <QDebug>
#include <QPainter>
#include <QSvgRenderer>

namespace TrayIconCommon
{
qreal opacityForState(Vpn::ConnectionState state)
{
    switch (state) {
    case Vpn::ConnectionState::Connected:
    case Vpn::ConnectionState::Error: return kConnectedOpacity;
    case Vpn::ConnectionState::Disconnected:
    case Vpn::ConnectionState::Preparing:
    case Vpn::ConnectionState::Connecting:
    case Vpn::ConnectionState::Disconnecting:
    case Vpn::ConnectionState::Reconnecting:
    case Vpn::ConnectionState::Unknown:
    default: return kDisconnectedOpacity;
    }
}

QColor indicatorColorForState(Vpn::ConnectionState state)
{
    switch (state) {
    case Vpn::ConnectionState::Connected: return QColor(52, 199, 89);
    case Vpn::ConnectionState::Error: return QColor(235, 87, 87);
    default: return QColor();
    }
}

QPixmap renderTemplate(const QString &resourcePath, qreal opacity, int size)
{
    QSvgRenderer renderer(resourcePath);
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);

    if (!renderer.isValid()) {
        qWarning() << "Failed to load tray icon template:" << resourcePath;
        return pixmap;
    }

    QPainter painter(&pixmap);
    painter.setOpacity(opacity);
    renderer.render(&painter, QRectF(0, 0, size, size));
    return pixmap;
}

QPixmap colorizeTemplate(const QPixmap &mask, const QColor &foreground, int size)
{
    QPixmap result(size, size);
    result.fill(Qt::transparent);

    QPainter painter(&result);
    painter.fillRect(result.rect(), foreground);
    painter.setCompositionMode(QPainter::CompositionMode_DestinationIn);
    painter.drawPixmap(0, 0, mask);
    return result;
}

void drawStatusIndicator(QPainter &painter, const QColor &color, int size)
{
    const qreal dotSize = size * 0.35;
    const qreal dotOrigin = (size - dotSize) * 0.8;

    painter.setPen(Qt::NoPen);
    painter.setBrush(color);
    painter.drawEllipse(QRectF(dotOrigin, dotOrigin, dotSize, dotSize));
}

QPixmap buildPixmap(int size, qreal opacity, bool darkTheme, const QColor &indicatorColor)
{
    const QPixmap mask = renderTemplate(QString::fromLatin1(kTrayTemplateIconPath), opacity, size);
    const QColor foreground = darkTheme ? Qt::white : Qt::black;
    QPixmap pixmap = colorizeTemplate(mask, foreground, size);

    if (indicatorColor.isValid()) {
        QPainter painter(&pixmap);
        drawStatusIndicator(painter, indicatorColor, size);
    }

    return pixmap;
}

QIcon buildIcon(qreal opacity, bool darkTheme, const QColor &indicatorColor)
{
    QIcon icon;
    icon.addPixmap(buildPixmap(kDefaultTrayIconSize, opacity, darkTheme, indicatorColor));
    return icon;
}

QByteArray buildTemplatePng(qreal opacity)
{
    const QPixmap pixmap = renderTemplate(QString::fromLatin1(kTrayTemplateIconPath), opacity, kDefaultTrayIconSize);

    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);
    pixmap.save(&buffer, "PNG");
    return bytes;
}

} // namespace TrayIconCommon
