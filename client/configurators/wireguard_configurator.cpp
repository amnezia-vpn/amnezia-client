#include "wireguard_configurator.h"

#include <QDebug>
#include <QJsonDocument>
#include <QProcess>
#include <QRegularExpression>
#include <QString>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <type_traits>
#include <variant>

#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>

#include "core/models/containers/containers_defs.h"
#include "core/controllers/selfhosted/serverController.h"
#include "core/scripts_registry.h"
#include "core/server_defs.h"
#include "core/models/protocols/wireguardProtocolConfig.h"
#include "core/models/protocols/awgProtocolConfig.h"
#include "protocols/protocols_defs.h"
#include "settings.h"
#include "utilities.h"

WireguardConfigurator::WireguardConfigurator(std::shared_ptr<Settings> settings,
                                             const QSharedPointer<ServerController> &serverController, bool isAwg,
                                             QObject *parent)
    : ConfiguratorBase(settings, serverController, parent), m_isAwg(isAwg)
{
    m_serverConfigPath =
            m_isAwg ? amnezia::protocols::awg::serverConfigPath : amnezia::protocols::wireguard::serverConfigPath;
    m_serverPublicKeyPath =
            m_isAwg ? amnezia::protocols::awg::serverPublicKeyPath : amnezia::protocols::wireguard::serverPublicKeyPath;
    m_serverPskKeyPath =
            m_isAwg ? amnezia::protocols::awg::serverPskKeyPath : amnezia::protocols::wireguard::serverPskKeyPath;
    m_configTemplate = m_isAwg ? ProtocolScriptType::awg_template : ProtocolScriptType::wireguard_template;

    m_protocolName = m_isAwg ? config_key::awg : config_key::wireguard;
    m_defaultPort = m_isAwg ? protocols::wireguard::defaultPort : protocols::awg::defaultPort;
}

ConfiguratorBase::Vars WireguardConfigurator::generateProtocolVars(const ServerCredentials &credentials, DockerContainer container,
                                                                   const QSharedPointer<ProtocolConfig> &protocolConfig) const
{
    Vars vars = generateCommonVars(credentials, container);

    if (m_isAwg) {
        auto awgConfig = qSharedPointerCast<AwgProtocolConfig>(protocolConfig);
        if (!awgConfig) {
            return vars;
        }

        vars.append({{"$AWG_SUBNET_IP", awgConfig->serverProtocolConfig.subnetAddress}});
        vars.append({{"$AWG_SERVER_PORT", awgConfig->serverProtocolConfig.port}});

        const auto &awgData = awgConfig->serverProtocolConfig.awgData;
        vars.append({{"$JUNK_PACKET_COUNT", awgData.junkPacketCount}});
        vars.append({{"$JUNK_PACKET_MIN_SIZE", awgData.junkPacketMinSize}});
        vars.append({{"$JUNK_PACKET_MAX_SIZE", awgData.junkPacketMaxSize}});
        vars.append({{"$INIT_PACKET_JUNK_SIZE", awgData.initPacketJunkSize}});
        vars.append({{"$RESPONSE_PACKET_JUNK_SIZE", awgData.responsePacketJunkSize}});
        vars.append({{"$INIT_PACKET_MAGIC_HEADER", awgData.initPacketMagicHeader}});
        vars.append({{"$RESPONSE_PACKET_MAGIC_HEADER", awgData.responsePacketMagicHeader}});
        vars.append({{"$UNDERLOAD_PACKET_MAGIC_HEADER", awgData.underloadPacketMagicHeader}});
        vars.append({{"$TRANSPORT_PACKET_MAGIC_HEADER", awgData.transportPacketMagicHeader}});
        vars.append({{"$COOKIE_REPLY_PACKET_JUNK_SIZE", awgData.cookieReplyPacketJunkSize}});
        vars.append({{"$TRANSPORT_PACKET_JUNK_SIZE", awgData.transportPacketJunkSize}});
    } else {
        auto wgConfig = qSharedPointerCast<WireGuardProtocolConfig>(protocolConfig);
        if (!wgConfig) {
            return vars;
        }

        vars.append({{"$WIREGUARD_SUBNET_IP", wgConfig->serverProtocolConfig.subnetAddress}});
        vars.append({{"$WIREGUARD_SUBNET_CIDR", protocols::wireguard::defaultSubnetCidr}});
        vars.append({{"$WIREGUARD_SUBNET_MASK", protocols::wireguard::defaultSubnetMask}});
        vars.append({{"$WIREGUARD_SERVER_PORT", wgConfig->serverProtocolConfig.port}});
    }

    return vars;
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
    QRegularExpression regex("AllowedIPs = (\\d+\\.\\d+\\.\\d+\\.\\d+)");
    QRegularExpressionMatchIterator matchIterator = regex.globalMatch(input);

    QList<QHostAddress> ips;

    while (matchIterator.hasNext()) {
        QRegularExpressionMatch match = matchIterator.next();
        const QString address_string { match.captured(1) };
        const QHostAddress address { address_string };
        if (address.isNull()) {
            qWarning() << "Couldn't recognize the ip address: " << address_string;
        } else {
            ips << address;
        }
    }

    return ips;
}

