#include "ikev2Configurator.h"

#include <QDebug>
#include <QJsonDocument>
#include <QProcess>
#include <QString>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QUuid>

#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"
#include "core/utils/selfhosted/sshSession.h"
#include "core/utils/selfhosted/scriptsRegistry.h"
#include "core/utils/utilities.h"
#include "core/models/protocols/ikev2ProtocolConfig.h"

Ikev2Configurator::Ikev2Configurator(SshSession* sshSession, QObject *parent)
    : ConfiguratorBase(sshSession, parent)
{
}

Ikev2Configurator::ConnectionData Ikev2Configurator::prepareIkev2Config(const ServerCredentials &credentials, DockerContainer container,
                                                                        ErrorCode &errorCode)
{
    Ikev2Configurator::ConnectionData connData;
    connData.host = credentials.hostName;
    connData.clientId = Utils::getRandomString(16);
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

    errorCode = m_sshSession->runContainerScript(credentials, container, scriptCreateCert);

    QString scriptExportCert =
            QString("pk12util -W \"%1\" -d sql:/etc/ipsec.d -n \"%2\" -o \"%3\"").arg(connData.password).arg(connData.clientId).arg(certFileName);
    errorCode = m_sshSession->runContainerScript(credentials, container, scriptExportCert);

    connData.clientCert = m_sshSession->getTextFileFromContainer(container, credentials, certFileName, errorCode);
    connData.caCert = m_sshSession->getTextFileFromContainer(container, credentials, "/etc/ipsec.d/ca_cert_base64.p12", errorCode);

    qDebug() << "Ikev2Configurator::ConnectionData client cert size:" << connData.clientCert.size();
    qDebug() << "Ikev2Configurator::ConnectionData ca cert size:" << connData.caCert.size();

    return connData;
}

ProtocolConfig Ikev2Configurator::createConfig(const ServerCredentials &credentials, DockerContainer container, const ContainerConfig &containerConfig,
                                               const DnsSettings &dnsSettings,
                                               ErrorCode &errorCode)
{
    const Ikev2ServerConfig* serverConfig = nullptr;
    if (auto* ikev2Config = containerConfig.protocolConfig.as<Ikev2ProtocolConfig>()) {
        serverConfig = &ikev2Config->serverConfig;
    }

    ConnectionData connData = prepareIkev2Config(credentials, container, errorCode);
    if (errorCode != ErrorCode::NoError) {
        return Ikev2ProtocolConfig{};
    }

    QString configJson = genIkev2Config(connData);
    QJsonDocument doc = QJsonDocument::fromJson(configJson.toUtf8());
    QJsonObject configObj = doc.object();

    Ikev2ProtocolConfig protocolConfig;
    if (serverConfig) {
        protocolConfig.serverConfig = *serverConfig;
    } else {
        protocolConfig.serverConfig.hostName = connData.host;
    }
    
    Ikev2ClientConfig clientConfig;
    clientConfig.nativeConfig = configJson;
    clientConfig.hostName = connData.host;
    clientConfig.userName = connData.clientId;
    clientConfig.cert = QString(connData.clientCert.toBase64());
    clientConfig.password = connData.password;
    clientConfig.clientId = connData.clientId;
    
    protocolConfig.setClientConfig(clientConfig);
    
    return protocolConfig;
}

QString Ikev2Configurator::genIkev2Config(const ConnectionData &connData)
{
    QJsonObject config;
    config[configKey::hostName] = connData.host;
    config[configKey::userName] = connData.clientId;
    config[configKey::cert] = QString(connData.clientCert.toBase64());
    config[configKey::password] = connData.password;

    return QJsonDocument(config).toJson();
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
