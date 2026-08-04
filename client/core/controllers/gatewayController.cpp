#include "gatewayController.h"

#include <cstdlib>
#include <cstring>
#include <map>
#include <mutex>
#include <string>
#include <vector>

#include <QDebug>
#include <QCoreApplication>
#include <QEventLoop>
#include <QFutureWatcher>
#include <QPromise>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSharedPointer>
#include <QString>
#include <QStringList>
#include <QThread>
#include <QtConcurrent/QtConcurrent>

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
            qWarning() << "[amnezia-sdk]" << msg;
        } else {
            qDebug() << "[amnezia-sdk]" << msg;
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

    struct ReqHold
    {
        std::string os, app, lang, uuid, ucc, scc, st, sp, auth;
        amnezia_gateway_api_gateway_request c{};

        void fill(const GatewayController::GatewayRequest &q)
        {
            os = q.osVersion.toStdString();
            app = q.appVersion.toStdString();
            lang = q.appLanguage.toStdString();
            uuid = q.installationUuid.toStdString();
            ucc = q.userCountryCode.toStdString();
            scc = q.serverCountryCode.toStdString();
            st = q.serviceType.toStdString();
            sp = q.serviceProtocol.toStdString();
            if (!q.authData.isEmpty()) {
                auth = QString::fromUtf8(QJsonDocument(q.authData).toJson(QJsonDocument::Compact)).toStdString();
            }
            c.os_version = os.c_str();
            c.app_version = app.c_str();
            c.app_language = lang.c_str();
            c.installation_uuid = uuid.c_str();
            c.user_country_code = ucc.c_str();
            c.server_country_code = scc.c_str();
            c.service_type = st.c_str();
            c.service_protocol = sp.c_str();
            c.auth_data_json = auth.empty() ? nullptr : auth.c_str();
        }
    };

    GatewayController::ImportResult toAdapterImport(const amnezia_gateway_api_import_result &r)
    {
        GatewayController::ImportResult out;
        out.error = mapError(r.error);
        out.captchaRequired = r.captcha_required != 0;
        out.captchaId = QString::fromUtf8(r.captcha_id ? r.captcha_id : "");
        out.captchaImageBase64 = QString::fromUtf8(r.captcha_image ? r.captcha_image : "");
        out.hint = QString::fromUtf8(r.captcha_hint ? r.captcha_hint : "");
        out.serverConfigJson = QString::fromUtf8(r.server_config_json ? r.server_config_json : "");
        out.rawResponse = r.raw_response_json ? QByteArray(r.raw_response_json) : QByteArray();
        return out;
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
    auto client = m_controller;
    amnezia_gateway_api_json_result res{};
    runBlocking([&]() {
        res = amnezia_gateway_api_get_services(client.get(), osVersion.toUtf8().constData(), appVersion.toUtf8().constData(),
                                               cliName.toUtf8().constData(), appLanguage.toUtf8().constData());
    });

    servicesOut = QJsonDocument::fromJson(res.json ? QByteArray(res.json, static_cast<int>(res.json_len)) : QByteArray()).object();
    const amnezia::ErrorCode ec = mapError(res.error);
    amnezia_gateway_api_json_result_free(&res);
    return ec;
}

amnezia::ErrorCode GatewayController::importTrial(const GatewayRequest &request, const QString &publicKey,
                                                  const QString &email, QString &serverConfigJsonOut)
{
    auto client = m_controller;
    amnezia_gateway_api_import_result res{};
    runBlocking([&]() {
        ReqHold h;
        h.fill(request);
        res = amnezia_gateway_api_import_trial(client.get(), &h.c, publicKey.toUtf8().constData(), email.toUtf8().constData());
    });

    serverConfigJsonOut = QString::fromUtf8(res.server_config_json ? res.server_config_json : "");
    const amnezia::ErrorCode ec = mapError(res.error);
    amnezia_gateway_api_import_result_free(&res);
    return ec;
}

amnezia::ErrorCode GatewayController::deactivateDevice(const GatewayRequest &request)
{
    auto client = m_controller;
    int err = AMNEZIA_GATEWAY_API_OK;
    runBlocking([&]() {
        ReqHold h;
        h.fill(request);
        err = amnezia_gateway_api_deactivate_device(client.get(), &h.c);
    });
    return mapError(err);
}

GatewayController::ImportResult GatewayController::importService(const GatewayRequest &request, const QString &publicKey)
{
    auto client = m_controller;
    amnezia_gateway_api_import_result res{};
    runBlocking([&]() {
        ReqHold h;
        h.fill(request);
        res = amnezia_gateway_api_import_service(client.get(), &h.c, publicKey.toUtf8().constData());
    });
    GatewayController::ImportResult out = toAdapterImport(res);
    amnezia_gateway_api_import_result_free(&res);
    return out;
}

GatewayController::ImportResult GatewayController::resolveImportCaptcha(const GatewayRequest &request, const QString &publicKey,
                                                                       const QString &captchaId, const QString &captchaSolution)
{
    auto client = m_controller;
    amnezia_gateway_api_import_result res{};
    runBlocking([&]() {
        ReqHold h;
        h.fill(request);
        res = amnezia_gateway_api_resolve_import_captcha(client.get(), &h.c, publicKey.toUtf8().constData(),
                                                         captchaId.toUtf8().constData(), captchaSolution.toUtf8().constData());
    });
    GatewayController::ImportResult out = toAdapterImport(res);
    amnezia_gateway_api_import_result_free(&res);
    return out;
}

GatewayController::ImportResult GatewayController::updateService(const GatewayRequest &request, const QString &publicKey,
                                                                bool isConnectEvent)
{
    auto client = m_controller;
    amnezia_gateway_api_import_result res{};
    runBlocking([&]() {
        ReqHold h;
        h.fill(request);
        res = amnezia_gateway_api_update_service(client.get(), &h.c, publicKey.toUtf8().constData(), isConnectEvent ? 1 : 0);
    });
    GatewayController::ImportResult out = toAdapterImport(res);
    amnezia_gateway_api_import_result_free(&res);
    return out;
}

amnezia::ErrorCode GatewayController::getAccountInfoRaw(const GatewayRequest &request, const QString &cliVersion,
                                                        const QString &subscriptionStatus, QByteArray &rawJsonOut)
{
    auto client = m_controller;
    amnezia_gateway_api_json_result res{};
    runBlocking([&]() {
        ReqHold h;
        h.fill(request);
        res = amnezia_gateway_api_get_account_info_raw(client.get(), &h.c, cliVersion.toUtf8().constData(),
                                                       subscriptionStatus.toUtf8().constData());
    });
    rawJsonOut = res.json ? QByteArray(res.json, static_cast<int>(res.json_len)) : QByteArray();
    const amnezia::ErrorCode ec = mapError(res.error);
    amnezia_gateway_api_json_result_free(&res);
    return ec;
}

amnezia::ErrorCode GatewayController::exportNativeConfig(const GatewayRequest &request, const QString &publicKey,
                                                        QString &nativeConfigOut)
{
    auto client = m_controller;
    amnezia_gateway_api_native_config_result res{};
    runBlocking([&]() {
        ReqHold h;
        h.fill(request);
        res = amnezia_gateway_api_export_native_config(client.get(), &h.c, publicKey.toUtf8().constData());
    });
    nativeConfigOut = QString::fromUtf8(res.config ? res.config : "");
    const amnezia::ErrorCode ec = mapError(res.error);
    amnezia_gateway_api_native_config_result_free(&res);
    return ec;
}

amnezia::ErrorCode GatewayController::revokeNativeConfig(const GatewayRequest &request)
{
    auto client = m_controller;
    int err = AMNEZIA_GATEWAY_API_OK;
    runBlocking([&]() {
        ReqHold h;
        h.fill(request);
        err = amnezia_gateway_api_revoke_native_config(client.get(), &h.c);
    });
    return mapError(err);
}

GatewayController::AppStoreResult GatewayController::importServiceFromAppStore(const GatewayRequest &request,
                                                                              const QString &publicKey,
                                                                              const QString &transactionId)
{
    auto client = m_controller;
    amnezia_gateway_api_app_store_result res{};
    runBlocking([&]() {
        ReqHold h;
        h.fill(request);
        res = amnezia_gateway_api_import_from_app_store(client.get(), &h.c, publicKey.toUtf8().constData(),
                                                        transactionId.toUtf8().constData());
    });
    AppStoreResult out;
    out.error = mapError(res.error);
    out.serverConfigJson = QString::fromUtf8(res.server_config_json ? res.server_config_json : "");
    out.vpnKey = QString::fromUtf8(res.vpn_key ? res.vpn_key : "");
    out.crc = res.crc;
    amnezia_gateway_api_app_store_result_free(&res);
    return out;
}

QFuture<QPair<amnezia::ErrorCode, QJsonArray>> GatewayController::getNewsAsync(const QString &locale,
                                                                              const QStringList &userCountryCodes,
                                                                              const QStringList &serviceTypes)
{
    auto client = m_controller;
    const std::string localeStd = locale.toStdString();
    std::vector<std::string> countries;
    for (const QString &c : userCountryCodes) {
        countries.push_back(c.toStdString());
    }
    std::vector<std::string> types;
    for (const QString &t : serviceTypes) {
        types.push_back(t.toStdString());
    }

    return QtConcurrent::run([client, localeStd, countries, types]() -> QPair<amnezia::ErrorCode, QJsonArray> {
        std::vector<const char *> cc;
        for (const auto &s : countries) {
            cc.push_back(s.c_str());
        }
        std::vector<const char *> st;
        for (const auto &s : types) {
            st.push_back(s.c_str());
        }
        amnezia_gateway_api_json_result r = amnezia_gateway_api_get_news(client.get(), localeStd.c_str(),
                                                                         cc.empty() ? nullptr : cc.data(), cc.size(),
                                                                         st.empty() ? nullptr : st.data(), st.size());
        const amnezia::ErrorCode ec = mapError(r.error);
        QJsonArray arr;
        if (ec == amnezia::ErrorCode::NoError && r.json != nullptr) {
            arr = QJsonDocument::fromJson(QByteArray(r.json, static_cast<int>(r.json_len))).array();
        }
        amnezia_gateway_api_json_result_free(&r);
        return qMakePair(ec, arr);
    });
}

QFuture<QPair<amnezia::ErrorCode, QString>> GatewayController::getUpdaterEndpointAsync(const QString &cliVersion,
                                                                                     const QString &osVersion,
                                                                                     const QString &installationUuid)
{
    auto client = m_controller;
    const std::string cv = cliVersion.toStdString();
    const std::string ov = osVersion.toStdString();
    const std::string uuid = installationUuid.toStdString();

    return QtConcurrent::run([client, cv, ov, uuid]() -> QPair<amnezia::ErrorCode, QString> {
        amnezia_gateway_api_url_result r = amnezia_gateway_api_get_updater_endpoint(client.get(), cv.c_str(), ov.c_str(), uuid.c_str());
        const QPair<amnezia::ErrorCode, QString> out = qMakePair(mapError(r.error), QString::fromUtf8(r.url ? r.url : ""));
        amnezia_gateway_api_url_result_free(&r);
        return out;
    });
}

QFuture<QPair<amnezia::ErrorCode, QString>> GatewayController::getRenewalLinkAsync(const GatewayRequest &request,
                                                                                  const QString &cliVersion,
                                                                                  const QString &subscriptionStatus)
{
    auto client = m_controller;
    GatewayRequest req = request;
    const std::string cv = cliVersion.toStdString();
    const std::string ss = subscriptionStatus.toStdString();

    return QtConcurrent::run([client, req, cv, ss]() -> QPair<amnezia::ErrorCode, QString> {
        ReqHold h;
        h.fill(req);
        amnezia_gateway_api_url_result r = amnezia_gateway_api_get_renewal_link(client.get(), &h.c, cv.c_str(), ss.c_str());
        const QPair<amnezia::ErrorCode, QString> out = qMakePair(mapError(r.error), QString::fromUtf8(r.url ? r.url : ""));
        amnezia_gateway_api_url_result_free(&r);
        return out;
    });
}
