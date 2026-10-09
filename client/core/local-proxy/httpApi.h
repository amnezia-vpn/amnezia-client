#ifndef HTTPAPI_H
#define HTTPAPI_H

#include <QHttpServer>
#include <QHttpServerRequest>
#include <QHttpServerResponse>
#include <QObject>
#include <QScopedPointer>
#include <QTcpServer>
#include <QWeakPointer>

class ProxyService;

class HttpApi : public QObject
{
    Q_OBJECT

public:
    explicit HttpApi(QWeakPointer<ProxyService> service, QObject *parent = nullptr);
    ~HttpApi();

    bool start(quint16 port);
    void stop();

private:
    void setupRoutes();

    QHttpServerResponse handlePostUp();
    QHttpServerResponse handlePostDown();
    QHttpServerResponse handleGetPing() const;

    QHttpServer m_server;
    QScopedPointer<QTcpServer> m_tcpServer;
    QWeakPointer<ProxyService> m_service;
};

#endif // HTTPAPI_H
