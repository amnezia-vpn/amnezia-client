#include <QApplication>
#include <QClipboard>
#include <QDebug>
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
#include "configurators/openvpn_configurator.h"
#include "core/servercontroller.h"
#include "core/server_defs.h"
#include "protocols/protocols_defs.h"
#include "ui/qautostart.h"

#include "debug.h"
#include "defines.h"
#include "mainwindow.h"
#include "utils.h"
#include "vpnconnection.h"
#include "ui_mainwindow.h"
#include "ui/server_widget.h"
#include "ui_server_widget.h"

#ifdef Q_OS_MAC
#include "ui/macos_util.h"
#endif

using namespace amnezia;

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
    setupProtocolsPageConnections();
    setupNewServerPageConnections();

    ui->label_error_text->clear();
    installEventFilter(this);
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
    goToPage(Page::Start, true, false);

    if (m_settings.haveAuthData()) {
        goToPage(Page::Vpn, true, false);
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
    m_ipNetwok24Validator.setRegExp(Utils::ipNetwork24RegExp());

    ui->lineEdit_new_server_ip->setValidator(&m_ipAddressPortValidator);
    ui->lineEdit_network_settings_dns1->setValidator(&m_ipAddressValidator);
    ui->lineEdit_network_settings_dns2->setValidator(&m_ipAddressValidator);

    ui->lineEdit_proto_openvpn_subnet->setValidator(&m_ipNetwok24Validator);

    ui->toolBox_share_connection->removeItem(ui->toolBox_share_connection->indexOf(ui->page_share_shadowsocks));
    ui->page_share_shadowsocks->setVisible(false);
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
    qDebug() << "goToPage" << page;
    if (reset) {
        if (page == Page::ServerSettings) {
            updateServerPage();
        }
        if (page == Page::ShareConnection) {
//            QJsonObject ssConfig = ShadowSocksVpnProtocol::genShadowSocksConfig(m_settings.defaultServerCredentials());

//            QString ssString = QString("%1:%2@%3:%4")
//                    .arg(ssConfig.value("method").toString())
//                    .arg(ssConfig.value("password").toString())
//                    .arg(ssConfig.value("server").toString())
//                    .arg(QString::number(ssConfig.value("server_port").toInt()));

//            ssString = "ss://" + ssString.toUtf8().toBase64();
//            ui->lineEdit_share_ss_string->setText(ssString);
//            updateQRCodeImage(ssString, ui->label_share_ss_qr_code);

//            ui->label_share_ss_server->setText(ssConfig.value("server").toString());
//            ui->label_share_ss_port->setText(QString::number(ssConfig.value("server_port").toInt()));
//            ui->label_share_ss_method->setText(ssConfig.value("method").toString());
//            ui->label_share_ss_password->setText(ssConfig.value("password").toString());
        }
        if (page == Page::ServersList) {
            updateServersListPage();
        }
        if (page == Page::Start) {
            ui->label_new_server_wait_info->hide();
            ui->label_new_server_wait_info->clear();

            ui->progressBar_new_server_connection->setMinimum(0);
            ui->progressBar_new_server_connection->setMaximum(300);

            ui->pushButton_back_from_start->setVisible(!pagesStack.isEmpty());
        }
        if (page == Page::NewServer_2) {
            ui->pushButton_new_server_settings_cloak->setChecked(true);
            ui->pushButton_new_server_settings_cloak->setChecked(false);
            ui->pushButton_new_server_settings_ss->setChecked(true);
            ui->pushButton_new_server_settings_ss->setChecked(false);
            ui->pushButton_new_server_settings_openvpn->setChecked(true);
            ui->pushButton_new_server_settings_openvpn->setChecked(false);

            ui->lineEdit_new_server_cloak_port->setText(amnezia::protocols::cloak::defaultPort);
            ui->lineEdit_new_server_cloak_site->setText(amnezia::protocols::cloak::defaultRedirSite);

            ui->lineEdit_new_server_ss_port->setText(amnezia::protocols::shadowsocks::defaultPort);
            ui->comboBox_new_server_ss_cipher->setCurrentText(amnezia::protocols::shadowsocks::defaultCipher);

            ui->lineEdit_new_server_openvpn_port->setText(amnezia::protocols::openvpn::defaultPort);
            ui->comboBox_new_server_openvpn_proto->setCurrentText(amnezia::protocols::openvpn::defaultTransportProto);
        }
        if (page == Page::ServerVpnProtocols) {
            updateProtocolsPage();
        }
        if (page == Page::AppSettings) {
            updateAppSettingsPage();
        }
        if (page == Page::Sites) {
            updateSitesPage();
        }
        if (page == Page::Vpn) {
            updateVpnPage();
        }

        ui->pushButton_new_server_connect_key->setChecked(false);
    }

    if (slide)
        ui->stackedWidget_main->slideInWidget(getPageWidget(page), SlidingStackedWidget::RIGHT2LEFT);
    else
        ui->stackedWidget_main->setCurrentWidget(getPageWidget(page));

    pagesStack.push(page);
}

