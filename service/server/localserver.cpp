#include "localserver.h"

#include <QCoreApplication>
#include <QDebug>
#include <QFileInfo>
#include <QLocalServer>
#include <QLocalSocket>
#include <QObject>
#include <QSharedPointer>
#include <QString>

#include "ipc.h"
#include "killswitch.h"
#include "logger.h"

#ifdef Q_OS_WIN
    #include "tapcontroller_win.h"
#endif

#ifdef Q_OS_LINUX
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

uid_t g_allowedUid = static_cast<uid_t>(-1);
bool g_allowedUidSet = false;

static bool checkPeerCredentials(QLocalSocket *socket) {
    struct ucred cred{};
    socklen_t len = sizeof(cred);
    if (getsockopt(socket->socketDescriptor(), SOL_SOCKET, SO_PEERCRED, &cred, &len) != 0) {
        qWarning() << "LocalServer: SO_PEERCRED failed, rejecting connection";
        return false;
    }
    if (cred.uid == 0) return true;
    if (!g_allowedUidSet) {
        g_allowedUid = cred.uid;
        g_allowedUidSet = true;
        qDebug() << "LocalServer: registered session UID" << g_allowedUid;
    }
    if (cred.uid != g_allowedUid) {
        qWarning() << "LocalServer: rejected connection from unauthorized UID" << cred.uid;
        return false;
    }
    return true;
}
#endif

namespace {
Logger logger("WgDaemonServer");
}

LocalServer::LocalServer(QObject *parent) : QObject(parent),
    m_ipcServer(this)
{
    // Create the server and listen outside of QtRO
    m_server = QSharedPointer<QLocalServer>(new QLocalServer(this));
    m_server->setSocketOptions(QLocalServer::WorldAccessOption);

    if (!m_server->listen(amnezia::getIpcServiceUrl())) {
        qDebug() << QString("Unable to start the server: %1.").arg(m_server->errorString());
        return;
    }

    QObject::connect(m_server.data(), &QLocalServer::newConnection, this, [this]() {
        qDebug() << "LocalServer new connection";
        QLocalSocket *conn = m_server->nextPendingConnection();
#ifdef Q_OS_LINUX
        if (!checkPeerCredentials(conn)) {
            conn->close();
            conn->deleteLater();
            return;
        }
#endif
        m_serverNode.addHostSideConnection(conn);

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
    KillSwitch::instance()->init();

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

