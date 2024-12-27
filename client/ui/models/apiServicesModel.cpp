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

        constexpr char subscription[] = "subscription";
        constexpr char endDate[] = "end_date";
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

    auto apiServiceData = m_services.at(index.row());
    auto serviceType = apiServiceData.type;
    auto isServiceAvailable = apiServiceData.isServiceAvailable;

    switch (role) {
    case NameRole: {
        return apiServiceData.serviceInfo.name;
    }
    case CardDescriptionRole: {
        auto speed = apiServiceData.serviceInfo.speed;
        if (serviceType == serviceType::amneziaPremium) {
            return tr("Classic VPN for comfortable work, downloading large files and watching videos. "
                      "Works for any sites. Speed up to %1 MBit/s")
                    .arg(speed);
        } else if (serviceType == serviceType::amneziaFree){
            QString description = tr("VPN to access blocked sites in regions with high levels of Internet censorship. ");
            if (isServiceAvailable) {
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
            if (isServiceAvailable) {
                return false;
            }
        }
        return true;
    }
    case SpeedRole: {
        return tr("%1 MBit/s").arg(apiServiceData.serviceInfo.speed);
    }
    case TimeLimitRole: {
        auto timeLimit = apiServiceData.serviceInfo.timeLimit;
        if (timeLimit == "0") {
            return "";
        }
        return tr("%1 days").arg(timeLimit);
    }
    case RegionRole: {
        return apiServiceData.serviceInfo.region;
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
        auto price = apiServiceData.serviceInfo.price;
        if (price == "free") {
            return tr("Free");
        }
        return tr("%1 $/month").arg(price);
    }
    case EndDateRole: {
        return QDateTime::fromString(apiServiceData.subscription.endDate, Qt::ISODate).toLocalTime().toString("d MMM yyyy");
    }
    }

    return QVariant();
}

void ApiServicesModel::updateModel(const QJsonObject &data)
{
    beginResetModel();

    m_services.clear();

    m_countryCode = data.value(configKey::userCountryCode).toString();
    auto services = data.value(configKey::services).toArray();

    if (services.isEmpty()) {
        m_services.push_back(getApiServicesData(data));
        m_selectedServiceIndex = 0;
    } else {
        for (const auto &service : services) {
            m_services.push_back(getApiServicesData(service.toObject()));
        }
    }

    endResetModel();
}

void ApiServicesModel::setServiceIndex(const int index)
{
    m_selectedServiceIndex = index;
}

QJsonObject ApiServicesModel::getSelectedServiceInfo()
{
    auto service = m_services.at(m_selectedServiceIndex);
    return service.serviceInfo.object;
}

QString ApiServicesModel::getSelectedServiceType()
{
    auto service = m_services.at(m_selectedServiceIndex);
    return service.type;
}

QString ApiServicesModel::getSelectedServiceProtocol()
{
    auto service = m_services.at(m_selectedServiceIndex);
    return service.protocol;
}

QString ApiServicesModel::getSelectedServiceName()
{
    auto service = m_services.at(m_selectedServiceIndex);
    return service.serviceInfo.name;
}

QJsonArray ApiServicesModel::getSelectedServiceCountries()
{
    auto service = m_services.at(m_selectedServiceIndex);
    return service.availableCountries;
}

QString ApiServicesModel::getCountryCode()
{
    return m_countryCode;
}

QString ApiServicesModel::getStoreEndpoint()
{
    auto service = m_services.at(m_selectedServiceIndex);
    return service.storeEndpoint;
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
    roles[TimeLimitRole] = "timeLimit";
    roles[RegionRole] = "region";
    roles[FeaturesRole] = "features";
    roles[PriceRole] = "price";
    roles[EndDateRole] = "endDate";

    return roles;
}

ApiServicesModel::ApiServicesData ApiServicesModel::getApiServicesData(const QJsonObject &data)
{
    auto serviceInfo =  data.value(configKey::serviceInfo).toObject();
    auto serviceType = data.value(configKey::serviceType).toString();
    auto serviceProtocol = data.value(configKey::serviceProtocol).toString();
    auto availableCountries = data.value(configKey::availableCountries).toArray();

    auto subscriptionObject = data.value(configKey::subscription).toObject();

    ApiServicesData serviceData;
    serviceData.serviceInfo.name = serviceInfo.value(configKey::name).toString();
    serviceData.serviceInfo.price = serviceInfo.value(configKey::price).toString();
    serviceData.serviceInfo.region = serviceInfo.value(configKey::region).toString();
    serviceData.serviceInfo.speed = serviceInfo.value(configKey::speed).toString();
    serviceData.serviceInfo.timeLimit = serviceInfo.value(configKey::timelimit).toString();

    serviceData.type = serviceType;
    serviceData.protocol = serviceProtocol;

    serviceData.storeEndpoint = serviceInfo.value(configKey::storeEndpoint).toString();

    if (serviceInfo.value(configKey::isAvailable).isBool()) {
        serviceData.isServiceAvailable = data.value(configKey::isAvailable).toBool();
    } else {
        serviceData.isServiceAvailable = true;
    }

    serviceData.serviceInfo.object = serviceInfo;
    serviceData.availableCountries = availableCountries;

    serviceData.subscription.endDate = subscriptionObject.value(configKey::endDate).toString();

    return serviceData;
}
