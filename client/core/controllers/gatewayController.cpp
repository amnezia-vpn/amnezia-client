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

#ifdef AMNEZIA_DESKTOP
    #include "core/utils/ipcClient.h"
    #include "core/utils/networkUtilities.h"
#endif

#ifdef Q_OS_IOS
    #include "platforms/ios/ios_controller.h"
#endif

namespace
{
    // Key under which the library's failover caches (working proxy + proxy
    // lists) are persisted in the secure settings repository.
    const QString agwStateCacheKey = QStringLiteral("agw_state_v1");

    QStringList splitEndpoints(const char *raw)
    {
        return QString::fromUtf8(raw).split(", ", Qt::SkipEmptyParts);
    }

    void agwLogCallback(int level, const char *message, void *userData)
    {
        Q_UNUSED(userData);
        switch (level) {
        case AGW_LOG_ERROR: qCritical().noquote() << "agw:" << message; break;
        case AGW_LOG_WARNING: qWarning().noquote() << "agw:" << message; break;
        case AGW_LOG_INFO: qInfo().noquote() << "agw:" << message; break;
        default: qDebug().noquote() << "agw:" << message; break;
        }
    }

    // Called by the library (from a worker thread) right before every network
    // attempt: direct gateway, storage objects, health checks and proxies.
    void agwBeforeRequestCallback(const char *host, void *userData)
    {
        auto *controller = static_cast<GatewayController *>(userData);
        const QString hostString = QString::fromUtf8(host);
        if (QThread::currentThread() == controller->thread()) {
            controller->handleBeforeRequest(hostString);
        } else {
            // Blocking: the killswitch exception must exist before the
            // request proceeds. The controller's thread is either pumping the
            // sync-post event loop or running the application loop.
            QMetaObject::invokeMethod(
                    controller, [controller, hostString]() { controller->handleBeforeRequest(hostString); },
                    Qt::BlockingQueuedConnection);
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
    config[QStringLiteral("gateway_endpoint")] = gatewayEndpoint;
    config[QStringLiteral("public_key_pem")] = QString::fromUtf8(publicKey);
    config[QStringLiteral("s3_primary_endpoints")] = QJsonArray::fromStringList(primaryEndpoints);
    config[QStringLiteral("s3_fallback_endpoints")] = QJsonArray::fromStringList(fallbackEndpoints);
    config[QStringLiteral("is_dev_environment")] = isDevEnvironment;
    config[QStringLiteral("request_timeout_msecs")] = requestTimeoutMsecs;

    agw_callbacks callbacks {};
    callbacks.struct_size = sizeof(agw_callbacks);
    callbacks.log = &agwLogCallback;
    callbacks.on_before_request = &agwBeforeRequestCallback;
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

    // Same waiting semantics as the historical implementation: pump a local
    // event loop so the (typically UI) calling thread stays serviced.
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

    // Call sites pass the historical "%1v1/..." templates; the library takes
    // a path relative to the gateway base.
    QString path = endpoint;
    path.remove(QLatin1String("%1"));

    QJsonObject options;
    options[apiDefs::key::serviceType] = apiPayload.value(apiDefs::key::serviceType).toString("");
    options[apiDefs::key::userCountryCode] = apiPayload.value(apiDefs::key::userCountryCode).toString("");

    const QByteArray payload = QJsonDocument(apiPayload).toJson(QJsonDocument::Compact);
    const QByteArray optionsJson = QJsonDocument(options).toJson(QJsonDocument::Compact);

    agw_result result = agw_post(m_client, path.toUtf8().constData(), payload.constData(), optionsJson.constData(), 0);

    QByteArray responseBody;
    if (result.body != nullptr) {
        responseBody = QByteArray(result.body, static_cast<qsizetype>(result.body_len));
    }
    const int code = result.code;
    agw_result_free(&result);

    // Persist the failover caches on the controller's thread; the repository
    // is not assumed to be thread-safe.
    QMetaObject::invokeMethod(this, [this]() { persistState(); }, Qt::QueuedConnection);

    return qMakePair(code == AGW_OK ? amnezia::ErrorCode::NoError : static_cast<amnezia::ErrorCode>(code), responseBody);
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
