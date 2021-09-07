#ifndef PAGES_H
#define PAGES_H

#include <QObject>
#include <QQmlEngine>

namespace PageEnumNS
{
Q_NAMESPACE
enum Page {Start = 0, NewServer, NewServerProtocols, Vpn,
           Wizard, WizardLow, WizardMedium, WizardHigh, WizardVpnMode, ServerConfiguring,
           GeneralSettings, AppSettings, NetworkSettings, ServerSettings,
           ServerContainers, ServersList, ShareConnection,  Sites,
           OpenVpnSettings, ShadowSocksSettings, CloakSettings};
Q_ENUM_NS(Page)

static void declareQML() {
    qmlRegisterUncreatableMetaObject(
                PageEnumNS::staticMetaObject,
                "PageEnum",
                1, 0,
                "PageEnum",
                "Error: only enums"
                );
}

} // PAGES_H

#endif
