#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <QMenu>
#include <QMessageBox>
#include <QMetaEnum>
#include <QSysInfo>
#include <QThread>
#include <QTimer>

#include <protocols/shadowsocksvpnprotocol.h>

#include "core/errorstrings.h"
#include "core/openvpnconfigurator.h"
#include "core/servercontroller.h"
#include "ui/qautostart.h"

#include "debug.h"
#include "defines.h"
#include "mainwindow.h"
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
    m_vpnConnection(nullptr)
{
    ui->setupUi(this);

    setupTray();
    setupUiConnections();

    ui->label_error_text->clear();
    ui->widget_tittlebar->installEventFilter(this);

    ui->stackedWidget_main->setSpeed(200);
    ui->stackedWidget_main->setAnimation(QEasingCurve::Linear);

    bool needToHideCustomTitlebar = false;
    if (QOperatingSystemVersion::current() <= QOperatingSystemVersion::Windows7) {
        needToHideCustomTitlebar = true;
    }

#ifdef Q_OS_MAC
    fixWidget(this);
    needToHideCustomTitlebar = true;
#endif

    if (needToHideCustomTitlebar) {
        ui->widget_tittlebar->hide();
        resize(width(), height() - ui->stackedWidget_main->y());
        ui->stackedWidget_main->move(0,0);
    }

    // Post initialization

    if (m_settings.haveAuthData()) {
        goToPage(Page::Vpn, true, false);
    } else {
        goToPage(Page::Start, true, false);
    }

    connect(ui->lineEdit_sites_add_custom, &QLineEdit::returnPressed, [&](){
        ui->pushButton_sites_add_custom->click();
    });

    updateSettings();

    //ui->pushButton_general_settings_exit->hide();

    setFixedSize(width(),height());

    qInfo().noquote() << QString("Started %1 version %2").arg(APPLICATION_NAME).arg(APP_VERSION);
    qInfo().noquote() << QString("%1 (%2)").arg(QSysInfo::prettyProductName()).arg(QSysInfo::currentCpuArchitecture());

    Utils::initializePath(Utils::configPath());

    m_vpnConnection = new VpnConnection(this);
    connect(m_vpnConnection, SIGNAL(bytesChanged(quint64, quint64)), this, SLOT(onBytesChanged(quint64, quint64)));
    connect(m_vpnConnection, SIGNAL(connectionStateChanged(VpnProtocol::ConnectionState)), this, SLOT(onConnectionStateChanged(VpnProtocol::ConnectionState)));
    connect(m_vpnConnection, SIGNAL(vpnProtocolError(amnezia::ErrorCode)), this, SLOT(onVpnProtocolError(amnezia::ErrorCode)));

    onConnectionStateChanged(VpnProtocol::ConnectionState::Disconnected);

    if (m_settings.isAutoConnect() && m_settings.haveAuthData()) {
        QTimer::singleShot(1000, this, [this](){
            ui->pushButton_connect->setEnabled(false);
            onConnect();
        });
    }

    qDebug().noquote() << QString("Default config: %1").arg(Utils::defaultVpnConfigFileName());

    m_ipAddressValidator.setRegExp(Utils::ipAddressRegExp());
    m_ipAddressPortValidator.setRegExp(Utils::ipAddressPortRegExp());

    ui->lineEdit_new_server_ip->setValidator(&m_ipAddressPortValidator);
    ui->lineEdit_network_settings_dns1->setValidator(&m_ipAddressValidator);
    ui->lineEdit_network_settings_dns2->setValidator(&m_ipAddressValidator);
}

