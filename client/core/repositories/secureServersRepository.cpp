#include "secureServersRepository.h"

#include <QJsonDocument>
#include <QJsonArray>

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
    QJsonArray arr = QJsonDocument::fromJson(value("Servers/serversList").toByteArray()).array();
    for (const QJsonValue &val : arr) {
        m_servers.append(ServerConfig::fromJson(val.toObject()));
    }
    m_defaultServerIndex = value("Servers/defaultServerIndex", 0).toInt();
}

QVariant SecureServersRepository::value(const QString &key, const QVariant &defaultValue) const
{
    return m_settings->value(key, defaultValue);
}

void SecureServersRepository::setValue(const QString &key, const QVariant &value)
{
    m_settings->setValue(key, value);
}

void SecureServersRepository::syncToStorage()
{
    QJsonArray arr;
    for (const ServerConfig &cfg : m_servers) {
        arr.append(cfg.toJson());
    }
    setValue("Servers/serversList", QJsonDocument(arr).toJson());
}

void SecureServersRepository::invalidateCache()
{
    m_servers.clear();
    QJsonArray arr = QJsonDocument::fromJson(value("Servers/serversList").toByteArray()).array();
    for (const QJsonValue &val : arr) {
        m_servers.append(ServerConfig::fromJson(val.toObject()));
    }
    m_defaultServerIndex = value("Servers/defaultServerIndex", 0).toInt();
}

void SecureServersRepository::setServersArray(const QJsonArray &servers)
{
    m_servers.clear();
    for (const QJsonValue &val : servers) {
        m_servers.append(ServerConfig::fromJson(val.toObject()));
    }
    syncToStorage();
}

void SecureServersRepository::addServer(const ServerConfig &server)
{
    m_servers.append(server);
    syncToStorage();
    emit serverAdded(server);
}

void SecureServersRepository::editServer(int index, const ServerConfig &server)
{
    if (index < 0 || index >= m_servers.size()) {
        return;
    }
    m_servers.replace(index, server);
    syncToStorage();
    emit serverEdited(index, server);
}

void SecureServersRepository::removeServer(int index)
{
    if (index < 0 || index >= m_servers.size()) {
        return;
    }
    int defaultIndex = m_defaultServerIndex;
    m_servers.removeAt(index);

    if (defaultIndex == index) {
        setDefaultServer(0);
    } else if (defaultIndex > index) {
        setDefaultServer(defaultIndex - 1);
    }

    if (m_servers.isEmpty()) {
        setDefaultServer(0);
    }

    syncToStorage();
    emit serverRemoved(index);
}

ServerConfig SecureServersRepository::server(int index) const
{
    if (index < 0 || index >= m_servers.size()) {
        return SelfHostedServerConfig{};
    }
    return m_servers.at(index);
}

QVector<ServerConfig> SecureServersRepository::servers() const
{
    return m_servers;
}

int SecureServersRepository::serversCount() const
{
    return m_servers.size();
}

int SecureServersRepository::defaultServerIndex() const
{
    return m_defaultServerIndex;
}

void SecureServersRepository::setDefaultServer(int index)
{
    if (index < 0) {
        return;
    }
    if (m_servers.size() > 0 && index >= m_servers.size()) {
        return;
    }
    if (m_servers.isEmpty() && index != 0) {
        return;
    }
    if (m_defaultServerIndex == index) {
        return;
    }
    m_defaultServerIndex = index;
    setValue("Servers/defaultServerIndex", index);
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

bool SecureServersRepository::hasServerWithCrc(quint16 crc) const
{
    for (const ServerConfig& serverConfig : m_servers) {
        if (static_cast<quint16>(serverConfig.crc()) == crc) {
            return true;
        }
    }
    return false;
}
