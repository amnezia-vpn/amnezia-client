#include "amneziaApplication.h"

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
#include <QFileOpenEvent>
#include <QUrl>
#include <QCoreApplication>
#include <QtQuick/QQuickWindow>  
#include <QWindow>     

#include "core/protocols/qmlRegisterProtocols.h"
#include "logger.h"
#include "ui/controllers/qml/pageController.h"
#include "ui/models/installedAppsModel.h"
#include "version.h"

#include "platforms/ios/QRCodeReaderBase.h"
         

bool AmneziaApplication::m_forceQuit = false;

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS) && !defined(MACOS_NE)
namespace {
bool g_secondaryInstanceForDeepLink = false;
}

void AmneziaApplication::markSecondaryInstanceForDeepLink()
{
    g_secondaryInstanceForDeepLink = true;
}
#endif

AmneziaApplication::AmneziaApplication(int &argc, char *argv[]) : AMNEZIA_BASE_CLASS(argc, argv),
      m_optAutostart({QStringLiteral("a"), QStringLiteral("autostart")}, QStringLiteral("System autostart")),
      m_optCleanup  ({QStringLiteral("c"), QStringLiteral("cleanup")}, QStringLiteral("Cleanup logs")),
      m_optConnect  ({QStringLiteral("connect")}, QStringLiteral("Connect to server by index on startup"), QStringLiteral("index")),
      m_optImport   ({QStringLiteral("import")}, QStringLiteral("Import configuration from data string"), QStringLiteral("data"))
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
    if (m_vpnConnection) {
        m_vpnConnection->disconnectSlots();
        m_vpnConnection->disconnectFromVpn();
    }
#endif

    if (m_engine) {
        delete m_engine;
    }
}

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
namespace {
QString vpnUrlFromArguments(const QStringList &args)
{
    for (const QString &arg : args) {
        const QString t = arg.trimmed();
        if (t.startsWith(QLatin1String("vpn://"), Qt::CaseInsensitive)) {
            return t;
        }
    }
    return {};
}
} // namespace
#endif

#if defined(Q_OS_WIN) && !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
namespace {
void registerWindowsVpnUrlSchemeIfNeeded()
{
    QSettings flag(ORGANIZATION_NAME, APPLICATION_NAME);
    if (flag.value(QStringLiteral("protocolHandler/vpnRegistered")).toBool()) {
        return;
    }

    const QString exe = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());

    QSettings vpnKey(QStringLiteral("HKEY_CURRENT_USER\\Software\\Classes\\vpn"), QSettings::NativeFormat);
    vpnKey.setValue(QStringLiteral("."), QStringLiteral("URL:AmneziaVPN"));
    vpnKey.setValue(QStringLiteral("URL Protocol"), QString());

    QSettings cmdKey(QStringLiteral("HKEY_CURRENT_USER\\Software\\Classes\\vpn\\shell\\open\\command"), QSettings::NativeFormat);
    cmdKey.setValue(QStringLiteral("."), QStringLiteral("\"%1\" \"%2\"").arg(exe, QStringLiteral("%1")));

    flag.setValue(QStringLiteral("protocolHandler/vpnRegistered"), true);
}
} // namespace
#endif

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

    m_vpnConnection.reset(new VpnConnection(nullptr, nullptr));
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

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
#    ifdef Q_OS_WIN
    registerWindowsVpnUrlSchemeIfNeeded();
