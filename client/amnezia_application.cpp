#include "amnezia_application.h"

#include <QFontDatabase>
#include <QTimer>
#include <QTranslator>


#include "QZXing.h"

#include "debug.h"
#include "defines.h"


#include "platforms/ios/QRCodeReaderBase.h"
//#include "platforms/ios/MobileUtils.h"

#include "ui/pages.h"

#include "ui/pages_logic/AppSettingsLogic.h"
#include "ui/pages_logic/GeneralSettingsLogic.h"
#include "ui/pages_logic/NetworkSettingsLogic.h"
#include "ui/pages_logic/NewServerProtocolsLogic.h"
#include "ui/pages_logic/QrDecoderLogic.h"
#include "ui/pages_logic/ServerConfiguringProgressLogic.h"
#include "ui/pages_logic/ServerContainersLogic.h"
#include "ui/pages_logic/ServerListLogic.h"
#include "ui/pages_logic/ServerSettingsLogic.h"
#include "ui/pages_logic/ServerContainersLogic.h"
#include "ui/pages_logic/ShareConnectionLogic.h"
#include "ui/pages_logic/SitesLogic.h"
#include "ui/pages_logic/StartPageLogic.h"
#include "ui/pages_logic/VpnLogic.h"
#include "ui/pages_logic/WizardLogic.h"

#include "ui/pages_logic/protocols/CloakLogic.h"
#include "ui/pages_logic/protocols/OpenVpnLogic.h"
#include "ui/pages_logic/protocols/ShadowSocksLogic.h"


AmneziaApplication::AmneziaApplication(int &argc, char *argv[], bool allowSecondary,
  SingleApplication::Options options, int timeout, const QString &userData):
    #if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
        QAPPLICATION_CLASS(argc, argv);
    #else
        SingleApplication(argc, argv, allowSecondary, options, timeout, userData)
    #endif


{
    setQuitOnLastWindowClosed(false);
    m_settings = std::shared_ptr<Settings>(new Settings);

//    QObject::connect(&app, &QCoreApplication::aboutToQuit, uiLogic, [&engine, uiLogic](){
//        QObject::disconnect(engine, 0,0,0);
//        delete engine;

//        QObject::disconnect(uiLogic, 0,0,0);
//        delete uiLogic;
    //    });
}

AmneziaApplication::~AmneziaApplication()
{
    QObject::disconnect(m_engine, 0,0,0);
    delete m_engine;

    QObject::disconnect(m_uiLogic, 0,0,0);
    delete m_uiLogic;
}

void AmneziaApplication::init()
{
    m_engine = new QQmlApplicationEngine;
    m_uiLogic = new UiLogic(m_settings);

    const QUrl url(QStringLiteral("qrc:/ui/qml/main.qml"));
    QObject::connect(m_engine, &QQmlApplicationEngine::objectCreated,
                     this, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);

    m_engine->rootContext()->setContextProperty("Debug", &Debug::Instance());

    m_engine->rootContext()->setContextProperty("UiLogic", m_uiLogic);

    m_engine->rootContext()->setContextProperty("AppSettingsLogic", m_uiLogic->appSettingsLogic());
    m_engine->rootContext()->setContextProperty("GeneralSettingsLogic", m_uiLogic->generalSettingsLogic());
    m_engine->rootContext()->setContextProperty("NetworkSettingsLogic", m_uiLogic->networkSettingsLogic());
    m_engine->rootContext()->setContextProperty("NewServerProtocolsLogic", m_uiLogic->newServerProtocolsLogic());
    m_engine->rootContext()->setContextProperty("QrDecoderLogic", m_uiLogic->qrDecoderLogic());
    m_engine->rootContext()->setContextProperty("ServerConfiguringProgressLogic", m_uiLogic->serverConfiguringProgressLogic());
    m_engine->rootContext()->setContextProperty("ServerListLogic", m_uiLogic->serverListLogic());
    m_engine->rootContext()->setContextProperty("ServerSettingsLogic", m_uiLogic->serverSettingsLogic());
    m_engine->rootContext()->setContextProperty("ServerContainersLogic", m_uiLogic->serverprotocolsLogic());
    m_engine->rootContext()->setContextProperty("ShareConnectionLogic", m_uiLogic->shareConnectionLogic());
    m_engine->rootContext()->setContextProperty("SitesLogic", m_uiLogic->sitesLogic());
    m_engine->rootContext()->setContextProperty("StartPageLogic", m_uiLogic->startPageLogic());
    m_engine->rootContext()->setContextProperty("VpnLogic", m_uiLogic->vpnLogic());
    m_engine->rootContext()->setContextProperty("WizardLogic", m_uiLogic->wizardLogic());

#if defined(Q_OS_IOS)
    setStartPageLogic(uiLogic->startPageLogic());
#endif

    m_engine->load(url);

    if (m_engine->rootObjects().size() > 0) {
        m_uiLogic->setQmlRoot(m_engine->rootObjects().at(0));
    }

    if (m_settings->isSaveLogs()) {
        if (!Debug::init()) {
            qWarning() << "Initialization of debug subsystem failed";
        }
    }

#ifdef Q_OS_WIN
    if (m_parser.isSet("a")) m_uiLogic->showOnStartup();
    else emit m_uiLogic->show();
#else
    uiLogic->showOnStartup();
#endif

    // TODO - fix
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
    if (isPrimary()) {
        QObject::connect(this, &SingleApplication::instanceStarted, m_uiLogic, [this](){
            qDebug() << "Secondary instance started, showing this window instead";
            emit m_uiLogic->show();
            emit m_uiLogic->raise();
        });
    }
#endif
}

