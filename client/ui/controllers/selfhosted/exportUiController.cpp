#include "exportUiController.h"

#include "../systemController.h"

ExportUiController::ExportUiController(ExportController* exportController, QObject *parent)
    : QObject(parent),
      m_exportController(exportController)
{
}

void ExportUiController::generateFullAccessConfig(int serverIndex)
{
    clearPreviousConfig();
    auto result = m_exportController->generateFullAccessConfig(serverIndex);
    applyExportResult(result);
}

void ExportUiController::generateConnectionConfig(int serverIndex, int containerIndex, const QString &clientName)
{
    clearPreviousConfig();
    auto result = m_exportController->generateConnectionConfig(serverIndex, containerIndex, clientName);
    applyExportResult(result);
}

void ExportUiController::generateOpenVpnConfig(int serverIndex, const QString &clientName)
{
    clearPreviousConfig();
    auto result = m_exportController->generateOpenVpnConfig(serverIndex, clientName);
    applyExportResult(result);
}

void ExportUiController::generateWireGuardConfig(int serverIndex, const QString &clientName)
{
    clearPreviousConfig();
    auto result = m_exportController->generateWireGuardConfig(serverIndex, clientName);
    applyExportResult(result);
}

void ExportUiController::generateAwgConfig(int serverIndex, int containerIndex, const QString &clientName)
{
    clearPreviousConfig();
    auto result = m_exportController->generateAwgConfig(serverIndex, containerIndex, clientName);
    applyExportResult(result);
}


void ExportUiController::generateXrayConfig(int serverIndex, const QString &clientName)
{
    clearPreviousConfig();
    auto result = m_exportController->generateXrayConfig(serverIndex, clientName);
    applyExportResult(result);
}

QString ExportUiController::getConfig()
{
    return m_config;
}

QString ExportUiController::getNativeConfigString()
{
    return m_nativeConfigString;
}

QList<QString> ExportUiController::getQrCodes()
{
    return m_qrCodes;
}

void ExportUiController::exportConfig(const QString &fileName)
{
    SystemController::saveFile(fileName, m_config);
}

void ExportUiController::updateClientManagementModel(int serverIndex, int containerIndex)
{
    m_exportController->updateClientManagementModel(serverIndex, containerIndex);
}

void ExportUiController::revokeConfig(int row, int serverIndex, int containerIndex)
{
    m_exportController->revokeConfig(row, serverIndex, containerIndex);
    emit revokeConfigFinished();
}

void ExportUiController::renameClient(int row, const QString &clientName, int serverIndex, int containerIndex)
{
    m_exportController->renameClient(row, clientName, serverIndex, containerIndex);
}

int ExportUiController::getQrCodesCount()
{
    return m_qrCodes.size();
}

void ExportUiController::clearPreviousConfig()
{
    m_config.clear();
    m_nativeConfigString.clear();
    m_qrCodes.clear();

    emit exportConfigChanged();
}

void ExportUiController::applyExportResult(const ExportController::ExportResult &result)
{
    if (result.errorCode != ErrorCode::NoError) {
        emit exportErrorOccurred(result.errorCode);
        return;
    }

    m_config = result.config;
    m_nativeConfigString = result.nativeConfigString;
    m_qrCodes = result.qrCodes;

    emit exportConfigChanged();
}
