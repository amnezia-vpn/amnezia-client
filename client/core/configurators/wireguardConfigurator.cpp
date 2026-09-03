#include "wireguardConfigurator.h"

#include <QDebug>
#include <QAbstractSocket>
#include <QJsonDocument>
#include <QProcess>
#include <QRegularExpression>
#include <QString>
#include <QTemporaryDir>
#include <QTemporaryFile>

#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>

#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"
#include "core/utils/selfhosted/sshSession.h"
#include "core/utils/selfhosted/scriptsRegistry.h"
#include "core/utils/protocolEnum.h"
#include "core/protocols/protocolUtils.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"
#include "core/utils/utilities.h"
#include "core/models/containerConfig.h"
#include "core/models/protocols/wireGuardProtocolConfig.h"
#include "core/models/protocols/awgProtocolConfig.h"
#include <QJsonArray>

using namespace amnezia;

namespace
{

QString stripPrefixLength(const QString &address)
{
    return address.trimmed().section("/", 0, 0).trimmed();
}

QString withPrefixLength(const QString &address, const QString &prefixLength)
{
    if (address.contains("/")) {
        return address.trimmed();
    }
    return QString("%1/%2").arg(address.trimmed(), prefixLength);
}

QString formatEndpointHost(const QString &host)
{
    const QHostAddress hostAddress(host);
    if (hostAddress.protocol() == QAbstractSocket::IPv6Protocol) {
        return QString("[%1]").arg(host);
    }
    return host;
}

QHostAddress nextIpv6Address(const QHostAddress &address)
{
    Q_IPV6ADDR bytes = address.toIPv6Address();
    for (int i = 15; i >= 0; --i) {
        ++bytes[i];
        if (bytes[i] != 0) {
            return QHostAddress(bytes);
        }
    }
    // All-0xFF wrapped around to ::; the subnet is exhausted, so report no address.
    return QHostAddress();
}

QList<QHostAddress> filterAddressesByFamily(const QList<QHostAddress> &addresses, QAbstractSocket::NetworkLayerProtocol family)
{
    QList<QHostAddress> result;
    for (const QHostAddress &address : addresses) {
        if (address.protocol() == family) {
            result << address;
        }
    }
    return result;
}

}

WireguardConfigurator::WireguardConfigurator(SshSession* sshSession, bool isAwg,
                                             QObject *parent)
    : ConfiguratorBase(sshSession, parent), m_isAwg(isAwg)
{
    m_serverConfigPath =
            m_isAwg ? amnezia::protocols::awg::serverConfigPath : amnezia::protocols::wireguard::serverConfigPath;
    m_serverPublicKeyPath =
            m_isAwg ? amnezia::protocols::awg::serverPublicKeyPath : amnezia::protocols::wireguard::serverPublicKeyPath;
    m_serverPskKeyPath =
            m_isAwg ? amnezia::protocols::awg::serverPskKeyPath : amnezia::protocols::wireguard::serverPskKeyPath;
    m_configTemplate = m_isAwg ? ProtocolScriptType::awg_template : ProtocolScriptType::wireguard_template;

    m_protocolName = m_isAwg ? configKey::awg : configKey::wireguard;
    m_defaultPort = m_isAwg ? protocols::awg::defaultPort : protocols::wireguard::defaultPort;
}

WireguardConfigurator::ConnectionData WireguardConfigurator::genClientKeys()
{
    // TODO review
    constexpr size_t EDDSA_KEY_LENGTH = 32;

    ConnectionData connData;

    unsigned char buff[EDDSA_KEY_LENGTH];
    int ret = RAND_priv_bytes(buff, EDDSA_KEY_LENGTH);
    if (ret <= 0)
        return connData;

    EVP_PKEY *pKey = EVP_PKEY_new();
    q_check_ptr(pKey);
    pKey = EVP_PKEY_new_raw_private_key(EVP_PKEY_X25519, NULL, &buff[0], EDDSA_KEY_LENGTH);

    size_t keySize = EDDSA_KEY_LENGTH;

    // save private key
    unsigned char priv[EDDSA_KEY_LENGTH];
    EVP_PKEY_get_raw_private_key(pKey, priv, &keySize);
    connData.clientPrivKey = QByteArray::fromRawData((char *)priv, keySize).toBase64();

    // save public key
    unsigned char pub[EDDSA_KEY_LENGTH];
    EVP_PKEY_get_raw_public_key(pKey, pub, &keySize);
    connData.clientPubKey = QByteArray::fromRawData((char *)pub, keySize).toBase64();

    return connData;
}

