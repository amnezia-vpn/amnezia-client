#include "wireguard_configurator.h"

#include <QDebug>
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

#include "containers/containers_defs.h"
#include "core/controllers/serverController.h"
#include "core/models/protocols/awgProtocolConfig.h"
#include "core/models/protocols/wireguardProtocolConfig.h"
#include "core/scripts_registry.h"
#include "core/server_defs.h"
#include "settings.h"
#include "utilities.h"

WireguardConfigurator::WireguardConfigurator(std::shared_ptr<Settings> settings, const QSharedPointer<ServerController> &serverController,
                                             bool isAwg, QObject *parent)
    : ConfiguratorBase(settings, serverController, parent), m_isAwg(isAwg)
{
    m_serverConfigPath = m_isAwg ? amnezia::protocols::awg::serverConfigPath : amnezia::protocols::wireguard::serverConfigPath;
    m_serverPublicKeyPath = m_isAwg ? amnezia::protocols::awg::serverPublicKeyPath : amnezia::protocols::wireguard::serverPublicKeyPath;
    m_serverPskKeyPath = m_isAwg ? amnezia::protocols::awg::serverPskKeyPath : amnezia::protocols::wireguard::serverPskKeyPath;
    m_configTemplate = m_isAwg ? ProtocolScriptType::awg_template : ProtocolScriptType::wireguard_template;

    m_protocolName = m_isAwg ? config_key::awg : config_key::wireguard;
    m_defaultPort = m_isAwg ? protocols::wireguard::defaultPort : protocols::awg::defaultPort;
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

WireguardConfigurator::ConnectionData WireguardConfigurator::prepareWireguardConfig(const ServerCredentials &serverCredentials,
                                                                                    const ContainerConfig &containerConfig,
                                                                                    ErrorCode &errorCode)
{
    WireguardConfigurator::ConnectionData connData = WireguardConfigurator::genClientKeys();
    connData.host = serverCredentials.hostName;
    QString port = m_defaultPort;
    QString subnetAddress = protocols::wireguard::defaultSubnetAddress;

    if (containerConfig.protocolConfigs.contains(m_protocolName)) {
        auto existingConfig = containerConfig.protocolConfigs.value(m_protocolName);
        if (existingConfig) {
            auto protocolConfigVariant = ProtocolConfig::getProtocolConfigVariant(existingConfig);
            std::visit(
                    [&port, &subnetAddress](const auto &configPtr) {
                        if constexpr (requires {
                                          configPtr->serverProtocolConfig;
                                          configPtr->serverProtocolConfig.port;
                                          configPtr->serverProtocolConfig.subnetAddress;
                                      }) {
                            if (!configPtr->serverProtocolConfig.port.isEmpty()) {
                                port = configPtr->serverProtocolConfig.port;
                            }
                            if (!configPtr->serverProtocolConfig.subnetAddress.isEmpty()) {
                                subnetAddress = configPtr->serverProtocolConfig.subnetAddress;
                            }
                        }
                    },
                    protocolConfigVariant);
        }
    }

    connData.port = port;

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

    errorCode = m_serverController->runContainerScript(containerConfig, serverCredentials, getIpsScript, cbReadStdOut);
    if (errorCode != ErrorCode::NoError) {
        return connData;
    }
    auto ips = getIpsFromConf(stdOut);

    QHostAddress nextIp = [&] {
        QHostAddress result;
        QHostAddress lastIp;
        if (ips.empty()) {
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
            m_serverController->getTextFileFromContainer(containerConfig.containerType, serverCredentials, m_serverPublicKeyPath, errorCode);
    connData.serverPubKey.replace("\n", "");
    if (errorCode != ErrorCode::NoError) {
        return connData;
    }

    connData.pskKey =
            m_serverController->getTextFileFromContainer(containerConfig.containerType, serverCredentials, m_serverPskKeyPath, errorCode);
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

    errorCode = m_serverController->uploadTextFileToContainer(containerConfig.containerType, serverCredentials, configPart, m_serverConfigPath,
                                                              libssh::ScpOverwriteMode::ScpAppendToExisting);

    if (errorCode != ErrorCode::NoError) {
        return connData;
    }

    QString script = QString("sudo docker exec -i $CONTAINER_NAME bash -c 'wg syncconf wg0 <(wg-quick strip %1)'").arg(m_serverConfigPath);

    errorCode = m_serverController->runScript(
            serverCredentials,
            m_serverController->replaceVars(script, m_serverController->genVarsForScript(containerConfig, serverCredentials)));

    return connData;
}

QSharedPointer<ProtocolConfig> WireguardConfigurator::createConfig(const ServerCredentials &serverCredentials,
                                                                   const ContainerConfig &containerConfig, ErrorCode &errorCode)
{
    QString scriptData = amnezia::scriptData(m_configTemplate, containerConfig.containerType);
    QString config = m_serverController->replaceVars(scriptData, m_serverController->genVarsForScript(containerConfig, serverCredentials));

    ConnectionData connData = prepareWireguardConfig(serverCredentials, containerConfig, errorCode);
    if (errorCode != ErrorCode::NoError) {
        return nullptr;
    }

    config.replace("$WIREGUARD_CLIENT_PRIVATE_KEY", connData.clientPrivKey);
    config.replace("$WIREGUARD_CLIENT_IP", connData.clientIP);
    config.replace("$WIREGUARD_SERVER_PUBLIC_KEY", connData.serverPubKey);
    config.replace("$WIREGUARD_PSK", connData.pskKey);

    QString mtu = m_isAwg ? protocols::awg::defaultMtu : protocols::wireguard::defaultMtu;

    QSharedPointer<ProtocolConfig> protocolConfig;
    if (m_isAwg) {
        auto baseProtocolConfig = qSharedPointerCast<AwgProtocolConfig>(
                containerConfig.protocolConfigs.value(config_key::awg));
        protocolConfig = QSharedPointer<AwgProtocolConfig>::create(*baseProtocolConfig);
    } else {
        auto baseProtocolConfig = qSharedPointerCast<WireGuardProtocolConfig>(
                containerConfig.protocolConfigs.value(config_key::wireguard));
        protocolConfig = QSharedPointer<WireGuardProtocolConfig>::create(*baseProtocolConfig);
    }

    auto protocolConfigVariant = ProtocolConfig::getProtocolConfigVariant(protocolConfig);
    std::visit(
            [&](const auto &configPtr) {
                if constexpr (requires {
                                  configPtr->serverProtocolConfig;
                                  configPtr->clientProtocolConfig;
                                  configPtr->clientProtocolConfig.wireGuardData;
                              }) {
                    configPtr->serverProtocolConfig.port = connData.port;
                    configPtr->serverProtocolConfig.transportProto = "udp";
                    configPtr->serverProtocolConfig.subnetAddress = "";

                    configPtr->clientProtocolConfig.isEmpty = false;
                    configPtr->clientProtocolConfig.clientId = connData.clientPubKey;
                    configPtr->clientProtocolConfig.hostname = connData.host;
                    configPtr->clientProtocolConfig.port = connData.port.toInt();
                    configPtr->clientProtocolConfig.nativeConfig = config;

                    configPtr->clientProtocolConfig.wireGuardData.clientIp = connData.clientIP;
                    configPtr->clientProtocolConfig.wireGuardData.clientPrivateKey = connData.clientPrivKey;
                    configPtr->clientProtocolConfig.wireGuardData.clientPublicKey = connData.clientPubKey;
                    configPtr->clientProtocolConfig.wireGuardData.pskKey = connData.pskKey;
                    configPtr->clientProtocolConfig.wireGuardData.serverPubKey = connData.serverPubKey;
                    configPtr->clientProtocolConfig.wireGuardData.mtu = mtu;
                    configPtr->clientProtocolConfig.wireGuardData.persistentKeepAlive = "25";
                    configPtr->clientProtocolConfig.wireGuardData.allowedIps = QStringList { "0.0.0.0/0", "::/0" };
                }
            },
            protocolConfigVariant);

    return protocolConfig;
}

QSharedPointer<ProtocolConfig> WireguardConfigurator::processConfigWithLocalSettings(const QPair<QString, QString> &dns,
                                                                                     const bool isApiConfig,
                                                                                     QSharedPointer<ProtocolConfig> protocolConfig)
{
    return ConfiguratorBase::processConfigWithLocalSettings(dns, isApiConfig, protocolConfig);
}

QSharedPointer<ProtocolConfig> WireguardConfigurator::processConfigWithExportSettings(const QPair<QString, QString> &dns,
                                                                                      QSharedPointer<ProtocolConfig> protocolConfig)
{
    return ConfiguratorBase::processConfigWithExportSettings(dns, protocolConfig);
}
