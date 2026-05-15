#ifndef PAGECONTROLLER_H
#define PAGECONTROLLER_H

#include <QObject>
#include <QQmlEngine>

#include "core/utils/errorCodes.h"
#include "core/utils/routeModes.h"
#include "core/utils/commonStructs.h"
#include "core/controllers/settingsController.h"
#include "core/controllers/serversController.h"

namespace PageLoader
{
    Q_NAMESPACE
    enum class PageEnum {
        PageStart = 0,
        PageHome,
        PageShare,
        PageDeinstalling,

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

        PageDevMenu
    };
    Q_ENUM_NS(PageEnum)

    static void declareQmlPageEnum()
    {
        qmlRegisterUncreatableMetaObject(PageLoader::staticMetaObject, "PageEnum", 1, 0, "PageEnum", "Error: only enums");
    }
}

class PageController : public QObject
{
    Q_OBJECT
public:
    explicit PageController(ServersController* serversController, SettingsController* settingsController,
                            QObject *parent = nullptr);

    Q_PROPERTY(int safeAreaTopMargin READ getSafeAreaTopMargin NOTIFY safeAreaTopMarginChanged)
    Q_PROPERTY(int safeAreaBottomMargin READ getSafeAreaBottomMargin NOTIFY safeAreaBottomMarginChanged)
    Q_PROPERTY(int imeHeight READ getImeHeight NOTIFY imeHeightChanged)

public slots:
    bool isStartPageVisible();
    QString getPagePath(PageLoader::PageEnum page);

    void closeWindow();
    void hideWindow();
    void keyPressEvent(Qt::Key key);

    unsigned int getInitialPageNavigationBarColor();
    void updateNavigationBarColor(const int color);

    void showOnStartup();

    bool isTriggeredByConnectButton();
    void setTriggeredByConnectButton(bool trigger);

    void closeApplication();

    void setDrawerDepth(const int depth);
    int getDrawerDepth() const;
    int incrementDrawerDepth();
    int decrementDrawerDepth();

    bool isEdgeToEdgeEnabled();
    int getStatusBarHeight();
    int getNavigationBarHeight();
    int getSafeAreaTopMargin();
    int getSafeAreaBottomMargin();
    int getImeHeight();

private slots:
    void onShowErrorMessage(amnezia::ErrorCode errorCode);

signals:
    void goToPage(PageLoader::PageEnum page, bool slide = true);
    void goToStartPage();
    void goToPageHome();
    void goToPageSettings();
    void goToPageViewConfig();
    void goToPageSettingsServerServices();
    void goToPageSettingsBackup();
    void goToShareConnectionPage(QString headerText, QString configContentHeaderText, QString configCaption, QString configExtension,
                                 QString configFileName);

    void closePage();

    void restorePageHomeState(bool isContainerInstalled = false);

    void showErrorMessage(amnezia::ErrorCode);
    void showErrorMessage(const QString &errorMessage);
    void showNotificationMessage(const QString &message);

    void showBusyIndicator(bool visible);
    void disableControls(bool disabled);
    void disableTabBar(bool disabled);

    void hideMainWindow();
    void raiseMainWindow();

    void showPassphraseRequestDrawer();
    void passphraseRequestDrawerClosed(QString passphrase);

    void unsupportedConnectDrawerRequested();

    void escapePressed();
    void closeTopDrawer();

    void showChangelogDrawer();
    void imeHeightChanged(int height);
    void safeAreaTopMarginChanged();
    void safeAreaBottomMarginChanged();

private:
    ServersController* m_serversController;
    SettingsController* m_settingsController;

    bool m_isTriggeredByConnectButton;

    int m_drawerDepth = 0;

    mutable int m_cachedStatusBarHeight = -1;
    mutable int m_cachedNavigationBarHeight = -1;
    mutable bool m_cachedEdgeToEdgeEnabled = false;
    mutable bool m_edgeToEdgeCached = false;
    int m_imeHeight = 0;
};

#endif
