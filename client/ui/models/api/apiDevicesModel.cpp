#include "apiDevicesModel.h"

#include <QJsonObject>

#include "core/api/apiDefs.h"
#include "logger.h"

namespace
{
    Logger logger("ApiDevicesModel");

    constexpr QLatin1String gatewayAccount("gateway_account");
}

ApiDevicesModel::ApiDevicesModel(std::shared_ptr<Settings> settings, QObject *parent) : m_settings(settings), QAbstractListModel(parent)
{
}

int ApiDevicesModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_issuedConfigs.size();
}

QVariant ApiDevicesModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(rowCount()))
        return QVariant();

    IssuedConfigInfo issuedConfigInfo = m_issuedConfigs.at(index.row());

    switch (role) {
    case OsVersionRole: {
        return issuedConfigInfo.osVersion;
    }
    case SupportTagRole: {
        return issuedConfigInfo.installationUuid;
    }
    case CountryCodeRole: {
        return issuedConfigInfo.countryCode;
    }
    case LastUpdateRole: {
        return QDateTime::fromString(issuedConfigInfo.lastDownloaded, Qt::ISODate).toLocalTime().toString("d MMM yyyy");
    }
    case IsCurrentDeviceRole: {
        return issuedConfigInfo.installationUuid == m_settings->getInstallationUuid(false);
    }
    }

    return QVariant();
}

void ApiDevicesModel::updateModel(const QJsonArray &issuedConfigs)
{
    beginResetModel();

    m_issuedConfigs.clear();
    for (int i = 0; i < issuedConfigs.size(); i++) {
        IssuedConfigInfo issuedConfigInfo;
        QJsonObject issuedConfigObject = issuedConfigs.at(i).toObject();

        if (issuedConfigObject.value(apiDefs::key::sourceType).toString() != gatewayAccount) {
            continue;
        }

        issuedConfigInfo.installationUuid = issuedConfigObject.value(apiDefs::key::installationUuid).toString();
        issuedConfigInfo.workerLastUpdated = issuedConfigObject.value(apiDefs::key::workerLastUpdated).toString();
        issuedConfigInfo.lastDownloaded = issuedConfigObject.value(apiDefs::key::lastDownloaded).toString();
        issuedConfigInfo.sourceType = issuedConfigObject.value(apiDefs::key::sourceType).toString();
        issuedConfigInfo.osVersion = issuedConfigObject.value(apiDefs::key::osVersion).toString();

        issuedConfigInfo.countryName = issuedConfigObject.value(apiDefs::key::serverCountryName).toString();
        issuedConfigInfo.countryCode = issuedConfigObject.value(apiDefs::key::serverCountryCode).toString();

        m_issuedConfigs.push_back(issuedConfigInfo);
    }

    endResetModel();
}

QHash<int, QByteArray> ApiDevicesModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[OsVersionRole] = "osVersion";
    roles[SupportTagRole] = "supportTag";
    roles[CountryCodeRole] = "countryCode";
    roles[LastUpdateRole] = "lastUpdate";
    roles[IsCurrentDeviceRole] = "isCurrentDevice";
    return roles;
}
