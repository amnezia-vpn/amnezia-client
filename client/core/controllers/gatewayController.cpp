#include "gatewayController.h"

#include <QDebug>
#include <QEventLoop>
#include <QFutureWatcher>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMetaObject>
#include <QThread>
#include <QtConcurrent/QtConcurrentRun>

#include "core/repositories/secureAppSettingsRepository.h"
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
    }

    namespace agwOptionsKey
    {
        constexpr QLatin1String serviceType("service_type");
        constexpr QLatin1String userCountryCode("user_country_code");
    }

    constexpr QLatin1String agwStateCacheKey("agw_state");
    constexpr QLatin1String agwLogPrefix("agw:");
    constexpr QLatin1String endpointListSeparator(", ");
    constexpr QLatin1String legacyEndpointPlaceholder("%1");

    QStringList splitEndpoints(const char *raw)
    {
        return QString::fromUtf8(raw).split(endpointListSeparator, Qt::SkipEmptyParts);
    }

    void agwLogCallback(int level, const char *message, void *userData)
    {
        Q_UNUSED(userData);
        switch (level) {
        case AGW_LOG_ERROR: qCritical().noquote() << agwLogPrefix << message; break;
        case AGW_LOG_WARNING: qWarning().noquote() << agwLogPrefix << message; break;
        case AGW_LOG_INFO: qInfo().noquote() << agwLogPrefix << message; break;
        default: qDebug().noquote() << agwLogPrefix << message; break;
        }
    }
}

GatewayController::GatewayController(const QString &gatewayEndpoint, const bool isDevEnvironment, const int requestTimeoutMsecs,
                                     const bool isStrictKillSwitchEnabled, SecureAppSettingsRepository *appSettingsRepository,
                                     QObject *parent)
    : QObject(parent),
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

    agw_callbacks callbacks {};
    callbacks.struct_size = sizeof(agw_callbacks);
    callbacks.log = &agwLogCallback;
    callbacks.on_before_request = &GatewayController::onBeforeRequest;
    callbacks.on_before_request_user_data = this;

    m_client = agw_client_create(QJsonDocument(config).toJson(QJsonDocument::Compact).constData(), &callbacks);
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
    const int code = result.code;
    agw_result_free(&result);

    QMetaObject::invokeMethod(this, [this]() { persistState(); }, Qt::QueuedConnection);

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

void GatewayController::onBeforeRequest(const char *host, void *userData)
{
    auto *controller = static_cast<GatewayController *>(userData);
    const QString hostString = QString::fromUtf8(host);
    if (QThread::currentThread() == controller->thread()) {
        controller->handleBeforeRequest(hostString);
    } else {
        QMetaObject::invokeMethod(
                controller, [controller, hostString]() { controller->handleBeforeRequest(hostString); },
                Qt::BlockingQueuedConnection);
    }
}

void GatewayController::handleBeforeRequest(const QString &host)
{
#ifdef Q_OS_IOS
    Q_UNUSED(host);
    IosController::Instance()->requestInetAccess();
    QThread::msleep(10);
#endif

#ifdef AMNEZIA_DESKTOP
    if (m_isStrictKillSwitchEnabled) {
        const QString ip = NetworkUtilities::getIPAddress(host);
        if (!ip.isEmpty()) {
            IpcClient::withInterface([&](QSharedPointer<IpcInterfaceReplica> iface) {
                QRemoteObjectPendingReply<bool> reply = iface->addKillSwitchAllowedRange(QStringList { ip });
                if (!reply.waitForFinished(1000) || !reply.returnValue()) {
                    qWarning() << "GatewayController::handleBeforeRequest(): failed to add killswitch exception for" << host;
                }
            });
        }
    }
#else
    Q_UNUSED(host);
#endif
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
