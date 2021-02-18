
#include "defines.h"
#include "settings.h"

Settings::Settings(QObject* parent) :
    QObject(parent),
    m_settings (ORGANIZATION_NAME, APPLICATION_NAME, this)
{
}

bool Settings::haveAuthData() const
{
    return (!serverName().isEmpty() && !userName().isEmpty() && !password().isEmpty());
}

void Settings::setServerCredentials(const ServerCredentials &credentials)
{
    setServerName(credentials.hostName);
    setServerPort(credentials.port);
    setUserName(credentials.userName);
    setPassword(credentials.password);
}

ServerCredentials Settings::serverCredentials()
{
    ServerCredentials credentials;
    credentials.hostName = serverName();
    credentials.userName = userName();
    credentials.password = password();
    credentials.port = serverPort();

    return credentials;
}
