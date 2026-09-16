#include "apiCountryModel.h"

#include <QJsonObject>

#include "core/utils/api/apiUtils.h"
#include "core/utils/serverConfigUtils.h"
#include "core/utils/constants/apiKeys.h"
#include "core/utils/constants/apiConstants.h"
#include "logger.h"

namespace
{
    Logger logger("ApiCountryModel");

    constexpr QLatin1String countryConfig("country_config");
}

ApiCountryModel::ApiCountryModel(QObject *parent) : QAbstractListModel(parent)
{
}

int ApiCountryModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_countries.size();
}

QVariant ApiCountryModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(rowCount()))
        return QVariant();

    CountryInfo countryInfo = m_countries.at(index.row());
    IssuedConfigInfo issuedConfigInfo = m_issuedConfigs.value(countryInfo.countryCode);
    bool isIssued = issuedConfigInfo.sourceType == countryConfig;

    switch (role) {
    case CountryCodeRole: {
        return countryInfo.countryCode;
    }
    case CountryNameRole: {
        return countryInfo.countryName;
    }
    case CountryIsoCodeRole:
    case CountryImageCodeRole: {
        return apiUtils::getCountryFlagCode(countryInfo.countryCodeL10n, countryInfo.countryCode);
    }
    case IsIssuedRole: {
        return isIssued;
    }
    case IsWorkerExpiredRole: {
        return isWorkerExpired(issuedConfigInfo);
    }
    }

    return QVariant();
}

const QVector<ApiCountryModel::CountryInfo> &ApiCountryModel::countries() const
{
    return m_countries;
}

bool ApiCountryModel::isWorkerExpired(const IssuedConfigInfo &issuedConfigInfo) const
{
    if (issuedConfigInfo.workerLastUpdated.isEmpty()) {
        return false;
    }
    return issuedConfigInfo.lastDownloaded < issuedConfigInfo.workerLastUpdated;
}

bool ApiCountryModel::hasExpiredWorkerConfigs() const
{
    for (const CountryInfo &countryInfo : m_countries) {
        const IssuedConfigInfo issuedConfigInfo = m_issuedConfigs.value(countryInfo.countryCode);
        if (issuedConfigInfo.sourceType == countryConfig && isWorkerExpired(issuedConfigInfo)) {
            return true;
        }
    }
    return false;
}

void ApiCountryModel::updateModel(const QJsonArray &countries, const QString &currentCountryCode)
{
    beginResetModel();

    m_countries.clear();
    m_currentIndex = -1;

    for (int i = 0; i < countries.size(); i++) {
        CountryInfo countryInfo;
        QJsonObject countryObject = countries.at(i).toObject();

        countryInfo.countryName = countryObject.value(apiDefs::key::serverCountryName).toString();
        countryInfo.countryCode = countryObject.value(apiDefs::key::serverCountryCode).toString();
        countryInfo.countryCodeL10n = countryObject.value(apiDefs::key::serverCountryCodeL10n).toString();

        if (countryInfo.countryCode == currentCountryCode) {
            m_currentIndex = i;
        }
        m_countries.push_back(countryInfo);
    }

    endResetModel();

    emit currentIndexChanged(m_currentIndex);
    emit issuedConfigsChanged();
}

void ApiCountryModel::updateIssuedConfigsInfo(const QJsonArray &issuedConfigs)
{
    beginResetModel();

    m_issuedConfigs.clear();
    for (int i = 0; i < issuedConfigs.size(); i++) {
        IssuedConfigInfo issuedConfigInfo;
        QJsonObject issuedConfigObject = issuedConfigs.at(i).toObject();

        if (issuedConfigObject.value(apiDefs::key::sourceType).toString() != countryConfig) {
            continue;
        }

        issuedConfigInfo.installationUuid = issuedConfigObject.value(apiDefs::key::installationUuid).toString();
        issuedConfigInfo.workerLastUpdated = issuedConfigObject.value(apiDefs::key::workerLastUpdated).toString();
        issuedConfigInfo.lastDownloaded = issuedConfigObject.value(apiDefs::key::lastDownloaded).toString();
        issuedConfigInfo.sourceType = issuedConfigObject.value(apiDefs::key::sourceType).toString();
        issuedConfigInfo.osVersion = issuedConfigObject.value(apiDefs::key::osVersion).toString();

        m_issuedConfigs.insert(issuedConfigObject.value(apiDefs::key::serverCountryCode).toString(), issuedConfigInfo);
    }

    endResetModel();
    emit issuedConfigsChanged();
}

int ApiCountryModel::getCurrentIndex()
{
    return m_currentIndex;
}

void ApiCountryModel::setCurrentIndex(const int i)
{
    m_currentIndex = i;
    emit currentIndexChanged(m_currentIndex);
}

QHash<int, QByteArray> ApiCountryModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[CountryNameRole] = "countryName";
    roles[CountryCodeRole] = "countryCode";
    roles[CountryIsoCodeRole] = "countryIsoCode";
    roles[CountryImageCodeRole] = "countryImageCode";
    roles[IsIssuedRole] = "isIssued";
    roles[IsWorkerExpiredRole] = "isWorkerExpired";
    return roles;
}
