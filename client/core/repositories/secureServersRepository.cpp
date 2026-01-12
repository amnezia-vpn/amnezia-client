#include "secureServersRepository.h"

#include "core/utils/api/apiDefs.h"
#include "settings.h"

SecureServersRepository::SecureServersRepository(std::shared_ptr<Settings> settings)
    : m_settings(settings)
{
}

void SecureServersRepository::addServer(const QJsonObject &server)
{
    m_settings->addServer(server);
}

void SecureServersRepository::editServer(int index, const QJsonObject &server)
{
    m_settings->editServer(index, server);
}

void SecureServersRepository::removeServer(int index)
{
    m_settings->removeServer(index);
    
    int defaultIndex = m_settings->defaultServerIndex();
    if (defaultIndex == index) {
        m_settings->setDefaultServer(0);
    } else if (defaultIndex > index) {
        m_settings->setDefaultServer(defaultIndex - 1);
    }
    
    if (m_settings->serversCount() == 0) {
        m_settings->setDefaultServer(-1);
    }
}

QJsonObject SecureServersRepository::server(int index) const
{
    return m_settings->server(index);
}

QJsonArray SecureServersRepository::serversArray() const
{
    return m_settings->serversArray();
}

int SecureServersRepository::serversCount() const
{
    return m_settings->serversCount();
}

int SecureServersRepository::defaultServerIndex() const
{
    return m_settings->defaultServerIndex();
}

void SecureServersRepository::setDefaultServer(int index)
{
    m_settings->setDefaultServer(index);
}

void SecureServersRepository::setDefaultContainer(int serverIndex, DockerContainer container)
{
    m_settings->setDefaultContainer(serverIndex, container);
}

QJsonObject SecureServersRepository::containerConfig(int serverIndex, DockerContainer container) const
{
    return m_settings->containerConfig(serverIndex, container);
}

void SecureServersRepository::setContainerConfig(int serverIndex, DockerContainer container, const QJsonObject &config)
{
    m_settings->setContainerConfig(serverIndex, container, config);
}

void SecureServersRepository::clearLastConnectionConfig(int serverIndex, DockerContainer container)
{
    m_settings->clearLastConnectionConfig(serverIndex, container);
}

ServerCredentials SecureServersRepository::serverCredentials(int index) const
{
    return m_settings->serverCredentials(index);
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

    QJsonArray servers = m_settings->serversArray();
    for (const auto &serverValue : std::as_const(servers)) {
        QJsonObject server = serverValue.toObject();
        QJsonObject apiConfig = server.value(apiDefs::key::apiConfig).toObject();
        if (apiConfig.isEmpty()) {
            continue;
        }
        QString storedKey = apiConfig.value(apiDefs::key::vpnKey).toString();
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
    return false;
}
