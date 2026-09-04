#include "httpApi.h"

#include "localProxyDefs.h"
#include "proxyService.h"

#include <optional>

#include <QHostAddress>
#include <QJsonArray>
#include <QJsonObject>

namespace
{
    std::optional<int> extractInboundPort(const QJsonObject &config)
    {
        const QJsonArray inbounds = config.value(QLatin1String("inbounds")).toArray();
        if (inbounds.isEmpty() || !inbounds.at(0).isObject()) {
            return std::nullopt;
        }

        const QJsonObject firstInbound = inbounds.at(0).toObject();
        if (!firstInbound.contains(QLatin1String("port"))) {
            return std::nullopt;
        }

        return firstInbound.value(QLatin1String("port")).toInt();
    }

    QJsonValue proxyPortValue(const std::optional<int> &port)
    {
        return port ? QJsonValue(*port) : QJsonValue::Null;
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
        qCWarning(lcLocalProxy) << "Failed to start HTTP API server on port" << port;
        return false;
    }

    setupRoutes();
    m_server.bind(m_tcpServer.data());

    qCInfo(lcLocalProxy) << "HTTP API server is running on localhost:" << m_tcpServer->serverPort();
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
        qCWarning(lcLocalProxy) << "HTTP API: proxy backend is not initialized";
        return makeServiceUnavailableResponse(true);
    }

    const bool started = service->startXray();
    const auto port = started ? extractInboundPort(service->config()) : std::optional<int> {};
    if (!started) {
        qCWarning(lcLocalProxy) << "Failed to start Xray via HTTP API";
    }

    QJsonObject response;
    response["status"] = started ? "ok" : "error";
    response["proxyPort"] = proxyPortValue(port);
    return QHttpServerResponse(response);
}

QHttpServerResponse HttpApi::handlePostDown()
{
    auto service = m_service.lock();
    if (!service) {
        qCWarning(lcLocalProxy) << "HTTP API: proxy backend is not initialized";
        return makeServiceUnavailableResponse(false);
    }

    const bool stopped = service->stopXray();
    if (!stopped) {
        qCWarning(lcLocalProxy) << "Failed to stop Xray via HTTP API";
    }

    QJsonObject response;
    response["status"] = stopped ? "ok" : "error";
    return QHttpServerResponse(response);
}

QHttpServerResponse HttpApi::handleGetPing() const
{
    auto service = m_service.lock();
    if (!service) {
        qCWarning(lcLocalProxy) << "HTTP API: proxy backend is not initialized";
        return makeServiceUnavailableResponse(true);
    }

    const auto port = service->isXrayRunning() ? extractInboundPort(service->config()) : std::optional<int> {};

    QJsonObject response;
    response["status"] = "ok";
    response["proxyPort"] = proxyPortValue(port);
    return QHttpServerResponse(response);
}
