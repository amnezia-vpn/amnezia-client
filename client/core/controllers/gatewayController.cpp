#include "gatewayController.h"

#include <QDebug>
#include <QEventLoop>
#include <QFutureWatcher>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QThread>
#include <QUrl>
#include <QtConcurrent/QtConcurrentRun>

#include "core/repositories/secureAppSettingsRepository.h"
#include "core/utils/constants/apiConstants.h"
#include "core/utils/constants/apiKeys.h"
#include "core/utils/api/apiUtils.h"

#ifdef AMNEZIA_DESKTOP
    #include "core/utils/ipcClient.h"
    #include "core/utils/networkUtilities.h"
#endif

#ifdef Q_OS_IOS
    #include "platforms/ios/ios_controller.h"
#endif

namespace
{
    namespace agwConfigKey
    {
        constexpr QLatin1String gatewayEndpoint("gateway_endpoint");
        constexpr QLatin1String publicKeyPem("public_key_pem");
        constexpr QLatin1String s3PrimaryEndpoints("s3_primary_endpoints");
        constexpr QLatin1String s3FallbackEndpoints("s3_fallback_endpoints");
        constexpr QLatin1String isDevEnvironment("is_dev_environment");
        constexpr QLatin1String requestTimeoutMsecs("request_timeout_msecs");
        constexpr QLatin1String proxyRequestTimeoutMsecs("proxy_request_timeout_msecs");
        constexpr QLatin1String proxyStorageTimeoutMsecs("proxy_storage_timeout_msecs");
        constexpr QLatin1String proxyHealthTimeoutMsecs("proxy_health_timeout_msecs");
    }

    namespace agwOptionsKey
    {
        constexpr QLatin1String serviceType("service_type");
        constexpr QLatin1String userCountryCode("user_country_code");
    }

    namespace agwStateKey
    {
        constexpr QLatin1String workingProxy("working_proxy");
    }

    constexpr QLatin1String agwStateCacheKey("agw_state");
    constexpr QLatin1String agwTracePrefix("agw trace:");
    constexpr QLatin1String agwTraceFailoverMarker("suspicious");
    constexpr QLatin1String endpointListSeparator(", ");
    constexpr QLatin1String legacyEndpointPlaceholder("%1");

    QStringList splitEndpoints(const char *raw)
    {
        return QString::fromUtf8(raw).split(endpointListSeparator, Qt::SkipEmptyParts);
    }

    void logAgwTrace(const QString &path, const int code, const QString &trace)
    {
        if (trace.isEmpty()) {
            return;
        }
        const QString text = QStringLiteral("%1 %2\n%3").arg(agwTracePrefix, path, trace).replace(QLatin1Char('\n'), QStringLiteral("\n    "));
        if (code != AGW_OK) {
            qWarning().noquote() << text;
        } else if (trace.contains(agwTraceFailoverMarker)) {
            qInfo().noquote() << text;
        } else {
            qDebug().noquote() << text;
        }
    }
}

GatewayController::GatewayController(const QString &gatewayEndpoint, const bool isDevEnvironment, const int requestTimeoutMsecs,
                                     const bool isStrictKillSwitchEnabled, SecureAppSettingsRepository *appSettingsRepository,
                                     QObject *parent)
    : QObject(parent),
      m_gatewayEndpoint(gatewayEndpoint),
      m_isStrictKillSwitchEnabled(isStrictKillSwitchEnabled),
      m_appSettingsRepository(appSettingsRepository)
{
    const QByteArray publicKey = isDevEnvironment ? QByteArray(DEV_AGW_PUBLIC_KEY) : QByteArray(PROD_AGW_PUBLIC_KEY);
    m_publicKeyMissing = publicKey.isEmpty();

    QStringList primaryEndpoints;
    QStringList fallbackEndpoints;
    if (isDevEnvironment) {
        primaryEndpoints = splitEndpoints(DEV_S3_ENDPOINT);
    } else {
        primaryEndpoints = splitEndpoints(PROD_S3_ENDPOINT);
        fallbackEndpoints = splitEndpoints(FALLBACK_S3_ENDPOINT);
    }

    QJsonObject config;
    config[agwConfigKey::gatewayEndpoint] = gatewayEndpoint;
    config[agwConfigKey::publicKeyPem] = QString::fromUtf8(publicKey);
    config[agwConfigKey::s3PrimaryEndpoints] = QJsonArray::fromStringList(primaryEndpoints);
    config[agwConfigKey::s3FallbackEndpoints] = QJsonArray::fromStringList(fallbackEndpoints);
    config[agwConfigKey::isDevEnvironment] = isDevEnvironment;
    config[agwConfigKey::requestTimeoutMsecs] = requestTimeoutMsecs;
    config[agwConfigKey::proxyRequestTimeoutMsecs] = apiDefs::proxyRequestTimeoutMsecs;
    config[agwConfigKey::proxyStorageTimeoutMsecs] = apiDefs::proxyStorageTimeoutMsecs;
    config[agwConfigKey::proxyHealthTimeoutMsecs] = apiDefs::proxyHealthTimeoutMsecs;

    m_client = agw_client_create(QJsonDocument(config).toJson(QJsonDocument::Compact).constData(), nullptr);
    if (m_client == 0) {
        qCritical() << "GatewayController: failed to create gateway client (missing key or endpoint?)";
        return;
    }

    if (m_appSettingsRepository != nullptr) {
        const QByteArray state = m_appSettingsRepository->readGatewayProxyUrls(agwStateCacheKey);
        if (!state.isEmpty() && agw_import_state(m_client, state.constData()) == AGW_OK) {
            m_lastPersistedState = state;
        }
    }
}

