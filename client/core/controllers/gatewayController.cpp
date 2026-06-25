#include "gatewayController.h"

#include <map>
#include <mutex>
#include <string>
#include <vector>

#include <QDebug>
#include <QJsonDocument>
#include <QPointer>
#include <QPromise>
#include <QSharedPointer>
#include <QStringList>
#include <QThread>

#include <agw/client.h>
#include <agw/config.h>
#include <agw/types.h>

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
    // agw::ErrorCode → amnezia::ErrorCode. Значения 1100–1120 и NoError=0 совпадают численно;
    // отмена SDK маппится в ApiConfigTimeoutError (паритет: оригинал так же трактовал cancel/timeout).
    amnezia::ErrorCode mapError(agw::ErrorCode error)
    {
        if (error == agw::ErrorCode::Cancelled) {
            return amnezia::ErrorCode::ApiConfigTimeoutError;
        }
        return static_cast<amnezia::ErrorCode>(static_cast<int>(error));
    }

    std::vector<std::string> splitCsv(const QString &value)
    {
        std::vector<std::string> out;
        const QStringList parts = value.split(", ", Qt::SkipEmptyParts);
        for (const QString &p : parts) {
            out.push_back(p.toStdString());
        }
        return out;
    }

    agw::Config makeConfig(const QString &gatewayEndpoint, bool isDevEnvironment, int requestTimeoutMsecs,
                           bool isStrictKillSwitchEnabled)
    {
        agw::Config cfg;
        cfg.gatewayEndpoint = gatewayEndpoint.toStdString();

        const QByteArray pem = isDevEnvironment ? DEV_AGW_PUBLIC_KEY : PROD_AGW_PUBLIC_KEY;
        cfg.agwPublicKeyPem = std::string(pem.constData(), static_cast<std::size_t>(pem.size()));

        if (isDevEnvironment) {
            cfg.s3PrimaryEndpoints = splitCsv(QString(DEV_S3_ENDPOINT));
        } else {
            cfg.s3PrimaryEndpoints = splitCsv(QString(PROD_S3_ENDPOINT));
            cfg.s3FallbackEndpoints = splitCsv(QString(FALLBACK_S3_ENDPOINT));
        }

        cfg.isDevEnvironment = isDevEnvironment;
        cfg.requestTimeoutMsecs = requestTimeoutMsecs;

        // Лог-хук SDK → Qt-логи (SDK не логирует секреты/тела). Помогает диагностике failover/крипты.
        cfg.log = [](agw::LogLevel level, const std::string &message) {
            const QString msg = QString::fromStdString(message);
            switch (level) {
            case agw::LogLevel::Error: qWarning() << "[agw]" << msg; break;
            case agw::LogLevel::Warning: qWarning() << "[agw]" << msg; break;
            default: qDebug() << "[agw]" << msg; break;
            }
        };

        // Хост-специфика перед запросом (один раз, исходный хост). Kill-switch остаётся в приложении.
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

    // Реестр долгоживущих клиентов по окружению — кеш прокси переживает запросы (бывш. static m_proxyUrl).
    std::shared_ptr<agw::GatewayClient> getClientForEnv(const QString &gatewayEndpoint, bool isDevEnvironment,
                                                        int requestTimeoutMsecs, bool isStrictKillSwitchEnabled)
    {
        static std::mutex mutex;
        static std::map<std::string, std::shared_ptr<agw::GatewayClient>> clients;

        const std::string key = gatewayEndpoint.toStdString() + "|" + (isDevEnvironment ? "1" : "0") + "|"
                + std::to_string(requestTimeoutMsecs) + "|" + (isStrictKillSwitchEnabled ? "1" : "0");

        std::lock_guard<std::mutex> lock(mutex);
        auto it = clients.find(key);
        if (it != clients.end()) {
            return it->second;
        }
        auto client = std::make_shared<agw::GatewayClient>(
                makeConfig(gatewayEndpoint, isDevEnvironment, requestTimeoutMsecs, isStrictKillSwitchEnabled));
        clients.emplace(key, client);
        return client;
    }
} // namespace

GatewayController::GatewayController(const QString &gatewayEndpoint, const bool isDevEnvironment, const int requestTimeoutMsecs,
                                     const bool isStrictKillSwitchEnabled, QObject *parent)
    : QObject(parent),
      m_client(getClientForEnv(gatewayEndpoint, isDevEnvironment, requestTimeoutMsecs, isStrictKillSwitchEnabled))
{
}

amnezia::ErrorCode GatewayController::post(const QString &endpoint, const QJsonObject apiPayload, QByteArray &responseBody)
{
    const std::string payload = QJsonDocument(apiPayload).toJson().toStdString();
    const std::string serviceType = apiPayload.value(apiDefs::key::serviceType).toString().toStdString();
    const std::string userCountryCode = apiPayload.value(apiDefs::key::userCountryCode).toString().toStdString();

    const agw::Response r =
            m_client->post(endpoint.toStdString(), payload, agw::FailoverContext { serviceType, userCountryCode });

    responseBody = QByteArray::fromStdString(r.body);
    return mapError(r.error);
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

    m_client->postAsync(
            endpoint.toStdString(), payload,
            [promise, self](agw::Response r) {
                const amnezia::ErrorCode ec = mapError(r.error);
                const QByteArray body = QByteArray::fromStdString(r.body);
                // Маршалим результат с потока пула на поток объекта (Qt::QueuedConnection).
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
            agw::FailoverContext { serviceType, userCountryCode });

    return future;
}
