#include "servicesCatalogController.h"

#include <QJsonDocument>
#include <QSysInfo>

#include "core/controllers/gatewayController.h"
#include "core/api/apiDefs.h"

namespace
{
    constexpr char osVersion[] = "os_version";
}

ServicesCatalogController::ServicesCatalogController(std::shared_ptr<Settings> settings)
    : m_settings(settings)
{
}

ErrorCode ServicesCatalogController::fillAvailableServices(QJsonObject &servicesData)
{
    QJsonObject apiPayload;
    apiPayload[osVersion] = QSysInfo::productType();
    apiPayload[apiDefs::key::appLanguage] = m_settings->getAppLanguage().name().split("_").first();

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
    return ErrorCode::NoError;
}

ErrorCode ServicesCatalogController::executeRequest(const QString &endpoint, const QJsonObject &apiPayload, QByteArray &responseBody)
{
    GatewayController gatewayController(m_settings->getGatewayEndpoint(), m_settings->isDevGatewayEnv(), apiDefs::requestTimeoutMsecs,
                                        m_settings->isStrictKillSwitchEnabled());
    return gatewayController.post(endpoint, apiPayload, responseBody);
}

