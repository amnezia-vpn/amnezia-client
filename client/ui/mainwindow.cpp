#include <QKeyEvent>
#include <QMessageBox>
#include <QSysInfo>
#include <QThread>
#include <QTimer>

#include "communicator.h"

#include "core/errorstrings.h"
#include "core/openvpnconfigurator.h"
#include "core/servercontroller.h"

#include "debug.h"
#include "defines.h"
#include "mainwindow.h"
#include "settings.h"
#include "ui_mainwindow.h"
#include "utils.h"
#include "vpnconnection.h"

#ifdef Q_OS_MAC
#include "ui/macos_util.h"
#endif

MainWindow::MainWindow(QWidget *parent) :
#ifdef Q_OS_WIN
    CFramelessWindow(parent),
#else
    QMainWindow(parent),
#endif
    ui(new Ui::MainWindow),
    m_settings(new Settings),
    m_vpnConnection(nullptr)
{
    ui->setupUi(this);
    ui->widget_tittlebar->installEventFilter(this);

    ui->stackedWidget_main->setSpeed(200);
    ui->stackedWidget_main->setAnimation(QEasingCurve::Linear);

    ui->label_new_server_wait_info->setVisible(false);
    ui->progressBar_new_server_connection->setMinimum(0);
    ui->progressBar_new_server_connection->setMaximum(300);

#ifdef Q_OS_MAC
    ui->widget_tittlebar->hide();
    ui->stackedWidget_main->move(0,0);
    fixWidget(this);
#endif

    // Post initialization

    if (m_settings->haveAuthData()) {
        ui->stackedWidget_main->setCurrentWidget(ui->page_amnezia);
    } else {
        ui->stackedWidget_main->setCurrentWidget(ui->page_new_server);
    }

    connect(ui->pushButton_blocked_list, SIGNAL(clicked(bool)), this, SLOT(onPushButtonBlockedListClicked(bool)));
    connect(ui->pushButton_connect, SIGNAL(toggled(bool)), this, SLOT(onPushButtonConnectToggled(bool)));
    connect(ui->pushButton_settings, SIGNAL(clicked(bool)), this, SLOT(onPushButtonSettingsClicked(bool)));

    connect(ui->pushButton_back_from_sites, SIGNAL(clicked(bool)), this, SLOT(onPushButtonBackFromSitesClicked(bool)));
    connect(ui->pushButton_back_from_settings, SIGNAL(clicked(bool)), this, SLOT(onPushButtonBackFromSettingsClicked(bool)));
    connect(ui->pushButton_new_server_setup, SIGNAL(clicked(bool)), this, SLOT(onPushButtonNewServerSetup(bool)));
    connect(ui->pushButton_back_from_new_server, SIGNAL(clicked(bool)), this, SLOT(onPushButtonBackFromNewServerClicked(bool)));
    connect(ui->pushButton_new_server_connect_with_new_data, SIGNAL(clicked(bool)), this, SLOT(onPushButtonNewServerConnectWithNewData(bool)));

    setFixedSize(width(),height());

    qInfo().noquote() << QString("Started %1 version %2").arg(APPLICATION_NAME).arg(APP_VERSION);
    qInfo().noquote() << QString("%1 (%2)").arg(QSysInfo::prettyProductName()).arg(QSysInfo::currentCpuArchitecture());

    Utils::initializePath(Utils::configPath());

    m_vpnConnection = new VpnConnection(this);
    connect(m_vpnConnection, SIGNAL(bytesChanged(quint64, quint64)), this, SLOT(onBytesChanged(quint64, quint64)));
    connect(m_vpnConnection, SIGNAL(connectionStateChanged(VpnProtocol::ConnectionState)), this, SLOT(onConnectionStateChanged(VpnProtocol::ConnectionState)));
    connect(m_vpnConnection, SIGNAL(vpnProtocolError(amnezia::ErrorCode)), this, SLOT(onVpnProtocolError(amnezia::ErrorCode)));

    onConnectionStateChanged(VpnProtocol::ConnectionState::Disconnected);

    qDebug().noquote() << QString("Default config: %1").arg(Utils::defaultVpnConfigFileName());
}

MainWindow::~MainWindow()
{
    hide();

    m_vpnConnection->disconnectFromVpn();
    for (int i = 0; i < 50; i++) {
        qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
        QThread::msleep(100);
        if (m_vpnConnection->onDisconnected()) {
            break;
        }
    }

    delete m_vpnConnection;
    delete ui;

    qDebug() << "Application closed";
}

void MainWindow::goToPage(Page page)
{
    ui->stackedWidget_main->slideInIdx(static_cast<int>(page));
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == ui->widget_tittlebar) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

        if (!mouseEvent)
            return false;

        if(event->type() == QEvent::MouseButtonPress) {
            offset = mouseEvent->pos();
            canMove = true;
        }

        if(event->type() == QEvent::MouseButtonRelease) {
            canMove = false;
        }

        if (event->type() == QEvent::MouseMove) {
            if(canMove && (mouseEvent->buttons() & Qt::LeftButton)) {
                move(mapToParent(mouseEvent->pos() - offset));
            }

            event->ignore();
            return false;
        }
    }

    return QMainWindow::eventFilter(obj, event);
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

