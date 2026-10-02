#include "gatewayController.h"

#include <QCryptographicHash>
#include <QDebug>
#include <QEventLoop>
#include <QFutureWatcher>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMetaObject>
#include <QPromise>
#include <QThread>
#include <QThreadPool>

#include <future>
#include <memory>
#include <utility>

#include "core/repositories/secureAppSettingsRepository.h"
#include "core/utils/constants/apiKeys.h"
#include "core/utils/api/apiUtils.h"
#include "cryptoUtils.h"

#ifdef AMNEZIA_DESKTOP
    #include "core/utils/ipcClient.h"
    #include "core/utils/networkUtilities.h"
#endif

#ifdef Q_OS_IOS
    #include "platforms/ios/ios_controller.h"
#endif

#include "include/embedded_agw_public_keys.h"

namespace
{
    // Key under which the library's failover caches (working proxy + proxy
    // lists) are persisted in the secure settings repository.
    QString agwStateCacheKey(const QString &gatewayEndpoint, const bool isDevEnvironment)
    {
        const QByteArray endpointHash =
                QCryptographicHash::hash(gatewayEndpoint.toUtf8(), QCryptographicHash::Sha1).toHex().left(8);
        return QStringLiteral("agw_state_v1_%1_%2")
                .arg(isDevEnvironment ? QStringLiteral("dev") : QStringLiteral("prod"), QString::fromLatin1(endpointHash));
    }

    const QString kWorkingProxy = QStringLiteral("working_proxy");
    const QString kProxyLists = QStringLiteral("proxy_lists");
    const QString kVersion = QStringLiteral("version");

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

    thread_local bool t_onAgwControlThread = false;

    QThreadPool *agwControlPool()
    {
        static QThreadPool *pool = [] {
            auto *p = new QThreadPool;
            p->setObjectName(QStringLiteral("agw-control"));
            p->setMaxThreadCount(1);
            p->setExpiryTimeout(-1);
            return p;
        }();
        return pool;
    }

    QThreadPool *agwRequestPool()
    {
        static QThreadPool *pool = [] {
            auto *p = new QThreadPool;
            p->setObjectName(QStringLiteral("agw-requests"));
            p->setMaxThreadCount(8);
            return p;
        }();
        return pool;
    }

    template <typename F>
    auto runOnAgwControlThread(F &&f) -> decltype(f())
    {
        if (t_onAgwControlThread) {
            return f();
        }
        using Result = decltype(f());
        std::packaged_task<Result()> task(std::forward<F>(f));
        std::future<Result> result = task.get_future();
        agwControlPool()->start([&task]() {
            t_onAgwControlThread = true;
            task();
        });
        return result.get();
    }

    QHash<QString, QJsonObject> &agwStateCache()
    {
        static auto *cache = new QHash<QString, QJsonObject>;
        return *cache;
    }

    QJsonObject parseAgwState(const QByteArray &blob)
    {
        const QJsonDocument doc = QJsonDocument::fromJson(blob);
        return doc.isObject() ? doc.object() : QJsonObject();
    }

    QByteArray serializeAgwState(const QJsonObject &state)
    {
        return QJsonDocument(state).toJson(QJsonDocument::Compact);
    }

    QJsonObject withoutWorkingProxy(QJsonObject state)
    {
        state.remove(kWorkingProxy);
        return state;
    }

    QStringList decodeLegacyProxyList(const QByteArray &payload, const bool isDevEnvironment, const QByteArray &publicKey)
    {
        QByteArray plain;
        if (isDevEnvironment) {
            plain = payload;
        } else {
            if (payload.trimmed().startsWith('[')) {
                return {};
            }
            QByteArray pem = publicKey;
            while (!pem.isEmpty() && (pem.back() == ' ' || pem.back() == '\t' || pem.back() == '\r' || pem.back() == '\n')) {
                pem.chop(1);
            }
            const QByteArray hash = QCryptographicHash::hash(pem, QCryptographicHash::Sha512).toHex();
            plain = CryptoUtils::decryptAes256Cbc(QByteArray::fromBase64(payload), QByteArray::fromHex(hash.left(64)),
                                                  QByteArray::fromHex(hash.mid(64, 32)));
        }

        const QJsonDocument doc = QJsonDocument::fromJson(plain);
        QStringList urls;
        for (const QJsonValue &value : doc.array()) {
            const QString url = value.toString();
            if (url.startsWith(QLatin1String("https://")) || url.startsWith(QLatin1String("http://"))) {
                urls.append(url);
            }
        }
        return urls;
    }

    QJsonObject migrateLegacyProxyLists(SecureAppSettingsRepository *repository, const bool isDevEnvironment, const QByteArray &publicKey)
    {
        QJsonObject lists;
        for (const QString &cacheKey : repository->legacyGatewayProxyListKeys()) {
            const QStringList urls = decodeLegacyProxyList(repository->readLegacyGatewayProxyList(cacheKey), isDevEnvironment, publicKey);
            if (!urls.isEmpty()) {
                lists.insert(cacheKey, QJsonArray::fromStringList(urls));
            }
        }
        return lists;
    }

