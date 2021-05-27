#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QLabel>
#include <QListWidget>
#include <QMainWindow>
#include <QProgressBar>
#include <QPushButton>
#include <QRegExpValidator>
#include <QStack>
#include <QStringListModel>
#include <QSystemTrayIcon>

#include "3rd/QRCodeGenerator/QRCodeGenerator.h"

#include "framelesswindow.h"
#include "protocols/vpnprotocol.h"

#include "settings.h"
#include "sites_model.h"

class VpnConnection;

namespace Ui {
class MainWindow;
}

/**
 * @brief The MainWindow class - Main application window
 */
#ifdef Q_OS_WIN
class MainWindow : public CFramelessWindow
#else
class MainWindow : public QMainWindow
#endif

{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    enum Page {Start, NewServer, NewServerProtocols, Vpn,
               Wizard, WizardLow, WizardMedium, WizardHigh, WizardVpnMode, ServerConfiguring,
               GeneralSettings, AppSettings, NetworkSettings, ServerSettings,
               ServerVpnProtocols, ServersList, ShareConnection,  Sites,
               OpenVpnSettings, ShadowSocksSettings, CloakSettings};
    Q_ENUM(Page)

    void showOnStartup();

private slots:
    void onBytesChanged(quint64 receivedBytes, quint64 sentBytes);
    void onConnectionStateChanged(VpnProtocol::ConnectionState state);
    void onVpnProtocolError(amnezia::ErrorCode errorCode);

    void onPushButtonConnectClicked(bool checked);
    void onPushButtonNewServerConnect(bool);
    void installServer(const QMap<DockerContainer, QJsonObject> &containers);
    void onPushButtonNewServerImport(bool);

    void onPushButtonClearServer(bool);
    void onPushButtonForgetServer(bool);

    void onPushButtonAddCustomSitesClicked();

    void onTrayActionConnect(); // connect from context menu
    void setTrayState(VpnProtocol::ConnectionState state);

    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);

    void onConnect();
    void onConnectWorker(int serverIndex, const ServerCredentials &credentials, DockerContainer container, const QJsonObject &containerConfig);
    void onDisconnect();


private:
    void goToPage(Page page, bool reset = true, bool slide = true);
    void setStartPage(Page page, bool slide = true);
    void closePage();

    QWidget *getPageWidget(Page page);
    Page currentPage();

    bool installContainers(ServerCredentials credentials, const QMap<DockerContainer, QJsonObject> &containers,
        QWidget *page, QProgressBar *progress, QPushButton *button, QLabel *info);

    ErrorCode doInstallAction(const std::function<ErrorCode()> &action, QWidget *page, QProgressBar *progress, QPushButton *button, QLabel *info);

    void setupTray();
    void setTrayIcon(const QString &iconPath);

    void setupUiConnections();
    void setupNewServerConnections();
    void setupWizardConnections();
    void setupVpnPageConnections();
    void setupSitesPageConnections();
    void setupAppSettingsConnections();
    void setupGeneralSettingsConnections();
    void setupNetworkSettingsConnections();
    void setupProtocolsPageConnections();
    void setupNewServerPageConnections();
    void setupServerSettingsPageConnections();
    void setupSharePageConnections();

    void updateStartPage();
    void updateSitesPage();
    void updateVpnPage();
    void updateAppSettingsPage();
    void updateGeneralSettingPage();
    void updateServerPage();
    void updateServersListPage();
    void updateProtocolsPage();
    void updateOpenVpnPage(const QJsonObject &openvpnConfig, DockerContainer container, bool haveAuthData);
    void updateShadowSocksPage(const QJsonObject &ssConfig, DockerContainer container, bool haveAuthData);
    void updateCloakPage(const QJsonObject &ckConfig, DockerContainer container, bool haveAuthData);

    void updateSharingPage(int serverIndex, const ServerCredentials &credentials,
        DockerContainer container);

    void makeServersListItem(QListWidget* listWidget, const QJsonObject &server, bool isDefault, int index);

    void updateQRCodeImage(const QString &text, QLabel *label);

    QJsonObject getOpenVpnConfigFromPage(QJsonObject oldConfig);
    QJsonObject getShadowSocksConfigFromPage(QJsonObject oldConfig);
    QJsonObject getCloakConfigFromPage(QJsonObject oldConfig);

    QMap<DockerContainer, QJsonObject> getInstallConfigsFromProtocolsPage() const;
    QMap<DockerContainer, QJsonObject> getInstallConfigsFromWizardPage() const;

private:
    Ui::MainWindow *ui;
    VpnConnection* m_vpnConnection;
    Settings m_settings;

    QMap<Settings::RouteMode, SitesModel *> sitesModels;

    QAction* m_trayActionConnect;
    QAction* m_trayActionDisconnect;

    QSystemTrayIcon m_tray;
    QMenu* m_menu;

    QRegExpValidator m_ipAddressValidator;
    QRegExpValidator m_ipAddressPortValidator;
    QRegExpValidator m_ipNetwok24Validator;
    QRegExpValidator m_ipPortValidator;

    CQR_Encode m_qrEncode;

    bool canMove = false;
    QPoint offset;
    bool needToHideCustomTitlebar = false;

    bool eventFilter(QObject *obj, QEvent *event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void closeEvent(QCloseEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

    const QString ConnectedTrayIconName = "active.png";
    const QString DisconnectedTrayIconName = "default.png";
    const QString ErrorTrayIconName = "error.png";


    QStack<Page> pagesStack;
    int selectedServerIndex = -1; // server index to use when proto settings page opened
    DockerContainer selectedDockerContainer; // same
    ServerCredentials installCredentials; // used to save cred between pages new_server and new_server_protocols and wizard
};

#endif // MAINWINDOW_H
