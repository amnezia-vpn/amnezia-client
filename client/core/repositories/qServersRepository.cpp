#include "qServersRepository.h"

#include "core/repositories/secureServersRepository.h"
#include "settings.h"

QServersRepository::QServersRepository(std::shared_ptr<Settings> settings, QObject *parent)
    : QObject(parent), m_secureRepository(std::make_unique<SecureServersRepository>(settings))
{
}

QServersRepository::~QServersRepository() = default;

ServersRepository* QServersRepository::repository()
{
    return m_secureRepository.get();
}

void QServersRepository::addServer(const QJsonObject &server)
{
    m_secureRepository->addServer(server);
    emit serverAdded(server);
}

void QServersRepository::editServer(int index, const QJsonObject &server)
{
    m_secureRepository->editServer(index, server);
    emit serverEdited(index, server);
}

void QServersRepository::removeServer(int index)
{
    m_secureRepository->removeServer(index);
    emit serverRemoved(index);
}

QJsonObject QServersRepository::server(int index) const
{
    return m_secureRepository->server(index);
}

QJsonArray QServersRepository::serversArray() const
{
    return m_secureRepository->serversArray();
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
    QJsonObject server = m_secureRepository->server(serverIndex);
    emit serverEdited(serverIndex, server);
}

QJsonObject QServersRepository::containerConfig(int serverIndex, DockerContainer container) const
{
    return m_secureRepository->containerConfig(serverIndex, container);
}

void QServersRepository::setContainerConfig(int serverIndex, DockerContainer container, const QJsonObject &config)
{
    m_secureRepository->setContainerConfig(serverIndex, container, config);
    QJsonObject server = m_secureRepository->server(serverIndex);
    emit serverEdited(serverIndex, server);
}

void QServersRepository::clearLastConnectionConfig(int serverIndex, DockerContainer container)
{
    m_secureRepository->clearLastConnectionConfig(serverIndex, container);
    QJsonObject server = m_secureRepository->server(serverIndex);
    emit serverEdited(serverIndex, server);
}

ServerCredentials QServersRepository::serverCredentials(int index) const
{
    return m_secureRepository->serverCredentials(index);
}

bool QServersRepository::hasServerWithVpnKey(const QString &vpnKey) const
{
    return m_secureRepository->hasServerWithVpnKey(vpnKey);
}
