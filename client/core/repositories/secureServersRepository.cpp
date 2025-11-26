#include "secureServersRepository.h"

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

