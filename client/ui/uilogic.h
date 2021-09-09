#ifndef UILOGIC_H
#define UILOGIC_H

#include <QRegExpValidator>
#include <QQmlEngine>
#include <functional>

#include "property_helper.h"
#include "pages.h"
#include "protocols/vpnprotocol.h"
#include "containers/containers_defs.h"
#include "models/all_containers_model.h"

#include "settings.h"

class AppSettingsLogic;
class GeneralSettingsLogic;
class NetworkSettingsLogic;
class NewServerProtocolsLogic;
class NewServerConfiguringLogic;
class ServerListLogic;
class ServerSettingsLogic;
class ServerContainersLogic;
class ShareConnectionLogic;
class SitesLogic;
class StartPageLogic;
class VpnLogic;
class WizardLogic;

class OpenVpnLogic;
class ShadowSocksLogic;
class CloakLogic;

class VpnConnection;


class UiLogic : public QObject
{
    Q_OBJECT

    READONLY_PROPERTY(QObject *, allContainersModel)

    Q_PROPERTY(int currentPageValue READ getCurrentPageValue WRITE setCurrentPageValue NOTIFY currentPageValueChanged)
    Q_PROPERTY(QString trayIconUrl READ getTrayIconUrl WRITE setTrayIconUrl NOTIFY trayIconUrlChanged)
    Q_PROPERTY(bool trayActionDisconnectEnabled READ getTrayActionDisconnectEnabled WRITE setTrayActionDisconnectEnabled NOTIFY trayActionDisconnectEnabledChanged)
    Q_PROPERTY(bool trayActionConnectEnabled READ getTrayActionConnectEnabled WRITE setTrayActionConnectEnabled NOTIFY trayActionConnectEnabledChanged)

    Q_PROPERTY(QString dialogConnectErrorText READ getDialogConnectErrorText WRITE setDialogConnectErrorText NOTIFY dialogConnectErrorTextChanged)

public:
    explicit UiLogic(QObject *parent = nullptr);
    ~UiLogic();
    void showOnStartup();

    friend class AppSettingsLogic;
    friend class GeneralSettingsLogic;
    friend class NetworkSettingsLogic;
    friend class NewServerConfiguringLogic;
    friend class NewServerProtocolsLogic;
    friend class ServerListLogic;
    friend class ServerSettingsLogic;
    friend class ServerContainersLogic;
    friend class ShareConnectionLogic;
    friend class SitesLogic;
    friend class StartPageLogic;
    friend class VpnLogic;
    friend class WizardLogic;

    friend class OpenVpnLogic;
    friend class ShadowSocksLogic;
    friend class CloakLogic;

    Q_INVOKABLE void initalizeUiLogic();
    Q_INVOKABLE void onCloseWindow();


    int getCurrentPageValue() const;
    void setCurrentPageValue(int currentPageValue);
    QString getTrayIconUrl() const;
    void setTrayIconUrl(const QString &trayIconUrl);
    bool getTrayActionDisconnectEnabled() const;
    void setTrayActionDisconnectEnabled(bool trayActionDisconnectEnabled);
    bool getTrayActionConnectEnabled() const;
    void setTrayActionConnectEnabled(bool trayActionConnectEnabled);

    QString getDialogConnectErrorText() const;
    void setDialogConnectErrorText(const QString &dialogConnectErrorText);

signals:
    void currentPageValueChanged();
    void trayIconUrlChanged();
    void trayActionDisconnectEnabledChanged();
    void trayActionConnectEnabledChanged();
    void dialogConnectErrorTextChanged();


    void goToPage(int page, bool reset = true, bool slide = true);
    void closePage();
    void setStartPage(int page, bool slide = true);
    void showPublicKeyWarning();
    void showConnectErrorDialog();
    void show();
    void hide();

private:
    int m_currentPageValue;
    QString m_trayIconUrl;
    bool m_trayActionDisconnectEnabled;
    bool m_trayActionConnectEnabled;

    QString m_dialogConnectErrorText;

private slots:
    void installServer(const QMap<DockerContainer, QJsonObject> &containers);
    void setTrayState(VpnProtocol::ConnectionState state);

private:
    PageEnumNS::Page currentPage();
    struct ProgressFunc {
        std::function<void(bool)> setVisibleFunc;
        std::function<void(int)> setValueFunc;
        std::function<int(void)> getValueFunc;
        std::function<int(void)> getMaximiumFunc;
        std::function<void(bool)> setTextVisibleFunc;
        std::function<void(const QString&)> setTextFunc;
    };
    struct PageFunc {
        std::function<void(bool)> setEnabledFunc;
    };
    struct ButtonFunc {
        std::function<void(bool)> setVisibleFunc;
    };
    struct LabelFunc {
        std::function<void(bool)> setVisibleFunc;
        std::function<void(const QString&)> setTextFunc;
    };

