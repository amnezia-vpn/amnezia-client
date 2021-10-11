#include "ikev2_configurator.h"
#include <QApplication>
#include <QProcess>
#include <QString>
#include <QTemporaryDir>
#include <QDebug>
#include <QTemporaryFile>
#include <QJsonDocument>

#include "sftpdefs.h"

#include "core/server_defs.h"
#include "containers/containers_defs.h"
#include "core/scripts_registry.h"
#include "utils.h"

Ikev2Configurator::ConnectionData Ikev2Configurator::prepareIkev2Config(const ServerCredentials &credentials,
    DockerContainer container, ErrorCode *errorCode)
{
    Ikev2Configurator::ConnectionData connData;
    connData.host = credentials.hostName;
    connData.clientId = Utils::getRandomString(16);
    connData.password = Utils::getRandomString(16);

    QString certFileName = "/opt/amnezia/ikev2/clients/" + connData.clientId + ".p12";

    QString scriptCreateCert = QString("certutil -z <(head -c 1024 /dev/urandom) "\
                             "-S -c \"IKEv2 VPN CA\" -n \"%1\" "\
                             "-s \"O=IKEv2 VPN,CN=%1\" "\
                             "-k rsa -g 3072 -v 120 "\
                             "-d sql:/etc/ipsec.d -t \",,\" "\
                             "--keyUsage digitalSignature,keyEncipherment "\
                             "--extKeyUsage serverAuth,clientAuth -8 \"%1\"")
            .arg(connData.clientId);

    ErrorCode e = ServerController::runContainerScript(credentials, container, scriptCreateCert);

    QString scriptExportCert = QString("pk12util -W \"%1\" -d sql:/etc/ipsec.d -n \"%2\" -o \"%3\"")
            .arg(connData.password)
            .arg(connData.clientId)
            .arg(certFileName);
    e = ServerController::runContainerScript(credentials, container, scriptExportCert);

    connData.cert = ServerController::getTextFileFromContainer(container, credentials, certFileName, &e);
    qDebug() << "Ikev2Configurator::ConnectionData cert size:" << connData.cert.size();

    return connData;
}

QString Ikev2Configurator::genIkev2Config(const ServerCredentials &credentials,
    DockerContainer container, const QJsonObject &containerConfig, ErrorCode *errorCode)
{
    ConnectionData connData = prepareIkev2Config(credentials, container, errorCode);
    if (errorCode && *errorCode) {
        return "";
    }

    QJsonObject config;
    config[config_key::hostName] = connData.host;
    config[config_key::userName] = connData.clientId;
    config[config_key::cert] = QString(connData.cert.toBase64());
    config[config_key::password] = connData.password;

    return QJsonDocument(config).toJson();
}

