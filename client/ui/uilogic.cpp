#include <QApplication>
#include <QClipboard>
#include <QDebug>
#include <QDesktopServices>
#include <QFile>
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
#include <QRegularExpression>
#include <QQmlFile>
#include <QStandardPaths>
#include "configurators/cloak_configurator.h"
#include "configurators/vpn_configurator.h"
#include "configurators/openvpn_configurator.h"
#include "configurators/shadowsocks_configurator.h"
#include "configurators/ssh_configurator.h"

#include "core/servercontroller.h"
#include "core/server_defs.h"
#include "core/errorstrings.h"

#include "containers/containers_defs.h"

#include "ui/qautostart.h"

#include "debug.h"
#include "defines.h"
#include "uilogic.h"
#include "utilities.h"
#include "vpnconnection.h"
#include <functional>

#if defined Q_OS_MAC || defined Q_OS_LINUX
#include "ui/macos_util.h"
#endif

#ifdef Q_OS_ANDROID
#include "platforms/android/android_controller.h"
#endif

#include "platforms/ios/MobileUtils.h"

#include "pages_logic/AppSettingsLogic.h"
#include "pages_logic/GeneralSettingsLogic.h"
#include "pages_logic/NetworkSettingsLogic.h"
#include "pages_logic/NewServerProtocolsLogic.h"
#include "pages_logic/QrDecoderLogic.h"
#include "pages_logic/ServerConfiguringProgressLogic.h"
#include "pages_logic/ServerListLogic.h"
#include "pages_logic/ServerSettingsLogic.h"
#include "pages_logic/ServerContainersLogic.h"
#include "pages_logic/ShareConnectionLogic.h"
#include "pages_logic/SitesLogic.h"
#include "pages_logic/StartPageLogic.h"
#include "pages_logic/VpnLogic.h"
#include "pages_logic/WizardLogic.h"

#include "pages_logic/protocols/CloakLogic.h"
#include "pages_logic/protocols/OpenVpnLogic.h"
#include "pages_logic/protocols/ShadowSocksLogic.h"
#include "pages_logic/protocols/OtherProtocolsLogic.h"


using namespace amnezia;
using namespace PageEnumNS;

UiLogic::UiLogic(QObject *parent) :
    QObject(parent),
    m_dialogConnectErrorText{}
{
    m_containersModel = new ContainersModel(this);
    m_protocolsModel = new ProtocolsModel(this);
    m_vpnConnection = new VpnConnection();
    m_vpnConnection->moveToThread(&m_vpnConnectionThread);
    m_vpnConnectionThread.start();

    m_appSettingsLogic = new AppSettingsLogic(this);
    m_generalSettingsLogic = new GeneralSettingsLogic(this);
    m_networkSettingsLogic = new NetworkSettingsLogic(this);
    m_newServerProtocolsLogic = new NewServerProtocolsLogic(this);
    m_qrDecoderLogic = new QrDecoderLogic(this);
    m_serverConfiguringProgressLogic = new ServerConfiguringProgressLogic(this);
    m_serverListLogic = new ServerListLogic(this);
    m_serverSettingsLogic = new ServerSettingsLogic(this);
    m_serverprotocolsLogic = new ServerContainersLogic(this);
    m_shareConnectionLogic = new ShareConnectionLogic(this);
    m_sitesLogic = new SitesLogic(this);
    m_startPageLogic = new StartPageLogic(this);
    m_vpnLogic = new VpnLogic(this);
    m_wizardLogic = new WizardLogic(this);

    m_protocolLogicMap.insert(Proto::OpenVpn, new OpenVpnLogic(this));
    m_protocolLogicMap.insert(Proto::ShadowSocks, new ShadowSocksLogic(this));
    m_protocolLogicMap.insert(Proto::Cloak, new CloakLogic(this));
    //m_protocolLogicMap->insert(Proto::WireGuard, new WireguardLogic(this));

    m_protocolLogicMap.insert(Proto::Dns, new OtherProtocolsLogic(this));
    m_protocolLogicMap.insert(Proto::Sftp, new OtherProtocolsLogic(this));
    m_protocolLogicMap.insert(Proto::TorWebSite, new OtherProtocolsLogic(this));

}

UiLogic::~UiLogic()
{
    emit hide();

#ifdef AMNEZIA_DESKTOP
    if (m_vpnConnection->connectionState() != VpnProtocol::VpnConnectionState::Disconnected) {
        m_vpnConnection->disconnectFromVpn();
        for (int i = 0; i < 50; i++) {
            qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
            QThread::msleep(100);
            if (m_vpnConnection->isDisconnected()) {
                break;
            }
        }
    }
#endif

    m_vpnConnection->deleteLater();
    m_vpnConnectionThread.quit();
    m_vpnConnectionThread.wait(3000);

    qDebug() << "Application closed";
}

