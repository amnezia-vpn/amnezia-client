#include "secureServersRepository.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QThread>
#include <QCoreApplication>

#include "core/utils/api/apiDefs.h"
#include "core/models/serverConfig.h"
#include "core/models/containerConfig.h"
#include "core/protocols/protocolsDefs.h"

SecureServersRepository::SecureServersRepository(SecureQSettings* settings)
    : m_settings(settings)
{
}

QVariant SecureServersRepository::value(const QString &key, const QVariant &defaultValue) const
{
    QVariant returnValue;
    if (QThread::currentThread() == QCoreApplication::instance()->thread()) {
        returnValue = m_settings->value(key, defaultValue);
    } else {
        QMetaObject::invokeMethod(m_settings, "value", Qt::BlockingQueuedConnection, Q_RETURN_ARG(QVariant, returnValue),
                                  Q_ARG(const QString &, key), Q_ARG(const QVariant &, defaultValue));
    }
    return returnValue;
}

void SecureServersRepository::setValue(const QString &key, const QVariant &value)
{
    if (QThread::currentThread() == QCoreApplication::instance()->thread()) {
        m_settings->setValue(key, value);
    } else {
        QMetaObject::invokeMethod(m_settings, "setValue", Qt::BlockingQueuedConnection, Q_ARG(const QString &, key),
                                  Q_ARG(const QVariant &, value));
    }
}

QJsonArray SecureServersRepository::serversArray() const
{
    return QJsonDocument::fromJson(value("Servers/serversList").toByteArray()).array();
}

void SecureServersRepository::setServersArray(const QJsonArray &servers)
{
    setValue("Servers/serversList", QJsonDocument(servers).toJson());
    m_settings->sync();
}

void SecureServersRepository::addServer(const ServerConfig &server)
{
    QJsonArray servers = serversArray();
    servers.append(ServerConfigUtils::toJson(server));
    setServersArray(servers);
}

void SecureServersRepository::editServer(int index, const ServerConfig &server)
{
    QJsonArray servers = serversArray();
    if (index >= servers.size()) {
        return;
    }
    servers.replace(index, ServerConfigUtils::toJson(server));
    setServersArray(servers);
}

void SecureServersRepository::removeServer(int index)
{
    QJsonArray servers = serversArray();
    if (index >= servers.size()) {
        return;
    }
    
    servers.removeAt(index);
    setServersArray(servers);
    
    int defaultIndex = defaultServerIndex();
    if (defaultIndex == index) {
        setDefaultServer(0);
    } else if (defaultIndex > index) {
        setDefaultServer(defaultIndex - 1);
    }
    
    if (serversCount() == 0) {
        setDefaultServer(-1);
    }
}

ServerConfig SecureServersRepository::server(int index) const
{
    const QJsonArray &servers = serversArray();
    if (index >= servers.size()) {
        return SelfHostedServerConfig{};
    }
    return ServerConfigUtils::fromJson(servers.at(index).toObject());
}

QVector<ServerConfig> SecureServersRepository::servers() const
{
    QVector<ServerConfig> result;
    const QJsonArray &serversArray = this->serversArray();
    for (const QJsonValue &val : serversArray) {
        result.append(ServerConfigUtils::fromJson(val.toObject()));
    }
    return result;
}

int SecureServersRepository::serversCount() const
{
    return serversArray().size();
}

int SecureServersRepository::defaultServerIndex() const
{
    return value("Servers/defaultServerIndex", 0).toInt();
}

void SecureServersRepository::setDefaultServer(int index)
{
    setValue("Servers/defaultServerIndex", index);
    m_settings->sync();
}

void SecureServersRepository::setDefaultContainer(int serverIndex, DockerContainer container)
{
    ServerConfig config = server(serverIndex);
    ServerConfigUtils::visit(config, [container](auto& arg) {
        arg.defaultContainer = container;
    });
    editServer(serverIndex, config);
}

ContainerConfig SecureServersRepository::containerConfig(int serverIndex, DockerContainer container) const
{
    ServerConfig config = server(serverIndex);
    return ServerConfigUtils::containerConfig(config, container);
}

void SecureServersRepository::setContainerConfig(int serverIndex, DockerContainer container, const ContainerConfig &config)
{
    ServerConfig serverConfig = server(serverIndex);
    ServerConfigUtils::visit(serverConfig, [container, &config](auto& arg) {
        arg.containers[container] = config;
    });
    editServer(serverIndex, serverConfig);
}

void SecureServersRepository::clearLastConnectionConfig(int serverIndex, DockerContainer container)
{
    ServerConfig serverConfig = server(serverIndex);
    ContainerConfig containerCfg = ServerConfigUtils::containerConfig(serverConfig, container);
    
    ProtocolConfigUtils::clearClientConfig(containerCfg.protocolConfig);
    
    setContainerConfig(serverIndex, container, containerCfg);
}

ServerCredentials SecureServersRepository::serverCredentials(int index) const
{
    ServerConfig config = server(index);
    
    if (ServerConfigUtils::isSelfHosted(config)) {
        const SelfHostedServerConfig& selfHosted = ServerConfigUtils::asSelfHosted(config);
        if (selfHosted.hasCredentials()) {
            ServerCredentials cred;
            cred.hostName = selfHosted.hostName;
            cred.userName = selfHosted.userName.value_or(QString());
            cred.secretData = selfHosted.password.value_or(QString());
            cred.port = selfHosted.port.value_or(22);
            return cred;
        }
    }
    
    return ServerCredentials{};
}

bool SecureServersRepository::hasServerWithVpnKey(const QString &vpnKey) const
{
    QString normalizedInput = vpnKey.trimmed();
    if (normalizedInput.startsWith(QStringLiteral("vpn://"), Qt::CaseInsensitive)) {
        normalizedInput = normalizedInput.mid(QStringLiteral("vpn://").size());
    }
    if (normalizedInput.isEmpty()) {
        return false;
    }

    QVector<ServerConfig> serversList = servers();
    for (const ServerConfig& serverConfig : serversList) {
        if (ServerConfigUtils::isApiV1Config(serverConfig)) {
            const ApiV1ServerConfig& apiV1 = ServerConfigUtils::asApiV1(serverConfig);
            QString storedKey = apiV1.vpnKey();
            if (storedKey.isEmpty()) {
                continue;
            }
            QString normalizedStored = storedKey.trimmed();
            if (normalizedStored.startsWith(QStringLiteral("vpn://"), Qt::CaseInsensitive)) {
                normalizedStored = normalizedStored.mid(QStringLiteral("vpn://").size());
            }
            if (normalizedInput == normalizedStored) {
                return true;
            }
        } else if (ServerConfigUtils::isApiV2Config(serverConfig)) {
            const ApiV2ServerConfig& apiV2 = ServerConfigUtils::asApiV2(serverConfig);
            QString storedKey = apiV2.vpnKey();
            if (storedKey.isEmpty()) {
                continue;
            }
            QString normalizedStored = storedKey.trimmed();
            if (normalizedStored.startsWith(QStringLiteral("vpn://"), Qt::CaseInsensitive)) {
                normalizedStored = normalizedStored.mid(QStringLiteral("vpn://").size());
            }
            if (normalizedInput == normalizedStored) {
                return true;
            }
        }
    }
    return false;
}
