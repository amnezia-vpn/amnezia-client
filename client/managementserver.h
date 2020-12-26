#ifndef MANAGEMENTSERVER_H
#define MANAGEMENTSERVER_H

#include <QAbstractSocket>
#include <QString>

class QTcpServer;
class QTcpSocket;

class ManagementServer : public QObject
{
    Q_OBJECT

public:
    explicit ManagementServer(QObject *parent = nullptr);
    ~ManagementServer();

    bool start(const QString& host, unsigned int port);
    void stop();
    bool isOpen() const;

    QString readLine();
    qint64 writeCommand(const QString& message);

    QTcpSocket* socket() const;

signals:
    void readyRead();
    void serverStarted();

protected slots:
    void onAcceptError(QAbstractSocket::SocketError socketError);
    void onNewConnection();
    void onReadyRead();
    void onSocketDisconnected();
    void onSocketError(QAbstractSocket::SocketError socketError);

protected:
    QTcpServer* m_tcpServer;
    QTcpSocket* m_socket;
};

#endif // MANAGEMENTSERVER_H
