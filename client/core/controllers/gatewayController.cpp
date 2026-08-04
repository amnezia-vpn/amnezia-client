#include "gatewayController.h"

#include <cstdlib>
#include <cstring>
#include <map>
#include <mutex>
#include <string>
#include <vector>

#include <QDebug>
#include <QEventLoop>
#include <QJsonDocument>
#include <QPromise>
#include <QSharedPointer>
#include <QStringList>
#include <QThread>

#include <amnezia/gateway_sdk/c_abi.h>

#include "core/repositories/secureAppSettingsRepository.h"
#include "core/utils/constants/apiKeys.h"
#include "core/utils/networkUtilities.h"

#ifdef Q_OS_IOS
    #include "platforms/ios/ios_controller.h"
#endif

#ifdef AMNEZIA_DESKTOP
    #include "core/utils/ipcClient.h"
#endif

namespace
{
    amnezia::ErrorCode mapError(int error)
    {
        if (error == AMNEZIA_GATEWAY_API_CANCELLED) {
            return amnezia::ErrorCode::ApiConfigTimeoutError;
        }
        return static_cast<amnezia::ErrorCode>(error);
    }

    std::vector<std::string> splitEndpointList(const QString &value)
    {
        std::vector<std::string> out;
        const QStringList parts = value.split(", ", Qt::SkipEmptyParts);
        for (const QString &p : parts) {
            out.push_back(p.toStdString());
        }
        return out;
    }

    class WorkingProxyCache
    {
    public:
        std::string get(const std::string &key)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto it = m_store.find(key);
            return it == m_store.end() ? std::string() : it->second;
        }
        void set(const std::string &key, const std::string &value)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_store[key] = value;
        }

    private:
        std::mutex m_mutex;
        std::map<std::string, std::string> m_store;
    };

    WorkingProxyCache &workingProxyCache()
    {
        static WorkingProxyCache cache;
        return cache;
    }

    bool isWorkingProxyKey(const std::string &key)
    {
        return key.rfind("working_proxy_", 0) == 0;
    }

    char *dupC(const std::string &s)
    {
        char *buf = static_cast<char *>(std::malloc(s.size() + 1));
        if (buf != nullptr) {
            std::memcpy(buf, s.data(), s.size());
            buf[s.size()] = '\0';
        }
        return buf;
    }

    struct ClientCtx
    {
        bool strictKillSwitch = false;
        SecureAppSettingsRepository *appSettings = nullptr;
    };

    char *readCacheTramp(const char *key, void *ud)
    {
        const std::string k = key ? key : "";
        if (isWorkingProxyKey(k)) {
            const std::string v = workingProxyCache().get(k);
            return v.empty() ? nullptr : dupC(v);
        }
        auto *ctx = static_cast<ClientCtx *>(ud);
        if (ctx == nullptr || ctx->appSettings == nullptr) {
            return nullptr;
        }
        const QByteArray v = ctx->appSettings->readGatewayProxyUrls(QString::fromStdString(k));
        return v.isEmpty() ? nullptr : dupC(std::string(v.constData(), static_cast<std::size_t>(v.size())));
    }

    void writeCacheTramp(const char *key, const char *value, void *ud)
    {
        const std::string k = key ? key : "";
        if (isWorkingProxyKey(k)) {
            workingProxyCache().set(k, value ? value : "");
            return;
        }
        auto *ctx = static_cast<ClientCtx *>(ud);
        if (ctx != nullptr && ctx->appSettings != nullptr) {
            ctx->appSettings->writeGatewayProxyUrls(QString::fromStdString(k), QByteArray(value ? value : ""));
        }
    }

    void logTramp(int level, const char *message, void *)
    {
        const QString msg = QString::fromUtf8(message ? message : "");
        if (level >= 2) {
            qWarning() << "[amnezia-gateway-sdk]" << msg;
        } else {
            qDebug() << "[amnezia-gateway-sdk]" << msg;
        }
    }

    void onBeforeReqTramp(const char *hostC, void *ud)
    {
        auto *ctx = static_cast<ClientCtx *>(ud);
        const QString host = QString::fromUtf8(hostC ? hostC : "");
        (void)host;
        (void)ctx;
#ifdef Q_OS_IOS
        IosController::Instance()->requestInetAccess();
        QThread::msleep(10);
#endif
#ifdef AMNEZIA_DESKTOP
        if (ctx != nullptr && ctx->strictKillSwitch) {
            const QString ip = NetworkUtilities::getIPAddress(host);
            if (!ip.isEmpty()) {
                IpcClient::withInterface([&](QSharedPointer<IpcInterfaceReplica> iface) {
                    QRemoteObjectPendingReply<bool> reply = iface->addKillSwitchAllowedRange(QStringList { ip });
                    if (!reply.waitForFinished(1000) || !reply.returnValue()) {
                        qWarning() << "GatewayController: addKillSwitchAllowedRange failed";
                    }
                });
            }
        }
#endif
    }

    std::shared_ptr<amnezia_gateway_api_client> makeClient(const QString &gatewayEndpoint, bool isDevEnvironment,
                                                           int requestTimeoutMsecs, bool isStrictKillSwitchEnabled,
                                                           SecureAppSettingsRepository *appSettings)
    {
        const std::string endpoint = gatewayEndpoint.toStdString();

        const QByteArray pem = isDevEnvironment ? DEV_AGW_PUBLIC_KEY : PROD_AGW_PUBLIC_KEY;
        const std::string pemStd(pem.constData(), static_cast<std::size_t>(pem.size()));

        std::vector<std::string> primary;
        std::vector<std::string> fallback;
        if (isDevEnvironment) {
            primary = splitEndpointList(QString(DEV_S3_ENDPOINT));
        } else {
            primary = splitEndpointList(QString(PROD_S3_ENDPOINT));
            fallback = splitEndpointList(QString(FALLBACK_S3_ENDPOINT));
        }
        std::vector<const char *> primaryPtrs;
        for (const auto &s : primary) {
            primaryPtrs.push_back(s.c_str());
        }
        std::vector<const char *> fallbackPtrs;
        for (const auto &s : fallback) {
            fallbackPtrs.push_back(s.c_str());
        }

        auto *ctx = new ClientCtx{ isStrictKillSwitchEnabled, appSettings };

        amnezia_gateway_sdk_config c{};
        c.gateway_endpoint = endpoint.c_str();
        c.amnezia_gateway_sdk_public_key_pem = pemStd.c_str();
        c.s3_primary_endpoints = primaryPtrs.empty() ? nullptr : primaryPtrs.data();
        c.s3_primary_count = primaryPtrs.size();
        c.s3_fallback_endpoints = fallbackPtrs.empty() ? nullptr : fallbackPtrs.data();
        c.s3_fallback_count = fallbackPtrs.size();
        c.is_dev_environment = isDevEnvironment ? 1 : 0;
        c.request_timeout_msecs = requestTimeoutMsecs;
        c.on_before_request = &onBeforeReqTramp;
        c.on_before_request_user_data = ctx;
        c.log = &logTramp;
        c.read_cache = &readCacheTramp;
        c.write_cache = &writeCacheTramp;
        c.cache_user_data = ctx;

        amnezia_gateway_api_client *raw = amnezia_gateway_sdk_client_create(&c);
        return std::shared_ptr<amnezia_gateway_api_client>(raw, [ctx](amnezia_gateway_api_client *p) {
            amnezia_gateway_sdk_client_destroy(p);
            delete ctx;
        });
    }

    struct PostSink
    {
        QEventLoop *loop;
        QObject *context;
        amnezia_gateway_sdk_response *out;
    };

    void postSyncCallback(amnezia_gateway_sdk_response r, void *ud)
    {
        auto *sink = static_cast<PostSink *>(ud);
        *sink->out = r;
        QMetaObject::invokeMethod(sink->context, [sink]() { sink->loop->quit(); }, Qt::QueuedConnection);
    }

    struct AsyncHolder
    {
        QSharedPointer<QPromise<QPair<amnezia::ErrorCode, QByteArray>>> promise;
    };

    void postAsyncCallback(amnezia_gateway_sdk_response r, void *ud)
    {
        auto *holder = static_cast<AsyncHolder *>(ud);
        const amnezia::ErrorCode ec = mapError(r.error);
        const QByteArray body = r.body ? QByteArray(r.body, static_cast<int>(r.body_len)) : QByteArray();
        amnezia_gateway_sdk_response_free(&r);

        QMetaObject::invokeMethod(
                qApp,
                [holder, ec, body]() {
                    holder->promise->addResult(qMakePair(ec, body));
                    holder->promise->finish();
                    delete holder;
                },
                Qt::QueuedConnection);
    }
}

