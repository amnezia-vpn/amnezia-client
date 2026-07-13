#include "apiAccountInfoModel.h"

#include <QDateTime>
#include <QJsonObject>

#include "core/utils/api/apiUtils.h"
#include "core/utils/serverConfigUtils.h"
#include "logger.h"

namespace
{
    Logger logger("AccountInfoModel");
}

ApiAccountInfoModel::ApiAccountInfoModel(QObject *parent) : QAbstractListModel(parent)
{
}

int ApiAccountInfoModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return 1;
}

QVariant ApiAccountInfoModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(rowCount()))
        return QVariant();

    switch (role) {
    case SubscriptionStatusRole: {
        if (m_accountInfoData.configType == serverConfigUtils::ConfigType::AmneziaFreeV3) {
            return QStringLiteral("<p><a style=\"color: #28c840;\">%1</a>").arg(tr("Active"));
        }

        return apiUtils::isSubscriptionExpired(m_accountInfoData.subscriptionEndDate)
                ? QStringLiteral("<p><a style=\"color: #EB5757;\">%1</a>").arg(tr("Inactive"))
                : QStringLiteral("<p><a style=\"color: #28c840;\">%1</a>").arg(tr("Active"));
    }
    case EndDateRole: {
        if (m_accountInfoData.configType == serverConfigUtils::ConfigType::AmneziaFreeV3) {
            return "";
        }

        return QDateTime::fromString(m_accountInfoData.subscriptionEndDate, Qt::ISODate).toLocalTime().toString("d MMM yyyy");
    }
    case ConnectedDevicesRole: {
        if (m_accountInfoData.configType == serverConfigUtils::ConfigType::AmneziaFreeV3) {
            return "";
        }
        return tr("%1 out of %2").arg(m_accountInfoData.activeDeviceCount).arg(m_accountInfoData.maxDeviceCount);
    }
    case ServiceDescriptionRole: {
        return m_accountInfoData.subscriptionDescription;
    }
    case IsComponentVisibleRole: {
        return m_accountInfoData.configType == serverConfigUtils::ConfigType::AmneziaPremiumV2
                || m_accountInfoData.configType == serverConfigUtils::ConfigType::ExternalPremium;
    }
    case IsSubscriptionRenewalAvailableRole: {
        return m_accountInfoData.isRenewalAvailable;
    }
    case HasExpiredWorkerRole: {
        for (int i = 0; i < m_issuedConfigsInfo.size(); i++) {
            QJsonObject issuedConfigObject = m_issuedConfigsInfo.at(i).toObject();

            auto lastDownloaded = QDateTime::fromString(issuedConfigObject.value(apiDefs::key::lastDownloaded).toString());
            auto workerLastUpdated = QDateTime::fromString(issuedConfigObject.value(apiDefs::key::workerLastUpdated).toString());

            if (lastDownloaded < workerLastUpdated) {
                return true;
            }
        }
        return false;
    }
    case IsProtocolSelectionSupportedRole: {
        if (m_accountInfoData.supportedProtocols.size() > 1) {
            return true;
        }
        return false;
    }
    case IsSubscriptionExpiredRole: {
        if (m_accountInfoData.configType == serverConfigUtils::ConfigType::AmneziaFreeV3) {
            return false;
        }
        if (m_accountInfoData.isInAppPurchase) {
            return false;
        }
        if (m_accountInfoData.subscriptionEndDate.isEmpty()) {
            return false;
        }
        return apiUtils::isSubscriptionExpired(m_accountInfoData.subscriptionEndDate);
    }
    case IsSubscriptionExpiringSoonRole: {
        if (m_accountInfoData.configType == serverConfigUtils::ConfigType::AmneziaFreeV3) {
            return false;
        }
        if (m_accountInfoData.isInAppPurchase) {
            return false;
        }
        if (m_accountInfoData.subscriptionEndDate.isEmpty()) {
            return false;
        }
        return apiUtils::isSubscriptionExpiringSoon(m_accountInfoData.subscriptionEndDate);
    }
    case IsInAppPurchaseRole: {
        return m_accountInfoData.isInAppPurchase;
    }
    }

    return QVariant();
}