GatewayController::~GatewayController()
{
    agw_client_destroy(m_client);
}

amnezia::ErrorCode GatewayController::post(const QString &endpoint, const QJsonObject apiPayload, QByteArray &responseBody)
{
    QFuture<QPair<amnezia::ErrorCode, QByteArray>> future = postAsync(endpoint, apiPayload);

    QFutureWatcher<QPair<amnezia::ErrorCode, QByteArray>> watcher;
    QEventLoop wait;
    connect(&watcher, &QFutureWatcherBase::finished, &wait, &QEventLoop::quit);
    watcher.setFuture(future);
    if (!future.isFinished()) {
        wait.exec(QEventLoop::ExcludeUserInputEvents);
    }

    const QPair<amnezia::ErrorCode, QByteArray> result = future.result();
    responseBody = result.second;
    return result.first;
}

QFuture<QPair<amnezia::ErrorCode, QByteArray>> GatewayController::postAsync(const QString &endpoint, const QJsonObject apiPayload)
{
    prepareRequest();
    return QtConcurrent::run(
            [this, endpoint, apiPayload]() -> QPair<amnezia::ErrorCode, QByteArray> { return executePost(endpoint, apiPayload); });
}

QPair<amnezia::ErrorCode, QByteArray> GatewayController::executePost(const QString &endpoint, const QJsonObject &apiPayload)
{
    if (m_client == 0) {
        return qMakePair(m_publicKeyMissing ? amnezia::ErrorCode::ApiMissingAgwPublicKey : amnezia::ErrorCode::ApiConfigDownloadError,
                         QByteArray());
    }

    QString path = endpoint;
    path.remove(legacyEndpointPlaceholder);

    QJsonObject options;
    options[agwOptionsKey::serviceType] = apiPayload.value(apiDefs::key::serviceType).toString();
    options[agwOptionsKey::userCountryCode] = apiPayload.value(apiDefs::key::userCountryCode).toString();

    const QByteArray payload = QJsonDocument(apiPayload).toJson(QJsonDocument::Compact);
    const QByteArray optionsJson = QJsonDocument(options).toJson(QJsonDocument::Compact);

    agw_result result = agw_post(m_client, path.toUtf8().constData(), payload.constData(), optionsJson.constData(), 0);

    QByteArray responseBody;
    if (result.body != nullptr) {
        responseBody = QByteArray(result.body, static_cast<qsizetype>(result.body_len));
    }
    QString trace;
    if (result.trace != nullptr) {
        trace = QString::fromUtf8(result.trace, static_cast<qsizetype>(result.trace_len));
    }
    const int code = result.code;
    agw_result_free(&result);

    logAgwTrace(path, code, trace);
    persistState();

    return qMakePair(mapResultCode(code, responseBody), responseBody);
}

amnezia::ErrorCode GatewayController::mapResultCode(const int code, const QByteArray &responseBody)
{
    switch (code) {
    case AGW_OK: return apiUtils::checkApiResponseErrors(responseBody);
    case AGW_CANCELLED:
    case AGW_ERR_TIMEOUT: return amnezia::ErrorCode::ApiConfigTimeoutError;
    case AGW_ERR_SSL: return amnezia::ErrorCode::ApiConfigSslError;
    case AGW_ERR_CONFIG: return amnezia::ErrorCode::ApiMissingAgwPublicKey;
    case AGW_ERR_DECRYPT: return amnezia::ErrorCode::ApiConfigDecryptionError;
    default: return amnezia::ErrorCode::ApiConfigDownloadError;
    }
}

void GatewayController::prepareRequest()
{
#ifdef Q_OS_IOS
    IosController::Instance()->requestInetAccess();
    QThread::msleep(10);
#endif

#ifdef AMNEZIA_DESKTOP
    if (!m_isStrictKillSwitchEnabled) {
        return;
    }
    QStringList ips;
    for (const QString &host : requestHosts()) {
        const QString ip = NetworkUtilities::getIPAddress(host);
        if (!ip.isEmpty()) {
            ips.append(ip);
        }
    }
    if (ips.isEmpty()) {
        return;
    }
    IpcClient::withInterface([&](QSharedPointer<IpcInterfaceReplica> iface) {
        QRemoteObjectPendingReply<bool> reply = iface->addKillSwitchAllowedRange(ips);
        if (!reply.waitForFinished(1000) || !reply.returnValue()) {
            qWarning() << "GatewayController::prepareRequest(): Failed to execute remote addKillSwitchAllowedRange call";
        }
    });
#endif
}

QStringList GatewayController::requestHosts() const
{
    QStringList hosts;
    if (m_client != 0) {
        char *state = agw_export_state(m_client);
        if (state != nullptr) {
            const QString workingProxy = QJsonDocument::fromJson(QByteArray(state)).object().value(agwStateKey::workingProxy).toString();
            agw_string_free(state);
            if (!workingProxy.isEmpty()) {
                hosts.append(QUrl(workingProxy).host());
            }
        }
    }
    hosts.append(QUrl(m_gatewayEndpoint).host());
    hosts.removeAll(QString());
    hosts.removeDuplicates();
    return hosts;
}

void GatewayController::persistState()
{
    if (m_client == 0 || m_appSettingsRepository == nullptr) {
        return;
    }
    char *state = agw_export_state(m_client);
    if (state == nullptr) {
        return;
    }
    const QByteArray blob(state);
    agw_string_free(state);

    if (blob != m_lastPersistedState) {
        m_appSettingsRepository->writeGatewayProxyUrls(agwStateCacheKey, blob);
        m_lastPersistedState = blob;
    }
}