MainWindow::~MainWindow()
{
    hide();

    m_vpnConnection->disconnectFromVpn();
    for (int i = 0; i < 50; i++) {
        qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
        QThread::msleep(100);
        if (m_vpnConnection->isDisconnected()) {
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
                                                      .arg(m_settings.userName())
                                                      .arg(m_settings.serverName())
                                                      .arg(m_settings.serverPort()));
        }
        if (page == Page::ShareConnection) {
            QJsonObject ssConfig = ShadowSocksVpnProtocol::genShadowSocksConfig(m_settings.serverCredentials());

            QString ssString = QString("%1:%2@%3:%4")
                    .arg(ssConfig.value("method").toString())
                    .arg(ssConfig.value("password").toString())
                    .arg(ssConfig.value("server").toString())
                    .arg(QString::number(ssConfig.value("server_port").toInt()));

            ssString = "ss://" + ssString.toUtf8().toBase64();
            ui->lineEdit_share_ss_string->setText(ssString);
            updateQRCodeImage(ssString, ui->label_share_ss_qr_code);

            ui->label_share_ss_server->setText(ssConfig.value("server").toString());
            ui->label_share_ss_port->setText(QString::number(ssConfig.value("server_port").toInt()));
            ui->label_share_ss_method->setText(ssConfig.value("method").toString());
            ui->label_share_ss_password->setText(ssConfig.value("password").toString());
        }

        ui->pushButton_new_server_connect_key->setChecked(false);
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
    case(Page::AppSettings): return ui->page_app_settings;
    case(Page::NetworkSettings): return ui->page_network_settings;
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
    if (currentPage() == Page::Start || currentPage() == Page::NewServer) qApp->quit();
    else {
        hide();
        event->ignore();
    }
}

void MainWindow::showEvent(QShowEvent *event)
{
#ifdef Q_OS_MACX
    if (!event->spontaneous()) {
        setDockIconVisible(true);
    }
#endif
}

void MainWindow::hideEvent(QHideEvent *event)
{
#ifdef Q_OS_MACX
    if (!event->spontaneous()) {
        setDockIconVisible(false);
    }
#endif
}

void MainWindow::onPushButtonNewServerConnectWithNewData(bool)
{
    if (ui->pushButton_new_server_connect_key->isChecked()){
        if (ui->lineEdit_new_server_ip->text().isEmpty() ||
                ui->lineEdit_new_server_login->text().isEmpty() ||
                ui->textEdit_new_server_ssh_key->toPlainText().isEmpty() ) {

            ui->label_new_server_wait_info->setText(tr("Please fill in all fields"));
            return;
        }
    }
    else {
        if (ui->lineEdit_new_server_ip->text().isEmpty() ||
                ui->lineEdit_new_server_login->text().isEmpty() ||
                ui->lineEdit_new_server_password->text().isEmpty() ) {

            ui->label_new_server_wait_info->setText(tr("Please fill in all fields"));
            return;
        }
    }


    qDebug() << "Start connection with new data";

    ServerCredentials serverCredentials;
    serverCredentials.hostName = ui->lineEdit_new_server_ip->text();
    if (serverCredentials.hostName.contains(":")) {
        serverCredentials.port = serverCredentials.hostName.split(":").at(1).toInt();
        serverCredentials.hostName = serverCredentials.hostName.split(":").at(0);
    }
    serverCredentials.userName = ui->lineEdit_new_server_login->text();
    if (ui->pushButton_new_server_connect_key->isChecked()){
        QString key = ui->textEdit_new_server_ssh_key->toPlainText();
        if (key.contains("OPENSSH") && key.contains("BEGIN") && key.contains("PRIVATE KEY")) {
            key = OpenVpnConfigurator::convertOpenSShKey(key);
        }

        serverCredentials.password = key;
    }
    else {
        serverCredentials.password = ui->lineEdit_new_server_password->text();
    }


    bool ok = installServer(serverCredentials,
                            ui->page_new_server,
                            ui->progressBar_new_server_connection,
                            ui->pushButton_new_server_connect_with_new_data,
                            ui->label_new_server_wait_info);

    if (ok) {
        m_settings.setServerCredentials(serverCredentials);

        goToPage(Page::Vpn);
        qApp->processEvents();

        onConnect();
    }
}