void AmneziaApplication::registerTypes()
{
    QZXing::registerQMLTypes();

    qRegisterMetaType<VpnProtocol::VpnConnectionState>("VpnProtocol::VpnConnectionState");
    qRegisterMetaType<ServerCredentials>("ServerCredentials");

    qRegisterMetaType<DockerContainer>("DockerContainer");
    qRegisterMetaType<TransportProto>("TransportProto");
    qRegisterMetaType<Proto>("Proto");
    qRegisterMetaType<ServiceType>("ServiceType");
    qRegisterMetaType<Page>("Page");
    qRegisterMetaType<VpnProtocol::VpnConnectionState>("ConnectionState");

    qRegisterMetaType<PageProtocolLogicBase *>("PageProtocolLogicBase *");



    declareQmlPageEnum();
    declareQmlProtocolEnum();
    declareQmlContainerEnum();

    qmlRegisterType<PageType>("PageType", 1, 0, "PageType");
    qmlRegisterType<QRCodeReader>("QRCodeReader", 1, 0, "QRCodeReader");

    QScopedPointer<ContainerProps> containerProps(new ContainerProps);
    qmlRegisterSingletonInstance("ContainerProps", 1, 0, "ContainerProps", containerProps.get());

    QScopedPointer<ProtocolProps> protocolProps(new ProtocolProps);
    qmlRegisterSingletonInstance("ProtocolProps", 1, 0, "ProtocolProps", protocolProps.get());
}

void AmneziaApplication::loadFonts()
{
    QFontDatabase::addApplicationFont(":/fonts/Lato-Black.ttf");
    QFontDatabase::addApplicationFont(":/fonts/Lato-BlackItalic.ttf");
    QFontDatabase::addApplicationFont(":/fonts/Lato-Bold.ttf");
    QFontDatabase::addApplicationFont(":/fonts/Lato-BoldItalic.ttf");
    QFontDatabase::addApplicationFont(":/fonts/Lato-Italic.ttf");
    QFontDatabase::addApplicationFont(":/fonts/Lato-Light.ttf");
    QFontDatabase::addApplicationFont(":/fonts/Lato-LightItalic.ttf");
    QFontDatabase::addApplicationFont(":/fonts/Lato-Regular.ttf");
    QFontDatabase::addApplicationFont(":/fonts/Lato-Thin.ttf");
    QFontDatabase::addApplicationFont(":/fonts/Lato-ThinItalic.ttf");
}

void AmneziaApplication::loadTranslator()
{
    m_translator = new QTranslator;
    if (m_translator->load(QLocale(), QString("amneziavpn"), QLatin1String("_"), QLatin1String(":/translations"))) {
        installTranslator(m_translator);
    }
}

void AmneziaApplication::parseCommands()
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
        Debug::cleanUp();
        QTimer::singleShot(100,[this]{
            quit();
        });
        exec();
    }
}
