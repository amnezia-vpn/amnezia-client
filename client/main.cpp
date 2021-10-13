#include <QApplication>
#include <QFontDatabase>
#include <QCommandLineParser>
#include <QMessageBox>
#include <QTranslator>
#include <QTimer>
#include <QLoggingCategory>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "ui/uilogic.h"
#include <QDebug>

#include "ui/pages.h"

#include "ui/pages_logic/AppSettingsLogic.h"
#include "ui/pages_logic/GeneralSettingsLogic.h"
#include "ui/pages_logic/NetworkSettingsLogic.h"
#include "ui/pages_logic/ServerConfiguringProgressLogic.h"
#include "ui/pages_logic/NewServerProtocolsLogic.h"
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

#include "ui/uilogic.h"

#include "debug.h"
#include "defines.h"

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
#define QAPPLICATION_CLASS QGuiApplication
#include "singleapplication.h"
#undef QAPPLICATION_CLASS
#endif


#ifdef Q_OS_WIN
#include "Windows.h"
#endif

static void loadTranslator()
{
    QTranslator* translator = new QTranslator;
    if (translator->load(QLocale(), QString("amneziavpn"), QLatin1String("_"), QLatin1String(":/translations"))) {
        qApp->installTranslator(translator);
    }
}

int main(int argc, char *argv[])
{
    QLoggingCategory::setFilterRules(QStringLiteral("qtc.ssh=false"));

    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling, true);

#ifdef Q_OS_WIN
    AllowSetForegroundWindow(ASFW_ANY);
#endif

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
    SingleApplication app(argc, argv, true, SingleApplication::Mode::User | SingleApplication::Mode::SecondaryNotification);

    if (!app.isPrimary()) {
        QTimer::singleShot(1000, &app, [&](){
            app.quit();
        });
        return app.exec();
    }
#else
    QApplication app(argc, argv);
#endif

#ifdef Q_OS_WIN
    AllowSetForegroundWindow(0);
#endif

    loadTranslator();

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

    app.setApplicationName(APPLICATION_NAME);
    app.setOrganizationName(ORGANIZATION_NAME);
    app.setApplicationDisplayName(APPLICATION_NAME);

    QCommandLineParser parser;
    parser.setApplicationDescription(APPLICATION_NAME);
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption c_autostart {{"a", "autostart"}, "System autostart"};
    parser.addOption(c_autostart);

    parser.process(app);

    if (!Debug::init()) {
        qWarning() << "Initialization of debug subsystem failed";
    }

    app.setQuitOnLastWindowClosed(false);

    qRegisterMetaType<VpnProtocol::ConnectionState>("VpnProtocol::ConnectionState");
    qRegisterMetaType<DockerContainer>("DockerContainer");
    qRegisterMetaType<TransportProto>("TransportProto");
    qRegisterMetaType<Protocol>("Protocol");
    qRegisterMetaType<ServiceType>("ServiceType");
    qRegisterMetaType<Page>("Page");

    qRegisterMetaType<PageProtocolLogicBase *>("PageProtocolLogicBase *");

    UiLogic *uiLogic = new UiLogic;

    QQmlApplicationEngine engine;

    declareQmlPageEnum();
    declareQmlProtocolEnum();
    declareQmlContainerEnum();

    QScopedPointer<ContainerProps> containerProps(new ContainerProps);
    qmlRegisterSingletonInstance("ContainerProps", 1, 0, "ContainerProps", containerProps.get());

    QScopedPointer<ProtocolProps> protocolProps(new ProtocolProps);
    qmlRegisterSingletonInstance("ProtocolProps", 1, 0, "ProtocolProps", protocolProps.get());

    const QUrl url(QStringLiteral("qrc:/ui/qml/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);

    engine.rootContext()->setContextProperty("UiLogic", uiLogic);

    engine.rootContext()->setContextProperty("AppSettingsLogic", uiLogic->appSettingsLogic());
    engine.rootContext()->setContextProperty("GeneralSettingsLogic", uiLogic->generalSettingsLogic());
    engine.rootContext()->setContextProperty("NetworkSettingsLogic", uiLogic->networkSettingsLogic());
    engine.rootContext()->setContextProperty("ServerConfiguringProgressLogic", uiLogic->serverConfiguringProgressLogic());
    engine.rootContext()->setContextProperty("NewServerProtocolsLogic", uiLogic->newServerProtocolsLogic());
    engine.rootContext()->setContextProperty("ServerListLogic", uiLogic->serverListLogic());
    engine.rootContext()->setContextProperty("ServerSettingsLogic", uiLogic->serverSettingsLogic());
    engine.rootContext()->setContextProperty("ServerContainersLogic", uiLogic->serverVpnProtocolsLogic());
    engine.rootContext()->setContextProperty("ShareConnectionLogic", uiLogic->shareConnectionLogic());
    engine.rootContext()->setContextProperty("SitesLogic", uiLogic->sitesLogic());
    engine.rootContext()->setContextProperty("StartPageLogic", uiLogic->startPageLogic());
    engine.rootContext()->setContextProperty("VpnLogic", uiLogic->vpnLogic());
    engine.rootContext()->setContextProperty("WizardLogic", uiLogic->wizardLogic());

    engine.load(url);

    // TODO - fix
//#ifdef Q_OS_WIN
//    if (parser.isSet("a")) mainWindow.showOnStartup();
//    else mainWindow.show();
//#else
//    mainWindow.showOnStartup();
//#endif


     // TODO - fix
//#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
//    if (app.isPrimary()) {
//        QObject::connect(&app, &SingleApplication::instanceStarted, &mainWindow, [&](){
//            qDebug() << "Secondary instance started, showing this window instead";
//            mainWindow.show();
//            mainWindow.showNormal();
//            mainWindow.raise();
//        });
//    }
//#endif

    return app.exec();
}
