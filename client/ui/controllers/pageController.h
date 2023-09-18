#ifndef PAGECONTROLLER_H
#define PAGECONTROLLER_H

#include <QObject>
#include <QQmlEngine>

#include "ui/models/servers_model.h"

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
        PageSettingsBackup,
        PageSettingsAbout,
        PageSettingsLogging,
        PageSettingsSplitTunneling,

        PageServiceSftpSettings,
        PageServiceTorWebsiteSettings,
        PageServiceDnsSettings,

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

        PageProtocolOpenVpnSettings,
        PageProtocolShadowSocksSettings,
        PageProtocolCloakSettings,
        PageProtocolWireGuardSettings,
        PageProtocolIKev2Settings,
        PageProtocolRaw
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
    explicit PageController(const QSharedPointer<ServersModel> &serversModel, const std::shared_ptr<Settings> &settings,
                            QObject *parent = nullptr);

public slots:
    QString getInitialPage();
    QString getPagePath(PageLoader::PageEnum page);

    void closeWindow();
    void keyPressEvent(Qt::Key key);

    unsigned int getInitialPageNavigationBarColor();
    void updateNavigationBarColor(const int color);

    void showOnStartup();

    void updateDrawerRootPage(PageLoader::PageEnum page);
    void goToDrawerRootPage();
    void drawerOpen();
    void drawerClose();

signals:
    void goToPage(PageLoader::PageEnum page, bool slide = true);
    void goToStartPage();
    void goToPageHome();
    void goToPageSettings();
    void goToPageViewConfig();
    void goToPageSettingsServerServices();
    void goToPageSettingsBackup();

    void closePage();

    void restorePageHomeState(bool isContainerInstalled = false);
    void replaceStartPage();

    void showErrorMessage(const QString &errorMessage);
    void showNotificationMessage(const QString &message);

    void showBusyIndicator(bool visible);
    void enableTabBar(bool enabled);

    void hideMainWindow();
    void raiseMainWindow();

    void showPassphraseRequestDrawer();
    void passphraseRequestDrawerClosed(QString passphrase);

    void showTopCloseButton(bool visible);
    void forceCloseDrawer();

private:
    QSharedPointer<ServersModel> m_serversModel;

    std::shared_ptr<Settings> m_settings;

    PageLoader::PageEnum m_currentRootPage;
    int m_drawerLayer;
};

#endif // PAGECONTROLLER_H
