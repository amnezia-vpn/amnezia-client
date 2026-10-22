#include "gatewayController.h"

#include <QCoreApplication>
#include <QDebug>
#include <QEventLoop>
#include <QFutureWatcher>
#include <QHash>
#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMutex>
#include <QThread>
#include <QThreadPool>
#include <QTimer>
#include <QUrl>
#include <QtConcurrent/QtConcurrentRun>

#include <condition_variable>
#include <deque>
#include <functional>
#include <future>
#include <mutex>
#include <thread>
#include <type_traits>

#include "core/repositories/secureAppSettingsRepository.h"
#include "core/utils/constants/apiConstants.h"
#include "core/utils/constants/apiKeys.h"
#include "core/utils/api/apiUtils.h"

#ifdef AMNEZIA_DESKTOP
    #include <QHostInfo>

    #include <memory>

    #include "core/utils/ipcClient.h"
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
        constexpr QLatin1String dnsTimeoutMsecs("dns_timeout_msecs");
        constexpr QLatin1String totalTimeoutMsecs("total_timeout_msecs");
    }

    namespace agwOptionsKey
    {
        constexpr QLatin1String serviceType("service_type");
        constexpr QLatin1String userCountryCode("user_country_code");
    }

    namespace agwStateKey
    {
        constexpr QLatin1String workingProxy("working_proxy");
        constexpr QLatin1String hosts("hosts");
        constexpr QLatin1String addrs("addrs");
    }

    constexpr QLatin1String agwStateCacheKey("agw_state");
    constexpr QLatin1String agwTracePrefix("agw trace:");
    constexpr QLatin1String agwTraceFailoverMarker("suspicious");
    constexpr QLatin1String endpointListSeparator(", ");
    constexpr QLatin1String legacyEndpointPlaceholder("%1");

    constexpr int shutdownWaitMsecs = 2000;
    constexpr int lastKnownDnsWaitMsecs = 300;

    class AgwCallThread
    {
    public:
        static AgwCallThread &instance()
        {
            static AgwCallThread thread;
            return thread;
        }

        void run(const std::function<void()> &task)
        {
            std::promise<void> done;
            std::future<void> finished = done.get_future();
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_tasks.push_back([&task, &done]() {
                    task();
                    done.set_value();
                });
            }
            m_wakeUp.notify_one();
            finished.wait();
        }

        ~AgwCallThread()
        {
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_stopping = true;
            }
            m_wakeUp.notify_one();
            m_thread.join();
        }

    private:
        AgwCallThread() : m_thread([this]() { loop(); })
        {
        }

        void loop()
        {
            for (;;) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(m_mutex);
                    m_wakeUp.wait(lock, [this]() { return m_stopping || !m_tasks.empty(); });
                    if (m_tasks.empty()) {
                        return;
                    }
                    task = std::move(m_tasks.front());
                    m_tasks.pop_front();
                }
                task();
            }
        }

        std::mutex m_mutex;
        std::condition_variable m_wakeUp;
        std::deque<std::function<void()>> m_tasks;
        bool m_stopping = false;
        std::thread m_thread;
    };

    bool isMainThread()
    {
        QCoreApplication *app = QCoreApplication::instance();
        return app != nullptr && QThread::currentThread() == app->thread();
    }

    template <typename F>
    auto agwCall(F &&fn) -> decltype(fn())
    {
        if (!isMainThread()) {
            return fn();
        }
        using Result = decltype(fn());
        if constexpr (std::is_void_v<Result>) {
            AgwCallThread::instance().run([&fn]() { fn(); });
        } else {
            Result result {};
            AgwCallThread::instance().run([&fn, &result]() { result = fn(); });
            return result;
        }
    }

    QMutex &activeRequestsMutex()
    {
        static QMutex mutex;
        return mutex;
    }

    QHash<const GatewayController *, agw_cancel_handle> &activeRequests()
    {
        static QHash<const GatewayController *, agw_cancel_handle> requests;
        return requests;
    }

    int cancelActiveRequests()
    {
        QMutexLocker locker(&activeRequestsMutex());
        for (const agw_cancel_handle cancel : std::as_const(activeRequests())) {
            agwCall([cancel]() { agw_cancel_cancel(cancel); });
        }
        return activeRequests().size();
    }

    void installShutdownHook()
    {
        static std::once_flag installed;
        QCoreApplication *app = QCoreApplication::instance();
        if (app == nullptr) {
            return;
        }
        std::call_once(installed, [app]() {
            QObject::connect(app, &QCoreApplication::aboutToQuit, app, []() {
                const int cancelled = cancelActiveRequests();
                qInfo() << "GatewayController: cancelled" << cancelled << "gateway clients on shutdown";
                QThreadPool::globalInstance()->waitForDone(shutdownWaitMsecs);
            });
        });
    }

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
    m_cancel = agwCall([]() { return agw_cancel_create(); });
    {
        QMutexLocker locker(&activeRequestsMutex());
        activeRequests().insert(this, m_cancel);
    }
    installShutdownHook();

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
    config[agwConfigKey::dnsTimeoutMsecs] = apiDefs::dnsTimeoutMsecs;
    config[agwConfigKey::totalTimeoutMsecs] = apiDefs::totalRequestTimeoutMsecs;

    const QByteArray configJson = QJsonDocument(config).toJson(QJsonDocument::Compact);
    m_client = agwCall([&configJson]() { return agw_client_create(configJson.constData(), nullptr); });
    if (m_client == 0) {
        qCritical() << "GatewayController: failed to create gateway client (missing key or endpoint?)";
        return;
    }

    if (m_appSettingsRepository != nullptr) {
        const QByteArray state = m_appSettingsRepository->readGatewayProxyUrls(agwStateCacheKey);
        if (!state.isEmpty() && agwCall([this, &state]() { return agw_import_state(m_client, state.constData()); }) == AGW_OK) {
            m_lastPersistedState = state;
        }
    }
}

