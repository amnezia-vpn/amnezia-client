#include "cli_ipc.h"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QJsonDocument>
#include <QLocalServer>
#include <QLocalSocket>
#include <QProcess>
#include <QThread>
#include <QTimer>

#include "cli_context.h"

namespace cli
{

namespace ipc
{

QString socketName()
{
    return QStringLiteral("AmneziaVPN-cli-daemon");
}

bool isDaemonRunning(int timeoutMs)
{
    QLocalSocket socket;
    socket.connectToServer(socketName());
    return socket.waitForConnected(timeoutMs);
}

bool ensureDaemonRunning(const QString &programPath, int startupTimeoutMs)
{
    if (isDaemonRunning()) {
        return true;
    }

    QProcess daemonProcess;
    daemonProcess.setProgram(programPath);
    daemonProcess.setArguments({ QStringLiteral("__daemon") });
    daemonProcess.setStandardOutputFile(QProcess::nullDevice());
    daemonProcess.setStandardErrorFile(QProcess::nullDevice());

    if (!daemonProcess.startDetached()) {
        return false;
    }

    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < startupTimeoutMs) {
        if (isDaemonRunning()) {
            return true;
        }
        QThread::msleep(100);
    }

    return false;
}

Result sendRequest(const QJsonObject &request, int timeoutMs)
{
    QLocalSocket socket;
    socket.connectToServer(socketName());
    if (!socket.waitForConnected(timeoutMs)) {
        return Result::failure("CLI daemon is not running");
    }

    const QByteArray payload = QJsonDocument(request).toJson(QJsonDocument::Compact) + '\n';
    if (socket.write(payload) != payload.size()) {
        return Result::failure("Failed to send request to CLI daemon");
    }
    if (!socket.waitForBytesWritten(timeoutMs)) {
        return Result::failure("Timed out while sending request to CLI daemon");
    }
    if (!socket.waitForReadyRead(timeoutMs)) {
        return Result::failure("Timed out while waiting for CLI daemon response");
    }

    QByteArray response;
    while (socket.canReadLine() || socket.waitForReadyRead(100)) {
        response.append(socket.readLine());
        if (response.endsWith('\n')) {
            break;
        }
    }

    const QJsonDocument json = QJsonDocument::fromJson(response.trimmed());
    if (!json.isObject()) {
        return Result::failure("CLI daemon returned an invalid response");
    }

    return resultFromJson(json.object());
}

} // namespace ipc

IpcServer::IpcServer(Context *context, QObject *parent)
    : QObject(parent), m_context(context), m_server(new QLocalServer(this))
{
    QLocalServer::removeServer(ipc::socketName());
    m_server->listen(ipc::socketName());

    connect(m_server, &QLocalServer::newConnection, this, [this]() {
        while (m_server->hasPendingConnections()) {
            handleConnection(m_server->nextPendingConnection());
        }
    });
}

bool IpcServer::isListening() const
{
    return m_server->isListening();
}

QString IpcServer::errorString() const
{
    return m_server->errorString();
}

void IpcServer::handleConnection(QLocalSocket *socket)
{
    connect(socket, &QLocalSocket::readyRead, this, [this, socket]() {
        while (socket->canReadLine()) {
            const QByteArray requestBytes = socket->readLine().trimmed();
            if (requestBytes.isEmpty()) {
                continue;
            }

            const QJsonDocument requestJson = QJsonDocument::fromJson(requestBytes);
            Result result = requestJson.isObject() ? dispatch(requestJson.object()) : Result::failure("Invalid CLI request");

            socket->write(QJsonDocument(resultToJson(result)).toJson(QJsonDocument::Compact));
            socket->write("\n");
            socket->flush();
            socket->disconnectFromServer();
        }
    });

    connect(socket, &QLocalSocket::disconnected, socket, &QObject::deleteLater);
}

Result IpcServer::dispatch(const QJsonObject &request)
{
    const QString command = request.value("command").toString();

    if (command != "status" && command != "disconnect" && command != "daemon.shutdown") {
        m_context->reload();
    }

    if (command == "ping") {
        return Result::success("CLI daemon is running");
    }
    if (command == "status") {
        return Result::success("Status loaded", m_context->status());
    }
    if (command == "connect") {
        return m_context->startConnection(request.value("index").toInt(-1));
    }
    if (command == "disconnect") {
        return m_context->stopConnection();
    }
    if (command == "daemon.shutdown") {
        QTimer::singleShot(0, QCoreApplication::instance(), &QCoreApplication::quit);
        return Result::success("CLI daemon is stopping");
    }
    if (command == "servers.list") {
        return m_context->listServers();
    }
    if (command == "servers.show") {
        return m_context->showServer(request.value("index").toInt(-1));
    }
    if (command == "servers.add") {
        amnezia::ServerCredentials credentials;
        credentials.hostName = request.value("host").toString();
        credentials.userName = request.value("user").toString();
        credentials.secretData = request.value("secret").toString();
        credentials.port = request.value("port").toInt(22);
        return m_context->addServer(request.value("name").toString(), credentials);
    }
    if (command == "servers.import_file") {
        return m_context->importConfigFromFile(request.value("file").toString());
    }
    if (command == "servers.import_data") {
        return m_context->importConfigFromData(request.value("data").toString());
    }
    if (command == "servers.remove") {
        return m_context->removeServer(request.value("index").toInt(-1));
    }
    if (command == "servers.set_default") {
        return m_context->setDefaultServer(request.value("index").toInt(-1));
    }
    if (command == "servers.scan") {
        return m_context->scanServer(request.value("index").toInt(-1));
    }
    if (command == "countries.list") {
        return m_context->listCountries(request.value("index").toInt(-1));
    }
    if (command == "countries.set") {
        return m_context->setCountry(request.value("index").toInt(-1), request.value("country").toString());
    }
    if (command == "containers.list") {
        return m_context->listContainers(request.value("index").toInt(-1));
    }
    if (command == "containers.set_default") {
        return m_context->setDefaultContainer(request.value("index").toInt(-1),
                                              containerFromCliName(request.value("container").toString()));
    }
    if (command == "containers.remove") {
        return m_context->removeContainer(request.value("index").toInt(-1),
                                          containerFromCliName(request.value("container").toString()));
    }
    if (command == "install.server") {
        amnezia::ServerCredentials credentials;
        credentials.hostName = request.value("host").toString();
        credentials.userName = request.value("user").toString();
        credentials.secretData = request.value("secret").toString();
        credentials.port = request.value("ssh_port").toInt(22);

        return m_context->installServer(request.value("name").toString(), credentials,
                                        containerFromCliName(request.value("container").toString()),
                                        request.value("container_port").toInt(-1),
                                        ProtocolProps::transportProtoFromString(request.value("transport").toString()),
                                        request.value("key_passphrase").toString());
    }
    if (command == "install.container") {
        return m_context->installContainer(request.value("index").toInt(-1),
                                           containerFromCliName(request.value("container").toString()),
                                           request.value("container_port").toInt(-1),
                                           ProtocolProps::transportProtoFromString(request.value("transport").toString()),
                                           request.value("key_passphrase").toString());
    }
    if (command == "logs.cleanup") {
        return m_context->cleanupLogs();
    }

    return Result::failure(QString("Unknown command '%1'").arg(command));
}

} // namespace cli
