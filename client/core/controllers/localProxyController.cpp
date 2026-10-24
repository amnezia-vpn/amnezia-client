#include "localProxyController.h"

#include "core/controllers/api/subscriptionController.h"
#include "core/repositories/secureAppSettingsRepository.h"
#include "core/repositories/secureServersRepository.h"
#include "core/utils/api/apiUtils.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/localProxy/localProxyConstants.h"
#include "core/utils/localProxy/portUtils.h"
#include "core/utils/localProxy/xrayConfigUtils.h"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonParseError>

#ifdef AMNEZIA_DESKTOP
    #include "core/utils/ipcClient.h"

    #include <QHostAddress>
    #include <QHttpServer>
    #include <QHttpServerResponse>
    #include <QTcpServer>
#endif

using namespace amnezia;

LocalProxyController::LocalProxyController(SecureServersRepository *serversRepository,
                                           SecureAppSettingsRepository *appSettingsRepository,
                                           SubscriptionController *subscriptionController, QObject *parent)
    : QObject(parent),
      m_serversRepository(serversRepository),
      m_appSettingsRepository(appSettingsRepository),
      m_subscriptionController(subscriptionController)
{
    connect(m_serversRepository, &SecureServersRepository::serverEdited, this, &LocalProxyController::onServerEdited);
    connect(m_serversRepository, &SecureServersRepository::serverRemoved, this,
            [this](const QString &serverId, int) { onServerRemoved(serverId); });
}

LocalProxyController::~LocalProxyController()
{
    stop();
}

bool LocalProxyController::isEnabled() const
{
    return m_isEnabled;
}

int LocalProxyController::port() const
{
    return m_appSettingsRepository->localProxyPort();
}

QString LocalProxyController::ownerId() const
{
    return m_appSettingsRepository->localProxyOwnerId();
}

bool LocalProxyController::isPortUserDefined() const
{
    return m_appSettingsRepository->isLocalProxyPortUserDefined();
}

int LocalProxyController::activePort() const
{
    return m_activePort;
}

bool LocalProxyController::setPort(int port)
{
    if (port < localProxy::proxyPortMin || port > localProxy::proxyPortMax) {
        return false;
    }

    const bool isPortChanged = m_appSettingsRepository->localProxyPort() != static_cast<quint16>(port);
    if (isPortChanged) {
        m_appSettingsRepository->setLocalProxyPort(static_cast<quint16>(port));
        m_lastProxyPort = 0;
    }
    m_appSettingsRepository->setLocalProxyPortUserDefined(true);
    emit stateChanged();

    if (isPortChanged && m_isEnabled) {
        start();
    }
    return true;
}

bool LocalProxyController::isPortBusy(int port) const
{
    return !portUtils::isPortAvailable(port);
}

bool LocalProxyController::enable(const QString &ownerId, int port)
{
    if (!isSupported() || ownerId.isEmpty() || port < localProxy::proxyPortMin || port > localProxy::proxyPortMax) {
        return false;
    }

    if (m_isEnabled && m_appSettingsRepository->localProxyOwnerId() != ownerId) {
        return false;
    }

    if (!portUtils::isPortAvailable(port) && m_appSettingsRepository->isLocalProxyPortUserDefined()) {
        return false;
    }

    if (m_appSettingsRepository->localProxyPort() != static_cast<quint16>(port)) {
        m_appSettingsRepository->setLocalProxyPort(static_cast<quint16>(port));
    }

    m_appSettingsRepository->setLocalProxyOwnerId(ownerId);
    m_isEnabled = true;
    emit stateChanged();

    start();
    return true;
}

void LocalProxyController::disable()
{
    if (!m_isEnabled) {
        return;
    }

    qDebug() << "Local proxy is disabled";
    m_isEnabled = false;
    stop();
    emit stateChanged();
}

void LocalProxyController::start()
{
    if (!startApi(localProxy::apiPort)) {
        disableWithError(tr("Local proxy failed to start. Check if the port is available"));
        return;
    }

    if (!syncXray()) {
        disableWithError(tr("Couldn’t start the proxy due to an internal error. Try restarting the app"));
        return;
    }

    qDebug() << "Local proxy is running on 127.0.0.1:" << m_apiPort;
}

void LocalProxyController::onServerEdited(const QString &serverId)
{
    if (m_apiPort == 0 || m_appSettingsRepository->localProxyOwnerId() != serverId) {
        return;
    }

    qDebug() << "Owner server edited, restarting Xray";
    if (restartXray()) {
        m_appliedProxyPort = m_appSettingsRepository->localProxyPort();
    } else {
        disableWithError(tr("Couldn’t start the proxy due to an internal error. Try restarting the app"));
    }
}

void LocalProxyController::onServerRemoved(const QString &serverId)
{
    if (m_appSettingsRepository->localProxyOwnerId() != serverId) {
        return;
    }

    m_appSettingsRepository->setLocalProxyOwnerId(QString());
    if (m_isEnabled) {
        m_isEnabled = false;
        stop();
    }
    emit stateChanged();
}