GatewayController::~GatewayController()
{
    {
        QMutexLocker locker(&activeRequestsMutex());
        activeRequests().remove(this);
    }
    if (!m_inFlight.isFinished()) {
        qInfo() << "GatewayController: destroyed while a request is in flight, cancelling it";
        agwCall([this]() { agw_cancel_cancel(m_cancel); });
        m_inFlight.waitForFinished();
    }
    agwCall([this]() {
        agw_client_destroy(m_client);
        agw_cancel_destroy(m_cancel);
    });
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
    if (!future.isFinished()) {
        agwCall([this]() { agw_cancel_cancel(m_cancel); });
    }

    const QPair<amnezia::ErrorCode, QByteArray> result = future.result();
    responseBody = result.second;
    return result.first;
}

QFuture<QPair<amnezia::ErrorCode, QByteArray>> GatewayController::postAsync(const QString &endpoint, const QJsonObject apiPayload)
{
    prepareRequest();
    m_inFlight = QtConcurrent::run(
            [this, endpoint, apiPayload]() -> QPair<amnezia::ErrorCode, QByteArray> { return executePost(endpoint, apiPayload); });
    return m_inFlight;
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

    agw_result result = agw_post(m_client, path.toUtf8().constData(), payload.constData(), optionsJson.constData(), m_cancel);

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
    if (code == AGW_CANCELLED) {
        qInfo().noquote() << "GatewayController: request" << path << "cancelled, failover state is not saved";
    } else {
        persistState();
    }

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
    case AGW_ERR_NETWORK: return amnezia::ErrorCode::ApiGatewayUnreachableError;
    default: return amnezia::ErrorCode::ApiConfigDownloadError;
    }
}

#ifdef AMNEZIA_DESKTOP
namespace
{
    QMutex &knownHostAddressesMutex()
    {
        static QMutex mutex;
        return mutex;
    }

    QHash<QString, QStringList> &knownHostAddresses()
    {
        static QHash<QString, QStringList> addresses;
        return addresses;
    }

    QStringList knownAddresses(const QString &host)
    {
        QMutexLocker locker(&knownHostAddressesMutex());
        return knownHostAddresses().value(host);
    }

