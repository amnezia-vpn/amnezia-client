#include <QCoreApplication>
#include <QFileInfo>
#include <QLocalServer>
#include <QLocalSocket>

#include "ipc.h"
#include "localserver.h"
#include "utilities.h"

#include "logger.h"
#include "router.h"

#ifdef Q_OS_WIN
#include "tapcontroller_win.h"
#endif

#ifdef Q_OS_MAC
#include <iostream>
#endif

namespace {
Logger logger("WgDaemonServer");
}

LocalServer::LocalServer(QObject *parent) : QObject(parent), m_ipcServer(this) {
  // Create the server and listen outside of QtRO
  m_server = QSharedPointer<QLocalServer>(new QLocalServer(this));
  m_server->setSocketOptions(QLocalServer::WorldAccessOption);

  if (!m_server->listen(amnezia::getIpcServiceUrl())) {
    qDebug() << QString("Unable to start the server: %1.")
                    .arg(m_server->errorString());
    return;
  }

  QObject::connect(
      m_server.data(), &QLocalServer::newConnection, this, [this]() {
        qDebug() << "LocalServer new connection";
        QLocalSocket *socket = m_server->nextPendingConnection();
        m_connectionCount++;
        QObject::connect(socket, &QLocalSocket::disconnected, this, [this]() {
          if (m_connectionCount > 0)
            m_connectionCount--;
          if (m_connectionCount == 0) {
            if (m_ipcServer.hasPendingConnectionClose()) {
              m_ipcServer.amneziaKilled();
              m_tun2socks.stop();
            }
          }
        });

        m_serverNode.addHostSideConnection(socket);

        if (!m_isRemotingEnabled) {
          m_isRemotingEnabled = true;
          m_serverNode.enableRemoting(&m_ipcServer);
          m_serverNode.enableRemoting(&m_tun2socks);
        }
      });

  // Init Mozilla Wireguard Daemon
  if (!server.initialize()) {
    logger.error() << "Failed to initialize the server";
    return;
  }

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

LocalServer::~LocalServer() { qDebug() << "Local server stopped"; }
