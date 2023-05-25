#ifndef PAGES_H
#define PAGES_H

#include <QObject>
#include <QQmlEngine>

class PageType : public QObject
{
    Q_GADGET

public:
    enum Type {
        Basic,
        Proto,
        ShareProto,
        ClientInfo
    };
    Q_ENUM(Type)
};

namespace PageEnumNS
{
Q_NAMESPACE
enum class Page { Start = 0, NewServer, NewServerProtocols, Vpn,
                  Wizard, WizardLow, WizardMedium, WizardHigh, WizardVpnMode, ServerConfiguringProgress,
                  GeneralSettings, AppSettings, NetworkSettings, ServerSettings,
                  ServerContainers, ServersList, ShareConnection,  Sites,
                  ProtocolSettings, ProtocolShare, QrDecoder, QrDecoderIos, About, ViewConfig,
                  AdvancedServerSettings, ClientManagement, ClientInfo,

                  PageSetupWizardStart, PageTest, PageSetupWizardCredentials, PageSetupWizardProtocols, PageSetupWizardEasy,
                  PageSetupWizardProtocolSettings, PageSetupWizardInstalling, PageSetupWizardConfigSource, PageSetupWizardTextKey,

                  PageSettings, PageSettingsServersList,

                  PageStart, PageHome, PageShare};
Q_ENUM_NS(Page)

static void declareQmlPageEnum() {
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