void LocalProxyController::onVpnConnectionRequested()
{
    if (!m_isEnabled) {
        return;
    }

    disable();
    emit stoppedByVpn(tr("Local proxy stopped because VPN was turned on"));
}

void LocalProxyController::stop()
{
    stopXray();
    stopApi();
    m_appliedProxyPort = 0;
}

bool LocalProxyController::syncXray()
{
    const quint16 proxyPort = m_appSettingsRepository->localProxyPort();

    bool synced;
    if (!isXrayEngineRunning()) {
        synced = startXray();
    } else if (m_appliedProxyPort != proxyPort) {
        qDebug() << "Proxy port changed from" << m_appliedProxyPort << "to" << proxyPort;
        synced = restartXray();
    } else {
        return true;
    }

    if (synced) {
        m_appliedProxyPort = proxyPort;
    }
    return synced;
}

bool LocalProxyController::startXray()
{
    if (isXrayEngineRunning()) {
        return true;
    }

    const auto config = buildXrayConfig();
    if (!config) {
        return false;
    }

    if (!startXrayEngine(config->serializedConfig)) {
        return false;
    }

    qDebug() << "Xray started on port" << config->proxyPort;
    m_lastProxyPort = config->proxyPort;
    setActivePort(config->proxyPort);
    return true;
}

bool LocalProxyController::stopXray()
{
    if (!stopXrayEngine()) {
        return false;
    }

    setActivePort(0);
    return true;
}

bool LocalProxyController::restartXray()
{
    qDebug() << "Restarting Xray with updated config";

    if (!stopXray()) {
        return false;
    }

    return startXray();
}

void LocalProxyController::disableWithError(const QString &message)
{
    qWarning() << message;

    m_isEnabled = false;
    stop();
    emit stateChanged();
    emit startFailed(message);
}

void LocalProxyController::setActivePort(int port)
{
    if (m_activePort == port) {
        return;
    }

    m_activePort = port;
    emit stateChanged();
}

int LocalProxyController::runningProxyPort() const
{
    return isXrayEngineRunning() ? m_activePort : 0;
}

