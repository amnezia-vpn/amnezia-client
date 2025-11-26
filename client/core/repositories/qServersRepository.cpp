#include "qServersRepository.h"

#include "settings.h"

QServersRepository::QServersRepository(std::shared_ptr<Settings> settings, QObject *parent)
    : QObject(parent), m_settings(settings)
{
}

void QServersRepository::addServer(const QJsonObject &server)
{
    m_settings->addServer(server);
    emit serverAdded(server);
}

void QServersRepository::editServer(int index, const QJsonObject &server)
{
    m_settings->editServer(index, server);
    emit serverEdited(index, server);
}

void QServersRepository::removeServer(int index)
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
    
    emit serverRemoved(index);
}

QJsonObject QServersRepository::server(int index) const
{
    return m_settings->server(index);
}

QJsonArray QServersRepository::serversArray() const
{
    return m_settings->serversArray();
}

int QServersRepository::serversCount() const
{
    return m_settings->serversCount();
}

int QServersRepository::defaultServerIndex() const
{
    return m_settings->defaultServerIndex();
}

void QServersRepository::setDefaultServer(int index)
{
    m_settings->setDefaultServer(index);
    emit defaultServerChanged(index);
}

void QServersRepository::setDefaultContainer(int serverIndex, DockerContainer container)
{
    m_settings->setDefaultContainer(serverIndex, container);
    QJsonObject server = m_settings->server(serverIndex);
    emit serverEdited(serverIndex, server);
}

QJsonObject QServersRepository::containerConfig(int serverIndex, DockerContainer container) const
{
    return m_settings->containerConfig(serverIndex, container);
}

void QServersRepository::setContainerConfig(int serverIndex, DockerContainer container, const QJsonObject &config)
{
    m_settings->setContainerConfig(serverIndex, container, config);
    QJsonObject server = m_settings->server(serverIndex);
    emit serverEdited(serverIndex, server);
}

void QServersRepository::clearLastConnectionConfig(int serverIndex, DockerContainer container)
{
    m_settings->clearLastConnectionConfig(serverIndex, container);
    QJsonObject server = m_settings->server(serverIndex);
    emit serverEdited(serverIndex, server);
}

ServerCredentials QServersRepository::serverCredentials(int index) const
{
    return m_settings->serverCredentials(index);
}

