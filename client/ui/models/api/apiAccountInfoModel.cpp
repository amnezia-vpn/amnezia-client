#include "apiAccountInfoModel.h"

#include <QJsonObject>

#include "core/api/apiUtils.h"
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
        if (m_accountInfoData.configType == apiDefs::ConfigType::AmneziaFreeV3) {
            return tr("Active");
        }

        return apiUtils::isSubscriptionExpired(m_accountInfoData.subscriptionEndDate) ? tr("Inactive") : tr("Active");
    }
    case EndDateRole: {
        if (m_accountInfoData.configType == apiDefs::ConfigType::AmneziaFreeV3) {
            return "";
        }

        return QDateTime::fromString(m_accountInfoData.subscriptionEndDate, Qt::ISODate).toLocalTime().toString("d MMM yyyy");
    }
    case ConnectedDevicesRole: {
        if (m_accountInfoData.configType == apiDefs::ConfigType::AmneziaFreeV3) {
            return "";
        }
        return tr("%1 out of %2").arg(m_accountInfoData.activeDeviceCount).arg(m_accountInfoData.maxDeviceCount);
    }
    case ServiceDescriptionRole: {
        if (m_accountInfoData.configType == apiDefs::ConfigType::AmneziaPremiumV2) {
            return tr("Classic VPN for seamless work, downloading large files, and watching videos. Access all websites and online "
                      "resources. "
                      "Speeds up to 200 Mbps");
        } else if (m_accountInfoData.configType == apiDefs::ConfigType::AmneziaFreeV3) {
            return tr("Free unlimited access to a basic set of websites such as Facebook, Instagram, Twitter (X), Discord, Telegram and "
                      "more. YouTube is not included in the free plan.");
        } else {
            return "";
        }
    }
    case IsComponentVisibleRole: {
        return m_accountInfoData.configType == apiDefs::ConfigType::AmneziaPremiumV2
                || m_accountInfoData.configType == apiDefs::ConfigType::ExternalPremium;
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

    accountInfoData.configType = apiUtils::getConfigType(serverConfig);

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
    roles[HasExpiredWorkerRole] = "hasExpiredWorker";
    roles[IsProtocolSelectionSupportedRole] = "isProtocolSelectionSupported";

    return roles;
}