#    endif
    if (!m_parser.isSet(m_optImport)) {
        const QString vpnArg = vpnUrlFromArguments(QCoreApplication::arguments());
        if (!vpnArg.isEmpty()) {
            m_pendingVpnDeepLink.clear();
            QTimer::singleShot(0, this, [this, vpnArg]() { deliverVpnDeepLink(vpnArg); });
        }
    }
    if (!m_pendingVpnDeepLink.isEmpty()) {
        const QString pending = std::move(m_pendingVpnDeepLink);
        QTimer::singleShot(0, this, [this, pending]() { deliverVpnDeepLink(pending); });
    }
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

    amnezia::declareQmlProtocolEnum();
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
void AmneziaApplication::startLocalServer()
{
    const QString serverName(QStringLiteral("AmneziaVPNInstance"));
    QLocalServer::removeServer(serverName);

    QLocalServer *server = new QLocalServer(this);
    if (!server->listen(serverName)) {
        qWarning() << "QLocalServer::listen failed:" << server->errorString();
    }

    QObject::connect(server, &QLocalServer::newConnection, this, [this, server]() {
        QLocalSocket *sock = server->nextPendingConnection();
        if (!sock) {
            return;
        }

        QString vpnPayload;
        if (sock->waitForReadyRead(3000)) {
            const QByteArray buf = sock->readAll();
            static const QByteArray prefix = QByteArrayLiteral("VPN\n");
            if (buf.startsWith(prefix)) {
                vpnPayload = QString::fromUtf8(buf.mid(prefix.size())).trimmed();
            }
        }
        sock->deleteLater();

        if (!vpnPayload.isEmpty()) {
            QTimer::singleShot(0, this, [this, vpnPayload]() { deliverVpnDeepLink(vpnPayload); });
        }

        QTimer::singleShot(0, this, [this]() {
            if (m_coreController && m_coreController->pageController()) {
                emit m_coreController->pageController()->raiseMainWindow();
            }
        });
    }, Qt::QueuedConnection);
}
#endif

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
void AmneziaApplication::deliverVpnDeepLink(const QString &payload)
{
    if (!m_coreController) {
        return;
    }
    const QString trimmed = payload.trimmed();
    if (trimmed.isEmpty()) {
        return;
    }
    m_coreController->openVpnKeyImportPreview(trimmed);
}

QString vpnPayloadFromFileOpenUrl(const QUrl &url)
{
    const QString decoded = url.toString(QUrl::PrettyDecoded);
    const int idx = decoded.indexOf(QLatin1String("vpn://"), 0, Qt::CaseInsensitive);
    if (idx >= 0) {
        qDebug() << "vpn://" << decoded;
        return decoded.mid(idx).trimmed();
    }
    if (url.scheme().compare(QLatin1String("vpn"), Qt::CaseInsensitive) == 0) {
        qDebug() << "vpn://" << decoded;
        return decoded.trimmed();
    }
    return {};
}

#if !defined(MACOS_NE)
bool forwardVpnPayloadToPrimaryInstance(const QString &payload)
{
    if (payload.trimmed().isEmpty()) {
        return false;
    }
    QLocalSocket socket;
    socket.connectToServer(QStringLiteral("AmneziaVPNInstance"));
    if (!socket.waitForConnected(800)) {
        return false;
    }
    const QByteArray msg = QByteArrayLiteral("VPN\n") + payload.toUtf8() + '\n';
    socket.write(msg);
    socket.waitForBytesWritten(3000);
    socket.flush();
    return true;
}
#endif

bool AmneziaApplication::event(QEvent *event)
{
    if (event->type() == QEvent::FileOpen) {
        auto *foe = static_cast<QFileOpenEvent *>(event);
        const QUrl &url = foe->url();
        qDebug() << "url:" << url;
        const QString payload = vpnPayloadFromFileOpenUrl(url);
        qDebug() << "payload" << payload;
        if (!payload.isEmpty()) {
            if (m_coreController) {
                QTimer::singleShot(0, this, [this, payload]() { deliverVpnDeepLink(payload); });
                return true;
            }
#if !defined(MACOS_NE)
            // True secondary process (main skipped init): forward to the primary instance.
            if (g_secondaryInstanceForDeepLink) {
                if (forwardVpnPayloadToPrimaryInstance(payload)) {
                    qInfo().noquote() << "Forwarded vpn deep link to primary instance, bytes:" << payload.size();
                    QTimer::singleShot(0, qApp, &QCoreApplication::quit);
                    return true;
                }
                qWarning() << "vpn FileOpen: secondary instance could not reach primary (socket)";
                return true;
            }
#endif
            // Cold start: FileOpen can arrive while init() is still running (CoreController not ready yet).
            // Do not forward to our own local server — queue and flush at end of init().
            m_pendingVpnDeepLink = payload;
            return true;
        }
    }
    return AMNEZIA_BASE_CLASS::event(event);
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
