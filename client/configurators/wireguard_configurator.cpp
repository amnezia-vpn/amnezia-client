#include "wireguard_configurator.h"
#include <QApplication>
#include <QProcess>
#include <QString>
#include <QTemporaryDir>
#include <QDebug>
#include <QTemporaryFile>

#include <openssl/rand.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>
#include <openssl/pem.h>

#include "sftpdefs.h"

#include "core/server_defs.h"
#include "containers/containers_defs.h"
#include "core/scripts_registry.h"
#include "utils.h"

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
    pKey = EVP_PKEY_new_raw_private_key(EVP_PKEY_ED25519, NULL, &buff[0], EDDSA_KEY_LENGTH);


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
    DockerContainer container, ErrorCode *errorCode)
{
    WireguardConfigurator::ConnectionData connData = WireguardConfigurator::genClientKeys();
    connData.host = credentials.hostName;

    if (connData.clientPrivKey.isEmpty() || connData.clientPubKey.isEmpty()) {
        if (errorCode) *errorCode = ErrorCode::InternalError;
        return connData;
    }

    ErrorCode e = ErrorCode::NoError;
    connData.serverPubKey = ServerController::getTextFileFromContainer(container, credentials, amnezia::protocols::wireguard::serverPublicKeyPath, &e);
    connData.serverPubKey.replace("\n", "");
    if (e) {
        if (errorCode) *errorCode = e;
        return connData;
    }

    connData.pskKey = ServerController::getTextFileFromContainer(container, credentials, amnezia::protocols::wireguard::serverPskKeyPath, &e);
    connData.pskKey.replace("\n", "");

    if (e) {
        if (errorCode) *errorCode = e;
        return connData;
    }


    QString configPart = QString(
        "[Peer]\n"
        "PublicKey = %1\n"
        "PresharedKey = %2\n"
        "AllowedIPs = $WIREGUARD_SUBNET_IP/$WIREGUARD_SUBNET_CIDR\n\n").
            arg(connData.clientPubKey).
            arg(connData.pskKey);

    configPart = ServerController::replaceVars(configPart, ServerController::genVarsForScript(credentials, container));

    qDebug().noquote() << "Adding wg conf part to server" << configPart;

    e = ServerController::uploadTextFileToContainer(container, credentials, configPart,
        protocols::wireguard::serverConfigPath, QSsh::SftpOverwriteMode::SftpAppendToExisting);

    if (e) {
        if (errorCode) *errorCode = e;
        return connData;
    }

    e = ServerController::runScript(credentials,
        ServerController::replaceVars("sudo docker exec -i $CONTAINER_NAME bash -c 'wg syncconf wg0 <(wg-quick strip /opt/amnezia/wireguard/wg0.conf)'",
            ServerController::genVarsForScript(credentials, container)));

    return connData;
}

Settings &WireguardConfigurator::m_settings()
{
    static Settings s;
    return s;
}

QString WireguardConfigurator::genWireguardConfig(const ServerCredentials &credentials,
    DockerContainer container, const QJsonObject &containerConfig, ErrorCode *errorCode)
{
    QString config = ServerController::replaceVars(amnezia::scriptData(ProtocolScriptType::wireguard_template, container),
            ServerController::genVarsForScript(credentials, container, containerConfig));

    ConnectionData connData = prepareWireguardConfig(credentials, container, errorCode);
    if (errorCode && *errorCode) {
        return "";
    }

    config.replace("$WIREGUARD_CLIENT_PRIVATE_KEY", connData.clientPrivKey);
    config.replace("$WIREGUARD_SERVER_PUBLIC_KEY", connData.serverPubKey);
    config.replace("$WIREGUARD_PSK", connData.pskKey);

    QJsonObject jConfig;
    jConfig[config_key::config] = config;

    jConfig[config_key::hostName] = connData.host;
    jConfig[config_key::client_priv_key] = connData.clientPrivKey;
    jConfig[config_key::client_pub_key] = connData.clientPubKey;
    jConfig[config_key::psk_key] = connData.pskKey;
    jConfig[config_key::server_pub_key] = connData.serverPubKey;

    return QJsonDocument(jConfig).toJson();
}

QString WireguardConfigurator::processConfigWithLocalSettings(QString config)
{
    // TODO replace DNS if it already set
    config.replace("$PRIMARY_DNS", m_settings().primaryDns());
    config.replace("$SECONDARY_DNS", m_settings().secondaryDns());

    QJsonObject jConfig;
    jConfig[config_key::config] = config;

    return QJsonDocument(jConfig).toJson();
}

QString WireguardConfigurator::processConfigWithExportSettings(QString config)
{
    config.replace("$PRIMARY_DNS", m_settings().primaryDns());
    config.replace("$SECONDARY_DNS", m_settings().secondaryDns());

    return config;
}
