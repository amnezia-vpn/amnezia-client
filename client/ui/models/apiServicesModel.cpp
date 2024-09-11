#include "apiServicesModel.h"

#include <QJsonObject>

#include "logger.h"

namespace
{
    Logger logger("ApiServicesModel");

    namespace configKey
    {
        constexpr char userCountryCode[] = "user_country_code";
        constexpr char services[] = "services";
        constexpr char serviceInfo[] = "service_info";
        constexpr char serviceType[] = "service_type";
        constexpr char serviceProtocol[] = "service_protocol";

        constexpr char name[] = "name";
        constexpr char price[] = "price";
        constexpr char speed[] = "speed";
        constexpr char timelimit[] = "timelimit";
        constexpr char region[] = "region";

        constexpr char availableCountries[] = "available_countries";

        constexpr char storeEndpoint[] = "store_endpoint";

        constexpr char isAvailable[] = "is_available";
    }

    namespace serviceType
    {
        constexpr char amneziaFree[] = "amnezia-free";
        constexpr char amneziaPremium[] = "amnezia-premium";
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
    auto serviceType = service.value(configKey::serviceType).toString();

    switch (role) {
    case NameRole: {
        return serviceInfo.value(configKey::name).toString();
    }
    case CardDescriptionRole: {
        auto speed = serviceInfo.value(configKey::speed).toString();
        if (serviceType == serviceType::amneziaPremium) {
            return tr("Classic VPN for comfortable work, downloading large files and watching videos. "
                      "Works for any sites. Speed up to %1 MBit/s")
                    .arg(speed);
        } else if (serviceType == serviceType::amneziaFree){
            QString description = tr("VPN to access blocked sites in regions with high levels of Internet censorship. ");
            if (service.value(configKey::isAvailable).isBool() && !service.value(configKey::isAvailable).toBool()) {
                description += tr("<p><a style=\"color: #EB5757;\">Not available in your region. If you have VPN enabled, disable it, return to the previous screen, and try again.</a>");
            }
            return description;
        }
    }
    case ServiceDescriptionRole: {
        if (serviceType == serviceType::amneziaPremium) {
            return tr("Amnezia Premium - A classic VPN for comfortable work, downloading large files, and watching videos in high resolution. "
                      "It works for all websites, even in countries with the highest level of internet censorship.");
        } else {
            return tr("Amnezia Free is a free VPN to bypass blocking in countries with high levels of internet censorship");
        }
    }
    case IsServiceAvailableRole: {
        if (serviceType == serviceType::amneziaFree) {
            if (service.value(configKey::isAvailable).isBool() && !service.value(configKey::isAvailable).toBool()) {
                return false;
            }
        }
        return true;
    }
    case SpeedRole: {
        auto speed = serviceInfo.value(configKey::speed).toString();
        return tr("%1 MBit/s").arg(speed);
    }
    case WorkPeriodRole: {
        auto timelimit = serviceInfo.value(configKey::timelimit).toString();
        if (timelimit == "0") {
            return "";
        }
        return tr("%1 days").arg(timelimit);
    }
    case RegionRole: {
        return serviceInfo.value(configKey::region).toString();
    }
    case FeaturesRole: {
        if (serviceType == serviceType::amneziaPremium) {
            return tr("");
        } else {
            return tr("VPN will open only popular sites blocked in your region, such as Instagram, Facebook, Twitter and others. "
                      "Other sites will be opened from your real IP address, "
                      "<a href=\"%1/free\" style=\"color: #FBB26A;\">more details on the website.</a>");
        }
    }
    case PriceRole: {
        auto price = serviceInfo.value(configKey::price).toString();
        if (price == "free") {
            return tr("Free");
        }
        return tr("%1 $/month").arg(price);
    }
    }

    return QVariant();
}

void ApiServicesModel::updateModel(const QJsonObject &data)
{
    beginResetModel();

    m_countryCode = data.value(configKey::userCountryCode).toString();
    m_services = data.value(configKey::services).toArray();
    if (m_services.isEmpty()) {
        QJsonObject service;
        service.insert(configKey::serviceInfo, data.value(configKey::serviceInfo));
        service.insert(configKey::serviceType, data.value(configKey::serviceType));

        m_services.push_back(service);
        m_selectedServiceIndex = 0;
    }

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

QJsonArray ApiServicesModel::getSelectedServiceCountries()
{
    QJsonObject service = m_services.at(m_selectedServiceIndex).toObject();
    return service.value(configKey::availableCountries).toArray();
}

QString ApiServicesModel::getCountryCode()
{
    return m_countryCode;
}

QString ApiServicesModel::getStoreEndpoint()
{
    QJsonObject service = m_services.at(m_selectedServiceIndex).toObject();
    return service.value(configKey::storeEndpoint).toString();
}

QVariant ApiServicesModel::getSelectedServiceData(const QString roleString)
{
    QModelIndex modelIndex = index(m_selectedServiceIndex);
    auto roles = roleNames();
    for (auto it = roles.begin(); it != roles.end(); it++) {
        if (QString(it.value()) == roleString) {
            return data(modelIndex, it.key());
        }
    }

    return {};
}

QHash<int, QByteArray> ApiServicesModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[NameRole] = "name";
    roles[CardDescriptionRole] = "cardDescription";
    roles[ServiceDescriptionRole] = "serviceDescription";
    roles[IsServiceAvailableRole] = "isServiceAvailable";
    roles[SpeedRole] = "speed";
    roles[WorkPeriodRole] = "workPeriod";
    roles[RegionRole] = "region";
    roles[FeaturesRole] = "features";
    roles[PriceRole] = "price";

    return roles;
}
