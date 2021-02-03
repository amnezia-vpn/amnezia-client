#ifndef LOCALSERVER_H
#define LOCALSERVER_H

#include <QObject>
#include <QPointer>
#include <QProcess>
#include <QSharedPointer>
#include <QStringList>
#include <QVector>

#include "message.h"
#include "ipcserver.h"

class QLocalServer;
class QLocalSocket;
class QProcess;

class LocalServer : public QObject
{
    Q_OBJECT

public:
    explicit LocalServer(QObject* parent = nullptr);
    ~LocalServer();

//    bool isRunning() const;

protected slots:
//    void onDisconnected();
//    void onNewConnection();

//    void onFinished(int exitCode, QProcess::ExitStatus exitStatus);
//    void onStarted();

private:
//    void finishProcess(const QStringList& messageArgs);
//    void sendMessage(const Message& message);
//    void startProcess(const QStringList& messageArgs);

//    void routesAddRequest(const QStringList& messageArgs);
//    void routeDeleteRequest(const QStringList& messageArgs);

//    void checkAndInstallDriver(const QStringList& messageArgs);

    QSharedPointer<QLocalServer> m_server;
//    QPointer <QLocalSocket> m_clientConnection;

//    QVector<QProcess*> m_processList;
//    bool m_clientConnected;

    IpcServer m_ipcServer;
    QRemoteObjectHost m_serverNode;
    bool m_isRemotingEnabled = false;
};

#endif // LOCALSERVER_H
