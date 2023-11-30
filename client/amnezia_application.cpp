#include "amnezia_application.h"

#include <QClipboard>
#include <QFontDatabase>
#include <QMimeData>
#include <QQuickStyle>
#include <QResource>
#include <QStandardPaths>
#include <QTextDocument>
#include <QTimer>
#include <QTranslator>

#include <QQuickItem>

#include "logger.h"
#include "version.h"

#include "platforms/ios/QRCodeReaderBase.h"
#if defined(Q_OS_ANDROID)
    #include "platforms/android/android_controller.h"
#endif

#include "protocols/qml_register_protocols.h"

#if defined(Q_OS_IOS)
    #include "platforms/ios/ios_controller.h"
#endif

#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
AmneziaApplication::AmneziaApplication(int &argc, char *argv[]) : AMNEZIA_BASE_CLASS(argc, argv)
#else
AmneziaApplication::AmneziaApplication(int &argc, char *argv[], bool allowSecondary, SingleApplication::Options options,
                                       int timeout, const QString &userData)
    : SingleApplication(argc, argv, allowSecondary, options, timeout, userData)
#endif
{
    setQuitOnLastWindowClosed(false);

    // Fix config file permissions
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    {
        QSettings s(ORGANIZATION_NAME, APPLICATION_NAME);
        s.setValue("permFixed", true);
    }

    QString configLoc1 = QStandardPaths::standardLocations(QStandardPaths::ConfigLocation).first() + "/"
            + ORGANIZATION_NAME + "/" + APPLICATION_NAME + ".conf";
    QFile::setPermissions(configLoc1, QFileDevice::ReadOwner | QFileDevice::WriteOwner);

    QString configLoc2 = QStandardPaths::standardLocations(QStandardPaths::ConfigLocation).first() + "/"
            + ORGANIZATION_NAME + "/" + APPLICATION_NAME + "/" + APPLICATION_NAME + ".conf";
    QFile::setPermissions(configLoc2, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
#endif

    m_settings = std::shared_ptr<Settings>(new Settings);
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

    m_configurator = std::shared_ptr<VpnConfigurator>(new VpnConfigurator(m_settings, this));
    m_vpnConnection.reset(new VpnConnection(m_settings, m_configurator));
    m_vpnConnection->moveToThread(&m_vpnConnectionThread);
    m_vpnConnectionThread.start();

    initModels();
    loadTranslator();
    initControllers();

#ifdef Q_OS_ANDROID
    connect(AndroidController::instance(), &AndroidController::initialized, this,
            [this](bool status, bool connected, const QDateTime &connectionDate) {
                if (connected) {
                    m_connectionController->onConnectionStateChanged(Vpn::ConnectionState::Connected);
                    if (m_vpnConnection)
                        m_vpnConnection->restoreConnection();
                }
            });
    if (!AndroidController::instance()->initialize()) {
        qCritical() << QString("Init failed");
        if (m_vpnConnection)
            emit m_vpnConnection->connectionStateChanged(Vpn::ConnectionState::Error);
        return;
    }

    connect(AndroidController::instance(), &AndroidController::importConfigFromOutside, [this](QString data) {
        m_pageController->replaceStartPage();
        m_importController->extractConfigFromData(data);
        m_pageController->goToPageViewConfig();
    });
#endif

#ifdef Q_OS_IOS
    IosController::Instance()->initialize();
    connect(IosController::Instance(), &IosController::importConfigFromOutside, [this](QString data) {
        m_pageController->replaceStartPage();
        m_importController->extractConfigFromData(data);
        m_pageController->goToPageViewConfig();
    });

    connect(IosController::Instance(), &IosController::importBackupFromOutside, [this](QString filePath) {
        m_pageController->replaceStartPage();
        m_pageController->goToPageSettingsBackup();
        m_settingsController->importBackupFromOutside(filePath);
    });
#endif

    m_notificationHandler.reset(NotificationHandler::create(nullptr));

    connect(m_vpnConnection.get(), &VpnConnection::connectionStateChanged, m_notificationHandler.get(),
            &NotificationHandler::setConnectionState);

    connect(m_notificationHandler.get(), &NotificationHandler::raiseRequested, m_pageController.get(),
            &PageController::raiseMainWindow);
    connect(m_notificationHandler.get(), &NotificationHandler::connectRequested, m_connectionController.get(),
            &ConnectionController::openConnection);
    connect(m_notificationHandler.get(), &NotificationHandler::disconnectRequested, m_connectionController.get(),
            &ConnectionController::closeConnection);
    connect(this, &AmneziaApplication::translationsUpdated, m_notificationHandler.get(),
            &NotificationHandler::onTranslationsUpdated);

    m_engine->load(url);
    m_systemController->setQmlRoot(m_engine->rootObjects().value(0));

    if (m_settings->isSaveLogs()) {
        if (!Logger::init()) {
            qWarning() << "Initialization of debug subsystem failed";
        }
    }

#ifdef Q_OS_WIN
    if (m_parser.isSet("a"))
        m_pageController->showOnStartup();
    else
        emit m_pageController->raiseMainWindow();
#else
    m_pageController->showOnStartup();
#endif

        // TODO - fix
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
    if (isPrimary()) {
        QObject::connect(this, &SingleApplication::instanceStarted, m_pageController.get(), [this]() {
            qDebug() << "Secondary instance started, showing this window instead";
            emit m_pageController->raiseMainWindow();
        });
    }
#endif

// Android TextField clipboard workaround
// https://bugreports.qt.io/browse/QTBUG-113461
#ifdef Q_OS_ANDROID
    QObject::connect(qApp, &QGuiApplication::applicationStateChanged, [](Qt::ApplicationState state) {
        if (state == Qt::ApplicationActive) {
            if (qApp->clipboard()->mimeData()->formats().contains("text/html")) {
                QTextDocument doc;
                doc.setHtml(qApp->clipboard()->mimeData()->html());
                qApp->clipboard()->setText(doc.toPlainText());
            }
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

    //
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

QQmlApplicationEngine *AmneziaApplication::qmlEngine() const
{
    return m_engine;
}

void AmneziaApplication::initModels()
{
    m_containersModel.reset(new ContainersModel(m_settings, this));
    m_engine->rootContext()->setContextProperty("ContainersModel", m_containersModel.get());
    connect(m_configurator.get(), &VpnConfigurator::newVpnConfigCreated, m_containersModel.get(),
            &ContainersModel::updateContainersConfig);

    m_serversModel.reset(new ServersModel(m_settings, this));
    m_engine->rootContext()->setContextProperty("ServersModel", m_serversModel.get());
    connect(m_serversModel.get(), &ServersModel::currentlyProcessedServerIndexChanged, m_containersModel.get(),
            &ContainersModel::setCurrentlyProcessedServerIndex);
    connect(m_serversModel.get(), &ServersModel::defaultServerIndexChanged, m_containersModel.get(),
            &ContainersModel::setCurrentlyProcessedServerIndex);
    connect(m_containersModel.get(), &ContainersModel::containersModelUpdated, m_serversModel.get(),
            &ServersModel::updateContainersConfig);

    m_languageModel.reset(new LanguageModel(m_settings, this));
    m_engine->rootContext()->setContextProperty("LanguageModel", m_languageModel.get());
    connect(m_languageModel.get(), &LanguageModel::updateTranslations, this, &AmneziaApplication::updateTranslator);
    connect(this, &AmneziaApplication::translationsUpdated, m_languageModel.get(), &LanguageModel::translationsUpdated);

    m_sitesModel.reset(new SitesModel(m_settings, this));
    m_engine->rootContext()->setContextProperty("SitesModel", m_sitesModel.get());
    
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

#ifdef Q_OS_WINDOWS
    m_ikev2ConfigModel.reset(new Ikev2ConfigModel(this));
    m_engine->rootContext()->setContextProperty("Ikev2ConfigModel", m_ikev2ConfigModel.get());
#endif

    m_sftpConfigModel.reset(new SftpConfigModel(this));
    m_engine->rootContext()->setContextProperty("SftpConfigModel", m_sftpConfigModel.get());

    m_clientManagementModel.reset(new ClientManagementModel(m_settings, this));
    m_engine->rootContext()->setContextProperty("ClientManagementModel", m_clientManagementModel.get());
    connect(m_configurator.get(), &VpnConfigurator::newVpnConfigCreated, m_clientManagementModel.get(),
            &ClientManagementModel::appendClient);
}

void AmneziaApplication::initControllers()
{
    m_connectionController.reset(new ConnectionController(m_serversModel, m_containersModel, m_vpnConnection));
    m_engine->rootContext()->setContextProperty("ConnectionController", m_connectionController.get());

    connect(this, &AmneziaApplication::translationsUpdated, m_connectionController.get(),
            &ConnectionController::onTranslationsUpdated);

    m_pageController.reset(new PageController(m_serversModel, m_settings));
    m_engine->rootContext()->setContextProperty("PageController", m_pageController.get());

    m_installController.reset(new InstallController(m_serversModel, m_containersModel, m_protocolsModel, m_settings));
    m_engine->rootContext()->setContextProperty("InstallController", m_installController.get());
    connect(m_installController.get(), &InstallController::passphraseRequestStarted, m_pageController.get(),
            &PageController::showPassphraseRequestDrawer);
    connect(m_pageController.get(), &PageController::passphraseRequestDrawerClosed, m_installController.get(),
            &InstallController::setEncryptedPassphrase);
    connect(m_installController.get(), &InstallController::currentContainerUpdated, m_connectionController.get(),
            &ConnectionController::onCurrentContainerUpdated);

    m_importController.reset(new ImportController(m_serversModel, m_containersModel, m_settings));
    m_engine->rootContext()->setContextProperty("ImportController", m_importController.get());

    m_exportController.reset(new ExportController(m_serversModel, m_containersModel, m_clientManagementModel, m_settings, m_configurator));
    m_engine->rootContext()->setContextProperty("ExportController", m_exportController.get());

    m_settingsController.reset(new SettingsController(m_serversModel, m_containersModel, m_languageModel, m_settings));
    m_engine->rootContext()->setContextProperty("SettingsController", m_settingsController.get());
    if (m_settingsController->isAutoConnectEnabled() && m_serversModel->getDefaultServerIndex() >= 0) {
        QTimer::singleShot(1000, this, [this]() { m_connectionController->openConnection(); });
    }

    m_sitesController.reset(new SitesController(m_settings, m_vpnConnection, m_sitesModel));
    m_engine->rootContext()->setContextProperty("SitesController", m_sitesController.get());

    m_systemController.reset(new SystemController(m_settings));
    m_engine->rootContext()->setContextProperty("SystemController", m_systemController.get());
}
