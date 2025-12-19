#include "amnezia_application.h"

#include <QClipboard>
#include <QFontDatabase>
#include <QLocalServer>
#include <QLocalSocket>
#include <QMimeData>
#include <QQuickItem>
#include <QQuickStyle>
#include <QResource>
#include <QStandardPaths>
#include <QTextDocument>
#include <QTimer>
#include <QTranslator>
#include <QEvent>
#include <QDir>
#include <QSettings>

#include "logger.h"
#include "ui/controllers/pageController.h"
#include "ui/models/installedAppsModel.h"
#include "version.h"

#include "platforms/ios/QRCodeReaderBase.h"

#include "protocols/qml_register_protocols.h"
#include <QtQuick/QQuickWindow>  // for QQuickWindow
#include <QWindow>              // for qobject_cast<QWindow*>

bool AmneziaApplication::m_forceQuit = false;

AmneziaApplication::AmneziaApplication(int &argc, char *argv[]) : AMNEZIA_BASE_CLASS(argc, argv),
      m_optAutostart({QStringLiteral("a"), QStringLiteral("autostart")}, QStringLiteral("System autostart")),
      m_optCleanup  ({QStringLiteral("c"), QStringLiteral("cleanup")}, QStringLiteral("Cleanup logs")),
      m_optConnect  ({QStringLiteral("connect")}, QStringLiteral("Connect to server by index on startup"), QStringLiteral("index")),
      m_optImport   ({QStringLiteral("import")}, QStringLiteral("Import configuration from data string"), QStringLiteral("data"))
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
    if (m_vpnConnection) {
        QMetaObject::invokeMethod(m_vpnConnection.get(), "disconnectSlots", Qt::QueuedConnection);
        QMetaObject::invokeMethod(m_vpnConnection.get(), "disconnectFromVpn", Qt::QueuedConnection);
        QThread::msleep(2000);
    }

    m_vpnConnectionThread.requestInterruption();
    m_vpnConnectionThread.quit();

    if (!m_vpnConnectionThread.wait(3000)) {
        m_vpnConnectionThread.terminate();
        m_vpnConnectionThread.wait(500);
    }

    if (m_engine) {
        QObject::disconnect(m_engine, 0, 0, 0);
        delete m_engine;
    }
}

#ifdef Q_OS_ANDROID
namespace {
    static void clearQtCaches()
    {
        const QString cacheRoot = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
        if (!cacheRoot.isEmpty()) {
            QDir(cacheRoot + "/QtShaderCache").removeRecursively();
            QDir(cacheRoot + "/qmlcache").removeRecursively();
        }
    }
}
#endif

void AmneziaApplication::init()
{
    m_engine = new QQmlApplicationEngine;

    const QUrl url(QStringLiteral("qrc:/ui/qml/main2.qml"));
    QObject::connect(
        m_engine, &QQmlApplicationEngine::objectCreated, this,
        [this, url](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl) {
                QCoreApplication::exit(-1);
                return;
            }
            // install filter on main window
            if (auto win = qobject_cast<QQuickWindow*>(obj)) {
                win->installEventFilter(this);
                win->show();
            }
        },
        Qt::QueuedConnection);

    m_engine->rootContext()->setContextProperty("Debug", &Logger::Instance());

#ifdef MACOS_NE
    m_engine->rootContext()->setContextProperty("IsMacOsNeBuild", true);
#else
    m_engine->rootContext()->setContextProperty("IsMacOsNeBuild", false);
#endif

    m_vpnConnection.reset(new VpnConnection(m_settings));
    m_vpnConnection->moveToThread(&m_vpnConnectionThread);
    m_vpnConnectionThread.start();

    m_coreController.reset(new CoreController(m_vpnConnection, m_settings, m_engine));

    m_engine->addImportPath("qrc:/ui/qml/Modules/");

    if (m_parser.isSet(m_optImport)) {
        const QString data = m_parser.value(m_optImport);
        if (!data.isEmpty()) {
            if (m_coreController) {
                m_coreController->importConfigFromData(data);
            }
        }
    }

    m_engine->load(url);

    m_coreController->setQmlRoot();

    bool enabled = m_settings->isSaveLogs();
#ifndef Q_OS_ANDROID
    if (enabled) {
        if (!Logger::init(false)) {
            qWarning() << "Initialization of debug subsystem failed";
        }
    }
#endif
    Logger::setServiceLogsEnabled(enabled);

#ifdef Q_OS_WIN //TODO
    if (m_parser.isSet(m_optAutostart))
        m_coreController->pageController()->showOnStartup();
    else
        emit m_coreController->pageController()->raiseMainWindow();
#else
    m_coreController->pageController()->showOnStartup();
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

    if (m_parser.isSet(m_optConnect)) {
        bool ok = false;
        int idx = m_parser.value(m_optConnect).toInt(&ok);
        if (ok) {
            QTimer::singleShot(0, this, [this, idx]() {
                if (m_coreController) {
                    m_coreController->openConnectionByIndex(idx);
                }
            });
        }
    }
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

bool AmneziaApplication::parseCommands()
{
    m_parser.setApplicationDescription(APPLICATION_NAME);
    m_parser.addHelpOption();
    m_parser.addVersionOption();

    m_parser.addOption(m_optAutostart);
    m_parser.addOption(m_optCleanup);
    m_parser.addOption(m_optConnect);
    m_parser.addOption(m_optImport);
    
    m_parser.process(*this);

    if (m_parser.isSet(m_optCleanup)) {
        Logger::cleanUp();
        QTimer::singleShot(100, this, [this] { quit(); });
        exec();
        return false;
    }
    return true;
}

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS) && !defined(MACOS_NE)
void AmneziaApplication::startLocalServer() {
    const QString serverName("AmneziaVPNInstance");
    QLocalServer::removeServer(serverName);

    QLocalServer *server = new QLocalServer(this);
    server->listen(serverName);

    QObject::connect(server, &QLocalServer::newConnection, this, [server, this]() {
        if (server) {
            QLocalSocket *clientConnection = server->nextPendingConnection();
            clientConnection->deleteLater();
        }
        emit m_coreController->pageController()->raiseMainWindow(); //TODO
    });
}
#endif

bool AmneziaApplication::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::Close) {
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
        quit();
#else
        if (m_forceQuit) {
            quit();
        } else {
            if (m_coreController && m_coreController->pageController()) {
                m_coreController->pageController()->hideMainWindow();
            }
        }
#endif
        return true; // eat the close
    }
    // call base QObject::eventFilter
    return QObject::eventFilter(watched, event);
}

void AmneziaApplication::forceQuit()
{
    m_forceQuit = true;
    quit();
}

QQmlApplicationEngine *AmneziaApplication::qmlEngine() const
{
    return m_engine;
}

QNetworkAccessManager *AmneziaApplication::networkManager()
{
    return m_nam;
}

QClipboard *AmneziaApplication::getClipboard()
{
    return this->clipboard();
}
