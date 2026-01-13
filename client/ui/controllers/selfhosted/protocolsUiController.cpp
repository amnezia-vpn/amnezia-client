#include "protocolsUiController.h"

ProtocolsUiController::ProtocolsUiController(ProtocolsModel* protocolsModel, QObject *parent)
    : QObject(parent),
      m_protocolsModel(protocolsModel)
{
}

void ProtocolsUiController::updateProtocols(const QJsonObject &config)
{
    m_protocolsModel->updateModel(config);
}

QJsonObject ProtocolsUiController::getProtocolsConfig()
{
    return m_protocolsModel->getConfig();
}

