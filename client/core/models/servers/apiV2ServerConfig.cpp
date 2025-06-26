#include "apiV2ServerConfig.h"

#include <QJsonArray>
#include <QJsonDocument>

#include "protocols/protocols_defs.h"

using namespace amnezia;

ApiV2ServerConfig::ApiV2ServerConfig(const QJsonObject &serverConfigObject) : ServerConfig(serverConfigObject)
{
    name = serverConfigObject.value(config_key::name).toString();
    description = serverConfigObject.value(config_key::description).toString();

    auto apiConfigObject = serverConfigObject.value("api_config").toObject();

    auto availableCountriesArray = apiConfigObject.value("available_countries").toArray();
    for (const auto &countryValue : availableCountriesArray) {
        auto countryObject = countryValue.toObject();
        apiv2::Country country;
        country.code = countryObject.value("server_country_code").toString();
        country.name = countryObject.value("server_country_name").toString();
        apiConfig.availableCountries.append(country);
    }

    auto subscriptionObject = apiConfigObject.value("subscription").toObject();
    apiConfig.subscription.end_date = subscriptionObject.value("end_date").toString();

    auto publicKeyObject = apiConfigObject.value("public_key").toObject();
    apiConfig.publicKey.expiresAt = publicKeyObject.value("expires_at").toString();

    apiConfig.serverCountryCode = apiConfigObject.value("server_country_code").toString();
    apiConfig.serverCountryName = apiConfigObject.value("server_country_name").toString();

    apiConfig.serviceProtocol = apiConfigObject.value("service_protocol").toString();
    apiConfig.serviceType = apiConfigObject.value("service_type").toString();

    apiConfig.userCountryCode = apiConfigObject.value("user_country_code").toString();

    apiConfig.vpnKey = apiConfigObject.value("vpn_key").toString();

    auto authDataObject = serverConfigObject.value("auth_data").toObject();
    apiConfig.authData.apiKey = authDataObject.value("api_key").toString();
}

QJsonObject ApiV2ServerConfig::toJson() const
{
    QJsonObject json = ServerConfig::toJson();
    
    if (!name.isEmpty()) {
        json[config_key::name] = name;
    }
    if (!description.isEmpty()) {
        json[config_key::description] = description;
    }
    
    QJsonObject apiConfigJson;
    
    if (!apiConfig.availableCountries.isEmpty()) {
        QJsonArray availableCountriesArray;
        for (const auto &country : apiConfig.availableCountries) {
            QJsonObject countryObject;
            if (!country.code.isEmpty()) {
                countryObject["server_country_code"] = country.code;
            }
            if (!country.name.isEmpty()) {
                countryObject["server_country_name"] = country.name;
            }
            if (!countryObject.isEmpty()) {
                availableCountriesArray.append(countryObject);
            }
        }
        if (!availableCountriesArray.isEmpty()) {
            apiConfigJson["available_countries"] = availableCountriesArray;
        }
    }
    
    if (!apiConfig.subscription.end_date.isEmpty()) {
        QJsonObject subscriptionObject;
        subscriptionObject["end_date"] = apiConfig.subscription.end_date;
        apiConfigJson["subscription"] = subscriptionObject;
    }
    
    if (!apiConfig.publicKey.expiresAt.isEmpty()) {
        QJsonObject publicKeyObject;
        publicKeyObject["expires_at"] = apiConfig.publicKey.expiresAt;
        apiConfigJson["public_key"] = publicKeyObject;
    }
    
    if (!apiConfig.serverCountryCode.isEmpty()) {
        apiConfigJson["server_country_code"] = apiConfig.serverCountryCode;
    }
    if (!apiConfig.serverCountryName.isEmpty()) {
        apiConfigJson["server_country_name"] = apiConfig.serverCountryName;
    }
    
    if (!apiConfig.serviceProtocol.isEmpty()) {
        apiConfigJson["service_protocol"] = apiConfig.serviceProtocol;
    }
    if (!apiConfig.serviceType.isEmpty()) {
        apiConfigJson["service_type"] = apiConfig.serviceType;
    }
    
    if (!apiConfig.userCountryCode.isEmpty()) {
        apiConfigJson["user_country_code"] = apiConfig.userCountryCode;
    }
    
    if (!apiConfig.vpnKey.isEmpty()) {
        apiConfigJson["vpn_key"] = apiConfig.vpnKey;
    }

    QJsonObject authDataJson;
    if (!apiConfig.authData.apiKey.isEmpty()) {
        authDataJson["api_key"] = apiConfig.authData.apiKey;
    }
    if (!authDataJson.isEmpty()) {
        apiConfigJson["auth_data"] = authDataJson;
    }

    if (!apiConfigJson.isEmpty()) {
        json["api_config"] = apiConfigJson;
    }
    
    return json;
}