QList<QHostAddress> WireguardConfigurator::getIpsFromConf(const QString &input)
{
    QRegularExpression regex("AllowedIPs\\s*=\\s*([^\\r\\n]+)");
    QRegularExpressionMatchIterator matchIterator = regex.globalMatch(input);

    QList<QHostAddress> ips;

    while (matchIterator.hasNext()) {
        QRegularExpressionMatch match = matchIterator.next();
        const QStringList allowedIps = match.captured(1).split(",", Qt::SkipEmptyParts);
        for (const QString &allowedIp : allowedIps) {
            const QString addressString = stripPrefixLength(allowedIp);
            const QHostAddress address { addressString };
            if (address.isNull()) {
                qWarning() << "Couldn't recognize the ip address: " << addressString;
            } else {
                ips << address;
            }
        }
    }

    return ips;
}

bool WireguardConfigurator::hasServerIpv6Egress(const ServerCredentials &credentials, DockerContainer container)
{
    QString stdOut;
    auto cbReadStdOut = [&](const QString &data, libssh::Client &) {
        stdOut += data + "\n";
        return ErrorCode::NoError;
    };

    constexpr char probeServer[] = "2001:4860:4860::8888"; // Google Public DNS (IPv6)
    constexpr char probeName[] = "google.com";
    constexpr int probeTimeoutSec = 3;

    const QString probeScript =
        QString("timeout %1 nslookup %2 %3 >/dev/null 2>&1 && echo AMNEZIA_IPV6_OK || true")
            .arg(QString::number(probeTimeoutSec), probeName, probeServer);

    const auto errorCode = m_sshSession->runContainerScript(credentials, container, probeScript, cbReadStdOut);
    if (errorCode != ErrorCode::NoError) {
        qWarning() << "Unable to probe IPv6 egress in container, generating IPv4-only client config";
        return false;
    }

    const bool hasIpv6Egress = stdOut.contains("AMNEZIA_IPV6_OK");
    if (!hasIpv6Egress) {
        qInfo() << "Container IPv6 egress is unavailable, generating IPv4-only WG/AWG client config";
    }
    return hasIpv6Egress;
}

