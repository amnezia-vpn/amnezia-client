#ifndef WHATSAPPPROXYPROTOCOLCONFIG_H
#define WHATSAPPPROXYPROTOCOLCONFIG_H

#include <QJsonObject>
#include <QString>

namespace amnezia
{

    struct WhatsAppProxyProtocolConfig
    {
        QString port;
        QString mediaPort;
        QString publicHost;
        bool isEnabled = true;

        QString effectivePort() const;
        QString effectiveMediaPort() const;

        QJsonObject toJson() const;
        static WhatsAppProxyProtocolConfig fromJson(const QJsonObject &json);
        bool equalsDockerDeploymentSettings(const WhatsAppProxyProtocolConfig &other) const;
    };

} // namespace amnezia

#endif // WHATSAPPPROXYPROTOCOLCONFIG_H