void UiLogic::initalizeUiLogic()
{
#ifdef Q_OS_ANDROID
    connect(AndroidController::instance(), &AndroidController::initialized, [this](bool status, bool connected, const QDateTime& connectionDate) {
        if (connected) {
            vpnLogic()->onConnectionStateChanged(VpnProtocol::Connected);
        }
    });
    if (!AndroidController::instance()->initialize()) {
         qDebug() << QString("Init failed") ;
         emit VpnProtocol::Error;
         return;
    }
#endif

    qDebug() << "UiLogic::initalizeUiLogic()";

    m_notificationHandler = NotificationHandler::create(qmlRoot());

    connect(m_vpnConnection, &VpnConnection::connectionStateChanged, m_notificationHandler, &NotificationHandler::setConnectionState);
    connect(m_notificationHandler, &NotificationHandler::raiseRequested, this, &UiLogic::raise);
    connect(m_notificationHandler, &NotificationHandler::connectRequested, vpnLogic(), &VpnLogic::onConnect);
    connect(m_notificationHandler, &NotificationHandler::disconnectRequested, vpnLogic(), &VpnLogic::onDisconnect);

    if (m_settings.serversCount() > 0) {
        if (m_settings.defaultServerIndex() < 0) m_settings.setDefaultServer(0);
        emit goToPage(Page::Vpn, true, false);
    }
    else {
        emit goToPage(Page::Start, true, false);
    }

    selectedServerIndex = m_settings.defaultServerIndex();

    qInfo().noquote() << QString("Started %1 version %2").arg(APPLICATION_NAME).arg(APP_VERSION);
    qInfo().noquote() << QString("%1 (%2)").arg(QSysInfo::prettyProductName()).arg(QSysInfo::currentCpuArchitecture());
}

QString UiLogic::getDialogConnectErrorText() const
{
    return m_dialogConnectErrorText;
}

void UiLogic::setDialogConnectErrorText(const QString &dialogConnectErrorText)
{
    if (m_dialogConnectErrorText != dialogConnectErrorText) {
        m_dialogConnectErrorText = dialogConnectErrorText;
        emit dialogConnectErrorTextChanged();
    }
}

void UiLogic::showOnStartup()
{
    if (! m_settings.isStartMinimized()) {
        emit show();
    }
    else {
#ifdef Q_OS_WIN
        emit hide();
#elif defined Q_OS_MACX
    // TODO: fix: setDockIconVisible(false);
#endif
    }
}

void UiLogic::onUpdateAllPages()
{
    for (PageLogicBase *logic : {
         (PageLogicBase *) m_appSettingsLogic,
         (PageLogicBase *) m_generalSettingsLogic,
         (PageLogicBase *) m_networkSettingsLogic,
         (PageLogicBase *) m_serverConfiguringProgressLogic,
         (PageLogicBase *) m_newServerProtocolsLogic,
         (PageLogicBase *) m_serverListLogic,
         (PageLogicBase *) m_serverSettingsLogic,
         (PageLogicBase *) m_serverprotocolsLogic,
         (PageLogicBase *) m_shareConnectionLogic,
         (PageLogicBase *) m_sitesLogic,
         (PageLogicBase *) m_startPageLogic,
         (PageLogicBase *) m_vpnLogic,
         (PageLogicBase *) m_wizardLogic
    }) {
        logic->onUpdatePage();
    }
}

