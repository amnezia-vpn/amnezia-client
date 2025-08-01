#include "exportUIController.h"

#include <QBuffer>
#include <QDataStream>
#include <QDesktopServices>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QStandardPaths>

#include "core/controllers/vpnConfigurationController.h"
#include "core/controllers/selfhosted/exportController.h"
#include "core/qrCodeUtils.h"
#include "systemController.h"

ExportUIController::ExportUIController(const QSharedPointer<ServersModel> &serversModel, const QSharedPointer<ContainersModel> &containersModel,
                                      const QSharedPointer<ClientManagementModel> &clientManagementModel,
                                      QSharedPointer<ExportController> coreExportController,
                                      QSharedPointer<ClientManagementController> clientManagementController,
                                      QObject *parent)
    : QObject(parent),
      m_serversModel(serversModel),
      m_containersModel(containersModel),
      m_clientManagementModel(clientManagementModel),
      m_coreExportController(coreExportController),
      m_clientManagementController(clientManagementController)
{
}

void ExportUIController::generateFullAccessConfig()
{
    clearPreviousConfig();

    int serverIndex = m_serversModel->getProcessedServerIndex();
    QJsonObject serverConfig = m_serversModel->getServerConfig(serverIndex);

    auto result = m_coreExportController->generateFullAccessConfig(serverConfig);
    
    if (result.errorCode != ErrorCode::NoError) {
        emit exportErrorOccurred(result.errorCode);
        return;
    }

    m_config = result.config;
    m_nativeConfigString = result.nativeConfigString;
    m_qrCodes = result.qrCodes;
    
    emit exportConfigChanged();
}

void ExportUIController::generateConnectionConfig(const QString &clientName)
{
    clearPreviousConfig();

    int serverIndex = m_serversModel->getProcessedServerIndex();
    ServerCredentials credentials = m_serversModel->getServerCredentials(serverIndex);
    DockerContainer container = static_cast<DockerContainer>(m_containersModel->getProcessedContainerIndex());
    QJsonObject containerConfig = m_containersModel->getContainerConfig(container);
    QJsonObject serverConfig = m_serversModel->getServerConfig(serverIndex);
    auto dnsSettings = m_serversModel->getDnsPair(serverIndex);

    auto result = m_coreExportController->generateConnectionConfig(clientName, credentials, container, 
                                                                   containerConfig, serverConfig, dnsSettings);
    
    if (result.errorCode != ErrorCode::NoError) {
        emit exportErrorOccurred(result.errorCode);
        return;
    }

    m_config = result.config;
    m_nativeConfigString = result.nativeConfigString;
    m_qrCodes = result.qrCodes;
    
    emit exportConfigChanged();
}



void ExportUIController::generateOpenVpnConfig(const QString &clientName)
{
    clearPreviousConfig();

    int serverIndex = m_serversModel->getProcessedServerIndex();
    ServerCredentials credentials = m_serversModel->getServerCredentials(serverIndex);
    DockerContainer container = static_cast<DockerContainer>(m_containersModel->getProcessedContainerIndex());
    QJsonObject containerConfig = m_containersModel->getContainerConfig(container);
    auto dnsSettings = m_serversModel->getDnsPair(serverIndex);
    bool isApiConfig = qvariant_cast<bool>(m_serversModel->data(serverIndex, ServersModel::IsServerFromTelegramApiRole));

    auto result = m_coreExportController->generateOpenVpnConfig(clientName, credentials, container, 
                                                               containerConfig, dnsSettings, isApiConfig);
    
    if (result.errorCode != ErrorCode::NoError) {
        emit exportErrorOccurred(result.errorCode);
        return;
    }

    m_config = result.config;
    m_nativeConfigString = result.nativeConfigString;
    m_qrCodes = result.qrCodes;
    
    emit exportConfigChanged();
}

void ExportUIController::generateWireGuardConfig(const QString &clientName)
{
    clearPreviousConfig();

    int serverIndex = m_serversModel->getProcessedServerIndex();
    ServerCredentials credentials = m_serversModel->getServerCredentials(serverIndex);
    QJsonObject containerConfig = m_containersModel->getContainerConfig(DockerContainer::WireGuard);
    auto dnsSettings = m_serversModel->getDnsPair(serverIndex);
    bool isApiConfig = qvariant_cast<bool>(m_serversModel->data(serverIndex, ServersModel::IsServerFromTelegramApiRole));

    auto result = m_coreExportController->generateWireGuardConfig(clientName, credentials, 
                                                                 containerConfig, dnsSettings, isApiConfig);
    
    if (result.errorCode != ErrorCode::NoError) {
        emit exportErrorOccurred(result.errorCode);
        return;
    }

    m_config = result.config;
    m_nativeConfigString = result.nativeConfigString;
    m_qrCodes = result.qrCodes;
    
    emit exportConfigChanged();
}

