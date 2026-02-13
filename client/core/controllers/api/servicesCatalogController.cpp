#include "servicesCatalogController.h"

#include <QJsonDocument>
#include <QSysInfo>
#include <QJsonArray>
#include <QEventLoop>
#include <QDebug>

#include "core/controllers/gatewayController.h"
#include "core/utils/api/apiEnums.h"
#include "core/utils/constants/apiKeys.h"
#include "core/utils/constants/apiConstants.h"

#if defined(Q_OS_IOS) || defined(MACOS_NE)
#include "platforms/ios/ios_controller.h"
#endif

namespace
{
    constexpr char osVersion[] = "os_version";
    
    namespace configKey
    {
        constexpr char serviceType[] = "service_type";
        constexpr char serviceInfo[] = "service_info";
    }

    namespace serviceType
    {
        constexpr char amneziaPremium[] = "amnezia-premium";
    }
}

ServicesCatalogController::ServicesCatalogController(SecureAppSettingsRepository* appSettingsRepository)
    : m_appSettingsRepository(appSettingsRepository)
{
}

ErrorCode ServicesCatalogController::fillAvailableServices(QJsonObject &servicesData)
{
    QJsonObject apiPayload;
    apiPayload[osVersion] = QSysInfo::productType();
    apiPayload[apiDefs::key::appLanguage] = m_appSettingsRepository->getAppLanguage().name().split("_").first();

    QByteArray responseBody;
    ErrorCode errorCode = executeRequest(QString("%1v1/services"), apiPayload, responseBody);
    if (errorCode == ErrorCode::NoError) {
        if (!responseBody.contains("services")) {
            errorCode = ErrorCode::ApiServicesMissingError;
        }
    }

    if (errorCode != ErrorCode::NoError) {
        return errorCode;
    }

    servicesData = QJsonDocument::fromJson(responseBody).object();

#if defined(Q_OS_IOS) || defined(MACOS_NE)
    QEventLoop waitProducts;
    bool productsFetched = false;
    QString productPrice;
    QString productCurrency;
    
    IosController::Instance()->fetchProducts(QStringList() << QStringLiteral("amnezia_premium_6_month"),
                                             [&](const QList<QVariantMap> &products,
                                                 const QStringList &invalidIds,
                                                 const QString &errorString) {
                                                 if (!errorString.isEmpty() || products.isEmpty()) {
                                                     qWarning().noquote() << "[IAP] Failed to fetch product price:" << errorString;
                                                 } else {
                                                     const auto &product = products.first();
                                                     productPrice = product.value("price").toString();
                                                     productCurrency = product.value("currencyCode").toString();
                                                     productsFetched = true;
                                                     qInfo().noquote() << "[IAP] Fetched product price:" << productPrice << productCurrency;
                                                 }
                                                 waitProducts.quit();
                                             });
    waitProducts.exec();
    
    if (productsFetched && !productPrice.isEmpty()) {
        QJsonArray services = servicesData.value("services").toArray();
        for (int i = 0; i < services.size(); ++i) {
            QJsonObject service = services[i].toObject();
            if (service.value(configKey::serviceType).toString() == serviceType::amneziaPremium) {
                QJsonObject serviceInfo = service.value(configKey::serviceInfo).toObject();
                QString formattedPrice = productPrice;
                if (!productCurrency.isEmpty()) {
                    formattedPrice += " " + productCurrency;
                }
                serviceInfo["price"] = formattedPrice;
                service[configKey::serviceInfo] = serviceInfo;
                services[i] = service;
                servicesData["services"] = services;
                qInfo().noquote() << "[IAP] Updated premium service price in data:" << formattedPrice;
                break;
            }
        }
    }
#endif

    return ErrorCode::NoError;
}

ErrorCode ServicesCatalogController::executeRequest(const QString &endpoint, const QJsonObject &apiPayload, QByteArray &responseBody)
{
    GatewayController gatewayController(m_appSettingsRepository->getGatewayEndpoint(), m_appSettingsRepository->isDevGatewayEnv(), apiDefs::requestTimeoutMsecs,
                                        m_appSettingsRepository->isStrictKillSwitchEnabled());
    return gatewayController.post(endpoint, apiPayload, responseBody);
}

