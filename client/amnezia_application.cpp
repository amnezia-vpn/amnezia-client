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

#include "core/servercontroller.h"
#include "logger.h"
#include "version.h"

#include "platforms/ios/QRCodeReaderBase.h"
#if defined(Q_OS_ANDROID)
    #include "platforms/android/android_controller.h"
#endif

#include "protocols/qml_register_protocols.h"
#include "ui/pages.h"

#if defined(Q_OS_IOS)
    #include "platforms/ios/QtAppDelegate-C-Interface.h"
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
#if defined Q_OS_LINUX && !defined(Q_OS_ANDROID)
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

    //

    m_configurator = std::shared_ptr<VpnConfigurator>(new VpnConfigurator(m_settings, this));
    m_vpnConnection.reset(new VpnConnection(m_settings, m_configurator));
    m_vpnConnection->moveToThread(&m_vpnConnectionThread);
    m_vpnConnectionThread.start();

    initModels();
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

    connect(AndroidController::instance(), &AndroidController::importConfigFromOutside, m_importController.get(),
            &ImportController::extractConfigFromData);
    connect(AndroidController::instance(), &AndroidController::importConfigFromOutside, m_pageController.get(),
            &PageController::goToPageViewConfig);
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

    //

    m_engine->load(url);

    //    if (m_engine->rootObjects().size() > 0) {
    //        m_uiLogic->setQmlRoot(m_engine->rootObjects().at(0));
    //    }

    if (m_settings->isSaveLogs()) {
        if (!Logger::init()) {
            qWarning() << "Initialization of debug subsystem failed";
        }
    }

    // #ifdef Q_OS_WIN
    //     if (m_parser.isSet("a")) m_uiLogic->showOnStartup();
    //     else emit m_uiLogic->show();
    // #else
    //     m_uiLogic->showOnStartup();
    // #endif

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
    QObject::connect(qApp, &QApplication::applicationStateChanged, [](Qt::ApplicationState state) {
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
    if (locale != QLocale::English) {
        m_translator.reset(new QTranslator());
        if (m_translator->load(locale, QString("amneziavpn"), QLatin1String("_"), QLatin1String(":/i18n"))) {
            if (QCoreApplication::installTranslator(m_translator.get())) {
                m_settings->setAppLanguage(locale);
            }
        }
    }
}

void AmneziaApplication::updateTranslator(const QLocale &locale)
{
    QResource::registerResource(":/translations.qrc");
    if (!m_translator->isEmpty())
        QCoreApplication::removeTranslator(m_translator.get());

    if (locale == QLocale::English) {
        m_settings->setAppLanguage(locale);
        m_engine->retranslate();
    }

    if (m_translator->load(locale, QString("amneziavpn"), QLatin1String("_"), QLatin1String(":/i18n"))) {
        if (QCoreApplication::installTranslator(m_translator.get())) {
            m_settings->setAppLanguage(locale);
        }

        m_engine->retranslate();
    }

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

    m_serversModel.reset(new ServersModel(m_settings, this));
    m_engine->rootContext()->setContextProperty("ServersModel", m_serversModel.get());
    connect(m_serversModel.get(), &ServersModel::currentlyProcessedServerIndexChanged, m_containersModel.get(),
            &ContainersModel::setCurrentlyProcessedServerIndex);

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

    m_wireguardConfigModel.reset(new WireGuardConfigModel(this));
    m_engine->rootContext()->setContextProperty("WireGuardConfigModel", m_wireguardConfigModel.get());

#ifdef Q_OS_WINDOWS
    m_ikev2ConfigModel.reset(new Ikev2ConfigModel(this));
    m_engine->rootContext()->setContextProperty("Ikev2ConfigModel", m_ikev2ConfigModel.get());
#endif

    m_sftpConfigModel.reset(new SftpConfigModel(this));
    m_engine->rootContext()->setContextProperty("SftpConfigModel", m_sftpConfigModel.get());
}

void AmneziaApplication::initControllers()
{
    m_connectionController.reset(new ConnectionController(m_serversModel, m_containersModel, m_vpnConnection));
    m_engine->rootContext()->setContextProperty("ConnectionController", m_connectionController.get());

    m_pageController.reset(new PageController(m_serversModel));
    m_engine->rootContext()->setContextProperty("PageController", m_pageController.get());

    m_installController.reset(new InstallController(m_serversModel, m_containersModel, m_settings));
    m_engine->rootContext()->setContextProperty("InstallController", m_installController.get());
    connect(m_installController.get(), &InstallController::passphraseRequestStarted, m_pageController.get(),
            &PageController::showPassphraseRequestDrawer);
    connect(m_pageController.get(), &PageController::passphraseRequestDrawerClosed, m_installController.get(),
            &InstallController::setEncryptedPassphrase);

    m_importController.reset(new ImportController(m_serversModel, m_containersModel, m_settings));
    m_engine->rootContext()->setContextProperty("ImportController", m_importController.get());

    m_exportController.reset(new ExportController(m_serversModel, m_containersModel, m_settings, m_configurator));
    m_engine->rootContext()->setContextProperty("ExportController", m_exportController.get());

    m_settingsController.reset(new SettingsController(m_serversModel, m_containersModel, m_languageModel, m_settings));
    m_engine->rootContext()->setContextProperty("SettingsController", m_settingsController.get());

    m_sitesController.reset(new SitesController(m_settings, m_vpnConnection, m_sitesModel));
    m_engine->rootContext()->setContextProperty("SitesController", m_sitesController.get());
}
