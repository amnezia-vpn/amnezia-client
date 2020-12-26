#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "vpnprotocol.h"

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

public slots:


private slots:
    void onPushButtonBlockedListClicked(bool clicked);
    void onPushButtonConnectToggled(bool checked);
    void onPushButtonSettingsClicked(bool clicked);

    void onPushButtonBackFromSettingsClicked(bool clicked);
    void onPushButtonBackFromSitesClicked(bool clicked);

    void onBytesChanged(quint64 receivedBytes, quint64 sentBytes);
    void onConnectionStateChanged(VpnProtocol::ConnectionState state);

protected:
    void keyPressEvent(QKeyEvent* event);

private:
    void goToIndex(int index);

    Ui::MainWindow *ui;
    VpnConnection* m_vpnConnection;
};

#endif // MAINWINDOW_H
