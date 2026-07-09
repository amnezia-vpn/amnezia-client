#pragma once

#include <QObject>
#include <QScopedPointer>
#include <QHttpServer>
#include <QHttpServerRequest>
#include <QHttpServerResponse>
#include <QTcpServer>
#include <QWeakPointer>
#include "iproxyservice.h"

class HttpApi : public QObject {
    Q_OBJECT

public:
    explicit HttpApi(QWeakPointer<IProxyService> service, QObject* parent = nullptr);
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
    QWeakPointer<IProxyService> m_service;
}; 