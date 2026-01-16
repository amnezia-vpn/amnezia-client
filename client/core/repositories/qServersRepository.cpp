#include "qServersRepository.h"

#include "core/repositories/secureServersRepository.h"

QServersRepository::QServersRepository(SecureQSettings* settings, QObject *parent)
    : QObject(parent), m_secureRepository(std::make_unique<SecureServersRepository>(settings))
{
}

QServersRepository::~QServersRepository() = default;

ServersRepository* QServersRepository::repository()
{
    return m_secureRepository.get();
}

void QServersRepository::addServer(const ServerConfig &server)
{
    m_secureRepository->addServer(server);
    emit serverAdded(server);
}

void QServersRepository::editServer(int index, const ServerConfig &server)
{
    m_secureRepository->editServer(index, server);
    emit serverEdited(index, server);
}

void QServersRepository::removeServer(int index)
{
    m_secureRepository->removeServer(index);
    emit serverRemoved(index);
}

ServerConfig QServersRepository::server(int index) const
{
    return m_secureRepository->server(index);
}

QVector<ServerConfig> QServersRepository::servers() const
{
    return m_secureRepository->servers();
}

int QServersRepository::serversCount() const
{
    return m_secureRepository->serversCount();
}

int QServersRepository::defaultServerIndex() const
{
    return m_secureRepository->defaultServerIndex();
}

void QServersRepository::setDefaultServer(int index)
{
    m_secureRepository->setDefaultServer(index);
    emit defaultServerChanged(index);
}

void QServersRepository::setDefaultContainer(int serverIndex, DockerContainer container)
{
    m_secureRepository->setDefaultContainer(serverIndex, container);
    ServerConfig serverConfig = m_secureRepository->server(serverIndex);
    emit serverEdited(serverIndex, serverConfig);
}

ContainerConfig QServersRepository::containerConfig(int serverIndex, DockerContainer container) const
{
    return m_secureRepository->containerConfig(serverIndex, container);
}

void QServersRepository::setContainerConfig(int serverIndex, DockerContainer container, const ContainerConfig &config)
{
    m_secureRepository->setContainerConfig(serverIndex, container, config);
    ServerConfig serverConfig = m_secureRepository->server(serverIndex);
    emit serverEdited(serverIndex, serverConfig);
}

void QServersRepository::clearLastConnectionConfig(int serverIndex, DockerContainer container)
{
    m_secureRepository->clearLastConnectionConfig(serverIndex, container);
    ServerConfig serverConfig = m_secureRepository->server(serverIndex);
    emit serverEdited(serverIndex, serverConfig);
}

ServerCredentials QServersRepository::serverCredentials(int index) const
{
    return m_secureRepository->serverCredentials(index);
}

bool QServersRepository::hasServerWithVpnKey(const QString &vpnKey) const
{
    return m_secureRepository->hasServerWithVpnKey(vpnKey);
}
