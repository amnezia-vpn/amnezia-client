#include "amnezia_application.h"

#include <QFontDatabase>
#include <QStandardPaths>
#include <QTimer>
#include <QTranslator>
#include <QQuickStyle>

#include "logger.h"
#include "defines.h"

#include "platforms/ios/QRCodeReaderBase.h"

#include "ui/pages.h"

#if defined(Q_OS_IOS)
#include "platforms/ios/QtAppDelegate-C-Interface.h"
#endif

#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    AmneziaApplication::AmneziaApplication(int &argc, char *argv[]):
        AMNEZIA_BASE_CLASS(argc, argv)
#else
    AmneziaApplication::AmneziaApplication(int &argc, char *argv[], bool allowSecondary,
        SingleApplication::Options options, int timeout, const QString &userData):
    SingleApplication(argc, argv, allowSecondary, options, timeout, userData)
#endif
{
    setQuitOnLastWindowClosed(false);

    // Fix config file permissions
#ifdef Q_OS_LINUX
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
    if (m_engine) {
        QObject::disconnect(m_engine, 0,0,0);
        delete m_engine;
    }

    if (m_protocolProps) delete m_protocolProps;
    if (m_containerProps) delete m_containerProps;
}

void AmneziaApplication::init()
{
    m_engine = new QQmlApplicationEngine;

    const QUrl url(QStringLiteral("qrc:/ui/qml/main2.qml"));
    QObject::connect(m_engine, &QQmlApplicationEngine::objectCreated,
                     this, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);

    m_engine->rootContext()->setContextProperty("Debug", &Logger::Instance());

    //
    m_containersModel.reset(new ContainersModel(m_settings, this));
    m_engine->rootContext()->setContextProperty("ContainersModel", m_containersModel.get());

    m_serversModel.reset(new ServersModel(m_settings, this));
    m_engine->rootContext()->setContextProperty("ServersModel", m_serversModel.get());

    m_configurator = std::shared_ptr<VpnConfigurator>(new VpnConfigurator(m_settings, this));
    m_vpnConnection.reset(new VpnConnection(m_settings, m_configurator));

    m_connectionController.reset(new ConnectionController(m_serversModel, m_containersModel, m_vpnConnection));
    m_engine->rootContext()->setContextProperty("ConnectionController", m_connectionController.get());

    m_pageController.reset(new PageController(m_serversModel));
    m_engine->rootContext()->setContextProperty("PageController", m_pageController.get());

    m_installController.reset(new InstallController(m_serversModel, m_containersModel, m_settings));
    m_engine->rootContext()->setContextProperty("InstallController", m_installController.get());

    m_importController.reset(new ImportController(m_serversModel, m_containersModel, m_settings));
    m_engine->rootContext()->setContextProperty("ImportController", m_importController.get());

    m_exportController.reset(
        new ExportController(m_serversModel, m_containersModel, m_settings, m_configurator));
    m_engine->rootContext()->setContextProperty("ExportController", m_exportController.get());

    m_settingsController.reset(
        new SettingsController(m_serversModel, m_containersModel, m_settings));
    m_engine->rootContext()->setContextProperty("SettingsController", m_settingsController.get());

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

//#ifdef Q_OS_WIN
//    if (m_parser.isSet("a")) m_uiLogic->showOnStartup();
//    else emit m_uiLogic->show();
//#else
//    m_uiLogic->showOnStartup();
//#endif

//    // TODO - fix
//#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
//    if (isPrimary()) {
//        QObject::connect(this, &SingleApplication::instanceStarted, m_uiLogic, [this](){
//            qDebug() << "Secondary instance started, showing this window instead";
//            emit m_uiLogic->show();
//            emit m_uiLogic->raise();
//        });
//    }
//#endif

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

    m_containerProps = new ContainerProps;
    qmlRegisterSingletonInstance("ContainerProps", 1, 0, "ContainerProps", m_containerProps);

    m_protocolProps = new ProtocolProps;
    qmlRegisterSingletonInstance("ProtocolProps", 1, 0, "ProtocolProps", m_protocolProps);

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
    m_translator = new QTranslator;
    if (m_translator->load(QLocale(), QString("amneziavpn"), QLatin1String("_"), QLatin1String(":/translations"))) {
        installTranslator(m_translator);
    }
}

bool AmneziaApplication::parseCommands()
{
    m_parser.setApplicationDescription(APPLICATION_NAME);
    m_parser.addHelpOption();
    m_parser.addVersionOption();

    QCommandLineOption c_autostart {{"a", "autostart"}, "System autostart"};
    m_parser.addOption(c_autostart);

    QCommandLineOption c_cleanup {{"c", "cleanup"}, "Cleanup logs"};
    m_parser.addOption(c_cleanup);

    m_parser.process(*this);

    if (m_parser.isSet(c_cleanup)) {
        Logger::cleanUp();
        QTimer::singleShot(100, this, [this]{
            quit();
        });
        exec();
        return false;
    }
    return true;
}

QQmlApplicationEngine *AmneziaApplication::qmlEngine() const
{
    return m_engine;
}

