#include "torConfigModel.h"

#include "core/utils/protocolEnum.h"
#include "core/protocols/protocolUtils.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"
#include "core/models/containerConfig.h"
#include "core/models/protocols/torProtocolConfig.h"

using namespace amnezia;
using namespace ProtocolUtils;

TorConfigModel::TorConfigModel(QObject *parent) : QObject(parent)
{
}

QString TorConfigModel::site() const
{
    return m_protocolConfig.serverConfig.site;
}

void TorConfigModel::setSite(const QString &value)
{
    if (m_protocolConfig.serverConfig.site == value) {
        return;
    }
    m_protocolConfig.serverConfig.site = value;
    emit changed();
}

void TorConfigModel::updateModel(amnezia::DockerContainer container, const amnezia::TorProtocolConfig &protocolConfig)
{
    m_container = container;
    m_protocolConfig = protocolConfig;
    m_originalProtocolConfig = m_protocolConfig;
    
    emit changed();
}

amnezia::TorProtocolConfig TorConfigModel::getProtocolConfig()
{
    return m_protocolConfig;
}
