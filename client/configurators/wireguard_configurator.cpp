#include "wireguard_configurator.h"
#include <QApplication>
#include <QProcess>
#include <QString>
#include <QTemporaryDir>
#include <QDebug>
#include <QTemporaryFile>

#include "sftpdefs.h"

#include "core/server_defs.h"
#include "protocols/protocols_defs.h"
#include "core/scripts_registry.h"
#include "utils.h"

QProcessEnvironment WireguardConfigurator::prepareEnv()
{
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    QString pathEnvVar = env.value("PATH");

#ifdef Q_OS_WIN
    pathEnvVar.clear();
    pathEnvVar.prepend(QDir::toNativeSeparators(QApplication::applicationDirPath()) + "\\wireguard;");
#else
    pathEnvVar.prepend(QDir::toNativeSeparators(QApplication::applicationDirPath()) + "/Contents/MacOS");
#endif

    env.insert("PATH", pathEnvVar);
    qDebug().noquote() << "ENV PATH" << pathEnvVar;
    return env;
}

WireguardConfigurator::ConnectionData WireguardConfigurator::genClientKeys()
{
    ConnectionData connData;

    QString program;
#ifdef Q_OS_WIN
    program = QDir::toNativeSeparators(QApplication::applicationDirPath()) + "\\wireguard\\wg.exe";
#else
    program = QDir::toNativeSeparators(QApplication::applicationDirPath()) + "/Contents/MacOS/wg";
#endif

    // Priv
    {
        QProcess p;
        p.setProcessEnvironment(prepareEnv());
        p.setProcessChannelMode(QProcess::MergedChannels);
        p.setProgram(program);

        p.setArguments(QStringList() << "genkey");

        p.start();
        p.waitForFinished();

        connData.clientPrivKey = QString(p.readAll());
        connData.clientPrivKey.replace("\r", "");
        connData.clientPrivKey.replace("\n", "");
    }

    // Pub
    {
        QProcess p;
        p.setProcessEnvironment(prepareEnv());
        p.setProcessChannelMode(QProcess::MergedChannels);
        p.setProgram(program);

        p.setArguments(QStringList() << "pubkey");

        p.start();
        p.write(connData.clientPrivKey.toUtf8());
        p.closeWriteChannel();
        p.waitForFinished();

        connData.clientPubKey = QString(p.readAll());
        connData.clientPubKey.replace("\r", "");
        connData.clientPubKey.replace("\n", "");
    }

    return connData;
}

WireguardConfigurator::ConnectionData WireguardConfigurator::prepareWireguardConfig(const ServerCredentials &credentials,
    DockerContainer container, ErrorCode *errorCode)
{
    WireguardConfigurator::ConnectionData connData = WireguardConfigurator::genClientKeys();
    connData.host = credentials.hostName;

    if (connData.clientPrivKey.isEmpty() || connData.clientPubKey.isEmpty()) {
        if (errorCode) *errorCode = ErrorCode::EasyRsaExecutableMissing;
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

    e = ServerController::runScript(ServerController::sshParams(credentials),
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

    qDebug().noquote() << config;
    return config;
}

QString WireguardConfigurator::processConfigWithLocalSettings(QString config)
{
    // TODO replace DNS if it already set
    config.replace("$PRIMARY_DNS", m_settings().primaryDns());
    config.replace("$SECONDARY_DNS", m_settings().secondaryDns());

    return config;
}

QString WireguardConfigurator::processConfigWithExportSettings(QString config)
{
    config.replace("$PRIMARY_DNS", m_settings().primaryDns());
    config.replace("$SECONDARY_DNS", m_settings().secondaryDns());

    return config;
}
