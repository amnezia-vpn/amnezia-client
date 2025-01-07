#include "wireguard_configurator.h"

#include <QDebug>
#include <QJsonDocument>
#include <QProcess>
#include <QString>
#include <QTemporaryDir>
#include <QTemporaryFile>

#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>

#include "containers/containers_defs.h"
#include "core/controllers/serverController.h"
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

WireguardConfigurator::ConnectionData WireguardConfigurator::prepareWireguardConfig(const ServerCredentials &credentials,
                                                                                    DockerContainer container,
                                                                                    const QJsonObject &containerConfig, ErrorCode &errorCode)
{
    WireguardConfigurator::ConnectionData connData = WireguardConfigurator::genClientKeys();
    connData.host = credentials.hostName;
    connData.port = containerConfig.value(m_protocolName).toObject().value(config_key::port).toString(m_defaultPort);

    if (connData.clientPrivKey.isEmpty() || connData.clientPubKey.isEmpty()) {
        errorCode = ErrorCode::InternalError;
        return connData;
    }

    // Get list of already created clients (only IP addresses)
    QString nextIpNumber;
    {
        QString script = QString("cat %1 | grep AllowedIPs").arg(m_serverConfigPath);
        QString stdOut;
        auto cbReadStdOut = [&](const QString &data, libssh::Client &) {
            stdOut += data + "\n";
            return ErrorCode::NoError;
        };

        errorCode = m_serverController->runContainerScript(credentials, container, script, cbReadStdOut);
        if (errorCode != ErrorCode::NoError) {
            return connData;
        }

        stdOut.replace("AllowedIPs = ", "");
        stdOut.replace("/32", "");
        QStringList ips = stdOut.split("\n", Qt::SkipEmptyParts);

        // remove extra IPs from each line for case when user manually edited the wg0.conf
        // and added there more IPs for route his itnernal networks, like:
        //     ...
        //     AllowedIPs = 10.8.1.6/32, 192.168.1.0/24, 192.168.2.0/24, ...
        //     ...
        // without this code - next IP would be 1 if last item in 'ips' has format above
        QStringList vpnIps;
        for (const auto &ip : ips) {
          vpnIps.append(ip.split(",", Qt::SkipEmptyParts).first().trimmed());
        }
        ips = vpnIps;

        // Calc next IP address
        if (ips.isEmpty()) {
            nextIpNumber = "2";
        } else {
            int next = ips.last().split(".").last().toInt() + 1;
            if (next > 254) {
                errorCode = ErrorCode::AddressPoolError;
                return connData;
            }
            nextIpNumber = QString::number(next);
        }
    }

    QString subnetIp = containerConfig.value(m_protocolName).toObject().value(config_key::subnet_address).toString(protocols::wireguard::defaultSubnetAddress);
    {
        QStringList l = subnetIp.split(".", Qt::SkipEmptyParts);
        if (l.isEmpty()) {
            errorCode = ErrorCode::AddressPoolError;
            return connData;
        }
        l.removeLast();
        l.append(nextIpNumber);

        connData.clientIP = l.join(".");
    }

    // Get keys
    connData.serverPubKey = m_serverController->getTextFileFromContainer(container, credentials, m_serverPublicKeyPath, errorCode);
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

    QString script = QString("sudo docker exec -i $CONTAINER_NAME bash -c 'wg syncconf wg0 <(wg-quick strip %1)'").arg(m_serverConfigPath);

    errorCode = m_serverController->runScript(
            credentials, m_serverController->replaceVars(script, m_serverController->genVarsForScript(credentials, container)));

    return connData;
}

QString WireguardConfigurator::createConfig(const ServerCredentials &credentials, DockerContainer container,
                                            const QJsonObject &containerConfig, ErrorCode &errorCode)
{
    QString scriptData = amnezia::scriptData(m_configTemplate, container);
    QString config =
            m_serverController->replaceVars(scriptData, m_serverController->genVarsForScript(credentials, container, containerConfig));

    ConnectionData connData = prepareWireguardConfig(credentials, container, containerConfig, errorCode);
    if (errorCode != ErrorCode::NoError) {
        return "";
    }

    config.replace("$WIREGUARD_CLIENT_PRIVATE_KEY", connData.clientPrivKey);
    config.replace("$WIREGUARD_CLIENT_IP", connData.clientIP);
    config.replace("$WIREGUARD_SERVER_PUBLIC_KEY", connData.serverPubKey);
    config.replace("$WIREGUARD_PSK", connData.pskKey);

    const QJsonObject &wireguarConfig = containerConfig.value(ProtocolProps::protoToString(Proto::WireGuard)).toObject();
    QJsonObject jConfig;
    jConfig[config_key::config] = config;

    jConfig[config_key::hostName] = connData.host;
    jConfig[config_key::port] = connData.port.toInt();
    jConfig[config_key::client_priv_key] = connData.clientPrivKey;
    jConfig[config_key::client_ip] = connData.clientIP;
    jConfig[config_key::client_pub_key] = connData.clientPubKey;
    jConfig[config_key::psk_key] = connData.pskKey;
    jConfig[config_key::server_pub_key] = connData.serverPubKey;
    jConfig[config_key::mtu] = wireguarConfig.value(config_key::mtu).toString(protocols::wireguard::defaultMtu);

    jConfig[config_key::persistent_keep_alive] = "25";
    QJsonArray allowedIps { "0.0.0.0/0", "::/0" };
    jConfig[config_key::allowed_ips] = allowedIps;

    jConfig[config_key::clientId] = connData.clientPubKey;

    return QJsonDocument(jConfig).toJson();
}

QString WireguardConfigurator::processConfigWithLocalSettings(const QPair<QString, QString> &dns, const bool isApiConfig,
                                                              QString &protocolConfigString)
{
    processConfigWithDnsSettings(dns, protocolConfigString);

    return protocolConfigString;
}

QString WireguardConfigurator::processConfigWithExportSettings(const QPair<QString, QString> &dns, const bool isApiConfig,
                                                               QString &protocolConfigString)
{
    processConfigWithDnsSettings(dns, protocolConfigString);

    return protocolConfigString;
}