void MainWindow::closePage()
{
    Page prev = pagesStack.pop();
    //qDebug() << "closePage" << prev << "Set page" << pagesStack.top();
    ui->stackedWidget_main->slideInWidget(getPageWidget(pagesStack.top()), SlidingStackedWidget::LEFT2RIGHT);
}

QWidget *MainWindow::getPageWidget(MainWindow::Page page)
{
    switch (page) {
    case(Page::Start): return ui->page_start;
    case(Page::NewServer): return ui->page_new_server;
    case(Page::NewServer_2): return ui->page_new_server_2;
    case(Page::Vpn): return ui->page_vpn;
    case(Page::GeneralSettings): return ui->page_general_settings;
    case(Page::AppSettings): return ui->page_app_settings;
    case(Page::NetworkSettings): return ui->page_network_settings;
    case(Page::ServerSettings): return ui->page_server_settings;
    case(Page::ServerVpnProtocols): return ui->page_server_protocols;
    case(Page::ServersList): return ui->page_servers;
    case(Page::ShareConnection): return ui->page_share_connection;
    case(Page::Sites): return ui->page_sites;
    case(Page::OpenVpnSettings): return ui->page_proto_openvpn;
    case(Page::ShadowSocksSettings): return ui->page_proto_shadowsocks;
    case(Page::CloakSettings): return ui->page_proto_cloak;
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
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Escape && ! ui->stackedWidget_main->isAnimationRunning() ) {
            if (currentPage() != Page::Vpn && currentPage() != Page::Start) {
                closePage();
            }
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
#ifdef QT_DEBUG
    case Qt::Key_Q:
        qApp->quit();
    case Qt::Key_C:
        qDebug().noquote() << QJsonDocument(m_settings.serversArray()).toJson();
        break;
#endif
    case Qt::Key_A:
        goToPage(Page::Start);
        break;
    case Qt::Key_S:
        goToPage(Page::ServerSettings);
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

void MainWindow::onPushButtonNewServerConnect(bool)
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


    qDebug() << "MainWindow::onPushButtonNewServerConnect checking new server";


    ServerCredentials serverCredentials;
    serverCredentials.hostName = ui->lineEdit_new_server_ip->text();
    if (serverCredentials.hostName.contains(":")) {
        serverCredentials.port = serverCredentials.hostName.split(":").at(1).toInt();
        serverCredentials.hostName = serverCredentials.hostName.split(":").at(0);
    }
    serverCredentials.userName = ui->lineEdit_new_server_login->text();
    if (ui->pushButton_new_server_connect_key->isChecked()){
        QString key = ui->textEdit_new_server_ssh_key->toPlainText();
        if (key.startsWith("ssh-rsa")) {
            QMessageBox::warning(this, APPLICATION_NAME,
                                 tr("It's public key. Private key required"));

            return;
        }

        if (key.contains("OPENSSH") && key.contains("BEGIN") && key.contains("PRIVATE KEY")) {
            key = OpenVpnConfigurator::convertOpenSShKey(key);
        }

        serverCredentials.password = key;
    }
    else {
        serverCredentials.password = ui->lineEdit_new_server_password->text();
    }

    ui->pushButton_new_server_connect->setEnabled(false);
    ui->pushButton_new_server_connect->setText(tr("Connecting..."));
    ErrorCode e = ErrorCode::NoError;
    //QString output = ServerController::checkSshConnection(serverCredentials, &e);
    QString output;
    bool ok = true;
    if (e) {
        ui->label_new_server_wait_info->show();
        ui->label_new_server_wait_info->setText(errorString(e));
        ok = false;
    }
    else {
        if (output.contains("Please login as the user")) {
            output.replace("\n", "");
            ui->label_new_server_wait_info->show();
            ui->label_new_server_wait_info->setText(output);
            ok = false;
        }
    }

    ui->pushButton_new_server_connect->setEnabled(true);
    ui->pushButton_new_server_connect->setText(tr("Connect"));

    installCredentials = serverCredentials;
    if (ok) goToPage(Page::NewServer_2);
}

void MainWindow::onPushButtonNewServerConnectConfigure(bool)
{
    QJsonObject cloakConfig {
                    { config_key::port, ui->lineEdit_new_server_cloak_port->text() },
                    { config_key::container, amnezia::containerToString(DockerContainer::OpenVpnOverCloak) },
                    { config_key::cloak, QJsonObject {
                        { config_key::site, ui->lineEdit_new_server_cloak_site->text() }}
                    }
    };
    QJsonObject ssConfig {
                    { config_key::port, ui->lineEdit_new_server_ss_port->text() },
                    { config_key::container, amnezia::containerToString(DockerContainer::OpenVpnOverShadowSocks) },
                    { config_key::shadowsocks, QJsonObject {
                        { config_key::cipher, ui->comboBox_new_server_ss_cipher->currentText() }}
                    }
    };
    QJsonObject openVpnConfig {
                    { config_key::port, ui->lineEdit_new_server_openvpn_port->text() },
                    { config_key::container, amnezia::containerToString(DockerContainer::OpenVpn) },
                    { config_key::openvpn, QJsonObject {
                        { config_key::transport_proto, ui->comboBox_new_server_openvpn_proto->currentText() }}
                    }
    };

    QJsonArray containerConfigs;
    QList<DockerContainer> containers;

    if (ui->checkBox_new_server_cloak->isChecked()) {
        containerConfigs.append(cloakConfig);
        containers.append(DockerContainer::OpenVpnOverCloak);
    }

    if (ui->checkBox_new_server_ss->isChecked()) {
        containerConfigs.append(ssConfig);
        containers.append(DockerContainer::OpenVpnOverShadowSocks);
    }

    if (ui->checkBox_new_server_openvpn->isChecked()) {
        containerConfigs.append(openVpnConfig);
        containers.append(DockerContainer::OpenVpnOverShadowSocks);
    }

//    bool ok = true;
    bool ok = installServer(installCredentials, containers, containerConfigs,
                            ui->page_new_server_2,
                            ui->progressBar_new_server_connection,
                            ui->pushButton_new_server_connect,
                            ui->label_new_server_wait_info);

    if (ok) {
        QJsonObject server;
        server.insert(config_key::hostName, installCredentials.hostName);
        server.insert(config_key::userName, installCredentials.userName);
        server.insert(config_key::password, installCredentials.password);
        server.insert(config_key::port, installCredentials.port);
        server.insert(config_key::description, m_settings.nextAvailableServerName());

        server.insert(config_key::containers, containerConfigs);

        m_settings.addServer(server);
        updateSettings();

        goToPage(Page::Vpn);
        qApp->processEvents();

        //onConnect();
    }
}

void MainWindow::onPushButtonNewServerImport(bool)
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

    //m_settings.setServerCredentials(credentials);

    goToPage(Page::Vpn);
}

