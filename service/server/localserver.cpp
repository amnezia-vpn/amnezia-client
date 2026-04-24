#include "localserver.h"

#include <QCoreApplication>
#include <QDebug>
#include <QFileInfo>
#include <QLocalServer>
#include <QLocalSocket>
#include <QObject>
#include <QSharedPointer>
#include <QString>
#include <QTimer>
#include <functional>

#include "ipc.h"
#include "killswitch.h"
#include "logger.h"

#ifdef Q_OS_WIN
    #include "tapcontroller_win.h"
#endif

namespace {
Logger logger("WgDaemonServer");
constexpr int kKillSwitchInitRetryDelayMs = 1500;
constexpr int kKillSwitchInitMaxRetries = 20;
constexpr int kKillSwitchInitLongRetryDelayMs = 10000;
}

LocalServer::LocalServer(QObject *parent) : QObject(parent),
    m_ipcServer(this)
{
    // Create the server and listen outside of QtRO
    m_server = QSharedPointer<QLocalServer>(new QLocalServer(this));
    m_server->setSocketOptions(QLocalServer::WorldAccessOption);

    const QString ipcUrl = fblink::getIpcServiceUrl();
    if (!m_server->listen(ipcUrl)) {
        const QString firstError = m_server->errorString();
        qWarning() << QString("Unable to start the server (%1): %2. Retrying after cleanup.")
                              .arg(ipcUrl, firstError);

        m_server->close();
        QLocalServer::removeServer(ipcUrl);

        if (!m_server->listen(ipcUrl)) {
            qCritical() << QString("Unable to start the server after cleanup (%1): %2.")
                                    .arg(ipcUrl, m_server->errorString());
            QCoreApplication::exit(1);
            ::exit(1);
            return;
        }
    }

    QObject::connect(m_server.data(), &QLocalServer::newConnection, this, [this]() {
        qDebug() << "LocalServer new connection";
        m_serverNode.addHostSideConnection(m_server->nextPendingConnection());

        if (!m_isRemotingEnabled) {
            m_isRemotingEnabled = true;
            m_serverNode.enableRemoting(&m_ipcServer);
        }
    });

    // Init Mozilla Wireguard Daemon
    if (!server.initialize()) {
        logger.error() << "Failed to initialize the server";
        return;
    }

    m_networkWatcher.initialize();
    connect(&m_networkWatcher, &NetworkWatcher::networkChanged, &m_ipcServer, &IpcServer::networkChanged);
    connect(&m_networkWatcher, &NetworkWatcher::wakeup, &m_ipcServer, &IpcServer::wakeup);
    auto retriesLeft = QSharedPointer<int>::create(kKillSwitchInitMaxRetries);
    auto initKillSwitch = QSharedPointer<std::function<void()>>::create();
    *initKillSwitch = [this, retriesLeft, initKillSwitch]() {
        if (KillSwitch::instance()->init()) {
            logger.info() << "Kill switch state recovered on service startup";
            return;
        }

        if (*retriesLeft > 0) {
            (*retriesLeft)--;
            logger.warning() << "Kill switch init failed, retrying in" << kKillSwitchInitRetryDelayMs
                             << "ms, retries left:" << *retriesLeft;
            QTimer::singleShot(kKillSwitchInitRetryDelayMs, this, [initKillSwitch]() {
                (*initKillSwitch)();
            });
            return;
        }

        // Keep recovering in background for slow boot scenarios (e.g. WFP/BFE
        // startup lag after system boot). Without this, stale kill-switch state
        // can survive until next reboot.
        logger.warning() << "Kill switch recovery still failing after fast retries,"
                         << "continuing background retry in" << kKillSwitchInitLongRetryDelayMs << "ms";
        QTimer::singleShot(kKillSwitchInitLongRetryDelayMs, this, [initKillSwitch]() {
            (*initKillSwitch)();
        });
    };
    (*initKillSwitch)();

#ifdef Q_OS_LINUX
    // Signal handling for a proper shutdown.
    QObject::connect(qApp, &QCoreApplication::aboutToQuit,
                     []() { LinuxDaemon::instance()->deactivate(); });
#endif

#ifdef Q_OS_MAC
    // Signal handling for a proper shutdown.
    QObject::connect(qApp, &QCoreApplication::aboutToQuit,
                     []() { MacOSDaemon::instance()->deactivate(); });
#endif

#ifdef Q_OS_WIN
    // Signal handling for a proper shutdown.
    QObject::connect(qApp, &QCoreApplication::aboutToQuit,
                     []() { WindowsDaemon::instance()->deactivate(); });
#endif
}

LocalServer::~LocalServer()
{
    qDebug() << "Local server stopped";
}
