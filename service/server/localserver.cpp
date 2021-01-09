#include <QCoreApplication>
#include <QFileInfo>
#include <QLocalServer>
#include <QLocalSocket>

#include "localserver.h"
#include "utils.h"

LocalServer::LocalServer(QObject *parent) : QObject(parent),
    m_clientConnection(nullptr),
    m_clientConnected(false)
{
    m_server = new QLocalServer(this);
    m_server->setSocketOptions(QLocalServer::WorldAccessOption);

    if (!m_server->listen(Utils::serverName())) {
        qDebug() << QString("Unable to start the server: %1.").arg(m_server->errorString());
        return;
    }

    connect(m_server, &QLocalServer::newConnection, this, &LocalServer::onNewConnection);

    qDebug().noquote() << QString("Local server started on '%1'").arg(m_server->serverName());
}

LocalServer::~LocalServer()
{
    m_clientConnected = false;
    m_server->disconnect();

    QFile::remove(Utils::serverName());

    qDebug() << "Local server stopped";
}

bool LocalServer::isRunning() const
{
    return m_server->isListening();
}

void LocalServer::onNewConnection()
{
    if (m_clientConnection) {
        m_clientConnection->deleteLater();
    }

    m_clientConnection = m_server->nextPendingConnection();
    connect(m_clientConnection, &QLocalSocket::disconnected, this, &LocalServer::onDisconnected);
    m_clientConnected = true;

    qDebug() << "New connection";

    for(;;) {
        qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
        if (!m_clientConnected || !m_clientConnection) {
            break;
        }

        if (m_clientConnection->waitForReadyRead(1000) && m_clientConnection->canReadLine()) {
            char buf[1024];
            qint64 lineLength = m_clientConnection->readLine(buf, sizeof(buf));
            if (lineLength != -1) {
                QString line = buf;
                line = line.simplified();
                qDebug().noquote() << QString("Read line: '%1'").arg(line);
                Message incomingMessage(line);
                if (!incomingMessage.isValid()) {
                    qWarning().noquote() << "Message is not valid!";
                    continue;
                }

                switch (incomingMessage.state()) {
                case Message::State::Initialize:
                    sendMessage(Message(Message::State::Initialize, QStringList({"Server"})));
                    break;
                case Message::State::StartRequest:
                    startProcess(incomingMessage.args());
                    break;
                case Message::State::FinishRequest:
                    finishProcess(incomingMessage.args());
                    break;
                default:
                    ;
                }
            }
        }
    }

    qDebug() << "Released";
}

void LocalServer::finishProcess(const QStringList& args)
{
    Q_UNUSED(args)
}

void LocalServer::startProcess(const QStringList& messageArgs)
{
    if (messageArgs.size() < 1) {
        return;
    }

    QProcess* process = new QProcess();
    connect(process, SIGNAL(started()), this, SLOT(onStarted()));
    connect(process, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(onFinished(int, QProcess::ExitStatus)));

    const QString program = messageArgs.at(0);
    QStringList args;
    for (int i = 1; i < messageArgs.size(); i++) {
        args.append(messageArgs.at(i));
    }

    QFileInfo fi(program);
    const QString baseName = fi.baseName();
    if (!fi.exists()) {
        qWarning() << "This program does not exist";
        sendMessage(Message(Message::State::Started, QStringList({baseName})));
        sendMessage(Message(Message::State::Finished, QStringList({baseName, QString::number(-1)})));
        return;
    }

    process->setObjectName(baseName);

    qDebug().noquote() << QString("Start process '%1' - '%2' with args '%3'")
                .arg(baseName).arg(program).arg(args.join(","));

    process->start(program, args);
    m_processList.append(process);
}

void LocalServer::onFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitStatus)

    QProcess* process = (QProcess*)sender();
    sendMessage(Message(Message::State::Finished, QStringList({process->objectName(), QString::number(exitCode)})));
}

void LocalServer::onStarted()
{
    QProcess* process = (QProcess*)sender();
    sendMessage(Message(Message::State::Started, QStringList({process->objectName()})));
}

void LocalServer::onDisconnected()
{
    if (!m_clientConnected) {
        return;
    }

    m_clientConnected = false;
    QLocalSocket* clientConnection = (QLocalSocket*)sender();
    clientConnection->deleteLater();

    qDebug() << "Diconnected";
}

void LocalServer::sendMessage(const Message& message)
{
    if (!m_clientConnection || !m_clientConnected) {
        qDebug()<< "Cannot send data, remote peer is not connected";
        return;
    }

    const QString data = message.toString();
    bool status = m_clientConnection->write(QString(data + "\n").toUtf8());

    qDebug().noquote() << QString("Send message '%1', status '%2'").arg(data).arg(Utils::toString(status));
}