WireguardConfigurator::ConnectionData WireguardConfigurator::prepareWireguardConfig(const ServerCredentials &credentials,
                                                                                    DockerContainer container,
                                                                                    const QSharedPointer<ProtocolConfig> &protocolConfig,
                                                                                    ErrorCode &errorCode)
{
    WireguardConfigurator::ConnectionData connData = WireguardConfigurator::genClientKeys();
    connData.host = credentials.hostName;
    
    // Extract port from appropriate protocol config
    if (m_isAwg) {
        auto awgConfig = qSharedPointerCast<AwgProtocolConfig>(protocolConfig);
        connData.port = awgConfig ? awgConfig->serverProtocolConfig.port : m_defaultPort;
    } else {
        auto wgConfig = qSharedPointerCast<WireGuardProtocolConfig>(protocolConfig);
        connData.port = wgConfig ? wgConfig->serverProtocolConfig.port : m_defaultPort;
    }

    if (connData.clientPrivKey.isEmpty() || connData.clientPubKey.isEmpty()) {
        errorCode = ErrorCode::InternalError;
        return connData;
    }

    QString getIpsScript = QString("cat %1 | grep AllowedIPs").arg(m_serverConfigPath);
    QString stdOut;
    auto cbReadStdOut = [&](const QString &data, libssh::Client &) {
        stdOut += data + "\n";
        return ErrorCode::NoError;
    };

    errorCode = m_serverController->runContainerScript(credentials, container, getIpsScript, cbReadStdOut);
    if (errorCode != ErrorCode::NoError) {
        return connData;
    }
    auto ips = getIpsFromConf(stdOut);

    QHostAddress nextIp = [&] {
        QHostAddress result;
        QHostAddress lastIp;
        if (ips.empty()) {
            // Get subnet from protocol config
            QString subnetAddress;
            if (m_isAwg) {
                auto awgConfig = qSharedPointerCast<AwgProtocolConfig>(protocolConfig);
                subnetAddress = awgConfig ? awgConfig->serverProtocolConfig.subnetAddress : protocols::wireguard::defaultSubnetAddress;
            } else {
                auto wgConfig = qSharedPointerCast<WireGuardProtocolConfig>(protocolConfig);
                subnetAddress = wgConfig ? wgConfig->serverProtocolConfig.subnetAddress : protocols::wireguard::defaultSubnetAddress;
            }
            lastIp.setAddress(subnetAddress);
        } else {
            lastIp = ips.last();
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

    // Get keys
    connData.serverPubKey =
            m_serverController->getTextFileFromContainer(container, credentials, m_serverPublicKeyPath, errorCode);
    connData.serverPubKey.replace("\n", "");
    if (errorCode != ErrorCode::NoError) {
        return connData;
    }

    connData.pskKey = m_serverController->getTextFileFromContainer(container, credentials, m_serverPskKeyPath, errorCode);
    connData.pskKey.replace("\n", "");

    if (errorCode != ErrorCode::NoError) {
        return connData;
    }

    // Add client to config
    QString configPart = QString("[Peer]\n"
                                 "PublicKey = %1\n"
                                 "PresharedKey = %2\n"
                                 "AllowedIPs = %3/32\n\n")
                                 .arg(connData.clientPubKey, connData.pskKey, connData.clientIP);

    errorCode = m_serverController->uploadTextFileToContainer(container, credentials, configPart, m_serverConfigPath,
                                                              libssh::ScpOverwriteMode::ScpAppendToExisting);

    if (errorCode != ErrorCode::NoError) {
        return connData;
    }

    QString script = QString("sudo docker exec -i $CONTAINER_NAME bash -c 'wg syncconf wg0 <(wg-quick strip %1)'")
                             .arg(m_serverConfigPath);

    errorCode = m_serverController->runScript(
            credentials,
            m_serverController->replaceVars(script, generateProtocolVars(credentials, container)));

    return connData;
}

QSharedPointer<ProtocolConfig> WireguardConfigurator::createConfig(const ServerCredentials &credentials, DockerContainer container,
                                                               const QSharedPointer<ProtocolConfig> &protocolConfig, ErrorCode &errorCode)
{
    QSharedPointer<ProtocolConfig> result;
    
    if (m_isAwg) {
        auto awgConfig = qSharedPointerCast<AwgProtocolConfig>(protocolConfig);
        if (!awgConfig) {
            errorCode = ErrorCode::InternalError;
            return nullptr;
        }
        result = awgConfig;
    } else {
        auto wgConfig = qSharedPointerCast<WireGuardProtocolConfig>(protocolConfig);
        if (!wgConfig) {
            errorCode = ErrorCode::InternalError;
            return nullptr;
        }
        result = wgConfig;
    }

    QString scriptData = amnezia::scriptData(m_configTemplate, container);
    QString config = m_serverController->replaceVars(
            scriptData, generateProtocolVars(credentials, container, protocolConfig));

    ConnectionData connData = prepareWireguardConfig(credentials, container, protocolConfig, errorCode);
    if (errorCode != ErrorCode::NoError) {
        return nullptr;
    }

    config.replace("$WIREGUARD_CLIENT_PRIVATE_KEY", connData.clientPrivKey);
    config.replace("$WIREGUARD_CLIENT_IP", connData.clientIP);
    config.replace("$WIREGUARD_SERVER_PUBLIC_KEY", connData.serverPubKey);
    config.replace("$WIREGUARD_PSK", connData.pskKey);

    ProtocolConfigVariant variant = ProtocolConfig::getProtocolConfigVariant(result);
    std::visit([&connData, &config](const auto &protocolConfig) -> void {
        using ConfigType = std::decay_t<decltype(*protocolConfig)>;
        if constexpr (std::is_same_v<ConfigType, AwgProtocolConfig> || std::is_same_v<ConfigType, WireGuardProtocolConfig>) {
            protocolConfig->clientProtocolConfig.isEmpty = false;
            protocolConfig->clientProtocolConfig.clientId = connData.clientPubKey;
            protocolConfig->clientProtocolConfig.hostname = connData.host;
            protocolConfig->clientProtocolConfig.port = connData.port.toInt();
            protocolConfig->clientProtocolConfig.wireGuardData.clientPrivateKey = connData.clientPrivKey;
            protocolConfig->clientProtocolConfig.wireGuardData.clientIp = connData.clientIP;
            protocolConfig->clientProtocolConfig.wireGuardData.clientPublicKey = connData.clientPubKey;
            protocolConfig->clientProtocolConfig.wireGuardData.pskKey = connData.pskKey;
            protocolConfig->clientProtocolConfig.wireGuardData.serverPubKey = connData.serverPubKey;
            protocolConfig->clientProtocolConfig.wireGuardData.mtu = protocolConfig->serverProtocolConfig.mtu;
            protocolConfig->clientProtocolConfig.wireGuardData.persistentKeepAlive = "25";
            protocolConfig->clientProtocolConfig.wireGuardData.allowedIps = QStringList{"0.0.0.0/0", "::/0"};
            protocolConfig->clientProtocolConfig.nativeConfig = config;
        }
    }, variant);

    return result;
}

void WireguardConfigurator::processConfigWithLocalSettings(const QPair<QString, QString> &dns,
                                                           const bool isApiConfig, QSharedPointer<ProtocolConfig> &protocolConfig)
{
    processConfigWithDnsSettings(dns, protocolConfig);
}

void WireguardConfigurator::processConfigWithExportSettings(const QPair<QString, QString> &dns,
                                                            const bool isApiConfig, QSharedPointer<ProtocolConfig> &protocolConfig)
{
    processConfigWithDnsSettings(dns, protocolConfig);
}
