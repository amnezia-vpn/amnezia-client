#include "ikev2_configurator.h"

#include <QDebug>
#include <QJsonDocument>
#include <QProcess>
#include <QString>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QUuid>

#include "core/models/containers/containers_defs.h"
#include "core/controllers/selfhosted/serverController.h"
#include "core/scripts_registry.h"
#include "core/server_defs.h"
#include "protocols/protocols_defs.h"
#include "utilities.h"

Ikev2Configurator::Ikev2Configurator(std::shared_ptr<Settings> settings, const QSharedPointer<ServerController> &serverController, QObject *parent)
    : ConfiguratorBase(settings, serverController, parent)
{
}

ConfiguratorBase::Vars Ikev2Configurator::generateProtocolVars(const ServerCredentials &credentials, DockerContainer container,
                                                              const QSharedPointer<ProtocolConfig> &protocolConfig) const
{
    Vars vars = generateCommonVars(credentials, container);

    vars.append({{"$IPSEC_VPN_L2TP_NET", "192.168.42.0/24"}});
    vars.append({{"$IPSEC_VPN_L2TP_POOL", "192.168.42.10-192.168.42.250"}});
    vars.append({{"$IPSEC_VPN_L2TP_LOCAL", "192.168.42.1"}});

    vars.append({{"$IPSEC_VPN_XAUTH_NET", "192.168.43.0/24"}});
    vars.append({{"$IPSEC_VPN_XAUTH_POOL", "192.168.43.10-192.168.43.250"}});

    vars.append({{"$IPSEC_VPN_SHA2_TRUNCBUG", "yes"}});
    vars.append({{"$IPSEC_VPN_VPN_ANDROID_MTU_FIX", "yes"}});
    vars.append({{"$IPSEC_VPN_DISABLE_IKEV2", "no"}});
    vars.append({{"$IPSEC_VPN_DISABLE_L2TP", "no"}});
    vars.append({{"$IPSEC_VPN_DISABLE_XAUTH", "no"}});
    vars.append({{"$IPSEC_VPN_C2C_TRAFFIC", "no"}});

    return vars;
}

Ikev2Configurator::ConnectionData Ikev2Configurator::prepareIkev2Config(const ServerCredentials &credentials, DockerContainer container,
                                                                        ErrorCode &errorCode)
{
    Ikev2Configurator::ConnectionData connData;
    connData.host = credentials.hostName;
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

    errorCode = m_serverController->runContainerScript(credentials, container, scriptCreateCert);

    QString scriptExportCert =
            QString("pk12util -W \"%1\" -d sql:/etc/ipsec.d -n \"%2\" -o \"%3\"").arg(connData.password).arg(connData.clientId).arg(certFileName);
    errorCode = m_serverController->runContainerScript(credentials, container, scriptExportCert);

    connData.clientCert = m_serverController->getTextFileFromContainer(container, credentials, certFileName, errorCode);
    connData.caCert = m_serverController->getTextFileFromContainer(container, credentials, "/etc/ipsec.d/ca_cert_base64.p12", errorCode);

    qDebug() << "Ikev2Configurator::ConnectionData client cert size:" << connData.clientCert.size();
    qDebug() << "Ikev2Configurator::ConnectionData ca cert size:" << connData.caCert.size();

    return connData;
}

QSharedPointer<ProtocolConfig> Ikev2Configurator::createConfig(const ServerCredentials &credentials, DockerContainer container,
                                                            const QSharedPointer<ProtocolConfig> &protocolConfig, ErrorCode &errorCode)
{
    Q_UNUSED(protocolConfig)

    // IKEv2 uses a generic ProtocolConfig - no specific subclass needed
    if (!protocolConfig) {
        errorCode = ErrorCode::InternalError;
        return nullptr;
    }

    ConnectionData connData = prepareIkev2Config(credentials, container, errorCode);
    if (errorCode != ErrorCode::NoError) {
        return nullptr;
    }

    auto ikev2Config = qSharedPointerCast<Ikev2ProtocolConfig>(protocolConfig);
    if (!ikev2Config) {
        errorCode = ErrorCode::InternalError;
        return nullptr;
    }

    ikev2Config->clientProtocolConfig.isEmpty = false;
    ikev2Config->clientProtocolConfig.hostName = connData.host;
    ikev2Config->clientProtocolConfig.userName = connData.clientId;
    ikev2Config->clientProtocolConfig.cert = QString(connData.clientCert.toBase64());
    ikev2Config->clientProtocolConfig.password = connData.password;

    // Generate the appropriate native config based on platform
    QString nativeConfigStr;
    switch (Utils::systemType()) {
    case SystemType::iOS:
        [[fallthrough]];
    case SystemType::macOS:
        nativeConfigStr = genMobileConfig(connData);
        break;
    case SystemType::Android:
        nativeConfigStr = genIkev2Config(connData);
        break;
    default:
        nativeConfigStr = genStrongSwanConfig(connData);
        break;
    }

    ikev2Config->clientProtocolConfig.nativeConfig = nativeConfigStr;
    
    return ikev2Config;
}

QString Ikev2Configurator::genIkev2Config(const ConnectionData &connData)
{
    // Create temporary JSON for Android platform (will be eliminated when android protocols are updated)
    QJsonObject config;
    config[config_key::hostName] = connData.host;
    config[config_key::userName] = connData.clientId;
    config[config_key::cert] = QString(connData.clientCert.toBase64());
    config[config_key::password] = connData.password;

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
