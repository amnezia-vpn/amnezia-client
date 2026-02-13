#include "secureServersRepository.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QThread>
#include <QCoreApplication>

#include "core/utils/api/apiEnums.h"
#include "core/utils/constants/apiKeys.h"
#include "core/utils/constants/apiConstants.h"
#include "core/models/serverConfig.h"
#include "core/models/containerConfig.h"
#include "core/utils/protocolEnum.h"
#include "core/protocols/protocolUtils.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"

SecureServersRepository::SecureServersRepository(SecureQSettings* settings, QObject *parent)
    : QObject(parent), m_settings(settings)
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
    servers.append(server.toJson());
    setServersArray(servers);
    emit serverAdded(server);
}

void SecureServersRepository::editServer(int index, const ServerConfig &server)
{
    QJsonArray servers = serversArray();
    if (index < 0 || index >= servers.size()) {
        return;
    }
    servers.replace(index, server.toJson());
    setServersArray(servers);
    emit serverEdited(index, server);
}

void SecureServersRepository::removeServer(int index)
{
    QJsonArray servers = serversArray();
    if (index < 0 || index >= servers.size()) {
        return;
    }
    
    int defaultIndex = defaultServerIndex();
    
    servers.removeAt(index);
    setServersArray(servers);
    
    if (defaultIndex == index) {
        setDefaultServer(0);
    } else if (defaultIndex > index) {
        setDefaultServer(defaultIndex - 1);
    }
    
    if (serversCount() == 0) {
        setDefaultServer(0);
    }
    
    emit serverRemoved(index);
}

ServerConfig SecureServersRepository::server(int index) const
{
    const QJsonArray &servers = serversArray();
    if (index < 0 || index >= servers.size()) {
        return SelfHostedServerConfig{};
    }
    return ServerConfig::fromJson(servers.at(index).toObject());
}

QVector<ServerConfig> SecureServersRepository::servers() const
{
    QVector<ServerConfig> result;
    const QJsonArray &serversArray = this->serversArray();
    for (const QJsonValue &val : serversArray) {
        result.append(ServerConfig::fromJson(val.toObject()));
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
    QJsonArray servers = serversArray();
    
    if (servers.size() > 0 && index >= servers.size()) {
        return;
    }
    
    if (servers.size() == 0 && index != 0) {
        return;
    }
    
    int currentIndex = defaultServerIndex();
    if (currentIndex == index) {
        return;
    }
    
    setValue("Servers/defaultServerIndex", index);
    m_settings->sync();
    emit defaultServerChanged(index);
}

void SecureServersRepository::setDefaultContainer(int serverIndex, DockerContainer container)
{
    ServerConfig config = server(serverIndex);
    config.visit([container](auto& arg) {
        arg.defaultContainer = container;
    });
    editServer(serverIndex, config);
}

ContainerConfig SecureServersRepository::containerConfig(int serverIndex, DockerContainer container) const
{
    ServerConfig config = server(serverIndex);
    return config.containerConfig(container);
}

void SecureServersRepository::setContainerConfig(int serverIndex, DockerContainer container, const ContainerConfig &config)
{
    ServerConfig serverConfig = server(serverIndex);
    serverConfig.visit([container, &config](auto& arg) {
        arg.containers[container] = config;
    });
    editServer(serverIndex, serverConfig);
}

void SecureServersRepository::clearLastConnectionConfig(int serverIndex, DockerContainer container)
{
    ServerConfig serverConfig = server(serverIndex);
    ContainerConfig containerCfg = serverConfig.containerConfig(container);
    
    containerCfg.protocolConfig.clearClientConfig();
    
    setContainerConfig(serverIndex, container, containerCfg);
}

ServerCredentials SecureServersRepository::serverCredentials(int index) const
{
    ServerConfig config = server(index);
    
    if (config.isSelfHosted()) {
        const SelfHostedServerConfig* selfHosted = config.as<SelfHostedServerConfig>();
        if (!selfHosted) return ServerCredentials();
        auto creds = selfHosted->credentials();
        if (creds.has_value()) {
            return creds.value();
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
        if (serverConfig.isApiV1()) {
            const ApiV1ServerConfig* apiV1 = serverConfig.as<ApiV1ServerConfig>();
            if (!apiV1) continue;
            QString storedKey = apiV1->vpnKey();
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
        } else if (serverConfig.isApiV2()) {
            const ApiV2ServerConfig* apiV2 = serverConfig.as<ApiV2ServerConfig>();
            if (!apiV2) continue;
            QString storedKey = apiV2->vpnKey();
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
