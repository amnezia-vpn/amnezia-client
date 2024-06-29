#include "apiServicesModel.h"

#include <QJsonObject>

#include "logger.h"

namespace
{
    Logger logger("ApiServicesModel");

    namespace configKey
    {
        constexpr char countryCode[] = "country_code";
        constexpr char servicesDescription[] = "services_description";
        constexpr char services[] = "services";
        constexpr char serviceInfo[] = "service_info";
        constexpr char serviceType[] = "service_type";
        constexpr char serviceProtocol[] = "service_protocol";

        constexpr char description[] = "description";
        constexpr char name[] = "name";
        constexpr char price[] = "price";
    }
}

ApiServicesModel::ApiServicesModel(QObject *parent) : QAbstractListModel(parent)
{
}

int ApiServicesModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_services.size();
}

QVariant ApiServicesModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(rowCount()))
        return QVariant();

    QJsonObject service = m_services.at(index.row()).toObject();
    QJsonObject serviceInfo = service.value(configKey::serviceInfo).toObject();

    switch (role) {
    case NameRole: {
        return serviceInfo.value(configKey::name).toString();
    }
    case DescriptionRole: {
        return serviceInfo.value(configKey::description).toString();
    }
    case PriceRole: {
        return serviceInfo.value(configKey::price).toString();
    }
    }

    return QVariant();
}

void ApiServicesModel::updateModel(const QJsonObject &data)
{
    beginResetModel();

    m_countryCode = data.value(configKey::countryCode).toString();
    m_servicesDescription = data.value(configKey::servicesDescription).toString();
    m_services = data.value(configKey::services).toArray();

    endResetModel();
}

void ApiServicesModel::setServiceIndex(const int index)
{
    m_selectedServiceIndex = index;
}

QJsonObject ApiServicesModel::getSelectedServiceInfo()
{
    QJsonObject service = m_services.at(m_selectedServiceIndex).toObject();
    return service.value(configKey::serviceInfo).toObject();
}

QString ApiServicesModel::getSelectedServiceType()
{
    QJsonObject service = m_services.at(m_selectedServiceIndex).toObject();
    return service.value(configKey::serviceType).toString();
}

QString ApiServicesModel::getSelectedServiceProtocol()
{
    QJsonObject service = m_services.at(m_selectedServiceIndex).toObject();
    return service.value(configKey::serviceProtocol).toString();
}

QString ApiServicesModel::getSelectedServiceName()
{
    auto modelIndex = index(m_selectedServiceIndex, 0);
    return data(modelIndex, ApiServicesModel::Roles::NameRole).toString();
}

QString ApiServicesModel::getCountryCode()
{
    return m_countryCode;
}

QString ApiServicesModel::getServicesDescription()
{
    return m_servicesDescription;
}

QHash<int, QByteArray> ApiServicesModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[NameRole] = "name";
    roles[DescriptionRole] = "description";
    roles[PriceRole] = "price";
    return roles;
}
