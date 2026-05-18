#ifndef APICONFIG_H
#define APICONFIG_H

#include <QJsonObject>
#include <QJsonArray>
#include <QString>
#include <QDateTime>

#include "core/utils/serverConfigUtils.h"
#include "core/utils/constants/apiKeys.h"
#include "core/utils/constants/apiConstants.h"

namespace amnezia
{

struct ApiConfig
{
    QString serviceType;
    QString serviceProtocol;
    QString userCountryCode;
    QString serverCountryCode;
    QString serverCountryName;
    QString vpnKey;
    
    struct Subscription {
        QString endDate;
        
        QJsonObject toJson() const;
        static Subscription fromJson(const QJsonObject& json);
    };
    Subscription subscription;
    
    int activeDeviceCount;
    int maxDeviceCount;
    int issuedConfigs;
    QJsonArray availableCountries;
    QJsonArray supportedProtocols;
    
    struct ServiceInfo {
        bool isAdVisible = false;
        bool isRenewalAvailable = false;
        QString adHeader;
        QString adDescription;
        QString adEndpoint;
        
        QJsonObject toJson() const;
        static ServiceInfo fromJson(const QJsonObject& json);
    };
    ServiceInfo serviceInfo;
    
    struct PublicKeyInfo {
        QString expiresAt;
        
        QJsonObject toJson() const;
        static PublicKeyInfo fromJson(const QJsonObject& json);
    };
    PublicKeyInfo publicKey;
    
    QString stackType;
    QString cliVersion;
    bool isTestPurchase = false;
    bool isInAppPurchase = false;
    bool subscriptionExpiredByServer = false;
    
    bool isPremium() const;
    bool isFree() const;
    bool isExternalPremium() const;
    bool isSubscriptionExpired() const;
    
    QJsonObject toJson() const;
    static ApiConfig fromJson(const QJsonObject& json);
};

} // namespace amnezia

#endif // APICONFIG_H

