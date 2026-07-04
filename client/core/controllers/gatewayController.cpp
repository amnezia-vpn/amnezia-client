#include "gatewayController.h"

#include <map>
#include <mutex>
#include <string>
#include <vector>

#include <QDebug>
#include <QEventLoop>
#include <QJsonDocument>
#include <QPointer>
#include <QPromise>
#include <QSharedPointer>
#include <QStringList>
#include <QThread>

#include <amnezia/gateway_sdk/gateway_client.h>
#include <amnezia/gateway_sdk/config.h>
#include <amnezia/gateway_sdk/types.h>

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
    amnezia::ErrorCode mapError(amnezia::gateway_sdk::ErrorCode error)
    {
        if (error == amnezia::gateway_sdk::ErrorCode::Cancelled) {
            return amnezia::ErrorCode::ApiConfigTimeoutError;
        }
        return static_cast<amnezia::ErrorCode>(static_cast<int>(error));
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

    amnezia::gateway_sdk::Config makeConfig(const QString &gatewayEndpoint, bool isDevEnvironment, int requestTimeoutMsecs,
                           bool isStrictKillSwitchEnabled)
    {
        amnezia::gateway_sdk::Config cfg;
        cfg.gatewayEndpoint = gatewayEndpoint.toStdString();

        const QByteArray pem = isDevEnvironment ? DEV_AGW_PUBLIC_KEY : PROD_AGW_PUBLIC_KEY;
        cfg.agwPublicKeyPem = std::string(pem.constData(), static_cast<std::size_t>(pem.size()));

        if (isDevEnvironment) {
            cfg.s3PrimaryEndpoints = splitEndpointList(QString(DEV_S3_ENDPOINT));
        } else {
            cfg.s3PrimaryEndpoints = splitEndpointList(QString(PROD_S3_ENDPOINT));
            cfg.s3FallbackEndpoints = splitEndpointList(QString(FALLBACK_S3_ENDPOINT));
        }

        cfg.isDevEnvironment = isDevEnvironment;
        cfg.requestTimeoutMsecs = requestTimeoutMsecs;

        cfg.readCache = [](const std::string &key) -> std::string {
            if (isWorkingProxyKey(key)) {
                return workingProxyCache().get(key);
            }
            return {};
        };
        cfg.writeCache = [](const std::string &key, const std::string &value) {
            if (isWorkingProxyKey(key)) {
                workingProxyCache().set(key, value);
            }
        };

        cfg.log = [](amnezia::gateway_sdk::LogLevel level, const std::string &message) {
            const QString msg = QString::fromStdString(message);
            switch (level) {
            case amnezia::gateway_sdk::LogLevel::Error: qWarning() << "[amnezia-gateway-sdk]" << msg; break;
            case amnezia::gateway_sdk::LogLevel::Warning: qWarning() << "[amnezia-gateway-sdk]" << msg; break;
            default: qDebug() << "[amnezia-gateway-sdk]" << msg; break;
            }
        };

        cfg.onBeforeRequest = [isStrictKillSwitchEnabled](const std::string &hostStd) {
            const QString host = QString::fromStdString(hostStd);
            (void)host;
            (void)isStrictKillSwitchEnabled;
#ifdef Q_OS_IOS
            IosController::Instance()->requestInetAccess();
            QThread::msleep(10);
#endif
#ifdef AMNEZIA_DESKTOP
            if (isStrictKillSwitchEnabled) {
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
        };

        return cfg;
    }

    std::shared_ptr<amnezia::gateway_sdk::GatewayClient> getClientForEnv(const QString &gatewayEndpoint, bool isDevEnvironment,
                                                        int requestTimeoutMsecs, bool isStrictKillSwitchEnabled)
    {
        static std::mutex mutex;
        static std::map<std::string, std::shared_ptr<amnezia::gateway_sdk::GatewayClient>> clients;

        const std::string key = gatewayEndpoint.toStdString() + "|" + (isDevEnvironment ? "1" : "0") + "|"
                + std::to_string(requestTimeoutMsecs) + "|" + (isStrictKillSwitchEnabled ? "1" : "0");

        std::lock_guard<std::mutex> lock(mutex);
        auto it = clients.find(key);
        if (it != clients.end()) {
            return it->second;
        }
        auto client = std::make_shared<amnezia::gateway_sdk::GatewayClient>(
                makeConfig(gatewayEndpoint, isDevEnvironment, requestTimeoutMsecs, isStrictKillSwitchEnabled));
        clients.emplace(key, client);
        return client;
    }
}

GatewayController::GatewayController(const QString &gatewayEndpoint, const bool isDevEnvironment, const int requestTimeoutMsecs,
                                     const bool isStrictKillSwitchEnabled, QObject *parent)
    : QObject(parent),
      m_controller(getClientForEnv(gatewayEndpoint, isDevEnvironment, requestTimeoutMsecs, isStrictKillSwitchEnabled))
{
}

amnezia::ErrorCode GatewayController::post(const QString &endpoint, const QJsonObject apiPayload, QByteArray &responseBody)
{
    const std::string payload = QJsonDocument(apiPayload).toJson().toStdString();
    const std::string serviceType = apiPayload.value(apiDefs::key::serviceType).toString().toStdString();
    const std::string userCountryCode = apiPayload.value(apiDefs::key::userCountryCode).toString().toStdString();

    QEventLoop loop;
    QObject context;
    amnezia::gateway_sdk::Response result;

    m_controller->postAsync(
            endpoint.toStdString(), payload,
            [&loop, &context, &result](amnezia::gateway_sdk::Response r) {
                QMetaObject::invokeMethod(
                        &context,
                        [&loop, &result, r]() {
                            result = r;
                            loop.quit();
                        },
                        Qt::QueuedConnection);
            },
            amnezia::gateway_sdk::FailoverContext { serviceType, userCountryCode });

    loop.exec(QEventLoop::ExcludeUserInputEvents);

    responseBody = QByteArray::fromStdString(result.body);
    const amnezia::ErrorCode ec = mapError(result.error);
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

    QPointer<GatewayController> self(this);

    m_controller->postAsync(
            endpoint.toStdString(), payload,
            [promise, self](amnezia::gateway_sdk::Response r) {
                const amnezia::ErrorCode ec = mapError(r.error);
                const QByteArray body = QByteArray::fromStdString(r.body);

                auto deliver = [promise, ec, body]() {
                    promise->addResult(qMakePair(ec, body));
                    promise->finish();
                };
                if (self) {
                    QMetaObject::invokeMethod(self.data(), deliver, Qt::QueuedConnection);
                } else {
                    deliver();
                }
            },
            amnezia::gateway_sdk::FailoverContext { serviceType, userCountryCode });

    return future;
}
