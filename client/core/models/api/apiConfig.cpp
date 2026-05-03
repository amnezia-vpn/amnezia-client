#include "apiConfig.h"

#include <QJsonDocument>
#include <QDateTime>

#include "core/utils/api/apiUtils.h"
#include "core/utils/constants/apiKeys.h"

namespace amnezia
{

QJsonObject ApiConfig::Subscription::toJson() const
{
    QJsonObject obj;
    if (!endDate.isEmpty()) {
        obj[apiDefs::key::endDate] = endDate;
    }
    return obj;
}

ApiConfig::Subscription ApiConfig::Subscription::fromJson(const QJsonObject& json)
{
    Subscription sub;
    sub.endDate = json.value(apiDefs::key::endDate).toString();
    return sub;
}

QJsonObject ApiConfig::ServiceInfo::toJson() const
{
    QJsonObject obj;
    obj[apiDefs::key::isAdVisible] = isAdVisible;
    obj[apiDefs::key::isRenewalAvailable] = isRenewalAvailable;
    if (!adHeader.isEmpty()) {
        obj[apiDefs::key::adHeader] = adHeader;
    }
    if (!adDescription.isEmpty()) {
        obj[apiDefs::key::adDescription] = adDescription;
    }
    if (!adEndpoint.isEmpty()) {
        obj[apiDefs::key::adEndpoint] = adEndpoint;
    }
    return obj;
}

ApiConfig::ServiceInfo ApiConfig::ServiceInfo::fromJson(const QJsonObject& json)
{
    ServiceInfo info;
    info.isAdVisible = json.value(apiDefs::key::isAdVisible).toBool(false);
    info.isRenewalAvailable = json.value(apiDefs::key::isRenewalAvailable).toBool(false);
    info.adHeader = json.value(apiDefs::key::adHeader).toString();
    info.adDescription = json.value(apiDefs::key::adDescription).toString();
    info.adEndpoint = json.value(apiDefs::key::adEndpoint).toString();
    return info;
}

QJsonObject ApiConfig::PublicKeyInfo::toJson() const
{
    QJsonObject obj;
    if (!expiresAt.isEmpty()) {
        obj[apiDefs::key::expiresAt] = expiresAt;
    }
    return obj;
}

ApiConfig::PublicKeyInfo ApiConfig::PublicKeyInfo::fromJson(const QJsonObject& json)
{
    PublicKeyInfo info;
    info.expiresAt = json.value(apiDefs::key::expiresAt).toString();
    return info;
}

bool ApiConfig::isPremium() const
{
    return serviceType == "amnezia-premium";
}

bool ApiConfig::isFree() const
{
    return serviceType == "amnezia-free";
}

bool ApiConfig::isExternalPremium() const
{
    return serviceType == "external-premium";
}

bool ApiConfig::isSubscriptionExpired() const
{
    if (subscription.endDate.isEmpty()) {
        return false;
    }
    
    QDateTime endDate = QDateTime::fromString(subscription.endDate, Qt::ISODateWithMs);
    if (!endDate.isValid()) {
        endDate = QDateTime::fromString(subscription.endDate, Qt::ISODate);
    }
    
    if (!endDate.isValid()) {
        return false;
    }
    
    return endDate < QDateTime::currentDateTimeUtc();
}

QJsonObject ApiConfig::toJson() const
{
    QJsonObject obj;
    
    if (!serviceType.isEmpty()) {
        obj[apiDefs::key::serviceType] = serviceType;
    }
    if (!serviceProtocol.isEmpty()) {
        obj[QLatin1String("service_protocol")] = serviceProtocol;
    }
    if (!userCountryCode.isEmpty()) {
        obj[QLatin1String("user_country_code")] = userCountryCode;
    }
    if (!serverCountryCode.isEmpty()) {
        obj[apiDefs::key::serverCountryCode] = serverCountryCode;
    }
    if (!serverCountryName.isEmpty()) {
        obj[apiDefs::key::serverCountryName] = serverCountryName;
    }
    if (!vpnKey.isEmpty()) {
        obj[apiDefs::key::vpnKey] = vpnKey;
    }
    
    QJsonObject subscriptionObj = subscription.toJson();
    if (!subscriptionObj.isEmpty()) {
        obj[apiDefs::key::subscription] = subscriptionObj;
    }
    
    if (activeDeviceCount > 0) {
        obj[apiDefs::key::activeDeviceCount] = activeDeviceCount;
    }
    if (maxDeviceCount > 0) {
        obj[apiDefs::key::maxDeviceCount] = maxDeviceCount;
    }
    if (issuedConfigs > 0) {
        obj[apiDefs::key::issuedConfigs] = issuedConfigs;
    }
    
    if (!availableCountries.isEmpty()) {
        obj[apiDefs::key::availableCountries] = availableCountries;
    }
    
    if (!supportedProtocols.isEmpty()) {
        obj[apiDefs::key::supportedProtocols] = supportedProtocols;
    }
    
    QJsonObject serviceInfoObj = serviceInfo.toJson();
    if (!serviceInfoObj.isEmpty()) {
        obj[apiDefs::key::serviceInfo] = serviceInfoObj;
    }
    
    QJsonObject publicKeyObj = publicKey.toJson();
    if (!publicKeyObj.isEmpty()) {
        obj[apiDefs::key::publicKey] = publicKeyObj;
    }
    
    if (!stackType.isEmpty()) {
        obj[apiDefs::key::stackType] = stackType;
    }
    if (!cliVersion.isEmpty()) {
        obj[apiDefs::key::cliVersion] = cliVersion;
    }
    if (isTestPurchase) {
        obj[apiDefs::key::isTestPurchase] = isTestPurchase;
    }
    if (isInAppPurchase) {
        obj[apiDefs::key::isInAppPurchase] = isInAppPurchase;
    }
    if (subscriptionExpiredByServer) {
        obj[apiDefs::key::subscriptionExpiredByServer] = subscriptionExpiredByServer;
    }
    
    return obj;
}

ApiConfig ApiConfig::fromJson(const QJsonObject& json)
{
    ApiConfig config;
    
    config.serviceType = json.value(apiDefs::key::serviceType).toString();
    config.serviceProtocol = json.value(QLatin1String("service_protocol")).toString();
    config.userCountryCode = json.value(QLatin1String("user_country_code")).toString();
    config.serverCountryCode = json.value(apiDefs::key::serverCountryCode).toString();
    config.serverCountryName = json.value(apiDefs::key::serverCountryName).toString();
    config.vpnKey = json.value(apiDefs::key::vpnKey).toString();
    
    QJsonObject subscriptionObj = json.value(apiDefs::key::subscription).toObject();
    if (!subscriptionObj.isEmpty()) {
        config.subscription = Subscription::fromJson(subscriptionObj);
    }
    
    config.activeDeviceCount = json.value(apiDefs::key::activeDeviceCount).toInt(0);
    config.maxDeviceCount = json.value(apiDefs::key::maxDeviceCount).toInt(0);
    config.issuedConfigs = json.value(apiDefs::key::issuedConfigs).toInt(0);
    
    config.availableCountries = json.value(apiDefs::key::availableCountries).toArray();
    config.supportedProtocols = json.value(apiDefs::key::supportedProtocols).toArray();
    
    QJsonObject serviceInfoObj = json.value(apiDefs::key::serviceInfo).toObject();
    if (!serviceInfoObj.isEmpty()) {
        config.serviceInfo = ServiceInfo::fromJson(serviceInfoObj);
    }
    
    QJsonObject publicKeyObj = json.value(apiDefs::key::publicKey).toObject();
    if (!publicKeyObj.isEmpty()) {
        config.publicKey = PublicKeyInfo::fromJson(publicKeyObj);
    }
    
    config.stackType = json.value(apiDefs::key::stackType).toString();
    config.cliVersion = json.value(apiDefs::key::cliVersion).toString();
    config.isTestPurchase = json.value(apiDefs::key::isTestPurchase).toBool(false);
    config.isInAppPurchase = json.value(apiDefs::key::isInAppPurchase).toBool(false);
    config.subscriptionExpiredByServer = json.value(apiDefs::key::subscriptionExpiredByServer).toBool(false);
    
    return config;
}

} // namespace amnezia

