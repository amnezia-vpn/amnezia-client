#include <QSettings>

#include "defines.h"
#include "settings.h"

Settings::Settings(QObject* parent) : QObject(parent)
{
    m_settings = new QSettings(ORGANIZATION_NAME, APPLICATION_NAME, this);
    read();
}

void Settings::read()
{
    m_settings->beginGroup("Server");
    m_login = m_settings->value("login", QString()).toString();
    m_password = m_settings->value("password", QString()).toString();
    m_serverName = m_settings->value("serverName", QString()).toString();
    m_settings->endGroup();
}


void Settings::save()
{
    m_settings->beginGroup("Server");
    m_settings->setValue("login", m_login);
    m_settings->setValue("password", m_password);
    m_settings->setValue("serverName", m_serverName);
    m_settings->endGroup();
}

bool Settings::haveAuthData() const
{
    return (!serverName().isEmpty() && !login().isEmpty() && !password().isEmpty());
}

void Settings::setLogin(const QString& login)
{
    m_login = login;
}

void Settings::setPassword(const QString& password)
{
    m_password = password;
}

void Settings::setServerName(const QString& serverName)
{
    m_serverName = serverName;
}

QString Settings::login() const
{
    return m_login;
}

QString Settings::password() const
{
    return m_password;
}

QString Settings::serverName() const
{
    return m_serverName;
}
