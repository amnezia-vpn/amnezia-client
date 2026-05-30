#ifndef TRAYICONCOMMON_H
#define TRAYICONCOMMON_H

#include <QByteArray>
#include <QColor>
#include <QIcon>
#include <QPainter>
#include <QPixmap>

#include "core/protocols/vpnProtocol.h"

namespace TrayIconCommon {

constexpr int kDefaultTrayIconSize = 128;
constexpr char kTrayTemplateIconPath[] = ":/images/tray/icon.svg";

constexpr qreal kDisconnectedOpacity = 0.5;
constexpr qreal kConnectedOpacity = 1.0;

qreal opacityForState(Vpn::ConnectionState state);
QColor indicatorColorForState(Vpn::ConnectionState state);

QPixmap renderTemplate(const QString &resourcePath, qreal opacity, int size);
QPixmap colorizeTemplate(const QPixmap &mask, const QColor &foreground, int size);
void drawStatusIndicator(QPainter &painter, const QColor &color, int size);

QPixmap buildPixmap(int size, qreal opacity, bool darkTheme, const QColor &indicatorColor);
QIcon buildIcon(qreal opacity, bool darkTheme, const QColor &indicatorColor);
QByteArray buildTemplatePng(qreal opacity);

} // namespace TrayIconCommon

#endif // TRAYICONCOMMON_H
