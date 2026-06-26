#include "gatewayControllerAdapter.h"

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

#include <agw/gateway_controller.h>
#include <agw/config.h>
#include <agw/types.h>

#include "core/utils/constants/apiKeys.h"
#include "core/utils/networkUtilities.h"

#include "embedded_agw_public_keys.h"

#ifdef Q_OS_IOS
    #include "platforms/ios/ios_controller.h"
#endif

#ifdef AMNEZIA_DESKTOP
    #include "core/utils/ipcClient.h"
#endif

namespace
{
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

        cfg.log = [](agw::LogLevel level, const std::string &message) {
            const QString msg = QString::fromStdString(message);
            switch (level) {
            case agw::LogLevel::Error: qWarning() << "[agw]" << msg; break;
            case agw::LogLevel::Warning: qWarning() << "[agw]" << msg; break;
            default: qDebug() << "[agw]" << msg; break;
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
                            qWarning() << "GatewayControllerAdapter: addKillSwitchAllowedRange failed";
                        }
                    });
                }
            }
#endif
        };

        return cfg;
    }

    std::shared_ptr<agw::GatewayController> getClientForEnv(const QString &gatewayEndpoint, bool isDevEnvironment,
                                                        int requestTimeoutMsecs, bool isStrictKillSwitchEnabled)
    {
        static std::mutex mutex;
        static std::map<std::string, std::shared_ptr<agw::GatewayController>> clients;

        const std::string key = gatewayEndpoint.toStdString() + "|" + (isDevEnvironment ? "1" : "0") + "|"
                + std::to_string(requestTimeoutMsecs) + "|" + (isStrictKillSwitchEnabled ? "1" : "0");

        std::lock_guard<std::mutex> lock(mutex);
        auto it = clients.find(key);
        if (it != clients.end()) {
            return it->second;
        }
        auto client = std::make_shared<agw::GatewayController>(
                makeConfig(gatewayEndpoint, isDevEnvironment, requestTimeoutMsecs, isStrictKillSwitchEnabled));
        clients.emplace(key, client);
        return client;
    }
}

GatewayControllerAdapter::GatewayControllerAdapter(const QString &gatewayEndpoint, const bool isDevEnvironment, const int requestTimeoutMsecs,
                                     const bool isStrictKillSwitchEnabled, QObject *parent)
    : QObject(parent),
      m_controller(getClientForEnv(gatewayEndpoint, isDevEnvironment, requestTimeoutMsecs, isStrictKillSwitchEnabled))
{
}

amnezia::ErrorCode GatewayControllerAdapter::post(const QString &endpoint, const QJsonObject apiPayload, QByteArray &responseBody)
{
    const std::string payload = QJsonDocument(apiPayload).toJson().toStdString();
    const std::string serviceType = apiPayload.value(apiDefs::key::serviceType).toString().toStdString();
    const std::string userCountryCode = apiPayload.value(apiDefs::key::userCountryCode).toString().toStdString();

    qInfo().noquote() << "[agw-adapter] post (sync) endpoint=" << endpoint
                      << "payloadLen=" << payload.size() << "thread=" << QThread::currentThread();

    QEventLoop loop;
    QObject context;
    agw::Response result;

    m_controller->postAsync(
            endpoint.toStdString(), payload,
            [&loop, &context, &result](agw::Response r) {
                QMetaObject::invokeMethod(
                        &context,
                        [&loop, &result, r]() {
                            result = r;
                            loop.quit();
                        },
                        Qt::QueuedConnection);
            },
            agw::FailoverContext { serviceType, userCountryCode });

    loop.exec(QEventLoop::ExcludeUserInputEvents);

    responseBody = QByteArray::fromStdString(result.body);
    const amnezia::ErrorCode ec = mapError(result.error);
    qInfo().noquote() << "[agw-adapter] post (sync) result errorCode=" << static_cast<int>(ec)
                      << "bodyLen=" << responseBody.size();
    return ec;
}

QFuture<QPair<amnezia::ErrorCode, QByteArray>> GatewayControllerAdapter::postAsync(const QString &endpoint, const QJsonObject apiPayload)
{
    auto promise = QSharedPointer<QPromise<QPair<amnezia::ErrorCode, QByteArray>>>::create();
    promise->start();
    QFuture<QPair<amnezia::ErrorCode, QByteArray>> future = promise->future();

    const std::string payload = QJsonDocument(apiPayload).toJson().toStdString();
    const std::string serviceType = apiPayload.value(apiDefs::key::serviceType).toString().toStdString();
    const std::string userCountryCode = apiPayload.value(apiDefs::key::userCountryCode).toString().toStdString();

    QPointer<GatewayControllerAdapter> self(this);

    qInfo().noquote() << "[agw-adapter] postAsync endpoint=" << endpoint
                      << "payloadLen=" << payload.size() << "callerThread=" << QThread::currentThread();

    m_controller->postAsync(
            endpoint.toStdString(), payload,
            [promise, self](agw::Response r) {
                const amnezia::ErrorCode ec = mapError(r.error);
                const QByteArray body = QByteArray::fromStdString(r.body);
                qInfo().noquote() << "[agw-adapter] postAsync SDK callback errorCode=" << static_cast<int>(ec)
                                  << "bodyLen=" << body.size() << "poolThread=" << QThread::currentThread()
                                  << "→ marshalling to object thread";

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