void UiLogic::keyPressEvent(Qt::Key key)
{
    switch (key) {
    case Qt::Key_AsciiTilde:
    case Qt::Key_QuoteLeft: emit toggleLogPanel();
        break;
    case Qt::Key_L: Debug::openLogsFolder();
        break;
    case Qt::Key_K: Debug::openServiceLogsFolder();
        break;
#ifdef QT_DEBUG
    case Qt::Key_Q:
        qApp->quit();
        break;
    case Qt::Key_H:
        selectedServerIndex = m_settings.defaultServerIndex();
        selectedDockerContainer = m_settings.defaultContainer(selectedServerIndex);

        //updateSharingPage(selectedServerIndex, m_settings.serverCredentials(selectedServerIndex), selectedDockerContainer);
        emit goToPage(Page::ShareConnection);
        break;
#endif
    case Qt::Key_C:
        qDebug().noquote() << "Def server" << m_settings.defaultServerIndex() << m_settings.defaultContainerName(m_settings.defaultServerIndex());
        //qDebug().noquote() << QJsonDocument(m_settings.containerConfig(m_settings.defaultServerIndex(), m_settings.defaultContainer(m_settings.defaultServerIndex()))).toJson();
        qDebug().noquote() << QJsonDocument(m_settings.defaultServer()).toJson();
        break;
    case Qt::Key_A:
        emit goToPage(Page::Start);
        break;
    case Qt::Key_S:
        selectedServerIndex = m_settings.defaultServerIndex();
        emit goToPage(Page::ServerSettings);
        break;
    case Qt::Key_P:
        onGotoCurrentProtocolsPage();
        break;
    case Qt::Key_T:
        SshConfigurator::openSshTerminal(m_settings.serverCredentials(m_settings.defaultServerIndex()));
        break;
    case Qt::Key_Escape:
    case Qt::Key_Back:
        if (currentPage() == Page::Vpn) break;
        if (currentPage() == Page::ServerConfiguringProgress) break;
//        if (currentPage() == Page::Start && pagesStack.size() < 2) break;
//        if (currentPage() == Page::Sites &&
//                ui->tableView_sites->selectionModel()->selection().indexes().size() > 0) {
//            ui->tableView_sites->clearSelection();
//            break;
//        }

            emit closePage();
        //}
    default:
        ;
    }
}

void UiLogic::onCloseWindow()
{
    if (m_settings.serversCount() == 0) qApp->quit();
    else {
        hide();
    }
}

QString UiLogic::containerName(int container)
{
    return ContainerProps::containerHumanNames().value(static_cast<DockerContainer>(container));
}

QString UiLogic::containerDesc(int container)
{
    return ContainerProps::containerDescriptions().value(static_cast<DockerContainer>(container));

}

void UiLogic::onGotoCurrentProtocolsPage()
{
    selectedServerIndex = m_settings.defaultServerIndex();
    selectedDockerContainer = m_settings.defaultContainer(selectedServerIndex);
    emit goToPage(Page::ServerContainers);
}



//void UiLogic::showEvent(QShowEvent *event)
//{
//#if defined Q_OS_MACX
//    if (!event->spontaneous()) {
//        setDockIconVisible(true);
//    }
//    if (needToHideCustomTitlebar) {
//        ui->widget_tittlebar->hide();
//        resize(width(), 640);
//        ui->stackedWidget_main->move(0,0);
//    }
//#endif
//}

//void UiLogic::hideEvent(QHideEvent *event)
//{
//#if defined Q_OS_MACX
//    if (!event->spontaneous()) {
//        setDockIconVisible(false);
//    }
//#endif
//}




void UiLogic::installServer(QMap<DockerContainer, QJsonObject> &containers)
{
    if (containers.isEmpty()) return;

    emit goToPage(Page::ServerConfiguringProgress);
    QEventLoop loop;
    QTimer::singleShot(500, &loop, SLOT(quit()));
    loop.exec();
    qApp->processEvents();

    PageFunc page_new_server_configuring;
    page_new_server_configuring.setEnabledFunc = [this] (bool enabled) -> void {
        serverConfiguringProgressLogic()->set_pageEnabled(enabled);
    };
    ButtonFunc no_button;
    LabelFunc label_new_server_configuring_wait_info;
    label_new_server_configuring_wait_info.setTextFunc = [this] (const QString& text) -> void {
        serverConfiguringProgressLogic()->set_labelWaitInfoText(text);
    };
    label_new_server_configuring_wait_info.setVisibleFunc = [this] (bool visible) ->void {
        serverConfiguringProgressLogic()->set_labelWaitInfoVisible(visible);
    };
    ProgressFunc progressBar_new_server_configuring;
    progressBar_new_server_configuring.setVisibleFunc = [this] (bool visible) ->void {
        serverConfiguringProgressLogic()->set_progressBarVisible(visible);
    };
    progressBar_new_server_configuring.setValueFunc = [this] (int value) ->void {
        serverConfiguringProgressLogic()->set_progressBarValue(value);
    };
    progressBar_new_server_configuring.getValueFunc = [this] (void) -> int {
        return serverConfiguringProgressLogic()->progressBarValue();
    };
    progressBar_new_server_configuring.getMaximiumFunc = [this] (void) -> int {
        return serverConfiguringProgressLogic()->progressBarMaximium();
    };
    progressBar_new_server_configuring.setTextVisibleFunc = [this] (bool visible) ->void {
        serverConfiguringProgressLogic()->set_progressBarTextVisible(visible);
    };
    progressBar_new_server_configuring.setTextFunc = [this] (const QString& text) ->void {
        serverConfiguringProgressLogic()->set_progressBarText(text);
    };
    bool ok = installContainers(installCredentials, containers,
                                page_new_server_configuring,
                                progressBar_new_server_configuring,
                                no_button,
                                label_new_server_configuring_wait_info);

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
        server.insert(config_key::defaultContainer, ContainerProps::containerToString(containers.firstKey()));

        m_settings.addServer(server);
        m_settings.setDefaultServer(m_settings.serversCount() - 1);
        onUpdateAllPages();

        emit setStartPage(Page::Vpn);
        qApp->processEvents();
    }
    else {
        emit closePage();
    }
}