void ApiAccountInfoModel::updateModel(const QJsonObject &accountInfoObject, const QJsonObject &serverConfig)
{
    beginResetModel();

    AccountInfoData accountInfoData;

    m_availableCountries = accountInfoObject.value(apiDefs::key::availableCountries).toArray();
    m_issuedConfigsInfo = accountInfoObject.value(apiDefs::key::issuedConfigs).toArray();

    accountInfoData.activeDeviceCount = accountInfoObject.value(apiDefs::key::activeDeviceCount).toInt();
    accountInfoData.maxDeviceCount = accountInfoObject.value(apiDefs::key::maxDeviceCount).toInt();
    accountInfoData.subscriptionEndDate = accountInfoObject.value(apiDefs::key::subscriptionEndDate).toString();

    accountInfoData.configType = serverConfigUtils::configTypeFromJson(serverConfig);

    const QJsonObject apiConfig = serverConfig.value(apiDefs::key::apiConfig).toObject();
    accountInfoData.isInAppPurchase = apiConfig.value(apiDefs::key::isInAppPurchase).toBool(false);

    accountInfoData.subscriptionDescription = accountInfoObject.value(apiDefs::key::subscriptionDescription).toString();
    accountInfoData.isRenewalAvailable = accountInfoObject.value(apiDefs::key::isRenewalAvailable).toBool(false);

    for (const auto &protocol : accountInfoObject.value(apiDefs::key::supportedProtocols).toArray()) {
        accountInfoData.supportedProtocols.push_back(protocol.toString());
    }

    m_accountInfoData = accountInfoData;

    m_supportInfo = accountInfoObject.value(apiDefs::key::supportInfo).toObject();

    endResetModel();
}

QVariant ApiAccountInfoModel::data(const QString &roleString)
{
    QModelIndex modelIndex = index(0);
    auto roles = roleNames();
    for (auto it = roles.begin(); it != roles.end(); it++) {
        if (QString(it.value()) == roleString) {
            return data(modelIndex, it.key());
        }
    }

    return {};
}

QJsonArray ApiAccountInfoModel::getAvailableCountries()
{
    return m_availableCountries;
}

QJsonArray ApiAccountInfoModel::getIssuedConfigsInfo()
{
    return m_issuedConfigsInfo;
}

QString ApiAccountInfoModel::getTelegramBotLink()
{
    return m_supportInfo.value(apiDefs::key::telegram).toString();
}

QString ApiAccountInfoModel::getEmailLink()
{
    return m_supportInfo.value(apiDefs::key::email).toString();
}

QString ApiAccountInfoModel::getBillingEmailLink()
{
    return m_supportInfo.value(apiDefs::key::billingEmail).toString();
}

QString ApiAccountInfoModel::getSiteLink()
{
    return m_supportInfo.value(apiDefs::key::websiteName).toString();
}

QString ApiAccountInfoModel::getFullSiteLink()
{
    return m_supportInfo.value(apiDefs::key::website).toString();
}

QHash<int, QByteArray> ApiAccountInfoModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[SubscriptionStatusRole] = "subscriptionStatus";
    roles[EndDateRole] = "endDate";
    roles[ConnectedDevicesRole] = "connectedDevices";
    roles[ServiceDescriptionRole] = "serviceDescription";
    roles[IsComponentVisibleRole] = "isComponentVisible";
    roles[IsSubscriptionRenewalAvailableRole] = "isSubscriptionRenewalAvailable";
    roles[HasExpiredWorkerRole] = "hasExpiredWorker";
    roles[IsProtocolSelectionSupportedRole] = "isProtocolSelectionSupported";
    roles[IsSubscriptionExpiredRole] = "isSubscriptionExpired";
    roles[IsSubscriptionExpiringSoonRole] = "isSubscriptionExpiringSoon";
    roles[IsInAppPurchaseRole] = "isInAppPurchase";

    return roles;
}
