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
    
    // Config management endpoints
    QHttpServerResponse handleGetConfigs(const QHttpServerRequest &request);
    QHttpServerResponse handleAddConfigs(const QHttpServerRequest &request);
    QHttpServerResponse handleUpdateConfigs(const QHttpServerRequest &request);
    QHttpServerResponse handleDeleteConfig(const QHttpServerRequest &request);
    QHttpServerResponse handleActivateConfig(const QHttpServerRequest &request);
    QHttpServerResponse handleGetActiveConfig(const QHttpServerRequest &request);

    // Xray control endpoints
    QJsonObject handlePostUp();
    QJsonObject handlePostDown();
    QJsonObject handleGetPing() const;

    QHttpServer m_server;
    QScopedPointer<QTcpServer> m_tcpServer;
    QWeakPointer<IProxyService> m_service;
}; 