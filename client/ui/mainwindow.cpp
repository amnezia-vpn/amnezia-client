#include <QKeyEvent>
#include <QMessageBox>
#include <QSysInfo>
#include <QThread>

#include "communicator.h"
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

#ifdef Q_OS_MAC
    ui->widget_tittlebar->hide();
    ui->stackedWidget_main->move(0,0);
    fixWidget(this);
#endif

    // Post initialization

    if (m_settings->haveAuthData()) {
        goToPage(Page::Vpn);
    } else {
        goToPage(Page::Initialization);
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
        if (m_vpnConnection->disconnected()) {
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
    } else {
        qDebug() << "Start connection with new data";
        m_settings->setServerName(ui->lineEdit_new_server_ip->text());
        m_settings->setLogin(ui->lineEdit_new_server_login->text());
        m_settings->setPassword(ui->lineEdit_new_server_password->text());
        m_settings->save();

        //goToPage(Page::Vpn);

        if (requestOvpnConfig(m_settings->serverName(), m_settings->login(), m_settings->password())) {
            goToPage(Page::Vpn);

            ui->pushButton_connect->setDown(true);

            if (!m_vpnConnection->connectToVpn()) {
                ui->pushButton_connect->setChecked(false);
                QMessageBox::critical(this, APPLICATION_NAME, m_vpnConnection->lastError());
                return;
            }

        }
    }
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

void MainWindow::onPushButtonConnectToggled(bool checked)
{
    if (checked) {
        if (!m_vpnConnection->connectToVpn()) {
            ui->pushButton_connect->setChecked(false);
            QMessageBox::critical(this, APPLICATION_NAME, m_vpnConnection->lastError());
            return;
        }
        ui->pushButton_connect->setEnabled(false);
    } else {
        m_vpnConnection->disconnectFromVpn();
    }
}

bool MainWindow::requestOvpnConfig(const QString& hostName, const QString& userName, const QString& password, int port, int timeout)
{
    QSsh::SshConnectionParameters sshParams;
    sshParams.authenticationType = QSsh::SshConnectionParameters::AuthenticationTypePassword;
    sshParams.host = hostName;
    sshParams.userName = userName;
    sshParams.password = password;
    sshParams.timeout = timeout;
    sshParams.port = port;
    sshParams.hostKeyCheckingMode = QSsh::SshHostKeyCheckingMode::SshHostKeyCheckingNone;

    if (!ServerController::setupServer(sshParams, ServerController::OpenVPN)) {
        return false;
    }

    QString configData = OpenVpnConfigurator::genOpenVpnConfig(sshParams);
    if (configData.isEmpty()) {
        return false;
    }

    QFile file(Utils::defaultVpnConfigFileName());
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)){
        QTextStream stream(&file);
        stream << configData << endl;
        return true;
    }

    return false;
}

void MainWindow::on_pushButton_close_clicked()
{
    qApp->exit();
}
