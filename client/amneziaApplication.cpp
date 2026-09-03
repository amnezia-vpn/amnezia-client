#include "amneziaApplication.h"

#include <QClipboard>
#include <QFontDatabase>
#include <QLocalServer>
#include <QLocalSocket>
#include <QMimeData>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QQuickItem>
#include <QQuickStyle>
#include <QResource>
#include <QStandardPaths>
#include <QTextDocument>
#include <QTextStream>
#include <QTimer>
#include <QTranslator>
#include <QEvent>
#include <QDir>
#include <QSettings>
#include <QtQuick/QQuickWindow>  
#include <QWindow>     

#include "core/controllers/updateController.h"
#include "core/protocols/qmlRegisterProtocols.h"
#include "logger.h"
#include "ui/controllers/qml/pageController.h"
#include "ui/models/installedAppsModel.h"
#include "ui/utils/mtProxyPublicHostInput.h"
#include "version.h"
#include "core/utils/appUiConfig.h"

#include "platforms/ios/QRCodeReaderBase.h"
#ifdef Q_OS_IOS
    #include "platforms/ios/ioscontextmenu.h"
#endif

#ifdef Q_OS_ANDROID
#include "platforms/android/android_controller.h"
#endif
         

bool AmneziaApplication::m_forceQuit = false;

AmneziaApplication::AmneziaApplication(int &argc, char *argv[]) : AMNEZIA_BASE_CLASS(argc, argv),
      m_optAutostart({QStringLiteral("a"), QStringLiteral("autostart")}, QStringLiteral("System autostart")),
      m_optCleanup  ({QStringLiteral("c"), QStringLiteral("cleanup")}, QStringLiteral("Cleanup logs")),
      m_optConnect  ({QStringLiteral("connect")}, QStringLiteral("Connect to server by stable ID (a legacy zero-based index is also accepted)"), QStringLiteral("server")),
      m_optImport   ({QStringLiteral("import")}, QStringLiteral("Import configuration from data string"), QStringLiteral("data")),
      m_optStatus   ({QStringLiteral("status")}, QStringLiteral("Print connection status as JSON")),
      m_optListServers({QStringLiteral("list-servers")}, QStringLiteral("Print configured servers as JSON")),
      m_optDisconnect({QStringLiteral("disconnect")}, QStringLiteral("Disconnect the active VPN connection")),
      m_optToggle   ({QStringLiteral("toggle")}, QStringLiteral("Toggle the default VPN connection"))
{
    setDesktopFileName(QStringLiteral(APPLICATION_NAME));
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

    m_settings = new SecureQSettings(ORGANIZATION_NAME, APPLICATION_NAME, this);
    m_nam = new QNetworkAccessManager(this);
}

AmneziaApplication::~AmneziaApplication()
{
#ifdef AMNEZIA_DESKTOP
    if (m_vpnConnection && m_vpnConnectionThread.isRunning()) {
        QMetaObject::invokeMethod(m_vpnConnection.get(), "disconnectSlots", Qt::BlockingQueuedConnection);
        
        QMetaObject::invokeMethod(m_vpnConnection.get(), "disconnectFromVpn", Qt::BlockingQueuedConnection);
    }
#endif

    m_vpnConnectionThread.requestInterruption();
    m_vpnConnectionThread.quit();

    if (!m_vpnConnectionThread.wait(3000)) {
        m_vpnConnectionThread.terminate();
        m_vpnConnectionThread.wait(500);
    }

    if (m_engine) {
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

    const QUrl url(QStringLiteral(APP_QML_ENTRYPOINT));
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
#ifdef Q_OS_ANDROID
                QObject::connect(win, &QQuickWindow::sceneGraphError,
                    [](QQuickWindow::SceneGraphError, const QString &msg) {
                        qWarning() << "Scene graph error (suppressed):" << msg;
                    });
                // Keep graphics context alive across hide/show cycles to avoid
                // eglSwapBuffers/makeCurrent being called on a context Android has reclaimed.
                win->setPersistentSceneGraph(true);
                win->setPersistentGraphics(true);
#endif
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
                win->show();
#else
                if (!m_coreController || !m_coreController->pageController()->shouldStartMinimized()) {
                    win->show();
                }
#endif
            }
        },
        Qt::QueuedConnection);

    m_engine->rootContext()->setContextProperty("Debug", &Logger::Instance());

#ifdef MACOS_NE
    m_engine->rootContext()->setContextProperty("IsMacOsNeBuild", true);