void MainWindow::onPushButtonNewServerConnectWithExistingCode(bool)
{
    QString s = ui->lineEdit_start_existing_code->text();
    s.replace("vpn://", "");
    QJsonObject o = QJsonDocument::fromJson(QByteArray::fromBase64(s.toUtf8(), QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals)).object();

    ServerCredentials credentials;
    credentials.hostName = o.value("h").toString();
    credentials.port = o.value("p").toInt();
    credentials.userName = o.value("u").toString();
    credentials.password = o.value("w").toString();

    qDebug() << QString("Added server %3@%1:%2").
                arg(credentials.hostName).
                arg(credentials.port).
                arg(credentials.userName);

    //qDebug() << QString("Password") << credentials.password;

    if (!credentials.isValid()) {
        return;
    }

    m_settings.setServerCredentials(credentials);

    goToPage(Page::Vpn);
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


    ErrorCode e = ServerController::setupServer(credentials, Protocol::Any);
    qDebug() << "Setup server finished with code" << e;
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
    installServer(m_settings.serverCredentials(),
                  ui->page_server_settings,
                  ui->progressBar_server_settings_reinstall,
                  ui->pushButton_server_settings_reinstall,
                  ui->label_server_settings_wait_info);
}

void MainWindow::onPushButtonClearServer(bool)
{
    onDisconnect();

    ErrorCode e = ServerController::removeServer(m_settings.serverCredentials(), Protocol::Any);
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

    m_settings.setUserName("");
    m_settings.setPassword("");
    m_settings.setServerName("");
    m_settings.setServerPort();

    goToPage(Page::Start);
}

void MainWindow::onBytesChanged(quint64 receivedData, quint64 sentData)
{
    ui->label_speed_received->setText(VpnConnection::bytesPerSecToText(receivedData));
    ui->label_speed_sent->setText(VpnConnection::bytesPerSecToText(sentData));
}

