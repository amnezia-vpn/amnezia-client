#include <QKeyEvent>
#include <QMessageBox>
#include <QSysInfo>
#include <QThread>

#include "communicator.h"
#include "debug.h"
#include "defines.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "utils.h"
#include "vpnconnection.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_vpnConnection(nullptr)
{
    ui->setupUi(this);

    // Post initialization
    ui->widget_tittlebar->hide();
    ui->stackedWidget_main->setCurrentIndex(2);

    connect(ui->pushButton_blocked_list, SIGNAL(clicked(bool)), this, SLOT(onPushButtonBlockedListClicked(bool)));
    connect(ui->pushButton_connect, SIGNAL(toggled(bool)), this, SLOT(onPushButtonConnectToggled(bool)));
    connect(ui->pushButton_settings, SIGNAL(clicked(bool)), this, SLOT(onPushButtonSettingsClicked(bool)));

    connect(ui->pushButton_back_from_sites, SIGNAL(clicked(bool)), this, SLOT(onPushButtonBackFromSitesClicked(bool)));
    connect(ui->pushButton_back_from_settings, SIGNAL(clicked(bool)), this, SLOT(onPushButtonBackFromSettingsClicked(bool)));

    setFixedSize(width(),height());

    qInfo().noquote() << QString("Started %1 version %2").arg(APPLICATION_NAME).arg(APP_VERSION);
    qInfo().noquote() << QString("%1 (%2)").arg(QSysInfo::prettyProductName()).arg(QSysInfo::currentCpuArchitecture());


    QDir dir;
    QString configPath = Utils::systemConfigPath();
    if (!dir.mkpath(configPath)) {
        qWarning() << "Cannot initialize config path:" << configPath;
    }

    m_vpnConnection = new VpnConnection;
    connect(m_vpnConnection, SIGNAL(bytesChanged(quint64, quint64)), this, SLOT(onBytesChanged(quint64, quint64)));
    connect(m_vpnConnection, SIGNAL(connectionStateChanged(VpnProtocol::ConnectionState)), this, SLOT(onConnectionStateChanged(VpnProtocol::ConnectionState)));

    onConnectionStateChanged(VpnProtocol::ConnectionState::Disconnected);
}

MainWindow::~MainWindow()
{
    delete ui;

    hide();

    m_vpnConnection->disconnectFromVpn();
    for (int i = 0; i < 50; i++) {
        qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
        QThread::msleep(100);
        if (m_vpnConnection->disconnected()) {
            break;
        }
    }
    qDebug() << "Closed";
}

void MainWindow::goToIndex(int index)
{
    ui->stackedWidget_main->setCurrentIndex(index);
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_L:
        if (!Debug::openLogsFolder()) {
            QMessageBox::warning(this, APPLICATION_NAME, tr("Cannot open logs folder!"));
        }
        break;
    default:
        ;
    }
}

void MainWindow::onBytesChanged(quint64 receivedData, quint64 sentData)
{
    ui->label_speed_received->setText(VpnConnection::bytesToText(receivedData));
    ui->label_speed_sent->setText(VpnConnection::bytesToText(sentData));
}

void MainWindow::onPushButtonBackFromSettingsClicked(bool)
{
    goToIndex(2);
}

void MainWindow::onPushButtonBackFromSitesClicked(bool)
{
    goToIndex(2);
}

void MainWindow::onPushButtonBlockedListClicked(bool)
{
    goToIndex(3);
}

void MainWindow::onPushButtonSettingsClicked(bool)
{
    goToIndex(4);
}

void MainWindow::onConnectionStateChanged(VpnProtocol::ConnectionState state)
{
    bool pushButtonConnectEnabled = false;
    ui->label_state->setText(VpnProtocol::textConnectionState(state));

    switch (state) {
    case VpnProtocol::ConnectionState::Disconnected:
        onBytesChanged(0,0);
        ui->pushButton_connect->setChecked(false);
        pushButtonConnectEnabled = true;
        break;
    case VpnProtocol::ConnectionState::Preparing:
        pushButtonConnectEnabled = false;
        break;
    case VpnProtocol::ConnectionState::Connecting:
        pushButtonConnectEnabled = false;
        break;
    case VpnProtocol::ConnectionState::Connected:
        pushButtonConnectEnabled = true;
        break;
    case VpnProtocol::ConnectionState::Disconnecting:
        pushButtonConnectEnabled = false;
        break;
    case VpnProtocol::ConnectionState::TunnelReconnecting:
        pushButtonConnectEnabled = false;
        break;
    case VpnProtocol::ConnectionState::Error:
        pushButtonConnectEnabled = true;
        break;
    case VpnProtocol::ConnectionState::Unknown:
    default:
        pushButtonConnectEnabled = true;
        ;
    }

    ui->pushButton_connect->setEnabled(pushButtonConnectEnabled);
}

void MainWindow::onPushButtonConnectToggled(bool checked)
{
    if (checked) {
        m_vpnConnection->connectToVpn();
        ui->pushButton_connect->setEnabled(false);
    } else {
        m_vpnConnection->disconnectFromVpn();
    }
}


