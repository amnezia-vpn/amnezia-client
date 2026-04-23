#ifndef CLI_IPC_H
#define CLI_IPC_H

#include <QObject>

#include "cli_common.h"

class QLocalServer;
class QLocalSocket;

namespace cli
{

class Context;

class IpcServer : public QObject
{
    Q_OBJECT

public:
    explicit IpcServer(Context *context, QObject *parent = nullptr);

    bool isListening() const;
    QString errorString() const;

private:
    Result dispatch(const QJsonObject &request);
    void handleConnection(QLocalSocket *socket);

    Context *m_context = nullptr;
    QLocalServer *m_server = nullptr;
};

namespace ipc
{

QString socketName();
bool isDaemonRunning(int timeoutMs = 250);
bool ensureDaemonRunning(const QString &programPath, int startupTimeoutMs = 5000);
Result sendRequest(const QJsonObject &request, int timeoutMs = 15000);

} // namespace ipc

} // namespace cli

#endif // CLI_IPC_H
