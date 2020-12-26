#ifndef LOCALSERVER_H
#define LOCALSERVER_H

#include <QObject>
#include <QProcess>
#include <QStringList>
#include <QVector>

#include "message.h"

class QLocalServer;
class QLocalSocket;
class QProcess;

class LocalServer : public QObject
{
    Q_OBJECT

public:
    explicit LocalServer(const QString& name, QObject* parent = nullptr);
    ~LocalServer();

    bool isRunning() const;

protected slots:
    void onDisconnected();
    void onNewConnection();

    void onFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onStarted();

private:
    void finishProcess(const QStringList& messageArgs);
    void sendMessage(const Message& message);
    void startProcess(const QStringList& messageArgs);

    QLocalServer* m_server;
    QLocalSocket* m_clientConnection;
    QVector<QProcess*> m_processList;
    bool m_clientConnected;
};

#endif // LOCALSERVER_H
