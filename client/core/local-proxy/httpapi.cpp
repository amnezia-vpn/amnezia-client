#include "httpapi.h"
#include "proxylogger.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QHostAddress>
#include <optional>

namespace {

std::optional<int> extractInboundPort(const QJsonObject &config)
{
    if (!config.contains("inbounds") || !config["inbounds"].isArray()) {
        return std::nullopt;
    }

    const QJsonArray inbounds = config["inbounds"].toArray();
    if (inbounds.isEmpty() || !inbounds.at(0).isObject()) {
        return std::nullopt;
    }

    const QJsonObject firstInbound = inbounds.at(0).toObject();
    if (!firstInbound.contains("port")) {
        return std::nullopt;
    }

    return firstInbound.value("port").toInt();
}

QJsonValue proxyPortValue(const std::optional<int> &port)
{
    if (port.has_value()) {
        return QJsonValue(*port);
    }
    return QJsonValue::Null;
}

QHttpServerResponse makeServiceUnavailablePingResponse()
{
    QJsonObject payload{
        {"status", "error"},
        {"proxyPort", QJsonValue::Null}
    };
    return QHttpServerResponse(payload, QHttpServerResponse::StatusCode::ServiceUnavailable);
}

QHttpServerResponse makeServiceUnavailableStatusResponse()
{
    QJsonObject payload{
        {"status", "error"}
    };
    return QHttpServerResponse(payload, QHttpServerResponse::StatusCode::ServiceUnavailable);
}

} // namespace

HttpApi::HttpApi(QWeakPointer<IProxyService> service, QObject* parent)
    : QObject(parent)
    , m_tcpServer(new QTcpServer(this))
    , m_service(service)
{
    ProxyLogger::getInstance().debug("HttpApi initialized");
}

HttpApi::~HttpApi()
{
    stop();
}

bool HttpApi::start(quint16 port)
{
    ProxyLogger::getInstance().info(QString("Starting HTTP API server on port %1").arg(port));
    
    if (!m_tcpServer->listen(QHostAddress::LocalHost, port)) {
        ProxyLogger::getInstance().error(QString("Failed to start HTTP API server on port %1").arg(port));
        return false;
    }

    setupRoutes();
    m_server.bind(m_tcpServer.data());
    
    ProxyLogger::getInstance().info(QString("HTTP API server is running on localhost:%1").arg(m_tcpServer->serverPort()));
    ProxyLogger::getInstance().debug("Available endpoints:\n"
                              "  POST   /api/v1/up\n"
                              "  POST   /api/v1/down\n"
                              "  GET    /api/v1/ping");
    return true;
}

void HttpApi::stop()
{
    ProxyLogger::getInstance().info("Stopping HTTP API server");
    if (m_tcpServer) {
        m_tcpServer->close();
    }
}

void HttpApi::setupRoutes()
{
    ProxyLogger::getInstance().debug("Setting up HTTP API routes");

    m_server.route("/api/v1/up", QHttpServerRequest::Method::Post,
        [this] {
            ProxyLogger::getInstance().debug("Handling POST /api/v1/up request");
            return handlePostUp();
        });

    m_server.route("/api/v1/down", QHttpServerRequest::Method::Post,
        [this] {
            ProxyLogger::getInstance().debug("Handling POST /api/v1/down request");
            return handlePostDown();
        });

    m_server.route("/api/v1/ping", QHttpServerRequest::Method::Get,
        [this] {
            ProxyLogger::getInstance().debug("Handling GET /api/v1/ping request");
            return handleGetPing();
        });
}

QHttpServerResponse HttpApi::handlePostUp()
{
    if (auto service = m_service.lock()) {
        const bool started = service->startXray();
        QJsonObject response;
        response["status"] = started ? "ok" : "error";

        const auto port = started ? extractInboundPort(service->getConfig()) : std::optional<int>{};
        response["proxyPort"] = proxyPortValue(port);

        if (started) {
            if (port.has_value()) {
                ProxyLogger::getInstance().info(QString("Xray process started on port %1").arg(*port));
            } else {
                ProxyLogger::getInstance().warning("Xray started but inbound port is unknown (local proxy owner may be missing)");
            }
        } else {
            ProxyLogger::getInstance().warning("Failed to start Xray process via HTTP API");
        }

        return QHttpServerResponse(response);
    }

    ProxyLogger::getInstance().error("Service unavailable: proxy backend is not initialized");
    return makeServiceUnavailablePingResponse();
}

QHttpServerResponse HttpApi::handlePostDown()
{
    if (auto service = m_service.lock()) {
        const bool stopped = service->stopXray();
        QJsonObject response;
        response["status"] = stopped ? "ok" : "error";
        if (!stopped) {
            ProxyLogger::getInstance().warning("Failed to stop Xray process via HTTP API");
        } else {
            ProxyLogger::getInstance().info("Xray process stopped via HTTP API");
        }

        return QHttpServerResponse(response);
    }

    ProxyLogger::getInstance().error("Service unavailable: proxy backend is not initialized");
    return makeServiceUnavailableStatusResponse();
}

QHttpServerResponse HttpApi::handleGetPing() const
{
    if (auto service = m_service.lock()) {
        QJsonObject response;
        response["status"] = "ok";
        const bool isRunning = service->isXrayRunning();
        if (isRunning) {
            const auto port = extractInboundPort(service->getConfig());
            response["proxyPort"] = proxyPortValue(port);
            if (port.has_value()) {
                ProxyLogger::getInstance().debug(QString("Xray port: %1").arg(*port));
            } else {
                ProxyLogger::getInstance().warning("Unable to detect inbound port while Xray is running");
            }
        } else {
            response["proxyPort"] = QJsonValue::Null;
            ProxyLogger::getInstance().debug("Xray is not running");
        }

        return QHttpServerResponse(response);
    }

    ProxyLogger::getInstance().error("Service unavailable: proxy backend is not initialized");
    return makeServiceUnavailablePingResponse();
}