WireguardConfigurator::ConnectionData WireguardConfigurator::prepareWireguardConfig(const ServerCredentials &credentials,
                                                                                    DockerContainer container,
                                                                                    const WireGuardServerConfig* serverConfig,
                                                                                    const AwgServerConfig* awgServerConfig,
                                                                                    const DnsSettings &dnsSettings,
                                                                                    ErrorCode &errorCode)
{
    WireguardConfigurator::ConnectionData connData = WireguardConfigurator::genClientKeys();
    connData.host = credentials.hostName;
    
    QString portStr = m_defaultPort;
    if (serverConfig && !serverConfig->port.isEmpty()) {
        portStr = serverConfig->port;
    } else if (awgServerConfig && !awgServerConfig->port.isEmpty()) {
        portStr = awgServerConfig->port;
    }
    connData.port = portStr;

    if (connData.clientPrivKey.isEmpty() || connData.clientPubKey.isEmpty()) {
        errorCode = ErrorCode::InternalError;
        return connData;
    }

    QString configPath = m_serverConfigPath;
    if (container == DockerContainer::Awg) {
        configPath = amnezia::protocols::awg::serverLegacyConfigPath;
    }
    QString getIpsScript = QString("cat %1 | grep AllowedIPs").arg(configPath);
    QString stdOut;
    auto cbReadStdOut = [&](const QString &data, libssh::Client &) {
        stdOut += data + "\n";
        return ErrorCode::NoError;
    };

    errorCode = m_sshSession->runContainerScript(credentials, container, getIpsScript, cbReadStdOut);
    if (errorCode != ErrorCode::NoError) {
        return connData;
    }
    const auto ips = getIpsFromConf(stdOut);
    const auto ipv4Addresses = filterAddressesByFamily(ips, QAbstractSocket::IPv4Protocol);
    const auto ipv6Addresses = filterAddressesByFamily(ips, QAbstractSocket::IPv6Protocol);

    QHostAddress nextIp = [&] {
        QHostAddress result;
        QHostAddress lastIp;
        QString subnetAddress = protocols::wireguard::defaultSubnetAddress;
        if (serverConfig && !serverConfig->subnetAddress.isEmpty()) {
            subnetAddress = serverConfig->subnetAddress;
        } else if (awgServerConfig && !awgServerConfig->subnetAddress.isEmpty()) {
            subnetAddress = awgServerConfig->subnetAddress;
        }
        if (ipv4Addresses.empty()) {
            lastIp.setAddress(subnetAddress);
        } else {
            lastIp = ipv4Addresses.last();
        }
        quint8 lastOctet = static_cast<quint8>(lastIp.toIPv4Address());
        switch (lastOctet) {
        case 254: result.setAddress(lastIp.toIPv4Address() + 3); break;
        case 255: result.setAddress(lastIp.toIPv4Address() + 2); break;
        default: result.setAddress(lastIp.toIPv4Address() + 1); break;
        }

        return result;
    }();

    connData.clientIP = nextIp.toString();

    if (hasServerIpv6Egress(credentials, container)) {
        QHostAddress lastIpv6;
        QString subnetIpv6Address = m_isAwg ? protocols::awg::defaultSubnetIpv6Address
                                            : protocols::wireguard::defaultSubnetIpv6Address;
        if (serverConfig && !serverConfig->subnetIpv6Address.isEmpty()) {
            subnetIpv6Address = serverConfig->subnetIpv6Address;
        } else if (awgServerConfig && !awgServerConfig->subnetIpv6Address.isEmpty()) {
            subnetIpv6Address = awgServerConfig->subnetIpv6Address;
        }

        if (ipv6Addresses.empty()) {
            lastIpv6.setAddress(stripPrefixLength(subnetIpv6Address));
        } else {
            lastIpv6 = ipv6Addresses.last();
        }

        const QHostAddress nextIpv6 = nextIpv6Address(lastIpv6);
        if (!nextIpv6.isNull()) {
            connData.clientIPv6 = nextIpv6.toString();
        }
    }

    // Get keys
    connData.serverPubKey =
            m_sshSession->getTextFileFromContainer(container, credentials, m_serverPublicKeyPath, errorCode);
    connData.serverPubKey.replace("\n", "");
    if (errorCode != ErrorCode::NoError) {
        return connData;
    }

    connData.pskKey = m_sshSession->getTextFileFromContainer(container, credentials, m_serverPskKeyPath, errorCode);
    connData.pskKey.replace("\n", "");

    if (errorCode != ErrorCode::NoError) {
        return connData;
    }

    // Add client to config
    QStringList peerAllowedIps { withPrefixLength(connData.clientIP, "32") };
    if (!connData.clientIPv6.isEmpty()) {
        peerAllowedIps << withPrefixLength(connData.clientIPv6, protocols::wireguard::defaultClientIpv6Cidr);
    }

    QString configPart = QString("[Peer]\n"
                                 "PublicKey = %1\n"
                                 "PresharedKey = %2\n"
                                 "AllowedIPs = %3\n\n")
                                 .arg(connData.clientPubKey, connData.pskKey, peerAllowedIps.join(", "));

    errorCode = m_sshSession->uploadTextFileToContainer(container, credentials, configPart, configPath,
                                                              libssh::ScpOverwriteMode::ScpAppendToExisting);

    if (errorCode != ErrorCode::NoError) {
        return connData;
    }

    bool isAwg = (container == DockerContainer::Awg2);
    QString bin = isAwg ? QStringLiteral("awg") : QStringLiteral("wg");
    QString iface = isAwg ? QStringLiteral("awg0") : QStringLiteral("wg0");
    QString script = QString(
        "sudo docker exec -i $CONTAINER_NAME bash -c '%1 syncconf %2 <(%1-quick strip %3)'").arg(bin, iface, configPath);

    errorCode = m_sshSession->runScript(
            credentials,
            m_sshSession->replaceVars(script, amnezia::genBaseVars(credentials, container, dnsSettings.primaryDns, dnsSettings.secondaryDns)));

    return connData;
}

