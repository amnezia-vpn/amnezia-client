#include "gatewayControllerAdapter.h"

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

#include <amnezia/sdk/api.h>
#include <amnezia/sdk/gateway_controller.h>
#include <amnezia/sdk/config.h>
#include <amnezia/sdk/types.h>

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
    amnezia::ErrorCode mapError(amnezia::sdk::ErrorCode error)
    {
        if (error == amnezia::sdk::ErrorCode::Cancelled) {
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

    amnezia::sdk::Config makeConfig(const QString &gatewayEndpoint, bool isDevEnvironment, int requestTimeoutMsecs,
                           bool isStrictKillSwitchEnabled)
    {
        amnezia::sdk::Config cfg;
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

        cfg.log = [](amnezia::sdk::LogLevel level, const std::string &message) {
            const QString msg = QString::fromStdString(message);
            switch (level) {
            case amnezia::sdk::LogLevel::Error: qWarning() << "[amnezia-sdk]" << msg; break;
            case amnezia::sdk::LogLevel::Warning: qWarning() << "[amnezia-sdk]" << msg; break;
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
                            qWarning() << "GatewayControllerAdapter: addKillSwitchAllowedRange failed";
                        }
                    });
                }
            }
#endif
        };

        return cfg;
    }

    GatewayControllerAdapter::ImportResult toAdapterImport(const amnezia::sdk::api::ImportResult &r)
    {
        GatewayControllerAdapter::ImportResult out;
        out.error = mapError(r.error);
        out.captchaRequired = r.captchaRequired;
        out.captchaId = QString::fromStdString(r.captcha.captchaId);
        out.captchaImageBase64 = QString::fromStdString(r.captcha.captchaImageBase64);
        out.hint = QString::fromStdString(r.captcha.hint);
        out.serverConfigJson = QString::fromStdString(r.serverConfigJson);
        out.rawResponse = QByteArray::fromStdString(r.rawResponseJson);
        return out;
    }

    amnezia::sdk::api::GatewayRequest toApiReq(const GatewayControllerAdapter::GatewayRequest &q)
    {
        amnezia::sdk::api::GatewayRequest r;
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

    std::shared_ptr<amnezia::sdk::GatewayController> getClientForEnv(const QString &gatewayEndpoint, bool isDevEnvironment,
                                                        int requestTimeoutMsecs, bool isStrictKillSwitchEnabled)
    {
        static std::mutex mutex;
        static std::map<std::string, std::shared_ptr<amnezia::sdk::GatewayController>> clients;

        const std::string key = gatewayEndpoint.toStdString() + "|" + (isDevEnvironment ? "1" : "0") + "|"
                + std::to_string(requestTimeoutMsecs) + "|" + (isStrictKillSwitchEnabled ? "1" : "0");

        std::lock_guard<std::mutex> lock(mutex);
        auto it = clients.find(key);
        if (it != clients.end()) {
            return it->second;
        }
        auto client = std::make_shared<amnezia::sdk::GatewayController>(
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

void GatewayControllerAdapter::runBlocking(const std::function<void()> &work)
{
    QEventLoop loop;
    QFutureWatcher<void> watcher;
    QObject::connect(&watcher, &QFutureWatcher<void>::finished, &loop, &QEventLoop::quit);
    watcher.setFuture(QtConcurrent::run(work));
    loop.exec(QEventLoop::ExcludeUserInputEvents);
}

amnezia::ErrorCode GatewayControllerAdapter::getServices(const QString &osVersion, const QString &appVersion,
                                                         const QString &cliName, const QString &appLanguage,
                                                         QJsonObject &servicesOut)
{
    qInfo().noquote() << "[agw-adapter] getServices (typed) callerThread=" << QThread::currentThread();

    amnezia::sdk::api::JsonResult res;
    auto controller = m_controller;
    runBlocking([controller, &res, osVersion, appVersion, cliName, appLanguage]() {
        res = amnezia::sdk::api::getServices(*controller, osVersion.toStdString(), appVersion.toStdString(),
                                    cliName.toStdString(), appLanguage.toStdString());
    });

    servicesOut = QJsonDocument::fromJson(QByteArray::fromStdString(res.json)).object();
    const amnezia::ErrorCode ec = mapError(res.error);
    qInfo().noquote() << "[agw-adapter] getServices result errorCode=" << static_cast<int>(ec);
    return ec;
}

amnezia::ErrorCode GatewayControllerAdapter::importTrial(const GatewayRequest &request, const QString &publicKey,
                                                         const QString &email, QString &serverConfigJsonOut)
{
    auto controller = m_controller;
    const amnezia::sdk::api::GatewayRequest req = toApiReq(request);
    const std::string pk = publicKey.toStdString();
    const std::string em = email.toStdString();

    amnezia::sdk::api::ImportResult res;
    runBlocking([controller, req, pk, em, &res]() { res = amnezia::sdk::api::importTrial(*controller, req, pk, em); });

    serverConfigJsonOut = QString::fromStdString(res.serverConfigJson);
    const amnezia::ErrorCode ec = mapError(res.error);
    qInfo().noquote() << "[agw-adapter] importTrial result errorCode=" << static_cast<int>(ec);
    return ec;
}

amnezia::ErrorCode GatewayControllerAdapter::deactivateDevice(const GatewayRequest &request)
{
    auto controller = m_controller;
    const amnezia::sdk::api::GatewayRequest req = toApiReq(request);

    amnezia::sdk::ErrorCode err = amnezia::sdk::ErrorCode::NoError;
    runBlocking([controller, req, &err]() { err = amnezia::sdk::api::deactivateDevice(*controller, req); });

    const amnezia::ErrorCode ec = mapError(err);
    qInfo().noquote() << "[agw-adapter] deactivateDevice result errorCode=" << static_cast<int>(ec);
    return ec;
}

GatewayControllerAdapter::ImportResult GatewayControllerAdapter::importService(const GatewayRequest &request,
                                                                               const QString &publicKey)
{
    auto controller = m_controller;
    const amnezia::sdk::api::GatewayRequest req = toApiReq(request);
    const std::string pk = publicKey.toStdString();

    amnezia::sdk::api::ImportResult res;
    runBlocking([controller, req, pk, &res]() { res = amnezia::sdk::api::importService(*controller, req, pk); });

    qInfo().noquote() << "[agw-adapter] importService result errorCode=" << static_cast<int>(mapError(res.error));
    return toAdapterImport(res);
}

GatewayControllerAdapter::ImportResult GatewayControllerAdapter::resolveImportCaptcha(const GatewayRequest &request,
                                                                                     const QString &publicKey,
                                                                                     const QString &captchaId,
                                                                                     const QString &captchaSolution)
{
    auto controller = m_controller;
    const amnezia::sdk::api::GatewayRequest req = toApiReq(request);
    const std::string pk = publicKey.toStdString();
    const std::string cid = captchaId.toStdString();
    const std::string sol = captchaSolution.toStdString();

    amnezia::sdk::api::ImportResult res;
    runBlocking([controller, req, pk, cid, sol, &res]() {
        res = amnezia::sdk::api::resolveImportCaptcha(*controller, req, pk, cid, sol);
    });

    qInfo().noquote() << "[agw-adapter] resolveImportCaptcha result errorCode=" << static_cast<int>(mapError(res.error));
    return toAdapterImport(res);
}

GatewayControllerAdapter::ImportResult GatewayControllerAdapter::updateService(const GatewayRequest &request,
                                                                               const QString &publicKey,
                                                                               bool isConnectEvent)
{
    auto controller = m_controller;
    const amnezia::sdk::api::GatewayRequest req = toApiReq(request);
    const std::string pk = publicKey.toStdString();

    amnezia::sdk::api::ImportResult res;
    runBlocking([controller, req, pk, isConnectEvent, &res]() {
        res = amnezia::sdk::api::updateService(*controller, req, pk, isConnectEvent);
    });

    qInfo().noquote() << "[agw-adapter] updateService result errorCode=" << static_cast<int>(mapError(res.error));
    return toAdapterImport(res);
}

amnezia::ErrorCode GatewayControllerAdapter::getAccountInfoRaw(const GatewayRequest &request, const QString &cliVersion,
                                                              const QString &subscriptionStatus, QByteArray &rawJsonOut)
{
    auto controller = m_controller;
    const amnezia::sdk::api::GatewayRequest req = toApiReq(request);
    const std::string cv = cliVersion.toStdString();
    const std::string ss = subscriptionStatus.toStdString();

    amnezia::sdk::api::JsonResult res;
    runBlocking([controller, req, cv, ss, &res]() { res = amnezia::sdk::api::getAccountInfoRaw(*controller, req, cv, ss); });

    rawJsonOut = QByteArray::fromStdString(res.json);
    return mapError(res.error);
}

amnezia::ErrorCode GatewayControllerAdapter::exportNativeConfig(const GatewayRequest &request, const QString &publicKey,
                                                               QString &nativeConfigOut)
{
    auto controller = m_controller;
    const amnezia::sdk::api::GatewayRequest req = toApiReq(request);
    const std::string pk = publicKey.toStdString();

    amnezia::sdk::api::NativeConfigResult res;
    runBlocking([controller, req, pk, &res]() { res = amnezia::sdk::api::exportNativeConfig(*controller, req, pk); });

    nativeConfigOut = QString::fromStdString(res.config);
    return mapError(res.error);
}

amnezia::ErrorCode GatewayControllerAdapter::revokeNativeConfig(const GatewayRequest &request)
{
    auto controller = m_controller;
    const amnezia::sdk::api::GatewayRequest req = toApiReq(request);

    amnezia::sdk::ErrorCode err = amnezia::sdk::ErrorCode::NoError;
    runBlocking([controller, req, &err]() { err = amnezia::sdk::api::revokeNativeConfig(*controller, req); });

    return mapError(err);
}

GatewayControllerAdapter::AppStoreResult GatewayControllerAdapter::importServiceFromAppStore(
        const GatewayRequest &request, const QString &publicKey, const QString &transactionId)
{
    auto controller = m_controller;
    const amnezia::sdk::api::GatewayRequest req = toApiReq(request);
    const std::string pk = publicKey.toStdString();
    const std::string tx = transactionId.toStdString();

    amnezia::sdk::api::AppStoreImportResult res;
    runBlocking([controller, req, pk, tx, &res]() {
        res = amnezia::sdk::api::importServiceFromAppStore(*controller, req, pk, tx);
    });

    AppStoreResult out;
    out.error = mapError(res.error);
    out.serverConfigJson = QString::fromStdString(res.serverConfigJson);
    out.vpnKey = QString::fromStdString(res.vpnKey);
    out.crc = res.crc;
    qInfo().noquote() << "[agw-adapter] importServiceFromAppStore result errorCode=" << static_cast<int>(out.error);
    return out;
}

QFuture<QPair<amnezia::ErrorCode, QJsonArray>> GatewayControllerAdapter::getNewsAsync(const QString &locale,
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
        const amnezia::sdk::api::JsonResult r = amnezia::sdk::api::getNews(*controller, localeStd, countries, types);
        const amnezia::ErrorCode ec = mapError(r.error);
        if (ec != amnezia::ErrorCode::NoError) {
            return qMakePair(ec, QJsonArray());
        }
        const QJsonArray arr = QJsonDocument::fromJson(QByteArray::fromStdString(r.json)).array();
        return qMakePair(ec, arr);
    });
}

QFuture<QPair<amnezia::ErrorCode, QString>> GatewayControllerAdapter::getUpdaterEndpointAsync(
        const QString &cliVersion, const QString &osVersion, const QString &installationUuid)
{
    auto controller = m_controller;
    const std::string cv = cliVersion.toStdString();
    const std::string ov = osVersion.toStdString();
    const std::string uuid = installationUuid.toStdString();

    return QtConcurrent::run([controller, cv, ov, uuid]() -> QPair<amnezia::ErrorCode, QString> {
        const amnezia::sdk::api::UrlResult r = amnezia::sdk::api::getUpdaterEndpoint(*controller, cv, ov, uuid);
        return qMakePair(mapError(r.error), QString::fromStdString(r.url));
    });
}

QFuture<QPair<amnezia::ErrorCode, QString>> GatewayControllerAdapter::getRenewalLinkAsync(
        const GatewayRequest &request, const QString &cliVersion, const QString &subscriptionStatus)
{
    auto controller = m_controller;
    const amnezia::sdk::api::GatewayRequest req = toApiReq(request);
    const std::string cv = cliVersion.toStdString();
    const std::string ss = subscriptionStatus.toStdString();

    return QtConcurrent::run([controller, req, cv, ss]() -> QPair<amnezia::ErrorCode, QString> {
        const amnezia::sdk::api::RenewalResult r = amnezia::sdk::api::getRenewalLink(*controller, req, cv, ss);
        return qMakePair(mapError(r.error), QString::fromStdString(r.renewalUrl));
    });
}
