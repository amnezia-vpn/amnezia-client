#include "gatewayController.h"

#include <map>
#include <mutex>
#include <string>
#include <vector>

#include <QDebug>
#include <QEventLoop>
#include <QFutureWatcher>
#include <QJsonDocument>
#include <QPointer>
#include <QPromise>
#include <QSharedPointer>
#include <QStringList>
#include <QThread>
#include <QtConcurrent/QtConcurrent>

#include <amnezia/gateway_sdk/api.h>
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
            case amnezia::gateway_sdk::LogLevel::Error: qWarning() << "[amnezia-sdk]" << msg; break;
            case amnezia::gateway_sdk::LogLevel::Warning: qWarning() << "[amnezia-sdk]" << msg; break;
            default: qDebug() << "[amnezia-sdk]" << msg; break;
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

    GatewayController::ImportResult toAdapterImport(const amnezia::gateway_sdk::api::ImportResult &r)
    {
        GatewayController::ImportResult out;
        out.error = mapError(r.error);
        out.captchaRequired = r.captchaRequired;
        out.captchaId = QString::fromStdString(r.captcha.captchaId);
        out.captchaImageBase64 = QString::fromStdString(r.captcha.captchaImageBase64);
        out.hint = QString::fromStdString(r.captcha.hint);
        out.serverConfigJson = QString::fromStdString(r.serverConfigJson);
        out.rawResponse = QByteArray::fromStdString(r.rawResponseJson);
        return out;
    }

    amnezia::gateway_sdk::api::GatewayRequest toApiReq(const GatewayController::GatewayRequest &q)
    {
        amnezia::gateway_sdk::api::GatewayRequest r;
        r.osVersion = q.osVersion.toStdString();
        r.appVersion = q.appVersion.toStdString();
        r.appLanguage = q.appLanguage.toStdString();
        r.installationUuid = q.installationUuid.toStdString();
        r.userCountryCode = q.userCountryCode.toStdString();
        r.serverCountryCode = q.serverCountryCode.toStdString();
        r.serviceType = q.serviceType.toStdString();
        r.serviceProtocol = q.serviceProtocol.toStdString();
        if (!q.authData.isEmpty()) {
            r.authDataJson = QString::fromUtf8(QJsonDocument(q.authData).toJson(QJsonDocument::Compact)).toStdString();
        }
        return r;
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

void GatewayController::runBlocking(const std::function<void()> &work)
{
    QEventLoop loop;
    QFutureWatcher<void> watcher;
    QObject::connect(&watcher, &QFutureWatcher<void>::finished, &loop, &QEventLoop::quit);
    watcher.setFuture(QtConcurrent::run(work));
    loop.exec(QEventLoop::ExcludeUserInputEvents);
}

amnezia::ErrorCode GatewayController::getServices(const QString &osVersion, const QString &appVersion,
                                                         const QString &cliName, const QString &appLanguage,
                                                         QJsonObject &servicesOut)
{
    qInfo().noquote() << "[agw-adapter] getServices (typed) callerThread=" << QThread::currentThread();

    amnezia::gateway_sdk::api::JsonResult res;
    auto controller = m_controller;
    runBlocking([controller, &res, osVersion, appVersion, cliName, appLanguage]() {
        res = amnezia::gateway_sdk::api::getServices(*controller, osVersion.toStdString(), appVersion.toStdString(),
                                    cliName.toStdString(), appLanguage.toStdString());
    });

    servicesOut = QJsonDocument::fromJson(QByteArray::fromStdString(res.json)).object();
    const amnezia::ErrorCode ec = mapError(res.error);
    qInfo().noquote() << "[agw-adapter] getServices result errorCode=" << static_cast<int>(ec);
    return ec;
}

amnezia::ErrorCode GatewayController::importTrial(const GatewayRequest &request, const QString &publicKey,
                                                         const QString &email, QString &serverConfigJsonOut)
{
    auto controller = m_controller;
    const amnezia::gateway_sdk::api::GatewayRequest req = toApiReq(request);
    const std::string pk = publicKey.toStdString();
    const std::string em = email.toStdString();

    amnezia::gateway_sdk::api::ImportResult res;
    runBlocking([controller, req, pk, em, &res]() { res = amnezia::gateway_sdk::api::importTrial(*controller, req, pk, em); });

    serverConfigJsonOut = QString::fromStdString(res.serverConfigJson);
    const amnezia::ErrorCode ec = mapError(res.error);
    qInfo().noquote() << "[agw-adapter] importTrial result errorCode=" << static_cast<int>(ec);
    return ec;
}

amnezia::ErrorCode GatewayController::deactivateDevice(const GatewayRequest &request)
{
    auto controller = m_controller;
    const amnezia::gateway_sdk::api::GatewayRequest req = toApiReq(request);

    amnezia::gateway_sdk::ErrorCode err = amnezia::gateway_sdk::ErrorCode::NoError;
    runBlocking([controller, req, &err]() { err = amnezia::gateway_sdk::api::deactivateDevice(*controller, req); });

    const amnezia::ErrorCode ec = mapError(err);
    qInfo().noquote() << "[agw-adapter] deactivateDevice result errorCode=" << static_cast<int>(ec);
    return ec;
}

GatewayController::ImportResult GatewayController::importService(const GatewayRequest &request,
                                                                               const QString &publicKey)
{
    auto controller = m_controller;
    const amnezia::gateway_sdk::api::GatewayRequest req = toApiReq(request);
    const std::string pk = publicKey.toStdString();

    amnezia::gateway_sdk::api::ImportResult res;
    runBlocking([controller, req, pk, &res]() { res = amnezia::gateway_sdk::api::importService(*controller, req, pk); });

    qInfo().noquote() << "[agw-adapter] importService result errorCode=" << static_cast<int>(mapError(res.error));
    return toAdapterImport(res);
}

GatewayController::ImportResult GatewayController::resolveImportCaptcha(const GatewayRequest &request,
                                                                                     const QString &publicKey,
                                                                                     const QString &captchaId,
                                                                                     const QString &captchaSolution)
{
    auto controller = m_controller;
    const amnezia::gateway_sdk::api::GatewayRequest req = toApiReq(request);
    const std::string pk = publicKey.toStdString();
    const std::string cid = captchaId.toStdString();
    const std::string sol = captchaSolution.toStdString();

    amnezia::gateway_sdk::api::ImportResult res;
    runBlocking([controller, req, pk, cid, sol, &res]() {
        res = amnezia::gateway_sdk::api::resolveImportCaptcha(*controller, req, pk, cid, sol);
    });

    qInfo().noquote() << "[agw-adapter] resolveImportCaptcha result errorCode=" << static_cast<int>(mapError(res.error));
    return toAdapterImport(res);
}

GatewayController::ImportResult GatewayController::updateService(const GatewayRequest &request,
                                                                               const QString &publicKey,
                                                                               bool isConnectEvent)
{
    auto controller = m_controller;
    const amnezia::gateway_sdk::api::GatewayRequest req = toApiReq(request);
    const std::string pk = publicKey.toStdString();

    amnezia::gateway_sdk::api::ImportResult res;
    runBlocking([controller, req, pk, isConnectEvent, &res]() {
        res = amnezia::gateway_sdk::api::updateService(*controller, req, pk, isConnectEvent);
    });

    qInfo().noquote() << "[agw-adapter] updateService result errorCode=" << static_cast<int>(mapError(res.error));
    return toAdapterImport(res);
}

amnezia::ErrorCode GatewayController::getAccountInfoRaw(const GatewayRequest &request, const QString &cliVersion,
                                                              const QString &subscriptionStatus, QByteArray &rawJsonOut)
{
    auto controller = m_controller;
    const amnezia::gateway_sdk::api::GatewayRequest req = toApiReq(request);
    const std::string cv = cliVersion.toStdString();
    const std::string ss = subscriptionStatus.toStdString();

    amnezia::gateway_sdk::api::JsonResult res;
    runBlocking([controller, req, cv, ss, &res]() { res = amnezia::gateway_sdk::api::getAccountInfoRaw(*controller, req, cv, ss); });

    rawJsonOut = QByteArray::fromStdString(res.json);
    return mapError(res.error);
}

amnezia::ErrorCode GatewayController::exportNativeConfig(const GatewayRequest &request, const QString &publicKey,
                                                               QString &nativeConfigOut)
{
    auto controller = m_controller;
    const amnezia::gateway_sdk::api::GatewayRequest req = toApiReq(request);
    const std::string pk = publicKey.toStdString();

    amnezia::gateway_sdk::api::NativeConfigResult res;
    runBlocking([controller, req, pk, &res]() { res = amnezia::gateway_sdk::api::exportNativeConfig(*controller, req, pk); });

    nativeConfigOut = QString::fromStdString(res.config);
    return mapError(res.error);
}

amnezia::ErrorCode GatewayController::revokeNativeConfig(const GatewayRequest &request)
{
    auto controller = m_controller;
    const amnezia::gateway_sdk::api::GatewayRequest req = toApiReq(request);

    amnezia::gateway_sdk::ErrorCode err = amnezia::gateway_sdk::ErrorCode::NoError;
    runBlocking([controller, req, &err]() { err = amnezia::gateway_sdk::api::revokeNativeConfig(*controller, req); });

    return mapError(err);
}

GatewayController::AppStoreResult GatewayController::importServiceFromAppStore(
        const GatewayRequest &request, const QString &publicKey, const QString &transactionId)
{
    auto controller = m_controller;
    const amnezia::gateway_sdk::api::GatewayRequest req = toApiReq(request);
    const std::string pk = publicKey.toStdString();
    const std::string tx = transactionId.toStdString();

    amnezia::gateway_sdk::api::AppStoreImportResult res;
    runBlocking([controller, req, pk, tx, &res]() {
        res = amnezia::gateway_sdk::api::importServiceFromAppStore(*controller, req, pk, tx);
    });

    AppStoreResult out;
    out.error = mapError(res.error);
    out.serverConfigJson = QString::fromStdString(res.serverConfigJson);
    out.vpnKey = QString::fromStdString(res.vpnKey);
    out.crc = res.crc;
    qInfo().noquote() << "[agw-adapter] importServiceFromAppStore result errorCode=" << static_cast<int>(out.error);
    return out;
}

QFuture<QPair<amnezia::ErrorCode, QJsonArray>> GatewayController::getNewsAsync(const QString &locale,
                                                                                     const QStringList &userCountryCodes,
                                                                                     const QStringList &serviceTypes)
{
    auto controller = m_controller;
    const std::string localeStd = locale.toStdString();
    std::vector<std::string> countries;
    for (const QString &c : userCountryCodes) {
        countries.push_back(c.toStdString());
    }
    std::vector<std::string> types;
    for (const QString &t : serviceTypes) {
        types.push_back(t.toStdString());
    }

    return QtConcurrent::run([controller, localeStd, countries, types]() -> QPair<amnezia::ErrorCode, QJsonArray> {
        const amnezia::gateway_sdk::api::JsonResult r = amnezia::gateway_sdk::api::getNews(*controller, localeStd, countries, types);
        const amnezia::ErrorCode ec = mapError(r.error);
        if (ec != amnezia::ErrorCode::NoError) {
            return qMakePair(ec, QJsonArray());
        }
        const QJsonArray arr = QJsonDocument::fromJson(QByteArray::fromStdString(r.json)).array();
        return qMakePair(ec, arr);
    });
}

QFuture<QPair<amnezia::ErrorCode, QString>> GatewayController::getUpdaterEndpointAsync(
        const QString &cliVersion, const QString &osVersion, const QString &installationUuid)
{
    auto controller = m_controller;
    const std::string cv = cliVersion.toStdString();
    const std::string ov = osVersion.toStdString();
    const std::string uuid = installationUuid.toStdString();

    return QtConcurrent::run([controller, cv, ov, uuid]() -> QPair<amnezia::ErrorCode, QString> {
        const amnezia::gateway_sdk::api::UrlResult r = amnezia::gateway_sdk::api::getUpdaterEndpoint(*controller, cv, ov, uuid);
        return qMakePair(mapError(r.error), QString::fromStdString(r.url));
    });
}

QFuture<QPair<amnezia::ErrorCode, QString>> GatewayController::getRenewalLinkAsync(
        const GatewayRequest &request, const QString &cliVersion, const QString &subscriptionStatus)
{
    auto controller = m_controller;
    const amnezia::gateway_sdk::api::GatewayRequest req = toApiReq(request);
    const std::string cv = cliVersion.toStdString();
    const std::string ss = subscriptionStatus.toStdString();

    return QtConcurrent::run([controller, req, cv, ss]() -> QPair<amnezia::ErrorCode, QString> {
        const amnezia::gateway_sdk::api::RenewalResult r = amnezia::gateway_sdk::api::getRenewalLink(*controller, req, cv, ss);
        return qMakePair(mapError(r.error), QString::fromStdString(r.renewalUrl));
    });
}
