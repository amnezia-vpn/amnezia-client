#include "localProxyController.h"

#include "core/controllers/gatewayController.h"
#include "core/repositories/secureAppSettingsRepository.h"
#include "core/repositories/secureServersRepository.h"
#include "core/utils/api/apiUtils.h"
#include "core/utils/constants/apiConstants.h"
#include "core/utils/constants/apiKeys.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/localProxy/localProxyConstants.h"
#include "core/utils/localProxy/portUtils.h"
#include "core/utils/localProxy/xrayConfigUtils.h"
#include "version.h"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QSysInfo>
#include <QUuid>

#ifdef AMNEZIA_DESKTOP
    #include "core/utils/ipcClient.h"

    #include <QHostAddress>
    #include <QHttpServer>
    #include <QHttpServerResponse>
    #include <QTcpServer>
#endif

using namespace amnezia;

LocalProxyController::LocalProxyController(SecureServersRepository *serversRepository,
                                           SecureAppSettingsRepository *appSettingsRepository, QObject *parent)
    : QObject(parent), m_serversRepository(serversRepository), m_appSettingsRepository(appSettingsRepository)
{
    connect(m_appSettingsRepository, &SecureAppSettingsRepository::localProxySettingsChanged, this,
            &LocalProxyController::stateChanged);
    connect(m_appSettingsRepository, &SecureAppSettingsRepository::localProxySettingsChanged, this,
            &LocalProxyController::applySettings);
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
    return m_appSettingsRepository->isLocalProxyHttpEnabled();
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

    if (m_appSettingsRepository->localProxyPort() != static_cast<quint16>(port)) {
        m_appSettingsRepository->setLocalProxyPort(static_cast<quint16>(port));
    }
    m_appSettingsRepository->setLocalProxyPortUserDefined(true);
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

    if (m_appSettingsRepository->isLocalProxyHttpEnabled() && m_appSettingsRepository->localProxyOwnerId() != ownerId) {
        return false;
    }

    if (!portUtils::isPortAvailable(port) && m_appSettingsRepository->isLocalProxyPortUserDefined()) {
        return false;
    }

    if (m_appSettingsRepository->localProxyPort() != static_cast<quint16>(port)) {
        m_appSettingsRepository->setLocalProxyPort(static_cast<quint16>(port));
    }

    m_appSettingsRepository->setLocalProxyOwnerId(ownerId);
    m_appSettingsRepository->setLocalProxyHttpEnabled(true);
    return true;
}

void LocalProxyController::disable()
{
    if (m_appSettingsRepository->isLocalProxyHttpEnabled()) {
        m_appSettingsRepository->setLocalProxyHttpEnabled(false);
    }
}

void LocalProxyController::applySettings()
{
    if (!m_appSettingsRepository->isLocalProxyHttpEnabled()) {
        qDebug() << "Local proxy is disabled";
        stop();
        return;
    }

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
    m_appSettingsRepository->setLocalProxyHttpEnabled(false);
}

void LocalProxyController::onVpnConnectionRequested()
{
    if (!m_appSettingsRepository->isLocalProxyHttpEnabled()) {
        return;
    }

    m_appSettingsRepository->setLocalProxyHttpEnabled(false);
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

    if (m_appSettingsRepository->isLocalProxyHttpEnabled()) {
        m_appSettingsRepository->setLocalProxyHttpEnabled(false);
    } else {
        stop();
    }

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
        serializedConfig = fetchXrayConfigFromGateway(*ownerServer);
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
    int port = m_appSettingsRepository->localProxyPort();
    if (port < localProxy::proxyPortMin || port > localProxy::proxyPortMax) {
        port = localProxy::defaultProxyPort;
    }

    if (portUtils::waitForPort(port, localProxy::portReleaseWaitMs)) {
        return port;
    }

    if (m_appSettingsRepository->isLocalProxyPortUserDefined()) {
        qWarning() << "Local proxy port" << port << "is already in use";
        return std::nullopt;
    }

    std::optional<int> freePort;
    if (port != localProxy::defaultProxyPort && portUtils::isPortAvailable(localProxy::defaultProxyPort)) {
        freePort = localProxy::defaultProxyPort;
    } else {
        freePort = portUtils::findFirstAvailablePort(localProxy::defaultProxyPort + 1, localProxy::proxyPortMax);
    }

    if (!freePort) {
        qWarning() << "No available local proxy port in range" << localProxy::defaultProxyPort + 1 << "-"
                   << localProxy::proxyPortMax;
        return std::nullopt;
    }

    qDebug() << "Local proxy port" << port << "is busy, using" << *freePort;
    return freePort;
}

