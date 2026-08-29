#include "dnsttProtocolConfig.h"

#include <QJsonDocument>
#include <QObject>
#include <QRegularExpression>
#include <QStringList>

#include "core/utils/constants/configKeys.h"

namespace amnezia
{
    namespace
    {
        // Field names of the client config, shared by the persisted
        // "last_config" payload and dnstt_config_data.
        constexpr QLatin1String keyDomain("domain");
        constexpr QLatin1String keyResolvers("resolvers");
        constexpr QLatin1String keyBootstrapIp("bootstrap_ip");
        constexpr QLatin1String keyPublicKey("public_key");

        // Smallest payload dnstt can operate on; mirrors libdnstt's minMtu.
        constexpr int minMtu = 80;
    }

    int DnsttProtocolConfig::calculateMtu() const
    {
        const QString trimmedDomain = domain.trimmed();
        if (trimmedDomain.isEmpty()) {
            return 0;
        }

        // Names must be 255 octets or shorter in total length (RFC 1035
        // 2.3.4), minus the null terminator.
        int capacity = 255 - 1;
        const QStringList labels = trimmedDomain.split('.', Qt::SkipEmptyParts);
        for (const QString &label : labels) {
            // Subtract the length of the label and the length octet.
            capacity -= (label.length() + 1);
        }
        // Each label may be up to 63 bytes long but requires 64 to encode.
        capacity = (capacity * 63) / 64;
        // Base32 expands every 5 bytes to 8.
        capacity = (capacity * 5) / 8;

        // clientid + padding length prefix + padding + data length prefix
        const int numPadding = 3;
        const int mtu = capacity - 8 - 1 - numPadding - 1;
        return mtu > 0 ? mtu : 0;
    }

    bool DnsttProtocolConfig::isValid(QString *error) const
    {
        if (domain.trimmed().isEmpty()) {
            if (error) *error = QObject::tr("Domain cannot be empty");
            return false;
        }

        const int mtu = calculateMtu();
        if (mtu < minMtu) {
            if (error) {
                *error = QObject::tr("Domain is too long: it leaves %1 bytes of payload, at least %2 are required")
                                 .arg(mtu)
                                 .arg(minMtu);
            }
            return false;
        }

        if (resolvers.trimmed().isEmpty()) {
            if (error) *error = QObject::tr("Resolvers list cannot be empty");
            return false;
        }

        const QString key = publicKey.trimmed();
        static const QRegularExpression hexRegex("^[0-9a-fA-F]{64}$");
        if (!hexRegex.match(key).hasMatch()) {
            if (error) {
                *error = QObject::tr("Public key must be exactly 64 hexadecimal characters (got %1)").arg(key.length());
            }
            return false;
        }

        return true;
    }

    QJsonObject DnsttProtocolConfig::toClientJson() const
    {
        QJsonObject obj;
        obj[keyDomain] = domain.trimmed();
        obj[keyResolvers] = resolvers.trimmed();
        obj[keyBootstrapIp] = bootstrapIp.trimmed();
        obj[keyPublicKey] = publicKey.trimmed();
        return obj;
    }

    DnsttProtocolConfig DnsttProtocolConfig::fromClientJson(const QJsonObject &json)
    {
        DnsttProtocolConfig cfg;
        cfg.domain = json.value(keyDomain).toString();
        cfg.resolvers = json.value(keyResolvers).toString();
        cfg.bootstrapIp = json.value(keyBootstrapIp).toString();
        cfg.publicKey = json.value(keyPublicKey).toString();
        return cfg;
    }

    QJsonObject DnsttProtocolConfig::toJson() const
    {
        QJsonObject obj;
        obj[configKey::lastConfig] =
                QString::fromUtf8(QJsonDocument(toClientJson()).toJson(QJsonDocument::Compact));
        obj[configKey::isThirdPartyConfig] = isThirdPartyConfig;
        return obj;
    }

    DnsttProtocolConfig DnsttProtocolConfig::fromJson(const QJsonObject &json)
    {
        const QString lastConfigStr = json.value(configKey::lastConfig).toString();
        const QJsonDocument doc = QJsonDocument::fromJson(lastConfigStr.toUtf8());

        // Fall back to flat keys so a config written by an older build, which
        // stored the fields directly, still loads.
        DnsttProtocolConfig cfg = fromClientJson(doc.isObject() ? doc.object() : json);
        cfg.isThirdPartyConfig = json.value(configKey::isThirdPartyConfig).toBool(true);
        return cfg;
    }
} // namespace amnezia