bool MainWindow::installServer(ServerCredentials credentials,
    QList<DockerContainer> containers, QJsonArray configs,
    QWidget *page, QProgressBar *progress, QPushButton *button, QLabel *info)
{
    page->setEnabled(false);
    button->setVisible(false);

    info->setVisible(true);
    info->setText(tr("Please wait, configuring process may take up to 5 minutes"));


    for (int i = 0; i < containers.size(); ++i) {
        QTimer timer;
        connect(&timer, &QTimer::timeout, [progress](){
            progress->setValue(progress->value() + 1);
        });

        progress->setValue(0);
        timer.start(1000);

        progress->setTextVisible(true);
        progress->setFormat(QString("%1%2%3").arg(i+1).arg(tr("of")).arg(containers.size()));

        ErrorCode e = ServerController::setupContainer(credentials, containers.at(i), configs.at(i).toObject());
        qDebug() << "Setup server finished with code" << e;
        ServerController::disconnectFromHost(credentials);

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
    }




//    button->show();
//    page->setEnabled(true);
//    info->setText(tr("Amnezia server installed"));

    return true;
}

ErrorCode MainWindow::doInstallAction(const std::function<ErrorCode()> &action, QWidget *page, QProgressBar *progress, QPushButton *button, QLabel *info)
{
    progress->show();
    if (page) page->setEnabled(false);
    if (button) button->setVisible(false);

    if (info) info->setVisible(true);
    if (info) info->setText(tr("Please wait, configuring process may take up to 5 minutes"));


    QTimer timer;
    connect(&timer, &QTimer::timeout, [progress](){
        progress->setValue(progress->value() + 1);
    });

    progress->setValue(0);
    timer.start(1000);

    ErrorCode e = action();
    qDebug() << "doInstallAction finished with code" << e;

    if (e) {
        if (page) page->setEnabled(true);
        if (button) button->setVisible(true);
        if (info) info->setVisible(false);

        QMessageBox::warning(this, APPLICATION_NAME,
                             tr("Error occurred while configuring server.") + "\n" +
                             errorString(e));

        progress->hide();
        return e;
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


    progress->hide();
    if (button) button->show();
    if (page) page->setEnabled(true);
    if (info) info->setText(tr("Operation finished"));

    return ErrorCode::NoError;
}

void MainWindow::onPushButtonReinstallServer(bool)
{
//    onDisconnect();
//    installServer(m_settings.defaultServerCredentials(),
//                  ui->page_server_settings,
//                  ui->progressBar_server_settings_reinstall,
//                  ui->pushButton_server_settings_reinstall,
//                  ui->label_server_settings_wait_info);
}

void MainWindow::onPushButtonClearServer(bool)
{
    ui->page_server_settings->setEnabled(false);
    ui->pushButton_server_settings_clear->setText(tr("Uninstalling Amnezia software..."));

    if (m_settings.defaultServerIndex() == selectedServerIndex) {
        onDisconnect();
    }

    ErrorCode e = ServerController::removeAllContainers(m_settings.serverCredentials(selectedServerIndex));
    ServerController::disconnectFromHost(m_settings.serverCredentials(selectedServerIndex));
    if (e) {
        QMessageBox::warning(this, APPLICATION_NAME,
                             tr("Error occurred while configuring server.") + "\n" +
                             errorString(e) + "\n" +
                             tr("See logs for details."));

    }
    else {
        ui->label_server_settings_wait_info->show();
        ui->label_server_settings_wait_info->setText(tr("Amnezia server successfully uninstalled"));
    }

    ui->page_server_settings->setEnabled(true);
    ui->pushButton_server_settings_clear->setText(tr("Clear server from Amnezia software"));
}

void MainWindow::onPushButtonForgetServer(bool)
{
    if (m_settings.defaultServerIndex() == selectedServerIndex) {
        onDisconnect();
    }
    m_settings.removeServer(selectedServerIndex);
    m_settings.setDefaultServer(0);

    closePage();
    updateServersListPage();
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
    ui->stackedWidget_main->waitForAnimation();

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


    connect(ui->pushButton_new_server_connect, SIGNAL(clicked(bool)), this, SLOT(onPushButtonNewServerConnect(bool)));
    connect(ui->pushButton_new_server_connect_configure, SIGNAL(clicked(bool)), this, SLOT(onPushButtonNewServerConnectConfigure(bool)));
    connect(ui->pushButton_new_server_import, SIGNAL(clicked(bool)), this, SLOT(onPushButtonNewServerImport(bool)));

    connect(ui->pushButton_server_settings_reinstall, SIGNAL(clicked(bool)), this, SLOT(onPushButtonReinstallServer(bool)));
    connect(ui->pushButton_server_settings_clear, SIGNAL(clicked(bool)), this, SLOT(onPushButtonClearServer(bool)));
    connect(ui->pushButton_server_settings_forget, SIGNAL(clicked(bool)), this, SLOT(onPushButtonForgetServer(bool)));

    connect(ui->pushButton_vpn_add_site, &QPushButton::clicked, this, [this](){ goToPage(Page::Sites); });
    connect(ui->pushButton_settings, &QPushButton::clicked, this, [this](){ goToPage(Page::GeneralSettings); });
    connect(ui->pushButton_app_settings, &QPushButton::clicked, this, [this](){ goToPage(Page::AppSettings); });
    connect(ui->pushButton_network_settings, &QPushButton::clicked, this, [this](){ goToPage(Page::NetworkSettings); });
    connect(ui->pushButton_server_settings, &QPushButton::clicked, this, [this](){
        selectedServerIndex = m_settings.defaultServerIndex();
        goToPage(Page::ServerSettings);
    });
    connect(ui->pushButton_server_settings_protocols, &QPushButton::clicked, this, [this](){ goToPage(Page::ServerVpnProtocols); });
    connect(ui->pushButton_servers_list, &QPushButton::clicked, this, [this](){ goToPage(Page::ServersList); });
    connect(ui->pushButton_share_connection, &QPushButton::clicked, this, [this](){
        goToPage(Page::ShareConnection);
        updateShareCodePage();
    });

    connect(ui->pushButton_copy_sharing_code, &QPushButton::clicked, this, [this](){
        QGuiApplication::clipboard()->setText(ui->textEdit_sharing_code->toPlainText());
        ui->pushButton_copy_sharing_code->setText(tr("Copied"));

        QTimer::singleShot(3000, this, [this]() {
            ui->pushButton_copy_sharing_code->setText(tr("Copy"));
        });
    });

    connect(ui->pushButton_back_from_sites, &QPushButton::clicked, this, [this](){ closePage(); });
    connect(ui->pushButton_back_from_settings, &QPushButton::clicked, this, [this](){ closePage(); });
    connect(ui->pushButton_back_from_start, &QPushButton::clicked, this, [this](){ closePage(); });
    connect(ui->pushButton_back_from_new_server, &QPushButton::clicked, this, [this](){ closePage(); });
    connect(ui->pushButton_back_from_new_server_2, &QPushButton::clicked, this, [this](){ closePage(); });
    connect(ui->pushButton_back_from_app_settings, &QPushButton::clicked, this, [this](){ closePage(); });
    connect(ui->pushButton_back_from_network_settings, &QPushButton::clicked, this, [this](){ closePage(); });
    connect(ui->pushButton_back_from_server_settings, &QPushButton::clicked, this, [this](){ closePage(); });
    connect(ui->pushButton_back_from_servers, &QPushButton::clicked, this, [this](){ closePage(); });
    connect(ui->pushButton_back_from_share, &QPushButton::clicked, this, [this](){ closePage(); });
    connect(ui->pushButton_back_from_server_vpn_protocols, &QPushButton::clicked, this, [this](){ closePage(); });

    connect(ui->pushButton_back_from_openvpn_settings, &QPushButton::clicked, this, [this](){ closePage(); });
    connect(ui->pushButton_back_from_cloak_settings, &QPushButton::clicked, this, [this](){ closePage(); });
    connect(ui->pushButton_back_from_shadowsocks_settings, &QPushButton::clicked, this, [this](){ closePage(); });


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
        m_settings.setPrimaryDns(m_settings.cloudFlareNs1);
        updateSettings();
    });
    connect(ui->pushButton_network_settings_resetdns2, &QPushButton::clicked, this, [this](){
        m_settings.setSecondaryDns(m_settings.cloudFlareNs2);
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


    connect(ui->pushButton_check_for_updates, &QPushButton::clicked, this, [this](){
        QDesktopServices::openUrl(QUrl("https://github.com/amnezia-vpn/desktop-client/releases"));
    });

    connect(ui->pushButton_servers_add_new, &QPushButton::clicked, this, [this](){ goToPage(Page::Start); });

    connect(ui->lineEdit_server_settings_description, &QLineEdit::editingFinished, this, [this](){
        const QString &newText = ui->lineEdit_server_settings_description->text();
        QJsonObject server = m_settings.server(selectedServerIndex);
        server.insert(config_key::description, newText);
        m_settings.editServer(selectedServerIndex, server);
        updateServersListPage();
    });

    connect(ui->lineEdit_server_settings_description, &QLineEdit::returnPressed, this, [this](){
        ui->lineEdit_server_settings_description->clearFocus();
    });


}

void MainWindow::setupProtocolsPageConnections()
{
    QJsonObject openvpnConfig;

    // default buttons
    QList<DockerContainer> containers {
        DockerContainer::OpenVpn,
        DockerContainer::OpenVpnOverShadowSocks,
        DockerContainer::OpenVpnOverCloak
    };

    // default buttons
    QList<QPushButton *> defaultButtons {
        ui->pushButton_proto_openvpn_cont_default,
        ui->pushButton_proto_ss_openvpn_cont_default,
        ui->pushButton_proto_cloak_openvpn_cont_default
    };

    for (int i = 0; i < containers.size(); ++i) {
        connect(defaultButtons.at(i), &QPushButton::clicked, this, [this, containers, i](){
            m_settings.setDefaultContainer(selectedServerIndex, containers.at(i));
            updateProtocolsPage();
        });
    }

    // install buttons
    QList<QPushButton *> installButtons {
        ui->pushButton_proto_openvpn_cont_install,
        ui->pushButton_proto_ss_openvpn_cont_install,
        ui->pushButton_proto_cloak_openvpn_cont_install
    };

    for (int i = 0; i < containers.size(); ++i) {
        QPushButton *button = installButtons.at(i);
        DockerContainer container = containers.at(i);

        connect(button, &QPushButton::clicked, this, [this, container, button](bool checked){
            if (checked) {
                ErrorCode e = doInstallAction([this, container](){
                    return ServerController::setupContainer(m_settings.serverCredentials(selectedServerIndex), container);
                },
                ui->page_server_protocols, ui->progressBar_protocols_container_reinstall,
                nullptr, nullptr);

                if (!e) {
                    m_settings.setContainerConfig(selectedServerIndex, container, QJsonObject());
                }
            }
            else {
                button->setEnabled(false);
                ErrorCode e = ServerController::removeContainer(m_settings.serverCredentials(selectedServerIndex), container);
                m_settings.removeContainerConfig(selectedServerIndex, container);
                button->setEnabled(true);
            }

            updateProtocolsPage();
        });
    }


    // settings buttons

    // settings openvpn container
    connect(ui->pushButton_proto_openvpn_cont_openvpn_config, &QPushButton::clicked, this, [this](){
        updateOpenVpnPage(m_settings.protocolConfig(selectedServerIndex, DockerContainer::OpenVpn, Protocol::OpenVpn));
        selectedDockerContainer = DockerContainer::OpenVpn;
        goToPage(Page::OpenVpnSettings);
    });

    // settings shadowsocks container
    connect(ui->pushButton_proto_ss_openvpn_cont_openvpn_config, &QPushButton::clicked, this, [this](){
        updateOpenVpnPage(m_settings.protocolConfig(selectedServerIndex, DockerContainer::OpenVpnOverShadowSocks, Protocol::OpenVpn),
            DockerContainer::OpenVpnOverShadowSocks);
        selectedDockerContainer = DockerContainer::OpenVpnOverShadowSocks;
        goToPage(Page::OpenVpnSettings);
    });
    connect(ui->pushButton_proto_ss_openvpn_cont_ss_config, &QPushButton::clicked, this, [this](){
        updateOpenVpnPage(m_settings.protocolConfig(selectedServerIndex, DockerContainer::OpenVpnOverShadowSocks, Protocol::ShadowSocks));
        selectedDockerContainer = DockerContainer::OpenVpnOverShadowSocks;
        goToPage(Page::ShadowSocksSettings);
    });

    // settings cloak container
    connect(ui->pushButton_proto_cloak_openvpn_cont_openvpn_config, &QPushButton::clicked, this, [this](){
        updateOpenVpnPage(m_settings.protocolConfig(selectedServerIndex, DockerContainer::OpenVpnOverCloak, Protocol::OpenVpn));
        selectedDockerContainer = DockerContainer::OpenVpnOverCloak;
        goToPage(Page::OpenVpnSettings);
    });
    connect(ui->pushButton_proto_cloak_openvpn_cont_ss_config, &QPushButton::clicked, this, [this](){
        updateOpenVpnPage(m_settings.protocolConfig(selectedServerIndex, DockerContainer::OpenVpnOverCloak, Protocol::ShadowSocks));
        selectedDockerContainer = DockerContainer::OpenVpnOverCloak;
        goToPage(Page::ShadowSocksSettings);
    });
    connect(ui->pushButton_proto_cloak_openvpn_cont_cloak_config, &QPushButton::clicked, this, [this](){
        updateOpenVpnPage(m_settings.protocolConfig(selectedServerIndex, DockerContainer::OpenVpnOverCloak, Protocol::Cloak));
        selectedDockerContainer = DockerContainer::OpenVpnOverCloak;
        goToPage(Page::CloakSettings);
    });

    ///
    // Protocols pages
    connect(ui->checkBox_proto_openvpn_auto_encryption, &QCheckBox::stateChanged, this, [this](){
        ui->comboBox_proto_openvpn_cipher->setDisabled(ui->checkBox_proto_openvpn_auto_encryption->isChecked());
        ui->comboBox_proto_openvpn_hash->setDisabled(ui->checkBox_proto_openvpn_auto_encryption->isChecked());
    });

    connect(ui->pushButton_proto_openvpn_save, &QPushButton::clicked, this, [this](){
        QJsonObject protocolConfig = m_settings.protocolConfig(selectedServerIndex, selectedDockerContainer, Protocol::OpenVpn);
        protocolConfig = getOpenVpnConfigFromPage(protocolConfig);

        QJsonObject containerConfig = m_settings.containerConfig(selectedServerIndex, selectedDockerContainer);
        QJsonObject newContainerConfig = containerConfig;
        newContainerConfig.insert(config_key::openvpn, protocolConfig);

        ErrorCode e = doInstallAction([this, containerConfig, newContainerConfig](){
            return ServerController::updateContainer(m_settings.serverCredentials(selectedServerIndex), selectedDockerContainer, containerConfig, newContainerConfig);
        },
           ui->page_proto_openvpn, ui->progressBar_proto_openvpn_reset,
           ui->pushButton_proto_openvpn_save, ui->label_proto_openvpn_info);

        if (!e) {
            m_settings.setContainerConfig(selectedServerIndex, selectedDockerContainer, newContainerConfig);
            m_settings.clearLastConnectionConfig(selectedServerIndex, selectedDockerContainer);
        }
    });
}

void MainWindow::setupNewServerPageConnections()
{
    connect(ui->pushButton_connect, SIGNAL(clicked(bool)), this, SLOT(onPushButtonConnectClicked(bool)));
    connect(ui->pushButton_start_switch_page, &QPushButton::toggled, this, [this](bool toggled){
        if (toggled){
            ui->stackedWidget_start->setCurrentWidget(ui->page_start_new_server);
            ui->pushButton_start_switch_page->setText(tr("Import connection"));
        }
        else {
            ui->stackedWidget_start->setCurrentWidget(ui->page_start_import);
            ui->pushButton_start_switch_page->setText(tr("Set up your own server"));
        }
        //goToPage(Page::NewServer);
    });

    connect(ui->pushButton_new_server_connect_key, &QPushButton::toggled, this, [this](bool checked){
        ui->label_new_server_password->setText(checked ? tr("Private key") : tr("Password"));
        ui->pushButton_new_server_connect_key->setText(checked ? tr("Connect using SSH password") : tr("Connect using SSH key"));
        ui->lineEdit_new_server_password->setVisible(!checked);
        ui->textEdit_new_server_ssh_key->setVisible(checked);
    });

    connect(ui->pushButton_new_server_settings_cloak, &QPushButton::toggled, this, [this](bool toggle){
        ui->frame_new_server_settings_cloak->setMaximumHeight(toggle * 200);
        if (toggle)
            ui->frame_new_server_settings_parent_cloak->layout()->addWidget(ui->frame_new_server_settings_cloak);
        else
            ui->frame_new_server_settings_parent_cloak->layout()->removeWidget(ui->frame_new_server_settings_cloak);
    });
    connect(ui->pushButton_new_server_settings_ss, &QPushButton::toggled, this, [this](bool toggle){
        ui->frame_new_server_settings_ss->setMaximumHeight(toggle * 200);
        if (toggle)
            ui->frame_new_server_settings_parent_ss->layout()->addWidget(ui->frame_new_server_settings_ss);
        else
            ui->frame_new_server_settings_parent_ss->layout()->removeWidget(ui->frame_new_server_settings_ss);
    });
    connect(ui->pushButton_new_server_settings_openvpn, &QPushButton::toggled, this, [this](bool toggle){
        ui->frame_new_server_settings_openvpn->setMaximumHeight(toggle * 200);
        if (toggle)
            ui->frame_new_server_settings_parent_openvpn->layout()->addWidget(ui->frame_new_server_settings_openvpn);
        else
            ui->frame_new_server_settings_parent_openvpn->layout()->removeWidget(ui->frame_new_server_settings_ss);
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

    int serverIndex = m_settings.defaultServerIndex();
    ServerCredentials credentials = m_settings.serverCredentials(serverIndex);
    DockerContainer container = m_settings.defaultContainer(serverIndex);
    const QJsonObject &containerConfig = m_settings.containerConfig(serverIndex, container);

    onConnectWorker(serverIndex, credentials, container, containerConfig);
}

void MainWindow::onConnectWorker(int serverIndex, const ServerCredentials &credentials, DockerContainer container, const QJsonObject &containerConfig)
{
    ui->label_error_text->clear();
    ui->pushButton_connect->setChecked(true);
    qApp->processEvents();

    ErrorCode errorCode = m_vpnConnection->connectToVpn(serverIndex, credentials, container, containerConfig);
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
        IpcClient::Interface()->routeDelete(ipToDelete, "");
        IpcClient::Interface()->flushDns();
    }
}

void MainWindow::updateSettings()
{

}

void MainWindow::updateSitesPage()
{
    ui->listWidget_sites->clear();
    for (const QString &site : m_settings.customSites()) {
        makeSitesListItem(ui->listWidget_sites, site);
    }
}

void MainWindow::updateVpnPage()
{
    ui->radioButton_mode_selected_sites->setChecked(m_settings.customRouting());
    ui->pushButton_vpn_add_site->setEnabled(m_settings.customRouting());
}

void MainWindow::updateAppSettingsPage()
{
    ui->checkBox_autostart->setChecked(Autostart::isAutostart());
    ui->checkBox_autoconnect->setChecked(m_settings.isAutoConnect());

    ui->lineEdit_network_settings_dns1->setText(m_settings.primaryDns());
    ui->lineEdit_network_settings_dns2->setText(m_settings.secondaryDns());
}

void MainWindow::updateServerPage()
{
    ui->label_server_settings_wait_info->hide();
    ui->label_server_settings_wait_info->clear();

    QJsonObject server = m_settings.server(selectedServerIndex);
    QString port = server.value(config_key::port).toString();
    ui->label_server_settings_server->setText(QString("%1@%2%3%4")
        .arg(server.value(config_key::userName).toString())
        .arg(server.value(config_key::hostName).toString())
        .arg(port.isEmpty() ? "" : ":")
        .arg(port));
    ui->lineEdit_server_settings_description->setText(server.value(config_key::description).toString());

    QString selectedContainerName = m_settings.defaultContainerName(selectedServerIndex);

    ui->label_server_settings_current_vpn_protocol->setText(tr("Protocol: ") + selectedContainerName);

    //qDebug() << "DefaultContainer(selectedServerIndex)" << selectedServerIndex << containerToString(m_settings.defaultContainer(selectedServerIndex));

}

void MainWindow::updateServersListPage()
{
    ui->listWidget_servers->clear();
    const QJsonArray &servers = m_settings.serversArray();
    int defaultServer = m_settings.defaultServerIndex();

    ui->listWidget_servers->setUpdatesEnabled(false);
    for(int i = 0; i < servers.size(); i++) {
        makeServersListItem(ui->listWidget_servers, servers.at(i).toObject(), i == defaultServer, i);
    }
    ui->listWidget_servers->setUpdatesEnabled(true);
}

void MainWindow::updateProtocolsPage()
{
    ui->progressBar_protocols_container_reinstall->hide();

    auto containers = m_settings.containers(selectedServerIndex);

    DockerContainer defaultContainer = m_settings.defaultContainer(selectedServerIndex);
    ui->pushButton_proto_cloak_openvpn_cont_default->setChecked(defaultContainer == DockerContainer::OpenVpnOverCloak);
    ui->pushButton_proto_ss_openvpn_cont_default->setChecked(defaultContainer == DockerContainer::OpenVpnOverShadowSocks);
    ui->pushButton_proto_openvpn_cont_default->setChecked(defaultContainer == DockerContainer::OpenVpn);

    ui->pushButton_proto_cloak_openvpn_cont_default->setVisible(containers.contains(DockerContainer::OpenVpnOverCloak));
    ui->pushButton_proto_ss_openvpn_cont_default->setVisible(containers.contains(DockerContainer::OpenVpnOverShadowSocks));
    ui->pushButton_proto_openvpn_cont_default->setVisible(containers.contains(DockerContainer::OpenVpn));

    ui->pushButton_proto_cloak_openvpn_cont_share->setVisible(containers.contains(DockerContainer::OpenVpnOverCloak));
    ui->pushButton_proto_ss_openvpn_cont_share->setVisible(containers.contains(DockerContainer::OpenVpnOverShadowSocks));
    ui->pushButton_proto_openvpn_cont_share->setVisible(containers.contains(DockerContainer::OpenVpn));

    ui->pushButton_proto_cloak_openvpn_cont_install->setChecked(containers.contains(DockerContainer::OpenVpnOverCloak));
    ui->pushButton_proto_ss_openvpn_cont_install->setChecked(containers.contains(DockerContainer::OpenVpnOverShadowSocks));
    ui->pushButton_proto_openvpn_cont_install->setChecked(containers.contains(DockerContainer::OpenVpn));

    ui->frame_openvpn_ss_cloak_settings->setVisible(containers.contains(DockerContainer::OpenVpnOverCloak));
    ui->frame_openvpn_ss_settings->setVisible(containers.contains(DockerContainer::OpenVpnOverShadowSocks));
    ui->frame_openvpn_settings->setVisible(containers.contains(DockerContainer::OpenVpn));
}

void MainWindow::updateShareCodePage()
{
//    QJsonObject o;
//    o.insert("h", m_settings.serverName());
//    o.insert("p", m_settings.serverPort());
//    o.insert("u", m_settings.userName());
//    o.insert("w", m_settings.password());

//    QByteArray ba = QJsonDocument(o).toJson().toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
//    ui->textEdit_sharing_code->setText(QString("vpn://%1").arg(QString(ba)));

    //qDebug() << "Share code" << QJsonDocument(o).toJson();
}

void MainWindow::updateOpenVpnPage(const QJsonObject &openvpnConfig, DockerContainer container)
{   
    ui->radioButton_proto_openvpn_udp->setEnabled(true);
    ui->radioButton_proto_openvpn_tcp->setEnabled(true);

    ui->lineEdit_proto_openvpn_subnet->setText(openvpnConfig.value(config_key::subnet_address).
        toString(protocols::vpnDefaultSubnetAddress));

    QString trasnsport = openvpnConfig.value(config_key::transport_proto).
        toString(protocols::openvpn::defaultTransportProto);

    ui->radioButton_proto_openvpn_udp->setChecked(trasnsport == protocols::openvpn::defaultTransportProto);
    ui->radioButton_proto_openvpn_tcp->setChecked(trasnsport != protocols::openvpn::defaultTransportProto);

    ui->comboBox_proto_openvpn_cipher->setCurrentText(openvpnConfig.value(config_key::cipher).
        toString(protocols::openvpn::defaultCipher));

    ui->comboBox_proto_openvpn_hash->setCurrentText(openvpnConfig.value(config_key::hash).
        toString(protocols::openvpn::defaultHash));

    bool blockOutsideDns = openvpnConfig.value(config_key::block_outside_dns).toBool(protocols::openvpn::defaultBlockOutsideDns);
    ui->checkBox_proto_openvpn_block_dns->setChecked(blockOutsideDns);

    bool isNcpDisabled = openvpnConfig.value(config_key::ncp_disable).toBool(protocols::openvpn::defaultNcpDisable);
    ui->checkBox_proto_openvpn_auto_encryption->setChecked(!isNcpDisabled);

    if (container == DockerContainer::OpenVpnOverShadowSocks) {
        ui->radioButton_proto_openvpn_udp->setEnabled(false);
        ui->radioButton_proto_openvpn_tcp->setEnabled(false);
        ui->radioButton_proto_openvpn_tcp->setChecked(true);
    }
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

void MainWindow::makeServersListItem(QListWidget *listWidget, const QJsonObject &server, bool isDefault, int index)
{
    QSize size(310, 70);
    ServerWidget* widget = new ServerWidget(server, isDefault);

    widget->resize(size);

    connect(widget->ui->pushButton_default, &QPushButton::clicked, this, [this, index](){
        m_settings.setDefaultServer(index);
        updateSettings();
        updateServersListPage();
    });

    connect(widget->ui->pushButton_share, &QPushButton::clicked, this, [this, index](){
        goToPage(Page::ShareConnection);
        // update share page
    });

    connect(widget->ui->pushButton_settings, &QPushButton::clicked, this, [this, index](){
        selectedServerIndex = index;
        goToPage(Page::ServerSettings);
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

QJsonObject MainWindow::getOpenVpnConfigFromPage(QJsonObject oldConfig)
{
    oldConfig.insert(config_key::subnet_address, ui->lineEdit_proto_openvpn_subnet->text());
    oldConfig.insert(config_key::transport_proto, ui->radioButton_proto_openvpn_udp->isChecked() ? protocols::UDP : protocols::TCP);
    oldConfig.insert(config_key::ncp_disable, ! ui->checkBox_proto_openvpn_auto_encryption->isChecked());
    oldConfig.insert(config_key::cipher, ui->comboBox_proto_openvpn_cipher->currentText());
    oldConfig.insert(config_key::hash, ui->comboBox_proto_openvpn_hash->currentText());
    oldConfig.insert(config_key::block_outside_dns, ui->checkBox_proto_openvpn_block_dns->isChecked());

    return oldConfig;
}