std::optional<LocalProxyController::XrayConfig> LocalProxyController::buildXrayConfig() const
{
    const QString ownerId = m_appSettingsRepository->localProxyOwnerId();
    if (ownerId.isEmpty()) {
        qWarning() << "Local proxy owner server id is not configured";
        return std::nullopt;
    }

    const auto ownerServer = m_serversRepository->serverJsonById(ownerId);
    if (!ownerServer) {
        qWarning() << "Owner server with id" << ownerId << "not found";
        return std::nullopt;
    }

    if (!apiUtils::isPremiumServer(*ownerServer)) {
        qWarning() << "Server" << ownerServer->value(configKey::name).toString() << "is not premium, local proxy is unavailable";
        return std::nullopt;
    }

    auto serializedConfig = xrayConfigUtils::extractLastConfig(*ownerServer);
    if (!serializedConfig) {
        serializedConfig = fetchXrayConfigFromGateway(ownerId);
        if (!serializedConfig) {
            return std::nullopt;
        }
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(serializedConfig->toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "Failed to parse Xray config JSON:" << parseError.errorString();
        return std::nullopt;
    }

    const auto proxyPort = selectProxyPort();
    if (!proxyPort) {
        return std::nullopt;
    }

    QJsonObject xrayConfig = doc.object();
    if (!xrayConfigUtils::setInboundPort(xrayConfig, *proxyPort)) {
        qWarning() << "Failed to override local proxy inbound port, using original config";
        return XrayConfig { *serializedConfig, 0 };
    }

    return XrayConfig { QString::fromUtf8(QJsonDocument(xrayConfig).toJson(QJsonDocument::Compact)), *proxyPort };
}

std::optional<int> LocalProxyController::selectProxyPort() const
{
    int basePort = m_appSettingsRepository->localProxyPort();
    if (basePort < localProxy::proxyPortMin || basePort > localProxy::proxyPortMax) {
        basePort = localProxy::defaultProxyPort;
    }

    if (m_lastProxyPort == 0) {
        if (portUtils::waitForPort(basePort, localProxy::portReleaseWaitMs)) {
            return basePort;
        }

        if (m_appSettingsRepository->isLocalProxyPortUserDefined()) {
            qWarning() << "Local proxy port" << basePort << "is already in use";
            return std::nullopt;
        }
    }

    // Every restart moves to a new port so that clients notice the change and reconnect.
    const int from = (m_lastProxyPort > 0 ? m_lastProxyPort : basePort) + 1;
    auto freePort = portUtils::findFirstAvailablePort(from, localProxy::proxyPortMax);
    if (!freePort && from > basePort + 1) {
        freePort = portUtils::findFirstAvailablePort(basePort, from - 2);
    }

    if (!freePort) {
        qWarning() << "No available local proxy port in range" << basePort << "-" << localProxy::proxyPortMax;
        return std::nullopt;
    }

    qDebug() << "Local proxy port selected:" << *freePort << "(previous" << m_lastProxyPort << ")";
    return freePort;
}

std::optional<QString> LocalProxyController::fetchXrayConfigFromGateway(const QString &serverId) const
{
    QJsonObject serverConfig;
    const ErrorCode errorCode = m_subscriptionController->fetchServiceConfig(serverId, configKey::vless, serverConfig);
    if (errorCode != ErrorCode::NoError) {
        qWarning() << "Gateway request failed with error code" << static_cast<int>(errorCode);
        return std::nullopt;
    }

    const auto serializedConfig = xrayConfigUtils::extractLastConfig(serverConfig);
    if (!serializedConfig) {
        qWarning() << "Gateway response lacks Xray last_config payload";
        return std::nullopt;
    }

    qDebug() << "Fetched Xray config from gateway";
    return serializedConfig;
}

#ifdef AMNEZIA_DESKTOP

bool LocalProxyController::isSupported() const
{
    return true;
}

bool LocalProxyController::startApi(quint16 port)
{
    if (m_httpServer && m_apiPort == port) {
        return true;
    }

    stopApi();

    const auto proxyPortValue = [this]() {
        const int port = runningProxyPort();
        return port > 0 ? QJsonValue(port) : QJsonValue::Null;
    };

    auto httpServer = new QHttpServer(this);
    httpServer->route("/api/v1/up", QHttpServerRequest::Method::Post, [this, proxyPortValue] {
        const bool started = startXray();
        return QHttpServerResponse(QJsonObject { { "status", started ? "ok" : "error" }, { "proxyPort", proxyPortValue() } });
    });
    httpServer->route("/api/v1/down", QHttpServerRequest::Method::Post, [this] {
        const bool stopped = stopXray();
        return QHttpServerResponse(QJsonObject { { "status", stopped ? "ok" : "error" } });
    });
    httpServer->route("/api/v1/ping", QHttpServerRequest::Method::Get, [proxyPortValue] {
        return QHttpServerResponse(QJsonObject { { "status", "ok" }, { "proxyPort", proxyPortValue() } });
    });

    auto tcpServer = new QTcpServer(httpServer);
    if (!tcpServer->listen(QHostAddress::LocalHost, port) || !httpServer->bind(tcpServer)) {
        qWarning() << "Failed to start HTTP API server on port" << port;
        delete httpServer;
        return false;
    }

    m_httpServer = httpServer;
    m_apiPort = port;
    qDebug() << "HTTP API server is running on localhost:" << port;
    return true;
}

void LocalProxyController::stopApi()
{
    delete m_httpServer;
    m_httpServer = nullptr;
    m_apiPort = 0;
}

bool LocalProxyController::startXrayEngine(const QString &config)
{
    if (config.trimmed().isEmpty()) {
        qWarning() << "Xray config content is empty";
        return false;
    }

    const qint64 token = IpcClient::withInterface(
            [&](QSharedPointer<IpcInterfaceReplica> iface) -> qint64 {
                auto xrayStart = iface->xrayStartOwned(config);
                if (!xrayStart.waitForFinished()) {
                    return 0;
                }
                return xrayStart.returnValue();
            },
            []() -> qint64 { return 0; });

    if (token == 0) {
        qWarning() << "Failed to start Xray via IPC";
        return false;
    }

    m_xrayToken = token;
    return true;
}

bool LocalProxyController::stopXrayEngine()
{
    if (m_xrayToken == 0) {
        return true;
    }

    const bool stopped = IpcClient::withInterface(
            [this](QSharedPointer<IpcInterfaceReplica> iface) {
                auto xrayStop = iface->xrayStopOwned(m_xrayToken);
                return xrayStop.waitForFinished() && xrayStop.returnValue();
            },
            []() { return false; });

    if (!stopped) {
        qWarning() << "Failed to stop Xray via IPC";
        return false;
    }

    m_xrayToken = 0;
    qDebug() << "Xray stopped";
    return true;
}

bool LocalProxyController::isXrayEngineRunning() const
{
    if (m_xrayToken == 0) {
        return false;
    }

    return IpcClient::withInterface(
            [this](QSharedPointer<IpcInterfaceReplica> iface) {
                auto currentToken = iface->xrayCurrentToken();
                return currentToken.waitForFinished() && currentToken.returnValue() == m_xrayToken;
            },
            []() { return false; });
}

#else

bool LocalProxyController::isSupported() const
{
    return false;
}

bool LocalProxyController::startApi(quint16)
{
    return false;
}

void LocalProxyController::stopApi()
{
}

bool LocalProxyController::startXrayEngine(const QString &)
{
    return false;
}

bool LocalProxyController::stopXrayEngine()
{
    return true;
}

bool LocalProxyController::isXrayEngineRunning() const
{
    return false;
}

#endif
