#include <QCoreApplication>
#include <QFileInfo>
#include <QLocalServer>
#include <QLocalSocket>

#include "ipc.h"
#include "localserver.h"
#include "utilities.h"

#include "router.h"
#include "logger.h"

#ifdef Q_OS_WIN
#include "tapcontroller_win.h"
#endif

namespace {
Logger logger("MacOSDaemonServer");
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

    // Signal handling for a proper shutdown.
    QObject::connect(qApp, &QCoreApplication::aboutToQuit,
                     []() { MacOSDaemon::instance()->deactivate(); });
}

LocalServer::~LocalServer()
{
    qDebug() << "Local server stopped";
}

