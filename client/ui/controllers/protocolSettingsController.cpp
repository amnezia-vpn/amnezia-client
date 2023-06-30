#include "protocolSettingsController.h"

ProtocolSettingsController::ProtocolSettingsController(
    const QSharedPointer<ServersModel> &serversModel,
    const QSharedPointer<ContainersModel> &containersModel,
    const std::shared_ptr<Settings> &settings,
    QObject *parent)
    : QObject(parent)
    , m_serversModel(serversModel)
    , m_containersModel(containersModel)
    , m_settings(settings)
{}

QByteArray ProtocolSettingsController::getOpenVpnConfig()
{
    auto containerIndex = m_containersModel->index(
        m_containersModel->getCurrentlyProcessedContainerIndex());
    auto config = m_containersModel->data(containerIndex, ContainersModel::Roles::ConfigRole);
    return QByteArray();
}
