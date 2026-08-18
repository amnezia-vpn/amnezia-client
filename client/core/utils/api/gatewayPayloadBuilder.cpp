#include "gatewayPayloadBuilder.h"

#include <QJsonArray>
#include <QSysInfo>

#include "core/repositories/secureAppSettingsRepository.h"
#include "core/utils/api/apiUtils.h"
#include "core/utils/constants/apiKeys.h"
#include "version.h"

namespace
{
    bool isEmptyValue(const QJsonValue &value)
    {
        switch (value.type()) {
        case QJsonValue::Null:
        case QJsonValue::Undefined:
            return true;
        case QJsonValue::String:
            return value.toString().isEmpty();
        case QJsonValue::Object:
            return value.toObject().isEmpty();
        case QJsonValue::Array:
            return value.toArray().isEmpty();
        default:
            return false;
        }
    }
}

GatewayPayloadBuilder::GatewayPayloadBuilder(const SecureAppSettingsRepository *appSettingsRepository)
{
    m_payload[apiDefs::key::osVersion] = QSysInfo::productType();
    m_payload[apiDefs::key::appVersion] = QString(APP_VERSION);
    m_payload[apiDefs::key::cliName] = QString(APPLICATION_NAME);

    const QString distributionChannel = apiUtils::getDistributionChannel();
    if (!distributionChannel.isEmpty()) {
        m_payload[apiDefs::key::distribution] = distributionChannel;
    }

    if (appSettingsRepository == nullptr) {
        qWarning() << "SecureAppSettingsRepository is null, skip app language and installation uuid";
        return;
    }

    m_payload[apiDefs::key::appLanguage] = apiUtils::getAppLanguageCode(appSettingsRepository);
    m_payload[apiDefs::key::installationUuid] = appSettingsRepository->getInstallationUuid(true);
}

GatewayPayloadBuilder &GatewayPayloadBuilder::addField(QLatin1String key, const QJsonValue &value)
{
    if (!isEmptyValue(value)) {
        m_payload[key] = value;
    }
    return *this;
}

QJsonObject GatewayPayloadBuilder::build() const
{
    return m_payload;
}
