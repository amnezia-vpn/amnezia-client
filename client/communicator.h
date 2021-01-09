#ifndef COMMUNICATOR_H
#define COMMUNICATOR_H

#include <QObject>
#include <QStringList>

#include "message.h"

class LocalClient;

class Communicator : public QObject
{
    Q_OBJECT

public:
    explicit Communicator(QObject* parent = nullptr);
    ~Communicator();

    bool isConnected() const;
    void sendMessage(const Message& message);

signals:
    void messageReceived(const Message& message);

    void comminicatorConnected();
    void comminicatorDisconnected();

protected slots:
    void onConnected();
    void onLineAvailable(const QString& line);

protected:
    QString readData();
    bool writeData(const QString& data);
    void connectToServer();

    LocalClient* m_localClient;
};


#endif // COMMUNICATOR_H