    bool installContainers(ServerCredentials credentials,
                           const QMap<DockerContainer, QJsonObject> &containers,
                           const PageFunc& page,
                           const ProgressFunc& progress,
                           const ButtonFunc& button,
                           const LabelFunc& info);

    ErrorCode doInstallAction(const std::function<ErrorCode()> &action,
                              const PageFunc& page,
                              const ProgressFunc& progress,
                              const ButtonFunc& button,
                              const LabelFunc& info);

    void setupTray();
    void setTrayIcon(const QString &iconPath);


public:
    AppSettingsLogic *appSettingsLogic()                    { return m_appSettingsLogic; }
    GeneralSettingsLogic *generalSettingsLogic()            { return m_generalSettingsLogic; }
    NetworkSettingsLogic *networkSettingsLogic()            { return m_networkSettingsLogic; }
    NewServerConfiguringLogic *newServerConfiguringLogic()  { return m_newServerConfiguringLogic; }
    NewServerProtocolsLogic *newServerProtocolsLogic()      { return m_newServerProtocolsLogic; }
    ServerListLogic *serverListLogic()                      { return m_serverListLogic; }
    ServerSettingsLogic *serverSettingsLogic()              { return m_serverSettingsLogic; }
    ServerContainersLogic *serverVpnProtocolsLogic()      { return m_serverVpnProtocolsLogic; }
    ShareConnectionLogic *shareConnectionLogic()            { return m_shareConnectionLogic; }
    SitesLogic *sitesLogic()                                { return m_sitesLogic; }
    StartPageLogic *startPageLogic()                        { return m_startPageLogic; }
    VpnLogic *vpnLogic()                                    { return m_vpnLogic; }
    WizardLogic *wizardLogic()                              { return m_wizardLogic; }

    OpenVpnLogic *openVpnLogic()                            { return m_openVpnLogic; }
    ShadowSocksLogic *shadowSocksLogic()                    { return m_shadowSocksLogic; }
    CloakLogic *cloakLogic()                                { return m_cloakLogic; }

private:
    AppSettingsLogic *m_appSettingsLogic;
    GeneralSettingsLogic *m_generalSettingsLogic;
    NetworkSettingsLogic *m_networkSettingsLogic;
    NewServerConfiguringLogic *m_newServerConfiguringLogic;
    NewServerProtocolsLogic *m_newServerProtocolsLogic;
    ServerListLogic *m_serverListLogic;
    ServerSettingsLogic *m_serverSettingsLogic;
    ServerContainersLogic *m_serverVpnProtocolsLogic;
    ShareConnectionLogic *m_shareConnectionLogic;
    SitesLogic *m_sitesLogic;
    StartPageLogic *m_startPageLogic;
    VpnLogic *m_vpnLogic;
    WizardLogic *m_wizardLogic;

    OpenVpnLogic *m_openVpnLogic;
    ShadowSocksLogic *m_shadowSocksLogic;
    CloakLogic *m_cloakLogic;

    VpnConnection* m_vpnConnection;
    Settings m_settings;


    //    QRegExpValidator m_ipAddressValidator;
    //    QRegExpValidator m_ipAddressPortValidator;
    //    QRegExpValidator m_ipNetwok24Validator;
    //    QRegExpValidator m_ipPortValidator;

    //    QPoint offset;
    //    bool needToHideCustomTitlebar = false;

    //    void keyPressEvent(QKeyEvent* event) override;
    //    void showEvent(QShowEvent *event) override;
    //    void hideEvent(QHideEvent *event) override;

    const QString ConnectedTrayIconName = "active.png";
    const QString DisconnectedTrayIconName = "default.png";
    const QString ErrorTrayIconName = "error.png";


    //    QStack<Page> pagesStack;
    int selectedServerIndex = -1; // server index to use when proto settings page opened
    DockerContainer selectedDockerContainer; // same
    ServerCredentials installCredentials; // used to save cred between pages new_server and new_server_protocols and wizard
};
#endif // UILOGIC_H