    QStringList rememberAddresses(const QString &host, const QHostInfo &info)
    {
        QStringList ips;
        for (const QHostAddress &address : info.addresses()) {
            if (address.protocol() == QAbstractSocket::IPv4Protocol) {
                ips.append(address.toString());
            }
        }
        ips.removeDuplicates();
        if (!ips.isEmpty()) {
            QMutexLocker locker(&knownHostAddressesMutex());
            knownHostAddresses().insert(host, ips);
        }
        return ips;
    }

    struct HostLookupWait
    {
        QObject context;
        QEventLoop loop;
        QStringList ips;
        int pending = 0;
    };
}
#endif

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
    QHash<QString, QStringList> lastKnown;
    const QStringList hosts = requestHosts(&lastKnown);

    auto wait = std::make_shared<HostLookupWait>();
    std::weak_ptr<HostLookupWait> weakWait = wait;
    QStringList ips;
    bool fullDnsWait = false;
    for (const QString &host : hosts) {
        const QHostAddress literal(host);
        if (literal.protocol() == QAbstractSocket::IPv4Protocol) {
            ips.append(host);
            continue;
        }
        if (literal.protocol() == QAbstractSocket::IPv6Protocol) {
            continue;
        }
        const QStringList remembered = lastKnown.value(host);
        ips.append(remembered);
        const QStringList known = knownAddresses(host);
        if (!known.isEmpty()) {
            ips.append(known);
            QHostInfo::lookupHost(host, QCoreApplication::instance(), [host](const QHostInfo &info) { rememberAddresses(host, info); });
            continue;
        }
        if (remembered.isEmpty()) {
            fullDnsWait = true;
        } else {
            QHostInfo::lookupHost(host, QCoreApplication::instance(), [host](const QHostInfo &info) { rememberAddresses(host, info); });
        }
        ++wait->pending;
        QHostInfo::lookupHost(host, &wait->context, [host, weakWait](const QHostInfo &info) {
            const QStringList resolved = rememberAddresses(host, info);
            if (auto state = weakWait.lock()) {
                state->ips.append(resolved);
                if (--state->pending == 0) {
                    state->loop.quit();
                }
            }
        });
    }

    const int dnsWaitMsecs = fullDnsWait ? apiDefs::dnsTimeoutMsecs : lastKnownDnsWaitMsecs;
    if (wait->pending > 0 && !QCoreApplication::closingDown()) {
        QTimer::singleShot(dnsWaitMsecs, &wait->loop, &QEventLoop::quit);
        connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, &wait->loop, &QEventLoop::quit);
        wait->loop.exec(QEventLoop::ExcludeUserInputEvents);
    }
    ips.append(wait->ips);
    ips.removeDuplicates();

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

QStringList GatewayController::requestHosts(QHash<QString, QStringList> *lastKnown) const
{
    QStringList hosts;
    QJsonObject rememberedHosts;
    if (m_client != 0) {
        char *state = agwCall([this]() { return agw_export_state(m_client); });
        if (state != nullptr) {
            const QJsonObject stateObject = QJsonDocument::fromJson(QByteArray(state)).object();
            agwCall([state]() { agw_string_free(state); });
            const QString workingProxy = stateObject.value(agwStateKey::workingProxy).toString();
            if (!workingProxy.isEmpty()) {
                hosts.append(QUrl(workingProxy).host());
            }
            rememberedHosts = stateObject.value(agwStateKey::hosts).toObject();
        }
    }
    hosts.append(QUrl(m_gatewayEndpoint).host());
    hosts.removeAll(QString());
    hosts.removeDuplicates();

    if (lastKnown != nullptr) {
        for (const QString &host : hosts) {
            QStringList ips;
            for (const QJsonValue &addr : rememberedHosts.value(host).toObject().value(agwStateKey::addrs).toArray()) {
                const QHostAddress address(addr.toString());
                if (address.protocol() == QAbstractSocket::IPv4Protocol) {
                    ips.append(address.toString());
                }
            }
            if (!ips.isEmpty()) {
                lastKnown->insert(host, ips);
            }
        }
    }
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
