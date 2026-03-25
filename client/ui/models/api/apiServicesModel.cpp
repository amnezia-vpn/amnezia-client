#include "apiServicesModel.h"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>

#include "core/api/apiDefs.h"
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

        constexpr char name[] = "name";
        constexpr char price[] = "price";
        constexpr char speed[] = "speed";
        constexpr char timelimit[] = "timelimit";
        constexpr char region[] = "region";

        constexpr char description[] = "description";
        constexpr char cardDescription[] = "card_description";
        constexpr char features[] = "features";

        constexpr char availableCountries[] = "available_countries";

        constexpr char storeEndpoint[] = "store_endpoint";

        constexpr char isAvailable[] = "is_available";

        constexpr char subscriptionPlans[] = "subscription_plans";

        constexpr char premiumBenefitCountriesTitle[] = "premium_benefit_countries_title";
        constexpr char premiumBenefitCountriesBody[] = "premium_benefit_countries_body";
        constexpr char premiumBenefitDevicesTitle[] = "premium_benefit_devices_title";
        constexpr char premiumBenefitDevicesBody[] = "premium_benefit_devices_body";
        constexpr char premiumBenefitVideoTitle[] = "premium_benefit_video_title";
        constexpr char premiumBenefitVideoBody[] = "premium_benefit_video_body";
        constexpr char premiumBenefitTrafficTitle[] = "premium_benefit_traffic_title";
        constexpr char premiumBenefitTrafficBody[] = "premium_benefit_traffic_body";
    }

    QVariantList jsonObjectArrayToVariantList(const QJsonArray &arr)
    {
        QVariantList list;
        list.reserve(arr.size());
        for (const QJsonValue &v : arr) {
            if (!v.isObject()) {
                continue;
            }
            list.append(v.toObject().toVariantMap());
        }
        return list;
    }

    namespace serviceType
    {
        constexpr char amneziaFree[] = "amnezia-free";
        constexpr char amneziaPremium[] = "amnezia-premium";
        constexpr char amneziaTrial[] = "amnezia-trial";
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
        if (serviceType == serviceType::amneziaPremium || serviceType == serviceType::amneziaTrial) {
            return apiServiceData.serviceInfo.cardDescription.arg(speed);
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
        return apiServiceData.serviceInfo.features;
    }
    case PriceRole: {
        auto price = apiServiceData.serviceInfo.price;
        if (price == "free") {
            return tr("Free");
        }
#if defined(Q_OS_IOS) || defined(MACOS_NE)
        return tr("%1 $").arg(price);
#else
        return tr("%1 $/month").arg(price);
#endif
    }
    case EndDateRole: {
        return QDateTime::fromString(apiServiceData.subscription.endDate, Qt::ISODate).toLocalTime().toString("d MMM yyyy");
    }
    case OrderRole: {
        if (serviceType == serviceType::amneziaPremium) {
            return 0;
        }
        if (serviceType == serviceType::amneziaTrial) {
            return 1;
        }
        if (serviceType == serviceType::amneziaFree) {
            return 2;
        }
        return QVariant();
    }
    case SubscriptionPlansRole: {
        return apiServiceData.subscriptionPlans;
    }
    case PremiumBenefitPanelRowsRole: {
        return ApiServicesModel::buildPremiumBenefitPanelRows(apiServiceData);
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

int ApiServicesModel::serviceIndexForType(const QString &type) const
{
    for (int i = 0; i < m_services.size(); ++i) {
        if (m_services.at(i).type == type) {
            return i;
        }
    }
    return -1;
}

QVariant ApiServicesModel::getServiceFieldForType(const QString &type, const QString &roleString) const
{
    const int row = serviceIndexForType(type);
    if (row < 0) {
        return {};
    }
    const QModelIndex modelIndex = index(row);
    const auto roles = roleNames();
    for (auto it = roles.begin(); it != roles.end(); ++it) {
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
    roles[OrderRole] = "order";
    roles[SubscriptionPlansRole] = "subscriptionPlans";
    roles[PremiumBenefitPanelRowsRole] = "premiumBenefitRows";

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
    serviceData.serviceInfo.name = serviceInfo.value(configKey::name).toString();
    serviceData.serviceInfo.price = serviceInfo.value(configKey::price).toString();
    serviceData.serviceInfo.region = serviceInfo.value(configKey::region).toString();
    serviceData.serviceInfo.speed = serviceInfo.value(configKey::speed).toString();
    serviceData.serviceInfo.timeLimit = serviceInfo.value(configKey::timelimit).toString();

    serviceData.serviceInfo.cardDescription = serviceDescription.value(configKey::cardDescription).toString();
    serviceData.serviceInfo.description = serviceDescription.value(configKey::description).toString();
    serviceData.serviceInfo.features = serviceDescription.value(configKey::features).toString();

    serviceData.subscriptionPlans = jsonObjectArrayToVariantList(serviceDescription.value(configKey::subscriptionPlans).toArray());

    serviceData.premiumBenefitCountriesTitle = serviceDescription.value(configKey::premiumBenefitCountriesTitle).toString();
    serviceData.premiumBenefitCountriesBody = serviceDescription.value(configKey::premiumBenefitCountriesBody).toString();
    serviceData.premiumBenefitDevicesTitle = serviceDescription.value(configKey::premiumBenefitDevicesTitle).toString();
    serviceData.premiumBenefitDevicesBody = serviceDescription.value(configKey::premiumBenefitDevicesBody).toString();
    serviceData.premiumBenefitVideoTitle = serviceDescription.value(configKey::premiumBenefitVideoTitle).toString();
    serviceData.premiumBenefitVideoBody = serviceDescription.value(configKey::premiumBenefitVideoBody).toString();
    serviceData.premiumBenefitTrafficTitle = serviceDescription.value(configKey::premiumBenefitTrafficTitle).toString();
    serviceData.premiumBenefitTrafficBody = serviceDescription.value(configKey::premiumBenefitTrafficBody).toString();

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

QVariantList ApiServicesModel::buildPremiumBenefitPanelRows(const ApiServicesData &service)
{
    const QStringList icons = { QStringLiteral("qrc:/images/controls/globe-2.svg"),
                                QStringLiteral("qrc:/images/controls/smartphone.svg"),
                                QStringLiteral("qrc:/images/controls/gauge.svg"),
                                QStringLiteral("qrc:/images/controls/infinity.svg") };

    QVariantList out;

    const auto appendRow = [&out, &icons](int iconIndex, const QString &title, const QString &body) {
        if (title.isEmpty() && body.isEmpty()) {
            return;
        }
        QVariantMap m;
        m.insert(QStringLiteral("icon"), icons.at(iconIndex));
        m.insert(QStringLiteral("title"), title);
        m.insert(QStringLiteral("body"), body);
        out.append(m);
    };

    appendRow(0, service.premiumBenefitCountriesTitle, service.premiumBenefitCountriesBody);
    appendRow(1, service.premiumBenefitDevicesTitle, service.premiumBenefitDevicesBody);
    appendRow(2, service.premiumBenefitVideoTitle, service.premiumBenefitVideoBody);
    appendRow(3, service.premiumBenefitTrafficTitle, service.premiumBenefitTrafficBody);
    return out;
}
