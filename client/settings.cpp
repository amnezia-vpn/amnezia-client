#include "defines.h"
#include "settings.h"

#include <QDebug>

Settings::Settings(QObject* parent) :
    QObject(parent),
    m_settings (ORGANIZATION_NAME, APPLICATION_NAME, this)
{
    // Import old settings
    if (serversCount() == 0) {
        QString user = m_settings.value("Server/userName").toString();
        QString password = m_settings.value("Server/password").toString();
        QString serverName = m_settings.value("Server/serverName").toString();
        int port = m_settings.value("Server/serverPort").toInt();

        if (!user.isEmpty() && !password.isEmpty() && !serverName.isEmpty()){
            QJsonObject server;
            server.insert(userNameString, user);
            server.insert(passwordString, password);
            server.insert(hostNameString, serverName);
            server.insert(portString, port);
            server.insert(descriptionString, tr("Server #1"));

            addServer(server);
        }
    }
}

int Settings::serversCount() const
{
    return serversArray().size();
}

QJsonObject Settings::server(int index) const
{
    const QJsonArray &servers = serversArray();
    if (index >= servers.size()) return QJsonObject();

    return servers.at(index).toObject();
}

void Settings::addServer(const QJsonObject &server)
{
    QJsonArray servers = serversArray();
    servers.append(server);
    setServersArray(servers);
}

void Settings::removeServer(int index)
{
    QJsonArray servers = serversArray();
    if (index >= servers.size()) return;

    servers.removeAt(index);
    setServersArray(servers);
}

bool Settings::editServer(int index, const QJsonObject &server)
{
    QJsonArray servers = serversArray();
    if (index >= servers.size()) return false;

    servers.replace(index, server);
    setServersArray(servers);
    return true;
}

void Settings::setDefaultContainer(int serverIndex, DockerContainer container)
{
    QJsonObject s = server(serverIndex);
    s.insert(defaultContainerString, containerToString(container));
    editServer(serverIndex, s);
}

DockerContainer Settings::defaultContainer(int serverIndex) const
{
    return containerFromString(defaultContainerName(serverIndex));
}

QString Settings::defaultContainerName(int serverIndex) const
{
    QString name = server(serverIndex).value(defaultContainerString).toString();
    if (name.isEmpty()) {
        return containerToString(DockerContainer::OpenVpnOverCloak);
    }
    else return name;
}

bool Settings::haveAuthData() const
{
    ServerCredentials cred = defaultServerCredentials();

    return (!cred.hostName.isEmpty() && !cred.userName.isEmpty() && !cred.password.isEmpty());
}

//void Settings::setServerCredentials(const ServerCredentials &credentials)
//{
//    setServerName(credentials.hostName);
//    setServerPort(credentials.port);
//    setUserName(credentials.userName);
//    setPassword(credentials.password);
//}

ServerCredentials Settings::defaultServerCredentials() const
{
    const QJsonObject &s = defaultServer();

    ServerCredentials credentials;
    credentials.hostName = s.value(hostNameString).toString();
    credentials.userName = s.value(userNameString).toString();
    credentials.password = s.value(passwordString).toString();
    credentials.port = s.value(portString).toInt();

    return credentials;
}