void MainWindow::onConnectionStateChanged(VpnProtocol::ConnectionState state)
{
    qDebug() << "MainWindow::onConnectionStateChanged" << VpnProtocol::textConnectionState(state);

    bool pushButtonConnectEnabled = false;
    bool radioButtonsModeEnabled = false;
    ui->label_state->setText(VpnProtocol::textConnectionState(state));

    setTrayState(state);

    switch (state) {
    case VpnProtocol::ConnectionState::Disconnected:
        onBytesChanged(0,0);
        ui->pushButton_connect->setChecked(false);
        pushButtonConnectEnabled = true;
        radioButtonsModeEnabled = true;
        break;
    case VpnProtocol::ConnectionState::Preparing:
        pushButtonConnectEnabled = false;
        radioButtonsModeEnabled = false;
        break;
    case VpnProtocol::ConnectionState::Connecting:
        pushButtonConnectEnabled = false;
        radioButtonsModeEnabled = false;
        break;
    case VpnProtocol::ConnectionState::Connected:
        pushButtonConnectEnabled = true;
        radioButtonsModeEnabled = false;
        break;
    case VpnProtocol::ConnectionState::Disconnecting:
        pushButtonConnectEnabled = false;
        radioButtonsModeEnabled = false;
        break;
    case VpnProtocol::ConnectionState::Reconnecting:
        pushButtonConnectEnabled = true;
        radioButtonsModeEnabled = false;
        break;
    case VpnProtocol::ConnectionState::Error:
        ui->pushButton_connect->setChecked(false);
        pushButtonConnectEnabled = true;
        radioButtonsModeEnabled = true;
        break;
    case VpnProtocol::ConnectionState::Unknown:
        pushButtonConnectEnabled = true;
        radioButtonsModeEnabled = true;
    }

    ui->pushButton_connect->setEnabled(pushButtonConnectEnabled);
    ui->radioButton_mode_all_sites->setEnabled(radioButtonsModeEnabled);
    ui->radioButton_mode_selected_sites->setEnabled(radioButtonsModeEnabled);
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

    m_menu->addAction(QIcon(":/images/tray/application.png"), tr("Show") + " " + APPLICATION_NAME, this, [this](){
        show();
        raise();
    });
    m_menu->addSeparator();
    m_trayActionConnect = m_menu->addAction(tr("Connect"), this, SLOT(onConnect()));
    m_trayActionDisconnect = m_menu->addAction(tr("Disconnect"), this, SLOT(onDisconnect()));

    m_menu->addSeparator();

    m_menu->addAction(QIcon(":/images/tray/link.png"), tr("Visit Website"), [&](){
        QDesktopServices::openUrl(QUrl("https://amnezia.org"));
    });

    m_menu->addAction(QIcon(":/images/tray/cancel.png"), tr("Quit") + " " + APPLICATION_NAME, this, [&](){
//        QMessageBox::question(this, QMessageBox::question(this, tr("Exit"), tr("Do you really want to quit?"), QMessageBox::Yes | QMessageBox::No, );

        QMessageBox msgBox(QMessageBox::Question, tr("Exit"), tr("Do you really want to quit?"),
                           QMessageBox::Yes | QMessageBox::No, this, Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowStaysOnTopHint);
        msgBox.setDefaultButton(QMessageBox::Yes);
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
    connect(ui->pushButton_new_server_connect, SIGNAL(clicked(bool)), this, SLOT(onPushButtonNewServerConnectWithExistingCode(bool)));

    connect(ui->pushButton_server_settings_reinstall, SIGNAL(clicked(bool)), this, SLOT(onPushButtonReinstallServer(bool)));
    connect(ui->pushButton_server_settings_clear, SIGNAL(clicked(bool)), this, SLOT(onPushButtonClearServer(bool)));
    connect(ui->pushButton_server_settings_forget, SIGNAL(clicked(bool)), this, SLOT(onPushButtonForgetServer(bool)));

    connect(ui->pushButton_vpn_add_site, &QPushButton::clicked, this, [this](){ goToPage(Page::Sites); });
    connect(ui->pushButton_settings, &QPushButton::clicked, this, [this](){ goToPage(Page::GeneralSettings); });
    connect(ui->pushButton_app_settings, &QPushButton::clicked, this, [this](){ goToPage(Page::AppSettings); });
    connect(ui->pushButton_network_settings, &QPushButton::clicked, this, [this](){ goToPage(Page::NetworkSettings); });
    connect(ui->pushButton_server_settings, &QPushButton::clicked, this, [this](){ goToPage(Page::ServerSettings); });
    connect(ui->pushButton_share_connection, &QPushButton::clicked, this, [this](){
        goToPage(Page::ShareConnection);
        updateShareCode();
    });

    connect(ui->pushButton_copy_sharing_code, &QPushButton::clicked, this, [this](){
        QGuiApplication::clipboard()->setText(ui->textEdit_sharing_code->toPlainText());
        ui->pushButton_copy_sharing_code->setText(tr("Copied"));

        QTimer::singleShot(3000, this, [this]() {
            ui->pushButton_copy_sharing_code->setText(tr("Copy"));
        });
    });


    connect(ui->pushButton_back_from_sites, &QPushButton::clicked, this, [this](){ goToPage(Page::Vpn); });
    connect(ui->pushButton_back_from_settings, &QPushButton::clicked, this, [this](){ goToPage(Page::Vpn); });
    connect(ui->pushButton_back_from_new_server, &QPushButton::clicked, this, [this](){ goToPage(Page::Start); });
    connect(ui->pushButton_back_from_app_settings, &QPushButton::clicked, this, [this](){ goToPage(Page::GeneralSettings); });
    connect(ui->pushButton_back_from_network_settings, &QPushButton::clicked, this, [this](){ goToPage(Page::GeneralSettings); });
    connect(ui->pushButton_back_from_server_settings, &QPushButton::clicked, this, [this](){ goToPage(Page::GeneralSettings); });
    connect(ui->pushButton_back_from_share, &QPushButton::clicked, this, [this](){ goToPage(Page::GeneralSettings); });

    connect(ui->pushButton_sites_add_custom, &QPushButton::clicked, this, [this](){ onPushButtonAddCustomSitesClicked(); });

    connect(ui->radioButton_mode_selected_sites, &QRadioButton::toggled, ui->pushButton_vpn_add_site, &QPushButton::setEnabled);

    connect(ui->radioButton_mode_selected_sites, &QRadioButton::toggled, this, [this](bool toggled) {
        m_settings.setCustomRouting(toggled);
    });

    connect(ui->checkBox_autostart, &QCheckBox::stateChanged, this, [this](int state){
        if (state == Qt::Unchecked) {
            ui->checkBox_autoconnect->setChecked(false);
        }
        Autostart::setAutostart(state == Qt::Checked);
    });

    connect(ui->checkBox_autoconnect, &QCheckBox::stateChanged, this, [this](int state){
        m_settings.setAutoConnect(state == Qt::Checked);
    });

    connect(ui->pushButton_network_settings_resetdns1, &QPushButton::clicked, this, [this](){
        m_settings.setPrimaryDns(m_settings.cloudFlareNs1());
        updateSettings();
    });
    connect(ui->pushButton_network_settings_resetdns2, &QPushButton::clicked, this, [this](){
        m_settings.setSecondaryDns(m_settings.cloudFlareNs2());
        updateSettings();
    });

    connect(ui->lineEdit_network_settings_dns1, &QLineEdit::textEdited, this, [this](const QString &newText){
         if (m_ipAddressValidator.regExp().exactMatch(newText)) {
             m_settings.setPrimaryDns(newText);
         }
    });
    connect(ui->lineEdit_network_settings_dns2, &QLineEdit::textEdited, this, [this](const QString &newText){
         if (m_ipAddressValidator.regExp().exactMatch(newText)) {
             m_settings.setSecondaryDns(newText);
         }
    });

    connect(ui->pushButton_new_server_connect_key, &QPushButton::toggled, this, [this](bool checked){
        ui->label_new_server_password->setText(checked ? tr("Private key") : tr("Password"));
        ui->pushButton_new_server_connect_key->setText(checked ? tr("Connect using SSH password") : tr("Connect using SSH key"));
        ui->lineEdit_new_server_password->setVisible(!checked);
        ui->textEdit_new_server_ssh_key->setVisible(checked);
    });

    connect(ui->pushButton_check_for_updates, &QPushButton::clicked, this, [this](){
        QDesktopServices::openUrl(QUrl("https://github.com/amnezia-vpn/desktop-client/releases"));
    });

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
    case VpnProtocol::ConnectionState::Reconnecting:
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
#ifndef Q_OS_MAC
    if(reason == QSystemTrayIcon::DoubleClick || reason == QSystemTrayIcon::Trigger) {
        show();
        raise();
        setWindowState(Qt::WindowActive);
    }
#endif
}

void MainWindow::onConnect()
{
    ui->label_error_text->clear();
    ui->pushButton_connect->setChecked(true);
    qApp->processEvents();

    // TODO: Call connectToVpn with restricted server account
    ServerCredentials credentials = m_settings.serverCredentials();

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


void MainWindow::onPushButtonAddCustomSitesClicked()
{
    QString newSite = ui->lineEdit_sites_add_custom->text();

    if (newSite.isEmpty()) return;
    if (!newSite.contains(".")) return;

    // get domain name if it present
    newSite.replace("https://", "");
    newSite.replace("http://", "");
    newSite.replace("ftp://", "");

    newSite = newSite.split("/", QString::SkipEmptyParts).first();


    QStringList customSites = m_settings.customSites();
    if (!customSites.contains(newSite)) {
        customSites.append(newSite);
        m_settings.setCustomSites(customSites);

        QString newIp = Utils::getIPAddress(newSite);
        QStringList customIps = m_settings.customIps();
        if (!newIp.isEmpty() && !customIps.contains(newIp)) {
            customIps.append(newIp);
            m_settings.setCustomIps(customIps);

            if (m_vpnConnection->connectionState() == VpnProtocol::ConnectionState::Connected) {
                IpcClient::Interface()->routeAddList(m_vpnConnection->vpnProtocol()->vpnGateway(),
                    QStringList() << newIp);
            }
        }

        updateSettings();

        ui->lineEdit_sites_add_custom->clear();
    }
    else {
        qDebug() << "customSites already contains" << newSite;
    }
}

void MainWindow::onPushButtonDeleteCustomSiteClicked(const QString &siteToDelete)
{
    if (siteToDelete.isEmpty()) {
        return;
    }

    QString ipToDelete = Utils::getIPAddress(siteToDelete);

    QStringList customSites = m_settings.customSites();
    customSites.removeAll(siteToDelete);
    qDebug() << "Deleted custom site:" << siteToDelete;
    m_settings.setCustomSites(customSites);

    QStringList customIps = m_settings.customIps();
    customIps.removeAll(ipToDelete);
    qDebug() << "Deleted custom ip:" << ipToDelete;
    m_settings.setCustomIps(customIps);

    updateSettings();

    if (m_vpnConnection->connectionState() == VpnProtocol::ConnectionState::Connected) {
        IpcClient::Interface()->routeDelete(ipToDelete);
        IpcClient::Interface()->flushDns();
    }
}

void MainWindow::updateSettings()
{
    ui->radioButton_mode_selected_sites->setChecked(m_settings.customRouting());
    ui->pushButton_vpn_add_site->setEnabled(m_settings.customRouting());

    ui->checkBox_autostart->setChecked(Autostart::isAutostart());
    ui->checkBox_autoconnect->setChecked(m_settings.isAutoConnect());

    ui->lineEdit_network_settings_dns1->setText(m_settings.primaryDns());
    ui->lineEdit_network_settings_dns2->setText(m_settings.secondaryDns());


    ui->listWidget_sites->clear();
    for(const QString &site : m_settings.customSites()) {
        makeSitesListItem(ui->listWidget_sites, site);
    }
}

void MainWindow::updateShareCode()
{
    QJsonObject o;
    o.insert("h", m_settings.serverName());
    o.insert("p", m_settings.serverPort());
    o.insert("u", m_settings.userName());
    o.insert("w", m_settings.password());

    QByteArray ba = QJsonDocument(o).toJson().toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
    ui->textEdit_sharing_code->setText(QString("vpn://%1").arg(QString(ba)));

    //qDebug() << "Share code" << QJsonDocument(o).toJson();
}

void MainWindow::makeSitesListItem(QListWidget *listWidget, const QString &address)
{
    QSize size(310, 25);
    QWidget* widget = new QWidget;
    widget->resize(size);

    QLabel *label = new QLabel(address, widget);
    label->resize(size);

    QPushButton* btn = new QPushButton(widget);
    btn->resize(size);

    QPushButton* btn1 = new QPushButton(widget);
    btn1->resize(30, 25);
    btn1->move(280, 0);
    btn1->setCursor(QCursor(Qt::PointingHandCursor));

    connect(btn1, &QPushButton::clicked, this, [this, label]() {
        onPushButtonDeleteCustomSiteClicked(label->text());
        return;
    });

    QListWidgetItem* item = new QListWidgetItem(listWidget);
    item->setSizeHint(size);
    listWidget->setItemWidget(item, widget);

    widget->setStyleSheet(styleSheet());
}

void MainWindow::updateQRCodeImage(const QString &text, QLabel *label)
{
    int levelIndex = 1;
    int versionIndex = 0;
    bool bExtent = true;
    int maskIndex = -1;

    m_qrEncode.EncodeData( levelIndex, versionIndex, bExtent, maskIndex, text.toUtf8().data() );

    int qrImageSize = m_qrEncode.m_nSymbleSize;

    int encodeImageSize = qrImageSize + ( QR_MARGIN * 2 );
    QImage encodeImage( encodeImageSize, encodeImageSize, QImage::Format_Mono );

    encodeImage.fill( 1 );

    for ( int i = 0; i < qrImageSize; i++ )
        for ( int j = 0; j < qrImageSize; j++ )
            if ( m_qrEncode.m_byModuleData[i][j] )
                encodeImage.setPixel( i + QR_MARGIN, j + QR_MARGIN, 0 );

    label->setPixmap(QPixmap::fromImage(encodeImage.scaledToWidth(label->width())));
}
