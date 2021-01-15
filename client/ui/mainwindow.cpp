#include <QApplication>
#include <QDesktopServices>
#include <QKeyEvent>
#include <QMenu>
#include <QMessageBox>
#include <QMetaEnum>
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
    m_vpnConnection(nullptr),
    m_settings(new Settings)
{
    ui->setupUi(this);
    ui->label_error_text->clear();
    ui->widget_tittlebar->installEventFilter(this);

    ui->stackedWidget_main->setSpeed(200);
    ui->stackedWidget_main->setAnimation(QEasingCurve::Linear);

    ui->pushButton_blocked_list->setEnabled(false);
    ui->pushButton_share_connection->setEnabled(false);
#ifdef Q_OS_MAC
    ui->widget_tittlebar->hide();
    ui->stackedWidget_main->move(0,0);
    fixWidget(this);
#endif

    // Post initialization

    if (m_settings->haveAuthData()) {
        goToPage(Page::Vpn, true, false);
    } else {
        goToPage(Page::Start, true, false);
    }

    setupTray();
    setupUiConnections();

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

void MainWindow::goToPage(Page page, bool reset, bool slide)
{
    if (reset) {
        if (page == Page::NewServer) {
            ui->label_new_server_wait_info->hide();
            ui->label_new_server_wait_info->clear();

            ui->progressBar_new_server_connection->setMinimum(0);
            ui->progressBar_new_server_connection->setMaximum(300);
        }
        if (page == Page::ServerSettings) {
            ui->label_server_settings_wait_info->hide();
            ui->label_server_settings_wait_info->clear();
            ui->label_server_settings_server->setText(QString("%1@%2:%3")
                                                      .arg(m_settings->userName())
                                                      .arg(m_settings->serverName())
                                                      .arg(m_settings->serverPort()));
        }

    }

    if (slide)
        ui->stackedWidget_main->slideInWidget(getPageWidget(page));
    else
        ui->stackedWidget_main->setCurrentWidget(getPageWidget(page));
}

QWidget *MainWindow::getPageWidget(MainWindow::Page page)
{
    switch (page) {
    case(Page::Start): return ui->page_start;
    case(Page::NewServer): return ui->page_new_server;
    case(Page::Vpn): return ui->page_vpn;
    case(Page::GeneralSettings): return ui->page_general_settings;
    case(Page::ServerSettings): return ui->page_server_settings;
    case(Page::ShareConnection): return ui->page_share_connection;
    case(Page::Sites): return ui->page_sites;
    }
    return nullptr;
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

void MainWindow::closeEvent(QCloseEvent *event)
{
    event->ignore();
    hide();
}

void MainWindow::onPushButtonNewServerConnectWithNewData(bool)
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

    bool ok = installServer(serverCredentials,
                            ui->page_new_server,
                            ui->progressBar_new_server_connection,
                            ui->pushButton_new_server_connect_with_new_data,
                            ui->label_new_server_wait_info);

    if (ok) {
        m_settings->setServerCredentials(serverCredentials);
        m_settings->save();

        goToPage(Page::Vpn);
        qApp->processEvents();

        onConnect();
    }
}

bool MainWindow::installServer(ServerCredentials credentials,
                               QWidget *page, QProgressBar *progress, QPushButton *button, QLabel *info)
{
    page->setEnabled(false);
    button->setVisible(false);

    info->setVisible(true);
    info->setText(tr("Please wait, configuring process may take up to 5 minutes"));

    QTimer timer;
    connect(&timer, &QTimer::timeout, [progress](){
        progress->setValue(progress->value() + 1);
    });

    progress->setValue(0);
    timer.start(1000);


    ErrorCode e = ServerController::setupServer(credentials, Protocol::OpenVpn);
    if (e) {
        page->setEnabled(true);
        button->setVisible(true);
        info->setVisible(false);

        QMessageBox::warning(this, APPLICATION_NAME,
                             tr("Error occurred while configuring server.") + "\n" +
                             errorString(e));

        return false;
    }

    // just ui progressbar tweak
    timer.stop();

    int remaining_val = progress->maximum() - progress->value();

    if (remaining_val > 0) {
        QTimer timer1;
        QEventLoop loop1;

        connect(&timer1, &QTimer::timeout, [&](){
            progress->setValue(progress->value() + 1);
            if (progress->value() >= progress->maximum()) {
                loop1.quit();
            }
        });

        timer1.start(5);
        loop1.exec();
    }

    button->show();
    page->setEnabled(true);
    info->setText(tr("Amnezia server installed"));

    return true;
}

void MainWindow::onPushButtonReinstallServer(bool)
{
    onDisconnect();
    installServer(m_settings->serverCredentials(),
                  ui->page_server_settings,
                  ui->progressBar_server_settings_reinstall,
                  ui->pushButton_server_settings_reinstall,
                  ui->label_server_settings_wait_info);
}

void MainWindow::onPushButtonClearServer(bool)
{
    onDisconnect();

    ErrorCode e = ServerController::removeServer(m_settings->serverCredentials(), Protocol::Any);
    if (e) {
        QMessageBox::warning(this, APPLICATION_NAME,
                             tr("Error occurred while configuring server.") + "\n" +
                             errorString(e) + "\n" +
                             tr("See logs for details."));

        return;
    }
    else {
        ui->label_server_settings_wait_info->show();
        ui->label_server_settings_wait_info->setText(tr("Amnezia server successfully uninstalled"));
    }
}

void MainWindow::onPushButtonForgetServer(bool)
{
    onDisconnect();

    m_settings->setUserName("");
    m_settings->setPassword("");
    m_settings->setServerName("");
    m_settings->setServerPort();

    m_settings->save();

    goToPage(Page::Start);
}

void MainWindow::onBytesChanged(quint64 receivedData, quint64 sentData)
{
    qDebug() << "MainWindow::onBytesChanged" << receivedData << sentData;
    ui->label_speed_received->setText(VpnConnection::bytesPerSecToText(receivedData));
    ui->label_speed_sent->setText(VpnConnection::bytesPerSecToText(sentData));
}

void MainWindow::onConnectionStateChanged(VpnProtocol::ConnectionState state)
{
    qDebug() << "MainWindow::onConnectionStateChanged" << VpnProtocol::textConnectionState(state);

    bool pushButtonConnectEnabled = false;
    ui->label_state->setText(VpnProtocol::textConnectionState(state));

    setTrayState(state);

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
        pushButtonConnectEnabled = true;
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
    ui->label_error_text->setText(errorString(errorCode));
}

void MainWindow::onPushButtonConnectClicked(bool checked)
{
    if (checked) {
        onConnect();
    } else {
        onDisconnect();
    }
}

void MainWindow::setupTray()
{
    m_menu = new QMenu();
    //m_menu->setStyleSheet(styleSheet());

    m_menu->addAction(QIcon(":/images/tray/application.png"), tr("Show") + " " + APPLICATION_NAME, this, SLOT(show()));
    m_menu->addSeparator();
    m_trayActionConnect = m_menu->addAction(tr("Connect"), this, SLOT(onConnect()));
    m_trayActionDisconnect = m_menu->addAction(tr("Disconnect"), this, SLOT(onDisconnect()));

    m_menu->addSeparator();

    m_menu->addAction(QIcon(":/images/tray/link.png"), tr("Visit Website"), [&](){
        QDesktopServices::openUrl(QUrl("https://amnezia.org"));
    });

    m_menu->addAction(QIcon(":/images/tray/cancel.png"), tr("Quit") + " " + APPLICATION_NAME, this, [&](){
        QMessageBox msgBox(QMessageBox::Question, tr("Exit"), tr("Do you really want to quit?"),
                           QMessageBox::Yes | QMessageBox::No, Q_NULLPTR, Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowStaysOnTopHint);
        msgBox.setDefaultButton(QMessageBox::No);
        msgBox.raise();
        if (msgBox.exec() == QMessageBox::Yes) {
            qApp->quit();
        }
    });

    m_tray.setContextMenu(m_menu);
    setTrayState(VpnProtocol::ConnectionState::Disconnected);

    m_tray.show();

    connect(&m_tray, &QSystemTrayIcon::activated, this, &MainWindow::onTrayActivated);
}

void MainWindow::setTrayIcon(const QString &iconPath)
{
    m_tray.setIcon(QIcon(QPixmap(iconPath).scaled(128,128)));
}

MainWindow::Page MainWindow::currentPage()
{
    QWidget *currentPage = ui->stackedWidget_main->currentWidget();
    QMetaEnum e = QMetaEnum::fromType<MainWindow::Page>();

    for (int k = 0; k < e.keyCount(); k++)     {
        Page p = static_cast<MainWindow::Page>(e.value(k));
        if (currentPage == getPageWidget(p)) return p;
    }

    return Page::Start;
}

void MainWindow::setupUiConnections()
{
    connect(ui->pushButton_close, &QPushButton::clicked, this, [this](){
        if (currentPage() == Page::Start || currentPage() == Page::NewServer) qApp->quit();
        else hide();
    });
    connect(ui->pushButton_general_settings_exit, &QPushButton::clicked, this, [&](){ qApp->quit(); });
    connect(ui->pushButton_new_server_get_info, &QPushButton::clicked, this, [&](){
        QDesktopServices::openUrl(QUrl("https://amnezia.org"));
    });

    connect(ui->pushButton_connect, SIGNAL(clicked(bool)), this, SLOT(onPushButtonConnectClicked(bool)));
    connect(ui->pushButton_new_server_setup, &QPushButton::clicked, this, [this](){ goToPage(Page::NewServer); });
    connect(ui->pushButton_new_server_connect_with_new_data, SIGNAL(clicked(bool)), this, SLOT(onPushButtonNewServerConnectWithNewData(bool)));

    connect(ui->pushButton_server_settings_reinstall, SIGNAL(clicked(bool)), this, SLOT(onPushButtonReinstallServer(bool)));
    connect(ui->pushButton_server_settings_clear, SIGNAL(clicked(bool)), this, SLOT(onPushButtonClearServer(bool)));
    connect(ui->pushButton_server_settings_forget, SIGNAL(clicked(bool)), this, SLOT(onPushButtonForgetServer(bool)));

    connect(ui->pushButton_blocked_list, &QPushButton::clicked, this, [this](){ goToPage(Page::Sites); });
    connect(ui->pushButton_settings, &QPushButton::clicked, this, [this](){ goToPage(Page::GeneralSettings); });
    connect(ui->pushButton_server_settings, &QPushButton::clicked, this, [this](){ goToPage(Page::ServerSettings); });
    connect(ui->pushButton_share_connection, &QPushButton::clicked, this, [this](){ goToPage(Page::ShareConnection); });


    connect(ui->pushButton_back_from_sites, &QPushButton::clicked, this, [this](){ goToPage(Page::Vpn); });
    connect(ui->pushButton_back_from_settings, &QPushButton::clicked, this, [this](){ goToPage(Page::Vpn); });
    connect(ui->pushButton_back_from_new_server, &QPushButton::clicked, this, [this](){ goToPage(Page::Start); });
    connect(ui->pushButton_back_from_server_settings, &QPushButton::clicked, this, [this](){ goToPage(Page::GeneralSettings); });
    connect(ui->pushButton_back_from_share, &QPushButton::clicked, this, [this](){ goToPage(Page::GeneralSettings); });

}

void MainWindow::setTrayState(VpnProtocol::ConnectionState state)
{
    QString resourcesPath = ":/images/tray/%1";

    m_trayActionDisconnect->setEnabled(state == VpnProtocol::ConnectionState::Connected);
    m_trayActionConnect->setEnabled(state == VpnProtocol::ConnectionState::Disconnected);

    switch (state) {
    case VpnProtocol::ConnectionState::Disconnected:
        setTrayIcon(QString(resourcesPath).arg(DisconnectedTrayIconName));
        break;
    case VpnProtocol::ConnectionState::Preparing:
        setTrayIcon(QString(resourcesPath).arg(DisconnectedTrayIconName));
        break;
    case VpnProtocol::ConnectionState::Connecting:
        setTrayIcon(QString(resourcesPath).arg(DisconnectedTrayIconName));
        break;
    case VpnProtocol::ConnectionState::Connected:
        setTrayIcon(QString(resourcesPath).arg(ConnectedTrayIconName));
        break;
    case VpnProtocol::ConnectionState::Disconnecting:
        setTrayIcon(QString(resourcesPath).arg(DisconnectedTrayIconName));
        break;
    case VpnProtocol::ConnectionState::TunnelReconnecting:
        setTrayIcon(QString(resourcesPath).arg(DisconnectedTrayIconName));
        break;
    case VpnProtocol::ConnectionState::Error:
        setTrayIcon(QString(resourcesPath).arg(ErrorTrayIconName));
        break;
    case VpnProtocol::ConnectionState::Unknown:
    default:
        setTrayIcon(QString(resourcesPath).arg(DisconnectedTrayIconName));
    }

    //#ifdef Q_OS_MAC
    //    // Get theme from current user (note, this app can be launched as root application and in this case this theme can be different from theme of real current user )
    //    bool darkTaskBar = MacOSFunctions::instance().isMenuBarUseDarkTheme();
    //    darkTaskBar = forceUseBrightIcons ? true : darkTaskBar;
    //    resourcesPath = ":/images_mac/tray_icon/%1";
    //    useIconName = useIconName.replace(".png", darkTaskBar ? "@2x.png" : " dark@2x.png");
    //#endif

}

void MainWindow::onTrayActivated(QSystemTrayIcon::ActivationReason reason)
{
    if(reason == QSystemTrayIcon::DoubleClick || reason == QSystemTrayIcon::Trigger) {
        show();
        raise();
        setWindowState(Qt::WindowActive);
    }
}

void MainWindow::onConnect()
{
    ui->label_error_text->clear();
    ui->pushButton_connect->setChecked(true);
    qApp->processEvents();

    // TODO: Call connectToVpn with restricted server account
    ServerCredentials credentials = m_settings->serverCredentials();

    ErrorCode errorCode = m_vpnConnection->connectToVpn(credentials);
    if (errorCode) {
        //ui->pushButton_connect->setChecked(false);
        QMessageBox::critical(this, APPLICATION_NAME, errorString(errorCode));
        return;
    }

    ui->pushButton_connect->setEnabled(false);
}

void MainWindow::onDisconnect()
{
    ui->pushButton_connect->setChecked(false);
    m_vpnConnection->disconnectFromVpn();
}

void MainWindow::onTrayActionConnect()
{
    if(m_trayActionConnect->text() == tr("Connect")) {
        onConnect();
    } else if(m_trayActionConnect->text() == tr("Disconnect")) {
        onDisconnect();
    }
}

