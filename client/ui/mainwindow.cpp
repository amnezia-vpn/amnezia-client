#include <QApplication>
#include <QClipboard>
#include <QDebug>
#include <QDesktopServices>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHostInfo>
#include <QItemSelectionModel>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <QMenu>
#include <QMessageBox>
#include <QMetaEnum>
#include <QSysInfo>
#include <QThread>
#include <QTimer>


#include "configurators/vpn_configurator.h"
#include "configurators/openvpn_configurator.h"
#include "configurators/shadowsocks_configurator.h"
#include "configurators/cloak_configurator.h"

#include "core/servercontroller.h"
#include "core/server_defs.h"
#include "core/errorstrings.h"

#include "protocols/protocols_defs.h"
#include "protocols/shadowsocksvpnprotocol.h"

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
    setupNewServerConnections();
    setupWizardConnections();
    setupVpnPageConnections();
    setupSitesPageConnections();
    setupGeneralSettingsConnections();
    setupAppSettingsConnections();
    setupNetworkSettingsConnections();
    setupProtocolsPageConnections();
    setupNewServerPageConnections();
    setupSharePageConnections();
    setupServerSettingsPageConnections();

    ui->label_error_text->clear();
    installEventFilter(this);
    ui->widget_tittlebar->installEventFilter(this);

    ui->stackedWidget_main->setSpeed(200);
    ui->stackedWidget_main->setAnimation(QEasingCurve::Linear);


    ui->tableView_sites->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
//    ui->tableView_sites->setColumnWidth(0, 450);
//    ui->tableView_sites->setColumnWidth(1, 120);

    if (QOperatingSystemVersion::current() <= QOperatingSystemVersion::Windows7) {
        needToHideCustomTitlebar = true;
    }

#ifdef Q_OS_MAC
    fixWidget(this);
    needToHideCustomTitlebar = true;
#endif

    if (needToHideCustomTitlebar) {
        ui->widget_tittlebar->hide();
        resize(width(), 640);
        ui->stackedWidget_main->move(0,0);
    }

    // Post initialization
    goToPage(Page::Start, true, false);

    if (m_settings.defaultServerIndex() >= 0 && m_settings.serversCount() > 0) {
        goToPage(Page::Vpn, true, false);
    }

    //ui->pushButton_general_settings_exit->hide();
    updateSharingPage(selectedServerIndex, m_settings.serverCredentials(selectedServerIndex), selectedDockerContainer);

    setFixedSize(width(),height());

    qInfo().noquote() << QString("Started %1 version %2").arg(APPLICATION_NAME).arg(APP_VERSION);
    qInfo().noquote() << QString("%1 (%2)").arg(QSysInfo::prettyProductName()).arg(QSysInfo::currentCpuArchitecture());

    Utils::initializePath(Utils::configPath());

    m_vpnConnection = new VpnConnection(this);
    connect(m_vpnConnection, SIGNAL(bytesChanged(quint64, quint64)), this, SLOT(onBytesChanged(quint64, quint64)));
    connect(m_vpnConnection, SIGNAL(connectionStateChanged(VpnProtocol::ConnectionState)), this, SLOT(onConnectionStateChanged(VpnProtocol::ConnectionState)));
    connect(m_vpnConnection, SIGNAL(vpnProtocolError(amnezia::ErrorCode)), this, SLOT(onVpnProtocolError(amnezia::ErrorCode)));

    onConnectionStateChanged(VpnProtocol::Disconnected);

    if (m_settings.isAutoConnect() && m_settings.defaultServerIndex() >= 0) {
        QTimer::singleShot(1000, this, [this](){
            ui->pushButton_connect->setEnabled(false);
            onConnect();
        });
    }

    qDebug().noquote() << QString("Default config: %1").arg(Utils::defaultVpnConfigFileName());

    m_ipAddressValidator.setRegExp(Utils::ipAddressRegExp());
    m_ipAddressPortValidator.setRegExp(Utils::ipAddressPortRegExp());
    m_ipNetwok24Validator.setRegExp(Utils::ipNetwork24RegExp());
    m_ipPortValidator.setRegExp(Utils::ipPortRegExp());

    ui->lineEdit_new_server_ip->setValidator(&m_ipAddressPortValidator);
    ui->lineEdit_network_settings_dns1->setValidator(&m_ipAddressValidator);
    ui->lineEdit_network_settings_dns2->setValidator(&m_ipAddressValidator);

    ui->lineEdit_proto_openvpn_subnet->setValidator(&m_ipNetwok24Validator);

    ui->lineEdit_proto_openvpn_port->setValidator(&m_ipPortValidator);
    ui->lineEdit_proto_shadowsocks_port->setValidator(&m_ipPortValidator);
    ui->lineEdit_proto_cloak_port->setValidator(&m_ipPortValidator);

    //ui->toolBox_share_connection->removeItem(ui->toolBox_share_connection->indexOf(ui->page_share_shadowsocks));
    //ui->page_share_shadowsocks->setVisible(false);


    sitesModels.insert(Settings::VpnOnlyForwardSites, new SitesModel(Settings::VpnOnlyForwardSites));
    sitesModels.insert(Settings::VpnAllExceptSites, new SitesModel(Settings::VpnAllExceptSites));
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

void MainWindow::showOnStartup()
{
    if (! m_settings.isStartMinimized()) show();
    else {
#ifdef Q_OS_MACX
        setDockIconVisible(false);
#endif
    }
}

