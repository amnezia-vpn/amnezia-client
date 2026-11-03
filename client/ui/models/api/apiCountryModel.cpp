#include "apiCountryModel.h"

#include <QJsonObject>

#include "core/repositories/secureAppSettingsRepository.h"
#include "core/utils/api/apiUtils.h"
#include "core/utils/errorStrings.h"
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

void ApiCountryModel::setAppSettingsRepository(SecureAppSettingsRepository *settings)
{
    if (m_appSettingsRepository == settings) {
        return;
    }

    if (m_appSettingsRepository) {
        disconnect(m_appSettingsRepository, nullptr, this, nullptr);
    }

    m_appSettingsRepository = settings;
    if (m_appSettingsRepository) {
        connect(m_appSettingsRepository, &SecureAppSettingsRepository::connectionHistoryChanged,
                this, [this](const QString &serverId) {
                    if (serverId == m_serverId) {
                        refreshConnectionHistory();
                    }
                });
    }
    refreshConnectionHistory();
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
    case CountryImageCodeRole: {
        return apiUtils::getCountryFlagCode(countryInfo.countryCodeL10n, countryInfo.countryCode);
    }
    case IsIssuedRole: {
        return isIssued;
    }
    case IsWorkerExpiredRole: {
        return isWorkerExpired(issuedConfigInfo);
    }
    case AvailableProtocolsRole: {
        return countryInfo.availableProtocols;
    }
    case LastConnectionStateRole: {
        const QJsonObject entry = connectionHistoryEntry(countryInfo.countryCode);
        if (entry.isEmpty()) {
            return 0;
        }
        return entry.value("lastError").toInt(0) != 0 ? 2 : 1;
    }
    case LastConnectionAtRole: {
        return connectionHistoryEntry(countryInfo.countryCode).value("lastAttemptAt").toString();
    }
    case LastSuccessAtRole: {
        return connectionHistoryEntry(countryInfo.countryCode).value("lastSuccessAt").toString();
    }
    case LastConnectionSetupMsRole: {
        return connectionHistoryEntry(countryInfo.countryCode).value("setupMs").toInt();
    }
    case LastConnectionErrorRole: {
        const int errorCode = connectionHistoryEntry(countryInfo.countryCode).value("lastError").toInt(0);
        return errorCode != 0 ? errorString(static_cast<ErrorCode>(errorCode)) : QString();
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

void ApiCountryModel::updateModel(const QJsonArray &countries, const QString &currentCountryCode,
                                  const QString &userCountryCode)
{
    beginResetModel();

    m_countries.clear();
    m_currentCountryCode = currentCountryCode;
    m_userCountryCode = userCountryCode.toUpper();
    m_currentIndex = -1;

    for (int i = 0; i < countries.size(); i++) {
        CountryInfo countryInfo;
        QJsonObject countryObject = countries.at(i).toObject();

        countryInfo.countryName = countryObject.value(apiDefs::key::serverCountryName).toString();
        countryInfo.countryCode = countryObject.value(apiDefs::key::serverCountryCode).toString();
        countryInfo.countryCodeL10n = countryObject.value(apiDefs::key::serverCountryCodeL10n).toString();
        for (const QJsonValue &protocol : countryObject.value(apiDefs::key::availableProtocols).toArray()) {
            countryInfo.availableProtocols.append(protocol.toString());
        }

        if (countryInfo.countryCode == currentCountryCode) {
            m_currentIndex = i;
        }
        m_countries.push_back(countryInfo);
    }

    endResetModel();

    refreshConnectionHistory();

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

QString ApiCountryModel::getCurrentCountryCode() const
{
    return m_currentCountryCode;
}

QString ApiCountryModel::getUserCountryCode() const
{
    return m_userCountryCode;
}

QString ApiCountryModel::serverId() const
{
    return m_serverId;
}

void ApiCountryModel::setServerId(const QString &serverId)
{
    if (m_serverId == serverId) {
        return;
    }
    m_serverId = serverId;
    refreshConnectionHistory();
    emit serverIdChanged(m_serverId);
}

int ApiCountryModel::getCurrentIndex()
{
    return m_currentIndex;
}

QHash<int, QByteArray> ApiCountryModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[CountryNameRole] = "countryName";
    roles[CountryCodeRole] = "countryCode";
    roles[CountryImageCodeRole] = "countryImageCode";
    roles[IsIssuedRole] = "isIssued";
    roles[IsWorkerExpiredRole] = "isWorkerExpired";
    roles[AvailableProtocolsRole] = "availableProtocols";
    roles[LastConnectionStateRole] = "lastConnectionState";
    roles[LastConnectionAtRole] = "lastConnectionAt";
    roles[LastSuccessAtRole] = "lastSuccessAt";
    roles[LastConnectionSetupMsRole] = "lastConnectionSetupMs";
    roles[LastConnectionErrorRole] = "lastConnectionError";
    return roles;
}

QJsonObject ApiCountryModel::connectionHistoryEntry(const QString &countryCode) const
{
    if (countryCode.isEmpty() || m_connectionHistory.isEmpty()) {
        return {};
    }
    return m_connectionHistory.value(countryCode).toObject();
}

void ApiCountryModel::refreshConnectionHistory()
{
    m_connectionHistory = m_appSettingsRepository && !m_serverId.isEmpty()
            ? m_appSettingsRepository->connectionHistory(m_serverId)
            : QJsonObject();

    if (!m_countries.isEmpty()) {
        emit dataChanged(index(0), index(static_cast<int>(m_countries.size()) - 1));
    }
}
