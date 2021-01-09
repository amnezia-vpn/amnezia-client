#include <QDebug>
#include <QtNetwork>

#include "localclient.h"

LocalClient::LocalClient(QObject *parent) : QObject(parent),
    m_socket(new QLocalSocket(this))
{
    m_in.setDevice(m_socket);
    m_in.setVersion(QDataStream::Qt_5_10);

    connect(m_socket, &QLocalSocket::readyRead, this, &LocalClient::onReadyRead);
    connect(m_socket, &QLocalSocket::connected, this, &LocalClient::onConnected);
    connect(m_socket, QOverload<QLocalSocket::LocalSocketError>::of(&QLocalSocket::error), this, &LocalClient::displayError);
}

void LocalClient::connectToServer(const QString& name)
{
    m_blockSize = 0;
    m_socket->abort();
    m_socket->connectToServer(name);
}

QString LocalClient::serverName() const
{
    return m_socket->serverName();
}

void LocalClient::onConnected()
{
    emit connected();
}

bool LocalClient::connectedState() const
{
    return (m_socket->state() == QLocalSocket::ConnectedState);
}

quint64 LocalClient::write(const QByteArray& data)
{
    return m_socket->write(data);
}

void LocalClient::onReadyRead()
{
    if (m_socket->canReadLine()) {
        char buf[1024];
        qint64 lineLength = m_socket->readLine(buf, sizeof(buf));
        if (lineLength != -1) {
            QString line = buf;
            line = line.simplified();
            qDebug().noquote() << QString("Read line: '%1'").arg(line);
            emit lineAvailable(line);
        }
    }
}

void LocalClient::displayError(QLocalSocket::LocalSocketError socketError)
{
    Q_UNUSED(socketError)
    qDebug().noquote() << QString("The following error occurred: %1.").arg(m_socket->errorString());
}
