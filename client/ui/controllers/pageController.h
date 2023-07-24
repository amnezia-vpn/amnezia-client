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

        PageServiceSftpSettings,
        PageServiceTorWebsiteSettings,

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
    explicit PageController(const QSharedPointer<ServersModel> &serversModel, QObject *parent = nullptr);

public slots:
    QString getInitialPage();
    QString getPagePath(PageLoader::PageEnum page);

    void closeWindow();
    void keyPressEvent(Qt::Key key);

signals:
    void goToPageHome();
    void goToPageSettings();
    void goToPageViewConfig();
    void closePage();

    void restorePageHomeState(bool isContainerInstalled = false);
    void replaceStartPage();

    void showErrorMessage(QString errorMessage);
    void showInfoMessage(QString message);

    void showBusyIndicator(bool visible);

    void hideMainWindow();
    void raiseMainWindow();

private:
    QSharedPointer<ServersModel> m_serversModel;
};

#endif // PAGECONTROLLER_H