    QJsonObject loadAgwState(SecureAppSettingsRepository *repository, const QString &stateKey, const bool isDevEnvironment,
                             const QByteArray &publicKey)
    {
        auto &cache = agwStateCache();
        const auto cached = cache.constFind(stateKey);
        if (cached != cache.constEnd()) {
            return cached.value();
        }

        QJsonObject state = withoutWorkingProxy(parseAgwState(repository->readGatewayProxyUrls(stateKey)));
        if (state.value(kProxyLists).toObject().isEmpty()) {
            const QJsonObject migrated = migrateLegacyProxyLists(repository, isDevEnvironment, publicKey);
            if (!migrated.isEmpty()) {
                state.insert(kVersion, 1);
                state.insert(kProxyLists, migrated);
                repository->writeGatewayProxyUrls(stateKey, serializeAgwState(state));
                for (const QString &cacheKey : migrated.keys()) {
                    repository->removeLegacyGatewayProxyList(cacheKey);
                }
                qInfo().noquote() << "GatewayController: migrated" << migrated.size() << "pre-libagw proxy lists into" << stateKey;
            }
        }
        repository->removeLegacyGatewayProxyList(QStringLiteral("agw_state_v1"));

        cache.insert(stateKey, state);
        return state;
    }

    QJsonObject mergeAgwState(const QJsonObject &base, const QJsonObject &exported, const QJsonObject &current)
    {
        QJsonObject merged = current;
        merged.insert(kVersion, exported.value(kVersion).toInt(1));

        const QString exportedProxy = exported.value(kWorkingProxy).toString();
        if (exportedProxy != base.value(kWorkingProxy).toString()) {
            if (exportedProxy.isEmpty()) {
                merged.remove(kWorkingProxy);
            } else {
                merged.insert(kWorkingProxy, exportedProxy);
            }
        }

        const QJsonObject baseLists = base.value(kProxyLists).toObject();
        const QJsonObject exportedLists = exported.value(kProxyLists).toObject();
        QJsonObject mergedLists = merged.value(kProxyLists).toObject();
        for (auto it = exportedLists.constBegin(); it != exportedLists.constEnd(); ++it) {
            if (baseLists.value(it.key()) != it.value()) {
                mergedLists.insert(it.key(), it.value());
            }
        }
        if (!mergedLists.isEmpty()) {
            merged.insert(kProxyLists, mergedLists);
        }
        return merged;
    }
}

GatewayController::GatewayController(const QString &gatewayEndpoint, const bool isDevEnvironment, const int requestTimeoutMsecs,
                                     const bool isStrictKillSwitchEnabled, SecureAppSettingsRepository *appSettingsRepository,
                                     QObject *parent)
    : QObject(parent),
      m_isStrictKillSwitchEnabled(isStrictKillSwitchEnabled),
      m_appSettingsRepository(appSettingsRepository),
      m_stateKey(agwStateCacheKey(gatewayEndpoint, isDevEnvironment))
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

    const QByteArray configJson = QJsonDocument(config).toJson(QJsonDocument::Compact);
    m_client = runOnAgwControlThread([&configJson, &callbacks]() { return agw_client_create(configJson.constData(), &callbacks); });
    if (m_client == 0) {
        qCritical() << "GatewayController: failed to create gateway client (missing key or endpoint?)";
        return;
    }

    if (m_appSettingsRepository != nullptr) {
        const QJsonObject state = loadAgwState(m_appSettingsRepository, m_stateKey, isDevEnvironment, publicKey);
        if (!state.isEmpty()) {
            const QByteArray blob = serializeAgwState(state);
            const int importResult =
                    runOnAgwControlThread([client = m_client, &blob]() { return agw_import_state(client, blob.constData()); });
            if (importResult == AGW_OK) {
                m_baseState = state;
            }
        }
    }
}

GatewayController::~GatewayController()
{
    runOnAgwControlThread([client = m_client]() { agw_client_destroy(client); });
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
    auto promise = std::make_shared<QPromise<QPair<amnezia::ErrorCode, QByteArray>>>();
    QFuture<QPair<amnezia::ErrorCode, QByteArray>> future = promise->future();
    promise->start();
    agwRequestPool()->start([this, promise, endpoint, apiPayload]() {
        promise->addResult(executePost(endpoint, apiPayload));
        promise->finish();
    });
    return future;
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
    const QByteArray blob = runOnAgwControlThread([client = m_client]() -> QByteArray {
        char *state = agw_export_state(client);
        if (state == nullptr) {
            return {};
        }
        const QByteArray copy(state);
        agw_string_free(state);
        return copy;
    });
    const QJsonObject exported = parseAgwState(blob);
    if (exported.isEmpty()) {
        return;
    }

    auto &cache = agwStateCache();
    const QJsonObject current = cache.value(m_stateKey, m_baseState);
    const QJsonObject merged = mergeAgwState(m_baseState, exported, current);
    m_baseState = exported;
    if (merged == current) {
        return;
    }
    cache.insert(m_stateKey, merged);

    const QByteArray stored = serializeAgwState(withoutWorkingProxy(merged));
    if (stored != m_appSettingsRepository->readGatewayProxyUrls(m_stateKey)) {
        m_appSettingsRepository->writeGatewayProxyUrls(m_stateKey, stored);
    }
}
