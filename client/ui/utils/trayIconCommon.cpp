#include "trayIconCommon.h"

#include <QBuffer>
#include <QDebug>
#include <QPainter>
#include <QSvgRenderer>

namespace TrayIconCommon
{
QString resourcePathForState(Vpn::ConnectionState state, bool darkTheme)
{
    switch (state) {
    case Vpn::ConnectionState::Error:
        return QString::fromLatin1(kIconError);
    case Vpn::ConnectionState::Connected:
        return QString::fromLatin1(darkTheme ? kIconOnWhite : kIconOnBlack);
    case Vpn::ConnectionState::Disconnected:
    case Vpn::ConnectionState::Preparing:
    case Vpn::ConnectionState::Connecting:
    case Vpn::ConnectionState::Disconnecting:
    case Vpn::ConnectionState::Reconnecting:
    case Vpn::ConnectionState::Unknown:
    default:
        return QString::fromLatin1(darkTheme ? kIconOffLight : kIconOffBlack);
    }
}

QPixmap renderIcon(const QString &resourcePath, int size)
{
    QSvgRenderer renderer(resourcePath);
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);

    if (!renderer.isValid()) {
        qWarning() << "Failed to load tray icon:" << resourcePath;
        return pixmap;
    }

    QPainter painter(&pixmap);
    renderer.render(&painter, QRectF(0, 0, size, size));
    return pixmap;
}

QPixmap buildPixmap(int size, Vpn::ConnectionState state, bool darkTheme)
{
    return renderIcon(resourcePathForState(state, darkTheme), size);
}

QIcon buildIcon(Vpn::ConnectionState state, bool darkTheme)
{
    QIcon icon;
    icon.addPixmap(buildPixmap(kDefaultTrayIconSize, state, darkTheme));
    return icon;
}

QByteArray buildTemplatePng(Vpn::ConnectionState state)
{
    const QPixmap pixmap = renderIcon(resourcePathForState(state, /*darkTheme*/ true), kDefaultTrayIconSize);

    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);
    pixmap.save(&buffer, "PNG");
    return bytes;
}

} // namespace TrayIconCommon