bool UiLogic::installContainers(ServerCredentials credentials,
                                QMap<DockerContainer, QJsonObject> &containers,
                                const PageFunc &page,
                                const ProgressFunc &progress,
                                const ButtonFunc &button,
                                const LabelFunc &info)
{
    if (!progress.setValueFunc) return false;

    if (page.setEnabledFunc) {
        page.setEnabledFunc(false);
    }
    if (button.setVisibleFunc) {
        button.setVisibleFunc(false);
    }

    if (info.setVisibleFunc) {
        info.setVisibleFunc(true);
    }
    if (info.setTextFunc) {
        info.setTextFunc(tr("Please wait, configuring process may take up to 5 minutes"));
    }

    int cnt = 0;
    for (QMap<DockerContainer, QJsonObject>::iterator i = containers.begin(); i != containers.end(); i++, cnt++) {
        QTimer timer;
        connect(&timer, &QTimer::timeout, [progress](){
            progress.setValueFunc(progress.getValueFunc() + 1);
        });

        progress.setValueFunc(0);
        timer.start(1000);

        progress.setTextVisibleFunc(true);
        progress.setTextFunc(QString("Installing %1 %2 %3").arg(cnt+1).arg(tr("of")).arg(containers.size()));

        ErrorCode e = ServerController::setupContainer(credentials, i.key(), i.value());
        qDebug() << "Setup server finished with code" << e;
        ServerController::disconnectFromHost(credentials);

        if (e) {
            if (page.setEnabledFunc) {
                page.setEnabledFunc(true);
            }
            if (button.setVisibleFunc) {
                button.setVisibleFunc(true);
            }
            if (info.setVisibleFunc) {
                info.setVisibleFunc(false);
            }

            QMessageBox::warning(nullptr, APPLICATION_NAME,
                                 tr("Error occurred while configuring server.") + "\n" +
                                 errorString(e));

            return false;
        }

        // just ui progressbar tweak
        timer.stop();

        int remaining_val = progress.getMaximiumFunc() - progress.getValueFunc();

        if (remaining_val > 0) {
            QTimer timer1;
            QEventLoop loop1;

            connect(&timer1, &QTimer::timeout, [&](){
                progress.setValueFunc(progress.getValueFunc() + 1);
                if (progress.getValueFunc() >= progress.getMaximiumFunc()) {
                    loop1.quit();
                }
            });

            timer1.start(5);
            loop1.exec();
        }
    }


    if (button.setVisibleFunc) {
        button.setVisibleFunc(true);
    }
    if (page.setEnabledFunc) {
        page.setEnabledFunc(true);
    }
    if (info.setTextFunc) {
        info.setTextFunc(tr("Amnezia server installed"));
    }

    return true;
}

ErrorCode UiLogic::doInstallAction(const std::function<ErrorCode()> &action,
                                   const PageFunc &page,
                                   const ProgressFunc &progress,
                                   const ButtonFunc &button,
                                   const LabelFunc &info)
{
    progress.setVisibleFunc(true);
    if (page.setEnabledFunc) {
        page.setEnabledFunc(false);
    }
    if (button.setVisibleFunc) {
        button.setVisibleFunc(false);
    }
    if (info.setVisibleFunc) {
        info.setVisibleFunc(true);
    }
    if (info.setTextFunc) {
        info.setTextFunc(tr("Please wait, configuring process may take up to 5 minutes"));
    }

    QTimer timer;
    connect(&timer, &QTimer::timeout, [progress](){
        progress.setValueFunc(progress.getValueFunc() + 1);
    });

    progress.setValueFunc(0);
    timer.start(1000);

    ErrorCode e = action();
    qDebug() << "doInstallAction finished with code" << e;

    if (e) {
        if (page.setEnabledFunc) {
            page.setEnabledFunc(true);
        }
        if (button.setVisibleFunc) {
            button.setVisibleFunc(true);
        }
        if (info.setVisibleFunc) {
            info.setVisibleFunc(false);
        }
        QMessageBox::warning(nullptr, APPLICATION_NAME,
                             tr("Error occurred while configuring server.") + "\n" +
                             errorString(e));

        progress.setVisibleFunc(false);
        return e;
    }

    // just ui progressbar tweak
    timer.stop();

    int remaining_val = progress.getMaximiumFunc() - progress.getValueFunc();

    if (remaining_val > 0) {
        QTimer timer1;
        QEventLoop loop1;

        connect(&timer1, &QTimer::timeout, [&](){
            progress.setValueFunc(progress.getValueFunc() + 1);
            if (progress.getValueFunc() >= progress.getMaximiumFunc()) {
                loop1.quit();
            }
        });

        timer1.start(5);
        loop1.exec();
    }


    progress.setVisibleFunc(false);
    if (button.setVisibleFunc) {
        button.setVisibleFunc(true);
    }
    if (page.setEnabledFunc) {
        page.setEnabledFunc(true);
    }
    if (info.setTextFunc) {
        info.setTextFunc(tr("Operation finished"));
    }
    return ErrorCode::NoError;
}

