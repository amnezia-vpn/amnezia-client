#ifndef TRAYICONCOMMON_H
#define TRAYICONCOMMON_H

#include <QByteArray>
#include <QIcon>
#include <QPixmap>
#include <QString>

#include "core/protocols/vpnProtocol.h"

namespace TrayIconCommon
{
    constexpr int kDefaultTrayIconSize = 128;

    constexpr char kIconOffBlack[] = ":/images/tray/off-black.svg";
    constexpr char kIconOffLight[] = ":/images/tray/off-light.svg";
    constexpr char kIconOnBlack[] = ":/images/tray/on-black.svg";
    constexpr char kIconOnWhite[] = ":/images/tray/on-white.svg";
    constexpr char kIconError[] = ":/images/tray/error.svg";

    QString resourcePathForState(Vpn::ConnectionState state, bool darkTheme);

    QPixmap renderIcon(const QString &resourcePath, int size);

    QPixmap buildPixmap(int size, Vpn::ConnectionState state, bool darkTheme);
    QIcon buildIcon(Vpn::ConnectionState state, bool darkTheme);
    QByteArray buildTemplatePng(Vpn::ConnectionState state);

} // namespace TrayIconCommon

#endif // TRAYICONCOMMON_H
