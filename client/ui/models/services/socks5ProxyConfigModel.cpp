#include "socks5ProxyConfigModel.h"

#include "core/utils/protocolEnum.h"
#include "core/protocols/protocolUtils.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"

using namespace amnezia;

Socks5ProxyConfigModel::Socks5ProxyConfigModel(QObject *parent) : QObject(parent)
{
}

QString Socks5ProxyConfigModel::port() const
{
    return m_protocolConfig.port;
}

void Socks5ProxyConfigModel::setPort(const QString &value)
{
    if (m_protocolConfig.port == value) {
        return;
    }
    m_protocolConfig.port = value;
    emit changed();
}

QString Socks5ProxyConfigModel::username() const
{
    return m_protocolConfig.userName;
}

void Socks5ProxyConfigModel::setUsername(const QString &value)
{
    if (m_protocolConfig.userName == value) {
        return;
    }
    m_protocolConfig.userName = value;
    emit changed();
}

QString Socks5ProxyConfigModel::password() const
{
    return m_protocolConfig.password;
}

void Socks5ProxyConfigModel::setPassword(const QString &value)
{
    if (m_protocolConfig.password == value) {
        return;
    }
    m_protocolConfig.password = value;
    emit changed();
}

void Socks5ProxyConfigModel::updateModel(amnezia::DockerContainer container, const amnezia::Socks5ProxyProtocolConfig &protocolConfig)
{
    m_container = container;
    m_protocolConfig = protocolConfig;
    emit changed();
}

amnezia::Socks5ProxyProtocolConfig Socks5ProxyConfigModel::getProtocolConfig()
{
    return m_protocolConfig;
}