PageProtocolLogicBase *UiLogic::protocolLogic(Proto p)
{
    PageProtocolLogicBase *logic = m_protocolLogicMap.value(p);
    if (logic) return logic;
    else {
        qDebug() << "UiLogic::protocolLogic Warning: logic missing for" << p;
        return new PageProtocolLogicBase(this);
    }
}

QObject *UiLogic::qmlRoot() const
{
    return m_qmlRoot;
}

void UiLogic::setQmlRoot(QObject *newQmlRoot)
{
    m_qmlRoot = newQmlRoot;
}

NotificationHandler *UiLogic::notificationHandler() const
{
    return m_notificationHandler;
}

PageEnumNS::Page UiLogic::currentPage()
{
    return static_cast<PageEnumNS::Page>(currentPageValue());
}

void UiLogic::saveTextFile(const QString& desc, const QString& suggestedName, QString ext, const QString& data)
{
#ifdef Q_OS_IOS
    shareTempFile(suggestedName, ext, data);
    return;
#endif

    ext.replace("*", "");
    QString docDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QUrl fileName;
#ifdef AMNEZIA_DESKTOP
    fileName = QFileDialog::getSaveFileUrl(nullptr, desc,
        QUrl::fromLocalFile(docDir + "/" + suggestedName), "*" + ext);
    if (fileName.isEmpty()) return;
    if (!fileName.toString().endsWith(ext)) fileName = QUrl(fileName.toString() + ext);
#elif defined Q_OS_ANDROID
    fileName = QFileDialog::getSaveFileUrl(nullptr, suggestedName,
        QUrl::fromLocalFile(docDir), "*" + ext);
#endif

    qDebug() << "UiLogic::saveTextFile" << fileName;
    if (fileName.isEmpty()) return;

#ifdef AMNEZIA_DESKTOP
    QFile save(fileName.toLocalFile());
#else
    qDebug() << "UiLogic::saveTextFile" << QQmlFile::urlToLocalFileOrQrc(fileName);
    QFile save(QQmlFile::urlToLocalFileOrQrc(fileName));
#endif

    save.open(QIODevice::WriteOnly);
    save.write(data.toUtf8());
    save.close();

    QFileInfo fi(fileName.toLocalFile());
    QDesktopServices::openUrl(fi.absoluteDir().absolutePath());
}

void UiLogic::saveBinaryFile(const QString &desc, QString ext, const QString &data)
{
    ext.replace("*", "");
    QString fileName = QFileDialog::getSaveFileName(nullptr, desc,
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation), "*" + ext);

    if (fileName.isEmpty()) return;
    if (!fileName.endsWith(ext)) fileName.append(ext);

    QFile save(fileName);
    save.open(QIODevice::WriteOnly);
    save.write(QByteArray::fromBase64(data.toUtf8()));
    save.close();

    QFileInfo fi(fileName);
    QDesktopServices::openUrl(fi.absoluteDir().absolutePath());
}

void UiLogic::copyToClipboard(const QString &text)
{
    qApp->clipboard()->setText(text);
}

void UiLogic::shareTempFile(const QString &suggestedName, QString ext, const QString& data) {
    ext.replace("*", "");
    QString fileName = QDir::tempPath() + "/" + suggestedName;

    if (fileName.isEmpty()) return;
    if (!fileName.endsWith(ext)) fileName.append(ext);

    QFile::remove(fileName);
    qDebug() << "UiLogic::shareTempFile" << fileName;

    QFile save(fileName);
    save.open(QIODevice::WriteOnly);
    save.write(data.toUtf8());
    save.close();

    QStringList filesToSend;
    filesToSend.append(fileName);
    MobileUtils::shareText(filesToSend);
}
