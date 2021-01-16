#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QLabel>
#include <QMainWindow>
#include <QProgressBar>
#include <QPushButton>
#include <QSystemTrayIcon>

#include "framelesswindow.h"
#include "protocols/vpnprotocol.h"

class Settings;
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

    enum Page {Start, NewServer, Vpn, GeneralSettings, ServerSettings, ShareConnection,  Sites};
    Q_ENUM(Page)

private slots:
    void onBytesChanged(quint64 receivedBytes, quint64 sentBytes);
    void onConnectionStateChanged(VpnProtocol::ConnectionState state);
    void onVpnProtocolError(amnezia::ErrorCode errorCode);

    void onPushButtonConnectClicked(bool checked);
    void onPushButtonNewServerConnectWithNewData(bool);

    void onPushButtonReinstallServer(bool);
    void onPushButtonClearServer(bool);
    void onPushButtonForgetServer(bool);

    void onTrayActionConnect(); // connect from context menu
    void setTrayState(VpnProtocol::ConnectionState state);

    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);

    void onConnect();
    void onDisconnect();


private:
    void goToPage(Page page, bool reset = true, bool slide = true);
    QWidget *getPageWidget(Page page);
    Page currentPage();

    bool installServer(ServerCredentials credentials, QWidget *page, QProgressBar *progress, QPushButton *button, QLabel *info);

    void setupTray();
    void setTrayIcon(const QString &iconPath);

    void setupUiConnections();

    Ui::MainWindow *ui;
    VpnConnection* m_vpnConnection;
    Settings* m_settings;

    QAction* m_trayActionConnect;
    QAction* m_trayActionDisconnect;

    QSystemTrayIcon m_tray;
    QMenu* m_menu;

    bool canMove = false;
    QPoint offset;
    bool eventFilter(QObject *obj, QEvent *event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void closeEvent(QCloseEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

    const QString ConnectedTrayIconName = "active.png";
    const QString DisconnectedTrayIconName = "default.png";
    const QString ErrorTrayIconName = "error.png";
};

#endif // MAINWINDOW_H
