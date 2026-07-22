#ifndef PAGE_ENUM_H
#define PAGE_ENUM_H

#include <QObject>
#include <QQmlEngine>

namespace PageLoader
{
    Q_NAMESPACE
    enum class PageEnum {
        PageStart = 0,
        PageHome,
        PageShare,
        PageDeinstalling,
        PageAbout,

        PageSettingsServersList,
        PageSettings,
        PageSettingsServerData,
        PageSettingsServerInfo,
        PageSettingsServerProtocols,
        PageSettingsServerServices,
        PageSettingsServerProtocol,
        PageSettingsConnection,
        PageSettingsDns,
        PageSettingsApplication,
        PageSettingsNewsNotifications,
        PageSettingsNewsDetail,
        PageSettingsBackup,
        PageSettingsAbout,
        PageSettingsLogging,
        PageSettingsSplitTunneling,
        PageSettingsAppSplitTunneling,
        PageSettingsKillSwitch,
        PageSettingsApiServerInfo,
        PageSettingsApiAvailableCountries,
        PageSettingsApiSupport,
        PageSettingsApiInstructions,
        PageSettingsApiNativeConfigs,
        PageSettingsApiDevices,
        PageSettingsApiSubscriptionKey,
        PageSettingsKillSwitchExceptions,

        PageServiceSftpSettings,
        PageServiceTorWebsiteSettings,
        PageServiceDnsSettings,
        PageServiceSocksProxySettings,
        PageServiceMtProxySettings,
        PageServiceTelemtSettings,

        PageSetupWizardStart,
        PageSetupWizardCredentials,
        PageSetupWizardProtocols,
        PageSetupWizardEasy,
        PageSetupWizardProtocolSettings,
        PageSetupWizardInstalling,
        PageSetupWizardConfigSource,
        PageSetupWizardTextKey,
        PageSetupWizardViewConfig,
        PageSetupWizardQrReader,
        PageSetupWizardApiServicesList,
        PageSetupWizardApiFreeInfo,

        PageProtocolOpenVpnSettings,
        PageProtocolXraySettings,
        PageProtocolWireGuardSettings,
        PageProtocolAwgSettings,
        PageProtocolIKev2Settings,
        PageProtocolRaw,

        PageProtocolWireGuardClientSettings,
        PageProtocolAwgClientSettings,

        PageShareFullAccess,
        PageShareConnection,

        PageSetupWizardApiPremiumInfo,
        PageSetupWizardApiTrialEmail,

        PageDevMenu,

        PageProtocolXraySnapshots,
        PageProtocolXrayTransportSettings,
        PageProtocolXrayXmuxSettings,
        PageProtocolXrayXPaddingSettings,
        PageProtocolXrayFlowSettings,
        PageProtocolXraySecuritySettings,
        PageProtocolXrayXPaddingBytesSettings,

        PageSettingsLanguage,
    };
    Q_ENUM_NS(PageEnum)

    static void declareQmlPageEnum()
    {
        qmlRegisterUncreatableMetaObject(PageLoader::staticMetaObject, "PageEnum", 1, 0, "PageEnum", "Error: only enums");
    }
}

#endif // PAGE_ENUM_H
