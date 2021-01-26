
#include "defines.h"
#include "settings.h"

Settings::Settings(QObject* parent) :
    QObject(parent),
    m_settings (ORGANIZATION_NAME, APPLICATION_NAME, this)
{
    read();
}

void Settings::read()
{
    m_settings.beginGroup("Server");
    m_userName = m_settings.value("userName", QString()).toString();
    m_password = m_settings.value("password", QString()).toString();
    m_serverName = m_settings.value("serverName", QString()).toString();
    m_serverPort = m_settings.value("serverPort", 22).toInt();
    m_settings.endGroup();
}

void Settings::save()
{
    m_settings.beginGroup("Server");
    m_settings.setValue("userName", m_userName);
    m_settings.setValue("password", m_password);
    m_settings.setValue("serverName", m_serverName);
    m_settings.setValue("serverPort", m_serverPort);
    m_settings.endGroup();
}

bool Settings::haveAuthData() const
{
    return (!serverName().isEmpty() && !userName().isEmpty() && !password().isEmpty());
}

void Settings::setUserName(const QString& login)
{
    m_userName = login;
}

void Settings::setPassword(const QString& password)
{
    m_password = password;
}

void Settings::setServerName(const QString& serverName)
{
    m_serverName = serverName;
}

void Settings::setServerPort(int serverPort)
{
    m_serverPort = serverPort;
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
