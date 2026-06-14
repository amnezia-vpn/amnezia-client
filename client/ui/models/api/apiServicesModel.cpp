#include "apiServicesModel.h"

#include <QDateTime>
#include <QHash>
#include <QJsonArray>
#include <QJsonObject>

#include "core/utils/constants/apiKeys.h"
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
        constexpr char serviceDescription[] = "service_description";

        constexpr char description[] = "description";
        constexpr char cardDescription[] = "card_description";
        constexpr char serviceName[] = "service_name";

        constexpr char availableCountries[] = "available_countries";

        constexpr char storeEndpoint[] = "store_endpoint";

        constexpr char isAvailable[] = "is_available";

        constexpr char subscriptionPlans[] = "subscription_plans";
        constexpr char minPriceLabel[] = "min_price_label";
        constexpr char benefits[] = "benefits";
    }

    namespace serviceType
    {
        constexpr char amneziaFree[] = "amnezia-free";
        constexpr char amneziaPremium[] = "amnezia-premium";
    }
}

ApiServicesModel::ApiServicesModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_selectedServiceIndex(0)
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
        if (serviceType == serviceType::amneziaPremium) {
            return apiServiceData.serviceInfo.cardDescription;
        } else if (serviceType == serviceType::amneziaFree) {
            QString description = apiServiceData.serviceInfo.cardDescription;
            if (!isServiceAvailable) {
                description += tr("<p><a style=\"color: #EB5757;\">Not available in your region. If you have VPN enabled, disable it, "
                                  "return to the previous screen, and try again.</a>");
            }
            return description;
        }
    }
    case ServiceDescriptionRole: {
        return apiServiceData.serviceInfo.description;
    }
    case IsServiceAvailableRole: {
        if (serviceType == serviceType::amneziaFree) {
            if (!isServiceAvailable) {
                return false;
            }
        }
        return true;
    }
    case IsPremiumRole: {
        return serviceType == serviceType::amneziaPremium;
    }
    case HasSubscriptionPlansRole: {
        return !apiServiceData.subscriptionPlansJson.isEmpty();
    }
    case PriceRole: {
        return apiServiceData.minPriceLabel;
    }
    case EndDateRole: {
        return QDateTime::fromString(apiServiceData.subscription.endDate, Qt::ISODate).toLocalTime().toString("d MMM yyyy");
    }
    case TermsOfUseUrlRole: {
        return apiServiceData.serviceInfo.termsOfUseUrl;
    }
    case PrivacyPolicyUrlRole: {
        return apiServiceData.serviceInfo.privacyPolicyUrl;
    }
    case ShowRecommendedRole: {
        return serviceType == serviceType::amneziaPremium;
    }
    case OrderRole: {
        if (serviceType == serviceType::amneziaPremium) {
            return 0;
        }
        if (serviceType == serviceType::amneziaFree) {
            return 1;
        }
        return QVariant();
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
            auto serviceObject = service.toObject();
            m_services.push_back(getApiServicesData(serviceObject));
        }
    }

    if (!m_services.isEmpty() && m_selectedServiceIndex >= m_services.size()) {
        m_selectedServiceIndex = 0;
    }

    endResetModel();

    emit serviceSelectionChanged();
}

void ApiServicesModel::setServiceIndex(const int index)
{
    m_selectedServiceIndex = index;
    emit serviceSelectionChanged();
}

ApiServicesModel::ApiServicesData ApiServicesModel::selectedServiceData() const
{
    if (m_services.isEmpty() || m_selectedServiceIndex < 0 || m_selectedServiceIndex >= m_services.size()) {
        return {};
    }
    return m_services.at(m_selectedServiceIndex);
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

int ApiServicesModel::serviceIndexForType(const QString &type) const
{
    for (int serviceIndex = 0; serviceIndex < m_services.size(); ++serviceIndex) {
        if (m_services.at(serviceIndex).type == type) {
            return serviceIndex;
        }
    }
    return -1;
}

QHash<int, QByteArray> ApiServicesModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[NameRole] = "name";
    roles[CardDescriptionRole] = "cardDescription";
    roles[ServiceDescriptionRole] = "serviceDescription";
    roles[IsServiceAvailableRole] = "isServiceAvailable";
    roles[IsPremiumRole] = "isPremium";
    roles[HasSubscriptionPlansRole] = "hasSubscriptionPlans";
    roles[PriceRole] = "price";
    roles[EndDateRole] = "endDate";
    roles[TermsOfUseUrlRole] = "termsOfUseUrl";
    roles[PrivacyPolicyUrlRole] = "privacyPolicyUrl";
    roles[ShowRecommendedRole] = "showRecommended";
    roles[OrderRole] = "order";

    return roles;
}

ApiServicesModel::ApiServicesData ApiServicesModel::getApiServicesData(const QJsonObject &data)
{
    auto serviceInfo = data.value(configKey::serviceInfo).toObject();
    auto serviceType = data.value(configKey::serviceType).toString();
    auto serviceProtocol = data.value(configKey::serviceProtocol).toString();
    auto availableCountries = data.value(configKey::availableCountries).toArray();
    auto serviceDescription = data.value(configKey::serviceDescription).toObject();

    auto subscriptionObject = data.value(apiDefs::key::subscription).toObject();

    ApiServicesData serviceData;
    serviceData.serviceInfo.name = serviceDescription.value(configKey::serviceName).toString();

    serviceData.serviceInfo.cardDescription = serviceDescription.value(configKey::cardDescription).toString();
    serviceData.serviceInfo.description = serviceDescription.value(configKey::description).toString();
    serviceData.serviceInfo.termsOfUseUrl = serviceDescription.value(apiDefs::key::termsOfUseUrl).toString();
    serviceData.serviceInfo.privacyPolicyUrl = serviceDescription.value(apiDefs::key::privacyPolicyUrl).toString();

    serviceData.subscriptionPlansJson = serviceDescription.value(configKey::subscriptionPlans).toArray();
    serviceData.benefits = serviceDescription.value(configKey::benefits).toArray();

    serviceData.minPriceLabel = serviceDescription.value(configKey::minPriceLabel).toString().trimmed();

    serviceData.supportInfo = data.value(apiDefs::key::supportInfo).toObject();

    serviceData.type = serviceType;
    serviceData.protocol = serviceProtocol;

    serviceData.storeEndpoint = data.value(configKey::storeEndpoint).toString();

    if (data.value(configKey::isAvailable).isBool()) {
        serviceData.isServiceAvailable = data.value(configKey::isAvailable).toBool();
    } else {
        serviceData.isServiceAvailable = true;
    }

    serviceData.serviceInfo.object = serviceInfo;
    serviceData.availableCountries = availableCountries;

    serviceData.subscription.endDate = subscriptionObject.value(apiDefs::key::endDate).toString();

    return serviceData;
}