#else
    m_engine->rootContext()->setContextProperty("IsMacOsNeBuild", false);
#endif

#ifdef Q_OS_IOS
    m_engine->rootContext()->setContextProperty("IosContextMenu", new IosContextMenu(this));
#endif

#ifdef Q_OS_ANDROID
    m_engine->rootContext()->setContextProperty("IsPlayBuild", AndroidController::instance()->isPlay());
#else
    m_engine->rootContext()->setContextProperty("IsPlayBuild", false);
#endif

    m_vpnConnection.reset(new VpnConnection(nullptr, nullptr));
    m_vpnConnection->moveToThread(&m_vpnConnectionThread);
    m_vpnConnectionThread.start();

    m_coreController.reset(new CoreController(m_vpnConnection, m_settings, m_engine));

    m_engine->addImportPath(QStringLiteral(APP_QML_IMPORT_PATH));

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

    m_coreController->checkForAppUpdates();

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

}

void AmneziaApplication::registerTypes()
{
    qRegisterMetaType<ServerCredentials>("ServerCredentials");

    qRegisterMetaType<DockerContainer>("DockerContainer");
    using namespace amnezia::ProtocolEnumNS;
    qRegisterMetaType<TransportProto>("TransportProto");
    qRegisterMetaType<Proto>("Proto");
    qRegisterMetaType<ServiceType>("ServiceType");

    qmlRegisterType<QRCodeReader>("QRCodeReader", 1, 0, "QRCodeReader");

    m_containerProps.reset(new ContainerProps());
    qmlRegisterSingletonInstance("ContainerProps", 1, 0, "ContainerProps", m_containerProps.get());

    m_protocolProps.reset(new ProtocolProps());
    qmlRegisterSingletonInstance("ProtocolProps", 1, 0, "ProtocolProps", m_protocolProps.get());

    qmlRegisterSingletonType(QUrl("qrc:/ui/qml/Filters/ContainersModelFilters.qml"), "ContainersModelFilters", 1, 0,
                             "ContainersModelFilters");

    qmlRegisterType<InstalledAppsModel>("InstalledAppsModel", 1, 0, "InstalledAppsModel");

    qmlRegisterType<PublicHostInputValidator>("MtProxyConfig", 1, 0, "PublicHostInputValidator");
    qmlRegisterType<PublicHostInputValidator>("TelemtConfig", 1, 0, "PublicHostInputValidator");

    amnezia::declareQmlProtocolEnum();
    Vpn::declareQmlVpnConnectionStateEnum();
    PageLoader::declareQmlPageEnum();
    UpdateState::declareQmlUpdateStateEnum();
}

