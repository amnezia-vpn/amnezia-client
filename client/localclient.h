#ifndef LOCALCLIENT_H
#define LOCALCLIENT_H

#include <QDataStream>
#include <QLocalSocket>

class LocalClient : public QObject
{
    Q_OBJECT

public:
    explicit LocalClient(QObject *parent = nullptr);

    QString serverName() const;
    bool connectedState() const;
    quint64 write(const QByteArray& data);
    void connectToServer(const QString& name);

signals:
    void connected();
    void lineAvailable(const QString& line);

private slots:
    void displayError(QLocalSocket::LocalSocketError socketError);
    void onConnected();
    void onReadyRead();

private:
    QLocalSocket* m_socket;
    QDataStream m_in;
    quint32 m_blockSize;
};

#endif // LOCALCLIENT_H
