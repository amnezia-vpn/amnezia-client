#include "apiConfigUIController.h"

#include "core/controllers/api/apiConfigController.h"
#include "settings.h"

ApiConfigUIController::ApiConfigUIController(const QSharedPointer<ServersModel> &serversModel,
                                             const QSharedPointer<ApiServicesModel> &apiServicesModel,
                                             const QSharedPointer<ApiConfigsController> &coreController,
                                             const std::shared_ptr<Settings> &settings,
                                             QObject *parent)
    : QObject(parent), m_serversModel(serversModel), m_apiServicesModel(apiServicesModel), m_coreController(coreController), m_settings(settings)
{
    
}

bool ApiConfigUIController::exportNativeConfig(const QString &serverCountryCode, const QString &fileName)
{
    auto errorCode = m_coreController->exportNativeConfig(serverCountryCode, fileName);
    if (errorCode != amnezia::ErrorCode::NoError) emit errorOccurred(errorCode);
    return errorCode == amnezia::ErrorCode::NoError;
}

bool ApiConfigUIController::revokeNativeConfig(const QString &serverCountryCode)
{
    auto errorCode = m_coreController->revokeNativeConfig(serverCountryCode);
    if (errorCode != amnezia::ErrorCode::NoError && errorCode != amnezia::ErrorCode::ApiNotFoundError) emit errorOccurred(errorCode);
    return errorCode == amnezia::ErrorCode::NoError || errorCode == amnezia::ErrorCode::ApiNotFoundError;
}

void ApiConfigUIController::prepareVpnKeyExport()
{
    m_coreController->prepareVpnKeyExport();
    emit vpnKeyExportReady();
}

void ApiConfigUIController::copyVpnKeyToClipboard()
{
    m_coreController->copyVpnKeyToClipboard();
}

bool ApiConfigUIController::fillAvailableServices()
{
    auto errorCode = m_coreController->fillAvailableServices();
    if (errorCode != amnezia::ErrorCode::NoError) emit errorOccurred(errorCode);
    return errorCode == amnezia::ErrorCode::NoError;
}

bool ApiConfigUIController::importServiceFromGateway()
{
    auto errorCode = m_coreController->importServiceFromGateway();
    if (errorCode == amnezia::ErrorCode::NoError) {
        emit installServerFromApiFinished(tr("%1 installed successfully.").arg(m_apiServicesModel->getSelectedServiceName()));
    } else {
        emit errorOccurred(errorCode);
    }
    return errorCode == amnezia::ErrorCode::NoError;
}

bool ApiConfigUIController::updateServiceFromGateway(const int serverIndex, const QString &newCountryCode, const QString &newCountryName,
                                                     bool reloadServiceConfig)
{
    auto errorCode = m_coreController->updateServiceFromGateway(serverIndex, newCountryCode, newCountryName, reloadServiceConfig);
    if (errorCode == amnezia::ErrorCode::NoError) {
        if (reloadServiceConfig) emit reloadServerFromApiFinished(tr("API config reloaded"));
        else if (newCountryName.isEmpty()) emit updateServerFromApiFinished();
        else emit changeApiCountryFinished(tr("Successfully changed the country of connection to %1").arg(newCountryName));
    } else {
        emit errorOccurred(errorCode);
    }
    return errorCode == amnezia::ErrorCode::NoError;
}

bool ApiConfigUIController::updateServiceFromTelegram(const int serverIndex)
{
    auto errorCode = m_coreController->updateServiceFromTelegram(serverIndex);
    if (errorCode == amnezia::ErrorCode::NoError) emit updateServerFromApiFinished();
    else emit errorOccurred(errorCode);
    return errorCode == amnezia::ErrorCode::NoError;
}

bool ApiConfigUIController::deactivateDevice()
{
    auto errorCode = m_coreController->deactivateDevice();
    if (errorCode != amnezia::ErrorCode::NoError && errorCode != amnezia::ErrorCode::ApiNotFoundError) emit errorOccurred(errorCode);
    return errorCode == amnezia::ErrorCode::NoError || errorCode == amnezia::ErrorCode::ApiNotFoundError;
}

bool ApiConfigUIController::deactivateExternalDevice(const QString &uuid, const QString &serverCountryCode)
{
    auto errorCode = m_coreController->deactivateExternalDevice(uuid, serverCountryCode);
    if (errorCode != amnezia::ErrorCode::NoError && errorCode != amnezia::ErrorCode::ApiNotFoundError) emit errorOccurred(errorCode);
    return errorCode == amnezia::ErrorCode::NoError || errorCode == amnezia::ErrorCode::ApiNotFoundError;
}

bool ApiConfigUIController::isConfigValid()
{
    return m_coreController->isConfigValid();
}

void ApiConfigUIController::setCurrentProtocol(const QString &protocolName)
{
    m_coreController->setCurrentProtocol(protocolName);
}

bool ApiConfigUIController::isVlessProtocol()
{
    return m_coreController->isVlessProtocol();
}

QList<QString> ApiConfigUIController::getQrCodes()
{
    return m_coreController->getQrCodes();
}

int ApiConfigUIController::getQrCodesCount()
{
    return m_coreController->getQrCodesCount();
}

QString ApiConfigUIController::getVpnKey()
{
    return m_coreController->getVpnKey();
}