ProtocolConfig WireguardConfigurator::createConfig(const ServerCredentials &credentials, DockerContainer container,
                                                    const ContainerConfig &containerConfig,
                                                    const DnsSettings &dnsSettings,
                                                    ErrorCode &errorCode)
{
    const WireGuardServerConfig* wireguardServerConfig = nullptr;
    const WireGuardClientConfig* wireguardClientConfig = nullptr;
    const AwgServerConfig* awgServerConfig = nullptr;
    const AwgClientConfig* awgClientConfig = nullptr;
    
    if (auto* wireGuardProtocolConfig = containerConfig.getWireGuardProtocolConfig()) {
        wireguardServerConfig = &wireGuardProtocolConfig->serverConfig;
        if (wireGuardProtocolConfig->clientConfig.has_value()) {
            wireguardClientConfig = &wireGuardProtocolConfig->clientConfig.value();
        }
    } else if (auto* awgProtocolConfig = containerConfig.getAwgProtocolConfig()) {
        awgServerConfig = &awgProtocolConfig->serverConfig;
        if (awgProtocolConfig->clientConfig.has_value()) {
            awgClientConfig = &awgProtocolConfig->clientConfig.value();
        }
    }
    
    amnezia::ScriptVars vars = amnezia::genBaseVars(credentials, container, dnsSettings.primaryDns, dnsSettings.secondaryDns);
    vars.append(amnezia::genProtocolVarsForContainer(container, containerConfig));
    QString scriptData = amnezia::scriptData(m_configTemplate, container);
    QString config = m_sshSession->replaceVars(scriptData, vars);

    // The template lists every possible key, but each parameter is optional -
    // drop the lines whose value came out empty
    static const QRegularExpression emptyValueLine(R"(^\s*\S+\s*=\s*$)");
    auto configTemplateLines = config.split("\n");
    configTemplateLines.removeIf([](const QString &line) { return emptyValueLine.match(line).hasMatch(); });
    config = configTemplateLines.join("\n");

    ConnectionData connData = prepareWireguardConfig(credentials, container, wireguardServerConfig, awgServerConfig, dnsSettings, errorCode);
    if (errorCode != ErrorCode::NoError) {
        return WireGuardProtocolConfig{};
    }

    const QStringList clientAddresses = [&] {
        QStringList addresses { withPrefixLength(connData.clientIP, "32") };
        if (!connData.clientIPv6.isEmpty()) {
            addresses << withPrefixLength(connData.clientIPv6, protocols::wireguard::defaultClientIpv6Cidr);
        }
        return addresses;
    }();
    QStringList allowedIps { "0.0.0.0/0" };
    if (!connData.clientIPv6.isEmpty()) {
        allowedIps << "::/0";
    }

    config.replace("$WIREGUARD_CLIENT_PRIVATE_KEY", connData.clientPrivKey);
    config.replace("$WIREGUARD_CLIENT_ADDRESS", clientAddresses.join(", "));
    config.replace("$WIREGUARD_CLIENT_IP", connData.clientIP);
    config.replace("$WIREGUARD_ALLOWED_IPS", allowedIps.join(", "));
    config.replace("$WIREGUARD_ENDPOINT_HOST", formatEndpointHost(connData.host));
    config.replace("$WIREGUARD_SERVER_PUBLIC_KEY", connData.serverPubKey);
    config.replace("$WIREGUARD_PSK", connData.pskKey);

    QString mtu = protocols::wireguard::defaultMtu;
    if (wireguardClientConfig && !wireguardClientConfig->mtu.isEmpty()) {
        mtu = wireguardClientConfig->mtu;
    } else if (awgClientConfig && !awgClientConfig->mtu.isEmpty()) {
        mtu = awgClientConfig->mtu;
    }
    
    WireGuardProtocolConfig protocolConfig;
    if (wireguardServerConfig) {
        protocolConfig.serverConfig = *wireguardServerConfig;
    }
    
    WireGuardClientConfig clientConfig;
    clientConfig.nativeConfig = config;
    clientConfig.hostName = connData.host;
    clientConfig.port = connData.port.toInt();
    clientConfig.clientIp = connData.clientIP;
    clientConfig.clientIpv6 = connData.clientIPv6;
    clientConfig.clientPrivateKey = connData.clientPrivKey;
    clientConfig.clientPublicKey = connData.clientPubKey;
    clientConfig.serverPublicKey = connData.serverPubKey;
    clientConfig.presharedKey = connData.pskKey;
    clientConfig.clientId = connData.clientPubKey;
    clientConfig.allowedIps = allowedIps;
    const bool useKeepAliveRange = awgServerConfig && awgServerConfig->hasAwg3Params();
    clientConfig.persistentKeepAlive = useKeepAliveRange ? protocols::awg::defaultPersistentKeepAlive
                                                         : protocols::wireguard::defaultPersistentKeepAlive;
    clientConfig.mtu = mtu;
    clientConfig.isObfuscationEnabled = false;
    
    protocolConfig.setClientConfig(clientConfig);
    
    return protocolConfig;
}

ProtocolConfig WireguardConfigurator::processConfigWithLocalSettings(const ConnectionSettings &settings,
                                                                     ProtocolConfig protocolConfig)
{
    return ConfiguratorBase::processConfigWithLocalSettings(settings, protocolConfig);
}

ProtocolConfig WireguardConfigurator::processConfigWithExportSettings(const ExportSettings &settings,
                                                                      ProtocolConfig protocolConfig)
{
    return ConfiguratorBase::processConfigWithExportSettings(settings, protocolConfig);
}
