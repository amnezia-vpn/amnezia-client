#include "communicator.h"
#include "defines.h"
#include "localclient.h"
#include "utils.h"

Communicator::Communicator(QObject* parent) : QObject(parent),
    m_localClient(nullptr)
{
    connectToServer();
}

Communicator::~Communicator()
{

}

void Communicator::connectToServer()
{
    if (m_localClient) {
        delete m_localClient;
    }

    qDebug().noquote() << QString("Connect to local server '%1'").arg(SERVICE_NAME);

    m_localClient = new LocalClient(this);
    connect(m_localClient, &LocalClient::connected, this, &Communicator::onConnected);
    connect(m_localClient, &LocalClient::lineAvailable, this, &Communicator::onLineAvailable);
    m_localClient->connectToServer(SERVICE_NAME);
}

void Communicator::onConnected()
{
     qDebug().noquote() << QString("Connected to local server '%1'").arg(m_localClient->serverName());
     Message message(Message::State::Initialize, QStringList({"Ping"}));
     sendMessage(message);
}

void Communicator::onLineAvailable(const QString& line)
{
    Message message(line);
    if (!message.isValid()) {
        qDebug() << "Message is not valid";
        return;
    }

    emit messageReceived(message);
}

bool Communicator::connected() const
{
    if (!m_localClient) {
        return false;
    }

    return m_localClient->connectedState();
}

QString Communicator::readData()
{
    return QString();
}

bool Communicator::writeData(const QString& data)
{
    return m_localClient->write(data.toLocal8Bit());
}

void Communicator::sendMessage(const Message& message)
{
    if (!connected()) {
        return;
    }
    const QString data = message.toString();
    bool status = writeData(data + "\n");

    qDebug().noquote() << QString("Send message '%1', status '%2'").arg(data).arg(Utils::toString(status));
}