void ExportUIController::generateAwgConfig(const QString &clientName)
{
    clearPreviousConfig();

    int serverIndex = m_serversModel->getProcessedServerIndex();
    ServerCredentials credentials = m_serversModel->getServerCredentials(serverIndex);
    QJsonObject containerConfig = m_containersModel->getContainerConfig(DockerContainer::Awg);
    auto dnsSettings = m_serversModel->getDnsPair(serverIndex);
    bool isApiConfig = qvariant_cast<bool>(m_serversModel->data(serverIndex, ServersModel::IsServerFromTelegramApiRole));

    auto result = m_coreExportController->generateAwgConfig(clientName, credentials, 
                                                           containerConfig, dnsSettings, isApiConfig);
    
    if (result.errorCode != ErrorCode::NoError) {
        emit exportErrorOccurred(result.errorCode);
        return;
    }

    m_config = result.config;
    m_nativeConfigString = result.nativeConfigString;
    m_qrCodes = result.qrCodes;
    
    emit exportConfigChanged();
}

void ExportUIController::generateShadowSocksConfig()
{
    clearPreviousConfig();

    int serverIndex = m_serversModel->getProcessedServerIndex();
    ServerCredentials credentials = m_serversModel->getServerCredentials(serverIndex);
    DockerContainer container = static_cast<DockerContainer>(m_containersModel->getProcessedContainerIndex());
    QJsonObject containerConfig = m_containersModel->getContainerConfig(container);
    auto dnsSettings = m_serversModel->getDnsPair(serverIndex);
    bool isApiConfig = qvariant_cast<bool>(m_serversModel->data(serverIndex, ServersModel::IsServerFromTelegramApiRole));

    auto result = m_coreExportController->generateShadowSocksConfig(credentials, container, 
                                                                   containerConfig, dnsSettings, isApiConfig);
    
    if (result.errorCode != ErrorCode::NoError) {
        emit exportErrorOccurred(result.errorCode);
        return;
    }

    m_config = result.config;
    m_nativeConfigString = result.nativeConfigString;
    m_qrCodes = result.qrCodes;
    
    emit exportConfigChanged();
}

void ExportUIController::generateCloakConfig()
{
    clearPreviousConfig();

    int serverIndex = m_serversModel->getProcessedServerIndex();
    ServerCredentials credentials = m_serversModel->getServerCredentials(serverIndex);
    QJsonObject containerConfig = m_containersModel->getContainerConfig(DockerContainer::Cloak);
    auto dnsSettings = m_serversModel->getDnsPair(serverIndex);
    bool isApiConfig = qvariant_cast<bool>(m_serversModel->data(serverIndex, ServersModel::IsServerFromTelegramApiRole));

    auto result = m_coreExportController->generateCloakConfig(credentials, containerConfig, dnsSettings, isApiConfig);
    
    if (result.errorCode != ErrorCode::NoError) {
        emit exportErrorOccurred(result.errorCode);
        return;
    }

    m_config = result.config;
    m_nativeConfigString = result.nativeConfigString;
    m_qrCodes = result.qrCodes;
    
    emit exportConfigChanged();
}

void ExportUIController::generateXrayConfig(const QString &clientName)
{
    clearPreviousConfig();

    int serverIndex = m_serversModel->getProcessedServerIndex();
    ServerCredentials credentials = m_serversModel->getServerCredentials(serverIndex);
    QJsonObject containerConfig = m_containersModel->getContainerConfig(DockerContainer::Xray);
    auto dnsSettings = m_serversModel->getDnsPair(serverIndex);
    bool isApiConfig = qvariant_cast<bool>(m_serversModel->data(serverIndex, ServersModel::IsServerFromTelegramApiRole));

    auto result = m_coreExportController->generateXrayConfig(clientName, credentials, 
                                                            containerConfig, dnsSettings, isApiConfig);
    
    if (result.errorCode != ErrorCode::NoError) {
        emit exportErrorOccurred(result.errorCode);
        return;
    }

    m_config = result.config;
    m_nativeConfigString = result.nativeConfigString;
    m_qrCodes = result.qrCodes;
    
    emit exportConfigChanged();
}

QString ExportUIController::getConfig()
{
    return m_config;
}

QString ExportUIController::getNativeConfigString()
{
    return m_nativeConfigString;
}

QList<QString> ExportUIController::getQrCodes()
{
    return m_qrCodes;
}

void ExportUIController::exportConfig(const QString &fileName)
{
    SystemController::saveFile(fileName, m_config);
}

void ExportUIController::revokeConfig(const int row, const DockerContainer container, ServerCredentials credentials)
{
    ErrorCode errorCode = m_clientManagementController->revokeClient(row, container, credentials, 
                                                                    m_serversModel->getProcessedServerIndex());
    if (errorCode != ErrorCode::NoError) {
        emit exportErrorOccurred(errorCode);
    }
}

void ExportUIController::renameClient(const int row, const QString &clientName, const DockerContainer container, ServerCredentials credentials)
{
    ErrorCode errorCode = m_clientManagementController->renameClient(row, clientName, container, credentials);
    if (errorCode != ErrorCode::NoError) {
        emit exportErrorOccurred(errorCode);
    }
}

int ExportUIController::getQrCodesCount()
{
    return m_qrCodes.size();
}

void ExportUIController::clearPreviousConfig()
{
    m_config.clear();
    m_nativeConfigString.clear();
    m_qrCodes.clear();

    emit exportConfigChanged();
}