void AmneziaApplication::loadFonts()
{
    QQuickStyle::setStyle("Basic");

    QFontDatabase::addApplicationFont(QStringLiteral(APP_UI_FONT_RESOURCE));
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
    m_parser.addOption(m_optStatus);
    m_parser.addOption(m_optListServers);
    m_parser.addOption(m_optDisconnect);
    m_parser.addOption(m_optToggle);
    
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
std::optional<int> AmneziaApplication::forwardToRunningInstance()
{
    QLocalSocket socket;
    socket.connectToServer(QStringLiteral(APP_INSTANCE_NAME));
    if (!socket.waitForConnected(500)) {
        return std::nullopt;
    }

    const CliControl::Request request = CliControl::requestFromArguments(arguments());
    socket.write(CliControl::toJsonLine(CliControl::requestToJson(request)));
    if (!socket.waitForBytesWritten(1000) || !socket.waitForReadyRead(3000)) {
        writeCliResponse(QJsonObject {
            { QStringLiteral("version"), 1 },
            { QStringLiteral("ok"), false },
            { QStringLiteral("command"), CliControl::commandName(request.command) },
            { QStringLiteral("error"), QStringLiteral("control_response_timeout") }
        });
        return 2;
    }

    QByteArray responseData = socket.readAll();
    while (!responseData.contains('\n') && socket.waitForReadyRead(100)) {
        responseData.append(socket.readAll());
    }
    QJsonParseError parseError;
    const QJsonObject response = QJsonDocument::fromJson(responseData.trimmed(), &parseError).object();
    if (parseError.error != QJsonParseError::NoError || response.isEmpty()) {
        writeCliResponse(QJsonObject {
            { QStringLiteral("version"), 1 },
            { QStringLiteral("ok"), false },
            { QStringLiteral("command"), CliControl::commandName(request.command) },
            { QStringLiteral("error"), QStringLiteral("invalid_control_response") }
        });
        return 2;
    }
    if (request.isControlCommand()) {
        writeCliResponse(response);
    }
    return response.value(QStringLiteral("ok")).toBool() ? 0 : 2;
}

std::optional<int> AmneziaApplication::handleControlCommandWithoutRunningInstance()
{
    const CliControl::Request request = CliControl::requestFromArguments(arguments());
    if (!request.isControlCommand()) {
        return std::nullopt;
    }
    if (!request.isValid()) {
        writeCliResponse(QJsonObject {
            { QStringLiteral("version"), 1 },
            { QStringLiteral("ok"), false },
            { QStringLiteral("command"), CliControl::commandName(request.command) },
            { QStringLiteral("error"), request.error }
        });
        return 2;
    }
    if (request.command == CliControl::Command::Connect || request.command == CliControl::Command::Toggle) {
        return std::nullopt;
    }

    const bool isStatus = request.command == CliControl::Command::Status;
    const bool isDisconnect = request.command == CliControl::Command::Disconnect;
    writeCliResponse(QJsonObject {
        { QStringLiteral("version"), 1 },
        { QStringLiteral("ok"), isStatus || isDisconnect },
        { QStringLiteral("command"), CliControl::commandName(request.command) },
        { QStringLiteral("state"), QStringLiteral("stopped") },
        { QStringLiteral("serverId"), QString() },
        { QStringLiteral("error"), (isStatus || isDisconnect) ? QString() : QStringLiteral("client_not_running") }
    });
    return (isStatus || isDisconnect) ? 0 : 3;
}

void AmneziaApplication::executeStartupControlCommand()
{
    const CliControl::Request request = CliControl::requestFromArguments(arguments());
    if (request.isControlCommand() && m_coreController) {
        writeCliResponse(m_coreController->handleCliControlRequest(request));
    }
}

void AmneziaApplication::writeCliResponse(const QJsonObject &response) const
{
    QTextStream output(stdout);
    output << QString::fromUtf8(QJsonDocument(response).toJson(QJsonDocument::Compact)) << Qt::endl;
    output.flush();
}

void AmneziaApplication::processControlConnection(QLocalSocket *socket)
{
    if (!socket || socket->property("cliControlProcessed").toBool() || !socket->canReadLine()) {
        return;
    }
    socket->setProperty("cliControlProcessed", true);

    const QByteArray requestData = socket->readLine().trimmed();
    QJsonParseError parseError;
    const QJsonObject requestJson = QJsonDocument::fromJson(requestData, &parseError).object();
    QJsonObject response;
    if (parseError.error != QJsonParseError::NoError || requestJson.isEmpty()) {
        response = QJsonObject {
            { QStringLiteral("version"), 1 },
            { QStringLiteral("ok"), false },
            { QStringLiteral("error"), QStringLiteral("invalid_json") }
        };
    } else if (!m_coreController) {
        response = QJsonObject {
            { QStringLiteral("version"), 1 },
            { QStringLiteral("ok"), false },
            { QStringLiteral("error"), QStringLiteral("not_ready") }
        };
    } else {
        response = m_coreController->handleCliControlRequest(CliControl::requestFromJson(requestJson));
    }

    socket->write(CliControl::toJsonLine(response));
    socket->flush();
    socket->waitForBytesWritten(1000);
    socket->disconnectFromServer();
    socket->deleteLater();
}

bool AmneziaApplication::startLocalServer() {
    const QString serverName(APP_INSTANCE_NAME);
    QLocalServer::removeServer(serverName);

    m_localServer = new QLocalServer(this);
    m_localServer->setSocketOptions(QLocalServer::UserAccessOption);
    if (!m_localServer->listen(serverName)) {
        qCritical() << "Unable to start application control server:" << m_localServer->errorString();
        return false;
    }

    QObject::connect(m_localServer, &QLocalServer::newConnection, this, [this]() {
        while (m_localServer && m_localServer->hasPendingConnections()) {
            QLocalSocket *socket = m_localServer->nextPendingConnection();
            QObject::connect(socket, &QLocalSocket::readyRead, this, [this, socket]() {
                processControlConnection(socket);
            });
            if (socket->bytesAvailable() > 0) {
                processControlConnection(socket);
            }
        }
    });
    return true;
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
