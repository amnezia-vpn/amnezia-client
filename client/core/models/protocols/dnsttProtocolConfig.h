#ifndef DNSTTPROTOCOLCONFIG_H
#define DNSTTPROTOCOLCONFIG_H

#include <QJsonObject>
#include <QString>

namespace amnezia
{
    struct DnsttProtocolConfig
    {
        QString domain;
        QString resolvers;
        QString bootstrapIp;
        QString publicKey;

        // DNSTT is only ever imported from a link; there is no server-side
        // container to install, so this is always a third-party config.
        bool isThirdPartyConfig = true;

        // Payload bytes available inside one DNS name for this domain, per
        // RFC 1035 name limits. Kept in sync with libdnstt's CalculateMtu,
        // which is the authority at connect time.
        int calculateMtu() const;
        bool isValid(QString *error = nullptr) const;

        // Wire format handed to the Android layer as dnstt_config_data.
        // Keys are snake_case to match DnsttNative/Dnstt.kt.
        QJsonObject toClientJson() const;
        static DnsttProtocolConfig fromClientJson(const QJsonObject &json);

        // Persisted format, following the project convention of storing the
        // client config as a JSON string under "last_config".
        QJsonObject toJson() const;
        static DnsttProtocolConfig fromJson(const QJsonObject &json);
    };
} // namespace amnezia

#endif // DNSTTPROTOCOLCONFIG_H