std::optional<QString> LocalProxyController::fetchXrayConfigFromGateway(const QJsonObject &serverConfig) const
{
    const QJsonObject apiConfig = serverConfig.value(apiDefs::key::apiConfig).toObject();
    if (apiConfig.isEmpty()) {
        qWarning() << "Server API config is missing";
        return std::nullopt;
    }

    const QString userCountryCode = apiConfig.value(apiDefs::key::userCountryCode).toString();
    const QString serviceType = apiConfig.value(apiDefs::key::serviceType).toString();
    if (userCountryCode.isEmpty() || serviceType.isEmpty()) {
        qWarning() << "Server API config lacks service identifiers";
        return std::nullopt;
    }

    QJsonObject apiPayload;
    apiPayload[apiDefs::key::osVersion] = QSysInfo::productType();
    apiPayload[apiDefs::key::appVersion] = QString(APP_VERSION);

    const QString appLanguage = m_appSettingsRepository->getAppLanguage().name().split("_").first();
    if (!appLanguage.isEmpty()) {
        apiPayload[apiDefs::key::appLanguage] = appLanguage;
    }

    apiPayload[apiDefs::key::uuid] = m_appSettingsRepository->getInstallationUuid(true);
    apiPayload[apiDefs::key::userCountryCode] = userCountryCode;
    apiPayload[apiDefs::key::serviceType] = serviceType;
    apiPayload[apiDefs::key::serviceProtocol] = QStringLiteral("vless");
    apiPayload[apiDefs::key::publicKey] = QUuid::createUuid().toString(QUuid::WithoutBraces);

    const QJsonObject authData = serverConfig.value(apiDefs::key::authData).toObject();
    if (!authData.isEmpty()) {
        apiPayload[apiDefs::key::authData] = authData;
    }

    GatewayController gatewayController(m_appSettingsRepository->getGatewayEndpoint(), m_appSettingsRepository->isDevGatewayEnv(),
                                        apiDefs::requestTimeoutMsecs, m_appSettingsRepository->isStrictKillSwitchEnabled(),
                                        m_appSettingsRepository);

    QByteArray responseBody;
    const ErrorCode errorCode = gatewayController.post(QString("%1v1/config"), apiPayload, responseBody);
    if (errorCode != ErrorCode::NoError) {
        qWarning() << "Gateway request failed with error code" << static_cast<int>(errorCode);
        return std::nullopt;
    }

    QJsonParseError responseError;
    const QJsonDocument responseDoc = QJsonDocument::fromJson(responseBody, &responseError);
    if (responseError.error != QJsonParseError::NoError || !responseDoc.isObject()) {
        qWarning() << "Failed to parse gateway response:" << responseError.errorString();
        return std::nullopt;
    }

    QString data = responseDoc.object().value(configKey::config).toString();
    if (data.isEmpty()) {
        qWarning() << "Gateway response lacks config payload";
        return std::nullopt;
    }

    data.replace("vpn://", "");
    QByteArray decoded = QByteArray::fromBase64(data.toUtf8(), QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
    if (decoded.isEmpty()) {
        qWarning() << "Gateway config payload is empty";
        return std::nullopt;
    }

    const QByteArray uncompressed = qUncompress(decoded);
    if (!uncompressed.isEmpty()) {
        decoded = uncompressed;
    }

    QJsonParseError configError;
    const QJsonDocument configDoc = QJsonDocument::fromJson(decoded, &configError);
    if (configError.error != QJsonParseError::NoError || !configDoc.isObject()) {
        qWarning() << "Failed to parse gateway config JSON:" << configError.errorString();
        return std::nullopt;
    }

    const auto serializedConfig = xrayConfigUtils::extractLastConfig(configDoc.object());
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
