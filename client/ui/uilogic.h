#ifndef UILOGIC_H
#define UILOGIC_H

#include <QRegExpValidator>
#include <QQmlEngine>
#include <functional>
#include <QKeyEvent>
#include <QThread>
#include <QSystemTrayIcon>

#include "property_helper.h"
#include "pages.h"
#include "protocols/vpnprotocol.h"
#include "containers/containers_defs.h"

#include "models/containers_model.h"
#include "models/protocols_model.h"

#include "settings.h"

class AppSettingsLogic;
class GeneralSettingsLogic;
class NetworkSettingsLogic;
class NewServerProtocolsLogic;
class ServerConfiguringProgressLogic;
class ServerListLogic;
class ServerSettingsLogic;
class ServerContainersLogic;
class ShareConnectionLogic;
class SitesLogic;
class StartPageLogic;
class VpnLogic;
class WizardLogic;

class PageProtocolLogicBase;
class OpenVpnLogic;
class ShadowSocksLogic;
class CloakLogic;

class OtherProtocolsLogic;

class VpnConnection;


class UiLogic : public QObject
{
    Q_OBJECT

    AUTO_PROPERTY(bool, pageEnabled)
    AUTO_PROPERTY(int, pagesStackDepth)
    AUTO_PROPERTY(int, currentPageValue)

    READONLY_PROPERTY(QObject *, containersModel)
    READONLY_PROPERTY(QObject *, protocolsModel)

    // TODO: review
    Q_PROPERTY(QString dialogConnectErrorText READ getDialogConnectErrorText WRITE setDialogConnectErrorText NOTIFY dialogConnectErrorTextChanged)

public:
    explicit UiLogic(QObject *parent = nullptr);
    ~UiLogic();
    void showOnStartup();

    friend class AppSettingsLogic;
    friend class GeneralSettingsLogic;
    friend class NetworkSettingsLogic;
    friend class ServerConfiguringProgressLogic;
    friend class NewServerProtocolsLogic;
    friend class ServerListLogic;
    friend class ServerSettingsLogic;
    friend class ServerContainersLogic;
    friend class ShareConnectionLogic;
    friend class SitesLogic;
    friend class StartPageLogic;
    friend class VpnLogic;
    friend class WizardLogic;

    friend class PageProtocolLogicBase;
    friend class OpenVpnLogic;
    friend class ShadowSocksLogic;
    friend class CloakLogic;

    friend class OtherProtocolsLogic;

    Q_INVOKABLE virtual void onUpdatePage() {} // UiLogic is set as logic class for some qml pages
    Q_INVOKABLE void onUpdateAllPages();

    Q_INVOKABLE void initalizeUiLogic();
    Q_INVOKABLE void onCloseWindow();

    Q_INVOKABLE QString containerName(int container);
    Q_INVOKABLE QString containerDesc(int container);

    Q_INVOKABLE void onGotoPage(PageEnumNS::Page p, bool reset = true, bool slide = true) { emit goToPage(p, reset, slide); }
    Q_INVOKABLE void onGotoProtocolPage(Protocol p, bool reset = true, bool slide = true) { emit goToProtocolPage(p, reset, slide); }
    Q_INVOKABLE void onGotoShareProtocolPage(Protocol p, bool reset = true, bool slide = true) { emit goToShareProtocolPage(p, reset, slide); }

    Q_INVOKABLE void onGotoCurrentProtocolsPage();

    Q_INVOKABLE void keyPressEvent(Qt::Key key);

    Q_INVOKABLE bool saveTextFile(const QString& desc, const QString& ext, const QString& data);
    Q_INVOKABLE bool saveBinaryFile(const QString& desc, const QString& ext, const QString& data);
    Q_INVOKABLE void copyToClipboard(const QString& text);

    QString getDialogConnectErrorText() const;
    void setDialogConnectErrorText(const QString &dialogConnectErrorText);

signals:
    void trayIconUrlChanged();
    void trayActionDisconnectEnabledChanged();
    void trayActionConnectEnabledChanged();
    void dialogConnectErrorTextChanged();


    void goToPage(PageEnumNS::Page page, bool reset = true, bool slide = true);
    void goToProtocolPage(Protocol protocol, bool reset = true, bool slide = true);
    void goToShareProtocolPage(Protocol protocol, bool reset = true, bool slide = true);

    void closePage();
    void setStartPage(PageEnumNS::Page page, bool slide = true);
    void showPublicKeyWarning();
    void showConnectErrorDialog();
    void show();
    void hide();
    void raise();

private:
    QSystemTrayIcon *m_tray;

    QString m_dialogConnectErrorText;

private slots:
    // containers - INOUT arg
    void installServer(QMap<DockerContainer, QJsonObject> &containers);

    void setTrayState(VpnProtocol::ConnectionState state);
    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);

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
                           QMap<DockerContainer, QJsonObject> &containers,
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
    ServerConfiguringProgressLogic *serverConfiguringProgressLogic()  { return m_serverConfiguringProgressLogic; }
    NewServerProtocolsLogic *newServerProtocolsLogic()      { return m_newServerProtocolsLogic; }
    ServerListLogic *serverListLogic()                      { return m_serverListLogic; }
    ServerSettingsLogic *serverSettingsLogic()              { return m_serverSettingsLogic; }
    ServerContainersLogic *serverVpnProtocolsLogic()        { return m_serverVpnProtocolsLogic; }
    ShareConnectionLogic *shareConnectionLogic()            { return m_shareConnectionLogic; }
    SitesLogic *sitesLogic()                                { return m_sitesLogic; }
    StartPageLogic *startPageLogic()                        { return m_startPageLogic; }
    VpnLogic *vpnLogic()                                    { return m_vpnLogic; }
    WizardLogic *wizardLogic()                              { return m_wizardLogic; }

    Q_INVOKABLE PageProtocolLogicBase *protocolLogic(Protocol p);

    QObject *qmlRoot() const;
    void setQmlRoot(QObject *newQmlRoot);

private:
    QObject *m_qmlRoot{nullptr};

    AppSettingsLogic *m_appSettingsLogic;
    GeneralSettingsLogic *m_generalSettingsLogic;
    NetworkSettingsLogic *m_networkSettingsLogic;
    ServerConfiguringProgressLogic *m_serverConfiguringProgressLogic;
    NewServerProtocolsLogic *m_newServerProtocolsLogic;
    ServerListLogic *m_serverListLogic;
    ServerSettingsLogic *m_serverSettingsLogic;
    ServerContainersLogic *m_serverVpnProtocolsLogic;
    ShareConnectionLogic *m_shareConnectionLogic;
    SitesLogic *m_sitesLogic;
    StartPageLogic *m_startPageLogic;
    VpnLogic *m_vpnLogic;
    WizardLogic *m_wizardLogic;

    QMap<Protocol, PageProtocolLogicBase *> m_protocolLogicMap;

    VpnConnection* m_vpnConnection;
    QThread m_vpnConnectionThread;
    Settings m_settings;


    //    QRegExpValidator m_ipAddressValidator;
    //    QRegExpValidator m_ipAddressPortValidator;
    //    QRegExpValidator m_ipNetwok24Validator;
    //    QRegExpValidator m_ipPortValidator;

    //    QPoint offset;
    //    bool needToHideCustomTitlebar = false;

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
