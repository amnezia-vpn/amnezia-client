#include "ikev2_configurator.h"

#include <QDebug>
#include <QJsonDocument>
#include <QProcess>
#include <QString>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QUuid>

#include "containers/containers_defs.h"
#include "core/controllers/serverController.h"
#include "core/models/protocols/ipsecProtocolConfig.h"
#include "core/scripts_registry.h"
#include "core/server_defs.h"
#include "utilities.h"

Ikev2Configurator::Ikev2Configurator(std::shared_ptr<Settings> settings, const QSharedPointer<ServerController> &serverController,
                                     QObject *parent)
    : ConfiguratorBase(settings, serverController, parent)
{
}

Ikev2Configurator::ConnectionData Ikev2Configurator::prepareIkev2Config(const ServerCredentials &serverCredentials,
                                                                        const ContainerConfig &containerConfig, ErrorCode &errorCode)
{
    Ikev2Configurator::ConnectionData connData;
    connData.host = serverCredentials.hostName;
    connData.clientId = Utils::getRandomString(16);
    connData.password = Utils::getRandomString(16);
    connData.password = "";

    QString certFileName = "/opt/amnezia/ikev2/clients/" + connData.clientId + ".p12";

    QString scriptCreateCert = QString("certutil -z <(head -c 1024 /dev/urandom) "
                                       "-S -c \"IKEv2 VPN CA\" -n \"%1\" "
                                       "-s \"O=IKEv2 VPN,CN=%1\" "
                                       "-k rsa -g 3072 -v 120 "
                                       "-d sql:/etc/ipsec.d -t \",,\" "
                                       "--keyUsage digitalSignature,keyEncipherment "
                                       "--extKeyUsage serverAuth,clientAuth -8 \"%1\"")
                                       .arg(connData.clientId);

    errorCode = m_serverController->runContainerScript(containerConfig, serverCredentials, scriptCreateCert);

    QString scriptExportCert =
            QString("pk12util -W \"%1\" -d sql:/etc/ipsec.d -n \"%2\" -o \"%3\"").arg(connData.password).arg(connData.clientId).arg(certFileName);
    errorCode = m_serverController->runContainerScript(containerConfig, serverCredentials, scriptExportCert);

    connData.clientCert =
            m_serverController->getTextFileFromContainer(containerConfig.containerType, serverCredentials, certFileName, errorCode);
    connData.caCert = m_serverController->getTextFileFromContainer(containerConfig.containerType, serverCredentials,
                                                                   "/etc/ipsec.d/ca_cert_base64.p12", errorCode);

    qDebug() << "Ikev2Configurator::ConnectionData client cert size:" << connData.clientCert.size();
    qDebug() << "Ikev2Configurator::ConnectionData ca cert size:" << connData.caCert.size();

    return connData;
}

QSharedPointer<ProtocolConfig> Ikev2Configurator::createConfig(const ServerCredentials &serverCredentials, const ContainerConfig &containerConfig,
                                                               ErrorCode &errorCode)
{
    ConnectionData connData = prepareIkev2Config(serverCredentials, containerConfig, errorCode);
    if (errorCode != ErrorCode::NoError) {
        return nullptr;
    }

    auto baseProtocolConfig = qSharedPointerCast<IpsecProtocolConfig>(
            containerConfig.protocolConfigs.value(ProtocolProps::protoToString(Proto::Ikev2)));
    auto ipsecConfig = QSharedPointer<IpsecProtocolConfig>::create(*baseProtocolConfig);

    ipsecConfig->clientProtocolConfig.isEmpty = false;

    QJsonObject clientConfig;
    clientConfig[config_key::hostName] = connData.host;
    clientConfig[config_key::userName] = connData.clientId;
    clientConfig[config_key::cert] = QString(connData.clientCert.toBase64());
    clientConfig[config_key::password] = connData.password;

    ipsecConfig->clientProtocolConfig.nativeConfig = QJsonDocument(clientConfig).toJson();
    return ipsecConfig;
}

QString Ikev2Configurator::genMobileConfig(const ConnectionData &connData)
{
    QFile file(":/server_scripts/ipsec/mobileconfig.plist");
    file.open(QIODevice::ReadOnly);
    QString config = QString(file.readAll());

    config.replace("$CLIENT_NAME", connData.clientId);
    config.replace("$UUID1", QUuid::createUuid().toString());
    config.replace("$SERVER_ADDR", connData.host);

    QString subStr("$(UUID_GEN)");
    while (config.indexOf(subStr) > 0) {
        config.replace(config.indexOf(subStr), subStr.size(), QUuid::createUuid().toString());
    }

    config.replace("$P12_BASE64", connData.clientCert.toBase64());
    config.replace("$CA_BASE64", connData.caCert.toBase64());

    return config;
}

QString Ikev2Configurator::genStrongSwanConfig(const ConnectionData &connData)
{
    QFile file(":/server_scripts/ipsec/strongswan.profile");
    file.open(QIODevice::ReadOnly);
    QString config = QString(file.readAll());

    config.replace("$CLIENT_NAME", connData.clientId);
    config.replace("$UUID", QUuid::createUuid().toString());
    config.replace("$SERVER_ADDR", connData.host);

    QByteArray cert = connData.clientCert.toBase64();
    cert.replace("\r", "").replace("\n", "");
    config.replace("$P12_BASE64", cert);

    return config;
}