void MainWindow::onPushButtonNewServerConnectWithNewData(bool clicked)
{
    if (ui->lineEdit_new_server_ip->text().isEmpty() ||
            ui->lineEdit_new_server_login->text().isEmpty() ||
            ui->lineEdit_new_server_password->text().isEmpty() ) {
        QMessageBox::warning(this, APPLICATION_NAME, tr("Please fill in all fields"));
        return;
    }

    qDebug() << "Start connection with new data";

    ServerCredentials serverCredentials;
    serverCredentials.hostName = ui->lineEdit_new_server_ip->text();
    if (serverCredentials.hostName.contains(":")) {
        serverCredentials.port = serverCredentials.hostName.split(":").at(1).toInt();
        serverCredentials.hostName = serverCredentials.hostName.split(":").at(0);
    }
    serverCredentials.userName = ui->lineEdit_new_server_login->text();
    serverCredentials.password = ui->lineEdit_new_server_password->text();

    m_settings->setServerCredentials(serverCredentials);
    m_settings->save();

    ui->page_new_server->setEnabled(false);
    ui->pushButton_new_server_connect_with_new_data->setVisible(false);
    ui->label_new_server_wait_info->setVisible(true);

    QTimer timer;
    connect(&timer, &QTimer::timeout, [&](){
        ui->progressBar_new_server_connection->setValue(ui->progressBar_new_server_connection->value() + 1);
    });

    ui->progressBar_new_server_connection->setValue(0);
    timer.start(1000);


    ErrorCode e = ServerController::setupServer(serverCredentials, Protocol::Any);
    if (e) {
        ui->page_new_server->setEnabled(true);
        ui->pushButton_new_server_connect_with_new_data->setVisible(true);
        ui->label_new_server_wait_info->setVisible(false);

        QMessageBox::warning(this, APPLICATION_NAME,
                             tr("Error occurred while configuring server.") + "\n" +
                             errorString(e) + "\n" +
                             tr("See logs for details."));

        return;
    }

    // just ui progressbar tweak
    timer.stop();

    int remaining_val = ui->progressBar_new_server_connection->maximum() - ui->progressBar_new_server_connection->value();

    if (remaining_val > 0) {
        QTimer timer1;
        QEventLoop loop1;

        connect(&timer1, &QTimer::timeout, [&](){
            ui->progressBar_new_server_connection->setValue(ui->progressBar_new_server_connection->value() + 1);
            if (ui->progressBar_new_server_connection->value() >= ui->progressBar_new_server_connection->maximum()) {
                loop1.quit();
            }
        });

        timer1.start(5);
        loop1.exec();
    }

    goToPage(Page::Vpn);
    ui->pushButton_connect->setChecked(true);
}

void MainWindow::onBytesChanged(quint64 receivedData, quint64 sentData)
{
    ui->label_speed_received->setText(VpnConnection::bytesToText(receivedData));
    ui->label_speed_sent->setText(VpnConnection::bytesToText(sentData));
}

void MainWindow::onPushButtonBackFromNewServerClicked(bool)
{
    goToPage(Page::Initialization);
}

void MainWindow::onPushButtonNewServerSetup(bool)
{
    goToPage(Page::NewServer);
}

void MainWindow::onPushButtonBackFromSettingsClicked(bool)
{
    goToPage(Page::Vpn);
}

void MainWindow::onPushButtonBackFromSitesClicked(bool)
{
    goToPage(Page::Vpn);
}

void MainWindow::onPushButtonBlockedListClicked(bool)
{
    goToPage(Page::Sites);
}

void MainWindow::onPushButtonSettingsClicked(bool)
{
    goToPage(Page::SomeSettings);
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

void MainWindow::onVpnProtocolError(ErrorCode errorCode)
{
    QMessageBox::critical(this, APPLICATION_NAME, errorString(errorCode));
}

void MainWindow::onPushButtonConnectToggled(bool checked)
{
    if (checked) {
        // TODO: Call connectToVpn with restricted server account
        ServerCredentials credentials = m_settings->serverCredentials();

        ErrorCode errorCode = m_vpnConnection->connectToVpn(credentials);
        if (errorCode) {
            ui->pushButton_connect->setChecked(false);
            QMessageBox::critical(this, APPLICATION_NAME, errorString(errorCode));
            return;
        }
        ui->pushButton_connect->setEnabled(false);
    } else {
        m_vpnConnection->disconnectFromVpn();
    }
}

void MainWindow::on_pushButton_close_clicked()
{
    qApp->exit();
}
