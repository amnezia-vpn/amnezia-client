#include "amnezia_application.h"

#include <QClipboard>
#include <QFontDatabase>
#include <QMimeData>
#include <QQuickItem>
#include <QQuickStyle>
#include <QResource>
#include <QStandardPaths>
#include <QTextDocument>
#include <QTimer>
#include <QTranslator>
#include <QLocalSocket>
#include <QLocalServer>

#include "logger.h"
#include "ui/models/installedAppsModel.h"
#include "version.h"

#include "platforms/ios/QRCodeReaderBase.h"
#if defined(Q_OS_ANDROID)
    #include "core/installedAppsImageProvider.h"
    #include "platforms/android/android_controller.h"
#endif

#include "protocols/qml_register_protocols.h"

#if defined(Q_OS_IOS)
    #include "platforms/ios/ios_controller.h"
    #include <AmneziaVPN-Swift.h>
#endif

AmneziaApplication::AmneziaApplication(int &argc, char *argv[]) : AMNEZIA_BASE_CLASS(argc, argv)
{
    setQuitOnLastWindowClosed(false);

    // Fix config file permissions
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    {
        QSettings s(ORGANIZATION_NAME, APPLICATION_NAME);
        s.setValue("permFixed", true);
    }

    QString configLoc1 = QStandardPaths::standardLocations(QStandardPaths::ConfigLocation).first() + "/" + ORGANIZATION_NAME + "/"
            + APPLICATION_NAME + ".conf";
    QFile::setPermissions(configLoc1, QFileDevice::ReadOwner | QFileDevice::WriteOwner);

    QString configLoc2 = QStandardPaths::standardLocations(QStandardPaths::ConfigLocation).first() + "/" + ORGANIZATION_NAME + "/"
            + APPLICATION_NAME + "/" + APPLICATION_NAME + ".conf";
    QFile::setPermissions(configLoc2, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
#endif

    m_settings = std::shared_ptr<Settings>(new Settings);
    m_nam = new QNetworkAccessManager(this);
}

AmneziaApplication::~AmneziaApplication()
{
    m_vpnConnectionThread.quit();
    m_vpnConnectionThread.wait(3000);

    if (m_engine) {
        QObject::disconnect(m_engine, 0, 0, 0);
        delete m_engine;
    }
}

void AmneziaApplication::init()
{
    m_engine = new QQmlApplicationEngine;

    const QUrl url(QStringLiteral("qrc:/ui/qml/main2.qml"));
    QObject::connect(
            m_engine, &QQmlApplicationEngine::objectCreated, this,
            [url](QObject *obj, const QUrl &objUrl) {
                if (!obj && url == objUrl)
                    QCoreApplication::exit(-1);
            },
            Qt::QueuedConnection);

    m_engine->rootContext()->setContextProperty("Debug", &Logger::Instance());

    m_vpnConnection.reset(new VpnConnection(m_settings));
    m_vpnConnection->moveToThread(&m_vpnConnectionThread);
    m_vpnConnectionThread.start();

    initModels();
    loadTranslator();
    initControllers();

#ifdef Q_OS_ANDROID
    if (!AndroidController::initLogging()) {
        qFatal("Android logging initialization failed");
    }
    AndroidController::instance()->setSaveLogs(m_settings->isSaveLogs());
    connect(m_settings.get(), &Settings::saveLogsChanged, AndroidController::instance(), &AndroidController::setSaveLogs);

    AndroidController::instance()->setScreenshotsEnabled(m_settings->isScreenshotsEnabled());
    connect(m_settings.get(), &Settings::screenshotsEnabledChanged, AndroidController::instance(), &AndroidController::setScreenshotsEnabled);

    connect(m_settings.get(), &Settings::serverRemoved, AndroidController::instance(), &AndroidController::resetLastServer);

    connect(m_settings.get(), &Settings::settingsCleared, []() { AndroidController::instance()->resetLastServer(-1); });

    connect(AndroidController::instance(), &AndroidController::initConnectionState, this, [this](Vpn::ConnectionState state) {
        m_connectionController->onConnectionStateChanged(state);
        if (m_vpnConnection)
            m_vpnConnection->restoreConnection();
    });
    if (!AndroidController::instance()->initialize()) {
        qFatal("Android controller initialization failed");
    }

    connect(AndroidController::instance(), &AndroidController::importConfigFromOutside, this, [this](QString data) {
        emit m_pageController->goToPageHome();
        m_importController->extractConfigFromData(data);
        data.clear();
        emit m_pageController->goToPageViewConfig();
    });

    m_engine->addImageProvider(QLatin1String("installedAppImage"), new InstalledAppsImageProvider);
#endif

#ifdef Q_OS_IOS
    IosController::Instance()->initialize();
    connect(IosController::Instance(), &IosController::importConfigFromOutside, this, [this](QString data) {
        emit m_pageController->goToPageHome();
        m_importController->extractConfigFromData(data);
        emit m_pageController->goToPageViewConfig();
    });

    connect(IosController::Instance(), &IosController::importBackupFromOutside, this, [this](QString filePath) {
        emit m_pageController->goToPageHome();
        m_pageController->goToPageSettingsBackup();
        emit m_settingsController->importBackupFromOutside(filePath);
    });

    QTimer::singleShot(0, this, [this]() { AmneziaVPN::toggleScreenshots(m_settings->isScreenshotsEnabled()); });

    connect(m_settings.get(), &Settings::screenshotsEnabledChanged, [](bool enabled) { AmneziaVPN::toggleScreenshots(enabled); });
#endif

#ifndef Q_OS_ANDROID
    m_notificationHandler.reset(NotificationHandler::create(nullptr));

    connect(m_vpnConnection.get(), &VpnConnection::connectionStateChanged, m_notificationHandler.get(),
            &NotificationHandler::setConnectionState);

    connect(m_notificationHandler.get(), &NotificationHandler::raiseRequested, m_pageController.get(), &PageController::raiseMainWindow);
    connect(m_notificationHandler.get(), &NotificationHandler::connectRequested, m_connectionController.get(),
            static_cast<void (ConnectionController::*)()>(&ConnectionController::openConnection));
    connect(m_notificationHandler.get(), &NotificationHandler::disconnectRequested, m_connectionController.get(),
            &ConnectionController::closeConnection);
    connect(this, &AmneziaApplication::translationsUpdated, m_notificationHandler.get(), &NotificationHandler::onTranslationsUpdated);
#endif

    m_engine->addImportPath("qrc:/ui/qml/Modules/");
    m_engine->load(url);
    m_systemController->setQmlRoot(m_engine->rootObjects().value(0));

    bool enabled = m_settings->isSaveLogs();
#ifndef Q_OS_ANDROID
    if (enabled) {
        if (!Logger::init(false)) {
            qWarning() << "Initialization of debug subsystem failed";
        }
    }
#endif
    Logger::setServiceLogsEnabled(enabled);

#ifdef Q_OS_WIN
    if (m_parser.isSet("a"))
        m_pageController->showOnStartup();
    else
        emit m_pageController->raiseMainWindow();
#else
    m_pageController->showOnStartup();
#endif

// Android TextArea clipboard workaround
// Text from TextArea always has "text/html" mime-type:
// /qt/6.6.1/Src/qtdeclarative/src/quick/items/qquicktextcontrol.cpp:1865
// Next, html is created for this mime-type:
// /qt/6.6.1/Src/qtdeclarative/src/quick/items/qquicktextcontrol.cpp:1885
// And this html goes to the Androids clipboard, i.e. text from TextArea is always copied as richText:
// /qt/6.6.1/Src/qtbase/src/plugins/platforms/android/androidjniclipboard.cpp:46
// So we catch all the copies to the clipboard and clear them from "text/html"
#ifdef Q_OS_ANDROID
    connect(QGuiApplication::clipboard(), &QClipboard::dataChanged, []() {
        auto clipboard = QGuiApplication::clipboard();
        if (clipboard->mimeData()->hasHtml()) {
            clipboard->setText(clipboard->text());
        }
    });
#endif
}

void AmneziaApplication::registerTypes()
{
    qRegisterMetaType<ServerCredentials>("ServerCredentials");

    qRegisterMetaType<DockerContainer>("DockerContainer");
    qRegisterMetaType<TransportProto>("TransportProto");
    qRegisterMetaType<Proto>("Proto");
    qRegisterMetaType<ServiceType>("ServiceType");

    declareQmlProtocolEnum();
    declareQmlContainerEnum();

    qmlRegisterType<QRCodeReader>("QRCodeReader", 1, 0, "QRCodeReader");

    m_containerProps.reset(new ContainerProps());
    qmlRegisterSingletonInstance("ContainerProps", 1, 0, "ContainerProps", m_containerProps.get());

    m_protocolProps.reset(new ProtocolProps());
    qmlRegisterSingletonInstance("ProtocolProps", 1, 0, "ProtocolProps", m_protocolProps.get());

    qmlRegisterSingletonType(QUrl("qrc:/ui/qml/Filters/ContainersModelFilters.qml"), "ContainersModelFilters", 1, 0,
                             "ContainersModelFilters");

    qmlRegisterType<InstalledAppsModel>("InstalledAppsModel", 1, 0, "InstalledAppsModel");

    Vpn::declareQmlVpnConnectionStateEnum();
    PageLoader::declareQmlPageEnum();
}

void AmneziaApplication::loadFonts()
{
    QQuickStyle::setStyle("Basic");

    QFontDatabase::addApplicationFont(":/fonts/pt-root-ui_vf.ttf");
}

void AmneziaApplication::loadTranslator()
{
    auto locale = m_settings->getAppLanguage();
    m_translator.reset(new QTranslator());
    updateTranslator(locale);
}

void AmneziaApplication::updateTranslator(const QLocale &locale)
{
    if (!m_translator->isEmpty()) {
        QCoreApplication::removeTranslator(m_translator.get());
    }

    QString strFileName = QString(":/translations/amneziavpn") + QLatin1String("_") + locale.name() + ".qm";
    if (m_translator->load(strFileName)) {
        if (QCoreApplication::installTranslator(m_translator.get())) {
            m_settings->setAppLanguage(locale);
        }
    } else {
        m_settings->setAppLanguage(QLocale::English);
    }

    m_engine->retranslate();

    emit translationsUpdated();
}

bool AmneziaApplication::parseCommands()
{
    m_parser.setApplicationDescription(APPLICATION_NAME);
    m_parser.addHelpOption();
    m_parser.addVersionOption();

    QCommandLineOption c_autostart { { "a", "autostart" }, "System autostart" };
    m_parser.addOption(c_autostart);

    QCommandLineOption c_cleanup { { "c", "cleanup" }, "Cleanup logs" };
    m_parser.addOption(c_cleanup);

    m_parser.process(*this);

    if (m_parser.isSet(c_cleanup)) {
        Logger::cleanUp();
        QTimer::singleShot(100, this, [this] { quit(); });
        exec();
        return false;
    }
    return true;
}

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
void AmneziaApplication::startLocalServer() {
    const QString serverName("AmneziaVPNInstance");
    QLocalServer::removeServer(serverName);

    QLocalServer* server = new QLocalServer(this);
    server->listen(serverName);

    QObject::connect(server, &QLocalServer::newConnection, this, [server, this]() {
        if (server) {
            QLocalSocket* clientConnection = server->nextPendingConnection();
            clientConnection->deleteLater();
        }
        emit m_pageController->raiseMainWindow();
    });
}
#endif

QQmlApplicationEngine *AmneziaApplication::qmlEngine() const
{
    return m_engine;
}

void AmneziaApplication::initModels()
{
    m_containersModel.reset(new ContainersModel(this));
    m_engine->rootContext()->setContextProperty("ContainersModel", m_containersModel.get());

    m_defaultServerContainersModel.reset(new ContainersModel(this));
    m_engine->rootContext()->setContextProperty("DefaultServerContainersModel", m_defaultServerContainersModel.get());

    m_serversModel.reset(new ServersModel(m_settings, this));
    m_engine->rootContext()->setContextProperty("ServersModel", m_serversModel.get());
    connect(m_serversModel.get(), &ServersModel::containersUpdated, m_containersModel.get(), &ContainersModel::updateModel);
    connect(m_serversModel.get(), &ServersModel::defaultServerContainersUpdated, m_defaultServerContainersModel.get(),
            &ContainersModel::updateModel);
    m_serversModel->resetModel();

    m_languageModel.reset(new LanguageModel(m_settings, this));
    m_engine->rootContext()->setContextProperty("LanguageModel", m_languageModel.get());
    connect(m_languageModel.get(), &LanguageModel::updateTranslations, this, &AmneziaApplication::updateTranslator);
    connect(this, &AmneziaApplication::translationsUpdated, m_languageModel.get(), &LanguageModel::translationsUpdated);

    m_sitesModel.reset(new SitesModel(m_settings, this));
    m_engine->rootContext()->setContextProperty("SitesModel", m_sitesModel.get());

    m_appSplitTunnelingModel.reset(new AppSplitTunnelingModel(m_settings, this));
    m_engine->rootContext()->setContextProperty("AppSplitTunnelingModel", m_appSplitTunnelingModel.get());

    m_protocolsModel.reset(new ProtocolsModel(m_settings, this));
    m_engine->rootContext()->setContextProperty("ProtocolsModel", m_protocolsModel.get());

    m_openVpnConfigModel.reset(new OpenVpnConfigModel(this));
    m_engine->rootContext()->setContextProperty("OpenVpnConfigModel", m_openVpnConfigModel.get());

    m_shadowSocksConfigModel.reset(new ShadowSocksConfigModel(this));
    m_engine->rootContext()->setContextProperty("ShadowSocksConfigModel", m_shadowSocksConfigModel.get());

    m_cloakConfigModel.reset(new CloakConfigModel(this));
    m_engine->rootContext()->setContextProperty("CloakConfigModel", m_cloakConfigModel.get());

    m_wireGuardConfigModel.reset(new WireGuardConfigModel(this));
    m_engine->rootContext()->setContextProperty("WireGuardConfigModel", m_wireGuardConfigModel.get());

    m_awgConfigModel.reset(new AwgConfigModel(this));
    m_engine->rootContext()->setContextProperty("AwgConfigModel", m_awgConfigModel.get());

    m_xrayConfigModel.reset(new XrayConfigModel(this));
    m_engine->rootContext()->setContextProperty("XrayConfigModel", m_xrayConfigModel.get());

#ifdef Q_OS_WINDOWS
    m_ikev2ConfigModel.reset(new Ikev2ConfigModel(this));
    m_engine->rootContext()->setContextProperty("Ikev2ConfigModel", m_ikev2ConfigModel.get());
#endif

    m_sftpConfigModel.reset(new SftpConfigModel(this));
    m_engine->rootContext()->setContextProperty("SftpConfigModel", m_sftpConfigModel.get());

    m_socks5ConfigModel.reset(new Socks5ProxyConfigModel(this));
    m_engine->rootContext()->setContextProperty("Socks5ProxyConfigModel", m_socks5ConfigModel.get());

    m_clientManagementModel.reset(new ClientManagementModel(m_settings, this));
    m_engine->rootContext()->setContextProperty("ClientManagementModel", m_clientManagementModel.get());
    connect(m_clientManagementModel.get(), &ClientManagementModel::adminConfigRevoked, m_serversModel.get(),
            &ServersModel::clearCachedProfile);

    m_apiServicesModel.reset(new ApiServicesModel(this));
    m_engine->rootContext()->setContextProperty("ApiServicesModel", m_apiServicesModel.get());

    m_apiCountryModel.reset(new ApiCountryModel(this));
    m_engine->rootContext()->setContextProperty("ApiCountryModel", m_apiCountryModel.get());
    connect(m_serversModel.get(), &ServersModel::updateApiLanguageModel, this, [this]() {
        m_apiCountryModel->updateModel(m_serversModel->getProcessedServerData("apiAvailableCountries").toJsonArray(),
                                       m_serversModel->getProcessedServerData("apiServerCountryCode").toString());
    });
    connect(m_serversModel.get(), &ServersModel::updateApiServicesModel, this,
            [this]() { m_apiServicesModel->updateModel(m_serversModel->getProcessedServerData("apiConfig").toJsonObject()); });
}

void AmneziaApplication::initControllers()
{
    m_connectionController.reset(
            new ConnectionController(m_serversModel, m_containersModel, m_clientManagementModel, m_vpnConnection, m_settings));
    m_engine->rootContext()->setContextProperty("ConnectionController", m_connectionController.get());

    connect(m_connectionController.get(), qOverload<const QString &>(&ConnectionController::connectionErrorOccurred), this,
            [this](const QString &errorMessage) {
                emit m_pageController->showErrorMessage(errorMessage);
                emit m_vpnConnection->connectionStateChanged(Vpn::ConnectionState::Disconnected);
            });

    connect(m_connectionController.get(), qOverload<ErrorCode>(&ConnectionController::connectionErrorOccurred), this,
            [this](ErrorCode errorCode) {
                emit m_pageController->showErrorMessage(errorCode);
                emit m_vpnConnection->connectionStateChanged(Vpn::ConnectionState::Disconnected);
            });

    connect(m_connectionController.get(), &ConnectionController::connectButtonClicked, m_connectionController.get(),
            &ConnectionController::toggleConnection, Qt::QueuedConnection);

    m_pageController.reset(new PageController(m_serversModel, m_settings));
    m_engine->rootContext()->setContextProperty("PageController", m_pageController.get());

    m_installController.reset(new InstallController(m_serversModel, m_containersModel, m_protocolsModel, m_clientManagementModel,
                                                    m_apiServicesModel, m_settings));
    m_engine->rootContext()->setContextProperty("InstallController", m_installController.get());
    connect(m_installController.get(), &InstallController::passphraseRequestStarted, m_pageController.get(),
            &PageController::showPassphraseRequestDrawer);
    connect(m_pageController.get(), &PageController::passphraseRequestDrawerClosed, m_installController.get(),
            &InstallController::setEncryptedPassphrase);
    connect(m_installController.get(), &InstallController::currentContainerUpdated, m_connectionController.get(),
            &ConnectionController::onCurrentContainerUpdated);

    connect(m_installController.get(), &InstallController::updateServerFromApiFinished, this, [this]() {
        disconnect(m_reloadConfigErrorOccurredConnection);
        emit m_connectionController->configFromApiUpdated();
    });

    connect(m_connectionController.get(), &ConnectionController::updateApiConfigFromGateway, this, [this]() {
        m_reloadConfigErrorOccurredConnection = connect(
                m_installController.get(), qOverload<ErrorCode>(&InstallController::installationErrorOccurred), this,
                [this]() { emit m_vpnConnection->connectionStateChanged(Vpn::ConnectionState::Disconnected); },
                static_cast<Qt::ConnectionType>(Qt::AutoConnection || Qt::SingleShotConnection));
        m_installController->updateServiceFromApi(m_serversModel->getDefaultServerIndex(), "", "");
    });

    connect(m_connectionController.get(), &ConnectionController::updateApiConfigFromTelegram, this, [this]() {
        m_reloadConfigErrorOccurredConnection = connect(
                m_installController.get(), qOverload<ErrorCode>(&InstallController::installationErrorOccurred), this,
                [this]() { emit m_vpnConnection->connectionStateChanged(Vpn::ConnectionState::Disconnected); },
                static_cast<Qt::ConnectionType>(Qt::AutoConnection || Qt::SingleShotConnection));
        m_serversModel->removeApiConfig(m_serversModel->getDefaultServerIndex());
        m_installController->updateServiceFromTelegram(m_serversModel->getDefaultServerIndex());
    });

    connect(this, &AmneziaApplication::translationsUpdated, m_connectionController.get(), &ConnectionController::onTranslationsUpdated);

    m_importController.reset(new ImportController(m_serversModel, m_containersModel, m_settings));
    m_engine->rootContext()->setContextProperty("ImportController", m_importController.get());

    m_exportController.reset(new ExportController(m_serversModel, m_containersModel, m_clientManagementModel, m_settings));
    m_engine->rootContext()->setContextProperty("ExportController", m_exportController.get());

    m_settingsController.reset(
            new SettingsController(m_serversModel, m_containersModel, m_languageModel, m_sitesModel, m_appSplitTunnelingModel, m_settings));
    m_engine->rootContext()->setContextProperty("SettingsController", m_settingsController.get());
    if (m_settingsController->isAutoConnectEnabled() && m_serversModel->getDefaultServerIndex() >= 0) {
        QTimer::singleShot(1000, this, [this]() { m_connectionController->openConnection(); });
    }
    connect(m_settingsController.get(), &SettingsController::amneziaDnsToggled, m_serversModel.get(), &ServersModel::toggleAmneziaDns);

    m_sitesController.reset(new SitesController(m_settings, m_vpnConnection, m_sitesModel));
    m_engine->rootContext()->setContextProperty("SitesController", m_sitesController.get());

    m_appSplitTunnelingController.reset(new AppSplitTunnelingController(m_settings, m_appSplitTunnelingModel));
    m_engine->rootContext()->setContextProperty("AppSplitTunnelingController", m_appSplitTunnelingController.get());

    m_systemController.reset(new SystemController(m_settings));
    m_engine->rootContext()->setContextProperty("SystemController", m_systemController.get());
}