void MainWindow::goToPage(Page page, bool reset, bool slide)
{
    //qDebug() << "goToPage" << page;
    if (ui->stackedWidget_main->nextWidget() == getPageWidget(page)) return;

    if (reset) {
        if (page == Page::ServerSettings) {
            updateServerPage();
        }
        if (page == Page::ShareConnection) {

        }
        if (page == Page::Wizard) {
            ui->radioButton_setup_wizard_medium->setChecked(true);
        }
        if (page == Page::WizardHigh) {
            ui->lineEdit_setup_wizard_high_website_masking->setText(protocols::cloak::defaultRedirSite);
        }
        if (page == Page::ServerConfiguring) {
            ui->progressBar_new_server_configuring->setValue(0);
        }
        if (page == Page::GeneralSettings) {
            updateGeneralSettingPage();
        }
        if (page == Page::ServersList) {
            updateServersListPage();
        }
        if (page == Page::Start) {
            updateStartPage();
        }
        if (page == Page::NewServerProtocols) {
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
        if (page == Page::NetworkSettings) {
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

void MainWindow::setStartPage(MainWindow::Page page, bool slide)
{
    if (slide)
        ui->stackedWidget_main->slideInWidget(getPageWidget(page), SlidingStackedWidget::RIGHT2LEFT);
    else
        ui->stackedWidget_main->setCurrentWidget(getPageWidget(page));

    pagesStack.clear();
    pagesStack.push(page);

    if (page == Page::Start) updateStartPage();
}

void MainWindow::closePage()
{
    if (pagesStack.size() <= 1) return;

    Page prev = pagesStack.pop();
    //qDebug() << "closePage" << prev << "Set page" << pagesStack.top();
    ui->stackedWidget_main->slideInWidget(getPageWidget(pagesStack.top()), SlidingStackedWidget::LEFT2RIGHT);
}

QWidget *MainWindow::getPageWidget(MainWindow::Page page)
{
    switch (page) {
    case(Page::Start): return ui->page_start;
    case(Page::NewServer): return ui->page_new_server;
    case(Page::NewServerProtocols): return ui->page_new_server_protocols;
    case(Page::Wizard): return ui->page_setup_wizard;
    case(Page::WizardHigh): return ui->page_setup_wizard_high_level;
    case(Page::WizardLow): return ui->page_setup_wizard_low_level;
    case(Page::WizardMedium): return ui->page_setup_wizard_medium_level;
    case(Page::WizardVpnMode): return ui->page_setup_wizard_vpn_mode;
    case(Page::ServerConfiguring): return ui->page_new_server_configuring;
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
        break;
//    case Qt::Key_0:
//        *((char*)-1) = 'x';
//        break;
    case Qt::Key_H:
        selectedServerIndex = m_settings.defaultServerIndex();
        selectedDockerContainer = m_settings.defaultContainer(selectedServerIndex);

        updateSharingPage(selectedServerIndex, m_settings.serverCredentials(selectedServerIndex), selectedDockerContainer);
        goToPage(Page::ShareConnection);
        break;
#endif
    case Qt::Key_C:
        qDebug().noquote() << "Def server" << m_settings.defaultServerIndex() << m_settings.defaultContainerName(m_settings.defaultServerIndex());
        //qDebug().noquote() << QJsonDocument(m_settings.containerConfig(m_settings.defaultServerIndex(), m_settings.defaultContainer(m_settings.defaultServerIndex()))).toJson();
        qDebug().noquote() << QJsonDocument(m_settings.defaultServer()).toJson();
        break;
    case Qt::Key_A:
        goToPage(Page::Start);
        break;
    case Qt::Key_S:
        selectedServerIndex = m_settings.defaultServerIndex();
        goToPage(Page::ServerSettings);
        break;
    case Qt::Key_P:
        selectedServerIndex = m_settings.defaultServerIndex();
        selectedDockerContainer = m_settings.defaultContainer(selectedServerIndex);
        goToPage(Page::ServerVpnProtocols);
        break;
    case Qt::Key_Escape:
        if (currentPage() == Page::Vpn) break;
        if (currentPage() == Page::ServerConfiguring) break;
        if (currentPage() == Page::Start && pagesStack.size() < 2) break;
        if (currentPage() == Page::Sites &&
            ui->tableView_sites->selectionModel()->selection().indexes().size() > 0) {
            ui->tableView_sites->clearSelection();
            break;
        }

        if (! ui->stackedWidget_main->isAnimationRunning() && ui->stackedWidget_main->currentWidget()->isEnabled()) {
            closePage();
        }
    default:
        ;
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_settings.serversCount() == 0) qApp->quit();
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
    if (needToHideCustomTitlebar) {
        ui->widget_tittlebar->hide();
        resize(width(), 640);
        ui->stackedWidget_main->move(0,0);
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
#ifdef Q_DEBUG
    //QString output = ServerController::checkSshConnection(serverCredentials, &e);
#else
    QString output;
#endif

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
    if (ok) goToPage(Page::NewServer);
}

QMap<DockerContainer, QJsonObject> MainWindow::getInstallConfigsFromProtocolsPage() const
{
    QJsonObject cloakConfig {
                    { config_key::container, amnezia::containerToString(DockerContainer::OpenVpnOverCloak) },
                    { config_key::cloak, QJsonObject {
                        { config_key::port, ui->lineEdit_new_server_cloak_port->text() },
                        { config_key::site, ui->lineEdit_new_server_cloak_site->text() }}
                    }
    };
    QJsonObject ssConfig {
                    { config_key::container, amnezia::containerToString(DockerContainer::OpenVpnOverShadowSocks) },
                    { config_key::shadowsocks, QJsonObject {
                        { config_key::port, ui->lineEdit_new_server_ss_port->text() },
                        { config_key::cipher, ui->comboBox_new_server_ss_cipher->currentText() }}
                    }
    };
    QJsonObject openVpnConfig {
                    { config_key::container, amnezia::containerToString(DockerContainer::OpenVpn) },
                    { config_key::openvpn, QJsonObject {
                        { config_key::port, ui->lineEdit_new_server_openvpn_port->text() },
                        { config_key::transport_proto, ui->comboBox_new_server_openvpn_proto->currentText() }}
                    }
    };

    QMap<DockerContainer, QJsonObject> containers;

    if (ui->checkBox_new_server_cloak->isChecked()) {
        containers.insert(DockerContainer::OpenVpnOverCloak, cloakConfig);
    }

    if (ui->checkBox_new_server_ss->isChecked()) {
        containers.insert(DockerContainer::OpenVpnOverShadowSocks, ssConfig);
    }

    if (ui->checkBox_new_server_openvpn->isChecked()) {
        containers.insert(DockerContainer::OpenVpn, openVpnConfig);
    }

    return containers;
}

QMap<DockerContainer, QJsonObject> MainWindow::getInstallConfigsFromWizardPage() const
{
    QJsonObject cloakConfig {
                    { config_key::container, amnezia::containerToString(DockerContainer::OpenVpnOverCloak) },
                    { config_key::cloak, QJsonObject {
                        { config_key::site, ui->lineEdit_setup_wizard_high_website_masking->text() }}
                    }
    };
    QJsonObject ssConfig {
                    { config_key::container, amnezia::containerToString(DockerContainer::OpenVpnOverShadowSocks) }
    };
    QJsonObject openVpnConfig {
                    { config_key::container, amnezia::containerToString(DockerContainer::OpenVpn) }
    };

    QMap<DockerContainer, QJsonObject> containers;

    if (ui->radioButton_setup_wizard_high->isChecked()) {
        containers.insert(DockerContainer::OpenVpnOverCloak, cloakConfig);
    }

    if (ui->radioButton_setup_wizard_medium->isChecked()) {
        containers.insert(DockerContainer::OpenVpnOverShadowSocks, ssConfig);
    }

    if (ui->radioButton_setup_wizard_low->isChecked()) {
        containers.insert(DockerContainer::OpenVpn, openVpnConfig);
    }

    return containers;
}

void MainWindow::installServer(const QMap<DockerContainer, QJsonObject> &containers)
{
    if (containers.isEmpty()) return;

    goToPage(Page::ServerConfiguring);
    QEventLoop loop;
    QTimer::singleShot(500, &loop, SLOT(quit()));
    loop.exec();
    qApp->processEvents();

    bool ok = installContainers(installCredentials, containers,
                            ui->page_new_server_configuring,
                            ui->progressBar_new_server_configuring,
                            nullptr,
                            ui->label_new_server_configuring_wait_info);

    if (ok) {
        QJsonObject server;
        server.insert(config_key::hostName, installCredentials.hostName);
        server.insert(config_key::userName, installCredentials.userName);
        server.insert(config_key::password, installCredentials.password);
        server.insert(config_key::port, installCredentials.port);
        server.insert(config_key::description, m_settings.nextAvailableServerName());

        QJsonArray containerConfigs;
        for (const QJsonObject &cfg : containers) {
            containerConfigs.append(cfg);
        }
        server.insert(config_key::containers, containerConfigs);
        server.insert(config_key::defaultContainer, containerToString(containers.firstKey()));

        m_settings.addServer(server);
        m_settings.setDefaultServer(m_settings.serversCount() - 1);

        setStartPage(Page::Vpn);
        qApp->processEvents();
    }
    else {
        closePage();
    }
}

void MainWindow::onPushButtonNewServerImport(bool)
{
    QString s = ui->lineEdit_start_existing_code->text();
    s.replace("vpn://", "");
    QJsonObject o = QJsonDocument::fromJson(QByteArray::fromBase64(s.toUtf8(), QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals)).object();

    ServerCredentials credentials;
    credentials.hostName = o.value("h").toString();
    if (credentials.hostName.isEmpty()) credentials.hostName = o.value(config_key::hostName).toString();

    credentials.port = o.value("p").toInt();
    if (credentials.port == 0) credentials.port = o.value(config_key::port).toInt();

    credentials.userName = o.value("u").toString();
    if (credentials.userName.isEmpty()) credentials.userName = o.value(config_key::userName).toString();

    credentials.password = o.value("w").toString();
    if (credentials.password.isEmpty()) credentials.password = o.value(config_key::password).toString();

    if (credentials.isValid()) {
        o.insert(config_key::hostName, credentials.hostName);
        o.insert(config_key::port, credentials.port);
        o.insert(config_key::userName, credentials.userName);
        o.insert(config_key::password, credentials.password);

        o.remove("h");
        o.remove("p");
        o.remove("u");
        o.remove("w");
    }
    qDebug() << QString("Added server %3@%1:%2").
                arg(credentials.hostName).
                arg(credentials.port).
                arg(credentials.userName);

    //qDebug() << QString("Password") << credentials.password;

    if (credentials.isValid() || o.contains(config_key::containers)) {
        m_settings.addServer(o);
        m_settings.setDefaultServer(m_settings.serversCount() - 1);

        setStartPage(Page::Vpn);
    }
    else {
        qDebug() << "Failed to import profile";
        qDebug().noquote() << QJsonDocument(o).toJson();
        return;
    }

    if (!o.contains(config_key::containers)) {
        selectedServerIndex = m_settings.defaultServerIndex();
        selectedDockerContainer = m_settings.defaultContainer(selectedServerIndex);
        goToPage(Page::ServerVpnProtocols);
    }
}

bool MainWindow::installContainers(ServerCredentials credentials,
    const QMap<DockerContainer, QJsonObject> &containers,
    QWidget *page, QProgressBar *progress, QPushButton *button, QLabel *info)
{
    if (!progress) return false;

    if (page) page->setEnabled(false);
    if (button) button->setVisible(false);

    if (info) info->setVisible(true);
    if (info) info->setText(tr("Please wait, configuring process may take up to 5 minutes"));


    int cnt = 0;
    for (QMap<DockerContainer, QJsonObject>::const_iterator i = containers.constBegin(); i != containers.constEnd(); i++, cnt++) {
        QTimer timer;
        connect(&timer, &QTimer::timeout, [progress](){
            progress->setValue(progress->value() + 1);
        });

        progress->setValue(0);
        timer.start(1000);

        progress->setTextVisible(true);
        progress->setFormat(QString("Installing %1 %2 %3").arg(cnt+1).arg(tr("of")).arg(containers.size()));

        ErrorCode e = ServerController::setupContainer(credentials, i.key(), i.value());
        qDebug() << "Setup server finished with code" << e;
        ServerController::disconnectFromHost(credentials);

        if (e) {
            if (page) page->setEnabled(true);
            if (button) button->setVisible(true);
            if (info) info->setVisible(false);

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


    if (button) button->show();
    if (page) page->setEnabled(true);
    if (info) info->setText(tr("Amnezia server installed"));

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

    m_settings.setContainers(selectedServerIndex, {});
    m_settings.setDefaultContainer(selectedServerIndex, DockerContainer::None);

    ui->page_server_settings->setEnabled(true);
    ui->pushButton_server_settings_clear->setText(tr("Clear server from Amnezia software"));
}

void MainWindow::onPushButtonForgetServer(bool)
{
    if (m_settings.defaultServerIndex() == selectedServerIndex && m_vpnConnection->isConnected()) {
        onDisconnect();
    }
    m_settings.removeServer(selectedServerIndex);

    if (m_settings.defaultServerIndex() == selectedServerIndex) {
        m_settings.setDefaultServer(0);
    }
    else if (m_settings.defaultServerIndex() > selectedServerIndex) {
        m_settings.setDefaultServer(m_settings.defaultServerIndex() - 1);
    }

    if (m_settings.serversCount() == 0) {
        m_settings.setDefaultServer(-1);
    }


    selectedServerIndex = -1;

    updateServersListPage();

    if (m_settings.serversCount() == 0) {
        setStartPage(Page::Start);
    }
    else {
        closePage();
    }
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
    case VpnProtocol::Disconnected:
        onBytesChanged(0,0);
        ui->pushButton_connect->setChecked(false);
        pushButtonConnectEnabled = true;
        radioButtonsModeEnabled = true;
        break;
    case VpnProtocol::Preparing:
        pushButtonConnectEnabled = false;
        radioButtonsModeEnabled = false;
        break;
    case VpnProtocol::Connecting:
        pushButtonConnectEnabled = false;
        radioButtonsModeEnabled = false;
        break;
    case VpnProtocol::Connected:
        pushButtonConnectEnabled = true;
        radioButtonsModeEnabled = false;
        break;
    case VpnProtocol::Disconnecting:
        pushButtonConnectEnabled = false;
        radioButtonsModeEnabled = false;
        break;
    case VpnProtocol::Reconnecting:
        pushButtonConnectEnabled = true;
        radioButtonsModeEnabled = false;
        break;
    case VpnProtocol::Error:
        ui->pushButton_connect->setChecked(false);
        pushButtonConnectEnabled = true;
        radioButtonsModeEnabled = true;
        break;
    case VpnProtocol::Unknown:
        pushButtonConnectEnabled = true;
        radioButtonsModeEnabled = true;
    }

    ui->pushButton_connect->setEnabled(pushButtonConnectEnabled);
    ui->widget_vpn_mode->setEnabled(radioButtonsModeEnabled);
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
    setTrayState(VpnProtocol::Disconnected);

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

    connect(ui->pushButton_vpn_add_site, &QPushButton::clicked, this, [this](){ goToPage(Page::Sites); });


    QVector<QPushButton *> backButtons {
        ui->pushButton_back_from_sites,
        ui->pushButton_back_from_general_settings,
        ui->pushButton_back_from_start,
        ui->pushButton_back_from_new_server,
        ui->pushButton_back_from_new_server_protocols,
        ui->pushButton_back_from_setup_wizard,
        ui->pushButton_back_from_setup_wizard_high,
        ui->pushButton_back_from_setup_wizard_low,
        ui->pushButton_back_from_setup_wizard_medium,
        ui->pushButton_back_from_setup_wizard_vpn_mode,
        ui->pushButton_back_from_app_settings,
        ui->pushButton_back_from_network_settings,
        ui->pushButton_back_from_server_settings,
        ui->pushButton_back_from_servers,
        ui->pushButton_back_from_share,
        ui->pushButton_back_from_server_vpn_protocols,
        ui->pushButton_back_from_openvpn_settings,
        ui->pushButton_back_from_cloak_settings,
        ui->pushButton_back_from_shadowsocks_settings
    };

    for (QPushButton *b : backButtons) {
        connect(b, &QPushButton::clicked, this, [this](){ closePage(); });
    }

}

void MainWindow::setupNewServerConnections()
{
    connect(ui->pushButton_new_server_get_info, &QPushButton::clicked, this, [](){
        QDesktopServices::openUrl(QUrl("https://amnezia.org"));
    });

    connect(ui->pushButton_new_server_connect, SIGNAL(clicked(bool)), this, SLOT(onPushButtonNewServerConnect(bool)));
    connect(ui->pushButton_new_server_import, SIGNAL(clicked(bool)), this, SLOT(onPushButtonNewServerImport(bool)));

    connect(ui->pushButton_new_server_connect_configure, &QPushButton::clicked, this, [this](){
        installServer(getInstallConfigsFromProtocolsPage());
    });

}

void MainWindow::setupWizardConnections()
{
    connect(ui->pushButton_new_server_wizard, &QPushButton::clicked, this, [this](){ goToPage(Page::Wizard); });
    connect(ui->pushButton_new_server_advanced, &QPushButton::clicked, this, [this](){ goToPage(Page::NewServerProtocols); });
    connect(ui->pushButton_setup_wizard_next, &QPushButton::clicked, this, [this](){
        if (ui->radioButton_setup_wizard_high->isChecked()) goToPage(Page::WizardHigh);
        else if (ui->radioButton_setup_wizard_medium->isChecked()) goToPage(Page::WizardMedium);
        else if (ui->radioButton_setup_wizard_low->isChecked()) goToPage(Page::WizardLow);
    });

    connect(ui->pushButton_setup_wizard_high_next, &QPushButton::clicked, this, [this](){
        QString domain = ui->lineEdit_setup_wizard_high_website_masking->text();
        if (domain.isEmpty() || !domain.contains(".")) return;
        goToPage(Page::WizardVpnMode);
    });

    connect(ui->lineEdit_setup_wizard_high_website_masking, &QLineEdit::textEdited, this, [this](){
        QString text = ui->lineEdit_setup_wizard_high_website_masking->text();
        text.replace("http://", "");
        text.replace("https://", "");
        if (text.isEmpty()) return;
        text = text.split("/").first();
        ui->lineEdit_setup_wizard_high_website_masking->setText(text);
    });

    connect(ui->pushButton_setup_wizard_medium_next, &QPushButton::clicked, this, [this](){ goToPage(Page::WizardVpnMode); });

    connect(ui->pushButton_setup_wizard_vpn_mode_finish, &QPushButton::clicked, this, [this](){
        installServer(getInstallConfigsFromWizardPage());
        if (ui->checkBox_setup_wizard_vpn_mode->isChecked()) m_settings.setRouteMode(Settings::VpnOnlyForwardSites);
        else m_settings.setRouteMode(Settings::VpnAllSites);
    });

    connect(ui->pushButton_setup_wizard_low_finish, &QPushButton::clicked, this, [this](){
        installServer(getInstallConfigsFromWizardPage());
    });

    connect(ui->lineEdit_setup_wizard_high_website_masking, &QLineEdit::returnPressed, this, [this](){
        ui->pushButton_setup_wizard_high_next->click();
    });
}

void MainWindow::setupVpnPageConnections()
{
    connect(ui->radioButton_vpn_mode_all_sites, &QRadioButton::toggled, ui->pushButton_vpn_add_site, &QPushButton::setDisabled);

    connect(ui->radioButton_vpn_mode_all_sites, &QRadioButton::toggled, this, [this](bool toggled) {
        m_settings.setRouteMode(Settings::VpnAllSites);
    });

    connect(ui->radioButton_vpn_mode_forward_sites, &QRadioButton::toggled, this, [this](bool toggled) {
        m_settings.setRouteMode(Settings::VpnOnlyForwardSites);
    });

    connect(ui->radioButton_vpn_mode_except_sites, &QRadioButton::toggled, this, [this](bool toggled) {
        m_settings.setRouteMode(Settings::VpnAllExceptSites);
    });
}

void MainWindow::setupSitesPageConnections()
{
    connect(ui->pushButton_sites_add_custom, &QPushButton::clicked, this, [this](){ onPushButtonAddCustomSitesClicked(); });

    connect(ui->lineEdit_sites_add_custom, &QLineEdit::returnPressed, [&](){
        ui->pushButton_sites_add_custom->click();
    });

    connect(ui->pushButton_sites_delete, &QPushButton::clicked, this, [this](){
        Settings::RouteMode mode = m_settings.routeMode();

        QItemSelectionModel* selection = ui->tableView_sites->selectionModel();
        if (!selection) return;

        {
            QModelIndexList indexesSites = selection->selectedRows(0);

            QStringList sites;
            for (const QModelIndex &index : indexesSites) {
                sites.append(index.data().toString());
            }

            m_settings.removeVpnSites(mode, sites);
        }

        if (m_vpnConnection->connectionState() == VpnProtocol::Connected) {
            QModelIndexList indexesIps = selection->selectedRows(1);

            QStringList ips;
            for (const QModelIndex &index : indexesIps) {
                if (index.data().toString().isEmpty()) {
                    ips.append(index.sibling(index.row(), 0).data().toString());
                }
                else {
                    ips.append(index.data().toString());
                }
            }

            m_vpnConnection->deleteRoutes(ips);
            m_vpnConnection->flushDns();
        }

        updateSitesPage();
    });

    connect(ui->pushButton_sites_import, &QPushButton::clicked, this, [this](){
        QString fileName = QFileDialog::getOpenFileName(this, tr("Import IP addresses"),
            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));

        QFile file(fileName);
        if (!file.open(QIODevice::ReadOnly)) return;

        Settings::RouteMode mode = m_settings.routeMode();

        QStringList ips;
        while (!file.atEnd()) {
            QString line = file.readLine();

            int pos = 0;
            QRegExp rx = Utils::ipAddressWithSubnetRegExp();
            while ((pos = rx.indexIn(line, pos)) != -1) {
                ips << rx.cap(0);
                pos += rx.matchedLength();
            }
        }

        m_settings.addVpnIps(mode, ips);

        m_vpnConnection->addRoutes(QStringList() << ips);
        m_vpnConnection->flushDns();

        updateSitesPage();
    });
}

void MainWindow::setupAppSettingsConnections()
{
    connect(ui->checkBox_app_settings_autostart, &QCheckBox::stateChanged, this, [this](int state){
        if (state == Qt::Unchecked) {
            ui->checkBox_app_settings_autoconnect->setChecked(false);
        }
        Autostart::setAutostart(state == Qt::Checked);
    });

    connect(ui->checkBox_app_settings_autoconnect, &QCheckBox::stateChanged, this, [this](int state){
        m_settings.setAutoConnect(state == Qt::Checked);
    });

    connect(ui->checkBox_app_settings_start_minimized, &QCheckBox::stateChanged, this, [this](int state){
        m_settings.setStartMinimized(state == Qt::Checked);
    });

    connect(ui->pushButton_app_settings_check_for_updates, &QPushButton::clicked, this, [this](){
        QDesktopServices::openUrl(QUrl("https://github.com/amnezia-vpn/desktop-client/releases/latest"));
    });

    connect(ui->pushButton_app_settings_open_logs, &QPushButton::clicked, this, [this](){
        Debug::openLogsFolder();
        QDesktopServices::openUrl(QUrl::fromLocalFile(Utils::systemLogPath()));
    });
}

void MainWindow::setupGeneralSettingsConnections()
{
    connect(ui->pushButton_general_settings_exit, &QPushButton::clicked, this, [&](){ qApp->quit(); });

    connect(ui->pushButton_settings, &QPushButton::clicked, this, [this](){ goToPage(Page::GeneralSettings); });
    connect(ui->pushButton_general_settings_app_settings, &QPushButton::clicked, this, [this](){ goToPage(Page::AppSettings); });
    connect(ui->pushButton_general_settings_network_settings, &QPushButton::clicked, this, [this](){ goToPage(Page::NetworkSettings); });
    connect(ui->pushButton_general_settings_server_settings, &QPushButton::clicked, this, [this](){
        selectedServerIndex = m_settings.defaultServerIndex();
        goToPage(Page::ServerSettings);
    });
    connect(ui->pushButton_general_settings_servers_list, &QPushButton::clicked, this, [this](){ goToPage(Page::ServersList); });
    connect(ui->pushButton_general_settings_share_connection, &QPushButton::clicked, this, [this](){
        selectedServerIndex = m_settings.defaultServerIndex();
        selectedDockerContainer = m_settings.defaultContainer(selectedServerIndex);

        updateSharingPage(selectedServerIndex, m_settings.serverCredentials(selectedServerIndex), selectedDockerContainer);
        goToPage(Page::ShareConnection);
    });

    connect(ui->pushButton_general_settings_add_server, &QPushButton::clicked, this, [this](){ goToPage(Page::Start); });

}

void MainWindow::setupNetworkSettingsConnections()
{
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

    connect(ui->pushButton_network_settings_resetdns1, &QPushButton::clicked, this, [this](){
        m_settings.setPrimaryDns(m_settings.cloudFlareNs1);
        updateAppSettingsPage();
    });

    connect(ui->pushButton_network_settings_resetdns2, &QPushButton::clicked, this, [this](){
        m_settings.setSecondaryDns(m_settings.cloudFlareNs2);
        updateAppSettingsPage();
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
                    m_settings.setDefaultContainer(selectedServerIndex, container);
                }
            }
            else {
                button->setEnabled(false);
                ErrorCode e = ServerController::removeContainer(m_settings.serverCredentials(selectedServerIndex), container);
                m_settings.removeContainerConfig(selectedServerIndex, container);
                button->setEnabled(true);

                if (m_settings.defaultContainer(selectedServerIndex) == container) {
                    const auto &c = m_settings.containers(selectedServerIndex);
                    if (c.isEmpty()) m_settings.setDefaultContainer(selectedServerIndex, DockerContainer::None);
                    else m_settings.setDefaultContainer(selectedServerIndex, c.keys().first());
                }
            }

            updateProtocolsPage();
        });
    }

    // share buttons
    QList<QPushButton *> shareButtons {
        ui->pushButton_proto_openvpn_cont_share,
        ui->pushButton_proto_ss_openvpn_cont_share,
        ui->pushButton_proto_cloak_openvpn_cont_share
    };

    for (int i = 0; i < containers.size(); ++i) {
        QPushButton *button = shareButtons.at(i);
        DockerContainer container = containers.at(i);

        connect(button, &QPushButton::clicked, this, [this, button, container](){
            updateSharingPage(selectedServerIndex, m_settings.serverCredentials(selectedServerIndex), container);
            goToPage(Page::ShareConnection);
        });
    }

    // settings buttons

    // settings openvpn container
    connect(ui->pushButton_proto_openvpn_cont_openvpn_config, &QPushButton::clicked, this, [this](){
        selectedDockerContainer = DockerContainer::OpenVpn;
        updateOpenVpnPage(m_settings.protocolConfig(selectedServerIndex, selectedDockerContainer, Protocol::OpenVpn),
            selectedDockerContainer, m_settings.haveAuthData(selectedServerIndex));
        goToPage(Page::OpenVpnSettings);
    });

    // settings shadowsocks container
    connect(ui->pushButton_proto_ss_openvpn_cont_openvpn_config, &QPushButton::clicked, this, [this](){
        selectedDockerContainer = DockerContainer::OpenVpnOverShadowSocks;
        updateOpenVpnPage(m_settings.protocolConfig(selectedServerIndex, selectedDockerContainer, Protocol::OpenVpn),
            selectedDockerContainer, m_settings.haveAuthData(selectedServerIndex));
        goToPage(Page::OpenVpnSettings);
    });
    connect(ui->pushButton_proto_ss_openvpn_cont_ss_config, &QPushButton::clicked, this, [this](){
        selectedDockerContainer = DockerContainer::OpenVpnOverShadowSocks;
        updateShadowSocksPage(m_settings.protocolConfig(selectedServerIndex, selectedDockerContainer, Protocol::ShadowSocks),
            selectedDockerContainer, m_settings.haveAuthData(selectedServerIndex));
        goToPage(Page::ShadowSocksSettings);
    });

    // settings cloak container
    connect(ui->pushButton_proto_cloak_openvpn_cont_openvpn_config, &QPushButton::clicked, this, [this](){
        selectedDockerContainer = DockerContainer::OpenVpnOverCloak;
        updateOpenVpnPage(m_settings.protocolConfig(selectedServerIndex, selectedDockerContainer, Protocol::OpenVpn),
            selectedDockerContainer, m_settings.haveAuthData(selectedServerIndex));
        goToPage(Page::OpenVpnSettings);
    });
    connect(ui->pushButton_proto_cloak_openvpn_cont_ss_config, &QPushButton::clicked, this, [this](){
        selectedDockerContainer = DockerContainer::OpenVpnOverCloak;
        updateShadowSocksPage(m_settings.protocolConfig(selectedServerIndex, selectedDockerContainer, Protocol::ShadowSocks),
            selectedDockerContainer, m_settings.haveAuthData(selectedServerIndex));
        goToPage(Page::ShadowSocksSettings);
    });
    connect(ui->pushButton_proto_cloak_openvpn_cont_cloak_config, &QPushButton::clicked, this, [this](){
        selectedDockerContainer = DockerContainer::OpenVpnOverCloak;
        updateCloakPage(m_settings.protocolConfig(selectedServerIndex, selectedDockerContainer, Protocol::Cloak),
            selectedDockerContainer, m_settings.haveAuthData(selectedServerIndex));
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
        qDebug() << "Protocol saved with code:" << e << "for" << selectedServerIndex << selectedDockerContainer;
    });

    connect(ui->pushButton_proto_shadowsocks_save, &QPushButton::clicked, this, [this](){
        QJsonObject protocolConfig = m_settings.protocolConfig(selectedServerIndex, selectedDockerContainer, Protocol::ShadowSocks);
        protocolConfig = getShadowSocksConfigFromPage(protocolConfig);

        QJsonObject containerConfig = m_settings.containerConfig(selectedServerIndex, selectedDockerContainer);
        QJsonObject newContainerConfig = containerConfig;
        newContainerConfig.insert(config_key::shadowsocks, protocolConfig);

        ErrorCode e = doInstallAction([this, containerConfig, newContainerConfig](){
            return ServerController::updateContainer(m_settings.serverCredentials(selectedServerIndex), selectedDockerContainer, containerConfig, newContainerConfig);
        },
           ui->page_proto_shadowsocks, ui->progressBar_proto_shadowsocks_reset,
           ui->pushButton_proto_shadowsocks_save, ui->label_proto_shadowsocks_info);

        if (!e) {
            m_settings.setContainerConfig(selectedServerIndex, selectedDockerContainer, newContainerConfig);
            m_settings.clearLastConnectionConfig(selectedServerIndex, selectedDockerContainer);
        }
        qDebug() << "Protocol saved with code:" << e << "for" << selectedServerIndex << selectedDockerContainer;
    });

    connect(ui->pushButton_proto_cloak_save, &QPushButton::clicked, this, [this](){
        QJsonObject protocolConfig = m_settings.protocolConfig(selectedServerIndex, selectedDockerContainer, Protocol::Cloak);
        protocolConfig = getCloakConfigFromPage(protocolConfig);

        QJsonObject containerConfig = m_settings.containerConfig(selectedServerIndex, selectedDockerContainer);
        QJsonObject newContainerConfig = containerConfig;
        newContainerConfig.insert(config_key::cloak, protocolConfig);

        ErrorCode e = doInstallAction([this, containerConfig, newContainerConfig](){
            return ServerController::updateContainer(m_settings.serverCredentials(selectedServerIndex), selectedDockerContainer, containerConfig, newContainerConfig);
        },
           ui->page_proto_cloak, ui->progressBar_proto_cloak_reset,
           ui->pushButton_proto_cloak_save, ui->label_proto_cloak_info);

        if (!e) {
            m_settings.setContainerConfig(selectedServerIndex, selectedDockerContainer, newContainerConfig);
            m_settings.clearLastConnectionConfig(selectedServerIndex, selectedDockerContainer);
        }

        qDebug() << "Protocol saved with code:" << e << "for" << selectedServerIndex << selectedDockerContainer;
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

void MainWindow::setupServerSettingsPageConnections()
{
    connect(ui->pushButton_servers_add_new, &QPushButton::clicked, this, [this](){ goToPage(Page::Start); });

    connect(ui->pushButton_server_settings_protocols, &QPushButton::clicked, this, [this](){ goToPage(Page::ServerVpnProtocols); });
    connect(ui->pushButton_server_settings_share_full, &QPushButton::clicked, this, [this](){
        updateSharingPage(selectedServerIndex, m_settings.serverCredentials(selectedServerIndex), DockerContainer::None);
        goToPage(Page::ShareConnection);
    });

    connect(ui->pushButton_server_settings_clear, SIGNAL(clicked(bool)), this, SLOT(onPushButtonClearServer(bool)));
    connect(ui->pushButton_server_settings_forget, SIGNAL(clicked(bool)), this, SLOT(onPushButtonForgetServer(bool)));

    connect(ui->pushButton_server_settings_clear_client_cache, &QPushButton::clicked, this, [this](){
        ui->pushButton_server_settings_clear_client_cache->setText(tr("Cache cleared"));

        const auto &containers = m_settings.containers(selectedServerIndex);
        for (DockerContainer container: containers.keys()) {
            m_settings.clearLastConnectionConfig(selectedServerIndex, container);
        }

        QTimer::singleShot(3000, this, [this]() {
            ui->pushButton_server_settings_clear_client_cache->setText(tr("Clear client cached profile"));
        });
    });

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

void MainWindow::setupSharePageConnections()
{
    connect(ui->pushButton_share_full_copy, &QPushButton::clicked, this, [this](){
        QGuiApplication::clipboard()->setText(ui->textEdit_share_full_code->toPlainText());
        ui->pushButton_share_full_copy->setText(tr("Copied"));

        QTimer::singleShot(3000, this, [this]() {
            ui->pushButton_share_full_copy->setText(tr("Copy"));
        });
    });

    connect(ui->pushButton_share_amnezia_copy, &QPushButton::clicked, this, [this](){
        if (ui->textEdit_share_amnezia_code->toPlainText().isEmpty()) return;

        QGuiApplication::clipboard()->setText(ui->textEdit_share_amnezia_code->toPlainText());
        ui->pushButton_share_amnezia_copy->setText(tr("Copied"));

        QTimer::singleShot(3000, this, [this]() {
            ui->pushButton_share_amnezia_copy->setText(tr("Copy"));
        });
    });

    connect(ui->pushButton_share_amnezia_save, &QPushButton::clicked, this, [this](){
        if (ui->textEdit_share_amnezia_code->toPlainText().isEmpty()) return;

        QString fileName = QFileDialog::getSaveFileName(this, tr("Save AmneziaVPN config"),
            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation), "*.amnezia");
        QSaveFile save(fileName);
        save.open(QIODevice::WriteOnly);
        save.write(ui->textEdit_share_amnezia_code->toPlainText().toUtf8());
        save.commit();
    });

    connect(ui->pushButton_share_openvpn_copy, &QPushButton::clicked, this, [this](){
        QGuiApplication::clipboard()->setText(ui->textEdit_share_openvpn_code->toPlainText());
        ui->pushButton_share_openvpn_copy->setText(tr("Copied"));

        QTimer::singleShot(3000, this, [this]() {
            ui->pushButton_share_openvpn_copy->setText(tr("Copy"));
        });
    });

    connect(ui->pushButton_share_ss_copy, &QPushButton::clicked, this, [this](){
        QGuiApplication::clipboard()->setText(ui->lineEdit_share_ss_string->text());
        ui->pushButton_share_ss_copy->setText(tr("Copied"));

        QTimer::singleShot(3000, this, [this]() {
            ui->pushButton_share_ss_copy->setText(tr("Copy"));
        });
    });

//    connect(ui->pushButton_share_cloak_copy, &QPushButton::clicked, this, [this](){
//        QGuiApplication::clipboard()->setText(ui->textEdit_share_openvpn_code->toPlainText());
//        ui->pushButton_share_cloak_copy->setText(tr("Copied"));

//        QTimer::singleShot(3000, this, [this]() {
//            ui->pushButton_share_cloak_copy->setText(tr("Copy"));
//        });
//    });

    connect(ui->pushButton_share_amnezia_generate, &QPushButton::clicked, this, [this](){
        ui->pushButton_share_amnezia_generate->setEnabled(false);
        ui->pushButton_share_amnezia_copy->setEnabled(false);
        ui->pushButton_share_amnezia_generate->setText(tr("Generating..."));
        qApp->processEvents();

        ServerCredentials credentials = m_settings.serverCredentials(selectedServerIndex);
        QJsonObject containerConfig = m_settings.containerConfig(selectedServerIndex, selectedDockerContainer);
        containerConfig.insert(config_key::container, containerToString(selectedDockerContainer));

        ErrorCode e = ErrorCode::NoError;
        for (Protocol p: amnezia::protocolsForContainer(selectedDockerContainer)) {
            QJsonObject protoConfig = m_settings.protocolConfig(selectedServerIndex, selectedDockerContainer, p);

            QString cfg = VpnConfigurator::genVpnProtocolConfig(credentials, selectedDockerContainer, containerConfig, p, &e);
            if (e) {
                cfg = "Error generating config";
                break;
            }
            protoConfig.insert(config_key::last_config, cfg);

            containerConfig.insert(protoToString(p), protoConfig);
        }

        QByteArray ba;
        if (!e) {
            QJsonObject serverConfig = m_settings.server(selectedServerIndex);
            serverConfig.remove(config_key::userName);
            serverConfig.remove(config_key::password);
            serverConfig.remove(config_key::port);
            serverConfig.insert(config_key::containers, QJsonArray {containerConfig});
            serverConfig.insert(config_key::defaultContainer, containerToString(selectedDockerContainer));


            ba = QJsonDocument(serverConfig).toJson().toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
            ui->textEdit_share_amnezia_code->setPlainText(QString("vpn://%1").arg(QString(ba)));
        }
        else {
            ui->textEdit_share_amnezia_code->setPlainText(tr("Error while generating connection profile"));
        }

        ui->pushButton_share_amnezia_generate->setEnabled(true);
        ui->pushButton_share_amnezia_copy->setEnabled(true);
        ui->pushButton_share_amnezia_generate->setText(tr("Generate config"));
    });

    connect(ui->pushButton_share_openvpn_generate, &QPushButton::clicked, this, [this](){
        ui->pushButton_share_openvpn_generate->setEnabled(false);
        ui->pushButton_share_openvpn_copy->setEnabled(false);
        ui->pushButton_share_openvpn_save->setEnabled(false);
        ui->pushButton_share_openvpn_generate->setText(tr("Generating..."));

        ServerCredentials credentials = m_settings.serverCredentials(selectedServerIndex);
        const QJsonObject &containerConfig = m_settings.containerConfig(selectedServerIndex, selectedDockerContainer);

        ErrorCode e = ErrorCode::NoError;
        QString cfg = OpenVpnConfigurator::genOpenVpnConfig(credentials, selectedDockerContainer, containerConfig, &e);
        cfg = OpenVpnConfigurator::processConfigWithExportSettings(cfg);

        ui->textEdit_share_openvpn_code->setPlainText(cfg);

        ui->pushButton_share_openvpn_generate->setEnabled(true);
        ui->pushButton_share_openvpn_copy->setEnabled(true);
        ui->pushButton_share_openvpn_save->setEnabled(true);
        ui->pushButton_share_openvpn_generate->setText(tr("Generate config"));
    });

    connect(ui->pushButton_share_openvpn_save, &QPushButton::clicked, this, [this](){
        QString fileName = QFileDialog::getSaveFileName(this, tr("Save OpenVPN config"),
            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation), "*.ovpn");

        QSaveFile save(fileName);
        save.open(QIODevice::WriteOnly);
        save.write(ui->textEdit_share_openvpn_code->toPlainText().toUtf8());
        save.commit();
    });
}

void MainWindow::setTrayState(VpnProtocol::ConnectionState state)
{
    QString resourcesPath = ":/images/tray/%1";

    m_trayActionDisconnect->setEnabled(state == VpnProtocol::Connected);
    m_trayActionConnect->setEnabled(state == VpnProtocol::Disconnected);

    switch (state) {
    case VpnProtocol::Disconnected:
        setTrayIcon(QString(resourcesPath).arg(DisconnectedTrayIconName));
        break;
    case VpnProtocol::Preparing:
        setTrayIcon(QString(resourcesPath).arg(DisconnectedTrayIconName));
        break;
    case VpnProtocol::Connecting:
        setTrayIcon(QString(resourcesPath).arg(DisconnectedTrayIconName));
        break;
    case VpnProtocol::Connected:
        setTrayIcon(QString(resourcesPath).arg(ConnectedTrayIconName));
        break;
    case VpnProtocol::Disconnecting:
        setTrayIcon(QString(resourcesPath).arg(DisconnectedTrayIconName));
        break;
    case VpnProtocol::Reconnecting:
        setTrayIcon(QString(resourcesPath).arg(DisconnectedTrayIconName));
        break;
    case VpnProtocol::Error:
        setTrayIcon(QString(resourcesPath).arg(ErrorTrayIconName));
        break;
    case VpnProtocol::Unknown:
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

    if (m_settings.containers(serverIndex).isEmpty()) {
        ui->label_error_text->setText(tr("VPN Protocols is not installed.\n Please install VPN container at first"));
        ui->pushButton_connect->setChecked(false);
        return;
    }

    if (container == DockerContainer::None) {
        ui->label_error_text->setText(tr("VPN Protocol not choosen"));
        ui->pushButton_connect->setChecked(false);
        return;
    }


    const QJsonObject &containerConfig = m_settings.containerConfig(serverIndex, container);
    onConnectWorker(serverIndex, credentials, container, containerConfig);
}

void MainWindow::onConnectWorker(int serverIndex, const ServerCredentials &credentials, DockerContainer container, const QJsonObject &containerConfig)
{
    ui->label_error_text->clear();
    ui->pushButton_connect->setChecked(true);
    qApp->processEvents();

    ErrorCode errorCode = m_vpnConnection->connectToVpn(
        serverIndex, credentials, container, containerConfig
    );

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
    if (ui->radioButton_vpn_mode_all_sites->isChecked()) return;
    Settings::RouteMode mode = m_settings.routeMode();

    QString newSite = ui->lineEdit_sites_add_custom->text();

    if (newSite.isEmpty()) return;
    if (!newSite.contains(".")) return;

    if (!Utils::ipAddressWithSubnetRegExp().exactMatch(newSite)) {
        // get domain name if it present
        newSite.replace("https://", "");
        newSite.replace("http://", "");
        newSite.replace("ftp://", "");

        newSite = newSite.split("/", QString::SkipEmptyParts).first();
    }

    const auto &cbProcess = [this, mode](const QString &newSite, const QString &ip) {
        m_settings.addVpnSite(mode, newSite, ip);

        if (!ip.isEmpty()) {
            m_vpnConnection->addRoutes(QStringList() << ip);
            m_vpnConnection->flushDns();
        }

        updateSitesPage();
    };

    const auto &cbResolv = [this, cbProcess](const QHostInfo &hostInfo){
        const QList<QHostAddress> &addresses = hostInfo.addresses();
        QString ipv4Addr;
        for (const QHostAddress &addr: hostInfo.addresses()) {
            if (addr.protocol() == QAbstractSocket::NetworkLayerProtocol::IPv4Protocol) {
                cbProcess(hostInfo.hostName(), addr.toString());
                break;
            }
        }
    };

    ui->lineEdit_sites_add_custom->clear();

    if (Utils::ipAddressWithSubnetRegExp().exactMatch(newSite)) {
        cbProcess(newSite, "");
        return;
    }
    else {
        cbProcess(newSite, "");
        updateSitesPage();
        QHostInfo::lookupHost(newSite, this, cbResolv);
    }
}

void MainWindow::updateStartPage()
{
    ui->lineEdit_start_existing_code->clear();
    ui->textEdit_new_server_ssh_key->clear();
    ui->lineEdit_new_server_ip->clear();
    ui->lineEdit_new_server_password->clear();
    ui->textEdit_new_server_ssh_key->clear();
    ui->lineEdit_new_server_login->setText("root");

    ui->label_new_server_wait_info->hide();
    ui->label_new_server_wait_info->clear();

    ui->progressBar_new_server_connection->setMinimum(0);
    ui->progressBar_new_server_connection->setMaximum(300);

    ui->pushButton_back_from_start->setVisible(!pagesStack.isEmpty());

    ui->pushButton_new_server_connect->setVisible(true);
}

void MainWindow::updateSitesPage()
{
    Settings::RouteMode m = m_settings.routeMode();
    if (m == Settings::VpnAllSites) return;

    if (m == Settings::VpnOnlyForwardSites) ui->label_sites_add_custom->setText(tr("These sites will be opened using VPN"));
    if (m == Settings::VpnAllExceptSites) ui->label_sites_add_custom->setText(tr("These sites will be excepted from VPN"));

    ui->tableView_sites->setModel(sitesModels.value(m));
    sitesModels.value(m)->resetCache();
}

void MainWindow::updateVpnPage()
{
    Settings::RouteMode mode = m_settings.routeMode();
    ui->radioButton_vpn_mode_all_sites->setChecked(mode == Settings::VpnAllSites);
    ui->radioButton_vpn_mode_forward_sites->setChecked(mode == Settings::VpnOnlyForwardSites);
    ui->radioButton_vpn_mode_except_sites->setChecked(mode == Settings::VpnAllExceptSites);
    ui->pushButton_vpn_add_site->setEnabled(mode != Settings::VpnAllSites);
}

void MainWindow::updateAppSettingsPage()
{
    ui->checkBox_app_settings_autostart->setChecked(Autostart::isAutostart());
    ui->checkBox_app_settings_autoconnect->setChecked(m_settings.isAutoConnect());
    ui->checkBox_app_settings_start_minimized->setChecked(m_settings.isStartMinimized());

    ui->lineEdit_network_settings_dns1->setText(m_settings.primaryDns());
    ui->lineEdit_network_settings_dns2->setText(m_settings.secondaryDns());

    QString ver = QString("%1: %2 (%3)")
            .arg(tr("Software version"))
            .arg(QString(APP_MAJOR_VERSION))
            .arg(__DATE__);
    ui->label_app_settings_version->setText(ver);
}

void MainWindow::updateGeneralSettingPage()
{
    ui->pushButton_general_settings_share_connection->setEnabled(m_settings.haveAuthData(m_settings.defaultServerIndex()));
}

void MainWindow::updateServerPage()
{
    ui->label_server_settings_wait_info->hide();
    ui->label_server_settings_wait_info->clear();

    ui->pushButton_server_settings_clear->setVisible(m_settings.haveAuthData(selectedServerIndex));
    ui->pushButton_server_settings_clear_client_cache->setVisible(m_settings.haveAuthData(selectedServerIndex));
    ui->pushButton_server_settings_share_full->setVisible(m_settings.haveAuthData(selectedServerIndex));

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

    bool haveAuthData = m_settings.haveAuthData(selectedServerIndex);

    DockerContainer defaultContainer = m_settings.defaultContainer(selectedServerIndex);
    ui->pushButton_proto_cloak_openvpn_cont_default->setChecked(defaultContainer == DockerContainer::OpenVpnOverCloak);
    ui->pushButton_proto_ss_openvpn_cont_default->setChecked(defaultContainer == DockerContainer::OpenVpnOverShadowSocks);
    ui->pushButton_proto_openvpn_cont_default->setChecked(defaultContainer == DockerContainer::OpenVpn);

    ui->pushButton_proto_cloak_openvpn_cont_default->setVisible(haveAuthData && containers.contains(DockerContainer::OpenVpnOverCloak));
    ui->pushButton_proto_ss_openvpn_cont_default->setVisible(haveAuthData && containers.contains(DockerContainer::OpenVpnOverShadowSocks));
    ui->pushButton_proto_openvpn_cont_default->setVisible(haveAuthData && containers.contains(DockerContainer::OpenVpn));

    ui->pushButton_proto_cloak_openvpn_cont_share->setVisible(haveAuthData && containers.contains(DockerContainer::OpenVpnOverCloak));
    ui->pushButton_proto_ss_openvpn_cont_share->setVisible(haveAuthData && containers.contains(DockerContainer::OpenVpnOverShadowSocks));
    ui->pushButton_proto_openvpn_cont_share->setVisible(haveAuthData && containers.contains(DockerContainer::OpenVpn));

    ui->pushButton_proto_cloak_openvpn_cont_install->setChecked(containers.contains(DockerContainer::OpenVpnOverCloak));
    ui->pushButton_proto_ss_openvpn_cont_install->setChecked(containers.contains(DockerContainer::OpenVpnOverShadowSocks));
    ui->pushButton_proto_openvpn_cont_install->setChecked(containers.contains(DockerContainer::OpenVpn));

    ui->pushButton_proto_cloak_openvpn_cont_install->setEnabled(haveAuthData);
    ui->pushButton_proto_ss_openvpn_cont_install->setEnabled(haveAuthData);
    ui->pushButton_proto_openvpn_cont_install->setEnabled(haveAuthData);

    ui->frame_openvpn_ss_cloak_settings->setVisible(containers.contains(DockerContainer::OpenVpnOverCloak));
    ui->frame_openvpn_ss_settings->setVisible(containers.contains(DockerContainer::OpenVpnOverShadowSocks));
    ui->frame_openvpn_settings->setVisible(containers.contains(DockerContainer::OpenVpn));
}

void MainWindow::updateOpenVpnPage(const QJsonObject &openvpnConfig, DockerContainer container, bool haveAuthData)
{   
    ui->widget_proto_openvpn->setEnabled(haveAuthData);
    ui->pushButton_proto_openvpn_save->setVisible(haveAuthData);
    ui->progressBar_proto_openvpn_reset->setVisible(haveAuthData);

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

    bool isTlsAuth = openvpnConfig.value(config_key::tls_auth).toBool(protocols::openvpn::defaultTlsAuth);
    ui->checkBox_proto_openvpn_tls_auth->setChecked(isTlsAuth);

    if (container == DockerContainer::OpenVpnOverShadowSocks) {
        ui->radioButton_proto_openvpn_udp->setEnabled(false);
        ui->radioButton_proto_openvpn_tcp->setEnabled(false);
        ui->radioButton_proto_openvpn_tcp->setChecked(true);
    }

    ui->lineEdit_proto_openvpn_port->setText(openvpnConfig.value(config_key::port).
        toString(protocols::openvpn::defaultPort));

    ui->lineEdit_proto_openvpn_port->setEnabled(container == DockerContainer::OpenVpn);
}

void MainWindow::updateShadowSocksPage(const QJsonObject &ssConfig, DockerContainer container, bool haveAuthData)
{
    ui->widget_proto_ss->setEnabled(haveAuthData);
    ui->pushButton_proto_shadowsocks_save->setVisible(haveAuthData);
    ui->progressBar_proto_shadowsocks_reset->setVisible(haveAuthData);

    ui->comboBox_proto_shadowsocks_cipher->setCurrentText(ssConfig.value(config_key::cipher).
        toString(protocols::shadowsocks::defaultCipher));

    ui->lineEdit_proto_shadowsocks_port->setText(ssConfig.value(config_key::port).
        toString(protocols::shadowsocks::defaultPort));

    ui->lineEdit_proto_shadowsocks_port->setEnabled(container == DockerContainer::OpenVpnOverShadowSocks);
}

void MainWindow::updateCloakPage(const QJsonObject &ckConfig, DockerContainer container, bool haveAuthData)
{
    ui->widget_proto_cloak->setEnabled(haveAuthData);
    ui->pushButton_proto_cloak_save->setVisible(haveAuthData);
    ui->progressBar_proto_cloak_reset->setVisible(haveAuthData);

    ui->comboBox_proto_cloak_cipher->setCurrentText(ckConfig.value(config_key::cipher).
        toString(protocols::cloak::defaultCipher));

    ui->lineEdit_proto_cloak_site->setText(ckConfig.value(config_key::site).
        toString(protocols::cloak::defaultRedirSite));

    ui->lineEdit_proto_cloak_port->setText(ckConfig.value(config_key::port).
        toString(protocols::cloak::defaultPort));

    ui->lineEdit_proto_cloak_port->setEnabled(container == DockerContainer::OpenVpnOverCloak);
}

void MainWindow::updateSharingPage(int serverIndex, const ServerCredentials &credentials,
    DockerContainer container)
{
    selectedDockerContainer = container;
    selectedServerIndex = serverIndex;

    //const QJsonObject &containerConfig = m_settings.containerConfig(serverIndex, container);

    for (QWidget *page : {
         ui->page_share_amnezia,
         ui->page_share_openvpn,
         ui->page_share_shadowsocks,
         ui->page_share_cloak,
         ui->page_share_full_access }) {

        ui->toolBox_share_connection->removeItem(ui->toolBox_share_connection->indexOf(page));
        page->hide();
    }

    if (container == DockerContainer::OpenVpn) {
        ui->toolBox_share_connection->addItem(ui->page_share_amnezia, tr("  Share for Amnezia client"));
        ui->toolBox_share_connection->addItem(ui->page_share_openvpn, tr("  Share for OpenVPN client"));

        QString cfg = tr("Press Generate config");
        ui->textEdit_share_openvpn_code->setPlainText(cfg);
        ui->pushButton_share_openvpn_copy->setEnabled(false);
        ui->pushButton_share_openvpn_save->setEnabled(false);

//        QJsonObject protoConfig = m_settings.protocolConfig(serverIndex, container, Protocol::OpenVpn);
//        QString cfg = protoConfig.value(config_key::last_config).toString();
//        if (!cfg.isEmpty()) {
//            // TODO add redirect-gateway def1 bypass-dhcp here and on click Generate config
//            ui->textEdit_share_openvpn_code->setPlainText(cfg);
//        }
//        else {
//            cfg = tr("Press Generate config");
//            ui->textEdit_share_openvpn_code->setPlainText(cfg);
//            ui->pushButton_share_openvpn_copy->setEnabled(false);
//            ui->pushButton_share_openvpn_save->setEnabled(false);
//        }
        ui->toolBox_share_connection->setCurrentWidget(ui->page_share_openvpn);
    }

    if (container == DockerContainer::OpenVpnOverShadowSocks) {
        ui->toolBox_share_connection->addItem(ui->page_share_amnezia, tr("  Share for Amnezia client"));
        ui->toolBox_share_connection->addItem(ui->page_share_shadowsocks, tr("  Share for ShadowSocks client"));

        QJsonObject protoConfig = m_settings.protocolConfig(serverIndex, container, Protocol::ShadowSocks);
        QString cfg = protoConfig.value(config_key::last_config).toString();

        if (cfg.isEmpty()) {
            const QJsonObject &containerConfig = m_settings.containerConfig(serverIndex, container);

            ErrorCode e = ErrorCode::NoError;
            cfg = ShadowSocksConfigurator::genShadowSocksConfig(credentials, container, containerConfig, &e);

            ui->pushButton_share_ss_copy->setEnabled(true);
        }

        QJsonObject ssConfig = QJsonDocument::fromJson(cfg.toUtf8()).object();

        QString ssString = QString("%1:%2@%3:%4")
                .arg(ssConfig.value("method").toString())
                .arg(ssConfig.value("password").toString())
                .arg(ssConfig.value("server").toString())
                .arg(ssConfig.value("server_port").toString());

        ssString = "ss://" + ssString.toUtf8().toBase64();
        ui->lineEdit_share_ss_string->setText(ssString);
        updateQRCodeImage(ssString, ui->label_share_ss_qr_code);

        ui->label_share_ss_server->setText(ssConfig.value("server").toString());
        ui->label_share_ss_port->setText(ssConfig.value("server_port").toString());
        ui->label_share_ss_method->setText(ssConfig.value("method").toString());
        ui->label_share_ss_password->setText(ssConfig.value("password").toString());

        ui->toolBox_share_connection->setCurrentWidget(ui->page_share_shadowsocks);
        ui->page_share_shadowsocks->show();
        ui->page_share_shadowsocks->raise();
        qDebug() << ui->page_share_shadowsocks->size();
        ui->toolBox_share_connection->layout()->update();
    }

    if (container == DockerContainer::OpenVpnOverCloak) {
        ui->toolBox_share_connection->addItem(ui->page_share_amnezia, tr("  Share for Amnezia client"));
    }

    // Full access
    if (container == DockerContainer::None) {
        ui->toolBox_share_connection->addItem(ui->page_share_full_access, tr("  Share server full access"));

        const QJsonObject &server = m_settings.server(selectedServerIndex);

        QByteArray ba = QJsonDocument(server).toJson().toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);

        ui->textEdit_share_full_code->setText(QString("vpn://%1").arg(QString(ba)));
        ui->toolBox_share_connection->setCurrentWidget(ui->page_share_full_access);
    }

    //ui->toolBox_share_connection->addItem(ui->page_share_amnezia, tr("  Share for Amnezia client"));

    // Amnezia sharing
//    QJsonObject exportContainer;
//    for (Protocol p: protocolsForContainer(container)) {
//        QJsonObject protocolConfig = containerConfig.value(protoToString(p)).toObject();
//        protocolConfig.remove(config_key::last_config);
//        exportContainer.insert(protoToString(p), protocolConfig);
//    }
//    exportContainer.insert(config_key::container, containerToString(container));

//    ui->textEdit_share_amnezia_code->setPlainText(QJsonDocument(exportContainer).toJson());

    ui->textEdit_share_amnezia_code->setPlainText(tr(""));
}

void MainWindow::makeServersListItem(QListWidget *listWidget, const QJsonObject &server, bool isDefault, int index)
{
    QSize size(310, 70);
    ServerWidget* widget = new ServerWidget(server, isDefault);

    widget->resize(size);

    connect(widget->ui->pushButton_default, &QPushButton::clicked, this, [this, index](){
        m_settings.setDefaultServer(index);
        updateServersListPage();
    });

//    connect(widget->ui->pushButton_share, &QPushButton::clicked, this, [this, index](){
//        goToPage(Page::ShareConnection);
//        // update share page
//    });

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
    oldConfig.insert(config_key::port, ui->lineEdit_proto_openvpn_port->text());
    oldConfig.insert(config_key::tls_auth, ui->checkBox_proto_openvpn_tls_auth->isChecked());

    return oldConfig;
}

QJsonObject MainWindow::getShadowSocksConfigFromPage(QJsonObject oldConfig)
{
    oldConfig.insert(config_key::cipher, ui->comboBox_proto_shadowsocks_cipher->currentText());
    oldConfig.insert(config_key::port, ui->lineEdit_proto_shadowsocks_port->text());

    return oldConfig;
}

QJsonObject MainWindow::getCloakConfigFromPage(QJsonObject oldConfig)
{
    oldConfig.insert(config_key::cipher, ui->comboBox_proto_cloak_cipher->currentText());
    oldConfig.insert(config_key::site, ui->lineEdit_proto_cloak_site->text());
    oldConfig.insert(config_key::port, ui->lineEdit_proto_cloak_port->text());

    return oldConfig;
}
