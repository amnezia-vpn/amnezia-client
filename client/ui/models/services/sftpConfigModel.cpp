#include "sftpConfigModel.h"

#include "core/utils/protocolEnum.h"
#include "core/protocols/protocolUtils.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"

using namespace amnezia;

SftpConfigModel::SftpConfigModel(QObject *parent) : QObject(parent)
{
}

QString SftpConfigModel::port() const
{
    return m_protocolConfig.port;
}

void SftpConfigModel::setPort(const QString &value)
{
    if (m_protocolConfig.port == value) {
        return;
    }
    m_protocolConfig.port = value;
    emit changed();
}

QString SftpConfigModel::username() const
{
    return m_protocolConfig.userName.isEmpty() ? QString(protocols::sftp::defaultUserName) : m_protocolConfig.userName;
}

void SftpConfigModel::setUsername(const QString &value)
{
    if (m_protocolConfig.userName == value) {
        return;
    }
    m_protocolConfig.userName = value;
    emit changed();
}

QString SftpConfigModel::password() const
{
    return m_protocolConfig.password;
}

void SftpConfigModel::setPassword(const QString &value)
{
    if (m_protocolConfig.password == value) {
        return;
    }
    m_protocolConfig.password = value;
    emit changed();
}

void SftpConfigModel::updateModel(amnezia::DockerContainer container, const amnezia::SftpProtocolConfig &protocolConfig)
{
    m_container = container;
    m_protocolConfig = protocolConfig;
    applyDefaults(m_protocolConfig);
    emit changed();
}

amnezia::SftpProtocolConfig SftpConfigModel::getProtocolConfig()
{
    return m_protocolConfig;
}

void SftpConfigModel::applyDefaults(amnezia::SftpProtocolConfig& config)
{
    if (config.userName.isEmpty()) {
        config.userName = protocols::sftp::defaultUserName;
    }
}
