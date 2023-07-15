#ifndef LOCALSERVER_H
#define LOCALSERVER_H

#include <QObject>
#include <QPointer>
#include <QProcess>
#include <QSharedPointer>
#include <QStringList>
#include <QVector>

#include "ipcserver.h"

#ifdef Q_OS_MAC
#include "macos/daemon/macosdaemon.h"
#include "../../client/daemon/daemonlocalserver.h"
#endif

class QLocalServer;
class QLocalSocket;
class QProcess;

class LocalServer : public QObject
{
    Q_OBJECT

public:
    explicit LocalServer(QObject* parent = nullptr);
    ~LocalServer();

    QSharedPointer<QLocalServer> m_server;

    IpcServer m_ipcServer;
    QRemoteObjectHost m_serverNode;
    bool m_isRemotingEnabled = false;

#ifdef Q_OS_MAC
    MacOSDaemon daemon;
    DaemonLocalServer server{qApp};
#endif
};

#endif // LOCALSERVER_H