GatewayController::GatewayController(const QString &gatewayEndpoint, const bool isDevEnvironment, const int requestTimeoutMsecs,
                                     const bool isStrictKillSwitchEnabled, SecureAppSettingsRepository *appSettingsRepository,
                                     QObject *parent)
    : QObject(parent),
      m_controller(makeClient(gatewayEndpoint, isDevEnvironment, requestTimeoutMsecs, isStrictKillSwitchEnabled, appSettingsRepository))
{
}

amnezia::ErrorCode GatewayController::post(const QString &endpoint, const QJsonObject apiPayload, QByteArray &responseBody)
{
    const std::string payload = QJsonDocument(apiPayload).toJson().toStdString();
    const std::string serviceType = apiPayload.value(apiDefs::key::serviceType).toString().toStdString();
    const std::string userCountryCode = apiPayload.value(apiDefs::key::userCountryCode).toString().toStdString();
    const std::string endpointStd = endpoint.toStdString();

    QEventLoop loop;
    QObject context;
    amnezia_gateway_sdk_response result{};
    PostSink sink{ &loop, &context, &result };

    amnezia_gateway_sdk_client_post_async(m_controller.get(), endpointStd.c_str(), payload.c_str(), serviceType.c_str(),
                                          userCountryCode.c_str(), &postSyncCallback, &sink, nullptr);

    loop.exec(QEventLoop::ExcludeUserInputEvents);

    responseBody = result.body ? QByteArray(result.body, static_cast<int>(result.body_len)) : QByteArray();
    const amnezia::ErrorCode ec = mapError(result.error);
    amnezia_gateway_sdk_response_free(&result);
    return ec;
}

QFuture<QPair<amnezia::ErrorCode, QByteArray>> GatewayController::postAsync(const QString &endpoint, const QJsonObject apiPayload)
{
    auto promise = QSharedPointer<QPromise<QPair<amnezia::ErrorCode, QByteArray>>>::create();
    promise->start();
    QFuture<QPair<amnezia::ErrorCode, QByteArray>> future = promise->future();

    const std::string payload = QJsonDocument(apiPayload).toJson().toStdString();
    const std::string serviceType = apiPayload.value(apiDefs::key::serviceType).toString().toStdString();
    const std::string userCountryCode = apiPayload.value(apiDefs::key::userCountryCode).toString().toStdString();
    const std::string endpointStd = endpoint.toStdString();

    auto *holder = new AsyncHolder{ promise };
    amnezia_gateway_sdk_client_post_async(m_controller.get(), endpointStd.c_str(), payload.c_str(), serviceType.c_str(),
                                          userCountryCode.c_str(), &postAsyncCallback, holder, nullptr);

    return future;
}
