#include "apiCountryModel.h"

#include <QJsonObject>

#include "logger.h"

namespace
{
    Logger logger("ApiCountryModel");

    namespace configKey
    {
        constexpr char serverCountryCode[] = "server_country_code";
        constexpr char serverCountryName[] = "server_country_name";
    }
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

    QJsonObject countryInfo = m_countries.at(index.row()).toObject();

    switch (role) {
    case CountryCodeRole: {
        return countryInfo.value(configKey::serverCountryCode).toString();
    }
    case CountryNameRole: {
        return countryInfo.value(configKey::serverCountryName).toString();
    }
    case CountryImageCodeRole: {
        return countryInfo.value(configKey::serverCountryCode).toString().toUpper();
    }
    }

    return QVariant();
}

void ApiCountryModel::updateModel(const QJsonArray &data, const QString &currentCountryCode)
{
    beginResetModel();

    m_countries = data;
    for (int i = 0; i < m_countries.size(); i++) {
        if (m_countries.at(i).toObject().value(configKey::serverCountryCode).toString() == currentCountryCode) {
            m_currentIndex = i;
            emit currentIndexChanged(m_currentIndex);
            break;
        }
    }

    endResetModel();
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
    roles[CountryImageCodeRole] = "countryImageCode";
    return roles;
}
