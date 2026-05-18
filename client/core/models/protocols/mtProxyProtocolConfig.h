#ifndef MTPROXYPROTOCOLCONFIG_H
#define MTPROXYPROTOCOLCONFIG_H

#include <QJsonObject>
#include <QString>
#include <QStringList>

namespace amnezia {

    struct MtProxyProtocolConfig {
        QString port;
        QString secret;
        QString tag;
        QString tgLink;
        QString tmeLink;
        bool isEnabled = true;
        QString publicHost;
        QString transportMode;
        QString tlsDomain;
        QStringList additionalSecrets;
        QString workersMode;
        QString workers;
        bool natEnabled = false;
        QString natInternalIp;
        QString natExternalIp;

        QJsonObject toJson() const;

        static MtProxyProtocolConfig fromJson(const QJsonObject &json);

        // Port, transport, TLS, secrets, NAT, workers, isEnabled, additionalSecrets (order-independent).
        // Ignores tgLink / tmeLink (derived / display).
        bool equalsDockerDeploymentSettings(const MtProxyProtocolConfig &other) const;
    };

} // namespace amnezia

#endif // MTPROXYPROTOCOLCONFIG_H
