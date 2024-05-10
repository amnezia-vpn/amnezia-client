#ifndef SERIALIZATION_H
#define SERIALIZATION_H

#include <QJsonObject>
#include "transfer.h"

namespace amnezia::serialization
{
    namespace vmess
    {
        QJsonObject Deserialize(const QString &vmess, QString *alias, QString *errMessage);
        const QString Serialize(const StreamSettingsObject &transfer, const VMessServerObject &server, const QString &alias);
    } // namespace vmess

    namespace vmess_new
    {
        QJsonObject Deserialize(const QString &vmess, QString *alias, QString *errMessage);
        const QString Serialize(const StreamSettingsObject &transfer, const VMessServerObject &server, const QString &alias);
    } // namespace vmess_new

    namespace vless
    {
        QJsonObject Deserialize(const QString &vless, QString *alias, QString *errMessage);
    } // namespace vless

    namespace ss
    {
        QJsonObject Deserialize(const QString &ss, QString *alias, QString *errMessage);
        const QString Serialize(const ShadowSocksServerObject &server, const QString &alias, bool isSip002);
    } // namespace ss

    namespace ssd
    {
        QList<std::pair<QString, QJsonObject>> Deserialize(const QString &uri, QString *groupName, QStringList *logList);
    } // namespace ssd

    namespace trojan
    {
        QJsonObject Deserialize(const QString &trojan, QString *alias, QString *errMessage);
        const QString Serialize(const TrojanObject &server, const QString &alias);
    } // namespace trojan

    namespace outbounds
    {
        QJsonObject GenerateFreedomOUT(const QString &domainStrategy, const QString &redirect);
        QJsonObject GenerateBlackHoleOUT(bool useHTTP);
        QJsonObject GenerateShadowSocksOUT(const QList<ShadowSocksServerObject> &servers);
        QJsonObject GenerateShadowSocksServerOUT(const QString &address, int port, const QString &method, const QString &password);
        QJsonObject GenerateHTTPSOCKSOut(const QString &address, int port, bool useAuth, const QString &username, const QString &password);
        QJsonObject GenerateTrojanOUT(const QList<TrojanObject> &servers);
        QJsonObject GenerateTrojanServerOUT(const QString &address, int port, const QString &password);
        QJsonObject GenerateOutboundEntry(const QString &tag,                //
                                       const QString &protocol,           //
                                       const QJsonObject &settings,   //
                                       const QJsonObject &streamSettings, //
                                       const QJsonObject &mux = {},       //
                                       const QString &sendThrough = "0.0.0.0");
    } // namespace outbounds

    namespace inbounds
    {
        QJsonObject GenerateInboundEntry();
    }
}

#endif // SERIALIZATION_H
