#include "wireguard_configurator.h"
#include <QApplication>
#include <QProcess>
#include <QString>
#include <QTemporaryDir>
#include <QDebug>
#include <QTemporaryFile>
#include <QJsonDocument>


#include <openssl/rand.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>
#include <openssl/pem.h>


#include "containers/containers_defs.h"
#include "core/server_defs.h"
#include "core/scripts_registry.h"
#include "utilities.h"
#include "core/servercontroller.h"
#include "settings.h"

WireguardConfigurator::WireguardConfigurator(std::shared_ptr<Settings> settings, std::shared_ptr<ServerController> serverController, QObject *parent):
    ConfiguratorBase(settings, serverController, parent)
{

}

WireguardConfigurator::ConnectionData WireguardConfigurator::genClientKeys()
{
    // TODO review
    constexpr size_t EDDSA_KEY_LENGTH = 32;

    ConnectionData connData;

    unsigned char buff[EDDSA_KEY_LENGTH];
    int ret = RAND_priv_bytes(buff, EDDSA_KEY_LENGTH);
    if (ret <=0) return connData;

    EVP_PKEY * pKey = EVP_PKEY_new();
    q_check_ptr(pKey);
    pKey = EVP_PKEY_new_raw_private_key(EVP_PKEY_X25519, NULL, &buff[0], EDDSA_KEY_LENGTH);


    size_t keySize = EDDSA_KEY_LENGTH;

    // save private key
    unsigned char priv[EDDSA_KEY_LENGTH];
    EVP_PKEY_get_raw_private_key(pKey, priv, &keySize);
    connData.clientPrivKey = QByteArray::fromRawData((char*)priv, keySize).toBase64();

    // save public key
    unsigned char pub[EDDSA_KEY_LENGTH];
    EVP_PKEY_get_raw_public_key(pKey, pub, &keySize);
    connData.clientPubKey = QByteArray::fromRawData((char*)pub, keySize).toBase64();

    return connData;
}

WireguardConfigurator::ConnectionData WireguardConfigurator::prepareWireguardConfig(const ServerCredentials &credentials,
    DockerContainer container, const QJsonObject &containerConfig, ErrorCode *errorCode)
{
    WireguardConfigurator::ConnectionData connData = WireguardConfigurator::genClientKeys();
    connData.host = credentials.hostName;

    if (connData.clientPrivKey.isEmpty() || connData.clientPubKey.isEmpty()) {
        if (errorCode) *errorCode = ErrorCode::InternalError;
        return connData;
    }

    ErrorCode e = ErrorCode::NoError;

    // Get list of already created clients (only IP addresses)
    QString nextIpNumber;
    {
        QString script = QString("cat %1 | grep AllowedIPs").arg(amnezia::protocols::wireguard::serverConfigPath);
        QString stdOut;
        auto cbReadStdOut = [&](const QString &data, QSharedPointer<QSsh::SshRemoteProcess> proc) {
            stdOut += data + "\n";
        };

        e = m_serverController->runContainerScript(credentials, container, script, cbReadStdOut);
        if (errorCode && e) {
            *errorCode = e;
            return connData;
        }

        stdOut.replace("AllowedIPs = ", "");
        stdOut.replace("/32", "");
        QStringList ips = stdOut.split("\n", Qt::SkipEmptyParts);

        // Calc next IP address
        if (ips.isEmpty()) {
            nextIpNumber = "2";
        }
        else {
            int next = ips.last().split(".").last().toInt() + 1;
            if (next > 254) {
                if (errorCode) *errorCode = ErrorCode::AddressPoolError;
                return connData;
            }
            nextIpNumber = QString::number(next);
        }
    }

    QString subnetIp = containerConfig.value(config_key::subnet_address).toString(protocols::wireguard::defaultSubnetAddress);
    {
        QStringList l = subnetIp.split(".", Qt::SkipEmptyParts);
        if (l.isEmpty()) {
            if (errorCode) *errorCode = ErrorCode::AddressPoolError;
            return connData;
        }
        l.removeLast();
        l.append(nextIpNumber);

        connData.clientIP = l.join(".");
    }

    // Get keys
    connData.serverPubKey = m_serverController->getTextFileFromContainer(container, credentials, amnezia::protocols::wireguard::serverPublicKeyPath, &e);
    connData.serverPubKey.replace("\n", "");
    if (e) {
        if (errorCode) *errorCode = e;
        return connData;
    }

    connData.pskKey = m_serverController->getTextFileFromContainer(container, credentials, amnezia::protocols::wireguard::serverPskKeyPath, &e);
    connData.pskKey.replace("\n", "");

    if (e) {
        if (errorCode) *errorCode = e;
        return connData;
    }

    // Add client to config
    QString configPart = QString(
        "[Peer]\n"
        "PublicKey = %1\n"
        "PresharedKey = %2\n"
        "AllowedIPs = %3/32\n\n").
            arg(connData.clientPubKey).
            arg(connData.pskKey).
            arg(connData.clientIP);

    e = m_serverController->uploadTextFileToContainer(container, credentials, configPart,
        protocols::wireguard::serverConfigPath, QSsh::SftpOverwriteMode::SftpAppendToExisting);

    if (e) {
        if (errorCode) *errorCode = e;
        return connData;
    }

    e = m_serverController->runScript(credentials,
        m_serverController->replaceVars("sudo docker exec -i $CONTAINER_NAME bash -c 'wg syncconf wg0 <(wg-quick strip /opt/amnezia/wireguard/wg0.conf)'",
            m_serverController->genVarsForScript(credentials, container)));

    return connData;
}

QString WireguardConfigurator::genWireguardConfig(const ServerCredentials &credentials,
    DockerContainer container, const QJsonObject &containerConfig, ErrorCode *errorCode)
{
    QString config = m_serverController->replaceVars(amnezia::scriptData(ProtocolScriptType::wireguard_template, container),
            m_serverController->genVarsForScript(credentials, container, containerConfig));

    ConnectionData connData = prepareWireguardConfig(credentials, container, containerConfig, errorCode);
    if (errorCode && *errorCode) {
        return "";
    }

    config.replace("$WIREGUARD_CLIENT_PRIVATE_KEY", connData.clientPrivKey);
    config.replace("$WIREGUARD_CLIENT_IP", connData.clientIP);
    config.replace("$WIREGUARD_SERVER_PUBLIC_KEY", connData.serverPubKey);
    config.replace("$WIREGUARD_PSK", connData.pskKey);

    QJsonObject jConfig;
    jConfig[config_key::config] = config;

    jConfig[config_key::hostName] = connData.host;
    jConfig[config_key::client_priv_key] = connData.clientPrivKey;
    jConfig[config_key::client_ip] = connData.clientIP;
    jConfig[config_key::client_pub_key] = connData.clientPubKey;
    jConfig[config_key::psk_key] = connData.pskKey;
    jConfig[config_key::server_pub_key] = connData.serverPubKey;

    return QJsonDocument(jConfig).toJson();
}

QString WireguardConfigurator::processConfigWithLocalSettings(QString config)
{
    // TODO replace DNS if it already set
    config.replace("$PRIMARY_DNS", m_settings->primaryDns());
    config.replace("$SECONDARY_DNS", m_settings->secondaryDns());

    QJsonObject jConfig;
    jConfig[config_key::config] = config;

    return QJsonDocument(jConfig).toJson();
}

QString WireguardConfigurator::processConfigWithExportSettings(QString config)
{
    config.replace("$PRIMARY_DNS", m_settings->primaryDns());
    config.replace("$SECONDARY_DNS", m_settings->secondaryDns());

    return config;
}
