#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "vpnprotocol.h"

class Settings;
class VpnConnection;

namespace Ui {
class MainWindow;
}

/**
 * @brief The MainWindow class - Main application window
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    enum class Page {Initialization = 0, NewServer = 1, Vpn = 2, Sites = 3, SomeSettings = 4, Share = 5};

private slots:
    void onBytesChanged(quint64 receivedBytes, quint64 sentBytes);
    void onConnectionStateChanged(VpnProtocol::ConnectionState state);
    void onPushButtonBackFromNewServerClicked(bool clicked);
    void onPushButtonBackFromSettingsClicked(bool clicked);
    void onPushButtonBackFromSitesClicked(bool clicked);
    void onPushButtonBlockedListClicked(bool clicked);
    void onPushButtonConnectToggled(bool checked);
    void onPushButtonNewServerConnectWithNewData(bool clicked);
    void onPushButtonNewServerSetup(bool clicked);
    void onPushButtonSettingsClicked(bool clicked);

    void on_pushButton_close_clicked();

protected:
    bool requestOvpnConfig(const QString& hostName, const QString& userName, const QString& password, int port = 22, int timeout = 30);

private:
    void goToPage(Page page);

    Ui::MainWindow *ui;
    VpnConnection* m_vpnConnection;
    Settings* m_settings;

    bool canMove = false;
    QPoint offset;
    bool eventFilter(QObject *obj, QEvent *event) override;
    void keyPressEvent(QKeyEvent* event) override;
};

#endif // MAINWINDOW_H
