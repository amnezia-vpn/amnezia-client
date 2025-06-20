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
}

QJsonObject ApiV2ServerConfig::toJson() const
{
    // Сначала вызываем родительскую функцию для сериализации базовых полей
    QJsonObject json = ServerConfig::toJson();
    
    // Добавляем свои поля только если они не пустые
    if (!name.isEmpty()) {
        json[config_key::name] = name;
    }
    if (!description.isEmpty()) {
        json[config_key::description] = description;
    }
    
    // Сериализация API конфигурации
    QJsonObject apiConfigJson;
    
    // Сериализация доступных стран только если массив не пустой
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
    
    // Сериализация подписки только если есть данные
    if (!apiConfig.subscription.end_date.isEmpty()) {
        QJsonObject subscriptionObject;
        subscriptionObject["end_date"] = apiConfig.subscription.end_date;
        apiConfigJson["subscription"] = subscriptionObject;
    }
    
    // Сериализация публичного ключа только если есть данные
    if (!apiConfig.publicKey.expiresAt.isEmpty()) {
        QJsonObject publicKeyObject;
        publicKeyObject["expires_at"] = apiConfig.publicKey.expiresAt;
        apiConfigJson["public_key"] = publicKeyObject;
    }
    
    // Сериализация информации о сервере только если не пустая
    if (!apiConfig.serverCountryCode.isEmpty()) {
        apiConfigJson["server_country_code"] = apiConfig.serverCountryCode;
    }
    if (!apiConfig.serverCountryName.isEmpty()) {
        apiConfigJson["server_country_name"] = apiConfig.serverCountryName;
    }
    
    // Сериализация информации о сервисе только если не пустая
    if (!apiConfig.serviceProtocol.isEmpty()) {
        apiConfigJson["service_protocol"] = apiConfig.serviceProtocol;
    }
    if (!apiConfig.serviceType.isEmpty()) {
        apiConfigJson["service_type"] = apiConfig.serviceType;
    }
    
    // Сериализация информации о пользователе только если не пустая
    if (!apiConfig.userCountryCode.isEmpty()) {
        apiConfigJson["user_country_code"] = apiConfig.userCountryCode;
    }
    
    // Сериализация VPN ключа только если не пустой
    if (!apiConfig.vpnKey.isEmpty()) {
        apiConfigJson["vpn_key"] = apiConfig.vpnKey;
    }
    
    // Добавляем API конфигурацию в основной JSON только если есть данные
    if (!apiConfigJson.isEmpty()) {
        json["api_config"] = apiConfigJson;
    }
    
    return json;
}
