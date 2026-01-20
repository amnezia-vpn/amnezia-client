#include "apiNewsController.h"

#include "core/api/apiUtils.h"
#include "core/networkUtilities.h"
#include <QJsonDocument>
#include <QJsonObject>

namespace
{
    namespace configKey
    {
        constexpr char userCountryCode[] = "user_country_code";
        constexpr char serviceType[] = "service_type";
    }
}

ApiNewsController::ApiNewsController(const QSharedPointer<NewsModel> &newsModel, const std::shared_ptr<Settings> &settings,
                                     const QSharedPointer<ServersModel> &serversModel, QObject *parent)
    : QObject(parent), m_newsModel(newsModel), m_settings(settings), m_serversModel(serversModel)
{
}

void ApiNewsController::fetchNews(bool showError)
{
    if (m_serversModel.isNull()) {
        qWarning() << "ServersModel is null, skip fetchNews";
        return;
    }
    const auto stacks = m_serversModel->gatewayStacks();
    if (stacks.isEmpty()) {
        qDebug() << "No Gateway stacks, skip fetchNews";
        return;
    }

    auto gatewayController = QSharedPointer<GatewayController>::create(m_settings->getGatewayEndpoint(), m_settings->isDevGatewayEnv(),
                                                                       apiDefs::requestTimeoutMsecs, m_settings->isStrictKillSwitchEnabled());
    QJsonObject payload;
    payload.insert("locale", m_settings->getAppLanguage().name().split("_").first());

    const QJsonObject stacksJson = stacks.toJson();
    if (stacksJson.contains(configKey::userCountryCode)) {
        payload.insert(configKey::userCountryCode, stacksJson.value(configKey::userCountryCode));
    }
    if (stacksJson.contains(configKey::serviceType)) {
        payload.insert(configKey::serviceType, stacksJson.value(configKey::serviceType));
    }

    QString baseDomain = "gateway.example.com";
    QString endpoint = QString("%1v1/news");
    
    // 1. HTTP (primary transport - async)
    auto future = gatewayController->postAsync(endpoint, payload);
    future.then(this, [this, showError, gatewayController, baseDomain, endpoint, payload](QPair<ErrorCode, QByteArray> result) {
        auto [errorCode, responseBody] = result;
        
        // HTTP succeeded
        if (errorCode == ErrorCode::NoError) {
            QJsonDocument doc = QJsonDocument::fromJson(responseBody);
            QJsonArray newsArray;
            if (doc.isArray()) {
                newsArray = doc.array();
            } else if (doc.isObject()) {
                QJsonObject obj = doc.object();
                if (obj.value("news").isArray()) {
                    newsArray = obj.value("news").toArray();
                }
            }
            m_newsModel->updateModel(newsArray);
            emit fetchNewsFinished();
            return;
        }
        
        // HTTP failed, try DNS transports as fallback (synchronous)
        qDebug() << "[Transport] HTTP failed, trying DNS transports as fallback";
        GatewayController dnsGateway(m_settings->getGatewayEndpoint(), m_settings->isDevGatewayEnv(), apiDefs::requestTimeoutMsecs,
                                     m_settings->isStrictKillSwitchEnabled());
        QByteArray dnsResponseBody;
        ErrorCode dnsError = ErrorCode::UnknownError;
        
        // 2. DNS UDP
        dnsGateway.setDnsServer("127.0.0.1", baseDomain, NetworkUtilities::DnsTransport::Udp, 15353);
        dnsError = dnsGateway.postViaDns(endpoint, payload, dnsResponseBody);
        if (dnsError != ErrorCode::NoError) {
            // 3. DNS TCP
            dnsGateway.setDnsServer("127.0.0.1", baseDomain, NetworkUtilities::DnsTransport::Tcp, 15353);
            dnsError = dnsGateway.postViaDns(endpoint, payload, dnsResponseBody);
        }
        if (dnsError != ErrorCode::NoError) {
            // 4. DoT
            dnsGateway.setDnsServer("127.0.0.1", baseDomain, NetworkUtilities::DnsTransport::Tls, 8853);
            dnsError = dnsGateway.postViaDns(endpoint, payload, dnsResponseBody);
        }
        if (dnsError != ErrorCode::NoError) {
            // 5. DoH
            dnsGateway.setDnsServer("127.0.0.1", baseDomain, NetworkUtilities::DnsTransport::Https, 80, "/dns-query");
            dnsError = dnsGateway.postViaDns(endpoint, payload, dnsResponseBody);
        }
        
        if (dnsError != ErrorCode::NoError) {
            emit errorOccurred(dnsError, showError);
            return;
        }
        
        // DNS succeeded
        QJsonDocument doc = QJsonDocument::fromJson(dnsResponseBody);
        QJsonArray newsArray;
        if (doc.isArray()) {
            newsArray = doc.array();
        } else if (doc.isObject()) {
            QJsonObject obj = doc.object();
            if (obj.value("news").isArray()) {
                newsArray = obj.value("news").toArray();
            }
        }
        m_newsModel->updateModel(newsArray);
        emit fetchNewsFinished();
    });
}
