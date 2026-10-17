#include "httpApi.h"

#include "proxyService.h"

#include <QDebug>
#include <QHostAddress>
#include <QJsonObject>

namespace
{
    QJsonValue proxyPortValue(int port)
    {
        return port > 0 ? QJsonValue(port) : QJsonValue::Null;
    }

    QHttpServerResponse makeServiceUnavailableResponse(bool includeProxyPort)
    {
        QJsonObject payload { { "status", "error" } };
        if (includeProxyPort) {
            payload.insert("proxyPort", QJsonValue::Null);
        }
        return QHttpServerResponse(payload, QHttpServerResponse::StatusCode::ServiceUnavailable);
    }
}

HttpApi::HttpApi(QWeakPointer<ProxyService> service, QObject *parent)
    : QObject(parent), m_tcpServer(new QTcpServer(this)), m_service(service)
{
}

HttpApi::~HttpApi()
{
    stop();
}

bool HttpApi::start(quint16 port)
{
    if (!m_tcpServer->listen(QHostAddress::LocalHost, port)) {
        qWarning() << "Failed to start HTTP API server on port" << port;
        return false;
    }

    setupRoutes();
    m_server.bind(m_tcpServer.data());

    qDebug() << "HTTP API server is running on localhost:" << m_tcpServer->serverPort();
    return true;
}

void HttpApi::stop()
{
    if (m_tcpServer) {
        m_tcpServer->close();
    }
}

void HttpApi::setupRoutes()
{
    m_server.route("/api/v1/up", QHttpServerRequest::Method::Post, [this] { return handlePostUp(); });
    m_server.route("/api/v1/down", QHttpServerRequest::Method::Post, [this] { return handlePostDown(); });
    m_server.route("/api/v1/ping", QHttpServerRequest::Method::Get, [this] { return handleGetPing(); });
}

QHttpServerResponse HttpApi::handlePostUp()
{
    auto service = m_service.lock();
    if (!service) {
        qWarning() << "HTTP API: proxy backend is not initialized";
        return makeServiceUnavailableResponse(true);
    }

    const bool started = service->startXray();
    if (!started) {
        qWarning() << "Failed to start Xray via HTTP API";
    }

    QJsonObject response;
    response["status"] = started ? "ok" : "error";
    response["proxyPort"] = proxyPortValue(service->activePort());
    return QHttpServerResponse(response);
}

QHttpServerResponse HttpApi::handlePostDown()
{
    auto service = m_service.lock();
    if (!service) {
        qWarning() << "HTTP API: proxy backend is not initialized";
        return makeServiceUnavailableResponse(false);
    }

    const bool stopped = service->stopXray();
    if (!stopped) {
        qWarning() << "Failed to stop Xray via HTTP API";
    }

    QJsonObject response;
    response["status"] = stopped ? "ok" : "error";
    return QHttpServerResponse(response);
}

QHttpServerResponse HttpApi::handleGetPing() const
{
    auto service = m_service.lock();
    if (!service) {
        qWarning() << "HTTP API: proxy backend is not initialized";
        return makeServiceUnavailableResponse(true);
    }

    QJsonObject response;
    response["status"] = "ok";
    response["proxyPort"] = proxyPortValue(service->activePort());
    return QHttpServerResponse(response);
}
